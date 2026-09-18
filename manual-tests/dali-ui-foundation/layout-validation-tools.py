#!/usr/bin/env python3
"""Offline artifact validation and statistics. Never launches or controls a TC."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import re
import statistics
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
JSON_RECORDS = {"LR_CASE_CONTRACT", "LR_CASE_END", "LR_ACTION", "LR_CHECK", "LR_RESULT", "LR_SAMPLE", "LR_NOOP", "LR_CLEANUP_ERROR", "LR_FRAME"}


# The producer's verdict vocabulary. FAIL, PASS and EXTERNAL_REQUIRED are TERMINAL: they are
# only emitted once completed_scenarios == required_scenarios. FAILING, INCOMPLETE and PENDING
# are non-terminal and say nothing final about the case.
TERMINAL_VERDICTS = {"FAIL", "PASS", "EXTERNAL_REQUIRED"}
FAILED_VERDICTS = {"FAIL", "FAILING"}
VERDICTS = TERMINAL_VERDICTS | FAILED_VERDICTS | {"INCOMPLETE", "PENDING"}

# How the frame fence can end abnormally. A normal completion emits no record at all, so an
# LR_FRAME line always accompanies a render.frame-timeout failing row.
FRAME_RESULTS = {"timeout", "disconnected", "predicate-exception"}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, f"duplicate JSON field: {key}")
        result[key] = value
    return result


def finite_json_float(text):
    value = float(text)
    require(math.isfinite(value), f"nonfinite JSON exponent: {text}")
    return value


def strict_json(text):
    return json.loads(text, object_pairs_hook=unique_object, parse_float=finite_json_float,
                      parse_constant=lambda value: (_ for _ in ()).throw(ValueError(f"nonfinite JSON: {value}")))


def read_json(path):
    return strict_json(Path(path).read_text(encoding="utf-8"))


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def integer(value, minimum=0):
    return type(value) is int and value >= minimum


def finite_number(value):
    try:
        return type(value) in (int, float) and math.isfinite(value)
    except (OverflowError, ValueError):
        return False


def nonempty_string(value):
    return isinstance(value, str) and bool(value) and value == value.strip()


def hash_string(value):
    return isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value) is not None


def records(path):
    for number, raw in enumerate(Path(path).read_text(encoding="utf-8", errors="strict").splitlines(), 1):
        line = re.sub(r"\x1b\[[0-?]*[ -/]*[@-~]", "", raw).lstrip()
        kind = line.split(None, 1)[0] if line else ""
        if kind in JSON_RECORDS:
            parts = line.split(None, 1)
            require(len(parts) == 2, f"{path}:{number}: truncated {kind}")
            try:
                value = strict_json(parts[1])
            except (ValueError, TypeError) as error:
                raise ValueError(f"{path}:{number}: malformed {kind}: {error}") from error
            require(isinstance(value, dict), f"{path}:{number}: {kind} must contain an object")
            yield kind, value
        elif kind.startswith("LR_") and any(known.startswith(kind) for known in JSON_RECORDS):
            raise ValueError(f"{path}:{number}: truncated LR record marker")
        elif line.startswith("LR_") and "{" in line:
            raise ValueError(f"{path}:{number}: unknown structured LR record")
        elif re.search(r"\bLR_(CASE_CONTRACT|CASE_END|ACTION|CHECK|RESULT|SAMPLE|NOOP|CLEANUP_ERROR|FRAME)\b", line):
            # A record marker that is not at the start of its line is not a record: whatever
            # wrote it interleaved with the producer, so the evidence cannot be trusted.
            raise ValueError(f"{path}:{number}: record marker not at line start")


def numeric_check(row):
    """Preserve the producer's comparison type; do not round int64 through float."""
    actual, expected, kind = row["actual"], row["expected"], row["kind"]
    require(isinstance(actual, str) and isinstance(expected, str), "check values must be strings")
    tolerance = row["tolerance"]
    require(finite_number(tolerance) and tolerance >= 0, "invalid tolerance")
    if kind == "text":
        require(tolerance == 0, "text checks cannot have a numeric tolerance")
        return actual == expected
    if kind == "integer":
        require(tolerance == 0, "integer checks must be exact")
        require(re.fullmatch(r"-?[0-9]+", actual) is not None and
                re.fullmatch(r"-?[0-9]+", expected) is not None, "malformed integer check")
        a, e = int(actual), int(expected)
        require(-(2**63) <= a < 2**63 and -(2**63) <= e < 2**63, "integer outside int64 range")
        return a == e
    if kind == "failure":
        return False
    if kind == "color":
        # Captured pixel colour: "r,g,b" 8-bit channels, each within the per-channel tolerance.
        require(re.fullmatch(r"[0-9]{1,3},[0-9]{1,3},[0-9]{1,3}", actual) is not None and
                re.fullmatch(r"[0-9]{1,3},[0-9]{1,3},[0-9]{1,3}", expected) is not None, "malformed color check")
        a, e = [int(v) for v in actual.split(",")], [int(v) for v in expected.split(",")]
        require(all(0 <= v <= 255 for v in a+e), "color channel outside 8-bit range")
        return all(abs(x-y) <= tolerance for x, y in zip(a, e))
    require(kind == "numeric", f"unknown check kind: {kind}")
    try:
        a, e = float(actual), float(expected)
    except (ValueError, TypeError):
        return False
    return all(math.isfinite(v) for v in (a, e)) and abs(a-e) <= tolerance


def base_identity(value):
    require(isinstance(value["tc"], str) and re.fullmatch(r"LR[0-9]{2}", value["tc"]), "invalid TC identity")
    require(integer(value["pid"], 1) and integer(value["run"], 1), "invalid process/run identity")
    return value["tc"], value["pid"], value["run"]


def contract_actions(value):
    scenarios = value["scenarios"]
    require(isinstance(scenarios, list) and scenarios, "empty scenario contract")
    names, result = set(), []
    for scenario in scenarios:
        name = scenario["id"]
        require(nonempty_string(name) and name not in names, "duplicate/invalid scenario contract")
        names.add(name)
        steps, step_names, row_start = scenario["steps"], set(), 1
        require(isinstance(steps, list) and steps, f"empty step contract: {name}")
        for step in steps:
            require(nonempty_string(step["id"]) and step["id"] not in step_names, f"duplicate/invalid step: {name}")
            step_names.add(step["id"])
            required = step["required_checks"]
            require(integer(required, 1), f"invalid mandatory count: {name}")
            result.append((name, step["id"], required, row_start))
            row_start += required
    return scenarios, result


def reviewed_manifest(manifest_path, manifest_sha256):
    """Load a separately reviewed manifest, pinned outside the candidate log."""
    require(hash_string(manifest_sha256), "an externally pinned manifest SHA-256 is required")
    path = Path(manifest_path) if manifest_path is not None else HERE / "layout-validation-manifest.json"
    raw = path.read_bytes()
    require(hashlib.sha256(raw).hexdigest() == manifest_sha256, "reviewed manifest fingerprint differs from the external pin")
    manifest = strict_json(raw.decode("utf-8"))
    require(isinstance(manifest, dict), "reviewed manifest must be an object")
    return manifest


def reviewed_contracts(manifest_path, manifest_sha256, profile, requested):
    """Load the required case contracts without trusting the runtime contract."""
    require(profile in {"diagnostic", "release"}, "an explicit diagnostic/release contract profile is required")
    manifest = reviewed_manifest(manifest_path, manifest_sha256)
    bundle = manifest.get("reviewed_contracts")
    require(isinstance(bundle, dict) and type(bundle.get("schema")) is int and bundle["schema"] == 1,
            "missing or unsupported reviewed_contracts schema")
    profiles = bundle.get("profiles")
    require(isinstance(profiles, dict) and isinstance(profiles.get(profile), dict),
            f"missing reviewed profile contract: {profile}")
    cases = profiles[profile]
    selected = {}
    for tc in requested:
        expected = cases.get(tc)
        require(isinstance(expected, dict) and expected.get("reviewed") is True,
                f"missing/unreviewed manifest contract: {profile}/{tc}")
        require("scenarios" in expected, f"missing manifest scenarios: {profile}/{tc}")
        contract_actions(expected)
        selected[tc] = expected
    return selected


