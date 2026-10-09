#!/usr/bin/env python3
"""Actual original/owned version-370 topology and retained ownerless Native portal reloads.

@file portal_acceptance_save_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 16
"""

import hashlib
import lzma
import struct
import sys

from geology_save_fixture import chunks, encode_chunk, geology_chunk, read_stream, variant
from multilayer_state_fixture import AREA, ROOT, log_path, run_case, save_path

PREFIX = "r2mlvs3-r"
FIELDS = ((b"MAPT", 1), (b"MAPO", 1), (b"MAP2", 2), (b"M3LO", 1),
          (b"M3HI", 1), (b"MAP5", 1), (b"MAPE", 1), (b"MAP7", 1), (b"MAP8", 2))


# Each producer declares its feature; saved-input names never select output ownership.
from functools import partial
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import current_run_context, run_native
from multilayer_state_fixture import output_save_path
CONTEXT = current_run_context("portals", "portal-acceptance-save-fixture")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime("grftest.cfg")
output_save_path = partial(output_save_path, context=CONTEXT)
save_path = partial(save_path, context=CONTEXT)
run_case = partial(run_case, context=CONTEXT)
log_path = partial(log_path, context=CONTEXT)
variant = partial(variant, context=CONTEXT)


def housing(records, source, target, road, owner, source_layer, target_layer):
    """Encode only the existing public housing constructor's exact owned fixture fields.

    @param records Actual parsed save transport; unrelated chunks remain exact.
    @param source Actual upper endpoint address.
    @param target Actual reciprocal lower endpoint address.
    @param road Select the existing rail or road constructor.
    @param owner Actual saved map Owner; these owned legacy/ownerless fixtures use OWNER_NONE.
    @param source_layer Actual stable source layer value.
    @param target_layer Actual stable destination layer value.
    @return None; this is a load-boundary fixture, not gameplay excavation or construction.
    """
    # Preserve all unrelated tiles/fields while matching both public reciprocal constructors, including road/tram owners and the invalid tram type.
    for label, width in FIELDS:
        chunk = geology_chunk(records, label)
        payload = bytearray(chunk["payload"])
        for tile, other, shaft in ((source, target_layer, 1), (target, source_layer, 3)):
            value = 0
            if label == b"MAPT":
                value = (payload[tile] & 15) | (11 << 4)
            elif label == b"MAPO":
                value = owner
            elif label == b"MAP5":
                value = (other << 5) | (4 if road else 0) | shaft
            elif road and label == b"M3HI":
                value = 15 << 4  # Public SetRoadOwner maps OWNER_NONE tram ownership to OWNER_TOWN's four-bit representation.
            elif road and label == b"MAP7":
                value = owner
            elif road and label == b"MAP8":
                value = 63 << 6  # Actual INVALID_ROADTYPE in the public tram-type field.
            begin = tile * width
            payload[begin:begin + width] = value.to_bytes(width, "big")
        chunk["payload"] = bytes(payload)


def ownerless(records):
    """Retain an explicitly ownerless Sealed Native road pair without introducing a company or opening ground.

    @param records Actual owned Native Sealed rail input, including its exact packed geology.
    @return None; sole phase/reference authority remains unassigned.
    """
    housing(records, 1300, 5396, True, 16, 0, 1)
    work = geology_chunk(records, b"LPWK")
    work["rows"][1] = struct.pack(">IBBIIIQ", 1300, 0, 255, 0xFFFFFFFF, 0xFFFFF, 0xFFFF, 2)


