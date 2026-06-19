#pragma once

#include "minilang/hir/hir.hpp"

namespace minilang {

struct OptimizationStats {
    int constant_folds = 0;
    int algebraic_simplifications = 0;
    int dead_temps_removed = 0;
};

/// Applies rule-based optimizations to HIR.
class HirOptimizer {
public:
    explicit HirOptimizer(HirModule& module);

    OptimizationStats run();

private:
    bool optimize_function(HirFunction& func);
    bool fold_constants(HirFunction& func);
    bool simplify_algebra(HirFunction& func);
    bool remove_dead_temps(HirFunction& func);

    bool try_fold_binary(HirInstr& instr, const HirFunction& func) const;
    bool try_fold_unary(HirInstr& instr, const HirFunction& func) const;
    bool try_simplify_binary(HirInstr& instr, const HirFunction& func) const;

    const HirInstr* find_def(int temp, const HirFunction& func, std::size_t before) const;

    HirModule& module_;
    OptimizationStats stats_;
};

}  // namespace minilang
