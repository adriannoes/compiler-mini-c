---
id: language
title: Mini-C language
type: design
status: active
summary: Source-language scope, out-of-scope list, and product decisions.
related:
  - id: readme
    path: ../README.md
  - id: docs-index
    path: README.md
  - id: roadmap
    path: roadmap.md
  - id: tech-stack
    path: tech-stack.md
  - id: architecture
    path: architecture.md
  - id: curriculum-alignment
    path: curriculum-alignment.md
---

# Mini-C language

Mini-C is a C subset chosen to stay close to a typical college compilers course.

## Learning intent

| Intent | Detail |
|--------|--------|
| Portfolio | Public GitHub record of studying compilers end to end |
| Career lens | Stronger general engineering (parsers, ASTs, VMs, DSLs) — not a specialist compiler-engineer track |
| Theory must-haves | Bottom-up / First/Follow literacy; LLVM literacy for technical conversations |
| Primary artifact | Hand-written Mini-C → bytecode VM |

## Product decisions

| Choice | Value |
|--------|--------|
| Source language | Mini-C (C subset) |
| Parser style | Hand-written recursive descent (LR studied in notes only) |
| Execution path | Tree-walk → bytecode VM → local opts → LLVM literacy |
| Primary target | Bytecode VM ([roadmap.md](roadmap.md) stage B) |
| Docs language | English (portfolio-ready) |

## Initial scope

Unlocked phase by phase (see [roadmap.md](roadmap.md)):

- Types: `int`, `bool`, `void` (extend later if needed)
- Statements: declarations, assignment, `if`/`else`, `while`, `return`, blocks
- Expressions: arithmetic, comparisons, logical ops, calls
- Functions with parameters

## Out of scope (start)

Preprocessor, pointers, structs, `goto`, unions.

Revisit only after stage B is solid, and record any change here before coding (phase 11).

## Mindset

Prefer hand-written stages over generators or heavy frameworks. Each implementation phase should leave runnable examples, automated tests, and a short note about the concept it teaches.
