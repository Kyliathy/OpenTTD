"""Focused actual native city effects, custom GS authority and expiry file proof.

@file utility_power_save_fixture-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 9
@see docs/zz-reach2/architecture/towns/archi-towns.md
"""

import hashlib
import os
import re
import subprocess

from utility_power_save_fixture import ROOT, DIRECTORY, utility_chunks
from geology_save_fixture import chunks
from geology_generation_fixture import read_stream
from multilayer_state_fixture import log_path


from functools import partial
from pathlib import Path
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native
from multilayer_state_fixture import log_path
CONTEXT = current_run_context("utilities", "utility-power-save-fixture-5")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()
INPUTS = ROOT / "src/tests/fixtures/utility"
DIRECTORY = CONTEXT.saves
log_path = partial(log_path, context=CONTEXT)


def effect_chunks(path):
    """Read exact energy and actual town transport, keeping their authorities separate.

    @param path Native current-format city effect file.
    @return Exact utility header/table plus ordinary CITY bytes.
    """
    result = utility_chunks(path)
    result[b"CITY"] = next(row["raw"] for row in chunks(read_stream(path)[1]) if row["name"] == b"CITY")
    return result


def run_case(mode, source, baseline=None):
    """Run one actual accepted-world city observer, retaining its bounded source/effect evidence.

    @param mode Create four real city phases or verify one actual reloaded state.
    @param source Original paid world or generated phase; never modified while loading.
    @param baseline Exact normal rate printed by the original actual town owner, required on reload.
    @return Native log text, including original baseline output.
    """
    log = log_path(f"r2mlvs3mp-w-power-town-{mode}.log")
    environment = dict(os.environ, OTTD_UTILITY_FILE_CASE=mode, OTTD_UTILITY_FILE_INPUT=str(source))
    if baseline is not None:
        environment["OTTD_UTILITY_TOWN_BASELINE"] = baseline
    with log.open("wb") as sink:
        run_native(CONTEXT, [str(ROOT / "build/Debug/openttd_test.exe"), "[.utility-power-effect-file]"],
                       cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT,
                       creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence
    assert f"UTILITY_TOWN_FILE case={mode} actual_town=true metered_use=true" in evidence
    print(f"{mode}: actual native city effect boundary passed", flush=True)
    return evidence


def main():
    """Run five actual city/authority/expiry cases with byte-exact ledger and Town persistence.

    @return None; preserve startup and original paid-world input and verify every generated phase on reload.
    """
    source = ROOT / "src/tests/fixtures/haulage/spoil-loop-underground/m1-loop-underground-5-complete-loop.sav"
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    startup = RUNTIME / "scripts/game_start.scr"
    existed = startup.exists()
    previous = startup.read_bytes() if existed else None
    DIRECTORY.mkdir(parents=True, exist_ok=True)
    try:
        startup.write_text("# @see r2mlvs12-solar-farms-mirrors-and-eti.md9 actual city effect proof.\nutility_effect_check\n", encoding="utf-8")
        evidence = run_case("create", source)
        baseline = re.search(r"UTILITY_TOWN_BASELINE rate=(\d+)", evidence)
        assert baseline is not None
        # Actual saved native rate, GS rate, relinquished suspension and expired
        # source lifetime use all reload without another effect or energy debit.
        for phase in ("applied", "custom", "relinquished", "expired"):
            path = DIRECTORY / f"utility-town-{phase}.sav"
            expected = effect_chunks(path)
            run_case(phase, path, baseline.group(1))
            assert effect_chunks(DIRECTORY / f"utility-town-{phase}-reloaded.sav") == expected
        print("City effect proof: real metered bonus/custom/suppression/expiry, exact utility/CITY chunks and no replay benefit", flush=True)
    finally:
        if existed:
            startup.write_bytes(previous)
        else:
            startup.unlink()
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash, "retained paid city input changed"


if __name__ == "__main__":
    main()
