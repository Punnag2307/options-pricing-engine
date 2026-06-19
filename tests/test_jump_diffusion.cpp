// Jump-diffusion tests: it must reduce to Black-Scholes when there are no
// jumps, and with jumps switched on it must produce a volatility smile (the
// recovered Black-Scholes implied vol is higher in the wings than at the money).
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "ope/black_scholes.hpp"
#include "ope/implied_vol.hpp"
#include "ope/jump_diffusion.hpp"

static int g_failures = 0;

static void check(const char* name, bool ok, const char* detail) {
    if (ok) {
        std::printf("  ok    %-40s %s\n", name, detail);
    } else {
        std::printf("  FAIL  %-40s %s\n", name, detail);
        ++g_failures;
    }
}

int main() {
    using namespace ope;
    char buf[256];

    // 1) No jumps (lambda = 0) -> exactly Black-Scholes.
    {
        BSInputs in{100, 100, 0.03, 0.0, 0.20, 1.0, OptionType::Call};
        JumpParams none{0.0, 0.0, 0.0};
        const double jd = merton_jump_price(in, none);
        const double bs = black_scholes(in).price;
        std::snprintf(buf, sizeof(buf), "jd=%.6f bs=%.6f", jd, bs);
        check("lambda=0 reduces to Black-Scholes", std::fabs(jd - bs) < 1e-9, buf);
    }

    // 2) With downward-skewed jumps, the implied-vol smile slopes up in the
    //    wings. Price OTM options under the jump model, invert to BS implied
    //    vol, and confirm a low-strike (OTM put) vol exceeds the ATM vol.
    {
        const double S = 100, r = 0.03, sigma = 0.18, T = 0.5;
        JumpParams jp{0.8, -0.12, 0.20};  // frequent, downward, fat-tailed jumps

        auto implied = [&](double K, OptionType t) {
            BSInputs in{S, K, r, 0.0, sigma, T, t};
            const double mkt = merton_jump_price(in, jp);
            return implied_vol(in, mkt).vol;
        };

        const double iv_low = implied(80, OptionType::Put);   // OTM put, left wing
        const double iv_atm = implied(100, OptionType::Call);  // at the money
        std::snprintf(buf, sizeof(buf), "iv(K=80 put)=%.4f  iv(ATM)=%.4f", iv_low, iv_atm);
        check("Jumps produce a volatility smile/skew", iv_low > iv_atm + 1e-3, buf);
    }

    if (g_failures == 0) {
        std::printf("\nAll jump-diffusion tests passed.\n");
        return EXIT_SUCCESS;
    }
    std::printf("\n%d test(s) FAILED.\n", g_failures);
    return EXIT_FAILURE;
}
