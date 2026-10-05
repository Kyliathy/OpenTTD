#!/usr/bin/env python3
"""Actual geological round trips, older absence migration and malformed owned save refusals.

@file geology_save_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 4
"""

import argparse
import hashlib
import lzma
from functools import partial as bind_fixture_mutation

from geology_generation_fixture import Reader, CHUNK_RIFF, CHUNK_RECORDS, read_stream, saved_geology_seed
from multilayer_state_fixture import AREA, ROOT, run_case


def chunks(stream):
    """Collect immutable raw chunks and their payloads/rows for narrowly owned fixture mutations.

    @param stream Actual decompressed 372 save bytes.
    @return Ordered dictionaries; unknown ordinary chunks retain their original exact transport.
    """
    reader = Reader(stream)
    result = []
    # Preserve unrelated descriptors byte-for-byte; only targeted geological authority is changed by the caller.
    while True:
        start = reader.pos
        name = reader.take(4)
        if name == bytes(4):
            return result
        mode = reader.byte()
        kind = mode & 15
        payload = None
        rows = []
        if kind == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            payload = reader.take(length)
        elif kind in CHUNK_RECORDS:
            while True:
                length = reader.gamma()
                if not length:
                    break
                rows.append(reader.take(length - 1))
        else:
            raise ValueError("unsupported fixture chunk transport")
        result.append({"name": name, "kind": kind, "raw": stream[start:reader.pos], "payload": payload, "rows": rows})


def gamma(value):
    """Encode the existing chunk-record length format for an owned bounded fixture.

    @param value Unsigned record byte count plus its format's one-byte adjustment.
    @return Exact gamma bytes; values outside the format refuse.
    """
    for width, bound, prefix in ((1, 0x80, 0), (2, 0x4000, 0x80), (3, 0x200000, 0xc0), (4, 0x10000000, 0xe0)):
        if value < bound:
            encoded = bytearray(value.to_bytes(width, "big"))
            encoded[0] |= prefix
            return bytes(encoded)
    if value > 0xffffffff:
        raise ValueError("gamma value exceeds uint32")
    return b"\xf0" + value.to_bytes(4, "big")


def encode_chunk(chunk):
    """Write only one edited owner's transport while preserving its declared chunk type.

    @param chunk Geological chunk dictionary with bounded replacement payload/rows.
    @return Complete raw chunk transport.
    """
    if chunk["kind"] == CHUNK_RIFF:
        payload = chunk["payload"]
        length = len(payload)
        return chunk["name"] + bytes([(length >> 24) << 4]) + (length & 0xffffff).to_bytes(3, "big") + payload
    return chunk["name"] + bytes([chunk["kind"]]) + b"".join(gamma(len(row) + 1) + row for row in chunk["rows"]) + b"\0"


def variant(source, name, mutation, prefix="r2mlvs2-g"):
    """Create one repository-owned variant without changing the supplied original source bytes.

    @param source Actual native geological output in a delivered 372-through-374 format, produced by the owning fixture.
    @param name Owning plan's lowercase evidence suffix.
    @param mutation Callable changing only targeted geological dictionaries; remove marks an omitted chunk.
    @param prefix Owning plan's chronological evidence prefix; defaults to the geological save group.
    @return New local evidence save, preserving every unrelated raw chunk.
    """
    original = source.read_bytes()
    version, stream = read_stream(source)
    if version not in (372, 373, 374) or original[:4] != b"OTTX":
        raise ValueError("fixture needs actual LZMA/version-372-through-374 output")
    records = chunks(stream)
    mutation(records)
    encoded = b"".join(encode_chunk(record) if record.get("edited") else record["raw"]
                       for record in records if not record.get("remove")) + bytes(4)
    path = AREA / f"{prefix}-{name}-input.sav"
    path.write_bytes(original[:8] + lzma.compress(encoded))
    return path


def geology_chunk(records, name):
    """Locate one exact geological owner before mutating a fixture.

    @param records Parsed actual raw chunks.
    @param name Required four-byte owner label.
    @return The uniquely matched record, marked for re-encoding.
    """
    matches = [record for record in records if record["name"] == name]
    if len(matches) != 1:
        raise ValueError("fixture owner chunk missing/duplicated")
    matches[0]["edited"] = True
    return matches[0]


