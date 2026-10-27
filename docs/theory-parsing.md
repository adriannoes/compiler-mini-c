---
id: theory-parsing
title: Parsing theory literacy
type: theory
status: active
summary: Worked FIRST/FOLLOW sets, LL decisions, LR parsing, and parser-generator trade-offs.
related:
  - id: docs-index
    path: README.md
  - id: roadmap
    path: roadmap.md
  - id: curriculum-alignment
    path: curriculum-alignment.md
  - id: architecture
    path: architecture.md
  - id: language
    path: language.md
---

# Parsing theory literacy

Phase 3 connects parsing theory to the existing Mini-C parser.
The worked examples cover predictive parsing and bottom-up parsing without adding another parser to the compiler.
The implementation remains manual recursive descent, with its syntax contract in [language.md](language.md#phase-2-grammar).
Name resolution, Mini-C type analysis, and execution remain in later [roadmap phases](roadmap.md).

## Notation

A grammar describes valid sequences of tokens.
A production is a rule for expanding a nonterminal.
A nonterminal names a grammar rule.
A terminal is a token category or fixed spelling.
A derivation applies productions to generate a token sequence.

| Symbol | Meaning |
|--------|---------|
| `A -> alpha` | Expand nonterminal `A` to the symbol sequence `alpha` |
| `a \| b` | Choose either alternative |
| `epsilon` | The empty token sequence, with no input consumed |
| `$` | End of input, represented by the lexer's `EOF` token |
| `id` | One identifier token, such as `a`, `b`, or `c` |
| `E_tail`, `T_tail` | Nonterminals for repeated operators and operands |

Lookahead is an input token examined before consumption.
A nullable nonterminal can derive the empty sequence.
FIRST contains the possible first terminals of a derivation.
FOLLOW contains terminals possible immediately after a nonterminal's derivation.
FOLLOW also contains `$` when that derivation can end the input.

These notes keep nullability separate, so FIRST contains terminals only.
Some textbooks put `epsilon` inside FIRST instead. Both conventions carry the same information.
FOLLOW never contains `epsilon`.
The convention here follows [Cornell's top-down parsing notes](https://www.cs.cornell.edu/courses/cs4120/2023sp/notes/topdown/).

## FIRST and FOLLOW by hand

The study grammar below supports identifiers, parentheses, addition, and multiplication.
It is an expression subset for calculations, rather than the complete Mini-C grammar.
Its start symbol is `E`.

```text
E      -> T E_tail
E_tail -> + T E_tail | epsilon
T      -> F T_tail
T_tail -> * F T_tail | epsilon
F      -> id | ( E )
```

Only `E_tail` and `T_tail` are nullable because each has an empty alternative.
`F` starts with `id` or `(`, and both `T` and `E` start through `F`.
The nonempty alternatives of `E_tail` and `T_tail` start with `+` and `*`, respectively.

The FOLLOW calculation uses these steps:

1. Add `$` to `FOLLOW(E)` because `E` is the start symbol.
2. Add `)` to `FOLLOW(E)` from `F -> ( E )`.
3. Add `+` to `FOLLOW(T)` from `E -> T E_tail`.
4. Because `E_tail` is nullable, add `FOLLOW(E)` to `FOLLOW(T)`.
5. Add `FOLLOW(E)` to `FOLLOW(E_tail)` because `E_tail` ends that production.
6. Add `*` to `FOLLOW(F)` from `T -> F T_tail`.
7. Because `T_tail` is nullable, add `FOLLOW(T)` to `FOLLOW(F)`.
8. Add `FOLLOW(T)` to `FOLLOW(T_tail)` because `T_tail` ends that production.
9. Repeat propagation until no set changes.

The recursive tail productions add no terminals beyond these sets:

| Nonterminal | Nullable | FIRST | FOLLOW |
|-------------|----------|-------|--------|
| `E` | No | `{id, (}` | `{), $}` |
| `E_tail` | Yes | `{+}` | `{), $}` |
| `T` | No | `{id, (}` | `{+, ), $}` |
| `T_tail` | Yes | `{*}` | `{+, ), $}` |
| `F` | No | `{id, (}` | `{*, +, ), $}` |

For example, `+` follows a complete `T`, but it cannot start one.
The closing parenthesis propagates from `E` through nullable tails to `T` and `F`.
The `$` marker follows the same path.

## Predictive decisions and LL(1)

LL(1) expands the leftmost nonterminal with one lookahead token.
The first `L` means input consumption from left to right.
The second `L` means a leftmost derivation.

For each production `A -> alpha`, construct the predictive table as follows:

1. Put the production in each column named by `FIRST(alpha)`.
2. If `alpha` is nullable, also put the production in each column named by `FOLLOW(A)`.
3. Mark every empty cell as an error.

A grammar is LL(1) when no cell has competing productions.
These rules follow [Cornell's predictive table construction](https://www.cs.cornell.edu/courses/cs4120/2023sp/notes/topdown/#constructing-the-ppt).
The table for the study grammar is:

| Nonterminal | `id` | `(` | `+` | `*` | `)` | `$` |
|-------------|------|-----|-----|-----|-----|-----|
| `E` | `T E_tail` | `T E_tail` | error | error | error | error |
| `E_tail` | error | error | `+ T E_tail` | error | `epsilon` | `epsilon` |
| `T` | `F T_tail` | `F T_tail` | error | error | error | error |
| `T_tail` | error | error | `epsilon` | `* F T_tail` | `epsilon` | `epsilon` |
| `F` | `id` | `( E )` | error | error | error | error |

Every populated cell contains one production, so this expression grammar is LL(1).
For `id + id`, the first `T_tail` chooses `epsilon` because `+` belongs to `FOLLOW(T_tail)`.
The enclosing `E_tail` then consumes `+` and another term.
For `id + * id`, the cell for `T` and `*` is an error.
This example cannot skip the missing operand through an empty alternative.

## Connections to recursive descent

The [parser source](../src/parser.cpp) uses functions and loops instead of a predictive table.
Its expression levels implement the same operator grouping demonstrated by the study grammar.

| Grammar concept | Mini-C implementation | Result |
|-----------------|-----------------------|--------|
| Operator precedence | `additive()` calls `multiplicative()` | Multiplication becomes a child of addition |
| Repeated binary operations | `binary()` replaces its accumulated left child | `a - b - c` groups as `(a - b) - c` |
| Recursive assignment | `assignment()` parses its value with another `assignment()` call | `a = b = c` groups as `a = (b = c)` |
| Unary recursion | `unary()` parses another unary operand | `!-a` groups as `!(-a)` |
| Shared prefix | `primary()` consumes a name before it looks for `(` | A name and a call share the initial identifier |

### Left recursion and associativity

Left recursion lets a nonterminal derive itself before consuming input.
A direct translation of `E -> E + T | T` calls `E` again before it consumes any token.
That translation does not make progress in ordinary recursive descent.
The study grammar moves repetition into `E_tail`, and `binary()` implements repetition with a loop.
Its accumulated left child preserves left associativity, even though the tail grammar recurses on the right.

### Common prefixes

Left factoring delays a decision until a shared prefix ends.
The following schematic rules describe a name or a named call:

```text
Before:
P -> id | id ( arguments )

After:
P -> id C
C -> ( arguments ) | epsilon
```

Both original alternatives start with `id`, so that token alone does not select one.
After consuming `id`, `primary()` inspects the next token for `(`.
The full argument syntax, including an empty list, lives in [language.md](language.md#phase-2-grammar).

### The nearest-if rule

The LL(1) result above applies to the study expression grammar.
The complete statement grammar also needs the explicit nearest-`if` rule for `else`.
Consider this reduced statement grammar, where `other` represents a complete simple statement:

```text
S -> if ( id ) S O | other
O -> else S | epsilon
```

`O` is nullable and `FIRST(O)` contains `else`.
In the conditional production, `O` follows the first `S`, so `FOLLOW(S)` includes `else`.
`O` ends that production, so `FOLLOW(O)` also includes `else`.
The table cell for `O` and `else` therefore contains both alternatives.
Factoring the prefix does not remove this conflict.

For example, the grammar alone gives two possible attachments for this source:

```c
if (a) if (b) return 1; else return 2;
```

The project resolves the attachment through its grammar contract and parser control flow.
`statement()` finishes the inner statement before the outer `if` searches for its optional `else`.
The inner `if` therefore consumes `else` first.
In `if (a) { if (b) return 1; } else return 2;`, the braces attach `else` to the outer `if`.
The `parser.dangling_else` test covers both results.

An LR parser sees the analogous shift/reduce conflict at `else`.
Shifting `else` attaches it to the inner conditional, while reducing the shorter conditional first attaches it to the outer one.
See [Bison's shift/reduce explanation](https://www.gnu.org/software/bison/manual/html_node/Shift_002fReduce.html).

## Bottom-up parsing

A bottom-up parser combines recognized phrases into larger grammar symbols.
Its reductions reconstruct a rightmost derivation in reverse.
A rightmost derivation expands the rightmost remaining nonterminal at each step.

| Term | Meaning |
|------|---------|
| Shift | Consume one input token and push it onto the stack |
| Reduce | Replace a production's right side with its left-side nonterminal |
| Accept | Finish when the start production completes at end of input |
| Error | Reject input when the current state has no action |
| Handle | Production occurrence that reverses one rightmost derivation step |
| LR item | A production with a dot that records progress |
| LR state | A set of LR items used to select actions |

Each reduction replaces the production's matching symbols at the stack top.
For example, `E -> E + . T` records progress past `E +` within that production.
An LR(1) item also records reduction lookahead, as in `[E -> E + . T, $]`.
The next input token still starts `T`.
The `$` describes a permitted context after the complete production.
See [Cornell's LR(1) item explanation](https://www.cs.cornell.edu/courses/cs4120/2023sp/notes/lr/#lr1-construction).

The `L` in LR means input consumption from left to right.
The `R` means reconstruction of a rightmost derivation in reverse.
A full LR parser stores states alongside grammar symbols and uses action and transition tables.
The trace below omits those state numbers to focus on shifts and reductions.
The underlying construction appears in [Cornell's bottom-up parsing lecture](https://www.cs.cornell.edu/courses/cs4120/2013fa/lectures/lec06-fa13.pdf).

### Worked shift/reduce trace

This left-recursive grammar accepts the same expression subset as the earlier factored grammar:

```text
Start -> E
E     -> E + T | T
T     -> T * F | F
F     -> id | ( E )
```

The input is `id + id * id $`.
Each row shows the stack and remaining input before its action.
The stack top is on the right.
Only shifts consume input.

| Step | Stack | Remaining input | Action |
|-----:|-------|-----------------|--------|
| 0 | empty | `id + id * id $` | Shift `id` |
| 1 | `id` | `+ id * id $` | Reduce `F -> id` |
| 2 | `F` | `+ id * id $` | Reduce `T -> F` |
| 3 | `T` | `+ id * id $` | Reduce `E -> T` |
| 4 | `E` | `+ id * id $` | Shift `+` |
| 5 | `E +` | `id * id $` | Shift `id` |
| 6 | `E + id` | `* id $` | Reduce `F -> id` |
| 7 | `E + F` | `* id $` | Reduce `T -> F` |
| 8 | `E + T` | `* id $` | Shift `*` |
| 9 | `E + T *` | `id $` | Shift `id` |
| 10 | `E + T * id` | `$` | Reduce `F -> id` |
| 11 | `E + T * F` | `$` | Reduce `T -> T * F` |
| 12 | `E + T` | `$` | Reduce `E -> E + T` |
| 13 | `E` | `$` | Accept |

At step 8, shifting `*` permits completion of `T -> T * F`.
The reduction of `E -> E + T` waits until that term is complete.
Both the recursive-descent parser and this trace group the expression as `id + (id * id)`.
The grammar layers establish that grouping without an unconditional preference for shifts.

FOLLOW sets describe a particular grammar.
Here, `T -> T * F` adds `*` to `FOLLOW(T)`, unlike the earlier factored grammar.

### LR family comparison

The variants differ in how they determine reductions and retain context.
SLR means Simple LR, and LALR means Look-Ahead LR.

| Construction | State information | Reduction decision | Cost or limitation |
|--------------|-------------------|--------------------|--------------------|
| LR(0) | Items without reduction lookahead | An ordinary completed item reduces regardless of the next token | Some useful grammars produce conflicts |
| SLR(1) | LR(0) states plus FOLLOW sets | Reduce `A -> alpha` only on `FOLLOW(A)` | Global FOLLOW sets combine different contexts |
| LALR(1) | Merge LR(1) states with the same LR(0) core | Use the combined item lookahead sets | Merging keeps tables compact but can introduce conflicts |
| Canonical LR(1) | Keep distinct LR(1) item sets | Use the completed item's context-specific lookahead | Additional states can increase table size |

An LR(0) core removes lookahead annotations from an LR(1) item set.
The SLR restriction follows [Cornell's bottom-up lecture](https://www.cs.cornell.edu/courses/cs4120/2013fa/lectures/lec06-fa13.pdf).
The LALR merge and its limitations appear in [Cornell's LR/LALR notes](https://www.cs.cornell.edu/courses/cs4120/2023sp/notes/lr/#lalr-grammars).
The `0` in LR(0) describes reduction lookahead, while shifts still distinguish input token kinds.

Table construction and parser implementation style are separate choices.
Bison provides LALR, IELR, and canonical LR tables. Its default construction uses LALR.
See [Bison's table construction documentation](https://www.gnu.org/software/bison/manual/bison.html#LR-Table-Construction).
This project does not construct or execute any of those tables.

### Conflicts

A shift/reduce conflict offers both a shift and a reduction in the same state for the same lookahead.
The conditional example above demonstrates this at `else`.
An ambiguous expression grammar, such as `E -> E + E | E * E | id`, also permits competing groupings.
A grammar transformation or an explicit precedence policy can select the intended grouping.

A reduce/reduce conflict offers competing reductions for the same state and lookahead.
For example, `S -> A | B`, `A -> id`, and `B -> id` permit two reductions after reading `id`.
Each reduction reaches `S`, through a different derivation.
Choosing the first rule hides the competing interpretations and can change AST construction.
See [Bison's reduce/reduce discussion](https://www.gnu.org/software/bison/manual/html_node/Reduce_002fReduce.html).

A conflict alone does not prove that the original grammar is ambiguous.
For example, merging LALR states can introduce conflicts that canonical LR(1) avoids.
Two distinct derivations of the same complete input do establish ambiguity.
Studying LR makes these conflict reports readable even when the project uses recursive descent.

## Generator trade-offs

A parser generator produces parser code from a grammar specification.
Bison offers LR-based parsing. ANTLR 4 generates recursive-descent parsers with adaptive LL prediction, called ALL(*).
ANTLR 4 also rewrites supported direct left recursion before it generates the parser.
The prediction model and rewriting appear in the authors' [ALL(*) technical report, sections 1 and 2](https://www.antlr.org/papers/allstar-techreport.pdf).

| Choice | When it fits | Cost or risk | What it enables |
|--------|--------------|--------------|-----------------|
| Manual recursive descent | A small language with direct AST construction and custom diagnostics | The author maintains each grammar decision and its tests | Grammar rules remain visible as C++ functions |
| Bison | A grammar that fits LR parsing and benefits from conflict reports | Generated code and grammar integration add build and debugging work | Central grammar rules with explicit precedence and LR construction choices |
| ANTLR 4 | A shared grammar that benefits from adaptive top-down prediction | Generator and runtime integration add dependencies and prediction behavior to learn | Generated recursive-descent parsing with grammar-driven prediction |

These are project selection criteria, rather than performance measurements.
Mini-C keeps manual recursive descent because it exposes grammar decisions and AST ownership within the existing C++ code.
That choice avoids a generator integration task and keeps the learning work focused on each compiler stage.
The author must maintain precedence, ambiguity policies, and syntax tests.
The phase 3 examples require no new dependency or production parser code.

## Reproduce the parser connection

After the [normal build](tech-stack.md#build-and-test), save this source as `build/parsing-study.mc` from the repository root:

```c
int study() { return a + b * c; }
```

Operate the existing AST command:

```bash
./build/minic --ast build/parsing-study.mc
```

The complete expected output is:

```text
Program
  Function "study" @1:5
    Type INT @1:1
    Block @1:13
      Return @1:15
        Binary PLUS @1:24
          Name "a" @1:22
          Binary STAR @1:28
            Name "b" @1:26
            Name "c" @1:30
```

The multiplication is the right child of addition, as in the worked trace.
The undeclared names remain valid syntax because name resolution belongs to a later phase.
The source file stays in the ignored build directory.

## Study review

Use these prompts to repeat the calculations without copying the tables.

| Prompt | Expected result |
|--------|-----------------|
| Which study nonterminals are nullable? | Only `E_tail` and `T_tail` |
| Why does `)` belong to `FOLLOW(F)`? | It propagates from `FOLLOW(E)` through nullable tails |
| What happens for `id + * id`? | The predictive cell for `T` and `*` reports an error |
| Does common-prefix factoring settle the optional `else`? | No. `else` appears in both the nonempty alternative's FIRST set and the optional rule's FOLLOW set |
| Can an LALR conflict disappear with canonical LR(1)? | Yes, when merging states caused the conflict |
| Does the AST sample prove that its names exist? | No. The parser records name syntax without name resolution |

## Interview-ready blurb

I built the Mini-C parser by hand with recursive descent.
FIRST describes possible starting tokens, and FOLLOW helps determine when a nullable rule can end.
I calculated an LL(1) expression grammar and connected its decisions to the parsing functions.
The statement grammar also uses an explicit rule that attaches `else` to the nearest unmatched `if`.

LR parsers use shifts and reductions to reconstruct a rightmost derivation in reverse.
Their LR(0), SLR, LALR, and canonical LR(1) constructions differ in reduction context and table size.
I kept the manual parser to study grammar decisions, AST construction, and diagnostics within C++.
