"""Produce one declared paid ground source under a legally configured native starting time mode.

@file ground_network_clock_source.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group19C.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Next-game settings activate before paid owners exist;this does not claim normal-newgame bootstrap.
"""

import argparse
import json
import os
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from runner import current_run_context, run_native
from construction_provider_return import identity
from ground_network_trace import world
from save_tables import saved_table

MODES = {"calendar-default": (0, 12), "wallclock-default": (1, 12),
         "wallclock-slow": (1, 24), "wallclock-frozen": (1, 0)}
TAG = "[.ground-network-clock-source]"
SOURCES = ("src/newgame_settings.h", "src/openttd.cpp", "src/settings.cpp", "src/settings_type.h",
           "src/table/settings/economy_settings.ini", "src/timer/timer_game_calendar.cpp",
           "src/timer/timer_game_economy.cpp", "src/tests/ground_network_clock_fixture.h",
           "src/tests/underground_workflow_fixture.h", "src/tests/underground_workflow.cpp",
           "src/tests/underground_depot_fixture.h", "src/tests/tbm_reuse.cpp")


def produce(configuration, focus):
    """Earn actual native unfinished paid work after public next-game mode activation.

    @param configuration Exact independent native test application build.
    @param focus Calendar/default or one explicit Wallclock rate declared in MODES.
    @return Exact source report,with full native pure-repeat and original bill/owner observations.
    """
    units, rate = MODES[focus]
    context = current_run_context("network", "paid-ground")
    binary = ROOT / f"build/{configuration}/openttd_test.exe"
    env = dict(os.environ, OTTD_GROUND_CLOCK_UNITS=str(units), OTTD_GROUND_CLOCK_RATE=str(rate))
    result = run_native(context, [binary, TAG], cwd=ROOT / "build", env=env,
                        capture_output=True, timeout=110, check=False)
    raw = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    log = context.logs / f"clock-source-{focus}.log"
    log.write_text(raw + f"\nNATIVE_EXIT_CODE={result.returncode}\n", encoding="utf-8")
    assert result.returncode == 0, log
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", raw)
    assert counts is not None, log
    assert int(counts[2]) == 1
    match = re.search(r"GROUND_CLOCK_SOURCE ([^\r\n]+)", raw)
    assert match is not None, log
    facts = {key: int(value) for key, value in (field.split("=", 1) for field in match[1].split())}
    assert facts["units"] == units
    assert facts["rate"] == rate
    assert facts["earth"] > 0
    assert facts["rock"] > 0
    assert facts["setup"] > 0
    assert facts["work"] > 0
    assert facts["cash"] < 10000000 - facts["setup"]
    original = context.saves / "ground-clock-original.sav"
    repeated = context.saves / "ground-clock-original-repeat.sav"
    assert world(original) == world(repeated), "Native source observation changed serialized owners"
    settings = saved_table(original, b"PATS")[0]
    assert settings["economy.timekeeping_units"] == units
    assert settings["economy.minutes_per_calendar_year"] == rate
    projects = saved_table(original, b"TBMP")
    assert set(projects) == {0}
    project = projects[0]
    job = next(row for row in saved_table(original, b"XJOB").values() if row["id"] == project["job"])
    assert 0 < job["cursor"] < job["routes"]
    assert job["earth"] == facts["earth"]
    assert job["rock"] == facts["rock"]
    assert job["cost"] == facts["work"]
    assert project["id"] == facts["project"]
    assert job["id"] == facts["job"]
    clock = saved_table(original, b"DATE")[0]
    assert clock["tick_counter"] == facts["tick"]
    # The declared fixture starts on 1 January 1980 with zero clock fractions.
    # Actual timer owners must accrue every paid opportunity;setting a policy
    # alone cannot certify an advancing calendar or ordinary economic billing.
    calendar_ticks = (clock["date"] - 723180) * 74 + clock["date_fract"]
    expected_calendar = 0 if rate == 0 else facts["tick"] * 12 // rate
    assert calendar_ticks == expected_calendar, (clock, facts)
    assert clock["economy_date"] * 74 + clock["economy_date_fract"] == facts["tick"]
    report = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md19C.1;public next-game configuration/activation before fresh paid owners,actual generated geology/paid Surface setup/commissioning/unfinished cut;declared fixture,not normal-newgame bootstrap",
              "configuration": configuration, "focus": focus, "units": units, "rate": rate, "tag": TAG,
              "native_version": 434, "controller": identity(__file__), "binary": identity(binary),
              "sources": [identity(ROOT / name) for name in SOURCES],
              "native": {"log": identity(log), "assertions": int(counts[1]), "cases": 1},
              "observations": facts, "original": identity(original), "pure_repeat": identity(repeated),
              "project": project, "job": job, "clock": clock,
              "native_clock_progress": {"calendar_ticks": calendar_ticks, "economic_ticks": facts["tick"]},
              "fixture_classification": "declared generated rock/Surface ground,10million starting cash and command-funded logistics;original paid cuts/actual wagons;no normal-newgame bootstrap claim",
              "remaining": "Independent configuration source identity and actual native fresh-peer initial/next/later time-mode comparisons."}
    path = context.output("reports", f"ground-clock-{configuration}-{focus}-source.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(path), "native_assertions": int(counts[1]), "paid": facts}), flush=True)
    return path


def main():
    """Select exactly one legal starting mode and independent native producer.

    @return None;source setup/refusal or missing actual paid progress publishes no acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=MODES, required=True)
    args = parser.parse_args()
    produce(args.config, args.focus)


if __name__ == "__main__":
    main()
