"""Measure revised whole-unit spoil using retained native saves without rebuilding their routes.

@file whole_spoil.py
@see r2tbmf-tbm-fixes.md Group5B
@see ../../architecture/savegame/archi-savegame.md
"""

import hashlib
import os
from pathlib import Path
import subprocess
import sys
import time

sys.dont_write_bytecode = True
# Repository markers identify the source checkout after tools move; neither
# documentation depth nor the caller's working directory owns runtime output.
ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / "CMakeLists.txt").is_file() and (parent / "src").is_dir())
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native, start_native

CONTEXT = current_run_context("tbm", "work-advice")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()

sys.path.insert(0, str(ROOT / "src/tests"))
from tbm_logistics_save_fixture import owned_chunks, saved_table


def digest(path):
    """Identify retained file bytes before and after native execution.

    @param path Existing repository-owned input.
    @return SHA256 hexadecimal digest.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """Run the two focused native balance inputs and compare their immediate resaves.

    @return None; refuse failed execution, altered inputs or changed old cargo/receipts.
    """
    build = ROOT / "build"
    fixtures = ROOT / "src/tests/fixtures/haulage"
    script = RUNTIME / "scripts/game_start.scr"
    startup = script.read_bytes() if script.exists() else None
    protected = [build / "play.cfg", build / "native-art.cfg",
                 resolve_input_path(fixtures / "tbm-feedback-haul50-393.sav"), resolve_input_path(fixtures / "tbm-motion-wait-393.sav")]
    before_hashes = {path: digest(path) for path in protected}
    try:
        script.write_text("# @see r2tbmf-tbm-fixes.md Group5B\ntbm_feedback\n", encoding="utf-8")
        # Each ordinary startup reconstructs the original saved participants;
        # no fixture recreation or direct controller/vehicle edits occur.
        for mode, source in [("haul50", protected[2]), ("old-work", protected[3])]:
            env = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_TBM_FEEDBACK_")}
            env["OTTD_TBM_FEEDBACK_BALANCE"] = mode
            log = CONTEXT.logs / f"r2tbmf-m-whole-spoil-{mode}.log"
            start = time.monotonic()
            with log.open("wb") as output:
                result = run_native(CONTEXT, [str(build / "RelWithDebInfo/openttd_test.exe"), "[.tbm-feedback]"],
                                        cwd=build, env=env, stdout=output, stderr=subprocess.STDOUT, timeout=110)
            assert result.returncode == 0, log
            target = CONTEXT.saves / f"tbm-spoil-{mode}-loaded-394.sav"
            old_version, old = owned_chunks(source)
            new_version, new = owned_chunks(target)
            assert (old_version, new_version) == (393, 394)
            assert old.keys() == new.keys()
            # The earlier rate migration changes only the representation of
            # unfinished rational work. Whole cargo, paid history and terrain
            # must not be retuned retroactively by the yield table.
            for name in old:
                if name != b"TBMP":
                    assert old[name] == new[name], (mode, name)
            projects = saved_table(source, b"TBMP")
            for project in projects.values():
                project["work_remainder"] *= 2
            assert projects == saved_table(target, b"TBMP")
            lines = log.read_text(encoding="utf-8").splitlines()
            assert any(line.startswith("FEEDBACK_COMPLETE ticks=20000 ") for line in lines), log
            print(f"{mode}: native continuation and immediate saved owners verified in {time.monotonic() - start:.2f}s")
            for line in lines:
                if line.startswith(("BALANCE_", "FEEDBACK_PHASE ", "FEEDBACK_BLOCK ")):
                    print(line)
    finally:
        # The user's next launch retains exactly its previous startup script,
        # even if a native assertion or bounded process timeout interrupts us.
        if startup is None:
            script.unlink(missing_ok=True)
        else:
            script.write_bytes(startup)
        for path, expected in before_hashes.items():
            assert digest(path) == expected, path


if __name__ == "__main__":
    main()
