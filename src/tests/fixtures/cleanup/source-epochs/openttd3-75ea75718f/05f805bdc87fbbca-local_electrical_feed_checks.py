"""Verify genuine local electrical-feed files in bounded isolated native processes.

@file local_electrical_feed_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group9C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Each reload/refusal invocation selects one checkpoint or fault, preserving inputs.
"""

import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context, run_native
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks, encode_chunk

FIELDS = (("type", 1), ("id", 4), ("reference", 4), ("owner", 1), ("layer", 1), ("kind", 1),
          ("active", 1), ("epoch", 8), ("a", 8), ("b", 8), ("c", 8), ("d", 8), ("ticks", 4),
          ("remainder", 2), ("lifetime_generated", 8), ("lifetime_consumed", 8), ("transferred", 8),
          ("imported_link", 4), ("destination", 4), ("source_endpoint", 4), ("destination_endpoint", 4),
          ("town", 2), ("town_bonus", 1), ("town_applied", 1), ("town_suspended", 1), ("domain", 1),
          ("riser", 2), ("share", 4), ("parent", 4), ("town_terminal", 2), ("town_supply", 4),
          ("town_source_port", 4), ("generator", 2))
SOURCES = ("src/utility_power.h", "src/utility_power.cpp", "src/utility_power_transfer.h",
           "src/utility_power_lineage.cpp", "src/geothermal_utility.cpp", "src/transport_tube_power.cpp",
           "src/saveload/saveload.h", "src/saveload/utility_power_sl.cpp", "src/saveload/geothermal_generator_sl.cpp", "src/saveload/afterload.cpp",
           "src/tests/utility_power_local_feed.cpp", "src/tests/utility_power_local_feed_fixture.h",
           "src/tests/utility_power_local_feed_load.cpp", "src/tests/geothermal_generator_operating_load.cpp")
AUTHORITY = {b"INDY", b"GPGH", b"GPGD", b"GCBH", b"GCBD", b"GCSH", b"GCSD", b"GHTH", b"GHTD",
             b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"UCVH", b"UCVD", b"DEPT"}


def identity(path):
    """Identify one immutable input or actual source/result.

    @param path Existing repository-owned artifact.
    @return Relative path, byte count and SHA256.
    """
    path = Path(path).resolve()
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def native(context, configuration, tag, label, environment=None):
    """Run one isolated real native owner boundary.

    @param context Reserved output/runtime/diagnostic owner.
    @param configuration Actual compiled binary configuration.
    @param tag Exact bounded Catch selection.
    @param label Unique process diagnostic basename.
    @param environment Optional explicit source and independent expected values.
    @return Actual stdout and nonzero log-bound native counts.
    """
    result = run_native(context, [ROOT / f"build/{configuration}/openttd_test.exe", tag], cwd=ROOT / "build",
                        env=dict(os.environ, **(environment or {})), capture_output=True, timeout=110, check=False)
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    log = context.logs / f"{label}.log"
    log.write_text(output + f"\nNATIVE_EXIT_CODE={result.returncode}\n", encoding="utf-8")
    # Native failure and an empty selection both forbid an acceptance receipt.
    assert result.returncode == 0 and "All tests passed" in output, log
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
    assert counts is not None, log
    assert min(int(counts[1]), int(counts[2])) > 0, log
    print(f"{label}: {counts[1]} assertions / {counts[2]} cases", flush=True)
    return output, {"log": identity(log), "assertions": int(counts[1]), "cases": int(counts[2])}


def world_chunks(path):
    """Read every complete original native chunk without interpreting it as a balance.

    @param path Genuine current original or ordinary output.
    @return Complete raw chunks indexed by unique native four-byte name.
    """
    version, stream = read_stream(path)
    assert version == 432, (path, version)
    records = chunks(stream)
    result = {row["name"]: row["raw"] for row in records}
    assert len(result) == len(records), path
    assert AUTHORITY <= result.keys(), (path, AUTHORITY - result.keys())
    return result


def read_case(context, configuration, checkpoint, source, label, action="inspect", corrupt=None):
    """Load through actual executable startup and independently check preview.

    @param context Reserved runtime/output/diagnostic owner.
    @param configuration Actual native binary configuration.
    @param checkpoint Original writer phase and independent native/utility digests.
    @param source Immutable original or previous ordinary output.
    @param label Unique process diagnostic basename.
    @param action Inspect/resave, copied preview refusal or full refusal/recovery.
    @param corrupt Optional separately written malformed file.
    @return Optional ordinary resave and complete native evidence.
    """
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs12-solar-farms-mirrors-and-eti.md9C.\nlocal_feed_check\n", encoding="utf-8")
    environment = {"OTTD_LOCAL_FEED_INPUT": str(source), "OTTD_LOCAL_FEED_ACTION": action,
                   "OTTD_LOCAL_FEED_PHASE": str(checkpoint["phase"]), "OTTD_LOCAL_FEED_UTILITY": str(checkpoint["utility"]),
                   "OTTD_LOCAL_FEED_NATIVE": str(checkpoint["native"])}
    if corrupt is not None:
        environment["OTTD_LOCAL_FEED_CORRUPT"] = str(corrupt)
    output, proof = native(context, configuration, "[.utility-power-local-feed-native-file]", label, environment)
    proof.update({"input": identity(source), "action": action, "phase": checkpoint["phase"]})
    match = re.search(r"LOCAL_FEED_READ phase=\d+ path=([^\r\n]+)", output)
    saved = Path(match[1]) if match else None
    if saved is not None:
        assert saved.resolve() != source.resolve(), "Ordinary resaving must preserve the original input."
        proof["output"] = identity(saved)
    if corrupt is not None:
        proof["corrupt"] = identity(corrupt)
    return saved, proof


