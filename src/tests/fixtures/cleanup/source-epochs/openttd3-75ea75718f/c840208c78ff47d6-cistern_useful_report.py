"""Accept actual original-world Cistern modal transport, useful PCHM output and three whole empty returns.

@file cistern_useful_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DCC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Ordinary route files/runtime silhouette and actual paid TBM/bootstrap retain their later acceptance tasks.
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
from cistern_useful_checks import PREPARE_RECEIPT


def main():
    """Require exact opposite ready worlds, real original useful delivery/payment/power and complete cross-build empty-return output.

    @return None; changed originals, missing actual modal states, duplicated native accounting or unequal complete worlds prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / PREPARE_RECEIPT
    assert identity(receipt)["sha256"] == "9d3d8bbe1ae9509bfc9555b47f9aaad9b3753bbb8e8280effff8b1b641c49561"
    history = json.loads(receipt.read_text(encoding="utf-8")); originals = {row["configuration"]: row for row in history["records"]}
    configurations = ("Debug", "RelWithDebInfo"); cache, routes, records = {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Every independent useful run must continue the exact opposite original
    # paid ready world. Dated preparation/source binaries remain opaque history,
    # never permission to recreate participants or replace the actual shipment.
    for name in args.reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in configurations and cfg not in routes and report["focus"] == "cistern-useful"
        opposite = configurations[1 - configurations.index(cfg)]; original = originals[opposite]; point = original["prepared"]
        assert report["prepare_report"] == original["report"] and report["prepare_receipt"] == identity(receipt)
        assert report["native"]["input"] == original["native"]["output"]
        result = report["result"]
        assert result["year"] == 1980 and result["original_tick"] == point["tick"]
        assert 0 < result["route_ticks"] < 24000 and result["tick"] == point["tick"] + result["route_ticks"] + 148
        assert result["delivered"] == result["propellant"] == 20 and result["unused_oil"] == 6 and result["reserve"] == 31980
        assert result["income"] > 0 and result["total_income"] == point["source_income"] + result["income"]
        assert result["cash"] > 0 and result["loan"] == 0 and result["consumed"] > 0 and result["quarter_history"] > 0
        scenes, returned = report["scenes"], report["returned"]
        assert len(scenes) == 5 and {row["scene"] for row in scenes} == set(range(5))
        assert len(returned) == 3 and {row["role"] for row in returned} == set(range(3))
        # Scene labels observe real native loaded states, including partial
        # station loading. Return markers require later whole empty home
        # storage; an initial depot presence or paid callback cannot satisfy it.
        for row in scenes:
            assert 0 < row["cargo"] <= 20 and point["tick"] < row["tick"] <= result["tick"] - 148
        for row in returned:
            assert max(scene["tick"] for scene in scenes) < row["tick"] <= result["tick"] - 148
        count_native(report["native"], summary[cfg]); routes[cfg] = report; records.append({"report": identity(path), **report})
    assert routes.keys() == set(configurations) and summary["Debug"] == summary["RelWithDebInfo"]
    for key in ("result", "scenes", "returned"):
        assert routes["Debug"][key] == routes["RelWithDebInfo"][key]
    left, right = [routes[cfg]["native"]["output"] for cfg in configurations]
    assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
    builds = []
    # Both current native test targets must finish before this same actual
    # useful world can authorize later ordinary route-file/runtime-art consumers.
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DCC/original15DC.1; actual paid Cistern useful modal transport/native PCHM output/three whole returns; files/raster remain15DD",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "result": routes["Debug"]["result"], "prepare_receipt": identity(receipt),
        "complete_world_pair": {"left": left, "right": right, "complete_chunks_exact": 126},
        "exact_identities_verified": len(cache), "builds": builds, "native_version": 434,
        "test_binaries": [identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        "fixture_classification": "Exact ordinary-loaded original paid worlds retain all upstream owners. Actual native producer dispatch/normal service without rating/packet grants carries20CHEM through conventional shafts and layer-local paid Cistern into real PCHM20PROP output, preserving source raw histories, six unused Oil/reserve and native current/historical accounts. Three whole empty stopped returns plus148 native opportunities retain output/income without replay. Prepared floor/machinery/Connected shafts and explicit multi-industry scenario option remain boundaries. No file/raster, paid TBM/bootstrap, gallery or human acceptance is claimed.",
        "remaining": "15DD must retain actual loaded modal/final worlds and untouched native baselines, prove ordinary repeated/opposite-reader continuation and inspect the real moving Cistern native pixels/picking before15D.1.",
    }
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "result": result["result"]}))


if __name__ == "__main__":
    main()
