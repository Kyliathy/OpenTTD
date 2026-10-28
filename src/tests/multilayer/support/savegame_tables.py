"""Read-only native save-table transport; importing creates no runtime context.

@file savegame_tables.py Sole owner of the existing logistics/geology decoder primitives.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group8.0.
@see docs/zz-reach2/architecture/savegame/archi-savegame.md.
Native fixture entry points re-export this one decoder. Saved named schemas,
not this helper, own gameplay field meanings. Plan4 Group56 binds complete
original/merged ledgers without excluding clocks, ordinary owners or custody.
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

    @param stream Actual decompressed original or merged save bytes.
    @return Ordered dictionaries; unknown ordinary chunks retain their original exact transport.
    """
    reader = Reader(stream)
    result = []
    # Preserve unrelated descriptors byte-for-byte; only targeted geological authority is changed by the caller.
    while True:
        start = reader.pos
        name = reader.take(4)
        if name == bytes(4):
            if reader.pos != len(stream):
                raise ValueError("trailing bytes after native save terminator")
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


def table_header(reader, depth=0):
    """Decode the writer's named fields and recursively appended struct headers.

    @param reader Bounded actual table header reader.
    @param depth Actual nested schema depth; bounded before recursion to refuse malformed headers.
    @return Ordered field descriptors, including the actual nested schema.
    """
    if depth > 64:
        raise ValueError("native table nesting exceeds the bounded reader")
    fields, names = [], set()
    # Every nested schema follows the complete parent header, as in SlTableHeader;
    # interpreting nesting inline would assign unrelated bytes to vehicle fields.
    while True:
        kind = reader.byte()
        if kind == 0:
            break
        name = reader.take(reader.gamma()).decode("utf-8")
        # A repeated/unknown schema field could overwrite custody or clock
        # observations while leaving a partial diagnostic apparently equal.
        if name in names or not 1 <= (kind & 15) <= 11 or kind & ~31:
            raise ValueError("duplicate or unsupported native table field")
        names.add(name)
        fields.append([name, kind, None])
    for field in fields:
        if field[1] & 15 == 11:
            field[2] = table_header(reader, depth + 1)
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
            # Nonempty objects consume at least one encoded byte. Empty
            # historical schemas retain only their count, avoiding an unbounded
            # no-op walk without dropping any saved field.
            if nested:
                if count > len(reader.data) - reader.pos:
                    raise ValueError("nested table count exceeds the bounded row")
                for index in range(count):
                    values.update(table_record(reader, nested, f"{key}[{index}]."))
        elif primitive == 10:
            values[key] = reader.take(reader.gamma()).hex()
        else:
            count = reader.gamma() if kind & 16 else 1
            width = WIDTHS[primitive]
            signed = primitive in (1, 3, 5, 7)
            if count > (len(reader.data) - reader.pos) // width:
                raise ValueError("native numeric array exceeds the bounded row")
            items = [int.from_bytes(reader.take(width), "big", signed=signed) for _ in range(count)]
            values[key] = items if kind & 16 else items[0]
    return values


def saved_table(path, name):
    """Decode an actual current-format table, retaining its sparse object identities.

    @param path Real command-driven source or game-loader output.
    @param name Four-byte owner chunk label.
    @return Object identity to fully decoded fields, without changing any input.
    """
    return saved_owner_table(path, name)


def decode_table(chunk):
    """Decode a complete named table once, preserving dense holes and sparse identity.

    @param chunk Already framed native owner with original name/kind/rows.
    @return Original pool IDs mapped to every named/nested field; no live lookup or normalization.
    """
    # Missing/wrong framing cannot turn an absent header into an empty accepted
    # owner; every named pool must carry its own original schema first.
    if chunk["kind"] not in (3, 4) or not chunk["rows"]:
        raise ValueError("named owner requires a TABLE header")
    header = Reader(chunk["rows"][0])
    fields = table_header(header)
    if header.pos != len(header.data):
        raise ValueError("native table header was not fully decoded")
    result = {}
    # A retired dense slot still consumes its original ordinal. Sparse tables
    # must carry an explicit unique ID and cannot disguise absence as a hole.
    for index, row in enumerate(chunk["rows"][1:]):
        if not row:
            if chunk["kind"] != 3:
                raise ValueError("empty sparse row has no original identity")
            continue
        reader = Reader(row)
        owner = reader.gamma() if chunk["kind"] == 4 else index
        if owner in result:
            raise ValueError("duplicate native sparse owner identity")
        result[owner] = table_record(reader, fields)
        if reader.pos != len(row):
            # SaveReal_AIPL/SaveReal_GSDT append the script VM's own wire data
            # after their named header fields (ai_sl.cpp/game_sl.cpp). Preserve
            # every such byte explicitly; no other owner may hide an unknown tail.
            if chunk["name"] not in (b"AIPL", b"GSDT"):
                raise ValueError("native table row was not fully decoded: " + chunk["name"].decode("ascii"))
            if "$native_script_tail" in result[owner]:
                raise ValueError("script schema collides with explicit opaque native tail")
            result[owner]["$native_script_tail"] = reader.take(len(row) - reader.pos).hex()
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


MERGED_CONTRACTS = {441: "src/tests/fixtures/saveload/merged-family-set.json",
                    446: "src/tests/fixtures/saveload/merged-446-family-set.json"}


