"""Focused native powerhouse construction saves, ordinary continuations and malformed-file refusal.

@file geothermal_generator_file_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8C.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md artifact ownership.
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

CONTEXT = current_run_context("geothermal", "generator-construction")
STRUCTURAL = ("missing-header", "missing-table", "schema", "count", "oversized", "foreign-column",
              "model", "work", "receipt", "boolean", "owner", "site", "industry", "duplicate", "metadata",
              "copied-company", "copied-site", "copied-industry")


def identity(path):
    """Identify one immutable input or actual produced file.

    @param path Existing repository-owned source, binary, log or generated output.
    @return Exact relative path, length and SHA256.
    """
    path = Path(path).resolve()
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def native(configuration, tag, label, environment=None):
    """Run one isolated bounded native case and preserve actual failure diagnostics.

    @param configuration Actual native build owner.
    @param tag Exact focused Catch tag.
    @param label Unique diagnostic basename within the run.
    @param environment Optional declared original/corrupt input and observer operation.
    @return Actual output and hash-bound assertion/case evidence.
    """
    result = run_native(CONTEXT, [ROOT / "build" / configuration / "openttd_test.exe", tag],
                        cwd=ROOT / "build", env=dict(os.environ, **(environment or {})),
                        capture_output=True, timeout=110, check=False)
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    log = CONTEXT.logs / f"{label}.log"
    log.write_text(output + f"\nNATIVE_EXIT_CODE={result.returncode}\n", encoding="utf-8")
    assert result.returncode == 0 and "All tests passed" in output, log
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
    assert counts is not None, log
    print(f"{label}: {counts[1]} assertions / {counts[2]} cases", flush=True)
    return output, {"log": identity(log), "assertions": int(counts[1]), "cases": int(counts[2]), "exit_code": result.returncode}


def authority(path):
    """Read exact native inventory, original construction and cold-site owner chunks.

    @param path Genuine current writer output or ordinary resave.
    @return Exact construction/reference bytes, independent of a passing aggregate check.
    """
    version, stream = read_stream(path)
    assert version >= 427, (path, version)
    names = {b"INDY", b"GPGH", b"GPGD", b"GCSH", b"GCSD"}
    result = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}
    assert result.keys() == names, path
    return result


def read_case(configuration, source, action, expected, label, corrupt=None):
    """Use the actual ordinary executable, never copied in-memory construction restoration.

    @param configuration Native build owner.
    @param source Exact valid original or ordinary resave.
    @param action Inspect, continue, legacy, or a declared refusal/recovery probe.
    @param expected Expected original native work count.
    @param label Unique diagnostic basename.
    @param corrupt Optional fresh corrupt copy; its immutable original remains unchanged.
    @return Optional ordinary output plus exact native evidence.
    """
    startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md8C.\ngeothermal_generator_check\n", encoding="utf-8")
    environment = {"OTTD_GEOTHERMAL_GENERATOR_INPUT": str(source), "OTTD_GEOTHERMAL_GENERATOR_ACTION": action,
                   "OTTD_GEOTHERMAL_GENERATOR_EXPECTED": str(expected)}
    if corrupt is not None:
        environment["OTTD_GEOTHERMAL_GENERATOR_CORRUPT"] = str(corrupt)
    output, proof = native(configuration, "[.geothermal-generator-native-file]", label, environment)
    proof.update({"input": identity(source), "action": action, "expected_work": expected})
    match = re.search(r"GEOTHERMAL_GENERATOR_READ .* path=([^\r\n]+)", output)
    saved = Path(match[1]) if match else None
    if saved is not None:
        assert saved.resolve() != source.resolve(), "A resave must preserve its original input."
        proof["output"] = identity(saved)
    if corrupt is not None:
        proof["corrupt"] = identity(corrupt)
    return saved, proof


def malformed(source, fault):
    """Change only one declared generator transport boundary in a fresh copied save.

    @param source Genuine native partially built original.
    @param fault Exact structural/scalar/metadata or late native-clock attack.
    @return Fresh generated corrupt input, with all unrelated chunks retained byte for byte.
    """
    version, stream = read_stream(source)
    assert version >= 427, (source, version)
    records = chunks(stream)
    header = next(row for row in records if row["name"] == b"GPGH")
    table = next(row for row in records if row["name"] == b"GPGD")
    if fault in ("missing-header", "missing-table"):
        (header if fault == "missing-header" else table)["remove"] = True
    elif fault in ("schema", "count", "oversized"):
        row = bytearray(header["payload"]); assert len(row) == 8
        if fault == "schema":
            row[:4] = (99).to_bytes(4, "big")
        else:
            row[4:8] = (0 if fault == "count" else 0xffffffff).to_bytes(4, "big")
        header["payload"] = bytes(row); header["edited"] = True
    elif fault == "foreign-column":
        assert table["rows"][0].count(b"materials_committed") == 1
        table["rows"][0] = table["rows"][0].replace(b"materials_committed", b"xaterials_committed"); table["edited"] = True
    elif fault == "metadata":
        metadata = next(row for row in records if row["name"] == b"MLMD")
        row = bytearray(metadata["rows"][1]); flags = int.from_bytes(row[-20:-16], "big"); assert flags & 16777216
        row[-20:-16] = (flags & ~16777216).to_bytes(4, "big"); metadata["rows"][1] = bytes(row); metadata["edited"] = True
    else:
        row = bytearray(table["rows"][1]); assert len(row) == 20
        if fault == "model": row[7] = 255
        elif fault == "work": row[8:10] = (593).to_bytes(2, "big")
        elif fault == "receipt": row[10] = 0
        elif fault == "boolean": row[11] = 2
        elif fault == "owner": row[6] = 250
        elif fault == "site": row[2:6] = (0xffffffff).to_bytes(4, "big")
        elif fault == "industry": row[:2] = (64000).to_bytes(2, "big")
        # These IDs fit every scalar bound but are absent from this file's
        # copied/native owner sets, exercising reference validation separately.
        elif fault == "copied-company":
            assert row[6] == 0; row[6] = 1
        elif fault == "copied-site":
            assert int.from_bytes(row[2:6], "big") == 0; row[2:6] = (1).to_bytes(4, "big")
        elif fault == "copied-industry":
            assert int.from_bytes(row[:2], "big") == 0; row[:2] = (1).to_bytes(2, "big")
        elif fault == "future-tick": row[12:20] = (0xffffffffffffffff).to_bytes(8, "big")
        elif fault == "duplicate":
            table["rows"].append(bytes(row)); header["payload"] = header["payload"][:4] + (2).to_bytes(4, "big"); header["edited"] = True
        else: raise ValueError(fault)
        table["rows"][1] = bytes(row); table["edited"] = True
    encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"]
                       for row in records if not row.get("remove")) + bytes(4)
    target = CONTEXT.saves / f"generator-corrupt-{fault}.sav"
    target.write_bytes(source.read_bytes()[:8] + lzma.compress(encoded))
    return target


def main():
    """Execute one bounded writer/reload/refusal burst without changing immutable inputs.

    @return None; failed native or exact-owner assertions prevent a passing receipt.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("focused", "geometry", "mine-geometry", "write", "reload", "corruption", "legacy"), required=True)
    parser.add_argument("--writer-report", type=Path)
    parser.add_argument("--legacy", type=Path)
    parser.add_argument("--faults", nargs="+")
    args = parser.parse_args()
    receipt = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md8C / archi-map-layers.md: native construction persistence; boundary materials, not a driven first-plant proof.",
               "configuration": args.configuration, "phase": args.phase,
               "binary": identity(ROOT / f"build/{args.configuration}/openttd_test.exe"), "cases": []}
    if args.phase == "focused":
        # Run each owner boundary in a fresh process. The native test harness
        # has shared globals, so a combined Catch run could conceal bad setup.
        for suffix in ("definition", "funding", "refusal", "work", "work-locks", "stage",
                       "delivery", "delivery-excess", "wire", "takeover", "abandonment", "reuse"):
            _, proof = native(args.configuration, f"[geothermal-generator-{suffix}]", f"generator-{suffix}")
            receipt["cases"].append(proof)
    elif args.phase in ("geometry", "mine-geometry"):
        # Shared industry creation/intake changes require actual producer
        # evidence for existing packed layouts before their descriptors refresh.
        producers = (
                ("processor", "[layer-processor-definition],[layer-processor-presentation],[layer-processor]"),
                ("station", "[conventional-station-art],[underground-facility]"),
                ("terminal", "[conventional-terminal-art],[underground-facility],[underground-loading]"))
        if args.phase == "mine-geometry":
            producers = (("mine", "[layer-mine-definition],[layer-mine-art-geometry],[layer-mine-presentation]"),)
        for label, tag in producers:
            _, proof = native(args.configuration, tag, f"geometry-{label}")
            proof["producer"] = label
            receipt["cases"].append(proof)
    elif args.phase == "write":
        output, proof = native(args.configuration, "[geothermal-generator-save-file]", "generator-native-writer")
        receipt["cases"].append(proof)
        receipt["saves"] = {stage: {"work": int(work), "cash": int(cash), **identity(path)}
                            for stage, work, cash, path in re.findall(
                                r"GEOTHERMAL_GENERATOR_SAVE stage=(\w+) work=(\d+) cash=(\d+) path=([^\r\n]+)", output)}
        assert set(receipt["saves"]) == {"shortage", "partial", "complete"}, receipt
    elif args.phase == "legacy":
        assert args.legacy, "Legacy absence needs an actual retained older-format input."
        assert read_stream(args.legacy)[0] == 426, "Use a genuine original426, never a relabelled current save."
        saved, proof = read_case(args.configuration, args.legacy.resolve(), "legacy", 0, "genuine426-absence")
        # Introducing an empty generator family cannot rewrite the genuine
        # paid packet, source order, site or collector construction authorities.
        names = {b"INDY", b"SSOH", b"SSOR", b"CAPA", b"GCBH", b"GCBD", b"GCSH", b"GCSD"}
        original = {row["name"]: row["raw"] for row in chunks(read_stream(args.legacy)[1]) if row["name"] in names}
        resaved = {row["name"]: row["raw"] for row in chunks(read_stream(saved)[1]) if row["name"] in names}
        assert original.keys() == names and original == resaved, "Legacy paid owners changed while adding empty generator transport."
        proof["unchanged_legacy_owner_chunks"] = sorted(name.decode("ascii") for name in names)
        receipt["cases"].append(proof)
    else:
        assert args.writer_report, "A native reader needs the exact original writer report."
        writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
        receipt["writer"] = identity(args.writer_report)
        # Every reader uses the exact retained writer inputs. A replaced file
        # cannot inherit a prior report's expected work or paid cash receipt.
        for source in writer["saves"].values():
            assert identity(ROOT / source["path"]) == {key: source[key] for key in ("path", "bytes", "sha256")}
        if args.phase == "reload":
            # Each independent native boundary must survive repeated ordinary
            # loading, and unfinished originals must finish without repurchase.
            for stage, source in writer["saves"].items():
                original = ROOT / source["path"]
                first, proof = read_case(args.configuration, original, "inspect", source["work"], f"{stage}-inspect"); receipt["cases"].append(proof)
                second, proof = read_case(args.configuration, first, "inspect", source["work"], f"{stage}-repeat"); receipt["cases"].append(proof)
                assert authority(original) == authority(first) == authority(second), stage
                if stage != "complete":
                    continued, proof = read_case(args.configuration, original, "continue", source["work"], f"{stage}-continue"); receipt["cases"].append(proof)
                    _, proof = read_case(args.configuration, continued, "inspect", 592, f"{stage}-continued-load"); receipt["cases"].append(proof)
            if args.legacy:
                _, proof = read_case(args.configuration, args.legacy.resolve(), "legacy", 0, "genuine426-absence"); receipt["cases"].append(proof)
        else:
            faults = args.faults or list(STRUCTURAL) + ["future-tick"]
            assert set(faults) <= set(STRUCTURAL) | {"future-tick"}, faults
            source = ROOT / writer["saves"]["partial"]["path"]
            # Attack fresh copies independently so a rejected/partially loaded
            # candidate cannot hide the next fault or alter its valid original.
            for fault in faults:
                corrupt = malformed(source, fault)
                actions = ("native-refusal",) if fault == "future-tick" else ("preview-refusal", "full-refusal")
                # Structural preview refusal and actual full-load recovery are
                # separate guarantees; a future clock needs native afterload.
                for action in actions:
                    _, proof = read_case(args.configuration, source, action, 296, f"{fault}-{action}", corrupt); receipt["cases"].append(proof)
    receipt["assertions"] = sum(row["assertions"] for row in receipt["cases"])
    receipt["test_cases"] = sum(row["cases"] for row in receipt["cases"])
    report = CONTEXT.reports / f"generator-{args.configuration}-{args.phase}.json"
    report.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(report, flush=True)


if __name__ == "__main__":
    main()
