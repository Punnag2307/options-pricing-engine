"""Generate the convergence figure: binomial tree and Monte Carlo both
converging to the closed-form Black-Scholes price.

Run after building:
    PYTHONPATH=build python python/convergence.py
Writes docs/convergence.png.
"""
import os

import matplotlib.pyplot as plt
import numpy as np

import ope

# Reference contract: ATM call, S=K=100, r=5%, q=0, vol=20%, T=1y.
inp = ope.BSInputs(spot=100, strike=100, rate=0.05, dividend=0.0,
                   vol=0.20, maturity=1.0, type=ope.OptionType.Call)
bs_price = ope.black_scholes(inp).price

# --- Binomial tree vs number of steps ---
tree_steps = np.unique(np.logspace(0, 3, 40).astype(int))
tree_prices = [ope.binomial_price(inp, int(n), ope.Exercise.European)
               for n in tree_steps]

# --- Monte Carlo vs number of paths (with +/- 2 standard-error band) ---
mc_paths = np.unique(np.logspace(2, 6.3, 30).astype(int))
mc_prices, mc_err = [], []
for n in mc_paths:
    r = ope.monte_carlo_price(inp, int(n), True, 2024)
    mc_prices.append(r.price)
    mc_err.append(r.std_error)
mc_prices = np.array(mc_prices)
mc_err = np.array(mc_err)

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 4.2))

ax1.axhline(bs_price, color="black", lw=1.5, ls="--",
            label=f"Black-Scholes = {bs_price:.4f}")
ax1.plot(tree_steps, tree_prices, color="#1f77b4", marker="o", ms=3, lw=1,
         label="Binomial tree")
ax1.set_xscale("log")
ax1.set_xlabel("Tree steps")
ax1.set_ylabel("Option price")
ax1.set_title("Binomial tree converges to closed form")
ax1.legend()
ax1.grid(alpha=0.3)

ax2.axhline(bs_price, color="black", lw=1.5, ls="--",
            label=f"Black-Scholes = {bs_price:.4f}")
ax2.fill_between(mc_paths, mc_prices - 2 * mc_err, mc_prices + 2 * mc_err,
                 color="#ff7f0e", alpha=0.25, label="Monte Carlo +/- 2 SE")
ax2.plot(mc_paths, mc_prices, color="#ff7f0e", marker="o", ms=3, lw=1,
         label="Monte Carlo estimate")
ax2.set_xscale("log")
ax2.set_xlabel("Monte Carlo paths")
ax2.set_ylabel("Option price")
ax2.set_title("Monte Carlo converges as paths grow")
ax2.legend()
ax2.grid(alpha=0.3)

fig.suptitle("Three independent pricers agree on the same option value",
             fontsize=12, fontweight="bold")
fig.tight_layout()

os.makedirs("docs", exist_ok=True)
out = "docs/convergence.png"
fig.savefig(out, dpi=130, bbox_inches="tight")
print(f"Wrote {out}")
print(f"Black-Scholes price: {bs_price:.6f}")
print(f"Tree (1000 steps):   {ope.binomial_price(inp, 1000, ope.Exercise.European):.6f}")
r = ope.monte_carlo_price(inp, 2_000_000, True, 2024)
print(f"Monte Carlo (2M):    {r.price:.6f} +/- {r.std_error:.6f}")
