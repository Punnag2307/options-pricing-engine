// Verifies the three pricers agree, and that the tree's early-exercise logic
// reproduces well-known American-option properties.
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "ope/binomial.hpp"
#include "ope/black_scholes.hpp"
#include "ope/monte_carlo.hpp"

static int g_failures = 0;

static void check(const char* name, bool ok, const char* detail) {
    if (ok) {
        std::printf("  ok    %-34s %s\n", name, detail);
    } else {
        std::printf("  FAIL  %-34s %s\n", name, detail);
        ++g_failures;
    }
}

int main() {
    using namespace ope;
    char buf[256];

    // Reference European call: S=K=100, r=5%, q=0, vol=20%, T=1.
    BSInputs call{100, 100, 0.05, 0.0, 0.20, 1.0, OptionType::Call};
    const double bs = black_scholes(call).price;

    // 1) Binomial tree -> Black-Scholes as steps grow.
    {
        const double tree = binomial_price(call, 2000, Exercise::European);
        const double err = std::fabs(tree - bs);
        std::snprintf(buf, sizeof(buf), "tree=%.5f bs=%.5f err=%.2e", tree, bs, err);
        check("Tree converges to Black-Scholes", err < 1e-2, buf);
    }

    // 2) Monte Carlo -> within a few standard errors of Black-Scholes.
    {
        MCResult mc = monte_carlo_price(call, 2'000'000, /*antithetic=*/true);
        const double err = std::fabs(mc.price - bs);
        std::snprintf(buf, sizeof(buf), "mc=%.5f bs=%.5f err=%.4f se=%.4f",
                      mc.price, bs, err, mc.std_error);
        check("MC within 3 standard errors of BS", err < 3.0 * mc.std_error, buf);
    }

    // 3) Antithetic variates reduce the standard error vs plain sampling.
    {
        MCResult plain = monte_carlo_price(call, 500'000, /*antithetic=*/false, 7);
        MCResult anti  = monte_carlo_price(call, 500'000, /*antithetic=*/true, 7);
        std::snprintf(buf, sizeof(buf), "plain se=%.5f  antithetic se=%.5f",
                      plain.std_error, anti.std_error);
        check("Antithetic lowers standard error", anti.std_error < plain.std_error, buf);
    }

    // 4) American put is worth more than European put (early-exercise premium).
    {
        BSInputs put{100, 110, 0.05, 0.0, 0.20, 1.0, OptionType::Put};
        const double amer = binomial_price(put, 1000, Exercise::American);
        const double euro = binomial_price(put, 1000, Exercise::European);
        std::snprintf(buf, sizeof(buf), "american=%.5f european=%.5f", amer, euro);
        check("American put >= European put", amer >= euro - 1e-9 && amer > euro, buf);
    }

    // 5) On a non-dividend stock, an American call equals the European call.
    {
        const double amer = binomial_price(call, 1000, Exercise::American);
        const double euro = binomial_price(call, 1000, Exercise::European);
        std::snprintf(buf, sizeof(buf), "american=%.5f european=%.5f", amer, euro);
        check("American call == European call (q=0)", std::fabs(amer - euro) < 1e-6, buf);
    }

    if (g_failures == 0) {
        std::printf("\nAll convergence tests passed.\n");
        return EXIT_SUCCESS;
    }
    std::printf("\n%d test(s) FAILED.\n", g_failures);
    return EXIT_FAILURE;
}
