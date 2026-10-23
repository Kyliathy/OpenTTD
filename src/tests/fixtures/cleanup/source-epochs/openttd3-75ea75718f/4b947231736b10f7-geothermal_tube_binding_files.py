"""Verify original geothermal capsule bindings through bounded ordinary native files.

@file geothermal_tube_binding_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BCC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BDDC; valid compression envelopes in malformed copies.
Private provider/binding transport is separate from powered work, switching and bootstrap.
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

AUTHORITY = {b"TUVH", b"TUVD", b"TGSH", b"TGSD", b"INDY", b"GPGH", b"GPGD", b"GCBH", b"GCBD", b"GCSH", b"GCSD",
             b"GHTH", b"GHTD", b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"UCVH", b"UCVD", b"DEPT"}
SOURCES = ("src/transport_tube.h", "src/transport_tube_geothermal.h", "src/transport_tube_geothermal.cpp",
           "src/transport_tube_runtime.cpp", "src/transport_tube_topology.cpp", "src/transport_tube_power.cpp",
           "src/utility_power.cpp", "src/utility_power_transfer.h", "src/saveload/transport_tube_vehicle_sl.cpp",
           "src/multilayer_state.cpp", "src/saveload/afterload.cpp", "src/saveload/vehicle_sl.cpp", "src/saveload/saveload.cpp", "src/saveload/saveload.h",
           "src/tests/geothermal_tube_capsule_fixture.h", "src/tests/transport_tube_geothermal_binding.cpp",
           "src/tests/transport_tube_geothermal_binding_load.cpp")


def world_chunks(path):
    """Read every complete current native chunk without treating references as energy.

    @param path Genuine original or ordinary current output.
    @return Complete unique-name raw transport.
    """
    version, stream = read_stream(path)
    assert version == 434, (path, version)
    rows = chunks(stream)
    result = {row["name"]: row["raw"] for row in rows}
    assert len(result) == len(rows) and AUTHORITY <= result.keys(), path
    return result


def scalar_digest(values):
    """Hash independently decoded semantic fields in the canonical fixed-byte order.

    @param values Unsigned original scalar values, each represented by eight little-endian bytes.
    @return Canonical uint64 FNV-1a, independent of the native executable's reported binding digest.
    """
    value = 14695981039346656037
    # Every original scalar contributes exactly eight bytes; native padding,
    # table encoding and host byte order cannot redefine the saved identity.
    for field in values:
        for byte in field.to_bytes(8, "little"):
            value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return value


def binding_digest(path, require_external=False):
    """Independently decode genuine older/defaulted or current binding references.

    @param path Genuine input or ordinary current resave, never a rewritten version header.
    @param require_external Require absent geothermal source/feed on every original external row.
    @return Canonical digest using explicit older MAX/MAX defaults and original prepared tick.
    """
    version, stream = read_stream(path)
    records = {row["name"]: row for row in chunks(stream)}
    if b"TUVH" not in records:
        assert version < 387 and b"TUVD" not in records
        return scalar_digest((0, (1 << 64) - 1))
    header = records[b"TUVH"]["payload"]
    rows = records[b"TUVD"]["rows"][1:]
    assert len(header) == 16 and int.from_bytes(header[4:8], "big") == len(rows)
    values = [len(rows), int.from_bytes(header[8:16], "big")]
    widths = (4, 4, 4, 2, 8, 8, 1, 1, 1, 4, 4) if version >= 434 else (4, 4, 4, 2, 8, 8, 1, 1, 1)
    # Rows retain original consumer order and every history/fraction field;
    # old external bindings acquire no implicit native source or feed.
    for row in rows:
        assert len(row) == sum(widths)
        fields, offset = [], 0
        for width in widths:
            fields.append(int.from_bytes(row[offset:offset + width], "big"))
            offset += width
        if version < 434:
            fields.extend(((1 << 32) - 1,) * 2)
        if require_external:
            assert fields[-2:] == [(1 << 32) - 1] * 2
        values.extend(fields)
    return scalar_digest(values)


def external_case(context, configuration):
    """Migrate actual386/387 external invoice/work history with explicit absent new references.

    @param context Reserved output/runtime/log owner.
    @param configuration Current compiled native configuration.
    @return Independent input, migration and two pure repeated-current observations per original.
    """
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    proofs = []
    # Both actual boundaries retain their original invoice and used energy.
    # The387 historical observer ticks its capsule, so only subsequent pure
    # observers are compared as complete repeated-current round trips.
    for version, basename, mode, output_name in (
            (386, "tube-power-consumed.sav", "consumed", "tube-power-consumed-reloaded.sav"),
            (387, "meter387-tube-meter-bound-service.sav", "bound-service", "tube-meter-bound-service.sav")):
        source = ROOT / "src/tests/fixtures/tubes" / basename
        original = identity(source)
        assert read_stream(source)[0] == version
        startup.write_text("# @see r2mlvs17-underground-transport-tubes.md6BCC.\ntube_power_check\n", encoding="utf-8")
        output, proof = native(context, configuration, "[.transport-tube-power-file]", f"external-{version}",
                               {"OTTD_TUBE_POWER_INPUT": str(source), "OTTD_TUBE_POWER_CASE": mode,
                                "OTTD_TUBE_POWER_OUTPUT_PREFIX": f"legacy{version}-"})
        saved = context.output("saves", f"legacy{version}-{output_name}")
        records = {row["name"]: row for row in chunks(read_stream(saved)[1])}
        assert records[b"TGSH"]["payload"] == bytes.fromhex("00000001") + bytes(16)
        assert len(records[b"TGSD"]["rows"]) == 1
        world_chunks(saved)
        ledger = re.search(r"TUBE_POWER_FILE .* ledger=([0-9A-F]+)", output)
        assert ledger is not None
        checkpoint = {"phase": 100, "source": scalar_digest((0, 0, 0)), "native": scalar_digest((0,)),
                      "binding": binding_digest(saved, True), "utility": int(ledger[1], 16)}
        proof.update({"input": original, "output": identity(saved), "original_version": version, "checkpoint": checkpoint})
        first, first_proof = read_case(context, configuration, checkpoint, saved, f"external-{version}-pure-first")
        second, second_proof = read_case(context, configuration, checkpoint, first, f"external-{version}-pure-second")
        assert world_chunks(first) == world_chunks(second), "Repeated external migration changed complete current chunks."
        assert binding_digest(first, True) == checkpoint["binding"]
        assert identity(source) == original
        proofs.extend((proof, first_proof, second_proof))
    return proofs


def read_case(context, configuration, checkpoint, source, label, action="inspect", corrupt=None):
    """Load and independently preview the exact original provider/binding owners.

    @param context Reserved runtime/output/log owner.
    @param configuration Actual compiled configuration.
    @param checkpoint Original phase and independently recorded digests.
    @param source Immutable original or previous ordinary output.
    @param label Unique diagnostic basename.
    @param action Inspect/resave, preview-refusal or full-refusal with recovery.
    @param corrupt Optional independent malformed copy.
    @return Optional actual current resave and native evidence.
    """
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md6BCC.\ngeothermal_binding_check\n", encoding="utf-8")
    environment = {"OTTD_GEOTHERMAL_BINDING_INPUT": str(source), "OTTD_GEOTHERMAL_BINDING_ACTION": action,
                   "OTTD_GEOTHERMAL_BINDING_PHASE": str(checkpoint["phase"]), "OTTD_GEOTHERMAL_BINDING_SOURCE": str(checkpoint["source"]),
                   "OTTD_GEOTHERMAL_BINDING_BINDING": str(checkpoint["binding"]), "OTTD_GEOTHERMAL_BINDING_UTILITY": str(checkpoint["utility"]),
                   "OTTD_GEOTHERMAL_BINDING_NATIVE": str(checkpoint["native"])}
    if corrupt is not None:
        environment["OTTD_GEOTHERMAL_BINDING_CORRUPT"] = str(corrupt)
    output, proof = native(context, configuration, "[.tube-geothermal-binding-native-file]", label, environment)
    proof.update({"input": identity(source), "action": action, "phase": checkpoint["phase"]})
    match = re.search(r"GEOTHERMAL_BINDING_READ phase=\d+ path=([^\r\n]+)", output)
    saved = Path(match[1]) if match else None
    if saved is not None:
        assert saved.resolve() != source.resolve()
        proof["output"] = identity(saved)
    if corrupt is not None:
        proof["corrupt"] = identity(corrupt)
    return saved, proof


def corrupt_copy(context, source, fault):
    """Change only a fresh current provider/binding copy.

    @param context Reserved output owner.
    @param source Genuine434 active original with two actual bindings.
    @param fault Missing-column, raw-boolean, wrong-source, wrong-feed or external-role mutation.
    @return Exact separately identifiable malformed input.
    """
    version, stream = read_stream(source)
    assert version == 434
    assert source.read_bytes()[:4] == b"OTTX"
    rows = chunks(stream)
    table = next(row for row in rows if row["name"] == b"TUVD")
    assert len(table["rows"]) == 3 and all(len(row) == 41 for row in table["rows"][1:])
    if fault == "missing-column":
        assert table["rows"][0].count(b"geothermal_source") == 1
        table["rows"][0] = table["rows"][0].replace(b"geothermal_source", b"xeothermal_source")
    else:
        row = bytearray(table["rows"][1])
        if fault == "raw-boolean":
            row[32] = 2
        elif fault == "wrong-source":
            row[33:37] = (1).to_bytes(4, "big")
        elif fault == "wrong-feed":
            row[37:41] = table["rows"][2][37:41]
        elif fault == "external-role":
            row[33:41] = bytes([255]) * 8
        else:
            raise ValueError(fault)
        table["rows"][1] = bytes(row)
    table["edited"] = True
    encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"] for row in rows) + bytes(4)
    destination = context.output("saves", f"bad-geothermal-binding-{fault}.sav")
    # Preserve the actual LZMA envelope and version; semantic field refusal
    # must not be replaced by rejecting LZMA bytes under a zlib file tag.
    destination.write_bytes(source.read_bytes()[:8] + lzma.compress(encoded))
    assert read_stream(destination) == (version, encoded)
    return destination


def main():
    """Execute one bounded actual writer, current checkpoint pair or independent malformed pair.

    @return None; any changed original, failed/empty native selection or raw repeat difference aborts acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("write", "reload", "refuse", "legacy", "external"), required=True)
    parser.add_argument("--writer-report", type=Path)
    parser.add_argument("--case", type=int, choices=range(5), default=0)
    parser.add_argument("--fault", choices=("missing-column", "raw-boolean", "wrong-source", "wrong-feed", "external-role"), default="wrong-source")
    args = parser.parse_args()
    context = current_run_context("tubes", "geothermal-bindings")
    context.prepare_runtime()
    report = {"_documentation": "r2mlvs17-underground-transport-tubes.md6BCC; actual434 source/provider binding transport; native powered work and switching remain later",
              "configuration": args.config, "phase": args.phase, "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
              "controller": identity(__file__), "sources": [identity(ROOT / name) for name in SOURCES]}
    if args.phase == "external":
        report["native"] = external_case(context, args.config)
    elif args.phase == "write":
        output, proof = native(context, args.config, "[tube-geothermal-binding-save-file]", "writer")
        # Capture each actual lifecycle checkpoint independently; a successful
        # writer without all five original reference scenes is insufficient.
        checkpoints = [{"phase": int(row[0]), "source": int(row[1]), "binding": int(row[2]), "utility": int(row[3]), "native": int(row[4]),
                        "input": identity(Path(row[5]))} for row in re.findall(
                            r"GEOTHERMAL_BINDING_SAVE phase=(\d+) source=(\d+) binding=(\d+) utility=(\d+) native=(\d+) path=([^\r\n]+)", output)]
        assert len(checkpoints) == 5
        for checkpoint in checkpoints:
            world_chunks(ROOT / checkpoint["input"]["path"])
        report.update({"native": [proof], "checkpoints": checkpoints})
    else:
        assert args.writer_report is not None
        writer = json.loads(args.writer_report.read_text(encoding="utf-8")); checkpoint = writer["checkpoints"][args.case].copy()
        source = ROOT / checkpoint["input"]["path"]; original = identity(source); assert original == checkpoint["input"]
        if args.phase == "legacy":
            assert read_stream(source)[0] == 433
            checkpoint.update({"phase": 100, "binding": binding_digest(source, True)})
        report.update({"writer": identity(args.writer_report), "case": args.case})
        if args.phase in ("reload", "legacy"):
            first, proof = read_case(context, args.config, checkpoint, source, "first")
            second, repeat = read_case(context, args.config, checkpoint, first, "second")
            before = {row["name"]: row["raw"] for row in chunks(read_stream(source)[1])}
            loaded, repeated = world_chunks(first), world_chunks(second)
            authority = AUTHORITY - {b"TUVD"} if args.phase == "legacy" else AUTHORITY
            assert all(before[name] == loaded[name] for name in authority), "Original provider/utility/native reference chunks changed."
            assert binding_digest(first, args.phase == "legacy") == binding_digest(source, args.phase == "legacy")
            assert loaded == repeated, "Repeated ordinary binding reload changed complete native chunks."
            report.update({"native": [proof, repeat], "owned_chunks_exact": len(authority), "repeat_complete_chunks_exact": len(loaded),
                           "first_changed_chunks": [name.decode() for name in before if before[name] != loaded.get(name)]})
        else:
            bad = corrupt_copy(context, source, args.fault); report.update({"fault": args.fault, "native": []})
            # Each malformed file gets independent preview/full processes;
            # successful recovery cannot rely on another observer's live state.
            for action in ("preview-refusal", "full-refusal"):
                _, proof = read_case(context, args.config, checkpoint, source, action, action, bad)
                report["native"].append(proof)
        assert identity(source) == original
    destination = context.output("reports", f"geothermal-binding-files-{args.config}-{args.phase}-{args.case}-{args.fault}.json")
    destination.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(destination, flush=True)


if __name__ == "__main__":
    main()
