#include "minilang/parser/parser.hpp"

#include <iostream>
#include <utility>

namespace minilang {

Parser::Parser(Lexer& lexer) : lexer_(lexer) {
    advance();
}

void Parser::advance() {
    current_ = lexer_.next_token();
    if (current_.kind == TokenKind::Invalid) {
        error(current_.location, current_.lexeme);
    }
}

bool Parser::check(TokenKind kind) const {
    return current_.kind == kind;
}

bool Parser::match(TokenKind kind) {
    if (!check(kind)) {
        return false;
    }
    advance();
    return true;
}

Token Parser::consume(TokenKind kind, const char* message) {
    if (check(kind)) {
        Token tok = current_;
        advance();
        return tok;
    }
    error_at_current(message);
    return current_;
}

void Parser::error(const SourceLocation& loc, const std::string& message) {
    if (panic_mode_) {
        return;
    }
    panic_mode_ = true;
    had_error_ = true;
    std::cerr << "parse error at " << loc.to_string() << ": " << message << '\n';
}

void Parser::error_at_current(const std::string& message) {
    error(current_.location, message);
}

void Parser::synchronize() {
    panic_mode_ = false;
    while (current_.kind != TokenKind::Eof) {
        if (current_.kind == TokenKind::Semicolon) {
            advance();
            return;
        }
        if (current_.kind == TokenKind::RBrace) {
            return;
        }
        advance();
    }
}

TypeKind Parser::parse_type() {
    if (match(TokenKind::KwInt)) {
        return TypeKind::Int;
    }
    if (match(TokenKind::KwBool)) {
        return TypeKind::Bool;
    }
    if (match(TokenKind::KwVoid)) {
        return TypeKind::Void;
    }
    error_at_current("expected type (int, bool, or void)");
    return TypeKind::Int;
}

Program Parser::parse() {
    Program program;
    program.location = current_.location;

    while (!check(TokenKind::Eof)) {
        if (check(TokenKind::KwInt) || check(TokenKind::KwBool) ||
            check(TokenKind::KwVoid)) {
            std::unique_ptr<FunctionDecl> func = parse_function();
            if (func) {
                program.functions.push_back(std::move(func));
            }
        } else {
            error_at_current("expected function declaration");
            synchronize();
        }
    }

    return program;
}

std::unique_ptr<FunctionDecl> Parser::parse_function() {
    SourceLocation loc = current_.location;
    const TypeKind return_type = parse_type();

    if (!check(TokenKind::Identifier)) {
        error_at_current("expected function name");
        synchronize();
        return nullptr;
    }

    std::string name = current_.lexeme;
    advance();
    consume(TokenKind::LParen, "expected '(' after function name");

    std::vector<Param> parameters;
    if (!check(TokenKind::RParen)) {
        parameters = parse_parameters();
    }
    consume(TokenKind::RParen, "expected ')' after parameters");

    auto func = std::make_unique<FunctionDecl>();
    func->location = loc;
    func->return_type = return_type;
    func->name = std::move(name);
    func->parameters = std::move(parameters);
    func->body = parse_block();
    return func;
}

std::vector<Param> Parser::parse_parameters() {
    std::vector<Param> params;
    do {
        SourceLocation loc = current_.location;
        const TypeKind type = parse_type();
        if (!check(TokenKind::Identifier)) {
            error_at_current("expected parameter name");
            return params;
        }
        Param param;
        param.location = loc;
        param.type = type;
        param.name = current_.lexeme;
        advance();
        params.push_back(std::move(param));
    } while (match(TokenKind::Comma));
    return params;
}

std::unique_ptr<BlockStmt> Parser::parse_block() {
    SourceLocation loc = current_.location;
    consume(TokenKind::LBrace, "expected '{'");

    auto block = std::make_unique<BlockStmt>();
    block->location = loc;

    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        block->statements.push_back(parse_statement());
    }

    consume(TokenKind::RBrace, "expected '}'");
    return block;
}

std::unique_ptr<Stmt> Parser::parse_statement() {
    if (check(TokenKind::KwInt) || check(TokenKind::KwBool)) {
        return parse_var_decl();
    }
    if (check(TokenKind::KwIf)) {
        return parse_if_statement();
    }
    if (check(TokenKind::KwWhile)) {
        return parse_while_statement();
    }
    if (check(TokenKind::KwReturn)) {
        return parse_return_statement();
    }
    if (check(TokenKind::LBrace)) {
        return std::make_unique<Stmt>(std::move(*parse_block()));
    }

    SourceLocation loc = current_.location;
    std::unique_ptr<Expr> expr = parse_expression();
    consume(TokenKind::Semicolon, "expected ';' after expression");
    ExprStmt stmt;
    stmt.location = loc;
    stmt.expression = std::move(expr);
    return std::make_unique<Stmt>(std::move(stmt));
}

