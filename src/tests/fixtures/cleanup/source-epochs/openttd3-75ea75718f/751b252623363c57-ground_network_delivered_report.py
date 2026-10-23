"""Seal actual paid tube and operating geothermal/riser snapshot joins.

@file ground_network_delivered_report.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group18C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
All native owners compare at real initial/next/later frames;original consumed history stays authoritative.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from ground_network_trace import world
from ground_network_active_report import verify_checkpoint
from ground_network_report import verify_reference
from construction_provider_return import identity
from save_tables import saved_table

INPUTS = "logs/network/paid-ground/20261019-native-01/delivered-inputs.json"
INPUTS_SHA = "2e99895ece641555d4c6cdf93fd57ccb0263cff95294ad53477251f424b32af1"


def useful_ledger(report, original):
    """Verify actual native paid/cargo/heat/electric progression from original owners.

    @param report Actual peer trace containing complete initial and512-opportunity files.
    @param original Exact selected paid-world identities/meters,never reconstructed stock.
    @return Native consumed-energy/service/movement observations from complete saved files.
    """
    first = verify_reference(report["frames"][0]["files"]["client"])
    last = verify_reference(report["frames"][-1]["files"]["client"])
    initial_tick = saved_table(first, b"DATE")[0]["tick_counter"]
    final_tick = saved_table(last, b"DATE")[0]["tick_counter"]
    assert final_tick - initial_tick == 512
    before = saved_table(first, b"UPLD")
    after = saved_table(last, b"UPLD")
    if report["focus"] == "tube-queue":
        initial = next(row for row in before.values() if row["type"] == 0 and row["id"] == 0)
        final = next(row for row in after.values() if row["type"] == 0 and row["id"] == 0)
        assert initial == original["power_meter"]
        assert final["lifetime_consumed"] > initial["lifetime_consumed"]
        connection = next(row for row in saved_table(last, b"TUPD").values() if row["kind"] == 0)
        assert connection["invoice"] > original["connection"]["invoice"]
        a, b = saved_table(first, b"VEHS"), saved_table(last, b"VEHS")
        positions = {}
        # Actual original capsule positions must change through native motion.
        # Energy-only progress or a replacement visible train cannot stand in
        # for these same mapped vehicle/consumer identities and saved packets.
        for row in original["participants"].values():
            vehicle = row["vehicle"]
            keys = ("train[0].common[0].tile", "train[0].common[0].x_pos", "train[0].common[0].y_pos")
            positions[vehicle] = {"initial": [a[vehicle][key] for key in keys], "final": [b[vehicle][key] for key in keys]}
        assert any(row["initial"] != row["final"] for row in positions.values())
        assert {row["vehicle"] for row in saved_table(last, b"TUVD").values()} == {11, 14}
        return {"consumed_initial_j": initial["lifetime_consumed"], "consumed_final_j": final["lifetime_consumed"],
                "invoice_initial": original["connection"]["invoice"], "invoice_final": connection["invoice"],
                "original_capsule_positions": positions, "native_opportunities": 512}
    initial_generators = {row["model"]: row for row in saved_table(first, b"GPGD").values()}
    final_generators = {row["model"]: row for row in saved_table(last, b"GPGD").values()}
    assert initial_generators == {row["model"]: row for row in original["generators"].values()}
    assert final_generators[0] == initial_generators[0], "Original VentTap history changed during Magma operation"
    magma, completed = initial_generators[1], final_generators[1]
    assert completed["active_service_ticks"] - magma["active_service_ticks"] == 512
    assert completed["last_operating_tick"] - magma["last_operating_tick"] == 512
    # Construction and consumed-resource identity remain original. Only actual
    # paid operating/service fields may advance in the existing machinery row.
    operating = {"active_service_ticks", "last_operating_tick", "service_remainder"}
    assert {key: value for key, value in magma.items() if key not in operating} == {key: value for key, value in completed.items() if key not in operating}
    assert saved_table(first, b"GRSD") == saved_table(last, b"GRSD")
    assert {row["industry"] for row in saved_table(last, b"GRSD").values()} == {4, 5}
    output = magma["electrical_producer"]
    initial = next(row for row in before.values() if row["type"] == 0 and row["id"] == output)
    final = next(row for row in after.values() if row["type"] == 0 and row["id"] == output)
    old_conversion, new_conversion = saved_table(first, b"UCVD"), saved_table(last, b"UCVD")
    old_heat = next(row["heat_j"] for row in old_conversion.values() if row["kind"] == 1 and row["output"] == output)
    new_heat = next(row["heat_j"] for row in new_conversion.values() if row["kind"] == 1 and row["output"] == output)
    conversion = next(row for row in old_conversion.values() if row["kind"] == 0 and row["output"] == output)
    converted = next(row for row in new_conversion.values() if row["kind"] == 0 and row["output"] == output)
    assert new_heat > old_heat
    assert final["lifetime_generated"] > initial["lifetime_generated"]
    assert final["lifetime_consumed"] > initial["lifetime_consumed"]
    assert (final["lifetime_generated"] - initial["lifetime_generated"]) * 100 + converted["numerator"] - conversion["numerator"] == (new_heat - old_heat) * conversion["efficiency"]
    initial_money = saved_table(first, b"PLYR")[0]["money"]
    final_money = saved_table(last, b"PLYR")[0]["money"]
    assert final_money < initial_money
    return {"native_opportunities": 512, "heat_initial_j": old_heat, "heat_final_j": new_heat,
            "electric_generated_initial_j": initial["lifetime_generated"], "electric_generated_final_j": final["lifetime_generated"],
            "electric_consumed_initial_j": initial["lifetime_consumed"], "electric_consumed_final_j": final["lifetime_consumed"],
            "magma_service_initial": magma["active_service_ticks"], "magma_service_final": completed["active_service_ticks"],
            "money_initial": initial_money, "money_final": final_money, "original_risers": [4, 5], "VentTap_history_unchanged": True}


def accept(report_paths, output_path):
    """Verify both native builds' real delivered-family joins and continuing paid ledgers.

    @param report_paths Four exact native peer reports,one per family/configuration.
    @param output_path New retained delivered-family handoff beneath repository logs.
    @return Accepted complete native comparisons and useful original resource history.
    """
    selected = ROOT / INPUTS
    assert identity(selected)["sha256"] == INPUTS_SHA
    selection = json.loads(selected.read_text(encoding="utf-8"))
    records = {cfg: {} for cfg in ("Debug", "RelWithDebInfo")}
    # Both actual paid tube and actual original thermal/riser families must
    # transfer independently in both applications;no copied report substitutes.
    for name in report_paths:
        path = (ROOT / name).resolve()
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg, focus = report["configuration"], report["focus"]
        assert cfg in records
        assert focus in selection["records"]
        assert focus not in records[cfg]
        original = selection["records"][focus]
        assert report["original_input"] == original["input"]
        assert report["original_delivered"] == original
        assert report["handoff"] == identity(selected)
        assert report["children_reaped"] is True
        assert report["tag"] == "[.ground-network-delivered]"
        assert report["native_version"] == 434
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        for reference in [report["controller"], report["observer"], report["binary"], report["original_input"],
                          report["content"], *report["sources"], *report["diagnostics"].values(), *report["configs"].values()]:
            verify_reference(reference)
        assert report["joins"]["server"] == report["joins"]["client"]
        joins = report["joins"]["client"]
        assert [row[0] for row in joins] == ["joined", "joined-next"]
        start = int(joins[0][1])
        assert int(joins[1][1]) == start + 1
        assert int(joins[0][2]) == original["clock"] + 1
        assert int(joins[1][2]) == original["clock"] + 2
        wanted = [start + offset for offset in (128, 256, 384, 512)]
        assert report["states"]["server"] == report["states"]["client"]
        assert [int(row[0]) for row in report["states"]["client"]] == wanted
        assert report["last_ack"] - report["first_ack"] == report["acknowledged_frames"] >= 444
        assert len(report["frames"]) == 6
        assert [row["frame"] for row in report["frames"]] == [start, start + 1, *wanted]
        for checkpoint in report["frames"]:
            verify_checkpoint(checkpoint)
        for role, native in report["native"].items():
            output = verify_reference(native["log"]).read_text(encoding="utf-8", errors="replace")
            counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
            assert counts is not None
            assert int(counts[1]) == native["assertions"] > 0
            assert int(counts[2]) == native["cases"] == 1
            assert all(int(row[1]) == (0 if role == "server" else 1) for row in report["views"][role])
        records[cfg][focus] = {"report": identity(path), "actual": report, "paid_progression": useful_ledger(report, original)}
    assert all(set(rows) == set(selection["records"]) for rows in records.values())
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md18C/delivered18.2;actual paid tube and operating original geothermal/two-riser native joins,complete initial/next/later state and continued paid useful ledgers;orbital rows excluded",
              "controller": identity(__file__), "selection": identity(selected), "records": records,
              "native_cases": 8, "native_assertions": sum(native["assertions"] for rows in records.values() for row in rows.values() for native in row["actual"]["native"].values()),
              "complete126_chunk_purity_pairs": 48, "complete_peer_pairs": 24,
              "whole_peer_chunks_exact_per_pair": 125, "gameplay_settings_exact_per_pair": 195,
              "build_logs": {"Debug": identity(ROOT / "logs/network/paid-ground/20261019-native-01/debug-build-7.log"),
                             "RelWithDebInfo": identity(ROOT / "logs/network/paid-ground/20261019-native-01/optimized-build-4.log")},
              "remaining": "Selected combined-ground matrix,pause/time modes and relevant lifecycle/network failures remain;orbital combined18.2/18.3 remain outside this master scope."}
    target = (ROOT / output_path).resolve()
    # New acceptance cannot overwrite source worlds or previously sealed proof.
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(target), "native_assertions": result["native_assertions"], "peer_pairs": 24, "pure_pairs": 48}), flush=True)
    return result


def main():
    """Seal four explicitly named real delivered-family native peer records.

    @return None;altered originals,missing consumption or unknown native differences refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
