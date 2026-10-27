---
id: docs-index
title: Documentation index
type: index
status: active
summary: Catalog of project docs and how front matter cross-references work.
related:
  - id: readme
    path: ../README.md
  - id: roadmap
    path: roadmap.md
  - id: tech-stack
    path: tech-stack.md
  - id: language
    path: language.md
  - id: architecture
    path: architecture.md
  - id: curriculum-alignment
    path: curriculum-alignment.md
  - id: theory-parsing
    path: theory-parsing.md
  - id: theory-llvm
    path: theory-llvm.md
  - id: references
    path: references.md
  - id: publication
    path: publication.md
---

# Documentation

All in-repo documentation is English. Each doc starts with YAML front matter so contexts stay separate but linked.

## Front matter

| Field | Purpose |
|-------|---------|
| `id` | Stable document id (use this in `related`) |
| `title` | Human-readable title |
| `type` | `overview` \| `index` \| `plan` \| `reference` \| `design` \| `theory` |
| `status` | `active` \| `draft` \| `done` |
| `summary` | One-line purpose |
| `related` | List of `{ id, path }` links to sibling docs |

When you add a doc, give it an `id`, list it here, and add it under `related` on the docs it touches.

## Catalog

| id | Path | Type |
|----|------|------|
| `readme` | [../README.md](../README.md) | overview |
| `docs-index` | [README.md](README.md) | index |
| `roadmap` | [roadmap.md](roadmap.md) | plan |
| `references` | [references.md](references.md) | reference |
| `tech-stack` | [tech-stack.md](tech-stack.md) | reference |
| `language` | [language.md](language.md) | design |
| `architecture` | [architecture.md](architecture.md) | design |
| `curriculum-alignment` | [curriculum-alignment.md](curriculum-alignment.md) | reference |
| `theory-parsing` | [theory-parsing.md](theory-parsing.md) | theory |
| `theory-llvm` | [theory-llvm.md](theory-llvm.md) | theory |
| `publication` | [publication.md](publication.md) | plan |
