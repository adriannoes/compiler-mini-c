---
id: theory-llvm
title: LLVM literacy
type: theory
status: draft
summary: Enough LLVM/IR/toolchain vocabulary to discuss compilers confidently — not a specialist career track.
related:
  - id: docs-index
    path: README.md
  - id: roadmap
    path: roadmap.md
  - id: curriculum-alignment
    path: curriculum-alignment.md
  - id: architecture
    path: architecture.md
  - id: tech-stack
    path: tech-stack.md
---

# LLVM literacy

Filled during [roadmap.md](roadmap.md) phase 12. Purpose: conversation fluency and mental model — not training for LLVM/Clang engineer roles.

## Goals

After this phase you should be able to:

- Map `minic` stages onto Clang/LLVM vocabulary (front end, IR, passes, codegen)
- Read a small function’s LLVM IR and identify locals, branches, and calls
- Explain what `opt` and `llc` roughly do
- Discuss SSA and passes at a conceptual level

## Non-goals

- Owning a production target backend
- Deep MLIR / TableGen expertise
- Competing with specialist toolchain resumes
- Matching Harvard CS153’s full LLVM lab ladder or MIT’s x86/dataflow project depth

## Why this exists

Harvard CS153 teaches IR/LLVM early as a practical compilation target. MIT 6.110 instead climaxes on native codegen + dataflow. Our phase 12 follows the Harvard *literacy* idea without adopting either school’s full lab load. See [curriculum-alignment.md](curriculum-alignment.md).

## Outline (to complete)

### Mental model

- [ ] Clang as C/C++ front end; LLVM as IR + pass pipeline + backends
- [ ] How our tree-walk / bytecode stages relate to “IR + interpreter/VM”

### Hands-on (light)

- [ ] `clang -S -emit-llvm` on a tiny C file; annotate the `.ll`
- [ ] Optional: run `opt` with a simple pass and diff IR
- [ ] Optional: emit a tiny Mini-C subset to LLVM IR **or** only study IR from C

### Vocabulary checklist

- [ ] Module, function, basic block, instruction
- [ ] SSA intuition (why versions of values appear)
- [ ] Pass / pass manager (what “an optimization pass” means)
- [ ] Codegen / target (why IR is portable)

## Interview-ready blurb

_Write a short explanation of LLVM as if in a design interview. Emphasize literacy, not specialist claims._
