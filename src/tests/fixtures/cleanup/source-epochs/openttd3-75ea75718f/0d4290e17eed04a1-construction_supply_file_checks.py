"""Actual purchased construction freight and ordinary unfinished-save continuations.

@file construction_supply_file_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CD.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md artifact ownership.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context, run_native
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks

CONTEXT = current_run_context("surface-supply", "construction")


def identity(path):
    """Identify an actual retained input, output or binary.

    @param path Existing file owned by the source checkout or generated run.
    @return Repository-relative path, length and exact SHA256.
    """
    path = Path(path).resolve()
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def native(configuration, tag, label, environment=None):
    """Run one isolated focused native case with a strict two-minute boundary.

    @param configuration Actual native build configuration.
    @param tag Exact Catch tag, including hidden ordinary-loader entry.
    @param label Unique diagnostic basename.
    @param environment Explicit observer inputs and expected independent money receipt.
    @return Native output plus retained exit/assertion/case evidence.
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
    print(f"{label}: {counts.group(1)} assertions / {counts.group(2)} cases", flush=True)
    return output, {"log": identity(log), "assertions": int(counts.group(1)),
                    "cases": int(counts.group(2)), "exit_code": result.returncode}


def authority(path):
    """Extract exact paid physical/history and native construction owner chunks.

    @param path Genuine writer output or its actual ordinary resave.
    @return Exact retained owner chunks; absent owners are also compared.
    """
    version, stream = read_stream(path)
    # Later admitted geothermal formats retain the original paid-purpose
    # chunks. Bound supported versions and still compare their complete bytes.
    assert 426 <= version <= 431, (path, version)
    names = {b"SSOH", b"SSOR", b"CAPA", b"GCBH", b"GCBD", b"GRSH", b"GRSD"}
    result = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}
    assert {b"SSOH", b"SSOR", b"CAPA"} <= result.keys(), path
    return result


def paid_materials(path):
    """Read actual frozen invoices and first-dispatch receipts from retained426-schema rows.

    @param path Genuine native writer checkpoint, never a synthesized inventory.
    @return Per-label quantities, frozen prices, dispatch and refund history.
    """
    version, stream = read_stream(path)
    # Formats427..431 add other owners, without changing this exact34-byte
    # paid invoice schema; a future format requires another explicit audit.
    assert 426 <= version <= 431, (path, version)
    table = next(row for row in chunks(stream) if row["name"] == b"SSOR")
    result = {}
    # Native LABEL uses its established U32 wire conversion. Every dense row
    # has the frozen34-byte schema; no array ordinal precedes the actual ID.
    for row in table["rows"][1:]:
        assert len(row) == 34, path
        label = row[7:11][::-1].decode("ascii")
        assert label not in result, (path, label)
        result[label] = {"quantity": int.from_bytes(row[19:21], "big"),
                         "unit_price": int.from_bytes(row[21:29], "big", signed=True),
                         "refunded": int.from_bytes(row[29:31], "big"),
                         "dispatched": int.from_bytes(row[31:33], "big")}
    return result


def read_case(configuration, source, stage, action, cash, label):
    """Load a genuine world with the ordinary application and continue only native owners.

    @param configuration Actual native build configuration.
    @param source Exact original or already accepted ordinary resave.
    @param stage Expected purchased, outbound, delivered, partial or complete boundary.
    @param action Inspect without gameplay mutation, or continue actual delivery/construction.
    @param cash Independent exact writer receipt, not the loaded file's asserted value.
    @param label Unique native diagnostic basename.
    @return Genuine new ordinary save plus actual native evidence.
    """
    startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs6-mines-and-resource-chains.md13CD.\nconstruction_supply_check\n", encoding="utf-8")
    output, proof = native(configuration, "[.surface-supply-construction-native]", label,
                           {"OTTD_CONSTRUCTION_INPUT": str(source), "OTTD_CONSTRUCTION_STAGE": stage,
                            "OTTD_CONSTRUCTION_ACTION": action, "OTTD_CONSTRUCTION_CASH": str(cash)})
    match = re.search(r"CONSTRUCTION_SUPPLY_NATIVE_SAVE .* path=([^\r\n]+)", output)
    assert match is not None, proof
    saved = Path(match.group(1))
    assert saved.resolve() != source.resolve(), "An ordinary resave must preserve its exact input."
    proof.update({"input": identity(source), "output": identity(saved), "stage": stage, "action": action})
    return saved, proof


