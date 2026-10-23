"""Verify native typed orders and independent Space definitions without replacing retained saves."""
import argparse
import json
import os
import struct
import subprocess
from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream, fresh_world
from launch_center_checks import riff_location
from runner import current_run_context, run_native


def native(context, configuration, action, source=None, corrupt=None, reject=False):
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("space_orders_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_SPACE_ORDERS_ACTION=action)
    env.pop("OTTD_SPACE_ORDERS_SOURCE", None)
    env.pop("OTTD_SPACE_ORDERS_CORRUPT", None)
    if source is not None:
        env["OTTD_SPACE_ORDERS_SOURCE"] = str(source)
    if corrupt is not None:
        env["OTTD_SPACE_ORDERS_CORRUPT"] = str(corrupt)
    log = context.logs / "native-orders.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.space-orders]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=120)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        assert result.returncode != 0 and "All tests passed" not in evidence, log
        assert any(text in evidence.lower() for text in ("space content", "space order")), log
        return
    assert result.returncode == 0 and "All tests passed" in evidence and "SPACE_ORDERS native=true" in evidence, log
    if corrupt:
        assert "preview_reject=true authority_unchanged=true" in evidence, log
    return context.saves / "orders-partial-414.sav" if action == "prepare" else context.saves / "orders-complete-414.sav"


def damage(context, source, kind):
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 414
    name = b"SDEF" if kind.startswith("content-") else b"SPOR"
    start, body, length, end = riff_location(stream, name)
    changed = bytearray(stream)
    if kind.endswith("missing"):
        del changed[start:end]
    elif kind == "content-version":
        changed[body:body + 2] = struct.pack(">H", 99)
    elif kind == "content-signature":
        changed[body + 9] ^= 1
    elif kind == "counter":
        changed[body:body + 8] = bytes(8)
    elif kind == "generation":
        changed[body + 13:body + 21] = bytes(8)
    elif kind == "cursor":
        changed[body + 21] = 33
    elif kind == "started":
        changed[body + 22] = 2
    elif kind == "timer":
        changed[body + 24:body + 28] = bytes([255]) * 4
    elif kind == "count":
        changed[body + 30] = 33
    elif kind == "action":
        changed[body + 31] = 255
    elif kind == "target-kind":
        changed[body + 32] = 255
    elif kind == "body-generation":
        changed[body + 37:body + 45] = struct.pack(">Q", 1)
    elif kind == "role":
        changed[body + 31] = 1  # Structurally valid Survey on a real mining probe: physical load must reject.
    elif kind == "vehicle":
        changed[body + 9:body + 13] = struct.pack(">I", 999999)
    elif kind == "removed-target":
        changed[body + 33:body + 37] = struct.pack(">I", 90000)  # Valid monotonic target meaning, currently absent: recoverable block.
    else:
        raise ValueError(kind)
    target = context.saves / f"orders-{kind}.sav"
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--profiles-only", action="store_true", help="Verify new selected/Classic worlds and cold resave of the new families.")
    args = parser.parse_args()
    if args.profiles_only:
        context = current_run_context("space", "orders-profiles-verify")
        fresh_world(context, args.configuration)
        records = []
        for profile, version in (("multilayer-industry", 2), ("classic", 0)):
            owner = context.for_feature("space", "new-" + profile)
            source = owner.saves / "new-world.sav"
            _, before = chunks(source)
            assert struct.unpack(">H", before[b"SDEF"][3:5])[0] == version
            if version == 0:
                assert before[b"SDEF"][3:] == bytes(10)
            assert before[b"SPOR"][3:] == struct.pack(">QB", 1, 0)
            assert before[b"SVST"][3:] == before[b"LCOP"][3:] == bytes(1)
            assert before[b"SPSU"][3:] == before[b"SPFL"][3:] == bytes(1)
            assert before[b"SPKN"][3:] == bytes(2)
            assert before[b"SPOD"][3:] == before[b"SPDJ"][3:] == struct.pack(">QH", 1, 0)
            reload = context.for_feature("space", "orders-profile-" + profile + "-reload")
            ordinary(reload, {"name": profile, "snapshot": identity(source)}, args.configuration)
            after = chunks(reload.saves / "ordinary-resave.sav")[1]
            # fresh_world saves before the ordinary first human company is created.
            # Account conservation uses the retained owned-world tests, not this startup fixture.
            for key in (b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SDEF", b"SPOR", b"SVST", b"LCOP", b"ASTR", b"VEHS", b"CAPA"):
                assert before[key] == after[key], (profile, key)
            records.append({"profile": profile, "source": identity(source), "resave": identity(reload.saves / "ordinary-resave.sav"), "no_free_space_vehicles": True})
        report = context.reports / "space-content-profiles.json"
        report.write_text(json.dumps({"configuration": args.configuration, "profiles": records}, indent=2) + "\n", encoding="utf-8")
        print(f"Space content profiles verified: {report.relative_to(ROOT)}", flush=True)
        return
    context = current_run_context("space", "orders-verify")
    retained = ROOT / "src/tests/fixtures/demos/space/space-mining-approach-1990-405.sav"
    original = identity(retained)
    prepare = context.for_feature("space", "orders-build")
    built = native(prepare, args.configuration, "prepare")
    version, before = chunks(built)
    assert version == 414 and struct.unpack(">H", before[b"SDEF"][3:5])[0] == 2
    next_generation, count = struct.unpack(">QB", before[b"SPOR"][3:12])
    assert count == 3 and next_generation > 3
    outputs = []
    for scenario in ("orders-resume", "orders-replay"):
        output = native(context.for_feature("space", scenario), args.configuration, "continue", built)
        outputs.append(chunks(output)[1])
    prepared = chunks(prepare.saves / "orders-complete-414.sav")[1]
    exact = (b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF", b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR", b"STNN", b"ORDL")
    for key in exact:
        assert prepared[key] == outputs[0][key] == outputs[1][key], ("native continuation", key)
    owner = context.for_feature("space", "orders-ordinary")
    ordinary(owner, {"name": "orders", "snapshot": identity(built)}, args.configuration)
    after = chunks(owner.saves / "ordinary-resave.sav")[1]
    for key in exact:
        assert before[key] == after[key], ("ordinary paused resave", key)
    kinds = ("content-missing", "content-version", "content-signature", "orders-missing", "counter", "generation",
             "cursor", "started", "timer", "count", "action", "target-kind", "body-generation", "role", "vehicle")
    physical = {"role", "vehicle"}
    for kind in kinds:
        owner = context.for_feature("space", "orders-reject-" + kind)
        corrupt = damage(owner, built, kind)
        native(owner, args.configuration, "continue", corrupt, reject=True)
        if kind not in physical:
            native(context.for_feature("space", "orders-preview-" + kind), args.configuration, "continue", built, corrupt)
    owner = context.for_feature("space", "orders-removed")
    removed = damage(owner, built, "removed-target")
    native(owner, args.configuration, "blocked", removed)
    assert identity(retained) == original
    report = {"configuration": args.configuration, "retained": original, "interrupted": identity(built),
              "exact_prepared_and_reloaded_results": 3, "ordinary_exact_chunks": [k.decode() for k in exact],
              "native_corrupt_rejections": len(kinds), "independent_preview_rejections": len(kinds) - len(physical),
              "removed_target_cargo_preserved": True,
              "remaining_actions": "real survey, blueprint construction, unload/descent and paid service producers"}
    target = context.reports / "space-orders-verification.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Typed Space orders and content verified: {target.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
