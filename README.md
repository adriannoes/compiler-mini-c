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
The planned execution target is a custom bytecode virtual machine.
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
The program does not yet check syntax, check types, or execute Mini-C programs.
See [docs/language.md](docs/language.md) for the exact lexical rules.

## Build and use

From the repository root, configure and build with CMake 3.20 or later and a C++17 compiler:

```bash
cmake -S . -B build
cmake --build build
./build/minic --tokens examples/hello.mc
./build/minic --tokens examples/lexer.mc
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

## Tests

After the build, run the tests from the repository root:

```bash
ctest --test-dir build --output-on-failure
```

The 21 tests cover token types, text, positions, comments, lexical errors, and command behavior.
CTest runs the tests without an external test library.
The command tests compare output, diagnostics, and exit codes.

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
| 2 | Recursive-descent parser | pending |
| 3 | Parsing theory (First/Follow, bottom-up) | pending |
| 4 | Symbol table / scopes | pending |
| 5 | Type checking | pending |
| 6 | Tree-walk interpreter (stage A) | pending |
| 7 | Functions and control flow | pending |
| 8 | Bytecode compiler | pending |
| 9 | VM (stage B, primary target) | pending |
| 10 | Local optimizations | pending |
| 11 | Optional language features | pending |
| 12 | LLVM literacy | pending |
| 13 | Wrap-up / portfolio polish | pending |

The next implementation phase is the parser, which requires separate approval.
See [docs/tech-stack.md](docs/tech-stack.md) for build requirements and conventions.
