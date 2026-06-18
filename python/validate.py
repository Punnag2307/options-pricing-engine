"""Cross-validate the compiled C++ engine against an independent
Black-Scholes implementation written in pure Python/SciPy.

If the two agree to ~1e-9 we can trust the C++ core. This is the
miniature version of the same idea we'll use later when checking the
tree and Monte-Carlo pricers against the closed form.

Run after building, e.g.:
    PYTHONPATH=build python python/validate.py
"""
import math

import numpy as np
from scipy.stats import norm

import ope  # the compiled C++ module


def bs_reference(S, K, r, q, sigma, T, is_call):
    """Independent closed-form reference, unrelated to the C++ code."""
    if T <= 0 or sigma <= 0:
        fwd = S * math.exp(-q * T) - K * math.exp(-r * T)
        return max(fwd, 0.0) if is_call else max(-fwd, 0.0)
    d1 = (math.log(S / K) + (r - q + 0.5 * sigma**2) * T) / (sigma * math.sqrt(T))
    d2 = d1 - sigma * math.sqrt(T)
    if is_call:
        return S * math.exp(-q * T) * norm.cdf(d1) - K * math.exp(-r * T) * norm.cdf(d2)
    return K * math.exp(-r * T) * norm.cdf(-d2) - S * math.exp(-q * T) * norm.cdf(-d1)


def main():
    rng = np.random.default_rng(0)
    max_err = 0.0
    n = 10_000
    for _ in range(n):
        S = rng.uniform(10, 200)
        K = rng.uniform(10, 200)
        r = rng.uniform(-0.01, 0.10)
        q = rng.uniform(0.0, 0.05)
        sigma = rng.uniform(0.05, 0.80)
        T = rng.uniform(0.01, 3.0)
        is_call = bool(rng.integers(0, 2))

        otype = ope.OptionType.Call if is_call else ope.OptionType.Put
        inp = ope.BSInputs(spot=S, strike=K, rate=r, dividend=q,
                           vol=sigma, maturity=T, type=otype)
        cpp = ope.black_scholes(inp).price
        ref = bs_reference(S, K, r, q, sigma, T, is_call)
        max_err = max(max_err, abs(cpp - ref))

    print(f"Compared {n:,} random options against SciPy reference.")
    print(f"Max absolute price error: {max_err:.3e}")
    assert max_err < 1e-9, "C++ engine disagrees with reference!"
    print("PASS - C++ engine matches the independent reference.")

    # Show one full Greeks readout.
    inp = ope.BSInputs(spot=100, strike=100, rate=0.05, dividend=0.0,
                       vol=0.20, maturity=1.0, type=ope.OptionType.Call)
    g = ope.black_scholes(inp)
    print("\nExample (ATM call, S=K=100, r=5%, vol=20%, T=1y):")
    for name in ("price", "delta", "gamma", "vega", "theta", "rho"):
        print(f"  {name:6s} = {getattr(g, name): .6f}")


if __name__ == "__main__":
    main()
