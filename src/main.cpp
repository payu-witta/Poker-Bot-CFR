#include <algorithm>
#include <cmath>
#include <cstdlib>
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
    return 0;
}
