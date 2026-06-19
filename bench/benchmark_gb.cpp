// Google Benchmark suite. Built only when configured with -DBUILD_GBENCH=ON
// (it pulls Google Benchmark via FetchContent). Unlike the hand-rolled
// `bench` executable, this gives proper warmup, statistics across repetitions,
// and a throughput rate, and uses benchmark::DoNotOptimize to stop the
// compiler eliding the work.
//
// Run with statistics, e.g.:
//   ./build/bench_gb --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
#include <benchmark/benchmark.h>

#include <random>
#include <vector>

#include "ope/black_scholes.hpp"
#include "ope/implied_vol.hpp"

namespace {

// A fixed book of varied options so we are not pricing one constant input.
std::vector<ope::BSInputs> make_book(int m) {
    std::mt19937_64 rng(12345);
    std::uniform_real_distribution<double> uS(50, 150), uK(50, 150),
        uVol(0.05, 0.60), uT(0.05, 2.0);
    std::bernoulli_distribution coin(0.5);
    std::vector<ope::BSInputs> book;
    book.reserve(m);
    for (int i = 0; i < m; ++i) {
        book.push_back(ope::BSInputs{uS(rng), uK(rng), 0.03, 0.0, uVol(rng),
                                     uT(rng),
                                     coin(rng) ? ope::OptionType::Call
                                               : ope::OptionType::Put});
    }
    return book;
}

const std::vector<ope::BSInputs> g_book = make_book(4096);

// Market prices for the implied-vol benchmark (recovering the input vol).
const std::vector<double> g_mkt = [] {
    std::vector<double> m;
    m.reserve(g_book.size());
    for (const auto& in : g_book) m.push_back(ope::black_scholes(in).price);
    return m;
}();

}  // namespace

static void BM_BlackScholesGreeks(benchmark::State& state) {
    std::size_t i = 0;
    for (auto _ : state) {
        ope::Greeks g = ope::black_scholes(g_book[i++ % g_book.size()]);
        benchmark::DoNotOptimize(g);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BlackScholesGreeks);

static void BM_ImpliedVol(benchmark::State& state) {
    std::size_t i = 0;
    for (auto _ : state) {
        const std::size_t k = i++ % g_book.size();
        ope::IVResult r = ope::implied_vol(g_book[k], g_mkt[k]);
        benchmark::DoNotOptimize(r);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ImpliedVol);

BENCHMARK_MAIN();
