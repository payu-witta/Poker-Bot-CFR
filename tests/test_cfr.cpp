#include "kuhn.h"
#include "leduc.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <unordered_map>

static int passed = 0;
static int failed = 0;

struct TestFailure {
    std::string msg;
};

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) throw TestFailure{std::string(msg) + " (line " + std::to_string(__LINE__) + ")"}; \
    } while (0)

#define ASSERT_NEAR(a, b, tol, msg) \
    do { \
        if (std::abs((a) - (b)) > (tol)) { \
            std::stringstream ss; \
            ss << msg << ": got " << (a) << ", expected " << (b) << " (line " << __LINE__ << ")"; \
            throw TestFailure{ss.str()}; \
        } \
    } while (0)

void run_test(const std::string& name, std::function<void()> fn) {
    std::cout << "[" << name << "]\n";
    try {
        fn();
        std::cout << "  PASS\n\n";
        ++passed;
    } catch (const TestFailure& e) {
        std::cout << "  FAIL: " << e.msg << "\n\n";
        ++failed;
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << "\n\n";
        ++failed;
    }
}

int count_terminals(const std::shared_ptr<GameNode>& node) {
    if (node->is_terminal()) return 1;
    int n = 0;
    if (node->is_chance_node()) {
        for (auto& [next, prob] : node->chance_outcomes()) n += count_terminals(next);
        return n;
    }
    for (auto& a : node->legal_actions()) n += count_terminals(node->apply_action(a));
    return n;
}

void collect_terminals(const std::shared_ptr<GameNode>& node, std::vector<std::shared_ptr<GameNode>>& out) {
    if (node->is_terminal()) {
        out.push_back(node);
        return;
    }
    if (node->is_chance_node()) {
        for (auto& [next, prob] : node->chance_outcomes()) collect_terminals(next, out);
        return;
    }
    for (auto& a : node->legal_actions()) collect_terminals(node->apply_action(a), out);
}

void collect_info_set_keys(const std::shared_ptr<GameNode>& node, std::unordered_map<std::string, int>& keys) {
    if (node->is_terminal()) return;
    if (node->is_chance_node()) {
        for (auto& [next, prob] : node->chance_outcomes()) collect_info_set_keys(next, keys);
        return;
    }
    keys[node->information_set_key()]++;
    for (auto& a : node->legal_actions()) collect_info_set_keys(node->apply_action(a), keys);
}

void test_kuhn_terminal_count() {
    auto root = std::make_shared<KuhnNode>(-1, -1, "");
    int n = count_terminals(root);
    std::cout << "  terminal nodes: " << n << "\n";
    ASSERT(n == 45, "expected 9 deals x 5 action sequences, got " + std::to_string(n));
}

void test_kuhn_zero_sum() {
    auto root = std::make_shared<KuhnNode>(-1, -1, "");
    std::vector<std::shared_ptr<GameNode>> terms;
    collect_terminals(root, terms);
    for (auto& t : terms) ASSERT_NEAR(t->utility(0) + t->utility(1), 0.0, 1e-9, "zero-sum violated");
    std::cout << "  zero-sum at " << terms.size() << " terminals\n";
}

void test_kuhn_fold_utilities() {
    auto bf = std::make_shared<KuhnNode>(2, 0, "bf");
    ASSERT(bf->is_terminal(), "bf must be terminal");
    ASSERT_NEAR(bf->utility(0), 1.0, 1e-9, "p0 wins the ante when p1 folds to a bet");
    ASSERT_NEAR(bf->utility(1), -1.0, 1e-9, "p1 loses the ante when folding");

    auto cbf = std::make_shared<KuhnNode>(0, 2, "cbf");
    ASSERT(cbf->is_terminal(), "cbf must be terminal");
    ASSERT_NEAR(cbf->utility(0), -1.0, 1e-9, "p0 loses the ante after folding to a bet");
    ASSERT_NEAR(cbf->utility(1), 1.0, 1e-9, "p1 wins the ante when p0 folds");
}

void test_kuhn_showdown_utilities() {
    auto cc = std::make_shared<KuhnNode>(2, 0, "cc");
    ASSERT_NEAR(cc->utility(0), 1.0, 1e-9, "K beats J at cc");
    ASSERT_NEAR(cc->utility(1), -1.0, 1e-9, "J loses to K at cc");

    auto bc = std::make_shared<KuhnNode>(0, 2, "bc");
    ASSERT_NEAR(bc->utility(0), -2.0, 1e-9, "J loses 2 at bc showdown");
    ASSERT_NEAR(bc->utility(1), 2.0, 1e-9, "K wins 2 at bc showdown");

    auto cbc = std::make_shared<KuhnNode>(1, 0, "cbc");
    ASSERT_NEAR(cbc->utility(0), 2.0, 1e-9, "Q beats J at cbc");
}

void test_kuhn_info_set_keys() {
    auto s1 = std::make_shared<KuhnNode>(2, 0, "");
    ASSERT(s1->information_set_key() == "K:", "p0 with K and no history should be K:");

    auto s2 = std::make_shared<KuhnNode>(2, 1, "b");
    ASSERT(s2->current_player() == 1, "p1 acts after p0 bets");
    ASSERT(s2->information_set_key() == "Q:b", "p1 with Q facing a bet should be Q:b");

    auto s3 = std::make_shared<KuhnNode>(0, 2, "cb");
    ASSERT(s3->current_player() == 0, "p0 acts at cb");
    ASSERT(s3->information_set_key() == "J:cb", "p0 with J at cb should be J:cb");
}

