CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
FRONTEND_SRCS = src/lexer/token.cpp src/lexer/lexer.cpp src/ast/ast.cpp src/parser/parser.cpp \
                src/semantic/semantic_analyzer.cpp src/hir/hir.cpp src/hir/hir_lower.cpp \
                src/hir/optimize.cpp src/codegen/llvm_emitter.cpp
LEXER_TARGET = build/minilang_lexer
PARSER_TARGET = build/minilang_parser
SEMANTIC_TARGET = build/minilang_semantic
HIR_TARGET = build/minilang_hir
COMPILE_TARGET = build/minilang_compile

.PHONY: all clean test lexer parser semantic hir compile

all: $(LEXER_TARGET) $(PARSER_TARGET) $(SEMANTIC_TARGET) $(HIR_TARGET) $(COMPILE_TARGET)

lexer: $(LEXER_TARGET)
parser: $(PARSER_TARGET)
semantic: $(SEMANTIC_TARGET)
hir: $(HIR_TARGET)
compile: $(COMPILE_TARGET)

$(LEXER_TARGET): $(FRONTEND_SRCS) src/tools/lexer_main.cpp | build
	$(CXX) $(CXXFLAGS) -o $@ $(FRONTEND_SRCS) src/tools/lexer_main.cpp

$(PARSER_TARGET): $(FRONTEND_SRCS) src/tools/parser_main.cpp | build
	$(CXX) $(CXXFLAGS) -o $@ $(FRONTEND_SRCS) src/tools/parser_main.cpp

$(SEMANTIC_TARGET): $(FRONTEND_SRCS) src/tools/semantic_main.cpp | build
	$(CXX) $(CXXFLAGS) -o $@ $(FRONTEND_SRCS) src/tools/semantic_main.cpp

$(HIR_TARGET): $(FRONTEND_SRCS) src/tools/hir_main.cpp | build
	$(CXX) $(CXXFLAGS) -o $@ $(FRONTEND_SRCS) src/tools/hir_main.cpp

$(COMPILE_TARGET): $(FRONTEND_SRCS) src/tools/compile_main.cpp | build
	$(CXX) $(CXXFLAGS) -o $@ $(FRONTEND_SRCS) src/tools/compile_main.cpp

build:
	mkdir -p build

test: $(LEXER_TARGET) $(PARSER_TARGET) $(SEMANTIC_TARGET) $(HIR_TARGET) $(COMPILE_TARGET)
	./$(LEXER_TARGET) tests/programs/factorial.minilang > /dev/null
	./$(LEXER_TARGET) tests/programs/hello.minilang > /dev/null
	./$(PARSER_TARGET) tests/programs/factorial.minilang > /dev/null
	./$(PARSER_TARGET) tests/programs/hello.minilang > /dev/null
	./$(SEMANTIC_TARGET) tests/programs/factorial.minilang
	./$(SEMANTIC_TARGET) tests/programs/hello.minilang
	./$(HIR_TARGET) tests/programs/factorial.minilang > /dev/null
	./$(HIR_TARGET) tests/programs/hello.minilang > /dev/null
	./$(COMPILE_TARGET) tests/programs/helloWorld.minilang -o build/helloWorld.ll
	./$(COMPILE_TARGET) tests/programs/factorial.minilang -o build/factorial.ll

clean:
	rm -rf build
