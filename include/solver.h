#pragma once

#include "game.h"

#include <algorithm>
#include <string>
#include <vector>

struct InfoSetData {
    std::vector<std::string> actions;
    std::vector<double> regrets;
    std::vector<double> strat_sum;

    explicit InfoSetData(const std::vector<std::string>& acts) : actions(acts), regrets(acts.size(), 0.0), strat_sum(acts.size(), 0.0) {}

    std::vector<double> regret_matching() const {
        std::vector<double> s(actions.size());
        double total = 0.0;
        for (size_t i = 0; i < s.size(); ++i) {
            s[i] = std::max(regrets[i], 0.0);
            total += s[i];
        }
        for (double& x : s) x = total > 0.0 ? x / total : 1.0 / s.size();
        return s;
    }

    std::vector<double> average_strategy() const {
        std::vector<double> s = strat_sum;
        double total = 0.0;
        for (double x : s) total += x;
        for (double& x : s) x = total > 0.0 ? x / total : 1.0 / s.size();
        return s;
    }
};
