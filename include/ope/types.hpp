#pragma once

namespace ope {

// Which side of the contract we are pricing.
enum class OptionType { Call, Put };

// When the option may be exercised.
enum class Exercise { European, American };

// Market and contract inputs shared by every pricing model.
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

}  // namespace ope
