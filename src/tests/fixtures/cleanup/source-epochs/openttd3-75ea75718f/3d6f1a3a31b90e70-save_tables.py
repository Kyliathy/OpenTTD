"""Read native named save tables without constructing or changing gameplay owners.

@file save_tables.py Pure bounded schema inspection for ordinary file acceptance.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FA.
The field decoder follows tbm_logistics_save_fixture.py's delivered wire reader,
without importing that command-line runner's runtime/output side effects.
"""

from pathlib import Path
import sys
import struct
from collections import Counter

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "mlm/tools/savegames"))
from savegame_plane_digest import Reader, read_stream, CHUNK_RIFF, CHUNK_RECORDS

WIDTHS = {1: 1, 2: 1, 3: 2, 4: 2, 5: 4, 6: 4, 7: 8, 8: 8, 9: 2}


def chunks(stream):
    """Read complete native chunks without importing a fixture runner.

    @param stream Actual decompressed native save bytes.
    @return Ordered original raw chunks, bounded RIFF payloads and table rows.
    """
    reader, result = Reader(stream), []
    # Preserve complete owners, including unknown ordinary fields. This pure
    # decoder never creates a runtime profile, output folder or fixture world.
    while True:
        start = reader.pos
        name = reader.take(4)
        if name == bytes(4):
            assert reader.pos == len(stream), "Trailing bytes after native save terminator."
            return result
        mode = reader.byte()
        kind, payload, rows = mode & 15, None, []
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
            raise ValueError("Unknown native chunk transport.")
        result.append({"name": name, "kind": kind, "raw": stream[start:reader.pos], "payload": payload, "rows": rows})


def geology_snapshot(path):
    """Read original geological metadata and every packed rock-cell state.

    @param path Genuine current native save; never an output mutation target.
    @return Immutable wire header and independent state counts per actual rock plane.
    """
    version, stream = read_stream(Path(path))
    assert version >= 399, "Regional geology inspection requires its actual wire header."
    payload = next(row["payload"] for row in chunks(stream) if row["name"] == b"GEOS")
    header = struct.unpack(">HH6IBHQ", payload[:39])
    wire, generator, width, height, next_id, packed_length, deposits, spans, edges, hardness, seed = header
    assert wire == 2 and generator > 0, "Only actual generated regional geology can prove solid baseline."
    plane_size = width * height
    assert packed_length == (2 * plane_size + 3) // 4 and len(payload) == 39 + packed_length
    packed = payload[39:]
    layers = {}
    # The geological owner packs Underground then Deep, four two-bit states
    # per byte. Count the whole plane so unrequested swept rims are covered too.
    for plane, name in enumerate(("underground", "deep")):
        counts = Counter((packed[index // 4] >> (2 * (index % 4))) & 3
                         for index in range(plane * plane_size, (plane + 1) * plane_size))
        layers[name] = {state: counts[number] for number, state in enumerate(("solid", "excavating", "excavated", "reserved"))}
    return {"header": payload[:39].hex(), "width": width, "height": height, "hardness_version": hardness,
            "hardness_seed": seed, "layers": layers}


def table_header(reader):
    """Decode named fields before their appended nested schemas.

    @param reader Bounded native header reader.
    @return Ordered field names, wire kinds and nested field descriptions.
    """
    fields = []
    # The wire stores the complete parent header before every child schema;
    # interleaving them would silently assign bytes to unrelated owners.
    while True:
        kind = reader.byte()
        if kind == 0:
            break
        fields.append([reader.take(reader.gamma()).decode("utf-8"), kind, None])
    for field in fields:
        if field[1] & 15 == 11:
            field[2] = table_header(reader)
    return fields


def table_record(reader, fields, prefix=""):
    """Decode every field using the file's own schema.

    @param reader Bounded actual row reader.
    @param fields Parsed native field schema.
    @param prefix Parent name used for precise nested differences.
    @return All scalar, array, string and nested values; no unknown bytes skip.
    """
    values = {}
    # Custody and invoice comparisons must include the entire object. Every
    # nested field remains named rather than becoming an opaque excluded blob.
    for name, kind, nested in fields:
        key, primitive = prefix + name, kind & 15
        if primitive == 11:
            count = reader.gamma()
            values[key + ".count"] = count
            for index in range(count):
                values.update(table_record(reader, nested, f"{key}[{index}]."))
        elif primitive == 10:
            values[key] = reader.take(reader.gamma()).hex()
        else:
            count = reader.gamma() if kind & 16 else 1
            width, signed = WIDTHS[primitive], primitive in (1, 3, 5, 7)
            items = [int.from_bytes(reader.take(width), "big", signed=signed) for _ in range(count)]
            values[key] = items if kind & 16 else items[0]
    return values


def saved_table(path, name):
    """Read complete current named table rows under their original pool IDs.

    @param path Genuine native original or ordinary application resave.
    @param name Exact four-byte table label.
    @return Sparse identity to complete decoded native field values.
    """
    version, stream = read_stream(path)
    assert version >= 375, "Named table inspection requires the native table format."
    chunk = next(row for row in chunks(stream) if row["name"] == name)
    assert chunk["kind"] in (3, 4), "Owner must be a current named table."
    header = Reader(chunk["rows"][0])
    fields = table_header(header)
    assert header.pos == len(header.data), "Native table header was not fully decoded."
    result = {}
    # Sparse tables encode the actual pool ID; dense tables use the wire row
    # position. Neither form can quietly renumber a vehicle or company.
    for index, row in enumerate(chunk["rows"][1:]):
        # A dense native pool retains empty ordinal slots after deletion.
        # Skipping the absent object preserves later IDs; sparse tables must
        # still carry an explicit identity and cannot use this representation.
        if not row:
            assert chunk["kind"] == 3, "An empty sparse native row has no identity."
            continue
        reader = Reader(row)
        identity = reader.gamma() if chunk["kind"] == 4 else index
        result[identity] = table_record(reader, fields)
        assert reader.pos == len(row), (name, identity, "Native row was not fully decoded.")
    return result
