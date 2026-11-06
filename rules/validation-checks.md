# Validation Checks

This document records checks that can be run by humans today and converted into
skills, scripts, or MCP tools later.

The commands are intentionally conservative. They find likely issues, not final
verdicts. Review each hit in context before changing code.

## Boolean Enabled API Naming

### Preferred Pattern Count

Use this to see how widely the preferred pattern appears:

```sh
rg -o '\bSet[A-Za-z0-9]+Enabled\(bool' \
  dali-ui-foundation/public-api \
  dali-ui-components/public-api \
  dali-ui-foundation/extension-api \
  dali-ui-foundation/integration-api \
  dali-ui-components/integration-api | wc -l

rg -o '\bIs[A-Za-z0-9]+Enabled\(\) const' \
  dali-ui-foundation/public-api \
  dali-ui-components/public-api \
  dali-ui-foundation/extension-api \
  dali-ui-foundation/integration-api \
  dali-ui-components/integration-api | wc -l
```

During the boolean naming cleanup that motivated this rule, the API surface had
130 `Set*Enabled(bool)` occurrences and 166 `Is*Enabled() const` occurrences.

### Avoided Patterns

Search for getter/setter forms that should usually be renamed:

```sh
rg -n '\bGet[A-Za-z0-9]+Enabled\(\) const' \
  dali-ui-foundation/public-api \
  dali-ui-components/public-api \
  dali-ui-foundation/extension-api \
  dali-ui-foundation/integration-api \
  dali-ui-components/integration-api

rg -n '\bSetEnable[A-Za-z0-9]+\(bool' \
  dali-ui-foundation/public-api \
  dali-ui-components/public-api \
  dali-ui-foundation/extension-api \
  dali-ui-foundation/integration-api \
  dali-ui-components/integration-api

rg -n '\bEnable[A-Za-z0-9]+\(bool' \
  dali-ui-foundation/public-api \
  dali-ui-components/public-api \
  dali-ui-foundation/extension-api \
  dali-ui-foundation/integration-api \
  dali-ui-components/integration-api

rg -n '\bIsEnable[A-Za-z0-9]+\(\) const' \
  dali-ui-foundation/public-api \
  dali-ui-components/public-api \
  dali-ui-foundation/extension-api \
  dali-ui-foundation/integration-api \
  dali-ui-components/integration-api
```

Expected exceptions may include:

- `SetEnabled(bool)` on `View`.
- Internal implementation helpers that are not public option properties.
- External API calls from DALi adaptor or platform interfaces.

### Serialized Property Keys

When a C++ API is renamed, check that serialized property keys were not changed
accidentally:

```sh
rg -n '"enable[A-Za-z0-9]+"' dali-ui-foundation dali-ui-components
```

If property keys are changed intentionally, document the migration impact.

## Public API Rename Checklist

For public API rename changes, check all of these:

```sh
rg -n 'OldApiName' dali-ui-foundation dali-ui-components automated-tests manual-tests samples wiki
rg -n 'NewApiName' dali-ui-foundation dali-ui-components automated-tests manual-tests samples wiki
git diff --check
```

Also inspect:

- Public headers.
- Extension and integration forwarding APIs.
- Internal implementation method names.
- Tests, samples, and wiki references.
- PR description public API change table.

## Component Boundary Checks

`dali-ui-components` must not include foundation integration or internal
headers. A useful first pass:

```sh
rg -n '#include <dali-ui-foundation/(integration-api|internal)/' dali-ui-components
```

Hits should usually be treated as violations unless there is an explicit design
decision.

## Length Unit Tags

Every documented pixel length in the five API directories, outside the generated `*.autogen.h` headers, must carry
exactly one `@UNIT_LOGICAL` or `@UNIT_PIXEL` tag (see [Length Units](length-units.md)).

Check the whole API surface:

```sh
python3 scripts/check-length-unit-tags.py --all
```

Check only the lines a change touched:

```sh
python3 scripts/check-length-unit-tags.py --diff origin/devel
```

The script reports a missing tag, more than one tag, a tag that is not the first token, an unknown `@UNIT_` word, a
tag that differs from the class tag, a Set/Get pair with different tags and, with `--all`, a stale allowlist entry and
a copy of the tag definitions that differs from `DEFINITIONS` in the script or, in a header, does not follow its tag.
It finds candidates by C++ type and name, so a hit can be a false positive: add it to the allowlist in the script
with a reason instead of adding a tag. For the same reason it can miss a length whose type or name it does not
recognize, so zero violations does not prove that every length is tagged; review new length APIs by hand as well.

After changing the script, run its self-test, which checks that `--diff` applies class tags of unchanged headers and
reports a deleted tag:

```sh
python3 scripts/check-length-unit-tags.py --self-test
```
