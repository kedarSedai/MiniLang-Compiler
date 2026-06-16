#pragma once

#include "minilang/ast/ast.hpp"
#include "minilang/hir/hir.hpp"

namespace minilang {

/// Lowers a semantically valid AST into HIR.
class HirLowerer {
public:
    HirModule lower(const Program& program);

private:
    void lower_function(const FunctionDecl& func);
    void lower_block(const BlockStmt& block);
    void lower_statement(const Stmt& stmt);
    int lower_expression(const Expr& expr);

    HirModule module_;
    HirFunction* current_ = nullptr;
    int next_temp_ = 0;
    int next_label_ = 0;
    std::vector<std::vector<std::string>> scope_stack_;

    int fresh_temp();
    std::string fresh_label(const std::string& prefix);
    void declare_local(const std::string& name, TypeKind type = TypeKind::Int);
    void push_scope();
    void pop_scope();
};

}  // namespace minilang
