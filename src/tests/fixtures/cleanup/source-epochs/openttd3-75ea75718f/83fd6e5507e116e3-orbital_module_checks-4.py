"""Reject malformed418 assembly receipts through independent full-load and copied-preview paths.

@file orbital_module_checks-4.py Exact work/remainder/clock and original consumer refusal boundaries.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group3.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location
from runner import current_run_context

native = runpy.run_path(str(Path(__file__).with_name("orbital_module_checks-2.py")))["native"]
KINDS = ("fraction", "remainder", "future-tick", "future-interval", "missing-interval", "duplicate-consumer", "consumer-id", "missing-material", "work-limit", "old-version")


def rows(stream):
    """Locate bounded418 job/facility appendices without resolving any live owner.

    @param stream Decompressed native save stream.
    @return Structure IDs mapped to exact work, material, consumer and receipt offsets.
    """
    _, body, _, end = riff_location(stream, b"SPST")
    count = struct.unpack_from(">H", stream, body + 8)[0]
    cursor = body + 10
    result = {}
    # Walk every declared row and its actual reference counts so damage changes only the chosen field, preserving all native framing.
    for _ in range(count):
        start = cursor
        site = struct.unpack_from(">H", stream, start)[0]
        cursor += 29
        quantities = []
        for _ in range(2):
            quantities.append(cursor + 2)
            packets = struct.unpack_from(">H", stream, cursor + 4)[0]
            cursor += 6 + 6 * packets
        present = stream[cursor]
        cursor += 1
        consumer = assembly = None
        if present:
            slots = stream[cursor + 4]
            cursor += 5
            for _ in range(slots):
                packets = struct.unpack_from(">H", stream, cursor + 3)[0]
                cursor += 5 + 6 * packets
            consumer = cursor + 4
            cursor += 48
            for _ in range(2):
                packets = struct.unpack_from(">H", stream, cursor + 2)[0]
                cursor += 4 + 6 * packets
            assembly = cursor
            cursor += 4
        result[site] = {"start": start, "work": start + 23, "quantities": quantities,
                        "consumer": consumer, "assembly": assembly, "receipt": cursor}
        cursor += 20
        assert cursor <= end
    assert cursor == end
    return result


def damage(context, source, kind):
    """Change one independently malformed field in a copy of an actual paid partial-work save.

    @param context Isolated malformed-copy owner.
    @param source Genuine418 fractional checkpoint, kept immutable.
    @param kind Exact refusal boundary.
    @return Repository-owned malformed native copy.
    """
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 418
    changed = bytearray(stream)
    offsets = rows(stream)
    receipt = offsets[1]["receipt"]
    if kind == "fraction":
        changed[receipt:receipt + 2] = struct.pack(">H", 62915)
    elif kind == "remainder":
        changed[receipt + 2:receipt + 4] = struct.pack(">H", 750)
    elif kind == "future-tick":
        changed[receipt + 4:receipt + 12] = struct.pack(">Q", 0xFFFFFFFFFFFFFFFF)
    elif kind == "future-interval":
        changed[receipt + 12:receipt + 20] = struct.pack(">Q", 0xFFFFFFFFFFFFFFFF)
    elif kind == "missing-interval":
        changed[receipt + 12:receipt + 20] = struct.pack(">Q", 0)
    elif kind == "duplicate-consumer":
        assembly = offsets[0]["assembly"]
        consumer = offsets[0]["consumer"]
        changed[assembly:assembly + 4] = stream[consumer:consumer + 4]
    elif kind == "consumer-id":
        assembly = offsets[0]["assembly"]
        changed[assembly:assembly + 4] = struct.pack(">I", 0xFFFFFFFE)
    elif kind == "missing-material":
        quantity = offsets[1]["quantities"][0]
        changed[quantity:quantity + 2] = struct.pack(">H", 7)
    elif kind == "work-limit":
        work = offsets[1]["work"]
        changed[work:work + 4] = struct.pack(">I", 297)
    elif kind != "old-version":
        raise ValueError(kind)
    target = context.saves / ("assembly-malformed-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", (417 if kind == "old-version" else version) << 16) + changed)
    return target


def main(second=False):
    """Verify five independent decoder/preview refusals in one bounded burst.

    @param second False verifies physical work receipts; True verifies consumers, materials, work limits and schema ownership.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--source", type=Path, required=True, help="Genuine module-assembly-partial-418.sav")
    args = parser.parse_args()
    source = args.source.resolve()
    assert source.is_relative_to(ROOT)
    original = identity(source)
    part = "2" if second else "1"
    kinds = KINDS[5:] if second else KINDS[:5]
    context = current_run_context("space", "module-assembly-refusals-" + part)
    # Full load and preview each inspect their own copy; preview must never publish a changed tick, fraction, packet owner or source account.
    for kind in kinds:
        corrupt = damage(context.for_feature("space", "module-assembly-damage-" + kind), source, kind)
        native(context.for_feature("space", "module-assembly-reject-" + kind), args.configuration, "partial", corrupt, reject=True)
        native(context.for_feature("space", "module-assembly-preview-" + kind), args.configuration, "partial", source, corrupt=corrupt)
    assert identity(source) == original
    report = context.reports / ("orbital-module-assembly-refusals-" + part + ".json")
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group3; independent native/copied receipt refusals.",
        "configuration": args.configuration, "input": original, "full_load_and_preview": list(kinds),
        "active_native_owners_and_rng_unchanged": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Native assembly refusal boundaries verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
