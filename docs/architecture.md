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
                                -> minic::parse -> Program -> --ast output
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

The `minic_frontend` CMake library contains tokens, the lexer, the parser, and the AST printer.
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
The lexer phase is complete. The approved parser implementation is in progress.

## Parser and AST

`minic::parse(const std::vector<Token>&)` consumes the unmodified result of `minic::lex` and returns a `Program` by value.
The parser borrows tokens only during that call.
Each AST name and integer literal owns its text, so the tree survives destruction of tokens and source text.
Integers retain their original decimal digits without conversion or numeric limits.

`src/ast.hpp` defines concrete C++ structures grouped in `std::variant`.
A variant stores one of its listed node types.
`std::unique_ptr` gives each child one owner, and containers preserve source order.
This design costs allocations and string copies but does not require shared ownership or external lifetime management.
Adding a node also requires a printer overload, which keeps tree output consistent with the C++ node definitions.

`Program` owns function definitions by value, and each function owns its body block by value.
Expression variants represent integers, booleans, names, unary operations, binary operations, and assignments.
Statement variants currently represent blocks and returns.
The next increments add the remaining nodes from the [approved grammar](language.md#phase-2-grammar).
Optional return values use a null pointer.

An expression location identifies its literal, name, or operator token.
A statement location identifies its keyword or opening brace.
A function location identifies its name, and `TypeSyntax` retains the type keyword position.
Parentheses affect tree structure without a separate node.
All positions preserve the lexer's line and byte-column rules.

The C++ types specify node shapes, not Mini-C expression types.
`TypeSyntax` records a type keyword without semantic validation.
The parser accepts undeclared names, mixed-type expressions, and returns without matching type declarations.
Name resolution and Mini-C type analysis remain in phases 4 and 5.

`src/parser.cpp` assigns one parsing method to each precedence level.
Loops construct binary operations with left associativity.
Recursive calls construct unary operations and assignments with right associativity.
Assignments require a name node on the left, which is a syntactic restriction.
The grammar and exact operator rules live in [language.md](language.md#phase-2-grammar).

`ParseError` carries the unexpected token position and an English diagnostic.
The parser stops at the first error and destroys the unfinished tree through exclusive ownership.
It performs no error recovery.
The command completes lexing first, so lexical errors take priority over syntax errors.
The existing token command never invokes the parser.

`minic::print_ast` returns deterministic text after it visits the complete tree.
The printer uses two spaces per level and appends `@line:column` to each positioned node.
Binary children appear in left-to-right order, and assignment children appear as target then value.
The command prints no partial tree if lexing or parsing fails.
Fixtures in `tests/hello.ast` and `tests/expressions.ast` specify the complete expected output.

## Parser learning note

Recursive descent turns grammar rules into ordinary parsing methods.
A lower-precedence method calls a higher-precedence method for its operands.
For `1 + 2 * 3`, the addition therefore contains a multiplication as its right child.
Parentheses call the complete expression parser again, which permits a different grouping.

The initial parser accepts only functions without parameters, blocks, returns, and expressions without calls.
The complete approved grammar also includes declarations, expression statements, parameters, calls, and control flow.
Phase 2 remains in progress until those increments and their tests are complete.

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
