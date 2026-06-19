#pragma once

#include "minilang/hir/hir.hpp"

#include <iosfwd>
#include <vector>

namespace minilang {

/// Static metrics extracted from HIR for ML / heuristic pass selection.
struct HirFeatures {
    int function_count = 0;
    int instruction_count = 0;
    int binary_op_count = 0;
    int unary_op_count = 0;
    int call_count = 0;
    int branch_count = 0;
    int jump_count = 0;
    int label_count = 0;
    int const_int_count = 0;
    int const_bool_count = 0;
    int load_store_count = 0;
    int local_count = 0;
    int max_temps_per_function = 0;
    int loop_hint_count = 0;
};

HirFeatures extract_hir_features(const HirModule& module);
void dump_hir_features(std::ostream& out, const HirFeatures& features);
std::vector<double> hir_features_to_vector(const HirFeatures& features);

}  // namespace minilang
