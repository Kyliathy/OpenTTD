#!/usr/bin/env python3
"""Dedicated generation and Surface preservation with saved metadata feature seeds.

@file geology_generation_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 3
"""

import hashlib
import json
import sys

from multilayer_state_fixture import AREA, ROOT, log_path, run_case

sys.path.insert(0, str(AREA / "tools"))
from savegame_plane_digest import Reader, PLANE_BYTES, CHUNK_RIFF, CHUNK_RECORDS, read_stream
from inspect_layer_save import read_chunks


def saved_geology_seed(stream):
    """Read the exact schema-1 metadata row without depending on the next chunk's position.

    @param stream Actual decompressed dedicated save output.
    @return Saved geology root, independent of future geology chunk placement.
    """
    reader = Reader(stream)
    # Skip ordinary chunk transport, retaining only the metadata table's own bounded row.
    while True:
        chunk = reader.take(4)
        if chunk == bytes(4):
            raise ValueError("new fixture lacks metadata")
        mode = reader.byte()
        kind = mode & 15
        if kind == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            reader.take(length)
            continue
        if kind not in CHUNK_RECORDS:
            raise ValueError("unsupported chunk transport")
        rows = []
        while True:
            length = reader.gamma()
            if not length:
                break
            rows.append(reader.take(length - 1))
        if chunk == b"MLMD":
            # The existing mandatory schema-1 table has one header and one 51-byte row; another format needs an explicit fixture update.
            if len(rows) != 2 or len(rows[1]) != 51 or rows[1][:2] != b"\x00\x01":
                raise ValueError("unexpected metadata transport")
            return int.from_bytes(rows[1][35:43], "big")


def surface_digests(stream):
    """Hash actual saved Surface fields using the existing plane reader, without changing supplied saves.

    @param stream Actual decompressed 64x64 generated world.
    @return Ten field-to-SHA256 values over precisely the first Surface plane.
    """
    planes, _ = read_chunks(stream)
    return {name.decode("ascii"): hashlib.sha256(planes[name][:4096 * width]).hexdigest()
            for name, width in PLANE_BYTES.items()}


def main():
    """Generate native/repeated/Classic candidates and verify saved roots and unchanged Surface fields.

    @return None; failures retain logs and restore all existing configuration/startup bytes.
    """
    startup = ROOT / "build/scripts/game_start.scr"
    config = ROOT / "build/grftest.cfg"
    original_script = startup.read_bytes()
    original_config = config.read_bytes()
    saves = {}
    try:
        # Only profile selection changes between these actual worlds; seed, climate, size and every Surface setting are identical.
        for name, profile in (("native", "multilayer_industry"), ("repeated", "multilayer_industry"), ("classic", "classic")):
            config.write_text("# @see r2mlvs2-geology-and-excavation.md, generation fixture.\n"
                              "[game_creation]\nmap_x = 6\nmap_y = 6\nlandscape = temperate\n"
                              f"gameplay_profile = {profile}\n"
                              "[network]\nserver_advertise = false\nserver_bind_addresses = 127.0.0.1\n", encoding="utf-8")
            expected = "MLVS_PROFILE id=0 version=0" if profile == "classic" else "MLVS_GEOLOGY generator=1"
            saves[name] = run_case(name, ["-G", "1234"], expected, prefix="r2mlvs2-e")
        streams = {name: read_stream(path)[1] for name, path in saves.items()}
        roots = {name: saved_geology_seed(stream) for name, stream in streams.items()}
        assert roots["native"] != 0, "native root not saved"
        assert roots["native"] == roots["repeated"], "repeated root differs"
        assert roots["classic"] == 0, "Classic acquired geology"
        digests = {name: surface_digests(stream) for name, stream in streams.items()}
        assert digests["native"] == digests["repeated"] == digests["classic"], "Surface generation changed"
        assert "MLVS_GEOLOGY" not in log_path("r2mlvs2-e-classic.log").read_text(encoding="utf-8", errors="replace"), "Classic activated geology"
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
    (AREA / "r2mlvs2-e-generation-results.txt").write_text(
        "R2MLVS2 Group 3: native/repeated/Classic dedicated generation passed; saved roots and all ten Surface fields verified.\n"
        "Current MLVI supplies cargos, with no new Surface industry definitions; enabled geology leaves Surface unchanged.\n"
        + json.dumps({"roots": roots, "surface": digests["classic"]}, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
