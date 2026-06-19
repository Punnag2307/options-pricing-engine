"""Build an implied-volatility surface.

A flat Black-Scholes world has no smile, so we generate a realistic option
chain by pricing under Merton jump-diffusion (downward, fat-tailed jumps, as in
equity markets), then recover the Black-Scholes implied volatility of each
quote with the engine's solver. The result is the classic surface: a skew
across strikes that flattens as maturity grows.

For each strike we price the out-of-the-money option (a put below spot, a call
above) -- which is what markets actually quote -- and invert it to implied vol.

Run after building:
    PYTHONPATH=build python python/vol_surface.py
Writes docs/vol_surface.png.
"""
import os

import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401 (registers 3d projection)

import ope

# Market setup and a downward-skewed jump process (equity-like).
SPOT, RATE, SIGMA = 100.0, 0.03, 0.18
JUMP = ope.JumpParams(lambda_=0.8, mu_j=-0.12, delta=0.20)

strikes = np.linspace(70, 130, 31)
maturities = np.array([0.1, 0.25, 0.5, 0.75, 1.0, 1.5, 2.0])

iv = np.full((len(maturities), len(strikes)), np.nan)
for i, T in enumerate(maturities):
    for j, K in enumerate(strikes):
        otype = ope.OptionType.Put if K < SPOT else ope.OptionType.Call
        inp = ope.BSInputs(spot=SPOT, strike=K, rate=RATE, dividend=0.0,
                           vol=SIGMA, maturity=float(T), type=otype)
        price = ope.merton_jump_price(inp, JUMP)
        res = ope.implied_vol(inp, price)
        if res.converged:
            iv[i, j] = res.vol

moneyness = strikes / SPOT
fig = plt.figure(figsize=(13, 5.2))

# Left: 3D surface.
ax1 = fig.add_subplot(1, 2, 1, projection="3d")
X, Y = np.meshgrid(moneyness, maturities)
ax1.plot_surface(X, Y, iv * 100, cmap="viridis", edgecolor="none", alpha=0.95)
ax1.set_xlabel("Moneyness (K / S)")
ax1.set_ylabel("Maturity (years)")
ax1.set_zlabel("Implied vol (%)")
ax1.set_title("Implied volatility surface")
ax1.view_init(elev=22, azim=-60)

# Right: smile slices at a few maturities.
ax2 = fig.add_subplot(1, 2, 2)
for i, T in enumerate(maturities):
    if T in (0.1, 0.5, 1.0, 2.0):
        ax2.plot(moneyness, iv[i] * 100, marker="o", ms=3, label=f"T = {T:g}y")
ax2.axvline(1.0, color="grey", ls=":", lw=1)
ax2.set_xlabel("Moneyness (K / S)")
ax2.set_ylabel("Implied vol (%)")
ax2.set_title("Volatility smile flattens with maturity")
ax2.legend()
ax2.grid(alpha=0.3)

fig.suptitle("Implied-vol surface recovered from a jump-diffusion option chain",
             fontsize=12, fontweight="bold")
fig.tight_layout()

os.makedirs("docs", exist_ok=True)
out = "docs/vol_surface.png"
fig.savefig(out, dpi=130, bbox_inches="tight")
print(f"Wrote {out}")

# Quick text summary of the skew at the shortest maturity.
row = iv[0] * 100
print(f"At T={maturities[0]:g}y:  IV(K=70)={row[0]:.1f}%  "
      f"IV(ATM)={row[len(strikes)//2]:.1f}%  IV(K=130)={row[-1]:.1f}%")
