"""Accept surviving tube company custody, terminal liquidation and focused native files.

@file tube_company_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group14D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Terminal native liquidation is explicit vehicle/packet destruction; surviving takeover retains cargo.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from geothermal_tube_work_files import compare_chunks, compare_next_step
from local_electrical_feed_checks import identity
from tube_company_checks import TAGS
from tube_sprite_report import verify_references
from tube_work_file_report import OWNERS, retained_writers


def main():
    """Verify exact current company cases, native file continuations and four successful builds.

    @return None; missing/stale evidence, changed owners or any native failure forbids acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=18, required=True, help="Nine exact tube/shared-producer company reports per configuration.")
    parser.add_argument("--affected-reports", nargs=4, required=True, help="Current opposite-writer cache3/step0 reports in both builds.")
    parser.add_argument("--build-logs", nargs=4, required=True, help="Successful final game/test build logs.")
    parser.add_argument("--output", required=True, help="Absent immutable closure JSON beneath repository logs.")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    cache, grid, files, records = {}, set(), {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    writers = retained_writers()
    # Surviving takeover and terminal deletion have different native cargo
    # outcomes. Both source families, prior lifecycle owners and focused real
    # files must pass independently; no single parked takeover substitutes.
    for name in (*args.reports, *args.affected_reports):
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in configurations
        if "focus" in report:
            focus = report["focus"]
            assert focus in TAGS and (cfg, focus) not in grid
            grid.add((cfg, focus)); processes = [report["native"]]
        else:
            phase, case = report["phase"], report["case"]
            assert (phase, case) in (("cache", 3), ("step", 0)) and (cfg, phase) not in files
            writer = writers[configurations[1 - configurations.index(cfg)]]
            assert report["writer"] == writer["report"]
            points = {point["step"]: point for point in writer["checkpoints"] if point["phase"] == case}
            assert report["checkpoint"] == points[0]
            processes = report["native"]
            if phase == "cache":
                assert len(processes) == 1
                proof = processes[0]
                assert proof["external_connected"] is True and proof["native_connected"] is False
                assert proof["cache_rebuilds"] == 3
                assert compare_chunks(ROOT / proof["cache_before"]["path"], ROOT / proof["output"]["path"], None) == 126
                assert compare_chunks(ROOT / proof["input"]["path"], ROOT / proof["output"]["path"], OWNERS) == len(OWNERS)
            else:
                assert len(processes) == 2 and report["uninterrupted_baseline"] == points[1]["input"]
                first, second = [ROOT / process["output"]["path"] for process in processes]
                assert report["next_step"] == compare_next_step(first, second, points[0]["tick"], points[1]["tick"])
                assert compare_chunks(first, second, OWNERS) == len(OWNERS)
            files[cfg, phase] = report
        for process in processes:
            log = (ROOT / process["log"]["path"]).read_text(encoding="utf-8")
            count = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
            assert count is not None and "NATIVE_EXIT_CODE=0" in log
            assert (int(count[1]), int(count[2])) == (process["assertions"], process["cases"])
            summary[cfg]["cases"] += process["cases"]
            summary[cfg]["assertions"] += process["assertions"]
        records.append({"report": identity(path), **report})
    assert grid == {(cfg, focus) for cfg in configurations for focus in TAGS}
    assert set(files) == {(cfg, phase) for cfg in configurations for phase in ("cache", "step")}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    pairs = []
    # The production ownership correction must preserve ordinary current file
    # behavior. Both readers of opposite genuine originals retain complete
    # cache worlds and actual uninterrupted/continued output pairs.
    for phase, index, boundary in (("cache", 0, "cache_before"), ("cache", 0, "output"), ("step", 0, "output"), ("step", 1, "output")):
        a, b = [files[cfg, phase]["native"][index][boundary] for cfg in configurations]
        assert compare_chunks(ROOT / a["path"], ROOT / b["path"], None) == 126
        pairs.append({"phase": phase, "index": index, "boundary": boundary, "left": a, "right": b, "complete_chunks_exact": 126})
    builds = []
    # Keep PowerShell diagnostics byte-exact; BOM decoding is only a success
    # check and cannot rewrite a historical or current accepted build log.
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log
        assert "openttd.exe" in log or "openttd_test.exe" in log
        builds.append(identity(path))
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md14D; canonical company changes retain surviving native custody and declare terminal native destruction without permission/quantity reuse",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "cross_reader_pairs": pairs, "exact_identities_verified": len(cache),
        "builds": builds, "native_version": 434, "packed_art": packed,
        "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
        "reproduced_stranding": identity(ROOT / "logs/tubes/company-lifecycle/20261018-010014-e09660139e/takeover.log"),
        "reproduced_closed_history": identity(ROOT / "logs/tubes/company-lifecycle/20261018-011622-8d9c2258bf/takeover.log"),
        "previous_component_receipt": identity(ROOT / "logs/tubes/component-edits/20261018-native-01/verification.json"),
        "ownership_contract": {
            "takeover": "Canonical vehicle/account ownership transfers first. The same surviving front retains its powerless association, actual packet/orders/position and demand carry; old original source permissions and unused grants retire.",
            "recovery": "Native power gating brakes without cargo relocation. The successor explicitly purchases a fresh source and uses the existing held on-route reconnect command, transferring the carry once and earning fresh consumed work through real arrival.",
            "closed_history": "The sole shared ownership graph couples only a live external binding's original source and typed recipient even after old shares expire. Closed/live source cases and repeated takeover retain ownership consistency; inactive bindings cannot attach unrelated tombstones.",
            "liquidation": "Canonical INVALID_OWNER removal destroys actual vehicles/packets, retires unused rights and retains generated/consumed/invoice history. No ownerless vehicle, refund or second stock store is created.",
            "reuse": "Native company/vehicle pools may reuse numeric IDs. Fresh paid depot/purchase and explicit normal company availability do not reactivate former source, consumer or feed identities.",
        },
        "fixture_classification": "Prepared floor/materials, loaded17-unit stock and explicit service wear are boundary inputs; real commands/native movement or repair earn consumed work. This is not useful-route/bootstrap or human acceptance.",
        "remaining": "14.4 queued/starved files,15 useful introduction-date routes; runtime gallery, host/client and human feedback remain separate.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "cross_reader_pairs": len(pairs)}))


if __name__ == "__main__":
    main()
