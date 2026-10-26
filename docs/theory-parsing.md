---
id: theory-parsing
title: Parsing theory literacy
type: theory
status: draft
summary: First/Follow, LL vs LR, and why parser generators exist — study notes, not an LR implementation.
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

Filled during [roadmap.md](roadmap.md) phase 3. Implementation stays recursive descent; this doc is the academic vocabulary layer.

## Goals

After this phase you should be able to:

- Compute First and Follow sets for a small grammar by hand
- Say when a grammar is (or is not) LL(1)
- Explain shift-reduce parsing and name the LR family at a high level
- Argue when a hand-written parser beats a generator (and the reverse)

## Non-goals

- Implementing LR/LALR tables in `minic`
- Shipping a flex/bison (or ANTLR) front end

## Why this exists

Stanford CS143 and Harvard CS153 both teach top-down **and** bottom-up parsing in lecture (often with generators in the project). We keep hand-written recursive descent for the implementation and use this doc for the shared academic vocabulary. See [curriculum-alignment.md](curriculum-alignment.md).

## Outline (to complete)

### First and Follow

- [ ] Definitions and worked example
- [ ] How Follow feeds predictive parsing decisions

### LL(1) and recursive descent

- [ ] Left recursion / common prefixes vs our hand-written grammar shape
- [ ] Relationship between LL(1) tables and recursive-descent functions

### Bottom-up family

- [ ] Items, shift, reduce, accept, conflict
- [ ] LR(0) vs SLR vs LALR vs LR(1) in one comparison table
- [ ] Why courses teach this even when projects use recursive descent

### Generator trade-offs

- [ ] Bison / ANTLR / similar: when they win
- [ ] Why this project still chooses hand-written parsing

## Interview-ready blurb

_Write 5–8 sentences here at the end of phase 3._
