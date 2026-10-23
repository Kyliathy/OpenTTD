"""Seal the selected ground performance scope from exact, independently closed receipts.

@file ground_performance_acceptance.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21F and master11.6.
Historical observer/binary identities remain dated; physical artifacts and production stay exact.
No native process is launched and no previous receipt or gameplay state is changed.
"""

import argparse
import hashlib
import json
import re
import sys
import unittest
from pathlib import Path

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from support.save_tables import chunks, read_stream

DIRECTORY = ROOT / "logs/performance/ground-maps/20261019-native-01"
RECEIPTS = {
    "ordinary-baseline": "27c6131b6952f0174b9155d6fdc7d484fb92c46dbc90c69ac63a9200d68db264",
    "original-performance": "5cd7cfe3987218597147bfdbcadc4a596cde33d28fc279ac96e8f309825063bc",
    "dense-owner": "3852f24153ff5c60eeab42bb29df37d8b1a77d482d074b95f00a01001d408b04",
    "dense-planning": "c53a31e13ee4a97b074b8bff789ad5a421547586e7ea0ffeae84800dc4436a87",
    "utility-performance": "2bc8deb7260e7ba2963826db28e1f4836e99626dd83ead42a31d04dbe9586d9b",
    "tube-performance": "956f67a0bdbc849f7ef61b495487edf8dc261e08406247f3527fff4319008e9e",
    "view-performance": "bec97d35e4f2822d2a9e08af62d0307a3b37e4a949f4ddc4498edad43d253a78",
    "file-performance": "10d63ef739d31bbb98230dde0505ae73d30a7ae5d079dafabd003d78dbdf0adf",
    "network-performance": "e741123dc468cb20b0d101cd5855b0de2f918a55df29712093c1e4f99318e617",
}


def dictionaries(value):
    """Yield nested metadata without reopening historical mutable-source validators."""
    if isinstance(value, dict):
        yield value
        for child in value.values():
            yield from dictionaries(child)
    elif isinstance(value, list):
        for child in value:
            yield from dictionaries(child)


def dated(reference):
    """Only binaries and non-fixture test implementations may retain dated bytes."""
    path = reference["path"].replace("\\", "/")
    return path.startswith("build/") or (path.startswith("src/tests/") and not path.startswith("src/tests/fixtures/"))


def strict_reference(reference):
    """Stream-hash a complete immutable artifact or current production owner."""
    path = (ROOT / reference["path"]).resolve()
    assert path.is_relative_to(ROOT), reference
    assert path.stat().st_size == reference["bytes"], reference
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    assert digest.hexdigest() == reference["sha256"], reference
    return path


def native_counts(native, unique):
    """Count actual successful invocations once, checking their retained Catch log."""
    path = strict_reference(native["log"])
    data = path.read_bytes()
    text = data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
    match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    counts = native.get("assertions", native.get("native_assertions")), native.get("cases", native.get("native_cases"))
    assert match and tuple(map(int, match.groups())) == counts and all(counts)
    assert native["exit_status"] == 0
    key = native["log"]["path"]
    if key in unique:
        assert unique[key] == counts
    unique[key] = counts


def bound(output, source, metric, observed, limit):
    """Compare a measured quantity with its predeclared limit, preserving both."""
    assert 0 <= observed <= limit, (source, metric, observed, limit)
    output.append({"report": source, "metric": metric, "observed": observed, "limit": limit, "fraction": observed / limit})


