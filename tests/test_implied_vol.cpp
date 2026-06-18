// Implied-vol solver tests: recovering a known vol from its price, across a
// grid of strikes and vols, plus the no-solution edge case.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

#include "ope/black_scholes.hpp"
#include "ope/implied_vol.hpp"

static int g_failures = 0;

static void check(const char* name, bool ok, const char* detail) {
    if (ok) {
        std::printf("  ok    %-38s %s\n", name, detail);
    } else {
        std::printf("  FAIL  %-38s %s\n", name, detail);
        ++g_failures;
    }
}

int main() {
    using namespace ope;
    char buf[256];

    // Round-trip: price an option at a known vol, then recover that vol from
    // the price. Sweep strikes (ITM/ATM/OTM) and several true vols.
    double worst = 0.0;
    int worst_iters = 0;
    for (double K : {70.0, 90.0, 100.0, 110.0, 140.0}) {
        for (double true_vol : {0.10, 0.20, 0.35, 0.60}) {
            for (OptionType t : {OptionType::Call, OptionType::Put}) {
                BSInputs in{100, K, 0.05, 0.0, true_vol, 1.0, t};
                const double mkt = black_scholes(in).price;
                const IVResult iv = implied_vol(in, mkt);
                if (iv.converged) {
                    worst = std::max(worst, std::fabs(iv.vol - true_vol));
                    worst_iters = std::max(worst_iters, iv.iterations);
                } else {
                    worst = 1e9;  // force a failure if anything did not converge
                }
            }
        }
    }
    std::snprintf(buf, sizeof(buf), "max vol error=%.2e, max iters=%d", worst, worst_iters);
    check("Round-trip recovers true vol", worst < 1e-6, buf);

    // A price strictly below intrinsic value has no valid implied vol.
    {
        BSInputs in{100, 80, 0.05, 0.0, 0.20, 1.0, OptionType::Call};
        const IVResult iv = implied_vol(in, 5.0);  // intrinsic ~ 20, so 5 is impossible
        std::snprintf(buf, sizeof(buf), "converged=%d vol=%.4f", iv.converged, iv.vol);
        check("Below-intrinsic price -> no solution", !iv.converged && std::isnan(iv.vol), buf);
    }

    if (g_failures == 0) {
        std::printf("\nAll implied-vol tests passed.\n");
        return EXIT_SUCCESS;
    }
    std::printf("\n%d test(s) FAILED.\n", g_failures);
    return EXIT_FAILURE;
}
