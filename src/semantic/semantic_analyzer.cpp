#include "minilang/semantic/semantic_analyzer.hpp"

#include <iostream>
#include <sstream>

namespace minilang {

namespace {

std::string type_name(TypeKind type) {
    return type_kind_name(type);
}

}  // namespace

SemanticAnalyzer::SemanticAnalyzer(const Program& program) : program_(program) {}

bool SemanticAnalyzer::analyze() {
    register_functions();
    for (const std::unique_ptr<FunctionDecl>& func : program_.functions) {
        if (func) {
            check_function(*func);
        }
    }
    return !had_error_;
}

void SemanticAnalyzer::error(const SourceLocation& loc, const std::string& message) {
    if (panic_mode_) {
        return;
    }
    panic_mode_ = true;
    had_error_ = true;
    std::cerr << "semantic error at " << loc.to_string() << ": " << message << '\n';
}

void SemanticAnalyzer::register_functions() {
    for (const std::unique_ptr<FunctionDecl>& func : program_.functions) {
        if (!func) {
            continue;
        }
        if (functions_.count(func->name) > 0) {
            error(func->location, "duplicate function '" + func->name + "'");
            continue;
        }
        FunctionSymbol symbol;
        symbol.return_type = func->return_type;
        symbol.location = func->location;
        for (const Param& param : func->parameters) {
            symbol.parameter_types.push_back(param.type);
        }
        functions_.emplace(func->name, std::move(symbol));
    }
}

void SemanticAnalyzer::check_function(const FunctionDecl& func) {
    panic_mode_ = false;
    current_return_type_ = func.return_type;
    enter_scope();
    for (const Param& param : func.parameters) {
        declare_variable(param.name, param.type, param.location);
    }
    if (func.body) {
        check_block(*func.body);
    }
    exit_scope();
}

void SemanticAnalyzer::enter_scope() {
    scopes_.emplace_back();
}

void SemanticAnalyzer::exit_scope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
    }
}

bool SemanticAnalyzer::declare_variable(const std::string& name, TypeKind type,
                                        const SourceLocation& loc) {
    if (scopes_.empty()) {
        enter_scope();
    }
    auto& scope = scopes_.back();
    if (scope.count(name) > 0) {
        error(loc, "variable '" + name + "' already declared in this scope");
        return false;
    }
    scope.emplace(name, VariableSymbol{type, loc});
    return true;
}

