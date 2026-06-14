cmake_minimum_required(VERSION 3.16)
project(minilang_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(minilang_frontend
    src/lexer/token.cpp
    src/lexer/lexer.cpp
    src/ast/ast.cpp
    src/parser/parser.cpp
    src/semantic/semantic_analyzer.cpp
    src/hir/hir.cpp
    src/hir/hir_lower.cpp
    src/hir/optimize.cpp
)

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

enable_testing()
add_test(NAME lexer_smoke COMMAND minilang_lexer ${CMAKE_SOURCE_DIR}/tests/programs/factorial.minilang)
add_test(NAME parser_smoke COMMAND minilang_parser ${CMAKE_SOURCE_DIR}/tests/programs/factorial.minilang)
add_test(NAME parser_hello COMMAND minilang_parser ${CMAKE_SOURCE_DIR}/tests/programs/hello.minilang)
add_test(NAME semantic_factorial COMMAND minilang_semantic ${CMAKE_SOURCE_DIR}/tests/programs/factorial.minilang)
add_test(NAME semantic_hello COMMAND minilang_semantic ${CMAKE_SOURCE_DIR}/tests/programs/hello.minilang)
add_test(NAME hir_factorial COMMAND minilang_hir ${CMAKE_SOURCE_DIR}/tests/programs/factorial.minilang)
add_test(NAME hir_hello COMMAND minilang_hir ${CMAKE_SOURCE_DIR}/tests/programs/hello.minilang)
