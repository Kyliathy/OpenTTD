"""Accept original Shield useful freight with continuing finite extraction and three whole empty returns.

@file shield_useful_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15ECC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Ordinary loaded-world files, runtime silhouette and actual TBM/bootstrap retain their later tasks.
"""
import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from local_electrical_feed_checks import identity
from geothermal_tube_work_files import compare_chunks
from tube_sprite_report import verify_references
from mole_useful_raster_report import count_native
from shield_useful_checks import PREPARE_RECEIPT, PREPARE_SHA256, validate_result


def main():
    """Require exact opposite ready worlds and equal complete useful outputs from two independent native runs.

    @return None; changed originals, native failures, lost cargo or unequal complete worlds prevent a receipt.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    # Acceptance artifacts belong to the reproducible log owner and are
    # immutable once written, so another run cannot silently replace evidence.
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / PREPARE_RECEIPT
    assert identity(receipt)["sha256"] == PREPARE_SHA256
    history = json.loads(receipt.read_text(encoding="utf-8"))
    originals = {row["configuration"]: row for row in history["records"] if row.get("focus") == "route-prepare"}
    configurations = ("Debug", "RelWithDebInfo")
    cache, routes, records = {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Each actual useful run borrows the exact opposite accepted ready world.
    # Earlier source/preparation binaries stay opaque historical identities;
    # a current rebuild cannot authorize participant/source reconstruction.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations and cfg not in routes and report["focus"] == "shield-useful"
        opposite = configurations[1 - configurations.index(cfg)]
        original = originals[opposite]
        assert report["prepare_report"] == original["report"] and report["prepare_receipt"] == identity(receipt)
        assert report["native"]["input"] == original["native"]["output"]
        validate_result(report["result"], report["scenes"], report["returned"], original["prepared"])
        count_native(report["native"], summary[cfg])
        routes[cfg] = report
        records.append(dict(report=identity(path), **report))
    assert set(routes) == set(configurations)
    for key in ("result", "scenes", "returned"):
        assert routes["Debug"][key] == routes["RelWithDebInfo"][key]
    left, right = [routes[cfg]["native"]["output"] for cfg in configurations]
    assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
    builds = []
    # Only completed current test targets support this native route handoff.
    # Production targets retain their previous evidence because this group
    # changes only test fixtures/controllers, with no gameplay/schema changes.
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log
        builds.append(identity(path))
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15ECC/original15EC.1; original useful Shield modal freight/continuing finite extraction/whole empty returns; files/raster remain15ED",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "result": routes["Debug"]["result"], "prepare_receipt": identity(receipt),
        "complete_world_pair": {"left": left, "right": right, "complete_chunks_exact": 126},
        "exact_identities_verified": len(cache), "builds": builds, "native_version": 434,
        "test_binaries": [identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        "fixture_classification": "Exact accepted ordinary-loaded paid worlds retain original source, receiver, fronts, claims and power. Common native clocks/vehicles/industry work and genuine dispatch/earned service carry finite Deep RARE through both conventional layer crossings and local paid Shield into actual Surface ELEC intake/payment. Every reserve/buffer/queue/hold/packet/native account owner conserves continuing extraction, including genuine remaining inventory after three whole empty returns and148 stopped opportunities. Prepared floor/machinery/Connected shafts remain explicit boundary inputs. RFMT is absent; no Electronics manufacturing, operating profit, source freeze, packet/rating grant, pose/order/clock/cash repair or TBM/bootstrap claim.",
        "remaining": "15ED must retain every real loaded modal/final world and untouched common native baselines, prove ordinary repeated/opposite-reader continuation and inspect actual moving Shield native pixels/picking before15E.1.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "result": result["result"]}))


if __name__ == "__main__":
    main()
