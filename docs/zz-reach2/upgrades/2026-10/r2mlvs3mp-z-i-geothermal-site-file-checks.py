"""Focused actual388 cold blueprint files on declared fixture-open floor.

@file r2mlvs3mp-z-i-geothermal-site-file-checks.py
@see r2mlvs3mp-master-plan.md checkpoint10.3
@see r2mlvs17-geothermal-energy-and-industry.md Group1; actual paid operating plant remains10.6.
"""

import hashlib
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks, encode_chunk

DIRECTORY = ROOT / "src/tests/fixtures/geothermal"
PREFIX = "r2mlvs3mp-z-i"


def run_case(mode, source, corrupt=None):
    """Run one accepted-world cold planning case in an isolated bounded native process.

    @param mode Creation, exact reader, cancellation or independent parser preview.
    @param source Retained actual native input, never overwritten.
    @param corrupt Optional narrowly edited test-owned parser candidate.
    @return Successful actual native evidence, retaining diagnostics on refusal/failure.
    """
    suffix = mode if corrupt is None else corrupt.stem
    log = ROOT / "logs" / f"{PREFIX}-geothermal-site-{suffix}.log"
    environment = dict(os.environ, OTTD_GEOTHERMAL_SITE_CASE=mode, OTTD_GEOTHERMAL_SITE_INPUT=str(source))
    if corrupt is not None:
        environment["OTTD_GEOTHERMAL_SITE_CORRUPT"] = str(corrupt)
    with log.open("wb") as sink:
        subprocess.run([str(ROOT / "build/Debug/openttd_test.exe"), "[.geothermal-site-file]"], cwd=ROOT / "build",
                       env=environment, stdout=sink, stderr=subprocess.STDOUT,
                       creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence
    assert f"GEOTHERMAL_SITE_FILE case={mode}" in evidence
    print(f"{suffix}: actual cold planning native boundary passed", flush=True)
    return evidence


def authority(path, sites=True):
    """Read actual cold/natural/electrical chunk bytes, without an invented private runtime model.

    @param path Retained input or actual ordinary writer output.
    @param sites Include new388 cold family when true; older input remains legitimately absent.
    @return Exact authoritative bytes; fixture-open packed terrain is deliberately outside the preserved natural/source quantities.
    """
    version, stream = read_stream(path)
    assert version in (387, 388)
    names = (b"GVNT", b"UPLH", b"UPLD", b"TUPH", b"TUPD", b"TUVH", b"TUVD")
    if sites:
        names += (b"GCSH", b"GCSD")
    result = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}
    assert len(result) == len(names)
    return result


def corrupt_sites(source):
    """Create six narrowly edited cold parser candidates, preserving every original input.

    @param source Actual writer-produced388 claimed site, with its original natural/electrical owners.
    @return Missing/count/source/endpoint/boolean/natural-family candidates using actual native transport.
    """
    version, stream = read_stream(source)
    assert version == 388
    outputs = []
    # Independent edits start from the same genuine writer output; one bad
    # candidate cannot contaminate another or manufacture construction/heat.
    for fault in ("missing-table", "bad-count", "bad-vent", "bad-endpoint", "bad-active", "missing-natural-family"):
        records = chunks(stream)
        header = next(row for row in records if row["name"] == b"GCSH")
        table = next(row for row in records if row["name"] == b"GCSD")
        if fault == "missing-table":
            records.remove(table)
        elif fault == "missing-natural-family":
            records.remove(next(row for row in records if row["name"] == b"GVNT"))
        elif fault == "bad-count":
            payload = bytearray(header["payload"])
            assert len(payload) == 8
            payload[4:8] = (257).to_bytes(4, "big")
            header.update(payload=bytes(payload), edited=True)
        else:
            row = bytearray(table["rows"][1])
            assert len(row) == 20
            if fault == "bad-vent":
                row[4:8] = (999).to_bytes(4, "big")
            elif fault == "bad-active":
                row[19] = 2
            else:
                row[14:16] = (int.from_bytes(row[10:12], "big") + 1).to_bytes(2, "big")
                row[16:18] = (int.from_bytes(row[12:14], "big") + 1).to_bytes(2, "big")
            table["rows"][1] = bytes(row)
            table["edited"] = True
        output = DIRECTORY / f"cold-site-corrupt-{fault}.sav"
        output.write_bytes(b"OTTN" + source.read_bytes()[4:8] + b"".join(encode_chunk(row) for row in records))
        outputs.append(output)
    return outputs


def main():
    """Check cold creation/exact reload/cancel and ordinary loading without any operating plant or energy claim.

    @return None; preserve the original387 input and startup script, and use distinct labelled388 fixture outputs.
    """
    DIRECTORY.mkdir(exist_ok=True)
    startup = ROOT / "build/scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    retained = ROOT / "src/tests/fixtures/tubes/recovery387-tube-meter-recover-source.sav"
    original = hashlib.sha256(retained.read_bytes()).hexdigest()
    try:
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md1 cold planning persistence only.\ngeothermal_site_check\n", encoding="utf-8")
        run_case("legacy-read", retained)
        assert authority(DIRECTORY / "fixture388-cold-site-legacy-read.sav", False) == authority(retained, False)
        run_case("fixture-create", retained)
        claimed = DIRECTORY / "fixture388-cold-site-fixture-create.sav"
        assert authority(claimed, False) == authority(retained, False)
        run_case("claimed-read", claimed)
        assert authority(DIRECTORY / "fixture388-cold-site-claimed-read.sav") == authority(claimed)
        run_case("fixture-cancel", claimed)
        cancelled = DIRECTORY / "fixture388-cold-site-fixture-cancel.sav"
        run_case("cancelled-read", cancelled)
        assert authority(DIRECTORY / "fixture388-cold-site-cancelled-read.sav") == authority(cancelled)
        # Each corrupted preview preserves live cold/natural/electrical roots
        # and permits a clean valid retry in the same accepted-world process.
        for corrupt in corrupt_sites(claimed):
            run_case("preview", claimed, corrupt)
        output = DIRECTORY / "fixture388-cold-site-normal-resaved.sav"
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md1 ordinary cold blueprint loading.\n"
                           "set savegame_format lzma\n"
                           f'save "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
        log = ROOT / "logs" / f"{PREFIX}-geothermal-site-normal-loader.log"
        runner = ROOT / "docs/zz-reach2/upgrades/2026-10/tools/run_game_logged.sh"
        subprocess.run(["C:/Program Files/Git/bin/bash.exe", runner.as_posix(), (ROOT / "build").as_posix(), "Debug", log.as_posix(),
                        "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos",
                        "-c", "native-art.cfg", "-g", claimed.as_posix()], cwd=ROOT / "build",
                       env=dict(os.environ, OTTD_DEBUG="misc=3,sl=3,grf=2"), creationflags=subprocess.CREATE_NO_WINDOW,
                       timeout=110, check=True)
        assert authority(output) == authority(claimed)
        print("Actual388 cold planning proof: exact claimed/cancelled identities and ordinary reload; six corrupt previews refuse; original387 source, natural capacity and all electrical chunks preserved; fixture floor supplies no paid completed plant", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        assert hashlib.sha256(retained.read_bytes()).hexdigest() == original, "retained387 input changed"


if __name__ == "__main__":
    main()
