#include "minilang/hir/optimize.hpp"

#include <unordered_set>

namespace minilang {

namespace {

bool is_int_const(const HirInstr* def, int64_t* out) {
    if (!def || def->op != HirOp::ConstInt) {
        return false;
    }
    *out = def->int_value;
    return true;
}

bool is_bool_const(const HirInstr* def, bool* out) {
    if (!def || def->op != HirOp::ConstBool) {
        return false;
    }
    *out = def->bool_value;
    return true;
}

}  // namespace

HirOptimizer::HirOptimizer(HirModule& module) : module_(module) {}

OptimizationStats HirOptimizer::run() {
    for (std::unique_ptr<HirFunction>& func : module_.functions) {
        if (func) {
            optimize_function(*func);
        }
    }
    return stats_;
}

bool HirOptimizer::optimize_function(HirFunction& func) {
    bool changed = false;
    changed = fold_constants(func) || changed;
    changed = simplify_algebra(func) || changed;
    changed = remove_dead_temps(func) || changed;
    return changed;
}

const HirInstr* HirOptimizer::find_def(int temp, const HirFunction& func,
                                       std::size_t before) const {
    for (std::size_t i = 0; i < before && i < func.instructions.size(); ++i) {
        const HirInstr& instr = func.instructions[i];
        if (instr.dest == temp) {
            return &instr;
        }
    }
    return nullptr;
}

bool HirOptimizer::try_fold_binary(HirInstr& instr, const HirFunction& func) const {
    if (instr.op != HirOp::Binary) {
        return false;
    }

    int64_t left_int = 0;
    int64_t right_int = 0;
    bool left_bool = false;
    bool right_bool = false;

    const HirInstr* left_def = find_def(instr.lhs, func, func.instructions.size());
    const HirInstr* right_def = find_def(instr.rhs, func, func.instructions.size());

    switch (instr.binary_op) {
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod:
            if (!is_int_const(left_def, &left_int) || !is_int_const(right_def, &right_int)) {
                return false;
            }
            switch (instr.binary_op) {
                case BinaryOp::Add:
                    instr.op = HirOp::ConstInt;
                    instr.int_value = left_int + right_int;
                    break;
                case BinaryOp::Sub:
                    instr.op = HirOp::ConstInt;
                    instr.int_value = left_int - right_int;
                    break;
                case BinaryOp::Mul:
                    instr.op = HirOp::ConstInt;
                    instr.int_value = left_int * right_int;
                    break;
                case BinaryOp::Div:
                    if (right_int == 0) {
                        return false;
                    }
                    instr.op = HirOp::ConstInt;
                    instr.int_value = left_int / right_int;
                    break;
                case BinaryOp::Mod:
                    if (right_int == 0) {
                        return false;
                    }
                    instr.op = HirOp::ConstInt;
                    instr.int_value = left_int % right_int;
                    break;
                default:
                    break;
            }
            instr.type = TypeKind::Int;
            instr.lhs = -1;
            instr.rhs = -1;
            return true;
        case BinaryOp::Eq:
        case BinaryOp::Ne:
        case BinaryOp::Lt:
        case BinaryOp::Le:
        case BinaryOp::Gt:
        case BinaryOp::Ge:
            if (!is_int_const(left_def, &left_int) || !is_int_const(right_def, &right_int)) {
                return false;
            }
            instr.op = HirOp::ConstBool;
            switch (instr.binary_op) {
                case BinaryOp::Eq:
                    instr.bool_value = left_int == right_int;
                    break;
                case BinaryOp::Ne:
                    instr.bool_value = left_int != right_int;
                    break;
                case BinaryOp::Lt:
                    instr.bool_value = left_int < right_int;
                    break;
                case BinaryOp::Le:
                    instr.bool_value = left_int <= right_int;
                    break;
                case BinaryOp::Gt:
                    instr.bool_value = left_int > right_int;
                    break;
                case BinaryOp::Ge:
                    instr.bool_value = left_int >= right_int;
                    break;
                default:
                    break;
            }
            instr.type = TypeKind::Bool;
            instr.lhs = -1;
            instr.rhs = -1;
            return true;
        case BinaryOp::And:
        case BinaryOp::Or:
            if (!is_bool_const(left_def, &left_bool) || !is_bool_const(right_def, &right_bool)) {
                return false;
            }
            instr.op = HirOp::ConstBool;
            instr.bool_value =
                instr.binary_op == BinaryOp::And ? (left_bool && right_bool) : (left_bool || right_bool);
            instr.type = TypeKind::Bool;
            instr.lhs = -1;
            instr.rhs = -1;
            return true;
    }
    return false;
}

bool HirOptimizer::try_fold_unary(HirInstr& instr, const HirFunction& func) const {
    if (instr.op != HirOp::Unary) {
        return false;
    }

    const HirInstr* operand_def = find_def(instr.lhs, func, func.instructions.size());
    if (instr.unary_op == UnaryOp::Neg) {
        int64_t value = 0;
        if (!is_int_const(operand_def, &value)) {
            return false;
        }
        instr.op = HirOp::ConstInt;
        instr.int_value = -value;
        instr.type = TypeKind::Int;
        instr.lhs = -1;
        return true;
    }

    bool value = false;
    if (!is_bool_const(operand_def, &value)) {
        return false;
    }
    instr.op = HirOp::ConstBool;
    instr.bool_value = !value;
    instr.type = TypeKind::Bool;
    instr.lhs = -1;
    return true;
}

bool HirOptimizer::try_simplify_binary(HirInstr& instr, const HirFunction& func) const {
    if (instr.op != HirOp::Binary || instr.binary_op != BinaryOp::Mul) {
        return false;
    }

    int64_t right_int = 0;
    int64_t left_int = 0;
    const HirInstr* right_def = find_def(instr.rhs, func, func.instructions.size());
    const HirInstr* left_def = find_def(instr.lhs, func, func.instructions.size());

    if (is_int_const(right_def, &right_int) && right_int == 0) {
        instr.op = HirOp::ConstInt;
        instr.int_value = 0;
        instr.type = TypeKind::Int;
        instr.lhs = -1;
        instr.rhs = -1;
        return true;
    }

    if (is_int_const(left_def, &left_int) && left_int == 0) {
        instr.op = HirOp::ConstInt;
        instr.int_value = 0;
        instr.type = TypeKind::Int;
        instr.lhs = -1;
        instr.rhs = -1;
        return true;
    }

    return false;
}

bool HirOptimizer::fold_constants(HirFunction& func) {
    bool changed = false;
    for (HirInstr& instr : func.instructions) {
        if (try_fold_binary(instr, func)) {
            ++stats_.constant_folds;
            changed = true;
        } else if (try_fold_unary(instr, func)) {
            ++stats_.constant_folds;
            changed = true;
        }
    }
    return changed;
}

bool HirOptimizer::simplify_algebra(HirFunction& func) {
    bool changed = false;
    for (HirInstr& instr : func.instructions) {
        if (try_simplify_binary(instr, func)) {
            ++stats_.algebraic_simplifications;
            changed = true;
        }
    }
    return changed;
}

bool HirOptimizer::remove_dead_temps(HirFunction& func) {
    std::unordered_set<int> used;
    for (const HirInstr& instr : func.instructions) {
        if (instr.lhs >= 0) {
            used.insert(instr.lhs);
        }
        if (instr.rhs >= 0) {
            used.insert(instr.rhs);
        }
        for (int arg : instr.args) {
            used.insert(arg);
        }
    }

    std::vector<HirInstr> kept;
    kept.reserve(func.instructions.size());
    int removed = 0;

    for (const HirInstr& instr : func.instructions) {
        const bool is_computation = instr.dest >= 0 && instr.op != HirOp::Label &&
                                    instr.op != HirOp::Jump && instr.op != HirOp::BrCond &&
                                    instr.op != HirOp::Ret && instr.op != HirOp::StoreLocal &&
                                    instr.op != HirOp::LoadLocal;

        if (is_computation && used.count(instr.dest) == 0) {
            ++removed;
            continue;
        }
        kept.push_back(instr);
    }

    if (removed > 0) {
        stats_.dead_temps_removed += removed;
        func.instructions = std::move(kept);
        return true;
    }
    return false;
}

}  // namespace minilang