def main():
    """Run a bounded writer or one-recipient ordinary-save burst.

    @return None; failed native or exact-owner checks raise before a passing receipt is published.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("write", "load"), required=True)
    parser.add_argument("--recipient", choices=("collector", "riser"))
    parser.add_argument("--writer-report", type=Path)
    args = parser.parse_args()
    receipt = {"owner_plan": "docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md#group-13cd---deliver-the-purchased-four-material-construction-bill",
               "architecture": "docs/zz-reach2/architecture/map-layers/archi-map-layers.md",
               "configuration": args.configuration, "phase": args.phase,
               "binary": identity(ROOT / "build" / args.configuration / "openttd_test.exe"),
               "prepared_floor_and_route": True, "paid_TBM_first_plant_bootstrap": False, "cases": []}
    if args.phase == "write":
        receipt["recipients"] = {}
        for recipient in ("collector", "riser"):
            output, proof = native(args.configuration, f"[surface-supply-construction-{recipient}]", f"{recipient}-writer")
            paths = {stage: Path(path) for stage, path in re.findall(
                rf"CONSTRUCTION_SUPPLY_SAVE name={recipient}-(\w+) .* path=([^\r\n]+)", output)}
            assert set(paths) == {"purchased", "outbound", "delivered", "partial", "complete"}, paths
            match = re.search(r"CONSTRUCTION_SUPPLY_RECEIPT .* units=(\d+) seed=(\d+) setup=(\d+) cash=(\d+) journey_ticks=(\d+)", output)
            assert match is not None, proof
            materials = paid_materials(paths["complete"])
            assert sum(row["quantity"] for row in materials.values()) == int(match[1]), materials
            assert sum(row["quantity"] * row["unit_price"] for row in materials.values()) == int(match[2]), materials
            for row in materials.values():
                assert row["dispatched"] == row["quantity"] and row["refunded"] == 0, materials
            receipt["recipients"][recipient] = {"units": int(match[1]), "seed": int(match[2]), "paid_materials": materials,
                "setup": int(match[3]), "cash": int(match[4]), "journey_ticks": int(match[5]),
                "saves": {stage: identity(path) for stage, path in paths.items()}}
            receipt["cases"].append(proof)
    else:
        assert args.recipient and args.writer_report, "Load bursts require a recipient and exact writer report."
        writer = json.loads(args.writer_report.read_text(encoding="utf-8"))
        assert writer["configuration"] == args.configuration, args.writer_report
        source = writer["recipients"][args.recipient]
        receipt["writer"] = identity(args.writer_report)
        for stage, record in source["saves"].items():
            original = ROOT / record["path"]
            assert identity(original) == record, original
            first, proof = read_case(args.configuration, original, stage, "inspect", source["cash"], f"{args.recipient}-{stage}-inspect")
            assert authority(original) == authority(first), (original, first)
            receipt["cases"].append(proof)
            second, proof = read_case(args.configuration, first, stage, "inspect", source["cash"], f"{args.recipient}-{stage}-repeat")
            assert authority(original) == authority(second), (original, second)
            receipt["cases"].append(proof)
            # Only unfinished checkpoints need active continuation. A complete
            # checkpoint already has its driven empty retry and no second bill.
            if stage != "complete":
                continued, proof = read_case(args.configuration, original, stage, "continue", source["cash"], f"{args.recipient}-{stage}-continue")
                receipt["cases"].append(proof)
                _, proof = read_case(args.configuration, continued, "complete", "inspect", source["cash"], f"{args.recipient}-{stage}-continued-load")
                receipt["cases"].append(proof)
    receipt["assertions"] = sum(row["assertions"] for row in receipt["cases"])
    receipt["test_cases"] = sum(row["cases"] for row in receipt["cases"])
    report = CONTEXT.reports / f"construction-{args.configuration}-{args.phase}-{args.recipient or 'both'}.json"
    report.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(report, flush=True)


if __name__ == "__main__":
    main()
