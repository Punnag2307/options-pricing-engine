# Options Pricing Engine

A C++ options pricing engine with a Python interface. The numerical core is
written in C++ for speed and numerical control; a thin
[pybind11](https://github.com/pybind/pybind11) layer exposes it to Python so
that validation, plotting, and benchmarking can be done from notebooks — the
same split used in production quant systems (fast C++ core, Python research
layer).

## Status

| Component | Status |
|---|---|
| Black-Scholes-Merton closed form + Greeks | done |
| pybind11 Python bindings | done |
| C++ test suite + Python cross-validation | done |
| Binomial tree pricer (European + American) | done |
| Monte Carlo pricer (antithetic variance reduction) | done |
| Three-method convergence test | done |
| Implied-volatility solver (safeguarded Newton) | done |
| Latency benchmark | done |
| CI (GitHub Actions) | planned |
| Volatility-surface plots | planned |

## The model

For a European option under Black-Scholes-Merton with continuous dividend
yield `q`:

```
d1 = [ ln(S/K) + (r - q + sigma^2 / 2) T ] / (sigma sqrt(T))
d2 = d1 - sigma sqrt(T)

Call = S e^{-qT} N(d1) - K e^{-rT} N(d2)
Put  = K e^{-rT} N(-d2) - S e^{-qT} N(-d1)
```

The Greeks (delta, gamma, vega, theta, rho) are computed analytically rather
than by bumping inputs, so they are exact and cheap. The normal CDF uses
`std::erfc`, which is stable across the whole real line. Degenerate inputs
(`T <= 0` or `sigma <= 0`) fall back to discounted intrinsic value.

## Three pricing methods, cross-checked

The engine prices the same option three independent ways, which is how the
results are validated against each other:

- **Closed form** (Black-Scholes-Merton) — exact, instant, European only.
- **Binomial tree** (Cox-Ross-Rubinstein) — converges to the closed form for
  European options, and also prices **American** options via early-exercise
  checks at every node, where no closed form exists.
- **Monte Carlo** — simulates terminal prices under risk-neutral geometric
  Brownian motion. Uses antithetic variates to cut variance, and reports a
  **standard error** so the estimate comes with a confidence band rather than
  a false sense of exactness.

All three agree on the same option value:

![Convergence of the three pricers](docs/convergence.png)

The test suite also confirms the tree reproduces known American-option facts:
an American put is worth strictly more than the European put (the early-exercise
premium), while an American call on a non-dividend stock equals the European
call.

## Implied volatility

Given a market price, the engine solves for the volatility that reproduces it
under Black-Scholes. The solver is Newton-Raphson driven by vega, but
safeguarded: at every step it keeps the root inside a no-arbitrage bracket and
falls back to a bisection step whenever a Newton step would jump outside that
bracket or vega is too small to trust (which happens deep in or out of the
money, exactly where naive Newton-Raphson diverges). Prices below intrinsic
value are rejected as having no solution. A round-trip test recovers the input
volatility across a grid of strikes and vols to better than `1e-6`, typically
in three or four iterations.

## Performance

`bench/benchmark.cpp` times each pricer over a book of 4,096 *varied* options
(random spot/strike/vol/maturity, mixed calls and puts) so the figures are not
flattered by pricing one constant input repeatedly — which would hand the CPU
perfect branch prediction and unrealistically low timings. Numbers are
amortized, single-thread, hot-cache; run it yourself with `./build/benchmark`.

As a rough reference (Release, `-O3`):

```
Black-Scholes price + 5 Greeks :  ~100 ns/call
Implied volatility solve       :    ~1 us/call
Binomial tree (American, 1000) :   tens of ms/call  (scales as O(steps^2))
Monte Carlo (1,000,000 paths)  :   tens of ms
```

Two caveats, stated so the numbers stay honest: these measure the C++ core, not
the Python binding (the pybind11 boundary costs far more per call, so batch to
amortize it); and at tens of nanoseconds per call a single clock read costs as
much as the operation, so the figure is a mean over a large batch rather than
per-call percentiles.

## Layout

```
include/ope/types.hpp           shared types (inputs, option type, exercise style)
include/ope/black_scholes.hpp   closed-form interface + Greeks
include/ope/binomial.hpp        binomial tree interface
include/ope/monte_carlo.hpp     Monte Carlo interface
include/ope/implied_vol.hpp     implied-volatility solver interface
src/                            implementations
bindings/bindings.cpp           pybind11 module definition
bench/benchmark.cpp             latency benchmark for each pricer
tests/test_black_scholes.cpp    closed-form references + put-call parity
tests/test_convergence.cpp      tree/MC convergence + American-option checks
tests/test_implied_vol.cpp      implied-vol round-trip + no-solution edge case
python/validate.py              cross-check vs independent SciPy reference
python/convergence.py           generates the convergence chart above
CMakeLists.txt                  build for lib, tests, benchmark, Python module
```

## Build

Requires a C++20 compiler, CMake >= 3.18, and pybind11 (`pip install pybind11`).

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(python -m pybind11 --cmakedir)"
cmake --build build -j

# C++ tests
ctest --test-dir build --output-on-failure

# Python cross-validation
PYTHONPATH=build python python/validate.py
```

## Use from Python

```python
import ope

inp = ope.BSInputs(spot=100, strike=100, rate=0.05,
                   vol=0.20, maturity=1.0, type=ope.OptionType.Call)

# Closed form + Greeks
g = ope.black_scholes(inp)
print(g.price, g.delta, g.vega)

# Binomial tree (European or American)
print(ope.binomial_price(inp, steps=1000, exercise=ope.Exercise.European))
print(ope.binomial_price(inp, steps=1000, exercise=ope.Exercise.American))

# Monte Carlo with standard error
mc = ope.monte_carlo_price(inp, num_paths=1_000_000)
print(mc.price, mc.std_error)

# Implied volatility from a market price
price = ope.black_scholes(inp).price
iv = ope.implied_vol(inp, price)
print(iv.vol, iv.iterations, iv.converged)
```

Regenerate the convergence chart, and run the benchmark, with:

```bash
PYTHONPATH=build python python/convergence.py
./build/benchmark
```
