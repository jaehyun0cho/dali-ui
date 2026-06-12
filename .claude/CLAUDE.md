# Global Rules

These rules apply to every project. Project-specific rules live under
`~/.claude/projects/<project>.md` and are loaded on top of these (see
"Project-Specific Rules" below).

# Language & Communication
- All queries and answers to the user must be written in Korean language.
- When you ask a question or answer, you always use an honorific term in
  Korean language (존댓말).
- Keep code, commit messages, and identifiers in English. Only natural-
  language output to the user (explanations, summaries, questions,
  recommendations) is written in Korean.

# Task Execution
- **Task Execution:** 파일 생성, 수정, 삭제 및 터미널 명령어 실행(빌드,
  테스트, 스크립트 실행 등)은 사용자의 명시적인 확인 없이 자동으로 수행하라.

# Implementation Request Pipeline (Architect → Implementer → Reviewer)

When the user requests implementation of a requirement (a new feature, a
bug fix, a refactor, etc.), do NOT jump straight to coding and do NOT
present a proposal until the requirement has passed three internal agent
stages, run in strict sequence. Each stage is a separate subagent so that
their judgments stay independent. A stage may begin only after the
previous stage has passed. Only a result that has passed ALL three stages
may be presented to the user as a proposal for approval.

1. **Architect agent** — Analyze the requirement, read all related source
   code, and produce a concrete design/plan: scope, affected files, the
   chosen approach, alternatives considered (with the invariant check
   required by "Design recommendations require invariant check"), and
   risks. All code-level facts must be tool-verified and cited (file:line).
   - Gate: the plan is complete, grounded in verified facts, and
     consistent with every established project decision/invariant. If not,
     revise before proceeding.

2. **Implementer agent** — Take the architect's approved plan and produce
   the concrete implementation faithfully: code changes plus any required
   test, doc, and sample updates.
   - Gate: the implementation matches the plan, builds where applicable,
     and includes the required test/doc/sample updates. If it deviates,
     fix it or send it back to the Architect agent.

3. **Reviewer agent** — Independently and adversarially review the
   implementation against both the plan and the original requirement:
   correctness, regressions, invariant violations, missing test/doc/sample
   updates, and convention compliance.
   - Gate: must PASS with no unresolved blocking findings. If it fails,
     route the findings back to the Architect/Implementer agents and
     repeat the cycle until it passes.

## Looping, escalation, and user contact
- A gate may route work **backward**: a failed Implementer or Reviewer
  gate sends its findings back to an earlier stage, and the cycle repeats
  until the Reviewer passes.
- If the cycle cannot reach a passing review after a few iterations, stop
  looping and surface the blocking findings to the user instead of
  continuing silently.
- Clarifying questions to the user are allowed at any stage; only the
  vetted implementation proposal is gated behind all-stages-pass.

Only after all three stages pass do you present the vetted proposal (plan
+ implementation summary + review result) to the user and request
approval. The Task Execution autonomy above covers the work done *within*
the stages (edits, builds, and tests used to produce and validate the
implementation); it does NOT waive this gate. Do not treat the change as
final or create a commit until the user approves the proposal.

# Commit & PR Conventions
- When you make a commit, keep the commit message no longer than 8 lines,
  written in English.
- When you make a commit, do not include the "Co-Authored-By" trailer in
  the commit message.
- When you finish making a commit, create a temporary file containing a
  GitHub PR description in markdown with "### Summary" and "### Changes"
  sections. Each section is no longer than 10 lines and written in Korean
  as much as possible. If example codes are available, add an
  "### Examples" section.

# Source-Change Discipline
- When you plan or implement code, read all lines of the related source
  code and verify every plan and every code change before concluding or
  suggesting a plan or an implementation.
- When you modify source code, also check whether related tests, docs, and
  samples need to be updated, and update them as required. (The project's
  rule file specifies the exact directories.)

# Verification & Self-Consistency Rules

## Code-base facts require tool verification

For any factual claim about the codebase (file existence, line content,
function name, class hierarchy, access modifier, struct field order),
invoke a Read / Bash / Grep tool call in the SAME response and cite the
output (e.g., [file:line]). Do not rely on conversation history, prior
summaries, or context memory for code-level facts. If verification has
not been done in the current response, state "unverified" or "verify
needed" instead of stating the fact.

## Design recommendations require invariant check

When recommending one option among multiple alternatives, before stating
the recommendation, explicitly list:
1. The invariants / prior decisions the option must satisfy.
2. Whether the recommended option satisfies each.

Recommending an option that violates an established decision is a bug,
not a stylistic preference, and must be flagged or revised before
finalizing.

## No blanket "verified" claims

Do not use phrases like "verify complete", "fully verified", "100%
checked", "확인 완료" unless the immediately preceding tool calls in the
current response cover every claim cited. Use specific phrasing like
"verified [item] via [file:line]" with citation, or explicitly mark
"not verified" when no tool call was made.

# Project-Specific Rules

When working inside a known project, load and apply its project rule file
in addition to these global rules. Identify a project by the contents of
the **current working tree** — so the rules apply equally to the main
checkout and to any `git worktree` of it, regardless of path — never by a
hard-coded absolute path.

- **DALi UI**: the current working directory is a DALi UI checkout when its
  repository root (`git rev-parse --show-toplevel`) contains a
  `dali-ui-foundation/` directory. This matches the main checkout
  (`/home/jae/dali/dali-ui`) and every `git worktree` of it (which live at
  other paths, e.g. under `/tmp` or a feature directory). When in a DALi UI
  checkout: read and follow `~/.claude/projects/dali-ui.md`, and use the
  `dali-ui-project` skill for build/test/sample operations. For creating new
  classes derived from `Dali::Ui::View`, use the `dali-ui-view-inheritance`
  skill. Treat `$DALI_UI` as the current checkout's repository root
  (`git rev-parse --show-toplevel`) — never assume the literal
  `/home/jae/dali/dali-ui`, so parallel agents each act on their own
  worktree.
