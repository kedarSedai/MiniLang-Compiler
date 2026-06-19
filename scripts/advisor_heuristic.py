#!/usr/bin/env python3
"""Example external ML advisor: reads HIR feature JSON, prints a pass plan."""

import json
import sys


def recommend(features: dict) -> dict:
    passes = []
    if features.get("binary_op_count", 0) >= 4 or features.get("const_int_count", 0) >= 3:
        passes.append("constant_fold")
    if features.get("binary_op_count", 0) >= 2:
        passes.append("algebraic_simplify")
    if (
        features.get("max_temps_per_function", 0) >= 3
        or features.get("loop_hint_count", 0) > 0
        or features.get("branch_count", 0) > 0
    ):
        passes.append("dead_temp_remove")
    if not passes:
        passes = ["constant_fold", "algebraic_simplify", "dead_temp_remove"]
    return {"passes": passes, "max_iterations": 3}


def main() -> int:
    if len(sys.argv) != 2:
        print('{"passes":["constant_fold","algebraic_simplify","dead_temp_remove"],"max_iterations":3}')
        return 0
    features = json.loads(sys.argv[1])
    print(json.dumps(recommend(features)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
