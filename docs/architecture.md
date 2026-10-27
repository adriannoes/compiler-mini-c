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
The `--bytecode` command analyzes the program, compiles instructions, and prints their disassembly.
The `--run` command completes static analysis, executes the supported subset, and prints the entry result when it has a value.
The `--run-vm` command compiles the analyzed program and executes its bytecode through the VM.
The `--bytecode-opt` and `--run-vm-opt` commands also optimize bytecode before disassembly or execution.

```text
source (.mc) -> minic::lex -> tokens
tokens -> --tokens output
tokens -> minic::parse -> Program
Program -> --ast output
Program -> minic::resolve_names -> NameResolution -> --resolve output
Program -> minic::check_types -> TypeAnalysis -> --check status
Program -> minic::compile_bytecode -> BytecodeProgram -> --bytecode output
Program -> minic::interpret -> Value -> --run output
Program -> minic::compile_bytecode -> BytecodeProgram -> minic::execute_bytecode -> Value -> --run-vm output
BytecodeProgram -> minic::optimize_bytecode -> BytecodeProgram -> --bytecode-opt output or --run-vm-opt execution
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
The `minic` executable links to `minic_vm` and `minic_optimizer`.
Both libraries link to `minic_runtime` and `minic_bytecode`, which depend on `minic_frontend`.
Analysis tests link directly to `minic_frontend`, and runtime tests link to `minic_runtime`.
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

The interpreter owns its `TypeAnalysis` during execution and indexes variable and parameter declarations by symbol identifier.
It uses the recorded bindings for reads and assignments without another search by source name.
Each block creates an environment, and a scope guard ends its lifetime on normal exit, return, or error.
Variable slots use `std::optional<Value>` so a missing value remains distinct from `void`.
Insertion precedes initializer evaluation, and assignment does not read the previous target value.

Statement execution returns `std::optional<Value>` as a control signal.
`std::nullopt` continues execution, while an engaged value propagates a return through nested blocks.
An engaged `std::monostate` ends a `void` entry, unlike fallthrough without a return.
Expressions evaluate their left operand before the right operand, and logical operators can skip the right operand.
Calls evaluate arguments before entering an isolated frame and return their values to the caller.
Conditions select one branch, and loops reevaluate their condition before each body execution.

The public call accepts temporary programs because its returned value contains no AST references.
Repeated calls create fresh analysis and environment state without changing the source tree.
No interpreter instance or environment escapes the call.

`--run <file>` uses `main` as its entry and formats the returned value only after execution succeeds.
An integer or boolean result ends with a newline, while a `void` result prints nothing.
The process status remains `0` on success, independently of the entry result, and becomes `1` on failure.
Function calls, conditions, and loops execute using the existing static bindings and return signals.
The existing analysis modes do not invoke execution.

## Interpreter learning note

A lexical binding identifies a declaration, while an environment stores the value for its active execution.
Separate symbol identifiers preserve shadowing even when a declaration appears after an earlier use of an outer variable.
Explicit missing-value state reports uninitialized reads without changing static name or type analysis.
Return propagation and block cleanup define execution order without requiring function-call frames in phase 6.

## Interpreter validation and limits

Phase 6 adds six numeric test groups, sixteen interpreter API groups, and twenty-one command tests.
That phase ends with 162 CTest tests, including the original token, AST, resolution, and type-analysis contracts.
All 162 pass in both normal and AddressSanitizer/UndefinedBehaviorSanitizer builds without compiler warnings on Apple Clang 17.
The commands live in [tech-stack.md](tech-stack.md#sanitizer-validation).

The API tests cover numeric boundaries, missing values, lexical bindings, evaluation order, skipped nodes, return propagation, and static error priority.
They also cover repeated execution, AST preservation, and returned values that outlive temporary programs.
Command tests compare exact result text, empty void output, positioned diagnostics, and process exit codes.
The runtime-error example passes `--check` and fails with the expected uninitialized-read diagnostic in `--run`.

The runtime requires an entry without parameters and executes functions, conditions, loops, and recursion under phase 7 rules.
Numeric and initialization rules remain unchanged.
The interpreter does not implement bytecode or VM behavior.
Recursive AST traversal remains limited by the available C++ call stack.

## Function execution and limits

The approved [phase 7 rules](language.md#phase-7-execution-rules) define control flow, calls, frame isolation, and execution limits.
`if` executes only its selected branch, and `while` evaluates its condition on every iteration.
Explicit blocks retain the existing environment guard, including fresh local slots on each loop iteration.
The existing return signal propagates through both control-flow statements.

`ExecutionLimits` supplies positive step and call-depth limits without changing existing interpreter calls.
The two-argument function delegates to the overload with limits.
A single counter charges expression and statement entry, including each function body block and repeated loop condition.
The interpreter compares the counter with its limit before incrementing it, so the counter cannot overflow.
Budget exhaustion reports the next node position without evaluating it.

Each call owns a `CallFrame` through `std::unique_ptr` in a vector of active frames.
The frame owns its environment stack, and lookup remains inside that frame.
This adds one frame allocation per call and keeps environment guards valid while nested calls grow the frame vector.
Arguments evaluate in the caller before parameter values enter the new frame.
The interpreter reuses its existing static analysis instead of repeating it for each activation.

A frame guard creates the activation, and an environment guard creates its shared parameter and outer-body environment.
Parameters enter that environment as copied values before the body starts.
The environment guard ends local lifetimes before the frame guard ends the activation on return or error.
No variable-slot reference crosses an argument evaluation or a nested call.
Return values own their data and remain valid after the callee frame ends.

The call-depth guard compares the number of active frames with its positive limit before allocating another frame.
The entry counts as depth one, and returning frees capacity for later calls.
The shared step counter continues across calls and returns, so nested calls cannot reset the execution budget.
Limit failures identify the called function or the next statement or expression as specified in the language rules.

The factorial and Fibonacci examples exercise one and two recursive calls per recursive case, respectively.
Bytecode and VM work remain in their later roadmap phases.
The existing analysis modes remain independent of execution limits.

Phase 7 adds thirteen interpreter API groups and ten command tests, for 185 tests in the complete suite.
Existing call and control-flow cases now require successful execution instead of unsupported-node errors.
The new groups cover frame isolation, copied parameters, argument order, return propagation, recursion, and both execution limits.
Command tests compare exact example results, callee diagnostics, limit positions, empty error output, and exit codes.
All 185 tests pass in normal and AddressSanitizer/UndefinedBehaviorSanitizer builds without compiler warnings on Apple Clang 17.

Function evaluation still uses the C++ call stack, and a larger configured call-depth limit can exceed the available native stack.
Parsing, static analysis, and deeply nested AST evaluation also retain their existing native-stack limits.
The default call-depth limit passes the recursive error cases under both sanitizers.

## Bytecode format and compilation

Bytecode is an instruction sequence for the stack VM.
The `minic_bytecode` library owns its format and deterministic printer independently of the interpreter.
The library provides `BytecodeProgram`, its records, `compile_bytecode`, and `print_bytecode`.
The `--bytecode` command prints the complete disassembly after compilation succeeds.

`BytecodeProgram` owns functions in source order.
Each function owns its name, return type, source position, local metadata, constants, and instruction vector.
The first local slots hold parameters in declaration order. Later slots identify distinct local declarations, including shadowed names.
Each activation will have its own slots. Lexical bindings select slot indices during compilation, without runtime name searches.
This reserves space for every declaration in a function but avoids slot reuse and scope instructions.

Constants contain booleans or an `IntegerConstant` with owned decimal digits and a negative flag.
Compilation preserves digits and defers integer conversion to execution, including literals in skipped code and the minimum signed integer.
Instructions contain an `OpCode`, one index operand where applicable, and an owned `SourceLocation` value.
Jump operands are absolute instruction indices within the current function. Call operands are function indices within the program.
The format uses C++ records in memory. It defines no packed encoding, file serialization, or compatibility version.

The instruction contract defines these stack effects:

| Instruction | Operand | Stack effect and behavior |
|-------------|---------|---------------------------|
| `STEP` | None | Charge one source expression or statement entry without changing the value stack |
| `LOAD_CONSTANT` | Constant index | Convert and push the constant when execution reaches the instruction |
| `LOAD_LOCAL` | Local index | Push a copy, or report a read without a value |
| `STORE_LOCAL` | Local index | Copy the top value into the slot and retain the top value |
| `RESET_LOCAL` | Local index | Clear the slot before each declaration execution |
| `POP` | None | Discard the top value |
| `NEGATE`, `NOT` | None | Replace one operand with its unary result |
| `ADD`, `SUBTRACT`, `MULTIPLY`, `DIVIDE`, `REMAINDER` | None | Pop right and left operands, then push the numeric result |
| `EQUAL`, `NOT_EQUAL`, `LESS`, `LESS_EQUAL`, `GREATER`, `GREATER_EQUAL` | None | Pop right and left operands, then push the boolean result |
| `JUMP` | Instruction index | Continue at the target |
| `JUMP_IF_FALSE`, `JUMP_IF_TRUE` | Instruction index | Pop a boolean and jump when it matches the condition |
| `CALL` | Function index | Consume arguments in parameter order and push a result only for a non-void function |
| `RETURN` | None | End the current function with its top value |
| `RETURN_VOID` | None | End the current function without a value |
| `UNREACHABLE` | None | Identify an internal control-flow failure if execution reaches it |

Instruction positions preserve operators, name uses, calls, and statement locations for future diagnostics.
Separate `STEP` instructions preserve source-level accounting even when lowering requires several stack operations.
Synthetic jumps, constants, stores, and returns consume no extra source steps.
The compiler places loop-condition steps at the backward jump target and places skipped expression steps inside their selected paths.

`compile_bytecode` analyzes names and types once before generating code.
It preassigns function indices for forward and recursive calls, then lowers each body in source order.
Parameters occupy the first slots, and declarations add slots as the compiler visits them.
All declarations and uses map through the static symbol identifiers. The compiler never chooses a slot from its text name.
Compilation retains unreachable statements and performs no constant folding or arithmetic evaluation.

Every expression leaves one stack value except a `void` call, which leaves none.
Expression statements discard non-void values. Initializers store and discard their value, while assignments retain it for surrounding expressions.
Conditional jumps consume their boolean operand. A skipped logical operand uses a synthetic constant to produce the short-circuit result.
Arguments enter the stack from left to right before `CALL` consumes them using the callee signature.
`RETURN` consumes a result, and `RETURN_VOID` handles explicit returns and implicit void fallthrough.
Each non-void function ends with `UNREACHABLE` as an internal guard, which the existing conservative return analysis makes unreachable.

Eighteen compiler test groups inspect bindings, constants, jumps, calls, ownership, static errors, source steps, and existing examples.
An independent test helper traverses generated control flow with abstract stack types.
It checks operand indices, branch merges, stack underflow, call signatures, and return types without executing Mini-C values.
The tests cover both paths of each conditional jump and the backward edges of loops.

Twelve existing examples compile and pass the stack analysis, including factorial, Fibonacci, mutual recursion, and the uninitialized-read example.
Six exact command fixtures cover hello, calls, control flow, short-circuit logic, factorial, and the runtime-error example.
The fixtures record source positions, slot indices, constant text, function indices, and jump targets.
Sixteen command tests cover those fixtures, entry-independent compilation, numeric text, argument errors, and the existing front-end diagnostics.
Phase 8 ends with 222 tests. All pass in normal and AddressSanitizer/UndefinedBehaviorSanitizer builds without compiler warnings on Apple Clang 17.
The commands live in [tech-stack.md](tech-stack.md#sanitizer-validation).

The printer numbers functions, constants, locals, and instructions from zero and includes each instruction position.
It preserves integer digits, prints booleans as `true` or `false`, and uses quoted names.
It prints structure without executing instructions or validating arbitrary caller-built bytecode.
The VM executes the instruction contract and supports comparisons with the interpreter. Phase 10 retains optimizations.
Bytecode compilation and printing still traverse finite C++ structures and retain the existing native-stack limit for deeply nested source.

## VM execution

The `minic_vm` library executes compiler-produced bytecode and its optimized form through an iterative instruction loop.
It links to `minic_bytecode` and `minic_runtime` and shares the existing numeric helpers and owned `Value` type.
`ExecutionLimits` lives in `runtime.hpp`, which `interpreter.hpp` still includes. Existing interpreter API calls remain compatible.
The public API accepts a selected entry and optional positive execution limits, without performing another static analysis.

The machine owns a vector of frames and a vector of temporary values for each execution.
A frame records a function index, instruction index, stack base, and optional values for each local slot.
The function metadata and instructions remain in the caller bytecode, which execution reads without modification.
The machine copies instructions before dispatch and keeps no frame reference across frame growth.
This design supports recursive Mini-C calls without adding recursive C++ calls.

The stack base separates caller temporaries from the current activation.
Stack operations guard that boundary, instruction and operand indices have bounds guards, and arithmetic guards operand types.
Invalid indices and operand types report `std::invalid_argument` when an instruction uses them.
`UNREACHABLE` reports an internal logic error if a typed non-void function falls through.
The API expects compiler-produced control flow. These guards do not define a loader or verifier for arbitrary binary programs.

`LOAD_CONSTANT` converts owned digits only when execution reaches the instruction.
Unary and binary operations use the same checked helpers as the interpreter.
`STORE_LOCAL` retains its top value, while `RESET_LOCAL` clears a declaration slot before its initializer.
Conditional jumps pop their boolean, and absolute targets preserve compiler-generated short-circuit and loop paths.
`STEP` alone charges the source budget, so synthetic instructions do not change the phase 7 limit contract.

`CALL` uses the callee signature to locate argument values above the caller stack base.
The machine checks depth after argument evaluation, copies parameters into new local slots, and then consumes the argument values.
The new frame base preserves caller temporaries, including earlier arguments and pending binary operands.
The caller instruction index already points after `CALL`, so ending the callee resumes the caller without another lookup.
The machine ends each frame with an empty temporary range, then supplies a result only for a non-void return.
No frame or value reference survives vector growth during call entry or return.

Twenty-seven VM groups cover operations, slots, branches, loops, calls, return restoration, numeric errors, limits, ownership, guarded accesses, and examples.
The deep-call test reaches 5002 active frames and returns through pending additions with a configured call-depth limit of 6000.
Differential tests compare values and exact runtime messages and positions with the interpreter.
The budget test compares ten programs at eighty step limits and three call-depth limits, for 2400 engine comparisons.
It covers both completed runs and stopped runs, including loops, skipped operands, nested calls, and shared budgets.

The fixture group compares twelve existing examples between the VM and interpreter, including the runtime-error example.
Temporary entries preserve the source fixtures for control flow and mutual recursion.
Thirty-one VM command tests reuse the interpreter scenarios with a distinct temporary file path and the `--run-vm` flag.
They compare exact output, diagnostics, and exit codes, including default step and depth failures.
The command compiles before execution and prints only the completed result through the existing value formatter.
The complete suite contains 280 tests. All pass in normal and AddressSanitizer/UndefinedBehaviorSanitizer builds without compiler warnings on Apple Clang 17.
The prose scan reports no findings. The build commands live in [tech-stack.md](tech-stack.md#sanitizer-validation).

Explicit VM calls use heap-owned frame and value vectors. Source parsing and AST compilation retain their existing native-stack limits.
The step and depth limits constrain compiler-produced execution, with `STEP` marking the source-level budget.
The format still defines no binary loader, optimizer, or compatibility version.

## Local bytecode optimization

The `minic_optimizer` library links to `minic_bytecode` and `minic_runtime`.
`optimize_bytecode` copies the input and transforms each function independently.
It preserves function order, signatures, locals, and source metadata. It adds no external dependency.
The optimizer provides local constant folding, control-flow cleanup, and small instruction rewrites.
The optimized commands compile once, optimize the owned bytecode, and then print or execute the result.

A basic block is an instruction region with one entry.
The optimizer marks function entry, jump targets, and instructions after jumps or returns as block entries.
It tracks a suffix of known constant values and their producing instructions inside each block.
`STEP` preserves that tracking without changing the abstract value stack.
Calls, local reads, local writes, and other unknown operations end the known suffix.
Block entries also end tracking, so branch merges cannot borrow values from one predecessor.

Successful unary and binary evaluations replace their producer loads and operation with one constant load.
The replacement occupies the operation position, while every interleaved `STEP` remains in place and in order.
The evaluator uses the same guarded numeric helpers as execution. A runtime error cancels that candidate without changing its original operations.
This preserves literal range errors, arithmetic errors, and limit failures during execution.

The pass copies surviving instructions and maps old instruction indices to new indices.
Targets at omitted instructions map to the next surviving instruction. Surviving jumps receive the remapped target.
Each rewrite stays inside a block, so no additional entry can bypass required value producers.
Folded constants append to the existing pool. Unused constants remain to avoid another pool-remapping pass.
This copies bytecode and retains some unused constants, but keeps ownership and existing indices explicit.

Constant boolean branches become unconditional jumps or fallthrough paths after consuming their known condition.
A known constant and its discard can disappear together, but only after successful constant conversion.
Local reads, calls, and potentially failing operations retain their behavior even when no later instruction consumes their result.
Jump threading redirects a jump through chains of unconditional jumps without crossing source steps.
An unconditional jump to its next instruction is redundant and disappears.

Reachability analysis traverses successors from each function entry, with both successors for an unknown conditional branch.
Only reachable instructions survive compaction, so unreachable source steps can disappear without changing execution counts.
The pipeline repeats local rewrites, jump threading, and reachability cleanup until instructions and targets stop changing.
Value rewrites and unreachable cleanup shrink the code, while jump threading resolves chains.
Every function remains in source order, including functions that no other function calls.

Twenty optimization test groups cover constants, numeric failures, source steps, block entries, effects, jumps, ownership, examples, and stable repeated optimization.
Differential tests compare the interpreter, original VM, and optimized VM for values and exact runtime diagnostics.
Eight programs at eighty step budgets and three call-depth limits provide 1920 limit combinations.
Tests also require fewer value instructions in constant expressions and stable results from repeated optimization.

The fixture group compares thirteen examples across the interpreter, original VM, and optimized VM.
Seven exact disassembly fixtures record optimized instructions, pool indices, source positions, and jump targets.
Thirty-two optimized execution cases reuse the existing command scenarios and include the new optimization example.
Fifteen optimized disassembly cases cover fixtures, empty programs, front-end errors, missing files, and argument diagnostics.
Each command scenario uses a distinct temporary input path across execution engines.
The optimization example decreases from 69 instructions to 36 and returns `24` in all three engines.
The instruction counts describe code reduction without a timing measurement.
The phase 10 suite contains 350 tests. All pass in normal and AddressSanitizer/UndefinedBehaviorSanitizer builds without compiler warnings on Apple Clang 17.
The prose scan reports no findings. The build commands live in [tech-stack.md](tech-stack.md#sanitizer-validation).

Reachability asks whether control flow can visit an instruction.
Liveness asks whether later code can read a value before another assignment replaces it.
Store elimination needs liveness information and error analysis. The current optimizer preserves stores and performs no global constant propagation.

## Local array representation

`ArrayExtent` owns decimal length text and its source position within a `VarDecl`.
`IndexExpr` owns the array name and its index expression, with the existing expression position at the name.
This resembles a named call and avoids treating an entire array as an expression value.
All earlier scalar AST layouts and print forms retain their existing behavior.

Name resolution binds indexed reads and writes directly to the declaration symbol and also visits the index expression.
Type analysis rejects whole-array uses, scalar indexing, non-integer indices, and incompatible element assignments.
It checks lengths without converting ordinary integer expressions or changing their runtime range rules.
`TypeAnalysis::array_length` returns the checked extent for a declaration in the existing AST.
Its declaration map retains the same AST lifetime requirement as the other analysis maps.

`BytecodeLocal::array_length` is zero for a scalar and positive for a fixed array.
The disassembler prints an array slot as `int[4]` or `bool[2]`, while scalar slot output stays unchanged.
`RESET_LOCAL` clears scalar or array storage according to that metadata.
Three indexed instructions use a local slot operand:

| Instruction | Stack effect | Behavior |
|-------------|--------------|----------|
| `CHECK_INDEX` | Retain the top index | Check a write address before its right operand |
| `LOAD_ELEMENT` | Pop the index, push the element | Check bounds and reject an element without a value |
| `STORE_ELEMENT` | Pop the value and index, push the value | Store the scalar without reading the old element |

The compiler emits a target step, index expression, and `CHECK_INDEX` before an indexed assignment value.
`STORE_ELEMENT` follows that value, so nested calls preserve the saved index below their own argument range.
Reads use `LOAD_ELEMENT` after the index expression and its source steps.
The type checker and compiler retain the same scalar `int`, `bool`, and `void` APIs for values and function signatures.

Nine front-end groups cover bracket positions, AST ownership, grammar limits, bindings, element types, lengths, errors, and lowering order.
The compiler stack-analysis tests also cover indexed operations inside calls, conditions, and loops.
Both execution engines use `detail::StorageSlot`, which owns an optional scalar and a vector of optional elements.
Each function activation owns its slots. An executed declaration resets its slot and gives every array element no value.
The interpreter stores slots in block environments, and the VM stores slots in its explicit call frames.
The VM retains element storage until its frame ends. Each declaration resets the element values for its slot.
Static bindings prevent access after the block scope ends.

The shared slot helpers check signed indices before conversion and report bounds or missing-value errors at the indexed name.
The interpreter saves an integer offset before the assignment value and finds the slot again after that value returns.
It retains no slot reference across recursive calls.
The VM keeps the evaluated index below the right operand on its value stack.
Element storage stays outside the scalar `Value` API and ordinary argument and return values.

Fourteen execution groups cover both element types, evaluation order, initialization, loop resets, scopes, calls, recursion, limits, and malformed instructions.
The differential group compares all three engines across 1920 combinations of program, step budget, and call-depth limit.
The optimizer retains indexed checks, reads, and stores as barriers, including unused reads that can fail.

The array examples exercise scalar calls, loops, boolean conditions, and missing element values.
Repeated API execution preserves both bytecode inputs and recreates element storage after successful and failed runs.
Twenty-three command cases cover the nine existing modes, including stage independence, bounds errors, and error priority.
Golden outputs preserve bracket positions, extent text, element bindings, metadata, and indexed instruction order.
The phase 11 suite contains 398 tests. All pass in normal and AddressSanitizer/UndefinedBehaviorSanitizer builds without compiler warnings on Apple Clang 17.
The prose scan reports no findings. Build commands live in [tech-stack.md](tech-stack.md#sanitizer-validation).

## LLVM study boundary

The [LLVM notes](theory-llvm.md#pipeline-map) map the implemented stages to Clang analysis, LLVM IR, passes, and native code generation.
The phase 12 laboratory compiles separate C sources with the host Clang.
It does not consume the Mini-C AST or bytecode and does not link LLVM libraries into the project.
This boundary permits an IR experiment without a second implementation of Mini-C runtime contracts.
The separate laboratory generates LLVM IR at `-O0` and `-O1`, then compiles both C and IR paths into native executables.
Its four native checks exercise five bounded cases each and preserve the existing 398 compiler tests.
The [annotated IR](theory-llvm.md#read-the-o0-ir) connects stack storage, control-flow blocks, calls, and SSA values to concrete generated instructions.

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
