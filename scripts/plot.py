import csv
import os

import matplotlib.pyplot as plt

SOLVERS = ["vanilla", "cfr_plus", "dcfr"]
GAMES = {"kuhn": "Kuhn Poker", "leduc": "Leduc Hold'em"}
STYLE = {
    "vanilla": ("#2563EB", "o", "Vanilla CFR"),
    "cfr_plus": ("#DC2626", "s", "CFR+"),
    "dcfr": ("#16A34A", "D", "DCFR (alpha=1.5, beta=0, gamma=2)")
}


def load_csv(path):
    iters, expls = [], []
    with open(path) as f:
        for row in csv.DictReader(f):
            expl = float(row["exploitability"])
            # log scale can't draw zeros
            if expl > 0:
                iters.append(int(row["iteration"]))
                expls.append(expl)
    return iters, expls


def draw(ax, res, annotate):
    for s in SOLVERS:
        if s not in res:
            continue
        iters, expls = res[s]
        color, marker, label = STYLE[s]
        ax.loglog(iters, expls, color=color, marker=marker, markersize=3, linewidth=2, label=label, alpha=0.9)
        if annotate and expls:
            ax.annotate(f"{expls[-1]:.4f}", xy=(iters[-1], expls[-1]), xytext=(iters[-1] * 0.5, expls[-1] * 3), fontsize=9, color=color, arrowprops=dict(arrowstyle="->", color=color, lw=1))

    ax.set_xlabel("Iterations")
    ax.set_ylabel("Exploitability")
    ax.legend(framealpha=0.9)
    ax.grid(True, which="both", alpha=0.3, linestyle="--")
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)


def save(fig, path):
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    fig.tight_layout()
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"saved {path}")


def plot_game(res, game, path):
    fig, ax = plt.subplots(figsize=(9, 5.5))
    draw(ax, res, True)
    ax.set_title(f"CFR convergence on {GAMES[game]}", fontweight="bold")
    save(fig, path)
