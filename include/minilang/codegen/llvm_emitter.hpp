#pragma once

#include "minilang/hir/hir.hpp"

#include <ostream>
#include <string>

namespace minilang {

/// Emits textual LLVM IR from an optimized HIR module.
class LlvmEmitter {
public:
    explicit LlvmEmitter(const HirModule& module);

    void emit(std::ostream& out) const;

private:
    void emit_header(std::ostream& out) const;
    void emit_function(std::ostream& out, const HirFunction& func) const;
    void emit_instruction(std::ostream& out, const HirFunction& func,
                          const HirInstr& instr, bool& in_entry) const;

    static std::string llvm_local_name(const std::string& name);
    static std::string llvm_label_name(const std::string& label);
    static std::string llvm_temp(int temp);
    static std::string llvm_type(TypeKind type);
    static std::string llvm_binop(BinaryOp op);

    TypeKind return_type_of(const std::string& callee) const;

    const HirModule& module_;
};

}  // namespace minilang
