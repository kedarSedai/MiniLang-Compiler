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
make compile
```

**CMake (optional):**

```bash
cmake -S . -B build
cmake --build build
```

## How to run

Compile a MiniLang program to LLVM IR, build with `clang`, and run (requires `clang`):

```bash
make compile
./build/minilang_compile tests/programs/factorial.minilang -o build/out.ll --run
echo exit:$?
```

For `factorial.minilang`, `echo exit:$?` prints `120` (factorial of 5). Programs communicate numeric results via `return` from `main` until a `print` builtin is added.

Emit LLVM IR only (no run):

```bash
./build/minilang_compile tests/programs/factorial.minilang -o build/out.ll
```

Earlier pipeline stages:

```bash
./build/minilang_lexer tests/programs/factorial.minilang
./build/minilang_parser tests/programs/factorial.minilang
./build/minilang_semantic tests/programs/factorial.minilang
./build/minilang_hir tests/programs/factorial.minilang
## Current phase: LLVM IR emission

The compiler lowers optimized HIR to **textual LLVM IR** (`.ll` files):

```bash
make compile
./build/minilang_compile tests/programs/helloWorld.minilang -o build/helloWorld.ll
```

Compile and run (requires `clang`):

```bash
./build/minilang_compile tests/programs/helloWorld.minilang -o build/helloWorld.ll --run
clang build/factorial.ll -o build/factorial && ./build/factorial; echo exit:$?
```

Earlier stages:

```bash
./build/minilang_hir tests/programs/factorial.minilang
./build/minilang_semantic tests/programs/factorial.minilang
```

## Roadmap

1. ~~Lexer~~ ✓
2. ~~Parser + AST~~ ✓
3. ~~Semantic analysis~~ ✓
4. ~~HIR + rule-based optimizations~~ ✓
5. ~~LLVM IR emission~~ ✓ (current)
6. ML optimization advisor + evaluation harness
