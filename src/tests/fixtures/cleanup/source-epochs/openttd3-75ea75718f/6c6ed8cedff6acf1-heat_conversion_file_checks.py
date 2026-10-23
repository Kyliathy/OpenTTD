"""Run focused exact shared-conversion boundaries and genuine ordinary428 file checks.

@file heat_conversion_file_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8DB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
These numerical source/expense fixtures do not prove installed physical plant operation.
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

CONTEXT = current_run_context("geothermal", "atomic-conversion")
FAULTS = ("missing-header", "missing-table", "schema", "source-count", "origin-count", "oversized", "epoch",
          "foreign-column", "output", "foreign-output", "input", "efficiency", "numerator", "ratio",
          "capacity", "origin", "foreign-origin", "heat", "duplicate-metadata", "duplicate-origin", "metadata", "copied-owner")


def identity(path):
    """Identify exact actual source, binary, input or generated evidence.

    @param path Existing repository-owned file.
    @return Relative path, byte length and SHA256, preserving immutable original identity.
    """
    path = Path(path).resolve()
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def native(configuration, tag, label, environment=None):
    """Run one bounded isolated actual native boundary with complete diagnostics.

    @param configuration Actual compiled native owner.
    @param tag Exact focused Catch tag; a tag matching nothing cannot pass.
    @param label Unique diagnostic basename.
    @param environment Optional declared input/observer operation.
    @return Actual output and exact log-bound native counts.
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
    """Read exact original shared utility and conversion owner chunks.

    @param path Actual current writer input or ordinary native resave.
    @return All six complete source/consumer/slice/path/ratio/provenance byte authorities.
    """
    version, stream = read_stream(path)
    assert version >= 428, (path, version)
    names = {b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"UCVH", b"UCVD"}
    result = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}
    assert result.keys() == names, path
    return result


def read_case(configuration, source, stage, action, label, corrupt=None):
    """Exercise ordinary native loading, independent preview and full error recovery.

    @param configuration Actual native build owner.
    @param source Exact valid original or resave.
    @param stage Pending, fractional, electric or genuine427 legacy construction boundary.
    @param action Inspect, continue, legacy or independent preview/full refusal.
    @param label Unique diagnostic basename.
    @param corrupt Optional fresh corrupted copy, never its immutable original.
    @return Optional ordinary output and exact actual native proof.
    """
    startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md8DB.\nutility_conversion_check\n", encoding="utf-8")
    environment = {"OTTD_UTILITY_CONVERSION_INPUT": str(source), "OTTD_UTILITY_CONVERSION_STAGE": stage,
                   "OTTD_UTILITY_CONVERSION_ACTION": action}
    if corrupt is not None:
        environment["OTTD_UTILITY_CONVERSION_CORRUPT"] = str(corrupt)
    output, proof = native(configuration, "[.utility-conversion-native-file]", label, environment)
    proof.update({"input": identity(source), "stage": stage, "action": action})
    match = re.search(r"UTILITY_CONVERSION_READ .* path=([^\r\n]+)", output)
    saved = Path(match[1]) if match else None
    if saved is not None:
        assert source.resolve() != saved.resolve(), "Preserve the original input."
        proof["output"] = identity(saved)
    if corrupt is not None:
        proof["corrupt"] = identity(corrupt)
    return saved, proof


