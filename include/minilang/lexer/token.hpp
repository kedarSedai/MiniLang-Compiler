#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace minilang {

enum class TokenKind : std::uint16_t {
    // End / error
    Eof,
    Invalid,

    // Literals & names
    Identifier,
    IntLiteral,

    // Keywords
    KwInt,
    KwBool,
    KwVoid,
    KwIf,
    KwElse,
    KwWhile,
    KwReturn,
    KwTrue,
    KwFalse,

    // Operators & punctuation
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Assign,       // =
    EqEq,         // ==
    NotEq,        // !=
    Lt,
    Le,
    Gt,
    Ge,
    AndAnd,       // &&
    OrOr,         // ||
    Bang,         // !
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Semicolon,
    Comma,
};

struct SourceLocation {
    std::string filename;
    int line = 1;
    int column = 1;

    std::string to_string() const;
};

struct Token {
    TokenKind kind = TokenKind::Eof;
    std::string lexeme;
    SourceLocation location;
    std::optional<int64_t> int_value;

    bool is(TokenKind k) const { return kind == k; }
    bool is_one_of(std::initializer_list<TokenKind> kinds) const;
};

const char* token_kind_name(TokenKind kind);

/// Classify identifier spelling; returns Keyword kind or Identifier.
TokenKind classify_keyword(std::string_view spelling);

}  // namespace minilang
