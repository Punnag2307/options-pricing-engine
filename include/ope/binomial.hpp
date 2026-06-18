#pragma once

#include "ope/types.hpp"

namespace ope {

// Cox-Ross-Rubinstein binomial tree price for a European or American option.
//
// For American options there is no closed form; the tree handles them by
// checking the early-exercise value at every node during backward induction.
// `steps` is the number of time steps in the tree (more = more accurate,
// converging to Black-Scholes for European options as steps -> infinity).
double binomial_price(const BSInputs& in, int steps, Exercise ex);

}  // namespace ope
