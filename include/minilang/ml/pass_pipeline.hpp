#pragma once

#include <string>
#include <vector>

namespace minilang {

enum class OptPassKind {
    ConstantFold,
    AlgebraicSimplify,
    DeadTempRemove,
};

struct PassPlan {
    std::vector<OptPassKind> passes;
    int max_iterations = 3;
};

const char* pass_kind_name(OptPassKind pass);
OptPassKind pass_kind_from_string(const std::string& name);

PassPlan all_passes_plan();
PassPlan no_passes_plan();

}  // namespace minilang
