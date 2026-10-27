---
id: readme
title: compiler-mini-c
type: overview
status: active
summary: Completed educational compiler, usage, verified examples, and project evidence.
related:
  - id: docs-index
    path: docs/README.md
  - id: roadmap
    path: docs/roadmap.md
  - id: tech-stack
    path: docs/tech-stack.md
  - id: language
    path: docs/language.md
  - id: architecture
    path: docs/architecture.md
  - id: curriculum-alignment
    path: docs/curriculum-alignment.md
  - id: references
    path: docs/references.md
  - id: theory-parsing
    path: docs/theory-parsing.md
  - id: theory-llvm
    path: docs/theory-llvm.md
---

# compiler-mini-c

This educational Mini-C compiler uses C++17.
It implements a manual front end, interpreter, bytecode virtual machine (VM), local optimizations, and checked local arrays.
The approved [roadmap](docs/roadmap.md) is complete. The implementation target is the bytecode VM.
Worked parsing notes and an independent C/LLVM laboratory provide separate study evidence.

## Build and run

Use CMake 3.20 or later and a C++17 compiler.
The compiler and its tests require no external C++ libraries, parser generator, or LLVM installation.
From the repository root, configure and build the project:

```bash
cmake -S . -B build
cmake --build build
./build/minic --run-vm-opt examples/arrays.mc
ctest --test-dir build --output-on-failure
```

The array example prints `10` and exits with status 0.
CTest contains 398 tests. Build directories remain outside version control.

## Implemented behavior

| Surface | Behavior |
|---------|----------|
| Lexer | 35 token types, maximal munch, comments, owned text, and positions that count lines and byte columns from 1 |
| Parser | Recursive descent, operator precedence, named calls, blocks, declarations, assignment, conditions, loops, and local arrays |
| Name analysis | Lexical scopes, shadowing, redeclaration errors, and bindings between uses and declarations |
| Type analysis | Distinct `int`, `bool`, and `void` rules, scalar signatures, return-path checks, and array restrictions |
| Interpreter | Tree-walk execution with ordered evaluation, isolated calls, copied parameters, and checked arithmetic |
| Bytecode VM | 29 opcodes, explicit frames, owned storage, recursion, and source-step accounting |
| Optimizer | Guarded constant folding, known-branch cleanup, jump threading, and reachability cleanup |
| Arrays | Local one-dimensional `int` or `bool` arrays, lengths from 1 through 1024, and checked indexed access |

Integers use the signed 64-bit range. Arithmetic reports positioned overflow and zero-divisor errors.
Variables and array elements have no value until an initializer or assignment gives them one.
Each function activation owns its local storage, and every executed array declaration resets all its elements.
Indexed assignment checks and saves the index before evaluating the assigned value.

Execution defaults to 1000000 source steps and 128 active calls, including the entry frame.
The C++ APIs accept positive custom limits and another parameterless entry. The CLI executes parameterless `main` with the defaults.
A successful integer or boolean result prints with a newline. A successful `void` result prints nothing.
Process status is 0 on success, independently of the Mini-C return value, and 1 on failure.

