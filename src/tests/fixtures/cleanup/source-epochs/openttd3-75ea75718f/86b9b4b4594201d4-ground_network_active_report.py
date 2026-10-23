"""Seal actual paid active excavation/wagon-wait joins in both native configurations.

@file ground_network_active_report.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group18B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Every initial/next/later gameplay owner is checked;seven native local unit preferences are explicit.
"""

import argparse
import hashlib
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network_trace import ROOT, FRAMES, LOCAL_UNITS, world
from ground_network_report import verify_reference
from construction_provider_return import identity
from save_tables import saved_table

INPUTS = "logs/network/paid-ground/20261019-native-01/active-inputs.json"
INPUTS_SHA = "0c247a8383a9e1b91157d0ce41d5f6cb63e4e64042100eafaf2651f4d967b6bb"
SCENARIOS = ("standard-active", "deep-active", "wagon-wait")


def verify_checkpoint(checkpoint):
    """Verify a complete actual native same-frame pair and pure repeat writes.

    @param checkpoint Explicit native host/client files and their complete observed chunk identities.
    @return None;unknown setting,history,vehicle,packet,clock or feature differences refuse.
    """
    data, settings = {}, {}
    # No native family is omitted:both original and repeated writes contain
    # all126 chunks,and every nonlocal field must match the independent peer.
    for role, reference in checkpoint["files"].items():
        path = verify_reference(reference)
        data[role] = world(path)
        assert data[role] == world(verify_reference(checkpoint["pure_repeats"][role]))
        assert {name: hashlib.sha256(raw).hexdigest() for name, raw in data[role].items()} == checkpoint["complete_chunk_sha256"][role]
        settings[role] = saved_table(path, b"PATS")
    assert set(data) == {"server", "client"}
    assert data["server"].keys() == data["client"].keys()
    assert {name for name in data["server"] if data["server"][name] != data["client"][name]} == {"PATS"}
    assert settings["server"][0].keys() == settings["client"][0].keys()
    assert set(settings["server"]) == set(settings["client"]) == {0}
    assert set(checkpoint["native_local_units"]) == set(LOCAL_UNITS)
    for name, default in LOCAL_UNITS.items():
        assert settings["server"][0][name] == 0
        assert settings["client"][0][name] == default
        assert checkpoint["native_local_units"][name] == {"server": 0, "client": default}
    canonical = {role: {name: value for name, value in rows[0].items() if name not in LOCAL_UNITS}
                 for role, rows in settings.items()}
    assert canonical["server"] == canonical["client"]
    assert len(canonical["server"]) == checkpoint["canonical_settings_equal"] == 195
    assert checkpoint["complete_peer_chunks_equal"] == 125