void test_kuhn_info_set_count() {
    auto root = std::make_shared<KuhnNode>(-1, -1, "");
    std::unordered_map<std::string, int> keys;
    collect_info_set_keys(root, keys);
    std::cout << "  info sets: " << keys.size() << "\n";
    ASSERT(keys.size() == 12, "expected 12 info sets, got " + std::to_string(keys.size()));
}

std::shared_ptr<GameNode> leduc_root() { return std::make_shared<LeducNode>(-1, -1, -1, 1, "", "", true, 0); }

void test_leduc_terminal_count() {
    int n = count_terminals(leduc_root());
    std::cout << "  terminal nodes: " << n << "\n";
    ASSERT(n == 1116, "expected 1116 terminals, got " + std::to_string(n));
}

void test_leduc_zero_sum() {
    std::vector<std::shared_ptr<GameNode>> terms;
    collect_terminals(leduc_root(), terms);
    for (auto& t : terms) ASSERT_NEAR(t->utility(0) + t->utility(1), 0.0, 1e-9, "zero-sum violated at leduc terminal");
    std::cout << "  zero-sum at " << terms.size() << " terminals\n";
}

void test_leduc_fold_utilities() {
    // p0 bets (3 in), p1 folds (1 in): p0 nets +1
    auto bf = std::make_shared<LeducNode>(2, 0, -1, 1, "bf", "");
    ASSERT(bf->is_terminal(), "bf must be terminal");
    ASSERT_NEAR(bf->utility(0), 1.0, 1e-9, "p0 wins 1 when p1 folds to a bet");
    ASSERT_NEAR(bf->utility(1), -1.0, 1e-9, "p1 loses 1 when folding");

    // p0 bets (3 in), p1 raises (5 in), p0 folds: p0 nets -3
    auto bbf = std::make_shared<LeducNode>(0, 2, -1, 1, "bbf", "");
    ASSERT(bbf->is_terminal(), "bbf must be terminal");
    ASSERT_NEAR(bbf->utility(0), -3.0, 1e-9, "p0 loses 3 after raise-fold");
    ASSERT_NEAR(bbf->utility(1), 3.0, 1e-9, "p1 wins 3 after raise-fold");
}

void test_leduc_showdown_winner() {
    auto pair = std::make_shared<LeducNode>(0, 2, 0, 2, "cc", "cc");
    ASSERT_NEAR(pair->utility(0), 1.0, 1e-9, "pair of J beats K high");
    ASSERT_NEAR(pair->utility(1), -1.0, 1e-9, "K high loses to pair");

    auto tie = std::make_shared<LeducNode>(1, 1, 0, 2, "cc", "cc");
    ASSERT_NEAR(tie->utility(0), 0.0, 1e-9, "Q vs Q ties for p0");
    ASSERT_NEAR(tie->utility(1), 0.0, 1e-9, "Q vs Q ties for p1");
}

void test_leduc_info_set_keys() {
    auto s1 = std::make_shared<LeducNode>(2, 0, -1, 1, "", "");
    ASSERT(s1->information_set_key() == "K:r1:", "round 1 p0 with K should be K:r1:");

    auto s2 = std::make_shared<LeducNode>(2, 1, -1, 1, "b", "");
    ASSERT(s2->current_player() == 1, "p1 acts after p0 bets");
    ASSERT(s2->information_set_key() == "Q:r1:b", "round 1 p1 with Q facing a bet should be Q:r1:b");

    auto s3 = std::make_shared<LeducNode>(1, 0, 2, 2, "cc", "");
    ASSERT(s3->information_set_key() == "Q:K:r2:cc|", "round 2 p0 with Q and K community should be Q:K:r2:cc|");

    auto s4 = std::make_shared<LeducNode>(0, 2, 2, 2, "bc", "b");
    ASSERT(s4->current_player() == 1, "p1 acts after p0 bets in round 2");
    ASSERT(s4->information_set_key() == "K:K:r2:bc|b", "round 2 p1 with K and K community should be K:K:r2:bc|b");
}

void test_leduc_info_set_count() {
    std::unordered_map<std::string, int> keys;
    collect_info_set_keys(leduc_root(), keys);
    std::cout << "  info sets: " << keys.size() << "\n";
    ASSERT(keys.size() == 288, "expected 288 info sets, got " + std::to_string(keys.size()));
}

void test_leduc_raise_cap() {
    auto has_bet = [](const LeducNode& n) {
        auto acts = n.legal_actions();
        return std::find(acts.begin(), acts.end(), "b") != acts.end();
    };
    ASSERT(!has_bet(LeducNode(2, 0, -1, 1, "bb", "")), "no raise after 2 bets");
    ASSERT(has_bet(LeducNode(2, 0, -1, 1, "b", "")), "raise should be allowed after 1 bet");
}

int main() {
    std::cout << "--- Kuhn Poker ---\n\n";
    run_test("terminal node count", test_kuhn_terminal_count);
    run_test("zero-sum property", test_kuhn_zero_sum);
    run_test("fold utilities", test_kuhn_fold_utilities);
    run_test("showdown utilities", test_kuhn_showdown_utilities);
    run_test("info set keys", test_kuhn_info_set_keys);
    run_test("info set count", test_kuhn_info_set_count);

    std::cout << "--- Leduc Hold'em ---\n\n";
    run_test("terminal node count", test_leduc_terminal_count);
    run_test("zero-sum property", test_leduc_zero_sum);
    run_test("fold utilities", test_leduc_fold_utilities);
    run_test("showdown winner", test_leduc_showdown_winner);
    run_test("info set keys", test_leduc_info_set_keys);
    run_test("info set count", test_leduc_info_set_count);
    run_test("raise cap", test_leduc_raise_cap);

    std::cout << "passed: " << passed << ", failed: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}
