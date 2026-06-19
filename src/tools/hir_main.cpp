#include "minilang/hir/hir.hpp"
#include "minilang/hir/hir_lower.hpp"
#include "minilang/hir/optimize.hpp"
#include "minilang/lexer/lexer.hpp"
#include "minilang/parser/parser.hpp"
#include "minilang/semantic/semantic_analyzer.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string read_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("could not open file: " + path);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: minilang_hir <file.minilang>\n";
        return 1;
    }

    const std::string path = argv[1];
    try {
        const std::string source = read_file(path);
        minilang::Lexer lexer(source, path);
        minilang::Parser parser(lexer);
        const minilang::Program program = parser.parse();
        if (lexer.had_error() || parser.had_error()) {
            return 2;
        }

        minilang::SemanticAnalyzer analyzer(program);
        if (!analyzer.analyze()) {
            return 2;
        }

        minilang::HirLowerer lowerer;
        minilang::HirModule module = lowerer.lower(program);

        minilang::HirOptimizer optimizer(module);
        const minilang::OptimizationStats stats = optimizer.run();

        std::cout << "HIR for " << path << '\n';
        std::cout << "optimizations: constant_folds=" << stats.constant_folds
                  << " algebraic=" << stats.algebraic_simplifications
                  << " dead_temps_removed=" << stats.dead_temps_removed << '\n';
        minilang::dump_hir_module(std::cout, module);
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
