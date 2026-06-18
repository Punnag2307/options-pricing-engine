#pragma once

#include "ope/types.hpp"

namespace ope {

struct IVResult {
    double vol;        // implied volatility (NaN if no solution exists)
    int iterations;    // iterations used
    bool converged;    // whether it reached the tolerance
};

// Recover the Black-Scholes implied volatility that reproduces `market_price`
// for the given contract. Uses Newton-Raphson driven by vega, safeguarded by
// a bisection step whenever Newton would leave the no-arbitrage bracket or
// vega is too small to trust (deep in/out of the money). The `vol` field of
// `in` is ignored; it is the unknown being solved for.
IVResult implied_vol(const BSInputs& in, double market_price,
                     double tol = 1e-8, int max_iter = 100);

}  // namespace ope
