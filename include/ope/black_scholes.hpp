#pragma once

namespace ope {

// Which side of the contract we are pricing.
enum class OptionType { Call, Put };

// All inputs for a European option under the Black-Scholes-Merton model.
// Rates and dividend yield are continuously compounded. Time is in years.
struct BSInputs {
    double spot;      // S  - current price of the underlying
    double strike;    // K  - strike price
    double rate;      // r  - risk-free rate (continuously compounded)
    double dividend;  // q  - continuous dividend yield (0 if none)
    double vol;       // sigma - annualised volatility
    double maturity;  // T  - time to expiry in years
    OptionType type;
};

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
