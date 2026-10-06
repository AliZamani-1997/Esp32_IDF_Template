---
name: git-commit
description: "Load this whenever you are about to commit in this repository — after finishing any piece of work, or when asked to commit, split commits, write a commit message, or review staged changes. It defines the topic-splitting rule (one commit per subject, never one giant commit), the conventional-commit format and allowed scopes, the pre-commit build gate, and the git safety rules. Trigger words: commit, git, stage, diff, message, changelog, split, 提交, کامیت."
agent_created: true
---

# Committing

The full standard is `docs/CODING_STANDARDS.md` §10. This skill is the workflow.

## The one rule that matters most

> **One commit per subject.** If the working tree contains changes that belong to
> several different subjects, split them into several commits. Never land one
> giant "misc changes" commit.

A *subject* is a single coherent concern: one component, one layer, one document,
one tooling change. If you cannot describe the change in one line without using
the word "and", it is two commits.

**Ask of every file: which subject does this belong to?** Then stage and commit
one subject at a time. `git add <paths>` per group — not `git add -A`.

---

## 1. Before you commit anything

```bash
git status --short                 # what is actually dirty?
git diff --stat                    # how big is it?
git diff <path>                    # read the real change, not your memory of it
```

- **The build must be green.** If you touched C++, CMake or a component:
  ```bash
  tools/idf.cmd build            # must exit 0 with no new warnings
  ```
  Docs-only, skill-only and `.gitignore`-only changes do not need a build.
- **Read the diff before you write the message.** A message that describes what
  you *intended* rather than what the diff *does* is worse than no message.

---

## 2. Splitting a dirty tree by subject

Work through the tree and assign every path to exactly one subject. Typical
subjects in this repo:

| Subject | Paths |
|---|---|
| a layer's code | `components/core/**`, `components/rtos/**`, `components/drivers/**` |
| the application | `main/**` |
| the build | `CMakeLists.txt`, `**/CMakeLists.txt`, `sdkconfig.defaults` |
| a document | `docs/<DOC>.md` |
| a project skill | `.workbuddy-ai/skills/<name>/**` |
| editor config | `.vscode/**` |
| ignore rules | `.gitignore` |

Then commit them one at a time:

```bash
git add components/core
git commit -m "feat(core): ..."

git add components/rtos
git commit -m "feat(rtos): ..."
```

**One subject per commit, even when it feels small.** A three-file documentation
change and a new component are two commits.

### Do not split too far

A single subject spread across several files is **one** commit — a new component
and its `CMakeLists.txt` and its umbrella header are one subject, not three. Split
by *concern*, not by file.

### Never commit generated or local files

Already gitignored — do not force them in: `build/`, `managed_components/`,
`sdkconfig`, `graphify-out/`, `.scratch/`, `eim_config.toml`, `.workbuddy-ai/*`
(except `.workbuddy-ai/skills/`, which **is** tracked).

When a path holds both ignored and tracked content, **dry-run first**:

```bash
git add -n .workbuddy-ai        # prints exactly what would be staged
```

`.workbuddy-ai/` is the trap: `git add -n .workbuddy-ai` must list only the
three `SKILL.md` files. If memory logs or other local files appear, the
`.gitignore` rule is wrong — fix it before staging, not after.

---

## 3. The message

```
<type>(<scope>): <summary>
```

`<summary>`: imperative, lower-case, no trailing period, ≤ 72 characters.

| type | use for |
|---|---|
| `feat` | new capability |
| `fix` | bug fix |
| `refactor` | structure changed, behaviour unchanged |
| `docs` | documentation only |
| `build` | CMake, dependencies, toolchain |
| `chore` | everything else (ignore rules, editor config, skills) |
| `test` | tests |

`<scope>` is a component or area: `core`, `rtos`, `drivers`, `app`, `docs`,
`tools`, `vscode`, `skills`.

