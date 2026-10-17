"""Focused saved consumption/release retries and retained pre-settlement utility migration.

@file utility_power_save_fixture-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 8
@see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-v-utility-settlement-contract.md
"""

import hashlib
import re

from utility_power_save_fixture import ROOT, DIRECTORY, run_case, utility_chunks


from functools import partial
from pathlib import Path
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native
from multilayer_state_fixture import log_path
CONTEXT = current_run_context("utilities", "utility-power-save-fixture-1")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()
INPUTS = ROOT / "src/tests/fixtures/utility"
DIRECTORY = CONTEXT.saves
log_path = partial(log_path, context=CONTEXT)
run_case = partial(run_case, context=CONTEXT)


def main():
    """Run six isolated native migration/settlement cases separately from the ten publication cases.

    @return None; preserve startup and every retained input, comparing exact committed transport on reload.
    """
    startup = RUNTIME / "scripts/game_start.scr"
    existed = startup.exists()
    previous = startup.read_bytes() if existed else None
    inputs = [INPUTS / f"utility-v381-{phase}.sav" for phase in ("published", "granted", "lost")]
    inputs.append(INPUTS / "utility-granted.sav")
    hashes = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs}
    try:
        startup.write_text("# @see r2mlvs12-solar-farms-mirrors-and-eti.md8 saved settlement proof.\nutility_file_check\n", encoding="utf-8")
        # Actual pre-settlement files load through the explicit older descriptor
        # path, preserving available energy and absence of historical use.
        for phase in ("published", "granted", "lost"):
            run_case(phase, INPUTS / f"utility-v381-{phase}.sav", f"legacy-{phase}", prefix="r2mlvs3mp-v")
        evidence = run_case("settle", INPUTS / "utility-granted.sav", "settle", prefix="r2mlvs3mp-v")
        match = re.search(r"UTILITY_TICKET sequence=(\d+) seal=(\d+)", evidence)
        assert match is not None, "actual issued ticket missing"
        # Replay the exact original pre-commit ticket after actual game loading;
        # persisted sequence must refuse it without another expense or energy use.
        for phase in ("consumed", "released"):
            source = DIRECTORY / f"utility-{phase}.sav"
            expected = utility_chunks(source)
            run_case(phase, source, f"{phase}-load", match.groups(), prefix="r2mlvs3mp-v")
            assert utility_chunks(DIRECTORY / f"utility-{phase}-reloaded.sav") == expected
        print("Saved settlement proof: three older native phases, exact consumed/released reloads and original issued-ticket refusal", flush=True)
    finally:
        if existed:
            startup.write_bytes(previous)
        else:
            startup.unlink()
        # Only generated outputs change. All retained native and version381
        # inputs remain available for repeatable older-save and retry evidence.
        for path, expected in hashes.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == expected, "retained utility input changed"


if __name__ == "__main__":
    main()
