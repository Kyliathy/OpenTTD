"""Accept the first autonomous1970 Mole route without claiming later model/file/art coverage.

@file tube_useful_route_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15BB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Prepared floor/machinery/commissioned portals remain explicit boundaries; Group16 owns actual TBM/bootstrap acceptance.
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
from tube_sprite_report import verify_references
from tube_useful_route_checks import TAGS


def main():
    """Require current native producers, actual original-custody route and four successful targets.

    @return None; changed owners, stale evidence, missing native scope or mismatched independent route prevents acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=8, required=True, help="Four exact current producer/Mole reports per configuration.")
    parser.add_argument("--build-logs", nargs=4, required=True)
    parser.add_argument("--output", required=True, help="Absent immutable receipt under repository logs.")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    cache, grid, routes, records = {}, set(), {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Earlier ordered/production cases remain distinct producer boundaries.
    # Only the new actual Mole driver can prove autonomous arrivals, original
    # packet transfer, native final payment and both complete empty returns.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg, focus = report["configuration"], report["focus"]
        assert cfg in configurations and focus in TAGS and (cfg, focus) not in grid
        process = report["native"]
        log = (ROOT / process["log"]["path"]).read_text(encoding="utf-8")
        count = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
        assert count is not None and "NATIVE_EXIT_CODE=0" in log
        assert (int(count[1]), int(count[2])) == (process["assertions"], process["cases"])
        assert process["cases"] > 0 and process["assertions"] > 0
        grid.add((cfg, focus))
        summary[cfg]["cases"] += process["cases"]
        summary[cfg]["assertions"] += process["assertions"]
        if focus == "mole":
            assert process["cases"] == 1
            assert all(value > 0 for value in report["route"].values())
            assert [row[1:] for row in report["cargo_transitions"]] == [[0, 20, 0, 0, 0], [0, 0, 20, 0, 0], [0, 0, 0, 20, 0], [0, 0, 0, 0, 20]]
            assert "empty_returns=2" in log and "autonomous=true" in log
            routes[cfg] = {"route": report["route"], "cargo_transitions": report["cargo_transitions"]}
        records.append({"report": identity(path), **report})
    assert grid == {(cfg, focus) for cfg in configurations for focus in TAGS}
    assert summary["Debug"] == summary["RelWithDebInfo"] and routes["Debug"] == routes["RelWithDebInfo"]
    builds = []
    # Preserve byte-exact native build diagnostics. Decode only to verify
    # target success, without rewriting any accepted historical evidence.
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
        "_documentation": "r2mlvs17-underground-transport-tubes.md15BB; first autonomous useful1970 Mole native route producer, not later model/date, file or runtime-silhouette acceptance",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "route": routes["Debug"], "exact_identities_verified": len(cache),
        "builds": builds, "native_version": 434, "packed_art": packed,
        "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
        "native_driver": "Coherent1970 calendars advance through native Calendar/Economy/GameTick TimerManagers, including ordinary daily processing. CallVehicleTicks owns station loading, shared allocation, stable vehicle ticks, cargo aging and deferred native depot restart.",
        "cargo_contract": "Five actual74-tick finite mine opportunities dispatch20 Coal. Original packet IDs/first station/mine purpose survive capsule and receiving wagon custody. Every opportunity conserves20 physical units across five native owners. Only actual Surface intake earns one final delivery/payment. Both actual empty home-depot returns and two further native days cannot repeat income or recreate cargo.",
        "power_contract": "One original paid external producer supplies native consumed work and service; no geothermal prerequisite, synthetic grant, private movement or second source ledger. All route construction/vehicle purchases use positive explicit fixture prices and charged native commands, without a balance/profitability claim.",
        "fixture_classification": "Excavated floor, machinery completion and commissioned reciprocal portals are disclosed boundary inputs; actual TBM excavation/bootstrap remains Group16. CFMT accepts real useful Coal fuel input; no unsupported claim of completed metal output without its Copper co-input.",
        "remaining": "15BC ordinary files/runtime silhouette, the other four introduction-date useful routes and explicit Underground/Deep conventional chains; final gallery/human/operator gates remain pending.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "route": routes["Debug"]}))


if __name__ == "__main__":
    main()
