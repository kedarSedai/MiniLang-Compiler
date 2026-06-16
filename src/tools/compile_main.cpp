#include "minilang/codegen/llvm_emitter.hpp"
#include "minilang/hir/hir_lower.hpp"
#include "minilang/hir/optimize.hpp"
#include "minilang/lexer/lexer.hpp"
#include "minilang/parser/parser.hpp"
#include "minilang/semantic/semantic_analyzer.hpp"

#include <cstdlib>
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

void print_usage() {
    std::cerr << "Usage: minilang_compile <file.minilang> [-o output.ll] [--run]\n";
}

std::string default_output_path(const std::string& input) {
    const std::string suffix = ".minilang";
    if (input.size() >= suffix.size() &&
        input.compare(input.size() - suffix.size(), suffix.size(), suffix) == 0) {
        return input.substr(0, input.size() - suffix.size()) + ".ll";
    }
    return input + ".ll";
}

int run_clang(const std::string& ll_path, const std::string& exe_path) {
    const std::string cmd =
        "clang " + ll_path + " -o " + exe_path + " -Wno-override-module 2>&1";
    std::cerr << "running: " << cmd << '\n';
    return std::system(cmd.c_str());
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const std::string input = argv[1];
    std::string output = default_output_path(input);
    bool run_after = false;

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--run") {
            run_after = true;
        } else if (arg == "-o" && i + 1 < argc) {
            output = argv[++i];
        } else {
            std::cerr << "unknown argument: " << arg << '\n';
            print_usage();
            return 1;
        }
    }

    try {
        const std::string source = read_file(input);
        minilang::Lexer lexer(source, input);
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
        optimizer.run();

        std::ofstream out(output);
        if (!out) {
            throw std::runtime_error("could not write: " + output);
        }

        minilang::LlvmEmitter emitter(module);
        emitter.emit(out);
        out.close();

        std::cout << "LLVM IR written to " << output << '\n';

        if (run_after) {
            const std::string exe = "build/a.out";
            if (run_clang(output, exe) != 0) {
                std::cerr << "clang failed to compile " << output << '\n';
                return 3;
            }
            const int exit_code = std::system(exe.c_str());
            if (exit_code != 0) {
                std::cerr << "program exited with code " << exit_code << '\n';
                return exit_code;
            }
            std::cout << "program ran successfully (exit 0)\n";
        }
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