def main():
    """Reload the untouched original and owned topology through the actual current executable.

    @return None; retain exact supplied inputs/startup/config bytes and repository-local evidence.
    """
    startup, config = RUNTIME / "scripts/game_start.scr", RUNTIME / "grftest.cfg"
    originals = (ROOT / "build/save/camera-deep-repaired.sav", save_path("r2mlvs3-e-sealed-input.sav"))
    hashes = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in originals}
    original_script, original_config = startup.read_bytes(), config.read_bytes()
    try:
        version, stream = read_stream(originals[0])
        assert version == 370
        original = run_case("original370", ["-g", originals[0].as_posix()], "MLVS_METADATA absent profile=0 version=0 legacy=true", prefix=PREFIX)
        records = chunks(stream)
        dimensions = geology_chunk(records, b"MAPS")
        width, height, layers = struct.unpack(">IIB", dimensions["rows"][1])
        dimensions["edited"] = False
        assert layers == 4
        plane = width * height
        endpoints = []
        # Separate real legacy rail, road tunnel and Surface/Deep elevator pairs use the existing version-370 map layout, with no work/geology additions.
        for x, road, target_layer in ((20, False, 1), (24, True, 1), (28, True, 3)):
            source, target = 20 * width + x, target_layer * plane + 20 * width + x
            housing(records, source, target, road, 16, 0, target_layer)
            endpoints.extend((source, target))
        encoded = b"".join(encode_chunk(record) if record.get("edited") else record["raw"] for record in records) + bytes(4)
        authored = output_save_path(f"{PREFIX}-legacy-pairs-input.sav")
        authored.write_bytes(originals[0].read_bytes()[:8] + lzma.compress(encoded))
        loaded = run_case("legacy-pairs", ["-g", authored.as_posix()], "MLVS_METADATA absent profile=0 version=0 legacy=true", prefix=PREFIX)
        repeated = run_case("legacy-repeat", ["-g", loaded.as_posix()], "profile=0 version=0 legacy=true", prefix=PREFIX)
        expected = {record["name"]: record for record in chunks(read_stream(authored)[1])}
        # Exact public endpoint metadata must survive both actual loaders; Classic allocates no work rows and migration creates no packed excavation authority.
        for output in (loaded, repeated):
            restored = {record["name"]: record for record in chunks(read_stream(output)[1])}
            for label, width in FIELDS:
                for tile in endpoints:
                    begin = tile * width
                    assert restored[label]["payload"][begin:begin + width] == expected[label]["payload"][begin:begin + width], (label, tile)
            assert len(restored[b"LPWK"]["rows"]) == 1
        retained = variant(originals[1], "ownerless", ownerless, prefix=PREFIX)
        loaded_ownerless = run_case("ownerless", ["-g", retained.as_posix()], "MLVS_PORTAL_WORK_LOADED pairs=1", prefix=PREFIX)
        repeated_ownerless = run_case("ownerless-repeat", ["-g", loaded_ownerless.as_posix()], "MLVS_PORTAL_WORK_LOADED pairs=1", prefix=PREFIX)
        expected_ownerless = {record["name"]: record["raw"] for record in chunks(read_stream(retained)[1])}
        for output in (loaded_ownerless, repeated_ownerless):
            restored = {record["name"]: record["raw"] for record in chunks(read_stream(output)[1])}
            for label in (b"LPWH", b"LPWK", b"GEOS", b"GDEP", b"GSPN", b"GVNT"):
                assert restored[label] == expected_ownerless[label], label
        assert original.is_file()
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
        for path, digest in hashes.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    (CONTEXT.reports / f"{PREFIX}-save-results.txt").write_text(
        "@see r2mlvs3-sealed-portals-and-deep-rail.md, Group 16.\n"
        "Five actual loads passed: unchanged original370, owned legacy rail/road/elevator plus repeat, ownerless Native road plus repeat.\n"
        "Legacy endpoints remain exact; Native work/geology chunks remain byte-exact. Supplied saves/startup/config retained.\n"
        "Original370 contains no portal tiles; owned version-370 portal topology is separate load evidence, not a claim of original fleet scenarios.\n", encoding="utf-8")


def resume_ownerless_migration():
    """Check only the newly added pre-work-schema Native ownerless-road migration.

    @return None; preserve retained owned input/startup/config and all existing accepted evidence.
    """
    prefix = "r2mlvs3-u"
    source = save_path(f"{PREFIX}-ownerless-input.sav")
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    startup, config = RUNTIME / "scripts/game_start.scr", RUNTIME / "grftest.cfg"
    original_script, original_config = startup.read_bytes(), config.read_bytes()
    try:
        version, stream = read_stream(source)
        assert version in (375, 376, 377, 378)
        records = chunks(stream)
        metadata = geology_chunk(records, b"MLMD")
        row = bytearray(metadata["rows"][1])
        row[31:35] = (int.from_bytes(row[31:35], "big") & ~16).to_bytes(4, "big")
        metadata["rows"][1] = bytes(row)
        # Version 374 predates portal work chunks; retain actual Native topology/geology while removing only the absent family's identity.
        for record in records:
            if record["name"] in (b"LPWH", b"LPWK"):
                record["remove"] = True
        encoded = b"".join(encode_chunk(record) if record.get("edited") else record["raw"]
                           for record in records if not record.get("remove")) + bytes(4)
        authored = output_save_path(f"{prefix}-old-ownerless-input.sav")
        authored.write_bytes(b"OTTX" + (374 << 16).to_bytes(4, "big") + lzma.compress(encoded))
        loaded = run_case("old-ownerless", ["-g", authored.as_posix()], "MLVS_PORTAL_WORK_LOADED pairs=1", prefix=prefix)
        restored = {record["name"]: record for record in chunks(read_stream(loaded)[1])}
        assert struct.unpack(">IBBIIIQ", restored[b"LPWK"]["rows"][1]) == (1300, 0, 255, 0xFFFFFFFF, 0xFFFFF, 0xFFFF, 1)
        original = {record["name"]: record for record in records}
        for label in (b"GEOS", b"GDEP", b"GSPN", b"GVNT"):
            assert original[label]["raw"] == restored[label]["raw"], label
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash
    (CONTEXT.reports / f"{prefix}-migration-results.txt").write_text(
        "@see r2mlvs3-sealed-portals-and-deep-rail.md, Group 16.\n"
        "Actual version-374 Native ownerless road migrated to one explicitly Sealed/unassigned record; all geological bytes unchanged.\n"
        "Original retained input/startup/config bytes preserved. No company or usable excavation/traffic authority was invented.\n", encoding="utf-8")


