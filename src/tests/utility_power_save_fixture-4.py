"""Normal executable acceptance of directed utility state, independently of hidden native test hooks.

@file utility_power_save_fixture-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 8B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md distributed loading
"""

import hashlib
import argparse
import os
import subprocess

from utility_power_save_fixture import ROOT, DIRECTORY, utility_chunks
from multilayer_state_fixture import log_path
from geology_save_fixture import chunks
from geology_generation_fixture import read_stream


from functools import partial
from pathlib import Path
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native
from multilayer_state_fixture import log_path
CONTEXT = current_run_context("utilities", "utility-power-save-fixture-4")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()
INPUTS = ROOT / "src/tests/fixtures/utility"
DIRECTORY = CONTEXT.saves
log_path = partial(log_path, context=CONTEXT)


def main():
    """Load/resave one current directed-link world through the normal built game without a fixture observer.

    @return None; preserve source/startup bytes and compare exact utility chunks against the ordinary save.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--town", action="store_true", help="check the current applied native city effect and ordinary CITY bytes")
    parser.add_argument("--retired", action="store_true", help="check current retired company accounts through the normal game")
    arguments = parser.parse_args()
    source = resolve_input_path(INPUTS / ("utility-town-applied.sav" if arguments.town else "utility-transfer-rebuilt.sav"))
    output = DIRECTORY / ("utility-town-normal-resaved.sav" if arguments.town else "utility-normal-resaved.sav")
    if arguments.retired:
        source = resolve_input_path(INPUTS / "utility-company-retired.sav")
        output = DIRECTORY / "utility-company-normal-resaved.sav"
    expected = utility_chunks(source)
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    previous_output_time = output.stat().st_mtime_ns if output.exists() else None
    startup = RUNTIME / "scripts/game_start.scr"
    existed = startup.exists()
    previous = startup.read_bytes() if existed else None
    log = log_path("r2mlvs3mp-w-utility-town-normal-loader.log" if arguments.town else "r2mlvs3mp-v-utility-normal-loader.log")
    if arguments.retired:
        log = log_path("r2mlvs3mp-z-a-company-normal-loader.log")
    try:
        startup.write_text("# @see r2mlvs12-solar-farms-mirrors-and-eti.md8B normal directed utility acceptance.\n"
                           "set savegame_format lzma\n"
                           f'save "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
        environment = dict(os.environ, OTTD_DEBUG="misc=3,sl=3,grf=2")
        runner = ROOT / "mlm/tools/run/run_game_logged.sh"
        # The established runner refreshes only the console subsystem of the
        # ordinary built executable; no Catch observer or alternate loader runs.
        run_native(CONTEXT, ["C:/Program Files/Git/bin/bash.exe", runner.as_posix(), (ROOT / "build").as_posix(),
                        "Debug", log.as_posix(), "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X",
                        "-I", "original_dos", "-c", "native-art.cfg", "-g", source.as_posix()],
                       cwd=ROOT / "build", env=environment, creationflags=subprocess.CREATE_NO_WINDOW,
                       timeout=110, check=True)
        assert output.is_file(), "normal loader did not reach ordinary save"
        assert output.stat().st_mtime_ns != previous_output_time, "normal loader reused a stale prior output"
        assert utility_chunks(output) == expected, "normal executable changed directed utility authority"
        if arguments.town:
            # Actual town rate/counter/custom ownership stays in its ordinary
            # chunk; utility-side metadata cannot stand in for physical effect.
            before = next(row["raw"] for row in chunks(read_stream(source)[1]) if row["name"] == b"CITY")
            after = next(row["raw"] for row in chunks(read_stream(output)[1]) if row["name"] == b"CITY")
            assert before == after, "normal executable changed actual saved town effect"
        print("Normal utility acceptance: original executable loader/writer, exact current directed-flow chunks, no fixture hook", flush=True)
    finally:
        if existed:
            startup.write_bytes(previous)
        else:
            startup.unlink()
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash, "directed native input changed"


if __name__ == "__main__":
    main()
