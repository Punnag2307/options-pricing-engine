// Latency benchmark for the three pricers. Build in Release (-O3) and run the
// executable directly. Numbers are amortized, single-thread, hot-cache, and
// measured over a *book of varied options* (not one constant input) so the
// branch predictor and caches are not unrealistically favourable.
//
// Caveats worth stating when quoting these:
//   * This is the latency of the C++ core. Calling through the Python binding
//     adds the pybind11 boundary cost (orders of magnitude larger per call);
//     amortize by batching.
//   * At ~10s of ns per call, timing each call individually is impossible: a
//     single clock read costs as much as the operation. So we time a large
//     batch and divide -- this yields a mean, not percentiles.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <initializer_list>
#include <random>
#include <vector>

#include "ope/binomial.hpp"
#include "ope/black_scholes.hpp"
#include "ope/implied_vol.hpp"
#include "ope/monte_carlo.hpp"

using clk = std::chrono::high_resolution_clock;

int main() {
    using namespace ope;

    // A book of varied options: random spot/strike/vol/maturity, mixed
    // call/put. Pricing across these defeats "you only timed one option".
    std::mt19937_64 rng(12345);
    std::uniform_real_distribution<double> uS(50, 150), uK(50, 150),
        uVol(0.05, 0.60), uT(0.05, 2.0);
    std::bernoulli_distribution coin(0.5);

    const int M = 4096;
    std::vector<BSInputs> book;
    book.reserve(M);
    for (int i = 0; i < M; ++i) {
        book.push_back(BSInputs{uS(rng), uK(rng), 0.03, 0.0, uVol(rng), uT(rng),
                                coin(rng) ? OptionType::Call : OptionType::Put});
    }

    std::printf("Benchmark (single thread, %d varied options)\n", M);
    std::printf("--------------------------------------------\n");

    // Black-Scholes price + all five Greeks.
    {
        const int reps = 3000;
        volatile double sink = 0.0;
        const auto t0 = clk::now();
        for (int r = 0; r < reps; ++r)
            for (const auto& in : book) {
                const Greeks g = black_scholes(in);
                sink += g.price + g.delta + g.gamma + g.vega + g.theta + g.rho;
            }
        const auto t1 = clk::now();
        const long calls = static_cast<long>(M) * reps;
        const double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / calls;
        std::printf("Black-Scholes price + 5 Greeks : %8.1f ns/call  (%6.1f M/sec)\n",
                    ns, 1e3 / ns);
        (void)sink;
    }

    // Implied volatility: precompute each option's market price, then solve.
    {
        std::vector<double> mkt(M);
        for (int i = 0; i < M; ++i) mkt[i] = black_scholes(book[i]).price;
        const int reps = 400;
        volatile double sink = 0.0;
        const auto t0 = clk::now();
        for (int r = 0; r < reps; ++r)
            for (int i = 0; i < M; ++i) sink += implied_vol(book[i], mkt[i]).vol;
        const auto t1 = clk::now();
        const long calls = static_cast<long>(M) * reps;
        const double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / calls;
        std::printf("Implied volatility solve       : %8.1f ns/call  (%6.1f M/sec)\n",
                    ns, 1e3 / ns);
        (void)sink;
    }

    // Binomial tree (American), varied options, a few step counts.
    for (int steps : {100, 500, 1000}) {
        volatile double sink = 0.0;
        const auto t0 = clk::now();
        for (const auto& in : book) sink += binomial_price(in, steps, Exercise::American);
        const auto t1 = clk::now();
        const double us = std::chrono::duration<double, std::micro>(t1 - t0).count() / M;
        std::printf("Binomial tree (American, %4d)  : %8.2f us/call\n", steps, us);
        (void)sink;
    }

    // Monte Carlo on a single representative option, two path counts.
    const BSInputs atm{100, 100, 0.05, 0.0, 0.20, 1.0, OptionType::Call};
    for (long paths : {100'000L, 1'000'000L}) {
        const auto t0 = clk::now();
        const MCResult r = monte_carlo_price(atm, paths, true, 1);
        const auto t1 = clk::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::printf("Monte Carlo (%8ld paths)   : %8.2f ms     -> %.4f +/- %.4f\n",
                    paths, ms, r.price, r.std_error);
    }
    return 0;
}
