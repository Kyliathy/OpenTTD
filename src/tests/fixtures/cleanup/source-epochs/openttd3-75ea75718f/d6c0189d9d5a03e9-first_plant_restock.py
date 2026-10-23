"""Buy actual paid replacement supplies in the retained original funded first-plant world.

@file first_plant_restock.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1CCCB/operating1.3B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Retired consumed bills remain historical; new physical packets belong only to native paid Surface orders.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))
from runner import current_run_context
from local_electrical_feed_checks import identity, native
from geothermal_tube_binding_files import world_chunks
from tube_sprite_report import verify_references, compare_worlds
from mole_useful_raster_report import count_native
from shield_source_files import parse_snapshot
from first_plant_lifecycle import PROTECTED as RETIRED_PROTECTED

FUNDING = "logs/geothermal/source-rebuild/20261018-native-01/funding-verification.json"
FUNDING_SHA = "4c0f3b708a90f56ceb5cfd2111bd2bf9007cc83c14efd73f6ef073146f31032e"
SOURCES = ("src/tests/geothermal_first_plant_restock.cpp", "src/tests/plant_rebuild_material_fixture.h",
           "src/tests/multilayer/utilities/first_plant_restock.py", "src/surface_supply.cpp", "src/surface_supply_cmd.cpp",
           "src/cargopacket.cpp", "src/multilayer_bootstrap.cpp", "src/geothermal_collector.cpp", "src/geothermal_generator.cpp")
CASES = ("Paid Surface seed quotes are pure and use canonical dates and tariffs",
         "Paid Surface orders settle once and retain the company economy-day cap",
         "Purchased material reaches the native receiver without freight payment")
# New native paid packets/purchase history legitimately change CAPA/SSOH/SSOR.
# All other completed physical/geological/spoil/vehicle/order/riser owners from
# retirement remain complete native chunks, including the stopped real carrier.
PROTECTED = RETIRED_PROTECTED - {"CAPA", "SSOH", "SSOR"}


def validate_points(points, funded):
    """Reconcile new physical purchases with retired and retained actual whole-bill history.

    @param points Actual original0 and all-paid1 native world observations.
    @param funded Exact opposite accepted original fresh-funded checkpoint.
    @return None; changed original owners, free materials, moved carrier or lost historical consumption refuses acceptance.
    """
    assert [point["stage"] for point in points] == [0, 1]
    first, final = (point["snapshot"] for point in points); old = funded["snapshot"]
    original = world_chunks(ROOT / points[0]["input"]["path"]); source = world_chunks(ROOT / funded["input"]["path"])
    assert len(original) == 126 and original == source
    for key in ("tick", "year", "cash", "geology", "projects", "exchange", "risers", "utility", "supply", "vehicles", "industries", "packets"):
        assert first[key] == old[key], key
    assert first["year"] == 2000 and first["packets"] == 0 and first["invoices"] == 0
    assert final["invoices"] > 0 and final["cash"] == first["cash"] - final["invoices"]
    assert final["orders"] == first["orders"] + 4 and final["next_order"] == first["next_order"] + 4 and final["packets"] == 4
    assert final["supply"] != first["supply"]
    for key in ("tick", "year", "geology", "projects", "exchange", "risers", "utility", "origin", "carrier", "vehicles", "industries"):
        assert final[key] == first[key], key
    total = 0
    # The exact frozen44STEL/28RFMT/14ELEC/6BATY replacement bill is new
    # physical cargo. Its old consumed counterpart plus retained risers still
    # reconcile all116 earlier purchases; no demolished bill is reused as stock.
    for index, quantity in enumerate((44, 28, 14, 6)):
        retained = (8, 8, 4, 4)[index]; prior = quantity + retained
        for point in points:
            stage, current = point["stage"], point["snapshot"]
            assert current[f"retired{index}"] == quantity and current[f"live_consumed{index}"] == retained
            assert current[f"bought{index}"] == prior + (quantity if stage == 1 else 0)
            assert current[f"dispatched{index}"] == first[f"dispatched{index}"] == prior
            assert current[f"physical{index}"] == current[f"source{index}"] == (quantity if stage == 1 else 0)
        assert final[f"cargo{index}"] == first[f"cargo{index}"]
        total += quantity
    assert total == 92
    for index in range(3):
        for current in (first, final):
            assert current[f"industry{index}"] == old[f"industry{index}"]
            assert current[f"work{index}"] == current[f"committed{index}"] == 0
    # Complete native chunks retain physical vehicle poses/coupling/orders,
    # paid spoil/ground and original riser construction, independent of scalars.
    for point in points:
        chunks = world_chunks(ROOT / point["input"]["path"]); assert chunks.keys() == original.keys()
        for name in PROTECTED:
            assert chunks[name.encode()] == original[name.encode()], (point["stage"], name)


def buy(configuration, funding_report):
    """Run exact producer boundaries and buy the complete bill in the opposite funded original.

    @param configuration Actual current Debug or optimized native test executable.
    @param funding_report Exact sealed opposite native configuration's1CCB funding report.
    @return None; print the invocation-owned paid stock report only after original/native checks pass.
    """
    receipt = ROOT / FUNDING; assert identity(receipt)["sha256"] == FUNDING_SHA
    history = json.loads(receipt.read_text(encoding="utf-8")); selected = identity(funding_report)
    record = next(row for row in history["records"] if row["report"] == selected); assert record["configuration"] != configuration
    funded = next(point for point in record["checkpoints"] if point["stage"] == 3)
    source = ROOT / funded["input"]["path"]; assert identity(source) == funded["input"]
    context = current_run_context("geothermal", "replacement-stock"); context.prepare_runtime(); processes = []
    # Actual source command, cap and native purpose/payment boundaries run in
    # isolated processes before dependent purchase on the retained paid world.
    for index, case in enumerate(CASES):
        _, proof = native(context, configuration, case, f"producer-{index}"); processes.append({"case": case, **proof})
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md1CCCB; actual paid replacement supply.\nplant_rebuild_stock\n", encoding="utf-8")
    output, proof = native(context, configuration, "[.geothermal-first-plant-rebuild-stock]", "original-purchase", {"OTTD_PLANT_RESTOCK_INPUT": str(source)})
    points = []
    # Native observations retain paid history and actual material custody;
    # parsing never assigns quantities, references or a destination to gameplay.
    for stage, text, path in re.findall(r"PLANT_RESTOCK_WORLD stage=(\d+) snapshot=([^\s]+) path=([^\r\n]+)", output):
        points.append({"stage": int(stage), "snapshot": parse_snapshot(text), "input": identity(path)})
    validate_points(points, funded); assert identity(source) == funded["input"]
    proof.update(input=funded["input"], output=points[-1]["input"])
    report = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CCCB; real paid replacement stock, actual carriage/labour remain1CCCC",
              "configuration": configuration, "focus": "buy", "native": processes + [proof], "checkpoints": points,
              "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
              "sources": [identity(ROOT / name) for name in SOURCES], "funding_receipt": identity(receipt),
              "funding_report": selected, "funded_checkpoint": funded, "protected_complete_chunks": sorted(PROTECTED),
              "fixture_classification": "Exact opposite accepted original fresh-funded stage3, same three-stop/four-hold native carrier and original Surface supplier. Native paid orders add44STEL/28RFMT/14ELEC/6BATY as four genuine construction-purpose source packets, retaining original histories/caps and exact cash debit. Original retired92 units and retained-riser24 remain historical consumed material; fresh machinery remains empty/zero work/service while carrier stays whole stopped/empty. No direct receipt, stock/cash/clock/floor/energy or arrival/rating/order-cursor grant. Actual replacement transport/construction/empty return remain1CCCC and fresh useful service/ordinary files remain1CCD/1CD."}
    path = context.output("reports", f"first-plant-replacement-stock-{configuration}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


def accept(reports, build_logs, output):
    """Accept both actual paid source originals and every complete cross-build world.

    @param reports Two current opposite-original purchase reports.
    @param build_logs Successful current Debug/optimized test-target diagnostics.
    @param output Absent immutable paid replacement stock receipt under repository logs.
    @return None; stale evidence, unpaid/missing material or unequal original owners prevent acceptance.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, records = {}, {}; summary = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    # Current code/binaries are fresh identities; exact sealed funding inputs
    # remain opaque original history, not a fresh old-build assertion.
    for name in reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in summary and cfg not in records and report["focus"] == "buy"
        validate_points(report["checkpoints"], report["funded_checkpoint"])
        for proof in report["native"]:
            count_native(proof, summary[cfg])
        records[cfg] = {"report": identity(path), **report}
    assert records.keys() == summary.keys() and summary["Debug"] == summary["RelWithDebInfo"]
    pairs = []
    # Complete native comparison includes every actual source packet/purpose,
    # link graph, company debit and unchanged physical carrier owner.
    for first, second in zip(records["Debug"]["checkpoints"], records["RelWithDebInfo"]["checkpoints"]):
        assert first["stage"] == second["stage"] and first["snapshot"] == second["snapshot"]
        assert compare_worlds(first["input"], second["input"]) == 126
        pairs.append({"stage": first["stage"], "left": first["input"], "right": second["input"], "complete_chunks_exact": 126})
    builds = []
    # Actual target/link diagnostics support current executable identities;
    # source-only admission cannot stand in for real command/packet results.
    for name in build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md1CCCB; actual paid replacement material custody acceptance",
              "controller": identity(__file__), "records": list(records.values()), "summary": summary,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "complete_cross_build_world_pairs": pairs, "funding_receipt": identity(ROOT / FUNDING), "builds": builds,
              "native_version": 434, "exact_identities_verified": len(cache),
              "remaining": "1CCCC actual loaded carriage/native labour/whole empty return and original-next captures,1CCD fresh useful service and1CD ordinary files must pass before1CCC/1CC/1.3B/original1.3. Master10.7/runtime-art/9.5B/later TBM-to-tube/human/operator/network gates stay pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "complete_world_pairs": len(pairs)}), flush=True)


def main():
    """Select one bounded actual purchase or its independent two-build acceptance.

    @return None; missing phase arguments or incomplete native evidence stop before a receipt.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("buy", "accept"), required=True)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo")); parser.add_argument("--funding-report", type=Path)
    parser.add_argument("--reports", nargs=2); parser.add_argument("--build-logs", nargs=2); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "buy":
        assert args.config is not None and args.funding_report is not None
        buy(args.config, args.funding_report)
    else:
        assert args.reports is not None and args.build_logs is not None and args.output is not None
        accept(args.reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
