// Minimal self-contained C++ tests. No external framework so it builds anywhere.
// Each case checks against an independently computed Black-Scholes value.
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "ope/black_scholes.hpp"

static int g_failures = 0;

static void check(const char* name, double got, double expected, double tol) {
    const double diff = std::fabs(got - expected);
    if (diff > tol) {
        std::printf("  FAIL  %-22s got=%.6f  expected=%.6f  diff=%.2e\n",
                    name, got, expected, diff);
        ++g_failures;
    } else {
        std::printf("  ok    %-22s %.6f\n", name, got);
    }
}

int main() {
    using namespace ope;

    // Textbook ATM case: S=100, K=100, r=5%, q=0, vol=20%, T=1y.
    // Reference values are standard and reproducible from any BS calculator.
    {
        BSInputs in{100, 100, 0.05, 0.0, 0.20, 1.0, OptionType::Call};
        Greeks g = black_scholes(in);
        std::printf("Call S=100 K=100 r=5%% vol=20%% T=1\n");
        check("price", g.price, 10.450584, 1e-4);
        check("delta", g.delta, 0.636831, 1e-4);
        check("gamma", g.gamma, 0.018762, 1e-4);
        check("vega",  g.vega,  37.524035, 1e-3);
        check("rho",   g.rho,   53.232482, 1e-3);
    }

    // Same parameters, put side. Verify put-call parity holds:
    // C - P = S*e^{-qT} - K*e^{-rT}.
    {
        BSInputs c{100, 100, 0.05, 0.0, 0.20, 1.0, OptionType::Call};
        BSInputs p{100, 100, 0.05, 0.0, 0.20, 1.0, OptionType::Put};
        Greeks gc = black_scholes(c);
        Greeks gp = black_scholes(p);
        const double parity_lhs = gc.price - gp.price;
        const double parity_rhs = 100.0 - 100.0 * std::exp(-0.05 * 1.0);
        std::printf("Put-call parity\n");
        check("C - P", parity_lhs, parity_rhs, 1e-9);
    }

    // Edge case: expired option returns intrinsic value, no Greeks.
    {
        BSInputs in{120, 100, 0.05, 0.0, 0.20, 0.0, OptionType::Call};
        Greeks g = black_scholes(in);
        std::printf("Expired call (T=0)\n");
        check("intrinsic", g.price, 20.0, 1e-9);
        check("delta", g.delta, 0.0, 1e-12);
    }

    if (g_failures == 0) {
        std::printf("\nAll tests passed.\n");
        return EXIT_SUCCESS;
    }
    std::printf("\n%d test(s) FAILED.\n", g_failures);
    return EXIT_FAILURE;
}
