"""Verify real mined cargo through paid pods, saved target loss and onboard service."""
import argparse
import json
import os
import struct
import subprocess
from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream
from launch_center_checks import riff_location
from launch_logistics_checks import native as logistics_native
from runner import current_run_context, run_native


def native(context, configuration, action, source, corrupt=None, reject=False):
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("space_descent_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_SPACE_DESCENT_ACTION=action, OTTD_SPACE_DESCENT_SOURCE=str(source))
    env.pop("OTTD_SPACE_DESCENT_CORRUPT", None)
    if corrupt is not None:
        env["OTTD_SPACE_DESCENT_CORRUPT"] = str(corrupt)
    log = context.logs / "native-descent.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.space-descent]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=120)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        assert result.returncode != 0 and "All tests passed" not in evidence, log
        assert any(word in evidence.lower() for word in ("space descent", "space manifest", "space pod", "space fleet")), log
        return
    assert result.returncode == 0 and "All tests passed" in evidence and "SPACE_DESCENT native=true" in evidence, log
    if corrupt:
        assert "preview_reject=true authority_unchanged=true" in evidence, log
    return context.saves / ("descent-partial-414.sav" if action == "prepare" else "descent-complete-414.sav")


def first_supply_packet(stream):
    _, body, _, _ = riff_location(stream, b"SPSU")
    pos = body + 1
    for _ in range(stream[body]):
        pods = stream[pos + 60]
        pos += 61 + 10 * pods
        for _ in range(3):
            packets = stream[pos]
            pos += 1
            if packets:
                return stream[pos:pos + 4]
            pos += 6 * packets
    raise AssertionError("The paid mining fixture must retain its actual service preload")


def damage(context, source, kind):
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 414
    start, body, length, end = riff_location(stream, b"SPDJ")
    changed = bytearray(stream)
    row = body + 10
    if kind == "missing":
        del changed[start:end]
    elif kind == "count":
        changed[body + 8:body + 10] = struct.pack(">H", 1025)
    elif kind == "next-id":
        changed[body:body + 8] = bytes(8)
    elif kind == "pod-generation":
        changed[row + 2:row + 10] = bytes(8)
    elif kind == "job-id":
        changed[row + 10:row + 18] = bytes(8)
    elif kind == "target-generation":
        changed[row + 20:row + 28] = struct.pack(">Q", 999)
    elif kind == "sender-generation":
        changed[row + 40:row + 48] = struct.pack(">Q", 999)
    elif kind == "quantity":
        changed[row + 49:row + 51] = struct.pack(">H", 11)
    elif kind == "work":
        changed[row + 51:row + 53] = struct.pack(">H", 149)
    elif kind == "age":
        changed[row + 53:row + 55] = struct.pack(">H", 185)
    elif kind == "block":
        changed[row + 55] = 255
    elif kind == "packet-count":
        changed[row + 56] = 41
    elif kind == "packet-id":
        changed[row + 57:row + 61] = struct.pack(">I", 0xFFFFFFFF)
    elif kind == "duplicate-supply-owner":
        changed[row + 57:row + 61] = first_supply_packet(stream)
    elif kind == "outside-circle":
        point = struct.unpack(">I", stream[row + 28:row + 32])[0] + 20 * 65536 + 1
        changed[row + 28:row + 32] = struct.pack(">I", point)
        _, pods, _, _ = riff_location(stream, b"SPOD")
        changed[pods + 10 + 48:pods + 10 + 52] = struct.pack(">I", point)
    else:
        raise ValueError(kind)
    target = context.saves / f"descent-{kind}.sav"
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    context = current_run_context("space", "descent-verify")
    retained = ROOT / "src/tests/fixtures/demos/space/space-launch-hub-1990-408.sav"
    original = identity(retained)
    ground = context.for_feature("space", "descent-ground-production")
    logistics_native(ground, args.configuration, "prepare")
    prepare = context.for_feature("space", "descent-prepare")
    partial = native(prepare, args.configuration, "prepare", ground.saves / "logistics-bootstrap-414.sav")
    service = prepare.saves / "descent-service-414.sav"
    expected = chunks(prepare.saves / "descent-complete-414.sav")[1]
    exact = (b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF", b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR", b"STNN", b"LCTR", b"LCHB", b"LHND")
    for label, source, action in (("drop", partial, "continue"), ("service", service, "service")):
        for replay in ("resume", "replay"):
            result_path = native(context.for_feature("space", f"descent-{replay}-{label}"), args.configuration, action, source)
            result = chunks(result_path)[1]
            for key in exact:
                assert expected[key] == result[key], ("native interrupted descent/service continuation", label, key, identity(prepare.saves / "descent-complete-414.sav"), identity(result_path))
        owner = context.for_feature("space", "descent-ordinary-" + label)
        ordinary(owner, {"name": "descent-" + label, "snapshot": identity(source)}, args.configuration)
        before, after = chunks(source)[1], chunks(owner.saves / "ordinary-resave.sav")[1]
        for key in exact:
            assert before[key] == after[key], ("ordinary paused reload", label, key)
    kinds = ("missing", "count", "next-id", "pod-generation", "job-id", "target-generation", "sender-generation", "quantity", "work", "age", "block", "packet-count", "packet-id", "duplicate-supply-owner", "outside-circle")
    physical = {"quantity"}
    for kind in kinds:
        owner = context.for_feature("space", "descent-reject-" + kind)
        corrupt = damage(owner, partial, kind)
        native(owner, args.configuration, "continue", corrupt, reject=True)
        if kind not in physical:
            native(context.for_feature("space", "descent-preview-" + kind), args.configuration, "continue", partial, corrupt=corrupt)
    assert identity(retained) == original
    report = context.reports / "space-descent-verification.json"
    report.write_text(json.dumps({"configuration": args.configuration, "retained": original, "interrupted": identity(partial), "service_partial": identity(service),
                                 "conserved_mined_units": 20, "paid_shells_expended": 2, "service_consumed": {"RFMT": 1, "ELEC": 1},
                                 "foreign_overlapping_center_refusal": True, "full_storage_refusal": True, "explicit_target_loss_recovery": True,
                                 "ordinary_exact_chunks": [key.decode() for key in exact], "native_corrupt_rejections": len(kinds), "preview_rejections": len(kinds) - len(physical),
                                 "remaining_scope": "survey/construction business actions, orbital resupply, art and full Plan9/10 acceptance"}, indent=2) + "\n", encoding="utf-8")
    print(f"Paid cargo descent and onboard service verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
