#include "minilang/lexer/lexer.hpp"

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
    std::cerr << "Usage: minilang_lexer <file.minilang>\n";
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
        const std::vector<minilang::Token> tokens = lexer.tokenize();

        for (const minilang::Token& tok : tokens) {
            std::cout << minilang::token_kind_name(tok.kind);
            if (!tok.lexeme.empty() && tok.kind != minilang::TokenKind::Eof) {
                std::cout << " '" << tok.lexeme << "'";
            }
            if (tok.int_value.has_value() &&
                (tok.kind == minilang::TokenKind::IntLiteral ||
                 tok.kind == minilang::TokenKind::KwTrue ||
                 tok.kind == minilang::TokenKind::KwFalse)) {
                std::cout << " value=" << *tok.int_value;
            }
            std::cout << " @ " << tok.location.to_string() << '\n';
        }

        if (lexer.had_error()) {
            return 2;
        }
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
