#include "minilang/ml/eval_harness.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {

void print_usage() {
    std::cerr << "Usage: minilang_eval <file1.minilang> [file2.minilang ...]\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::vector<std::string> programs;
    programs.reserve(static_cast<std::size_t>(argc - 1));
    for (int i = 1; i < argc; ++i) {
        programs.emplace_back(argv[i]);
    }

    try {
        const minilang::EvalReport report = minilang::run_eval_harness(programs);
        minilang::dump_eval_report(std::cout, report);
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
