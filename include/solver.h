#pragma once

#include "game.h"

#include <algorithm>
#include <string>
#include <unordered_map>
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

using StrategyMap = std::unordered_map<std::string, std::unordered_map<std::string, double>>;

class CFRSolver {
public:
    std::unordered_map<std::string, InfoSetData> info_sets;
    int iteration = 0;

    void train(const std::shared_ptr<GameNode>& root, int n) {
        double reach[2];
        for (int t = 0; t < n; ++t) {
            ++iteration;
            reach[0] = reach[1] = 1.0;
            cfr(root, reach, 1.0);
        }
    }

    StrategyMap get_average_strategy() const {
        StrategyMap out;
        for (auto& [key, isd] : info_sets) {
            auto avg = isd.average_strategy();
            for (size_t i = 0; i < avg.size(); ++i) out[key][isd.actions[i]] = avg[i];
        }
        return out;
    }

private:
    // reach reuses one raw array on purpose, no allocation on the recursive path
    double cfr(const std::shared_ptr<GameNode>& node, double* reach, double chance_reach) {
        if (node->is_terminal()) return node->utility(0);

        if (node->is_chance_node()) {
            double ev = 0.0;
            for (auto& [next, prob] : node->chance_outcomes()) ev += prob * cfr(next, reach, chance_reach * prob);
            return ev;
        }

        int player = node->current_player();
        auto actions = node->legal_actions();
        InfoSetData& isd = info_sets.try_emplace(node->information_set_key(), actions).first->second;
        auto strat = isd.regret_matching();
        int n = static_cast<int>(actions.size());

        for (int i = 0; i < n; ++i) isd.strat_sum[i] += reach[player] * strat[i];

        std::vector<double> vals(n);
        double node_val = 0.0;
        for (int i = 0; i < n; ++i) {
            double old = reach[player];
            reach[player] *= strat[i];
            vals[i] = cfr(node->apply_action(actions[i]), reach, chance_reach);
            reach[player] = old;
            node_val += strat[i] * vals[i];
        }

        double cf_reach = reach[1 - player] * chance_reach;
        double sign = player == 0 ? 1.0 : -1.0;
        for (int i = 0; i < n; ++i) isd.regrets[i] += cf_reach * sign * (vals[i] - node_val);

        return node_val;
    }
};
