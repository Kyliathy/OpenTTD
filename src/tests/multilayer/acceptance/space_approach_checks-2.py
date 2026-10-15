"""Reject malformed native shared approach ownership in independent bounded bursts.

@file space_approach_checks-2.py Duplicate slots, target incarnation, future priority and staging bounds.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location
from runner import current_run_context

native = runpy.run_path(str(Path(__file__).with_name("space_approach_checks-1.py")))["native"]


def approach_offsets(stream):
    """Locate optional420 requests while preserving the existing bounded queue framing.

    @param stream Decompressed same-file native stream.
    @return Actual request-field offsets in native hull order.
    """
    _, body, _, end = riff_location(stream, b"SPOR")
    count = stream[body + 8]
    cursor, result = body + 9, []
    # Each declared native hull has a fixed prefix, bounded orders and one optional request; no other chunk is searched or changed.
    for _ in range(count):
        orders = stream[cursor + 21]
        cursor += 22 + 14 * orders
        present = stream[cursor]
        cursor += 1
        if present:
            # This scenario queues exactly two craft at Core0; unrelated retained body approaches keep their original owners.
            if stream[cursor] == 1 and struct.unpack_from(">I", stream, cursor + 1)[0] == 0:
                result.append(cursor)
            cursor += 30
        assert cursor <= end
    assert cursor == end and len(result) == 2
    return result


def damage(context, source, kind):
    """Change exactly one semantic request boundary in a copy of the genuine queued save.

    @param context Repository-local malformed-input owner.
    @param source This run's genuine queued native checkpoint.
    @param kind Duplicate admission, stale target, future request clock or invalid staging.
    @return Malformed copy with intact native chunk framing.
    """
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 422
    changed = bytearray(stream)
    offsets = approach_offsets(stream)
    first = offsets[0]
    if kind == "duplicate-slot":
        changed[offsets[1] + 21] = changed[first + 21] = 0
    elif kind == "target-generation":
        generation = struct.unpack_from(">Q", changed, first + 5)[0]
        changed[first + 5:first + 13] = struct.pack(">Q", generation + 1)
    elif kind == "future-tick":
        changed[first + 13:first + 21] = bytes([255]) * 8
    elif kind == "staging-bounds":
        changed[first + 22:first + 26] = struct.pack(">i", -1)
    else:
        raise ValueError(kind)
    target = context.saves / ("approach-malformed-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def main(second=False):
    """Run two independent full-load and copied-preview refusal cases per native burst.

    @param second False tests slot/target ownership; True tests clock/staging bounds.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--approaches", type=Path, required=True, help="Exact space-approaches.json from the preceding acceptance burst")
    args = parser.parse_args()
    report_path = args.approaches.resolve()
    assert report_path.is_relative_to(ROOT)
    prior = json.loads(report_path.read_text(encoding="utf-8"))
    assert prior["configuration"] == args.configuration
    source = ROOT / prior["partial"]["path"]
    original = identity(source)
    assert original == prior["partial"]
    context = current_run_context("space", "approach-refusals-2" if second else "approach-refusals-1")
    tested = []
    # Full loading and independent preview each reject the same copied semantic error without mutating the genuine active world.
    for kind in (("future-tick", "staging-bounds") if second else ("duplicate-slot", "target-generation")):
        malformed = damage(context.for_feature("space", "approach-damage-" + kind), source, kind)
        native(context.for_feature("space", "approach-reject-" + kind), args.configuration, "reject", malformed, reject=True)
        native(context.for_feature("space", "approach-preview-" + kind), args.configuration, "preview", source, corrupt=malformed)
        tested.append({"kind": kind, "malformed": identity(malformed), "full_load_refused": True, "preview_refused": True})
    assert identity(source) == original
    report = context.reports / "space-approach-refusals.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs8-space-world-and-construction.md Group11; independent native loader/preview refusal.",
        "configuration": args.configuration, "approaches": identity(report_path), "source": original, "cases": tested}, indent=2) + "\n", encoding="utf-8")
    print(f"Native shared approach refusals verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
