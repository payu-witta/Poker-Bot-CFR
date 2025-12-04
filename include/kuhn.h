#pragma once

#include "game.h"

class KuhnNode : public GameNode {
public:
    int p0_card;
    int p1_card;
    std::string history;

    KuhnNode(int p0, int p1, std::string hist) : p0_card(p0), p1_card(p1), history(std::move(hist)) {}

    bool is_terminal() const override {
        if (history.empty()) return false;
        if (history.find('f') != std::string::npos) return true;
        return history == "cc" || history == "bc" || history == "cbc";
    }

    bool is_chance_node() const override { return p0_card == -1; }

    int current_player() const override { return static_cast<int>(history.size()) % 2; }

    double utility(int player) const override {
        auto f = history.find('f');
        if (f != std::string::npos) {
            int winner = 1 - static_cast<int>(f) % 2;
            return winner == player ? 1.0 : -1.0;
        }
        double pot = (history == "bc" || history == "cbc") ? 2.0 : 1.0;
        int winner = p0_card > p1_card ? 0 : 1;
        return winner == player ? pot : -pot;
    }

    std::vector<std::pair<std::shared_ptr<GameNode>, double>> chance_outcomes() const override {
        std::vector<std::pair<std::shared_ptr<GameNode>, double>> out;
        for (auto [c0, c1, prob] : hole_deals()) {
            out.push_back({std::make_shared<KuhnNode>(c0, c1, ""), prob});
        }
        return out;
    }

    std::vector<std::string> legal_actions() const override {
        if (history.empty() || history.back() == 'c') return {"c", "b"};
        return {"f", "c"};
    }

    std::shared_ptr<GameNode> apply_action(const std::string& action) const override {
        return std::make_shared<KuhnNode>(p0_card, p1_card, history + action);
    }

    std::string information_set_key() const override {
        int card = current_player() == 0 ? p0_card : p1_card;
        return card_name(card) + ":" + history;
    }
};
