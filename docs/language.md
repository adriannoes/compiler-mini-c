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
| Career lens | General software engineering through parsers, syntax trees, and interpreters |
| Theory must-haves | Bottom-up parsing, First/Follow, and LLVM concepts for technical conversations |
| Primary artifact | Hand-written Mini-C → bytecode VM |

## Product decisions

| Choice | Value |
|--------|--------|
| Source language | Mini-C (C subset) |
| Parser style | Hand-written recursive descent (LR studied in notes only) |
| Execution path | Tree-walk → bytecode VM → local opts → LLVM literacy |
| Primary target | Bytecode VM ([roadmap.md](roadmap.md) stage B) |
| Docs language | English for the public portfolio |

Specialist compiler engineering is outside the project scope.

## Implemented lexical rules

A token is a source unit with a type, text, and position.
The lexer recognizes the following tokens:

| Token type | Source text |
|------------|-------------|
| `INT` | `int` |
| `RETURN` | `return` |
| `IDENTIFIER` | `[A-Za-z_][A-Za-z0-9_]*` |
| `INTEGER` | `[0-9]+` |
| `LEFT_PAREN`, `RIGHT_PAREN` | `(`, `)` |
| `LEFT_BRACE`, `RIGHT_BRACE` | `{`, `}` |
| `SEMICOLON` | `;` |
| `EOF` | Empty text at the end of the input |

Identifiers use ASCII letters, digits, and underscores. An identifier cannot start with a digit.
Names are case sensitive. The lexer reads the complete identifier before it compares the text with `int` and `return`.
For example, `integer`, `int2`, and `Return` are identifiers.

Integer tokens preserve all decimal digits as text.
Leading zeroes do not select another base.
The lexer does not convert numbers or enforce numeric limits.
Numeric limits belong to a later stage.

The lexer skips spaces, tabs, vertical tabs, form feeds, and line endings.
A `//` comment ends at a line ending or the end of the input.
A `/* ... */` comment ends at the first `*/`. Block comments do not nest.
Comments separate tokens, so `int/* note */return` produces two keyword tokens.

Line and column numbers start at 1.
Columns count bytes, and a tab advances one column.
LF, CRLF, and CR each advance one line and reset the column to 1.
The `EOF` position follows the last input byte, including skipped whitespace and comments.

The lexer stops at the first error.
An unsupported character reports its position.
A block comment without `*/` reports the position of its opening `/*`.
Non-printable bytes appear in diagnostics as hexadecimal values.

## Token command

After the build, run `./build/minic --tokens examples/hello.mc` from the repository root.
Each output line uses `line:column TYPE "text"`.
The command emits the complete token list only if lexing succeeds.

The command returns `0` on success.
It returns `1` for lexical errors, file errors, or invalid arguments.
Diagnostics go to standard error, and lexical errors include the file path, line, and column.

## Planned language scope

The following language scope grows phase by phase (see [roadmap.md](roadmap.md)):

- Types: `int`, `bool`, `void` (extend later if needed)
- Statements: declarations, assignment, `if`/`else`, `while`, `return`, blocks
- Expressions: arithmetic, comparisons, logical ops, calls
- Functions with parameters

Operators and keywords other than `int` and `return` remain pending in phase 1.
Until that work is complete, names such as `bool` and `if` are identifiers.
The lexer recognizes token boundaries but does not check statement structure or types.

## Out of scope (start)

Preprocessor, pointers, structs, `goto`, unions.

After stage B works, reconsider these limits in phase 11.
Record each scope change here before implementation.

## Mindset

Implement each stage by hand.
Each implementation phase must include runnable examples, automated tests, and a short note about the concept it teaches.