std::unique_ptr<Stmt> Parser::parse_var_decl() {
    SourceLocation loc = current_.location;
    const TypeKind type = parse_type();

    if (!check(TokenKind::Identifier)) {
        error_at_current("expected variable name");
        return nullptr;
    }

    std::string name = current_.lexeme;
    advance();
    consume(TokenKind::Assign, "expected '=' in variable declaration");
    std::unique_ptr<Expr> init = parse_expression();
    consume(TokenKind::Semicolon, "expected ';' after variable declaration");

    VarDeclStmt stmt;
    stmt.location = loc;
    stmt.type = type;
    stmt.name = std::move(name);
    stmt.initializer = std::move(init);
    return std::make_unique<Stmt>(std::move(stmt));
}

std::unique_ptr<Stmt> Parser::parse_if_statement() {
    SourceLocation loc = current_.location;
    consume(TokenKind::KwIf, "expected 'if'");
    consume(TokenKind::LParen, "expected '(' after 'if'");
    std::unique_ptr<Expr> condition = parse_expression();
    consume(TokenKind::RParen, "expected ')' after if condition");

    std::unique_ptr<Stmt> then_branch = parse_statement();
    std::unique_ptr<Stmt> else_branch;
    if (match(TokenKind::KwElse)) {
        else_branch = parse_statement();
    }

    IfStmt stmt;
    stmt.location = loc;
    stmt.condition = std::move(condition);
    stmt.then_branch = std::move(then_branch);
    stmt.else_branch = std::move(else_branch);
    return std::make_unique<Stmt>(std::move(stmt));
}

std::unique_ptr<Stmt> Parser::parse_while_statement() {
    SourceLocation loc = current_.location;
    consume(TokenKind::KwWhile, "expected 'while'");
    consume(TokenKind::LParen, "expected '(' after 'while'");
    std::unique_ptr<Expr> condition = parse_expression();
    consume(TokenKind::RParen, "expected ')' after while condition");

    std::unique_ptr<Stmt> body = parse_statement();

    WhileStmt stmt;
    stmt.location = loc;
    stmt.condition = std::move(condition);
    stmt.body = std::move(body);
    return std::make_unique<Stmt>(std::move(stmt));
}

std::unique_ptr<Stmt> Parser::parse_return_statement() {
    SourceLocation loc = current_.location;
    consume(TokenKind::KwReturn, "expected 'return'");

    std::unique_ptr<Expr> value;
    if (!check(TokenKind::Semicolon)) {
        value = parse_expression();
    }
    consume(TokenKind::Semicolon, "expected ';' after return");

    ReturnStmt stmt;
    stmt.location = loc;
    stmt.value = std::move(value);
    return std::make_unique<Stmt>(std::move(stmt));
}

std::unique_ptr<Expr> Parser::parse_expression() {
    return parse_logical_or();
}

