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
The `--tokens` command prints the tokens after the lexer succeeds.
The `--ast` command passes tokens to the parser and prints the complete tree after syntax parsing succeeds.

The `--resolve` command also resolves names and prints scopes, symbols, and bindings after the analysis succeeds.
The `--check` command also analyzes Mini-C types and returns success without printing output.

```text
source (.mc) -> minic::lex -> tokens
tokens -> --tokens output
tokens -> minic::parse -> Program
Program -> --ast output
Program -> minic::resolve_names -> NameResolution -> --resolve output
Program -> minic::check_types -> TypeAnalysis -> --check status
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

The `minic_frontend` CMake library contains tokens, the lexer, the parser, the symbol table, name resolution, and type analysis.
It also contains the AST and resolution printers.
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
The lexer and parser phases are complete within their agreed scopes.

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
Expression variants represent integers, booleans, names, unary operations, binary operations, assignments, and named calls.
Statement variants represent blocks, returns, declarations, expression statements, empty statements, conditionals, and loops.
Each function also owns its ordered parameters by value.
Optional return values, initializers, and else branches use a null pointer.
All required child pointers are non-null in parser results.
The complete syntax contract lives in the [approved grammar](language.md#phase-2-grammar).

An expression location identifies its literal, name, or operator token.
A block location identifies its opening brace, and a return, conditional, or loop location identifies its keyword.
An expression statement location identifies its first token, and an empty statement location identifies its semicolon.
Function, parameter, and declaration locations identify their names, and `TypeSyntax` retains each type keyword position.
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
Control-flow labels identify `Condition`, `Then`, `Else`, and `Body` children.
The `Else` label appears only when an else branch exists.
Each parser example has a fixture under `tests/` that specifies its complete expected AST output.

## Parser learning note

Recursive descent turns grammar rules into ordinary parsing methods.
A lower-precedence method calls a higher-precedence method for its operands.
For `1 + 2 * 3`, the addition therefore contains a multiplication as its right child.
Parentheses call the complete expression parser again, which permits a different grouping.

Within a block, a type keyword identifies a declaration before statement parsing starts.
The block stores both declarations and statements through the same C++ variant.
Only the block parser accepts declarations, which prevents a declaration from becoming a body without braces.
Parameters require a type and a name, and calls store argument expressions in source order.
The parser accepts calls without resolving the function or comparing its parameter count.

An `if` parser call consumes its body through the statement parser before it looks for an `else`.
An inner `if` therefore consumes its own optional `else` before the outer call continues.
Braces finish the inner block before the outer `if` searches for its `else`.
The parser needs no backtracking or separate ambiguity resolution pass.
The same statement parser accepts nested loops, blocks, returns, and empty bodies.

## Parser validation and limits

Phase 2 includes 18 parser test groups and 23 command tests, plus the original 11 lexer test groups.
The complete set contains 52 CTest tests.
Both the normal build and a separate build with AddressSanitizer and UndefinedBehaviorSanitizer pass all 52 tests.
Both builds use `-Wall -Wextra -Wpedantic` without warnings on Apple Clang 17.
The commands live in [tech-stack.md](tech-stack.md#sanitizer-validation).

The parser stops at the first syntax error and does not attempt recovery.
Syntax validation accepts undeclared names, invalid Mini-C types, and arbitrary integer text lengths.
The parser and printer use recursive calls, so deeply nested input depends on the available C++ call stack.
Execution, name resolution, numeric conversion, and Mini-C type analysis remain outside phase 2.

## Symbol table and ownership

`src/symbol_table.hpp` defines `SymbolTable`, which owns vectors of symbols and scopes.
A symbol records its name, category, source position, declaring scope, and borrowed AST declaration.
The declaration uses a variant of pointers to `FunctionDecl`, `Parameter`, or `VarDecl`.
The table owns copies of names and positions.
The AST retains ownership of declaration nodes.

`SymbolId` and `ScopeId` contain vector indices with separate C++ types.
The identifiers remain stable when the vectors grow or the table moves.
References to vector elements do not have that guarantee.
Callers must retain identifiers instead of element references across table changes.
The AST must remain alive, unmodified, and unmoved while callers use its borrowed declarations.

Scope zero is the global scope.
Each child scope records its category, source position, parent identifier, and a name-to-symbol hash map.
Lookup examines the current scope before each parent.
Insertion rejects a duplicate in that scope with `ResolveError`, which carries the second declaration position.
Parent lookup, shadowing, and sibling isolation use the same table operations.

The approved [name rules](language.md#phase-4-name-rules) define function and local visibility.
`resolve_names(const Program&)` connects those rules to AST traversal without changing the tree.
Its `NameResolution` result owns the table and separate use-to-symbol bindings.
Each binding identifies a borrowed expression, its symbol, its use scope, and its read, write, or call role.
This costs auxiliary storage but preserves syntax-only parsing and supplies declarations for later type analysis.

`NameResolution::binding(const Expr&)` returns a binding pointer or `nullptr` for an expression without a binding.
`table()` and `bindings()` expose read-only views of the owned data.
A hash map connects expression addresses to binding indices.
Result copies and moves preserve these indices, but the AST must retain its original nodes.
Deleted overloads reject temporary programs to prevent references to declarations that expire at the end of the call expression.

The resolver registers all function names before it visits bodies, so forward and recursive calls use the same lookup operation.
Parameters and outer body declarations share a scope. Only nested blocks create additional scopes.
Local insertion precedes initializer traversal, so `int x = x;` binds to the newly declared variable.
An undeclared use or category mismatch throws `ResolveError` before the caller receives a result.
The resolver does not inspect `TypeSyntax` or convert integer text.

`print_resolution(const NameResolution&)` returns a string for the `--resolve <file>` command.
Output uses vector order instead of hash-map iteration, so symbol identifiers and text remain deterministic.
The printer shows each use position and its selected declaration position.
The existing token and AST modes do not invoke the new analysis.

The command keeps the parsed `Program` alive until resolution output completes.
It completes each analysis stage before the next stage starts and prints no partial result after an error.
The resolver uses recursive traversal, so deeply nested input depends on the available C++ call stack.
The borrowed AST lifetime remains a caller responsibility in the C++ API.

## Name resolution learning note

Lexical scope determines a binding from source structure, without executing the program.
An outer lookup permits access to parameters and function names from nested blocks.
The resolver records the selected declaration at each use, so a later local declaration cannot change an earlier binding.
A completed scope map alone cannot preserve that distinction.
Separate bindings keep the earlier choice available to later compiler stages.

## Name resolution validation and limits

Phase 4 adds four symbol-table test groups, eleven resolver groups, and thirteen command tests.
The complete suite contains 80 CTest tests, including the original parser and token contracts.
All 80 pass in both normal and AddressSanitizer/UndefinedBehaviorSanitizer builds without compiler warnings on Apple Clang 17.
The commands live in [tech-stack.md](tech-stack.md#sanitizer-validation).

The command fixtures compare complete output, diagnostics, and exit codes.
The resolver tests inspect actual declarations and use bindings for shadowing, visibility, category rules, and scope isolation.
They preserve syntax-only parsing and permit type-invalid programs when their names resolve.
Phase 4 does not establish that a program can execute safely or pass later type analysis.

## Type rules and analysis design

`src/type_rules.hpp` defines `Type`, which represents `int`, `bool`, or `void` independently of `TypeSyntax`.
`TypeSyntax` continues to record source syntax without validating it.
The internal rule functions map type keywords and determine operator results without evaluating expressions.
They reject incompatible types with `TypeError`, which carries a source position and an English diagnostic.

The approved [type rules](language.md#phase-5-type-rules) define declarations, operators, calls, conditions, and returns.
Explicit operator rules prevent accidental C++ conversions between Mini-C integers and booleans.
Variables and parameters reject `void` at the written type keyword position.
Operator errors identify the operator position.

`check_types(const Program&)` returns `TypeAnalysis` without changing the AST.
The result owns its `NameResolution` and expression-type map.
Internal name resolution prevents callers from pairing the type analysis with bindings from a different program.
The result borrows AST nodes, so the caller must keep the tree alive, unchanged, and unmoved.
This adds auxiliary storage but leaves all earlier analysis modes independent.

`TypeAnalysis::type_of(const Expr&)` returns an optional type for an expression in the analyzed tree.
It returns `std::nullopt` for an expression outside that tree.
`resolution()` exposes a read-only view of the owned bindings and symbol table.
Result copies and moves preserve these associations, but the AST must retain its original nodes.
Deleted overloads reject temporary programs to prevent immediately expired references.

The checker first resolves every name, then prepares function signatures, then visits all bodies.
The complete signature pass permits forward calls without delaying argument or return-type analysis.
Each expression records its result type in a separate map after its children pass analysis.
Assignment targets also receive expression types.
The checker obtains variable and parameter types from the declaration selected by name resolution.

Statement traversal also computes whether a statement guarantees return.
Only returns, blocks with a guaranteed return, and conditionals with two returning branches establish that guarantee.
Loops never establish it because the checker does not evaluate conditions.
Traversal continues after a guaranteed return to detect type errors in later statements.
This limit avoids a control-flow graph but rejects some valid nonterminating functions.

`--check <file>` uses empty standard output and return code `0` on success.
Errors use the existing positioned diagnostic format and return code `1`.
The command keeps the `Program` alive during the complete analysis.
It does not execute the program or require a `main` function.
Recursive AST traversal still depends on the available C++ call stack.

## Type analysis learning note

The AST records the written type, but a name use needs the declaration selected by lexical scope.
Name resolution supplies that declaration, and type analysis uses it without another scope search.
Operator rules then combine child types into a parent type.
The separate result makes those computed types observable without adding semantic state to the syntax tree.

## Type analysis validation and limits

Phase 5 adds five type-rule test groups, sixteen API groups, and eighteen command tests.
The complete suite contains 119 CTest tests, including the original token, AST, and resolution contracts.
All 119 pass in both normal and AddressSanitizer/UndefinedBehaviorSanitizer builds without compiler warnings on Apple Clang 17.
The commands live in [tech-stack.md](tech-stack.md#sanitizer-validation).

The API tests inspect computed expression types and the declarations selected by the owned name-resolution result.
They also cover signature priority, conservative return guarantees, later statements after returns, AST preservation, and result copies and moves.
Command tests compare empty success output, exact diagnostics, and exit codes.
The type-error example also retains successful AST and name-resolution output.

Type analysis does not establish numeric representability, definite initialization, absence of division by zero, or termination.
It does not execute code, and its conservative return rules reject some valid programs with nonterminating loops.
Execution and runtime error behavior remain in later roadmap phases.

## Runtime values and interpreter design

`src/runtime.hpp` defines `Value` as a variant of `std::monostate`, `std::int64_t`, and `bool`.
The empty alternative represents `void`, and the integer and boolean alternatives remain distinct.
Values own their data and contain no AST pointers.
`RuntimeError` carries a source position and an English diagnostic.

The `minic_runtime` library links to `minic_frontend`, while the front-end library remains independent of execution.
Internal numeric helpers compare operand bounds before signed arithmetic.
Decimal conversion accumulates an unsigned magnitude within the permitted limit before converting to a signed value.
The negative-literal path permits the magnitude of the minimum signed integer without negating that integer in C++.
These guards prevent C++ overflow and invalid signed division without compiler extensions.

The approved [execution rules](language.md#phase-6-execution-rules) define the phase 6 subset and numeric behavior.
`interpret(const Program&, const std::string&)` performs internal type analysis before selecting its entry.
The interpreter borrows the AST during that call and returns an independent value.
Block environments use resolved symbol identifiers and an explicit state for variables without values.
This costs environment storage but preserves lexical bindings without changing AST nodes.

The interpreter owns its `TypeAnalysis` during execution and builds an index from variable declarations to symbol identifiers.
It uses the recorded bindings for reads and assignments without another search by source name.
Each block creates an environment, and a scope guard ends its lifetime on normal exit, return, or error.
Variable slots use `std::optional<Value>` so a missing value remains distinct from `void`.
Insertion precedes initializer evaluation, and assignment does not read the previous target value.

Statement execution returns `std::optional<Value>` as a control signal.
`std::nullopt` continues execution, while an engaged value propagates a return through nested blocks.
An engaged `std::monostate` ends a `void` entry, unlike fallthrough without a return.
Expressions evaluate their left operand before the right operand, and logical operators can skip the right operand.
Reached calls and control-flow nodes throw `RuntimeError` instead of starting phase 7 behavior.

The public call accepts temporary programs because its returned value contains no AST references.
Repeated calls create fresh analysis and environment state without changing the source tree.
No interpreter instance or environment escapes the call.

The final increment will add `--run <file>` and examples within the phase 6 subset.
Calls and control flow will remain phase 7 work, with explicit errors for reached unsupported nodes.
The existing analysis modes will not invoke the runtime library.

## Interpreter learning note

A lexical binding identifies a declaration, while an environment stores the value for its active execution.
Separate symbol identifiers preserve shadowing even when a declaration appears after an earlier use of an outer variable.
Explicit missing-value state reports uninitialized reads without changing static name or type analysis.
Return propagation and block cleanup define execution order without requiring function-call frames in phase 6.

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
