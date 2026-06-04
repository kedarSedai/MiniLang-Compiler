CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
SRCS = src/lexer/token.cpp src/lexer/lexer.cpp src/tools/lexer_main.cpp
TARGET = build/minilang_lexer

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRCS) | build
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

build:
	mkdir -p build

test: $(TARGET)
	./$(TARGET) tests/programs/factorial.minilang
	./$(TARGET) tests/programs/hello.minilang

clean:
	rm -rf build
