"""Verify original geothermal tube source transport with bounded ordinary native processes.

@file geothermal_tube_source_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BBC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BDDC; valid compression envelopes in malformed copies.
Prepared native fixtures are distinct from paid bootstrap and capsule work.
"""
import argparse
import json
import lzma
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks, encode_chunk
from local_electrical_feed_checks import identity, native

AUTHORITY = {b"TGSH", b"TGSD", b"INDY", b"GPGH", b"GPGD", b"GCBH", b"GCBD", b"GCSH", b"GCSD",
             b"GHTH", b"GHTD", b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"UCVH", b"UCVD", b"DEPT"}
SOURCES = ("src/transport_tube_geothermal.h", "src/transport_tube_geothermal.cpp", "src/saveload/transport_tube_geothermal_sl.cpp",
           "src/tests/transport_tube_geothermal_load.cpp", "src/tests/geothermal_tube_power_fixture.h", "src/fios.h",
           "src/multilayer_state.h", "src/multilayer_state.cpp", "src/saveload/saveload.cpp", "src/saveload/saveload.h",
           "src/saveload/afterload.cpp", "src/utility_power.cpp", "src/geothermal_generator.cpp")


def world_chunks(path):
    """Read complete current native transport without inferring quantities.

    @param path Actual original or ordinary resave.
    @return Unique-name complete raw chunks.
    """
    version, stream = read_stream(path)
    assert version == 433, (path, version)
    records = chunks(stream)
    result = {row["name"]: row["raw"] for row in records}
    assert len(result) == len(records) and AUTHORITY <= result.keys(), path
    return result


def read_case(context, configuration, checkpoint, source, label, action="inspect", corrupt=None):
    """Load, independently preview, optionally refuse/recover, and ordinarily resave.

    @param context Reserved output/runtime/log owner.
    @param configuration Actual compiled configuration.
    @param checkpoint Independent original phase and expected digests.
    @param source Immutable ordinary native input.
    @param label Unique diagnostic basename.
    @param action Inspect, preview-refusal or full-refusal.
    @param corrupt Optional independently malformed copy.
    @return Optional resave and exact native evidence.
    """
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md6BBC.\ngeothermal_source_check\n", encoding="utf-8")
    environment = {"OTTD_GEOTHERMAL_SOURCE_INPUT": str(source), "OTTD_GEOTHERMAL_SOURCE_ACTION": action,
                   "OTTD_GEOTHERMAL_SOURCE_PHASE": str(checkpoint["phase"]), "OTTD_GEOTHERMAL_SOURCE_SOURCE": str(checkpoint["source"]),
                   "OTTD_GEOTHERMAL_SOURCE_UTILITY": str(checkpoint["utility"]), "OTTD_GEOTHERMAL_SOURCE_NATIVE": str(checkpoint["native"])}
    if corrupt is not None:
        environment["OTTD_GEOTHERMAL_SOURCE_CORRUPT"] = str(corrupt)
    output, proof = native(context, configuration, "[.tube-geothermal-source-native-file]", label, environment)
    proof.update({"input": identity(source), "action": action, "phase": checkpoint["phase"]})
    match = re.search(r"GEOTHERMAL_SOURCE_READ phase=\d+ path=([^\r\n]+)", output)
    saved = Path(match[1]) if match else None
    if saved is not None:
        assert saved.resolve() != source.resolve()
        proof["output"] = identity(saved)
    if corrupt is not None:
        proof["corrupt"] = identity(corrupt)
    return saved, proof


