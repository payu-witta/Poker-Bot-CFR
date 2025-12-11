#include "exploit.h"
#include "kuhn.h"
#include "leduc.h"
#include "solver.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Args {
    std::string game = "kuhn";
    std::string solver = "vanilla";
    int iterations = 10000;
    int eval_points = 60;
    std::string output;
    bool verbose = true;
};

Args parse_args(int argc, char* argv[]) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        std::string key = argv[i];
        bool has_val = i + 1 < argc;
        if (key == "--game" && has_val) args.game = argv[++i];
        else if (key == "--solver" && has_val) args.solver = argv[++i];
        else if (key == "--iterations" && has_val) args.iterations = std::stoi(argv[++i]);
        else if (key == "--eval-points" && has_val) args.eval_points = std::stoi(argv[++i]);
        else if (key == "--output" && has_val) args.output = argv[++i];
        else if (key == "--quiet") args.verbose = false;
        else if (key == "--help") {
            std::cout << "Usage: cfr_solver [options]\n"
                         "  --game        kuhn|leduc             (default: kuhn)\n"
                         "  --solver      vanilla|cfr_plus       (default: vanilla)\n"
                         "  --iterations  N                      (default: 10000)\n"
                         "  --eval-points N                      (default: 60)\n"
                         "  --output      path/to/output.csv     (default: stdout)\n"
                         "  --quiet       suppress progress output\n";
            std::exit(0);
        }
    }
    return args;
}

std::vector<int> log_checkpoints(int total, int n_points) {
    std::vector<int> pts;
    for (int i = 0; i < n_points; ++i) {
        double t = std::pow(static_cast<double>(total), static_cast<double>(i) / (n_points - 1));
        int cp = std::max(1, static_cast<int>(std::round(t)));
        if (pts.empty() || cp != pts.back()) pts.push_back(cp);
    }
    if (pts.back() != total) pts.push_back(total);
    return pts;
}

int main(int argc, char* argv[]) {
    Args args = parse_args(argc, argv);

    if (args.game != "kuhn" && args.game != "leduc") {
        std::cerr << "Error: --game must be 'kuhn' or 'leduc'\n";
        return 1;
    }
    if (args.solver != "vanilla" && args.solver != "cfr_plus") {
        std::cerr << "Error: --solver must be 'vanilla' or 'cfr_plus'\n";
        return 1;
    }

    std::shared_ptr<GameNode> root;
    if (args.game == "kuhn") root = std::make_shared<KuhnNode>(-1, -1, "");
    else root = std::make_shared<LeducNode>(-1, -1, -1, 1, "", "", true, 0);

    CFRSolver solver(args.solver == "cfr_plus" ? SolverMode::CFR_PLUS : SolverMode::VANILLA);

    if (args.verbose) {
        std::cout << "Game: " << (args.game == "kuhn" ? "Kuhn Poker" : "Leduc Hold'em") << "\n"
                  << "Solver: " << (args.solver == "cfr_plus" ? "CFR+" : "Vanilla CFR") << "\n"
                  << "Iters: " << args.iterations << "\n"
                  << "Output: " << (args.output.empty() ? "stdout" : args.output) << "\n"
                  << std::string(40, '-') << "\n";
    }

    auto start = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); };

    std::ofstream file;
    std::ostream* out = &std::cout;
    if (!args.output.empty()) {
        std::filesystem::path p(args.output);
        if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
        file.open(args.output);
        if (!file) {
            std::cerr << "Error: cannot open output file: " << args.output << "\n";
            return 1;
        }
    }

    *out << "iteration,exploitability\n";
    int prev = 0;
    for (int cp : log_checkpoints(args.iterations, args.eval_points)) {
        if (cp > prev) solver.train(root, cp - prev);
        prev = cp;
        double expl = compute_exploitability(root, solver.get_average_strategy());
        *out << cp << "," << std::fixed << std::setprecision(8) << expl << "\n";

        if (args.verbose) {
            std::cout << "  iter " << std::setw(8) << cp << " | exploitability: " << std::fixed << std::setprecision(6) << std::setw(10) << expl
                      << " | elapsed: " << std::setprecision(1) << elapsed() << "s" << std::endl;
        }
    }

    if (args.verbose) std::cout << std::string(40, '-') << "\nDone. Total time: " << std::fixed << std::setprecision(2) << elapsed() << "s\n";
    return 0;
}
