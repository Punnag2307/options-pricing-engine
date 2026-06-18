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
| Binomial tree pricer (American options) | planned |
| Monte Carlo pricer (variance reduction) | planned |
| Implied-volatility solver | planned |
| Latency benchmarks + vol-surface plots | planned |

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

## Layout

```
include/ope/black_scholes.hpp   public types + interface
src/black_scholes.cpp           implementation
bindings/bindings.cpp           pybind11 module definition
tests/test_black_scholes.cpp    C++ tests (hand-checked references, parity)
python/validate.py              cross-check vs independent SciPy reference
CMakeLists.txt                  build for lib, tests, and Python module
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
g = ope.black_scholes(inp)
print(g.price, g.delta, g.vega)
```
