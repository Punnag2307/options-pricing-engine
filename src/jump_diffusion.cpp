#include "ope/jump_diffusion.hpp"

#include <cmath>

#include "ope/black_scholes.hpp"

namespace ope {

double merton_jump_price(const BSInputs& in, const JumpParams& jp, int max_terms) {
    // No time left: defer to the closed form's intrinsic-value handling.
    if (in.maturity <= 0.0) {
        return black_scholes(in).price;
    }

    // k is the expected proportional jump size; lambda_p is the jump intensity
    // under the risk-neutral measure used for the Poisson weights.
    const double k = std::exp(jp.mu_j + 0.5 * jp.delta * jp.delta) - 1.0;
    const double lambda_p = jp.lambda * (1.0 + k);
    const double lambda_pT = lambda_p * in.maturity;

    double price = 0.0;
    double poisson = std::exp(-lambda_pT);  // P(N = 0)

    for (int n = 0; n <= max_terms; ++n) {
        // Black-Scholes price conditioned on exactly n jumps: variance and
        // drift are shifted to account for the n jumps.
        BSInputs term = in;
        term.vol = std::sqrt(in.vol * in.vol + n * jp.delta * jp.delta / in.maturity);
        term.rate = in.rate - jp.lambda * k + n * std::log(1.0 + k) / in.maturity;

        price += poisson * black_scholes(term).price;

        // Advance the Poisson weight: P(n+1) = P(n) * lambda_pT / (n+1).
        poisson *= lambda_pT / (n + 1);
    }

    return price;
}

}  // namespace ope