def reviewed_metrics(manifest_path, manifest_sha256, family):
    """Select a complete metric family; a plan cannot silently drop a case or phase."""
    require(nonempty_string(family), "performance plan requires a reviewed metric family")
    manifest = reviewed_manifest(manifest_path, manifest_sha256)
    bundle = manifest.get("reviewed_performance_metrics")
    require(isinstance(bundle, dict) and type(bundle.get("schema")) is int and bundle["schema"] == 1,
            "missing or unsupported reviewed_performance_metrics schema")
    families = bundle.get("families")
    require(isinstance(families, dict), "missing reviewed metric families")
    contract = families.get(family)
    require(isinstance(contract, dict) and contract.get("reviewed") is True,
            f"missing/unreviewed metric family: {family}")
    metrics = contract.get("metrics")
    require(isinstance(metrics, list) and metrics and
            all(isinstance(metric, str) and re.fullmatch(r"LR[0-9]{2}\.[A-Za-z0-9_.-]+", metric) for metric in metrics) and
            len(set(metrics)) == len(metrics), f"invalid reviewed metric family: {family}")
    return metrics


def validate_plan_metrics(plan, plan_path):
    require(nonempty_string(plan.get("contract_manifest")), "performance plan requires a reviewed contract manifest path")
    require(hash_string(plan.get("contract_manifest_sha256")), "performance plan requires an externally pinned contract manifest SHA-256")
    contract_path = Path(plan_path).resolve().parent / plan["contract_manifest"]
    expected = reviewed_metrics(contract_path, plan["contract_manifest_sha256"], plan.get("metric_family"))
    metrics = plan.get("metrics")
    require(isinstance(metrics, list) and all(isinstance(metric, str) for metric in metrics) and
            len(set(metrics)) == len(metrics), "invalid metric family")
    require(set(metrics) == set(expected),
            f"plan metric family differs from reviewed contract: {plan['metric_family']}: "
            f"missing={sorted(set(expected)-set(metrics))}, extra={sorted(set(metrics)-set(expected))}")
    return contract_path


def check_log(path, requested, external=False, *, manifest_path=None, manifest_sha256=None, profile=None):
    require(requested and len(set(requested)) == len(requested), "requested TC list must be nonempty and unique")
    require(all(isinstance(tc, str) and re.fullmatch(r"LR[0-9]{2}", tc) for tc in requested), "invalid requested TC")
    reviewed = reviewed_contracts(manifest_path, manifest_sha256, profile, requested)
    # Select the newest run encountered for each TC. An abandoned pre-Reset run
    # cannot poison a later run, and an old PASS cannot hide a newer incomplete run.
    grouped, latest, seen = {}, {}, set()
    for kind, value in records(path):
        base = base_identity(value)
        if base[0] not in requested:
            continue
        if base not in seen:
            seen.add(base)
            latest[base[0]] = base
        grouped.setdefault(base, []).append((kind, value))
    require(set(latest) == set(requested), f"missing requested TCs: {sorted(set(requested)-set(latest))}")
    # A failing row is EVIDENCE, not a parse error: the structural invariants below (every
    # scenario completed, every declared row observed, unique IDs, totals consistent) are the
    # only thing that decides whether the log can be trusted, and they must all be evaluated
    # even for a log full of failures. The failing rows are reported separately.
    completed, external_tcs, failing_rows, frame_failures = [], [], {}, {}
    for tc in requested:
        base = latest[tc]
        contract, actions, rows, final, ending = None, {}, {}, None, None
        failing, frames = [], {}
        previous_totals = (0, 0, 0)
        for kind, value in grouped[base]:
            require(ending is None or (kind == "LR_CASE_END" and ending == value),
                    f"new/changed records after case end: {base}")
            if kind == "LR_CASE_CONTRACT":
                require(contract is None or contract == value, f"changed immutable contract: {base}")
                contract_actions(value)
                require(value["scenarios"] == reviewed[tc]["scenarios"],
                        f"runtime contract differs from reviewed manifest: {profile}/{tc}")
                contract = value
                continue
            require(contract is not None, f"missing case contract before {kind}: {base}")
            if kind == "LR_SAMPLE":
                # Timing values have their own validator, but their run identity
                # and position before the terminal cleanup record still matter.
                continue
            if kind == "LR_NOOP":
                # An ignored button press: it creates no rows and changes no state.
                require(nonempty_string(value.get("button")) and nonempty_string(value.get("reason")), f"malformed noop: {base}")
                continue
            if kind == "LR_FRAME":
                # An abnormal frame-fence completion. It is not a row and changes no count, but
                # it names WHY the action's render.frame-timeout row exists, which the row alone
                # does not, and the pairing of the two is enforced with the row counts below.
                require(value.get("result") in FRAME_RESULTS and nonempty_string(value.get("reason")) and
                        nonempty_string(value.get("scenario")) and isinstance(value.get("step"), str) and
                        integer(value.get("action_seq"), 1) and integer(value.get("attempts")) and
                        finite_number(value.get("elapsed_ms")) and value["elapsed_ms"] >= 0 and
                        integer(value.get("deadline_ms"), 1), f"malformed frame record: {base}")
                frames[value["action_seq"]] = value
                continue
            if kind == "LR_CLEANUP_ERROR":
                # A cleanup that threw leaves the process state unknown, so the run's evidence
                # is void. Scoped to the latest run of a requested TC: an abandoned earlier run
                # cannot poison a later one, and another TC's cleanup is not this TC's problem.
                require(False, f"cleanup failure: {base}")
            scenarios, expected_actions = contract_actions(contract)
            if kind == "LR_CASE_END":
                require(final is not None, f"case ended without a result: {base}")
                fields = ("failures", "completed_steps", "completed_scenarios", "required_scenarios")
                require(all(integer(value[k]) for k in fields), f"invalid case end counts: {base}")
                require(value["cleanup_passed"] is True, f"cleanup failure at case end: {base}")
                require(value["reason"] in {"reset", "exit"}, f"invalid case end reason: {base}")
                # The case end must agree with the final result on every count, failures
                # included: they describe the same run, and a disagreement means one of the two
                # records is not this run's.
                require(all(value[k] == final[k] for k in fields), f"case end differs from final result: {base}")
                ending = value
            elif kind == "LR_ACTION":
                sequence = value["action_seq"]
                require(integer(sequence, 1) and sequence == len(actions)+1, f"noncontiguous action: {base}")
                require(sequence <= len(expected_actions), f"extra action: {base}")
                name, step, required, _ = expected_actions[sequence-1]
                require((value["scenario"], value["step"], value["required_checks"]) == (name, step, required),
                        f"action differs from immutable contract: {base}/{sequence}")
                require(integer(value["required_checks"], 1), "invalid action count type")
                actions[sequence] = value
            elif kind == "LR_CHECK":
                sequence = value["action_seq"]
                require(integer(sequence, 1) and sequence in actions, f"row without action: {base}")
                name, step, required, start = expected_actions[sequence-1]
                require((value["scenario"], value["step"]) == (name, step), f"row scope mismatch: {base}")
                require(integer(value["row"], 1) and start <= value["row"] < start+required, f"row outside action range: {base}")
                require(nonempty_string(value["id"]), "empty assertion ID")
                identity = (sequence, value["row"])
                require(identity not in rows or rows[identity] == value, f"changed frozen row: {base}/{identity}")
                # Recompute the comparison independently: the producer's verdict flag must
                # agree with its own values, so a "passed" row carrying mismatched or
                # non-numeric values is invalid evidence rather than a pass.
                passed = value["passed"] is True
                require(numeric_check(value) == passed, f"row verdict disagrees with its values: {base}/{value['id']}")
                if not passed and identity not in rows:
                    failing.append({"action_seq": sequence, "row": value["row"], "id": value["id"],
                                    "scenario": value["scenario"], "step": value["step"],
                                    "actual": value["actual"], "expected": value["expected"],
                                    "tolerance": value["tolerance"]})
                rows[identity] = value
            else:
                require(kind == "LR_RESULT", "unsupported check-log record")
                fields = ("failures", "completed_steps", "completed_scenarios", "required_scenarios", "checks")
                require(all(integer(value[k]) for k in fields), f"invalid result counts: {base}")
                require(value["required_scenarios"] == len(scenarios), f"result changed required scenarios: {base}")
                totals = (value["completed_steps"], value["completed_scenarios"], value["checks"])
                require(all(a <= b for a, b in zip(previous_totals, totals)), f"cumulative result regressed: {base}")
                # Verdict grammar, not verdict value: FAIL/PASS/EXTERNAL_REQUIRED are terminal
                # and may only appear once every declared scenario has completed, and the
                # failure counter and the verdict must tell the same story.
                require(value["verdict"] in VERDICTS, f"unknown verdict: {base}: {value['verdict']}")
                require((value["verdict"] in TERMINAL_VERDICTS) ==
                        (value["completed_scenarios"] == value["required_scenarios"]),
                        f"terminal verdict before completion (or non-terminal after it): {base}: {value['verdict']}")
                require((value["failures"] != 0) == (value["verdict"] in FAILED_VERDICTS),
                        f"failure count disagrees with the verdict: {base}: {value['verdict']}")
                previous_totals, final = totals, value
        require(contract is not None and final is not None, f"missing contract/final: {base}")
        require(ending is not None, f"missing latest-run case end/cleanup evidence: {base}")
        scenarios, expected_actions = contract_actions(contract)
        allowed = {"PASS", "EXTERNAL_REQUIRED"} if external else {"PASS"}
        require(final["completed_scenarios"] == len(scenarios), f"missing scenario: {base}")
        require(final["completed_steps"] == len(actions) == len(expected_actions), f"missing action: {base}")
        for sequence, (_, _, required, _) in enumerate(expected_actions, 1):
            observed = [value for (seq, _), value in rows.items() if seq == sequence]
            # A fence failure short-circuits the action, so it is the usual cause of a short row
            # count. Name it here rather than leaving the operator with a bare arithmetic message.
            hint = f"; frame failure: {frames[sequence]['result']}" if sequence in frames else ""
            require(len(observed) == required, f"missing/extra check rows: {base}/{sequence}: {len(observed)}/{required}{hint}")
            require(len({value["id"] for value in observed}) == required, f"duplicate assertion IDs replace required checks: {base}/{sequence}")
            if sequence in frames:
                require(any(row["action_seq"] == sequence and row["id"] == "render.frame-timeout" for row in failing),
                        f"frame failure record without its render.frame-timeout row: {base}/{sequence}")
        require(len(rows) == final["checks"], f"cumulative row mismatch: {base}")
        # The summary must account for exactly the failing rows and nothing else: a failure
        # counted without a row (an overflow, or an Append after the step finished) would
        # otherwise be invisible.
        require(final["failures"] == len(failing), f"summary failures differ from failing rows: {base}: {final['failures']}/{len(failing)}")
        if failing:
            require(final["verdict"] == "FAIL", f"failing rows without a FAIL verdict: {base}: {final['verdict']}")
            failing_rows[tc] = failing
        else:
            require(final["verdict"] in allowed, f"incomplete/external verdict: {base}: {final['verdict']}")
        completed.append(tc)
        if frames:
            frame_failures[tc] = [{"action_seq": sequence, "scenario": frames[sequence]["scenario"],
                                   "step": frames[sequence]["step"], "result": frames[sequence]["result"],
                                   "reason": frames[sequence]["reason"], "attempts": frames[sequence]["attempts"],
                                   "elapsed_ms": frames[sequence]["elapsed_ms"]}
                                  for sequence in sorted(frames)]
        if final["verdict"] == "EXTERNAL_REQUIRED":
            external_tcs.append(tc)
    return {"status": "observations-valid-with-failures" if failing_rows else "observations-valid",
            "tcs": sorted(completed), "contract_profile": profile, "contract_manifest_sha256": manifest_sha256,
            "external_verification_pending": bool(external_tcs), "external_tcs": sorted(external_tcs),
            "failing_rows": {tc: failing_rows[tc] for tc in sorted(failing_rows)},
            "frame_failures": {tc: frame_failures[tc] for tc in sorted(frame_failures)}}