def corrupt_copy(context, source, fault):
    """Change only a fresh malformed transport copy, preserving its immutable original.

    @param context Reserved output owner for this fault.
    @param source Genuine432 original with nonzero local-feed lineage.
    @param fault Explicit missing-column, wrong-machine, wrong-port or cable-gap mutation.
    @return Hash-identifiable malformed path beneath this invocation's output home.
    """
    version, stream = read_stream(source)
    assert version == 432
    records = chunks(stream)
    table = next(row for row in records if row["name"] == b"UPLD")
    offsets = {}
    cursor = 0
    # Use the declared native scalar layout rather than guessing offsets from
    # a screenshot/digest; every mutation first checks the exact current width.
    for name, width in FIELDS:
        offsets[name] = (cursor, width)
        cursor += width
    assert all(len(row) == cursor for row in table["rows"][1:]), source
    if fault == "missing-column":
        assert table["rows"][0].count(b"generator") == 1
        table["rows"][0] = table["rows"][0].replace(b"generator", b"xenerator")
    else:
        # Select a declared feed for machine/port probes; the cable probe moves
        # the original shared middle cell, leaving a structurally valid plane.
        for index, raw in enumerate(table["rows"][1:], 1):
            if raw[offsets["type"][0]] != 4:
                continue
            expected = 1 if fault == "cable-gap" else 3
            if raw[offsets["kind"][0]] != expected:
                continue
            row = bytearray(raw)
            if fault == "wrong-machine":
                collector = next(record for record in records if record["name"] == b"GCBD")
                offset, width = offsets["generator"]
                row[offset:offset + width] = collector["rows"][1][:2]
            elif fault == "wrong-port":
                offset, width = offsets["source_endpoint"]
                row[offset:offset + width] = (int.from_bytes(row[offset:offset + width], "big") + 1).to_bytes(width, "big")
            elif fault == "cable-gap":
                if row[offsets["domain"][0]] != 0:
                    continue  # The coexisting Heat pipe is a separate network; corrupt only the electrical lineage cell.
                offset, width = offsets["source_endpoint"]
                tile = int.from_bytes(row[offset:offset + width], "big")
                if tile != 3 * 64 * 64 + 24 * 64 + 35:
                    continue
                for field in ("source_endpoint", "destination_endpoint"):
                    offset, width = offsets[field]
                    row[offset:offset + width] = (tile + 64).to_bytes(width, "big")
            else:
                raise ValueError(fault)
            table["rows"][index] = bytes(row)
            break
        else:
            raise AssertionError(f"No actual native target for {fault}")
    table["edited"] = True
    encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"] for row in records) + bytes(4)
    destination = context.output("saves", f"bad-local-feed-{fault}.sav")
    destination.write_bytes(b"OTTZ" + (version << 16).to_bytes(4, "big") + lzma.compress(encoded))
    return destination


