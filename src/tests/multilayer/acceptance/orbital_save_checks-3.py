"""Admit explicit native legacy append-only defaults for orbital save acceptance.

@file orbital_save_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group14
The real original header controls normalization. Every removed append must equal
its native empty value; actual original payloads and source files stay unchanged.
"""
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream
from launch_center_checks import riff_location


def payload(raw, family):
    """Select one original RIFF body independently of its length prefix.

    @param raw Exact decompressed stream. @param family Actual four-byte family tag.
    @return Complete original body with no changes to a native save.
    """
    _, start, _, end = riff_location(raw, family)
    return raw[start:end]


def normalize_orders(raw):
    """Remove only absent approach fields appended after the original queues.

    @param raw Current native stream upgraded from a genuine pre420 file.
    @return Original old SPOR body, retaining all issued identities and instructions.
    """
    data = payload(raw, b"SPOR")
    position, result = 9, bytearray(data[:9])
    # The native count/entry extents determine every original row; a missing
    # old approach can gain only explicit absence, never a new berth entitlement.
    for _ in range(data[8]):
        size = 22 + 14 * data[position + 21]
        result.extend(data[position:position + size])
        position += size
        assert data[position] == 0
        position += 1
    assert position == len(data)
    return bytes(result)


def normalize_products(raw, version):
    """Remove only genuine absent factory/replacement provenance appended to old paid hulls.

    @param raw Actual current native stream.
    @param version Genuine pre429 source schema. @return Exact source-era SVST body.
    """
    assert 414 <= version < 429
    data = payload(raw, b"SVST")
    position, result = 1, bytearray(data[:1])
    # Existing Earth hull receipts retain all paid fields and original content
    # versions. Empty new provenance cannot commission or materialize a product.
    for _ in range(data[0]):
        size = 49 if version < 427 else 67
        result.extend(data[position:position + size])
        position += size
        if version < 427:
            assert data[position:position + 18] == b"\xff\xff" + bytes(16)
            position += 18
        assert data[position] == 0
        position += 1
    assert position == len(data)
    return bytes(result)


def accepted_engine_repair(before, after, configuration):
    """Reuse the exact real old-Core/current-inspection catalog repair boundary.

    @param before Original legacy chunk map.
    @param after Actual current reconstructed chunk map.
    @param configuration Matching paid processor producer configuration.
    @return Immutable original report/source/after identities; historical binaries are not replayed.
    """
    index = ROOT / "logs/orbital-processor-verification.json"
    proof = json.loads(index.read_text())
    row, = [row for row in proof["configurations"] if row["configuration"] == configuration]
    reference, = [ref for ref in row["reports"] if ref["path"].endswith("/orbital-processor-inspected.json")]
    assert identity(ROOT / reference["path"]) == reference
    report = json.loads((ROOT / reference["path"]).read_text())
    source, saved = [ROOT / report[key]["path"] for key in ("source", "after")]
    assert identity(source) == report["source"]
    assert identity(saved) == report["after"]
    assert before[b"ENGN"] == chunks(source)[1][b"ENGN"]
    assert after[b"ENGN"] == chunks(saved)[1][b"ENGN"]
    return dict(index=identity(index), report=reference, source=report["source"], after=report["after"])


def compare_upgrade(source, saved, configuration, physical, decoder):
    """Compare every original owner, accepting only named native schema/default migrations.

    @param source Genuine immutable legacy save path.
    @param saved Current ordinary reconstruction path.
    @param configuration Matching actual native build and canonical producer configuration.
    @param physical Complete thirty-family physical owner list.
    @param decoder Independent versioned SPST semantic decoder.
    @return Exact byte/semantic comparisons and explicit native migration evidence.
    """
    old_version, before = chunks(source)
    version, after = chunks(saved)
    assert version == 438
    raw, current = read_stream(source)[1], read_stream(saved)[1]
    assert decoder(raw, old_version)[0] == decoder(current, version)[0]
    exact, upgrades, engine = [], [], None
    # Each changed physical family has its own narrow source-era permission.
    # A general successful load, a current catalog or a discarded field cannot
    # authorize a change to any other paid packet, hull, queue or work owner.
    for family in physical:
        assert family in before and family in after
        if before[family] == after[family]:
            exact.append(family.decode())
            continue
        if family == b"SPST":
            assert old_version < 436
            upgrades.append("SPST: exact original semantics plus native empty defaults")
        elif family == b"ASTR":
            assert old_version < 424
            actual = payload(current, family)
            assert actual[-26:] == b"\x00" + b"\xff" * 9 + bytes(16)
            assert payload(raw, family) == actual[:-26]
            upgrades.append("ASTR: unknown empty starter history only")
        elif family == b"SPOR":
            assert old_version < 420
            assert payload(raw, family) == normalize_orders(current)
            upgrades.append("SPOR: absent approach reservations only")
        elif family == b"SVST":
            assert old_version < 429
            assert payload(raw, family) == normalize_products(current, old_version)
            upgrades.append("SVST: absent factory/replacement provenance only")
        elif family == b"SDEF":
            assert old_version < 431
            assert before[family][3:] == bytes.fromhex("0002a1608eff08bf795b")
            assert after[family][3:] == bytes.fromhex("000321b33aca30e28355")
            upgrades.append("SDEF: current catalog3; original paid provenance remains2")
        elif family == b"ENGN":
            assert old_version < 429
            engine = accepted_engine_repair(before, after, configuration)
            upgrades.append("ENGN: exact already-qualified old-Core catalog repair")
        else:
            raise AssertionError((source, family))
    for family in (b"CDIM", b"SPRC"):
        if family in before:
            assert before[family] == after[family]
            exact.append(family.decode())
        elif family == b"SPRC":
            parser = runpy.run_path(str(Path(__file__).with_name("orbital_processor_checks-3.py")))
            assert parser["records"](current) == {}
            upgrades.append("SPRC: disabled empty processing metadata")
        else:
            index = json.loads((ROOT / "logs/orbital-processor-verification.json").read_text())
            row, = [row for row in index["configurations"] if row["configuration"] == configuration]
            canonical = row["checkpoints"]["native/inspected"]
            assert identity(ROOT / canonical["path"]) == canonical
            assert after[family] == chunks(ROOT / canonical["path"])[1][family]
            upgrades.append("CDIM: exact accepted canonical configured dimensions")
    return dict(exact_families=exact, native_legacy_upgrades=upgrades, engine_repair=engine)
