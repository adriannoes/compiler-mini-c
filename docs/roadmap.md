---
id: roadmap
title: Roadmap
type: plan
status: active
summary: Full phase-by-phase learning and build plan for the Mini-C compiler.
related:
  - id: readme
    path: ../README.md
  - id: docs-index
    path: README.md
  - id: tech-stack
    path: tech-stack.md
  - id: language
    path: language.md
  - id: architecture
    path: architecture.md
  - id: curriculum-alignment
    path: curriculum-alignment.md
  - id: theory-parsing
    path: theory-parsing.md
  - id: theory-llvm
    path: theory-llvm.md
  - id: references
    path: references.md
---

# Roadmap

High-level status lives in [../README.md](../README.md). This file holds the detailed plan.

Planning status: closed. Study companions and index search terms: [references.md](references.md).

The goal is a complete Mini-C front end and bytecode virtual machine for the portfolio.
The study plan also covers undergraduate theory, especially bottom-up parsing, and LLVM concepts for technical discussions.
Specialist compiler engineering is outside the scope.
See [curriculum-alignment.md](curriculum-alignment.md) and [language.md](language.md).

Each implementation phase includes hand-written code, small examples, automated tests, and a short learning note.
The pipeline lives in [architecture.md](architecture.md).

Each phase lists lookup terms as they appear in English textbooks.
Use these terms in the Dragon Book, *Crafting Interpreters*, or Tiger Book indexes.
See [references.md](references.md).

## Phase 0: Project map

Lookup terms: `compiler architecture`, `front end`, `back end`, `middle end`, `pass`

- [x] Local repo and CMake skeleton
- [x] Split docs (README summary + focused docs with front matter)
- [x] Build on this machine (`cmake` + C++17)
- [x] Align plan with typical syllabi and market transfer skills
- [x] Book/lookup guide in [references.md](references.md)
- [ ] Keep docs cross-linked as the project grows

## Phase 1: Lexer

Status: complete.

Lookup terms: `lexical analysis`, `scanner`, `token`, `lexeme`, `regular expression`, `finite automaton`, `DFA`, `NFA`, `maximal munch`, `keyword`, `identifier`, `source location`

