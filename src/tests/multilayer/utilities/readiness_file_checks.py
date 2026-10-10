"""Focused public readiness against actual retained native386 and recovered387 worlds.

@file readiness_file_checks.py
@see r2mlvs3mp-master-plan.md checkpoint10.2
@see r2mlvs17-underground-transport-tubes.md Group8
"""

import hashlib
import importlib.util
from pathlib import Path
import sys

sys.dont_write_bytecode = True
# Repository markers identify the source checkout after tools move; neither
# documentation depth nor the caller's working directory owns runtime output.
ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / "CMakeLists.txt").is_file() and (parent / "src").is_dir())
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native, start_native

CONTEXT = current_run_context("transport-tubes", "readiness-file-checks")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()



def main():
    """Inspect actual facility/source queries and exact saved owners in two bounded native processes.

    @return None; preserve both input files and the original startup script even after a failed check.
    """
    helper = Path(__file__).with_name("recovery_file_checks.py")
    spec = importlib.util.spec_from_file_location("tube_readiness_native_helper", helper)
    native = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(native)
    native.CONTEXT = CONTEXT
    native.DIRECTORY = CONTEXT.saves
    native.PREFIX = "r2mlvs3mp-z-g"
    directory = ROOT / "src/tests/fixtures/tubes"
    inputs = ((directory / "tube-power-consumed.sav", False, "readiness386-"),
              (directory / "recovery387-tube-meter-recover-source.sav", True, "readiness387-"))
    retained = {source: hashlib.sha256(source.read_bytes()).hexdigest() for source, _, _ in inputs}
    startup = RUNTIME / "scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    try:
        startup.write_text("# @see r2mlvs17-underground-transport-tubes.md8 pure accepted-world readiness.\ntube_power_check\n", encoding="utf-8")
        # Each input keeps its actual energy/invoice/binding owner, with a
        # separate output prefix rather than overwriting either prior evidence.
        for source, bindings, prefix in inputs:
            native.run_case("readiness", source, output_prefix=prefix)
            assert native.authority(CONTEXT.saves / f"{prefix}tube-readiness-loaded.sav", bindings) == native.authority(source, bindings)
        print("Actual public readiness: retained386 and recovered387 identities resolve; queries preserve all four/six authority chunks and all original input bytes", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        assert all(hashlib.sha256(source.read_bytes()).hexdigest() == digest for source, digest in retained.items())


if __name__ == "__main__":
    main()
