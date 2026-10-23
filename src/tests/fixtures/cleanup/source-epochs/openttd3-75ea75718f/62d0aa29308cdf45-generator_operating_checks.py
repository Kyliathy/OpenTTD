"""Run bounded actual native powerhouse operating checks with isolated diagnostics.

@file generator_operating_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8DC-8E.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Prepared floor/receiver-boundary materials do not claim the paid-TBM/carrier bootstrap.
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

CONTEXT = current_run_context("geothermal", "generator-operation")
MODEL = "vent-tap"  # Explicit independent expected model, never inferred from a potentially malformed save.
MAGMA_TAGS = ("geothermal-magma-construction", "geothermal-magma-locks", "geothermal-magma-service",
              "geothermal-magma-fractions", "geothermal-magma-lifecycle", "geothermal-magma-contention",
              "geothermal-magma-definition-capacity", "geothermal-magma-definition-foreign",
              "geothermal-generator-definition", "geothermal-generator-delivery", "geothermal-generator-delivery-excess")
TAGS = ("geothermal-generator-ports", "geothermal-generator-service", "geothermal-generator-operating-waits",
        "geothermal-generator-zero-heat", "geothermal-generator-operating-removal", "geothermal-generator-operating-wire",
        "geothermal-generator-operating-company", "geothermal-generator-no-thermal-domain",
        "geothermal-generator-contention", "geothermal-generator-view-equivalence")
FAULTS = ("missing-header", "missing-table", "schema", "count", "field", "input", "output", "fraction",
          "boolean", "billing", "port", "future-tick", "conversion-carry", "thermal-origin", "wrong-origin", "model-rate",
          "older-model", "model", "site-kind")


def identity(path):
    """Identify the exact producer or retained native evidence.

    @param path Existing repository-owned artifact.
    @return Relative path, byte count and SHA256.
    """
    path = Path(path).resolve()
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def native(configuration, tag, label, environment=None):
    """Run one focused native process, requiring actual cases and assertions.

    @param configuration Compiled native build owner.
    @param tag Exact isolated Catch tag.
    @param label Unique diagnostic basename within this invocation.
    @param environment Optional explicit ordinary-file startup observer inputs.
    @return Actual output and native log-bound counts.
    """
    result = run_native(CONTEXT, [ROOT / f"build/{configuration}/openttd_test.exe", tag], cwd=ROOT / "build",
                        env=dict(os.environ, **(environment or {})), capture_output=True, timeout=110, check=False)
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    log = CONTEXT.logs / f"{label}.log"
    log.write_text(output + f"\nNATIVE_EXIT_CODE={result.returncode}\n", encoding="utf-8")
    assert result.returncode == 0 and "All tests passed" in output, log
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
    assert counts, log
    print(f"{label}: {counts[1]} assertions / {counts[2]} cases", flush=True)
    return output, {"log": identity(log), "assertions": int(counts[1]), "cases": int(counts[2]), "exit_code": result.returncode}


def owned(path):
    """Read complete native construction/physical/shared operating authorities.

    @param path Actual genuine429 original or native resave.
    @return Exact owner chunks; repeat loading cannot silently renew any quantity.
    """
    version, stream = read_stream(path)
    assert version >= 429, (path, version)
    names = {b"INDY", b"GPGH", b"GPGD", b"GCBH", b"GCBD", b"GCSH", b"GCSD", b"GHTH", b"GHTD",
             b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"UCVH", b"UCVD"}
    result = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}
    assert result.keys() == names, path
    return result


def read_case(configuration, source, ticks, action, label, corrupt=None):
    """Exercise actual ordinary executable loading and its independent preview.

    @param configuration Actual compiled native owner.
    @param source Exact original or previously produced native resave.
    @param ticks Independent checkpoint0/1/3/4, blocked5 or repaired6; blocked time earns no native service.
    @param action Inspect, continue, preview-refusal or full-refusal.
    @param label Unique diagnostic basename.
    @param corrupt Optional fresh malformed copy, preserving the input.
    @return Optional native resave and exact log-bound proof.
    """
    startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md8DC.\ngeothermal_operation_check\n", encoding="utf-8")
    environment = {"OTTD_GEOTHERMAL_OPERATION_INPUT": str(source), "OTTD_GEOTHERMAL_OPERATION_ACTION": action,
                   "OTTD_GEOTHERMAL_OPERATION_TICKS": str(ticks), "OTTD_GEOTHERMAL_OPERATION_MODEL": MODEL}
    if corrupt is not None:
        environment["OTTD_GEOTHERMAL_OPERATION_CORRUPT"] = str(corrupt)
    output, proof = native(configuration, "[.geothermal-generator-operation-native-file]", label, environment)
    proof.update(input=identity(source), ticks=ticks, action=action)
    match = re.search(r"GEOTHERMAL_OPERATION_READ .* ticks=(\d+) path=([^\r\n]+)", output)
    saved = Path(match[2]) if match else None
    if saved is not None:
        assert saved.resolve() != source.resolve(), "Preserve original inputs."
        proof.update(output=identity(saved), output_ticks=int(match[1]))
    if corrupt is not None:
        proof["corrupt"] = identity(corrupt)
    return saved, proof


def corrupt_copy(source, fault):
    """Mutate one exact native operating boundary in a fresh generated file.

    @param source Genuine429 original with three committed common opportunities.
    @param fault Named header/scalar/reference/billing/physical/clock fault.
    @return New malformed path; unrelated ordinary owners retain exact bytes.
    """
    version, stream = read_stream(source)
    assert version >= 429
    records = chunks(stream)
    header = next(row for row in records if row["name"] == b"GPGH")
    table = next(row for row in records if row["name"] == b"GPGD")
    assert len(header["payload"]) == 8 and len(table["rows"]) == 2 and len(table["rows"][1]) == 46
    if fault in ("missing-header", "missing-table"):
        (header if fault == "missing-header" else table)["remove"] = True
    elif fault in ("schema", "count"):
        row = bytearray(header["payload"])
        offset, value = (0, 1) if fault == "schema" else (4, 0)
        row[offset:offset + 4] = value.to_bytes(4, "big"); header.update(payload=bytes(row), edited=True)
    elif fault == "field":
        assert table["rows"][0].count(b"service_remainder") == 1
        table["rows"][0] = table["rows"][0].replace(b"service_remainder", b"xervice_remainder"); table["edited"] = True
    elif fault == "port":
        routes = next(row for row in records if row["name"] == b"GHTD")
        row = bytearray(routes["rows"][1]); assert len(row) == 17 and row[0] == 0
        row[13:17] = (int.from_bytes(row[13:17], "big") + 1).to_bytes(4, "big")
        routes["rows"][1] = bytes(row); routes["edited"] = True
    elif fault in ("conversion-carry", "thermal-origin", "wrong-origin", "model-rate"):
        conversion = next(row for row in records if row["name"] == b"UCVD")
        metadata, origin = bytearray(conversion["rows"][1]), bytearray(conversion["rows"][2])
        assert len(metadata) == len(origin) == 31 and metadata[0] == 0 and origin[0] == 1
        assert metadata[10] == (80 if MODEL == "magma-loop" else 20) and int.from_bytes(origin[23:31], "big") == 8
        # These are the actual native plant's original shared conversion rows,
        # rather than an isolated numeric source fixture or private lineage copy.
        if fault == "conversion-carry": metadata[10] += 1
        elif fault == "thermal-origin": origin[23:31] = (9).to_bytes(8, "big")
        elif fault == "wrong-origin": origin[19:23] = (1).to_bytes(4, "big")
        else: metadata[11:19] = (150000 if MODEL == "magma-loop" else 350000).to_bytes(8, "big")
        conversion["rows"][1] = bytes(metadata); conversion["rows"][2] = bytes(origin); conversion["edited"] = True
    elif fault == "site-kind":
        cold = next(row for row in records if row["name"] == b"GCSD")
        # Retained site0 is the generator blueprint and site1 the collector.
        # Mutating only the original kind cannot turn either machine into the other.
        row = bytearray(cold["rows"][1]); assert row[9] == (3 if MODEL == "magma-loop" else 1)
        row[9] = 1 if MODEL == "magma-loop" else 3
        cold["rows"][1] = bytes(row); cold["edited"] = True
    elif fault == "older-model":
        assert MODEL == "magma-loop" and version >= 430, "Only the newly admitted model tests older semantic refusal."
    else:
        row = bytearray(table["rows"][1])
        if fault == "input": row[20:24] = (1).to_bytes(4, "big")
        elif fault == "output": row[24:28] = bytes(4)
        elif fault == "fraction": row[28] = 74
        elif fault == "boolean": row[29] = 2
        elif fault == "billing": row[38:46] = (4).to_bytes(8, "big")
        elif fault == "future-tick": row[30:38] = (0xffffffffffffffff).to_bytes(8, "big")
        elif fault == "model": row[7] = 0 if MODEL == "magma-loop" else 1
        else: raise ValueError(fault)
        table["rows"][1] = bytes(row); table["edited"] = True
    encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"] for row in records if not row.get("remove")) + bytes(4)
    target = CONTEXT.saves / f"powerhouse-corrupt-{fault}.sav"
    header = bytearray(source.read_bytes()[:8])
    if fault == "older-model": header[4:6] = (429).to_bytes(2, "big")
    target.write_bytes(header + lzma.compress(encoded))
    return target


def main():
    """Dispatch one bounded active-group burst and publish its complete receipt.

    @return None; fail immediately for a native refusal or a tag matching nothing.
    """
    global MODEL
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--model", choices=("vent-tap", "magma-loop"), default="vent-tap")
    parser.add_argument("--phase", choices=("focused", "foundation", "geometry", "write", "reload", "corruption", "legacy", "legacy-core",
                                         "construction-write", "construction-reload", "construction-foundation"), default="focused")
    parser.add_argument("--tags", nargs="+", choices=TAGS + MAGMA_TAGS)
    parser.add_argument("--writer-report", type=Path)
    parser.add_argument("--part", type=int, choices=(1, 2, 3), default=1,
                        help="Bound ordinary operating reloads to checkpoints(0,1), (3,4) or (5,6), keeping each burst below two minutes.")
    parser.add_argument("--faults", nargs="+", choices=FAULTS)
    parser.add_argument("--legacy", type=Path)
    args = parser.parse_args()
    MODEL = args.model
    proofs = []
    report = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md8DC; actual native operating boundary, prepared floor/materials",
              "configuration": args.config, "phase": args.phase, "model": MODEL, "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"), "proofs": proofs}
    if args.phase == "focused":
        # Isolated native processes prevent another case's fixture globals
        # from supplying any missing original construction/utility owner.
        for tag in args.tags or (MAGMA_TAGS if MODEL == "magma-loop" else TAGS):
            _, proof = native(args.config, f"[{tag}]", f"{args.config}-{tag}"); proofs.append(proof)
    elif args.phase == "construction-foundation":
        # The new canonical model and shared native admission must preserve
        # the original paid bill, construction/lifecycle and Surface bootstrap.
        for tag in ("geothermal-generator-funding", "geothermal-generator-refusal", "geothermal-generator-work",
                    "geothermal-generator-work-locks", "geothermal-generator-stage", "geothermal-generator-wire",
                    "geothermal-generator-takeover", "geothermal-generator-abandonment", "geothermal-generator-reuse",
                    "geothermal-definition", "multilayer-bootstrap"):
            _, proof = native(args.config, f"[{tag}]", f"construction-foundation-{tag}"); proofs.append(proof)
    elif args.phase == "foundation":
        # The changed common driver and shared retirement boundary must retain
        # existing collector, paid capsule and atomic utility behavior; this is
        # a focused consumer burst rather than a repository-wide regression.
        for tag in ("utility-conversion-atomic", "utility-conversion-bypass", "utility-heat-domain", "utility-power-mutation",
                    "geothermal-heat-source", "geothermal-heat-allocation", "transport-tube-meter", "orbital-service-contract"):
            _, proof = native(args.config, f"[{tag}]", f"foundation-{tag}"); proofs.append(proof)
    elif args.phase == "geometry":
        # Geometry descriptors must originate from actual current compiled
        # producers before their hashes authorize the unchanged packed art.
        for label, tag in (
                ("processor", "[layer-processor-definition],[layer-processor-presentation],[layer-processor]"),
                ("station", "[conventional-station-art],[underground-facility]"),
                ("terminal", "[conventional-terminal-art],[underground-facility],[underground-loading]"),
                ("mine", "[layer-mine-definition],[layer-mine-art-geometry],[layer-mine-presentation]")):
            _, proof = native(args.config, tag, f"geometry-{label}"); proof["producer"] = label; proofs.append(proof)
    elif args.phase == "write":
        tag = "[geothermal-magma-save-file]" if MODEL == "magma-loop" else "[geothermal-generator-operation-save-file]"
        output, proof = native(args.config, tag, f"{args.config}-writer"); proofs.append(proof)
        report["saves"] = {ticks: identity(path) for ticks, path in re.findall(r"GEOTHERMAL_OPERATION_SAVE ticks=(\d+) path=([^\r\n]+)", output)}
        assert report["saves"].keys() == {"0", "1", "3", "4", "5", "6"}
    elif args.phase == "construction-write":
        assert MODEL == "magma-loop", "The existing construction runner retains first-model coverage."
        output, proof = native(args.config, "[geothermal-magma-construction-save-file]", "construction-writer"); proofs.append(proof)
        report["saves"] = {stage: dict(identity(path), work=int(work)) for stage, work, path in re.findall(
            r"GEOTHERMAL_GENERATOR_SAVE stage=(\w+) work=(\d+) cash=\d+ path=([^\r\n]+)", output)}
        assert {stage: row["work"] for stage, row in report["saves"].items()} == {"shortage": 0, "partial": 296, "complete": 888}
    elif args.phase == "construction-reload":
        assert MODEL == "magma-loop" and args.writer_report
        writer = json.loads(args.writer_report.read_text(encoding="utf-8")); assert writer["model"] == MODEL
        report["writer_report"] = identity(args.writer_report)
        startup = CONTEXT.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
        startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md8E.\ngeothermal_generator_check\n", encoding="utf-8")
        # All three genuine originals repeat twice with identical native owners;
        # shortage/partial then continue only their remaining received materials/work.
        for stage, row in writer["saves"].items():
            source = ROOT / row["path"]; last = source
            for iteration in range(2):
                output, proof = native(args.config, "[.geothermal-generator-native-file]", f"construction-{stage}-{iteration}", {
                    "OTTD_GEOTHERMAL_GENERATOR_INPUT": str(last), "OTTD_GEOTHERMAL_GENERATOR_ACTION": "inspect",
                    "OTTD_GEOTHERMAL_GENERATOR_MODEL": MODEL, "OTTD_GEOTHERMAL_GENERATOR_EXPECTED": str(row["work"])})
                saved = Path(re.search(r"GEOTHERMAL_GENERATOR_READ .* path=([^\r\n]+)", output)[1])
                assert owned(source) == owned(saved); proof.update(input=identity(last), output=identity(saved)); proofs.append(proof); last = saved
            if stage != "complete":
                output, proof = native(args.config, "[.geothermal-generator-native-file]", f"construction-{stage}-continue", {
                    "OTTD_GEOTHERMAL_GENERATOR_INPUT": str(source), "OTTD_GEOTHERMAL_GENERATOR_ACTION": "continue",
                    "OTTD_GEOTHERMAL_GENERATOR_MODEL": MODEL, "OTTD_GEOTHERMAL_GENERATOR_EXPECTED": str(row["work"])})
                saved = Path(re.search(r"GEOTHERMAL_GENERATOR_READ .* work=888 .* path=([^\r\n]+)", output)[1])
                proof.update(input=identity(source), output=identity(saved)); proofs.append(proof)
                output, proof = native(args.config, "[.geothermal-generator-native-file]", f"construction-{stage}-continued-read", {
                    "OTTD_GEOTHERMAL_GENERATOR_INPUT": str(saved), "OTTD_GEOTHERMAL_GENERATOR_ACTION": "inspect",
                    "OTTD_GEOTHERMAL_GENERATOR_MODEL": MODEL, "OTTD_GEOTHERMAL_GENERATOR_EXPECTED": "888"})
                checked = Path(re.search(r"GEOTHERMAL_GENERATOR_READ .* path=([^\r\n]+)", output)[1])
                assert owned(saved) == owned(checked); proof.update(input=identity(saved), output=identity(checked)); proofs.append(proof)
    elif args.phase == "legacy-core":
        assert args.legacy and read_stream(args.legacy)[0] == 428, "Use the retained genuine428 electrical-conversion original."
        startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
        startup.parent.mkdir(parents=True, exist_ok=True)
        startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md8DC.\nutility_conversion_check\n", encoding="utf-8")
        source = args.legacy.resolve()
        output, proof = native(args.config, "[.utility-conversion-native-file]", "genuine428-conversion", {
            "OTTD_UTILITY_CONVERSION_INPUT": str(source), "OTTD_UTILITY_CONVERSION_STAGE": "electric",
            "OTTD_UTILITY_CONVERSION_ACTION": "inspect"})
        match = re.search(r"UTILITY_CONVERSION_READ .* path=([^\r\n]+)", output); assert match
        saved = Path(match[1]); old_version, old_stream = read_stream(source); new_version, new_stream = read_stream(saved)
        assert old_version == 428 and new_version >= 429
        old = {row["name"]: row for row in chunks(old_stream)}; new = {row["name"]: row for row in chunks(new_stream)}
        # The existing shared original-use/numerator/provenance owner is exact
        # across the later native-facility extension; no plant is fabricated.
        for name in (b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"UCVH", b"UCVD"):
            assert old[name]["raw"] == new[name]["raw"], name
        proof.update(input=identity(source), output=identity(saved)); proofs.append(proof)
    elif args.phase == "legacy":
        assert args.legacy and read_stream(args.legacy)[0] == 427, "Use the retained genuine427 partial powerhouse."
        startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
        startup.parent.mkdir(parents=True, exist_ok=True)
        startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md8DC.\ngeothermal_generator_check\n", encoding="utf-8")
        source = args.legacy.resolve()
        output, proof = native(args.config, "[.geothermal-generator-native-file]", "genuine427-construction", {
            "OTTD_GEOTHERMAL_GENERATOR_INPUT": str(source), "OTTD_GEOTHERMAL_GENERATOR_ACTION": "inspect",
            "OTTD_GEOTHERMAL_GENERATOR_EXPECTED": "296"})
        match = re.search(r"GEOTHERMAL_GENERATOR_READ .* path=([^\r\n]+)", output); assert match
        saved = Path(match[1]); old_version, old_stream = read_stream(source); new_version, new_stream = read_stream(saved)
        assert old_version == 427 and new_version >= 429
        old = {row["name"]: row for row in chunks(old_stream)}; new = {row["name"]: row for row in chunks(new_stream)}
        # The429 extension deliberately changes only this family's schema and
        # appended fields. Native stock and original cold identities are exact;
        # all original20 construction bytes survive, with no operating rights.
        for name in (b"INDY", b"GCSH", b"GCSD"):
            assert old[name]["raw"] == new[name]["raw"], name
        assert old[b"GPGH"]["payload"] == (1).to_bytes(4, "big") + (1).to_bytes(4, "big")
        assert new[b"GPGH"]["payload"] == (2).to_bytes(4, "big") + (1).to_bytes(4, "big")
        assert len(old[b"GPGD"]["rows"]) == len(new[b"GPGD"]["rows"]) == 2
        assert old[b"GPGD"]["rows"][1] == new[b"GPGD"]["rows"][1][:20]
        assert new[b"GPGD"]["rows"][1][20:] == bytes([255]) * 8 + bytes(18)
        proof.update(input=identity(source), output=identity(saved)); proofs.append(proof)
    else:
        assert args.writer_report, "Use actual retained writer evidence."
        writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
        assert writer.get("model", "vent-tap") == MODEL, "Choose the original writer's independently declared model."
        report["writer_report"] = identity(args.writer_report)
        if args.phase == "reload":
            # Repeated ordinary resaves preserve every complete selected owner;
            # unfinished originals then advance through the same common driver.
            # Six complete checkpoints exceeded two minutes on this host, so
            # each invocation owns only one explicitly numbered two-state part.
            selected = {1: (0, 1), 2: (3, 4), 3: (5, 6)}[args.part]
            report["part"] = args.part
            for ticks in selected:
                source = ROOT / writer["saves"][str(ticks)]["path"]
                first, proof = read_case(args.config, source, ticks, "inspect", f"reload-{ticks}-1"); proofs.append(proof)
                second, proof = read_case(args.config, first, ticks, "inspect", f"reload-{ticks}-2"); proofs.append(proof)
                assert owned(source) == owned(first) == owned(second), source
                if ticks in (0, 1, 3, 5):
                    continued, proof = read_case(args.config, source, ticks, "continue", f"continue-{ticks}"); proofs.append(proof)
                    target = {0: 1, 1: 3, 3: 4, 5: 6}[ticks]
                    _, proof = read_case(args.config, continued, target, "inspect", f"continued-{ticks}-read"); proofs.append(proof)
        else:
            assert args.faults and len(args.faults) <= 6, "Keep corruption bursts bounded."
            source = ROOT / writer["saves"]["3"]["path"]
            for fault in args.faults:
                bad = corrupt_copy(source, fault)
                # The actual synchronized clock is resolved only after native
                # world loading; copied scalar preview cannot borrow live time.
                for action in (("full-refusal",) if fault == "future-tick" else ("preview-refusal", "full-refusal")):
                    _, proof = read_case(args.config, source, 3, action, f"{fault}-{action}", bad); proofs.append(proof)
    report.update(case_runs=sum(p["cases"] for p in proofs), assertions=sum(p["assertions"] for p in proofs))
    suffix = f"-{args.part}" if args.phase == "reload" else ""
    path = CONTEXT.output("reports", f"generator-operation-{args.config}-{args.phase}{suffix}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
