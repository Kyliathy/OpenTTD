"""Accept exact original-source paid Cistern route preparation before physical movement.

@file cistern_route_prepare_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DCB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
No useful shipment, receiver production, runtime raster or paid TBM/bootstrap is earned by stopped preparation.
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
from cistern_route_prepare import SOURCE_RECEIPT


def main():
    """Require both exact opposite source inputs, actual paid stopped participants and complete cross-build ready worlds.

    @return None; changed originals, missing native construction, premature source dispatch or unequal complete worlds prevents acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / SOURCE_RECEIPT; history = json.loads(receipt.read_text(encoding="utf-8")); source = history["source"]
    assert identity(receipt)["sha256"] == "27d2c1e9bde125da41a67f15de58be119cadd0518b8737337c766555b27cfc98"
    lineage = ROOT / history["source_lineage"][0]["path"]; assert identity(lineage) == history["source_lineage"][0]
    originals = {row["configuration"]: row for row in json.loads(lineage.read_text(encoding="utf-8"))["records"]}
    configurations = ("Debug", "RelWithDebInfo")
    cache, prepared, records = {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Both real construction runs must continue exact opposite original
    # completed source worlds. Current normal publication compatibility is
    # source-owned; no newly reconstructed stock or matching scalar can replace it.
    for name in args.reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in configurations and cfg not in prepared and report["focus"] == "route-prepare"
        opposite = configurations[1 - configurations.index(cfg)]; original = originals[opposite]
        assert report["source_report"] == original["report"] and report["source_receipt"] == identity(receipt)
        assert report["native"]["input"] == original["native"]["output"]
        point = report["prepared"]
        assert point["original_parts"] == source["parts"] and len(point["parts"]) == 5
        assert len(set(point["original_parts"] + point["parts"])) == 9
        assert len(point["industries"]) == len(set(point["industries"])) == 4 and point["industries"][:3] == source["industries"]
        assert len(point["stations"]) == len(set(point["stations"])) == 6 and point["stations"][:3] == source["stations"]
        assert point["tick"] == source["tick"] and point["source_income"] == source["income"]
        assert 0 < point["cash"] < source["cash"] and point["loan"] == source["loan"] == 0
        assert point["multiple_industry_per_town"] is True
        count_native(report["native"], summary[cfg]); prepared[cfg] = report; records.append({"report": identity(path), **report})
    assert prepared.keys() == set(configurations) and summary["Debug"] == summary["RelWithDebInfo"]
    assert prepared["Debug"]["prepared"] == prepared["RelWithDebInfo"]["prepared"]
    left, right = [prepared[cfg]["native"]["output"] for cfg in configurations]
    assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
    builds = []
    # Accept only completed current native test targets while preserving all
    # earlier source/calendar diagnostics and immutable original handoffs.
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DCB; exact paid stopped Cistern modal preparation; useful route15DCC remains pending",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "prepared": prepared["Debug"]["prepared"], "source_receipt": identity(receipt),
        "complete_world_pair": {"left": left, "right": right, "complete_chunks_exact": 126},
        "exact_identities_verified": len(cache), "builds": builds, "native_version": 434,
        "test_binaries": [identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        "fixture_classification": "Exact original source worlds retain all upstream participants,20CHEM and six unused Oil. An explicit native synchronized saved multi-industry option allows the second real Chemical Works/PCHM in the original one-town scenario. Native commands fund all actual new modal vehicles/rail/berths and paid external power. Prepared floor/completed machinery/Connected shafts remain boundaries. All fronts stay stopped/empty; no shipment, useful intake, receiver product, raster or paid TBM/bootstrap is claimed.",
        "remaining": "15DCC must load these exact original nine-part/four-industry/six-station ready worlds and earn actual modal movement, single native final payment, real PCHM output and three whole empty returns before15DC.1.",
    }
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "prepared": result["prepared"]}))


if __name__ == "__main__":
    main()
