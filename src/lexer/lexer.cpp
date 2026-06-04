#include "minilang/lexer/lexer.hpp"

#include <cctype>
#include <stdexcept>

namespace minilang {

namespace {

bool is_identifier_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool is_identifier_part(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

}  // namespace

Lexer::Lexer(std::string source, std::string filename)
    : source_(std::move(source)), filename_(std::move(filename)) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    for (;;) {
        Token tok = next_token();
        tokens.push_back(tok);
        if (tok.kind == TokenKind::Eof || tok.kind == TokenKind::Invalid) {
            break;
        }
    }
    return tokens;
}

Token Lexer::next_token() {
    skip_whitespace_and_comments();
    start_ = current_;
    token_start_line_ = line_;
    token_start_column_ = column_;

    if (is_at_end()) {
        return make_token(TokenKind::Eof, "");
    }

    char c = advance();

    if (is_identifier_start(c)) {
        return scan_identifier();
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
        return scan_number();
    }

    switch (c) {
        case '(': return make_token(TokenKind::LParen, "(");
        case ')': return make_token(TokenKind::RParen, ")");
        case '{': return make_token(TokenKind::LBrace, "{");
        case '}': return make_token(TokenKind::RBrace, "}");
        case '[': return make_token(TokenKind::LBracket, "[");
        case ']': return make_token(TokenKind::RBracket, "]");
        case ';': return make_token(TokenKind::Semicolon, ";");
        case ',': return make_token(TokenKind::Comma, ",");
        case '+': return make_token(TokenKind::Plus, "+");
        case '-': return make_token(TokenKind::Minus, "-");
        case '*': return make_token(TokenKind::Star, "*");
        case '%': return make_token(TokenKind::Percent, "%");
        case '!':
            if (match('=')) {
                return make_token(TokenKind::NotEq, "!=");
            }
            return make_token(TokenKind::Bang, "!");
        case '=':
            if (match('=')) {
                return make_token(TokenKind::EqEq, "==");
            }
            return make_token(TokenKind::Assign, "=");
        case '<':
            if (match('=')) {
                return make_token(TokenKind::Le, "<=");
            }
            return make_token(TokenKind::Lt, "<");
        case '>':
            if (match('=')) {
                return make_token(TokenKind::Ge, ">=");
            }
            return make_token(TokenKind::Gt, ">");
        case '/':
            if (match('/')) {
                while (!is_at_end() && peek() != '\n') {
                    advance();
                }
                return next_token();
            }
            if (match('*')) {
                while (!is_at_end()) {
                    if (peek() == '*' && peek_next() == '/') {
                        advance();
                        advance();
                        break;
                    }
                    if (peek() == '\n') {
                        ++line_;
                        column_ = 1;
                    }
                    advance();
                }
                if (is_at_end()) {
                    return error_token("unterminated block comment");
                }
                return next_token();
            }
            return make_token(TokenKind::Slash, "/");
        case '&':
            if (match('&')) {
                return make_token(TokenKind::AndAnd, "&&");
            }
            return error_token("unexpected '&' (use &&)");
        case '|':
            if (match('|')) {
                return make_token(TokenKind::OrOr, "||");
            }
            return error_token("unexpected '|' (use ||)");
        case '"':
            return scan_string();
        default:
            break;
    }

    return error_token(std::string("unexpected character '") + c + "'");
}

char Lexer::peek() const {
    if (is_at_end()) {
        return '\0';
    }
    return source_[current_];
}

char Lexer::peek_next() const {
    if (current_ + 1 >= source_.size()) {
        return '\0';
    }
    return source_[current_ + 1];
}

char Lexer::advance() {
    char c = source_[current_++];
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (is_at_end() || source_[current_] != expected) {
        return false;
    }
    advance();
    return true;
}

bool Lexer::is_at_end() const {
    return current_ >= source_.size();
}

void Lexer::skip_whitespace_and_comments() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                advance();
                break;
            default:
                return;
        }
    }
}

Token Lexer::make_token(TokenKind kind, std::string lexeme) {
    Token tok;
    tok.kind = kind;
    tok.lexeme = std::move(lexeme);
    tok.location.filename = filename_;
    tok.location.line = token_start_line_;
    tok.location.column = token_start_column_;
    return tok;
}

Token Lexer::error_token(std::string message) {
    had_error_ = true;
    Token tok;
    tok.kind = TokenKind::Invalid;
    tok.lexeme = std::move(message);
    tok.location = current_location();
    return tok;
}

Token Lexer::scan_identifier() {
    while (is_identifier_part(peek())) {
        advance();
    }
    std::string text(source_.substr(start_, current_ - start_));
    TokenKind kind = classify_keyword(text);
    Token tok = make_token(kind, text);
    if (kind == TokenKind::KwTrue) {
        tok.int_value = 1;
    } else if (kind == TokenKind::KwFalse) {
        tok.int_value = 0;
    }
    return tok;
}

Token Lexer::scan_number() {
    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    std::string text(source_.substr(start_, current_ - start_));
    Token tok = make_token(TokenKind::IntLiteral, text);
    try {
        tok.int_value = std::stoll(text);
    } catch (const std::exception&) {
        return error_token("integer literal out of range");
    }
    return tok;
}

Token Lexer::scan_string() {
    // Placeholder for a future string type; unterminated strings are errors.
    while (!is_at_end() && peek() != '"') {
        if (peek() == '\n') {
            return error_token("unterminated string literal");
        }
        advance();
    }
    if (is_at_end()) {
        return error_token("unterminated string literal");
    }
    advance();  // closing quote
    std::string text(source_.substr(start_, current_ - start_));
    return make_token(TokenKind::Invalid, "string literals not supported yet: " + text);
}

SourceLocation Lexer::current_location() const {
    SourceLocation loc;
    loc.filename = filename_;
    loc.line = line_;
    loc.column = column_;
    return loc;
}

}  // namespace minilang
