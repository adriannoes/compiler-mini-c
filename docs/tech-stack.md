---
id: tech-stack
title: Tech stack
type: reference
status: active
summary: Host language, build system, tooling, and repo conventions.
related:
  - id: readme
    path: ../README.md
  - id: docs-index
    path: README.md
  - id: roadmap
    path: roadmap.md
  - id: language
    path: language.md
  - id: architecture
    path: architecture.md
  - id: theory-llvm
    path: theory-llvm.md
  - id: curriculum-alignment
    path: curriculum-alignment.md
---

# Tech stack

## Core

| Layer | Choice | Notes |
|-------|--------|-------|
| Host language | C++17 | Matches the college compilers course style |
| Build | CMake 3.20+ | Out-of-source builds in `build/` |
| Binary | `minic` | CLI entry point under `src/main.cpp` |
| Tests | CTest and a C++ test executable | Unit tests and command tests under `tests/` |
| VCS | Git (local) | Public GitHub later for portfolio |
| LLVM tools | Optional (phase 12) | `clang -emit-llvm`, `opt`, `llc` for literacy labs only |

## Tooling on this machine

| Tool | Role |
|------|------|
| Apple Clang (`c++` / `clang`) | Compile C++ and later emit sample LLVM IR |
| CMake (Homebrew) | Configure and build |
| Make (CMake generator) | Drive the compile |

## Repo layout

```text
compiler-mini-c/
├── CMakeLists.txt
├── README.md              # overview + todo summary
├── docs/                  # focused docs (this folder)
├── examples/              # Mini-C sample programs (.mc)
├── src/                   # compiler sources
└── tests/                 # automated checks
```

## Build and test

From the repository root, configure and build the project:

```bash
cmake -S . -B build
cmake --build build
./build/minic --help
./build/minic --tokens examples/hello.mc
./build/minic --ast examples/hello.mc
./build/minic --resolve examples/scopes.mc
./build/minic --check examples/types.mc
./build/minic --bytecode examples/factorial.mc
./build/minic --run examples/interpreter.mc
./build/minic --run examples/factorial.mc
./build/minic --run-vm examples/factorial.mc
./build/minic --bytecode-opt examples/optimization.mc
./build/minic --run-vm-opt examples/optimization.mc
ctest --test-dir build --output-on-failure
```

CTest is part of CMake.
CMake enables tests by default.
The tests use only the C++ standard library and CMake.
The C++ tests compare tokens, AST structure, source positions, symbol tables, name bindings, expression types, execution values, bytecode, and diagnostics.
Compiler tests analyze instruction operands and stack types across control flow without executing bytecode.
VM tests compare execution values, source error positions, diagnostics, and limits with the interpreter.
Optimization tests also compare optimized bytecode with both execution engines and require fewer instructions in selected examples.
The command tests compare output, diagnostics, and exit codes.
CMake enables `-Wall -Wextra -Wpedantic` for Clang and GCC, including test targets.

## Sanitizer validation

AddressSanitizer detects memory access errors, and UndefinedBehaviorSanitizer detects selected undefined C++ operations.
Both use compiler instrumentation and add no project dependency.
From the repository root, create a separate Debug build with Clang:

```bash
cmake -S . -B build-optimizer-sanitize \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-optimizer-sanitize
ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build-optimizer-sanitize --output-on-failure
```

The phase 10 validation passes all 350 tests in both normal and sanitizer builds on Apple Clang 17.
Both builds compile without warnings.
Build directories remain outside version control.

## Conventions

- English only: docs, comments, commit messages, user-facing diagnostics
- Hand-written stages without parser generators such as flex, bison, or ANTLR
- Bottom-up / LLVM depth lives in theory docs for literacy, not as the career target
- Prefer small, testable increments aligned with [roadmap.md](roadmap.md)
