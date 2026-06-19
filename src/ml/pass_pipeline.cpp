#include "minilang/ml/pass_pipeline.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace minilang {

const char* pass_kind_name(OptPassKind pass) {
    switch (pass) {
        case OptPassKind::ConstantFold:
            return "constant_fold";
        case OptPassKind::AlgebraicSimplify:
            return "algebraic_simplify";
        case OptPassKind::DeadTempRemove:
            return "dead_temp_remove";
    }
    return "unknown";
}

namespace {

std::string normalize_pass_name(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name;
}

}  // namespace

OptPassKind pass_kind_from_string(const std::string& name) {
    const std::string normalized = normalize_pass_name(name);
    if (normalized == "constant_fold") {
        return OptPassKind::ConstantFold;
    }
    if (normalized == "algebraic_simplify") {
        return OptPassKind::AlgebraicSimplify;
    }
    if (normalized == "dead_temp_remove") {
        return OptPassKind::DeadTempRemove;
    }
    throw std::runtime_error("unknown optimization pass: " + name);
}

PassPlan all_passes_plan() {
    return PassPlan{
        {OptPassKind::ConstantFold, OptPassKind::AlgebraicSimplify, OptPassKind::DeadTempRemove},
        3,
    };
}

PassPlan no_passes_plan() {
    return PassPlan{{}, 1};
}

}  // namespace minilang
