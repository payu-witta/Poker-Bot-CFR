import argparse
import csv
import os
import sys

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


def plot_all(results, path):
    fig, axes = plt.subplots(1, 2, figsize=(14, 5.5))
    for ax, game in zip(axes, GAMES):
        draw(ax, results.get(game, {}), False)
        ax.set_title(GAMES[game], fontweight="bold")
    fig.suptitle("Vanilla CFR vs CFR+ vs DCFR", fontweight="bold", y=1.02)
    save(fig, path)


def main():
    ap = argparse.ArgumentParser(description="Plot CFR convergence from the solver CSVs")
    ap.add_argument("--game", choices=["kuhn", "leduc", "all"], default="all")
    ap.add_argument("--results-dir", default="results")
    ap.add_argument("--plots-dir", default="plots")
    args = ap.parse_args()

    games = list(GAMES) if args.game == "all" else [args.game]
    results = {}
    for g in games:
        results[g] = {}
        for s in SOLVERS:
            path = os.path.join(args.results_dir, f"{g}_{s}.csv")
            if os.path.exists(path):
                results[g][s] = load_csv(path)
                print(f"loaded {path} ({len(results[g][s][0])} checkpoints)")
            else:
                print(f"missing {path}, see the README for how to generate it")

    if not any(results.values()):
        sys.exit(1)

    for g in games:
        if results[g]:
            plot_game(results[g], g, os.path.join(args.plots_dir, f"{g}_comparison.png"))
    if len(games) == 2:
        plot_all(results, os.path.join(args.plots_dir, "combined_comparison.png"))

    print(f"\n{'game':<16}{'solver':<36}final exploitability")
    for g in games:
        for s in SOLVERS:
            if s in results[g]:
                print(f"{GAMES[g]:<16}{STYLE[s][2]:<36}{results[g][s][1][-1]:.6f}")


if __name__ == "__main__":
    main()
