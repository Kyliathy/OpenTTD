"""Actual391 collector-to-shared-meter saves and genuine389/390 migration.

@file r2mlvs3mp-z-n-thermal-file-checks.py
@see r2mlvs17-geothermal-energy-and-industry.md Group2.
@see r2mlvs3mp-master-plan.md checkpoint10.3; physical links/operating generators retain later groups.
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
PREFIX = "r2mlvs3mp-z-n"


def run_case(mode, source, corrupt=None):
    """Run one bounded accepted-world collector/meter case with retained diagnostics.

    @param mode Migration, publication, saved reader, atomic expense, next tick or preview.
    @param source Retained/native writer input, never overwritten.
    @param corrupt Optional deliberately damaged test copy for independent preview refusal.
    @return Captured successful native evidence; failure preserves its log.
    """
    suffix = mode if corrupt is None else corrupt.stem
    log = ROOT / "logs" / f"{PREFIX}-thermal-{suffix}.log"
    environment = dict(os.environ, OTTD_THERMAL_SOURCE_CASE=mode, OTTD_THERMAL_SOURCE_INPUT=str(source))
    if corrupt is not None:
        environment["OTTD_THERMAL_SOURCE_CORRUPT"] = str(corrupt)
    with log.open("wb") as sink:
        subprocess.run([str(ROOT / "build/Debug/openttd_test.exe"), "[.geothermal-heat-file]"], cwd=ROOT / "build",
                       env=environment, stdout=sink, stderr=subprocess.STDOUT,
                       creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence and f"THERMAL_SOURCE_FILE case={mode}" in evidence
    print(f"{suffix}: actual collector/shared-meter boundary passed", flush=True)
    return evidence


def authority(path, current=False):
    """Compare exact physical source/stock transport and, optionally, current meter/clock state.

    @param path Retained predecessor or actual native current output.
    @param current Include versioned construction reference, shared ledger and common clock for exact current reloads.
    @return Exact selected chunks; no decoded private runtime meter is created.
    """
    version, stream = read_stream(path)
    assert version in (389, 390, 391)
    names = (b"GVNT", b"GCSH", b"GCSD", b"INDY", b"TUPH", b"TUPD")
    if current:
        names += (b"GCBH", b"GCBD", b"UPLH", b"UPLD", b"TUVH", b"TUVD")
    result = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}
    assert len(result) == len(names)
    return result


def corrupt_sources(source):
    """Create narrow391 reference/role/rate/version/required-field faults while preserving the good file.

    @param source Actual391 publication/grant writer output.
    @return Deliberately damaged independent parser candidates under the feature fixture directory.
    """
    version, stream = read_stream(source)
    assert version == 391
    outputs = []
    # Each independent fault targets one association contract, so refusal
    # cannot be credited merely to broken compression or unrelated terrain.
    for fault in ("missing-source", "electrical-source", "unfinished-source", "inflated-rate", "old-revision", "missing-field"):
        records = chunks(stream)
        table = next(row for row in records if row["name"] == b"GCBD")
        header = next(row for row in records if row["name"] == b"GCBH")
        utility = next(row for row in records if row["name"] == b"UPLD")
        original = table["rows"][1]
        assert len(original) == 23
        if fault == "old-revision":
            payload = bytearray(header["payload"])
            payload[:4] = (1).to_bytes(4, "big")
            header.update(payload=bytes(payload), edited=True)
        elif fault == "missing-field":
            assert b"heat_producer\0" in table["rows"][0]
            table["rows"][0] = table["rows"][0].replace(b"heat_producer\0", b"heat_produceq\0", 1)
            table["edited"] = True
        elif fault == "inflated-rate":
            # Change only the natural rate. The generic utility ledger still
            # validates, while association checking detects that its otherwise
            # valid meter no longer agrees with the original physical source.
            vent = next(record for record in records if record["name"] == b"GVNT")
            natural = bytearray(vent["rows"][1])
            natural[22:30] = (1000001).to_bytes(8, "big")
            vent["rows"][1] = bytes(natural)
            vent["edited"] = True
        else:
            row = bytearray(original)
            if fault == "missing-source":
                row[19:23] = (3000).to_bytes(4, "big")
            elif fault == "electrical-source":
                electric = next(candidate for candidate in utility["rows"][1:] if candidate[0] == 0 and candidate[-1] == 0)
                row[19:23] = electric[1:5]
            else:
                row[7:9] = (295).to_bytes(2, "big")
            table["rows"][1] = bytes(row)
            table["edited"] = True
        output = DIRECTORY / f"thermal391-corrupt-{fault}.sav"
        output.write_bytes(b"OTTN" + source.read_bytes()[4:8] + b"".join(encode_chunk(row) for row in records))
        outputs.append(output)
    return outputs


def main():
    """Verify native migration, offers, atomic settlement, next tick, malformed preview and ordinary game resave.

    @return None; preserve predecessor files and the repository's original startup script.
    """
    retained = DIRECTORY / "fixture389-collector-fixture-complete.sav"
    earlier = ROOT / "src/tests/fixtures/utility/fixture390-heat-normal-resaved.sav"
    originals = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in (retained, earlier)}
    startup = ROOT / "build/scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    try:
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md Group2 native source boundary.\nthermal_source_check\n", encoding="utf-8")
        run_case("legacy-read", earlier)
        assert authority(DIRECTORY / "fixture391-thermal-legacy-read.sav") == authority(earlier)
        run_case("legacy-read", retained)
        assert authority(DIRECTORY / "fixture391-thermal-legacy-read.sav") == authority(retained)
        run_case("fixture-offer", retained)
        offer = DIRECTORY / "fixture391-thermal-fixture-offer.sav"
        assert authority(offer) == authority(retained)
        run_case("offer-read", offer)
        assert authority(DIRECTORY / "fixture391-thermal-offer-read.sav", True) == authority(offer, True)
        run_case("fixture-use", offer)
        used = DIRECTORY / "fixture391-thermal-fixture-use.sav"
        run_case("used-read", used)
        assert authority(DIRECTORY / "fixture391-thermal-used-read.sav", True) == authority(used, True)
        run_case("fixture-next", used)
        advanced = DIRECTORY / "fixture391-thermal-fixture-next.sav"
        run_case("next-read", advanced)
        assert authority(DIRECTORY / "fixture391-thermal-next-read.sav", True) == authority(advanced, True)
        for corrupt in corrupt_sources(offer):
            run_case("preview", offer, corrupt)
        output = DIRECTORY / "fixture391-thermal-normal-resaved.sav"
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md Group2 ordinary shared source load.\n"
                           "set savegame_format lzma\n"
                           f'save "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
        runner = ROOT / "docs/zz-reach2/upgrades/2026-10/tools/run_game_logged.sh"
        log = ROOT / "logs" / f"{PREFIX}-thermal-normal-loader.log"
        subprocess.run(["C:/Program Files/Git/bin/bash.exe", runner.as_posix(), (ROOT / "build").as_posix(), "Debug", log.as_posix(),
                        "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos",
                        "-c", "native-art.cfg", "-g", used.as_posix()], cwd=ROOT / "build",
                       env=dict(os.environ, OTTD_DEBUG="misc=3,sl=3,grf=2"), creationflags=subprocess.CREATE_NO_WINDOW,
                       timeout=110, check=True)
        assert authority(output, True) == authority(used, True)
        print("Actual391 thermal source proof: genuine389/390 migration, completed collector binding, bounded offer, real shared reservation/expense, exact reload/next tick, six malformed previews and ordinary resave. Physical heat links and operating generator remain later owners.", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        for path, digest in originals.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, "retained source input changed"


if __name__ == "__main__":
    main()
