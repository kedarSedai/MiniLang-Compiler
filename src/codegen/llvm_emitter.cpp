#include "minilang/codegen/llvm_emitter.hpp"

#include <cctype>
#include <sstream>

namespace minilang {

namespace {

std::string sanitize_identifier(const std::string& name) {
    if (name.empty()) {
        return "v";
    }
    std::string out;
    out.reserve(name.size());
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            out.push_back(c);
        } else {
            out.push_back('_');
        }
    }
    if (!std::isalpha(static_cast<unsigned char>(out.front())) && out.front() != '_') {
        out.insert(out.begin(), '_');
    }
    return out;
}

TypeKind local_type(const HirFunction& func, const std::string& name) {
    auto found = func.local_types.find(name);
    if (found != func.local_types.end()) {
        return found->second;
    }
    return TypeKind::Int;
}

bool is_comparison(BinaryOp op) {
    switch (op) {
        case BinaryOp::Eq:
        case BinaryOp::Ne:
        case BinaryOp::Lt:
        case BinaryOp::Le:
        case BinaryOp::Gt:
        case BinaryOp::Ge:
            return true;
        default:
            return false;
    }
}

bool is_logical(BinaryOp op) {
    return op == BinaryOp::And || op == BinaryOp::Or;
}

}  // namespace

LlvmEmitter::LlvmEmitter(const HirModule& module) : module_(module) {}

void LlvmEmitter::emit(std::ostream& out) const {
    emit_header(out);
    for (const std::unique_ptr<HirFunction>& func : module_.functions) {
        if (func) {
            emit_function(out, *func);
        }
    }
}

void LlvmEmitter::emit_header(std::ostream& out) const {
    out << "; MiniLang LLVM IR (generated)\n";
    out << "target triple = \"unknown-unknown-unknown\"\n\n";
}

std::string LlvmEmitter::llvm_local_name(const std::string& name) {
    return '%' + sanitize_identifier(name) + ".addr";
}

std::string LlvmEmitter::llvm_label_name(const std::string& label) {
    return sanitize_identifier(label);
}

std::string LlvmEmitter::llvm_temp(int temp) {
    return "%t" + std::to_string(temp);
}

std::string LlvmEmitter::llvm_type(TypeKind type) {
    switch (type) {
        case TypeKind::Int:
            return "i32";
        case TypeKind::Bool:
            return "i1";
        case TypeKind::Void:
            return "void";
    }
    return "i32";
}

std::string LlvmEmitter::llvm_binop(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add:
            return "add";
        case BinaryOp::Sub:
            return "sub";
        case BinaryOp::Mul:
            return "mul";
        case BinaryOp::Div:
            return "sdiv";
        case BinaryOp::Mod:
            return "srem";
        case BinaryOp::Eq:
            return "eq";
        case BinaryOp::Ne:
            return "ne";
        case BinaryOp::Lt:
            return "slt";
        case BinaryOp::Le:
            return "sle";
        case BinaryOp::Gt:
            return "sgt";
        case BinaryOp::Ge:
            return "sge";
        case BinaryOp::And:
            return "and";
        case BinaryOp::Or:
            return "or";
    }
    return "add";
}

TypeKind LlvmEmitter::return_type_of(const std::string& callee) const {
    for (const std::unique_ptr<HirFunction>& func : module_.functions) {
        if (func && func->name == callee) {
            return func->return_type;
        }
    }
    return TypeKind::Int;
}

void LlvmEmitter::emit_function(std::ostream& out, const HirFunction& func) const {
    out << "define " << llvm_type(func.return_type) << " @" << sanitize_identifier(func.name)
        << '(';

    for (std::size_t i = 0; i < func.parameters.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << llvm_type(func.parameters[i].type) << " %"
            << sanitize_identifier(func.parameters[i].name);
    }
    out << ") {\n";

    out << "entry:\n";

    for (const std::string& local : func.locals) {
        const TypeKind type = local_type(func, local);
        out << "  " << llvm_local_name(local) << " = alloca " << llvm_type(type) << '\n';
    }

    for (const Param& param : func.parameters) {
        out << "  store " << llvm_type(param.type) << " %"
            << sanitize_identifier(param.name) << ", " << llvm_type(param.type) << "* "
            << llvm_local_name(param.name) << '\n';
    }

    bool has_ret = false;
    bool in_entry = true;
    for (const HirInstr& instr : func.instructions) {
        if (instr.op == HirOp::Ret) {
            has_ret = true;
        }
        emit_instruction(out, func, instr, in_entry);
    }

    if (!has_ret) {
        if (func.return_type == TypeKind::Void) {
            out << "  ret void\n";
        } else {
            out << "  ret i32 0\n";
        }
    }

    out << "}\n\n";
}

