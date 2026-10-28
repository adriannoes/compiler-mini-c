---
id: references
title: Books and lookup terms
type: reference
status: active
summary: Suggested books and per-phase concept names to search in indexes and TOCs.
related:
  - id: readme
    path: ../README.md
  - id: docs-index
    path: README.md
  - id: roadmap
    path: roadmap.md
  - id: curriculum-alignment
    path: curriculum-alignment.md
  - id: theory-parsing
    path: theory-parsing.md
  - id: theory-llvm
    path: theory-llvm.md
---

# Books and lookup terms

The project roadmap is complete.
To study a phase, select its terms from [roadmap.md](roadmap.md).
Search for those terms in a book index or table of contents.

## Suggested books

| Role | Book | Why |
|------|------|-----|
| Primary theory | Aho, Lam, Sethi, Ullman: *Compilers: Principles, Techniques, and Tools* ("Dragon Book", 2nd ed.) | Undergraduate topics: lexing, LL/LR parsing, semantics, intermediate representation (IR), runtime, and introductory optimization |
| Primary practice (VM / bytecode) | Nystrom: *Crafting Interpreters* | Covers tree-walk interpreters, bytecode, virtual machines (VMs), and frames. The examples use languages other than C++, but the concepts transfer |
| Alternate project view | Appel: *Modern Compiler Implementation* ("Tiger Book", C or ML/Java editions) | Covers a complete compiler project, including IR, frames, instruction selection, and register allocation |
| Heavier engineering | Cooper & Torczon: *Engineering a Compiler* | Covers IR and optimization beyond local peephole transformations |
| LLVM literacy (optional) | LLVM project documentation, guides such as *Getting Started with LLVM*, and the official LLVM Language Reference | For phase 12, use the official reference and `clang -emit-llvm` before a full textbook |

Use the Dragon Book for terminology and theory.
Use *Crafting Interpreters* for VM implementation concepts.
Use the LLVM Language Reference for phase 12.

You do not need to read every chapter.
Use the lookup terms for each phase.

## Quick map: phase → book emphasis

| Phase | Reach for first | Also useful |
|------:|-----------------|-------------|
| 1 | Dragon: lexical analysis | Crafting Interpreters: scanning |
| 2 | Dragon: top-down parsing, syntax-directed translation | Crafting Interpreters: parsing expressions |
| 3 | Dragon: First/Follow, bottom-up / LR | None |
| 4 through 5 | Dragon: symbol tables, type checking | Tiger: semantic analysis |
| 6 through 7 | Crafting Interpreters: evaluating expressions, functions | Dragon: runtime environments |
| 8 through 9 | Crafting Interpreters: bytecode / VM | Dragon: intermediate code |
| 10 | Dragon: peephole optimization and introductory dataflow analysis | Engineering a Compiler: local optimization |
| 11 | Dragon / Tiger chapters matching the feature | Crafting Interpreters: GC chapter if chosen |
| 12 | LLVM Language Reference | Harvard IR examples and introductory static single assignment (SSA) material in Dragon |

## Per-phase lookup terms

Search for these terms in a book index or on the web.
The complete checklist is in [roadmap.md](roadmap.md).

### Phase 1: Lexer

`lexical analysis`, `scanner`, `token`, `lexeme`, `regular expression`, `finite automaton` / `DFA` / `NFA`, `maximal munch`, `keyword`, `identifier`, `source location`

### Phase 2: Recursive-descent parser

`context-free grammar`, `CFG`, `derivation`, `parse tree`, `abstract syntax tree` / `AST`, `recursive-descent parsing`, `predictive parsing`, `LL(1)`, `lookahead`, `operator precedence`, `associativity`, `left recursion`, `syntax error recovery`, `panic mode`

### Phase 3: Parsing theory

`FIRST set`, `FOLLOW set`, `LL(1) grammar`, `left factoring`, `shift-reduce parsing`, `LR(0)`, `SLR`, `LALR`, `LR(1)`, `item` (LR item), `handle`, `shift/reduce conflict`, `reduce/reduce conflict`, `parser generator`

### Phase 4: Symbol table / scopes

`symbol table`, `environment`, `lexical scope`, `static scope`, `nested scopes`, `shadowing`, `name resolution`, `declaration`, `binding`

### Phase 5: Types / semantics

`semantic analysis`, `type checking`, `type system`, `type environment`, `syntax-directed definition`, `attribute grammar` (optional depth), `annotated AST`, `arity`, `return path`

### Phase 6: Tree-walk interpreter

`interpreter`, `tree-walk interpreter`, `evaluation`, `operational semantics` (light), `environment` (runtime), `dynamic error`

### Phase 7: Functions / control flow

`activation record`, `stack frame`, `calling convention` (conceptual), `parameter passing`, `call stack`, `control flow`, `recursion`

### Phase 8: Bytecode compiler

`intermediate representation` / `IR`, `three-address code`, `bytecode`, `instruction set`, `constant pool`, `jump` / `branch`, `label`, `disassembler`, `codegen`

### Phase 9: VM

`virtual machine`, `stack machine`, `fetch-decode-execute`, `instruction dispatch`, `frame pointer`, `runtime error`

### Phase 10: Local optimizations

`constant folding`, `constant propagation` (local), `dead code elimination`, `unreachable code`, `peephole optimization`, `local optimization`, `basic block`, `data-flow analysis` (optional intro), `liveness analysis` (optional intro)

### Phase 11: Optional features

The selected feature uses `arrays`.
Other study terms depend on the feature: `structs` / `records`, `garbage collection`, `mark and sweep`, and `reference counting`.

### Phase 12: LLVM literacy

`LLVM IR`, `SSA` / `static single assignment`, `basic block`, `phi node` (recognition only), `pass`, `pass manager`, `clang`, `opt`, `llc`, `codegen`, `target triple`

### Phase 13: Wrap-up

`BNF`, `EBNF`, `compiler architecture`, `front end`, `back end`, `middle end`
