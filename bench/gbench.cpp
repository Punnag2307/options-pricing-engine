// Google Benchmark micro-benchmarks. The standard harness handles warmup,
// runs each case until the timing is statistically stable, prevents the
// compiler from optimising the work away (benchmark::DoNotOptimize), and
// reports proper statistics. Built only when -DOPE_BUILD_GBENCH=ON.
//
// Run:  ./build/gbench
//       ./build/gbench --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
#include <benchmark/benchmark.h>

#include <random>
#include <vector>

#include "ope/binomial.hpp"
#include "ope/black_scholes.hpp"
#include "ope/implied_vol.hpp"
#include "ope/monte_carlo.hpp"

using namespace ope;

namespace {
// A book of 4096 varied options (power of two so we can index with a mask).
std::vector<BSInputs> make_book() {
    std::mt19937_64 rng(12345);
    std::uniform_real_distribution<double> uS(50, 150), uK(50, 150),
        uVol(0.05, 0.60), uT(0.05, 2.0);
    std::bernoulli_distribution coin(0.5);
    std::vector<BSInputs> book;
    book.reserve(4096);
    for (int i = 0; i < 4096; ++i) {
        book.push_back(BSInputs{uS(rng), uK(rng), 0.03, 0.0, uVol(rng), uT(rng),
                                coin(rng) ? OptionType::Call : OptionType::Put});
    }
    return book;
}
}  // namespace

static void BM_BlackScholesGreeks(benchmark::State& state) {
    const auto book = make_book();
    std::size_t i = 0;
    for (auto _ : state) {
        const Greeks g = black_scholes(book[i++ & 4095]);
        benchmark::DoNotOptimize(g);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BlackScholesGreeks);

static void BM_ImpliedVol(benchmark::State& state) {
    const auto book = make_book();
    std::vector<double> mkt(book.size());
    for (std::size_t k = 0; k < book.size(); ++k) mkt[k] = black_scholes(book[k]).price;
    std::size_t i = 0;
    for (auto _ : state) {
        const std::size_t idx = i++ & 4095;
        const IVResult r = implied_vol(book[idx], mkt[idx]);
        benchmark::DoNotOptimize(r);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ImpliedVol);

static void BM_BinomialAmerican(benchmark::State& state) {
    const BSInputs in{100, 100, 0.05, 0.0, 0.20, 1.0, OptionType::Put};
    const int steps = static_cast<int>(state.range(0));
    for (auto _ : state) {
        const double p = binomial_price(in, steps, Exercise::American);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(BM_BinomialAmerican)->Arg(100)->Arg(500)->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

static void BM_MonteCarlo(benchmark::State& state) {
    const BSInputs in{100, 100, 0.05, 0.0, 0.20, 1.0, OptionType::Call};
    const long paths = static_cast<long>(state.range(0));
    for (auto _ : state) {
        const MCResult r = monte_carlo_price(in, paths, true, 1);
        benchmark::DoNotOptimize(r);
    }
    state.SetItemsProcessed(state.iterations() * paths);
}
BENCHMARK(BM_MonteCarlo)->Arg(100'000)->Arg(1'000'000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
