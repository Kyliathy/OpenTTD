"""Focused normal-game rejection and real Null cleanup of malformed electrical transport.

@file utility_power_save_fixture-8.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md Groups9-10
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint11.2
"""

import hashlib
import os
import subprocess
import uuid

from utility_power_save_fixture import ROOT, DIRECTORY


from functools import partial
from pathlib import Path
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native
from multilayer_state_fixture import log_path
CONTEXT = current_run_context("utilities", "utility-power-save-fixture-8")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()
INPUTS = ROOT / "src/tests/fixtures/utility"
DIRECTORY = CONTEXT.saves
log_path = partial(log_path, context=CONTEXT)


def main():
    """Reject one read-phase and one fixup-phase corruption through the normal loader's actual Null cleanup.

    @return None; immutable input/config/startup bytes and concrete rejection/cleanup logs are required.
    """
    sources = {"active": INPUTS / "utility-company-corrupt-active.sav",
               "balance": INPUTS / "utility-company-corrupt-balance.sav"}
    protected = {path: hashlib.sha256(path.read_bytes()).hexdigest()
                 for path in (*sources.values(), ROOT / "build/native-art.cfg")}
    startup = RUNTIME / "scripts/game_start.scr"
    existed = startup.exists()
    previous = startup.read_bytes() if existed else None
    prefix = "r2mlvs3mp-z-a-utility-null-" + uuid.uuid4().hex
    try:
        startup.write_text("# @see r2mlvs14-savegame-determinism-and-lifecycle.md10 partial-load rejection.\nquit\n", encoding="utf-8")
        # Each process attempts an actual malformed world independently. The
        # production loader nulls partial references before rebuilding its menu;
        # this does not promise restoration of a previously running world.
        for fault, source in sources.items():
            log = CONTEXT.logs / (prefix + "-" + fault + ".log")
            runner = ROOT / "mlm/tools/run/run_game_logged.sh"
            environment = dict(os.environ, OTTD_DEBUG="misc=5,sl=5")
            run_native(CONTEXT, ["C:/Program Files/Git/bin/bash.exe", runner.as_posix(), (ROOT / "build").as_posix(),
                            "Debug", log.as_posix(), "-v", "null:ticks=2", "-s", "null", "-m", "null",
                            "-x", "-X", "-I", "original_dos", "-c", "native-art.cfg", "-g", source.as_posix()],
                           cwd=ROOT / "build", env=environment, creationflags=subprocess.CREATE_NO_WINDOW,
                           timeout=110, check=True)
            text = log.read_text(encoding="utf-8", errors="replace")
            expected = "Invalid utility consumer wire fields" if fault == "active" else "Invalid utility power structural balances"
            assert expected in text, "actual corruption refusal missing: " + fault
            assert "Nulling pointers for UPLH" in text, "actual partial-load cleanup missing: " + fault
            assert "Nulling pointers for UPLD" in text, "actual table cleanup missing: " + fault
            print(fault + ": normal malformed load rejected with actual Null cleanup; log=" + log.name, flush=True)
    finally:
        if existed:
            startup.write_bytes(previous)
        else:
            startup.unlink()
        # Partial-load diagnostics cannot rewrite any source or user config.
        for path, expected in protected.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == expected, "protected partial-load input changed"


if __name__ == "__main__":
    main()
