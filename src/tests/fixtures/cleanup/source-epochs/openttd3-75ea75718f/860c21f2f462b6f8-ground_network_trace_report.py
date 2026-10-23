"""Seal actual same-frame paid-world state with strictly bounded client preferences.

@file ground_network_trace_report.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Groups17B/17C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Every native gameplay chunk and setting is compared;no whole chunk is ignored.
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


def verify_company_commands(report):
    """Validate native administrator enrolment and actual company-client command bytes.

    @param report One actual native company scenario including complete execution streams.
    @return Authenticated public key and exact capsule input identity;no secret key is read.
    """
    assert report["focus"] == "company"
    assert report["tag"] == "[.ground-network-company]"
    commands = report["commands"]
    assert len(commands["server"]) == 8
    assert len(commands["client"]) == len(report["client_received_agreed_commands"]) == 6
    assert commands["server"][-6:] == commands["client"]
    server_log = verify_reference(report["native"]["server"]["log"]).read_text(encoding="utf-8", errors="replace")
    client_log = verify_reference(report["native"]["client"]["log"]).read_text(encoding="utf-8", errors="replace")
    key = re.search(r"Added ([0-9A-F]{64}) to company:1\.", server_log)
    assert key is not None
    assert "Client::ReceiveServerMove(): client_id=2, company_id=0" in client_log
    assert "Client::ReceiveServerClientInfo(): client_id=2, playas=0" in client_log
    assert "Kicking client" not in server_log
    capsule = report["original_capsule"]
    encoded_id = capsule.to_bytes(4, "little")
    expected = [(0, 93, bytes([0]) + key[1].encode("ascii") + b"\0", "CmdCompanyAllowListCtrl"),
                (255, 72, b"\0\0", "CmdPause"), (255, 72, b"\2\0", "CmdPause"),
                (0, 61, encoded_id + b"Paid peer capsule\0", "CmdRenameVehicle"),
                (0, 122, encoded_id + b"\1", "CmdStartStopVehicle"),
                (0, 122, encoded_id + b"\1", "CmdStartStopVehicle")]
    # Each actual server/client execution must carry the same complete payload
    # and company as the received native command ID/frame. Packet authority is
    # proved by real server admission,not a test-written client_playas field.
    for index, (company, command, payload, function) in enumerate(expected):
        record = re.fullmatch(r"cmd: ([0-9a-f]+); ([0-9a-f]+); ([0-9a-f]+); ([0-9a-f]+); 00000000; ([0-9A-F]+) \((\w+)\)", commands["client"][index])
        assert record is not None
        assert int(record[3], 16) == company
        assert int(record[4], 16) == command
        assert bytes.fromhex(record[5]) == payload
        assert record[6] == function
        assert int(report["client_received_agreed_commands"][index][0]) == command
    agreed = [int(row[1]) for row in report["client_received_agreed_commands"]]
    assert agreed[:3] == [agreed[0]] * 3
    assert 256 < agreed[3] == agreed[4] < 384
    assert 384 < agreed[5] < 512
    assert report["command_states"]["server"] == report["command_states"]["client"]
    assert [row[3] for row in report["command_states"]["client"]] == ["false", "true", "false", "false"]
    # The company remains closed to arbitrary users. Only this real client's
    # public key is added through the ordinary administrator/native command.
    for checkpoint in report["frames"]:
        for reference in checkpoint["files"].values():
            company = saved_table(verify_reference(reference), b"PLYR")[0]
            assert company["allow_any"] == 0
            assert company["allow_list.count"] == 1
            assert bytes.fromhex(company["allow_list[0].key"]).decode("ascii") == key[1]
    return {"authenticated_public_key": key[1], "original_capsule": capsule, "agreed_command_frames": agreed}


def accept(report_paths, output_path, focus="baseline"):
    """Verify both configurations' independent complete real-peer frame captures.

    @param report_paths Explicit successful native trace reports,one per build.
    @param output_path New retained handoff JSON below repository logs.
    @param focus Baseline unpause transport or actual company-client command scenario.
    @return Validated native states,strict local preferences and exact command provenance.
    """
    records = {}
    companies = {}
    # A passing log from one application cannot cover the other build or its
    # independent native snapshot receiver. Verify every report identity first.
    for name in report_paths:
        path = (ROOT / name).resolve()
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo")
        assert cfg not in records
        assert report["native_version"] == 434
        assert report["children_reaped"] is True
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        for reference in [report["controller"], report["observer"], report["binary"], report["handoff"],
                          report["original_input"], report["content"], *report["sources"],
                          *report["diagnostics"].values(), *report["configs"].values()]:
            verify_reference(reference)
        assert report["states"]["server"] == report["states"]["client"]
        assert [int(row[0]) for row in report["states"]["client"]] == list(FRAMES)
        assert report["last_ack"] - report["first_ack"] == report["acknowledged_frames"] >= 444
        # Native command execution records carry economic date/fraction,
        # company,command,error and complete payload. Received packets add the
        # actual agreed frame;these are not invented from wall-clock timing.
        commands = report["commands"]
        assert commands["server"][0].endswith("0100 (CmdPause)")
        assert commands["server"][1].endswith("0201 (CmdPause)")
        if focus == "company":
            companies[cfg] = verify_company_commands(report)
        else:
            assert len(commands["client"]) == len(report["client_received_agreed_commands"]) == 2
            assert commands["server"][-2:] == commands["client"]
            assert len(commands["server"]) == 4
            assert commands["client"][0].endswith("0000 (CmdPause)")
            assert commands["client"][1].endswith("0200 (CmdPause)")
            assert all(row[0] == "72" for row in report["client_received_agreed_commands"])
            assert len({row[1] for row in report["client_received_agreed_commands"]}) == 1
        for role, native in report["native"].items():
            log = verify_reference(native["log"]).read_text(encoding="utf-8", errors="replace")
            match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", log)
            assert match is not None
            assert int(match[1]) == native["assertions"] > 0
            assert int(match[2]) == native["cases"] == 1
            expected_layer = 0 if role == "server" else 1
            assert all(int(row[1]) == expected_layer for row in report["views"][role])
        assert [row["frame"] for row in report["frames"]] == list(FRAMES)
        # Each same-peer repeat is byte-exact across all126 decoded chunks.
        # The only peer difference may be the seven explicitly native local
        # unit fields;all remaining195 PATS fields and125 whole chunks match.
        for checkpoint in report["frames"]:
            data = {}
            for role, reference in checkpoint["files"].items():
                native_path = verify_reference(reference)
                data[role] = world(native_path)
                repeated = world(verify_reference(checkpoint["pure_repeats"][role]))
                assert data[role] == repeated
                assert {key: hashlib.sha256(raw).hexdigest() for key, raw in data[role].items()} == checkpoint["complete_chunk_sha256"][role]
            assert data["server"].keys() == data["client"].keys()
            changed = {key for key in data["server"] if data["server"][key] != data["client"][key]}
            assert changed == {"PATS"}
            tables = {role: saved_table(ROOT / reference["path"], b"PATS") for role, reference in checkpoint["files"].items()}
            assert set(tables["server"]) == set(tables["client"]) == {0}
            assert tables["server"][0].keys() == tables["client"][0].keys()
            assert set(checkpoint["native_local_units"]) == set(LOCAL_UNITS)
            for field, default in LOCAL_UNITS.items():
                assert tables["server"][0][field] == 0
                assert tables["client"][0][field] == default
                assert checkpoint["native_local_units"][field] == {"server": 0, "client": default}
            canonical = {role: {field: value for field, value in table[0].items() if field not in LOCAL_UNITS}
                         for role, table in tables.items()}
            assert canonical["server"] == canonical["client"]
            assert len(canonical["server"]) == checkpoint["canonical_settings_equal"] == 195
            assert checkpoint["complete_peer_chunks_equal"] == 125
        records[cfg] = {"report": identity(path), "actual": report}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md17B;four actual agreed frames per native build,complete native owners and195 gameplay settings equal;only seven declared NoNetworkSync unit preferences differ",
              "controller": identity(__file__), "registration": identity(ROOT / "src/tests/tbm_reuse.cpp"),
              "records": records, "native_cases": 4,
              "native_assertions": sum(native["assertions"] for row in records.values() for native in row["actual"]["native"].values()),
              "complete126_chunk_purity_pairs": 16, "same_frame_peer_pairs": 8,
              "exact_whole_peer_chunks_per_pair": 125, "exact_gameplay_setting_fields_per_pair": 195,
              "native_local_unit_fields": LOCAL_UNITS,
              "remaining": "Player-issued company commands,active excavation/wagon waits,additional failure/pause/time-mode joins and selected tube/thermal lifecycle matrix remain subsequent scope;no combined orbital acceptance."}
    if focus == "company":
        previous = ROOT / "logs/network/paid-ground/20261019-native-01/trace-verification.json"
        assert identity(previous)["sha256"] == "ec3a2db9bd75abbd1f5efc328e4310b2431a03160cde886df7ddb68f05c29397"
        result.update(focus=focus, preceding_trace_handoff=identity(previous), company_commands=companies,
                      company_observer=identity(ROOT / "src/tests/ground_network_commands_fixture.h"),
                      remaining="Active excavation/wagon waits,additional failure/pause/time-mode joins and selected tube/thermal lifecycle matrix remain subsequent scope;no combined orbital acceptance.")
    target = (ROOT / output_path).resolve()
    # Retained acceptance must never overwrite historical proof or introduce
    # output into immutable source/fixture ownership.
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(target), "native_assertions": result["native_assertions"],
                      "same_frame_peer_pairs": 8, "pure_pairs": 16}), flush=True)
    return result


def main():
    """Seal explicit independent real-peer captures without searching older logs.

    @return None;any unexpected gameplay or diagnostic difference refuses publication.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--focus", choices=("baseline", "company"), default="baseline")
    args = parser.parse_args()
    accept(args.report, args.output, args.focus)


if __name__ == "__main__":
    main()
