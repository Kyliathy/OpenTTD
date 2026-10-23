"""Seal original paid thermal-riser failure/recommissioning on independent native peers.

@file ground_network_thermal_report.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group21D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Complete native owners and original consumed histories remain authoritative.
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
RECOVERY = "logs/network/paid-ground/20261019-native-01/power-recovery-verification.json"
RECOVERY_SHA = "89e1de5ab7ba59e1afebc4712cd72e87d63234e719b7e7f83976fe7c3d0b4b97"


def accept(report_paths, output_path):
    """Require complete real-peer failure/recovery and unchanged original paid machinery.

    @param report_paths One actual thermal-riser report from each native configuration.
    @param output_path New immutable diagnostic receipt beneath repository logs.
    @return Accepted initial/next/later states,original histories and useful recovered service.
    """
    assert identity(ROOT / INPUTS)["sha256"] == INPUTS_SHA
    assert identity(ROOT / RECOVERY)["sha256"] == RECOVERY_SHA
    original = json.loads((ROOT / INPUTS).read_text(encoding="utf-8"))["records"]["thermal-operating"]
    original_path = verify_reference(original["input"])
    original_risers = saved_table(original_path, b"GRSD")
    original_generators = saved_table(original_path, b"GPGD")
    original_utility = saved_table(original_path, b"UPLD")
    records = {}
    for name in report_paths:
        path = (ROOT / name).resolve()
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in records
        assert report["focus"] == "thermal-riser" and report["tag"] == "[.ground-network-thermal-riser]"
        assert report["children_reaped"] is True and report["native_version"] == 434
        assert report["handoff"] == identity(ROOT / INPUTS)
        assert report["original_input"] == original["input"]
        assert report["original_delivered"] == original
        for reference in [report["controller"], report["observer"], report["scenario_observer"], report["binary"],
                          report["content"], *report["sources"], *report["configs"].values(),
                          *report["diagnostics"].values()]:
            verify_reference(reference)
        assert report["states"]["server"] == report["states"]["client"]
        assert report["joins"]["server"] == report["joins"]["client"]
        assert report["thermal"]["server"] == report["thermal"]["client"]
        observations = report["thermal"]["client"]
        assert len(observations) == 6
        assert [row["tick"] - original["clock"] for row in observations] == [1, 2, 129, 257, 385, 513]
        assert [row["operational"] for row in observations] == [1, 1, 0, 0, 1, 1]
        assert [row["old_active"] for row in observations] == [1, 1, 0, 0, 0, 0]
        assert all(row["first_active"] == 1 for row in observations)
        assert all(row["fresh_edge"] == 4294967295 for row in observations[:4])
        first, held, final = observations[0], observations[2], observations[-1]
        assert final["fresh_edge"] not in (first["old_edge"], first["first_edge"], 4294967295)
        assert all(row["old_edge"] == first["old_edge"] and row["first_edge"] == first["first_edge"] for row in observations)
        assert all(row["old_flow"] == held["old_flow"] for row in observations[2:])
        assert observations[3]["useful"] == held["useful"]
        assert final["useful"] > held["useful"] > first["useful"]
        assert final["heat"] > held["heat"] > first["heat"]
        assert all(left["heat"] <= right["heat"] and left["useful"] <= right["useful"]
                   for left, right in zip(observations, observations[1:]))
        assert all(row["generated"] * 100 == row["heat"] * 35 for row in observations)
        assert all(row["vent_service"] == first["vent_service"] == 10285 and row["vent_use"] == first["vent_use"] == 9990
                   for row in observations)
        assert final["cash"] < first["cash"]
        assert final["fresh_flow"] > 0
        assert report["acknowledged_frames"] >= 444
        commands = report["commands"]["client"]
        assert report["commands"]["server"][-len(commands):] == commands
        assert len(commands) == len(report["client_received_agreed_commands"]) == 5
        assert [re.search(r"\((Cmd\w+)\)$", command)[1] for command in commands] == [
            "CmdPause", "CmdPause", "CmdChangeLayerPortalConnection", "CmdChangeLayerPortalConnection", "CmdConnectGeothermalTownService"]
        assert all("; 00; " in command for command in commands[2:])
        assert len(report["frames"]) == 6
        for checkpoint, observation in zip(report["frames"], observations, strict=True):
            verify_checkpoint(checkpoint)
            assert checkpoint["frame"] == observation["frame"]
            saved = ROOT / checkpoint["files"]["server"]["path"]
            assert saved_table(saved, b"DATE")[0]["tick_counter"] == observation["tick"]
        saved = ROOT / report["frames"][-1]["files"]["server"]["path"]
        risers = saved_table(saved, b"GRSD")
        assert risers.keys() == original_risers.keys()
        for index, before in original_risers.items():
            after = dict(risers[index])
            if before["industry"] == 4:
                assert after["portal_revision"] == before["portal_revision"] + 2
                after["portal_revision"] = before["portal_revision"]
            assert after == before
        construction = ("industry", "site", "owner", "model", "work_ticks", "materials_committed",
                        "has_work_tick", "last_work_tick", "heat_consumer", "electrical_producer")
        generators = saved_table(saved, b"GPGD")
        assert generators.keys() == original_generators.keys()
        for index, before in original_generators.items():
            assert {field: generators[index][field] for field in construction} == {field: before[field] for field in construction}
        utility = saved_table(saved, b"UPLD")
        by_link = {row["id"]: row for row in utility.values() if row["type"] == 4}
        assert by_link[first["old_edge"]]["active"] == 0
        assert by_link[first["old_edge"]]["b"] == held["old_flow"]
        assert by_link[final["fresh_edge"]]["active"] == 1
        assert by_link[final["fresh_edge"]]["riser"] == 4
        assert by_link[final["fresh_edge"]]["reference"] == by_link[first["old_edge"]]["reference"]
        assert by_link[final["fresh_edge"]]["destination"] == by_link[first["old_edge"]]["destination"]
        assert {row["id"] for row in utility.values() if row["type"] == 1} == {row["id"] for row in original_utility.values() if row["type"] == 1}
        for native in report["native"].values():
            text = verify_reference(native["log"]).read_text(encoding="utf-8", errors="replace")
            counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
            assert counts and int(counts[1]) == native["assertions"] > 0 and int(counts[2]) == native["cases"] == 1
            assert "Sync error detected" not in text and "sync_err:" not in text
        records[cfg] = {"report": identity(path), "actual": report}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    for left, right in zip(records["Debug"]["actual"]["frames"], records["RelWithDebInfo"]["actual"]["frames"], strict=True):
        for role in ("server", "client"):
            assert world(ROOT / left["files"][role]["path"]) == world(ROOT / right["files"][role]["path"])
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md21D;actual original paid first-plant two-riser failure/recommissioning and useful Surface export,complete native peers and preserved spent histories;no rebuilt bootstrap or private grant",
              "controller": identity(__file__), "inputs": identity(ROOT / INPUTS), "tube_recovery": identity(ROOT / RECOVERY),
              "records": records, "native_cases": 4,
              "native_assertions": sum(n["assertions"] for row in records.values() for n in row["actual"]["native"].values()),
              "complete_peer_pairs": 12, "all_chunk_pure_pairs": 24, "all_chunk_cross_build_pairs": 12,
              "native_owner_sources": [identity(ROOT / name) for name in (
                  "src/layer_portal_work.cpp", "src/geothermal_riser_construction.cpp", "src/geothermal_riser.cpp",
                  "src/utility_power.cpp", "src/geothermal_service_cmd.cpp", "src/geothermal_generator.cpp",
                  "src/tests/ground_network_thermal_fixture.h", "src/tests/tbm_reuse.cpp")],
              "remaining": "Whole mixed vent reuse/direct-processing/date/orbital tasks and normal-newgame/performance/human/operator acceptance remain pending."}
    target = (ROOT / output_path).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": result["native_assertions"]}), flush=True)
    return result


def main():
    """Seal only complete actual paid-riser peer evidence;any missing history,state or provenance refuses."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
