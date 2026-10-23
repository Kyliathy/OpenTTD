"""Accept actual stopped native Oil/Salt stock worlds before precursor haulage.

@file cistern_stock_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DBB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This receipt earns no chemical output, input carriage or Cistern movement.
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
from cistern_source_stocks import CARRIER_RECEIPT


def main():
    """Require opposite original carriers, exact native stopped stocks/provenance and complete cross-build worlds.

    @return None; changed original identities, missing real finite debit or inconsistent native output prevents acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / CARRIER_RECEIPT; history = json.loads(receipt.read_text(encoding="utf-8"))
    configurations = ("Debug", "RelWithDebInfo")
    carriers = {row["configuration"]: row for row in history["records"] if row["focus"] == "carrier-contract"}
    cache, stocks, records = {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Stock production must continue the exact accepted opposite carrier.
    # Source work/packets remain native; a new unit fixture, copied registry or
    # matching aggregate scalar cannot replace its original actual participants.
    for name in args.reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in configurations and cfg not in stocks and report["focus"] == "source-stocks"
        opposite = configurations[1 - configurations.index(cfg)]; carrier = carriers[opposite]
        assert report["carrier_report"] == carrier["report"] and report["carrier_receipt"] == identity(receipt)
        assert report["native"]["input"] == carrier["native"][1]["output"]
        stock, packets = report["stock"], report["packets"]
        assert stock["parts"][:2] == [carrier["carrier"]["head"], carrier["carrier"]["tanker"]]
        assert len(stock["parts"]) == len(set(stock["parts"])) == 4
        assert len(stock["industries"]) == len(set(stock["industries"])) == 3
        assert len(stock["stations"]) == len(set(stock["stations"])) == 3
        assert stock["salt"] == stock["reserve_before"] - stock["reserve_after"] == 20 and stock["oil"] >= 20
        assert stock["salt_opportunities"] == 5 and stock["tick"] == stock["oil_opportunities"] + 74 * 5
        assert stock["cash"] > 0 and stock["loan"] == 0
        assert len({row["id"] for row in packets}) == len(packets) and all(row["purpose"] == 0 for row in packets)
        # Preserve every native split/age and its actual first station, with
        # distinct raw roles. No source packet can be relabelled into CHEM or
        # counted as both a raw input and an already useful manufactured load.
        for role, industry, station in (("oil", stock["industries"][0], stock["stations"][0]), ("salt", stock["industries"][2], stock["stations"][2])):
            selected = [row for row in packets if row["source"] == industry]
            assert sum(row["units"] for row in selected) == stock[role]
            assert all(row["station"] == station for row in selected)
        assert {row["source"] for row in packets} == {stock["industries"][0], stock["industries"][2]}
        count_native(report["native"], summary[cfg]); stocks[cfg] = report; records.append({"report": identity(path), **report})
    assert stocks.keys() == set(configurations) and summary["Debug"] == summary["RelWithDebInfo"]
    assert stocks["Debug"]["stock"] == stocks["RelWithDebInfo"]["stock"]
    assert stocks["Debug"]["packets"] == stocks["RelWithDebInfo"]["packets"]
    left, right = [stocks[cfg]["native"]["output"] for cfg in configurations]
    assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
    builds = []
    # Retain original diagnostics unchanged and require successful current
    # native test-target completion for both independent source simulations.
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DBB; actual stopped Oil/Salt stock only; precursor carriage and CHEM output remain15DBC",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "stock": stocks["Debug"]["stock"], "packets": stocks["Debug"]["packets"],
        "carrier_receipt": identity(receipt), "complete_world_pair": {"left": left, "right": right, "complete_chunks_exact": 126},
        "exact_identities_verified": len(cache), "builds": builds, "native_version": 434,
        "test_binaries": [identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        "fixture_classification": "Actual ordinary native pools retain the original carrier. Real command-funded source/rail and bounded original Oil/finite Salt work own all stocks. Prepared machinery/floor/Connected shaft and explicit pre-acquisition author funds remain boundaries. Trains stay stopped; no input shipment, chemical output, Cistern movement or TBM/bootstrap claim.",
        "remaining": "15DBC must load these exact original stock worlds, drive both native fronts into CHEM, preserve actual unused inputs and earn whole empty returns;15DBD owns final source handoff.",
    }
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "stock": result["stock"]}))


if __name__ == "__main__":
    main()