def compare_budget(actual, baseline, source):
    """Reconcile every measured owner category against the original frozen budgets."""
    rows = []
    axis = 512 if "connections" in actual else max(128, actual.get("axis", actual.get("source_counts", {}).get("width", 128)))
    candidates = [r for r in baseline["budgets"] if r["configuration"] == actual["configuration"] and r["axis"] <= axis]
    budget = max(candidates, key=lambda row: row["axis"])
    assert actual.get("budget", budget) == budget
    assert actual.get("budget_excesses", []) == [] and actual["children_reaped"] is True
    assert actual["failure"] is None and actual["status"] == "observed"
    peaks = actual["peak_commit_bytes"]
    for role, peak in (peaks.items() if isinstance(peaks, dict) else [("process", peaks)]):
        bound(rows, source, role + ".peak_commit_bytes", peak, budget["peak_commit_bytes"])
    timing = actual.get("timings", {})
    if "simulation_ns" in timing:
        bound(rows, source, "simulation.p99_ns", timing["simulation_ns"]["p99_ns"], budget["simulation_p99_ns"])
        bound(rows, source, "simulation.maximum_ns", timing["simulation_ns"]["maximum_ns"], budget["simulation_max_ns"])
        for save in actual["synchronous_saves"]:
            bound(rows, source, "save.ns", save["synchronous_ns"], budget["save_ns"])
        for command in actual["commands"]:
            bound(rows, source, "command.ns", command["synchronous_ns"], budget["command_p99_ns"])
        first = actual.get("first_resumed_work")
        bound(rows, source, "load_and_first_work.ns" if first else "startup_to_ready.ns",
              first[0]["load_and_first_work_ns"] if first else actual["ready"]["startup_to_ready_ns"], budget["startup_or_load_ns"])
    elif "p99_ns" in timing:
        bound(rows, source, actual["kind"] + ".p99_ns", timing["p99_ns"], budget["command_p99_ns" if actual["kind"] == "command" else "route_p99_ns"])
    else:
        for operation, values in timing.items():
            bound(rows, source, operation + ".p99_ns", values["p99_ns"], budget["command_p99_ns" if operation == "reserve-estimate" else "simulation_p99_ns"])
            if "connections" in actual:
                bound(rows, source, operation + ".maximum_ns", values["maximum_ns"], budget["simulation_max_ns"])
    complete = actual.get("complete", [{}])[0]
    for field in ("setup_ns", "unused_expiry_ns", "retirement_ns"):
        if field in complete:
            bound(rows, source, field, complete[field], budget["startup_or_load_ns" if field == "setup_ns" else "command_p99_ns"])
    if "join_upper_bound_ns" in actual:
        bound(rows, source, "join_upper_bound_ns", actual["join_upper_bound_ns"], budget["join_reconstruction_ns"])
    return rows


def wire_ownership(original):
    """Attribute compact original file owners; serialized bytes are explicitly not heap sizes."""
    result = []
    for record in original["records"]:
        reference = record["actual"]["saves"]["final-world"]
        version, stream = read_stream(strict_reference(reference))
        owners = [{"owner": row["name"].decode("ascii"), "wire_bytes": len(row["raw"]), "records": len(row["rows"])} for row in chunks(stream)]
        assert version == 434 and len(owners) == 126 and sum(row["wire_bytes"] for row in owners) + 4 == len(stream)
        result.append({"configuration": record["configuration"], "stage": record["stage"], "save": reference,
                       "compressed_bytes": reference["bytes"], "stream_bytes": len(stream), "owners": owners})
    return result


