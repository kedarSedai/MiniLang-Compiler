#include "minilang/lexer/lexer.hpp"
#include "minilang/parser/parser.hpp"

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
    std::cerr << "Usage: minilang_parser <file.minilang>\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        print_usage();
        return 1;
    }

    const std::string path = argv[1];
    try {
        const std::string source = read_file(path);
        minilang::Lexer lexer(source, path);
        if (lexer.had_error()) {
            return 2;
        }

        minilang::Parser parser(lexer);
        const minilang::Program program = parser.parse();
        minilang::dump_program(std::cout, program);

        if (lexer.had_error() || parser.had_error()) {
            return 2;
        }
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
