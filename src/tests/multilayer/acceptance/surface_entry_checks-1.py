"""Create an authentic ordinary new-game company for the Surface factory chain.

@file surface_entry_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BB.
The ordinary GUI new-game path creates the company and its initial bank loan.
The startup script only pauses, saves and exits: no supplied stock, floor,
facility, vehicle, work or cash is installed by this harness.
"""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from space_preview_checks import ROOT, identity, chunks
from artifact_paths import create_run_context
from runner import console_executable, run_native

sys.path.insert(0, str(ROOT / "src/tests"))
from tbm_logistics_save_fixture import saved_table

SOURCES = ("src/openttd.cpp", "src/genworld.cpp", "src/company_cmd.cpp", "src/misc_cmd.cpp",
           "src/table/settings/world_settings.ini", "src/table/settings/difficulty_settings.ini",
           "src/layer_industry_policy.cpp", "src/multilayer_recipe.cpp")


def generate(configuration, seed):
    """Generate and retain one actual ordinary new-game starting world.

    @param configuration Complete current ordinary game executable.
    @param seed Explicit reproducible native terrain/geology/asteroid seed.
    @return Exact application, settings, original save and native company provenance.
    """
    context = create_run_context("space", "surface-entry-newgame")
    context.prepare_runtime()
    application = ROOT / "build" / configuration / "openttd.exe"
    executable = console_executable(context, application)
    config = context.config_path.read_text(encoding="utf-8")
    assert config.count("[game_creation]") == 1
    # These are ordinary pre-game controls, with an unchanged native initial
    # loan. A larger published bank ceiling permits later actual loan commands;
    # it does not grant an upfront company balance or erase incurred debt.
    config += ("gameplay_profile = multilayer_industry\nlandscape = temperate\nland_generator = 0\n"
               "\n[difficulty]\nmax_loan = 5000000\nmax_no_competitors = 0\nnumber_towns = 2\n"
               "industry_density = 5\nterrain_type = 0\nquantity_sea_lakes = 0\n"
               "\n[economy]\nmultiple_industry_per_town = true\n")
    context.config_path.write_text(config, encoding="utf-8")
    output = context.saves / "surface-entry-original-440.sav"
    script = context.runtime_dir / "scripts/game_start.scr"
    script.write_text(f'pause\nset savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = subprocess.SW_HIDE
    log = context.logs / "newgame.log"
    # Null video deliberately creates a spectator, not a normal player. Use
    # the actual GUI startup hidden so its own CompanyStartup path supplies
    # the same bank-funded company a player receives through New Game.
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), "-v", "win32", "-r", "640x480", "-s", "null", "-m", "null",
                            "-x", "-X", "-I", "original_dos", "-d", "sl=3", "-c", str(context.config_path),
                            "-G", str(seed), "-t", "2000", "-g"], cwd=context.runtime_dir,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110, startupinfo=startup,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and output.is_file(), log
    assert not re.search(r"(?:save failed|load failed|error loading|assertion failed)", terminal, re.I), log
    geometry, = saved_table(output, b"MAPS").values()
    profile, = saved_table(output, b"MLMD").values()
    settings, = saved_table(output, b"PATS").values()
    assert geometry == dict(dim_x=64, dim_y=64, layers=4)
    assert profile["profile_id"] == 1 and profile["policy_flags"] == 0
    assert settings["difficulty.max_loan"] == 5000000
    assert settings["game_creation.generation_seed"] == seed
    assert settings["game_creation.starting_year"] == 2000
    companies = saved_table(output, b"PLYR")
    company, = companies.values()
    assert company["money"] == company["current_loan"] > 0
    assert company["is_ai"] == 0
    assert company["money"] <= 5000000
    assert all(value == 0 for value in company["yearly_expenses"])
    assert company["cur_economy[0].income"] == 0
    assert not saved_table(output, b"CAPA")
    vehicles = saved_table(output, b"VEHS")
    # Normal industry generation may emit ambient smoke/effects. They are
    # distinct native types, never an inherited paid carrier or Space hull.
    assert all(vehicle["type"] in (4, 5) for vehicle in vehicles.values()), log
    # Native world generation may create neutral oil-rig stations. None may
    # grant a player-owned launch center or an inherited freight origin.
    stations = saved_table(output, b"STNN")
    for station in stations.values():
        owners = [value for key, value in station.items() if key.endswith("owner")]
        assert all(owner != next(iter(companies)) for owner in owners), station
    report = dict(_documentation=__doc__, complete=True, configuration=configuration, seed=seed, year=2000,
                  application=identity(application), original=identity(output), config=identity(context.config_path),
                  startup_script=identity(script), terminal=identity(log),
                  geometry=geometry, profile=profile,
                  native_game_identity=saved_table(output, b"DATE")[0]["id"],
                  company_id=next(iter(companies)), money=company["money"], loan=company["current_loan"],
                  initial_company_source="MakeNewGameDone -> DoStartupNewCompany(false) -> native INITIAL_LOAN",
                  maximum_bank_setting=5000000, cash_granted_by_harness=0, packet_count=0,
                  company_vehicle_count=0, native_ambient_vehicles=len(vehicles),
                  original_industries=saved_table(output, b"INDY"),
                  native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))],
                  sources=[identity(ROOT / name) for name in SOURCES] + [identity(Path(__file__))],
                  factory_or_launch_entry_complete=False)
    target = context.reports / "newgame.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)
    return identity(target)


def main():
    """Create one explicit bounded original world and its fresh membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--seed", type=int, default=420)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    reference = generate(args.configuration, args.seed)
    target.write_text(json.dumps(dict(_documentation=__doc__, report=reference), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
