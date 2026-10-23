"""Verify normal game loading/resaving of real first-plant service and loss files.

@file first_plant_service_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group9B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The production executable receives only ordinary pause/format/save/quit commands.
"""
import argparse
import json
from pathlib import Path
import subprocess

from first_plant_checks import CONTEXT, ROOT, identity, verify_original, saved_table
from first_plant_service_checks import complete_chunks
from runner import run_native, console_executable


def ordinary(config, label, source):
    """Load and resave one actual original with the built normal game.

    @param config Successfully built production configuration.
    @param label Explicit accepted checkpoint or interruption.
    @param source Exact repository-owned original; never changed by this process.
    @return Exact production/input/output/log identities and observed load-only initialization.
    """
    context = CONTEXT.for_feature("geothermal", f"first-plant-game-{config.lower()}-{label}")
    context.prepare_runtime()
    assert saved_table(source, b"DATE")[0]["pause_mode"] & 1, "Ordinary startup proof requires an already paused original."
    output = context.saves / "ordinary-resave.sav"
    script = context.runtime_dir / "scripts/game_start.scr"
    script.write_text("# @see r2mlvs12 Group9B and r2mlvs18 Group8FC ordinary production resave.\n"
                      f'set savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    executable = console_executable(context, ROOT / f"build/{config}/openttd.exe")
    log = context.logs / "ordinary.log"
    before_identity = identity(source)
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null",
                            "-x", "-X", "-I", "original_dos", "-c", str(context.config_path), "-g", str(source)],
                            cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=90, check=False)
    assert result.returncode == 0 and output.is_file(), log
    before, after, initialized = verify_original(source, output, startup_linkgraph=True)
    assert identity(source) == before_identity, "Normal loading changed its original input."
    created = sorted(set(saved_table(output, b"PLYR")) - set(saved_table(source, b"PLYR")))
    # The game-start script observes native afterload before a complete frame
    # clears the transient CargoDist wait. Every other complete chunk stays
    # exact; the preceding validator checks every clock field and only that
    # specific added pause bit. Older absence/player creation remain explicit.
    if label not in ("old430", "company"):
        original_version, original_chunks = complete_chunks(source)
        loaded_version, loaded_chunks = complete_chunks(output)
        assert original_version == loaded_version
        assert [(key, value) for key, value in original_chunks if key != "DATE"] == [(key, value) for key, value in loaded_chunks if key != "DATE"], label
    original_pause = saved_table(source, b"DATE")[0]["pause_mode"]
    loaded_pause = saved_table(output, b"DATE")[0]["pause_mode"]
    return {"stage": label, "game": identity(ROOT / f"build/{config}/openttd.exe"), "input": before_identity,
            "output": identity(output), "log": identity(log), "native_manager_initialized": initialized,
            "native_new_human_company": created, "startup_pause_before": original_pause, "startup_pause_after": loaded_pause,
            "owners_before": before, "owners_after": after}


def main():
    """Verify one configuration's bounded ordinary production load/save burst.

    @return None; retain current, interrupted and explicit older-absence proof.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-report", type=Path, required=True)
    parser.add_argument("--physical-report", type=Path, required=True)
    parser.add_argument("--owner-report", type=Path, required=True)
    args = parser.parse_args()
    parent, physical, owners = [json.loads(path.read_text()) for path in (args.original_report, args.physical_report, args.owner_report)]
    config = parent["configuration"]
    assert physical["configuration"] == owners["configuration"] == config
    cases = [(stage, ROOT / parent[stage]["path"]) for stage in ("connected", "partial", "complete")]
    cases += [("source", ROOT / physical["boundaries"]["source"]["path"]),
              ("all-sources", ROOT / owners["boundaries"]["all-sources"]["path"]),
              ("company", ROOT / owners["boundaries"]["company"]["path"]),
              ("old430", ROOT / parent["input"]["path"])]
    proofs = []
    # Separate process-local configs and output homes preserve the originals
    # while using the actual normal executable, without any test observer.
    for label, source in cases:
        proofs.append(ordinary(config, label, source))
    complete = next(row for row in proofs if row["stage"] == "complete")
    proofs.append(ordinary(config, "complete-repeat", ROOT / complete["output"]["path"]))
    report = {"_documentation": "r2mlvs12 Group9B; r2mlvs18 Group8FC; exact normal-game current/older/loss/repeated loading",
              "configuration": config, "controller": identity(__file__), "parents": [identity(path) for path in (args.original_report, args.physical_report, args.owner_report)],
              "ordinary_processes": proofs}
    path = CONTEXT.output("reports", f"first-plant-normal-game-{config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Normal {config}: {len(proofs)} exact ordinary resave processes\n{path}", flush=True)


if __name__ == "__main__":
    main()
