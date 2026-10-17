"""Pair actual full/copied refusal of coherent malformed hauler refits, capacity and packet mass.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12
Only invocation-owned file copies change; original paid hulls and packet owners remain immutable inputs.
"""
import argparse
from pathlib import Path
import runpy
import struct
import json

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location
from artifact_paths import create_run_context

HERE = Path(__file__).parent
SHARED = runpy.run_path(str(HERE / "processor_freight_checks-1.py"))
TABLES = runpy.run_path(str(HERE / "heavy_joined_checks-2.py"))
FAULTS = ("cargo", "capacity", "quantity")


def hauler(raw):
    """Locate this exact original paid17 product through complete native receipt framing.

    @param raw Decompressed438 positive freight checkpoint, with original absent replacement intents.
    @return Actual current paid17 job's VehicleID and generation selected from its single factory's serial ordering.
    """
    _, body, _, end = riff_location(raw, b"SVST"); count = raw[body]; assert count <= 64 and body + 1 + 68 * count == end
    found = []
    # Every original fixed paid receipt keeps its own empty retirement-intent byte; arbitrary packet or name bytes cannot impersonate the actual17 factory product.
    for start in range(body + 1, end, 68):
        assert raw[start + 67] == 0
        if int.from_bytes(raw[start + 41:start + 43], "big") == 17:
            assert int.from_bytes(raw[start + 49:start + 51], "big") != 65535 and int.from_bytes(raw[start + 59:start + 67], "big") > 0
            found.append((int.from_bytes(raw[start + 49:start + 59], "big"), int.from_bytes(raw[start + 59:start + 67], "big"),
                          int.from_bytes(raw[start:start + 4], "big"), int.from_bytes(raw[start + 4:start + 12], "big")))
    # This exact retained producer has an earlier materialization and the current ordinary purchase at one factory. Its monotonic job serial, not model equality or a newest-file search, identifies the same completed job checked by the native observer.
    assert len(found) == 2 and len({row[0] for row in found}) == 1 and len({row[1] for row in found}) == 2
    selected = max(found, key=lambda row: row[1])
    return selected[2:]


def field(row, name):
    """Select exactly one original common scalar or list from its self-describing native row.

    @param row Original TABLES native extent and decoded field metadata.
    @param name Actual field suffix, preserving native container framing.
    @return Unique original position/count/width metadata.
    """
    result, = [value for value in row["fields"] if value["path"].endswith("." + name)]
    return result


def damage(context, source, fault):
    """Change one independently decoded original hauler boundary in a new malformed file.

    @param context Exclusive invocation-owned candidate destination.
    @param source Exact positive loaded438 native checkpoint; never overwritten.
    @param fault Canonical label refusal, invented headroom or excessive physical packet count.
    @return Explicit unchanged-version candidate path.
    """
    version, stream = read_stream(source); assert version == 438; raw = bytearray(stream); vehicle, _ = hauler(raw)
    row, = [r for r in TABLES["table_rows"](raw, b"VEHS") if r["index"] == vehicle]
    cargo, capacity, references = (field(row, name) for name in ("cargo_type", "cargo_cap", "cargo.packets"))
    assert cargo["width"] == 1 and capacity["width"] == 2 and references["width"] == 4 and references["count"] > 0
    actual_capacity = int.from_bytes(raw[capacity["position"]:capacity["end"]], "big"); assert actual_capacity == 20
    if fault == "cargo":
        _, body, _, end = riff_location(raw, b"CDIM"); assert body + 1 + raw[body] * 6 == end
        meanings = [(raw[p], bytes(raw[p + 1:p + 5]), raw[p + 5]) for p in range(body + 1, end, 6)]
        ordinal, _, weight = next(row for row in meanings if row[1] == b"PASS"); assert weight > 0
        # Coherent positive dimensions and headroom isolate canonical role admission: an undeclared cargo must refuse even when its physical count and mass fit.
        raw[cargo["position"]] = ordinal; raw[capacity["position"]:capacity["end"]] = (320 // weight).to_bytes(2, "big")
    elif fault == "capacity":
        raw[capacity["position"]:capacity["end"]] = (actual_capacity + 1).to_bytes(2, "big")
    else:
        assert fault == "quantity"
        packet = int.from_bytes(raw[references["position"]:references["position"] + 4], "big") - 1
        original, = [r for r in TABLES["table_rows"](raw, b"CAPA") if r["index"] == packet]
        count = next(value for value in original["fields"] if value["path"] == "count")
        # Only the actual uniquely held packet grows in this copied file; cached carrier capacity cannot hide its independently decoded positive excess mass.
        raw[count["position"]:count["end"]] = (actual_capacity + 1).to_bytes(2, "big")
    target = context.saves / ("malformed-hauler-" + fault + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + raw)
    return target


def main():
    """Execute one bounded independent actual copied/full refusal pair against an exact positive loaded parent."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--parent", type=Path, required=True)
    parser.add_argument("--fault", choices=FAULTS, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); parent = args.parent.resolve(); proof = SHARED["read"](parent)
    assert proof["configuration"] == args.configuration and proof["scope"] == "native" and proof["phase"] == "loaded" and proof["complete"]
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    context = create_run_context("space", "processor-freight-refusal"); context.prepare_runtime(); candidate = damage(context, source, args.fault)
    copied = SHARED["execute"](args.configuration, "preview", "loaded", parent, candidate)
    full = SHARED["execute"](args.configuration, "full-refusal", "loaded", parent, candidate)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, configuration=args.configuration, complete=True, fault=args.fault, source=identity(source),
                                          candidate=identity(candidate), parent=identity(parent), reports=[p.relative_to(ROOT).as_posix() for p in (copied, full)]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
