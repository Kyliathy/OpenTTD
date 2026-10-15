"""Reject independent malformed native Dock connections, jobs and schema versions.

@file orbital_dock_checks-2.py Same-file421 source/berth/part ownership refusals.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4C.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location
from runner import current_run_context

native = runpy.run_path(str(Path(__file__).with_name("orbital_dock_checks-1.py")))["native"]


def rows(stream, version=421):
    """Locate contained owners independently of live pools or native decoding.

    @param stream Decompressed native421/422 stream.
    @param version Same-file schema;422 adds bounded historical receipts to the family header.
    @return Module records with exact connection/job/packet and extension offsets.
    """
    _, body, _, end = riff_location(stream, b"SPST")
    count = struct.unpack_from(">H", stream, body + 8)[0]
    cursor = body + (42 if version >= 422 else 10)
    result = {}
    # Walk bounded physical counts to preserve framing when changing exactly one semantic owner.
    for _ in range(count):
        start = cursor
        site = struct.unpack_from(">H", stream, start)[0]
        cursor += 29
        for _ in range(2):
            packets = struct.unpack_from(">H", stream, cursor + 4)[0]
            cursor += 6 + 6 * packets
        present = stream[cursor]
        cursor += 1
        row = {"start": start, "kind": stream[start + 11]}
        if present:
            slots = stream[cursor + 4]
            cursor += 5
            for _ in range(slots):
                packets = struct.unpack_from(">H", stream, cursor + 3)[0]
                cursor += 5 + 6 * packets
            row["producer"] = cursor
            row["consumer"] = cursor + 4
            cursor += 17
            row["jobs"] = []
            # The two actual jobs share their existing field order; only the second belongs to the421 appendix.
            for berth in range(2):
                job = {"craft": cursor, "parts": []}
                cursor += 31
                for _ in range(2):
                    packets = struct.unpack_from(">H", stream, cursor + 2)[0]
                    job["parts"].append(cursor + 4 if packets else None)
                    cursor += 4 + 6 * packets
                row["jobs"].append(job)
                if berth == 0:
                    row["assembly"] = cursor
                    cursor += 4  # Original assembly consumer.
                    row["extension_start"] = row["source"] = cursor
                    cursor += 10  # Original native source incarnation.
            row["extension_end"] = cursor
        row["receipt"] = cursor
        cursor += 20
        assert cursor <= end
        result[site] = row
    assert cursor == end
    return result


def replace_family(stream, name, payload):
    """Replace one bounded RIFF family without altering any other native owner.

    @param stream Decompressed native stream.
    @param name Exact four-byte family identifier.
    @param payload Correctly framed replacement payload.
    @return Same stream with its exact length header and payload replaced.
    """
    start, _, _, end = riff_location(stream, name)
    size = len(payload)
    assert size < 1 << 28
    header = bytes([(size >> 24) << 4, (size >> 16) & 255, (size >> 8) & 255, size & 255])
    return stream[:start + 4] + header + payload + stream[end:]


def older_stream(stream, version, current_version=421):
    """Produce real pre421 module framing and, when needed, pre420 queue framing.

    @param stream Genuine current save with unchanged semantic paid modules and no retired receipts.
    @param version Requested older schema:418 or420.
    @param current_version Same-file current schema;422 has a bounded history header that must be empty before deliberate downgrade.
    @return Correctly framed older stream, allowing explicit activated-role refusal to be tested.
    """
    assert version in (418, 420)
    offsets = rows(stream, current_version)
    _, body, _, end = riff_location(stream, b"SPST")
    payload = bytearray(stream[body:body + 10])
    previous = body + (42 if current_version >= 422 else 10)
    if current_version >= 422:
        # Never discard spent history while constructing an older-schema role fixture; these activation cases have not removed any module.
        assert stream[body + 10:body + 42] == bytes(32)
    # Strip every421 appendix, retaining each original contained module and its unchanged exact material receipt.
    for row in offsets.values():
        if "extension_start" not in row:
            continue
        payload.extend(stream[previous:row["extension_start"]])
        previous = row["extension_end"]
    payload.extend(stream[previous:end])
    stream = replace_family(stream, b"SPST", payload)
    if version < 420:
        _, body, _, end = riff_location(stream, b"SPOR")
        count = stream[body + 8]
        cursor = body + 9
        payload = bytearray(stream[body:cursor])
        # Older queues had neither the request-presence byte nor its optional30-byte physical approach owner.
        for _ in range(count):
            start = cursor
            cursor += 22 + 14 * stream[cursor + 21]
            payload.extend(stream[start:cursor])
            present = stream[cursor]
            assert present in (0, 1)
            cursor += 1 + (30 if present else 0)
        assert cursor == end
        stream = replace_family(stream, b"SPOR", payload)
    return stream


def damage(context, source, kind):
    """Change one same-file semantic boundary while preserving valid transport framing.

    @param context Explicit malformed-input owner.
    @param source Identity-verified genuine active two-job checkpoint.
    @param kind Original source generation, duplicate hull, duplicate part or genuine older schema.
    @return Repository-local malformed native copy.
    """
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 422
    offsets = rows(stream, version)
    docks = [row for row in offsets.values() if "source" in row and any(job["parts"][0] is not None for job in row["jobs"][1:])]
    assert len(docks) == 1
    dock = docks[0]
    changed = bytearray(stream)
    if kind == "source-generation":
        offset = dock["source"] + 2
        generation = struct.unpack_from(">Q", stream, offset)[0]
        changed[offset:offset + 8] = struct.pack(">Q", generation + 1)
    elif kind == "duplicate-hull":
        first, second = (job["craft"] for job in dock["jobs"])
        changed[second:second + 12] = stream[first:first + 12]
    elif kind == "duplicate-part":
        first, second = (job["parts"][0] for job in dock["jobs"])
        changed[second:second + 4] = stream[first:first + 4]
    elif kind == "old-version":
        changed = older_stream(stream, 420, current_version=version)
        version = 420
    else:
        raise ValueError(kind)
    target = context.saves / ("dock-malformed-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def main():
    """Run two full-load plus independent preview refusals per focused burst."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--service", type=Path, required=True, help="Exact preceding native Dock service report")
    parser.add_argument("--batch", type=int, required=True, choices=(1, 2))
    args = parser.parse_args()
    report_path = args.service.resolve()
    assert report_path.is_relative_to(ROOT)
    preceding = json.loads(report_path.read_text(encoding="utf-8"))
    assert preceding["configuration"] == args.configuration and preceding["phase"] == "service"
    source = ROOT / preceding["partial"]["path"]
    original = identity(source)
    assert original == preceding["partial"]
    context = current_run_context("space", "dock-refusals-" + str(args.batch))
    tested = []
    # Each fault exercises both independent boundaries without modifying the valid source or live packet/utility owners.
    for kind in (("source-generation", "duplicate-hull") if args.batch == 1 else ("duplicate-part", "old-version")):
        malformed = damage(context.for_feature("space", "dock-damage-" + kind), source, kind)
        native(context.for_feature("space", "dock-reject-" + kind), args.configuration, "reject", malformed, reject=True)
        if kind == "old-version":
            log = context.for_feature("space", "dock-reject-" + kind).logs / "native-orbital-dock.log"
            assert "Invalid orbital Dock activation version" in log.read_text(encoding="utf-8", errors="replace")
        native(context.for_feature("space", "dock-preview-" + kind), args.configuration, "preview", source, corrupt=malformed)
        tested.append({"kind": kind, "malformed": identity(malformed), "full_load_refused": True, "preview_refused": True})
    assert identity(source) == original
    target = context.reports / "orbital-dock-refusals.json"
    target.write_text(json.dumps({"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group4C; independent copied and live validation.",
        "configuration": args.configuration, "service": identity(report_path), "source": original, "cases": tested}, indent=2) + "\n", encoding="utf-8")
    print(f"Native Dock refusals verified: {target.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
