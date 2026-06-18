// Latency benchmark for the three pricers. Build in Release (-O3) for
// meaningful numbers, then run the executable directly. Results vary by
// machine; this backs the "fast pricing" claim with a real figure.
#include <chrono>
#include <cstdio>
#include <initializer_list>

#include "ope/binomial.hpp"
#include "ope/black_scholes.hpp"
#include "ope/implied_vol.hpp"
#include "ope/monte_carlo.hpp"

using clk = std::chrono::high_resolution_clock;

int main() {
    using namespace ope;
    const BSInputs in{100, 100, 0.05, 0.0, 0.20, 1.0, OptionType::Call};

    std::printf("Benchmark (single thread)\n");
    std::printf("-------------------------\n");

    // Black-Scholes: price + all five Greeks. `volatile` sink stops the
    // optimiser from deleting the loop.
    {
        const int N = 5'000'000;
        volatile double sink = 0.0;
        const auto t0 = clk::now();
        for (int i = 0; i < N; ++i) {
            const Greeks g = black_scholes(in);
            sink += g.price + g.delta + g.gamma + g.vega + g.theta + g.rho;
        }
        const auto t1 = clk::now();
        const double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
        std::printf("Black-Scholes price + 5 Greeks : %8.1f ns/call  (%6.1f M/sec)\n",
                    ns, 1e3 / ns);
        (void)sink;
    }

    // Implied volatility (Newton + bisection safeguard).
    {
        const double mkt = black_scholes(in).price;  // recover 20% vol
        const int N = 1'000'000;
        volatile double sink = 0.0;
        const auto t0 = clk::now();
        for (int i = 0; i < N; ++i) sink += implied_vol(in, mkt).vol;
        const auto t1 = clk::now();
        const double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
        std::printf("Implied volatility solve       : %8.1f ns/call  (%6.1f M/sec)\n",
                    ns, 1e3 / ns);
        (void)sink;
    }

    // Binomial tree (American), a few step counts.
    for (int steps : {100, 500, 1000}) {
        const int N = 2000;
        volatile double sink = 0.0;
        const auto t0 = clk::now();
        for (int i = 0; i < N; ++i) sink += binomial_price(in, steps, Exercise::American);
        const auto t1 = clk::now();
        const double us = std::chrono::duration<double, std::micro>(t1 - t0).count() / N;
        std::printf("Binomial tree (American, %4d)  : %8.2f us/call\n", steps, us);
        (void)sink;
    }

    // Monte Carlo, two path counts.
    for (long paths : {100'000L, 1'000'000L}) {
        const auto t0 = clk::now();
        const MCResult r = monte_carlo_price(in, paths, true, 1);
        const auto t1 = clk::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::printf("Monte Carlo (%8ld paths)   : %8.2f ms     -> %.4f +/- %.4f\n",
                    paths, ms, r.price, r.std_error);
    }

    return 0;
}