const SemanticAnalyzer::VariableSymbol* SemanticAnalyzer::lookup_variable(
    const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

const SemanticAnalyzer::FunctionSymbol* SemanticAnalyzer::lookup_function(
    const std::string& name) const {
    auto found = functions_.find(name);
    if (found == functions_.end()) {
        return nullptr;
    }
    return &found->second;
}

bool SemanticAnalyzer::types_equal(TypeKind left, TypeKind right) {
    return left == right;
}

bool SemanticAnalyzer::is_numeric(TypeKind type) {
    return type == TypeKind::Int;
}

bool SemanticAnalyzer::is_comparable(TypeKind type) {
    return type == TypeKind::Int || type == TypeKind::Bool;
}

void SemanticAnalyzer::check_block(const BlockStmt& block) {
    enter_scope();
    for (const std::unique_ptr<Stmt>& stmt : block.statements) {
        if (stmt) {
            check_statement(*stmt);
        }
    }
    exit_scope();
}

void SemanticAnalyzer::check_statement(const Stmt& stmt) {
    switch (stmt.kind) {
        case Stmt::Kind::Block:
            check_block(stmt.block);
            break;
        case Stmt::Kind::VarDecl: {
            const VarDeclStmt& decl = stmt.var_decl;
            if (!decl.initializer) {
                error(decl.location, "variable '" + decl.name + "' requires an initializer");
                break;
            }
            const TypeKind init_type = check_expression(*decl.initializer);
            if (!had_error_ && !types_equal(init_type, decl.type)) {
                std::ostringstream msg;
                msg << "cannot initialize " << type_name(decl.type) << " variable '" << decl.name
                    << "' with " << type_name(init_type) << " expression";
                error(decl.location, msg.str());
            }
            declare_variable(decl.name, decl.type, decl.location);
            break;
        }
        case Stmt::Kind::If: {
            const TypeKind cond_type = check_expression(*stmt.if_stmt.condition);
            if (!had_error_ && cond_type != TypeKind::Bool) {
                error(stmt.if_stmt.condition->location(),
                      "if condition must be bool, got " + type_name(cond_type));
            }
            check_statement(*stmt.if_stmt.then_branch);
            if (stmt.if_stmt.else_branch) {
                check_statement(*stmt.if_stmt.else_branch);
            }
            break;
        }
        case Stmt::Kind::While: {
            const TypeKind cond_type = check_expression(*stmt.while_stmt.condition);
            if (!had_error_ && cond_type != TypeKind::Bool) {
                error(stmt.while_stmt.condition->location(),
                      "while condition must be bool, got " + type_name(cond_type));
            }
            check_statement(*stmt.while_stmt.body);
            break;
        }
        case Stmt::Kind::Return: {
            const ReturnStmt& ret = stmt.return_stmt;
            if (current_return_type_ == TypeKind::Void) {
                if (ret.value) {
                    error(ret.location, "void function cannot return a value");
                }
                break;
            }
            if (!ret.value) {
                error(ret.location,
                      "function returning " + type_name(current_return_type_) +
                          " must return a value");
                break;
            }
            const TypeKind value_type = check_expression(*ret.value);
            if (!had_error_ && !types_equal(value_type, current_return_type_)) {
                std::ostringstream msg;
                msg << "return type mismatch: expected " << type_name(current_return_type_)
                    << ", got " << type_name(value_type);
                error(ret.location, msg.str());
            }
            break;
        }
        case Stmt::Kind::Expr:
            check_expression(*stmt.expr_stmt.expression);
            break;
    }
}

TypeKind SemanticAnalyzer::check_expression(const Expr& expr) {
    switch (expr.kind) {
        case Expr::Kind::IntLiteral:
            return TypeKind::Int;
        case Expr::Kind::BoolLiteral:
            return TypeKind::Bool;
        case Expr::Kind::Variable: {
            const VariableSymbol* symbol = lookup_variable(expr.variable.name);
            if (!symbol) {
                error(expr.variable.location, "undefined variable '" + expr.variable.name + "'");
                return TypeKind::Int;
            }
            return symbol->type;
        }
        case Expr::Kind::Unary: {
            const TypeKind operand_type = check_expression(*expr.unary.operand);
            if (had_error_) {
                return TypeKind::Int;
            }
            if (expr.unary.op == UnaryOp::Neg) {
                if (!is_numeric(operand_type)) {
                    error(expr.unary.location,
                          "unary '-' requires int operand, got " + type_name(operand_type));
                }
                return TypeKind::Int;
            }
            if (operand_type != TypeKind::Bool) {
                error(expr.unary.location,
                      "unary '!' requires bool operand, got " + type_name(operand_type));
            }
            return TypeKind::Bool;
        }
        case Expr::Kind::Binary: {
            const TypeKind left_type = check_expression(*expr.binary.left);
            const TypeKind right_type = check_expression(*expr.binary.right);
            if (had_error_) {
                return TypeKind::Int;
            }
            switch (expr.binary.op) {
                case BinaryOp::Add:
                case BinaryOp::Sub:
                case BinaryOp::Mul:
                case BinaryOp::Div:
                case BinaryOp::Mod:
                    if (!is_numeric(left_type) || !is_numeric(right_type)) {
                        error(expr.binary.location,
                              "arithmetic operator requires int operands");
                    }
                    return TypeKind::Int;
                case BinaryOp::Eq:
                case BinaryOp::Ne:
                    if (!types_equal(left_type, right_type) || !is_comparable(left_type)) {
                        error(expr.binary.location,
                              "equality operator requires operands of the same int or bool type");
                    }
                    return TypeKind::Bool;
                case BinaryOp::Lt:
                case BinaryOp::Le:
                case BinaryOp::Gt:
                case BinaryOp::Ge:
                    if (!is_numeric(left_type) || !is_numeric(right_type)) {
                        error(expr.binary.location, "comparison operator requires int operands");
                    }
                    return TypeKind::Bool;
                case BinaryOp::And:
                case BinaryOp::Or:
                    if (left_type != TypeKind::Bool || right_type != TypeKind::Bool) {
                        error(expr.binary.location, "logical operator requires bool operands");
                    }
                    return TypeKind::Bool;
            }
            return TypeKind::Int;
        }
        case Expr::Kind::Call: {
            const FunctionSymbol* symbol = lookup_function(expr.call.callee);
            if (!symbol) {
                error(expr.call.location, "undefined function '" + expr.call.callee + "'");
                return TypeKind::Int;
            }
            if (expr.call.arguments.size() != symbol->parameter_types.size()) {
                std::ostringstream msg;
                msg << "call to '" << expr.call.callee << "' expects "
                    << symbol->parameter_types.size() << " argument(s), got "
                    << expr.call.arguments.size();
                error(expr.call.location, msg.str());
                return symbol->return_type;
            }
            for (std::size_t i = 0; i < expr.call.arguments.size(); ++i) {
                const TypeKind arg_type = check_expression(*expr.call.arguments[i]);
                if (!had_error_ && !types_equal(arg_type, symbol->parameter_types[i])) {
                    std::ostringstream msg;
                    msg << "argument " << (i + 1) << " of call to '" << expr.call.callee
                        << "' expects " << type_name(symbol->parameter_types[i]) << ", got "
                        << type_name(arg_type);
                    error(expr.call.arguments[i]->location(), msg.str());
                }
            }
            return symbol->return_type;
        }
    }
    return TypeKind::Int;
}

}  // namespace minilang