def corrupt_copy(context, source, fault):
    """Mutate exactly one fresh source-format copy, preserving the original.

    @param context Reserved output owner.
    @param source Genuine433 active original file.
    @param fault Missing-column, raw boolean, duplicate-root, wrong-plant or wrong-port.
    @return Hash-identifiable malformed input.
    """
    version, stream = read_stream(source)
    assert version == 433
    assert source.read_bytes()[:4] == b"OTTX"
    records = chunks(stream)
    table = next(row for row in records if row["name"] == b"TGSD")
    assert all(len(row) == 28 for row in table["rows"][1:])
    assert len(table["rows"]) == 3
    if fault == "missing-column":
        assert table["rows"][0].count(b"generator") == 1
        table["rows"][0] = table["rows"][0].replace(b"generator", b"xenerator")
    else:
        row = bytearray(table["rows"][1])
        if fault == "raw-boolean":
            row[27] = 2
        elif fault == "duplicate-root":
            row[10:14] = table["rows"][2][10:14]
        elif fault == "wrong-plant":
            collector = next(record for record in records if record["name"] == b"GCBD")
            row[4:6] = collector["rows"][1][:2]
        elif fault == "wrong-port":
            row[15:19] = (int.from_bytes(row[15:19], "big") + 1).to_bytes(4, "big")
        else:
            raise ValueError(fault)
        table["rows"][1] = bytes(row)
    table["edited"] = True
    encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"] for row in records) + bytes(4)
    destination = context.output("saves", f"bad-geothermal-source-{fault}.sav")
    # Preserve the genuine LZMA tag/version so native source-field admission,
    # rather than an unrelated decompression error, owns the refusal.
    destination.write_bytes(source.read_bytes()[:8] + lzma.compress(encoded))
    assert read_stream(destination) == (version, encoded)
    return destination


def empty_digest(zero_bytes=24):
    """Calculate the independently specified empty issuer/receipt/count digest.

    @param zero_bytes Explicit empty owner's scalar byte count; source has three uint64 fields, native generator one.
    @return Fixed-field FNV-1a for the specified empty owner.
    """
    value = 14695981039346656037
    for byte in bytes(zero_bytes):
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return value


def external_case(context, configuration):
    """Migrate genuine external-source and metered-binding files without inventing a geothermal source.

    @param context Reserved output/runtime/log owner.
    @param configuration Actual current native configuration.
    @return Independent original inputs and complete repeated-current native evidence.
    """
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    proofs = []
    # Both original format boundaries must retain their actual invoice/work
    # history while the new reference-only registry migrates empty.
    for version, basename, mode, output_name in (
            (386, "tube-power-consumed.sav", "consumed", "tube-power-consumed-reloaded.sav"),
            (387, "meter387-tube-meter-bound-service.sav", "bound-service", "tube-meter-bound-service.sav")):
        source = ROOT / "src/tests/fixtures/tubes" / basename
        original = identity(source)
        assert read_stream(source)[0] == version
        startup.write_text("# @see r2mlvs17-underground-transport-tubes.md6BBC.\ntube_power_check\n", encoding="utf-8")
        environment = {"OTTD_TUBE_POWER_INPUT": str(source), "OTTD_TUBE_POWER_CASE": mode,
                       "OTTD_TUBE_POWER_OUTPUT_PREFIX": f"legacy{version}-"}
        output, proof = native(context, configuration, "[.transport-tube-power-file]", f"external-{version}", environment)
        saved = context.output("saves", f"legacy{version}-{output_name}")
        records = chunks(read_stream(saved)[1])
        header = next(row for row in records if row["name"] == b"TGSH")
        table = next(row for row in records if row["name"] == b"TGSD")
        assert header["payload"] == bytes.fromhex("00000001") + bytes(16) and len(table["rows"]) == 1
        proof.update({"input": identity(source), "output": identity(saved), "original_version": version})
        proofs.append(proof)
        ledger = re.search(r"TUBE_POWER_FILE .* ledger=([0-9A-F]+)", output)
        assert ledger is not None
        checkpoint = {"phase": 4, "source": empty_digest(), "utility": int(ledger[1], 16), "native": empty_digest(8)}
        # The retained binding observer explicitly calls a native capsule Tick.
        # Its ordinary vehicle clock is therefore outside a pure round trip.
        # Subsequent generic observers perform no tick and compare every chunk.
        first, first_proof = read_case(context, configuration, checkpoint, saved, f"external-{version}-pure-first")
        second, second_proof = read_case(context, configuration, checkpoint, first, f"external-{version}-pure-second")
        proofs.extend((first_proof, second_proof))
        first_chunks = {row["name"]: row["raw"] for row in chunks(read_stream(first)[1])}
        second_chunks = {row["name"]: row["raw"] for row in chunks(read_stream(second)[1])}
        assert first_chunks == second_chunks, "Repeated external-source migration changed current native chunks."
        assert identity(ROOT / original["path"]) == original
    return proofs