def legacy_case(context, configuration, writer_path, ticks):
    """Verify a genuine pre432 operating input and its exact no-feed migration.

    @param context Reserved native runtime/output owner.
    @param configuration Actual current compiled binary.
    @param writer_path Immutable prior operating-writer receipt, including model and input identity.
    @param ticks Independent original nonzero operating checkpoint.
    @return Two ordinary native proofs and exact semantic/default/whole-chunk migration evidence.
    """
    writer = json.loads(writer_path.read_text(encoding="utf-8"))
    original = writer["saves"][str(ticks)]
    source = ROOT / original["path"]
    assert identity(source) == original, source
    version, stream = read_stream(source)
    assert version == 431, (source, version)
    old = {row["name"]: row for row in chunks(stream)}
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs12-solar-farms-mirrors-and-eti.md9C; genuine431 migration.\ngeothermal_operation_check\n", encoding="utf-8")
    current = source
    proofs = []
    outputs = []
    # The existing real operating checker independently verifies the old model,
    # original generation/service fractions and physical references on each load.
    for label in ("legacy-first", "legacy-repeat"):
        environment = {"OTTD_GEOTHERMAL_OPERATION_INPUT": str(current), "OTTD_GEOTHERMAL_OPERATION_ACTION": "inspect",
                       "OTTD_GEOTHERMAL_OPERATION_TICKS": str(ticks), "OTTD_GEOTHERMAL_OPERATION_MODEL": writer["model"]}
        output, proof = native(context, configuration, "[.geothermal-generator-operation-native-file]", label, environment)
        match = re.search(r"GEOTHERMAL_OPERATION_READ .* path=([^\r\n]+)", output)
        assert match is not None, output
        saved = Path(match[1])
        proof.update({"input": identity(current), "output": identity(saved)})
        outputs.append(saved)
        proofs.append(proof)
        current = saved
    loaded = world_chunks(outputs[0])
    repeated = world_chunks(outputs[1])
    assert loaded == repeated, "Repeated migrated native output changed complete chunks."
    assert all(old[name]["raw"] == loaded[name] for name in AUTHORITY - {b"UPLD"}), "Old operating authority changed."
    _, migrated_stream = read_stream(outputs[0])
    migrated = next(row for row in chunks(migrated_stream) if row["name"] == b"UPLD")
    assert migrated["rows"][1:] == [row + bytes(2) for row in old[b"UPLD"]["rows"][1:]], "Older rows gained nonzero feed references or changed quantities."
    assert identity(source) == original, "The genuine older input changed."
    return {"native": proofs, "older_writer": identity(writer_path), "model": writer["model"], "ticks": ticks,
            "older_version": version, "migrated_version": 432, "older_generator_defaults_zero": len(migrated["rows"]) - 1,
            "unchanged_owner_chunks": len(AUTHORITY) - 1, "repeat_complete_chunks_exact": len(loaded)}


def main():
    """Execute one bounded writer, checkpoint reload or explicit refusal pair.

    @return None; failed/empty native selection, changed input or owner drift aborts acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("write", "reload", "refuse", "legacy"), required=True)
    parser.add_argument("--writer-report", type=Path)
    parser.add_argument("--case", type=int, choices=range(4), default=1)
    parser.add_argument("--fault", choices=("missing-column", "wrong-machine", "wrong-port", "cable-gap"), default="wrong-machine")
    args = parser.parse_args()
    context = current_run_context("utilities", "local-feed")
    context.prepare_runtime()
    report = {"_documentation": "r2mlvs12-solar-farms-mirrors-and-eti.md9C; real native432 local-feed files; no capsule-work/bootstrap claim",
              "configuration": args.config, "phase": args.phase, "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
              "controller": identity(__file__), "sources": [identity(ROOT / name) for name in SOURCES]}
    if args.phase == "legacy":
        assert args.writer_report is not None, "Legacy proof requires its genuine older writer receipt."
        report.update(legacy_case(context, args.config, args.writer_report, args.case))
    elif args.phase == "write":
        output, proof = native(context, args.config, "[utility-power-local-feed-save-file]", "write")
        rows = re.findall(r"LOCAL_FEED_SAVE phase=(\d+) utility=(\d+) native=(\d+) path=([^\r\n]+)", output)
        assert len(rows) == 4, rows
        report["native"] = [proof]
        report["checkpoints"] = [{"phase": int(phase), "utility": int(utility), "native": int(native_digest), "input": identity(path)}
                                 for phase, utility, native_digest, path in rows]
    else:
        assert args.writer_report is not None, "Reload/refusal requires the exact writer receipt."
        writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
        checkpoint = next(row for row in writer["checkpoints"] if row["phase"] == args.case)
        source = ROOT / checkpoint["input"]["path"]
        assert identity(source) == checkpoint["input"], source
        report["writer"] = identity(args.writer_report)
        if args.phase == "reload":
            first, proof = read_case(context, args.config, checkpoint, source, "first")
            second, repeat = read_case(context, args.config, checkpoint, first, "second")
            original, loaded, repeated = world_chunks(source), world_chunks(first), world_chunks(second)
            assert all(original[name] == loaded[name] for name in AUTHORITY), "Original local/shared/native authorities changed."
            assert loaded == repeated, "Repeated native reload changed complete chunks."
            report.update({"native": [proof, repeat], "case": args.case, "owned_chunks_exact": len(AUTHORITY),
                           "first_changed_chunks": [name.decode() for name in original if original[name] != loaded.get(name)],
                           "repeat_complete_chunks_exact": len(loaded)})
        else:
            corrupt = corrupt_copy(context, source, args.fault)
            report.update({"fault": args.fault, "case": args.case, "native": []})
            # Separate processes prove copied-preview rejection and destructive
            # full-load rejection/recovery independently for the same bad copy.
            for action in ("preview-refusal", "full-refusal"):
                _, proof = read_case(context, args.config, checkpoint, source, action, action, corrupt)
                report["native"].append(proof)
        assert identity(source) == checkpoint["input"], "The retained original changed."
    path = context.output("reports", f"local-feed-{args.config}-{args.phase}-{args.case}-{args.fault}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
