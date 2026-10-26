---
id: architecture
title: Architecture
type: design
status: active
summary: Compiler pipeline stages and how they map to the roadmap.
related:
  - id: readme
    path: ../README.md
  - id: docs-index
    path: README.md
  - id: roadmap
    path: roadmap.md
  - id: tech-stack
    path: tech-stack.md
  - id: language
    path: language.md
  - id: theory-parsing
    path: theory-parsing.md
  - id: theory-llvm
    path: theory-llvm.md
---

# Architecture

## Pipeline

```text
source (.mc)
  → lexer (tokens)                         # phase 1
  → parser (AST)                           # phase 2
  → [theory: First/Follow, LR literacy]    # phase 3 (notes)
  → name resolution / scopes               # phase 4
  → type checking                          # phase 5
  → tree-walk interpreter                  # phases 6–7  [stage A]
  → bytecode compiler + VM                 # phases 8–9  [stage B — main target]
  → local optimizations                    # phase 10
  → [LLVM literacy / optional IR peek]     # phase 12     [not a career track]
```

## Stages

| Stage | What runs the program | Roadmap |
|-------|----------------------|---------|
| A | Tree-walk interpreter over the AST | phases 6–7 |
| B | Custom bytecode + VM | phases 8–9 (primary) |
| Opts | Local / peep-hole transforms | phase 10 |
| Literacy | LLVM vocabulary + light lab | phase 12 |

## Why staged

Front end (lexer → types) is validated early with stage A. Stage B teaches code generation and a simple runtime. Local opts prove you touched optimization without requiring a full data-flow framework. LLVM stays a literacy track so the portfolio can complete at a solid VM while still supporting informed conversations about industrial toolchains.

## Growing this doc

As phases land, add:

- BNF / grammar sketch
- AST node overview
- Bytecode ISA table
- Optimization pass list
- Known limits
