"""Focused native paid Surface save, corruption, cancellation and older-absence proof.

@file surface_supply_file_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CC.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md artifact ownership.
"""

import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context, run_native

CONTEXT = current_run_context("surface-supply", "native-files")
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks, encode_chunk


def native(configuration, tag, label, environment=None):
    """Run one isolated bounded native case and retain its actual exit status.

    @param configuration Actual Debug or RelWithDebInfo binary owner.
    @param tag Exact focused Catch tag, including hidden ordinary-reader tags.
    @param label Unique diagnostic basename for this invocation.
    @param environment Optional exact input/action/corruption values.
    @return Native success evidence and assertion/case counts.
    """
    log = CONTEXT.logs / f"{label}.log"
    result = run_native(CONTEXT, [ROOT / "build" / configuration / "openttd_test.exe", tag],
                        cwd=ROOT / "build", env=dict(os.environ, **(environment or {})),
                        capture_output=True, timeout=110, check=False)
    evidence = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    log.write_text(evidence + f"\nNATIVE_EXIT_CODE={result.returncode}\n", encoding="utf-8")
    assert result.returncode == 0 and "All tests passed" in evidence, log
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", evidence)
    assert counts is not None, log
    print(f"{label}: {counts.group(1)} assertions, {counts.group(2)} cases", flush=True)
    return {"path": log.relative_to(ROOT).as_posix(), "assertions": int(counts.group(1)),
            "cases": int(counts.group(2)), "exit_code": result.returncode}


def read_case(configuration, source, mode, label, corrupt=None):
    """Use the actual ordinary loader and accepted-world observer, never a copied in-memory history.

    @param configuration Actual native binary owner.
    @param source Genuine original or ordinary resave.
    @param mode Native observer operation: inspect, continue, continued, legacy or refusal.
    @param label Unique log name preserving the exact input/mode attempt.
    @param corrupt Optional fresh test-owned corrupt copy; immutable source stays untouched.
    @return Native result evidence.
    """
    environment = {"OTTD_SURFACE_SUPPLY_INPUT": str(source), "OTTD_SURFACE_SUPPLY_ACTION": mode}
    if corrupt is not None:
        environment["OTTD_SURFACE_SUPPLY_CORRUPT"] = str(corrupt)
    startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# @see r2mlvs6-mines-and-resource-chains.md13CC native save observer.\nsurface_supply_check\n", encoding="utf-8")
    return native(configuration, "[.surface-supply-native-file]", label, environment)


def paid_authority(path):
    """Read exact saved history and native packet fields, independently of a successful load's aggregate checks.

    @param path Genuine426 source or ordinary resave, never a repaired copy of a corrupt file.
    @return Exact three owner chunks; loss of any physical unit or purpose field changes this comparison.
    """
    version, stream = read_stream(path)
    assert version == 426, (path, version)
    owners = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in (b"SSOH", b"SSOR", b"CAPA")}
    assert len(owners) == 3, path
    return owners


def malformed(source, fault):
    """Change one paid-family boundary in a fresh copy, retaining unrelated chunks byte for byte.

    @param source Genuine426 save written by the actual native stock/history owners.
    @param fault Exact presence, bound, schema, history or native-purpose mismatch.
    @return New malformed save under this invocation's generated review directory.
    """
    version, stream = read_stream(source)
    assert version == 426, (source, version)
    records = chunks(stream)
    header = next(row for row in records if row["name"] == b"SSOH")
    table = next(row for row in records if row["name"] == b"SSOR")
    if fault in ("missing-header", "missing-table"):
        (header if fault == "missing-header" else table)["remove"] = True
    elif fault in ("schema", "count", "cursor", "oversized"):
        data = bytearray(header["payload"])
        assert len(data) == 12
        if fault == "schema":
            data[:4] = (2).to_bytes(4, "big")
        elif fault == "count":
            data[4:8] = (3).to_bytes(4, "big")
            data[8:12] = (4).to_bytes(4, "big")
        elif fault == "cursor":
            data[8:12] = (1).to_bytes(4, "big")
        else:
            data[4:8] = (65536).to_bytes(4, "big")
            data[8:12] = (65537).to_bytes(4, "big")
        header["payload"] = bytes(data); header["edited"] = True
    elif fault == "foreign-column":
        assert table["rows"][0].count(b"dispatched") == 1
        table["rows"][0] = table["rows"][0].replace(b"dispatched", b"dispatchxx")
        table["edited"] = True
    elif fault in ("packet-purpose", "packet-dispatch", "packet-both"):
        packets = next(row for row in records if row["name"] == b"CAPA")
        columns = (b"company_supply_order", b"company_supply_shipped") if fault == "packet-both" else (
            b"company_supply_order" if fault == "packet-purpose" else b"company_supply_shipped",)
        # Missing both columns must not turn all bought stock into ordinary
        # freight by defaulting purpose and dispatch simultaneously.
        for column in columns:
            assert packets["rows"][0].count(column) == 1
            packets["rows"][0] = packets["rows"][0].replace(column, b"x" + column[1:])
        packets["edited"] = True
    elif fault in ("packet-order", "packet-count", "packet-flag"):
        packets = next(row for row in records if row["name"] == b"CAPA")
        row = bytearray(packets["rows"][1])
        assert len(row) == 32 and int.from_bytes(row[27:31], "big") == 1, row.hex()
        if fault == "packet-order":
            row[27:31] = (999).to_bytes(4, "big")
        elif fault == "packet-count":
            row[8:10] = (11).to_bytes(2, "big")
        else:
            row[31] = 1
        packets["rows"][1] = bytes(row); packets["edited"] = True
    else:
        row = bytearray(table["rows"][1])
        # The native dense Table transport owns ordinals outside each payload;
        # this row is exactly the declared34-byte paid history, without a copied index.
        assert len(row) == 34, (len(row), row.hex())
        if fault == "identity":
            row[0:4] = (0).to_bytes(4, "big")
        elif fault == "refund":
            row[29:31] = (11).to_bytes(2, "big")
        elif fault == "dispatch":
            row[31:33] = (1).to_bytes(2, "big")
        elif fault == "native-label":
            row[7:11] = b"TMFR"  # Native LABEL/U32 wire encoding of RFMT; actual first-order packets remain Steel.
        else:
            raise ValueError(fault)
        table["rows"][1] = bytes(row); table["edited"] = True
    encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"]
                       for row in records if not row.get("remove")) + bytes(4)
    target = CONTEXT.saves / f"paid-surface-{fault}.sav"
    target.write_bytes(source.read_bytes()[:8] + lzma.compress(encoded))
    return target


