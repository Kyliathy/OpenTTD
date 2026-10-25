"""Independent original-to-441receipt comparison for native migration producers.

@file merged_supply_receipts.py
@see docs/zz-reach2/architecture/savegame/archi-savegame.md
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md
This reader uses the retained-save framing helper and explicit frozen struct
formats. It does not call the native codec or generate expected receipt bytes.
"""
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from savegame_tables import Reader, read_stream, chunks, decode_table, table_header


def selected_chunks(path):
    """Read selected actual chunk extents from one original or new successor.

    @param path Immutable retained input or invocation-owned generated successor.
    @return Original envelope version and selected RIFFpayloads/TABLErows with actual kind nibbles.
    """
    version, decoded = read_stream(path)
    result = {}
    # Follow disjoint original extents; owner-looking bytes inside another
    # payload cannot provide a marker, receipt, DATEor capability declaration.
    for chunk in chunks(decoded):
        name, kind = chunk["name"], chunk["kind"]
        content = chunk["payload"] if kind == 0 else chunk["rows"]
        if name in (b"MLLN", b"SSOR", b"SSOH", b"DATE", b"MLMD"):
            assert name not in result
            result[name] = (kind, content)
    return version, result


def flat_table(chunk):
    """Decode actual bounded numeric TABLEfields, numeric arrays and opaque session IDs.

    @param chunk Actual kind and rows; only contiguous TABLE3and its explicit numeric schema are supported.
    @return Named original scalar/array rows, without cargo, company or live-world lookups.
    """
    kind, rows = chunk
    assert kind == 3 and rows
    header = Reader(rows[0])
    columns = table_header(header)
    for name, field_type, nested in columns:
        # This receipt view intentionally admits numeric/session fields only;
        # the shared complete-table reader owns nested transport for all owners.
        assert 1 <= (field_type & 15) <= 8 or field_type == 26
    assert header.pos == len(rows[0])
    result = decode_table(dict(name=b"SSOR", kind=kind, rows=rows))
    # The receipt API historically exposes opaque session bytes. Preserve that
    # public shape while all actual scalar/array/cursor parsing has one owner.
    for values in result.values():
        for name, field_type, nested in columns:
            if field_type == 26:
                values[name] = bytes.fromhex(values[name])
    return list(result.values())


def native_receipt(purchaser, x, y, cost, inflation, year, buyer, state, packet):
    """Name explicit original A/current Native variant facts without deriving a tariff.

    @param purchaser Original payer byte.
    @param x Original Surface-local column.
    @param y Original Surface-local row.
    @param cost Frozen complete native charge.
    @param inflation Original Q16price factor.
    @param year Original purchase calendar year.
    @param buyer Original packet provenance IDor unavailable sentinel.
    @param state Original Pending/Fulfilled/Cancelled/Lost discriminant.
    @param packet Original issuance history, never a live stock lookup.
    @return Named Native evidence for exact independent comparison.
    """
    return dict(kind="Native", purchaser=purchaser, x=x, y=y, cost=cost, inflation=inflation,
                year=year, buyer=buyer, state=state, packet=packet)


def legacy_receipt(price, calendar, refunded, dispatched, closed):
    """Name only facts that original Bactually saved.

    @param price Original positive unit tariff.
    @param calendar Original calendar date, separate from cap day.
    @param refunded Already removed/refunded units.
    @param dispatched Irreversible first-departure history.
    @param closed Literal original0/1closed flag.
    @return Exact LegacyBevidence with structurally unavailable Native facts omitted.
    """
    assert closed in (0, 1)
    return dict(kind="LegacyB", price=price, calendar=calendar, refunded=refunded, dispatched=dispatched, closed=bool(closed))


