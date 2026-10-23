"""Seal real paid-ground peer equality under each legally initialized native clock policy.

@file ground_network_clock_report.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group19C.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Complete native state and original paid owners supplement actual calendar/economy ratios.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from ground_network_clock_source import MODES
from ground_network_report import verify_reference
from ground_network_active_report import verify_checkpoint
from ground_network_trace import world
from construction_provider_return import identity
from save_tables import saved_table

INPUTS = "logs/network/paid-ground/20261019-native-01/clock-inputs.json"
INPUTS_SHA = "3a07d19b75969ff44033a9ed28fb0db4a13cd0fcca5fdca83cbaa8d529fd731d"


def clock_elapsed(initial, later, prefix):
    """Measure native date/fraction progress without inventing a second clock owner.

    @param initial Complete original DATE row at the first unpaused opportunity.
    @param later Complete DATE row after exactly512 further native opportunities.
    @param prefix Empty for calendar technology time,or economy_ for economic scheduling.
    @return Native day/fraction progress measured in74-fraction days.
    """
    return (later[prefix + "date"] - initial[prefix + "date"]) * 74 + (
        later[prefix + "date_fract"] - initial[prefix + "date_fract"])


def accept(report_paths, output_path):
    """Verify actual independent peers,complete owners,clock policy and paid continuation.

    @param report_paths Eight real native peer reports:four starting policies per build.
    @param output_path New immutable time-mode handoff beneath repository logs.
    @return Complete exact accepted evidence;old source receipts remain dated immutable lineage.
    """
    inputs_path = ROOT / INPUTS
    assert identity(inputs_path)["sha256"] == INPUTS_SHA
    selection = json.loads(inputs_path.read_text(encoding="utf-8"))
    records = {cfg: {} for cfg in ("Debug", "RelWithDebInfo")}
    # Independent applications must join every legal source and continue its
    # original physical paid work. Replaying a command list cannot certify join.
    for name in report_paths:
        path = (ROOT / name).resolve()
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg, focus = report["configuration"], report["focus"]
        assert cfg in records
        assert focus in MODES and focus not in records[cfg]
        assert report["tag"] == "[.ground-network-clock-peer]"
        assert report["native_version"] == 434
        assert report["children_reaped"] is True
        assert report["handoff"] == identity(inputs_path)
        original = selection["records"][cfg][focus]
        assert report["original_input"] == original["input"]
        assert report["original_work"] == original
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        for reference in [report["controller"], report["observer"], report["binary"], report["original_input"],
                          report["content"], *report["sources"], *report["diagnostics"].values(), *report["configs"].values()]:
            verify_reference(reference)
        assert report["work"]["server"] == report["work"]["client"]
        work = report["work"]["client"]
        assert len(work) == 6
        initial, following, last = work[0], work[1], work[-1]
        assert initial["tick"] == original["clock"] + 1
        assert following["tick"] == initial["tick"] + 1
        assert following["frame"] == initial["frame"] + 1
        assert [row["tick"] - initial["tick"] for row in work[2:]] == [128, 256, 384, 512]
        assert initial["cursor"] == original["job"]["cursor"]
        assert initial["phase"] == original["project"]["phase"]
        assert initial["blocked"] == 0
        assert (initial["earth"], initial["rock"], initial["cost"]) == (
            original["job"]["earth"], original["job"]["rock"], original["job"]["cost"])
        assert all(row["project"] == original["project"]["id"] for row in work)
        assert all(row["job"] == original["job"]["id"] for row in work)
        assert last["partial"] > initial["partial"] or last["cursor"] > initial["cursor"]
        assert report["states"]["server"] == report["states"]["client"]
        assert report["joins"]["server"] == report["joins"]["client"]
        assert report["last_ack"] - report["first_ack"] == report["acknowledged_frames"] >= 444
        commands = report["commands"]
        assert len(commands["server"]) == 4
        assert len(commands["client"]) == len(report["client_received_agreed_commands"]) == 2
        assert commands["server"][-2:] == commands["client"]
        assert commands["client"][0].endswith("0000 (CmdPause)")
        assert commands["client"][1].endswith("0200 (CmdPause)")
        for role, native in report["native"].items():
            output = verify_reference(native["log"]).read_text(encoding="utf-8", errors="replace")
            counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
            assert counts is not None
            assert int(counts[1]) == native["assertions"] > 0
            assert int(counts[2]) == native["cases"] == 1
            assert "Sync error detected" not in output
            assert "sync_err:" not in output
            assert all(int(row[1]) == (0 if role == "server" else 1) for row in report["views"][role])
        stages = ["joined", "joined-next", *[f"f{row['frame']}" for row in work[2:]]]
        assert [row["stage"] for row in report["frames"]] == stages
        units, rate = MODES[focus]
        # Every full native checkpoint preserves the legally selected saved
        # policy;no live unsupported settings change can fabricate a ratio.
        for checkpoint, observation in zip(report["frames"], work, strict=True):
            verify_checkpoint(checkpoint)
            assert checkpoint["frame"] == observation["frame"]
            for reference in checkpoint["files"].values():
                saved = ROOT / reference["path"]
                settings = saved_table(saved, b"PATS")[0]
                assert settings["economy.timekeeping_units"] == units
                assert settings["economy.minutes_per_calendar_year"] == rate
                assert saved_table(saved, b"DATE")[0]["tick_counter"] == observation["tick"]
        first_file = ROOT / report["frames"][0]["files"]["server"]["path"]
        last_file = ROOT / report["frames"][-1]["files"]["server"]["path"]
        first_clock = saved_table(first_file, b"DATE")[0]
        last_clock = saved_table(last_file, b"DATE")[0]
        calendar = clock_elapsed(first_clock, last_clock, "")
        economic = clock_elapsed(first_clock, last_clock, "economy_")
        assert calendar == (0 if rate == 0 else 512 * 12 // rate)
        assert economic == 512
        first_cash = saved_table(first_file, b"PLYR")[0]["money"]
        last_cash = saved_table(last_file, b"PLYR")[0]["money"]
        assert 0 < last_cash < first_cash
        records[cfg][focus] = {"report": identity(path), "actual": report,
                               "paid_cash": {"initial": first_cash, "later": last_cash,
                                             "native_debit": first_cash - last_cash},
                               "clock_progress": {"calendar": calendar, "economic": economic,
                                                  "native_opportunities": 512,
                                                  "initial": first_clock, "later": last_clock}}
    assert all(set(rows) == set(MODES) for rows in records.values())
    # Matching tick opportunities also compare every complete native chunk
    # across builds,including accounting,physical parts,orders,packets and RNG.
    for focus in MODES:
        for left, right in zip(records["Debug"][focus]["actual"]["frames"],
                               records["RelWithDebInfo"][focus]["actual"]["frames"], strict=True):
            for role in ("server", "client"):
                assert world(ROOT / left["files"][role]["path"]) == world(ROOT / right["files"][role]["path"])
    assertions = sum(native["assertions"] for modes in records.values() for row in modes.values()
                     for native in row["actual"]["native"].values())
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md19C.2;real independent fresh native joins on unchanged legal starting-mode paid originals;all authoritative native owners and measured calendar/economy progress;declared fixtures,not normal-newgame bootstrap",
              "controller": identity(__file__), "inputs": identity(inputs_path), "records": records,
              "native_observer_sources": [identity(ROOT / name) for name in (
                  "src/tests/ground_network_active_fixture.h", "src/tests/ground_network_fixture.h",
                  "src/tests/tbm_reuse.cpp", "src/openttd.cpp", "src/timer/timer_game_calendar.cpp",
                  "src/timer/timer_game_economy.cpp")],
              "native_cases": 16, "native_assertions": assertions,
              "complete_peer_pairs": 48, "all_chunk_pure_pairs": 96,
              "all_chunk_cross_build_pairs": 48,
              "remaining": "Relevant actual peer power-link/lifecycle/date failure scope before master11.3 closure;orbital and normal-newgame acceptance remain separate."}
    target = (ROOT / output_path).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_assertions": assertions}), flush=True)
    return result


def main():
    """Seal only complete focused actual native peer evidence.

    @return None;any missing join,state,clock,paid continuation or provenance refuses acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
