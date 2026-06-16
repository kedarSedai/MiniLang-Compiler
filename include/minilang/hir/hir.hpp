#pragma once

#include "minilang/ast/ast.hpp"

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace minilang {

/// High-level IR: flat temporaries + structured control flow, lowered from the AST.

enum class HirOp {
    ConstInt,
    ConstBool,
    LoadLocal,
    StoreLocal,
    Unary,
    Binary,
    Call,
    Ret,
    BrCond,
    Jump,
    Label,
};

struct HirInstr {
    HirOp op = HirOp::ConstInt;
    SourceLocation location;

    int dest = -1;
    int lhs = -1;
    int rhs = -1;

    int64_t int_value = 0;
    bool bool_value = false;
    TypeKind type = TypeKind::Int;

    UnaryOp unary_op = UnaryOp::Neg;
    BinaryOp binary_op = BinaryOp::Add;

    std::string local_name;
    std::string callee;
    std::string then_label;
    std::string else_label;
    std::string jump_label;
    std::vector<int> args;
};

struct HirFunction {
    SourceLocation location;
    std::string name;
    TypeKind return_type = TypeKind::Int;
    std::vector<Param> parameters;
    std::vector<std::string> locals;
    std::unordered_map<std::string, TypeKind> local_types;
    std::vector<HirInstr> instructions;
};

struct HirModule {
    SourceLocation location;
    std::vector<std::unique_ptr<HirFunction>> functions;
};

const char* hir_op_name(HirOp op);
void dump_hir_module(std::ostream& out, const HirModule& module);

}  // namespace minilang
