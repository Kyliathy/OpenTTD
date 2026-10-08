"""Focused390 shared thermal/electrical owner files and genuine389 migration.

@file r2mlvs3mp-z-k-heat-file-checks.py
@see r2mlvs17-geothermal-energy-and-industry.md Group2B.
@see r2mlvs3mp-master-plan.md checkpoint10.3; operating transported generator remains10.6.
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

DIRECTORY = ROOT / "src/tests/fixtures/utility"
PREFIX = "r2mlvs3mp-z-k"


def run_case(mode, source, corrupt=None):
    """Run one bounded accepted-world actual native domain/accounting case.

    @param mode Genuine legacy, exact reader, domain registration or actual expense settlement.
    @param source Retained/native writer input, never overwritten.
    @param corrupt Optional narrowly edited current-format parser candidate.
    @return Successful captured native evidence, preserving diagnostics on failure.
    """
    suffix = mode if corrupt is None else corrupt.stem
    log = ROOT / "logs" / f"{PREFIX}-heat-{suffix}.log"
    environment = dict(os.environ, OTTD_UTILITY_HEAT_CASE=mode, OTTD_UTILITY_HEAT_INPUT=str(source))
    if corrupt is not None:
        environment["OTTD_UTILITY_HEAT_CORRUPT"] = str(corrupt)
    with log.open("wb") as sink:
        subprocess.run([str(ROOT / "build/Debug/openttd_test.exe"), "[.utility-heat-file]"], cwd=ROOT / "build",
                       env=environment, stdout=sink, stderr=subprocess.STDOUT,
                       creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence and f"UTILITY_HEAT_FILE case={mode}" in evidence
    print(f"{suffix}: actual native shared-domain boundary passed", flush=True)
    return evidence


def authority(path, utility=False):
    """Read exact native physical/thermal/electrical authority chunks without a private runtime model.

    @param path Genuine389 retained input or actual390 writer output.
    @param utility Include newly versioned shared utility bytes for exact current-format reloads only.
    @return Exact natural/collector/tube/native-stock transport plus optional current utility transport.
    """
    version, stream = read_stream(path)
    assert version in (389, 390)
    names = (b"GVNT", b"TUPH", b"TUPD", b"TUVH", b"TUVD", b"GCSH", b"GCSD", b"GCBH", b"GCBD", b"INDY")
    if utility:
        names += (b"UPLH", b"UPLD")
    result = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}
    assert len(result) == len(names)
    return result


def corrupt_heat(source):
    """Create six independent domain/role/eligibility/phase/required-field corruptions from genuine native390 output.

    @param source Actual current writer output with one heat source, direct/generator requests and separate original electricity.
    @return Narrowly edited test-owned parser candidates; every original file remains intact.
    """
    version, stream = read_stream(source)
    assert version == 390
    outputs = []
    # Independent edits preserve original source/consumer identities and
    # quantities except the one corrupt reference/field being tested.
    for fault in ("source-domain", "consumer-domain", "consumer-role", "cross-domain-link", "phase", "missing-domain-field"):
        records = chunks(stream)
        header = next(row for row in records if row["name"] == b"UPLH")
        table = next(row for row in records if row["name"] == b"UPLD")
        if fault == "phase":
            payload = bytearray(header["payload"])
            assert len(payload) == 96
            phase = int.from_bytes(payload[32:40], "big")
            assert phase & 2
            payload[32:40] = (phase & ~2).to_bytes(8, "big")
            header.update(payload=bytes(payload), edited=True)
        elif fault == "missing-domain-field":
            raw = table["rows"][0]
            assert b"domain\0" in raw
            table["rows"][0] = raw.replace(b"domain\0", b"domaiq\0", 1)
            table["edited"] = True
        else:
            source_row = next(row for row in table["rows"][1:] if row[0] == 0 and row[-1] == 1)
            consumer = next(row for row in table["rows"][1:] if row[0] == 1 and row[-1] == 1)
            electric = next(row for row in table["rows"][1:] if row[0] == 0 and row[-1] == 0)
            assert len(source_row) == len(consumer) == len(electric) == 105
            if fault == "cross-domain-link":
                consumer_id = consumer[1:5]
                original = next(row for row in table["rows"][1:] if row[0] == 2 and row[1:5] == consumer_id)
                row = bytearray(original)
                row[5:9] = electric[1:5]
            else:
                original = source_row if fault == "source-domain" else consumer
                row = bytearray(original)
                if fault == "consumer-role":
                    row[11] = 1
                else:
                    row[-1] = 0
            table["rows"][table["rows"].index(original)] = bytes(row)
            table["edited"] = True
        output = DIRECTORY / f"heat390-corrupt-{fault}.sav"
        output.write_bytes(b"OTTN" + source.read_bytes()[4:8] + b"".join(encode_chunk(row) for row in records))
        outputs.append(output)
    return outputs


def main():
    """Prove exact actual domain/role/carry/grant/use loading and genuine legacy electrical preservation.

    @return None; restore original startup script and preserve the retained389 completed collector hash.
    """
    retained = ROOT / "src/tests/fixtures/geothermal/fixture389-collector-fixture-complete.sav"
    original = hashlib.sha256(retained.read_bytes()).hexdigest()
    startup = ROOT / "build/scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    try:
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md2B native shared typed accounting boundary.\nutility_heat_check\n", encoding="utf-8")
        run_case("legacy-read", retained)
        legacy = DIRECTORY / "fixture390-heat-legacy-read.sav"
        assert authority(legacy) == authority(retained)
        run_case("fixture-heat", retained)
        heat = DIRECTORY / "fixture390-heat-fixture-heat.sav"
        assert authority(heat) == authority(retained)
        run_case("heat-read", heat)
        assert authority(DIRECTORY / "fixture390-heat-heat-read.sav", True) == authority(heat, True)
        run_case("fixture-use", heat)
        used = DIRECTORY / "fixture390-heat-fixture-use.sav"
        assert authority(used) == authority(retained)
        run_case("used-read", used)
        assert authority(DIRECTORY / "fixture390-heat-used-read.sav", True) == authority(used, True)
        for corrupt in corrupt_heat(heat):
            run_case("preview", heat, corrupt)
        output = DIRECTORY / "fixture390-heat-normal-resaved.sav"
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md2B ordinary shared heat accounting load.\n"
                           "set savegame_format lzma\n"
                           f'save "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
        log = ROOT / "logs" / f"{PREFIX}-heat-normal-loader.log"
        runner = ROOT / "docs/zz-reach2/upgrades/2026-10/tools/run_game_logged.sh"
        subprocess.run(["C:/Program Files/Git/bin/bash.exe", runner.as_posix(), (ROOT / "build").as_posix(), "Debug", log.as_posix(),
                        "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos",
                        "-c", "native-art.cfg", "-g", used.as_posix()], cwd=ROOT / "build",
                       env=dict(os.environ, OTTD_DEBUG="misc=3,sl=3,grf=2"), creationflags=subprocess.CREATE_NO_WINDOW,
                       timeout=110, check=True)
        assert authority(output, True) == authority(used, True)
        print("Actual390 shared heat proof: genuine389 migration, typed real reservations/expense settlement, exact reload and ordinary game resave; six corrupt previews refuse; original389 input, natural/collector/native/tube authority and decoded electrical lineage preserved; operating generator remains10.6", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        assert hashlib.sha256(retained.read_bytes()).hexdigest() == original, "retained389 input changed"


if __name__ == "__main__":
    main()
