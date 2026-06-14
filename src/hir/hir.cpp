#include "minilang/hir/hir.hpp"

namespace minilang {

const char* hir_op_name(HirOp op) {
    switch (op) {
        case HirOp::ConstInt:
            return "const_int";
        case HirOp::ConstBool:
            return "const_bool";
        case HirOp::LoadLocal:
            return "load_local";
        case HirOp::StoreLocal:
            return "store_local";
        case HirOp::Unary:
            return "unary";
        case HirOp::Binary:
            return "binary";
        case HirOp::Call:
            return "call";
        case HirOp::Ret:
            return "ret";
        case HirOp::BrCond:
            return "br_cond";
        case HirOp::Jump:
            return "jump";
        case HirOp::Label:
            return "label";
    }
    return "unknown";
}

namespace {

void write_indent(std::ostream& out, int indent) {
    for (int i = 0; i < indent; ++i) {
        out << "  ";
    }
}

void dump_instr(std::ostream& out, const HirInstr& instr, int indent) {
    write_indent(out, indent);
    out << hir_op_name(instr.op);
    switch (instr.op) {
        case HirOp::ConstInt:
            out << " %t" << instr.dest << " = " << instr.int_value;
            break;
        case HirOp::ConstBool:
            out << " %t" << instr.dest << " = " << (instr.bool_value ? "true" : "false");
            break;
        case HirOp::LoadLocal:
            out << " %t" << instr.dest << " = local[" << instr.local_name << ']';
            break;
        case HirOp::StoreLocal:
            out << " local[" << instr.local_name << "] = %t" << instr.lhs;
            break;
        case HirOp::Unary:
            out << " %t" << instr.dest << " = " << unary_op_name(instr.unary_op) << " %t"
                << instr.lhs;
            break;
        case HirOp::Binary:
            out << " %t" << instr.dest << " = %t" << instr.lhs << ' '
                << binary_op_name(instr.binary_op) << " %t" << instr.rhs;
            break;
        case HirOp::Call:
            out << " %t" << instr.dest << " = " << instr.callee << '(';
            for (std::size_t i = 0; i < instr.args.size(); ++i) {
                if (i > 0) {
                    out << ", ";
                }
                out << "%t" << instr.args[i];
            }
            out << ')';
            break;
        case HirOp::Ret:
            if (instr.lhs >= 0) {
                out << " %t" << instr.lhs;
            } else {
                out << " void";
            }
            break;
        case HirOp::BrCond:
            out << " %t" << instr.lhs << " ? " << instr.then_label << " : "
                << instr.else_label;
            break;
        case HirOp::Jump:
            out << ' ' << instr.jump_label;
            break;
        case HirOp::Label:
            out << ' ' << instr.jump_label << ':';
            break;
    }
    out << " @ " << instr.location.to_string() << '\n';
}

}  // namespace

void dump_hir_module(std::ostream& out, const HirModule& module) {
    out << "HIRModule @ " << module.location.to_string() << '\n';
    for (const std::unique_ptr<HirFunction>& func : module.functions) {
        if (!func) {
            continue;
        }
        out << "  function " << type_kind_name(func->return_type) << ' ' << func->name << '('
            << func->parameters.size() << ") @ " << func->location.to_string() << '\n';
        for (const Param& param : func->parameters) {
            out << "    param " << type_kind_name(param.type) << ' ' << param.name << '\n';
        }
        if (!func->locals.empty()) {
            out << "    locals:";
            for (const std::string& local : func->locals) {
                out << ' ' << local;
            }
            out << '\n';
        }
        out << "    instructions:\n";
        for (const HirInstr& instr : func->instructions) {
            dump_instr(out, instr, 3);
        }
    }
}

}  // namespace minilang