def change_partial(records):
    """Apply a bounded depleted/surveyed balance and one real cavity to owned transport.

    @param records Actual geological dictionaries; unrelated chunks remain exact.
    @return None; mutation creates no new ore identity or saved job.
    """
    deposits = geology_chunk(records, b"GDEP")
    row = bytearray(deposits["rows"][1])
    if len(row) != 36:
        raise ValueError("unexpected deposit codec")
    row[19:27] = (17).to_bytes(8, "big")
    row[27:35] = (1).to_bytes(8, "big")
    deposits["rows"][1] = bytes(row)
    terrain = geology_chunk(records, b"GEOS")
    payload = bytearray(terrain["payload"])
    index = 7 * 64 + 3
    payload[29 + index // 4] |= 2 << ((index % 4) * 2)
    terrain["payload"] = bytes(payload)


def corrupt(records, kind):
    """Introduce one structural/reference failure with unchanged unrelated state.

    @param records Actual owned fixture chunks.
    @param kind Named tested failure: reserved, truncated, dimensions, reference or missing.
    @return None; the resulting world must refuse before startup/save.
    """
    if kind == "reference":
        spans = geology_chunk(records, b"GSPN")
        row = bytearray(spans["rows"][1])
        row[:4] = (999).to_bytes(4, "big")
        spans["rows"][1] = bytes(row)
        return
    terrain = geology_chunk(records, b"GEOS")
    payload = bytearray(terrain["payload"])
    if kind == "reserved":
        payload[29 + (7 * 64 + 3) // 4] = 3
    elif kind == "truncated":
        payload.pop()
    elif kind == "dimensions":
        payload[4:8] = (128).to_bytes(4, "big")
    elif kind == "missing":
        terrain["remove"] = True
    else:
        raise ValueError("unknown fixture mutation")
    terrain["payload"] = bytes(payload)


def main():
    """Run focused geological save/refusal cases, preserving original scripts/config and older supplied saves.

    @return None; failed boundaries retain logs and always restore original inputs.
    """
    startup = ROOT / "build/scripts/game_start.scr"
    config = ROOT / "build/grftest.cfg"
    original_script = startup.read_bytes()
    original_config = config.read_bytes()
    older = (("native371", AREA / "r2mlvs14-b-native-new.sav"),
             ("legacy370", ROOT / "build/save/camera-deep-repaired.sav"))
    older_hashes = {path: hashlib.sha256(path.read_bytes()).hexdigest() for _, path in older}
    parser = argparse.ArgumentParser()
    parser.add_argument("--refusals-only", action="store_true", help="retain/verify accepted-load evidence and repeat only the current corrupt boundaries")
    args = parser.parse_args()
    try:
        config.write_text("# @see r2mlvs2-geology-and-excavation.md, saved geology fixture.\n"
                          "[game_creation]\nmap_x = 6\nmap_y = 6\nlandscape = temperate\n"
                          "gameplay_profile = multilayer_industry\n"
                          "[network]\nserver_advertise = false\nserver_bind_addresses = 127.0.0.1\n", encoding="utf-8")
        if not args.refusals_only:
            source = run_case("native", ["-G", "1234"], "MLVS_GEOLOGY generator=1", prefix="r2mlvs2-g")
            source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
            reloaded = run_case("reload", ["-g", source.as_posix()], "MLVS_GEOLOGY_LOADED generator=1 deposits=10 packed_bytes=2048", prefix="r2mlvs2-g")
            partial = variant(source, "partial", change_partial)
            saved_partial = run_case("partial", ["-g", partial.as_posix()], "MLVS_GEOLOGY_LOADED generator=1 deposits=10 packed_bytes=2048", prefix="r2mlvs2-g")
            repeated_partial = run_case("partial-reload", ["-g", saved_partial.as_posix()], "MLVS_GEOLOGY_LOADED generator=1 deposits=10 packed_bytes=2048", prefix="r2mlvs2-g")
            original_rows = {record["name"]: record for record in chunks(read_stream(partial)[1])}
            # Complete authority survives two actual loaders; spatial cache addresses and reserve/survey quantities cannot be reconstructed differently.
            for output in (saved_partial, repeated_partial):
                restored_rows = {record["name"]: record for record in chunks(read_stream(output)[1])}
                for label in (b"GEOS", b"GDEP", b"GSPN"):
                    assert restored_rows[label]["raw"] == original_rows[label]["raw"], (label, "geological authority changed on reload")
            assert saved_geology_seed(read_stream(source)[1]) == saved_geology_seed(read_stream(reloaded)[1]), "seed changed"
            # Older explicit native and four-layer infrastructure files have no geological chunks: neither becomes solid after migration.
            for name, path in older:
                run_case(name, ["-g", path.as_posix()], "MLVS_GEOLOGY_LOADED generator=0 deposits=0 packed_bytes=0", prefix="r2mlvs2-g")
        else:
            source = AREA / "r2mlvs2-g-native.sav"
            source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
            # A partial rerun reports accepted boundaries only after their retained logs and saved outputs still prove success.
            for name, expected in (("native", "MLVS_GEOLOGY generator=1"),
                                   ("reload", "MLVS_GEOLOGY_LOADED generator=1"),
                                   ("partial", "MLVS_GEOLOGY_LOADED generator=1"),
                                   ("partial-reload", "MLVS_GEOLOGY_LOADED generator=1"),
                                   ("native371", "MLVS_GEOLOGY_LOADED generator=0"),
                                   ("legacy370", "MLVS_GEOLOGY_LOADED generator=0")):
                evidence = (AREA / f"r2mlvs2-g-{name}.log").read_text(encoding="utf-8", errors="replace")
                assert expected in evidence, (name, "prior acceptance evidence missing")
                assert (AREA / f"r2mlvs2-g-{name}.sav").is_file(), (name, "prior accepted save missing")
        for kind, expected in (("reserved", "Reserved geology terrain encoding"),
                               ("truncated", "Invalid geology payload bounds"),
                               ("dimensions", "Invalid geology dimensions"),
                               ("reference", "Invalid geology span reference"),
                               ("missing", "Missing required geology header")):
            malformed = variant(source, kind, bind_fixture_mutation(corrupt, kind=kind))
            run_case(kind, ["-g", malformed.as_posix()], expected, accepted=False, prefix="r2mlvs2-g")
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash, "source fixture changed"
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
        for path, digest in older_hashes.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, "supplied older save changed"
    scope = "Six earlier accepted-load boundaries verified from retained evidence; five refusals executed in this run.\n" if args.refusals_only else "All eleven boundaries executed in this run.\n"
    (AREA / "r2mlvs2-g-save-results.txt").write_text(
        "R2MLVS2 Group 4: six actual geological/older loads and five corrupt/missing/reference refusals passed.\n"
        "Partial balance, survey information, cavity and all three owner chunks survived two reloads exactly.\n"
        "Original source/config/startup/older-save SHA256 preserved. Job transport belongs to Group 10.\n" + scope, encoding="utf-8")


if __name__ == "__main__":
    main()