The [current grammar](docs/language.md#current-grammar) and [execution boundaries](docs/architecture.md#execution-boundaries) define the complete scope and limits.

## Commands

After the build, invoke `./build/minic` from the repository root with one mode and one source path.
Use `./build/minic --help` for usage.

| Mode | Result |
|------|--------|
| `--tokens <file>` | Print token types, owned source text, and positions |
| `--ast <file>` | Print the abstract syntax tree (AST), which records parsed structure |
| `--resolve <file>` | Print scopes, symbols, and declaration bindings |
| `--check <file>` | Analyze names and types, with no output on success |
| `--bytecode <file>` | Print stack instructions after static analysis |
| `--bytecode-opt <file>` | Print optimized stack instructions |
| `--run <file>` | Execute the analyzed program with the interpreter |
| `--run-vm <file>` | Compile and execute the program on the VM |
| `--run-vm-opt <file>` | Compile, optimize, and execute the program on the VM |

The token and AST modes remain independent of name and type rules.
Name resolution does not analyze types or initialization.
Execution and bytecode modes analyze the complete source before compilation or execution.
Diagnostics go to standard error. Failed execution prints no partial result.

For example, inspect the array program before execution:

```bash
./build/minic --tokens examples/arrays.mc
./build/minic --ast examples/arrays.mc
./build/minic --resolve examples/arrays.mc
./build/minic --check examples/arrays.mc
./build/minic --bytecode examples/arrays.mc
./build/minic --bytecode-opt examples/arrays.mc
```

## Executable examples

After the build, pass an example path to `--run`, `--run-vm`, or `--run-vm-opt`.
All three modes produce the same output and status for these examples:

| Example | Demonstrates | Output |
|---------|--------------|--------|
| [hello.mc](examples/hello.mc) | A minimal entry function | `0` |
| [interpreter.mc](examples/interpreter.mc) | Arithmetic, assignment, and nested scope | `42` |
| [calls.mc](examples/calls.mc) | Parameters and nested calls | `6` |
| [scopes.mc](examples/scopes.mc) | Shadowing and a forward function call | `1` |
| [types.mc](examples/types.mc) | Matching types, calls, and returns | `2` |
| [short_circuit.mc](examples/short_circuit.mc) | Logical evaluation that skips a failing operand | `true` |
| [factorial.mc](examples/factorial.mc) | Direct recursion | `720` |
| [fibonacci.mc](examples/fibonacci.mc) | Two recursive calls per case | `55` |
| [optimization.mc](examples/optimization.mc) | Constant expressions, a known branch, and unreachable code | `24` |
| [arrays.mc](examples/arrays.mc) | Integer and boolean arrays, indexed writes, calls, and a loop | `10` |
| [lexer.mc](examples/lexer.mc) | All 33 original token types. The array example adds both brackets | `0` |

For example, compare recursive execution paths:

```bash
./build/minic --run examples/factorial.mc
./build/minic --run-vm examples/factorial.mc
./build/minic --run-vm-opt examples/factorial.mc
```

Each command prints `720` and exits with status 0.

## Analysis and error examples

These files demonstrate stage boundaries or expected failures:

| Example | Expected behavior |
|---------|-------------------|
| [expressions.mc](examples/expressions.mc) | `--ast` succeeds. Name analysis rejects undeclared `left` at line 9, column 10 |
| [control_flow.mc](examples/control_flow.mc) | Analysis succeeds. Execution reports a missing `main` because this file defines only helper functions |
| [recursion.mc](examples/recursion.mc) | Analysis accepts mutual recursion. Execution reports a missing `main` |
| [type_error.mc](examples/type_error.mc) | Syntax and names succeed. Type analysis rejects an integer initializer for `bool` at line 3, column 16 |
| [runtime_error.mc](examples/runtime_error.mc) | Type analysis succeeds. Execution rejects a variable read without a value at line 3, column 15 |
| [array_error.mc](examples/array_error.mc) | Type analysis succeeds. Execution rejects `values[1]` without a value at line 5, column 10 |

For example, after the build, operate this command from the repository root:

```bash
./build/minic --run-vm-opt examples/array_error.mc
```

It prints no standard output and exits with status 1. Its diagnostic is:

```text
examples/array_error.mc:5:10: error: The array element "values[1]" has no value.
```

## Studied concepts and LLVM laboratory

The [parsing notes](docs/theory-parsing.md) contain worked FIRST/FOLLOW sets, LL(1) decisions, and a shift/reduce trace.
They compare LR constructions and parser-generator trade-offs without adding a generated parser.

The [LLVM notes](docs/theory-llvm.md) map the implemented stages to Clang/LLVM.
A separate C laboratory compares generated LLVM intermediate representation (IR) at `-O0` and `-O1` and follows the loop's value selections.
With Clang and CMake available, operate it from the repository root:

```bash
cmake -P tests/llvm_lab.cmake
```

Four native executions check five bounded cases each, print `10`, and exit with status 0.
The script records the compiler version and results under ignored `build-llvm-lab/`.
The annotated IR examples record Apple Clang 17.0.0 and its target metadata.
The Mini-C compiler emits bytecode. Native C laboratory results do not establish a native Mini-C backend.

## Validation and limits

The compiler suite contains 398 CTest cases for stages, commands, values, source positions, diagnostics, ownership, and execution limits.
Differential cases compare the interpreter, VM, and optimized VM, including stopped execution and runtime failures.
The gallery checks all 17 Mini-C files across the three execution modes.
The [tooling reference](docs/tech-stack.md#sanitizer-validation) records the 398-test AddressSanitizer/UndefinedBehaviorSanitizer baseline from phase 11 and its reproduction commands.
The compiler sources and CMake targets retain that baseline's contents through the documentation and LLVM study phases.

The language excludes pointers, structs, unions, strings, floating-point values, globals, prototypes, and dynamic allocation.
Arrays cannot act as whole values, parameters, or return values. The language excludes initializer lists and multidimensional arrays.
The project implements no general dataflow framework, register allocation, garbage collector, JIT, binary bytecode loader, or LLVM backend.
Standalone `opt` and `llc` execution remains optional and deferred.
Runtime budgets do not bound parser nesting, source size, native stack use, or total memory. This project is an educational compiler rather than a sandbox.
The [coverage report](docs/curriculum-alignment.md) separates implemented work, completed study exercises, and excluded capabilities.

## Documentation

| Document | Purpose |
|----------|---------|
| [Architecture](docs/architecture.md#reference-map) | Pipeline, ownership, complete instruction set, limits, and build commands |
| [Language](docs/language.md#current-grammar) | Current grammar, types, evaluation order, diagnostics, and accepted limits |
| [Coverage](docs/curriculum-alignment.md) | Project evidence and comparisons with cited course material |
| [Parsing theory](docs/theory-parsing.md) | FIRST/FOLLOW, LL/LR examples, and generator decisions |
| [LLVM study](docs/theory-llvm.md) | Stage mapping, annotated IR, tool roles, and a separate native laboratory |
| [Tooling](docs/tech-stack.md) | Dependencies, build, tests, laboratory, and sanitizer commands |
| [Roadmap](docs/roadmap.md) | Completed required phases and deferred optional work |
| [References](docs/references.md) | Books and study terms by phase |
| [Documentation index](docs/README.md) | Catalog and front-matter conventions |
