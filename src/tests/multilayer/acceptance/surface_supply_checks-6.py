"""Create bounded malformed order-file copies and check one full/copied refusal pair.

@file surface_supply_checks-6.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCC
Only invocation-owned current-version copies change. Original save versions and
accepted source files remain immutable; declared receipts are never procurement.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream, Reader, CHUNK_RIFF, CHUNK_RECORDS
from launch_center_checks import riff_location
from artifact_paths import create_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("surface_supply_checks-5.py")))
CASES = ("missing", "repeated", "frontier", "day", "count", "truncated", "trailing", "payer", "owner",
         "origin", "position", "cargo", "quantity", "price", "buyer", "state", "pending-packet",
         "fulfilled-packet", "missing-day-field", "missing-map-field", "missing-station-field")


def family_bounds(raw, wanted):
    """Locate one original native family, including table framing without scanning payload text.

    @param raw Original complete decompressed stream. @param wanted Four-byte native family.
    @return Its exact whole chunk bounds, excluding the terminating stream marker.
    """
    reader = Reader(raw)
    # Native declared lengths select the real family, so arbitrary cargo strings
    # and duplicate text inside nested data cannot impersonate a schema boundary.
    while True:
        start = reader.pos
        name = reader.take(4)
        assert name != bytes(4), wanted
        mode = reader.byte()
        kind = mode & 15
        if kind == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            reader.take(length)
        elif kind in CHUNK_RECORDS:
            # Every exact original array extent includes its own index; retain
            # all rows and nested data while locating the next chunk boundary.
            while True:
                length = reader.gamma()
                if not length:
                    break
                reader.take(length - 1)
        else:
            raise ValueError(kind)
        if name == wanted:
            return start, reader.pos


def rename_schema_field(raw, family, field, renamed):
    """Hide an exact native authority field in the original bounded table header only.

    @param raw Original complete native stream. @param family Actual native table identity.
    @param field Exact authority name, excluding longer names or row data.
    @param renamed Equal-width unknown field name. @return Changed stream, with all row/framing bytes retained.
    """
    first, last = family_bounds(raw, family)
    reader = Reader(raw[first + 5:last])
    extent = reader.gamma()
    absolute = first + 5 + reader.pos
    header = Reader(reader.take(extent - 1))
    changed = bytearray(raw)
    matches = []

    def visit(depth=0):
        """Walk the original parent-first native schema without interpreting row data.

        @param depth Bounded nested descriptor depth; station discriminator stays unchanged.
        """
        assert depth < 16
        fields = []
        # Native nested schemas follow the whole parent list. Exact header names
        # exclude calendar/economy suffixes and coincidental strings in cargo data.
        while True:
            tag = header.byte()
            if tag == 0:
                break
            width = header.gamma()
            position = absolute + header.pos
            name = header.take(width)
            fields.append(tag)
            # Only original BaseStation authority is hidden; the outer station
            # discriminator continues selecting the actual native object shape.
            if name == field and (field != b"facilities" or depth > 0):
                assert len(renamed) == width
                matches.append(position)
                changed[position:position + width] = renamed
        # Both normal/waypoint branches describe the same canonical base field;
        # hiding that field consistently leaves native variant framing intact.
        for tag in fields:
            if tag & 15 == 11:
                visit(depth + 1)

    visit()
    assert header.pos == len(header.data) and matches
    return changed


def malformed(context, source, case):
    """Change exactly one declared current-file ownership/shape fact.

    @param context Repository-owned isolated evidence destination.
    @param source Actual valid440 file with four declared order states.
    @param case Explicit malformed boundary. @return New current-version file; source unchanged.
    """
    version, raw = read_stream(source)
    assert version == 440 and case in CASES
    original = identity(source)
    start, body, length, end = riff_location(raw, b"SSOR")
    assert length == 14 + 4 * 53
    payload = bytearray(raw[body:end])
    changed = bytearray(raw)
    if case == "missing":
        del changed[start:end]
    elif case == "repeated":
        changed[end:end] = raw[start:end]
    elif case.startswith("missing-"):
        family, field, renamed = {
            "missing-day-field": (b"DATE", b"economy_date", b"economy_datx"),
            "missing-map-field": (b"MAPS", b"dim_x", b"dim_q"),
            "missing-station-field": (b"STNN", b"facilities", b"facilitiex"),
        }[case]
        changed = rename_schema_field(raw, family, field, renamed)
    else:
        if case == "frontier":
            payload[0:8] = bytes(8)
        elif case == "day":
            struct.pack_into("<i", payload, 8, struct.unpack_from("<i", payload, 8)[0] + 1)
        elif case == "count":
            payload[12:14] = b"\xff\xff"
        elif case == "truncated":
            payload.pop()
        elif case == "trailing":
            payload.append(0)
        else:
            offset, value = {
                "payer": (8, b"\xfe"), "owner": (9, b"\x0e"), "origin": (10, struct.pack("<H", 63999)),
                "position": (12, struct.pack("<H", 4095)), "cargo": (16, b"XXXX"), "quantity": (20, bytes(2)),
                "price": (22, bytes(8)), "buyer": (46, struct.pack("<H", 14)), "state": (48, b"\xff"),
                "pending-packet": (49, struct.pack("<I", 17)),
                "fulfilled-packet": (53 + 49, struct.pack("<I", 0xFFFFFF)),
            }[case]
            payload[14 + offset:14 + offset + len(value)] = value
        # Preserve native RIFF framing for exact length faults; these copies
        # test SSOR admission rather than an unrelated stream truncation error.
        assert len(payload) < 1 << 24
        changed[start:end] = b"SSOR\x00" + len(payload).to_bytes(3, "big") + payload
    target = context.saves / ("malformed-surface-order-" + case + "-440.sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    assert identity(source) == original
    return target


def main():
    """Run one bounded malformed-file pair, retaining both original native terminals."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--case", choices=CASES, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    context = create_run_context("space", "surface-supply-order-malformed")
    source = (ROOT / args.source).resolve()
    bad = malformed(context, source, args.case)
    reports = []
    # Preview must retain the open world; full refusal must expose no partial
    # restored rights. Each operation uses a fresh independently bounded process.
    for phase in ("preview-refusal", "full-refusal"):
        reports.append(SHARED["execute"](args.configuration, phase, source, bad))
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     case=args.case, source=identity(source), malformed=identity(bad), reports=reports,
                                     sources=[identity(Path(__file__))], runtime_procurement_acceptance=False), indent=2) + "\n", encoding="utf-8")
    print(identity(target), flush=True)


if __name__ == "__main__":
    main()
