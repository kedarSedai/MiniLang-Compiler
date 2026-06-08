#include "minilang/ast/ast.hpp"

namespace minilang {

namespace {

void write_indent(std::ostream& out, int indent) {
    for (int i = 0; i < indent; ++i) {
        out << "  ";
    }
}

void dump_location_suffix(std::ostream& out, const SourceLocation& loc) {
    out << " @ " << loc.to_string();
}

}  // namespace

Expr::Expr(IntLiteralExpr e) : kind(Kind::IntLiteral), int_literal(std::move(e)) {}
Expr::Expr(BoolLiteralExpr e) : kind(Kind::BoolLiteral), bool_literal(std::move(e)) {}
Expr::Expr(VariableExpr e) : kind(Kind::Variable), variable(std::move(e)) {}
Expr::Expr(BinaryExpr e) : kind(Kind::Binary), binary(std::move(e)) {}
Expr::Expr(UnaryExpr e) : kind(Kind::Unary), unary(std::move(e)) {}
Expr::Expr(CallExpr e) : kind(Kind::Call), call(std::move(e)) {}

const SourceLocation& Expr::location() const {
    switch (kind) {
        case Kind::IntLiteral:
            return int_literal.location;
        case Kind::BoolLiteral:
            return bool_literal.location;
        case Kind::Variable:
            return variable.location;
        case Kind::Binary:
            return binary.location;
        case Kind::Unary:
            return unary.location;
        case Kind::Call:
            return call.location;
    }
    return int_literal.location;
}

Stmt::Stmt(BlockStmt s) : kind(Kind::Block), block(std::move(s)) {}
Stmt::Stmt(VarDeclStmt s) : kind(Kind::VarDecl), var_decl(std::move(s)) {}
Stmt::Stmt(IfStmt s) : kind(Kind::If), if_stmt(std::move(s)) {}
Stmt::Stmt(WhileStmt s) : kind(Kind::While), while_stmt(std::move(s)) {}
Stmt::Stmt(ReturnStmt s) : kind(Kind::Return), return_stmt(std::move(s)) {}
Stmt::Stmt(ExprStmt s) : kind(Kind::Expr), expr_stmt(std::move(s)) {}

const SourceLocation& Stmt::location() const {
    switch (kind) {
        case Kind::Block:
            return block.location;
        case Kind::VarDecl:
            return var_decl.location;
        case Kind::If:
            return if_stmt.location;
        case Kind::While:
            return while_stmt.location;
        case Kind::Return:
            return return_stmt.location;
        case Kind::Expr:
            return expr_stmt.location;
    }
    return block.location;
}

const char* type_kind_name(TypeKind kind) {
    switch (kind) {
        case TypeKind::Int:
            return "int";
        case TypeKind::Bool:
            return "bool";
        case TypeKind::Void:
            return "void";
    }
    return "unknown";
}

const char* binary_op_name(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add:
            return "+";
        case BinaryOp::Sub:
            return "-";
        case BinaryOp::Mul:
            return "*";
        case BinaryOp::Div:
            return "/";
        case BinaryOp::Mod:
            return "%";
        case BinaryOp::Eq:
            return "==";
        case BinaryOp::Ne:
            return "!=";
        case BinaryOp::Lt:
            return "<";
        case BinaryOp::Le:
            return "<=";
        case BinaryOp::Gt:
            return ">";
        case BinaryOp::Ge:
            return ">=";
        case BinaryOp::And:
            return "&&";
        case BinaryOp::Or:
            return "||";
    }
    return "?";
}

const char* unary_op_name(UnaryOp op) {
    switch (op) {
        case UnaryOp::Neg:
            return "-";
        case UnaryOp::Not:
            return "!";
    }
    return "?";
}

static void dump_expr(std::ostream& out, const Expr& expr, int indent);
static void dump_stmt(std::ostream& out, const Stmt& stmt, int indent);

static void dump_block(std::ostream& out, const BlockStmt& block, int indent) {
    write_indent(out, indent);
    out << "Block";
    dump_location_suffix(out, block.location);
    out << '\n';
    for (const auto& inner : block.statements) {
        dump_stmt(out, *inner, indent + 1);
    }
}

