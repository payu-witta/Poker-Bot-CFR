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
