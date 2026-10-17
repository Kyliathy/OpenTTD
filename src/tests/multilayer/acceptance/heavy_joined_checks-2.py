"""Damage invocation-owned native copies to test ordinary/Space ownership and configured mass joins.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CD
Native self-describing table extents select actual station/carrier references. Malformed copies grant no gameplay asset and never replace original accepted inputs.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, Reader, CHUNK_RIFF, CHUNK_RECORDS, read_stream, identity
from launch_center_checks import riff_location
from runner import current_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_joined_checks-1.py")))
PACKETS = runpy.run_path(str(Path(__file__).with_name("heavy_descent_checks-2.py")))
GROUPS = {"owners": ("station-service", "ordinary-service", "station-descent", "ordinary-descent"),
          "dimensions": ("coherent-mass", "configured-mass", "missing-dimensions", "duplicate-dimensions"),
          "handling": ("station-handling", "ordinary-handling")}
CASES = {**{k: "dock-heavy" for k in GROUPS["owners"][:2]}, **{k: "descent" for k in (*GROUPS["owners"][2:], *GROUPS["dimensions"])},
         **{k: "handling" for k in GROUPS["handling"]}}


def gamma(value):
    """Encode an admitted original native table count or extent.

    @param value Nonnegative bounded count/extent for a deliberately changed copied row.
    @return The native simple-gamma bytes, retaining complete container framing.
    """
    assert 0 <= value < 2**32
    # Each leading prefix announces one more native byte; this framing permits insertion into an actual empty carrier list without guessing a fixed row size.
    for width, bits, prefix in ((1, 7, 0), (2, 14, 0x80), (3, 21, 0xC0), (4, 28, 0xE0)):
        if value < 1 << bits:
            result = bytearray(value.to_bytes(width, "big")); result[0] |= prefix; return bytes(result)
    return b"\xf0" + value.to_bytes(4, "big")


def table_rows(raw, selected):
    """Decode actual nested native field extents without allocating gameplay objects.

    @param raw Exact decompressed native stream.
    @param selected Original four-byte native table identity.
    @return Actual row bounds and all scalar/list offsets, preserving repeated bucket occurrences.
    """
    def header(reader, depth=0):
        """Read one bounded native schema.

        @param reader Original exact header extent.
        @param depth Bounded nested schema depth.
        @return Ordered primitive/name/child descriptions.
        """
        assert depth < 16; fields = []
        # Native child descriptions follow the complete parent list; names inside string/cargo payloads cannot impersonate schema fields.
        while True:
            tag = reader.byte()
            if tag == 0:
                break
            size = reader.gamma(); assert size < 4096 and len(fields) < 1024; fields.append([tag, reader.take(size).decode("utf-8"), None])
        for field in fields:
            if field[0] & 15 == 11:
                field[2] = header(reader, depth + 1)
        return fields

    def walk(reader, fields, absolute, prefix=""):
        """Walk one actual row using its own counts and schema.

        @param reader Exact original row extent.
        @param fields Actual admitted native schema.
        @param absolute Original stream offset of this row.
        @param prefix Original ancestor path including repeated bucket ordinals.
        @return Flat scalar/list metadata with exact count and payload bounds.
        """
        result = []
        # Every nested occurrence contributes its original extents; repeated goods or next-hop buckets cannot overwrite an earlier packet-reference list.
        for tag, name, children in fields:
            path = prefix + name; primitive = tag & 15
            if primitive == 11:
                count = reader.gamma(); assert count <= 65536
                for index in range(count):
                    result.extend(walk(reader, children, absolute, path + "[" + str(index) + "]."))
            elif primitive == 10:
                reader.take(reader.gamma())
            else:
                width = {1: 1, 2: 1, 3: 2, 4: 2, 5: 4, 6: 4, 7: 8, 8: 8, 9: 2}[primitive]
                count_start = absolute + reader.pos; count = reader.gamma() if tag & 16 else 1; position = absolute + reader.pos
                reader.take(width * count); result.append(dict(path=path, start=count_start, position=position, width=width, count=count, end=absolute + reader.pos))
        return result

    reader = Reader(raw)
    # Inspect native top-level framing in order. Only the exact requested chunk supplies a schema and sparse row identity.
    while True:
        family_start = reader.pos; family = reader.take(4); assert family != bytes(4); mode = reader.byte(); kind = mode & 15
        if kind == CHUNK_RIFF:
            reader.take(((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()); continue
        assert kind in CHUNK_RECORDS
        if family != selected:
            while True:
                extent = reader.gamma()
                if extent == 0:
                    break
                reader.take(extent - 1)
            continue
        assert kind in (3, 4); extent = reader.gamma(); description = Reader(reader.take(extent - 1)); fields = header(description); assert description.pos == len(description.data); rows = []; ordinal = 0
        while True:
            start = reader.pos; extent = reader.gamma()
            if extent == 0:
                return rows
            absolute = reader.pos; row = Reader(reader.take(extent - 1)); index = row.gamma() if kind == 4 else ordinal; ordinal += 1
            if kind == 3 and extent == 1:
                continue  # Native dense-table holes have no fields or container to mutate.
            found = walk(row, fields, absolute); assert row.pos == len(row.data)
            rows.append(dict(index=index, start=start, payload=absolute, end=reader.pos, chunk_start=family_start, fields=found))


def alias_native(raw, family, packet):
    """Place an already Space-owned packet into an actual ordinary native container in a malformed copy.

    @param raw Original decompressed current437 input.
    @param family Actual Station or Vehicle table.
    @param packet Existing raw physical packet identity, already owned by a paid job.
    @return Reframed malformed stream changing only one native packet-reference list.
    """
    candidates = []; quantity_offset = PACKETS["packet_count_offset"](raw, packet); quantity = int.from_bytes(raw[quantity_offset:quantity_offset + 2], "big"); assert quantity > 0
    # Real station buckets normally have stock; ordinary carriers may legitimately be empty. Inserting a reference changes only a malformed copy, never a valid-world packet owner.
    for row in table_rows(raw, family):
        capacity = next((int.from_bytes(raw[f["position"]:f["end"]], "big") for f in row["fields"] if f["path"].endswith(".cargo_cap")), 0)
        for field in row["fields"]:
            # The malformed ordinary owner is a real empty freight hold with enough original capacity, avoiding an unrelated over-capacity/cached-action failure before the ownership join.
            ordinary = family == b"VEHS" and field["path"].endswith(".cargo.packets") and not field["path"].startswith("space[") and field["count"] == 0 and capacity >= quantity
            station = family == b"STNN" and field["path"].endswith(".second")
            if ordinary or station:
                assert field["width"] == 4; candidates.append((row, field))
    assert candidates, family; row, field = next(((r, f) for r, f in candidates if f["count"]), candidates[0]); reference = (packet + 1).to_bytes(4, "big")
    if field["count"]:
        position = field["position"]; assert raw[position:position + 4] != reference; return raw[:position] + reference + raw[position + 4:]
    # Empty real native holds need their packet list, existing Keep action count and sparse row extent reframed together. This preserves native cache consistency while isolating the single duplicate physical owner; no extra vehicle, cargo type or feature job is fabricated.
    counts = next(f for f in row["fields"] if f["path"].endswith(".cargo.action_counts")); assert counts["width"] == 4 and counts["count"] == 4 and raw[counts["position"]:counts["end"]] == bytes(16)
    consistent = bytearray(raw); keep = counts["position"] + 8; consistent[keep:keep + 4] = quantity.to_bytes(4, "big")
    payload = consistent[row["payload"]:field["start"]] + gamma(1) + reference + consistent[field["end"]:row["end"]]
    return raw[:row["start"]] + gamma(len(payload) + 1) + payload + raw[row["end"]:]


def damage(context, source, kind):
    """Break one joined invariant in a fresh invocation-owned copied save.

    @param context Explicit malformed-artifact owner.
    @param source Exact accepted current437 heavy or handling checkpoint.
    @param kind Independent native alias or configured-dimensions fault.
    @return New malformed native save; all accepted source identities remain untouched.
    """
    version, raw = read_stream(source); assert version == 437; changed = bytearray(raw)
    if kind in GROUPS["owners"] or kind in GROUPS["handling"]:
        if kind.endswith("service"):
            # 437 retains the exact accepted436 SPST wire layout; only the independent CDIM metadata family is new.
            dock = SHARED["LAYOUT"]["structure_rows"](raw, 436)["rows"][4]; job = dock["jobs"][0]; offset = job["parts"][0]["packet_offsets"][0]
        elif kind.endswith("descent"):
            _, body, _, _ = riff_location(raw, b"SPDJ"); assert int.from_bytes(raw[body + 8:body + 10], "big") > 0; offset = body + 10 + 62
        else:
            _, body, _, _ = riff_location(raw, b"LHND"); assert int.from_bytes(raw[body + 8:body + 10], "big") > 0; offset = body + 11 + 35
        packet = int.from_bytes(raw[offset:offset + 4], "big"); changed = alias_native(raw, b"STNN" if kind.startswith("station") else b"VEHS", packet)
    elif kind == "coherent-mass":
        _, body, _, _ = riff_location(raw, b"SPDJ"); row = body + 10; quantity = int.from_bytes(raw[row + 49:row + 51], "big"); original = raw[row + 58]; assert original >= 2
        changed[row + 58] = original // 2; changed[row + 59:row + 61] = (quantity * changed[row + 58]).to_bytes(2, "big")
    else:
        start, body, _, end = riff_location(raw, b"CDIM"); count = raw[body]; assert count > 1
        if kind == "configured-mass":
            _, descent, _, _ = riff_location(raw, b"SPDJ"); cargo = raw[descent + 10 + 48]; positions = [body + 1 + 6 * i for i in range(count) if raw[body + 1 + 6 * i] == cargo]; assert len(positions) == 1
            position = positions[0] + 5; changed[position] ^= 1
        elif kind == "missing-dimensions":
            changed = raw[:start] + raw[end:]
        else:
            assert kind == "duplicate-dimensions"; changed[body + 7] = raw[body + 1]
    assert changed != raw; target = context.saves / ("malformed-heavy-joined-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed); return target


def main():
    """Require independent clean full/copied refusals in scopes of at most four malformed candidates."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=tuple(GROUPS), required=True); parser.add_argument("--selector", nargs="+", type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); states = {}; reports = []
    # Exact explicit current native membership supplies every original paid state; malformed copies and prior smoke selectors cannot supply another candidate's authority.
    for selector in args.selector:
        selected = SHARED["read"](selector); assert selected["configuration"] == args.configuration and selected["complete"] and selected["scope"].startswith("native-")
        for member in selected["reports"]:
            proof = SHARED["read"](ROOT / member); assert proof["case"] not in states; states[proof["case"]] = ROOT / member
    for kind in GROUPS[args.scope]:
        selected = states[CASES[kind]]; proof = SHARED["read"](selected); source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
        context = current_run_context("space", "heavy-joined-refusal-" + kind); malformed = damage(context, source, kind)
        full = context.for_feature("space", "heavy-joined-full-" + kind); preview = context.for_feature("space", "heavy-joined-preview-" + kind)
        SHARED["native"](full, args.configuration, malformed, reject=True); assertions = SHARED["native"](preview, args.configuration, source, corrupt=malformed)
        assert identity(source) == proof["after"]; target = context.reports / "heavy-joined-refusal.json"
        target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CD; same candidate independently refuses clean full startup and copied preview, with active-world authority/money/RNG unchanged.",
                                         configuration=args.configuration, kind=kind, proof=identity(selected), source=proof["after"], malformed=identity(malformed), assertions=assertions,
                                         native_reports=[identity(p) for owner in (full, preview) for p in sorted(owner.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
        reports.append(target); print(args.configuration, "refused", kind, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CD; bounded paired joined-admission refusals.", configuration=args.configuration,
                                          scope="reject-" + args.scope, complete=True, reports=[p.relative_to(ROOT).as_posix() for p in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
