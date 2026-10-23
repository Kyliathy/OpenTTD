"""Seal actual paid service replacement and useful original cargo recovery through native peers.

@file ground_network_recovery_report.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group21C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Exact broken originals continue without a copied world,retired-source reuse or private cargo/energy grants.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT, HANDOFF, HANDOFF_SHA
from ground_network_report import verify_reference
from ground_network_active_report import verify_checkpoint
from ground_network_trace import world
from construction_provider_return import identity
from save_tables import saved_table

LOSS = "logs/network/paid-ground/20261019-native-01/power-loss-verification.json"
LOSS_SHA = "2739375a1c6143fdfaf63c5f2962af2f4ac38a3d0dad0f860b71c3d9060c5ea6"


def accept(report_paths, output_path):
    """Verify actual paid native recovery on both independent application builds.

    @param report_paths Two actual recovery reports continuing their exact sealed broken inputs.
    @param output_path New immutable recovery handoff beneath repository logs.
    @return Full accepted native state,paid service and useful original120-unit cargo evidence.
    """
    loss_path = ROOT / LOSS
    assert identity(loss_path)["sha256"] == LOSS_SHA
    assert identity(ROOT / HANDOFF)["sha256"] == HANDOFF_SHA
    loss = json.loads(loss_path.read_text(encoding="utf-8"))
    paid = json.loads((ROOT / HANDOFF).read_text(encoding="utf-8"))
    records = {}
    # Each native build must continue its original broken paid world;an
    # intact replacement source world cannot certify loss/reconnection recovery.
    for name in report_paths:
        path = (ROOT / name).resolve()
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        original = loss["records"][cfg]
        assert report["focus"] == "power-recovery"
        assert report["tag"] == "[.ground-network-power-recovery]"
        assert report["children_reaped"] is True
        assert report["native_version"] == 434
        assert report["handoff"] == identity(loss_path)
        assert report["original_input"] == original["broken_input"]
        assert report["original_delivered"] == original
        assert report["recovery_participants"] == paid["records"][cfg]["useful_participants"]
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        for reference in [report["controller"], report["observer"], report["scenario_observer"], report["binary"],
                          report["original_input"], report["content"], *report["sources"],
                          *report["diagnostics"].values(), *report["configs"].values()]:
            verify_reference(reference)
        assert report["states"]["server"] == report["states"]["client"]
        assert report["joins"]["server"] == report["joins"]["client"]
        assert [int(row[2]) for row in report["joins"]["client"]] == [original["clock"] + 1, original["clock"] + 2]
        assert report["last_ack"] - report["first_ack"] == report["acknowledged_frames"] >= 444
        assert report["recovery"]["server"] == report["recovery"]["client"]
        progress = report["recovery"]["client"]
        assert len(progress) == 6
        assert [row["tick"] - original["clock"] for row in progress] == [1, 2, 129, 1025, 2049, 3073]
        assert progress[0]["fresh_source"] == 4294967295
        assert progress[0]["intake"] == progress[0]["initial_intake"]
        old = original["closed"]
        for row in progress:
            assert row["old_source"] == old["source"] == 0
            assert row["old_energy"] == old["consumed"]
            assert row["physical"] == row["extracted"]
            assert row["extracted"] + row["remaining"] == paid["records"][cfg]["useful_participants"]["opening"]
            assert row["initial_intake"] == progress[0]["initial_intake"]
        last = progress[-1]
        assert last["fresh_source"] != last["old_source"]
        assert last["fresh_source"] != 4294967295
        assert last["fresh_energy"] > 0
        assert last["intake"] >= last["initial_intake"] + 120
        assert last["home"] == 1
        commands = report["commands"]
        assert commands["server"][-len(commands["client"]):] == commands["client"]
        assert len(commands["client"]) == len(report["client_received_agreed_commands"]) == 7
        names = [re.search(r"\((Cmd\w+)\)$", command)[1] for command in commands["client"]]
        assert names == ["CmdPause", "CmdPause", "CmdConnectTubePower", "CmdSetTubeVehiclePower",
                         "CmdSetTubeVehiclePower", "CmdModifyOrder", "CmdStartStopVehicle"]
        assert all("; 00; " in command for command in commands["client"][2:])
        assert len(report["frames"]) == 6
        for checkpoint, observation in zip(report["frames"], progress, strict=True):
            verify_checkpoint(checkpoint)
            assert checkpoint["frame"] == observation["frame"]
            saved = ROOT / checkpoint["files"]["server"]["path"]
            assert saved_table(saved, b"DATE")[0]["tick_counter"] == observation["tick"]
            old_connection = next(row for row in saved_table(saved, b"TUPD").values() if row["kind"] == 0 and row["source"] == 0)
            old_meter = next(row for row in saved_table(saved, b"UPLD").values() if row["type"] == 0 and row["id"] == 0)
            assert old_connection["active"] == old_meter["active"] == 0
            assert old_connection["invoice"] == old["invoice"]
            assert old_meter["lifetime_consumed"] == old["consumed"]
        final = ROOT / report["frames"][-1]["files"]["server"]["path"]
        connections = saved_table(final, b"TUPD")
        fresh_connection = next(row for row in connections.values() if row["kind"] == 0 and row["source"] == last["fresh_source"])
        assert fresh_connection["active"] == 1
        assert fresh_connection["invoice"] > 0
        assert fresh_connection["portal"] == original["actual"]["original_delivered"]["connection"]["portal"]
        assert fresh_connection["interchange"] == original["actual"]["original_delivered"]["connection"]["interchange"]
        # ReconnectTubePowerVehicle retires each original consumer and issues
        # a fresh binding. Historical rows remain authoritative tombstones;
        # require their spent history rather than filtering them out or
        # incorrectly demanding that they reference the replacement source.
        original_bindings = saved_table(ROOT / original["broken_input"]["path"], b"TUVD")
        bindings = saved_table(final, b"TUVD")
        assert len(original_bindings) == 2 and len(bindings) == 4
        historical_fields = ("consumer", "source", "vehicle", "consumed", "geothermal_source", "local_feed")
        for index, before in original_bindings.items():
            after = bindings[index]
            assert {field: after[field] for field in historical_fields} == {field: before[field] for field in historical_fields}
            assert after["active"] == after["remainder"] == after["phase"] == 0
            assert before["prepared"] <= after["prepared"] < last["tick"]
        fresh_bindings = {index: row for index, row in bindings.items() if index not in original_bindings}
        assert {row["vehicle"] for row in fresh_bindings.values()} == {row["vehicle"] for row in original_bindings.values()}
        assert all(row["source"] == last["fresh_source"] and row["active"] == 1 for row in fresh_bindings.values())
        assert all(row["consumer"] not in original_bindings for row in fresh_bindings.values())
        for native in report["native"].values():
            text = verify_reference(native["log"]).read_text(encoding="utf-8", errors="replace")
            counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
            assert counts is not None
            assert int(counts[1]) == native["assertions"] > 0
            assert int(counts[2]) == native["cases"] == 1
            assert "Sync error detected" not in text
            assert "sync_err:" not in text
        records[cfg] = {"report": identity(path), "actual": report,
                        "fresh_paid_connection": fresh_connection,
                        "useful_recovered": last["intake"] - last["initial_intake"]}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    # Equal opportunity offsets require complete native equality across
    # independent builds,including ordinary cargo aging and settled histories.
    for left, right in zip(records["Debug"]["actual"]["frames"], records["RelWithDebInfo"]["actual"]["frames"], strict=True):
        for role in ("server", "client"):
            assert world(ROOT / left["files"][role]["path"]) == world(ROOT / right["files"][role]["path"])
    assertions = sum(native["assertions"] for row in records.values() for native in row["actual"]["native"].values())
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md21C;actual independent native joins from sealed broken originals,paid fresh service/explicit original-hull binding/ordinary freight,useful120-unit Surface recovery and finite conservation;retired history never reused/refunded",
              "controller": identity(__file__), "loss": identity(loss_path), "paid_original": identity(ROOT / HANDOFF),
              "records": records, "native_cases": 4, "native_assertions": assertions,
              "complete_peer_pairs": 12, "all_chunk_pure_pairs": 24, "all_chunk_cross_build_pairs": 12,
              "native_owner_sources": [identity(ROOT / name) for name in (
                  "src/transport_tube_cmd.cpp", "src/transport_tube_power.cpp", "src/transport_tube_runtime.cpp",
                  "src/tests/ground_network_recovery_fixture.h", "src/tests/tbm_reuse.cpp")],
              "remaining": "Selected actual thermal-riser failure before master11.3 closure;whole mixed depot/interchange/vent/direct-processing/date/orbital tasks remain pending."}
    target = (ROOT / output_path).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": assertions}), flush=True)
    return result


def main():
    """Seal only complete real native paid-recovery evidence.

    @return None;any missing useful delivery,conservation,command,state or provenance refuses acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