def deep_state(records, connected=False):
    """Prepare actual Native Underground/Deep load evidence with separate pair and geological owners.

    @param records Actual retained Native company save, containing no pre-existing portal pair.
    @param connected Supply explicit prepared destination foundations for the Connected load boundary.
    @return None; fixture floor authoring is not a TBM excavation or gameplay purchase claim.
    """
    source, target = 5396, 13588
    housing(records, source, target, False, 0, 1, 3)
    metadata = geology_chunk(records, b"MLMD")
    row = bytearray(metadata["rows"][1])
    row[31:35] = (int.from_bytes(row[31:35], "big") | 16).to_bytes(4, "big")
    metadata["rows"][1] = bytes(row)
    geology_chunk(records, b"LPWH")["payload"] = struct.pack(">II", 1, 1)
    work = geology_chunk(records, b"LPWK")
    work["rows"] = work["rows"][:1] + [struct.pack(">IBBIIIQ", source, 2 if connected else 0, 0, 0xFFFFFFFF, 0xFFFFF, 0xFFFF, 1)]
    terrain = geology_chunk(records, b"GEOS")
    payload = bytearray(terrain["payload"])
    floor = [1300, 1236]
    if connected:
        floor += [4096 + 1300, 4096 + 1364]
    # The packed owner stores Underground then Deep; only explicitly authored physical foundations open, independently of the map's plane addresses.
    for packed_cell in floor:
        shift, index = (packed_cell % 4) * 2, 29 + packed_cell // 4
        payload[index] = (payload[index] & ~(3 << shift)) | (2 << shift)
    terrain["payload"] = bytes(payload)


def resume_deep_roundtrips():
    """Exercise the new Deep rail topology in actual Sealed/Connected files without rerunning earlier families.

    @return None; retain source/config/startup bytes and exact portal/geological authority through repeated loads.
    """
    from functools import partial
    prefix = "r2mlvs3-x"
    source = save_path("r2mlvs3-e-native-company.sav")
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    startup, config = RUNTIME / "scripts/game_start.scr", RUNTIME / "grftest.cfg"
    original_script, original_config = startup.read_bytes(), config.read_bytes()
    try:
        for name, connected in (("deep-sealed", False), ("deep-connected", True)):
            authored = variant(source, name, partial(deep_state, connected=connected), prefix=prefix)
            loaded = run_case(name, ["-g", authored.as_posix()], "MLVS_PORTAL_WORK_LOADED pairs=1", prefix=prefix)
            repeated = run_case(name + "-repeat", ["-g", loaded.as_posix()], "MLVS_PORTAL_WORK_LOADED pairs=1", prefix=prefix)
            expected = {record["name"]: record["raw"] for record in chunks(read_stream(authored)[1])}
            # Both real loaders must retain the one actual Underground canonical key, Deep partner and independent packed/deposit/vent authority.
            for output in (loaded, repeated):
                restored = {record["name"]: record for record in chunks(read_stream(output)[1])}
                for label in (b"LPWH", b"LPWK", b"GEOS", b"GDEP", b"GSPN", b"GVNT"):
                    assert restored[label]["raw"] == expected[label], (name, label)
                assert restored[b"MAP5"]["payload"][5396] == 97
                assert restored[b"MAP5"]["payload"][13588] == 35
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash
    (CONTEXT.reports / f"{prefix}-deep-results.txt").write_text(
        "@see r2mlvs3-sealed-portals-and-deep-rail.md, Group 16.\n"
        "Four actual Deep rail loads passed: separate Sealed/Connected Underground/Deep pairs and each repeated reload.\n"
        "Canonical key/reciprocal Deep topology and LPWH/LPWK/GEOS/GDEP/GSPN/GVNT remain exact; retained source/config/startup unchanged.\n"
        "Explicit fixture foundations are not a TBM excavation or occupied vehicle scenario.\n", encoding="utf-8")


