#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

using StrategyMap = std::unordered_map<std::string, std::unordered_map<std::string, double>>;

class GameNode {
public:
    virtual ~GameNode() = default;

    virtual bool is_terminal() const = 0;
    virtual bool is_chance_node() const = 0;
    virtual int current_player() const = 0;
    virtual double utility(int player) const = 0;
    virtual std::vector<std::pair<std::shared_ptr<GameNode>, double>> chance_outcomes() const = 0;
    virtual std::vector<std::string> legal_actions() const = 0;
    virtual std::shared_ptr<GameNode> apply_action(const std::string& action) const = 0;
    virtual std::string information_set_key() const = 0;
};

inline std::string card_name(int c) {
    if (c == 0) return "J";
    if (c == 1) return "Q";
    return "K";
}

inline std::vector<std::tuple<int, int, double>> hole_deals() {
    std::vector<std::tuple<int, int, double>> deals;
    for (int c0 = 0; c0 < 3; ++c0) {
        for (int c1 = 0; c1 < 3; ++c1) {
            // 6 card deck {J,J,Q,Q,K,K}: same rank pairs are 2/30, mixed pairs 4/30
            deals.push_back({c0, c1, c0 == c1 ? 2.0 / 30.0 : 4.0 / 30.0});
        }
    }
    return deals;
}
