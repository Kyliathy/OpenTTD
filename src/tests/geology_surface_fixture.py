#!/usr/bin/env python3
"""Current writer Surface comparison against the retained pre-job feature-disabled baseline.

@file geology_surface_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 18
"""

import hashlib
import json

from geology_generation_fixture import read_stream, saved_geology_seed, surface_digests
from multilayer_state_fixture import AREA, ROOT, run_case, save_path


# Each producer declares its feature; saved-input names never select output ownership.
from functools import partial
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native
from multilayer_state_fixture import output_save_path
CONTEXT = current_run_context("geology", "geology-surface-fixture")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime("grftest.cfg")
output_save_path = partial(output_save_path, context=CONTEXT)
save_path = partial(save_path, context=CONTEXT)
run_case = partial(run_case, context=CONTEXT)


def main():
    """Compare actual Classic/native Surface fields and load retained predecessor job state.

    @return None; preserve baseline, startup and configuration bytes exactly.
    """
    startup, config = RUNTIME / "scripts/game_start.scr", RUNTIME / "grftest.cfg"
    baseline = save_path("r2mlvs2-e-classic.sav")
    original_script, original_config = startup.read_bytes(), config.read_bytes()
    baseline_hash = hashlib.sha256(baseline.read_bytes()).hexdigest()
    prior = surface_digests(read_stream(baseline)[1])
    measured = {}
    try:
        # Surface settings/seed remain identical to the supplied earlier baseline; only the explicit profile changes.
        for name, profile in (("classic-current", "classic"), ("native-current", "multilayer_industry")):
            config.write_text("# @see r2mlvs2-geology-and-excavation.md, Surface preservation fixture.\n"
                              "[game_creation]\nmap_x = 6\nmap_y = 6\nlandscape = temperate\n"
                              f"gameplay_profile = {profile}\n"
                              "[network]\nserver_advertise = false\nserver_bind_addresses = 127.0.0.1\n", encoding="utf-8")
            expected = "MLVS_PROFILE id=0 version=0" if profile == "classic" else "MLVS_GEOLOGY generator=1"
            saved = run_case(name, ["-G", "1234"], expected, prefix="r2mlvs2-z")
            _, stream = read_stream(saved)
            measured[name] = surface_digests(stream)
            assert measured[name] == prior, (name, "Surface differs from retained baseline")
            assert (saved_geology_seed(stream) == 0) == (profile == "classic")
        old = save_path("r2mlvs2-o-active.sav")
        old_hash = hashlib.sha256(old.read_bytes()).hexdigest()
        run_case("old-job-current", ["-g", old.as_posix()],
                 "MLVS_EXCAVATION_JOB id=0 state=3 cursor=0 partial=77 revision=2", prefix="r2mlvs2-z")
        assert hashlib.sha256(old.read_bytes()).hexdigest() == old_hash
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
        assert hashlib.sha256(baseline.read_bytes()).hexdigest() == baseline_hash
    (CONTEXT.reports / "r2mlvs2-z-surface-results.txt").write_text(
        "@see r2mlvs2-geology-and-excavation.md, Group 18.\n"
        "Current Classic/native generation matches all ten retained feature-disabled Surface field SHA256 values.\n"
        "Current writer also accepts the retained version-373 paused job without fabricating validated cut input.\n"
        "Baseline/predecessor/startup/config bytes preserved exactly.\n" + json.dumps(measured, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
