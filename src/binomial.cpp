#include "ope/binomial.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ope {
namespace {

double intrinsic_value(double spot, double strike, OptionType type) {
    return (type == OptionType::Call) ? std::max(spot - strike, 0.0)
                                      : std::max(strike - spot, 0.0);
}

}  // namespace

double binomial_price(const BSInputs& in, int steps, Exercise ex) {
    if (steps < 1) steps = 1;

    // Degenerate cases collapse to intrinsic value (discounted forward).
    if (in.maturity <= 0.0 || in.vol <= 0.0) {
        return intrinsic_value(in.spot, in.strike, in.type);
    }

    const double dt = in.maturity / steps;
    const double u  = std::exp(in.vol * std::sqrt(dt));   // up factor
    const double d  = 1.0 / u;                            // down factor
    const double disc = std::exp(-in.rate * dt);          // per-step discount

    // Risk-neutral probability of an up move (Cox-Ross-Rubinstein).
    const double p = (std::exp((in.rate - in.dividend) * dt) - d) / (u - d);
    const double q = 1.0 - p;

    // Terminal underlying prices and option payoffs.
    // Node j at the final step has j up moves: S * u^j * d^(steps - j).
    std::vector<double> value(steps + 1);
    for (int j = 0; j <= steps; ++j) {
        const double S_T = in.spot * std::pow(u, j) * std::pow(d, steps - j);
        value[j] = intrinsic_value(S_T, in.strike, in.type);
    }

    // Backward induction. At step i, node j corresponds to spot
    // S * u^j * d^(i - j). For American options, take the better of holding
    // (discounted expected value) and exercising (intrinsic value now).
    for (int i = steps - 1; i >= 0; --i) {
        for (int j = 0; j <= i; ++j) {
            double hold = disc * (p * value[j + 1] + q * value[j]);
            if (ex == Exercise::American) {
                const double S_ij = in.spot * std::pow(u, j) * std::pow(d, i - j);
                const double exercise = intrinsic_value(S_ij, in.strike, in.type);
                hold = std::max(hold, exercise);
            }
            value[j] = hold;
        }
    }

    return value[0];
}

}  // namespace ope
