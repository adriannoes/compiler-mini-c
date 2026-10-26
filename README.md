---
id: readme
title: compiler-mini-c
type: overview
status: active
summary: Entry point — project pitch, build, and high-level todo.
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

Educational Mini-C compiler in C++. Built stage by stage, by hand, as a public learning record.

Goal: solid undergraduate-style compiler skills and enough theory (parsing + LLVM) to discuss the field confidently — not a path into specialist compiler engineering.

Planning is **closed**. Phase checklists and textbook lookup terms live in [docs/roadmap.md](docs/roadmap.md) and [docs/references.md](docs/references.md).

## Docs

| Doc | What it covers |
|-----|----------------|
| [docs/README.md](docs/README.md) | Index and front-matter conventions |
| [docs/roadmap.md](docs/roadmap.md) | Full learning plan, checklists, lookup terms |
| [docs/references.md](docs/references.md) | Suggested books and per-phase search terms |
| [docs/tech-stack.md](docs/tech-stack.md) | Tools, language, build |
| [docs/language.md](docs/language.md) | Mini-C scope and design choices |
| [docs/architecture.md](docs/architecture.md) | Compiler pipeline map |
| [docs/curriculum-alignment.md](docs/curriculum-alignment.md) | College syllabus vs market vs this plan |
| [docs/theory-parsing.md](docs/theory-parsing.md) | First/Follow and bottom-up parsing (literacy) |
| [docs/theory-llvm.md](docs/theory-llvm.md) | LLVM concepts for conversation fluency |

## Todo (summary)

Source of truth for details: [docs/roadmap.md](docs/roadmap.md).

| Phase | Topic | Status |
|------:|-------|--------|
| 0 | Project scaffold | done |
| 1 | Lexer (+ tests) | pending |
| 2 | Recursive-descent parser | pending |
| 3 | Parsing theory (First/Follow, bottom-up) | pending |
| 4 | Symbol table / scopes | pending |
| 5 | Type checking | pending |
| 6 | Tree-walk interpreter (stage A) | pending |
| 7 | Functions and control flow | pending |
| 8 | Bytecode compiler | pending |
| 9 | VM (stage B — primary target) | pending |
| 10 | Local optimizations | pending |
| 11 | Optional language features | pending |
| 12 | LLVM literacy | pending |
| 13 | Wrap-up / portfolio polish | pending |

**Next:** phase 1 (lexer).

## Build

```bash
cmake -S . -B build
cmake --build build
./build/minic --help
```

See [docs/tech-stack.md](docs/tech-stack.md) for requirements.
