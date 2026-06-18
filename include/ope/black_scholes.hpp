#pragma once

#include "ope/types.hpp"

namespace ope {

// Price plus the standard first/second-order sensitivities ("the Greeks").
struct Greeks {
    double price;   // theoretical option value
    double delta;   // d(price)/d(spot)
    double gamma;   // d(delta)/d(spot)   -- same for calls and puts
    double vega;    // d(price)/d(vol), per 1.00 change in vol (e.g. 0.20 -> 1.20)
    double theta;   // d(price)/d(time), per year (negative = time decay)
    double rho;     // d(price)/d(rate), per 1.00 change in rate
};

// Closed-form Black-Scholes-Merton price and Greeks for a European option.
// Handles the degenerate edges (T <= 0 or vol <= 0) by returning the
// discounted intrinsic value with zero sensitivities.
Greeks black_scholes(const BSInputs& in);

}  // namespace ope
