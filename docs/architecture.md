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

## Current implementation

The command reads a source file and passes its text to `minic::lex`.
The lexer separates the text into tokens, which are units such as names and numbers.
The command prints the tokens after the lexer succeeds.

```text
source (.mc) -> minic::lex -> tokens -> --tokens output
```

`src/token.hpp` defines token types and source positions.
Each token owns its source text, so the result remains valid after the caller destroys the input buffer.
This copies small strings but avoids a shared lifetime between tokens and the input buffer.

`src/lexer.cpp` scans the source from left to right.
It reads a complete identifier before keyword classification and consumes all adjacent digits for an integer.
Comments and whitespace update the source position without producing tokens.
These rules teach token boundaries, keyword recognition, and position tracking.

The scanner reads at most one extra byte to distinguish operators with a common prefix.
It chooses `==` before `=`, for example.
The scanner handles `//` and `/*` comments before it emits `/` as division.
This keeps the implementation in the existing scanner and adds no dependencies.

The lexer throws `LexError` at the first unsupported character or unterminated block comment.
The command adds the file path and prints the diagnostic to standard error.
The command emits no partial token list on failure.

The `minic_frontend` CMake library contains the lexer and token definitions.
Both `minic` and the unit tests link to that library.
CTest also runs command tests against the built executable.
See [tech-stack.md](tech-stack.md) for build and test commands and [language.md](language.md) for lexical rules.

## Lexer learning note

Maximal munch means choosing the longest supported token at each position.
For `===`, the scanner emits `EQUAL_EQUAL` and then `ASSIGN`.
For `=/**/=`, the comment separates two `ASSIGN` tokens.
Token recognition establishes boundaries, but later stages determine syntax, operator precedence, and types.

The [lexer example](../examples/lexer.mc) contains every token type.
The command test compares its entire output with [tests/lexer.tokens](../tests/lexer.tokens).
Unit tests also cover operators at the end of input, shared prefixes, division, and comments without nesting.
The lexer phase is complete. Parser implementation requires separate approval.

## Planned pipeline

```text
source (.mc)
  → lexer (tokens)                         # phase 1
  → parser (AST)                           # phase 2
  → [theory: First/Follow, LR literacy]    # phase 3 (notes)
  → name resolution / scopes               # phase 4
  → type checking                          # phase 5
  → tree-walk interpreter                  # phases 6-7  [stage A]
  → bytecode compiler + VM                 # phases 8-9  [stage B, main target]
  → local optimizations                    # phase 10
  → [LLVM literacy / optional IR peek]     # phase 12     [not a career track]
```

## Stages

| Stage | What runs the program | Roadmap |
|-------|----------------------|---------|
| A | Tree-walk interpreter over the AST | phases 6-7 |
| B | Custom bytecode + VM | phases 8-9 (primary) |
| Opts | Local / peep-hole transforms | phase 10 |
| Literacy | LLVM vocabulary + light lab | phase 12 |

## Why staged

Stage A executes programs to test the lexer, parser, and type rules together.
Stage B teaches code generation and a simple runtime.
Local optimizations introduce program transformations without a full data-flow framework.
LLVM study provides vocabulary and small experiments for technical discussions.
The main deliverable remains the bytecode virtual machine.

## Growing this doc

As phases land, add:

- BNF / grammar sketch
- AST node overview
- Bytecode ISA table
- Optimization pass list
- Known limits
