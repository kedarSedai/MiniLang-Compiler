CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
FRONTEND_SRCS = src/lexer/token.cpp src/lexer/lexer.cpp src/ast/ast.cpp src/parser/parser.cpp
LEXER_TARGET = build/minilang_lexer
PARSER_TARGET = build/minilang_parser

.PHONY: all clean test lexer parser

all: $(LEXER_TARGET) $(PARSER_TARGET)

lexer: $(LEXER_TARGET)

parser: $(PARSER_TARGET)

$(LEXER_TARGET): $(FRONTEND_SRCS) src/tools/lexer_main.cpp | build
	$(CXX) $(CXXFLAGS) -o $@ $(FRONTEND_SRCS) src/tools/lexer_main.cpp

$(PARSER_TARGET): $(FRONTEND_SRCS) src/tools/parser_main.cpp | build
	$(CXX) $(CXXFLAGS) -o $@ $(FRONTEND_SRCS) src/tools/parser_main.cpp

build:
	mkdir -p build

test: $(LEXER_TARGET) $(PARSER_TARGET)
	./$(LEXER_TARGET) tests/programs/factorial.minilang
	./$(LEXER_TARGET) tests/programs/hello.minilang
	./$(PARSER_TARGET) tests/programs/factorial.minilang
	./$(PARSER_TARGET) tests/programs/hello.minilang

clean:
	rm -rf build
