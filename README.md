---
id: readme
title: compiler-mini-c
type: overview
status: active
summary: Project purpose, current behavior, build commands, and roadmap summary.
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
---

# compiler-mini-c

This project is an educational Mini-C compiler in C++17.
Each stage uses hand-written code, examples, and automated tests.
The repository records the work for a public portfolio.

The goal is to learn undergraduate compiler concepts and apply them to general software engineering.
The execution target is a custom bytecode virtual machine.
Parsing theory and LLVM study support technical discussions. Specialist compiler engineering is outside the project scope.

The project plan is complete.
Phase checklists and study terms live in [docs/roadmap.md](docs/roadmap.md) and [docs/references.md](docs/references.md).

## Current behavior

The lexer converts source text into tokens, which are units such as keywords, names, and numbers.
The `--tokens` command prints each token with its original text, line, and column.
The command reads a source file, skips comments, and reports lexical errors.

The lexer recognizes `int`, `bool`, `void`, `if`, `else`, `while`, `return`, `true`, and `false`.
It also recognizes identifiers, decimal integers, punctuation, and all operators in the phase 1 scope.
The lexer chooses the longest supported operator and distinguishes division from comments.
Phase 1 is complete, with 33 token types including `EOF`.
The parser builds an abstract syntax tree (AST), which records program structure.
The `--ast` command prints the tree after lexing and parsing succeed.
The parser supports expressions, functions with parameters, named calls, local declarations, blocks, and return statements.
It also supports `if`/`else` and `while`, including bodies without braces.
Phase 2 is complete within the [approved grammar](docs/language.md#phase-2-grammar).
The token and AST commands do not execute programs.
See [docs/language.md](docs/language.md) for lexical rules, grammar, precedence, and accepted limits.

Phase 3 study notes include worked FIRST/FOLLOW sets, LL(1) decisions, and a shift/reduce trace.
They compare LR constructions and parser-generator choices in [docs/theory-parsing.md](docs/theory-parsing.md).

Phase 4 provides a symbol table, nested scopes, and `minic::resolve_names` in the front-end library.
The approved [name rules](docs/language.md#phase-4-name-rules) record visibility, shadowing, and redeclaration policies.
The analysis returns separate declaration bindings without modifying the AST.
The `--resolve` command prints scopes, symbols, and the declaration selected for each name use.

Phase 5 provides `minic::check_types` and explicit rules for `int`, `bool`, and `void` in the front-end library.
The approved [type rules](docs/language.md#phase-5-type-rules) define operators, declarations, calls, conditions, and returns.
The analysis returns expression types and owns its name-resolution result without modifying the AST.
The `--check` command analyzes the complete program and returns `0` without output when the analysis succeeds.

Phase 6 provides `minic::interpret`, owned runtime values, and guarded signed 64-bit arithmetic in a separate library.
The approved [execution rules](docs/language.md#phase-6-execution-rules) define numeric errors, initialization behavior, entry selection, and the supported subset.
The API executes the supported subset after static analysis and returns a value that does not borrow the AST.
The `--run` command executes `main` without parameters and prints its integer or boolean result.
Phase 7 adds `if`/`else`, `while`, and a configurable step budget to execution.
Function calls use isolated frames and copied parameters, including direct and mutual recursion.
The [phase 7 rules](docs/language.md#phase-7-execution-rules) define these behaviors and the configurable execution limits.

Phase 8 provides owned bytecode records, `minic::compile_bytecode`, and the `minic::print_bytecode` disassembler, which prints instructions for inspection.
The [bytecode contract](docs/architecture.md#bytecode-format-and-compilation) defines a stack instruction set with source positions and explicit step markers.
Compilation covers the current language after static analysis.
The `--bytecode` command prints instructions without executing them.

Phase 9 provides `minic::execute_bytecode` for entries, local variables, operations, branches, loops, calls, and returns.
It shares numeric rules and source step limits with the interpreter and uses explicit frames for recursion.
The `--run-vm` command compiles the source and executes `main` on the VM.
The [VM rules](docs/language.md#phase-9-vm-rules) define the approved execution contract.

Phase 10 provides `minic::optimize_bytecode`, local constant folding, unreachable-code cleanup, and small instruction rewrites in owned bytecode.
The [optimization rules](docs/language.md#phase-10-optimization-rules) preserve source steps, numeric failures, effects, and the original input.
The `--bytecode-opt` command prints optimized instructions, and `--run-vm-opt` executes them on the VM.

Phase 11 adds syntax, bindings, type analysis, and bytecode for fixed local `int` and `bool` arrays.
Lengths range from 1 through 1024, and brackets add two token types for a total of 35.
The [array rules](docs/language.md#phase-11-local-arrays) define bounds, initialization, and indexed assignment order.
Array execution remains in progress.

## Build and use

From the repository root, configure and build with CMake 3.20 or later and a C++17 compiler:

```bash
cmake -S . -B build
cmake --build build
./build/minic --tokens examples/hello.mc
./build/minic --tokens examples/lexer.mc
./build/minic --ast examples/hello.mc
./build/minic --ast examples/expressions.mc
./build/minic --ast examples/calls.mc
./build/minic --ast examples/control_flow.mc
./build/minic --ast examples/lexer.mc
./build/minic --resolve examples/scopes.mc
./build/minic --resolve examples/recursion.mc
./build/minic --check examples/types.mc
./build/minic --check examples/recursion.mc
./build/minic --bytecode examples/hello.mc
./build/minic --bytecode examples/factorial.mc
./build/minic --run examples/interpreter.mc
./build/minic --run examples/short_circuit.mc
./build/minic --run examples/factorial.mc
./build/minic --run examples/fibonacci.mc
./build/minic --run-vm examples/factorial.mc
./build/minic --run-vm examples/fibonacci.mc
./build/minic --bytecode-opt examples/optimization.mc
./build/minic --run-vm-opt examples/optimization.mc
```

The token output includes the source position and ends with `EOF`:

```text
3:1 INT "int"
3:5 IDENTIFIER "main"
3:9 LEFT_PAREN "("
3:10 RIGHT_PAREN ")"
3:12 LEFT_BRACE "{"
4:3 RETURN "return"
4:10 INTEGER "0"
4:11 SEMICOLON ";"
5:1 RIGHT_BRACE "}"
6:1 EOF ""
```

Use `./build/minic --help` to see the available commands.
The [lexer example](examples/lexer.mc) includes every token type, with commas in parameters and calls.
Its expected output lives in [tests/lexer.tokens](tests/lexer.tokens).
The [expression example](examples/expressions.mc) demonstrates precedence, grouping, and assignment without evaluating them.
Its expected tree lives in [tests/expressions.ast](tests/expressions.ast).
The [call example](examples/calls.mc) adds parameters, declarations, nested calls, and an empty statement.
The [control-flow example](examples/control_flow.mc) demonstrates the nearest-`if` rule for `else` and a braced loop.
The expected ASTs for parser examples live under `tests/`.

The [scope example](examples/scopes.mc) demonstrates nested scopes, shadowing, and a call before its function definition.
The [recursion example](examples/recursion.mc) demonstrates mutual recursion without a `main` function.
Their complete expected resolution output lives in [tests/scopes.resolution](tests/scopes.resolution) and [tests/recursion.resolution](tests/recursion.resolution).
`Read`, `Write`, and `Call` lines identify both the use position and its declaration symbol.

Name resolution does not analyze types, argument counts, return compatibility, or definite initialization.
For example, `int x = x;` resolves to its own declaration without establishing that `x` holds an initialized value.
The existing expression example remains valid with `--ast` but fails with `--resolve` because `left` and `right` have no declarations.

The [type example](examples/types.mc) demonstrates matching calls, boolean conditions, and compatible returns.
The [type-error example](examples/type_error.mc) has valid syntax and names, but assigns an integer initializer to a boolean variable.
Both `--ast` and `--resolve` accept that file.
From the repository root, operate the type-check command after the build:

```bash
./build/minic --check examples/type_error.mc
```

The command returns `1`, prints no standard output, and writes this diagnostic to standard error:

```text
examples/type_error.mc:3:16: error: Expected type bool. Received int.
```

Type checking preserves the existing grammar and does not evaluate expressions or execute programs.
It does not enforce numeric limits, analyze definite initialization, or require `main`.
Non-void functions require a guaranteed return under the [conservative return rules](docs/language.md#functions-and-conditions).

The [interpreter example](examples/interpreter.mc) prints `42` after assignment and nested scopes.
The [short-circuit example](examples/short_circuit.mc) prints `true` without evaluating skipped assignments or division by zero.
Both commands return status `0`. A `void` entry produces no output.
The printed result does not become the process exit status.

The [factorial example](examples/factorial.mc) prints `720` for `factorial(6)`.
The [Fibonacci example](examples/fibonacci.mc) prints `55` for `fibonacci(10)`.
Both use recursive calls with separate parameter and local values for each activation.
The existing call, scope, and type examples also execute with `--run` and print `6`, `1`, and `2`, respectively.
The control-flow and mutual-recursion examples lack `main`. Command tests add temporary entry functions without changing those source examples.

The [runtime-error example](examples/runtime_error.mc) passes `--check` but fails with `--run` because its initializer reads a variable without a value.
From the repository root, operate the execution command after the build:

```bash
./build/minic --run examples/runtime_error.mc
```

The command returns `1`, prints no standard output, and writes this diagnostic to standard error:

```text
examples/runtime_error.mc:3:15: error: The variable "value" has no value.
```

The interpreter rejects out-of-range literals, integer overflow, division by zero, and remainder by zero when execution reaches them.
It completes static analysis for every function before it executes the selected entry.
Calls, conditions, and loops execute, while syntax-only examples retain their original purpose.
The entry still requires zero parameters, and called functions receive copied argument values.
Execution defaults to at most 1000000 expression or statement steps and 128 active calls, including the entry.
These limits bound loops and recursion. The CLI uses fixed defaults and adds no limit flags.
The API accepts positive limits through `interpret(program, entry, ExecutionLimits{max_steps, max_call_depth})`.
The original `interpret(program, entry)` API uses the defaults.

The bytecode command accepts programs without `main`, including the control-flow and mutual-recursion examples.
It also accepts runtime-error examples because compilation does not read variables or evaluate arithmetic.
The [hello bytecode fixture](tests/hello.bytecode) records its complete instruction output:

```text
bytecode
function 0 "main" int parameters=0 @3:5
  constant 0 int "0"
  0 STEP @3:12
  1 STEP @4:3
  2 STEP @4:10
  3 LOAD_CONSTANT 0 @4:10
  4 RETURN @4:3
  5 UNREACHABLE @3:12
end
```

Indices start at zero. `STEP` marks a source expression or statement entry, and `@line:column` records its source position.
`UNREACHABLE` guards against internal fallthrough from a non-void function.
The [instruction table](docs/architecture.md#bytecode-format-and-compilation) defines operands and stack effects for the VM.

The `--run-vm` command prints `720` for the factorial example and `55` for the Fibonacci example.
Both commands return status `0`. Integers and booleans include a newline, and a `void` result produces no output.
The command compiles all functions before selecting `main`, so static errors retain priority over execution errors.
The VM requires an entry without parameters, while `--bytecode` still accepts programs without an entry.
The control-flow and mutual-recursion tests add temporary entries without changing their source examples.

The interpreter and VM share signed 64-bit numeric rules, missing-value errors, and the default execution limits.
Only source steps consume the budget. The VM uses explicit frames, so recursive Mini-C calls do not use recursive C++ calls.
The API accepts custom limits through `execute_bytecode(bytecode, entry, limits)` and returns a value independent of the bytecode lifetime.
The existing `--run` command continues to use the tree-walk interpreter.

The [optimization example](examples/optimization.mc) combines constant expressions, a known branch, a discarded value, and code after a return.
It prints `24` through `--run`, `--run-vm`, and `--run-vm-opt`, with process status `0`.
Optimization reduces its instruction sequence from 69 instructions to 36 while preserving every executed source step.
These counts describe generated code, without a timing claim.
The complete outputs live in [the original fixture](tests/optimization.bytecode) and [the optimized fixture](tests/optimization.optimized.bytecode).

Optimization preserves error positions, argument order, calls, local reads, and the configured execution limits.
It retains operations that can fail when execution reaches them. It can omit operations on paths that execution never reaches.
The optimizer retains unused pool constants and every function signature, so named entry selection remains available.
The existing disassembly and execution commands keep their original behavior.

## Tests

After the build, run the tests from the repository root:

```bash
ctest --test-dir build --output-on-failure
```

The 360 tests cover tokens, AST structure, source positions, names, types, interpretation, bytecode, VM execution, optimization, arrays, and commands.
CTest runs the tests without an external test library.
The command tests compare output, diagnostics, and exit codes.
The 350 tests from phase 10 also pass with AddressSanitizer and UndefinedBehaviorSanitizer.
The sanitizer commands live in [docs/tech-stack.md](docs/tech-stack.md#sanitizer-validation).

## Docs

| Doc | What it covers |
|-----|----------------|
| [docs/README.md](docs/README.md) | Index and front-matter conventions |
| [docs/roadmap.md](docs/roadmap.md) | Full learning plan, checklists, lookup terms |
| [docs/references.md](docs/references.md) | Suggested books and per-phase search terms |
| [docs/tech-stack.md](docs/tech-stack.md) | Tools, language, build |
| [docs/language.md](docs/language.md) | Mini-C scope and design choices |
| [docs/architecture.md](docs/architecture.md) | Compiler pipeline map |
| [docs/curriculum-alignment.md](docs/curriculum-alignment.md) | Course coverage, career goals, and this plan |
| [docs/theory-parsing.md](docs/theory-parsing.md) | First/Follow and bottom-up parsing (literacy) |
| [docs/theory-llvm.md](docs/theory-llvm.md) | LLVM concepts for conversation fluency |

## Todo (summary)

Source of truth for details: [docs/roadmap.md](docs/roadmap.md).

| Phase | Topic | Status |
|------:|-------|--------|
| 0 | Project scaffold | done |
| 1 | Lexer (+ tests) | done |
| 2 | Recursive-descent parser | done |
| 3 | Parsing theory (First/Follow, bottom-up) | done |
| 4 | Symbol table / scopes | done |
| 5 | Type checking | done |
| 6 | Tree-walk interpreter (stage A) | done |
| 7 | Functions and control flow | done |
| 8 | Bytecode compiler | done |
| 9 | VM (stage B, primary target) | done |
| 10 | Local optimizations | done |
| 11 | Optional language features | in progress |
| 12 | LLVM literacy | pending |
| 13 | Wrap-up / portfolio polish | pending |

Phase 2 delivers syntax and AST output, and phase 3 records the related theory.
Phase 4 adds name resolution without changing those syntax contracts.
Phase 5 adds type analysis and the independent `--check` command.
Phase 6 executes the expression and block subset through `--run`.
Phase 7 adds calls, control flow, and recursion before bytecode and VM work.
Phase 8 compiles the same language to inspectable bytecode.
Phase 9 executes bytecode with explicit frames and compares its results and diagnostics with the interpreter.
Phase 10 optimizes bytecode with guarded constant folding and local control-flow cleanup.
See [docs/tech-stack.md](docs/tech-stack.md) for build requirements and conventions.
