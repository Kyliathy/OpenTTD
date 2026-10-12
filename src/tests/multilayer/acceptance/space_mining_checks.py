"""Prepare and verify native owned mining probes through the real game/save writer."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import struct
from PIL import Image
from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream, Reader, CHUNK_RIFF, CHUNK_RECORDS
from runner import current_run_context, run_native, save_native_capture

DEST = ROOT / "src/tests/fixtures/demos/space"
MANIFEST = DEST / "mining-checkpoints.json"


def native(context, configuration, action, source=None):
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("space_mining_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_SPACE_MINING_ACTION=action)
    if source is not None:
        environment["OTTD_SPACE_MINING_SOURCE"] = str(source)
    log = context.logs / f"mining-{configuration}.log"
    with log.open("wb") as sink:
        run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.space-mining]"],
                   cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, env=environment, check=True, timeout=150)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence and "conserved=true holds=60 native=true" in evidence, log
    for ppm in context.images.glob("*.ppm"):
        with Image.open(ppm) as raster:
            save_native_capture(context, raster, ppm, ppm.with_suffix(".png"))
    print(f"Native mining {action}/{configuration}:2200 ticks conserved", flush=True)


def prepare(context, configuration):
    previous = json.loads(MANIFEST.read_text()) if MANIFEST.exists() else None
    assert previous is None or not previous.get("normal_starter_targets"), "retained current mining demo exists; choose fresh checkpoint names"
    source = DEST / "space-survey-approach-1990-404.sav"
    original = identity(source)
    native(context, configuration, "prepare")
    records = []
    for scene in ("approach", "working", "holds"):
        generated = context.saves / f"space-mining-{scene}-405.sav"
        version, data = chunks(generated)
        assert version == 405 and b"ASTR" in data and b"VEHS" in data and b"CAPA" in data
        target = DEST / f"space-mining-{scene}-1990-405.sav"
        assert not target.exists()
        if scene == "approach":
            _, before = chunks(source)
            for key in (b"GEOS", b"GDEP", b"GSPN", b"GVNT", b"ORDL", b"INDY", b"LMNH", b"LMNR", b"UPLH", b"UPLD"):
                assert before[key] == data[key], f"mining preparation changed Surface/underground owner {key}"
        shutil.copyfile(generated, target)
        records.append({"name": scene, "snapshot": identity(target), "paused": True, "layer": "Outer Space"})
    assert identity(source) == original
    MANIFEST.write_text(json.dumps({"save_version": 405, "calendar_year": 1990, "source": original,
        "native_probes": 3, "purchase_charge": 120000, "normal_starter_targets": True, "previous_preparation": previous, "finite_extraction": True, "surface_descent": False,
        "cargo_owner": "native VehicleCargoList/CAPA", "native_evidence": context.directory.relative_to(ROOT).as_posix(),
        "checkpoints": records, "user_playtest": "pending"}, indent=2) + "\n", encoding="utf-8")


def reject_corrupt(context, configuration, source, kind):
    """Use the real decoder to reject old byte6 and broken native lease references."""
    version, data = read_stream(source)
    changed = bytearray(data)
    reader = Reader(data)
    def gamma(value):
        assert value < 16384
        return bytes([value]) if value < 128 else struct.pack(">H", value | 0x8000)
    found = False
    while not found:
        name = reader.take(4)
        assert name != bytes(4)
        mode = reader.byte()
        chunk_type = mode & 15
        if chunk_type == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            start = reader.pos
            reader.take(length)
            if kind == "lease" and name == b"ASTR":
                changed[start + 51 + 36:start + 51 + 40] = bytes(4)  # Valid legacy VehicleID0 is not the mining probe.
                found = True
        elif chunk_type in CHUNK_RECORDS:
            first = True
            template = None
            while True:
                length = reader.gamma()
                if length == 0:
                    break
                start = reader.pos
                record = reader.take(length - 1)
                if kind == "old-byte6" and name == b"VEHS" and not first:
                    row = Reader(record)
                    row.gamma()
                    changed[start + row.pos] = 6
                    found = True
                    break
                if kind == "fleet-bound" and name == b"VEHS" and not first:
                    row = Reader(record)
                    row.gamma()
                    if record[row.pos] == 6:
                        template = record[row.pos:]
                first = False
            if kind == "fleet-bound" and name == b"VEHS":
                assert template is not None
                additions = b""
                for index in range(1000, 1064):
                    row = gamma(index) + template
                    additions += gamma(len(row) + 1) + row
                changed[reader.pos - 1:reader.pos - 1] = additions
                found = True
        else:
            raise ValueError("unexpected native framing")
    context.prepare_runtime()
    damaged = context.saves / f"corrupt-{kind}.sav"
    damaged.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    (context.runtime_dir / "scripts/game_start.scr").write_text("space_mining_test\n", encoding="utf-8")
    log = context.logs / "rejection.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.space-mining]"],
            cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT,
            env=dict(os.environ, OTTD_SPACE_MINING_ACTION="verify", OTTD_SPACE_MINING_SOURCE=str(damaged)), timeout=35)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode != 0 and "All tests passed" not in evidence, log
    expected = {"old-byte6": "Space vehicle in historical save", "lease": "Invalid native Space fleet", "fleet-bound": "Too many native Space vehicles"}[kind]
    assert expected in evidence, log
    print(f"Native decoder rejected {kind}/{configuration}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("prepare", "verify", "inspect", "exercise"))
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    args = parser.parse_args()
    context = current_run_context("space", "mining-" + args.mode)
    if args.mode == "exercise":
        # Exercise purchases, contention and loaded recall in fresh review outputs only.
        native(context, args.configuration, "prepare")
        return
    if args.mode == "prepare":
        prepare(context, args.configuration)
        return
    manifest = json.loads(MANIFEST.read_text())
    if args.mode == "verify":
        reject_corrupt(context.for_feature("space", "mining-reject-old"), args.configuration,
            ROOT / "src/tests/fixtures/demos/underground/mine-haulage-403.sav", "old-byte6")
        reject_corrupt(context.for_feature("space", "mining-reject-lease"), args.configuration,
            ROOT / next(row for row in manifest["checkpoints"] if row["name"] == "working")["snapshot"]["path"], "lease")
        reject_corrupt(context.for_feature("space", "mining-reject-bound"), args.configuration,
            ROOT / manifest["checkpoints"][0]["snapshot"]["path"], "fleet-bound")
    for scene in manifest["checkpoints"]:
        source = ROOT / scene["snapshot"]["path"]
        assert identity(source) == scene["snapshot"]
        owner = context.for_feature("space", "mining-" + args.mode + "-" + scene["name"])
        ordinary(owner, scene, args.configuration, graphical=args.mode == "inspect", inspector=args.mode == "inspect")
        if args.mode == "verify":
            first = context.for_feature("space", "mining-resume-" + scene["name"])
            second = context.for_feature("space", "mining-replay-" + scene["name"])
            native(first, args.configuration, "verify", source)
            native(second, args.configuration, "verify", source)
            a = chunks(first.saves / "space-mining-continued-405.sav")
            b = chunks(second.saves / "space-mining-continued-405.sav")
            for key in (b"ASTR", b"VEHS", b"CAPA", b"PLYR"):
                assert a[1][key] == b[1][key], f"native replay diverged in {key}"
            print(f"Exact deterministic native fleet/cargo/accounts replay: {scene['name']}", flush=True)


if __name__ == "__main__":
    main()
