"""Actual393 physical Heat grant/use/expiry proof from retained paid392 native paths.

@file r2mlvs3mp-z-p-heat-allocation-file-checks.py
@see r2mlvs17-geothermal-energy-and-industry.md Group3.
@see r2mlvs3mp-master-plan.md checkpoint10.3; actual operating plant output retains10.6.
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
PREFIX = "r2mlvs3mp-z-p-heat-allocation"


def run_case(mode, source, corrupt=None):
    """Run one separate bounded accepted-world connected heat case.

    @param mode Native migration/grant/use/expiry reader or copied preview.
    @param source Retained input or actual new writer output; never overwritten.
    @param corrupt Optional intentionally malformed current copy.
    @return Successful captured evidence, preserving failed diagnostics.
    """
    suffix = mode if corrupt is None else corrupt.stem
    log = ROOT / "logs" / f"{PREFIX}-{suffix}.log"
    environment = dict(os.environ, OTTD_HEAT_ALLOCATION_CASE=mode, OTTD_HEAT_ALLOCATION_INPUT=str(source))
    if corrupt is not None:
        environment["OTTD_HEAT_ALLOCATION_CORRUPT"] = str(corrupt)
    with log.open("wb") as sink:
        subprocess.run([str(ROOT / "build/Debug/openttd_test.exe"), "[.geothermal-allocation-file]"], cwd=ROOT / "build",
                       env=environment, stdout=sink, stderr=subprocess.STDOUT,
                       creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence and f"HEAT_ALLOCATION_FILE case={mode}" in evidence
    print(f"{suffix}: actual native physical Heat boundary passed", flush=True)
    return evidence


def authority(path, current=False):
    """Compare actual physical/native authority while retaining shared quantities in their sole owner.

    @param path Retained392 predecessor or actual393 writer output.
    @param current Include entire shared ledger and physical route family for exact reload/resave.
    @return Exact original source/stock/geometry chunks and optional current rights.
    """
    version, stream = read_stream(path)
    assert version in (392, 393)
    names = (b"GVNT", b"GCSH", b"GCSD", b"GCBH", b"GCBD", b"INDY", b"TUPH", b"TUPD", b"TUVH", b"TUVD")
    if current:
        names += (b"UPLH", b"UPLD", b"GHTH", b"GHTD")
    records = chunks(stream)
    result = {row["name"]: row["raw"] for row in records if row["name"] in names}
    assert len(result) == len(names)
    return result


def corrupt_routes(source):
    """Create independent physical reference/position/geometry/epoch/revision/metadata faults.

    @param source Actual393 connected grant file, preserved unchanged.
    @return Intentionally malformed current copies; native compression and unrelated owners remain valid.
    """
    version, stream = read_stream(source)
    assert version == 393
    outputs = []
    # Each scalar fault preserves unrelated source/stock/quantity transport;
    # refusal therefore exercises copied physical family authority.
    for fault in ("port-layer", "missing-cell", "cell-position", "source-reference", "epoch", "revision", "metadata"):
        records = chunks(stream)
        table = next(row for row in records if row["name"] == b"GHTD")
        header = next(row for row in records if row["name"] == b"GHTH")
        assert len(header["payload"]) == 24
        if fault in ("epoch", "revision"):
            raw = bytearray(header["payload"])
            offset, size = (16, 8) if fault == "epoch" else (0, 4)
            raw[offset:offset + size] = (0).to_bytes(size, "big")
            header.update(payload=bytes(raw), edited=True)
        elif fault == "metadata":
            metadata = next(row for row in records if row["name"] == b"MLMD")
            raw = bytearray(metadata["rows"][1]); mask = int.from_bytes(raw[31:35], "big")
            assert mask & 131072
            raw[31:35] = (mask & ~131072).to_bytes(4, "big")
            metadata["rows"][1] = bytes(raw); metadata["edited"] = True
        else:
            kind = 0 if fault == "port-layer" else 2
            ordinal = next(index for index, row in enumerate(table["rows"][1:], 1) if len(row) == 17 and row[0] == kind)
            raw = bytearray(table["rows"][ordinal])
            if fault == "port-layer":
                raw[13:17] = (1).to_bytes(4, "big")
            elif fault == "missing-cell":
                raw[13:17] = (0xFFFFFFFF).to_bytes(4, "big")
            elif fault == "cell-position":
                raw[9:13] = (100).to_bytes(4, "big")
            else:
                raw[5:9] = (0xFFFFFFFF).to_bytes(4, "big")
            table["rows"][ordinal] = bytes(raw); table["edited"] = True
        output = DIRECTORY / f"heat393-corrupt-{fault}.sav"
        output.write_bytes(b"OTTN" + source.read_bytes()[4:8] + b"".join(encode_chunk(row) for row in records))
        outputs.append(output)
    return outputs


def main():
    """Verify genuine migration, actual connected whole grants/use/expiry, independent preview and ordinary resave.

    @return None; restore the repository startup script and preserve retained original bytes.
    """
    retained = DIRECTORY / "fixture392-utility-path-normal-resaved.sav"
    original = hashlib.sha256(retained.read_bytes()).hexdigest()
    startup = ROOT / "build/scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    try:
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md Group3 connected Heat file boundary.\nheat_allocation_check\n", encoding="utf-8")
        run_case("legacy-read", retained)
        assert authority(DIRECTORY / "fixture393-heat-allocation-legacy-read.sav") == authority(retained)
        run_case("fixture-grant", retained)
        grant = DIRECTORY / "fixture393-heat-allocation-fixture-grant.sav"
        assert authority(grant) == authority(retained)
        # Every current reader must preserve exact shared source/grant/use
        # and original physical path bytes, with no query-time regeneration.
        for writer, reader, source in ((None, "grant-read", grant), ("fixture-use", "used-read", grant)):
            if writer is not None:
                run_case(writer, source)
                source = DIRECTORY / f"fixture393-heat-allocation-{writer}.sav"
            run_case(reader, source)
            assert authority(DIRECTORY / f"fixture393-heat-allocation-{reader}.sav", True) == authority(source, True)
        used = DIRECTORY / "fixture393-heat-allocation-fixture-use.sav"
        run_case("fixture-expire", used)
        expired = DIRECTORY / "fixture393-heat-allocation-fixture-expire.sav"
        run_case("expired-read", expired)
        assert authority(DIRECTORY / "fixture393-heat-allocation-expired-read.sav", True) == authority(expired, True)
        for corrupt in corrupt_routes(grant):
            run_case("preview", grant, corrupt)
        output = DIRECTORY / "fixture393-heat-allocation-normal-resaved.sav"
        startup.write_text("# @see r2mlvs17-geothermal-energy-and-industry.md Group3 ordinary connected Heat reload.\nset savegame_format lzma\n"
                           f'save "{output.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
        runner = ROOT / "docs/zz-reach2/upgrades/2026-10/tools/run_game_logged.sh"
        log = ROOT / "logs" / f"{PREFIX}-normal-loader.log"
        subprocess.run(["C:/Program Files/Git/bin/bash.exe", runner.as_posix(), (ROOT / "build").as_posix(), "Debug", log.as_posix(),
                        "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", "native-art.cfg", "-g", used.as_posix()],
                       cwd=ROOT / "build", env=dict(os.environ, OTTD_DEBUG="misc=3,sl=3,grf=2"), creationflags=subprocess.CREATE_NO_WINDOW, timeout=110, check=True)
        assert authority(output, True) == authority(used, True)
        print("Actual393 physical Heat: genuine392 absence, connected native whole grants/use/expiry, exact reload, seven malformed previews and ordinary resave. Original natural/native stock/path/startup preserved; operating plant output remains10.6.", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        assert hashlib.sha256(retained.read_bytes()).hexdigest() == original, "retained392 predecessor changed"


if __name__ == "__main__":
    main()
