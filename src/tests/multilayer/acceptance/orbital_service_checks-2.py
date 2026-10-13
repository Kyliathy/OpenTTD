"""Reject malformed contained service using independent native load and copied preview.

@file orbital_service_checks-2.py Bounded service/reference refusal burst.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1B.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location
from runner import current_run_context

native = runpy.run_path(str(Path(__file__).with_name("orbital_service_checks-1.py")))["native"]
KINDS = ("producer", "consumer", "work", "missing-metal", "missing-electronics", "duplicate-packet",
         "packet-id", "future-craft", "service-serial", "flag")


def damage(context, source, kind):
    """Alter one bounded service field in a copy, preserving the genuine paused-work input.

    @param context Isolated malformed-copy owner.
    @param source Actual native partial-service save.
    @param kind Exact refusal boundary.
    @return Repository-local malformed copy.
    """
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 418
    changed = bytearray(stream)
    _, body, _, _ = riff_location(stream, b"SPST")
    row = body + 10
    assert stream[row + 41] == 1
    offset = row + 47
    # Skip each bounded416 sparse stock slot to reach the append-only417 physical service fields.
    for _ in range(stream[row + 46]):
        packets = struct.unpack_from(">H", stream, offset + 3)[0]
        offset += 5 + 6 * packets
    assert struct.unpack_from(">H", stream, offset + 29)[0] == 111
    assert struct.unpack_from(">H", stream, offset + 50)[0] == 1
    assert struct.unpack_from(">H", stream, offset + 60)[0] == 1
    if kind in ("producer", "consumer"):
        start = offset + (0 if kind == "producer" else 4)
        changed[start:start + 4] = struct.pack(">I", 0xFFFFFFFE)
    elif kind == "work":
        changed[offset + 29:offset + 31] = struct.pack(">H", 149)
    elif kind in ("missing-metal", "missing-electronics"):
        start = offset + (48 if kind == "missing-metal" else 58)
        changed[start:start + 2] = bytes(2)
    elif kind == "duplicate-packet":
        changed[offset + 62:offset + 66] = stream[offset + 52:offset + 56]
    elif kind == "packet-id":
        changed[offset + 52:offset + 56] = struct.pack(">I", 0xFFFFFFFF)
    elif kind == "future-craft":
        changed[offset + 21:offset + 29] = struct.pack(">Q", 0xFFFFFFFFFFFFFFFF)
    elif kind == "service-serial":
        changed[offset + 40:offset + 48] = struct.pack(">Q", 0xFFFFFFFFFFFFFFFF)
    elif kind == "flag":
        changed[offset + 31] = 2
    else:
        raise ValueError(kind)
    target = context.saves / ("service-malformed-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def main():
    """Verify refusal parity without allowing a preview to replace the separate active world's authority."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--source", type=Path, required=True, help="Native service-work-418.sav")
    args = parser.parse_args()
    source = args.source.resolve()
    assert source.is_relative_to(ROOT)
    original = identity(source)
    context = current_run_context("space", "service-refusals")
    # Both acceptance paths inspect each independently malformed reference against its own copied/native owners.
    for kind in KINDS:
        corrupt = damage(context.for_feature("space", "service-damage-" + kind), source, kind)
        native(context.for_feature("space", "service-reject-" + kind), args.configuration, "work", corrupt, reject=True)
        native(context.for_feature("space", "service-preview-" + kind), args.configuration, "work", source, corrupt=corrupt)
    assert identity(source) == original
    report = context.reports / "orbital-service-refusals.json"
    report.write_text(json.dumps({"configuration": args.configuration, "input": original,
        "full_load_and_preview": list(KINDS), "active_world_unchanged": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Core service refusal boundaries verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
