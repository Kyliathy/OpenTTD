"""Assisting bounded native paid-service and earlier utility file checks.

@file power_file_checks.py
@see r2mlvs3mp-master-plan.md checkpoint10.2
@see r2mlvs16-underground-transport-tubes.md Group5
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

CONTEXT = current_run_context("transport-tubes", "power-file-checks")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()

sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks, encode_chunk

INPUTS = ROOT / "src/tests/fixtures/tubes"
DIRECTORY = CONTEXT.saves
PREFIX = "r2mlvs3mp-z-d"


def run_case(tag, command, environment):
    """Run one native process with bounded inactivity and retained repository-local evidence.

    @param tag Unique owning group's log basename suffix.
    @param command Complete native executable/fixture selection.
    @param environment Explicit isolated feature observer inputs.
    @return Exact successful native output, retaining diagnostic output on failure.
    """
    log = CONTEXT.logs / f"{PREFIX}-{tag}.log"
    with log.open("wb") as sink:
        run_native(CONTEXT, command, cwd=ROOT / "build", env=environment, stdout=sink,
                       stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW,
                       timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence, tag
    print(f"{tag}: native boundary passed", flush=True)
    return evidence


def service_chunks(path):
    """Read exact authoritative physical and electrical transport for a current real save.

    @param path Actual ordinary native writer output.
    @return Exact source/connection/invoice bytes, excluding unrelated world state.
    """
    version, stream = read_stream(path)
    assert version == 386
    selected = {row["name"]: row["raw"] for row in chunks(stream)
                if row["name"] in (b"TUPH", b"TUPD", b"UPLH", b"UPLD")}
    assert len(selected) == 4
    return selected


def corrupt_services(source):
    """Create narrowly edited test-owned malformed family candidates from a genuine save.

    @param source Actual consumed-phase native file; its original bytes stay unchanged.
    @return Four corrupt test-owned paths for independent preview/refusal checks.
    """
    version, stream = read_stream(source)
    assert version == 386
    outputs = []
    # Each corruption starts from the genuine writer independently; no edit
    # changes terrain, other feature histories or the original paid source.
    for fault in ("missing-table", "bad-count", "bad-fraction", "lost-fraction"):
        records = chunks(stream)
        header = next(row for row in records if row["name"] == b"TUPH")
        table = next(row for row in records if row["name"] == b"TUPD")
        if fault == "missing-table":
            records.remove(table)
        elif fault == "bad-count":
            payload = bytearray(header["payload"])
            assert len(payload) == 12
            payload[4:8] = (257).to_bytes(4, "big")
            header.update(payload=bytes(payload), edited=True)
        else:
            row = bytearray(table["rows"][1])
            assert len(row) == 45 and row[0] == 0
            row[26:28] = (1000 if fault == "bad-fraction" else 0).to_bytes(2, "big")
            table["rows"][1] = bytes(row)
            table["edited"] = True
        output = DIRECTORY / f"tube-power-corrupt-{fault}.sav"
        output.write_bytes(b"OTTN" + source.read_bytes()[4:8] + b"".join(encode_chunk(row) for row in records))
        outputs.append(output)
    return outputs


def main():
    """Check retained385 utility prerequisites, then current386 paid service creation/reloads/refusals.

    @return None; restore startup, preserve all supplied inputs, retain new feature saves/logs.
    """
    DIRECTORY.mkdir(parents=True, exist_ok=True)
    startup = RUNTIME / "scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    source = ROOT / "src/tests/fixtures/haulage/spoil-loop-underground/m1-loop-underground-5-complete-loop.sav"
    utility = ROOT / "src/tests/fixtures/utility"
    inputs = [source, utility / "utility-granted.sav", utility / "utility-town-applied.sav"]
    hashes = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs}
    executable = str(ROOT / "build/Debug/openttd_test.exe")
    try:
        startup.write_text("# @see r2mlvs16-underground-transport-tubes.md5 prerequisites.\nutility_file_check\n", encoding="utf-8")
        environment = dict(os.environ, OTTD_UTILITY_FILE_CASE="granted", OTTD_UTILITY_FILE_INPUT=str(inputs[1]))
        run_case("utility-retained-grant", [executable, "[.utility-power-file]"], environment)
        startup.write_text("# @see r2mlvs16-underground-transport-tubes.md5 prerequisites.\nutility_effect_check\n", encoding="utf-8")
        environment.update(OTTD_UTILITY_FILE_CASE="applied", OTTD_UTILITY_FILE_INPUT=str(inputs[2]), OTTD_UTILITY_TOWN_BASELINE="11269")
        run_case("utility-retained-effect", [executable, "[.utility-power-effect-file]"], environment)
        startup.write_text("# @see r2mlvs16-underground-transport-tubes.md5 paid native service.\ntube_power_check\n", encoding="utf-8")
        environment = dict(os.environ, OTTD_TUBE_POWER_CASE="create", OTTD_TUBE_POWER_INPUT=str(source))
        run_case("tube-power-file-create", [executable, "[.transport-tube-power-file]"], environment)
        # Reload each genuine phase independently. Exact physical invoice and
        # electrical chunks must survive without a refreshed lease or grant.
        for phase in ("granted", "consumed", "closed"):
            path = DIRECTORY / f"tube-power-{phase}.sav"
            expected = service_chunks(path)
            environment.update(OTTD_TUBE_POWER_CASE=phase, OTTD_TUBE_POWER_INPUT=str(path))
            run_case(f"tube-power-file-{phase}", [executable, "[.transport-tube-power-file]"], environment)
            assert service_chunks(DIRECTORY / f"tube-power-{phase}-reloaded.sav") == expected
        consumed = DIRECTORY / "tube-power-consumed.sav"
        startup.write_text("# @see r2mlvs16-underground-transport-tubes.md5 normal loaded invoice.\n"
                           "set savegame_format lzma\n"
                           f'save "{(DIRECTORY / "tube-power-normal-resaved").as_posix()}"\nquit\n', encoding="utf-8")
        log = CONTEXT.logs / f"{PREFIX}-tube-power-normal-loader.log"
        runner = ROOT / "mlm/tools/run/run_game_logged.sh"
        run_native(CONTEXT, ["C:/Program Files/Git/bin/bash.exe", runner.as_posix(), (ROOT / "build").as_posix(),
                        "Debug", log.as_posix(), "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X",
                        "-I", "original_dos", "-c", "native-art.cfg", "-g", consumed.as_posix()],
                       cwd=ROOT / "build", env=dict(os.environ, OTTD_DEBUG="misc=3,sl=3,grf=2"),
                       creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
        assert service_chunks(DIRECTORY / "tube-power-normal-resaved.sav") == service_chunks(consumed)
        print("tube-power-normal: ordinary game loader/writer preserves exact connection/invoice/ledger", flush=True)
        startup.write_text("# @see r2mlvs16-underground-transport-tubes.md5 paid native service.\ntube_power_check\n", encoding="utf-8")
        # Independent failed previews cannot pollute live physical/electrical
        # authority, and a valid retry must accept the original current file.
        for corrupt in corrupt_services(consumed):
            environment.update(OTTD_TUBE_POWER_CASE="preview", OTTD_TUBE_POWER_INPUT=str(consumed), OTTD_TUBE_POWER_CORRUPT=str(corrupt))
            run_case(f"tube-power-{corrupt.stem}", [executable, "[.transport-tube-power-file]"], environment)
        print("Paid tube native proof: retained385 grant/effect; current386 grant/use/closure; exact reloads; four independent corrupt previews", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        for path, digest in hashes.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, "retained paid/utility source changed"


if __name__ == "__main__":
    main()