- [x] Token types and `SourceLocation` (line/column)
- [x] Scan `int`, `return`, identifiers, decimal integers, punctuation, and comments
- [x] Scan `bool`, `void`, `if`, `else`, `while`, `true`, and `false`
- [x] Extend unit and command tests for keywords and identifier boundaries
- [x] Token dump mode for debugging
- [x] Unit tests and command tests for valid and invalid input
- [x] Scan the approved operators and comma
- [x] Extend tests and examples for operators and the comma
- [x] Record a lexer learning note in [architecture.md](architecture.md#lexer-learning-note)

The approved scope adds seven keywords, fifteen operators, and a comma.
The exact token list and limits live in [language.md](language.md#phase-1-scope).
The completion work uses two local commits, with a build, tests, and prose review before each commit.
The first commit adds the keywords, keyword boundary tests, and documentation.
The second commit adds operators and the comma, regression tests, and an example with expected command output.

Phase 1 is complete. The parser scope received separate approval.

The first increment reads `examples/hello.mc` and reports token types, text, and positions.
The lexer reports unsupported characters and unterminated block comments.
The lexer recognizes all 33 token types in the agreed scope.
The 21 tests cover keywords, identifier boundaries, operators, comments, source positions, and command behavior.
The [lexer example](../examples/lexer.mc) contains every token type.
Its expected command output lives in [tests/lexer.tokens](../tests/lexer.tokens).
Phase 1 is complete.
See [language.md](language.md) for lexical rules and [architecture.md](architecture.md) for the implementation.

## Phase 2: Recursive-descent parser

Status: complete.

The approved grammar and operator rules live in [language.md](language.md#phase-2-grammar).
AST ownership and parser boundaries live in [architecture.md](architecture.md).
Implementation uses three local increments:

1. Expressions, functions without parameters, blocks, return statements, AST output, examples, and tests.
2. Local declarations, expression statements, empty statements, parameters, calls, and their tests.
3. `if`/`else`, `while`, nearest-`if` tests, and complete example coverage.

Each increment includes documentation, a build without warnings, applicable tests, and a diff review before its local commit.
The final validation also uses AddressSanitizer and UndefinedBehaviorSanitizer.
Semantic analysis and execution remain outside phase 2.

Increment 1 is complete: expressions without calls, functions without parameters, blocks, returns, and the AST command.
The [expression example](../examples/expressions.mc) has an expected tree in [tests/expressions.ast](../tests/expressions.ast).
Increment 2 is complete: parameters, local declarations, calls, expression statements, and empty statements.
The [call example](../examples/calls.mc) has an expected tree in [tests/calls.ast](../tests/calls.ast).
Increment 3 is complete: `if`/`else`, `while`, nearest-`if` tests, and expected trees for every example.
The [control-flow example](../examples/control_flow.mc) demonstrates bodies with and without braces.
The 52 tests include the original 21 lexer and command tests.
All 52 tests pass in normal and AddressSanitizer/UndefinedBehaviorSanitizer builds without compiler warnings.
The build commands live in [tech-stack.md](tech-stack.md#sanitizer-validation).

Lookup terms: `context-free grammar`, `CFG`, `derivation`, `parse tree`, `abstract syntax tree`, `AST`, `recursive-descent parsing`, `predictive parsing`, `LL(1)`, `lookahead`, `operator precedence`, `associativity`, `left recursion`, `syntax error recovery`, `panic mode`

- [x] Parse expressions with correct precedence
- [x] Parse statements and blocks
- [x] Build a typed AST in C++
- [x] AST printer
- [x] Parser tests (valid programs + expected syntax errors)

## Phase 3: Parsing theory (literacy, little or no production code)

Status: complete (study notes).

Notes: [theory-parsing.md](theory-parsing.md).

The notes calculate FIRST/FOLLOW sets and a predictive table for a small expression grammar.
They distinguish that grammar's LL(1) property from the complete statement grammar's explicit nearest-`if` rule.
A worked shift/reduce trace connects bottom-up parsing to the same expression grouping as the implemented parser.
The notes also compare LR(0), SLR, LALR, and canonical LR(1), with generator trade-offs and an interview summary.

The calculations include five set rows, 30 predictive cells, and 14 trace steps.
An independent calculation reproduces the sets, table decisions, optional-`else` conflict, and reverse rightmost derivation.
The documented AST example matches the existing command's complete output.
This phase adds documentation only, with no compiler behavior change or new dependency.

Lookup terms: `FIRST set`, `FOLLOW set`, `LL(1) grammar`, `left factoring`, `shift-reduce parsing`, `LR(0)`, `SLR`, `LALR`, `LR(1)`, `LR item`, `handle`, `shift/reduce conflict`, `reduce/reduce conflict`, `parser generator`

- [x] Work First/Follow examples by hand on a tiny grammar
- [x] Explain LL(1) conflicts vs what recursive descent handles
- [x] Summarize shift-reduce / LR family (LR(0), SLR, LALR, LR(1)) at conversation level
- [x] Note when you would reach for Bison/ANTLR vs a hand-written parser
- [x] Do not implement a bottom-up parser in this project (by design)

## Phase 4: Symbol table and scopes

Status: in progress.

The approved name rules live in [language.md](language.md#phase-4-name-rules).
Table ownership and the separate binding design live in [architecture.md](architecture.md#symbol-table-and-ownership).
Delivery uses three local increments:

1. Symbol table, parent scopes, stable identifiers, focused tests, and the approved design record.
2. AST traversal, declaration visibility, use bindings, category errors, and resolver tests.
3. Resolution output, command integration, examples, command tests, and sanitizer validation.

Increment 1 is complete: the table owns symbols and scopes while declarations remain in the AST.
Four new test groups cover declarations, lookup and isolation, redeclarations, and identifier stability.
The resolver API and command remain pending. Phase 4 is not complete.

Lookup terms: `symbol table`, `environment`, `lexical scope`, `static scope`, `nested scopes`, `shadowing`, `name resolution`, `declaration`, `binding`

- [x] Nested environments
- [ ] Errors for undeclared / redeclared names
- [ ] Tests for scope edge cases

## Phase 5: Semantic analysis / types

Lookup terms: `semantic analysis`, `type checking`, `type system`, `type environment`, `syntax-directed definition`, `attribute grammar`, `annotated AST`, `arity`, `return path`

- [ ] Check `int` / `bool` / `void` programs
- [ ] Clear compile-time diagnostics
- [ ] Tests for type errors

## Phase 6: Tree-walk interpreter (stage A)

Lookup terms: `interpreter`, `tree-walk interpreter`, `evaluation`, `operational semantics`, `environment`, `dynamic error`

- [ ] Evaluate AST directly
- [ ] Run `examples/` end to end
- [ ] Interpreter tests

## Phase 7: Functions and control flow

Lookup terms: `activation record`, `stack frame`, `calling convention`, `parameter passing`, `call stack`, `control flow`, `recursion`

- [ ] `if` / `while` / functions working in the interpreter
- [ ] Examples: factorial, fibonacci
- [ ] Regression tests for call/return and recursion

## Phase 8: Bytecode compiler

Lookup terms: `intermediate representation`, `IR`, `three-address code`, `bytecode`, `instruction set`, `constant pool`, `jump`, `branch`, `label`, `disassembler`, `code generation`

- [ ] Define a small instruction set
- [ ] Compile AST → bytecode chunk
- [ ] Disassembler for debugging
- [ ] Tests that compilation succeeds for fixture programs

## Phase 9: VM (stage B, primary target)

Lookup terms: `virtual machine`, `stack machine`, `fetch-decode-execute`, `instruction dispatch`, `frame pointer`, `runtime error`

- [ ] Execute the same examples as stages 6-7 on the VM
- [ ] Differential tests: interpreter vs VM results where useful

## Phase 10: Local optimizations

Lookup terms: `constant folding`, `constant propagation`, `dead code elimination`, `unreachable code`, `peephole optimization`, `local optimization`, `basic block`, `data-flow analysis`, `liveness analysis`

- [ ] Constant folding (AST and/or bytecode)
- [ ] Trivial dead-code / unreachable cleanup where easy
- [ ] Small peep-hole passes on bytecode
- [ ] Tests that optimized and unoptimized runs agree
- [ ] Optional stretch: one data-flow idea in notes only (e.g. liveness intuition)

## Phase 11: Optional language features (pick 1-2)

Lookup terms (by choice): `array`, `struct`, `record`, `garbage collection`, `mark and sweep`, `reference counting`

- [ ] Choose features after stage B + local opts feel solid
- [ ] Document chosen features in [language.md](language.md) before implementing
- [ ] Tests and examples for each feature

## Phase 12: LLVM literacy (conversation fluency)

Notes: [theory-llvm.md](theory-llvm.md).

Lookup terms: `LLVM IR`, `SSA`, `static single assignment`, `basic block`, `phi node`, `pass`, `pass manager`, `clang`, `opt`, `llc`, `code generation`, `target triple`

- [ ] Map our pipeline stages onto Clang/LLVM vocabulary (front end, IR, passes, codegen)
- [ ] Read a small C snippet's LLVM IR (`clang -S -emit-llvm`) and annotate it
- [ ] Optional thin experiment: emit a tiny subset to LLVM IR or walk an existing `.ll` file through `opt`/`llc`
- [ ] Write a short "how I would explain LLVM in an interview" section in [theory-llvm.md](theory-llvm.md)
- [ ] Explicit non-goal: production Clang/MLIR engineering depth

## Phase 13: Wrap-up

Lookup terms: `BNF`, `EBNF`, `compiler architecture`, `front end`, `back end`, `middle end`

- [ ] Final notes in [architecture.md](architecture.md): BNF, ISA, limits, how to run tests
- [ ] Update [curriculum-alignment.md](curriculum-alignment.md) with what was actually covered
- [ ] Portfolio polish: README accuracy, example gallery, clear "what I built / what I studied"
