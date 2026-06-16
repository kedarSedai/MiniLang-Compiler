#include "minilang/hir/hir_lower.hpp"

#include <algorithm>

namespace minilang {

HirModule HirLowerer::lower(const Program& program) {
    module_.location = program.location;
    for (const std::unique_ptr<FunctionDecl>& func : program.functions) {
        if (func) {
            lower_function(*func);
        }
    }
    return std::move(module_);
}

void HirLowerer::lower_function(const FunctionDecl& func) {
    auto hir_func = std::make_unique<HirFunction>();
    hir_func->location = func.location;
    hir_func->name = func.name;
    hir_func->return_type = func.return_type;
    hir_func->parameters = func.parameters;

    current_ = hir_func.get();
    next_temp_ = 0;
    scope_stack_.clear();
    push_scope();

    for (const Param& param : func.parameters) {
        declare_local(param.name, param.type);
    }

    if (func.body) {
        lower_block(*func.body);
    }

    pop_scope();
    module_.functions.push_back(std::move(hir_func));
    current_ = nullptr;
}

void HirLowerer::lower_block(const BlockStmt& block) {
    push_scope();
    for (const std::unique_ptr<Stmt>& stmt : block.statements) {
        if (stmt) {
            lower_statement(*stmt);
        }
    }
    pop_scope();
}

void HirLowerer::lower_statement(const Stmt& stmt) {
    switch (stmt.kind) {
        case Stmt::Kind::Block:
            lower_block(stmt.block);
            break;
        case Stmt::Kind::VarDecl: {
            const VarDeclStmt& decl = stmt.var_decl;
            declare_local(decl.name, decl.type);
            if (decl.initializer) {
                const int value_temp = lower_expression(*decl.initializer);
                HirInstr store;
                store.op = HirOp::StoreLocal;
                store.location = decl.location;
                store.local_name = decl.name;
                store.lhs = value_temp;
                current_->instructions.push_back(std::move(store));
            }
            break;
        }
        case Stmt::Kind::If: {
            const int cond_temp = lower_expression(*stmt.if_stmt.condition);
            const std::string then_label = fresh_label("then");
            const std::string else_label = fresh_label("else");
            const std::string end_label = fresh_label("endif");

            HirInstr branch;
            branch.op = HirOp::BrCond;
            branch.location = stmt.if_stmt.location;
            branch.lhs = cond_temp;
            branch.then_label = then_label;
            branch.else_label = stmt.if_stmt.else_branch ? else_label : end_label;
            current_->instructions.push_back(std::move(branch));

            HirInstr then_lbl;
            then_lbl.op = HirOp::Label;
            then_lbl.location = stmt.if_stmt.location;
            then_lbl.jump_label = then_label;
            current_->instructions.push_back(std::move(then_lbl));

            lower_statement(*stmt.if_stmt.then_branch);

            HirInstr jump_end;
            jump_end.op = HirOp::Jump;
            jump_end.location = stmt.if_stmt.location;
            jump_end.jump_label = end_label;
            current_->instructions.push_back(std::move(jump_end));

            if (stmt.if_stmt.else_branch) {
                HirInstr else_lbl;
                else_lbl.op = HirOp::Label;
                else_lbl.location = stmt.if_stmt.location;
                else_lbl.jump_label = else_label;
                current_->instructions.push_back(std::move(else_lbl));

                lower_statement(*stmt.if_stmt.else_branch);
            }

            HirInstr end_lbl;
            end_lbl.op = HirOp::Label;
            end_lbl.location = stmt.if_stmt.location;
            end_lbl.jump_label = end_label;
            current_->instructions.push_back(std::move(end_lbl));
            break;
        }
        case Stmt::Kind::While: {
            const std::string cond_label = fresh_label("while_cond");
            const std::string body_label = fresh_label("while_body");
            const std::string end_label = fresh_label("while_end");

            HirInstr jump_cond;
            jump_cond.op = HirOp::Jump;
            jump_cond.location = stmt.while_stmt.location;
            jump_cond.jump_label = cond_label;
            current_->instructions.push_back(std::move(jump_cond));

            HirInstr cond_lbl;
            cond_lbl.op = HirOp::Label;
            cond_lbl.location = stmt.while_stmt.location;
            cond_lbl.jump_label = cond_label;
            current_->instructions.push_back(std::move(cond_lbl));

            const int cond_temp = lower_expression(*stmt.while_stmt.condition);

            HirInstr branch;
            branch.op = HirOp::BrCond;
            branch.location = stmt.while_stmt.location;
            branch.lhs = cond_temp;
            branch.then_label = body_label;
            branch.else_label = end_label;
            current_->instructions.push_back(std::move(branch));

            HirInstr body_lbl;
            body_lbl.op = HirOp::Label;
            body_lbl.location = stmt.while_stmt.location;
            body_lbl.jump_label = body_label;
            current_->instructions.push_back(std::move(body_lbl));

            lower_statement(*stmt.while_stmt.body);

            HirInstr jump_back;
            jump_back.op = HirOp::Jump;
            jump_back.location = stmt.while_stmt.location;
            jump_back.jump_label = cond_label;
            current_->instructions.push_back(std::move(jump_back));

            HirInstr end_lbl;
            end_lbl.op = HirOp::Label;
            end_lbl.location = stmt.while_stmt.location;
            end_lbl.jump_label = end_label;
            current_->instructions.push_back(std::move(end_lbl));
            break;
        }
        case Stmt::Kind::Return: {
            HirInstr ret;
            ret.op = HirOp::Ret;
            ret.location = stmt.return_stmt.location;
            if (stmt.return_stmt.value) {
                ret.lhs = lower_expression(*stmt.return_stmt.value);
            } else {
                ret.lhs = -1;
            }
            current_->instructions.push_back(std::move(ret));
            break;
        }
        case Stmt::Kind::Expr:
            lower_expression(*stmt.expr_stmt.expression);
            break;
    }
}

int HirLowerer::lower_expression(const Expr& expr) {
    const int dest = fresh_temp();

    switch (expr.kind) {
        case Expr::Kind::IntLiteral: {
            HirInstr instr;
            instr.op = HirOp::ConstInt;
            instr.location = expr.int_literal.location;
            instr.dest = dest;
            instr.int_value = expr.int_literal.value;
            instr.type = TypeKind::Int;
            current_->instructions.push_back(std::move(instr));
            break;
        }
        case Expr::Kind::BoolLiteral: {
            HirInstr instr;
            instr.op = HirOp::ConstBool;
            instr.location = expr.bool_literal.location;
            instr.dest = dest;
            instr.bool_value = expr.bool_literal.value;
            instr.type = TypeKind::Bool;
            current_->instructions.push_back(std::move(instr));
            break;
        }
        case Expr::Kind::Variable: {
            HirInstr instr;
            instr.op = HirOp::LoadLocal;
            instr.location = expr.variable.location;
            instr.dest = dest;
            instr.local_name = expr.variable.name;
            current_->instructions.push_back(std::move(instr));
            break;
        }
        case Expr::Kind::Unary: {
            const int operand = lower_expression(*expr.unary.operand);
            HirInstr instr;
            instr.op = HirOp::Unary;
            instr.location = expr.unary.location;
            instr.dest = dest;
            instr.lhs = operand;
            instr.unary_op = expr.unary.op;
            instr.type = expr.unary.op == UnaryOp::Neg ? TypeKind::Int : TypeKind::Bool;
            current_->instructions.push_back(std::move(instr));
            break;
        }
        case Expr::Kind::Binary: {
            const int left = lower_expression(*expr.binary.left);
            const int right = lower_expression(*expr.binary.right);
            HirInstr instr;
            instr.op = HirOp::Binary;
            instr.location = expr.binary.location;
            instr.dest = dest;
            instr.lhs = left;
            instr.rhs = right;
            instr.binary_op = expr.binary.op;
            switch (expr.binary.op) {
                case BinaryOp::Add:
                case BinaryOp::Sub:
                case BinaryOp::Mul:
                case BinaryOp::Div:
                case BinaryOp::Mod:
                    instr.type = TypeKind::Int;
                    break;
                default:
                    instr.type = TypeKind::Bool;
                    break;
            }
            current_->instructions.push_back(std::move(instr));
            break;
        }
        case Expr::Kind::Call: {
            HirInstr instr;
            instr.op = HirOp::Call;
            instr.location = expr.call.location;
            instr.dest = dest;
            instr.callee = expr.call.callee;
            for (const std::unique_ptr<Expr>& arg : expr.call.arguments) {
                if (arg) {
                    instr.args.push_back(lower_expression(*arg));
                }
            }
            current_->instructions.push_back(std::move(instr));
            break;
        }
    }

    return dest;
}

int HirLowerer::fresh_temp() {
    return next_temp_++;
}

std::string HirLowerer::fresh_label(const std::string& prefix) {
    return prefix + '_' + std::to_string(next_label_++);
}

void HirLowerer::declare_local(const std::string& name, TypeKind type) {
    if (scope_stack_.empty()) {
        push_scope();
    }
    if (std::find(scope_stack_.back().begin(), scope_stack_.back().end(), name) ==
        scope_stack_.back().end()) {
        scope_stack_.back().push_back(name);
        current_->locals.push_back(name);
        current_->local_types[name] = type;
    }
}

void HirLowerer::push_scope() {
    scope_stack_.emplace_back();
}

void HirLowerer::pop_scope() {
    if (!scope_stack_.empty()) {
        scope_stack_.pop_back();
    }
}

}  // namespace minilang
