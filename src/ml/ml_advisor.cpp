#include "minilang/ml/ml_advisor.hpp"

#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace minilang {

namespace {

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

std::string trim(const std::string& value) {
    const std::size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const std::size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::string read_file_or_throw(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("could not open advisor file: " + path);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

int extract_json_int(const std::string& json, const std::string& key, int fallback) {
    const std::string needle = "\"" + key + "\"";
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return fallback;
    }
    const std::size_t colon = json.find(':', key_pos + needle.size());
    if (colon == std::string::npos) {
        return fallback;
    }
    std::size_t pos = colon + 1;
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    return std::stoi(json.substr(pos));
}

std::vector<std::string> extract_json_string_array(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return {};
    }
    const std::size_t bracket_start = json.find('[', key_pos);
    const std::size_t bracket_end = json.find(']', bracket_start);
    if (bracket_start == std::string::npos || bracket_end == std::string::npos) {
        return {};
    }

    std::vector<std::string> values;
    std::size_t pos = bracket_start + 1;
    while (pos < bracket_end) {
        const std::size_t quote_start = json.find('"', pos);
        if (quote_start == std::string::npos || quote_start >= bracket_end) {
            break;
        }
        const std::size_t quote_end = json.find('"', quote_start + 1);
        if (quote_end == std::string::npos || quote_end > bracket_end) {
            break;
        }
        values.push_back(json.substr(quote_start + 1, quote_end - quote_start - 1));
        pos = quote_end + 1;
    }
    return values;
}

PassPlan parse_plan_json(const std::string& json) {
    PassPlan plan;
    plan.max_iterations = extract_json_int(json, "max_iterations", 3);

    const std::vector<std::string> pass_names = extract_json_string_array(json, "passes");
    plan.passes.reserve(pass_names.size());
    for (const std::string& name : pass_names) {
        plan.passes.push_back(pass_kind_from_string(name));
    }
    return plan;
}

std::string features_to_json(const HirFeatures& features) {
    std::ostringstream out;
    out << '{';
    out << "\"function_count\":" << features.function_count << ',';
    out << "\"instruction_count\":" << features.instruction_count << ',';
    out << "\"binary_op_count\":" << features.binary_op_count << ',';
    out << "\"unary_op_count\":" << features.unary_op_count << ',';
    out << "\"call_count\":" << features.call_count << ',';
    out << "\"branch_count\":" << features.branch_count << ',';
    out << "\"jump_count\":" << features.jump_count << ',';
    out << "\"label_count\":" << features.label_count << ',';
    out << "\"const_int_count\":" << features.const_int_count << ',';
    out << "\"const_bool_count\":" << features.const_bool_count << ',';
    out << "\"load_store_count\":" << features.load_store_count << ',';
    out << "\"local_count\":" << features.local_count << ',';
    out << "\"max_temps_per_function\":" << features.max_temps_per_function << ',';
    out << "\"loop_hint_count\":" << features.loop_hint_count;
    out << '}';
    return out.str();
}

}  // namespace

void MlAdvisor::set_mode(AdvisorMode mode) {
    mode_ = mode;
}

void MlAdvisor::set_json_plan_path(std::string path) {
    json_plan_path_ = std::move(path);
}

void MlAdvisor::set_python_script(std::string path) {
    python_script_ = std::move(path);
}

const std::string& MlAdvisor::last_explanation() const {
    return last_explanation_;
}

PassPlan MlAdvisor::recommend(const HirFeatures& features) const {
    switch (mode_) {
        case AdvisorMode::Heuristic:
            return heuristic_recommend(features);
        case AdvisorMode::JsonPlan:
            return load_json_plan(json_plan_path_);
        case AdvisorMode::PythonScript:
            return invoke_python(features);
    }
    return all_passes_plan();
}

PassPlan MlAdvisor::heuristic_recommend(const HirFeatures& features) const {
    PassPlan plan;
    plan.max_iterations = 3;

    if (features.instruction_count <= 8 && features.binary_op_count == 0) {
        plan.passes = {OptPassKind::DeadTempRemove};
        last_explanation_ = "tiny program: dead-temp cleanup only";
        return plan;
    }

    if (features.binary_op_count >= 4 || features.const_int_count >= 3) {
        plan.passes.push_back(OptPassKind::ConstantFold);
    }
    if (features.binary_op_count >= 2) {
        plan.passes.push_back(OptPassKind::AlgebraicSimplify);
    }
    if (features.max_temps_per_function >= 3 || features.loop_hint_count > 0 ||
        features.branch_count > 0) {
        plan.passes.push_back(OptPassKind::DeadTempRemove);
    }

    if (plan.passes.empty()) {
        plan = all_passes_plan();
        last_explanation_ = "default full pipeline";
    } else {
        last_explanation_ = "heuristic selected passes: " + plan_to_string(plan);
    }
    return plan;
}

PassPlan MlAdvisor::load_json_plan(const std::string& path) const {
    const std::string json = read_file_or_throw(path);
    PassPlan plan = parse_plan_json(json);
    if (plan.passes.empty()) {
        plan = all_passes_plan();
    }
    last_explanation_ = "loaded plan from " + path + ": " + plan_to_string(plan);
    return plan;
}

PassPlan MlAdvisor::invoke_python(const HirFeatures& features) const {
    if (python_script_.empty()) {
        throw std::runtime_error("python advisor script path is empty");
    }

    const std::string payload = features_to_json(features);
    const std::string command =
        "python3 \"" + python_script_ + "\" '" + payload + "' 2>/dev/null";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("failed to invoke python advisor");
    }

    std::string output;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    const int status = pclose(pipe);
    if (status != 0) {
        throw std::runtime_error("python advisor exited with status " + std::to_string(status));
    }

    PassPlan plan = parse_plan_json(trim(output));
    if (plan.passes.empty()) {
        plan = heuristic_recommend(features);
        last_explanation_ = "python returned empty plan; fell back to heuristic";
    } else {
        last_explanation_ = "python advisor plan: " + plan_to_string(plan);
    }
    return plan;
}

}  // namespace minilang
