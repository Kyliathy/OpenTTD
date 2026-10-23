"""Seal real native paused clocks/state and complete post-resume peer equality.

@file ground_network_pause_report.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group19B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Paused observations align on a frozen gameplay tick;advancing network frame numbers remain explicit.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from ground_network_trace import world
from ground_network_report import verify_reference
from ground_network_active_report import verify_checkpoint
from ground_network_delivered_report import useful_ledger
from construction_provider_return import identity
from save_tables import saved_table

PREVIOUS = "logs/network/paid-ground/20261019-native-01/delivered-verification.json"
PREVIOUS_SHA = "4585dc35329ff2991cc0b1d917f73c57aabe46d519d0723be585f5a8318951c7"


def accept(report_paths, output_path):
    """Verify actual native Pause/unpause,full frozen state and resumed paid-world parity.

    @param report_paths Two exact pause reports,one per independent native build.
    @param output_path New retained pause/pace handoff below repository logs.
    @return Complete clock/state/command/frozen-range/resumed-original evidence.
    """
    previous_path = ROOT / PREVIOUS
    assert identity(previous_path)["sha256"] == PREVIOUS_SHA
    previous = json.loads(previous_path.read_text(encoding="utf-8"))
    records = {}
    # Real pause requires independent host/client observations and actual
    # agreed commands in both builds,not a private pause bit or replay clock.
    for name in report_paths:
        path = (ROOT / name).resolve()
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo")
        assert cfg not in records
        assert report["focus"] == "pause-calendar"
        assert report["tag"] == "[.ground-network-pause]"
        assert report["children_reaped"] is True
        assert report["native_version"] == 434
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        accepted = previous["records"][cfg]["tube-queue"]["actual"]
        assert report["original_input"] == accepted["original_input"]
        assert report["original_delivered"] == accepted["original_delivered"]
        for reference in [report["controller"], report["observer"], report["binary"], report["original_input"],
                          report["content"], report["handoff"], *report["sources"],
                          *report["diagnostics"].values(), *report["configs"].values()]:
            verify_reference(reference)
        settings = saved_table(ROOT / report["original_input"]["path"], b"PATS")[0]
        assert settings["economy.timekeeping_units"] == 0
        assert settings["economy.minutes_per_calendar_year"] == 0
        assert report["joins"]["server"] == report["joins"]["client"]
        source_tick = report["original_delivered"]["clock"]
        assert int(report["joins"]["client"][0][2]) == source_tick + 1
        assert report["states"]["server"] == report["states"]["client"]
        assert [int(row[1]) for row in report["states"]["client"]] == [source_tick + offset + 1 for offset in (128, 256, 384, 512)]
        commands = report["commands"]
        assert len(commands["server"]) == 6
        assert len(commands["client"]) == 4
        assert commands["server"][-4:] == commands["client"]
        for line, payload in zip(commands["client"], ("0000", "0200", "0001", "0000"), strict=True):
            assert re.fullmatch(r"cmd: [0-9a-f]+; [0-9a-f]+; ff; 00000048; 00000000; " + payload + r" \(CmdPause\)", line)
        agreed = report["client_received_agreed_commands"]
        assert len(agreed) == 4
        assert all(row[0] == "72" for row in agreed)
        assert agreed[0][1] == agreed[1][1]
        assert int(agreed[2][1]) < int(agreed[3][1])
        core_fields = ("tick", "calendar", "calendar_fract", "economy", "economy_fract", "rng0", "rng1", "speed")
        frozen = {}
        assert len(report["frames"]) == 8
        assert [row["stage"] for row in report["frames"][-2:]] == ["paused-a", "paused-b"]
        # Each peer must leave all126 serialized chunks unchanged across the
        # real paused interval. Network frames advance and are not mislabelled
        # as one agreed frame;the common native gameplay clock is frozen.
        for role, rows in report["pauses"].items():
            assert [row["stage"] for row in rows] == ["paused-a", "paused-b"]
            a, b = rows
            assert int(b["elapsed_ms"]) >= 500
            assert int(b["frame"]) > int(a["frame"])
            assert int(a["frame"]) >= int(agreed[2][1])
            assert int(b["frame"]) < int(agreed[3][1])
            assert a["speed"] == b["speed"] == "100"
            frozen[role] = {key: a[key] for key in core_fields}
            assert frozen[role] == {key: b[key] for key in core_fields}
            assert world(verify_reference(report["frames"][6]["files"][role])) == world(verify_reference(report["frames"][7]["files"][role]))
            # The same512 unpaused opportunities must end at the exact prior
            # uninterrupted native world,including all local/diagnostic chunks.
            # No date,revision,camera,setting or consumed-meter field is ignored.
            assert world(verify_reference(report["frames"][5]["files"][role])) == world(verify_reference(accepted["frames"][5]["files"][role]))
        assert frozen["server"] == frozen["client"]
        for index, checkpoint in enumerate(report["frames"]):
            verify_checkpoint(checkpoint)
            if index >= 6:
                assert checkpoint["comparison_kind"] == "frozen-gameplay-tick"
                assert checkpoint["frame"] is None
                assert checkpoint["actual_peer_frames"] == {role: int(rows[index - 6]["frame"]) for role, rows in report["pauses"].items()}
            else:
                assert checkpoint["comparison_kind"] == "agreed-network-frame"
        for role, native in report["native"].items():
            output = verify_reference(native["log"]).read_text(encoding="utf-8", errors="replace")
            counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
            assert counts is not None
            assert int(counts[1]) == native["assertions"] > 0
            assert int(counts[2]) == native["cases"] == 1
            assert all(int(row[1]) == (0 if role == "server" else 1) for row in report["views"][role])
        assert report["last_ack"] - report["first_ack"] == report["acknowledged_frames"] >= 444
        # The family decoder sees the same source and six actual unpaused
        # checkpoints;this alias selects its tube-ledger reader only and never
        # modifies the retained native report or any gameplay observation.
        paid = useful_ledger({**report, "focus": "tube-queue", "frames": report["frames"][:6]}, report["original_delivered"])
        records[cfg] = {"report": identity(path), "actual": report, "frozen_clocks": frozen["server"],
                        "all126_chunks_frozen_per_peer": True, "all126_chunks_match_uninterrupted_per_peer": True,
                        "resumed_paid_progression": paid}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md19B;real native Pause/unpause,complete frozen gameplay during advancing network frames,and exact uninterrupted-world parity after512 actual unpaused opportunities",
              "controller": identity(__file__), "preceding_delivered_handoff": identity(previous_path),
              "native_pause_observer": identity(ROOT / "src/tests/ground_network_pause_fixture.h"),
              "registration": identity(ROOT / "src/tests/tbm_reuse.cpp"), "records": records, "native_cases": 4,
              "native_assertions": sum(native["assertions"] for row in records.values() for native in row["actual"]["native"].values()),
              "complete126_chunk_purity_pairs": 32, "same_agreed_frame_peer_pairs": 12,
              "same_frozen_tick_peer_pairs": 4, "complete126_chunk_frozen_ranges": 4,
              "complete126_chunk_uninterrupted_comparisons": 4,
              "source_time_mode": {"units": "Calendar", "minutes_per_calendar_year": 0, "calendar_progress": "retained fixture frozen policy"},
              "supported_multiplayer_pacing": ["normal", "paused"], "multiplayer_fast_forward": "disabled by native toolbar/keyboard controls",
              "build_logs": {"Debug": identity(ROOT / "logs/network/paid-ground/20261019-native-01/debug-build-9.log"),
                             "RelWithDebInfo": identity(ROOT / "logs/network/paid-ground/20261019-native-01/optimized-build-5.log")},
              "remaining": "Legally initialized calendar-rate/Wallclock starting modes and relevant native power-link/lifecycle failure scenarios remain before master11.3;no full19.2 or orbital acceptance inferred."}
    target = (ROOT / output_path).resolve()
    # Retain a new exact proof;never rewrite the original paid input or prior
    # uninterrupted acceptance merely because the new run includes a pause.
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(target), "native_assertions": result["native_assertions"], "frozen_all126_ranges": 4, "resumed_all126_originals": 4}), flush=True)
    return result


def main():
    """Seal explicit independent native pause records without a broad regression.

    @return None;unknown state changes,missing real commands or altered originals refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
