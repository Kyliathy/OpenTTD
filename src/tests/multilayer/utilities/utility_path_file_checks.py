"""Actual paid multi-hop native authority, independent preview and ordinary executable resaves.

@file utility_path_file_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group8C.4.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group5.4 feature caller.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md native artifact routing.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
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

RETAINED_SHA = "d7c08d0ce92115b506fc3aa507c2f0058129d0d617f826b6d7e681266c9fd12b"
CASES = ("root", "first", "staged", "branch", "used", "split", "repaired", "retry",
         "upstream-loss", "relinked", "next-interval", "source-lost")
# Explicit403 scalar transport widths permit precise negative wire edits,
# without importing older runners that materialize unrelated native contexts.
WIRE_LAYOUT = (("type", 1), ("id", 4), ("reference", 4), ("owner", 1), ("layer", 1), ("kind", 1), ("active", 1),
               ("epoch", 8), ("a", 8), ("b", 8), ("c", 8), ("d", 8), ("ticks", 4), ("remainder", 2),
               ("lifetime_generated", 8), ("lifetime_consumed", 8), ("transferred", 8), ("imported_link", 4),
               ("destination", 4), ("source_endpoint", 4), ("destination_endpoint", 4), ("town", 2),
               ("town_bonus", 1), ("town_applied", 1), ("town_suspended", 1), ("domain", 1), ("riser", 2), ("share", 4), ("parent", 4))
WIRE_FIELDS = {name: (sum(width for _, width in WIRE_LAYOUT[:index]), width)
               for index, (name, width) in enumerate(WIRE_LAYOUT)}
assert sum(width for _, width in WIRE_LAYOUT) == 115
FAULTS = ("missing-path-header", "missing-path-table", "duplicate-id", "cyclic-parent", "wrong-original-source", "forged-parent",
          "missing-first-cell", "reversed-relay-cells", "wrong-final-edge", "repeated-ancestor-edge", "duplicate-path-row", "bad-path-count",
          "forged-flow", "lost-ancestor-with-free-rights", "short-original-window", "wrong-declared-inlet", "foreign-cable-owner",
          "wrong-cable-plane", "forged-source-reserve", "over-rate")


def authority(path):
    """Read exact shared utility, physical machinery, Heat, map and existing native custody.

    @param path Actual current native save or resave.
    @return Thirty-five exact owner chunks; no reconstruction supplies missing state.
    """
    version, stream = read_stream(path)
    assert version == 403, (path, version)
    names = (b"GRSH", b"GRSD", b"INDY", b"IIDS", b"LPWH", b"LPWK", b"GEOS", b"GDEP", b"GSPN",
             b"GVNT", b"GCSH", b"GCSD", b"GCBH", b"GCBD", b"UPLH", b"UPLD", b"UPPH", b"UPPD",
             b"GHTH", b"GHTD", b"TUPH", b"TUPD", b"TUVH", b"TUVD", b"MAPS", b"MAPT", b"MAPH",
             b"MAPO", b"MAP2", b"MAP5", b"MAPE", b"MAP7", b"MAP8", b"PLYR", b"VEHS")
    result = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}
    assert len(result) == len(names), (path, result.keys())
    return result


def native(configuration, source, mode, expected=None, corrupt=None, preview=None):
    """Run one bounded ordinary-loader proof with exact input and executable provenance.

    @param configuration Actual Debug/optimized executable owner.
    @param source Explicit immutable or prior-run native input.
    @param mode Actual accepted-world observer operation.
    @param expected Original writer's complete authority digests for this source.
    @param corrupt Optional fresh malformed copy.
    @param preview Optional different-world actual native preview candidate.
    @return Complete successful observer evidence, retaining failure logs otherwise.
    """
    environment = dict(os.environ, OTTD_PAID_PATH_INPUT=str(source), OTTD_PAID_PATH_CASE=mode)
    for name, value in (expected or {}).items():
        environment[f"OTTD_PAID_PATH_{name.upper()}"] = value
    if corrupt is not None:
        environment["OTTD_PAID_PATH_CORRUPT"] = str(corrupt)
    if preview is not None:
        environment["OTTD_PAID_PATH_PREVIEW"] = str(preview)
    (CONTEXT.runtime_dir / "scripts/game_start.scr").write_text(
        "# @see r2mlvs12-solar-farms-mirrors-and-eti.md8C actual paid-path observer.\nutility_paid_path_check\n", encoding="utf-8")
    suffix = corrupt.stem if corrupt is not None else source.stem
    log = CONTEXT.logs / f"paid-path-{mode}-{suffix}.log"
    with log.open("wb") as sink:
        run_native(CONTEXT, [ROOT / "build" / configuration / "openttd_test.exe", "[.utility-power-path-file]"],
                   cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence, log
    # Feature acceptance requires actual public prepare/commit and replay
    # calls, rather than merely reading a shared producer's equivalent save.
    if os.environ.get("OTTD_GEOTHERMAL_EXPORT") == "1" and mode in ("create", "retry-after-load"):
        calls = re.findall(r"GEOTHERMAL_EXPORT from=\d+ to=\d+ quantity=\d+ replay=refused", evidence)
        assert len(calls) == (6 if mode == "create" else 1), (mode, calls, log)
    print(f"{mode}-{suffix}: passed ({log.relative_to(ROOT).as_posix()})", flush=True)
    return evidence


def ordinary(configuration, source):
    """Load/resave with the ordinary game executable and compare complete selected authority.

    @param configuration Actual executable owner.
    @param source Explicit actual paid-path checkpoint.
    @return New ordinary resave with thirty-five exact authority chunks.
    """
    target = CONTEXT.saves / f"paid-path-ordinary-{source.stem}.sav"
    (CONTEXT.runtime_dir / "scripts/game_start.scr").write_text(
        "# @see r2mlvs12-solar-farms-mirrors-and-eti.md8C ordinary paid-path loader.\nset savegame_format lzma\n"
        f'save "{target.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    executable = console_executable(CONTEXT, ROOT / "build" / configuration / "openttd.exe")
    with (CONTEXT.logs / f"paid-path-ordinary-{source.stem}.log").open("wb") as sink:
        run_native(CONTEXT, [executable, "-v", "null:ticks=2", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos",
                            "-c", CONTEXT.config_path, "-g", source], cwd=ROOT / "build",
                   env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, timeout=110, check=True)
    assert authority(target) == authority(source), source
    print(f"ordinary-{source.stem}: thirty-five exact authority chunks", flush=True)
    return target


def wire_value(row, field):
    """Read one explicit current403 scalar field for a bounded negative mutation.

    @param row Actual115-byte native UPLD row.
    @param field Named fixed scalar from the current wire descriptor.
    @return Original unsigned saved value; no runtime owner is queried.
    """
    assert len(row) == 115
    offset, width = WIRE_FIELDS[field]
    return int.from_bytes(row[offset:offset + width], "big")


def edit_wire(table, kind, identity, changes, selector="id"):
    """Edit exactly one selected actual native row without rewriting unrelated fields.

    @param table Parsed actual UPLD chunk.
    @param kind Source/consumer/share/link discriminator.
    @param identity Exact actual selected identity.
    @param changes Named scalar values to forge.
    @param selector Saved identity field, using share for globally unique lineage slices.
    @return None; the parsed copy alone receives edits.
    """
    matches = [index for index, row in enumerate(table["rows"]) if index > 0
               and wire_value(row, "type") == kind and wire_value(row, selector) == identity]
    assert len(matches) == 1, (kind, identity, selector)
    index = matches[0]
    row = bytearray(table["rows"][index])
    # Every forged scalar retains all other original descriptor/quantity bytes;
    # normal native validation, rather than a rebuilt synthetic state, must refuse it.
    for field, value in changes.items():
        offset, width = WIRE_FIELDS[field]
        row[offset:offset + width] = value.to_bytes(width, "big")
    table["rows"][index] = bytes(row)
    table["edited"] = True


def malformed(source, fault):
    """Forge one original paid lineage/physical participant in a fresh test-owned copy.

    @param source Actual native partly used/released paid branched checkpoint.
    @param fault Exact bounded missing/reference/quantity/window/port scenario.
    @return New malformed file preserving all unrelated native chunk bytes.
    """
    version, stream = read_stream(source)
    assert version == 403
    records = chunks(stream)
    table = next(record for record in records if record["name"] == b"UPLD")
    header = next(record for record in records if record["name"] == b"UPPH")
    paths = next(record for record in records if record["name"] == b"UPPD")
    paid = [row for index, row in enumerate(table["rows"]) if index > 0
            and wire_value(row, "type") == 4 and wire_value(row, "kind") == 2]
    first = next(row for row in paid if wire_value(row, "reference") == 0)
    second = next(row for row in paid if wire_value(row, "reference") == 1)
    if fault.startswith("missing-path-"):
        missing = b"UPPH" if fault == "missing-path-header" else b"UPPD"
        records = [record for record in records if record["name"] != missing]
    elif fault in ("duplicate-id", "cyclic-parent", "wrong-original-source", "forged-parent"):
        changes = {"share": 3} if fault == "duplicate-id" else {"parent": 3} if fault == "cyclic-parent" else {"reference": 1} if fault == "wrong-original-source" else {"parent": 0}
        selected = 4 if fault == "duplicate-id" else 2 if fault == "cyclic-parent" else 3
        edit_wire(table, 3, selected, changes, "share")
    elif fault in ("missing-first-cell", "reversed-relay-cells", "wrong-final-edge", "repeated-ancestor-edge", "wrong-declared-inlet"):
        share = 2 if fault in ("missing-first-cell", "wrong-declared-inlet") else 3 if fault in ("reversed-relay-cells", "repeated-ancestor-edge") else 4
        indices = [index for index, row in enumerate(paths["rows"]) if index > 0 and int.from_bytes(row[:4], "big") == share]
        if fault == "missing-first-cell":
            paths["rows"].pop(indices[0])
            data = bytearray(header["payload"])
            data[16:24] = (int.from_bytes(data[16:24], "big") - 1).to_bytes(8, "big")
            header["payload"] = bytes(data); header["edited"] = True
        elif fault == "reversed-relay-cells":
            # Swap actual cable references only, retaining canonical ordinals;
            # the resulting native inlet must fail physical ordered continuity.
            a, b = indices[1:3]
            left, right = bytearray(paths["rows"][a]), bytearray(paths["rows"][b])
            left[8:12], right[8:12] = right[8:12], left[8:12]
            paths["rows"][a], paths["rows"][b] = bytes(left), bytes(right)
        else:
            index = indices[1] if fault == "wrong-declared-inlet" else indices[-1]
            if fault == "wrong-declared-inlet":
                # A real separately installed Deep cell is cardinally adjacent
                # to the source and paid housing, but is not the original
                # machinery's declared port. Copied owners must enforce that.
                alternate = next(row for row in table["rows"][1:] if wire_value(row, "type") == 4
                                 and wire_value(row, "kind") == 1 and wire_value(row, "source_endpoint") == 3 * 64 * 64 + 9 * 64 + 50)
                link = wire_value(alternate, "id")
            else:
                link = wire_value(first, "id")
            row = bytearray(paths["rows"][index]); row[8:12] = link.to_bytes(4, "big"); paths["rows"][index] = bytes(row)
        paths["edited"] = True
    elif fault in ("duplicate-path-row", "bad-path-count"):
        data = bytearray(header["payload"])
        if fault == "duplicate-path-row":
            paths["rows"].append(paths["rows"][1]); paths["edited"] = True
        data[16:24] = (int.from_bytes(data[16:24], "big") + 1).to_bytes(8, "big")
        header["payload"] = bytes(data); header["edited"] = True
    elif fault in ("forged-flow", "lost-ancestor-with-free-rights", "over-rate"):
        changes = {"b": wire_value(first, "b") + 1} if fault == "forged-flow" else {"active": 0} if fault == "lost-ancestor-with-free-rights" else {"a": 200001}
        edit_wire(table, 4, wire_value(first, "id"), changes)
    elif fault == "short-original-window":
        # Preserve actual original generation/lifetime numerically, while
        # forging a shorter valid source window whose physical200-kW
        # opportunity cannot carry the already settled original paid flow.
        edit_wire(table, 0, 0, {"ticks": 10, "a": 7400000})
    elif fault == "forged-source-reserve":
        original = next(row for row in table["rows"][1:] if wire_value(row, "type") == 0 and wire_value(row, "id") == 0)
        edit_wire(table, 0, 0, {"c": wire_value(original, "c") + 1})
    elif fault in ("foreign-cable-owner", "wrong-cable-plane"):
        link = int.from_bytes(paths["rows"][1][8:12], "big")
        edit_wire(table, 4, link, {"owner": 14} if fault == "foreign-cable-owner" else {"layer": 1})
    elif fault == "native-machinery-floor":
        # This wire-only edit preserves every scalar Industry/riser/lineage
        # participant while removing one actual packed machinery tile. The
        # original copied dimensions are64x64/four planes; no runtime tile
        # arithmetic or live map contributes to this malformed copy.
        native_map = next(record for record in records if record["name"] == b"MAPT")
        data = bytearray(native_map["payload"]); assert len(data) == 4 * 64 * 64
        port = 3 * 64 * 64 + 10 * 64 + 51
        assert data[port] >> 4 == 8
        data[port] &= 15
        native_map["payload"] = bytes(data); native_map["edited"] = True
    else:
        raise ValueError(fault)
    target = CONTEXT.saves / f"paid-path-corrupt-{fault}.sav"
    encoded = b"".join(encode_chunk(record) if record.get("edited") else record["raw"] for record in records) + bytes(4)
    target.write_bytes(b"OTTN" + source.read_bytes()[4:8] + encoded)
    return target


def main(default_phase="reload-1a"):
    """Execute focused real-file batches without changing retained or prior-run sources.

    @param default_phase Bounded selection supplied by a focused companion entry point.
    @return None; reports bind actual native evidence and startup is restored on failure.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="RelWithDebInfo")
    parser.add_argument("--phase", choices=("write", "reload-1a", "reload-1b", "reload-2a", "reload-2b", "preview-1", "preview-2", "refusal"), default=default_phase)
    parser.add_argument("--source-run", help="Explicit prior invocation holding paid-path-identities.json and actual paid-path files")
    parser.add_argument("--reference-run", help="Explicit prior writer whose original digests and thirty-five owner chunks must match a newly created writer batch")
    parser.add_argument("--geothermal-export", action="store_true", help="Exercise Plan17 public export preparation/commit/replay in the actual native writer and reload continuation")
    options = parser.parse_args()
    caller = "geothermal" if options.geothermal_export else "shared"
    print(f"PAID_PATH_RUN configuration={options.configuration} phase={options.phase} caller={caller} run={CONTEXT.run_id}", flush=True)
    retained = resolve_input_path(ROOT / "src/tests/fixtures/haulage/spoil-loop-deep/m1-loop-deep-4-complete-loop.sav")
    assert hashlib.sha256(retained.read_bytes()).hexdigest() == RETAINED_SHA
    startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    previous_caller = os.environ.get("OTTD_GEOTHERMAL_EXPORT")
    os.environ["OTTD_GEOTHERMAL_EXPORT"] = "1" if options.geothermal_export else "0"
    inputs = [retained]
    try:
        if options.source_run is None:
            evidence = native(options.configuration, retained, "create")
            matches = re.findall(r"PAID_PATH_SAVE case=([\w-]+) utility=([0-9A-F]+) heat=([0-9A-F]+) geology=([0-9A-F]+) risers=([0-9A-F]+)", evidence)
            identities = {case: dict(utility=utility, heat=heat, geology=geology, risers=risers)
                          for case, utility, heat, geology, risers in matches}
            assert set(identities) == set(CASES), identities.keys()
            document = {"_documentation": "docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group8C.4 actual paid original-writer authority; boundary supplies are not operating geothermal/bootstrap proof.",
                        "source_run": CONTEXT.run_id, "input_sha256": RETAINED_SHA, "export_caller": caller, "states": identities}
            CONTEXT.output("reports", "paid-path-identities.json").write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
            source_directory = CONTEXT.saves
        else:
            # Prior-run inputs are explicit and immutable; resolving the report
            # through the repository guard rejects path traversal or latest-run guesses.
            report_path = resolve_input_path(ROOT / "mlm/reviews/utilities/native-reload" / options.source_run / "reports/paid-path-identities.json")
            document = json.loads(report_path.read_text(encoding="utf-8"))
            assert document["source_run"] == options.source_run and document["input_sha256"] == RETAINED_SHA
            assert document.get("export_caller", "shared") == caller, "source writer must exercise the selected public export caller"
            identities = document["states"]
            source_directory = report_path.parent.parent / "saves"
            inputs.append(report_path)
        sources = {case: resolve_input_path(source_directory / f"paid-path-{case}.sav") for case in CASES}
        inputs.extend(sources.values())
        if options.reference_run is not None:
            reference_path = resolve_input_path(ROOT / "mlm/reviews/utilities/native-reload" / options.reference_run / "reports/paid-path-identities.json")
            reference = json.loads(reference_path.read_text(encoding="utf-8"))
            assert reference["source_run"] == options.reference_run and reference["input_sha256"] == RETAINED_SHA
            assert identities == reference["states"], "original native writer digests differ across source/configuration batches"
            inputs.append(reference_path)
            # Actual independent writers must preserve the same complete
            # original economy/physical state, even across configurations.
            for case in CASES:
                prior = resolve_input_path(reference_path.parent.parent / "saves" / f"paid-path-{case}.sav")
                assert authority(sources[case]) == authority(prior), case
                inputs.append(prior)
        original = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs}
        if options.phase.startswith("reload-"):
            selected = {"reload-1a": CASES[:3], "reload-1b": CASES[3:5], "reload-2a": CASES[5:8], "reload-2b": CASES[8:]}[options.phase]
            # Each checkpoint fully reloads before an exact native resave and
            # then loads/resaves through the ordinary playable executable.
            for case in selected:
                source = sources[case]
                native(options.configuration, source, "reload", identities[case])
                assert authority(CONTEXT.saves / f"paid-path-read-{source.stem}.sav") == authority(source), case
                ordinary(options.configuration, source)
            if options.phase == "reload-1b":
                native(options.configuration, retained, "independent", preview=sources["branch"])
                native(options.configuration, sources["used"], "resume-use", identities["used"])
            elif options.phase == "reload-2a":
                native(options.configuration, sources["split"], "retry-after-load", identities["split"])
                assert authority(CONTEXT.saves / "paid-path-read-retry.sav") == authority(sources["retry"])
        if options.phase.startswith("preview-"):
            selected = FAULTS[:10] if options.phase == "preview-1" else FAULTS[10:]
            for fault in selected:
                native(options.configuration, sources["used"], "preview", identities["used"], corrupt=malformed(sources["used"], fault))
        if options.phase == "refusal":
            for fault in ("missing-path-header", "cyclic-parent", "wrong-original-source", "repeated-ancestor-edge", "forged-flow", "wrong-declared-inlet"):
                native(options.configuration, sources["used"], "refusal", identities["used"], corrupt=malformed(sources["used"], fault))
            native(options.configuration, sources["used"], "native-refusal", identities["used"], corrupt=malformed(sources["used"], "native-machinery-floor"))
        report = CONTEXT.write_report(f"paid-path-{options.phase}.json", Path(__file__).relative_to(ROOT).as_posix(),
                                      list(CONTEXT.saves.glob("*.sav")), inputs, options.configuration,
                                      "src/tests/utility_power_path_load.cpp")
        assert all(hashlib.sha256(path.read_bytes()).hexdigest() == digest for path, digest in original.items())
        print(f"PAID_PATH_PROOF phase={options.phase} report={report.relative_to(ROOT).as_posix()}", flush=True)
    finally:
        if previous_caller is None:
            os.environ.pop("OTTD_GEOTHERMAL_EXPORT", None)
        else:
            os.environ["OTTD_GEOTHERMAL_EXPORT"] = previous_caller
        assert hashlib.sha256(retained.read_bytes()).hexdigest() == RETAINED_SHA
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)


if __name__ == "__main__":
    main()
