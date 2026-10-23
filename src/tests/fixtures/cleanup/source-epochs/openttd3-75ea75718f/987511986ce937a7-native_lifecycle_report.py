"""Seal focused selected tube lifecycle and original-file acceptance.

@file native_lifecycle_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md15C.
Owning feature receipts remain the lifecycle/state authorities; this audit never repairs private state.
"""

import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from local_electrical_feed_checks import identity
from native_lifecycle_return import TUBE_CASES
from tube_sprite_report import verify_references
from geothermal_tube_work_files import compare_chunks, compare_next_step


def count_native(report, totals):
    """Check every isolated native execution against its exact terminal log.

    @param report Current schema/lifecycle or ordinary file-reader report.
    @param totals Mutable cases/assertions accumulator for the actual configuration.
    @return None; empty or failed selections cannot enter acceptance.
    """
    native = report["native"]
    processes = native if isinstance(native, list) else [native]
    for proof in processes:  # Native file continuation has separate baseline/resume processes; count each real invocation once rather than counting retained lineage again.
        raw = (ROOT / proof["log"]["path"]).read_text(encoding="utf-8")
        match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", raw)
        assert match is not None and "NATIVE_EXIT_CODE=0" in raw
        assert (int(match[1]), int(match[2])) == (proof["assertions"], proof["cases"])
        totals["cases"] += proof["cases"]
        totals["assertions"] += proof["assertions"]


def accept(selection, build_logs, output):
    """Verify the complete current owner matrix and exact opposite-writer continuity.

    @param selection Explicit completed input index from the bounded focused runs.
    @param build_logs Successful current Debug/optimized test build logs, in order.
    @param output Absent immutable final receipt beneath repository logs.
    @return None; no public state, save, source image or gameplay owner is changed.
    """
    output = (ROOT / output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    index = json.loads((ROOT / selection).read_text(encoding="utf-8"))
    assert index["complete"]
    audit_path = ROOT / "logs/lifecycle/owner-return/20261018-native-01/tube-entry-audit.json"
    audit = json.loads(audit_path.read_text(encoding="utf-8"))
    for reference in audit["production_files"]:  # Coverage additions are test-only. Any changed native lifecycle hook requires its own new producer acceptance instead of borrowing this earlier policy audit.
        assert identity(ROOT / reference["path"]) == reference
    configurations = ("Debug", "RelWithDebInfo")
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    grid, cache, records = {}, {}, []
    for observation in index["records"]:  # Every named obligation must be present in each actual reader; a repeated easy case or similarly named report cannot fill a missing owner boundary.
        path = ROOT / observation["report"]
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations and cfg == observation["configuration"]
        assert report["binary"] == identity(ROOT / f"build/{cfg}/openttd_test.exe")
        if "focus" in observation:
            focus = observation["focus"]
            assert focus in TUBE_CASES and report["focus"] == focus and report["tag"] == TUBE_CASES[focus]
            assert report["controller"] == identity(ROOT / "src/tests/multilayer/native_lifecycle_return.py")
        else:
            focus = observation["phase"]
            assert focus in ("cache", "step") and report["phase"] == focus
            assert report["case"] == (3 if focus == "cache" else 0)
            assert report["controller"] == identity(ROOT / "src/tests/multilayer/utilities/geothermal_tube_work_files.py")
            writer = json.loads((ROOT / report["writer"]["path"]).read_text(encoding="utf-8"))
            assert writer["configuration"] == configurations[1 - configurations.index(cfg)], "original file must come from opposite writer"
            if focus == "cache":
                proof = report["native"][0]
                assert report["cache_complete_chunks_exact"] == compare_chunks(ROOT / proof["cache_before"]["path"], ROOT / proof["output"]["path"], None) == 126
            else:
                baseline, resumed = report["native"]
                assert report["next_step"] == compare_next_step(ROOT / baseline["output"]["path"], ROOT / resumed["output"]["path"], report["checkpoint"]["tick"], baseline["checkpoint"]["tick"])
                assert report["next_step"]["complete_chunks_exact"] == 125
                assert all(row["other_bytes_exact"] for row in report["next_step"]["native_load_events"])
        assert (cfg, focus) not in grid
        grid[cfg, focus] = report
        count_native(report, summary[cfg])
        records.append({"report": identity(path), **report})
    assert grid.keys() == {(cfg, focus) for cfg in configurations for focus in (*TUBE_CASES, "cache", "step")}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    pairs = []
    for phase in ("cache", "step"):  # Actual opposite readers must preserve complete126-chunk outputs at every pure-cache and baseline/resumed boundary, not merely a selected balance digest.
        for position in range(len(grid["Debug", phase]["native"])):
            boundaries = ("cache_before", "output") if phase == "cache" else ("output",)
            for boundary in boundaries:
                a, b = [grid[cfg, phase]["native"][position][boundary] for cfg in configurations]
                assert compare_chunks(ROOT / a["path"], ROOT / b["path"], None) == 126
                pairs.append({"phase": phase, "position": position, "boundary": boundary, "left": a, "right": b, "complete_chunks_exact": 126})
    builds = []
    for name in build_logs:  # Current test-only additions require both real compiled configurations; native production/game binaries remain unchanged and are not needlessly rebuilt.
        path = ROOT / name
        log = path.read_text(encoding="utf-8")
        assert "openttd_test.exe" in log and not re.search(r"\b(?:warning|error) (?:C\d+|LNK\d+|MSB\d+)", log)
        builds.append(identity(path))
    assert len(builds) == 2
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md15C.2;actual selected tube lifecycle/hooks/refusals and retained original-file continuity;thermal/full-profile/network gates separate",
              "controller": identity(__file__), "selection": identity(ROOT / selection), "entry_audit": identity(audit_path), "summary": summary,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "records": records, "cross_reader_world_pairs": pairs, "exact_artifact_identities": len(cache), "builds": builds,
              "original_owner_receipts": audit["accepted_owner_closures"], "native_version": 434, "artwork": identity(ROOT / "media/baseset/openttd.grf"),
              "public_artwork_delivery": identity(ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"),
              "fixture_classification": "Prepared floor/material/loaded17-unit packet fixtures exercise actual native commands,normal motion and public lifecycle hooks. Retained original files supply ordinary paid-use/source/cache/next-step continuity;no route is recreated.",
              "remaining": "15D actual vent/plant/riser lifecycle,15E policy/master9.5B closure,then10.8 paidTBM/tube final use;human/operator/network/orbital/future-profile gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "cases": result["native_cases"], "assertions": result["native_assertions"], "world_pairs": len(pairs)}), flush=True)


def main():
    """Seal an explicit complete focused tube lifecycle input index.

    @return None; output must be absent and all current producer checks must pass.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.build_logs, args.output)


if __name__ == "__main__":
    main()
