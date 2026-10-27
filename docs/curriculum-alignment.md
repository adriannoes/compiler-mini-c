---
id: curriculum-alignment
title: Curriculum coverage
type: reference
status: active
summary: Implemented compiler stages, study exercises, course comparisons, and accepted exclusions.
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
  - id: theory-parsing
    path: theory-parsing.md
  - id: theory-llvm
    path: theory-llvm.md
---

# Curriculum coverage

The completed project implements a Mini-C front end, interpreter, bytecode compiler, VM, local optimizations, and checked local arrays.
Parsing theory and a separate C/LLVM laboratory extend the implementation with worked study exercises.
The [roadmap](roadmap.md) records delivery, and the [architecture](architecture.md#reference-map) links the final technical contracts.

## Evidence levels

| Level | Meaning in this repository |
|-------|----------------------------|
| Implemented | Project source code and automated tests provide the behavior |
| Studied | Worked notes or an independent laboratory provide an exercise, without a Mini-C implementation of that topic |
| Excluded | The completed project contains no implementation or completed exercise for that capability |

These levels describe project evidence. They do not imply completion of any university course or its assignments.

## Implemented topics

| Topic | Delivered behavior | Evidence |
|-------|--------------------|----------|
| Lexical analysis | Maximal munch, 35 token types, positions, and owned text | [Lexer](../src/lexer.cpp), [tests](../tests/lexer_test.cpp) |
| Recursive-descent parsing | Precedence, assignment, named calls, control flow, arrays, and deterministic AST output | [Parser](../src/parser.cpp), [AST](../src/ast.hpp), [tests](../tests/parser_test.cpp) |
| Names and scopes | Lexical bindings, shadowing, redeclaration errors, and function visibility | [Resolution](../src/name_resolution.cpp), [symbol table](../src/symbol_table.cpp), [tests](../tests/name_resolution_test.cpp) |
| Static types | Scalar types, operator rules, calls, return paths, and array restrictions | [Type analysis](../src/type_checker.cpp), [rules](../src/type_rules.cpp), [tests](../tests/type_checker_test.cpp) |
| Tree-walk execution | Checked signed 64-bit arithmetic, evaluation order, returns, loops, and recursion | [Interpreter](../src/interpreter.cpp), [numeric helpers](../src/runtime.cpp), [tests](../tests/interpreter_test.cpp) |
| Activation records | Copied parameters, isolated locals, bounded calls, and return restoration | [Interpreter tests](../tests/interpreter_test.cpp), [VM tests](../tests/vm_test.cpp) |
| Intermediate code | Owned stack bytecode with source positions, slots, calls, branches, and source-step markers | [Compiler](../src/bytecode_compiler.cpp), [ISA](architecture.md#bytecode-format-and-compilation), [tests](../tests/bytecode_compiler_test.cpp) |
| Virtual machine | Iterative dispatch and explicit frames, with shared runtime rules | [VM](../src/vm.cpp), [tests](../tests/vm_test.cpp) |
| Local optimization | Guarded constant folding, branch cleanup, jump threading, and reachability cleanup | [Optimizer](../src/bytecode_optimizer.cpp), [tests](../tests/optimizer_test.cpp) |
| Local arrays | Fixed integer or boolean elements, bounds errors, missing values, and declaration resets | [Front-end tests](../tests/arrays_frontend_test.cpp), [execution tests](../tests/arrays_runtime_test.cpp) |
| Diagnostics and commands | Exact output, error positions, stage independence, and exit-status contracts | [Command tests](../tests/cli_test.cmake), [language rules](language.md) |

The compiler suite contains 398 tests. Differential cases compare values, runtime messages, positions, and execution limits across the three engines.
The [build reference](tech-stack.md#build-and-test) provides the commands that reproduce those checks.
The [execution boundaries](architecture.md#execution-boundaries) identify limits that tests do not remove.

## Studied topics

| Topic | Completed exercise | Limit |
|-------|--------------------|-------|
| FIRST, FOLLOW, and LL(1) | Worked sets, a predictive table, and invalid-input decisions in [parsing notes](theory-parsing.md#first-and-follow-by-hand) | No generated predictive table drives the Mini-C parser |
| Bottom-up parsing | A shift/reduce trace and LR(0), SLR, LALR, and LR(1) comparisons in [parsing notes](theory-parsing.md#bottom-up-parsing) | No LR table construction or parser implementation |
| Parser generators | Documented Bison and ANTLR trade-offs in [parsing notes](theory-parsing.md#generator-trade-offs) | No generator dependency |
| LLVM vocabulary | A stage map, SSA values, blocks, calls, and `phi` selections in [LLVM notes](theory-llvm.md) | The Mini-C compiler emits bytecode |
| LLVM IR and native compilation | Clang `-O0` and `-O1` IR, with [annotated examples](theory-llvm.md#read-the-o0-ir) and four native execution checks | The source language for this laboratory is C |

SSA means static single assignment, with one definition for each named value.
The LLVM laboratory reads generated SSA values. It does not implement SSA construction for Mini-C.
Its four native executions check five bounded inputs each. That result establishes the example behavior, rather than general C/Mini-C semantic equivalence.
Standalone `opt` and `llc` execution remains optional and deferred because these tools are unavailable on the recorded host's `PATH`.

## Excluded topics

| Capability | Project boundary |
|------------|------------------|
| Native Mini-C or LLVM backend | The execution target is the custom bytecode VM. Native C laboratory results belong to the study evidence |
| Global constant propagation and general dataflow optimization | The optimizer tracks constants within basic blocks and traverses reachability. It implements no general dataflow or liveness framework |
| Register allocation | VM values use owned containers. The project implements no assignment of values to CPU registers |
| Garbage collection and JIT compilation | Neither has an implementation or completed project exercise |
| Closures, objects, and subtyping | Functions use named calls and scalar signatures. Types remain `int`, `bool`, and `void` |
| Binary bytecode format | Records remain in memory, without a serialized encoding or binary loader |
| Untrusted-code sandbox | Runtime budgets do not bound source size, parser depth, native stack use, or total memory |

The [language exclusions](language.md#excluded-features) also record unsupported source features.
Finishing the approved roadmap does not change these limits.

## Course comparisons

The comparison uses the official pages linked below, reviewed on October 27, 2026.
MIT refers to Spring 2025, and Harvard refers to Fall 2019.
The table compares topics and project targets, rather than equivalent workload, grading, or credentials.

| Reference | Topics in the cited material | Evidence in this project | Difference |
|-----------|-----------------------------|--------------------------|------------|
| [Stanford CS143](https://web.stanford.edu/class/cs143/) | COOL, lexing, parsing, types, runtime environments, code generation, local/global optimization, register allocation, memory management, and JIT | Implemented front end, runtime, bytecode generation, and local optimization. Worked parsing notes | The project implements no COOL compiler, global optimization framework, register allocation, GC, or JIT |
| [MIT 6.110 project, Spring 2025](https://6110-sp25.github.io/project) | Decaf, semantic checking, x86-64 code generation, dataflow optimization, and further performance work | Implemented semantic analysis, lowering, executable VM, and local transforms | The project targets bytecode and implements no native backend, general dataflow framework, or compiler performance competition |
| [Harvard CS153 schedule, Fall 2019](https://groups.seas.harvard.edu/courses/cs153/2019fa/schedule.html) | Assembly, LLVMlite, recursive/LL/LR parsing, OAT, closures, types, dataflow, register allocation, and GC | Implemented recursive descent and type analysis, with worked LR notes and a C/LLVM laboratory | The project implements no x86lite/LLVMlite/OAT ladder, closures, subtyping, dataflow framework, register allocation, or GC |

The [Harvard syllabus](https://groups.seas.harvard.edu/courses/cs153/2019fa/) also describes uses of parsing and analysis outside compiler implementation.
For this project, the engineering evidence below is more specific than a claim about specialist compiler roles.

## Engineering practice demonstrated

| Practice | Concrete evidence |
|----------|-------------------|
| Ownership and lifetime design | Owned token text, AST children, runtime values, bytecode, and isolated call storage |
| Deterministic interfaces | Stable token, AST, binding, and instruction output, with positioned first-error diagnostics |
| Semantic preservation | Comparisons among the interpreter, VM, and optimized VM, including failures and source budgets |
| Resource reasoning | Explicit call frames, positive API limits, and documented limits of those controls |
| Reproducible technical writing | Runnable examples, expected results, linked contracts, and a separate laboratory with recorded tool versions |

A concise portfolio description is: an educational Mini-C compiler with an implemented bytecode VM, local optimizations, checked arrays, and documented theory exercises.
LLVM experience in this repository consists of IR reading and a separate C laboratory.
