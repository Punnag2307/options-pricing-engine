#include "ope/monte_carlo.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace ope {
namespace {

double payoff(double spot, double strike, OptionType type) {
    return (type == OptionType::Call) ? std::max(spot - strike, 0.0)
                                      : std::max(strike - spot, 0.0);
}

}  // namespace

MCResult monte_carlo_price(const BSInputs& in, long num_paths, bool antithetic,
                           unsigned long seed) {
    if (num_paths < 1) num_paths = 1;

    if (in.maturity <= 0.0 || in.vol <= 0.0) {
        return MCResult{payoff(in.spot, in.strike, in.type), 0.0};
    }

    // Terminal price under risk-neutral GBM:
    //   S_T = S * exp( (r - q - 0.5 sigma^2) T + sigma sqrt(T) Z ),  Z ~ N(0,1)
    const double drift = (in.rate - in.dividend - 0.5 * in.vol * in.vol) * in.maturity;
    const double diffusion = in.vol * std::sqrt(in.maturity);
    const double disc = std::exp(-in.rate * in.maturity);

    std::mt19937_64 rng(seed);
    std::normal_distribution<double> normal(0.0, 1.0);

    // Welford-style running mean/variance over the per-path discounted payoff.
    // With antithetic sampling each "path" is the average of the +Z and -Z
    // payoffs, which is the quantity whose variance we want to track.
    double mean = 0.0;
    double m2 = 0.0;
    long n = 0;

    for (long i = 0; i < num_paths; ++i) {
        const double z = normal(rng);
        const double st_up = in.spot * std::exp(drift + diffusion * z);
        double sample = disc * payoff(st_up, in.strike, in.type);

        if (antithetic) {
            const double st_dn = in.spot * std::exp(drift - diffusion * z);
            sample = 0.5 * (sample + disc * payoff(st_dn, in.strike, in.type));
        }

        ++n;
        const double delta = sample - mean;
        mean += delta / n;
        m2 += delta * (sample - mean);
    }

    const double variance = (n > 1) ? m2 / (n - 1) : 0.0;
    const double std_error = std::sqrt(variance / n);
    return MCResult{mean, std_error};
}

}  // namespace ope
