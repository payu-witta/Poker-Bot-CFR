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
