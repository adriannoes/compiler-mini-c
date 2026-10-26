---
id: curriculum-alignment
title: Curriculum and market alignment
type: reference
status: active
summary: How this project maps to typical compilers courses (incl. MIT/Stanford/Harvard) and industry transfer skills.
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
  - id: theory-parsing
    path: theory-parsing.md
  - id: theory-llvm
    path: theory-llvm.md
---

# Curriculum and market alignment

Research snapshot used to finalize the roadmap. Sources: Brazilian syllabi (e.g. UTFPR, USP, UFPel, UFV), Dragon Book–style undergrad courses, plus MIT / Stanford / Harvard offerings below. Market notes come from toolchain job descriptions (LLVM/Clang/MLIR) and essays on transferable compiler skills (DSLs, parsers, language tooling).

## Learning goals for this repo

| Goal | Priority |
|------|----------|
| End-to-end Mini-C compiler → bytecode VM | Primary (portfolio artifact) |
| College front-end + runtime literacy | Primary |
| Bottom-up / First/Follow theory | Required literacy (notes, not LR implementation) |
| Local optimizations | Required after the VM |
| LLVM vocabulary + light hands-on | Required literacy (discuss fluently) |
| Specialist compiler-engineer depth | Out of scope |

## MIT, Stanford, Harvard

### Stanford — [CS143 Compilers](https://web.stanford.edu/class/cs143/)

Classic undergrad compilers course. Students build a compiler for **COOL** (Classroom Object-Oriented Language), historically toward **MIPS/SPIM**. Lectures explicitly cover both **top-down and bottom-up parsing**, types/scopes, runtime, codegen, then **local + global optimization**, **register allocation**, **GC**, and a look at **JIT**.

Programming spine (typical): lexer → parser → semantic analysis / types → codegen. Flex/Bison (or Java equivalents) are part of the ecosystem; theory still includes First/Follow and LR. Optimization is taught seriously in lecture; the main project historically has *less* optimization than a follow-on course (e.g. CS243).

**Overlap with us:** phases 1–5 and codegen/runtime match well; our phase 3 theory matches their parsing lectures; our phase 10 is a lighter stand-in for their opt lectures.

**Deeper than us (by design):** full bottom-up implementation via generators, native assembly target, register allocation, GC, JIT as course topics.

### MIT — [6.110 / 6.035 Computer Language Engineering](https://6110-sp25.github.io/syllabus)

Project-heavy course: build a compiler for **Decaf** almost from scratch (parser generators allowed). Spring 2025 phase weights show the emphasis:

1. Lexing and parsing
2. IR + semantic checking
3. Code generation (**x86-64**)
4. **Dataflow** optimization framework
5. Register allocation and further opts (large final weight) + “compiler derby”

MIT pushes hard on **engineering a real native backend** and **optimization quality**, including optional SSA. Front end is necessary but not the grade climax.

**Overlap with us:** IR + semantics + “get something executable” parallels stages A/B; local opts acknowledge their opt story at small scale.

**Deeper than us (by design):** x86-64 codegen, dataflow framework, register allocation, performance competition. We stop at a correct bytecode VM + local opts + LLVM literacy instead of competing on native speed.

### Harvard — [CS153 Compilers](https://groups.seas.harvard.edu/courses/cs153/2019fa/)

Programming-intensive (OCaml). Notable shape: **assembly and IR / LLVM appear early**, then lexing, **recursive + LL + LR parsing**, closures, type checking / subtyping, objects, optimization, **dataflow**, **register allocation**, loop opts, GC.

Projects climb a ladder (e.g. x86lite → LLVMlite → OAT language versions → dataflow/regalloc). Stated outcome includes understanding **capabilities and limits of modern compilers** for debugging and optimization — not only “become a language implementor.”

**Overlap with us:** recursive-descent path, LR as theory, and especially **LLVM as a first-class teaching IR** strongly validates our phase 12 literacy track.

**Deeper than us (by design):** OCaml stack, LLVM-based lowering as core labs, closures/subtyping/objects, full dataflow + regalloc.

### Cross-school snapshot vs this plan

| Topic | Stanford CS143 | MIT 6.110 | Harvard CS153 | This project |
|-------|----------------|-----------|---------------|--------------|
| Lexer / parser project | Yes (often with generators) | Yes | Yes | Hand-written RD |
| Bottom-up / LR theory | Yes (lectures + tools) | Yes (as needed) | Yes (LL + LR lectures) | Phase 3 notes |
| Types / semantics | Yes | Yes (with IR) | Yes (+ subtyping/objects) | Phases 4–5 |
| Executable target | MIPS/asm (COOL) | x86-64 | Asm + LLVM IR ladder | Bytecode VM |
| Optimizations | Lectures; light in main PA | Heavy (dataflow + derby) | Dataflow + regalloc labs | Local opts (phase 10) |
| LLVM | Not the center | Not required | **Central** | Literacy (phase 12) |
| GC / JIT | Lectures | Variable | GC lecture | Optional reading only |

**Takeaway:** elite US undergrad courses agree on the front-end spine. They diverge on **how hard the backend/opt story is**. Harvard is the closest cultural match for “know LLVM enough to talk”; MIT is the furthest (native + dataflow as the point); Stanford sits in between with a complete classic pipeline and richer lecture topics (GC, JIT) we can skim as optional notes.

## Typical undergrad coverage vs this plan

| Topic | Typical course | This project |
|-------|----------------|--------------|
| Lexical analysis | Core | Phase 1 (implement) |
| Top-down parsing / recursive descent | Core | Phase 2 (implement) |
| Bottom-up / LR family, First/Follow | Core (often with generators) | Phase 3 (theory notes) |
| Symbol tables / scopes | Core | Phase 4 |
| Types / semantic analysis | Core | Phase 5 |
| Runtime / activation records | Core | Phases 6–9 |
| Intermediate code | Core | Phases 8–9 (bytecode) |
| Optimization / data-flow | Intro, or heavy (MIT/Harvard) | Phase 10 local opts; data-flow optional notes |
| Native codegen / register allocation | Common at MIT/Stanford/Harvard | Not required; peek via LLVM literacy |
| LLVM as teaching IR | Strong at Harvard; rarer elsewhere | Phase 12 literacy |

## Market: two different uses of this knowledge

**Specialist toolchain roles** want deep LLVM/IR/SSA, C++ systems, architecture, often open-source track record. This repo is foundation only.

**General software engineering** benefits most from parsing, ASTs, types, diagnostics, interpreters/VMs, and DSL instincts — exactly what stages A/B exercise. That is the career lens for this project. Harvard’s syllabus wording (use compilers better for debugging/optimization) matches this lens.

## Design choices that follow from the research

1. Implement recursive descent; study LR so syllabus vocabulary matches Stanford/Harvard lectures.
2. Prefer a finished VM + tests over an incomplete native backend (unlike MIT’s x86 climax).
3. Add local optimizations so “optimization” is not an empty checkbox relative to those courses.
4. Treat LLVM as readable literacy (Harvard-shaped), not a MIT-style optimization career track.
5. Optional later reading only: GC, JIT, SSA — common lecture topics we do not implement.

## Primary references

- Stanford CS143: https://web.stanford.edu/class/cs143/
- MIT 6.110 (Spring 2025): https://6110-sp25.github.io/syllabus and project pages
- Harvard CS153 (Fall 2019 schedule): https://groups.seas.harvard.edu/courses/cs153/2019fa/schedule.html
- MIT OCW 6.035 historical project segments: scanner/parser → semantics → codegen → dataflow → low-level opts
