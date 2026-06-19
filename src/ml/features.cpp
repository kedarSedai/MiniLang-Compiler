#include "minilang/ml/features.hpp"

#include <algorithm>
#include <ostream>
#include <unordered_set>

namespace minilang {

namespace {

int count_temps(const HirFunction& func) {
    int max_temp = -1;
    for (const HirInstr& instr : func.instructions) {
        if (instr.dest > max_temp) {
            max_temp = instr.dest;
        }
    }
    return max_temp + 1;
}

int count_loop_hints(const HirFunction& func) {
    std::unordered_set<std::string> labels;
    for (const HirInstr& instr : func.instructions) {
        if (instr.op == HirOp::Label) {
            labels.insert(instr.jump_label);
        }
    }

    int loops = 0;
    for (const HirInstr& instr : func.instructions) {
        if (instr.op == HirOp::Jump && labels.count(instr.jump_label) > 0) {
            ++loops;
        }
    }
    return loops;
}

void accumulate_function(const HirFunction& func, HirFeatures& features) {
    features.local_count += static_cast<int>(func.locals.size());
    features.max_temps_per_function =
        std::max(features.max_temps_per_function, count_temps(func));
    features.loop_hint_count += count_loop_hints(func);

    for (const HirInstr& instr : func.instructions) {
        ++features.instruction_count;
        switch (instr.op) {
            case HirOp::ConstInt:
                ++features.const_int_count;
                break;
            case HirOp::ConstBool:
                ++features.const_bool_count;
                break;
            case HirOp::LoadLocal:
            case HirOp::StoreLocal:
                ++features.load_store_count;
                break;
            case HirOp::Unary:
                ++features.unary_op_count;
                break;
            case HirOp::Binary:
                ++features.binary_op_count;
                break;
            case HirOp::Call:
                ++features.call_count;
                break;
            case HirOp::BrCond:
                ++features.branch_count;
                break;
            case HirOp::Jump:
                ++features.jump_count;
                break;
            case HirOp::Label:
                ++features.label_count;
                break;
            default:
                break;
        }
    }
}

}  // namespace

HirFeatures extract_hir_features(const HirModule& module) {
    HirFeatures features;
    features.function_count = static_cast<int>(module.functions.size());
    for (const std::unique_ptr<HirFunction>& func : module.functions) {
        if (func) {
            accumulate_function(*func, features);
        }
    }
    return features;
}

void dump_hir_features(std::ostream& out, const HirFeatures& features) {
    out << "function_count=" << features.function_count << '\n';
    out << "instruction_count=" << features.instruction_count << '\n';
    out << "binary_op_count=" << features.binary_op_count << '\n';
    out << "unary_op_count=" << features.unary_op_count << '\n';
    out << "call_count=" << features.call_count << '\n';
    out << "branch_count=" << features.branch_count << '\n';
    out << "jump_count=" << features.jump_count << '\n';
    out << "label_count=" << features.label_count << '\n';
    out << "const_int_count=" << features.const_int_count << '\n';
    out << "const_bool_count=" << features.const_bool_count << '\n';
    out << "load_store_count=" << features.load_store_count << '\n';
    out << "local_count=" << features.local_count << '\n';
    out << "max_temps_per_function=" << features.max_temps_per_function << '\n';
    out << "loop_hint_count=" << features.loop_hint_count << '\n';
}

std::vector<double> hir_features_to_vector(const HirFeatures& features) {
    return {
        static_cast<double>(features.function_count),
        static_cast<double>(features.instruction_count),
        static_cast<double>(features.binary_op_count),
        static_cast<double>(features.unary_op_count),
        static_cast<double>(features.call_count),
        static_cast<double>(features.branch_count),
        static_cast<double>(features.jump_count),
        static_cast<double>(features.label_count),
        static_cast<double>(features.const_int_count),
        static_cast<double>(features.const_bool_count),
        static_cast<double>(features.load_store_count),
        static_cast<double>(features.local_count),
        static_cast<double>(features.max_temps_per_function),
        static_cast<double>(features.loop_hint_count),
    };
}

}  // namespace minilang
