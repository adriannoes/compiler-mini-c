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

## Phase 1 scope

The phase 1 agreement adds exactly 23 token types to the first lexer increment.
The following table records the complete addition:

| Category | Source text | Token types |
|----------|-------------|-------------|
| Keywords | `bool`, `void`, `if`, `else`, `while`, `true`, `false` | `BOOL`, `VOID`, `IF`, `ELSE`, `WHILE`, `TRUE`, `FALSE` |
| Assignment | `=` | `ASSIGN` |
| Arithmetic | `+`, `-`, `*`, `/`, `%` | `PLUS`, `MINUS`, `STAR`, `SLASH`, `PERCENT` |
| Comparisons | `==`, `!=`, `<`, `<=`, `>`, `>=` | `EQUAL_EQUAL`, `BANG_EQUAL`, `LESS`, `LESS_EQUAL`, `GREATER`, `GREATER_EQUAL` |
| Logic | `!`, `&&`, `\|\|` | `BANG`, `AND_AND`, `OR_OR` |
| Separator | `,` | `COMMA` |

The lexer must choose the longest supported operator at each position.
It must distinguish division from line and block comments.
The source `-42` must produce `MINUS` and `INTEGER` tokens.
The source `++` must produce two `PLUS` tokens because Mini-C has no increment operator.
The characters `&` and `|` alone must remain lexical errors.

This phase preserves token text ownership, source positions, comment rules, diagnostics, and command exit codes.
It excludes strings, floating-point numbers, arrays, pointers, preprocessing, bitwise operators, compound assignment, and increment or decrement operators.
Phase 1 excludes syntax and type analysis.
The approved phase 2 scope appears below.
Delivery progress lives in [roadmap.md](roadmap.md).

## Implemented lexical rules

A token is a source unit with a type, text, and position.
The lexer recognizes the following tokens:

| Token type | Source text |
|------------|-------------|
| `INT` | `int` |
| `BOOL` | `bool` |
| `VOID` | `void` |
| `IF` | `if` |
| `ELSE` | `else` |
| `WHILE` | `while` |
| `RETURN` | `return` |
| `TRUE`, `FALSE` | `true`, `false` |
| `IDENTIFIER` | `[A-Za-z_][A-Za-z0-9_]*` |
| `INTEGER` | `[0-9]+` |
| `LEFT_PAREN`, `RIGHT_PAREN` | `(`, `)` |
| `LEFT_BRACE`, `RIGHT_BRACE` | `{`, `}` |
| `SEMICOLON` | `;` |
| `COMMA` | `,` |
| `ASSIGN` | `=` |
| `PLUS`, `MINUS`, `STAR`, `SLASH`, `PERCENT` | `+`, `-`, `*`, `/`, `%` |
| `EQUAL_EQUAL`, `BANG_EQUAL` | `==`, `!=` |
| `LESS`, `LESS_EQUAL`, `GREATER`, `GREATER_EQUAL` | `<`, `<=`, `>`, `>=` |
| `BANG`, `AND_AND`, `OR_OR` | `!`, `&&`, `\|\|` |
| `EOF` | Empty text at the end of the input |

Identifiers use ASCII letters, digits, and underscores. An identifier cannot start with a digit.
Names are case sensitive. The lexer reads the complete identifier before keyword classification.
For example, `integer`, `int2`, `boolean`, `if2`, `Return`, and `True` are identifiers.

Integer tokens preserve all decimal digits as text.
Leading zeroes do not select another base.
The lexer does not convert numbers or enforce numeric limits.
Numeric limits belong to a later stage.

Operators use the longest supported match at the current position.
For example, `===` produces `EQUAL_EQUAL` and `ASSIGN`.
Whitespace and comments separate operators, so `=/**/=` produces two `ASSIGN` tokens.
The source `-42` produces `MINUS` followed by `INTEGER` with text `42`.
The parser determines whether `-` means negation or subtraction.

Mini-C has no increment, decrement, or compound assignment operators.
The lexer reads `++`, `--`, `+=`, and `/=` as pairs of supported tokens.
Token recognition does not establish that a sequence is valid syntax.
The characters `&` and `|` alone are lexical errors.

