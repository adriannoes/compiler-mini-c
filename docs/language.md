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

## Current grammar

This grammar describes the parser after the array extension.
Braces mean repetition, square brackets mean an optional part, and quoted text names a token spelling.
These conventions form extended Backus-Naur form (EBNF), a compact notation for grammar rules.

```ebnf
program          = { function }, EOF ;
function         = type, IDENTIFIER, "(", [ parameters ], ")", block ;
type             = "int" | "bool" | "void" ;
parameters       = parameter, { ",", parameter } ;
parameter        = type, IDENTIFIER ;
block            = "{", { declaration | statement }, "}" ;
declaration      = type, IDENTIFIER, ( "[", INTEGER, "]"
                 | [ "=", expression ] ), ";" ;
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
                 | IDENTIFIER, [ "(", [ arguments ], ")" | "[", expression, "]" ]
                 | "(", expression, ")" ;
arguments        = expression, { ",", expression } ;
```

An assignment target must parse as a name or indexed access after parentheses disappear from the tree.
The parser rejects other target shapes at the assignment operator.
An `else` belongs to the nearest preceding `if` without an `else`, unless braces select another attachment.
Only named calls and named array accesses accept a suffix, and each primary accepts at most one suffix.

Syntax permits `void` declarations and unrestricted decimal length text.
Type analysis rejects variables and parameters of type `void`, invalid lengths, non-integer indices, and incompatible values.
Array lengths range from 1 through 1024. Parameters and return values remain scalar.
The [array rules](#phase-11-local-arrays) specify storage, bounds, and evaluation order.
The phase sections below retain the decisions and limits introduced by each increment.

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
| `LEFT_BRACKET`, `RIGHT_BRACKET` | `[`, `]` |
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

The [current grammar](#current-grammar) consolidates this syntax with the phase 11 array extension.
Phase 2 supports scalar declarations, names, and calls. Phase 11 adds bracketed lengths and indexed accesses.

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
The front-end library provides `check_types` analysis and the basic type rules.
The `--check` command performs the complete analysis independently of the earlier command modes.

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
Diagnostics identify invalid type syntax, an incompatible expression, an invalid call or return, or a missing return guarantee.

The checker visits expression children before their parent, with left operands before right operands.
It examines argument counts before argument types, then visits arguments in source order.
Declaration type errors precede initializer analysis, and invalid return forms precede returned-expression analysis.
Missing return guarantees follow the complete function body analysis.

The token, AST, and resolution commands do not perform type checking.
The checker does not enforce numeric limits, analyze definite initialization, require `main`, or execute code.
For example, `int x = x;` passes type analysis without establishing that `x` holds an initialized value.
Division by zero and integer overflow remain outside this analysis because the checker does not evaluate expressions.

### Type-check command

After the build, operate `./build/minic --check examples/types.mc` from the repository root.
The command completes lexing, parsing, name resolution, and type analysis in that order.
On success, it returns `0` without output on either stream.
On failure, it returns `1`, prints no standard output, and writes the diagnostic to standard error.
It stops at the first error with no partial result.

Diagnostics use `file:line:column: error: message`.
Type mismatches report the expected type and the actual type.
Invalid call counts report the function name and the expected and actual argument counts.
Other diagnostics identify an invalid `void` declaration, an invalid return form, or a missing return guarantee.

| Error | Reported position |
|-------|-------------------|
| Variable or parameter with type `void` | Written type keyword |
| Unary, binary, or assignment mismatch | Operator |
| Initializer or argument mismatch | Initializer or argument expression |
| Invalid call argument count | Called function name |
| Invalid return form or incompatible return type | `return` keyword |
| Missing return guarantee | Function name |

The [type example](../examples/types.mc) succeeds with empty output.
The [type-error example](../examples/type_error.mc) succeeds in the AST and resolution modes but fails in the type-check mode.
Its initializer reports `Expected type bool. Received int.` at line 3, column 16.
An empty program also succeeds because the checker does not require `main`.

## Phase 6 execution rules

The phase 6 interpreter evaluates expressions, declarations, assignments, blocks, empty statements, and returns.
Phase 7 extends this foundation with conditions, loops, and function calls.
The value library, `interpret` API, and `--run` command provide these behaviors.

### Values and numeric limits

Execution uses signed 64-bit integers, booleans, and `void` with no value.
Integers range from `-9223372036854775808` through `9223372036854775807`.
Only execution enforces this range. The earlier analysis modes retain their existing contracts.
Decimal leading zeroes do not change the base.
The interpreter converts only literals that it evaluates.

A minus directly applied to integer literal syntax permits `-9223372036854775808` without first constructing an invalid positive value.
Parentheses do not change this rule because the AST does not retain them.
The positive literal `9223372036854775808` remains outside the range.
Negating the minimum integer value produces an overflow error.
The source text and AST retain the original literal digits.

Addition, subtraction, multiplication, and negation report overflow before an invalid C++ calculation occurs.
Division truncates toward zero. A nonzero remainder has the sign of the left operand.
Division and remainder by zero produce runtime errors.
Both division and remainder of the minimum integer by `-1` produce overflow errors.
Integer operations do not wrap around the limits.

### Evaluation and environments

The interpreter completes name resolution and type analysis for the entire program before execution starts.
Static errors therefore take priority even in functions or statements that execution never reaches.
It then selects a function without parameters as the entry point.
The default entry name is `main`, and its return type can be `int`, `bool`, or `void`.
The earlier analysis commands still do not require an entry function.

Operands evaluate from left to right.
`&&` skips its right operand when the left operand is `false`.
`||` skips its right operand when the left operand is `true`.
Assignment evaluates its value without reading the previous target value, stores the result, and returns that result.
Chained assignments evaluate the inner assignment before they store the outer result.

Each block owns an environment containing its active local variables.
Runtime lookup uses the symbol chosen by name resolution, so shadowing retains its lexical meaning.
A declaration creates a slot without a value before its initializer runs.
A read from a slot without a value produces a runtime error.
Thus, `int x = x;` passes static analysis but fails during execution.

Assignment can initialize a variable declared without an initializer.
Block exit removes the local environment, including when a return or error ends the block.
A return propagates through nested blocks and ends the current function.
A `void` entry can also end without an explicit return.
Statements after an executed return do not run, but they still require valid static analysis.

Function calls use the phase 7 frame rules.
Skipped logical operands, unselected branches, and statements after a return do not trigger runtime errors or enter call frames.
All source still requires valid static analysis, including code that execution never reaches.

### API and command contract

The API is `Value interpret(const Program&, const std::string& entry = "main")`.
It analyzes the supplied program and executes the selected entry without parameters.
The returned value owns its data and remains valid after the AST expires.
The call does not change the AST or reuse caller-supplied analysis from another program.

The `--run <file>` command uses `main` as its entry.
It prints an integer in decimal or a boolean as `true` or `false`, followed by a newline.
A `void` result produces no output.
Successful execution returns status `0`, including when the entry returns a nonzero integer.
An error returns status `1`, with no standard output and a positioned diagnostic on standard error.

After the build, operate `./build/minic --run examples/interpreter.mc` from the repository root.
The command prints `42` and returns status `0`.
`./build/minic --run examples/short_circuit.mc` prints `true` and returns status `0`.
The [runtime-error example](../examples/runtime_error.mc) passes static analysis but fails on its initializer at line 3, column 15.
The [call example](../examples/calls.mc) prints `6` with `--run`.
The control-flow and mutual-recursion examples require an entry function. Their command tests add one in temporary files.
Syntax-only examples retain their earlier analysis contracts.

### Runtime diagnostics

Runtime diagnostics use `file:line:column: error: message`.
Static errors retain their earlier stage priority, and execution stops at its first runtime error.
Runtime errors do not expose C++ overflow, read uninitialized C++ storage, or print partial result values.

| Error | Reported position |
|-------|-------------------|
| Missing entry function | Line 1, column 1 |
| Entry with parameters | Entry function name |
| Positive integer literal outside the range | Integer literal |
| Negative literal outside the range | Unary minus |
| Arithmetic overflow or zero divisor | Operator |
| Variable without a value | Name use |
| Call-depth limit | Called function name |
| Execution step limit | Next expression or statement |

The caller receives `RuntimeError` for runtime failures through the C++ API.
The API can also report existing static-analysis errors before it selects or executes an entry.
Neither the API nor the command changes the source AST, name rules, type rules, or grammar.

## Phase 7 execution rules

Phase 7 completes interpreter support for the existing functions and control-flow syntax.
It preserves the grammar, lexical bindings, type rules, numeric behavior, and entry rules.
Conditions, loops, function frames, recursion, and execution limits are available.

### Conditions and loops

An `if` evaluates its condition once and executes only the selected branch.
An `if` without an `else` does nothing when its condition is false.
A `while` evaluates its condition before every iteration, including before the first iteration.
Both statements use the boolean types established by static analysis.
An unselected branch or skipped loop body produces no runtime side effects or errors.

Only an explicit block creates a lexical environment.
A loop body with braces creates a new environment on every iteration, so local variables do not retain earlier iteration values.
A body without braces uses its existing scope.
A return propagates through conditions and loops until it ends the current function.
The conservative static return rules remain unchanged, including their treatment of `while`.

### Calls and frames

Each function call uses the declaration selected by name resolution.
The interpreter evaluates arguments from left to right in the caller before creating the callee frame.
Parameters receive copies of argument values. Assigning to a parameter does not change the original caller variable.
Parameters and the outer function body share an environment, as defined by the existing scope rules.

A frame owns the active environments for one function activation.
Runtime lookup searches only the current frame, without examining caller environments.
Recursive activations reuse lexical symbol identifiers but own separate value slots.
Forward calls, direct recursion, and mutual recursion use the existing name bindings.
Each return ends only its current function, and the call supplies that value to the caller.

A `void` function returns no value, including when its body ends without an explicit return.
Static analysis still rejects invalid argument counts, invalid argument types, and invalid uses of a `void` result.
The interpreter performs static analysis once before entry selection and reuses it across calls.
The API and CLI entry still require zero parameters, independently of internal calls with arguments.

### Execution limits

`ExecutionLimits` defaults to `max_steps = 1000000` and `max_call_depth = 128`.
Both limits must be positive. Invalid API limits produce `std::invalid_argument` before static analysis.
The existing two-argument `interpret` call uses these defaults.
The overload `interpret(program, entry, limits)` accepts explicit limits without adding CLI flags.

Entering an expression or statement consumes one step before evaluation.
Each function body counts as a block, and each repeated loop condition consumes expression steps.
A unary minus and its direct literal each consume a step, including the minimum-integer case.
Skipped operands, branches, and statements consume no execution steps.
Static analysis does not consume the runtime budget.

The step limit reports `The execution step limit was reached.` at the next node that would exceed the budget.
The call-depth limit counts active frames, including the entry as depth one.
It reports `The call depth limit was reached.` when another frame would exceed the limit.
The diagnostic identifies the called function name.
Arguments complete evaluation before the interpreter attempts to enter that frame.
Limits stop nonterminating loops and recursion without changing the success output or failure status contract.

### Recursive examples

The [factorial example](../examples/factorial.mc) computes `factorial(6)` with a base case and one recursive call per remaining case.
The [Fibonacci example](../examples/fibonacci.mc) computes `fibonacci(10)` with two recursive calls per remaining case.
Each call receives its own copy of `n`, and intermediate values remain in the caller during nested calls.
With `--run`, these examples print `720` and `55`, respectively, and return process status `0`.

## Phase 8 bytecode rules

The approved scope compiles the existing language to bytecode without adding syntax or executing the result.
The API is `BytecodeProgram compile_bytecode(const Program&)`.
It performs complete name and type analysis once before generating instructions for all functions.
Like `--check`, compilation accepts an empty program and does not require `main` or an entry signature.
The result owns its data and remains valid after the AST expires.

Compilation preserves lexical bindings, copied parameters, argument order, short-circuit logic, branches, loops, and returns.
Each function has separate parameter and local slots. Every declaration clears its slot before its initializer, including during repeated loop iterations.
Integer constants retain their original digits and a separate sign for a directly negated literal.
Compilation does not convert literals, evaluate arithmetic, or reject runtime errors in code that execution can skip.
The static rules still apply to every function and statement.

Source positions and explicit step markers retain the phase 7 execution contract for the VM.
The unary-minus literal case retains two source steps. Assignment targets do not add a separate read or step.
The `--bytecode <file>` command prints deterministic instructions only after compilation succeeds.
It preserves the existing diagnostic format, returns status `1` without standard output on errors, and leaves earlier commands unchanged.
VM execution, binary file serialization, and optimizations remain outside phase 8.

From the repository root, operate the bytecode command after the build:

```bash
./build/minic --bytecode examples/factorial.mc
./build/minic --bytecode examples/control_flow.mc
./build/minic --bytecode examples/runtime_error.mc
```

Each command returns status `0` and prints functions, constants, local slots, and positioned instructions.
The factorial output includes a recursive `CALL 0`. The control-flow output includes forward branches and a backward loop jump.
The runtime-error output retains the uninitialized read as `LOAD_LOCAL`, without evaluating it.
The `--run` command still executes through the tree-walk interpreter and retains all phase 7 limits and diagnostics.

## Phase 9 VM rules

The approved VM executes the phase 8 instruction set without changing the language or bytecode format.
The API is `Value execute_bytecode(const BytecodeProgram&, const std::string& entry = "main")`.
The overload `execute_bytecode(program, entry, limits)` accepts the shared `ExecutionLimits` type.
The VM executes entries, expressions, local slots, branches, loops, calls, and returns.

The API accepts compiler-produced bytecode and its optimized form, and performs no AST analysis during execution.
The bytecode remains unchanged and supports repeated execution with independent runtime state.
The result owns its data. Missing entries and entries with parameters retain the interpreter diagnostics and source positions.
The entry defaults to `main` and requires zero parameters.

Numeric operations reuse the signed 64-bit runtime helpers, including their range, overflow, and divisor diagnostics.
Integer conversion occurs only at an executed `LOAD_CONSTANT` instruction.
Local slots distinguish a missing value from an initialized integer or boolean.
`RESET_LOCAL` clears each executed declaration before its initializer, including during repeated loop iterations.
An executed read without a value reports the original variable name and use position.

The VM uses explicit frames and an iterative instruction loop instead of recursive C++ calls.
Each frame records its function, next instruction, local values, and base within the value stack.
Calls copy arguments into separate parameter slots after argument evaluation and preserve the caller values below that base.
Returns end only the current activation and restore the caller stack before supplying a non-void result.
Void functions produce no stack value, including on implicit fallthrough.

Only `STEP` consumes a source step. The shared budget continues across calls and returns.
The defaults remain 1000000 steps and 128 active calls, including the entry as depth one.
Both limits must be positive. Invalid limits produce `std::invalid_argument` before entry selection.
Step exhaustion identifies the next source node. Depth exhaustion identifies the called name after argument evaluation.
Indices, operand types, and stack access have guards that report malformed executed bytecode as `std::invalid_argument`.
The API contract requires compiler-produced control flow, including `STEP` on execution cycles.

The `--run-vm <file>` command compiles the source and executes `main` on the VM.
It retains `--run` output, diagnostic, and exit-status rules, including no partial result on errors.
The command completes static analysis and compilation before it selects or executes the entry.
The existing `--run` command continues to use the tree-walk interpreter.
Optimizations and binary serialization remain outside phase 9.

From the repository root, operate the VM command after the build:

```bash
./build/minic --run-vm examples/factorial.mc
./build/minic --run-vm examples/fibonacci.mc
./build/minic --run-vm examples/short_circuit.mc
./build/minic --run-vm examples/runtime_error.mc
```

The first three commands print `720`, `55`, and `true`, respectively, and return status `0`.
The runtime-error command reports `The variable "value" has no value.` at line 3, column 15, and returns status `1`.
The control-flow and mutual-recursion examples still require an entry for execution, while compilation alone accepts them unchanged.

## Phase 10 optimization rules

The approved scope optimizes bytecode without changing the AST, language, or existing command behavior.
The API is `BytecodeProgram optimize_bytecode(const BytecodeProgram&)`.
It accepts compiler-produced bytecode, returns an owned copy, and preserves the original program.
Function indices, signatures, local slots, and source metadata retain their existing meaning.
Optimization does not require an entry or repeat static analysis.

Constant folding replaces successful constant operations with their computed values.
The optimizer uses the existing signed 64-bit helpers for integer conversion, arithmetic, comparisons, and unary operations.
If conversion or evaluation reports a runtime error, the original instructions remain for execution.
Thus, optimization does not report overflow or division by zero before execution reaches the original operation.
Original constants retain their digit text. Folded integer results use canonical decimal digits and a separate negative flag.

Every reachable source step retains its order and position, even when the optimizer removes value operations around it.
Step-limit and call-depth behavior must agree with the original VM and interpreter, including the first error position.
Calls, assignments, local reads, declaration resets, and other possible effects end local constant tracking.
The optimizer does not remove variable reads or apply identities that can suppress errors, such as replacing `x * 0` with zero.

The optimizer simplifies constant branches, removes unreachable code, and applies small local instruction rewrites.
Those transformations respect block entries and remap absolute jump targets after code removal.
Unreachable source steps can disappear because execution never visits them.
Discarding a known constant is safe only when its conversion succeeds.
The optimizer preserves calls and variable reads even when later code discards their result.
Functions remain available as named entries even when the selected entry never calls them.
The `--bytecode-opt` and `--run-vm-opt` commands expose optimized output and execution separately from the existing commands.
Global constant propagation, store elimination, language changes, and binary serialization remain outside this phase.

The optimized commands complete parsing, static analysis, and compilation before optimization starts.
`--bytecode-opt` accepts programs without an entry, prints the optimized disassembly, and performs no execution.
`--run-vm-opt` selects `main` without parameters, uses the default execution limits, and retains the existing output and error rules.
The API can execute optimized bytecode through `execute_bytecode` with a selected entry and custom limits.

From the repository root, operate the optimized commands after the build:

```bash
./build/minic --bytecode-opt examples/optimization.mc
./build/minic --run-vm-opt examples/optimization.mc
./build/minic --run-vm-opt examples/runtime_error.mc
```

The optimization example prints `24` and returns status `0`. Its code decreases from 69 instructions to 36.
The runtime-error example retains its diagnostic at line 3, column 15, with status `1` and no standard output.
Constant pools retain unused entries. Optimization preserves function indices, local metadata, and source positions for instructions that remain.

## Phase 11 local arrays

The approved extension adds local, one-dimensional arrays of `int` or `bool`.
Declarations use `int values[4];` or `bool flags[2];`. Each declaration still introduces one name within a block.
The length is decimal integer syntax with a static range from 1 through 1024, including decimal leading zeroes.
The parser preserves the length text and position without converting it. Type analysis enforces the range after name resolution.
The lexer adds `LEFT_BRACKET` and `RIGHT_BRACKET` for `[` and `]`, for 35 token types including `EOF`.

The declaration syntax extends the phase 2 grammar:

```ebnf
declaration -> type IDENTIFIER ("[" INTEGER "]" | ("=" expression)?) ";"
primary     -> IDENTIFIER "[" expression "]" | phase-2-primary
```

Only a named array has indexing syntax. Indexing has primary-expression precedence and does not permit another index or call suffix.
The parser also accepts indexed assignment targets, including a parenthesized indexed expression.
Array initializers, whole-array assignment, array arguments, array returns, and multidimensional arrays remain outside the language.
Function parameters and returns remain scalar. Scalar element values can serve as ordinary arguments and results.
Structs, pointers, dynamic allocation, and garbage collection remain outside this phase.

An indexed use binds to the array declaration with the existing `Read` or `Write` use kind.
Its index expression resolves in the same lexical scope, before the right operand of an assignment.
Array names follow the existing declaration visibility, shadowing, and collision rules.
Using an array name without an index is a type error. Indexing a scalar is also a type error.
The index must have type `int`, and the selected element has the declared scalar type without coercion.

Each function activation owns its array storage, and every executed declaration clears all its elements.
Elements have no value until assignment initializes them, independently of other elements.
An access requires an index from zero through length minus one. A missing element value and an invalid index produce positioned runtime errors.
Both errors identify the indexed use at the array name position.

An indexed assignment evaluates and checks its index before evaluating its right operand.
It then writes through that saved index without reading the previous element value, and returns the assigned scalar value.
This order preserves the selected element when the right operand changes variables used by the index or calls another function.
An indexed read evaluates its index, checks the bounds, and then reads the element.

Each indexed access consumes one source step, including an assignment target, in addition to the steps of its index expression.
The assignment expression consumes its existing parent step before the indexed target and right operand.
An array length is declaration metadata, so clearing storage adds no length-expression or per-element source steps.
The interpreter, original VM, and optimized VM must agree on values, error positions, and execution limits.
The optimizer treats element checks, reads, and writes as barriers and retains their effects and possible errors.

Syntax, bindings, types, bytecode, and execution support these rules in all three engines.
The existing commands and public scalar `Value` contract remain unchanged for programs without arrays.
The [array example](../examples/arrays.mc) returns `10` in all three engines.
The [array-error example](../examples/array_error.mc) reads an element without a value and fails at line 5, column 10.
Command fixtures cover tokens, AST data, bindings, types, both disassemblies, and all execution modes.
The syntax-only and names-only commands also accept invalid lengths and index types until type analysis checks them.

## Phase 12 LLVM study boundary

Phase 12 studies LLVM through an independent C laboratory and the [LLVM notes](theory-llvm.md).
The Mini-C syntax, analysis rules, bytecode, execution engines, diagnostics, and limits retain their phase 11 contracts.
The laboratory uses C integer semantics rather than the Mini-C signed 64-bit rules.
Clang remains a laboratory tool rather than a dependency of the Mini-C executable.
An LLVM backend and additional commands remain outside this phase.

## Language summary

The completed scope follows the [current grammar](#current-grammar) and the semantic rules above:

- Scalar types: `int`, `bool`, and `void` for function results
- Local arrays: `int` or `bool` elements with fixed lengths
- Statements: declarations, assignment, `if`/`else`, `while`, `return`, blocks
- Expressions: arithmetic, comparisons, logical ops, calls
- Functions with parameters

Phase 1 covers all nine keywords, fifteen operators, and the comma.
The original phase 1 vocabulary contains 33 token types, including identifiers, integers, punctuation, and `EOF`.
Phase 11 adds the bracket tokens for a total of 35.
The lexer recognizes token boundaries but does not check statement structure or types.

## Excluded features

The language excludes a preprocessor, pointers, structs, unions, `goto`, strings, floating-point values, and dynamic allocation.
It also excludes global variables, function prototypes, multidimensional arrays, array initializers, and whole-array values.
The compiler exposes no LLVM backend or native-code command.
The independent C/LLVM laboratory retains its [phase 12 study boundary](#phase-12-llvm-study-boundary).

## Mindset

Implement each stage by hand.
Each implementation phase must include runnable examples, automated tests, and a short note about the concept it teaches.
