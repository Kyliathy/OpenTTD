"""Reject malformed same-file spent-module receipts through full load and independent preview.

@file orbital_removal_checks-2.py Native422 issuance/material/service history bounds.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group5.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location
from runner import current_run_context

native = runpy.run_path(str(Path(__file__).with_name("orbital_removal_checks-1.py")))["native"]


def damage(context, source, kind):
    """Change one bounded aggregate claim, keeping every physical row and native family framed correctly.

    @param context Explicit malformed-input evidence owner.
    @param source Genuine422 removed-module checkpoint, preserved unchanged.
    @param kind Retired issuance overflow, live/retired overlap, material overflow or service without a retired owner.
    @return Repository-local malformed copy whose actual history boundary must refuse.
    """
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 422
    _, body, _, _ = riff_location(stream, b"SPST")
    changed = bytearray(stream)
    if kind == "unissued":
        changed[body + 10:body + 18] = struct.pack(">Q", 0xFFFFFFFFFFFFFFFF)
    elif kind == "live-overlap":
        next_generation = struct.unpack_from(">Q", stream, body)[0]
        changed[body + 10:body + 18] = struct.pack(">Q", next_generation - 1)
    elif kind == "material-limit":
        changed[body + 18:body + 26] = struct.pack(">Q", 0xFFFFFFFFFFFFFFFF)
    elif kind == "ownerless-service":
        # Canonical zero material/retired counts isolate this single semantic fault instead of also failing unrelated material bounds.
        changed[body + 10:body + 42] = struct.pack(">QQQQ", 0, 0, 0, 1)
    else:
        raise ValueError(kind)
    target = context.saves / ("removal-malformed-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def main():
    """Run two full-load and copied-preview refusals per focused native burst."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--removal", type=Path, required=True, help="Exact preceding native module removal report")
    parser.add_argument("--batch", type=int, required=True, choices=(1, 2))
    args = parser.parse_args()
    predecessor = args.removal.resolve()
    assert predecessor.is_relative_to(ROOT)
    previous = json.loads(predecessor.read_text(encoding="utf-8"))
    assert previous["configuration"] == args.configuration
    source = ROOT / previous["complete"]["path"]
    original = identity(source)
    assert original == previous["complete"]
    context = current_run_context("space", "module-removal-refusals-" + str(args.batch))
    tested = []
    # Each detached claim must fail both independent boundaries while every active structure/queue/utility/company/RNG owner remains unchanged.
    for kind in (("unissued", "live-overlap") if args.batch == 1 else ("material-limit", "ownerless-service")):
        malformed = damage(context.for_feature("space", "module-removal-damage-" + kind), source, kind)
        native(context.for_feature("space", "module-removal-reject-" + kind), args.configuration, "reject", malformed, reject=True)
        native(context.for_feature("space", "module-removal-preview-" + kind), args.configuration, "preview", source, corrupt=malformed)
        tested.append({"kind": kind, "malformed": identity(malformed), "full_load_refused": True, "preview_refused": True})
    assert identity(source) == original
    report = context.reports / "orbital-module-removal-refusals.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group5; same-file native spent-receipt bounds.",
        "configuration": args.configuration, "removal": identity(predecessor), "source": original, "cases": tested}, indent=2) + "\n", encoding="utf-8")
    print(f"Native module removal refusals verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
