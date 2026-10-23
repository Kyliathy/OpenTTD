"""Reject malformed activated modules in bounded full-load and independent preview bursts.

@file orbital_activation_checks-3.py Version and native facility-role refusal checks.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from runner import current_run_context

native = runpy.run_path(str(Path(__file__).with_name("orbital_activation_checks-1.py")))["native"]
dock_transport = runpy.run_path(str(Path(__file__).with_name("orbital_dock_checks-2.py")))
rows, older_stream = dock_transport["rows"], dock_transport["older_stream"]


def damage(context, source, kind):
    """Change exactly one semantic boundary in a copy of the genuine populated Storage save.

    @param context Explicit malformed-input evidence owner.
    @param source Immutable populated module produced by the preceding freight burst.
    @param kind Old schema, panel alias, stock overflow or unsupported work consumer.
    @return Malformed repository-local copy, preserving native chunk framing.
    """
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 422
    changed = bytearray(stream)
    row = rows(stream, version)[1]
    start = row["start"]
    assert stream[start + 41] == 1 and stream[start + 46] == 1
    if kind == "panel-alias":
        changed[row["consumer"] - 4:row["consumer"]] = struct.pack(">I", 0)
    elif kind == "stock-cap":
        changed[start + 48:start + 50] = struct.pack(">H", 1025)
    elif kind == "work-consumer":
        changed[row["assembly"]:row["assembly"] + 4] = struct.pack(">I", 0)
    elif kind == "old-version":
        changed = older_stream(stream, 418, current_version=version)
    else:
        raise ValueError(kind)
    target = context.saves / ("module-malformed-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", (418 if kind == "old-version" else version) << 16) + changed)
    return target


def main(second=False):
    """Verify two refusal boundaries per invocation, keeping native Debug loading below two minutes.

    @param second False selects schema/panel cases; True selects stock/work-consumer cases.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--freight", type=Path, required=True, help="Exact orbital-module-freight.json from the preceding burst")
    args = parser.parse_args()
    report_path = args.freight.resolve()
    assert report_path.is_relative_to(ROOT)
    freight = json.loads(report_path.read_text(encoding="utf-8"))
    assert freight["configuration"] == args.configuration
    source = ROOT / freight["complete"]["path"]
    original = identity(source)
    assert original == freight["complete"]
    context = current_run_context("space", "module-activation-refusals-2" if second else "module-activation-refusals-1")
    tested = []
    # Each copied boundary is refused by independent loader and preview paths while the genuine source and active owners remain intact.
    for kind in (("stock-cap", "work-consumer") if second else ("old-version", "panel-alias")):
        malformed = damage(context.for_feature("space", "module-activation-damage-" + kind), source, kind)
        native(context.for_feature("space", "module-activation-reject-" + kind), args.configuration, "reject", malformed, reject=True)
        if kind == "old-version":
            log = context.for_feature("space", "module-activation-reject-" + kind).logs / "native-module-activation.log"
            assert "Invalid orbital extension activation version" in log.read_text(encoding="utf-8", errors="replace")
        native(context.for_feature("space", "module-activation-preview-" + kind), args.configuration, "preview", source, corrupt=malformed)
        tested.append({"kind": kind, "malformed": identity(malformed), "full_load_refused": True, "preview_refused": True})
    assert identity(source) == original
    target = context.reports / "orbital-module-activation-refusals.json"
    target.write_text(json.dumps({"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group4; independent native loader/preview refusal.",
        "configuration": args.configuration, "freight": identity(report_path), "source": original, "cases": tested}, indent=2) + "\n", encoding="utf-8")
    print(f"Native module activation refusals verified: {target.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