The lexer skips spaces, tabs, vertical tabs, form feeds, and line endings.
A `//` comment ends at a line ending or the end of the input.
A `/* ... */` comment ends at the first `*/`. Block comments do not nest.
Comments separate tokens, so `int/* note */return` produces two keyword tokens.
A `/` that does not start a comment produces a `SLASH` token.

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

## Phase 2 grammar

The parser builds an abstract syntax tree (AST), which records program structure.
The following grammar defines the approved phase 2 scope.
Braces mean repetition, and brackets mean an optional part.
Quoted text names a token spelling.

```ebnf
program          = { function }, EOF ;
function         = type, IDENTIFIER, "(", [ parameters ], ")", block ;
type             = "int" | "bool" | "void" ;
parameters       = parameter, { ",", parameter } ;
parameter        = type, IDENTIFIER ;
block            = "{", { declaration | statement }, "}" ;
declaration      = type, IDENTIFIER, [ "=", expression ], ";" ;
statement        = block | if_statement | while_statement | return_statement
                 | [ expression ], ";" ;
if_statement     = "if", "(", expression, ")", statement, [ "else", statement ] ;
while_statement  = "while", "(", expression, ")", statement ;
return_statement = "return", [ expression ], ";" ;
expression       = assignment ;
assignment       = logical_or, [ "=", assignment ] ;
logical_or       = logical_and, { "||", logical_and } ;
logical_and      = equality, { "&&", equality } ;
equality         = comparison, { ( "==" | "!=" ), comparison } ;
comparison       = additive, { ( "<" | "<=" | ">" | ">=" ), additive } ;
additive         = multiplicative, { ( "+" | "-" ), multiplicative } ;
multiplicative   = unary, { ( "*" | "/" | "%" ), unary } ;
unary            = ( "!" | "-" ), unary | primary ;
primary          = INTEGER | "true" | "false"
                 | IDENTIFIER, [ "(", [ arguments ], ")" ]
                 | "(", expression, ")" ;
arguments        = expression, { ",", expression } ;
```

A program contains only function definitions at file scope.
An empty program is valid syntax, and the parser does not require a `main` function.
Function prototypes, nested functions, and global variables are outside phase 2.
Each parameter requires a type and a name. The empty parameter list uses `()`, not `(void)`.
Parameter lists and argument lists do not accept a trailing comma.
Calls use a function name. The grammar excludes chained calls and calls through other expressions.

A declaration introduces one name, with an optional initializer.
Declarations occur only within blocks, with no required ordering relative to statements.
An `if` or `while` body accepts one statement, with optional braces.
A declaration in either body requires a block.
A return accepts either `return;` or `return expression;`.

An `else` belongs to the nearest preceding `if` that has no `else`.
The parser consumes the complete body of an `if` before it looks for that `if`'s optional `else`.
An inner `if` therefore consumes its own `else` first.
An `else if` is an `else` whose statement is another `if`.

### Precedence and associativity

The table lists precedence from lowest to highest.
Associativity determines grouping within one level.

| Operators | Associativity |
|-----------|---------------|
| `=` | Right |
| `\|\|` | Left |
| `&&` | Left |
| `==`, `!=` | Left |
| `<`, `<=`, `>`, `>=` | Left |
| Binary `+`, `-` | Left |
| `*`, `/`, `%` | Left |
| Unary `!`, `-` | Right |
| Named calls and primary expressions | Primary syntax |

Parentheses override precedence and do not produce a separate AST node.
An assignment requires a name as its left operand, including a parenthesized name.
For example, `a = b = 1` groups as `a = (b = 1)`.
The parser rejects `a + b = 1` at the assignment token.
This rule concerns syntax, without name resolution or type analysis.

The `+` operator is binary only.
The token sequence `++` cannot form an expression in this grammar.
Two unary minus tokens can form nested negation, as in `--x`, without a decrement operation.
Compound assignment remains outside the language.

### Syntax and Mini-C types

The AST records `int`, `bool`, and `void` as type syntax.
The parser accepts each type keyword in return types, parameters, and declarations.
Later semantic analysis will determine whether a use of `void` is valid.
The parser does not resolve names, compare types, count call parameters, or require matching return values.
For example, undeclared names and `true + 1` remain valid syntax.
Integer literals preserve their decimal text, including leading zeroes, without conversion or numeric limits.

