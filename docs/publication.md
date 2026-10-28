---
id: publication
title: Portfolio publication
type: plan
status: active
summary: Approved source publication, local preparation evidence, and GitHub publication commands.
related:
  - id: docs-index
    path: README.md
  - id: readme
    path: ../README.md
  - id: roadmap
    path: roadmap.md
  - id: curriculum-alignment
    path: curriculum-alignment.md
  - id: tech-stack
    path: tech-stack.md
---

# Portfolio publication

The compiler roadmap reached completion on local `main` at `170dbb5`.
This document records the preparation for the first public GitHub publication.
On October 28, 2026, Adrianno approved the documentation commit, integration into `main`, public repository creation, and first push.
Each later commit requires his authorization, and each later push requires separate confirmation.

## Approved repository

| Field | Choice |
|-------|----------|
| Owner | `adriannoes`, from an authenticated GitHub API query |
| Name | `compiler-mini-c` |
| Visibility | Public, approved by Adrianno |
| Published branch | `main` |
| Git transport | HTTPS, from the current GitHub CLI configuration |
| License | [MIT](../LICENSE), selected by Adrianno for the educational project |
| Source of presentation | The root [README](../README.md), with linked examples and technical notes |

Repository description:

```text
Educational Mini-C compiler in C++ with a bytecode VM, local optimizations, checked arrays, and parsing/LLVM study notes.
```

Suggested topics are `compiler`, `compiler-design`, `cplusplus`, `bytecode`, `virtual-machine`, `parsing`, and `education`.
Topics are suggestions, rather than configured repository metadata.

## Publication scope

Adrianno selected a source publication without continuous integration (CI), which runs builds and tests automatically.
This study project retains its local CMake, CTest, and sanitizer commands.
The publication adds no GitHub Actions workflow.

The October 28 review found no reproducible functional defect in the tested compiler behavior.
It identified a parsing-theory correction, reference prose edits, and an explicit C++17 prerequisite for the README.
The theory correction distinguishes multiple parse trees from different expansion orders within one tree.

## Checked evidence

These observations describe the local baseline and read-only queries from October 27, 2026:

| Surface | Observed result |
|---------|-----------------|
| Integration | All required phases merged into local `main` at `170dbb5`, with a clean tree before publication preparation |
| Build and tests | The normal build compiles without warnings, and all 398 CTest cases pass |
| Examples | All nine modes checked on 17 Mini-C examples, for 153 command results |
| Execution agreement | Interpreter, VM, and optimized VM agree on output, diagnostics, and status for every example |
| LLVM study | Four native executions pass five bounded cases each and print `10` |
| Sanitizers | The [phase 11 baseline](tech-stack.md#sanitizer-validation) passes 398 cases. Compiler sources and targets retain that content |
| Documentation | Local links, heading anchors, front-matter identities, and the prose scan pass |
| Tracked content | 109 files, with no tracked file larger than 1 MB |
| Local artifacts | Build directories and generated binaries remain ignored |
| History authors | One recorded identity: `Adrianno Esnarriaga <hey@adrianno.es>` |
| Content review | The tracked baseline contains no Pipefy references or local user-directory paths |
| Limited credential scan | No matches in 373 reachable historical blobs for GitHub tokens, AWS access keys, private keys, or quoted credential assignments |
| Remote state | No local remote. The authenticated target lookup returns HTTP 404 |
| CI and releases | No GitHub Actions workflow or release configured in this local project |

The limited pattern scan does not establish the absence of all sensitive content.
The October 27 review authenticated as `adriannoes` through the API.
That review did not test Git transport authentication or repository creation permissions.
HTTP 404 does not reserve the proposed name. Repeat the target query before creation.

The README explains build commands, execution modes, example results, and accepted limits.
GitHub recognizes root README files and supports the project's relative links, as described in its [README documentation](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-readmes).
The October 27 review did not inspect rendered GitHub pages because it preceded publication.

## Portfolio introduction

I built an educational Mini-C compiler in C++ with a manual front end, interpreter, bytecode VM, local optimizations, and checked arrays.
The repository includes 398 automated tests, 17 Mini-C examples, and worked parsing and LLVM study notes.
The LLVM work uses a separate C laboratory. The Mini-C execution target remains the custom bytecode VM.
The [coverage document](curriculum-alignment.md) links implemented features, completed study exercises, and excluded capabilities to evidence.

## License and local preparation

Adrianno selected the MIT license and approved a local commit of the publication preparation.
The [LICENSE](../LICENSE) contains the standard text with `Copyright (c) 2026 Adrianno Esnarriaga`.
The [MIT reference](https://choosealicense.com/licenses/mit/) provides the license text.
The README links to the license, and the presentation names the implementation language as C++.

The preparation branch is `docs/publication-preparation`.
It contains the license, publication plan, documentation index entry, and language-name corrections.
After the local commit and explicit integration approval, inspect and merge the branch from the repository root:

```bash
git diff --check
git status --short --branch
git switch main
git merge --ff-only docs/publication-preparation
```

## GitHub publication commands

After local integration, inspect the state before requesting approval for remote changes:

```bash
git status --short --branch
git remote -v
gh api user --jq .login
gh repo view adriannoes/compiler-mini-c --json nameWithOwner,url,visibility,defaultBranchRef
```

The October 27 target query returned HTTP 404.
If a repository now appears, stop and assess its contents before proceeding.
After Adrianno explicitly approves public repository creation, use the existing local source:

```bash
gh repo create adriannoes/compiler-mini-c \
  --public \
  --source=. \
  --remote=origin \
  --description 'Educational Mini-C compiler in C++ with a bytecode VM, local optimizations, checked arrays, and parsing/LLVM study notes.'
```

This command creates the repository and configures `origin`. It omits the CLI's push flag.
The [GitHub CLI manual](https://cli.github.com/manual/gh_repo_create) documents those controls.
After Adrianno gives explicit approval for the push, publish only `main`:

```bash
git push --set-upstream origin main
```

## Publication read-back

After a successful push, inspect GitHub independently:

```bash
gh repo view adriannoes/compiler-mini-c --json nameWithOwner,url,visibility,defaultBranchRef,licenseInfo
git rev-parse main
gh api repos/adriannoes/compiler-mini-c/git/ref/heads/main --jq .object.sha
```

Compare the local and remote commit identifiers.
Make sure that visibility is public and that the default branch is `main`.
If the observed metadata differs, obtain approval for the specific correction before editing GitHub settings.
Open the published README and inspect its tables, relative links, and linked Mermaid diagrams.
Record the actual repository URL and observed license after those checks.
