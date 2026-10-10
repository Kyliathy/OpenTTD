"""Assisting focused actual387 capsule binding/service/wait files and retained386 acceptance.

@file recovery_file_checks.py
@see r2mlvs3mp-master-plan.md checkpoint10.2
@see r2mlvs17-underground-transport-tubes.md Groups6-7
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

CONTEXT = current_run_context("transport-tubes", "recovery-file-checks")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()

sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks, encode_chunk

INPUTS = ROOT / "src/tests/fixtures/tubes"
DIRECTORY = CONTEXT.saves
PREFIX = "r2mlvs3mp-z-f"


def run_case(mode, source, corrupt=None, output_prefix="recovery387-"):
    """Run one isolated native accepted-world case with a new output prefix.

    @param mode Actual saved service/binding/wait or independent preview operation.
    @param source Retained native input, never rewritten by the observer.
    @param corrupt Optional narrowly edited test-owned parser candidate.
    @param output_prefix Distinct native output basename prefix; callers preserve all earlier source/evidence files.
    @return Successful native evidence, retaining diagnostics on failure.
    """
    suffix = mode if corrupt is None else corrupt.stem
    if mode == "readiness":
        suffix = f"{mode}-{source.stem}"
    log = CONTEXT.logs / f"{PREFIX}-tube-meter-{suffix}.log"
    environment = dict(os.environ, OTTD_TUBE_POWER_CASE=mode, OTTD_TUBE_POWER_INPUT=str(source),
                       OTTD_TUBE_POWER_OUTPUT_PREFIX=output_prefix)
    if corrupt is not None:
        environment["OTTD_TUBE_POWER_CORRUPT"] = str(corrupt)
    with log.open("wb") as sink:
        run_native(CONTEXT, [str(ROOT / "build/Debug/openttd_test.exe"), "[.transport-tube-power-file]"],
                       cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT,
                       creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence
    assert f"TUBE_POWER_FILE case={mode}" in evidence
    print(f"{suffix}: actual native meter boundary passed", flush=True)
    return evidence


def authority(path, bindings=True):
    """Read exact actual physical/electrical families without a replacement private runtime model.

    @param path Ordinary native writer output or retained386 input.
    @param bindings Include actual387 capsule family when true.
    @return Exact authoritative chunk bytes; genuine older absence remains explicit.
    """
    version, stream = read_stream(path)
    assert version in (386, 387)
    names = (b"TUPH", b"TUPD", b"UPLH", b"UPLD", b"TUVH", b"TUVD") if bindings else (b"TUPH", b"TUPD", b"UPLH", b"UPLD")
    result = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}
    assert len(result) == len(names)
    return result


def corrupt_bindings(source):
    """Produce narrowly edited parser candidates from a genuine387 physical binding file.

    @param source Actual service-admitted native file; its original bytes remain unchanged.
    @return Test-owned missing/count/fraction/reference/future-tick candidates from actual native transport.
    """
    version, stream = read_stream(source)
    assert version == 387
    outputs = []
    # Every candidate starts from the ordinary writer independently; other
    # physical families, packets, source usage and original inputs stay exact.
    for fault in ("missing-table", "bad-count", "bad-fraction", "bad-consumer", "future-tick"):
        records = chunks(stream)
        header = next(row for row in records if row["name"] == b"TUVH")
        table = next(row for row in records if row["name"] == b"TUVD")
        if fault == "missing-table":
            records.remove(table)
        elif fault == "bad-count":
            payload = bytearray(header["payload"])
            assert len(payload) == 16
            payload[4:8] = (4097).to_bytes(4, "big")
            header.update(payload=bytes(payload), edited=True)
        else:
            row = bytearray(table["rows"][1])
            assert len(row) == 33
            if fault == "bad-fraction":
                row[12:14] = (1000).to_bytes(2, "big")
            elif fault == "bad-consumer":
                row[0:4] = (999).to_bytes(4, "big")
            else:
                row[14:22] = (0xFFFFFFFFFFFFFFFE).to_bytes(8, "big")
            table["rows"][1] = bytes(row)
            table["edited"] = True
        output = DIRECTORY / f"tube-recovery-corrupt-{fault}.sav"
        output.write_bytes(b"OTTN" + source.read_bytes()[4:8] + b"".join(encode_chunk(row) for row in records))
        outputs.append(output)
    return outputs


def main():
    """Check retained386 prefix and actual387 admitted service/continuation/wait/preview in bounded independent processes.

    @return None; preserve startup and every retained source, using distinct new387 output names.
    """
    startup = RUNTIME / "scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    retained = resolve_input_path(INPUTS / "tube-power-consumed.sav")
    original = hashlib.sha256(retained.read_bytes()).hexdigest()
    assert read_stream(retained)[0] == 386
    preserved = {f: hashlib.sha256(f.read_bytes()).hexdigest() for f in DIRECTORY.glob("meter387-*")}
    try:
        startup.write_text("# @see r2mlvs17-underground-transport-tubes.md6 actual native meter.\ntube_power_check\n", encoding="utf-8")
        run_case("consumed", retained)
        assert authority(DIRECTORY / "recovery387-tube-power-consumed-reloaded.sav", False) == authority(retained, False)
        run_case("bind-service", retained)
        admitted = DIRECTORY / "recovery387-tube-meter-bind-service.sav"
        expected = authority(admitted)
        run_case("bound-service", admitted)
        assert authority(DIRECTORY / "recovery387-tube-meter-bound-service.sav") == expected
        run_case("resume-service", admitted)
        run_case("bound-wait", admitted)
        waiting = DIRECTORY / "recovery387-tube-meter-bound-wait.sav"
        expected_wait = authority(waiting)
        run_case("wait-read", waiting)
        assert authority(DIRECTORY / "recovery387-tube-meter-wait-read.sav") == expected_wait
        run_case("recover-source", waiting)
        recovered = DIRECTORY / "recovery387-tube-meter-recover-source.sav"
        run_case("recovery-read", recovered)
        assert authority(DIRECTORY / "recovery387-tube-meter-recovery-read.sav") == authority(recovered)
        run_case("wait-read", resolve_input_path(INPUTS / "meter387-tube-meter-bound-wait.sav"))
        # Each bad preview must preserve both physical/electrical live owners
        # and permit a clean valid387 retry in the same ordinary process.
        for corrupt in corrupt_bindings(admitted):
            run_case("preview", admitted, corrupt)
        output = DIRECTORY / "recovery387-tube-meter-normal-resaved.sav"
        startup.write_text("# @see r2mlvs17-underground-transport-tubes.md6 ordinary metered capsule loading.\n"
                           "set savegame_format lzma\n"
                           f'save "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
        log = CONTEXT.logs / f"{PREFIX}-tube-meter-normal-loader.log"
        runner = ROOT / "mlm/tools/run/run_game_logged.sh"
        run_native(CONTEXT, ["C:/Program Files/Git/bin/bash.exe", runner.as_posix(), (ROOT / "build").as_posix(), "Debug", log.as_posix(),
                        "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos",
                        "-c", "native-art.cfg", "-g", admitted.as_posix()], cwd=ROOT / "build",
                       env=dict(os.environ, OTTD_DEBUG="misc=3,sl=3,grf=2"), creationflags=subprocess.CREATE_NO_WINDOW,
                       timeout=110, check=True)
        assert authority(output) == authority(admitted)
        print("Actual387 meter proof: unchanged386 prefix; saved whole service; exact binding/carry/ledger reload; next paid tick; saved grant loss; actual paid replacement and exact recovery reload; retained387 wait; five independent corrupt previews; ordinary loader/writer", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        assert hashlib.sha256(retained.read_bytes()).hexdigest() == original, "retained386 input changed"
        # New recovery outputs have a distinct prefix; earlier native387 proof
        # remains byte-identical alongside the unchanged386 input.
        assert all(hashlib.sha256(f.read_bytes()).hexdigest() == digest for f, digest in preserved.items())


if __name__ == "__main__":
    main()