def original_book(path, origin):
    """Read actual original A440or B426–434receipt evidence independently of the new writer.

    @param path Immutable retained original, never a transformed current file.
    @param origin Explicit catalogue A/Bselection; this is fixture interpretation, never runtime classifier evidence.
    @return Original version and normalized exact evidence dictionary for comparison with a new successor.
    """
    version, chunks = selected_chunks(path)
    rows = []
    if origin == "A":
        assert version == 440 and chunks[b"SSOR"][0] == 0
        payload = chunks[b"SSOR"][1]
        next_id, day, count = struct.unpack_from("<QiH", payload)
        record = struct.Struct("<QBBHHH4sHqQiiHBI")
        assert record.size == 53 and len(payload) == 14 + count * record.size
        # Original fixed field order is intentionally separate from tagged441.
        # Every actual payer/price/coordinate/state/issuance fact must survive.
        for index in range(count):
            identity, payer, owner, station, x, y, label, quantity, cost, inflation, year, cap_day, buyer, state, packet = record.unpack_from(payload, 14 + index * record.size)
            rows.append(dict(id=identity, owner=owner, station=station, label=label, quantity=quantity, day=cap_day,
                             receipt=native_receipt(payer, x, y, cost, inflation, year, buyer, state, packet)))
    else:
        # The original B426–434 dialect retained this exact named schema. The
        # caller explicitly selects original evidence, never runtime admission
        # based only on a numeric version or this fixture interpretation.
        assert origin == "B" and 426 <= version <= 434 and chunks[b"SSOH"][0] == 0
        schema, count, next_id = struct.unpack(">III", chunks[b"SSOH"][1])
        assert schema == 1 and next_id == count + 1
        day = flat_table(chunks[b"DATE"])[0]["economy_date"]
        table = flat_table(chunks[b"SSOR"])
        assert len(table) == count
        # B's original rows retain tariff/counters/date and unknown payer;
        # no current Native facts or inflation are reconstructed for them.
        # Its historical U32label column contains the original packed little-endian
        # label word;441instead emits the logical four label bytes explicitly.
        for row in table:
            rows.append(dict(id=row["id"], owner=row["owner"], station=row["station"], label=row["label"].to_bytes(4, "little"),
                             quantity=row["quantity"], day=row["economy_day"],
                             receipt=legacy_receipt(row["unit_price"], row["calendar_date"], row["refunded"], row["dispatched"], row["closed"])))
    return version, dict(next_id=next_id, day=day, rows=rows)


def merged_book(path):
    """Read the explicit new marker and tagged receipt contract independently.

    @param path New invocation-owned441successor or its second native save.
    @return Original441version, normalized exact book and actual raw SSORpayload for stable second-save comparison.
    """
    version, chunks = selected_chunks(path)
    assert version == 441 and chunks[b"MLLN"][0] == 0 and b"SSOH" not in chunks
    assert struct.unpack(">IHH", chunks[b"MLLN"][1]) == (0x4D4C4E31, 1, 441)
    kind, payload = chunks[b"SSOR"]
    assert kind == 0
    magic, schema, next_id, day, count = struct.unpack_from(">IHQiH", payload)
    assert magic == 0x53534D31 and schema == 1
    common, native, legacy = struct.Struct(">QBH4sHi"), struct.Struct(">BHHqQiHBI"), struct.Struct(">qiHHB")
    cursor, rows = 20, []
    # Each original tag chooses only its own frozen fields. Unavailable B
    # facts remain absent, and Native receipts keep their original payer/charge.
    for unused in range(count):
        tag = payload[cursor]
        cursor += 1
        identity, owner, station, label, quantity, cap_day = common.unpack_from(payload, cursor)
        cursor += common.size
        if tag == 1:
            receipt = native_receipt(*native.unpack_from(payload, cursor))
            cursor += native.size
        else:
            assert tag == 2
            receipt = legacy_receipt(*legacy.unpack_from(payload, cursor))
            cursor += legacy.size
        rows.append(dict(id=identity, owner=owner, station=station, label=label, quantity=quantity, day=cap_day, receipt=receipt))
    assert cursor == len(payload)
    return version, dict(next_id=next_id, day=day, rows=rows), payload


def migration_receipt(source, target, second, origin):
    """Compare an unchanged original against two actual merged native saves.

    @param source Immutable original A440or B426file.
    @param target First new merged successor, after actual native owner adoption.
    @param second Second save after complete merged native reconstruction.
    @param origin Explicit original catalogue interpretation for this fixture only.
    @return Compact evidence roles; caller binds actual paths/hashes/current executable separately.
    """
    old_version, original = original_book(source, origin)
    new_version, current, payload = merged_book(target)
    second_version, repeated, second_payload = merged_book(second)
    assert current == original, (current, original)
    assert repeated == current and second_payload == payload
    return dict(original_dialect=origin, original_version=old_version, successor_version=new_version,
                second_version=second_version, receipt_count=len(current["rows"]),
                receipt_tags=sorted({row["receipt"]["kind"] for row in current["rows"]}),
                exact_original_evidence=True, second_save_supply_bytes_equal=True)
