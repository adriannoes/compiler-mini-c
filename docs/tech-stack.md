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
| Tests | Grow from phase 1 | Golden/unit tests under `tests/`; required each phase |
| VCS | Git (local) | Public GitHub later for portfolio |
| LLVM tools | Optional (phase 12) | `clang -emit-llvm`, `opt`, `llc` for literacy labs only |

## Tooling on this machine

| Tool | Role |
|------|------|
| Apple Clang (`c++` / `clang`) | Compile C++; later emit sample LLVM IR |
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

## Build

```bash
cmake -S . -B build
cmake --build build
./build/minic --help
```

## Conventions

- English only: docs, comments, commit messages, user-facing diagnostics
- No parser generators in production code (flex/bison/ANTLR); hand-written stages
- Bottom-up / LLVM depth lives in theory docs for literacy, not as the career target
- Prefer small, testable increments aligned with [roadmap.md](roadmap.md)
