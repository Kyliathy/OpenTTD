"""Seal actual agreed paid tube-power loss and original loaded custody for later recovery.

@file ground_network_power_report.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group21B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Complete peer comparisons remain exact;across advancing hold ticks,custody projections exclude ordinary cargo aging.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from ground_network_report import verify_reference
from ground_network_active_report import verify_checkpoint
from ground_network_trace import world
from construction_provider_return import identity
from save_tables import saved_table

INPUTS = "logs/network/paid-ground/20261019-native-01/delivered-inputs.json"
INPUTS_SHA = "2e99895ece641555d4c6cdf93fd57ccb0263cff95294ad53477251f424b32af1"
PHYSICAL = ("tile", "x_pos", "y_pos", "z_pos", "cur_real_order_index", "cargo.packets", "cargo.action_counts")
PROVENANCE = ("source", "source_xy", "loaded_at_xy", "count", "source_type", "source_id")


def custody(path):
    """Read conserved original physical parts and packet custody without suppressing peer fields.

    @param path Actual native original or held checkpoint;no world is loaded or mutated.
    @return Original two hulls,packet references/counts/provenance and existing fractional energy carry.
    """
    vehicles, packets = saved_table(path, b"VEHS"), saved_table(path, b"CAPA")
    physical, cargo = {}, {}
    # These actual original native hulls have one physical part each. The
    # native observer separately walks complete chains at every held tick.
    for vehicle in (11, 14):
        row = vehicles[vehicle]
        physical[vehicle] = {field: row["train[0].common[0]." + field] for field in PHYSICAL}
        # Ordinary pool references encode index+1;zero is null. Read their
        # original native packet owners rather than inventing aggregate cargo.
        for reference in row["train[0].common[0].cargo.packets"]:
            assert reference > 0
            packet = packets[reference - 1]
            cargo[reference] = {field: packet[field] for field in PROVENANCE}
    power = saved_table(path, b"TUVD")
    assert {row["vehicle"] for row in power.values()} == {11, 14}
    carry = {row["vehicle"]: row["remainder"] for row in power.values()}
    return {"physical": physical, "packets": cargo, "carry": carry}


def accept(report_paths, output_path):
    """Verify both actual independent peer loss cases and preserve exact broken originals.

    @param report_paths Two complete actual native reports,one per configuration.
    @param output_path New immutable loss/custody handoff beneath repository logs.
    @return Exact accepted broken states;useful paid reconnection/delivery remains a separate next group.
    """
    inputs_path = ROOT / INPUTS
    assert identity(inputs_path)["sha256"] == INPUTS_SHA
    original = json.loads(inputs_path.read_text(encoding="utf-8"))["records"]["tube-queue"]
    records = {}
    # Each build must actually transfer the same original paid queue and
    # execute company0's closure through both native agreed command queues.
    for name in report_paths:
        path = (ROOT / name).resolve()
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        assert report["focus"] == "power-loss"
        assert report["tag"] == "[.ground-network-power-loss]"
        assert report["children_reaped"] is True
        assert report["native_version"] == 434
        assert report["handoff"] == identity(inputs_path)
        assert report["original_input"] == original["input"]
        assert report["original_delivered"] == original
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        for reference in [report["controller"], report["observer"], report["scenario_observer"], report["binary"],
                          report["original_input"], report["content"], *report["sources"],
                          *report["diagnostics"].values(), *report["configs"].values()]:
            verify_reference(reference)
        assert report["states"]["server"] == report["states"]["client"]
        assert report["joins"]["server"] == report["joins"]["client"]
        assert [int(row[2]) for row in report["joins"]["client"]] == [original["clock"] + 1, original["clock"] + 2]
        assert [int(row[1]) for row in report["states"]["client"]] == [original["clock"] + offset for offset in (129, 257, 385, 513)]
        assert report["last_ack"] - report["first_ack"] == report["acknowledged_frames"] >= 444
        assert report["power"]["server"] == report["power"]["client"]
        power = report["power"]["client"]
        assert len(power["closed"]) == 1
        closed = power["closed"][0]
        assert closed["source"] == original["power_meter"]["id"] == 0
        assert closed["consumed"] > original["power_meter"]["lifetime_consumed"]
        assert closed["invoice"] >= original["connection"]["invoice"]
        assert len(power["held"]) == 4
        for row in power["held"]:
            assert row["source"] == closed["source"]
            assert row["consumed"] == closed["consumed"]
            assert row["invoice"] == closed["invoice"]
            assert row["parts"] == row["packets"] == 2
            assert row["cargo"] == 120
        commands = report["commands"]
        assert len(commands["server"]) == 5
        assert len(commands["client"]) == len(report["client_received_agreed_commands"]) == 3
        assert commands["server"][-3:] == commands["client"]
        closure = commands["client"][-1]
        assert closure.endswith("00000000 (CmdCloseTubePower)")
        assert "; 00; " in closure
        assert "; 00000000; 00000000 (CmdCloseTubePower)" in closure
        assert int(report["client_received_agreed_commands"][-1][1]) == closed["frame"]
        assert len(report["frames"]) == 6
        source_custody = custody(ROOT / original["input"]["path"])
        held_custody = []
        for checkpoint in report["frames"]:
            verify_checkpoint(checkpoint)
        # Clock progression naturally ages cargo;only this between-tick
        # custody projection omits age. Full peer CAPA/VEHS always match above.
        for checkpoint, event in zip(report["frames"][2:], power["held"], strict=True):
            assert checkpoint["frame"] == event["frame"]
            saved = ROOT / checkpoint["files"]["server"]["path"]
            held = custody(saved)
            assert held["packets"] == source_custody["packets"]
            assert sum(row["count"] for row in held["packets"].values()) == 120
            connection = next(row for row in saved_table(saved, b"TUPD").values() if row["kind"] == 0)
            meter = next(row for row in saved_table(saved, b"UPLD").values() if row["type"] == 0 and row["id"] == 0)
            assert connection["active"] == meter["active"] == 0
            assert connection["invoice"] == closed["invoice"]
            assert connection["remainder"] == 0
            assert meter["lifetime_consumed"] == closed["consumed"]
            held_custody.append(held)
        assert all(row == held_custody[0] for row in held_custody)
        for native in report["native"].values():
            text = verify_reference(native["log"]).read_text(encoding="utf-8", errors="replace")
            counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
            assert counts is not None
            assert int(counts[1]) == native["assertions"] > 0
            assert int(counts[2]) == native["cases"] == 1
            assert "Sync error detected" not in text
            assert "sync_err:" not in text
        records[cfg] = {"report": identity(path), "actual": report, "held_custody": held_custody[0],
                        "broken_input": report["frames"][-1]["files"]["server"],
                        "clock": original["clock"] + 513, "closed": closed}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    for left, right in zip(records["Debug"]["actual"]["frames"], records["RelWithDebInfo"]["actual"]["frames"], strict=True):
        for role in ("server", "client"):
            assert world(ROOT / left["files"][role]["path"]) == world(ROOT / right["files"][role]["path"])
    assertions = sum(native["assertions"] for row in records.values() for native in row["actual"]["native"].values())
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md21B;actual agreed original paid-source retirement,complete peers and conserved original loaded custody;useful paid recovery remains21C",
              "controller": identity(__file__), "inputs": identity(inputs_path), "records": records,
              "native_owner_sources": [identity(ROOT / name) for name in (
                  "src/transport_tube_cmd.cpp", "src/transport_tube_power.cpp", "src/transport_tube_runtime.cpp",
                  "src/transport_tube.h", "src/utility_power.h", "src/tests/tbm_reuse.cpp")],
              "native_cases": 4, "native_assertions": assertions,
              "complete_peer_pairs": 12, "all_chunk_pure_pairs": 24, "all_chunk_cross_build_pairs": 12,
              "remaining": "Use these exact broken originals for actual paid service replacement/reconnection and useful cargo recovery21C;whole mixed lifecycle/date tasks remain pending."}
    target = (ROOT / output_path).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": assertions}), flush=True)
    return result


def main():
    """Seal complete actual loss evidence without changing any original paid world.

    @return None;missing command,custody,history,state or provenance refuses acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