def merged_contract(version):
    """Load the independently specified family contract of one merged writer version.

    @param version Actual envelope version of a merged save.
    @return Contract with its complete family map; a contract with a base takes the base's families (m2o28 Group 22).
    """
    import json
    if version not in MERGED_CONTRACTS:
        raise ValueError("no merged family contract for this version")
    contract = json.loads((ROOT / MERGED_CONTRACTS[version]).read_text(encoding="utf-8"))
    assert contract["schema"] == 1 and contract["merged_version"] == version
    if "base" in contract:
        base = json.loads((ROOT / contract["base"]).read_text(encoding="utf-8"))
        assert base["schema"] == 1 and "base" not in base
        contract = dict(contract, families=base["families"])
    return contract


def flat_table_fields(header, prefix=""):
    """Name every field of a table header, nested ones by their dotted path.

    @param header Parsed descriptors from table_header.
    @param prefix Parent path.
    @return Pairs of full path and saved field kind.
    """
    result = []
    for name, kind, nested in header:
        result.append((prefix + name, kind))
        if nested:
            result.extend(flat_table_fields(nested, prefix + name + "."))
    return result


def merged_world_chunks(path):
    """Read the complete independently specified merged family set, including empty and ordinary owners.

    @param path Actual new merged save (441 or 446); original A/B files retain their own separate readers.
    @return Unique name to exact raw transport, requiring every original-derived family and its exact framing kind.
    Counts121/126remain dated original observations. This compares explicit names
    and kinds against the fixture derived from unchanged originals plus allocated
    marker/tagged-wire decisions, never a list generated from a current output.
    No whole owner (including VIEW, GLOG, RNG, packets or accounts) is excluded.
    """
    version, stream = read_stream(path)
    contract = merged_contract(version)
    rows = chunks(stream)
    owners = {row["name"].decode("ascii"): row for row in rows}
    assert len(owners) == len(rows), "duplicate merged owner"
    assert owners.keys() == contract["families"].keys(), "incomplete or unknown merged family set"
    # Exact original framing remains part of each owner's contract. An empty
    # replacement TABLEcannot stand in for a required RIFF or missing paid history.
    for name, kind in contract["families"].items():
        assert owners[name]["kind"] == kind, (name, "changed merged framing")
    marker = contract.get("marker")
    if marker:
        expected = marker["magic"].encode("ascii") + marker["schema"].to_bytes(2, "big") + marker["writer"].to_bytes(2, "big")
        assert owners[marker["family"]]["payload"] == expected, "changed merged lineage marker"
    # Each declared column exists exactly once, at any nesting depth, with its declared kind.
    for name, columns in contract.get("table_columns", {}).items():
        fields = flat_table_fields(table_header(Reader(owners[name]["rows"][0])))
        for column, kind in columns.items():
            found = [field_kind for path, field_kind in fields if path == column or path.endswith("." + column)]
            assert found == [kind], (name, column, found, "changed merged column")
    return {name.encode("ascii"): row["raw"] for name, row in owners.items()}


def compare_merged_worlds(first, second):
    """Require exact bytes for every independently required merged owner after native reconstruction.

    @param first Actual first441successor; cannot define the expected family set.
    @param second Actual second save of the same admitted paused world.
    @return Explicit compared names; no field or whole family is normalized or excluded.
    """
    before, after = merged_world_chunks(first), merged_world_chunks(second)
    changed = [name.decode("ascii") for name in before if before[name] != after[name]]
    assert not changed, ("merged owner bytes changed", changed)
    return sorted(name.decode("ascii") for name in before)


def first_tick_worlds(prefix):
    """Compare the actual first native ticks from an original and its merged reload, without normalizing any owner.

    @param prefix Invocation-owned native output prefix; no retained original is written.
    @return Exact before/direct/reloaded paths and all independently required compared names, for the calling producer's identity receipt.
    """
    before = Path(str(prefix) + ".tick-before.sav")
    direct = Path(str(prefix) + ".tick-direct.sav")
    reloaded = Path(str(prefix) + ".tick-reloaded.sav")
    merged_world_chunks(before)
    return dict(before=before, direct=direct, reloaded=reloaded, families=compare_merged_worlds(direct, reloaded))


def saved_owner_table(path, name):
    """Decode current live owners with the original freight reader's dense holes.

    @param path Complete native save; no bytes or runtime files are changed.
    @param name Four-byte physical owner table identifier.
    @return Live rows keyed by their native pool identity.
    """
    version, stream = read_stream(path)
    assert version >= 375
    chunk, = [row for row in chunks(stream) if row["name"] == name]
    return decode_table(chunk)


def complete_world(path):
    """Observe every actual original/merged owner and complete named tables without gameplay side effects.

    @param path Actual immutable original or invocation-owned native successor.
    @return Version, exact raw owner map and fully decoded TABLE map; no owner omissions or normalization.
    """
    import json
    version, stream = read_stream(path)
    rows = chunks(stream)
    owners = {row["name"]: row for row in rows}
    if len(owners) != len(rows):
        raise ValueError("duplicate complete-world owner")
    if version in MERGED_CONTRACTS:
        contract = merged_contract(version)
        expected = {name.encode("ascii"): kind for name, kind in contract["families"].items()}
        # The independent original-derived family set, not a current output's
        # count, defines merged completeness and framing, including empty owners.
        if contract["merged_version"] != version or {name: row["kind"] for name, row in owners.items()} != expected:
            raise ValueError("incomplete or changed merged family set/framing")
    tables = {name: decode_table(row) for name, row in owners.items() if row["kind"] in (3, 4)}
    return version, {name: row["raw"] for name, row in owners.items()}, tables
