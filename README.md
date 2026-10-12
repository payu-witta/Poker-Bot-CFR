# Poker Bot CFR

A C++ implementation of Counterfactual Regret Minimization (CFR) for solving
imperfect-information poker games, with a small Python script for plotting.

## Motivation

I built this for an override request for COMPSCI 515 (Algorithms, Game Theory,
and Fairness) at UMass Amherst, to show some initiative beyond the standard
prerequisites.

I went down a rabbit hole that ended at Brown & Sandholm (2019), which showed
that a surprisingly simple change to how CFR weights its past regrets can
consistently outperform CFR+, the algorithm behind the Libratus poker AI. The
goal here is to implement the solvers from scratch, test them, and check that
claim empirically.

## Layout

```
include/
  game.h       game node interface, card helpers
  kuhn.h       Kuhn Poker
  leduc.h      Leduc Hold'em
  solver.h     info sets and the CFR solver
  exploit.h    best response and exploitability
src/main.cpp   solver binary, writes exploitability CSVs
tests/         C++ test suite
scripts/       plotting
results/       CSVs from the solver (not tracked)
plots/         PNGs from plot.py (not tracked)
```

## Background

### Counterfactual Regret Minimization

CFR is a self-play algorithm for finding Nash equilibria in extensive-form
games. Each iteration it traverses the game tree, computes how much regret it
has for not playing each action, and uses those regrets to update a mixed
strategy with regret matching: play each action in proportion to its positive
cumulative regret.

The time-averaged strategy converges to a Nash equilibrium. The current
strategy oscillates, and the average smooths it out.

### CFR+

CFR+ floors cumulative regrets at zero and weights the strategy accumulation
by the iteration number. It usually converges much faster than vanilla CFR.

### Discounted CFR (DCFR)

DCFR comes from:

> Noam Brown and Tuomas Sandholm. "Solving Imperfect-Information Games via
> Discounted Regret Minimization." Proceedings of the AAAI Conference on
> Artificial Intelligence, 2019.
> [arXiv:1809.04040](https://arxiv.org/abs/1809.04040)

Early iterations give noisy regret estimates, because the strategy is far from
equilibrium. DCFR discounts the contribution of past iterations, and it
discounts positive and negative regrets differently. On iteration `t`, after
the instantaneous regret is added, the accumulated values are multiplied by:

```
positive regrets: t^alpha / (t^alpha + 1)
negative regrets: t^beta / (t^beta + 1)
strategy weight: (t / (t + 1))^gamma
```

The paper recommends alpha = 3/2, beta = 0, gamma = 2, which are the defaults
here. With beta = 0 the negative regret multiplier is a constant 1/2, so past
mistakes are halved every iteration instead of being driven to minus infinity.
The paper reports that DCFR with these parameters beat CFR+ on every heads-up
no-limit hold'em subgame it tested, typically by a factor of 2 to 3.

### Exploitability

Exploitability measures how far a strategy profile is from a Nash equilibrium:
the sum of both players' gains from switching to a best response. Zero means
Nash equilibrium.

### Games

Kuhn Poker
- 2 players, ranks J/Q/K, 1 chip ante each, one round of betting, 12 information sets.
- Hole cards are dealt from a deck with two of each rank, so both players can hold the same rank.

Leduc Hold'em
- 2 players, 6 cards (J/J/Q/Q/K/K), two rounds of betting with a community card in between.
- Bets are 2 chips in round 1 and 4 chips in round 2, with at most 2 bets per round.
- 288 information sets, a standard benchmark for imperfect-information solvers.

## Building

Needs a C++20 compiler (GCC 10+ or Clang 12+) and CMake 3.14+.

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
cd ..
```

This produces `build/cfr_solver` and `build/test_cfr`.

## Running the solver

```bash
./build/cfr_solver --game kuhn --solver vanilla --iterations 10000 --output results/kuhn_vanilla.csv
./build/cfr_solver --game kuhn --solver cfr_plus --iterations 10000 --output results/kuhn_cfr_plus.csv
./build/cfr_solver --game kuhn --solver dcfr --iterations 10000 --output results/kuhn_dcfr.csv
./build/cfr_solver --game leduc --solver vanilla --iterations 100000 --output results/leduc_vanilla.csv
./build/cfr_solver --game leduc --solver cfr_plus --iterations 100000 --output results/leduc_cfr_plus.csv
./build/cfr_solver --game leduc --solver dcfr --iterations 100000 --output results/leduc_dcfr.csv
```

Larger iteration counts are better, these are just a starting point.

Options
- `--game kuhn|leduc`, default kuhn
- `--solver vanilla|cfr_plus|dcfr`, default vanilla
- `--iterations N`, default 10000
- `--eval-points N`, number of log spaced checkpoints where exploitability is measured, default 60
- `--output path.csv`, default stdout
- `--quiet`, suppress progress output

## Running tests

```bash
./build/test_cfr
# or through ctest
cd build && ctest --output-on-failure
```

The suite covers
- game logic for both games (terminal counts, zero-sum, utilities, info set keys and counts)
- convergence against exploitability thresholds
- Nash strategy properties on Kuhn Poker
- CFR+ converging faster than vanilla CFR
- DCFR discounting regrets and beating CFR+ at early checkpoints

## Plotting

```bash
pip install -r requirements.txt
python scripts/plot.py
python scripts/plot.py --game kuhn
```

Plots are written to `plots/`: `kuhn_comparison.png`, `leduc_comparison.png` and
`combined_comparison.png`. Every solver with a CSV in `results/` is drawn, and a
table of final exploitabilities is printed at the end. Zero exploitability
values are dropped because the plots use a log scale.

## Implementation notes

### Header only

Everything in `include/` is header only, so the solver binary and the tests
just add `include/` to their include path and compile one source file each.

### Solver

`CFRSolver` is game agnostic. It only talks to the `GameNode` interface, so a
new game is one class that implements it. The three variants share one
traversal and differ in how the strategy sum is weighted and how regrets are
updated after each node.

The reach probabilities are passed down the recursion as a raw `double[2]`
instead of a `std::array` or `std::vector`, to keep allocations off the hot
path.

### Exploitability

Exploitability is computed for each player and the two best response values
are summed.

1. A traversal follows the fixed strategy for the opponent and groups the best
   response player's nodes by information set, each with the opponent and
   chance reach of the node.
2. Information sets are processed deepest first. For each one the best response
   picks the action with the highest reach weighted value, where the value of
   each child is evaluated with the play already chosen deeper in the tree.
3. A final pass evaluates the resulting deterministic strategy against the
   fixed one.
