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
| Tests | CTest and C++ test executables | Unit tests and command tests under `tests/` |
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
│   └── llvm/              # independent C sources and reference LLVM IR
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
./build/minic --run-vm-opt examples/arrays.mc
ctest --test-dir build --output-on-failure
```

CTest is part of CMake.
CMake enables tests by default.
The tests use only the C++ standard library and CMake.
The C++ tests compare tokens, AST structure, source positions, symbol tables, name bindings, expression types, execution values, bytecode, and diagnostics.
Compiler tests analyze instruction operands and stack types across control flow without executing bytecode.
VM tests compare execution values, source error positions, diagnostics, and limits with the interpreter.
Optimization tests also compare optimized bytecode with both execution engines and require fewer instructions in selected examples.
Array tests compare initialization, bounds, evaluation order, recursion, and source limits in all three engines.
The command tests compare output, diagnostics, and exit codes.
CMake enables `-Wall -Wextra -Wpedantic` for Clang and GCC, including test targets.

## LLVM laboratory

The separate LLVM laboratory requires Clang in addition to CMake.
With Clang and CMake available, operate the laboratory from the repository root:

```bash
cmake -P tests/llvm_lab.cmake
```

The command generates IR and checks four native executions under ignored `build-llvm-lab/`.
The laboratory does not register CTest cases or change the compiler build dependencies.
The [LLVM notes](theory-llvm.md#reproduce-the-laboratory) record prerequisites, manual commands, reference IR, and expected results.
The phase 12 checks pass all 398 compiler tests and four laboratory executions without compiler warnings.
The recorded laboratory uses Apple Clang 17.0.0 (`clang-1700.6.4.2`).

## Sanitizer validation

AddressSanitizer detects memory access errors, and UndefinedBehaviorSanitizer detects selected undefined C++ operations.
Both use compiler instrumentation and add no project dependency.
From the repository root, create a separate Debug build with Clang:

```bash
cmake -S . -B build-arrays-sanitize \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-arrays-sanitize
ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build-arrays-sanitize --output-on-failure
```

The phase 11 validation passes all 398 tests in both normal and sanitizer builds on Apple Clang 17.
Both builds compile without warnings.
Build directories remain outside version control.

## Final project audit

The phase 13 normal build passes all 398 CTest cases without compiler warnings.
The example audit operates all nine modes on all 17 Mini-C files and compares the three execution modes, including failures.
Those 153 command results agree with the README gallery and stage boundaries.
The separate LLVM laboratory also passes all four native executions, with five bounded cases each.
The final prose scan reports no findings. Local documentation links, heading anchors, and front-matter paths resolve.
The documentation phase changes no compiler source, example, test, target, or dependency.

## Conventions

- English only: docs, comments, commit messages, user-facing diagnostics
- Hand-written stages without parser generators such as flex, bison, or ANTLR
- Bottom-up / LLVM depth lives in theory docs for literacy, not as the career target
- Prefer small, testable increments aligned with [roadmap.md](roadmap.md)
