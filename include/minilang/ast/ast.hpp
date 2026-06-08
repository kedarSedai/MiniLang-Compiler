#pragma once

#include "minilang/lexer/token.hpp"

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace minilang {

enum class TypeKind { Int, Bool, Void };

enum class BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    And,
    Or,
};

enum class UnaryOp { Neg, Not };

struct Expr;
struct Stmt;

struct IntLiteralExpr {
    SourceLocation location;
    int64_t value = 0;
};

struct BoolLiteralExpr {
    SourceLocation location;
    bool value = false;
};

struct VariableExpr {
    SourceLocation location;
    std::string name;
};

struct BinaryExpr {
    SourceLocation location;
    BinaryOp op = BinaryOp::Add;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

struct UnaryExpr {
    SourceLocation location;
    UnaryOp op = UnaryOp::Neg;
    std::unique_ptr<Expr> operand;
};

struct CallExpr {
    SourceLocation location;
    std::string callee;
    std::vector<std::unique_ptr<Expr>> arguments;
};

struct Expr {
    enum class Kind {
        IntLiteral,
        BoolLiteral,
        Variable,
        Binary,
        Unary,
        Call,
    };

    Kind kind = Kind::IntLiteral;
    IntLiteralExpr int_literal;
    BoolLiteralExpr bool_literal;
    VariableExpr variable;
    BinaryExpr binary;
    UnaryExpr unary;
    CallExpr call;

    explicit Expr(IntLiteralExpr e);
    explicit Expr(BoolLiteralExpr e);
    explicit Expr(VariableExpr e);
    explicit Expr(BinaryExpr e);
    explicit Expr(UnaryExpr e);
    explicit Expr(CallExpr e);

    const SourceLocation& location() const;
};

struct BlockStmt {
    SourceLocation location;
    std::vector<std::unique_ptr<Stmt>> statements;
};

struct VarDeclStmt {
    SourceLocation location;
    TypeKind type = TypeKind::Int;
    std::string name;
    std::unique_ptr<Expr> initializer;
};

struct IfStmt {
    SourceLocation location;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> then_branch;
    std::unique_ptr<Stmt> else_branch;
};

struct WhileStmt {
    SourceLocation location;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
};

struct ReturnStmt {
    SourceLocation location;
    std::unique_ptr<Expr> value;
};

struct ExprStmt {
    SourceLocation location;
    std::unique_ptr<Expr> expression;
};

struct Stmt {
    enum class Kind { Block, VarDecl, If, While, Return, Expr };

    Kind kind = Kind::Block;
    BlockStmt block;
    VarDeclStmt var_decl;
    IfStmt if_stmt;
    WhileStmt while_stmt;
    ReturnStmt return_stmt;
    ExprStmt expr_stmt;

    explicit Stmt(BlockStmt s);
    explicit Stmt(VarDeclStmt s);
    explicit Stmt(IfStmt s);
    explicit Stmt(WhileStmt s);
    explicit Stmt(ReturnStmt s);
    explicit Stmt(ExprStmt s);

    const SourceLocation& location() const;
};

struct Param {
    SourceLocation location;
    TypeKind type = TypeKind::Int;
    std::string name;
};

struct FunctionDecl {
    SourceLocation location;
    TypeKind return_type = TypeKind::Int;
    std::string name;
    std::vector<Param> parameters;
    std::unique_ptr<BlockStmt> body;
};

struct Program {
    SourceLocation location;
    std::vector<std::unique_ptr<FunctionDecl>> functions;
};

const char* type_kind_name(TypeKind kind);
const char* binary_op_name(BinaryOp op);
const char* unary_op_name(UnaryOp op);

void dump_program(std::ostream& out, const Program& program, int indent = 0);

}  // namespace minilang
