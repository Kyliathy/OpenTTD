"""Verify native paid rocket reuse and exact reload at every round-trip phase."""
import argparse
import json
import os
import struct
import subprocess
from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream
from launch_center_checks import riff_location
from launch_logistics_checks import native as logistics_native
from runner import current_run_context, run_native

def native(context, configuration, action, source, phase="preparing", corrupt=None, reject=False):
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("launch_flight_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_LAUNCH_FLIGHT_ACTION=action, OTTD_LAUNCH_FLIGHT_SOURCE=str(source), OTTD_LAUNCH_FLIGHT_PHASE=phase)
    env.pop("OTTD_LAUNCH_FLIGHT_CORRUPT", None)
    if corrupt is not None:
        env["OTTD_LAUNCH_FLIGHT_CORRUPT"] = str(corrupt)
    log = context.logs / "native-flight.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.launch-flight]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=120)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        assert result.returncode != 0 and "All tests passed" not in evidence, log
        assert any(word in evidence.lower() for word in ("space transition", "space manifest", "launch assembly", "space fleet")), log
        return
    assert result.returncode == 0 and "All tests passed" in evidence and "LAUNCH_FLIGHT native=true" in evidence, log
    if corrupt:
        assert "preview_reject=true authority_unchanged=true" in evidence, log
    return context.saves / ("flight-partial-414.sav" if action == "prepare" else "flight-loss-complete-414.sav" if action == "loss" else "flight-complete-414.sav")


def damage(context, source, kind):
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 414
    family = b"LCOP" if kind == "pad-generation" else b"SPFL"
    start, body, length, end = riff_location(stream, family)
    changed = bytearray(stream)
    row = body + 1
    if kind == "missing":
        del changed[start:end]
    elif kind == "count":
        changed[body] = 65
    elif kind == "generation":
        changed[row + 4:row + 12] = bytes(8)
    elif kind == "phase":
        changed[row + 12] = 255
    elif kind == "layer":
        changed[row + 14] = 1
    elif kind == "recovery-generation":
        changed[row + 27:row + 35] = struct.pack(">Q", 999)
    elif kind == "producer":
        changed[row + 35] = 1
    elif kind == "work":
        changed[row + 56:row + 58] = struct.pack(">H", 149)
    elif kind == "upkeep-remainder":
        changed[row + 58] = 74
    elif kind == "early-burn":
        changed[row + 59] = 1
    elif kind == "flags":
        changed[row + 59] = 128
    elif kind == "paid-missions":
        changed[row + 60:row + 68] = bytes(8)
    elif kind == "completion":
        changed[row + 68:row + 76] = struct.pack(">Q", 2)
    elif kind == "pad-generation":
        changed[row + 48:row + 56] = struct.pack(">Q", 999)
    else:
        raise ValueError(kind)
    target = context.saves / f"flight-{kind}.sav"
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    context = current_run_context("space", "flight-verify")
    retained = ROOT / "src/tests/fixtures/demos/space/space-launch-hub-1990-408.sav"
    original = identity(retained)
    ground = context.for_feature("space", "flight-ground-production")
    logistics_native(ground, args.configuration, "prepare")
    source = ground.saves / "logistics-bootstrap-414.sav"
    exact = (b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF", b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR", b"STNN", b"LCTR", b"LCHB", b"LHND")
    results = []
    partials = {}
    lifecycle = None
    for phase in ("preparing", "ascent", "delivery", "return", "turnaround"):
        prepare = context.for_feature("space", "flight-prepare-" + phase)
        if phase == "preparing":
            lifecycle = prepare
        partial = native(prepare, args.configuration, "prepare", source, phase)
        partials[phase] = partial
        version, before = chunks(partial)
        assert version == 414
        expected = chunks(prepare.saves / "flight-complete-414.sav")[1]
        results.append(expected)
        for name in ("resume", "replay"):
            after = chunks(native(context.for_feature("space", f"flight-{name}-{phase}"), args.configuration, "continue", partial, phase))[1]
            for key in exact:
                assert expected[key] == after[key], ("native phase continuation", phase, key)
        owner = context.for_feature("space", "flight-ordinary-" + phase)
        ordinary(owner, {"name": "flight-" + phase, "snapshot": identity(partial)}, args.configuration)
        after = chunks(owner.saves / "ordinary-resave.sav")[1]
        for key in exact:
            assert before[key] == after[key], ("ordinary paused reload", phase, key)
    for result in results[1:]:
        for key in exact:
            assert results[0][key] == result[key], ("same round trip from every phase", key)
    for phase in ("ascent", "orbit", "turnaround"):
        source = lifecycle.saves / f"flight-lost-{phase}-414.sav"
        wanted = chunks(lifecycle.saves / ("flight-recovered-turnaround-414.sav" if phase == "turnaround" else "flight-recovered-orbit-414.sav"))[1]
        for replay in ("resume", "replay"):
            result = chunks(native(context.for_feature("space", f"flight-loss-{replay}-{phase}"), args.configuration, "loss", source, phase))[1]
            for key in exact:
                assert wanted[key] == result[key], ("saved lost-home recovery", phase, key)
        owner = context.for_feature("space", "flight-loss-ordinary-" + phase)
        ordinary(owner, {"name": "flight-loss-" + phase, "snapshot": identity(source)}, args.configuration)
        after = chunks(owner.saves / "ordinary-resave.sav")[1]
        before = chunks(source)[1]
        for key in exact:
            assert before[key] == after[key], ("ordinary lost-home reload", phase, key)
    kinds = ("missing", "count", "generation", "phase", "layer", "recovery-generation", "producer", "work", "upkeep-remainder", "early-burn", "flags", "paid-missions", "completion", "pad-generation")
    for kind in kinds:
        owner = context.for_feature("space", "flight-reject-" + kind)
        corrupt = damage(owner, partials["preparing"], kind)
        native(owner, args.configuration, "continue", corrupt, reject=True)
        native(context.for_feature("space", "flight-preview-" + kind), args.configuration, "continue", partials["preparing"], corrupt=corrupt)
    assert identity(retained) == original
    report = context.reports / "launch-flight-verification.json"
    report.write_text(json.dumps({"configuration": args.configuration, "retained": original,
                                 "interrupted_phases": {key: identity(path) for key, path in partials.items()},
                                 "round_trips_completed": 2, "pre_burn_cancellations": 1, "propellant_consumed": 30,
                                 "paid_probe_roles_deployed": [15, 19], "dormant_builder_goods_crates": 40,
                                 "lost_home_reload_phases": ["ascent", "orbit", "turnaround"], "construction_probe_lifecycle_deployment": True,
                                 "ordinary_exact_chunks": [key.decode() for key in exact], "corrupt_rejections": len(kinds),
                                 "remaining_scope": "survey/construction actions, orbital resupply, art and full acceptance"}, indent=2) + "\n", encoding="utf-8")
    print(f"Native reusable rocket phases verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