def sample_medians(path, metrics, timer_floor_ns, with_metadata=False):
    grouped, identities, processes, scopes, operations = {}, set(), set(), {}, {}
    declared_actions, latest_runs, seen_runs = set(), {}, set()
    for kind, value in records(path):
        base = base_identity(value)
        if base not in seen_runs:
            seen_runs.add(base)
            latest_runs[base[0]] = base
        if kind == "LR_ACTION":
            declared_actions.add(base + (value["scenario"], value["step"], value["action_seq"]))
        if kind != "LR_SAMPLE":
            continue
        tc, pid, run = base_identity(value)
        processes.add(pid)
        metric = value["metric"]
        require(metric in metrics and metric.startswith(tc+"."), f"unplanned/misattributed metric: {metric}")
        require(value["profile"] == "release", f"timing sample from a non-Release build: {metric}")
        require(integer(value["operations"], 1) and finite_number(value["nanoseconds"]) and value["nanoseconds"] >= timer_floor_ns,
                f"sample below declared timer floor or invalid: {metric}")
        require(integer(value["iteration"]) and integer(value["action_seq"], 1), "invalid sample iteration/action")
        require(nonempty_string(value["scenario"]) and nonempty_string(value["step"]), "invalid sample scope")
        scope = (tc, pid, run, value["scenario"], value["step"], value["action_seq"])
        require(scope in declared_actions, f"sample without declared action: {metric}")
        require(metric not in scopes or scopes[metric] == scope, f"metric combines different runs/actions: {metric}")
        require(metric not in operations or operations[metric] == value["operations"], f"metric changes operation count: {metric}")
        scopes[metric], operations[metric] = scope, value["operations"]
        identity = scope + (metric, value["iteration"])
        require(identity not in identities, f"duplicate sample: {identity}")
        identities.add(identity)
        per_operation = value["nanoseconds"] / value["operations"]
        require(math.isfinite(per_operation) and per_operation > 0, "invalid normalized duration")
        grouped.setdefault(metric, []).append((value["iteration"], per_operation))
    require(all(scope[:3] == latest_runs[scope[0]] for scope in scopes.values()), "samples belong to an abandoned run")
    require(len(processes) == 1, "a process-pair side must contain exactly one process")
    require(set(grouped) == set(metrics), "missing metric")
    result = {}
    for metric, data in grouped.items():
        require(sorted(i for i, _ in data) == list(range(31)), f"expected exactly 31 post-warmup samples: {metric}")
        result[metric] = statistics.median(value for _, value in data)
    if with_metadata:
        return result, next(iter(processes)), operations, scopes
    return result


def paired_ci(log_ratios, alpha, draws, seed):
    require(log_ratios and all(finite_number(v) for v in log_ratios), "invalid paired log ratios")
    require(finite_number(alpha) and 0 < alpha < 1 and integer(draws, 1) and integer(seed), "invalid bootstrap settings")
    rng = random.Random(seed)
    n = len(log_ratios)
    boot = sorted(sum(log_ratios[rng.randrange(n)] for _ in range(n))/n for _ in range(draws))
    tail = alpha/2
    low = boot[max(0, int(math.floor(tail * draws)))]
    high = boot[min(draws-1, int(math.ceil((1-tail) * draws))-1)]
    return statistics.mean(log_ratios), low, high


