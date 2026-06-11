#pragma once

#include "minilang/ast/ast.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace minilang {

/// Walks the AST, builds symbol tables, and checks types and scopes.
class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(const Program& program);

    /// Registers functions, then type-checks all bodies. Returns false on error.
    bool analyze();

    bool had_error() const { return had_error_; }

private:
    struct FunctionSymbol {
        TypeKind return_type = TypeKind::Int;
        std::vector<TypeKind> parameter_types;
        SourceLocation location;
    };

    struct VariableSymbol {
        TypeKind type = TypeKind::Int;
        SourceLocation location;
    };

    void error(const SourceLocation& loc, const std::string& message);

    void register_functions();
    void check_function(const FunctionDecl& func);

    void enter_scope();
    void exit_scope();
    bool declare_variable(const std::string& name, TypeKind type, const SourceLocation& loc);
    const VariableSymbol* lookup_variable(const std::string& name) const;
    const FunctionSymbol* lookup_function(const std::string& name) const;

    TypeKind check_expression(const Expr& expr);
    void check_statement(const Stmt& stmt);
    void check_block(const BlockStmt& block);

    static bool types_equal(TypeKind left, TypeKind right);
    static bool is_numeric(TypeKind type);
    static bool is_comparable(TypeKind type);

    const Program& program_;
    std::unordered_map<std::string, FunctionSymbol> functions_;
    std::vector<std::unordered_map<std::string, VariableSymbol>> scopes_;

    TypeKind current_return_type_ = TypeKind::Int;
    bool had_error_ = false;
    bool panic_mode_ = false;
};

}  // namespace minilang
