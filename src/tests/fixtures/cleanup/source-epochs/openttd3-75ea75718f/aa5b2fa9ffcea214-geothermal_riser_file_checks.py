"""Native riser construction persistence, independent preview and ordinary loading.

@file geothermal_riser_file_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5 paid electrical bindings.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md shared artifact routing.
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
from artifact_paths import resolve_input_path
from runner import console_executable, current_run_context, run_native

CONTEXT = current_run_context("geothermal", "riser-native-files")
CONTEXT.prepare_runtime()
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks, encode_chunk


def authority(path, native_map=False):
    """Read exact native owners rather than inventing parallel stock or work balances.

    @param path Actual native input or resave.
    @param native_map Include exact underlying traffic/geometry chunks for relay coexistence proofs.
    @return Paid machinery, original portal, native materials and existing energy/geology chunks.
    """
    version, stream = read_stream(path)
    assert version in (400, 401, 402, 403), (path, version)
    names = (b"GRSH", b"GRSD", b"INDY", b"IIDS", b"LPWH", b"LPWK", b"GEOS", b"GDEP", b"GSPN",
             b"GVNT", b"GCSH", b"GCSD", b"GCBH", b"GCBD", b"UPLH", b"UPLD", b"GHTH", b"GHTD",
             b"TUPH", b"TUPD", b"TUVH", b"TUVD")
    if version >= 403:
        names += (b"UPPH", b"UPPD")
    if native_map:
        names += (b"MAPS", b"MAPT", b"MAPH", b"MAPO", b"MAP2", b"MAP5", b"MAPE", b"MAP7", b"MAP8")
    rows = {row["name"]: row["raw"] for row in chunks(stream) if row["name"] in names}
    assert len(rows) == len(names), (path, rows.keys())
    return rows


def native(configuration, tag, label, environment=None):
    """Run one bounded native proof and retain its exact executable provenance.

    @param configuration Debug or optimized build owner.
    @param tag Exact focused Catch producer/reader tag.
    @param label Unique diagnostic basename within this invocation.
    @param environment Explicit native input/case fields.
    @return Successful process evidence; failures retain their full log.
    """
    log = CONTEXT.logs / f"{label}.log"
    with log.open("wb") as sink:
        run_native(CONTEXT, [ROOT / "build" / configuration / "openttd_test.exe", tag],
                   cwd=ROOT / "build", env=dict(os.environ, **(environment or {})),
                   stdout=sink, stderr=subprocess.STDOUT, timeout=110, check=True)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence, log
    print(f"{label}: passed ({log.relative_to(ROOT).as_posix()})", flush=True)
    return evidence


def read_case(configuration, source, mode, expected=0, corrupt=None, preview=None):
    """Exercise the ordinary native loader or its independent copied preview.

    @param configuration Actual tested build.
    @param source Unmodified accepted native world to load first.
    @param mode Exact accepted-world observer operation.
    @param expected Saved labour expected by construction checkpoint readers.
    @param corrupt Optional test-owned malformed copy, never an immutable fixture.
    @param preview Optional different-world native file for independent preview.
    @return Successful observer evidence.
    """
    environment = dict(OTTD_RISER_FILE_INPUT=str(source), OTTD_RISER_FILE_CASE=mode,
                       OTTD_RISER_FILE_EXPECTED=str(expected))
    if corrupt is not None:
        environment["OTTD_RISER_FILE_CORRUPT"] = str(corrupt)
    if preview is not None:
        environment["OTTD_RISER_FILE_PREVIEW"] = str(preview)
    # Every accepted-world reader owns this invocation's next startup command;
    # a preceding ordinary resave cannot silently replace the observer by quit.
    (CONTEXT.runtime_dir / "scripts/game_start.scr").write_text(
        "# @see r2mlvs18-geothermal-energy-and-industry.md5 native observer.\ngeothermal_riser_check\n", encoding="utf-8")
    suffix = corrupt.stem if corrupt is not None else source.stem
    return native(configuration, "[.geothermal-riser-file]", f"{mode}-{suffix}", environment)


def malformed(source, fault):
    """Change one dependent native reference or receipt in a fresh test-owned copy.

    @param source Genuine native401..403 funded, partial or paid-linked construction file.
    @param fault Exact malformed-field/presence scenario.
    @return Fresh corrupt save preserving all unrelated original chunk bytes.
    """
    version, stream = read_stream(source)
    assert version in (401, 402, 403)
    records = chunks(stream)
    header = next(row for row in records if row["name"] == b"GRSH")
    table = next(row for row in records if row["name"] == b"GRSD")
    if fault.startswith("relay-"):
        utility = next(record for record in records if record["name"] == b"UPLD")
        index = next(index for index, row in enumerate(utility["rows"])
                     if index > 0 and row[0] == 4 and row[11] == 1 and row[10] == 1)
        row = bytearray(utility["rows"][index])
        assert len(row) == (115 if version >= 403 else 107)
        if fault == "relay-layer":
            row[10] = 3
        elif fault == "relay-heat":
            row[104] = 1
            row[21:29] = (1000000).to_bytes(8, "big")
        elif fault == "relay-surface":
            row[10] = 0
        elif fault == "relay-rate":
            row[21:29] = (200001).to_bytes(8, "big")
        elif fault == "relay-owner":
            row[9] = 14
        elif fault == "relay-native-port":
            # This is a structurally valid, unique Underground address on
            # actual generic machinery. Only native coexistence can refuse it.
            address = 64 * 64 + 20 * 64 + 32
            row[91:95] = row[95:99] = address.to_bytes(4, "big")
        else:
            raise AssertionError(fault)
        utility["rows"][index] = bytes(row)
        utility["edited"] = True
    elif fault.startswith("link-"):
        # Cross-family faults keep the actual native writer world intact and
        # change only the selected paid binding or its original saved owner.
        if fault in ("link-machine", "link-rate", "link-stamp", "link-downgrade"):
            utility = next(record for record in records if record["name"] == b"UPLD")
            index = next(index for index, row in enumerate(utility["rows"]) if index > 0 and row[0] == 4 and row[11] == 2)
            row = bytearray(utility["rows"][index])
            assert len(row) == (115 if version >= 403 else 107)
            if fault == "link-machine":
                row[105:107] = (63999).to_bytes(2, "big")
            elif fault == "link-rate":
                row[21:29] = (200001).to_bytes(8, "big")
            elif fault == "link-downgrade":
                row[11] = 0
                row[21:29] = (1000).to_bytes(8, "big")
                row[105:107] = bytes(2)
            else:
                row[13:21] = (int.from_bytes(row[13:21], "big") + 1).to_bytes(8, "big")
            utility["rows"][index] = bytes(row)
            utility["edited"] = True
        elif fault in ("link-unfinished", "link-retired"):
            row = bytearray(table["rows"][1])
            assert len(row) == 43
            if fault == "link-unfinished":
                row[31:33] = (295).to_bytes(2, "big")
            else:
                row[30] = 1
            table["rows"][1] = bytes(row)
            table["edited"] = True
        elif fault == "link-sealed":
            portals = next(record for record in records if record["name"] == b"LPWK")
            row = bytearray(portals["rows"][1])
            assert len(row) == 26
            row[4] = 0
            portals["rows"][1] = bytes(row)
            portals["edited"] = True
        elif fault == "link-pre1980":
            date = next(record for record in records if record["name"] == b"DATE")
            row = bytearray(date["rows"][1])
            # The real producer remains at 1980-01-01 throughout its explicit
            # labour ticks. Its first DATE scalar is the native calendar day.
            row[:4] = (int.from_bytes(row[:4], "big", signed=True) - 1).to_bytes(4, "big", signed=True)
            date["rows"][1] = bytes(row)
            date["edited"] = True
        else:
            raise ValueError(fault)
    elif fault == "missing-header":
        records.remove(header)
    elif fault == "missing-table":
        records.remove(table)
    elif fault == "bad-count":
        payload = bytearray(header["payload"])
        assert len(payload) == 8
        payload[4:8] = (257).to_bytes(4, "big")
        header.update(payload=bytes(payload), edited=True)
    elif fault == "omitted-native-machinery":
        # Clearing both table/count and the declared feature cannot erase
        # actual native INDY machinery classified by this file's own IIDS.
        table["rows"] = table["rows"][:1]
        table["edited"] = True
        header.update(payload=header["payload"][:4] + bytes(4), edited=True)
        metadata = next(row for row in records if row["name"] == b"MLMD")
        copied = bytearray(metadata["rows"][1])
        flags = int.from_bytes(copied[-20:-16], "big")
        assert flags & 262144
        copied[-20:-16] = (flags & ~262144).to_bytes(4, "big")
        metadata["rows"][1] = bytes(copied)
        metadata["edited"] = True
    else:
        row = bytearray(table["rows"][1])
        assert len(row) == 43
        if fault == "bad-work":
            row[31:33] = (297).to_bytes(2, "big")
        elif fault == "free-receipt":
            row[33] = 1
        elif fault == "bad-industry":
            row[0:2] = (63999).to_bytes(2, "big")
        elif fault == "bad-owner":
            row[2] = 14
        elif fault == "space-floor":
            row[3] = 2
        elif fault == "out-of-file-floor":
            row[4:6] = (100).to_bytes(2, "big")
        elif fault == "bad-retired-boolean":
            row[30] = 2
        elif fault == "future-portal":
            row[22:30] = (int.from_bytes(row[22:30], "big") + 1).to_bytes(8, "big")
        elif fault == "wrong-destination":
            row[16] = 0
        elif fault == "future-work-tick":
            row[35:43] = (int.from_bytes(row[35:43], "big") + 1).to_bytes(8, "big")
        else:
            raise ValueError(fault)
        table["rows"][1] = bytes(row)
        table["edited"] = True
    target = CONTEXT.saves / f"riser-corrupt-{fault}.sav"
    encoded = b"".join(encode_chunk(row) if row.get("edited") else row["raw"] for row in records) + bytes(4)
    target.write_bytes(b"OTTN" + source.read_bytes()[4:8] + encoded)
    return target


def ordinary(configuration, source, label):
    """Load and resave actual paid machinery using the ordinary game executable.

    @param configuration Executable build configuration.
    @param source Unmodified completed, sealed or retired native input.
    @param label Unique native construction checkpoint.
    @return Ordinary game resave after byte-exact authority comparison.
    """
    target = CONTEXT.saves / f"riser-ordinary-{label}.sav"
    startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md5B ordinary loader.\n"
                       "set savegame_format lzma\n"
                       f'save "{target.with_suffix("").as_posix()}"\nquit\n', encoding="utf-8")
    executable = console_executable(CONTEXT, ROOT / "build" / configuration / "openttd.exe")
    with (CONTEXT.logs / f"ordinary-{label}.log").open("wb") as sink:
        run_native(CONTEXT, [executable, "-v", "null:ticks=2", "-s", "null", "-m", "null",
                            "-x", "-X", "-I", "original_dos", "-c", CONTEXT.config_path, "-g", source],
                   cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration),
                   stdout=sink, stderr=subprocess.STDOUT, timeout=110, check=True)
    assert authority(target, native_map=label.startswith("relay-")) == authority(source, native_map=label.startswith("relay-")), label
    print(f"ordinary-{label}: exact paid/native/portal authority preserved", flush=True)
    return target


def main():
    """Run focused selectable file batches with immutable inputs and repository-owned evidence.

    @return None; startup is restored even on refusal/proof failure.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="RelWithDebInfo")
    parser.add_argument("--phase", choices=("all", "reload", "preview-1", "preview-2", "refusal", "ordinary", "link-reload", "link-preview", "link-refusal", "relay-reload", "relay-preview", "relay-refusal"), default="all")
    parser.add_argument("--prior-riser", type=Path, help="Explicit native400 completed-riser input for compatibility proof; never discovered from a latest run")
    parser.add_argument("--prior-link", type=Path, help="Explicit native401 paid-linked input for relay-format migration; never discovered from a latest run")
    options = parser.parse_args()
    print(f"RISER_NATIVE_RUN configuration={options.configuration} phase={options.phase} run={CONTEXT.run_id}", flush=True)
    retained = resolve_input_path(ROOT / "src/tests/fixtures/geothermal/fixture388-cold-site-fixture-create.sav")
    original = hashlib.sha256(retained.read_bytes()).hexdigest()
    prior = resolve_input_path(options.prior_riser) if options.prior_riser is not None else None
    prior_hash = hashlib.sha256(prior.read_bytes()).hexdigest() if prior is not None else None
    prior_link = resolve_input_path(options.prior_link) if options.prior_link is not None else None
    prior_link_hash = hashlib.sha256(prior_link.read_bytes()).hexdigest() if prior_link is not None else None
    startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
    previous = startup.read_bytes() if startup.exists() else None
    try:
        startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md5B native observer.\ngeothermal_riser_check\n", encoding="utf-8")
        native(options.configuration, "[.geothermal-riser-write]", "write-construction")
        funded = CONTEXT.saves / "riser-funded.sav"
        partial = CONTEXT.saves / "riser-partial.sav"
        linked = CONTEXT.saves / "riser-linked.sav"
        if options.phase in ("all", "reload"):
            read_case(options.configuration, retained, "legacy")
            read_case(options.configuration, retained, "independent-preview", preview=partial)
            # Each real checkpoint must preserve the actual INDY stock/history
            # and original paid receipt/stamp; no fixture-side reconstruction.
            for name, mode, expected in (("funded", "funded", 0), ("shortage", "shortage", 0),
                                         ("partial", "partial", 148), ("complete", "complete", 296),
                                         ("sealed", "sealed", 296), ("reopened", "complete", 296),
                                         ("retired", "retired", 296), ("rebuilt-portal", "retired", 296),
                                         ("abandoned", "retired", 296)):
                source = CONTEXT.saves / f"riser-{name}.sav"
                read_case(options.configuration, source, mode, expected)
                assert authority(CONTEXT.saves / f"riser-read-{mode}-{source.stem}.sav") == authority(source), name
            read_case(options.configuration, partial, "next-tick")
        faults = ("missing-header", "missing-table", "bad-count", "bad-work", "free-receipt", "bad-industry",
                  "bad-owner", "space-floor", "out-of-file-floor", "bad-retired-boolean", "future-portal",
                  "wrong-destination", "omitted-native-machinery", "future-work-tick")
        # Independent preview batches are selectable so slow machines can run
        # each half without turning this focused owner into a long regression.
        for phase, selected in (("preview-1", faults[:7]), ("preview-2", faults[7:])):
            if options.phase not in ("all", phase):
                continue
            for fault in selected:
                source = partial if fault == "future-work-tick" else funded
                read_case(options.configuration, source, "preview", corrupt=malformed(source, fault))
        if options.phase in ("all", "refusal"):
            # Exercise both scalar parsing/Null fixup and actual native-family
            # reference refusal, then recover with an ordinary valid full load.
            for fault in ("bad-work", "bad-industry", "wrong-destination", "omitted-native-machinery"):
                read_case(options.configuration, funded, "full-refusal", corrupt=malformed(funded, fault))
            for fault in ("invalid-native-stage", "invalid-native-stock"):
                read_case(options.configuration, funded, "full-native-refusal", corrupt=CONTEXT.saves / f"riser-{fault}.sav")
        if options.phase in ("all", "ordinary"):
            for label in ("partial", "complete", "sealed", "retired", "rebuilt-portal", "abandoned"):
                ordinary(options.configuration, CONTEXT.saves / f"riser-{label}.sav", label)
        if options.phase in ("all", "link-reload"):
            read_case(options.configuration, retained, "independent-preview", preview=linked)
            for label in ("linked", "relinked"):
                source = CONTEXT.saves / f"riser-{label}.sav"
                read_case(options.configuration, source, "link", 296)
                assert authority(CONTEXT.saves / f"riser-read-link-{source.stem}.sav") == authority(source)
            if prior is not None:
                assert read_stream(prior)[0] == 400, "compatibility input must be genuine native400"
                read_case(options.configuration, prior, "complete", 296)
                # Only the explicitly versioned utility transport changes;
                # original paid work/stock/portal/geology remain byte-exact.
                old = authority(prior)
                new = authority(CONTEXT.saves / f"riser-read-complete-{prior.stem}.sav")
                assert all(old[name] == new[name] for name in old if name not in (b"UPLH", b"UPLD"))
            for label in ("linked", "relinked"):
                ordinary(options.configuration, CONTEXT.saves / f"riser-{label}.sav", label)
        link_faults = ("link-machine", "link-rate", "link-stamp", "link-unfinished", "link-retired", "link-sealed", "link-pre1980", "link-downgrade")
        if options.phase in ("all", "link-preview"):
            for fault in link_faults:
                read_case(options.configuration, linked, "preview", corrupt=malformed(linked, fault))
        if options.phase in ("all", "link-refusal"):
            for fault in ("link-machine", "link-retired", "link-unfinished", "link-sealed", "link-pre1980", "link-downgrade"):
                read_case(options.configuration, linked, "full-refusal", corrupt=malformed(linked, fault))
        if options.phase == "all" or options.phase.startswith("relay-"):
            native(options.configuration, "[.geothermal-relay-write]", "write-relay")
            relay = CONTEXT.saves / "riser-relay-connected.sav"
            if options.phase in ("all", "relay-reload"):
                read_case(options.configuration, retained, "independent-relay-preview", preview=relay)
                for label, mode in (("connected", "relay"), ("split", "relay-split"), ("repaired", "relay")):
                    source = CONTEXT.saves / f"riser-relay-{label}.sav"
                    read_case(options.configuration, source, mode)
                    assert authority(CONTEXT.saves / f"riser-read-{mode}-{source.stem}.sav", native_map=True) == authority(source, native_map=True)
                    ordinary(options.configuration, source, f"relay-{label}")
                if prior_link is not None:
                    assert read_stream(prior_link)[0] == 401, "compatibility input must be genuine native401"
                    read_case(options.configuration, prior_link, "link", 296)
                    old = authority(prior_link)
                    new = authority(CONTEXT.saves / f"riser-read-link-{prior_link.stem}.sav")
                    assert all(old[name] == new[name] for name in old if name not in (b"UPLH", b"UPLD"))
            relay_faults = ("relay-layer", "relay-heat", "relay-surface", "relay-rate", "relay-owner")
            if options.phase in ("all", "relay-preview"):
                for fault in relay_faults:
                    read_case(options.configuration, relay, "relay-preview", corrupt=malformed(relay, fault))
            if options.phase in ("all", "relay-refusal"):
                for fault in relay_faults:
                    read_case(options.configuration, relay, "relay-refusal", corrupt=malformed(relay, fault))
                read_case(options.configuration, relay, "relay-native-refusal", corrupt=malformed(relay, "relay-native-port"))
        report = CONTEXT.write_report(f"riser-{options.phase}.json", Path(__file__).relative_to(ROOT).as_posix(),
                                      list(CONTEXT.saves.glob("*.sav")), [retained] + ([prior] if prior is not None else []) + ([prior_link] if prior_link is not None else []), options.configuration,
                                      "src/tests/geothermal_riser_load.cpp")
        print(f"RISER_NATIVE_PROOF phase={options.phase} report={report.relative_to(ROOT).as_posix()} "
              "paid_funding=true boundary_open_floor=true boundary_material_delivery=true transported_bootstrap=false", flush=True)
    finally:
        if previous is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(previous)
        assert hashlib.sha256(retained.read_bytes()).hexdigest() == original, "retained native388 input changed"
        if prior is not None:
            assert hashlib.sha256(prior.read_bytes()).hexdigest() == prior_hash, "explicit native400 input changed"
        if prior_link is not None:
            assert hashlib.sha256(prior_link.read_bytes()).hexdigest() == prior_link_hash, "explicit native401 input changed"


if __name__ == "__main__":
    main()
