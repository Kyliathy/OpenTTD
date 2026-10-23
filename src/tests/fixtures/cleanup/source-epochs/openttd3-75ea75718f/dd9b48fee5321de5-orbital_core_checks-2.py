"""Reject malformed Core activation/stock without mutating the active world.

@file orbital_core_checks-2.py Bounded independent full-load and copied-preview refusal burst.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location
from runner import current_run_context


native = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["native"]
KINDS = ("presence", "activation", "pending-stock", "stock-cap", "packet-count", "packet-id", "aging", "old-version")


def damage(context, source, kind):
    """Change one facility boundary in a copy of an actual populated Core save.

    @param context Isolated corrupted-input owner.
    @param source Genuine populated native Core checkpoint, kept unchanged.
    @param kind One malformed field under test.
    @return Repository-local malformed copy for full-load and independent preview checks.
    """
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 418
    changed = bytearray(stream)
    _, body, _, _ = riff_location(stream, b"SPST")
    row = body + 10
    # The first paid Core's construction inputs are fully consumed, so its unchanged415 prefix is exactly41 bytes.
    assert struct.unpack_from(">H", stream, row + 33)[0] == 0
    assert struct.unpack_from(">H", stream, row + 39)[0] == 0
    assert stream[row + 41] == 1 and stream[row + 46] == 1
    if kind == "presence":
        changed[row + 41] = 2
    elif kind == "activation":
        changed[row + 42:row + 44] = struct.pack(">H", 75)
    elif kind == "pending-stock":
        changed[row + 42:row + 44] = struct.pack(">H", 1)
    elif kind == "stock-cap":
        changed[row + 48:row + 50] = struct.pack(">H", 1025)
    elif kind == "packet-count":
        changed[row + 50:row + 52] = bytes(2)
    elif kind == "packet-id":
        changed[row + 52:row + 56] = struct.pack(">I", 0xFFFFFFFF)
    elif kind == "aging":
        changed[row + 44:row + 46] = struct.pack(">H", 65535)
    elif kind != "old-version":
        raise ValueError(kind)
    target = context.saves / ("core-malformed-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", (415 if kind == "old-version" else version) << 16) + changed)
    return target


def main():
    """Run bounded malformed facility cases independently of lifecycle production."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--source", type=Path, required=True, help="Populated core-stored-418.sav from lifecycle burst")
    args = parser.parse_args()
    source = args.source.resolve()
    assert source.is_relative_to(ROOT)
    original = identity(source)
    context = current_run_context("space", "core-refusals")
    # Each malformed copy must fail both native load and a preview while a separate valid active world retains its authority.
    for kind in KINDS:
        corrupt = damage(context.for_feature("space", "core-damage-" + kind), source, kind)
        native(context.for_feature("space", "core-reject-" + kind), args.configuration, "stored", corrupt, reject=True)
        native(context.for_feature("space", "core-preview-" + kind), args.configuration, "stored", source, corrupt=corrupt)
    assert identity(source) == original
    report = context.reports / "orbital-core-refusals.json"
    report.write_text(json.dumps({"configuration": args.configuration, "input": original,
                                 "full_load_and_preview": list(KINDS), "active_world_unchanged": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Core malformed-load/preview boundaries verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
