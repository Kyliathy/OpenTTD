"""Focused retained version382 utility migration after directed transfer descriptors were introduced.

@file utility_power_save_fixture-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 8B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md older-save policy
"""

import hashlib
import re

from utility_power_save_fixture import ROOT, DIRECTORY, run_case


from functools import partial
from pathlib import Path
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native
from multilayer_state_fixture import log_path
CONTEXT = current_run_context("utilities", "utility-power-save-fixture-3")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()
INPUTS = ROOT / "src/tests/fixtures/utility"
DIRECTORY = CONTEXT.saves
log_path = partial(log_path, context=CONTEXT)
run_case = partial(run_case, context=CONTEXT)


def main():
    """Load five unchanged original pre-transfer files, including original consumed-ticket replay.

    @return None; preserve startup and all original382 bytes while validating actual current-format acceptance.
    """
    startup = RUNTIME / "scripts/game_start.scr"
    existed = startup.exists()
    previous = startup.read_bytes() if existed else None
    phases = ("published", "granted", "lost", "consumed", "released")
    inputs = {phase: INPUTS / f"utility-v382-{phase}.sav" for phase in phases}
    hashes = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs.values()}
    original_issuer = (ROOT / "src/tests/fixtures/utility/provenance/settlement-v382.log").read_text(encoding="utf-8", errors="replace")
    match = re.search(r"UTILITY_TICKET sequence=(\d+) seal=(\d+)", original_issuer)
    assert match is not None, "original issued consumption ticket missing"
    try:
        startup.write_text("# @see r2mlvs12-solar-farms-mirrors-and-eti.md8B older utility mutation proof.\nutility_file_check\n", encoding="utf-8")
        # Older current-use/lifetime/sequence records gain no implicit route or
        # imported generation while loading through the explicit382 descriptor.
        for phase, path in inputs.items():
            run_case(phase, path, f"legacy382-{phase}", match.groups(), prefix="r2mlvs3mp-v")
        print("Older382 proof: five actual publication/grant/loss/consumption/release loads; original ticket remains stale", flush=True)
    finally:
        if existed:
            startup.write_bytes(previous)
        else:
            startup.unlink()
        for path, expected in hashes.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == expected, "retained382 input changed"


if __name__ == "__main__":
    main()
