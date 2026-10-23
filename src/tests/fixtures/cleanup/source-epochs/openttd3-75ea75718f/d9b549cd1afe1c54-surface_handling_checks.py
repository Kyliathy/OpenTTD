"""Verify actual launch-hub job saves, interrupted ageing/fees, replay and corrupt transport."""
import argparse
import json
import struct
from launch_center_checks import native, reject, riff_location
from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream
from runner import current_run_context


def damage(context, source, kind):
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 414
    start, body, length, end = riff_location(stream, b"LHND")
    assert length == 52, length
    changed = bytearray(stream)
    if kind == "missing":
        del changed[start:end]
    elif kind == "counter":
        changed[body:body + 8] = bytes(8)
    elif kind == "job-id":
        changed[body + 11:body + 19] = bytes(8)
    elif kind == "cargo":
        changed[body + 39] = 255
    elif kind == "direction":
        changed[body + 40] = 2
    elif kind == "timer":
        changed[body + 41:body + 43] = bytes(2)
    elif kind == "age":
        changed[body + 43:body + 45] = struct.pack(">H", 185)
    elif kind == "packets":
        changed[body + 45] = 65
    elif kind == "center-generation":
        changed[body + 21:body + 29] = struct.pack(">Q", 999)
    elif kind == "member-generation":
        changed[body + 31:body + 39] = struct.pack(">Q", 999)
    elif kind == "orphan-packet":
        changed[body + 46:body + 50] = struct.pack(">I", 999999)
    else:
        raise ValueError(kind)
    target = context.saves / f"corrupt-handling-{kind}.sav"
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    context = current_run_context("space", "surface-handling-verify")
    retained = ROOT / "src/tests/fixtures/demos/space/space-survey-approach-1990-404.sav"
    original = identity(retained)
    built = native(context.for_feature("space", "handling-build"), args.configuration, handling="prepare")
    version, before = chunks(built)
    assert version == 414
    assert struct.unpack(">QHB", before[b"LHND"][3:14]) == (2, 1, 0)
    outputs = []
    for name in ("handling-resume", "handling-replay"):
        result = native(context.for_feature("space", name), args.configuration, built, handling="continue")
        outputs.append(chunks(result)[1])
    for key in (b"LHND", b"LCTR", b"LCHB", b"STNN", b"CAPA", b"PLYR", b"ASTR", b"VEHS"):
        assert outputs[0][key] == outputs[1][key], ("continuation", key)
    assert struct.unpack(">QHB", outputs[0][b"LHND"][3:14]) == (2, 0, 1)
    owner = context.for_feature("space", "handling-ordinary")
    ordinary(owner, {"name": "handling", "snapshot": identity(built)}, args.configuration)
    after = chunks(owner.saves / "ordinary-resave.sav")[1]
    for key in (b"LHND", b"LCTR", b"LCHB", b"STNN", b"CAPA", b"PLYR"):
        assert before[key] == after[key], ("ordinary", key)
    kinds = ("missing", "counter", "job-id", "cargo", "direction", "timer", "age", "packets",
             "center-generation", "member-generation", "orphan-packet")
    physical = {"center-generation", "member-generation", "orphan-packet"}
    for kind in kinds:
        owner = context.for_feature("space", "handling-reject-" + kind)
        corrupt = damage(owner, built, kind)
        reject(owner, args.configuration, corrupt, kind)
        if kind not in physical:
            native(context.for_feature("space", "handling-preview-" + kind), args.configuration, built, corrupt)
    assert identity(retained) == original
    report = {"configuration": args.configuration, "retained": original, "interrupted": identity(built),
              "exact_native_continuations": 2, "ordinary_packet_and_job_preservation": True,
              "corrupt_native_rejections": len(kinds), "readonly_preview_rejections": len(kinds) - len(physical),
              "remaining_full_plan_scope": "rocket commissioning, descent, role orders, service, art, network and performance"}
    target = context.reports / "surface-handling-verification.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Surface handling save/replay/rejection verified: {target.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
