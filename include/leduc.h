#pragma once

#include "game.h"

#include <algorithm>
#include <unordered_map>

inline constexpr int LEDUC_BET_R1 = 2;
inline constexpr int LEDUC_BET_R2 = 4;

class LeducNode : public GameNode {
public:
    int p0_card;
    int p1_card;
    int community;
    int round_num;
    std::string history_r1;
    std::string history_r2;
    bool chance;
    int chance_type;

    LeducNode(int p0, int p1, int comm, int rnd, std::string h1, std::string h2, bool is_chance = false, int ctype = 0)
        : p0_card(p0), p1_card(p1), community(comm), round_num(rnd), history_r1(std::move(h1)), history_r2(std::move(h2)), chance(is_chance), chance_type(ctype) {}

    static int count_bets(const std::string& h) { return static_cast<int>(std::count(h.begin(), h.end(), 'b')); }

    static bool round_complete(const std::string& h) {
        if (h.empty()) return false;
        if (h == "cc") return true;
        return h.back() == 'c' && h.find('b') != std::string::npos;
    }

    bool is_terminal() const override {
        if (history_r1.find('f') != std::string::npos) return true;
        if (history_r2.find('f') != std::string::npos) return true;
        return round_num == 2 && round_complete(history_r2);
    }

    bool is_chance_node() const override { return chance; }

    int current_player() const override {
        const std::string& h = round_num == 1 ? history_r1 : history_r2;
        return static_cast<int>(h.size()) % 2;
    }

    std::pair<double, double> contributions() const {
        double c[2] = {1.0, 1.0};
        auto add_round = [&](const std::string& h, int bet) {
            double level[2] = {0.0, 0.0};
            int actor = 0;
            for (char a : h) {
                if (a == 'f') break;
                double call = level[1 - actor] - level[actor];
                double amt = a == 'b' ? call + bet : call;
                level[actor] += amt;
                c[actor] += amt;
                actor = 1 - actor;
            }
        };
        add_round(history_r1, LEDUC_BET_R1);
        if (history_r1.find('f') == std::string::npos && !history_r2.empty()) add_round(history_r2, LEDUC_BET_R2);
        return {c[0], c[1]};
    }

    // -1 means tie
    int showdown_winner() const {
        bool p0_pair = p0_card == community;
        bool p1_pair = p1_card == community;
        if (p0_pair && !p1_pair) return 0;
        if (p1_pair && !p0_pair) return 1;
        if (p0_card > p1_card) return 0;
        if (p1_card > p0_card) return 1;
        return -1;
    }

    double utility(int player) const override {
        auto [c0, c1] = contributions();
        double total = c0 + c1;

        int winner;
        auto f1 = history_r1.find('f');
        auto f2 = history_r2.find('f');
        if (f1 != std::string::npos) {
            winner = 1 - static_cast<int>(f1) % 2;
        } else if (f2 != std::string::npos) {
            winner = 1 - static_cast<int>(f2) % 2;
        } else {
            winner = showdown_winner();
            if (winner == -1) return 0.0;
        }

        double mine = player == 0 ? c0 : c1;
        return winner == player ? total - mine : -mine;
    }

    std::vector<std::string> legal_actions() const override {
        const std::string& h = round_num == 1 ? history_r1 : history_r2;
        if (!h.empty() && h.back() == 'b') {
            std::vector<std::string> acts = {"f", "c"};
            if (count_bets(h) < 2) acts.push_back("b");
            return acts;
        }
        return {"c", "b"};
    }

    std::string information_set_key() const override {
        std::string card = card_name(current_player() == 0 ? p0_card : p1_card);
        if (round_num == 1) return card + ":r1:" + history_r1;
        return card + ":" + card_name(community) + ":r2:" + history_r1 + "|" + history_r2;
    }
};