void LlvmEmitter::emit_instruction(std::ostream& out, const HirFunction& func,
                                   const HirInstr& instr, bool& in_entry) const {
    switch (instr.op) {
        case HirOp::Label:
            if (in_entry) {
                in_entry = false;
            }
            out << llvm_label_name(instr.jump_label) << ":\n";
            break;
        case HirOp::ConstInt:
            out << "  " << llvm_temp(instr.dest) << " = add i32 0, " << instr.int_value << '\n';
            break;
        case HirOp::ConstBool: {
            const int value = instr.bool_value ? 1 : 0;
            out << "  " << llvm_temp(instr.dest) << " = add i32 0, " << value << '\n';
            break;
        }
        case HirOp::LoadLocal: {
            const TypeKind type = local_type(func, instr.local_name);
            if (type == TypeKind::Bool) {
                const std::string raw = llvm_temp(instr.dest) + "_raw";
                out << "  " << raw << " = load i1, i1* " << llvm_local_name(instr.local_name)
                    << '\n';
                out << "  " << llvm_temp(instr.dest) << " = zext i1 " << raw << " to i32\n";
            } else {
                out << "  " << llvm_temp(instr.dest) << " = load i32, i32* "
                    << llvm_local_name(instr.local_name) << '\n';
            }
            break;
        }
        case HirOp::StoreLocal: {
            const TypeKind type = local_type(func, instr.local_name);
            if (type == TypeKind::Bool) {
                const std::string truncated = llvm_temp(instr.lhs) + "_trunc";
                out << "  " << truncated << " = trunc i32 " << llvm_temp(instr.lhs) << " to i1\n";
                out << "  store i1 " << truncated << ", i1* " << llvm_local_name(instr.local_name)
                    << '\n';
            } else {
                out << "  store i32 " << llvm_temp(instr.lhs) << ", i32* "
                    << llvm_local_name(instr.local_name) << '\n';
            }
            break;
        }
        case HirOp::Unary:
            if (instr.unary_op == UnaryOp::Neg) {
                out << "  " << llvm_temp(instr.dest) << " = sub i32 0, " << llvm_temp(instr.lhs)
                    << '\n';
            } else {
                const std::string cmp = llvm_temp(instr.dest) + "_cmp";
                const std::string bit = llvm_temp(instr.dest) + "_b";
                out << "  " << cmp << " = icmp ne i32 " << llvm_temp(instr.lhs) << ", 0\n";
                out << "  " << bit << " = xor i1 " << cmp << ", true\n";
                out << "  " << llvm_temp(instr.dest) << " = zext i1 " << bit << " to i32\n";
            }
            break;
        case HirOp::Binary:
            if (is_comparison(instr.binary_op)) {
                const std::string cmp = llvm_temp(instr.dest) + "_cmp";
                out << "  " << cmp << " = icmp " << llvm_binop(instr.binary_op) << " i32 "
                    << llvm_temp(instr.lhs) << ", " << llvm_temp(instr.rhs) << '\n';
                out << "  " << llvm_temp(instr.dest) << " = zext i1 " << cmp << " to i32\n";
            } else if (is_logical(instr.binary_op)) {
                const std::string left = llvm_temp(instr.lhs) + "_c";
                const std::string right = llvm_temp(instr.rhs) + "_c";
                const std::string bit = llvm_temp(instr.dest) + "_b";
                out << "  " << left << " = icmp ne i32 " << llvm_temp(instr.lhs) << ", 0\n";
                out << "  " << right << " = icmp ne i32 " << llvm_temp(instr.rhs) << ", 0\n";
                out << "  " << bit << " = " << llvm_binop(instr.binary_op) << " i1 " << left
                    << ", " << right << '\n';
                out << "  " << llvm_temp(instr.dest) << " = zext i1 " << bit << " to i32\n";
            } else {
                out << "  " << llvm_temp(instr.dest) << " = " << llvm_binop(instr.binary_op)
                    << " i32 " << llvm_temp(instr.lhs) << ", " << llvm_temp(instr.rhs) << '\n';
            }
            break;
        case HirOp::Call: {
            const TypeKind ret_type = return_type_of(instr.callee);
            std::ostringstream args;
            args << '(';
            for (std::size_t i = 0; i < instr.args.size(); ++i) {
                if (i > 0) {
                    args << ", ";
                }
                args << "i32 " << llvm_temp(instr.args[i]);
            }
            args << ')';
            if (ret_type == TypeKind::Void) {
                out << "  call void @" << sanitize_identifier(instr.callee) << args.str() << '\n';
            } else {
                out << "  " << llvm_temp(instr.dest) << " = call " << llvm_type(ret_type)
                    << " @" << sanitize_identifier(instr.callee) << args.str() << '\n';
            }
            break;
        }
        case HirOp::Ret:
            if (func.return_type == TypeKind::Void) {
                out << "  ret void\n";
            } else if (instr.lhs >= 0) {
                out << "  ret i32 " << llvm_temp(instr.lhs) << '\n';
            } else {
                out << "  ret i32 0\n";
            }
            break;
        case HirOp::BrCond: {
            const std::string cond = llvm_temp(instr.lhs) + "_br";
            out << "  " << cond << " = icmp ne i32 " << llvm_temp(instr.lhs) << ", 0\n";
            out << "  br i1 " << cond << ", label %" << llvm_label_name(instr.then_label)
                << ", label %" << llvm_label_name(instr.else_label) << '\n';
            break;
        }
        case HirOp::Jump:
            out << "  br label %" << llvm_label_name(instr.jump_label) << '\n';
            break;
    }
}

}  // namespace minilang