def resume_original_portals():
    """Verify genuine retained version-370 portal worlds and their current repeated reloads.

    @return None; preserve supplied original saves/startup/config and compare actual ordinary-edge diagnostics and complete endpoint fields.
    """
    prefix = "r2mlvs3-y"
    sources = (ROOT / "build/save/makinpr2.sav", ROOT / "build/save/makinpr2-connected.sav")
    hashes = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in sources}
    startup, config = RUNTIME / "scripts/game_start.scr", RUNTIME / "grftest.cfg"
    original_script, original_config = startup.read_bytes(), config.read_bytes()
    try:
        # Genuine supplied worlds each contain five usable rail pairs and four Deep road elevators; authored old road-tunnel coverage remains separately labelled.
        for name, source in zip(("original-portals", "original-connected"), sources):
            version, stream = read_stream(source)
            assert version == 370
            records = {record["name"]: record for record in chunks(stream)}
            endpoints = [index for index, tile in enumerate(records[b"MAPT"]["payload"]) if tile >> 4 == 11]
            assert len(endpoints) == 18
            expected = "MLVS_PORTAL_CONNECTIVITY rail=5/5 road=0/0 elevator=4/4 invalid=0"
            loaded = run_case(name, ["-g", source.as_posix()], expected, prefix=prefix)
            repeated = run_case(name + "-repeat", ["-g", loaded.as_posix()], expected, prefix=prefix)
            # Both actual loaders must retain every public portal field at each original endpoint; real tile dispatch must continue exposing all original ordinary edges.
            for output in (loaded, repeated):
                restored = {record["name"]: record for record in chunks(read_stream(output)[1])}
                for label, width in FIELDS:
                    for tile in endpoints:
                        begin = tile * width
                        assert restored[label]["payload"][begin:begin + width] == records[label]["payload"][begin:begin + width], (name, label, tile)
                assert len(restored[b"LPWK"]["rows"]) == 1
                assert int.from_bytes(restored[b"MLMD"]["rows"][1][2:3], "big") == 0
                assert "MLVS_GEOLOGY_LOADED generator=0 deposits=0 packed_bytes=0" in log_path(output.with_suffix(".log").name).read_text(encoding="utf-8", errors="replace")
        # Genuine originals have no road tunnel; the existing separately authored version-370 three-kind fixture proves that actual ordinary road dispatch also survives migration.
        authored_road = save_path("r2mlvs3-r-legacy-pairs-input.sav")
        authored_hash = hashlib.sha256(authored_road.read_bytes()).hexdigest()
        run_case("retained-road-topology", ["-g", authored_road.as_posix()], "MLVS_PORTAL_CONNECTIVITY rail=1/1 road=1/1 elevator=1/1 invalid=0", prefix=prefix)
        assert hashlib.sha256(authored_road.read_bytes()).hexdigest() == authored_hash
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
        for path, digest in hashes.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    (CONTEXT.reports / f"{prefix}-original-results.txt").write_text(
        "@see r2mlvs3-sealed-portals-and-deep-rail.md, Group 16 task 16.3.\n"
        "Five actual loads passed: unchanged genuine makinpr2/original-connected version-370 worlds, each current repeat, and retained separately authored road-tunnel topology.\n"
        "Actual runtime dispatch preserves rail=5/5 and elevator=4/4 usable ordinary pairs, invalid=0; eighteen original endpoint field sets remain exact.\n"
        "Classic migration allocates no work/geological authority and retains supplied original/startup/config bytes.\n"
        "Original road tunnels are absent from these worlds; separately authored version-370 road topology is retained as distinct coverage.\n", encoding="utf-8")


if __name__ == "__main__":
    if "--migration" in sys.argv:
        resume_ownerless_migration()
    elif "--deep" in sys.argv:
        resume_deep_roundtrips()
    elif "--original-portals" in sys.argv:
        resume_original_portals()
    else:
        main()
