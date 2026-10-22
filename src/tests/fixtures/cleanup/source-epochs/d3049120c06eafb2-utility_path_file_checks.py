"""Actual392 installed utility-cell command/save/path proof from a retained391 collector world.

@file utility_path_file_checks.py
@see r2mlvs17-geothermal-energy-and-industry.md Group3B.
@see r2mlvs3mp-master-plan.md checkpoint10.3; live heat allocation/generators retain later groups.
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

CONTEXT = current_run_context("geothermal", "utility-path-file-checks")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()

sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks, encode_chunk

INPUTS = ROOT / "src/tests/fixtures/geothermal"
DIRECTORY = CONTEXT.saves
PREFIX = "r2mlvs3mp-z-o"


def run_case(mode, source, corrupt=None):
    """Run one bounded accepted-world paid geometry case and retain diagnostics.

    @param mode Native migration/build/split/repaired reader or independent preview.
    @param source Retained input or actual current writer output, never overwritten.
    @param corrupt Optional deliberately damaged copied current file.
    @return Successful captured native evidence, preserving failed diagnostics.
    """
    suffix = mode if corrupt is None else corrupt.stem
    log = CONTEXT.logs / f"{PREFIX}-utility-path-{suffix}.log"
    environment = dict(os.environ, OTTD_UTILITY_PATH_CASE=mode, OTTD_UTILITY_PATH_INPUT=str(source))
    if corrupt is not None:
        environment["OTTD_UTILITY_PATH_CORRUPT"] = str(corrupt)
    with log.open("wb") as sink:
        run_native(CONTEXT, [str(ROOT / "build/Debug/openttd_test.exe"), "[.geothermal-utility-file]"], cwd=ROOT / "build",
                       env=environment, stdout=sink, stderr=subprocess.STDOUT,
                       creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence and f"UTILITY_PATH_FILE case={mode}" in evidence
    print(f"{suffix}: actual paid native installed-path boundary passed", flush=True)
    return evidence


def authority(path, current=False):
    """Compare exact physical sources and shared meters without inventing a runtime graph or energy balance.

    @param path Retained391 predecessor or actual392 writer output.
    @param current Include the entire current shared ledger for exact geometry/history reloads.
    @return Exact native authority chunks and original nonlocal shared rows.
    """
    version, stream = read_stream(path)
    assert version in (391, 392)
    records = chunks(stream)
    names = (b"GVNT", b"GCSH", b"GCSD", b"GCBH", b"GCBD", b"INDY", b"TUPH", b"TUPD", b"TUVH", b"TUVD")
    if current:
        names += (b"UPLH", b"UPLD")
    result = {row["name"]: row["raw"] for row in records if row["name"] in names}
    assert len(result) == len(names)
    utility = next(row for row in records if row["name"] == b"UPLD")
    # Geometry installation can append shared local identities, but must not
    # change a single original source/consumer/slice/portal quantity or meaning.
    result["original-meter-rows"] = tuple(row for row in utility["rows"] if not (len(row) == 105 and row[0] == 4 and row[11] == 1))
    return result


def corrupt_paths(source):
    """Create narrow installed-cell domain/capacity/geometry/role/topology/version/metadata faults.

    @param source Actual392 five-cell paid writer output.
    @return Deliberately damaged independent-preview candidates; every original remains intact.
    """
    version, stream = read_stream(source)
    assert version == 392
    outputs = []
    # Each independent fault leaves compression and unrelated native sources
    # intact, so refusal exercises the installed-path authority boundary.
    for fault in ("capacity", "domain", "layer", "role", "topology", "revision", "metadata"):
        records = chunks(stream)
        table = next(row for row in records if row["name"] == b"UPLD")
        header = next(row for row in records if row["name"] == b"UPLH")
        original = next(row for row in table["rows"][1:] if row[0] == 4 and row[11] == 1 and row[-1] == 1)
        assert len(original) == 105 and len(header["payload"]) == 104
        if fault in ("topology", "revision"):
            raw = bytearray(header["payload"])
            offset, value = (96, 0) if fault == "topology" else (0, 6)
            raw[offset:offset + 8] = value.to_bytes(8, "big")
            header.update(payload=bytes(raw), edited=True)
        elif fault == "metadata":
            metadata = next(row for row in records if row["name"] == b"MLMD")
            raw = bytearray(metadata["rows"][1])
            # The schema's feature mask follows version/profile/policy,
            # package identity/hash (including its array length) and content
            # version at the stable offset31 in this schema's51-byte row.
            mask = int.from_bytes(raw[31:35], "big")
            assert mask & 65536
            raw[31:35] = (mask & ~65536).to_bytes(4, "big")
            metadata["rows"][1] = bytes(raw)
            metadata["edited"] = True
        else:
            raw = bytearray(original)
            if fault == "capacity":
                raw[21:29] = (1000001).to_bytes(8, "big")
            elif fault == "domain":
                raw[-1] = 255
            elif fault == "role":
                raw[11] = 0
            else:
                raw[91:95] = raw[95:99] = (1).to_bytes(4, "big")
            table["rows"][table["rows"].index(original)] = bytes(raw)
            table["edited"] = True
        output = DIRECTORY / f"utility392-corrupt-{fault}.sav"
        output.write_bytes(b"OTTN" + source.read_bytes()[4:8] + b"".join(encode_chunk(row) for row in records))
        outputs.append(output)
    return outputs


def main():
    """Verify actual paid construction, native source ports, split/repair, exact reload and ordinary game resave.

    @return None; preserve retained predecessor hashes and the original repository startup script.
    """
    retained = resolve_input_path(INPUTS / "fixture391-thermal-normal-resaved.sav")
    original = hashlib.sha256(retained.read_bytes()).hexdigest()
    startup = RUNTIME / "scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    try:
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md Group3B actual paid path boundary.\nutility_path_check\n", encoding="utf-8")
        run_case("legacy-read", retained)
        assert authority(DIRECTORY / "fixture392-utility-path-legacy-read.sav") == authority(retained)
        run_case("fixture-build", retained)
        built = DIRECTORY / "fixture392-utility-path-fixture-build.sav"
        assert authority(built) == authority(retained)
        run_case("built-read", built)
        assert authority(DIRECTORY / "fixture392-utility-path-built-read.sav", True) == authority(built, True)
        run_case("fixture-split", built)
        split = DIRECTORY / "fixture392-utility-path-fixture-split.sav"
        run_case("split-read", split)
        assert authority(DIRECTORY / "fixture392-utility-path-split-read.sav", True) == authority(split, True)
        run_case("fixture-repair", split)
        repaired = DIRECTORY / "fixture392-utility-path-fixture-repair.sav"
        run_case("repaired-read", repaired)
        assert authority(DIRECTORY / "fixture392-utility-path-repaired-read.sav", True) == authority(repaired, True)
        for corrupt in corrupt_paths(built):
            run_case("preview", built, corrupt)
        output = DIRECTORY / "fixture392-utility-path-normal-resaved.sav"
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md Group3B ordinary installed path load.\n"
                           "set savegame_format lzma\n"
                           f'save "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
        runner = ROOT / "mlm/tools/run/run_game_logged.sh"
        log = CONTEXT.logs / f"{PREFIX}-utility-path-normal-loader.log"
        run_native(CONTEXT, ["C:/Program Files/Git/bin/bash.exe", runner.as_posix(), (ROOT / "build").as_posix(), "Debug", log.as_posix(),
                        "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos",
                        "-c", "native-art.cfg", "-g", repaired.as_posix()], cwd=ROOT / "build",
                       env=dict(os.environ, OTTD_DEBUG="misc=3,sl=3,grf=2"), creationflags=subprocess.CREATE_NO_WINDOW,
                       timeout=110, check=True)
        assert authority(output, True) == authority(repaired, True)
        print("Actual392 utility paths: genuine391 migration, paid native commands/source port, split/repaired fresh identity, exact reload, seven malformed previews and ordinary resave. Original native meters/sources/stock/startup preserved; live heat allocation/operating generator remain later groups.", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        assert hashlib.sha256(retained.read_bytes()).hexdigest() == original, "retained391 predecessor changed"


if __name__ == "__main__":
    main()
