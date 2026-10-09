"""Prepare a paused TBM demo through the ordinary optimized game loader.

@file demo_checkpoint.py
@see r2mlvs3mp-master-plan.md checkpoint10.3 and remaining normal-newgame acceptance11.4.
@see r2mlvs-how-to.md delivered portal, excavation and spoil-haulage controls.
The retained acceptance world supplies its starting logistics. This preparation
does not claim a normal-newgame bootstrap or a working geothermal generator.
"""

import hashlib
import os
from pathlib import Path
import subprocess
import sys

# Repository markers identify the source checkout after tools move; neither
# documentation depth nor the caller's working directory owns runtime output.
ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / "CMakeLists.txt").is_file() and (parent / "src").is_dir())
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native, start_native

CONTEXT = current_run_context("tbm", "demo-checkpoint")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()

sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks

PREFIX = "r2mlvs3mp-z-l"
AUTHORITY = (b"GEOS", b"GVNT", b"TBMH", b"TBMP", b"TBMS", b"TBXH", b"TBXA", b"TBXS", b"TBXW")


def physical_authority(path):
    """Read saved natural, excavation/project/support and actual spoil owners.

    @param path Retained385 input or ordinary390 loader/resave output.
    @return Version and byte-exact unchanged physical authority chunks.
    """
    version, stream = read_stream(path)
    rows = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in AUTHORITY}
    assert len(rows) == len(AUTHORITY), "missing retained physical authority"
    return version, rows


def load_and_save(source, destination, suffix):
    """Pause and resave through the normal optimized game, without fixture callbacks.

    @param source Existing native save; never written by this function.
    @param destination New test/demo-owned save inside the repository build/save directory.
    @param suffix Evidence basename distinguishing migration from current-format reload.
    @return None; the game must exit successfully and create its ordinary save.
    """
    # A checkpoint must preserve any previously prepared player's demo; naming
    # a fresh output is required instead of silently overwriting their world.
    assert not destination.exists(), f"destination already exists: {destination}"
    startup = RUNTIME / "scripts/game_start.scr"
    startup.write_text(
        "# @see r2mlvs3mp-z-l-demo-checkpoint.md ordinary paused TBM checkpoint.\n"
        "pause\nset savegame_format lzma\n"
        f'save "{destination.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    runner = ROOT / "mlm/tools/run/run_game_logged.sh"
    log = CONTEXT.logs / f"{PREFIX}-demo-{suffix}.log"
    run_native(CONTEXT,
        ["C:/Program Files/Git/bin/bash.exe", runner.as_posix(), (ROOT / "build").as_posix(),
         "RelWithDebInfo", log.as_posix(), "-v", "null:ticks=2", "-s", "null", "-m", "null",
         "-x", "-X", "-I", "original_dos", "-c", "native-art.cfg", "-g", source.as_posix()],
        cwd=ROOT / "build", env=dict(os.environ, OTTD_DEBUG="misc=3,sl=3,grf=2"),
        creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
    assert destination.exists(), "ordinary game did not produce the demo save"
    print(f"{suffix}: ordinary optimized game paused, loaded and saved", flush=True)


def main():
    """Prepare and independently reload the demo, preserving its original input and startup script.

    @return None; physical authority must remain byte-exact across385 migration and390 reload.
    """
    retained = ROOT / "src/tests/fixtures/haulage/ground-acceptance/acceptance-ground-bend-385-b-await-exchange.sav"
    original = hashlib.sha256(retained.read_bytes()).hexdigest()
    old_version, before = physical_authority(retained)
    assert old_version == 385
    directory = CONTEXT.saves
    directory.mkdir(parents=True, exist_ok=True)
    demo = directory / "underground-demo-390.sav"
    reloaded = directory / "underground-demo-390-reloaded.sav"
    startup = RUNTIME / "scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    try:
        load_and_save(retained, demo, "migration")
        version, migrated = physical_authority(demo)
        assert version == 390
        assert migrated == before, "migration changed natural/project/support/spoil authority"
        load_and_save(demo, reloaded, "reload")
        version, current = physical_authority(reloaded)
        assert version == 390
        assert current == migrated, "current reload changed physical authority"
        print(f"Demo: {demo.relative_to(ROOT)}", flush=True)
        print(f"Demo SHA256: {hashlib.sha256(demo.read_bytes()).hexdigest()}", flush=True)
        print("Native starting logistics remain explicitly supplied; normal-newgame acceptance is pending.", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        assert hashlib.sha256(retained.read_bytes()).hexdigest() == original, "retained input changed"


if __name__ == "__main__":
    main()
