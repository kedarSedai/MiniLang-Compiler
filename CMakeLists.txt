cmake_minimum_required(VERSION 3.16)
project(minilang_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MINILANG_CORE_SRCS
    src/lexer/token.cpp
    src/lexer/lexer.cpp
    src/ast/ast.cpp
    src/parser/parser.cpp
    src/semantic/semantic_analyzer.cpp
    src/hir/hir.cpp
    src/hir/hir_lower.cpp
    src/hir/optimize.cpp
    src/codegen/llvm_emitter.cpp
    src/ml/features.cpp
    src/ml/pass_pipeline.cpp
    src/ml/ml_advisor.cpp
    src/ml/eval_harness.cpp
)

add_library(minilang_frontend ${MINILANG_CORE_SRCS})

target_include_directories(minilang_frontend
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(minilang_frontend PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
)

add_executable(minilang_lexer src/tools/lexer_main.cpp)
target_link_libraries(minilang_lexer PRIVATE minilang_frontend)

add_executable(minilang_parser src/tools/parser_main.cpp)
target_link_libraries(minilang_parser PRIVATE minilang_frontend)

add_executable(minilang_semantic src/tools/semantic_main.cpp)
target_link_libraries(minilang_semantic PRIVATE minilang_frontend)

add_executable(minilang_hir src/tools/hir_main.cpp)
target_link_libraries(minilang_hir PRIVATE minilang_frontend)

add_executable(minilang_compile src/tools/compile_main.cpp)
target_link_libraries(minilang_compile PRIVATE minilang_frontend)

add_executable(minilang_eval src/tools/eval_main.cpp)
target_link_libraries(minilang_eval PRIVATE minilang_frontend)

enable_testing()
add_test(
    NAME lexer_smoke
    COMMAND minilang_lexer ${CMAKE_SOURCE_DIR}/tests/programs/factorial.minilang
)
add_test(
    NAME parser_smoke
    COMMAND minilang_parser ${CMAKE_SOURCE_DIR}/tests/programs/factorial.minilang
)
add_test(
    NAME compile_smoke
    COMMAND minilang_compile ${CMAKE_SOURCE_DIR}/tests/programs/factorial.minilang -o ${CMAKE_BINARY_DIR}/factorial.ll
)
add_test(
    NAME eval_smoke
    COMMAND minilang_eval ${CMAKE_SOURCE_DIR}/tests/programs/factorial.minilang
)