def accept(report_paths, output_path):
    """Validate three actual paid-work scenarios in two independent native builds.

    @param report_paths Six exact real peer reports,one per scenario/configuration.
    @param output_path New retained active-join handoff beneath repository logs.
    @return Complete accepted active-work/join/next-frame evidence and original lineage.
    """
    inputs_path = ROOT / INPUTS
    assert identity(inputs_path)["sha256"] == INPUTS_SHA
    selection = json.loads(inputs_path.read_text(encoding="utf-8"))
    prior = ROOT / "logs/network/paid-ground/20261019-native-01/company-verification.json"
    assert identity(prior)["sha256"] == "189c805c2dd591b4223f8797155ab9aa6bea6f4158837cb90ebbf1067f2647f3"
    records = {cfg: {} for cfg in ("Debug", "RelWithDebInfo")}
    # Independent active Standard/Deep and genuine loaded WaitForEmpties joins
    # must all pass. A completed paid tube cutter cannot stand in for these.
    for name in report_paths:
        path = (ROOT / name).resolve()
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg, focus = report["configuration"], report["focus"]
        assert cfg in records
        assert focus in SCENARIOS
        assert focus not in records[cfg]
        assert report["tag"] == "[.ground-network-active]"
        assert report["native_version"] == 434
        assert report["children_reaped"] is True
        assert report["handoff"] == identity(inputs_path)
        original = selection["records"][focus]
        assert report["original_input"] == original["input"]
        assert report["original_work"] == original
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        for reference in [report["controller"], report["observer"], report["binary"], report["original_input"],
                          report["content"], *report["sources"], *report["diagnostics"].values(), *report["configs"].values()]:
            verify_reference(reference)
        assert report["work"]["server"] == report["work"]["client"]
        observations = report["work"]["client"]
        assert len(observations) == 6
        initial, following, last = observations[0], observations[1], observations[-1]
        assert initial["tick"] == original["clock"] + 1
        assert following["tick"] == initial["tick"] + 1
        assert following["frame"] == initial["frame"] + 1
        assert [row["frame"] for row in observations[2:]] == list(FRAMES)
        assert all(row["project"] == original["project"]["id"] for row in observations)
        assert all(row["job"] == original["job"]["id"] for row in observations)
        assert initial["cursor"] == original["job"]["cursor"]
        assert initial["phase"] == original["project"]["phase"]
        assert initial["blocked"] == 0
        assert initial["earth"] == original["job"]["earth"]
        assert initial["rock"] == original["job"]["rock"]
        assert initial["cost"] == original["job"]["cost"]
        assert report["last_ack"] - report["first_ack"] == report["acknowledged_frames"] >= 444
        assert report["states"]["server"] == report["states"]["client"]
        commands = report["commands"]
        assert len(commands["client"]) == len(report["client_received_agreed_commands"]) == 2
        assert commands["server"][-2:] == commands["client"]
        assert commands["client"][0].endswith("0000 (CmdPause)")
        assert commands["client"][1].endswith("0200 (CmdPause)")
        if focus == "wagon-wait":
            assert len(commands["server"]) == 5
            expected = original["project"]["id"].to_bytes(4, "little") + b"\1"
            assert commands["server"][0].endswith(expected.hex().upper() + " (CmdControlTBMProject)")
            assert "; 00; 00000095; 00000000;" in commands["server"][0]
            assert initial["phase"] == 5
            assert initial["haul_phase"] == 3
            assert initial["carried"] == following["carried"] > 0
            assert initial["exchanges"] == 5
            assert (last["haul_x"], last["haul_y"]) != (initial["haul_x"], initial["haul_y"])
        else:
            assert len(commands["server"]) == 4
            # An active job must genuinely gain partial work or complete a
            # later paid cut after joining;metadata equality alone is insufficient.
            assert last["partial"] > initial["partial"] or last["cursor"] > initial["cursor"]
        for role, native in report["native"].items():
            output = verify_reference(native["log"]).read_text(encoding="utf-8", errors="replace")
            counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
            assert counts is not None
            assert int(counts[1]) == native["assertions"] > 0
            assert int(counts[2]) == native["cases"] == 1
            assert "Sync error detected" not in output
            assert "sync_err:" not in output
            assert all(int(row[1]) == (0 if role == "server" else 1) for row in report["views"][role])
        assert len(report["frames"]) == 6
        assert [row["stage"] for row in report["frames"]] == ["joined", "joined-next", *[f"f{frame}" for frame in FRAMES]]
        for checkpoint in report["frames"]:
            verify_checkpoint(checkpoint)
        records[cfg][focus] = {"report": identity(path), "actual": report}
    assert all(set(rows) == set(SCENARIOS) for rows in records.values())
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md18B/original18.1;actual430 paid active Standard/Deep and loaded wagon-wait originals join current434 native applications;complete initial/next/later peer state and real progression",
              "controller": identity(__file__), "selection": identity(inputs_path), "preceding_company_handoff": identity(prior),
              "native_observer": identity(ROOT / "src/tests/ground_network_active_fixture.h"),
              "registration": identity(ROOT / "src/tests/tbm_reuse.cpp"), "records": records, "native_cases": 12,
              "native_assertions": sum(native["assertions"] for rows in records.values() for row in rows.values() for native in row["actual"]["native"].values()),
              "complete126_chunk_purity_pairs": 72, "complete_peer_pairs": 36,
              "whole_peer_chunks_exact_per_pair": 125, "gameplay_settings_exact_per_pair": 195,
              "native_local_unit_fields": LOCAL_UNITS,
              "build_logs": {"Debug": identity(ROOT / "logs/network/paid-ground/20261019-native-01/debug-build-5.log"),
                             "RelWithDebInfo": identity(ROOT / "logs/network/paid-ground/20261019-native-01/optimized-build-3.log")},
              "remaining": "Delivered tube/thermal active joins,combined selected ground matrix,pause/economy modes and relevant lifecycle/network failures remain;orbital combined acceptance stays out of scope."}
    target = (ROOT / output_path).resolve()
    # Native acceptance writes only a new retained receipt,never an input,
    # earlier handoff or original paid-world artifact.
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(target), "native_assertions": result["native_assertions"], "peer_pairs": 36, "pure_pairs": 72}), flush=True)
    return result


def main():
    """Seal six explicitly named actual active-work peer reports.

    @return None;missing scenarios,unknown differences or altered originals refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
