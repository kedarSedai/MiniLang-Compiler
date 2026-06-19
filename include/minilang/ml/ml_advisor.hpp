#pragma once

#include "minilang/ml/features.hpp"
#include "minilang/ml/pass_pipeline.hpp"

#include <string>

namespace minilang {

enum class AdvisorMode {
    Heuristic,
    JsonPlan,
    PythonScript,
};

/// Selects an optimization pass plan from HIR features (heuristic or external model).
class MlAdvisor {
public:
    MlAdvisor() = default;

    void set_mode(AdvisorMode mode);
    void set_json_plan_path(std::string path);
    void set_python_script(std::string path);

    PassPlan recommend(const HirFeatures& features) const;
    const std::string& last_explanation() const;

private:
    PassPlan heuristic_recommend(const HirFeatures& features) const;
    PassPlan load_json_plan(const std::string& path) const;
    PassPlan invoke_python(const HirFeatures& features) const;

    AdvisorMode mode_ = AdvisorMode::Heuristic;
    std::string json_plan_path_;
    std::string python_script_;
    mutable std::string last_explanation_;
};

}  // namespace minilang