```
feat(core): add the core layer — error model, logging and timer
feat(rtos): add the FreeRTOS wrapper layer — task, mutex, queue
feat(drivers): add the peripheral wrapper layer — gpio, uart, i2c, spi, nvs
feat(app): convert the main component to C++ and wire the three layers
build: note the C++ standard and its constraints in the top-level CMakeLists
docs: add CODING_STANDARDS.md
docs: document the four-layer architecture
chore(skills): add the coding-standards, cpp-wrapper-layers and git-commit project skills
chore(vscode): record the current ESP-IDF setup path
chore: ignore graphify output and the scratch directory
fix(core): guard ESP_TIMER_ISR behind its Kconfig option
refactor: split the peripheral wrapper component into separate layers
```

### When the subject needs explaining

Add a body after a blank line. Say **why**, and anything a reviewer cannot see
from the diff — a constraint, a rejected alternative, a gotcha.

```bash
git commit -F - <<'EOF'
refactor: split the peripheral wrapper component into separate layers

The single wrapper component mixed three concerns: foundation (error, logging,
time), RTOS abstraction, and peripheral access. Nothing stopped foundation code
from reaching for a peripheral, so the layering was only a convention.

Now each layer is its own component and `REQUIRES` points only downward, so the
build system enforces the direction the standard describes.

Cross-layer types are fully qualified (`core::Result<T>`) so the dependency is
visible at every call site. The FreeRTOS layer is named `rtos` because ESP-IDF
already owns the component name `freertos`.
EOF
```

Use a HEREDOC (`-F -`) for multi-line messages. Avoid `-m` twice — it produces
awkward blank-line handling.

---

## 4. Safety rules (non-negotiable)

- **Never** `--no-verify`. If a hook fails, fix the cause.
- **Never** `git commit --amend` on a commit that is already pushed.
- **Never** `git push --force`, `git reset --hard`, or `git checkout --` to "clean
  up" — ask first, and prefer a new commit.
- **Never** commit secrets, tokens, or machine-local paths (`eim_config.toml`).
- Prefer a **new commit** over amending an existing one.
- If a pre-commit hook reformats files, stage the reformatted result and commit
  again rather than bypassing the hook.

---

## 5. Verify after committing

```bash
git log --oneline -n <count>     # does each line describe one subject?
git status --short               # is the tree clean (or only intentionally dirty)?
git show --stat HEAD             # did the right files land in the right commit?
```

Then confirm the split was honest: read the `--oneline` list top to bottom. If
two consecutive messages could be swapped without anyone noticing, they were
probably one subject; if one message covers two unrelated areas, it was two.

**Expected noise, not errors:** `git add` prints `warning: ... LF will be replaced
by CRLF` for every text file on this machine (`core.autocrlf`). It is informational
— the committed content is correct. Do not "fix" it by disabling autocrlf or by
re-normalising the whole tree.

**Deleting a file is a subject too.** Staging a deletion (`git add main/main.c`
when the file is gone) folds the removal into the commit that replaces it — the
C++ port commit carried `main.c`'s deletion, because the two are one subject.
Never let a deletion ride along in an unrelated commit.

---

## 6. Worked example — splitting a mixed working tree

A working tree holding a whole four-layer restructure was split into ten
commits, one per subject:

```
build: note the C++ standard and its constraints in the top-level CMakeLists
feat(core): add the core layer — error model, logging and timer
feat(rtos): add the FreeRTOS wrapper layer — task, mutex, queue
feat(drivers): add the peripheral wrapper layer — gpio, uart, i2c, spi, nvs
feat(app): convert the main component to C++ and wire the three layers
docs: add CODING_STANDARDS.md
docs: document the four-layer architecture
chore(skills): add the coding-standards, cpp-wrapper-layers and git-commit project skills
chore(vscode): record the current ESP-IDF setup path
chore: ignore graphify output and the scratch directory
```

The commits were made in the order above (oldest last in `git log`). All ten
were staged with explicit paths — no `git add -A`. `git status --short` was empty
afterwards.

Note what was *not* done: the components were not lumped into one
"feat: add wrapper layers" commit. Each layer is a separate subject because each
can be reviewed, reverted and understood on its own.

Note also what *was* kept together: `components/core/**` — its headers, its
sources, its `CMakeLists.txt` and its umbrella header — is one commit, because
it is one subject.

> **History note.** This repository started life as a single-commit template, so
> `git log` shows one "initial template" commit rather than the ten above. The
> example is kept because it is the clearest illustration of the splitting rule —
> apply it to whatever tree you are actually committing.