def malformed(source, fault):
    """Change one declared exact-conversion boundary in a fresh native copy.

    @param source Genuine electrical-output original, retaining1J/5 numerator/7J input.
    @param fault Exact header/scalar/reference/provenance/metadata boundary to attack.
    @return New generated corrupt file with unrelated chunks retained byte for byte.
    """
    version, stream = read_stream(source)
    assert version >= 428
    records = chunks(stream)
    header = next(row for row in records if row["name"] == b"UCVH")
    table = next(row for row in records if row["name"] == b"UCVD")
    assert len(header["payload"]) == 32 and len(table["rows"]) == 3
    if fault in ("missing-header", "missing-table"):
        (header if fault == "missing-header" else table)["remove"] = True
    elif fault in ("schema", "source-count", "origin-count", "oversized", "epoch"):
        row = bytearray(header["payload"])
        offset = {"schema": 0, "source-count": 8, "origin-count": 16, "oversized": 16, "epoch": 24}[fault]
        value = 99 if fault == "schema" else 0xffffffffffffffff if fault == "oversized" else 0 if fault != "epoch" else 3
        row[offset:offset + 8] = value.to_bytes(8, "big"); header["payload"] = bytes(row); header["edited"] = True
    elif fault == "foreign-column":
        assert table["rows"][0].count(b"numerator") == 1
        table["rows"][0] = table["rows"][0].replace(b"numerator", b"xumerator"); table["edited"] = True
    elif fault == "metadata":
        metadata = next(row for row in records if row["name"] == b"MLMD")
        row = bytearray(metadata["rows"][1]); flags = int.from_bytes(row[-20:-16], "big"); assert flags & 33554432
        row[-20:-16] = (flags & ~33554432).to_bytes(4, "big"); metadata["rows"][1] = bytes(row); metadata["edited"] = True
    elif fault == "copied-owner":
        original = next(row for row in records if row["name"] == b"UPLD")
        # Only actual original source/consumer owners change. Scalar IDs and
        # internal control agree, but Company1 is absent from the copied file.
        for index, payload in enumerate(original["rows"][1:], 1):
            row = bytearray(payload)
            if row[0] in (0, 1):
                assert row[9] == 0; row[9] = 1; original["rows"][index] = bytes(row)
        original["edited"] = True
    else:
        metadata, origin = bytearray(table["rows"][1]), bytearray(table["rows"][2])
        assert len(metadata) == len(origin) == 31 and metadata[0] == 0 and origin[0] == 1
        if fault == "output": metadata[1:5] = (2).to_bytes(4, "big")
        elif fault == "foreign-output": metadata[1:5] = bytes(4)
        elif fault == "input": metadata[5:9] = (1).to_bytes(4, "big")
        elif fault == "efficiency": metadata[9] = 101
        elif fault == "numerator": metadata[10] = 100
        elif fault == "ratio": metadata[10] = 6
        elif fault == "capacity": metadata[11:19] = (1).to_bytes(8, "big")
        elif fault == "origin": origin[19:23] = (2).to_bytes(4, "big")
        elif fault == "foreign-origin": origin[19:23] = (1).to_bytes(4, "big")
        elif fault == "heat": origin[23:31] = (8).to_bytes(8, "big")
        elif fault in ("duplicate-metadata", "duplicate-origin"):
            offset = 8 if fault == "duplicate-metadata" else 16
            header["payload"] = header["payload"][:offset] + (2).to_bytes(8, "big") + header["payload"][offset + 8:]; header["edited"] = True
            table["rows"].insert(2 if fault == "duplicate-metadata" else 3, bytes(metadata if fault == "duplicate-metadata" else origin))
        else: raise ValueError(fault)
        table["rows"][1] = bytes(metadata)
        table["rows"][3 if fault == "duplicate-metadata" else 2] = bytes(origin); table["edited"] = True
    encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"] for row in records if not row.get("remove")) + bytes(4)
    target = CONTEXT.saves / f"conversion-corrupt-{fault}.sav"
    target.write_bytes(source.read_bytes()[:8] + lzma.compress(encoded))
    return target


