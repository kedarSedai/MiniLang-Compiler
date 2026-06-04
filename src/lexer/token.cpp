#include "minilang/lexer/token.hpp"

#include <sstream>
#include <unordered_map>

namespace minilang {

std::string SourceLocation::to_string() const {
    std::ostringstream out;
    out << filename << ":" << line << ":" << column;
    return out.str();
}

bool Token::is_one_of(std::initializer_list<TokenKind> kinds) const {
    for (TokenKind k : kinds) {
        if (kind == k) {
            return true;
        }
    }
    return false;
}

const char* token_kind_name(TokenKind kind) {
    switch (kind) {
        case TokenKind::Eof: return "EOF";
        case TokenKind::Invalid: return "INVALID";
        case TokenKind::Identifier: return "IDENT";
        case TokenKind::IntLiteral: return "INT";
        case TokenKind::KwInt: return "KW_INT";
        case TokenKind::KwBool: return "KW_BOOL";
        case TokenKind::KwVoid: return "KW_VOID";
        case TokenKind::KwIf: return "KW_IF";
        case TokenKind::KwElse: return "KW_ELSE";
        case TokenKind::KwWhile: return "KW_WHILE";
        case TokenKind::KwReturn: return "KW_RETURN";
        case TokenKind::KwTrue: return "KW_TRUE";
        case TokenKind::KwFalse: return "KW_FALSE";
        case TokenKind::Plus: return "PLUS";
        case TokenKind::Minus: return "MINUS";
        case TokenKind::Star: return "STAR";
        case TokenKind::Slash: return "SLASH";
        case TokenKind::Percent: return "PERCENT";
        case TokenKind::Assign: return "ASSIGN";
        case TokenKind::EqEq: return "EQEQ";
        case TokenKind::NotEq: return "NOTEQ";
        case TokenKind::Lt: return "LT";
        case TokenKind::Le: return "LE";
        case TokenKind::Gt: return "GT";
        case TokenKind::Ge: return "GE";
        case TokenKind::AndAnd: return "ANDAND";
        case TokenKind::OrOr: return "OROR";
        case TokenKind::Bang: return "BANG";
        case TokenKind::LParen: return "LPAREN";
        case TokenKind::RParen: return "RPAREN";
        case TokenKind::LBrace: return "LBRACE";
        case TokenKind::RBrace: return "RBRACE";
        case TokenKind::LBracket: return "LBRACKET";
        case TokenKind::RBracket: return "RBRACKET";
        case TokenKind::Semicolon: return "SEMICOLON";
        case TokenKind::Comma: return "COMMA";
    }
    return "UNKNOWN";
}

TokenKind classify_keyword(std::string_view spelling) {
    static const std::unordered_map<std::string_view, TokenKind> table = {
        {"int", TokenKind::KwInt},
        {"bool", TokenKind::KwBool},
        {"void", TokenKind::KwVoid},
        {"if", TokenKind::KwIf},
        {"else", TokenKind::KwElse},
        {"while", TokenKind::KwWhile},
        {"return", TokenKind::KwReturn},
        {"true", TokenKind::KwTrue},
        {"false", TokenKind::KwFalse},
    };

    auto it = table.find(spelling);
    if (it != table.end()) {
        return it->second;
    }
    return TokenKind::Identifier;
}

}  // namespace minilang
