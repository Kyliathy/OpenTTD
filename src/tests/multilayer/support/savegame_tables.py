"""Read-only native save-table transport; importing creates no runtime context.

@file savegame_tables.py Sole owner of the existing logistics/geology decoder primitives.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group8.0.
@see docs/zz-reach2/architecture/savegame/archi-savegame.md.
Functions are moved without executable AST changes; native fixture entry points
re-export them. Saved named schemas, not this helper, own gameplay field meanings.
"""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "mlm/tools/savegames"))
from savegame_plane_digest import Reader, read_stream, CHUNK_RIFF, CHUNK_RECORDS

# Primitive widths decode the original named schema; they add no gameplay fields.
WIDTHS = {1: 1, 2: 1, 3: 2, 4: 2, 5: 4, 6: 4, 7: 8, 8: 8, 9: 2}


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


def table_header(reader):
    """Decode the writer's named fields and recursively appended struct headers.

    @param reader Bounded actual table header reader.
    @return Ordered field descriptors, including the actual nested schema.
    """
    fields = []
    # Every nested schema follows the complete parent header, as in SlTableHeader;
    # interpreting nesting inline would assign unrelated bytes to vehicle fields.
    while True:
        kind = reader.byte()
        if kind == 0:
            break
        name = reader.take(reader.gamma()).decode("utf-8")
        fields.append([name, kind, None])
    for field in fields:
        if field[1] & 15 == 11:
            field[2] = table_header(reader)
    return fields


def table_record(reader, fields, prefix=""):
    """Read one real table object using its saved schema and field lengths.

    @param reader Bounded actual row reader.
    @param fields Actual parsed descriptors; no private gameplay definitions.
    @param prefix Parent field path for precise difference reporting.
    @return Named scalar/array values, including all nested object contents.
    """
    values = {}
    # Cargo references, orders and physical state must be decoded together;
    # skipping an unknown byte would invalidate every later custody comparison.
    for name, kind, nested in fields:
        key = prefix + name
        primitive = kind & 15
        if primitive == 11:
            count = reader.gamma()
            values[key + ".count"] = count
            for index in range(count):
                values.update(table_record(reader, nested, f"{key}[{index}]."))
        elif primitive == 10:
            values[key] = reader.take(reader.gamma()).hex()
        else:
            count = reader.gamma() if kind & 16 else 1
            width = WIDTHS[primitive]
            signed = primitive in (1, 3, 5, 7)
            items = [int.from_bytes(reader.take(width), "big", signed=signed) for _ in range(count)]
            values[key] = items if kind & 16 else items[0]
    return values


def saved_table(path, name):
    """Decode an actual current-format table, retaining its sparse object identities.

    @param path Real command-driven source or game-loader output.
    @param name Four-byte owner chunk label.
    @return Object identity to fully decoded fields, without changing any input.
    """
    version, stream = read_stream(path)
    assert version >= 375, "named table inspection requires the delivered native portal table format"
    chunk = next(chunk for chunk in chunks(stream) if chunk["name"] == name)
    assert chunk["kind"] in (3, 4), "owner must be a current table"
    header_reader = Reader(chunk["rows"][0])
    fields = table_header(header_reader)
    assert header_reader.pos == len(header_reader.data), "table header not fully decoded"
    result = {}
    # Sparse tables carry their real pool IDs in each row; dense tables use row order.
    for index, row in enumerate(chunk["rows"][1:]):
        reader = Reader(row)
        identity = reader.gamma() if chunk["kind"] == 4 else index
        result[identity] = table_record(reader, fields)
        assert reader.pos == len(row), (name, identity, "row not fully decoded")
    return result


def owned_chunks(path):
    """Read complete vehicle, packet, project, receipt and company custody descriptors.

    @param path Actual normal-game output to compare at an atomic exchange boundary.
    @return Exact authoritative owner bytes; unknown unrelated chunks remain untouched.
    """
    version, stream = read_stream(path)
    names = {b"EIDS", b"VEHS", b"ORDL", b"CAPA", b"TBMH", b"TBMP", b"TBMS",
             b"TBXH", b"TBXA", b"TBXS", b"TBXW", b"INDY", b"STNN", b"PLYR",
             b"EXJS", b"XJOB", b"XJRT", b"XJTL", b"XPAS", b"GEOS", b"GDEP",
             b"GSPN", b"GVNT", b"SPRH", b"SPRR", b"LPWH", b"LPWK", b"DATE",
             b"ECMY", b"SRND", b"MAPS", b"MAPT", b"MAPH", b"MAPO", b"MAP2", b"M3LO", b"M3HI", b"MAP5", b"MAPE", b"MAP7", b"MAP8"}
    families = ({b"UPLH", b"UPLD"}, {b"LMNH", b"LMNR"}, {b"LPCH", b"LPCR", b"LPCC"})
    optional = set().union(*families)
    owners = {chunk["name"]: chunk["raw"] for chunk in chunks(stream) if chunk["name"] in names | optional}
    assert names.issubset(owners), "real loaded logistics owner family incomplete"
    # Every delivered optional ground family remains complete when present;
    # a missing processor/mine/utility table cannot hide behind a base-stock hash.
    for family in families:
        present = family & owners.keys()
        assert not present or present == family, "partial optional ground owner family"
    return version, owners


def saved_owner_table(path, name):
    """Decode current live owners with the original freight reader's dense holes.

    @param path Complete native save; no bytes or runtime files are changed.
    @param name Four-byte physical owner table identifier.
    @return Live rows keyed by their native pool identity.
    """
    version, stream = read_stream(path)
    assert version >= 375
    chunk, = [row for row in chunks(stream) if row["name"] == name]
    assert chunk["kind"] in (3, 4)
    header = Reader(chunk["rows"][0])
    fields = table_header(header)
    assert header.pos == len(header.data)
    result = {}
    # Retired dense-pool entries still consume a native identity. Skipping
    # their empty payload must preserve the identities of all surviving owners.
    for index, row in enumerate(chunk["rows"][1:]):
        if not row:
            assert chunk["kind"] == 3
            continue
        reader = Reader(row)
        owner = reader.gamma() if chunk["kind"] == 4 else index
        result[owner] = table_record(reader, fields)
        assert reader.pos == len(row)
    return result
