#pragma once

#include "minilang/hir/hir.hpp"
#include "minilang/hir/optimize.hpp"
#include "minilang/ml/pass_pipeline.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace minilang {

enum class OptStrategy {
    None,
    All,
    Advised,
};

struct EvalRow {
    std::string program;
    OptStrategy strategy = OptStrategy::None;
    int hir_before = 0;
    int hir_after = 0;
    int llvm_lines = 0;
    int constant_folds = 0;
    int algebraic_simplifications = 0;
    int dead_temps_removed = 0;
    std::string pass_plan;
};

struct EvalReport {
    std::vector<EvalRow> rows;
};

EvalReport run_eval_harness(const std::vector<std::string>& program_paths);
void dump_eval_report(std::ostream& out, const EvalReport& report);

HirModule lower_and_optimize(const std::string& source_path, OptStrategy strategy,
                             struct OptimizationStats* stats_out = nullptr,
                             PassPlan* plan_out = nullptr);

}  // namespace minilang
