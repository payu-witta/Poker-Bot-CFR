#include "kuhn.h"
#include "exploit.h"
#include "leduc.h"
#include "solver.h"

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

void test_vanilla_kuhn_exploitability() {
    auto root = std::make_shared<KuhnNode>(-1, -1, "");
    CFRSolver solver;
    solver.train(root, 10000);
    double expl = compute_exploitability(root, solver.get_average_strategy());
    std::cout << "  vanilla exploitability after 10k iters: " << expl << "\n";
    ASSERT(expl < 0.01, "expected < 0.01, got " + std::to_string(expl));
}

void test_cfr_plus_kuhn_exploitability() {
    auto root = std::make_shared<KuhnNode>(-1, -1, "");
    CFRSolver solver(SolverMode::CFR_PLUS);
    solver.train(root, 1000);
    double expl = compute_exploitability(root, solver.get_average_strategy());
    std::cout << "  cfr+ exploitability after 1k iters: " << expl << "\n";
    ASSERT(expl < 0.01, "expected < 0.01, got " + std::to_string(expl));
}

void test_kuhn_nash_properties() {
    auto root = std::make_shared<KuhnNode>(-1, -1, "");
    CFRSolver solver;
    solver.train(root, 50000);
    auto strat = solver.get_average_strategy();
    const double tol = 0.05;

    ASSERT(strat.count("Q:"), "Q: info set missing");
    double q_bet = strat["Q:"].count("b") ? strat["Q:"]["b"] : 0.0;
    std::cout << "  p0 Q bet prob: " << q_bet << "\n";
    ASSERT(q_bet < tol, "p0 with Q should not bet");

    ASSERT(strat.count("K:b"), "K:b info set missing");
    double k_call = strat["K:b"]["c"];
    std::cout << "  p1 K call prob facing a bet: " << k_call << "\n";
    ASSERT(k_call > 1.0 - tol, "p1 with K should always call");

    ASSERT(strat.count("J:b"), "J:b info set missing");
    double j_fold = strat["J:b"]["f"];
    std::cout << "  p1 J fold prob facing a bet: " << j_fold << "\n";
    ASSERT(j_fold > 1.0 - tol, "p1 with J should always fold");
}

void test_cfr_plus_faster_than_vanilla() {
    // area under the exploitability curve over the same checkpoints
    std::vector<int> checkpoints = {10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000};
    auto root = std::make_shared<KuhnNode>(-1, -1, "");

    auto auc = [&](SolverMode mode) {
        CFRSolver solver(mode);
        double total = 0.0;
        int prev = 0;
        for (int cp : checkpoints) {
            solver.train(root, cp - prev);
            prev = cp;
            total += compute_exploitability(root, solver.get_average_strategy());
        }
        return total;
    };

    double vanilla = auc(SolverMode::VANILLA);
    double plus = auc(SolverMode::CFR_PLUS);
    std::cout << "  vanilla auc: " << vanilla << "\n  cfr+ auc: " << plus << "\n";
    ASSERT(plus < vanilla, "cfr+ should have a lower auc than vanilla");
}

void test_leduc_convergence() {
    auto root = leduc_root();
    for (auto [name, mode] : {std::pair{"vanilla", SolverMode::VANILLA}, std::pair{"cfr+", SolverMode::CFR_PLUS}}) {
        CFRSolver solver(mode);
        solver.train(root, 1000);
        double expl = compute_exploitability(root, solver.get_average_strategy());
        std::cout << "  " << name << " exploitability after 1k iters: " << expl << "\n";
        ASSERT(expl < 0.1, std::string(name) + " should converge on leduc, got " + std::to_string(expl));
    }
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

    std::cout << "--- CFR convergence ---\n\n";
    run_test("vanilla cfr on kuhn", test_vanilla_kuhn_exploitability);
    run_test("cfr+ on kuhn", test_cfr_plus_kuhn_exploitability);
    run_test("cfr+ faster than vanilla", test_cfr_plus_faster_than_vanilla);
    run_test("kuhn nash strategy properties", test_kuhn_nash_properties);
    run_test("leduc convergence", test_leduc_convergence);

    std::cout << "passed: " << passed << ", failed: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}
