"""Seal actual selected thermal lifecycle hooks and focused original-world reads.

@file native_thermal_lifecycle_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md15D.
Closed paid producer receipts retain full-chain authority; current samples never rebuild them.
"""

import argparse
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from native_thermal_lifecycle import CASES
from native_lifecycle_report import count_native
from local_electrical_feed_checks import identity
from tube_sprite_report import verify_references, compare_worlds
from lifecycle_revision_log import compare_original, saved_table

ORIGINAL_CASES = ("retired-3", "service-4", "service-8")


def accept(selection, check_log, output):
    """Validate both current native owner matrices and exact paid-original samples.

    @param selection Explicit complete thirty-eight-report current input index.
    @param check_log Successful focused revision-log corruption unittest output.
    @param output Absent immutable acceptance receipt beneath repository logs.
    @return None; unchanged original gameplay fields and strict diagnostics are mandatory.
    """
    output = (ROOT / output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    index = json.loads((ROOT / selection).read_text(encoding="utf-8"))
    assert index["complete"]
    audit_path = ROOT / "logs/lifecycle/owner-return/20261018-native-01/thermal-entry-audit.json"
    audit = json.loads(audit_path.read_text(encoding="utf-8"))
    for reference in audit["production_files"]:  # This return changes observation code only. A changed native claim/machinery/accounting hook would require its own new producer acceptance.
        assert identity(ROOT / reference["path"]) == reference
    original_closure = json.loads((ROOT / audit["accepted_original_paid_receipts"][-1]["path"]).read_text(encoding="utf-8"))
    manifest_ref = original_closure["manifest"]
    assert identity(ROOT / manifest_ref["path"]) == manifest_ref
    manifest = json.loads((ROOT / manifest_ref["path"]).read_text(encoding="utf-8"))
    cfgs = ("Debug", "RelWithDebInfo")
    counts = {cfg: {"cases": 0, "assertions": 0} for cfg in cfgs}
    grid, cache, records, diagnostic_appends = {}, {}, [], []
    next_comparisons = 0
    for observation in index["records"]:  # Every named actual owner and each selected original state must occur once per reader; an easy repeated case cannot fill a different producer boundary.
        path = ROOT / observation["report"]
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in cfgs and cfg == observation["configuration"]
        assert report["binary"] == identity(ROOT / f"build/{cfg}/openttd_test.exe")
        if "focus" in observation:
            role = observation["focus"]
            assert role in CASES and report["focus"] == role and report["tag"] == CASES[role]
            assert report["controller"] == identity(ROOT / "src/tests/multilayer/native_thermal_lifecycle.py")
            count_native(report, counts[cfg])
        else:
            role = observation["original_case"]
            assert role in ORIGINAL_CASES and report["phase"] == "read" and len(report["results"]) == 1
            assert report["manifest"] == manifest_ref
            assert report["controller"] == identity(ROOT / "src/tests/multilayer/utilities/plant_lifecycle_files.py")
            result = report["results"][0]
            assert result["case"] == role
            opposite = cfgs[1 - cfgs.index(cfg)]
            assert result["original"] == manifest["cases"][role]["writers"][opposite]
            for iteration in ("first", "second"):  # Independent repeated ordinary loads preserve complete native gameplay state. A build-revision append is checked field by field, never dropped as a whole diagnostic chunk.
                proof = result[iteration]
                count_native(proof, counts[cfg])
                tick = saved_table(ROOT / proof["input"]["path"], b"DATE")[0]["tick_counter"]
                fresh = compare_original(proof["input"], proof["outputs"]["read"]["input"], tick)
                assert proof["original_read_comparison"] == fresh
                assert compare_worlds(proof["outputs"]["read"]["input"], proof["outputs"]["repeat"]["input"]) == 126
                if fresh["revision_append"] is not None:
                    diagnostic_appends.append({"configuration": cfg, "case": role, "iteration": iteration, "boundary": "read", **fresh})
                if proof["baseline"] is not None:
                    following = compare_original(proof["baseline"], proof["outputs"]["next"]["input"], tick)
                    assert proof["original_next_comparison"] == following
                    assert compare_worlds(proof["outputs"]["next"]["input"], proof["outputs"]["next-repeat"]["input"]) == 126
                    next_comparisons += 1
                    if following["revision_append"] is not None:
                        diagnostic_appends.append({"configuration": cfg, "case": role, "iteration": iteration, "boundary": "next", **following})
        assert (cfg, role) not in grid
        grid[cfg, role] = report
        records.append({"report": identity(path), **report})
    assert grid.keys() == {(cfg, role) for cfg in cfgs for role in (*CASES, *ORIGINAL_CASES)}
    assert counts["Debug"] == counts["RelWithDebInfo"]
    pairs = []
    for role in ORIGINAL_CASES:  # Both current readers must produce byte-identical complete worlds, including the same checked revision diagnostic, at every repeated original read and full next opportunity.
        for iteration in ("first", "second"):
            first, second = [grid[cfg, role]["results"][0][iteration] for cfg in cfgs]
            boundaries = ("read", "next") if first["baseline"] is not None else ("read",)
            for boundary in boundaries:
                a, b = first["outputs"][boundary]["input"], second["outputs"][boundary]["input"]
                assert compare_worlds(a, b) == 126
                pairs.append({"case": role, "iteration": iteration, "boundary": boundary, "left": a, "right": b, "complete_chunks_exact": 126})
    assert len(pairs) == 10 and next_comparisons == 8
    checks = (ROOT / check_log).read_text(encoding="utf-8")
    assert "Ran 5 tests" in checks and re.search(r"(?m)^OK\s*$", checks)
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md15D.2;actual thermal lifecycle/hooks and selected original paid-world continuation;strict native revision diagnostics;no human/network/future-factory acceptance",
              "controller": identity(__file__), "selection": identity(ROOT / selection), "entry_audit": identity(audit_path), "summary": counts,
              "native_cases": sum(row["cases"] for row in counts.values()), "native_assertions": sum(row["assertions"] for row in counts.values()),
              "records": records, "original_manifest": manifest_ref, "accepted_original_paid_receipts": audit["accepted_original_paid_receipts"],
              "cross_reader_world_pairs": pairs, "original_next_comparisons": next_comparisons, "strict_revision_diagnostic_appends": diagnostic_appends,
              "revision_checks": identity(ROOT / check_log), "revision_check_source": identity(ROOT / "src/tests/multilayer/lifecycle_revision_checks.py"),
              "revision_validator": identity(ROOT / "src/tests/multilayer/lifecycle_revision_log.py"), "exact_artifact_identities": len(cache), "native_version": 434,
              "artwork": identity(ROOT / "media/baseset/openttd.grf"), "production_cpp_changed": False,
              "limits": "Native originals retain all125 non-GLOG complete chunks and an independently exact single Load/Revision append when the committed build identity changes. Repeats/cross-reader worlds retain all126 chunks byte-exact. Closed26-state native/production acceptance retains full paid-chain authority;these three states are current focused samples.",
              "remaining": "15E selected lifecycle policy/master9.5B,then10.8 actualpaidTBM/tube final use;wholelater-profile/orbital/network/operator/human gates remain pending."}
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "cases": result["native_cases"], "assertions": result["native_assertions"], "world_pairs": len(pairs), "next_comparisons": next_comparisons}), flush=True)


def main():
    """Seal the explicit completed selected native thermal lifecycle observation index.

    @return None; no native process or public metadata mutation occurs.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--check-log", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.check_log, args.output)


if __name__ == "__main__":
    main()
