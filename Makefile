CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
FRONTEND_SRCS = src/lexer/token.cpp src/lexer/lexer.cpp src/ast/ast.cpp src/parser/parser.cpp src/semantic/semantic_analyzer.cpp
LEXER_TARGET = build/minilang_lexer
PARSER_TARGET = build/minilang_parser
SEMANTIC_TARGET = build/minilang_semantic

.PHONY: all clean test lexer parser semantic

all: $(LEXER_TARGET) $(PARSER_TARGET) $(SEMANTIC_TARGET)

lexer: $(LEXER_TARGET)

parser: $(PARSER_TARGET)

semantic: $(SEMANTIC_TARGET)

$(LEXER_TARGET): $(FRONTEND_SRCS) src/tools/lexer_main.cpp | build
	$(CXX) $(CXXFLAGS) -o $@ $(FRONTEND_SRCS) src/tools/lexer_main.cpp

$(PARSER_TARGET): $(FRONTEND_SRCS) src/tools/parser_main.cpp | build
	$(CXX) $(CXXFLAGS) -o $@ $(FRONTEND_SRCS) src/tools/parser_main.cpp

$(SEMANTIC_TARGET): $(FRONTEND_SRCS) src/tools/semantic_main.cpp | build
	$(CXX) $(CXXFLAGS) -o $@ $(FRONTEND_SRCS) src/tools/semantic_main.cpp

build:
	mkdir -p build

test: $(LEXER_TARGET) $(PARSER_TARGET) $(SEMANTIC_TARGET)
	./$(LEXER_TARGET) tests/programs/factorial.minilang
	./$(LEXER_TARGET) tests/programs/hello.minilang
	./$(PARSER_TARGET) tests/programs/factorial.minilang
	./$(PARSER_TARGET) tests/programs/hello.minilang
	./$(SEMANTIC_TARGET) tests/programs/factorial.minilang
	./$(SEMANTIC_TARGET) tests/programs/hello.minilang
	./$(SEMANTIC_TARGET) tests/programs/helloWorld.minilang
	! ./$(SEMANTIC_TARGET) tests/programs/semantic_bad_undefined.minilang
	! ./$(SEMANTIC_TARGET) tests/programs/semantic_bad_type.minilang

clean:
	rm -rf build
