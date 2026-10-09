"""Bounded actual directed utility flow, original-source import use and physical portal loss/rebuild files.

@file utility_power_save_fixture-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 8
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md lifecycle authority
"""

import hashlib
import os
import subprocess

from utility_power_save_fixture import ROOT, DIRECTORY, utility_chunks
from multilayer_state_fixture import log_path


from functools import partial
from pathlib import Path
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native
from multilayer_state_fixture import log_path
CONTEXT = current_run_context("utilities", "utility-power-save-fixture-2")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()
INPUTS = ROOT / "src/tests/fixtures/utility"
DIRECTORY = CONTEXT.saves
log_path = partial(log_path, context=CONTEXT)


def run_case(mode, source, prefix="r2mlvs3mp-v", context=CONTEXT):
    """Load a real world and verify its directed physical/ledger state in an isolated process.

    @param mode Create four actual flow files or inspect one retained transfer phase.
    @param source Original paid native Deep loop or generated phase, never rewritten during load.
    @param prefix Owning focused group's unique log basename prefix.
    @param context Explicit caller context for output/runtime ownership.
    @return None; retain bounded owning-plan native evidence and require successful complete acceptance.
    """
    log = log_path(f"{prefix}-power-transfer-{mode}.log", context=context)
    environment = dict(os.environ, OTTD_UTILITY_FILE_CASE=mode, OTTD_UTILITY_FILE_INPUT=str(source))
    with log.open("wb") as sink:
        run_native(context, [str(ROOT / "build/Debug/openttd_test.exe"), "[.utility-power-transfer-file]"],
                       cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT,
                       creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence
    assert f"UTILITY_TRANSFER case={mode} original_source=true physical_portal=true" in evidence
    print(f"{mode}: real directed transfer boundary passed", flush=True)


def main():
    """Run five native topology/flow cases separately from publication and settlement bursts.

    @return None; preserve startup/source bytes and require exact authoritative chunks through every reload.
    """
    source = ROOT / "src/tests/fixtures/haulage/spoil-loop-deep/m1-loop-deep-4-complete-loop.sav"
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    startup = RUNTIME / "scripts/game_start.scr"
    existed = startup.exists()
    previous = startup.read_bytes() if existed else None
    DIRECTORY.mkdir(parents=True, exist_ok=True)
    try:
        startup.write_text("# @see r2mlvs12-solar-farms-mirrors-and-eti.md8 physical utility flow proof.\nutility_transfer_check\n", encoding="utf-8")
        run_case("create", source)
        phases = (("transferred", "utility-transferred.sav"), ("consumed", "utility-import-consumed.sav"),
                  ("loss", "utility-transfer-loss.sav"), ("rebuilt", "utility-transfer-rebuilt.sav"))
        # Each real physical phase reloads unchanged: imports retain original
        # generation IDs, settled use and retired/rebuilt link identities.
        for mode, name in phases:
            path = DIRECTORY / name
            expected = utility_chunks(path)
            run_case(mode, path)
            assert utility_chunks(DIRECTORY / f"utility-transfer-{mode}-reloaded.sav") == expected
        print("Directed utility proof: exact flow/import/use/loss/rebuild chunks, actual portal lifecycle and no same-interval re-export", flush=True)
    finally:
        if existed:
            startup.write_bytes(previous)
        else:
            startup.unlink()
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash, "retained paid Deep source changed"


if __name__ == "__main__":
    main()
