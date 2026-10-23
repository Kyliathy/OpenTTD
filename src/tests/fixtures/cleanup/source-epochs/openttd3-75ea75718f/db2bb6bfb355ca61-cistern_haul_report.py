"""Accept original native Cistern precursor delivery, actual CHEM output and complete empty returns.

@file cistern_haul_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DBC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Final source acceptance, Cistern carriage and useful output receiver operation remain later groups.
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
from cistern_source_haul import STOCK_RECEIPT


def main():
    """Require exact opposite stock inputs, conserved raw/output owners, physical movement and complete cross-build output worlds.

    @return None; changed originals, incomplete native evidence or mismatched complete worlds prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / STOCK_RECEIPT; history = json.loads(receipt.read_text(encoding="utf-8"))
    configurations = ("Debug", "RelWithDebInfo")
    stocks = {row["configuration"]: row for row in history["records"]}
    cache, hauls, records = {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Each native continuation must consume its exact accepted opposite stock
    # world. Sealed historical source/binary identities remain opaque: current
    # instrumentation cannot authorize reconstructing old raw participants.
    for name in args.reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in configurations and cfg not in hauls and report["focus"] == "source-haul"
        opposite = configurations[1 - configurations.index(cfg)]; stock_report = stocks[opposite]; stock = stock_report["stock"]
        assert report["stock_report"] == stock_report["report"] and report["stock_receipt"] == identity(receipt)
        assert report["native"]["input"] == stock_report["native"]["output"]
        result = report["result"]
        for key in ("parts", "industries", "stations"):
            assert result[key] == stock[key]
        assert result["year"] == 1980 and result["original_tick"] == stock["tick"]
        assert 0 < result["route_ticks"] < 14000 and result["tick"] == stock["tick"] + result["route_ticks"] + 148
        assert result["oil"] == stock["oil"] and result["salt"] == stock["salt"] == result["chemicals"] == 20
        assert result["unused_oil"] == stock["oil"] - result["chemicals"] and result["unused_salt"] == 0
        assert result["reserve"] == stock["reserve_after"] and result["cash"] > 0 and result["income"] > 0 and result["loan"] == stock["loan"] == 0
        carried = {row["role"]: row for row in report["carried"]}; returned = {row["role"]: row for row in report["returned"]}
        assert len(report["carried"]) == len(report["returned"]) == 2 and carried.keys() == returned.keys() == {0, 1}
        # Every shipment must leave its real loading stop with the original
        # full native load and earn a later whole empty home return. A paid
        # callback or initial stored locomotive alone cannot establish passage.
        for role, cargo in ((0, "oil"), (1, "salt")):
            assert carried[role]["units"] == returned[role]["delivered"] == stock[cargo]
            assert stock["tick"] < carried[role]["tick"] < returned[role]["tick"] <= result["tick"] - 148
        count_native(report["native"], summary[cfg]); hauls[cfg] = report; records.append({"report": identity(path), **report})
    assert hauls.keys() == set(configurations) and summary["Debug"] == summary["RelWithDebInfo"]
    for key in ("result", "carried", "returned"):
        assert hauls["Debug"][key] == hauls["RelWithDebInfo"][key]
    left, right = [hauls[cfg]["native"]["output"] for cfg in configurations]
    assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
    builds = []
    # Retain exact successful current test-build diagnostics without changing
    # historical stock receipts or treating dated nested binary hashes as new.
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DBC; real original input carriage/CHEM output/whole returns; final source acceptance15DBD and Cistern route15DC remain pending",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "result": hauls["Debug"]["result"], "stock_receipt": identity(receipt),
        "complete_world_pair": {"left": left, "right": right, "complete_chunks_exact": 126},
        "exact_identities_verified": len(cache), "builds": builds, "native_version": 434,
        "test_binaries": [identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        "fixture_classification": "Exact original ordinary-loaded raw packets earn physical conventional delivery, actual all-input CHEM output and whole stopped empty returns. Full native clocks/vehicles/payments and bounded real processor opportunities retain every owner. Inherited prepared floor/machinery/shaft remain boundaries; no raw/output grant, pose repair, output dispatch, Cistern carriage or paid TBM/bootstrap claim.",
        "remaining": "15DBD verifies the complete current source handoff and four targets before15DC consumes this same original output/world for the Cistern route.",
    }
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "result": result["result"]}))


if __name__ == "__main__":
    main()