def compare(plan_path, experiment_path):
    plan, experiment = read_json(plan_path), read_json(experiment_path)
    require(plan["schema"] == 1, "unsupported performance plan schema")
    require(experiment["plan_sha256"] == digest(plan_path), "plan fingerprint differs")
    metrics = plan["metrics"]
    require(isinstance(metrics, list) and metrics and all(isinstance(m, str) and re.fullmatch(r"LR[0-9]{2}\.[A-Za-z0-9_.-]+", m) for m in metrics)
            and len(set(metrics)) == len(metrics), "invalid metric family")
    require(finite_number(plan["regression_margin"]) and plan["regression_margin"] == 0, "nonzero regression allowance is forbidden")
    require(plan["looks"] == [31, 62, 93] and all(type(v) is int for v in plan["looks"]), "predeclared bounded looks must be 31/62/93 independent process pairs")
    require(all(finite_number(plan[k]) for k in ("alpha", "precision", "mde", "timer_floor_ns")), "nonfinite precision/alpha/timer floor")
    require(0 < plan["alpha"] < .5 and 0 < plan["precision"] <= plan["mde"], "invalid precision or alpha")
    require(integer(plan["bootstrap_draws"], 1) and integer(plan["seed"]), "draws and seed must be nonnegative integers")
    alpha = plan["alpha"] / (len(metrics)*len(plan["looks"]))
    require(plan["bootstrap_draws"] * alpha/2 >= 20, "insufficient bootstrap tail resolution for family correction")
    require(plan["timer_floor_ns"] > 0, "timer floor must be positive")
    blocks = experiment["blocks"]
    require(isinstance(blocks, list) and len(blocks) in plan["looks"], "incomplete independent process pairs")
    contract_path = validate_plan_metrics(plan, plan_path)
    required_tcs = sorted({metric.split(".", 1)[0] for metric in metrics})
    reviewed_contracts(contract_path, plan["contract_manifest_sha256"], "release", required_tcs)
    pairs, process_ids, log_hashes = set(), set(), set()
    ratios, fingerprint = {metric: [] for metric in metrics}, None
    correctness_failures = []
    log_root = Path(experiment_path).resolve().parent
    for block in blocks:
        require(type(block["pair_id"]) in (str, int) and not isinstance(block["pair_id"], bool), "invalid pair ID")
        require(block["pair_id"] not in pairs, "duplicate independent process pair")
        pairs.add(block["pair_id"])
        require(block["order"] in {"AB", "BA"}, "invalid pair order")
        require(block["environment_reference"] == block["environment_candidate"], "unmatched runtime environment")
        environment = block["environment_reference"]
        required_environment = ("device", "cpu_affinity", "governor", "resolution", "dpi", "assets_sha256", "compiler_flags")
        require(isinstance(environment, dict) and all(k in environment and environment[k] not in (None, "", [], {}) for k in required_environment), "missing environment controls")
        require(hash_string(environment["assets_sha256"]), "invalid asset fingerprint")
        for side in ("reference", "candidate"):
            artifact = block[side+"_artifact"]
            require(all(hash_string(artifact[k]) for k in ("app_sha256", "library_sha256", "fixture_contract_sha256")), "invalid artifact hashes")
            # The observation hooks are compiled into every build, so there is no build flag to
            # assert here. What still matters, and is still falsifiable from the run's own log,
            # is that no capture was open across the timed intervals.
            require("diagnostics" not in artifact, "artifact field `diagnostics` was renamed to `capture_active_during_timing`")
            require(artifact["capture_active_during_timing"] is False and artifact["coverage"] is False and artifact["build_type"] == "Release",
                    "timing artifact must be Release with coverage off and no capture active during the timed intervals")
        require(block["reference_artifact"]["fixture_contract_sha256"] == block["candidate_artifact"]["fixture_contract_sha256"], "different workload contracts")
        current = (block["reference_artifact"], block["candidate_artifact"], environment)
        require(fingerprint is None or current == fingerprint, "artifact/environment changed between pairs")
        fingerprint = current
        observations = {}
        for side in ("reference", "candidate"):
            path = log_root / block[side+"_log"]
            content_hash = digest(path)
            require(content_hash not in log_hashes, "same log reused as independent process evidence")
            log_hashes.add(content_hash)
            # check_log only inspects the TCs the plan's metric family names, so a TC the plan
            # does not name would have its contract, its rows and its failures unexamined. A
            # timing log is evidence about one family and nothing else: a foreign TC in it means
            # unplanned work ran in the same process, which invalidates the timings too.
            present = sorted({base_identity(value)[0] for _, value in records(path)})
            require(set(present) <= set(required_tcs),
                    f"timing log contains TCs outside the plan's metric family: {block[side+'_log']}: "
                    f"planned={required_tcs}, unplanned={sorted(set(present)-set(required_tcs))}")
            medians, pid, counts, scopes = sample_medians(path, metrics, plan["timer_floor_ns"], True)
            require(pid not in process_ids, "same PID reused as an independent process (PID reuse requires a fresh cohort)")
            process_ids.add(pid)
            report = check_log(path, required_tcs, external=True, manifest_path=contract_path,
                               manifest_sha256=plan["contract_manifest_sha256"], profile="release")
            if report["failing_rows"] or report["frame_failures"]:
                correctness_failures.append({"pair_id": block["pair_id"], "side": side,
                                             "log": block[side+"_log"], "failing_rows": report["failing_rows"],
                                             "frame_failures": report["frame_failures"]})
            observations[side] = (medians, counts, scopes)
        reference, candidate = observations["reference"], observations["candidate"]
        require(reference[1] == candidate[1], "paired operation counts differ")
        for metric in metrics:
            # Run IDs and PIDs differ, but scenario, step and action must match.
            require(reference[2][metric][3:] == candidate[2][metric][3:], "paired metric scopes differ")
            ratios[metric].append(math.log(candidate[0][metric])-math.log(reference[0][metric]))
    require(abs(sum(b["order"] == "AB" for b in blocks)-sum(b["order"] == "BA" for b in blocks)) <= 1, "unbalanced order")
    # Validate every log, pair and sample before returning a valid failure. A malformed
    # later pair remains invalid evidence even when an earlier pair failed correctness.
    if correctness_failures:
        return {"status": "correctness-failed", "plan_sha256": digest(plan_path),
                "contract_manifest_sha256": plan["contract_manifest_sha256"],
                "correctness_failures": correctness_failures, "results": []}
    output = []
    for index, metric in enumerate(metrics):
        mean, low, high = paired_ci(ratios[metric], alpha, plan["bootstrap_draws"], plan["seed"]+index)
        if low > 0:
            verdict = "FAIL"
        elif high <= math.log1p(plan["mde"]) and (high-low)/2 <= math.log1p(plan["precision"]):
            verdict = "PASS_AT_DECLARED_RESOLUTION"
        else:
            verdict = "INDETERMINATE" if len(blocks) == 93 else "MORE_PAIRS_REQUIRED"
        require(high < math.log(sys.float_info.max), "ratio outside finite reportable range")
        output.append({"metric": metric, "pairs": len(blocks), "ratio": math.exp(mean), "ci_ratio": [math.exp(low), math.exp(high)], "verdict": verdict})
    return {"plan_sha256": digest(plan_path), "contract_manifest_sha256": plan["contract_manifest_sha256"],
            "family_alpha": plan["alpha"], "results": output}


# An explicit SHA-256 citation in contract prose. Only this shape is checked: asset byte
# hashes live in res/layout-validation/assets.json and are never quoted in the documents.
DOCUMENT_DIGEST_CITATION = re.compile(r"SHA-256\s+`([0-9a-f]{64})`")


def check_document_digests(paths, manifest_digest):
    """Every manifest SHA-256 the prose CITES must be the manifest's current digest.

    A stale citation is indistinguishable, to the operator who reads it, from the pin they
    are told to load into the trust store: it survived a manifest revision once already,
    and the plan-pin check could not see it because it is prose, not a plan field.
    """
    cited = 0
    for path in paths:
        path = Path(path)
        if not path.is_file():
            continue
        for value in DOCUMENT_DIGEST_CITATION.findall(path.read_text(encoding="utf-8")):
            cited += 1
            require(value == manifest_digest,
                    f"document cites a stale manifest SHA-256: {path.name}: {value} != {manifest_digest}")
    return cited


def audit():
    manifest = read_json(HERE / "layout-validation-manifest.json")
    expected = {f"LR{i:02d}" for i in range(1,66)}
    require(len(manifest["cases"]) == len(expected) and {case["id"] for case in manifest["cases"]} == expected, "manifest case IDs missing or duplicated")
    for case in manifest["cases"]:
        stem = HERE / "tc" / case["stem"]
        require(stem.with_suffix(".cpp").is_file() and stem.with_suffix(".md").is_file(), f"missing pair {case['id']}")
        require(f"REGISTER_MANUAL_TEST(TcLr{case['id'][2:]})" in stem.with_suffix(".cpp").read_text(), f"registration missing {case['id']}")
    require(len(list((HERE / "tc").glob("tc-lr*-layout-*.cpp"))) == len(expected), "unexpected TC count")
    # The md PROSE totals are what an operator reads before running anything, so they must
    # agree with the reviewed contract, not only with the table next to them: a stale total
    # makes the operator stop scenarios early and call the case complete.
    release = manifest["reviewed_contracts"]["profiles"]["release"]
    for case in manifest["cases"]:
        text = (HERE / "tc" / case["stem"]).with_suffix(".md").read_text(encoding="utf-8")
        scenarios = release[case["id"]]["scenarios"]
        steps = sum(len(scenario["steps"]) for scenario in scenarios)
        checks = sum(step["required_checks"] for scenario in scenarios for step in scenario["steps"])
        for stated in re.findall(r"총 scenario 수는 ([0-9]+)개", text):
            require(int(stated) == len(scenarios),
                    f"md scenario total differs from the contract: {case['id']}: {stated}/{len(scenarios)}")
        profiles = re.findall(r"^Profile [^:\n]*: ([0-9]+) scenario, ([0-9]+) action, ([0-9,]+) checks", text, re.M)
        require(profiles, f"md states no profile totals: {case['id']}")
        for stated in profiles:
            observed = (len(scenarios), steps, checks)
            require(tuple(int(value.replace(",", "")) for value in stated) == observed,
                    f"md profile totals differ from the contract: {case['id']}: {stated}/{observed}")
    assets = read_json(HERE / "res/layout-validation/assets.json")
    # Asset inventory stores only byte hashes; it does not certify installed font selection.
    entries = assets["files"]
    if isinstance(entries, dict):
        entries = [dict(value, path=key) for key, value in entries.items()]
    for entry in entries:
        name = entry.get("path", entry.get("file", entry.get("name")))
        path = HERE / "res/layout-validation" / name
        require(digest(path) == entry["sha256"], f"asset hash changed: {name}")
    manifest_digest = digest(HERE / "layout-validation-manifest.json")
    plans = sorted((HERE).glob("layout-validation-*plan.example.json"))
    for plan in plans:
        value = read_json(plan)
        pinned = value.get("contract_manifest_sha256")
        require(pinned == manifest_digest, f"example plan pins a different manifest: {plan.name}")
        validate_plan_metrics(value, plan)
    families = manifest["reviewed_performance_metrics"]["families"]
    declared = [read_json(plan)["metric_family"] for plan in plans]
    require(len(declared) == len(set(declared)) and set(declared) == set(families),
            "example plans must cover each reviewed metric family exactly once")
    # Same failure style as the plan pins, for the digests the PROSE quotes.
    documents = sorted(HERE.glob("layout-validation-*.md")) + sorted((HERE / "tc").glob("tc-lr*.md"))
    citations = check_document_digests(documents, manifest_digest)
    return {"status": "inventory-valid", "cases": len(expected), "assets": len(entries), "plans": len(plans),
            "document_digest_citations": citations, "manifest_sha256": manifest_digest}


