#include "minilang/ml/eval_harness.hpp"

#include "minilang/codegen/llvm_emitter.hpp"
#include "minilang/hir/hir_lower.hpp"
#include "minilang/hir/optimize.hpp"
#include "minilang/lexer/lexer.hpp"
#include "minilang/ml/features.hpp"
#include "minilang/ml/ml_advisor.hpp"
#include "minilang/parser/parser.hpp"
#include "minilang/semantic/semantic_analyzer.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace minilang {

namespace {

std::string read_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("could not open file: " + path);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

int count_hir_instructions(const HirModule& module) {
    int total = 0;
    for (const std::unique_ptr<HirFunction>& func : module.functions) {
        if (func) {
            total += static_cast<int>(func->instructions.size());
        }
    }
    return total;
}

int count_llvm_lines(const HirModule& module) {
    std::ostringstream buffer;
    LlvmEmitter emitter(module);
    emitter.emit(buffer);
    int lines = 0;
    const std::string text = buffer.str();
    for (char c : text) {
        if (c == '\n') {
            ++lines;
        }
    }
    return lines;
}

std::string strategy_name(OptStrategy strategy) {
    switch (strategy) {
        case OptStrategy::None:
            return "none";
        case OptStrategy::All:
            return "all";
        case OptStrategy::Advised:
            return "advised";
    }
    return "unknown";
}

std::string plan_to_string(const PassPlan& plan) {
    std::ostringstream out;
    for (std::size_t i = 0; i < plan.passes.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << pass_kind_name(plan.passes[i]);
    }
    return out.str();
}

HirModule compile_to_hir(const std::string& source_path) {
    const std::string source = read_file(source_path);
    Lexer lexer(source, source_path);
    Parser parser(lexer);
    const Program program = parser.parse();
    if (lexer.had_error() || parser.had_error()) {
        throw std::runtime_error("parse failed for " + source_path);
    }

    SemanticAnalyzer analyzer(program);
    if (!analyzer.analyze()) {
        throw std::runtime_error("semantic analysis failed for " + source_path);
    }

    HirLowerer lowerer;
    return lowerer.lower(program);
}

PassPlan plan_for_strategy(OptStrategy strategy, const HirFeatures& features) {
    switch (strategy) {
        case OptStrategy::None:
            return no_passes_plan();
        case OptStrategy::All:
            return all_passes_plan();
        case OptStrategy::Advised: {
            MlAdvisor advisor;
            return advisor.recommend(features);
        }
    }
    return no_passes_plan();
}

EvalRow evaluate_program(const std::string& path, OptStrategy strategy) {
    HirModule module = compile_to_hir(path);
    const int hir_before = count_hir_instructions(module);
    const HirFeatures features = extract_hir_features(module);

    PassPlan plan = plan_for_strategy(strategy, features);
    HirOptimizer optimizer(module, plan);
    const OptimizationStats stats = optimizer.run();

    EvalRow row;
    row.program = path;
    row.strategy = strategy;
    row.hir_before = hir_before;
    row.hir_after = count_hir_instructions(module);
    row.llvm_lines = count_llvm_lines(module);
    row.constant_folds = stats.constant_folds;
    row.algebraic_simplifications = stats.algebraic_simplifications;
    row.dead_temps_removed = stats.dead_temps_removed;
    row.pass_plan = plan_to_string(plan);
    return row;
}

}  // namespace

HirModule lower_and_optimize(const std::string& source_path, OptStrategy strategy,
                             OptimizationStats* stats_out, PassPlan* plan_out) {
    HirModule module = compile_to_hir(source_path);
    const HirFeatures features = extract_hir_features(module);
    PassPlan plan = plan_for_strategy(strategy, features);

    HirOptimizer optimizer(module, plan);
    const OptimizationStats stats = optimizer.run();

    if (stats_out) {
        *stats_out = stats;
    }
    if (plan_out) {
        *plan_out = plan;
    }
    return module;
}

EvalReport run_eval_harness(const std::vector<std::string>& program_paths) {
    EvalReport report;
    const OptStrategy strategies[] = {OptStrategy::None, OptStrategy::All, OptStrategy::Advised};

    for (const std::string& path : program_paths) {
        for (OptStrategy strategy : strategies) {
            report.rows.push_back(evaluate_program(path, strategy));
        }
    }
    return report;
}

void dump_eval_report(std::ostream& out, const EvalReport& report) {
    out << std::left << std::setw(28) << "program" << std::setw(8) << "strategy" << std::setw(8)
        << "hir_in" << std::setw(8) << "hir_out" << std::setw(8) << "llvm_ln" << std::setw(8)
        << "folds" << std::setw(8) << "alg" << std::setw(8) << "dead" << "pass_plan\n";

    for (const EvalRow& row : report.rows) {
        out << std::left << std::setw(28) << row.program << std::setw(8)
            << strategy_name(row.strategy) << std::setw(8) << row.hir_before << std::setw(8)
            << row.hir_after << std::setw(8) << row.llvm_lines << std::setw(8)
            << row.constant_folds << std::setw(8) << row.algebraic_simplifications << std::setw(8)
            << row.dead_temps_removed << row.pass_plan << '\n';
    }
}

}  // namespace minilang
