"""Verify real paid passive shells, spacecraft stock/freight and interrupted service."""
import argparse
import json
import os
import struct
import subprocess
from PIL import Image
from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream
from launch_center_checks import riff_location
from runner import current_run_context, run_native, save_native_capture


def native(context, configuration, action, source=None, corrupt=None, reject=False):
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("launch_logistics_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_LAUNCH_LOGISTICS_ACTION=action)
    for name, value in (("OTTD_LAUNCH_LOGISTICS_SOURCE", source), ("OTTD_LAUNCH_LOGISTICS_CORRUPT", corrupt)):
        env.pop(name, None)
        if value is not None:
            env[name] = str(value)
    log = context.logs / "native-logistics.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.launch-logistics]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=120)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        assert result.returncode != 0 and "All tests passed" not in evidence, log
        assert any(word in evidence.lower() for word in ("space pod", "space manifest", "launch assembly", "space fleet")), log
        return
    assert result.returncode == 0 and "All tests passed" in evidence and "LAUNCH_LOGISTICS native=true" in evidence, log
    if corrupt:
        assert "preview_reject=true authority_unchanged=true" in evidence, log
    for ppm in context.images.glob("*.ppm"):
        with Image.open(ppm) as raster:
            save_native_capture(context, raster, ppm, ppm.with_suffix(".png"))
    return context.saves / ("logistics-partial-414.sav" if action == "prepare" else "logistics-complete-414.sav")


def damage(context, source, kind):
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 414
    family = b"SPSU" if kind.startswith("manifest-") else b"SPOD"
    start, body, length, end = riff_location(stream, family)
    changed = bytearray(stream)
    row = body + 10  # First native paid shell is aboard the mining probe.
    if kind.endswith("missing"):
        del changed[start:end]
    elif kind == "pod-count":
        changed[body + 8:body + 10] = struct.pack(">H", 1025)
    elif kind == "pod-next-generation":
        changed[body:body + 8] = bytes(8)
    elif kind == "pod-generation":
        changed[row + 2:row + 10] = bytes(8)
    elif kind == "pod-duplicate":
        changed[row + 65:row + 75] = changed[row:row + 10]
    elif kind == "pod-capital":
        changed[row + 60:row + 64] = bytes(4)
    elif kind == "pod-version":
        changed[row + 58:row + 60] = struct.pack(">H", 99)
    elif kind == "pod-carrier-generation":
        changed[row + 36:row + 44] = struct.pack(">Q", 999)
    elif kind == "pod-unowned-custody":
        changed[row + 11] = 1  # Claims center storage with no actual center owner.
    elif kind == "pod-boolean":
        changed[row + 64] = 2
    elif kind == "manifest-count":
        changed[body] = 65
    elif kind == "manifest-age":
        changed[body + 1 + 36:body + 1 + 38] = struct.pack(">H", 185)
    elif kind == "manifest-generation":
        changed[body + 1 + 4:body + 1 + 12] = bytes(8)
    elif kind == "manifest-pod-count":
        changed[body + 1 + 60] = 9
    else:
        raise ValueError(kind)
    target = context.saves / f"logistics-{kind}.sav"
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    context = current_run_context("space", "logistics-verify")
    retained = ROOT / "src/tests/fixtures/demos/space/space-launch-hub-1990-408.sav"
    original = identity(retained)
    prepare = context.for_feature("space", "logistics-prepare")
    partial = native(prepare, args.configuration, "prepare")
    version, before = chunks(partial)
    assert version == 414
    exact = (b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF", b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR", b"STNN", b"LCTR", b"LCHB", b"LHND")
    prepared = chunks(prepare.saves / "logistics-complete-414.sav")[1]
    for name in ("logistics-resume", "logistics-replay"):
        result = chunks(native(context.for_feature("space", name), args.configuration, "continue", partial))[1]
        for key in exact:
            assert prepared[key] == result[key], ("native interrupted continuation", key)
    owner = context.for_feature("space", "logistics-ordinary")
    ordinary(owner, {"name": "logistics", "snapshot": identity(partial)}, args.configuration)
    after = chunks(owner.saves / "ordinary-resave.sav")[1]
    for key in exact:
        assert before[key] == after[key], ("ordinary paused reload", key)
    kinds = ("pod-missing", "pod-count", "pod-next-generation", "pod-generation", "pod-duplicate", "pod-capital", "pod-version",
             "pod-carrier-generation", "pod-unowned-custody", "pod-boolean", "manifest-missing", "manifest-count", "manifest-age", "manifest-generation", "manifest-pod-count")
    for kind in kinds:
        owner = context.for_feature("space", "logistics-reject-" + kind)
        corrupt = damage(owner, partial, kind)
        native(owner, args.configuration, "continue", corrupt, reject=True)
        native(context.for_feature("space", "logistics-preview-" + kind), args.configuration, "continue", partial, corrupt)
    assert identity(retained) == original
    report = context.reports / "launch-logistics-verification.json"
    report.write_text(json.dumps({"configuration": args.configuration, "retained": original, "partial": identity(partial),
                                 "paid_shells": 10, "center_pod_cap": 8, "probe_pod_cap": 2, "probe_roles_carried": [15, 19, 20],
                                 "construction_goods_crates": 40, "rocket_payload_tonnes": 30, "service_consumed": {"RFMT": 1, "ELEC": 1},
                                 "ordinary_exact_chunks": [key.decode() for key in exact], "corrupt_rejections": len(kinds),
                                 "native_workshop_images": [identity(path) for path in sorted(prepare.images.glob("*.png"))],
                                 "remaining_scope": "role business actions, orbital resupply, art and full acceptance"}, indent=2) + "\n", encoding="utf-8")
    print(f"Native paid pod and supply logistics verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
