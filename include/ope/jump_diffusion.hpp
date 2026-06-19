#pragma once

#include "ope/types.hpp"

namespace ope {

// Parameters of the lognormal jump component in Merton's jump-diffusion model.
struct JumpParams {
    double lambda;  // jump intensity (expected number of jumps per year)
    double mu_j;    // mean of the log jump size
    double delta;   // standard deviation of the log jump size
};

// Merton (1976) jump-diffusion price for a European option. The price is a
// Poisson-weighted sum of Black-Scholes prices, each conditioned on n jumps
// having occurred. Reusing the closed form term-by-term, it adds the fat tails
// that produce a volatility smile when the result is inverted back to a
// Black-Scholes implied vol. As lambda -> 0 it collapses to plain
// Black-Scholes.
double merton_jump_price(const BSInputs& in, const JumpParams& jp,
                         int max_terms = 60);

}  // namespace ope
