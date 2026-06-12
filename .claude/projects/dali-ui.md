# DALi UI Project Rules

Apply these rules on top of the global rules in `~/.claude/CLAUDE.md`
whenever the working directory is the DALi UI project.

## Identity & Paths
- `$DALI_UI` is the **current checkout's repository root**, not a fixed
  path. Resolve it with `DALI_UI="$(git rev-parse --show-toplevel)"`. The
  working directory is always somewhere inside `$DALI_UI`.
- A directory is a DALi UI checkout when `$DALI_UI` contains a
  `dali-ui-foundation/` directory. The main checkout is
  `/home/jae/dali/dali-ui`, but `git worktree`s of it live at other paths;
  always operate on the worktree you are currently in, never on the literal
  `/home/jae/dali/dali-ui`. This keeps parallel agents isolated to their own
  worktrees.
- DALi UI depends on `/home/jae/dali/dali-core` (a separate repository, not
  part of this worktree); you may refer to it for context.

## Planning & Implementation
- When you plan or implement layout code, check and compare how Android,
  Compose, Flutter, MAUI, and WPF implement the same concept before
  deciding on an approach. Use that comparison only for your own internal
  reasoning; never surface these framework names in the output (see the
  rule below).
- Do not mention any other UI framework in source code, comments,
  descriptions, PR text, or commit messages. In particular, never name
  Android, MAUI, or Flutter in commit messages, titles, code, or comments.
- When you modify source code, check whether the following also need
  updates, and update them as required:
  - `automated-tests`
  - `docs`
  - `manual-tests`
  - `samples`

## Project Structure
- DALi Handle/Body pattern with `IntrusivePtr` reference counting.
- Trait system for attaching data to Views (TraitId-based storage).
- Layout system: `LayoutParams` is a Handle/Body type with no View
  dependency. `SetLayoutParams` stores the handle as-is and calls
  `InvalidateMeasure` (no `Clone`).
- `@CHAIN_START` / `@CHAIN_MANUAL` / `@CHAIN_END` markers generate the
  `view.autogen.h` macro.
- A pre-commit hook (`scripts/run-format.sh check`) runs `clang-format-20`.

## Key Files
- `dali-ui-foundation/public-api/view.h` — View class with the layout
  params API (`SetLayoutParams`, `GetLayoutParams<T>`).
- `dali-ui-foundation/internal/layout/layout-params-impl.h` — base impl
  (`GetTraitId` is pure virtual).
- `dali-ui-foundation/public-api/view.autogen.h` — generated chain macro.
- `docs/layout-structure.md` — layout architecture documentation.

## Related Skills
- `dali-ui-project` — build, test, manual-test, and sample build
  commands for this project.
- `dali-ui-view-inheritance` — creating a new class derived from
  `Dali::Ui::View` (handle/impl pair, type registration, ABI-safe API).