def main():
    """Execute one bounded active-owner burst and publish actual source/binary/input evidence.

    @return None; any failed native, original identity or owner-byte check prevents a passing report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("focused", "geometry", "write", "reload", "legacy", "corruption"), required=True)
    parser.add_argument("--writer-report", type=Path)
    parser.add_argument("--legacy", type=Path)
    parser.add_argument("--faults", nargs="+")
    args = parser.parse_args()
    receipt = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md8DB / archi-map-layers.md: actual shared numerical conversion/expense and ordinary files; physical powerhouse remains8DC.",
               "configuration": args.configuration, "phase": args.phase,
               "binary": identity(ROOT / f"build/{args.configuration}/openttd_test.exe"), "cases": []}
    if args.phase == "focused":
        # Fresh processes prevent earlier tests' globals from supplying a
        # missing original source, complete invoice or fractional numerator.
        for suffix in ("definition", "atomic", "fraction", "refusal", "reentry", "advanced", "origins", "stage", "zero", "capacity", "lifetime", "bypass"):
            _, proof = native(args.configuration, f"[utility-conversion-{suffix}]", f"conversion-{suffix}")
            receipt["cases"].append(proof)
        for tag in ("utility-heat-domain", "utility-power-mutation"):
            _, proof = native(args.configuration, f"[{tag}]", tag); receipt["cases"].append(proof)
    elif args.phase == "geometry":
        # Shared headers/build ownership changed. Refresh actual compiled
        # producers before their descriptors authorize the existing packed art.
        for label, tag in (
                ("processor", "[layer-processor-definition],[layer-processor-presentation],[layer-processor]"),
                ("station", "[conventional-station-art],[underground-facility]"),
                ("terminal", "[conventional-terminal-art],[underground-facility],[underground-loading]"),
                ("mine", "[layer-mine-definition],[layer-mine-art-geometry],[layer-mine-presentation]")):
            _, proof = native(args.configuration, tag, f"geometry-{label}"); proof["producer"] = label
            receipt["cases"].append(proof)
    elif args.phase == "write":
        output, proof = native(args.configuration, "[utility-conversion-save-file]", "conversion-native-writer"); receipt["cases"].append(proof)
        receipt["saves"] = {stage: {"heat": int(heat), "electricity": int(electric), "numerator": int(numerator), "cash": int(cash), **identity(path)}
                            for stage, heat, electric, numerator, cash, path in re.findall(
                                r"UTILITY_CONVERSION_SAVE stage=(\w+) heat=(\d+) electricity=(\d+) numerator=(\d+) cash=(\d+) path=([^\r\n]+)", output)}
        assert receipt["saves"].keys() == {"pending", "fractional", "electric"}
    elif args.phase == "legacy":
        assert args.legacy and read_stream(args.legacy)[0] == 427, "Use a genuine original427."
        saved, proof = read_case(args.configuration, args.legacy.resolve(), "legacy", "legacy", "genuine427-absence")
        # Empty conversion transport must preserve actual earlier native
        # construction stock, original claims, work and paid capital history.
        names = {b"INDY", b"GPGH", b"GPGD", b"GCSH", b"GCSD"}
        old = {row["name"]: row["raw"] for row in chunks(read_stream(args.legacy)[1]) if row["name"] in names}
        new = {row["name"]: row["raw"] for row in chunks(read_stream(saved)[1]) if row["name"] in names}
        assert old.keys() == names and old == new
        receipt["cases"].append(proof)
    else:
        assert args.writer_report
        writer = json.loads(args.writer_report.read_text(encoding="utf-8")); receipt["writer"] = identity(args.writer_report)
        # Preserve the exact writer inputs; a replaced file cannot inherit
        # prior expected quantities, numerator or complete company invoices.
        for source in writer["saves"].values():
            assert identity(ROOT / source["path"]) == {key: source[key] for key in ("path", "bytes", "sha256")}
        if args.phase == "reload":
            # Every original repeats through ordinary native loading; pending
            # use and fractional service continue without replaying old benefits.
            for stage, source in writer["saves"].items():
                original = ROOT / source["path"]
                first, proof = read_case(args.configuration, original, stage, "inspect", f"{stage}-inspect"); receipt["cases"].append(proof)
                second, proof = read_case(args.configuration, first, stage, "inspect", f"{stage}-repeat"); receipt["cases"].append(proof)
                assert owned(original) == owned(first) == owned(second), stage
                if stage != "electric":
                    continued, proof = read_case(args.configuration, original, stage, "continue", f"{stage}-continue"); receipt["cases"].append(proof)
                    result_stage = "fractional" if stage == "pending" else "electric"
                    _, proof = read_case(args.configuration, continued, result_stage, "inspect", f"{stage}-continued-load"); receipt["cases"].append(proof)
        else:
            assert args.faults and len(args.faults) <= 6 and set(args.faults) <= set(FAULTS), "Select at most six faults per focused burst."
            source = ROOT / writer["saves"]["electric"]["path"]
            # Fresh corrupt copies and isolated preview/full probes keep each
            # refusal independent of an earlier candidate or reset/recovery.
            for fault in args.faults:
                corrupt = malformed(source, fault)
                for action in ("preview-refusal", "full-refusal"):
                    _, proof = read_case(args.configuration, source, "electric", action, f"{fault}-{action}", corrupt); receipt["cases"].append(proof)
    receipt["assertions"] = sum(row["assertions"] for row in receipt["cases"])
    receipt["test_cases"] = sum(row["cases"] for row in receipt["cases"])
    report = CONTEXT.reports / f"conversion-{args.configuration}-{args.phase}.json"
    report.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8"); print(report, flush=True)


if __name__ == "__main__":
    main()
