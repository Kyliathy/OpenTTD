"""Exercise native Surface launch facility construction, ordinary saves and corrupt transport."""
import argparse
import json
import os
import struct
import subprocess
from PIL import Image
from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream, Reader, CHUNK_RIFF, CHUNK_RECORDS
from runner import current_run_context, run_native, save_native_capture


def native(context, configuration, source=None, corrupt=None):
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("launch_center_test\n", encoding="utf-8")
    env = dict(os.environ)
    env.pop("OTTD_LAUNCH_CENTER_SOURCE", None)
    env.pop("OTTD_LAUNCH_CENTER_CORRUPT", None)
    if source is not None:
        env["OTTD_LAUNCH_CENTER_SOURCE"] = str(source)
    if corrupt is not None:
        env["OTTD_LAUNCH_CENTER_CORRUPT"] = str(corrupt)
    log = context.logs / "native-launch-center.log"
    try:
        with log.open("wb") as sink:
            run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.launch-center]"],
                       cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, env=env, check=True, timeout=120)
    except subprocess.CalledProcessError as error:
        raise RuntimeError(f"Native launch-center failure: {log}") from error
    text = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in text and "LAUNCH_CENTER native=true centers=1" in text, log
    if corrupt is not None:
        assert "LAUNCH_CENTER preview_reject=true authority_unchanged=true" in text, log
    for ppm in context.images.glob("*.ppm"):
        with Image.open(ppm) as raster:
            save_native_capture(context, raster, ppm, ppm.with_suffix(".png"))
    return context.saves / "launch-center-408.sav"


def riff_location(stream, wanted):
    reader = Reader(stream)
    while True:
        start = reader.pos
        name = reader.take(4)
        assert name != bytes(4), wanted
        mode = reader.byte()
        kind = mode & 15
        if kind == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            body = reader.pos
            reader.take(length)
            if name == wanted:
                return start, body, length, reader.pos
        elif kind in CHUNK_RECORDS:
            while True:
                length = reader.gamma()
                if not length:
                    break
                reader.take(length - 1)
        else:
            raise ValueError("unexpected native framing")


def damaged_copy(context, source, kind):
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 408
    start, body, length, end = riff_location(stream, b"LCTR")
    assert length == 25
    changed = bytearray(stream)
    if kind.startswith("hub-"):
        start, body, length, end = riff_location(stream, b"LCHB")
        assert length == 41
        if kind == "hub-missing":
            del changed[start:end]
        elif kind == "hub-counter":
            changed[body:body + 8] = bytes(8)
        elif kind == "hub-bound":
            changed[body + 8:body + 10] = struct.pack(">H", 65)
        elif kind == "hub-member-bound":
            changed[body + 20] = 33
        elif kind == "hub-duplicate":
            changed[body + 31:body + 33] = changed[body + 21:body + 23]
        elif kind == "hub-generation":
            changed[body + 12:body + 20] = struct.pack(">Q", 999)
        elif kind == "hub-orphan":
            changed[body + 31:body + 33] = struct.pack(">H", 63999)
        else:
            raise ValueError(kind)
    elif kind == "missing":
        del changed[start:end]
    elif kind == "counter":
        changed[body:body + 8] = bytes(8)
    elif kind == "rotation":
        changed[body + 24] = 4
    elif kind == "bound":
        changed[body + 8:body + 10] = struct.pack(">H", 65)
    elif kind == "orphan":
        changed[body + 10:body + 12] = struct.pack(">H", 63999)
    else:
        raise ValueError(kind)
    result = context.saves / f"corrupt-launch-{kind}.sav"
    result.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return result


def reject(context, configuration, source, kind):
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("launch_center_test\n", encoding="utf-8")
    log = context.logs / "native-rejection.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.launch-center]"],
            cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT,
            env=dict(os.environ, OTTD_LAUNCH_CENTER_SOURCE=str(source)), timeout=45)
    text = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode != 0 and "All tests passed" not in text, log
    assert any(reason in text.lower() for reason in ("launch-center", "launch centers", "launch-center record")), log
    print(f"Native corrupt launch-center rejection: {kind}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    context = current_run_context("space", "launch-center-verify")
    source = ROOT / "src/tests/fixtures/demos/space/space-survey-approach-1990-404.sav"
    original = identity(source)
    built = native(context.for_feature("space", "launch-center-build"), args.configuration)
    version, data = chunks(built)
    assert version == 408
    next_generation, count = struct.unpack(">QH", data[b"LCTR"][3:13])
    assert (next_generation, count) == (3, 1)
    assert len(data[b"LCTR"][3:]) == 25
    assert struct.unpack(">QH", data[b"LCHB"][3:13]) == (4, 1)
    assert len(data[b"LCHB"][3:]) == 41
    for scenario in ("launch-center-reload", "launch-center-replay"):
        owner = context.for_feature("space", scenario)
        result = native(owner, args.configuration, built)
        continued = chunks(result)[1]
        for key in (b"LCTR", b"LCHB", b"STNN", b"ASTR", b"VEHS", b"CAPA", b"PLYR"):
            assert continued[key] == data[key], (scenario, key)
    owner = context.for_feature("space", "launch-center-ordinary")
    ordinary(owner, {"name": "launch-center", "snapshot": identity(built)}, args.configuration)
    after = chunks(owner.saves / "ordinary-resave.sav")[1]
    assert after[b"LCTR"] == data[b"LCTR"] and after[b"LCHB"] == data[b"LCHB"] and after[b"STNN"] == data[b"STNN"]
    corrupt_kinds = ("missing", "counter", "rotation", "bound", "orphan",
                     "hub-missing", "hub-counter", "hub-bound", "hub-member-bound", "hub-duplicate", "hub-generation", "hub-orphan")
    for kind in corrupt_kinds:
        owner = context.for_feature("space", "launch-center-reject-" + kind)
        corrupt = damaged_copy(owner, built, kind)
        reject(owner, args.configuration, corrupt, kind)
        if kind not in ("orphan", "hub-generation", "hub-orphan"):  # Full native reference validation occurs after load, not in the bounded FIOS transport preview.
            native(context.for_feature("space", "launch-center-preview-" + kind), args.configuration, built, corrupt)
    assert identity(source) == original
    report = {"configuration": args.configuration, "source": original, "built": identity(built),
              "native_construction": True, "ordinary_exact_anchor_and_station": True,
              "independent_exact_reloads": 2, "corrupt_native_rejections": len(corrupt_kinds),
              "readonly_preview_rejections": len(corrupt_kinds) - 3, "saved_exclusive_members": 2, "rockets_or_cargo_delivery_claimed": False}
    target = context.reports / "launch-center-verification.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Launch-center native lifecycle and ordinary reload verified: {target.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