### AST command and errors

From the repository root, operate `./build/minic --ast examples/hello.mc` after the build.
The command completes lexing before parsing.
A lexical error therefore takes priority over a syntax error elsewhere in the file.
The parser stops at its first syntax error, without recovery or a partial AST result.
It reports the unexpected token position, including the lexer's exact `EOF` position.

On success, the command prints the complete indented AST and returns `0`.
On failure, it prints no AST, writes a diagnostic to standard error, and returns `1`.
Diagnostics use `file:line:column: error: message`.
The existing `--tokens` mode performs no parsing and preserves its output, diagnostics, and exit codes.

## Phase 4 name rules

Name resolution connects each name use to its declaration.
These rules define the approved phase 4 scope without changing the grammar.
The front-end library provides the symbol table and `resolve_names` analysis.
The `--resolve` command prints the result after the complete analysis succeeds.

### Scopes and declarations

A scope contains declarations and links to an enclosing scope.
The global scope contains function names only.
Each function has a separate child scope for its parameters and outer body.
Parameters and declarations in that body share the same scope.
Each nested block creates a child scope. An `if` or `while` creates no scope without a block.

Lookup searches the current scope before its enclosing scopes.
Shadowing means that an inner declaration hides an outer declaration with the same name.
Mini-C permits shadowing, including a local name that hides a function.
Declarations in separate functions or sibling blocks remain isolated.
Names remain case sensitive.

A second declaration in the same scope is an error, regardless of its written type.
This rule rejects duplicate functions, duplicate parameters, and a local declaration that repeats a parameter in the outer body.
A nested block can declare a name that hides that parameter.
Functions, parameters, and variables share one namespace, which is one set of names per scope.
There are no built-in functions or function overloads.

### Visibility and uses

The resolver first registers every function name in source order.
Function names are visible throughout the program, including before their definitions.
This permits recursion and mutual recursion without function prototypes.
The resolver then visits each function body in source order.

A local declaration becomes visible before the resolver visits its initializer.
In `int x = x;`, the initializer refers to the new variable, including when another `x` exists outside the block.
Name resolution accepts this reference without determining whether the variable holds an initialized value.
Before a local declaration, lookup can still find an outer declaration.
Without an outer declaration, that earlier use is an undeclared-name error.

A read or assignment target must resolve to a variable or parameter.
A named call must resolve to a function.
Lookup does not skip a nearer declaration of a different category.
For example, a variable that hides a function prevents calls to that function through the hidden name.
The resolver visits arguments and all other expressions without evaluation.

### Errors and limits

Name resolution stops at the first error and returns no partial result.
The resolver detects duplicate functions before body analysis.
Other errors follow the body traversal order, with assignment targets before values and callees before arguments.
Diagnostics identify the undeclared use, invalid category use, or second declaration.

The parser and `--ast` remain syntax-only operations.
Phase 4 does not compare Mini-C types, count arguments, require matching returns, or impose numeric limits.
It does not analyze definite initialization, require `main`, or execute the program.
Type syntax, including `void` parameters and variables, remains unvalidated.

### Resolution command

After the build, operate `./build/minic --resolve examples/scopes.mc` from the repository root.
The command completes lexing, then parsing, then name resolution.
Errors in an earlier stage take priority over errors in a later stage anywhere in the file.
The command returns `0` with the complete resolution output on standard output.
It returns `1` with no standard output for a lexical, syntax, resolution, file, or argument error.

Resolution diagnostics use `file:line:column: error: message` on standard error.
An undeclared use reports `Undeclared name "x".`.
A second declaration reports `Name "x" is already declared in this scope.`.
A category mismatch reports that the name does not refer to a function, or to a variable or parameter.
The messages identify source names without interpreting their types.

Output starts with `Resolution`, then lists scopes, symbols, and bindings.
Identifiers start at zero and apply only to that analysis result.
All functions receive symbol identifiers first, in source order.
Parameters and local declarations then receive identifiers during body traversal.
Bindings appear in traversal order, with `Read`, `Write`, or `Call` as the use category.

