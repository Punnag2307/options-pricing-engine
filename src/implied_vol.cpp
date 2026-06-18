#include "ope/implied_vol.hpp"

#include <cmath>
#include <limits>
#include <numbers>

#include "ope/black_scholes.hpp"

namespace ope {

IVResult implied_vol(const BSInputs& in, double market_price, double tol,
                     int max_iter) {
    const double nan = std::numeric_limits<double>::quiet_NaN();

    if (in.maturity <= 0.0) {
        return IVResult{nan, 0, false};
    }

    // Volatility is bounded to a wide but finite bracket: 0.0001% to 500%.
    double lo = 1e-6;
    double hi = 5.0;

    BSInputs probe = in;  // we vary only the volatility
    auto price_at = [&](double v) {
        probe.vol = v;
        return black_scholes(probe).price;
    };

    // Black-Scholes price is strictly increasing in volatility, so if the
    // market price lies outside [price(lo), price(hi)] there is no solution
    // (e.g. a quote below intrinsic value -> arbitrage).
    if (market_price <= price_at(lo) || market_price >= price_at(hi)) {
        return IVResult{nan, 0, false};
    }

    // Brenner-Subrahmanyam closed-form approximation as the initial guess
    // (exact for at-the-money options, a good start elsewhere).
    double v = std::sqrt(2.0 * std::numbers::pi / in.maturity) *
               market_price / in.spot;
    if (!(v > lo && v < hi)) {
        v = 0.5 * (lo + hi);
    }

    for (int i = 1; i <= max_iter; ++i) {
        probe.vol = v;
        const Greeks g = black_scholes(probe);
        const double diff = g.price - market_price;

        if (std::fabs(diff) < tol) {
            return IVResult{v, i, true};
        }

        // Keep the root bracketed: price increasing in vol, so a price that is
        // too high means vol is too high.
        if (diff > 0.0) hi = v; else lo = v;

        // Newton step, safeguarded: fall back to bisection if vega is tiny or
        // the Newton step would jump outside the current bracket.
        const double v_newton = v - diff / g.vega;
        if (!(g.vega > 1e-10) || v_newton <= lo || v_newton >= hi) {
            v = 0.5 * (lo + hi);
        } else {
            v = v_newton;
        }
    }

    return IVResult{v, max_iter, false};
}

}  // namespace ope
