"""Focused actual389 collector files, preserving retained388 cold and natural/electrical authorities.

@file r2mlvs3mp-z-j-geothermal-collector-file-checks.py
@see r2mlvs3mp-master-plan.md checkpoint10.3; transported paid bootstrap remains10.6.
@see r2mlvs17-geothermal-energy-and-industry.md Group1B.
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
PREFIX = "r2mlvs3mp-z-j"


def run_case(mode, source, corrupt=None):
    """Run one bounded accepted-world native construction case.

    @param mode Actual funding, stock-shortage, paid-work or exact-reader case.
    @param source Retained native input, never overwritten.
    @param corrupt Optional narrowly changed test-owned parser candidate.
    @return Captured successful real native evidence.
    """
    suffix = mode if corrupt is None else corrupt.stem
    log = ROOT / "logs" / f"{PREFIX}-collector-{suffix}.log"
    environment = dict(os.environ, OTTD_GEOTHERMAL_COLLECTOR_CASE=mode, OTTD_GEOTHERMAL_COLLECTOR_INPUT=str(source))
    if corrupt is not None:
        environment["OTTD_GEOTHERMAL_COLLECTOR_CORRUPT"] = str(corrupt)
    with log.open("wb") as sink:
        subprocess.run([str(ROOT / "build/Debug/openttd_test.exe"), "[.geothermal-collector-file]"], cwd=ROOT / "build",
                       env=environment, stdout=sink, stderr=subprocess.STDOUT,
                       creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence and f"GEOTHERMAL_COLLECTOR_FILE case={mode}" in evidence
    print(f"{suffix}: actual native collector boundary passed", flush=True)
    return evidence


def authority(path, construction=False):
    """Read exact real owner bytes without a private model of native stock, natural capacity or electricity.

    @param path Retained input or real native output.
    @param construction Include cold/built construction and native stocks when comparing exact reloads.
    @return Actual authoritative chunks; no packed geometry is fabricated by this script.
    """
    version, stream = read_stream(path)
    assert version in (388, 389)
    names = (b"GVNT", b"UPLH", b"UPLD", b"TUPH", b"TUPD", b"TUVH", b"TUVD")
    if construction:
        names += (b"GCSH", b"GCSD", b"GCBH", b"GCBD", b"INDY")
    result = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}
    assert len(result) == len(names)
    return result


def corrupt_collectors(source):
    """Create eight independent bad native construction previews, retaining the actual writer input.

    @param source Genuine funded389 native collector with no completed material receipt/work.
    @return Missing/count/work/receipt/native-industry/cold-site/company-family corruption candidates.
    """
    version, stream = read_stream(source)
    assert version == 389
    outputs = []
    # Each corruption starts from the same real native world. No edited
    # candidate can contaminate another or erase the retained cold input.
    for fault in ("missing-table", "bad-count", "bad-work", "free-receipt", "bad-industry", "bad-site", "bad-owner", "missing-cold-family"):
        records = chunks(stream)
        header = next(row for row in records if row["name"] == b"GCBH")
        table = next(row for row in records if row["name"] == b"GCBD")
        if fault == "missing-table":
            records.remove(table)
        elif fault == "missing-cold-family":
            records.remove(next(row for row in records if row["name"] == b"GCSD"))
        elif fault == "bad-count":
            payload = bytearray(header["payload"])
            assert len(payload) == 8
            payload[4:8] = (257).to_bytes(4, "big")
            header.update(payload=bytes(payload), edited=True)
        else:
            row = bytearray(table["rows"][1])
            assert len(row) == 19
            if fault == "bad-work":
                row[7:9] = (297).to_bytes(2, "big")
            elif fault == "free-receipt":
                row[9] = 1
            elif fault == "bad-industry":
                row[0:2] = (63999).to_bytes(2, "big")
            elif fault == "bad-site":
                row[2:6] = (255).to_bytes(4, "big")
            else:
                row[6] = 14
            table["rows"][1] = bytes(row)
            table["edited"] = True
        output = DIRECTORY / f"collector389-corrupt-{fault}.sav"
        output.write_bytes(b"OTTN" + source.read_bytes()[4:8] + b"".join(encode_chunk(row) for row in records))
        outputs.append(output)
    return outputs


def main():
    """Prove real funding, shortage and paid construction reload with exact original owners and ordinary loading.

    @return None; restore the original startup script and preserve the retained388 cold input hash.
    """
    retained = DIRECTORY / "fixture388-cold-site-fixture-create.sav"
    original = hashlib.sha256(retained.read_bytes()).hexdigest()
    startup = ROOT / "build/scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    try:
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md1B actual construction boundary.\ngeothermal_collector_check\n", encoding="utf-8")
        run_case("legacy-read", retained)
        assert authority(DIRECTORY / "fixture389-collector-legacy-read.sav") == authority(retained)
        run_case("fixture-fund", retained)
        funded = DIRECTORY / "fixture389-collector-fixture-fund.sav"
        assert authority(funded) == authority(retained)
        # Exact reads cover native input stocks and saved receipt/work/tick as
        # well as cold/source/electrical owners. Progress uses actual paid work.
        for produce, read, source in (
            (None, "funded-read", funded),
            ("fixture-shortage", "shortage-read", funded),
            ("fixture-partial", "partial-read", DIRECTORY / "fixture389-collector-fixture-shortage.sav"),
            ("fixture-complete", "complete-read", DIRECTORY / "fixture389-collector-fixture-partial.sav"),
        ):
            if produce is not None:
                run_case(produce, source)
                source = DIRECTORY / f"fixture389-collector-{produce}.sav"
            run_case(read, source)
            assert authority(DIRECTORY / f"fixture389-collector-{read}.sav", True) == authority(source, True)
            assert authority(source) == authority(retained)
        for corrupt in corrupt_collectors(funded):
            run_case("preview", funded, corrupt)
        completed = DIRECTORY / "fixture389-collector-fixture-complete.sav"
        output = DIRECTORY / "fixture389-collector-normal-resaved.sav"
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md1B ordinary completed collector loading.\n"
                           "set savegame_format lzma\n"
                           f'save "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
        log = ROOT / "logs" / f"{PREFIX}-collector-normal-loader.log"
        runner = ROOT / "docs/zz-reach2/upgrades/2026-10/tools/run_game_logged.sh"
        subprocess.run(["C:/Program Files/Git/bin/bash.exe", runner.as_posix(), (ROOT / "build").as_posix(), "Debug", log.as_posix(),
                        "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos",
                        "-c", "native-art.cfg", "-g", completed.as_posix()], cwd=ROOT / "build",
                       env=dict(os.environ, OTTD_DEBUG="misc=3,sl=3,grf=2"), creationflags=subprocess.CREATE_NO_WINDOW,
                       timeout=110, check=True)
        assert authority(output, True) == authority(completed, True)
        print("Actual389 collector proof: full capital settlement, exact funded/shortage/partial/completed reload, eight corrupt preview refusals and ordinary game loading; original388 input, natural capacity and all electrical chunks preserved; transported bootstrap remains10.6", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        assert hashlib.sha256(retained.read_bytes()).hexdigest() == original, "retained388 input changed"


if __name__ == "__main__":
    main()