def shrink(path):
    failure = read_json(path)
    require(isinstance(failure["nodes"], list) and failure["nodes"], "nodes required")
    candidates = []
    for index in range(len(failure["nodes"])):
        item = dict(failure)
        item["nodes"] = failure["nodes"][:index] + failure["nodes"][index+1:]
        item["candidate_removed_index"] = index
        item["reproduced"] = False
        candidates.append(item)
    return {"candidates": candidates, "requires_same_assertion_failure_on_replay": True}



def self_test():
    """Synthetic evidence exercises both acceptance and rejection end to end."""
    checks = []

    def passed(name, condition):
        require(condition, "self-test: "+name)
        checks.append(name)

    def rejected(name, callback):
        try:
            callback()
        except (ValueError, KeyError, TypeError, OverflowError):
            checks.append(name)
            return
        raise ValueError("self-test accepted invalid evidence: "+name)

    def rejected_with(name, callback, substring):
        """Rejection for the stated REASON: a rejection by an unrelated rule is not proof."""
        try:
            callback()
        except (ValueError, KeyError, TypeError, OverflowError) as error:
            require(substring in str(error), f"self-test: {name}: rejected for the wrong reason: {error}")
            checks.append(name)
            return
        raise ValueError("self-test accepted invalid evidence: "+name)

    def cli(name, arguments, expected_code):
        result = subprocess.run([sys.executable, str(Path(__file__).resolve())]+arguments,
                                capture_output=True, text=True, check=False)
        passed(name, result.returncode == expected_code and isinstance(strict_json(result.stdout), dict))

    def clone(value):
        return json.loads(json.dumps(value))

    def lines(path, values, extra=""):
        path.write_text("".join(kind+" "+json.dumps(value, allow_nan=True)+"\n" for kind, value in values)+extra, encoding="utf-8")

    def case(pid=100, run=1, scenarios=2, tc="LR01"):
        base = {"tc":tc,"pid":pid,"run":run}
        contract = dict(base, scenarios=[{"id":f"s{i}","steps":[{"id":"run","required_checks":2}]} for i in range(scenarios)])
        result = [("LR_CASE_CONTRACT", contract)]
        for i in range(scenarios):
            scope = dict(base, scenario=f"s{i}",step="run",action_seq=i+1)
            result.append(("LR_ACTION", dict(scope, required_checks=2)))
            result.append(("LR_CHECK", dict(scope,row=1,id="width",kind="numeric",actual="1.0005",expected="1",tolerance=.001,passed=True)))
            result.append(("LR_CHECK", dict(scope,row=2,id="count",kind="integer",actual="1",expected="1",tolerance=0,passed=True)))
        result.append(("LR_RESULT", dict(base, scenario=f"s{scenarios-1}",completed_steps=scenarios,completed_scenarios=scenarios,required_scenarios=scenarios,checks=scenarios*2,failures=0,verdict="PASS")))
        result.append(("LR_CASE_END", dict(base, completed_steps=scenarios, completed_scenarios=scenarios,
                                           required_scenarios=scenarios, failures=0, cleanup_passed=True, reason="exit")))
        return result

    passed("numeric-finite-tolerance", numeric_check({"kind":"numeric","actual":"1.0005","expected":"1","tolerance":.001}))
    passed("numeric-nan-rejected", not numeric_check({"kind":"numeric","actual":"nan","expected":"nan","tolerance":0}))
    passed("numeric-inf-rejected", not numeric_check({"kind":"numeric","actual":"inf","expected":"inf","tolerance":0}))
    passed("text-numeric-spelling-exact", not numeric_check({"kind":"text","actual":"01","expected":"1","tolerance":0}))
    passed("integer-above-double-precision", not numeric_check({"kind":"integer","actual":"9007199254740993","expected":"9007199254740992","tolerance":0}))
    passed("color-within-channel-tolerance", numeric_check({"kind":"color","actual":"229,57,53","expected":"231,55,56","tolerance":3}))
    passed("color-outside-channel-tolerance", not numeric_check({"kind":"color","actual":"229,57,53","expected":"229,57,60","tolerance":3}))
    rejected("color-malformed", lambda: numeric_check({"kind":"color","actual":"229,57","expected":"229,57,53","tolerance":3}))
    rejected("unknown-comparison-kind", lambda: numeric_check({"kind":"guess","actual":"1","expected":"1","tolerance":0}))
    rejected("duplicate-json-fields", lambda: strict_json('{"passed":false,"passed":true}'))
    rejected("nonfinite-json", lambda: strict_json('{"precision":NaN}'))
    rejected("overflowing-json-exponent", lambda: strict_json('{"precision":1e400}'))
    with tempfile.TemporaryDirectory(prefix="layout-tools-self-test-") as directory:
        root=Path(directory); log=root/"checks.log"; good=case()
        manifest_path = root/"reviewed-manifest.json"
        manifest = {"reviewed_contracts": {"schema": 1, "profiles": {
            "diagnostic": {"LR01": {"reviewed": True, "scenarios": clone(good[0][1]["scenarios"])}},
            "release": {"LR59": {"reviewed": True, "scenarios": clone(case(scenarios=1, tc="LR59")[0][1]["scenarios"])}}
        }}}
        manifest["reviewed_performance_metrics"] = {"schema": 1, "families": {
            "synthetic": {"reviewed": True, "metrics": ["LR59.synthetic"]},
            "two_cases": {"reviewed": True, "metrics": ["LR59.synthetic", "LR60.synthetic"]}
        }}
        manifest_path.write_text(json.dumps(manifest))
        manifest_pin = digest(manifest_path)
        contract_cli = ["--manifest", str(manifest_path), "--manifest-sha256", manifest_pin, "--profile", "diagnostic"]

        def checked(path, requested, external=False):
            return check_log(path, requested, external, manifest_path=manifest_path,
                             manifest_sha256=manifest_pin, profile="diagnostic")

        base_scope={"tc":"LR01","pid":100,"run":1}
        lines(log,good)
        rejected("missing-external-contract-pin", lambda: check_log(log,["LR01"],manifest_path=manifest_path,profile="diagnostic"))
        rejected("missing-contract-profile", lambda: check_log(log,["LR01"],manifest_path=manifest_path,manifest_sha256=manifest_pin))
        rejected("wrong-contract-pin", lambda: check_log(log,["LR01"],manifest_path=manifest_path,manifest_sha256="0"*64,profile="diagnostic"))
        rejected("wrong-profile-case-contract", lambda: check_log(log,["LR01"],manifest_path=manifest_path,manifest_sha256=manifest_pin,profile="release"))

        def invalid_manifest(name, value):
            manifest_path.write_text(json.dumps(value))
            rejected(name, lambda: check_log(log,["LR01"],manifest_path=manifest_path,
                                            manifest_sha256=digest(manifest_path),profile="diagnostic"))
            manifest_path.write_text(json.dumps(manifest))

        invalid_manifest("missing-reviewed-contract-metadata", {})
        bad_manifest=clone(manifest);bad_manifest["reviewed_contracts"]["profiles"]["diagnostic"]["LR01"]["reviewed"]=False
        invalid_manifest("unreviewed-contract-refused", bad_manifest)
        bad_manifest=clone(manifest);bad_manifest["reviewed_contracts"]["profiles"]["diagnostic"]["LR01"]["scenarios"][0]["steps"][0]["required_checks"]=0
        invalid_manifest("empty-reviewed-assertions-refused", bad_manifest)
        lines(log,case(scenarios=1))
        rejected("deleted-scenario-and-shrunken-runtime-contract", lambda: checked(log,["LR01"]))
        lines(log,good)
        contracted=clone(good)
        for scenario in contracted[0][1]["scenarios"]: scenario["steps"][0]["required_checks"]=1
        contracted=[entry for entry in contracted if entry[0]!="LR_CHECK" or entry[1]["row"]==1]
        for kind,value in contracted:
            if kind=="LR_ACTION":value["required_checks"]=1
            if kind=="LR_RESULT":value["checks"]=2
        lines(log,contracted)
        rejected("deleted-assertion-and-shrunken-runtime-contract", lambda: checked(log,["LR01"]))
        lines(log,good)
        passed("complete-two-scenario-log", checked(log,["LR01"])["status"] == "observations-valid")
        lines(log,good[:1]+[("LR_NOOP",{"tc":"LR01","pid":100,"run":1,"button":"Run all","reason":"step still running"})]+good[1:])
        passed("noop-record-ignored", checked(log,["LR01"])["status"] == "observations-valid")
        lines(log,good[:-1]);rejected("missing-latest-run-cleanup-end",lambda:checked(log,["LR01"]))
        for name,updates in [
            ("cleanup-end-failure", {"failures":1,"cleanup_passed":False}),
            ("cleanup-end-unconfirmed", {"cleanup_passed":False}),
            ("cleanup-end-count-mismatch", {"completed_steps":1}),
            ("cleanup-end-invalid-reason", {"reason":"unknown"}),
            ("cleanup-end-wrong-run", {"run":2}),
        ]:
            bad=clone(good);bad[-1][1].update(updates);lines(log,bad)
            rejected(name,lambda:checked(log,["LR01"]))
        lines(log,good+[("LR_RESULT",clone(good[-2][1]))])
        rejected("result-after-closed-run",lambda:checked(log,["LR01"]))
        lines(log,good+[("LR_SAMPLE",{"tc":"LR01","pid":100,"run":1})])
        rejected("sample-after-closed-run",lambda:checked(log,["LR01"]))
        lines(log,good+[("LR_SAMPLE",{"tc":"LR01","pid":100,"run":2})])
        rejected("new-run-sample-cannot-reuse-old-pass",lambda:checked(log,["LR01"]))
        latest=case(run=2)
        lines(log,good+latest[:-1]);rejected("older-end-cannot-close-new-run",lambda:checked(log,["LR01"]))
        changed=clone(good);changed[-1][1]["reason"]="reset";lines(log,changed)
        passed("complete-run-cleanup-through-reset",checked(log,["LR01"])["status"]=="observations-valid")
        cleanup_error=("LR_CLEANUP_ERROR",{"tc":"LR01","pid":100,"run":1,"scenario":"s1"})
        lines(log,good[:-1]+[cleanup_error]+good[-1:])
        rejected_with("cleanup-error-marker-overrides-pass",lambda:checked(log,["LR01"]),"cleanup failure")
        lines(log,[("LR_CLEANUP_ERROR",{"tc":"LR01","pid":100,"run":1,"scenario":"s0"})]+case(run=2))
        passed("cleanup-error-of-other-run-ignored",checked(log,["LR01"])["status"]=="observations-valid")
        lines(log,good,'noise LR_CHECK {"tc":"LR01","pid":100,"run":1}\n')
        rejected_with("embedded-record-marker-rejected",lambda:checked(log,["LR01"]),"record marker not at line start")
        lines(log,good)
        cli("complete-log-cli-zero",["check-log",str(log),"--tc","LR01"]+contract_cli,0)
        lines(log,good[:-1]+[clone(good[2])]+good[-1:])
        passed("identical-frozen-page-repeat", checked(log,["LR01"])["status"] == "observations-valid")
        bad=clone(good);bad=[entry for entry in bad if entry[1].get("scenario") != "s1" or entry[0]=="LR_RESULT"]
        bad[-2][1].update(completed_steps=1,completed_scenarios=1,required_scenarios=1,checks=2)
        lines(log,bad);rejected("lowered-summary-cannot-hide-scenario",lambda:checked(log,["LR01"]))
        lines(log,good[1:]);rejected("missing-immutable-contract",lambda:checked(log,["LR01"]))
        bad=clone(good);bad.pop(2);lines(log,bad);rejected("missing-result-page",lambda:checked(log,["LR01"]))
        cli("missing-page-cli-invalid",["check-log",str(log),"--tc","LR01"]+contract_cli,2)
        bad=clone(good);bad[3][1]["id"]="width";lines(log,bad);rejected("duplicate-assertion-cannot-fill-count",lambda:checked(log,["LR01"]))
        bad=clone(good);bad[3][1]["row"]=3;lines(log,bad);rejected("noncontiguous-row-numbers",lambda:checked(log,["LR01"]))
        bad=clone(good);bad[2][1]["actual"]="nan";lines(log,bad);rejected("passed-flag-cannot-hide-nan",lambda:checked(log,["LR01"]))
        bad=clone(good);bad[2][1].update(kind="text",actual="01",expected="1",tolerance=0);lines(log,bad);rejected("text-log-is-not-numeric",lambda:checked(log,["LR01"]))
        # A FAILING log is valid evidence: every structural invariant is still evaluated and the
        # failing rows are named, instead of the first one aborting the whole validation.
        failing=clone(good)
        failing[2][1].update(actual="2",expected="1",tolerance=.001,passed=False)
        failing[-2][1].update(failures=1,verdict="FAIL");failing[-1][1].update(failures=1)
        lines(log,failing)
        report=checked(log,["LR01"])
        passed("complete-failing-log-valid-with-named-failures",
               report["status"]=="observations-valid-with-failures" and
               [row["id"] for row in report["failing_rows"]["LR01"]]==["width"])
        cli("failing-log-cli-nonzero",["check-log",str(log),"--tc","LR01"]+contract_cli,1)
        truncated=[entry for entry in clone(failing)
                   if entry[1].get("scenario")!="s1" and entry[0] not in ("LR_RESULT","LR_CASE_END")]
        stopped=dict(base_scope,scenario="s0",completed_steps=1,completed_scenarios=1,required_scenarios=2,
                     checks=2,failures=1,verdict="FAILING")
        truncated.append(("LR_RESULT",stopped))
        truncated.append(("LR_CASE_END",dict(base_scope,completed_steps=1,completed_scenarios=1,
                                             required_scenarios=2,failures=1,cleanup_passed=True,reason="exit")))
        lines(log,truncated)
        rejected_with("truncated-failing-log-invalid-for-completion",lambda:checked(log,["LR01"]),"missing scenario")
        bad=clone(failing);bad[-2][1]["failures"]=2;bad[-1][1]["failures"]=2;lines(log,bad)
        rejected_with("failing-summary-mismatch",lambda:checked(log,["LR01"]),"summary failures differ from failing rows")
        early=dict(base_scope,scenario="s0",completed_steps=1,completed_scenarios=1,required_scenarios=2,
                   checks=2,failures=0,verdict="PASS")
        lines(log,good[:4]+[("LR_RESULT",early)]+good[4:])
        rejected_with("terminal-verdict-before-completion",lambda:checked(log,["LR01"]),"terminal verdict before completion")
        lines(log,good)
        lines(log,good,"LR_CHECK\n");rejected("truncated-record-after-complete-run",lambda:checked(log,["LR01"]))
        lines(log,good,"LR_CHE\n");rejected("truncated-record-marker",lambda:checked(log,["LR01"]))
        lines(log,good,'LR_CHECK {"tc":\n');rejected("truncated-json-after-complete-run",lambda:checked(log,["LR01"]))
        bad=clone(good);changed=clone(good[2]);changed[1]["actual"]="1.0004";lines(log,bad[:-1]+[changed]+bad[-1:]);rejected("changed-frozen-row",lambda:checked(log,["LR01"]))
        new=case(run=2);lines(log,good+[new[0]]);rejected("old-pass-cannot-hide-new-incomplete-run",lambda:checked(log,["LR01"]))
        lines(log,[good[0]]+new);passed("reset-selects-newest-run",checked(log,["LR01"])["status"]=="observations-valid")
        bad=clone(good);bad[-2][1]["verdict"]="EXTERNAL_REQUIRED";lines(log,bad);rejected("external-evidence-required-by-default",lambda:checked(log,["LR01"]))
        passed("external-mode-remains-pending",checked(log,["LR01"],external=True)["external_verification_pending"])
        cli("external-pending-cli-nonzero",["check-log",str(log),"--tc","LR01","--external"]+contract_cli,1)

        # The frame fence. An abnormal completion writes render.frame-timeout, so the action is
        # short of its declared rows and protocol.required_checks fires too. The LR_FRAME record
        # carries the reason; it is not a row, and it must never appear without its failing row.
        frame_scope=dict(base_scope,scenario="s1",step="run",action_seq=2)
        frame_record={"result":"timeout","reason":"frame watchdog reached the total deadline",
                      "attempts":3,"elapsed_ms":4001.2,"deadline_ms":4000}

        def frame_log(*, failing=True, record=True, updates=None, extra=""):
            entries=clone(good)
            if failing:
                entries[5]=["LR_CHECK",dict(frame_scope,row=1,id="render.frame-timeout",kind="failure",
                                            actual="frame watchdog reached the total deadline",expected="a fresh frame",tolerance=0,passed=False)]
                entries[6]=["LR_CHECK",dict(frame_scope,row=2,id="protocol.required_checks",kind="failure",
                                            actual="0",expected="2",tolerance=0,passed=False)]
                entries[-2][1].update(failures=2,verdict="FAIL")
                entries[-1][1].update(failures=2)
            if record:
                entries.insert(7,["LR_FRAME",dict(frame_scope,**dict(frame_record,**(updates or {})))])
            lines(log,entries,extra)

        frame_log()
        report=checked(log,["LR01"])
        passed("frame-record-accepted-with-failure-row",
               report["status"]=="observations-valid-with-failures" and
               [row["id"] for row in report["failing_rows"]["LR01"]]==["render.frame-timeout","protocol.required_checks"] and
               len(report["frame_failures"]["LR01"])==1 and
               report["frame_failures"]["LR01"][0]["action_seq"]==2 and
               report["frame_failures"]["LR01"][0]["result"]=="timeout")
        frame_log(failing=False)
        rejected_with("frame-record-without-failure-row-rejected",lambda:checked(log,["LR01"]),
                      "frame failure record without its render.frame-timeout row")
        frame_log(updates={"result":"unknown"})
        rejected_with("frame-record-malformed-rejected",lambda:checked(log,["LR01"]),"malformed frame record")
        frame_log(record=False,extra='noise LR_FRAME {"tc":"LR01","pid":100,"run":1}\n')
        rejected_with("frame-marker-mid-line-rejected",lambda:checked(log,["LR01"]),"record marker not at line start")
        lines(log,[("LR_FRAME",dict(frame_scope,**frame_record))]+case(run=2))
        passed("frame-record-of-other-run-ignored",checked(log,["LR01"])["status"]=="observations-valid")
        lines(log,good)
        plan={"schema":1,"regression_margin":0,"alpha":.1,"looks":[31,62,93],"seed":7,"bootstrap_draws":2400,"precision":.01,"mde":.02,"timer_floor_ns":100,"metrics":["LR59.synthetic"]}
        plan.update(contract_manifest=manifest_path.name,contract_manifest_sha256=manifest_pin,metric_family="synthetic")
        plan_path=root/"plan.json";experiment_path=root/"experiment.json"
        plan_path.write_text(json.dumps(plan))
        artifact={"app_sha256":"a"*64,"library_sha256":"b"*64,"fixture_contract_sha256":"c"*64,"capture_active_during_timing":False,"coverage":False,"build_type":"Release"}
        environment={"device":"synthetic","cpu_affinity":[0],"governor":"fixed","resolution":[240,160],"dpi":96,"assets_sha256":"d"*64,"compiler_flags":"-O3"}
        experiment={"plan_sha256":digest(plan_path),"blocks":[]}
        for index in range(31):
            block={"pair_id":index,"order":"AB" if index%2==0 else "BA","environment_reference":environment,"environment_candidate":environment,"reference_artifact":artifact,"candidate_artifact":artifact}
            for side in ("reference","candidate"):
                pid=1000+index*2+(side=="candidate")
                entries=case(pid=pid,run=pid*10,scenarios=1,tc="LR59")
                scope={"tc":"LR59","pid":pid,"run":pid*10,"scenario":"s0","step":"run","action_seq":1}
                samples=[("LR_SAMPLE",dict(scope,profile="release",metric="LR59.synthetic",iteration=i,nanoseconds=100000.0,operations=100)) for i in range(31)]
                path=root/f"{index}-{side}.log";lines(path,entries[:2]+samples+entries[2:]);block[side+"_log"]=path.name
            experiment["blocks"].append(block)
        experiment_path.write_text(json.dumps(experiment))
        passed("independent-identical-performance-pass",compare(plan_path,experiment_path)["results"][0]["verdict"]=="PASS_AT_DECLARED_RESOLUTION")
        cli("precise-identical-performance-cli-zero",["compare",str(plan_path),str(experiment_path)],0)
        passed("complete-reviewed-metric-family", validate_plan_metrics(plan, plan_path) == manifest_path)
        for name, updates in [
            ("plan-omitted-phase", {"metrics": []}),
            ("plan-omitted-entire-case", {"metric_family": "two_cases"}),
            ("plan-obsolete-phase", {"metrics": ["LR59.first_measure"]}),
            ("plan-extra-phase", {"metrics": ["LR59.synthetic", "LR59.other"]}),
            ("plan-duplicate-phase", {"metrics": ["LR59.synthetic", "LR59.synthetic"]}),
            ("plan-missing-family", {"metric_family": None}),
            ("plan-unreviewed-family", {"metric_family": "unknown"}),
        ]:
            candidate = dict(plan, **updates)
            rejected(name, lambda: validate_plan_metrics(candidate, plan_path))
        for name, mutate in [
            ("metric-contract-missing", lambda value: value.pop("reviewed_performance_metrics")),
            ("metric-contract-invalid-schema", lambda value: value["reviewed_performance_metrics"].update(schema=True)),
            ("metric-contract-unreviewed", lambda value: value["reviewed_performance_metrics"]["families"]["synthetic"].update(reviewed=False)),
            ("metric-contract-duplicate", lambda value: value["reviewed_performance_metrics"]["families"]["synthetic"]["metrics"].append("LR59.synthetic")),
        ]:
            candidate = clone(manifest); mutate(candidate)
            manifest_path.write_text(json.dumps(candidate))
            rejected(name, lambda: reviewed_metrics(manifest_path, digest(manifest_path), "synthetic"))
        manifest_path.write_text(json.dumps(manifest))
        # A contract document quoting a manifest digest that is no longer the manifest's own.
        stale_doc = root / "layout-validation-stale-map.md"
        stale_doc.write_text("표의 수치는 단일 계약(`layout-validation-manifest.json`, SHA-256 `%s`)에서 생성했습니다.\n" % ("0" * 64),
                             encoding="utf-8")
        rejected_with("doc-quoted-digest-must-match-manifest",
                      lambda: check_document_digests([stale_doc], manifest_pin),
                      "document cites a stale manifest SHA-256")

        def fail_performance_row(path, row_id="width", faster=False):
            values = list(records(path))
            for kind, value in values:
                if kind == "LR_CHECK" and value["id"] == ("width" if row_id == "width" else "count"):
                    value.update(id=row_id, actual="999", passed=False)
                elif kind == "LR_RESULT":
                    value.update(failures=1, verdict="FAIL")
                elif kind == "LR_CASE_END":
                    value.update(failures=1)
                elif faster and kind == "LR_SAMPLE":
                    value["nanoseconds"] *= .5
            lines(path, values)

        for side, row_id, faster in [("candidate", "width", False), ("reference", "width", False),
                                      ("candidate", "work.measure.producers", True)]:
            path = root / ("0-" + side + ".log")
            original_failure_log = path.read_text()
            fail_performance_row(path, row_id, faster)
            report = compare(plan_path, experiment_path)
            failure = report["correctness_failures"][0]
            passed("performance-correctness-failure-" + side + "-" + row_id,
                   report["status"] == "correctness-failed" and not report["results"] and
                   len(report["correctness_failures"]) == 1 and failure["pair_id"] == 0 and
                   failure["side"] == side and failure["log"] == path.name and
                   [row["id"] for row in failure["failing_rows"]["LR59"]] == [row_id])
            cli("performance-correctness-cli-" + side + "-" + row_id,
                ["compare", str(plan_path), str(experiment_path)], 1)
            # An early valid failure must not hide a malformed log in the last pair.
            late = root / "30-candidate.log"; late_original = late.read_text()
            late.write_text(late_original + "LR_CHECK\n")
            rejected_with("late-malformed-after-failure-" + side + "-" + row_id,
                          lambda: compare(plan_path, experiment_path), "truncated LR_CHECK")
            cli("late-malformed-after-failure-cli-" + side + "-" + row_id,
                ["compare", str(plan_path), str(experiment_path)], 2)
            late.write_text(late_original)
            path.write_text(original_failure_log)
        failed_paths = [root / "0-reference.log", root / "30-candidate.log"]
        originals = [path.read_text() for path in failed_paths]
        for path in failed_paths: fail_performance_row(path)
        report = compare(plan_path, experiment_path)
        passed("all-pair-side-failures-preserved", [(item["pair_id"], item["side"])
               for item in report["correctness_failures"]] == [(0, "reference"), (30, "candidate")])
        for path, original in zip(failed_paths, originals): path.write_text(original)
        # A TC the plan's metric family does not name is never inspected by check_log, so its
        # contract and its failures would be invisible. It also means unplanned work shared the
        # timed process. Both readings make the log invalid evidence, passing or failing.
        foreign_path = root / "0-candidate.log"
        foreign_original = foreign_path.read_text()
        foreign_entries = list(records(foreign_path))
        foreign_pass = case(pid=1001, run=10011, scenarios=1, tc="LR60")
        lines(foreign_path, foreign_entries + foreign_pass)
        rejected_with("performance-log-foreign-tc-rejected", lambda: compare(plan_path, experiment_path),
                      "outside the plan's metric family")
        foreign_fail = clone(foreign_pass)
        foreign_fail[2][1].update(actual="2", expected="1", tolerance=.001, passed=False)
        foreign_fail[-2][1].update(failures=1, verdict="FAIL"); foreign_fail[-1][1].update(failures=1)
        lines(foreign_path, foreign_entries + [tuple(entry) for entry in foreign_fail])
        rejected_with("performance-log-foreign-failing-tc-not-hidden", lambda: compare(plan_path, experiment_path),
                      "outside the plan's metric family")
        foreign_path.write_text(foreign_original)

        def bad_experiment(name, mutate):
            candidate=clone(experiment);mutate(candidate);experiment_path.write_text(json.dumps(candidate))
            rejected(name,lambda:compare(plan_path,experiment_path))
            experiment_path.write_text(json.dumps(experiment))

        bad_experiment("copied-log-is-not-independent",lambda e:e["blocks"][1].update(reference_log=e["blocks"][0]["reference_log"]))
        bad_experiment("duplicate-pair-ID",lambda e:e["blocks"][1].update(pair_id=e["blocks"][0]["pair_id"]))
        bad_experiment("plan-fingerprint-mismatch",lambda e:e.update(plan_sha256="0"*64))
        for field in ("contract_manifest", "contract_manifest_sha256"):
            invalid=clone(plan);invalid.pop(field);plan_path.write_text(json.dumps(invalid))
            experiment_path.write_text(json.dumps(dict(experiment,plan_sha256=digest(plan_path))))
            rejected("performance-plan-missing-"+field,lambda:compare(plan_path,experiment_path))
        plan_path.write_text(json.dumps(plan));experiment_path.write_text(json.dumps(experiment))
        bad_experiment("incomplete-pair-count",lambda e:e["blocks"].pop())
        bad_experiment("environment-mismatch",lambda e:e["blocks"][0]["environment_candidate"].update(governor="different"))
        bad_experiment("artifact-profile-mismatch",lambda e:e["blocks"][0]["candidate_artifact"].update(coverage=True))
        original=(root/"1-reference.log").read_text()
        altered=[(kind,dict(value,pid=1000)) for kind,value in records(root/"1-reference.log")]
        lines(root/"1-reference.log",altered);rejected("same-process-different-run-is-not-independent",lambda:compare(plan_path,experiment_path));(root/"1-reference.log").write_text(original)
        original=(root/"0-candidate.log").read_text()
        for name,mutate in [
            ("sample-operation-mismatch",lambda value:value.update(operations=101)),
            ("unplanned-metric-family",lambda value:value.update(metric="LR59.other")),
            ("sample-NaN",lambda value:value.update(nanoseconds=float("nan"))),
            ("sample-Inf",lambda value:value.update(nanoseconds=float("inf"))),
            ("sample-below-timer-floor",lambda value:value.update(nanoseconds=1)),
            ("sample-action-not-declared",lambda value:value.update(action_seq=2)),
        ]:
            values=list(records(root/"0-candidate.log"))
            for kind,value in values:
                if kind=="LR_SAMPLE":mutate(value)
            lines(root/"0-candidate.log",values);rejected(name,lambda:compare(plan_path,experiment_path));(root/"0-candidate.log").write_text(original)
        values=list(records(root/"0-candidate.log"));values=[entry for entry in values if entry[0]!="LR_SAMPLE" or entry[1]["iteration"]!=30]
        lines(root/"0-candidate.log",values);rejected("missing-post-warmup-sample",lambda:compare(plan_path,experiment_path));(root/"0-candidate.log").write_text(original)
        for field,value in [("alpha",0),("alpha",float("nan")),("precision",float("inf")),("precision",.03),("mde",float("inf")),("bootstrap_draws",10),("seed",True),("timer_floor_ns",0),("metrics",["LR59.synthetic","LR59.synthetic"])]:
            invalid=clone(plan);invalid[field]=value;plan_path.write_text(json.dumps(invalid));experiment_path.write_text(json.dumps(dict(experiment,plan_sha256=digest(plan_path))))
            rejected("invalid-plan-"+field+"-"+str(value),lambda:compare(plan_path,experiment_path))
        plan_path.write_text(json.dumps(plan));experiment_path.write_text(json.dumps(experiment))
        # Independent processes with a tiny but identical slowdown must FAIL,
        # even when the increase is below the declared detection limit.
        for index in range(31):
            path=root/f"{index}-candidate.log";values=list(records(path))
            for kind,value in values:
                if kind=="LR_SAMPLE":value["nanoseconds"]*=1.0001
            lines(path,values)
        passed("tiny-significant-slowdown-fails-before-mde",compare(plan_path,experiment_path)["results"][0]["verdict"]=="FAIL")
        cli("tiny-slowdown-cli-nonzero",["compare",str(plan_path),str(experiment_path)],1)
        # The same claim on data that is NOT degenerate: five distinct slowdown factors give the
        # paired log ratios a real spread, so the bootstrap interval has a nonzero width.
        for index in range(31):
            path=root/f"{index}-candidate.log";values=list(records(path))
            factor=1.0001+0.00002*((index%5)-2)
            for kind,value in values:
                if kind=="LR_SAMPLE":value["nanoseconds"]=100000.0*factor
            lines(path,values)
        passed("small-slowdown-with-variance-fails",compare(plan_path,experiment_path)["results"][0]["verdict"]=="FAIL")
        for index in range(31):
            path=root/f"{index}-candidate.log";values=list(records(path))
            for kind,value in values:
                if kind=="LR_SAMPLE":value["nanoseconds"]=100000.0
            lines(path,values)
        for index in range(31):
            path=root/f"{index}-candidate.log";values=list(records(path))
            for kind,value in values:
                if kind=="LR_SAMPLE":value["nanoseconds"]=100000.0*(.5 if index%2 else 2)
            lines(path,values)
        passed("wide-confidence-interval-does-not-pass",compare(plan_path,experiment_path)["results"][0]["verdict"]=="MORE_PAIRS_REQUIRED")
    return {"status":"self-test-pass","checks":len(checks),"cases":checks}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("audit")
    sub.add_parser("self-test")
    check = sub.add_parser("check-log")
    check.add_argument("log")
    check.add_argument("--tc", action="append", required=True)
    check.add_argument("--external", action="store_true")
    check.add_argument("--manifest", default=str(HERE / "layout-validation-manifest.json"))
    check.add_argument("--manifest-sha256", help="Required SHA-256 pinned in a trusted source outside the candidate log")
    check.add_argument("--profile", choices=["diagnostic", "release"], help="Required reviewed contract profile")
    perf = sub.add_parser("compare")
    perf.add_argument("plan")
    perf.add_argument("experiment")
    mini = sub.add_parser("shrink-candidates")
    mini.add_argument("failure")
    args = parser.parse_args()
    try:
        if args.command == "audit": result = audit()
        elif args.command == "self-test": result = self_test()
        elif args.command == "check-log": result = check_log(args.log, args.tc, args.external,
                                                             manifest_path=args.manifest, manifest_sha256=args.manifest_sha256, profile=args.profile)
        elif args.command == "compare": result = compare(args.plan, args.experiment)
        else: result = shrink(args.failure)
        print(json.dumps(result, indent=2, ensure_ascii=False, allow_nan=False))
        # Exit 0: nothing to act on. Exit 1: the evidence is valid and says something is
        # wrong or still owed (failing rows, an external verification, a timing regression).
        # Exit 2: the evidence itself could not be validated -- see the except branch.
        if result.get("failing_rows") or result.get("correctness_failures"):
            return 1
        if result.get("external_verification_pending"):
            return 1
        if any(r["verdict"] != "PASS_AT_DECLARED_RESOLUTION" for r in result.get("results", [])):
            return 1
        return 0
    except (OSError, ValueError, KeyError, TypeError, OverflowError) as error:
        print(json.dumps({"status":"invalid-evidence","error":str(error)}, ensure_ascii=False))
        return 2


if __name__ == "__main__":
    sys.exit(main())
