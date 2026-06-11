# MiniLang Compiler (ML-Guided Optimization Research)

A small research compiler with the classic pipeline: **lexer → parser → semantic analysis → IR → optimization → codegen**. The long-term goal is LLVM-based code generation with an ML advisor for optimization (using pre-trained models, not training from scratch).

## Tech stack choice: C++

| Criterion | C++ | Rust |
|-----------|-----|------|
| LLVM integration | Native C++ API (what LLVM is written in) | `inkwell` wrapper (solid, extra layer) |
| Research artifacts | Easy for reviewers to read if they know compilers | Growing, slightly smaller hiring pool |
| Lexer/parser | Hand-written or Flex/Bison (standard in courses) | `logos` / `lalrpop` (excellent ergonomics) |
| ML inference later | ONNX Runtime C++, Python subprocess, or `llama.cpp` CLI | Same options via FFI |

**Decision:** **C++17** with CMake, hand-written lexer/parser (clear for a paper and debugging), and LLVM for the back end in later phases. Comfort + LLVM alignment outweigh Rust’s safety benefits for this solo timeline.

## Project layout

```
compiler/
  CMakeLists.txt
  include/minilang/     # public headers
  src/                  # implementation
  tests/programs/       # sample MiniLang sources
  docs/LANGUAGE.md      # lexical + syntax sketch
```

## Build

**Makefile (no CMake required):**

```bash
make
./build/minilang_parser tests/programs/hello.minilang
```

**CMake (optional):**

```bash
cmake -S . -B build
cmake --build build
```

## Current phase: Semantic analysis

The front end runs lexer → parser → semantic analyzer. Use the semantic driver to type-check a program:

```bash
make semantic
./build/minilang_semantic tests/programs/factorial.minilang
```

Earlier stages are still available:

```bash
./build/minilang_lexer tests/programs/factorial.minilang
./build/minilang_parser tests/programs/factorial.minilang
```

## Roadmap

1. ~~Lexer~~ ✓
2. ~~Parser + AST~~ ✓
3. ~~Semantic analysis~~ ✓ (current)
4. HIR + rule-based optimizations
5. LLVM IR emission
6. ML optimization advisor + evaluation harness