def accept(output, checks):
    """Audit selected receipts and seal scope, measured limits and all retained failed attempts."""
    receipts, references, historical, strict, native, metrics, checks_seen = {}, {}, {}, {}, {}, [], {}
    for name, expected in RECEIPTS.items():
        reference = identity(DIRECTORY / (name + "-verification.json"))
        assert reference["sha256"] == expected
        receipts[name] = json.loads(strict_reference(reference).read_text(encoding="utf-8"))
        references[name] = reference
    baseline = receipts["ordinary-baseline"]
    for name, receipt in receipts.items():
        for value in dictionaries(receipt):
            if {"path", "bytes", "sha256"}.issubset(value):
                key = value["path"], value["bytes"], value["sha256"]
                (historical if dated(value) else strict)[key] = value
        for record in receipt.get("records", []):
            actual = record["actual"]
            assert json.loads(strict_reference(record["report"]).read_text(encoding="utf-8")) == actual
            peers = actual["native"]
            for child in (peers.values() if "server" in peers else [peers]):
                native_counts(child, native)
            metrics.extend(compare_budget(actual, baseline, record["report"]))
        for record in receipt.get("regressions", []):
            actual = record["actual"]
            assert actual["children_reaped"] is True
            native_counts(actual.get("native", actual), native)
        checks_ref = receipt.get("focused_checks")
        if checks_ref:
            checks_seen[checks_ref["log"]["path"]] = (checks_ref["log"], checks_ref["tests"])
    dense_check = receipts["dense-planning"]["dated_dense_focused_checks"]
    checks_seen[dense_check["path"]] = (dense_check, 6)
    for ref, count in checks_seen.values():
        data = strict_reference(ref).read_bytes()
        text = data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
        assert re.search(r"Ran " + str(count) + r" tests in [\d.]+s\s+OK\s*$", text)
    for reference in strict.values():
        strict_reference(reference)
    cases, assertions = sum(row[1] for row in native.values()), sum(row[0] for row in native.values())
    assert cases == 282 and sum(row[1] for row in checks_seen.values()) == 57
    failures = []
    for refusal in baseline["candidate_refusals"]:
        actual = json.loads(strict_reference(refusal["report"]).read_text(encoding="utf-8"))
        assert actual["children_reaped"] is True and actual["failure"] is not None
        failures.append({**refusal, "disposition": "outside110sworkflow;notacceptedmaporcompletednativecase"})
    assert len(failures) == 6
    prefix = json.loads(strict_reference(receipts["network-performance"]["historical_long_prefix"]).read_text(encoding="utf-8"))
    checks_ref = identity(ROOT / checks)
    check_data = strict_reference(checks_ref).read_bytes()
    check_text = check_data.decode("utf-16" if check_data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
    assert re.search(r"Ran 4 tests in [\d.]+s\s+OK\s*$", check_text)
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21F;selectedgroundonly;master11.6",
              "classification": "selected_ground_performance_acceptance", "accepted": True, "controller": identity(__file__),
              "receipts": references, "native_cases": cases, "native_assertions": assertions, "unique_native_invocations": len(native),
              "native_invocations": [{"log": key, "assertions": value[0], "cases": value[1]} for key, value in sorted(native.items())],
              "component_refusal_checks": 57, "aggregate_checks": {"log": checks_ref, "tests": 4},
              "strict_artifact_count": len(strict), "dated_test_binary_identity_count": len(historical),
              "dated_test_binary_bindings": list(historical.values()), "current_production_and_physical_artifacts": list(strict.values()),
              "frozen_budgets": baseline["budgets"], "measured_comparisons": metrics,
              "largest_completed_under_policy": baseline["largest_completed_under_policy"], "refused_map_attempts": failures,
              "retained_network_workflow_failure": prefix["retained_workflow_failure"],
              "owner_payload_observations": [{"receipt": name, "report": row["report"], "owners": row["actual"].get("owners"),
                                              "complete": row["actual"].get("complete")} for name, receipt in receipts.items() for row in receipt["records"]],
              "compact_serialized_owner_attribution": wire_ownership(receipts["original-performance"]),
              "diagnosis": ["No admitted selected workload exceeds its frozen acceptance budget;no production optimization required.",
                            "Six larger-map110s refusals remain failures,not engine hardmaximum4096 passes.",
                            "First Debug52 startup observer was repaused by ordinary newgame completion;observer scheduling corrected.",
                            "Old tube284761100ns observer-inclusive Debug timing retained;native batch now excludes separately recorded checks_ns;physics and budget unchanged.",
                            "Old Debug1024 network later512frame horizon exceeded110s despite32.743s completed join;focused32tick horizon retains six snapshots and finishes88.084s;45s join budget unchanged.",
                            "Optimized24 build guard stopped during extension-caused kernel pool pressure;user resolved extension;subsequent guarded builds passed. No game-owner RAM crash inferred."],
              "memory_scope": "OS lifetime private-commit/working peaks include startup/generation/read. Native packed-map estimates, sparse geology/index/job/utility retained payloads and actual entity/packet counts are separate. Eight compact file wire-owner byte tables are serialized transport attribution,not live allocator bytes;per-entity/packet heap allocator overhead is unassigned and full22.1 stayspending.",
              "uncertainty": "Single machine;five intervals;5heavyroutequeries p99=observedmax;uncontrolled desktop/cache;10ms samples plusOS lifetimepeaks;nullraster/noFPSclaim;largestadmittedworkflow!=enginehardmaximum;prepared dense/component work separate from actualoriginal/genuinebootstrap.",
              "remaining": ["whole mixed orbital/fleet/handling performance and full per-owner allocator attribution", "human art review/master11.7", "demo feedback/12.6 and full Plans3/4/M1 acceptance"]}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_cases": cases, "native_assertions": assertions,
                      "comparisons": len(metrics), "strict_artifacts": len(strict)}), flush=True)


class AcceptanceChecks(unittest.TestCase):
    """Reject changed physical bytes, false counts and raised acceptance quantities."""

    def test_changed_artifact_identity(self):
        reference = identity(DIRECTORY / "ordinary-baseline-verification.json")
        reference["sha256"] = "0" * 64
        with self.assertRaises(AssertionError):
            strict_reference(reference)

    def test_production_cannot_be_dated(self):
        for path in ("src/map.cpp", "src/tbm_project.cpp", "src/tests/fixtures/performance/ground-workloads.json", "mlm/reviews/world.sav"):
            self.assertFalse(dated({"path": path}))
        self.assertTrue(dated({"path": "src/tests/ground_performance_fixture.h"}))

    def test_count_cannot_be_inflated(self):
        receipt = json.loads((DIRECTORY / "ordinary-baseline-verification.json").read_text(encoding="utf-8"))
        native = dict(receipt["records"][0]["actual"]["native"])
        native["assertions"] += 1
        with self.assertRaises(AssertionError):
            native_counts(native, {})

    def test_one_unit_excess(self):
        rows = []
        bound(rows, {}, "test.ns", 45, 45)
        with self.assertRaises(AssertionError):
            bound(rows, {}, "test.ns", 46, 45)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--checks")
    parser.add_argument("--output")
    args = parser.parse_args()
    if args.self_test:
        unittest.main(argv=[sys.argv[0]])
    else:
        assert args.checks and args.output
        accept(args.output, args.checks)