A scope line shows its identifier, category, optional parent identifier, and source position.
The global position is `@1:1`. A function scope uses its function name position, and a block scope uses its opening brace.
A symbol line shows its identifier, category, quoted name, declaring scope, and declaration position.
A binding line shows the use category, quoted name, use scope, use position, selected symbol, and declaration position.
The complete examples live in [scopes.resolution](../tests/scopes.resolution) and [recursion.resolution](../tests/recursion.resolution).

## Phase 5 type rules

Type checking determines whether expressions and declarations use compatible Mini-C types.
These rules define the approved phase 5 scope without changing the grammar or name rules.
The front-end library provides the basic type rules.
AST type analysis and the `--check` command remain pending.

### Types and expressions

Mini-C types are `int`, `bool`, and `void`.
Variables and parameters must use `int` or `bool`. Function return types can also use `void`.
Mini-C does not convert `int` to `bool` or `bool` to `int`.
Integer literals have type `int` without numeric conversion or a limit on their text length.
Boolean literals have type `bool`.

| Expression | Required operand types | Result type |
|------------|------------------------|-------------|
| Unary `-` | `int` | `int` |
| `+`, `-`, `*`, `/`, `%` | Both `int` | `int` |
| `<`, `<=`, `>`, `>=` | Both `int` | `bool` |
| Unary `!` | `bool` | `bool` |
| `&&`, `\|\|` | Both `bool` | `bool` |
| `==`, `!=` | Matching `int` or matching `bool` | `bool` |
| Assignment | Value type matches the target type | Target type |
| Named call | Arguments match the function parameters | Function return type |

A name expression has the type written in its resolved variable or parameter declaration.
An initializer must have the same type as its variable.
Assignment does not convert the value, and an assignment expression retains the target type.
A standalone expression statement can discard an `int`, `bool`, or `void` result.
A `void` call cannot supply an operand, argument, initializer, condition, or returned value.

### Functions and conditions

A call must supply exactly one argument for each parameter, in parameter order.
Each argument type must match the corresponding parameter type.
The checker examines all function signatures before it visits bodies.
Forward calls, recursion, and mutual recursion retain the existing name-resolution behavior.

An `if` or `while` condition must have type `bool`.
A `void` function accepts `return;` and can end without an explicit return.
A `void` function cannot use `return expression;`, including an expression with type `void`.
An `int` or `bool` function must return a value with its declared return type.
Such a function must also guarantee a return value on every path under the rules that follow.

A return statement guarantees return.
A block guarantees return when a statement in its sequence guarantees return.
An `if` guarantees return only when it has an `else` and both branches guarantee return.
A `while` never establishes a return guarantee, even with a literal `true` condition.
The checker examines every statement, including statements after a guaranteed return.

These rules are conservative because the checker does not evaluate conditions.
They reject some non-void functions whose loops never end.
This limit keeps return analysis separate from constant evaluation and program execution.

### Errors and limits

The checker stops at the first error and returns no partial analysis.
Name resolution completes before type analysis, so name errors take priority throughout the program.
Function signature errors precede body type errors.
Other type errors follow source traversal order.
Diagnostics identify invalid type syntax, an incompatible expression, an invalid call or return, or a missing return guarantee.

The token, AST, and resolution commands do not perform type checking.
The checker does not enforce numeric limits, analyze definite initialization, require `main`, or execute code.
For example, `int x = x;` passes type analysis without establishing that `x` holds an initialized value.
Division by zero and integer overflow remain outside this analysis because the checker does not evaluate expressions.

## Planned language scope

The following language scope grows phase by phase (see [roadmap.md](roadmap.md)):

- Types: `int`, `bool`, `void` (extend later if needed)
- Statements: declarations, assignment, `if`/`else`, `while`, `return`, blocks
- Expressions: arithmetic, comparisons, logical ops, calls
- Functions with parameters

Phase 1 covers all nine keywords, fifteen operators, and the comma.
The lexer recognizes 33 token types, including identifiers, integers, punctuation, and `EOF`.
The lexer recognizes token boundaries but does not check statement structure or types.

## Out of scope (start)

Preprocessor, pointers, structs, `goto`, unions.

After stage B works, reconsider these limits in phase 11.
Record each scope change here before implementation.

## Mindset

Implement each stage by hand.
Each implementation phase must include runnable examples, automated tests, and a short note about the concept it teaches.
