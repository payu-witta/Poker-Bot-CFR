#include "kuhn.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>

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

int main() {
    std::cout << "--- Kuhn Poker ---\n\n";
    run_test("terminal node count", test_kuhn_terminal_count);
    run_test("zero-sum property", test_kuhn_zero_sum);

    std::cout << "passed: " << passed << ", failed: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}