static void dump_stmt(std::ostream& out, const Stmt& stmt, int indent) {
    write_indent(out, indent);
    switch (stmt.kind) {
        case Stmt::Kind::Block:
            dump_block(out, stmt.block, indent);
            break;
        case Stmt::Kind::VarDecl:
            out << "VarDecl " << type_kind_name(stmt.var_decl.type) << ' '
                << stmt.var_decl.name;
            dump_location_suffix(out, stmt.var_decl.location);
            out << '\n';
            write_indent(out, indent + 1);
            out << "init:\n";
            dump_expr(out, *stmt.var_decl.initializer, indent + 2);
            break;
        case Stmt::Kind::If:
            out << "If";
            dump_location_suffix(out, stmt.if_stmt.location);
            out << '\n';
            write_indent(out, indent + 1);
            out << "cond:\n";
            dump_expr(out, *stmt.if_stmt.condition, indent + 2);
            write_indent(out, indent + 1);
            out << "then:\n";
            dump_stmt(out, *stmt.if_stmt.then_branch, indent + 2);
            if (stmt.if_stmt.else_branch) {
                write_indent(out, indent + 1);
                out << "else:\n";
                dump_stmt(out, *stmt.if_stmt.else_branch, indent + 2);
            }
            break;
        case Stmt::Kind::While:
            out << "While";
            dump_location_suffix(out, stmt.while_stmt.location);
            out << '\n';
            write_indent(out, indent + 1);
            out << "cond:\n";
            dump_expr(out, *stmt.while_stmt.condition, indent + 2);
            write_indent(out, indent + 1);
            out << "body:\n";
            dump_stmt(out, *stmt.while_stmt.body, indent + 2);
            break;
        case Stmt::Kind::Return:
            out << "Return";
            dump_location_suffix(out, stmt.return_stmt.location);
            out << '\n';
            if (stmt.return_stmt.value) {
                write_indent(out, indent + 1);
                out << "value:\n";
                dump_expr(out, *stmt.return_stmt.value, indent + 2);
            }
            break;
        case Stmt::Kind::Expr:
            out << "ExprStmt";
            dump_location_suffix(out, stmt.expr_stmt.location);
            out << '\n';
            dump_expr(out, *stmt.expr_stmt.expression, indent + 1);
            break;
    }
}

static void dump_expr(std::ostream& out, const Expr& expr, int indent) {
    write_indent(out, indent);
    switch (expr.kind) {
        case Expr::Kind::IntLiteral:
            out << "IntLiteral " << expr.int_literal.value;
            dump_location_suffix(out, expr.int_literal.location);
            out << '\n';
            break;
        case Expr::Kind::BoolLiteral:
            out << "BoolLiteral " << (expr.bool_literal.value ? "true" : "false");
            dump_location_suffix(out, expr.bool_literal.location);
            out << '\n';
            break;
        case Expr::Kind::Variable:
            out << "Variable " << expr.variable.name;
            dump_location_suffix(out, expr.variable.location);
            out << '\n';
            break;
        case Expr::Kind::Binary:
            out << "Binary " << binary_op_name(expr.binary.op);
            dump_location_suffix(out, expr.binary.location);
            out << '\n';
            write_indent(out, indent + 1);
            out << "left:\n";
            dump_expr(out, *expr.binary.left, indent + 2);
            write_indent(out, indent + 1);
            out << "right:\n";
            dump_expr(out, *expr.binary.right, indent + 2);
            break;
        case Expr::Kind::Unary:
            out << "Unary " << unary_op_name(expr.unary.op);
            dump_location_suffix(out, expr.unary.location);
            out << '\n';
            dump_expr(out, *expr.unary.operand, indent + 1);
            break;
        case Expr::Kind::Call:
            out << "Call " << expr.call.callee << '(' << expr.call.arguments.size() << ')';
            dump_location_suffix(out, expr.call.location);
            out << '\n';
            for (const auto& arg : expr.call.arguments) {
                write_indent(out, indent + 1);
                out << "arg:\n";
                dump_expr(out, *arg, indent + 2);
            }
            break;
    }
}

void dump_program(std::ostream& out, const Program& program, int indent) {
    write_indent(out, indent);
    out << "Program";
    dump_location_suffix(out, program.location);
    out << '\n';
    for (const auto& func : program.functions) {
        write_indent(out, indent + 1);
        out << "Function " << type_kind_name(func->return_type) << ' ' << func->name
            << '(' << func->parameters.size() << ')';
        dump_location_suffix(out, func->location);
        out << '\n';
        for (const Param& param : func->parameters) {
            write_indent(out, indent + 2);
            out << "param " << type_kind_name(param.type) << ' ' << param.name;
            dump_location_suffix(out, param.location);
            out << '\n';
        }
        write_indent(out, indent + 2);
        out << "body:\n";
        dump_block(out, *func->body, indent + 3);
    }
}

}  // namespace minilang
