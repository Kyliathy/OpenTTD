"""Actual shared one-hop lineage files, explicit older migration and independent corrupt preview.

@file utility_lineage_file_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group8C.1.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md shared native artifact routing.
"""

import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import sys

ROOT = next(parent for parent in Path(__file__).resolve().parents
            if (parent / "CMakeLists.txt").is_file() and (parent / "src").is_dir())
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import console_executable, current_run_context, run_native
from artifact_paths import resolve_input_path

CONTEXT = current_run_context("utilities", "native-reload")
CONTEXT.prepare_runtime()
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks, encode_chunk


def authority(path):
    """Read exact current shared lineage and original native physical-pair authority.

    @param path Actual current native save or resave.
    @return Exact utility/parent/segment and portal family bytes.
    """
    version, stream = read_stream(path)
    assert version == 403, (path, version)
    names = (b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"LPWH", b"LPWK")
    rows = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}
    assert len(rows) == len(names), (path, rows.keys())
    return rows


def native(configuration, source, mode, corrupt=None, preview=None):
    """Exercise actual ordinary native acceptance or this file's independent copied preview.

    @param configuration Exact Debug/optimized executable owner.
    @param source Unmodified accepted source file.
    @param mode Native accepted-world observer case.
    @param corrupt Optional newly generated malformed copy.
    @param preview Optional valid different-world lineage file.
    @return Successful focused process evidence with exact provenance.
    """
    environment = dict(os.environ, OTTD_UTILITY_FILE_INPUT=str(source), OTTD_UTILITY_FILE_CASE=mode)
    if corrupt is not None:
        environment["OTTD_UTILITY_LINEAGE_CORRUPT"] = str(corrupt)
    if preview is not None:
        environment["OTTD_UTILITY_LINEAGE_PREVIEW"] = str(preview)
    (CONTEXT.runtime_dir / "scripts/game_start.scr").write_text(
        "# @see r2mlvs12-solar-farms-mirrors-and-eti.md8C native lineage observer.\nutility_transfer_check\n", encoding="utf-8")
    suffix = corrupt.stem if corrupt is not None else source.stem
    log = CONTEXT.logs / f"lineage-{mode}-{suffix}.log"
    with log.open("wb") as sink:
        run_native(CONTEXT, [ROOT / "build" / configuration / "openttd_test.exe", "[.utility-power-transfer-file]"],
                   cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence, log
    print(f"{mode}-{suffix}: passed ({log.relative_to(ROOT).as_posix()})", flush=True)
    return evidence


def malformed(source, fault):
    """Change one current owner/counter/parent/path field in a fresh test-owned copy.

    @param source Actual native403 original-source40-J transfer from81-J publication.
    @param fault Exact malformed identity/reference/presence scenario.
    @return New corrupt copy preserving all unrelated original chunks and input bytes.
    """
    version, stream = read_stream(source)
    assert version == 403
    records = chunks(stream)
    header = next(row for row in records if row["name"] == b"UPLH")
    table = next(row for row in records if row["name"] == b"UPLD")
    path_header = next(row for row in records if row["name"] == b"UPPH")
    paths = next(row for row in records if row["name"] == b"UPPD")
    if fault.startswith("missing-path-"):
        missing = b"UPPH" if fault == "missing-path-header" else b"UPPD"
        records = [row for row in records if row["name"] != missing]
    elif fault in ("bad-path-count", "bad-path-epoch"):
        data = bytearray(path_header["payload"])
        offset = 16 if fault == "bad-path-count" else 8
        data[offset:offset + 8] = (int.from_bytes(data[offset:offset + 8], "big") + 1).to_bytes(8, "big")
        path_header["payload"] = bytes(data); path_header["edited"] = True
    elif fault == "bad-counter":
        data = bytearray(header["payload"]); assert len(data) == 112
        data[104:112] = (3).to_bytes(8, "big")
        header["payload"] = bytes(data); header["edited"] = True
    elif fault in ("duplicate-id", "bad-parent", "bad-donor", "hidden-source-parent", "missing-parent-field"):
        if fault == "missing-parent-field":
            fields = table["rows"][0]
            table["rows"][0] = fields.replace(b"parent\x00", b"parenz\x00", 1)
            assert table["rows"][0] != fields
        else:
            kind = 0 if fault == "hidden-source-parent" else 3
            candidates = [index for index, row in enumerate(table["rows"]) if index > 0 and row[0] == kind]
            index = candidates[-1] if fault in ("bad-parent", "bad-donor") else candidates[0]
            row = bytearray(table["rows"][index]); assert len(row) == 115
            if fault == "duplicate-id":
                row[107:111] = (1).to_bytes(4, "big")
            elif fault in ("bad-parent", "hidden-source-parent"):
                row[111:115] = (1).to_bytes(4, "big")
            else:
                row[21:29] = (41).to_bytes(8, "big")
            table["rows"][index] = bytes(row)
        table["edited"] = True
    elif fault in ("wrong-edge", "bad-ordinal", "wrong-path-share", "duplicate-path"):
        row = bytearray(paths["rows"][1]); assert len(row) == 12
        offset = 8 if fault == "wrong-edge" else 4 if fault == "bad-ordinal" else 0
        row[offset:offset + 4] = (0 if fault == "wrong-path-share" else 1).to_bytes(4, "big")
        if fault == "duplicate-path":
            paths["rows"].append(paths["rows"][1])
            data = bytearray(path_header["payload"]); data[16:24] = (2).to_bytes(8, "big")
            path_header["payload"] = bytes(data); path_header["edited"] = True
        else:
            paths["rows"][1] = bytes(row)
        paths["edited"] = True
    else:
        raise ValueError(fault)
    target = CONTEXT.saves / f"lineage-corrupt-{fault}.sav"
    encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"] for row in records) + bytes(4)
    target.write_bytes(b"OTTN" + source.read_bytes()[4:8] + encoded)
    return target


def ordinary(configuration, source):
    """Load/resave actual shared lineage with the ordinary game executable.

    @param configuration Exact executable configuration.
    @param source Actual native transferred/rebuilt checkpoint.
    @return Exact resave with no fixture-side authority reconstruction.
    """
    target = CONTEXT.saves / f"lineage-ordinary-{source.stem}.sav"
    (CONTEXT.runtime_dir / "scripts/game_start.scr").write_text(
        "# @see r2mlvs12-solar-farms-mirrors-and-eti.md8C ordinary lineage loader.\nset savegame_format lzma\n"
        f'save "{target.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    executable = console_executable(CONTEXT, ROOT / "build" / configuration / "openttd.exe")
    with (CONTEXT.logs / f"lineage-ordinary-{source.stem}.log").open("wb") as sink:
        run_native(CONTEXT, [executable, "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos",
                            "-c", CONTEXT.config_path, "-g", source], cwd=ROOT / "build",
                   env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, timeout=110, check=True)
    assert authority(target) == authority(source)
    print(f"ordinary-{source.stem}: six exact utility/path/portal chunks", flush=True)
    return target


def main():
    """Run focused actual lineage creation/reload or bounded corrupt-copy batches.

    @return None; preserve every retained input hash and native startup content.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="RelWithDebInfo")
    parser.add_argument("--phase", choices=("all", "reload", "preview-1", "preview-2", "refusal"), default="all")
    options = parser.parse_args()
    original = resolve_input_path(ROOT / "src/tests/fixtures/haulage/spoil-loop-deep/m1-loop-deep-4-complete-loop.sav")
    legacy = resolve_input_path(ROOT / "src/tests/fixtures/utility/utility-v383-transferred.sav")
    hashes = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in (original, legacy)}
    startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    print(f"UTILITY_LINEAGE_RUN configuration={options.configuration} phase={options.phase} run={CONTEXT.run_id}", flush=True)
    try:
        native(options.configuration, original, "create")
        transferred = CONTEXT.saves / "utility-transferred.sav"
        if options.phase in ("all", "reload"):
            native(options.configuration, original, "lineage-independent", preview=transferred)
            for mode, name in (("transferred", "utility-transferred.sav"), ("consumed", "utility-import-consumed.sav"),
                               ("loss", "utility-transfer-loss.sav"), ("rebuilt", "utility-transfer-rebuilt.sav")):
                source = CONTEXT.saves / name
                native(options.configuration, source, mode)
                assert authority(CONTEXT.saves / f"utility-transfer-{mode}-reloaded.sav") == authority(source)
            native(options.configuration, legacy, "legacy-transferred")
            ordinary(options.configuration, transferred)
            ordinary(options.configuration, CONTEXT.saves / "utility-transfer-rebuilt.sav")
        faults = ("missing-path-header", "missing-path-table", "bad-path-count", "bad-path-epoch", "bad-counter", "duplicate-id", "bad-parent",
                  "bad-donor", "hidden-source-parent", "missing-parent-field", "wrong-edge", "bad-ordinal", "wrong-path-share", "duplicate-path")
        # Selectable halves retain the same exact source while bounding slow
        # native machines without running unrelated utility/feature regressions.
        for phase, selected in (("preview-1", faults[:7]), ("preview-2", faults[7:])):
            if options.phase not in ("all", phase):
                continue
            for fault in selected:
                native(options.configuration, transferred, "lineage-preview", corrupt=malformed(transferred, fault))
        if options.phase in ("all", "refusal"):
            for fault in ("missing-path-table", "bad-counter", "bad-parent", "wrong-edge", "bad-ordinal", "wrong-path-share"):
                native(options.configuration, transferred, "lineage-refusal", corrupt=malformed(transferred, fault))
        report = CONTEXT.write_report(f"utility-lineage-{options.phase}.json", Path(__file__).relative_to(ROOT).as_posix(),
                                      list(CONTEXT.saves.glob("*.sav")), [original, legacy], options.configuration,
                                      "src/tests/utility_power_transfer_load.cpp")
        print(f"UTILITY_LINEAGE_PROOF phase={options.phase} report={report.relative_to(ROOT).as_posix()} operating_geothermal_export=false", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        for path, expected in hashes.items():
            assert hashlib.sha256(path.read_bytes()).hexdigest() == expected, path


if __name__ == "__main__":
    main()