def main():
    """Execute focused save boundaries with explicit original/binary identities and no fixture writes.

    @return None; failed native cases retain their precise logs before raising.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="Debug")
    parser.add_argument("--phase", choices=("reload", "corruption", "all"), default="all")
    parser.add_argument("--source", type=Path, help="Exact previously generated426 input; otherwise write it natively")
    parser.add_argument("--legacy", type=Path, help="Exact genuine pre426 input for absence compatibility")
    parser.add_argument("--faults", nargs="+", help="Explicit corruption subset for a bounded continuation")
    args = parser.parse_args()
    CONTEXT.prepare_runtime()
    evidence = []
    if args.source is None:
        evidence.append(native(args.configuration, "[surface-supply-save-file]", "write-source"))
        source = CONTEXT.saves / "paid-source-stock-426.sav"
    else:
        source = args.source.resolve(strict=True)
    original = hashlib.sha256(source.read_bytes()).hexdigest()
    if args.phase in ("reload", "all"):
        original_authority = paid_authority(source)
        evidence.append(read_case(args.configuration, source, "inspect", "ordinary-first-load"))
        resave = CONTEXT.saves / "paid-surface-inspect.sav"
        assert paid_authority(resave) == original_authority, "ordinary load changed paid history or native packet authority"
        second_input = CONTEXT.saves / "paid-surface-first-resave.sav"
        shutil.copyfile(resave, second_input)
        evidence.append(read_case(args.configuration, second_input, "inspect", "ordinary-second-load"))
        assert paid_authority(resave) == original_authority, "repeated ordinary load changed paid history or native packet authority"
        evidence.append(read_case(args.configuration, source, "continue", "ordinary-cancellation"))
        evidence.append(read_case(args.configuration, CONTEXT.saves / "paid-surface-continue.sav", "continued", "ordinary-continued-cancellation"))
    if args.phase in ("corruption", "all"):
        # Structural faults must refuse independent preview and full loading;
        # native-purpose faults intentionally pass scalar preview and fail actual custody acceptance.
        structural = ("missing-header", "missing-table", "schema", "count", "cursor", "oversized", "foreign-column", "identity", "refund", "packet-purpose", "packet-dispatch", "packet-both")
        physical = ("dispatch", "native-label", "packet-order", "packet-count", "packet-flag")
        selected = set(args.faults or structural + physical)
        if not selected.issubset(set(structural + physical)):
            parser.error("unknown paid-family fault")
        for fault in structural:
            if fault not in selected:
                continue
            bad = malformed(source, fault)
            evidence.append(read_case(args.configuration, source, "preview", f"preview-{fault}", bad))
            evidence.append(read_case(args.configuration, source, "full-refusal", f"refusal-{fault}", bad))
        for fault in physical:
            if fault not in selected:
                continue
            evidence.append(read_case(args.configuration, source, "native-refusal", f"refusal-{fault}", malformed(source, fault)))
    if args.legacy is not None:
        legacy = args.legacy.resolve(strict=True)
        assert read_stream(legacy)[0] < 426, legacy
        evidence.append(read_case(args.configuration, legacy, "legacy", "genuine-older-absence"))
    assert hashlib.sha256(source.read_bytes()).hexdigest() == original, "original native source changed"
    report = {"_documentation": "r2mlvs6-mines-and-resource-chains.md13CC; real stationary-stock fixtures, driven acceptance pending13CD",
              "configuration": args.configuration, "phase": args.phase,
              "selected_faults": args.faults,
              "source": {"path": str(source), "sha256": original}, "native": evidence}
    target = CONTEXT.reports / f"paid-surface-{args.phase}.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"SURFACE_SUPPLY_PROOF report={target.relative_to(ROOT).as_posix()}", flush=True)


if __name__ == "__main__":
    main()
