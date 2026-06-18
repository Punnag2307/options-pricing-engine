#include "ope/black_scholes.hpp"

#include <algorithm>
#include <cmath>

namespace ope {
namespace {

constexpr double kInvSqrt2  = 0.7071067811865476;  // 1 / sqrt(2)
constexpr double kInvSqrt2Pi = 0.3989422804014327; // 1 / sqrt(2*pi)

// Standard normal CDF via erfc. Stable across the whole real line, no tables.
inline double norm_cdf(double x) {
    return 0.5 * std::erfc(-x * kInvSqrt2);
}

// Standard normal PDF.
inline double norm_pdf(double x) {
    return kInvSqrt2Pi * std::exp(-0.5 * x * x);
}

// Discounted intrinsic value, used when there is no optionality left
// (expired, or zero volatility so the payoff is deterministic).
Greeks intrinsic(const BSInputs& in) {
    const double disc_spot   = in.spot   * std::exp(-in.dividend * in.maturity);
    const double disc_strike = in.strike * std::exp(-in.rate     * in.maturity);
    const double payoff = (in.type == OptionType::Call)
                              ? std::max(disc_spot - disc_strike, 0.0)
                              : std::max(disc_strike - disc_spot, 0.0);
    return Greeks{payoff, 0.0, 0.0, 0.0, 0.0, 0.0};
}

}  // namespace

Greeks black_scholes(const BSInputs& in) {
    // No time left or no volatility -> deterministic payoff, no Greeks.
    if (in.maturity <= 0.0 || in.vol <= 0.0) {
        return intrinsic(in);
    }

    const double sqrtT  = std::sqrt(in.maturity);
    const double volSqrtT = in.vol * sqrtT;
    const double d1 = (std::log(in.spot / in.strike) +
                       (in.rate - in.dividend + 0.5 * in.vol * in.vol) * in.maturity) /
                      volSqrtT;
    const double d2 = d1 - volSqrtT;

    const double df_r = std::exp(-in.rate * in.maturity);      // discount factor (rate)
    const double df_q = std::exp(-in.dividend * in.maturity);  // discount factor (dividend)

    const double Nd1 = norm_cdf(d1);
    const double Nd2 = norm_cdf(d2);
    const double pdf_d1 = norm_pdf(d1);

    Greeks g{};

    // Gamma and Vega are identical for calls and puts.
    g.gamma = df_q * pdf_d1 / (in.spot * volSqrtT);
    g.vega  = in.spot * df_q * pdf_d1 * sqrtT;

    if (in.type == OptionType::Call) {
        g.price = in.spot * df_q * Nd1 - in.strike * df_r * Nd2;
        g.delta = df_q * Nd1;
        g.theta = -(in.spot * df_q * pdf_d1 * in.vol) / (2.0 * sqrtT)
                  - in.rate * in.strike * df_r * Nd2
                  + in.dividend * in.spot * df_q * Nd1;
        g.rho   = in.strike * in.maturity * df_r * Nd2;
    } else {
        g.price = in.strike * df_r * norm_cdf(-d2) - in.spot * df_q * norm_cdf(-d1);
        g.delta = df_q * (Nd1 - 1.0);
        g.theta = -(in.spot * df_q * pdf_d1 * in.vol) / (2.0 * sqrtT)
                  + in.rate * in.strike * df_r * norm_cdf(-d2)
                  - in.dividend * in.spot * df_q * norm_cdf(-d1);
        g.rho   = -in.strike * in.maturity * df_r * norm_cdf(-d2);
    }

    return g;
}

}  // namespace ope