def main():
    """Execute one bounded writer, reload pair, malformed pair or genuine older migration.

    @return None; changed inputs, empty/failing native selections and raw repeat differences abort acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("write", "reload", "refuse", "legacy", "external"), required=True)
    parser.add_argument("--writer-report", type=Path)
    parser.add_argument("--case", type=int, choices=range(4), default=0)
    parser.add_argument("--fault", choices=("missing-column", "raw-boolean", "duplicate-root", "wrong-plant", "wrong-port"), default="wrong-plant")
    args = parser.parse_args()
    context = current_run_context("tubes", "geothermal-source-files")
    context.prepare_runtime()
    report = {"_documentation": "r2mlvs17-underground-transport-tubes.md6BBC; actual433 original inlet native reference transport; no capsule-work/bootstrap claim",
              "configuration": args.config, "phase": args.phase, "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
              "controller": identity(__file__), "sources": [identity(ROOT / name) for name in SOURCES]}
    if args.phase == "external":
        report["native"] = external_case(context, args.config)
    elif args.phase == "write":
        output, proof = native(context, args.config, "[tube-geothermal-source-save-file]", "writer")
        checkpoints = [{"phase": int(row[0]), "source": int(row[1]), "utility": int(row[2]), "native": int(row[3]),
                        "input": identity(Path(row[4]))} for row in re.findall(
                            r"GEOTHERMAL_SOURCE_SAVE phase=(\d+) source=(\d+) utility=(\d+) native=(\d+) path=([^\r\n]+)", output)]
        assert len(checkpoints) == 4
        for checkpoint in checkpoints:
            world_chunks(ROOT / checkpoint["input"]["path"])
        report.update({"checkpoints": checkpoints, "native": [proof]})
    else:
        assert args.writer_report is not None
        writer = json.loads(args.writer_report.read_text())
        checkpoint = writer["checkpoints"][args.case].copy()
        if args.phase == "legacy":
            source = ROOT / checkpoint["input"]["path"]
            assert read_stream(source)[0] == 432
            checkpoint.update({"phase": 4, "source": empty_digest()})
        else:
            source = ROOT / checkpoint["input"]["path"]
        original_identity = identity(source)
        assert original_identity == checkpoint["input"]
        report.update({"writer": identity(args.writer_report), "case": args.case})
        if args.phase in ("reload", "legacy"):
            first, proof = read_case(context, args.config, checkpoint, source, "first")
            second, repeat = read_case(context, args.config, checkpoint, first, "second")
            loaded, repeated = world_chunks(first), world_chunks(second)
            assert loaded == repeated, "Repeated load changed complete native chunks."
            original = {row["name"]: row["raw"] for row in chunks(read_stream(source)[1])}
            authority = AUTHORITY if args.phase == "reload" else AUTHORITY - {b"TGSH", b"TGSD"}
            assert all(original[name] == loaded[name] for name in authority), "Original native/shared references changed."
            report.update({"native": [proof, repeat], "owned_chunks_exact": len(authority), "repeat_complete_chunks_exact": len(loaded),
                           "first_changed_chunks": [name.decode() for name in original if original[name] != loaded.get(name)]})
        else:
            corrupt = corrupt_copy(context, source, args.fault)
            report.update({"fault": args.fault, "native": []})
            # Independent processes prove copied preview and full refusal with
            # valid recovery; neither can borrow the other process's live state.
            for action in ("preview-refusal", "full-refusal"):
                _, proof = read_case(context, args.config, checkpoint, source, action, action, corrupt)
                report["native"].append(proof)
        assert identity(source) == original_identity
    destination = context.output("reports", f"geothermal-source-files-{args.config}-{args.phase}-{args.case}-{args.fault}.json")
    destination.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(destination, flush=True)


if __name__ == "__main__":
    main()
