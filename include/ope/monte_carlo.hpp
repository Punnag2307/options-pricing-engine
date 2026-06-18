#pragma once

#include "ope/types.hpp"

namespace ope {

// Monte Carlo result: the price estimate plus its standard error, so callers
// can build a confidence interval and see the estimate is statistical, not exact.
struct MCResult {
    double price;
    double std_error;  // standard error of the price estimate
};

// Monte Carlo price for a European option under risk-neutral geometric
// Brownian motion. `antithetic` enables antithetic-variate variance reduction
// (each normal draw Z is paired with -Z), which lowers the standard error for
// the same number of draws at almost no extra cost. `seed` makes runs
// reproducible.
MCResult monte_carlo_price(const BSInputs& in, long num_paths,
                           bool antithetic = true, unsigned long seed = 42UL);

}  // namespace ope
