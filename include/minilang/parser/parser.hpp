#pragma once

#include "minilang/ast/ast.hpp"
#include "minilang/lexer/lexer.hpp"

namespace minilang {

/// Recursive-descent parser for MiniLang.
class Parser {
public:
    explicit Parser(Lexer& lexer);

    Program parse();

    bool had_error() const { return had_error_; }

private:
    void advance();
    bool check(TokenKind kind) const;
    bool match(TokenKind kind);
    Token consume(TokenKind kind, const char* message);
    void synchronize();

    void error(const SourceLocation& loc, const std::string& message);
    void error_at_current(const std::string& message);

    TypeKind parse_type();
    std::unique_ptr<FunctionDecl> parse_function();
    std::vector<Param> parse_parameters();
    std::unique_ptr<BlockStmt> parse_block();
    std::unique_ptr<Stmt> parse_statement();
    std::unique_ptr<Stmt> parse_var_decl();
    std::unique_ptr<Stmt> parse_if_statement();
    std::unique_ptr<Stmt> parse_while_statement();
    std::unique_ptr<Stmt> parse_return_statement();

    std::unique_ptr<Expr> parse_expression();
    std::unique_ptr<Expr> parse_logical_or();
    std::unique_ptr<Expr> parse_logical_and();
    std::unique_ptr<Expr> parse_equality();
    std::unique_ptr<Expr> parse_comparison();
    std::unique_ptr<Expr> parse_term();
    std::unique_ptr<Expr> parse_factor();
    std::unique_ptr<Expr> parse_unary();
    std::unique_ptr<Expr> parse_call();
    std::unique_ptr<Expr> finish_call(const std::string& name, const SourceLocation& loc);
    std::unique_ptr<Expr> parse_primary();

    Lexer& lexer_;
    Token current_;
    bool had_error_ = false;
    bool panic_mode_ = false;
};

}  // namespace minilang
