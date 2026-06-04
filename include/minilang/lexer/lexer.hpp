#pragma once

#include "minilang/lexer/token.hpp"

#include <string>
#include <vector>

namespace minilang {

/// Scans MiniLang source into a token stream.
class Lexer {
public:
    explicit Lexer(std::string source, std::string filename = "<input>");

    /// Returns the next token and advances.
    Token next_token();

    /// All tokens until EOF (excluding Invalid unless present in source).
    std::vector<Token> tokenize();

    bool had_error() const { return had_error_; }

private:
    char peek() const;
    char peek_next() const;
    char advance();
    bool match(char expected);
    bool is_at_end() const;

    void skip_whitespace_and_comments();
    Token make_token(TokenKind kind, std::string lexeme);
    Token error_token(std::string message);

    Token scan_identifier();
    Token scan_number();
    Token scan_string();  // reserved for future string literals

    SourceLocation current_location() const;

    std::string source_;
    std::string filename_;
    std::size_t current_ = 0;
    std::size_t start_ = 0;
    int line_ = 1;
    int column_ = 1;
    int token_start_line_ = 1;
    int token_start_column_ = 1;
    bool had_error_ = false;
};

}  // namespace minilang