std::unique_ptr<Expr> Parser::parse_logical_or() {
    std::unique_ptr<Expr> expr = parse_logical_and();
    while (check(TokenKind::OrOr)) {
        SourceLocation loc = current_.location;
        advance();
        std::unique_ptr<Expr> right = parse_logical_and();
        BinaryExpr bin;
        bin.location = loc;
        bin.op = BinaryOp::Or;
        bin.left = std::move(expr);
        bin.right = std::move(right);
        expr = std::make_unique<Expr>(std::move(bin));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_logical_and() {
    std::unique_ptr<Expr> expr = parse_equality();
    while (check(TokenKind::AndAnd)) {
        SourceLocation loc = current_.location;
        advance();
        std::unique_ptr<Expr> right = parse_equality();
        BinaryExpr bin;
        bin.location = loc;
        bin.op = BinaryOp::And;
        bin.left = std::move(expr);
        bin.right = std::move(right);
        expr = std::make_unique<Expr>(std::move(bin));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_equality() {
    std::unique_ptr<Expr> expr = parse_comparison();
    while (true) {
        BinaryOp op;
        if (check(TokenKind::EqEq)) {
            op = BinaryOp::Eq;
        } else if (check(TokenKind::NotEq)) {
            op = BinaryOp::Ne;
        } else {
            break;
        }
        SourceLocation loc = current_.location;
        advance();
        std::unique_ptr<Expr> right = parse_comparison();
        BinaryExpr bin;
        bin.location = loc;
        bin.op = op;
        bin.left = std::move(expr);
        bin.right = std::move(right);
        expr = std::make_unique<Expr>(std::move(bin));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_comparison() {
    std::unique_ptr<Expr> expr = parse_term();
    while (true) {
        BinaryOp op;
        if (check(TokenKind::Lt)) {
            op = BinaryOp::Lt;
        } else if (check(TokenKind::Le)) {
            op = BinaryOp::Le;
        } else if (check(TokenKind::Gt)) {
            op = BinaryOp::Gt;
        } else if (check(TokenKind::Ge)) {
            op = BinaryOp::Ge;
        } else {
            break;
        }
        SourceLocation loc = current_.location;
        advance();
        std::unique_ptr<Expr> right = parse_term();
        BinaryExpr bin;
        bin.location = loc;
        bin.op = op;
        bin.left = std::move(expr);
        bin.right = std::move(right);
        expr = std::make_unique<Expr>(std::move(bin));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_term() {
    std::unique_ptr<Expr> expr = parse_factor();
    while (true) {
        BinaryOp op;
        if (check(TokenKind::Plus)) {
            op = BinaryOp::Add;
        } else if (check(TokenKind::Minus)) {
            op = BinaryOp::Sub;
        } else {
            break;
        }
        SourceLocation loc = current_.location;
        advance();
        std::unique_ptr<Expr> right = parse_factor();
        BinaryExpr bin;
        bin.location = loc;
        bin.op = op;
        bin.left = std::move(expr);
        bin.right = std::move(right);
        expr = std::make_unique<Expr>(std::move(bin));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_factor() {
    std::unique_ptr<Expr> expr = parse_unary();
    while (true) {
        BinaryOp op;
        if (check(TokenKind::Star)) {
            op = BinaryOp::Mul;
        } else if (check(TokenKind::Slash)) {
            op = BinaryOp::Div;
        } else if (check(TokenKind::Percent)) {
            op = BinaryOp::Mod;
        } else {
            break;
        }
        SourceLocation loc = current_.location;
        advance();
        std::unique_ptr<Expr> right = parse_unary();
        BinaryExpr bin;
        bin.location = loc;
        bin.op = op;
        bin.left = std::move(expr);
        bin.right = std::move(right);
        expr = std::make_unique<Expr>(std::move(bin));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_unary() {
    if (check(TokenKind::Bang)) {
        const SourceLocation loc = current_.location;
        advance();
        std::unique_ptr<Expr> operand = parse_unary();
        UnaryExpr unary;
        unary.location = loc;
        unary.op = UnaryOp::Not;
        unary.operand = std::move(operand);
        return std::make_unique<Expr>(std::move(unary));
    }
    if (check(TokenKind::Minus)) {
        const SourceLocation loc = current_.location;
        advance();
        std::unique_ptr<Expr> operand = parse_unary();
        UnaryExpr unary;
        unary.location = loc;
        unary.op = UnaryOp::Neg;
        unary.operand = std::move(operand);
        return std::make_unique<Expr>(std::move(unary));
    }
    return parse_call();
}

std::unique_ptr<Expr> Parser::parse_call() {
    return parse_primary();
}

std::unique_ptr<Expr> Parser::finish_call(const std::string& name,
                                          const SourceLocation& loc) {
    std::vector<std::unique_ptr<Expr>> arguments;
    if (!check(TokenKind::RParen)) {
        do {
            arguments.push_back(parse_expression());
        } while (match(TokenKind::Comma));
    }
    consume(TokenKind::RParen, "expected ')' after arguments");

    CallExpr call;
    call.location = loc;
    call.callee = name;
    call.arguments = std::move(arguments);
    return std::make_unique<Expr>(std::move(call));
}

std::unique_ptr<Expr> Parser::parse_primary() {
    if (check(TokenKind::IntLiteral)) {
        const Token tok = current_;
        advance();
        IntLiteralExpr lit;
        lit.location = tok.location;
        lit.value = tok.int_value.value_or(0);
        return std::make_unique<Expr>(std::move(lit));
    }

    if (check(TokenKind::KwTrue)) {
        const Token tok = current_;
        advance();
        BoolLiteralExpr lit;
        lit.location = tok.location;
        lit.value = true;
        return std::make_unique<Expr>(std::move(lit));
    }

    if (check(TokenKind::KwFalse)) {
        const Token tok = current_;
        advance();
        BoolLiteralExpr lit;
        lit.location = tok.location;
        lit.value = false;
        return std::make_unique<Expr>(std::move(lit));
    }

    if (check(TokenKind::Identifier)) {
        std::string name = current_.lexeme;
        SourceLocation loc = current_.location;
        advance();
        if (check(TokenKind::LParen)) {
            advance();
            return finish_call(name, loc);
        }
        VariableExpr var;
        var.location = loc;
        var.name = std::move(name);
        return std::make_unique<Expr>(std::move(var));
    }

    if (match(TokenKind::LParen)) {
        std::unique_ptr<Expr> expr = parse_expression();
        consume(TokenKind::RParen, "expected ')' after expression");
        return expr;
    }

    error_at_current("expected expression");
    IntLiteralExpr lit;
    lit.location = current_.location;
    lit.value = 0;
    return std::make_unique<Expr>(std::move(lit));
}

}  // namespace minilang
