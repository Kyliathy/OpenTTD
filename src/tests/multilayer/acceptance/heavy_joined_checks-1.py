"""Verify joined heavy admission from exact dated native producer boundaries.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CD
Old indexes bind report/save membership only; overwritten producer binaries are never requalified. Every new process and artifact has its own invocation owner.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream
from PIL import Image
from runner import current_run_context, run_native, save_native_capture

AUDIT = runpy.run_path(str(Path(__file__).with_name("probe_role_checks-5.py")))
LAYOUT = runpy.run_path(str(Path(__file__).with_name("heavy_contained_service_checks-4.py")))
DESCENT = runpy.run_path(str(Path(__file__).with_name("heavy_descent_checks-1.py")))
EXACT = AUDIT["EXACT"]
INDEXES = {
    "dock": ("heavy-dock-service", "2b70758f523be0bbdbdb825f4b392ab22f3e9e07c894764968df213dc4a31d3a"),
    "core": ("heavy-contained-service", "5fced76266f959fb98e79a713561d54a189d629668af32a2b6c3419b603ef9bb"),
    "onboard": ("heavy-onboard-service", "fbe4b7c9dd43788bb473c137806ac5f94b02a32e5d834d52d9e6c3fa1099596c"),
    "descent": ("heavy-descent", "73dfd9264f301f1b2a0a35c6bf53823cd737439f77f47721dc704c5b7e99dba7"),
    "old-service": ("legacy-contained-service", "042474085fd47f9750430043494901ee648c24e0712ec64d710a614d2a2cf698"),
    "old-descent": ("legacy-descent", "dc10eb579476c65df77cf7c52cde4b40876c1d3c80f38dea6269060174c095e5"),
    "mining": ("heavy-mining-runtime", "36c4f1680a1d63a01832cee9a2185cb2567979bc9fd66433835f08dc761d3799"),
    "handling": ("surface-collection", "273521e60e67109e5786b7e9366e8767e1747d497bdac68d394a211dbd553310"),
}
CASES = {
    "dock-heavy": ("dock", "heavy-partial"), "dock-legacy": ("dock", "legacy-partial"),
    "core": ("core", "partial"), "orphan": ("core", "sender-lost"),
    "onboard": ("onboard", "partial"), "paused": ("onboard", "paused"),
    "descent": ("descent", "partial"), "lost-descent": ("descent", "sender-lost"),
    "old-service": ("old-service", "partial"), "old-descent": ("old-descent", "partial"),
    "mining": ("mining", "working"), "handling": ("handling", "handling"),
}
GROUPS = {"services": tuple(CASES)[:4], "flight": tuple(CASES)[4:8], "legacy": tuple(CASES)[8:]}


def focused(configuration):
    """Run only active joined/model/service and required shared-power contracts.

    @param configuration Exact current test build.
    @return Invocation-owned focused report with actual terminal and executable evidence.
    """
    context = current_run_context("space", "heavy-joined-focused"); context.prepare_runtime(); log = context.logs / "focused.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"),
                            "[heavy-joined-validation],[heavy-contained-service],[orbital-service-contract],[orbital-dock-contract],[utility-power-ledger],[utility-power-mutation],[utility-power-effects],[utility-power-stage]"],
                            cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, timeout=90)
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace")); assert result.returncode == 0 and match, log
    target = context.reports / "heavy-joined-focused.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CD; detached joins, model bills, retained Dock/Core and useful shared-power contracts.", configuration=configuration,
                                     assertions=int(match[1]), cases=int(match[2]), native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def read(path):
    """Read an explicitly selected retained proof.

    @param path Exact repository-owned report path.
    @return Parsed metadata without recursive requalification of dated producers.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def entry(configuration, case):
    """Resolve one original saved state from frozen exact membership.

    @param configuration Matching original producer configuration.
    @param case Explicit paid-world scenario, never a newest-file search.
    @return Dated index, actual producer report and its original save identities.
    """
    group, phase = CASES[case]; name, digest = INDEXES[group]; index = ROOT / ("logs/" + name + "-verification.json")
    assert identity(index)["sha256"] == digest; accepted = read(index)
    references = accepted["reports"] if group == "handling" else next(row for row in accepted["configurations"] if row["configuration"] == configuration)["reports"]
    selected = []
    # Exact original configuration/phase membership supplies the paid owner. A later ordinary reconstruction cannot impersonate a genuine older-format producer.
    for reference in references:
        member = reference if isinstance(reference, dict) else identity(ROOT / reference); path = ROOT / member["path"]
        assert identity(path) == member; report = read(path)
        if report.get("configuration") == configuration and report.get("stage" if group == "handling" else "phase") == phase and "after" in report:
            selected.append((member, report["after"]))
    assert len(selected) == 1, (case, selected); producer, saved = selected[0]; assert identity(ROOT / saved["path"]) == saved
    return identity(index), producer, saved


def native(context, configuration, source, corrupt=None, reject=False, capture=False):
    """Run a single actual native admission or independent malformed reader.

    @param context Invocation-owned runtime/log/artifact owner.
    @param configuration Exact matching current executable build.
    @param source Original valid input, or the independent malformed startup candidate.
    @param corrupt Optional copied candidate checked while the valid world remains open.
    @param reject Whether startup must cleanly refuse before its observer runs.
    @param capture Whether actual native inspector windows are captured without simulation work.
    @return Passing assertion count; intended clean full refusals return zero.
    """
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("heavy_joined_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_HEAVY_JOINED_SOURCE=str(source))
    environment.pop("OTTD_HEAVY_JOINED_CORRUPT", None); environment.pop("OTTD_HEAVY_JOINED_CAPTURE", None)
    if corrupt is not None:
        environment["OTTD_HEAVY_JOINED_CORRUPT"] = str(corrupt)
    if capture:
        environment["OTTD_HEAVY_JOINED_CAPTURE"] = "1"
    log = context.logs / "heavy-joined.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.heavy-joined-native]"], cwd=ROOT / "build",
                            env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=30 if reject or corrupt is not None else 50)
    evidence = log.read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    if reject:
        assert result.returncode == 1 and not match and "Broken savegame" in evidence and "HEAVY_JOINED native=true" not in evidence and "Fatal test error" not in evidence and "Assertion failed" not in evidence, log
        return 0
    assert result.returncode == 0 and match and int(match[2]) == 1 and "HEAVY_JOINED native=true" in evidence, log
    if corrupt is not None:
        assert "HEAVY_JOINED preview_reject=true authority_unchanged=true" in evidence, log
        expected = "Invalid configured cargo dimension row" if "duplicate-dimensions" in corrupt.name else "Changed or missing configured cargo dimension preview" if any(kind in corrupt.name for kind in ("coherent-mass", "configured-mass", "missing-dimensions")) else "Shared or missing native/Space cargo ownership preview"
        assert "HEAVY_JOINED preview_error=" + expected in evidence, log
    return int(match[1])


def legacy_handling_migrations(source, saved):
    """Qualify only explicit absent fields and proven original engine startup in the frozen423 handling fixture.

    @param source Exact dated423 Surface handling input.
    @param saved Actual current437 native resave of that input.
    @return The narrowly admitted changed families; every other serialized byte is compared by verify().
    """
    old_version, before = chunks(source); version, after = chunks(saved); assert old_version == 423 and version == 437
    left, right = before[b"SVST"][3:], after[b"SVST"][3:]; count = left[0]; assert right[0] == count and len(left) == 1 + count * 49 and len(right) == 1 + count * 68
    # Genuine Earth receipts gain only absent factory/generation/serial/replacement fields. Every original paid hull, model, price and work byte stays exact.
    for index in range(count):
        old = left[1 + index * 49:1 + (index + 1) * 49]; current = right[1 + index * 68:1 + (index + 1) * 68]
        assert current[:49] == old and current[49:] == b"\xff\xff" + bytes(17)
    # The pre424 field has no saved starter input; migration appends explicit unknown site/rotation and zero preference/distance, never reconstructed history.
    assert after[b"ASTR"][3:-26] == before[b"ASTR"][3:] and after[b"ASTR"][-26:] == b"\0" + b"\xff" * 9 + bytes(16)
    table = runpy.run_path(str(Path(__file__).with_name("heavy_joined_checks-2.py")))["table_rows"]
    old_raw, new_raw = read_stream(source)[1], read_stream(saved)[1]; old_rows, new_rows = table(old_raw, b"ENGN"), table(new_raw, b"ENGN")
    assert len(old_rows) == len(new_rows); normalized = bytearray(after[b"ENGN"]); changed = {}
    allowed = {"reliability", "reliability_start", "reliability_max", "reliability_final", "duration_phase_1", "duration_phase_2", "duration_phase_3", "flags", "company_avail"}
    # The accepted pre429 RestoreUnstartedSpaceEngine owner starts only four untouched canonical definitions. Mask only its actual startup fields after proving their original constructor values; initialized or denied history cannot be overwritten.
    for old, new in zip(old_rows, new_rows, strict=True):
        assert old["index"] == new["index"] and len(old["fields"]) == len(new["fields"])
        for original, current in zip(old["fields"], new["fields"], strict=True):
            assert original["path"] == current["path"] and original["width"] == current["width"] and original["count"] == current["count"]
            a, b = old_raw[original["position"]:original["end"]], new_raw[current["position"]:current["end"]]
            if a == b:
                continue
            assert old["index"] in (265, 266, 267, 268) and original["path"] in allowed
            assert int.from_bytes(a, "big") == (65535 if original["path"] == "reliability" else 0)
            assert int.from_bytes(b, "big") != 0
            relative = current["position"] - new["chunk_start"] - 5; normalized[relative:relative + len(a)] = a
            changed.setdefault(old["index"], set()).add(original["path"])
    assert set(changed) == {265, 266, 267, 268} and all(fields == allowed for fields in changed.values()) and bytes(normalized) == before[b"ENGN"]
    return {b"SVST", b"ASTR", b"ENGN"}


def verify(configuration, case):
    """Upgrade and inspect one actual retained heavy/legacy physical state.

    @param configuration Matching current executable build.
    @param case Explicit frozen producer scenario.
    @return Current proof with exact old boundary, new437 state and terminal native evidence.
    """
    index, producer, selected = entry(configuration, case); source = ROOT / selected["path"]
    context = current_run_context("space", "heavy-joined-" + case)
    previous_berth = os.environ.get("OTTD_HEAVY_JOINED_SECOND_BERTH")
    try:
        if case == "dock-legacy":
            os.environ["OTTD_HEAVY_JOINED_SECOND_BERTH"] = "1"
        else:
            os.environ.pop("OTTD_HEAVY_JOINED_SECOND_BERTH", None)
        assertions = native(context, configuration, source, capture=case not in ("handling", "old-descent", "lost-descent", "mining"))
    finally:
        if previous_berth is None:
            os.environ.pop("OTTD_HEAVY_JOINED_SECOND_BERTH", None)
        else:
            os.environ["OTTD_HEAVY_JOINED_SECOND_BERTH"] = previous_berth
    # Native raster bytes remain immutable evidence; PNG conversion changes only the review container and retains exact drawn pixels.
    for image in context.images.glob("*.ppm"):
        with Image.open(image) as raster:
            save_native_capture(context, raster, image, image.with_suffix(".png"))
    saved = context.saves / "heavy-joined-437.sav"; version, after = chunks(saved); old_version, before = chunks(source); assert version == 437
    migrated = legacy_handling_migrations(source, saved) if case == "handling" else set()
    # Genuine old descents gain five explicitly absent provenance bytes, and contained roles gain their explicit unknown field. Normalization preserves all original work/packet receipts.
    for family in EXACT:
        if family in migrated:
            continue  # Only the exact qualified old absent-field/native-startup deltas above are excluded from this direct wire comparison.
        if family == b"SPST" and old_version < 436:
            if old_version >= 426:
                assert before[family][3:] == LAYOUT["legacy_payload"](read_stream(saved)[1]), (case, family)
            else:
                assert before[family] == after[family], (case, family)  # The actual old handling fixture has no module to acquire manufacturing fields.
        elif family == b"SDEF" and old_version < 431:
            assert int.from_bytes(before[family][3:5], "big") == 2 and int.from_bytes(after[family][3:5], "big") == 3
        elif family == b"SPDJ" and old_version < 434:
            assert before[family] == DESCENT["normalize_legacy"](after[family]), (case, family)
        else:
            assert before[family] == after[family], (case, family)
    assert identity(source) == selected
    target = context.reports / "heavy-joined.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CD; same-file cargo ownership/dimensions and actual original service disclosures. Historical indexes are immutable report/save boundaries, not current binary claims.",
                                     configuration=configuration, case=case, entry_index=index, producer=producer, source=selected, source_version=old_version, save_version=437,
                                     after=identity(saved), assertions=assertions, images=[identity(p) for p in sorted(context.images.glob("*.png"))], procurement_acceptance=False,
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def reload(configuration, proof_path):
    """Check ordinary reconstruction and native admission from that process's own actual resave.

    @param configuration Matching current native/product build.
    @param proof_path Exact current native checkpoint proof.
    @return Ordinary and own-resave proof preserving every physical owner and the configured snapshot.
    """
    proof = read(proof_path); assert proof["configuration"] == configuration; source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    context = current_run_context("space", "heavy-joined-reload-" + proof["case"]); previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    try:
        os.environ["OTTD_TEST_CONFIGURATION"] = configuration; ordinary(context, dict(name=proof["case"], snapshot=proof["after"]), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    saved = context.saves / "ordinary-resave.sav"; assert chunks(saved)[0] == 437
    resumed_context = context.for_feature("space", "heavy-joined-own-" + proof["case"]); assertions = native(resumed_context, configuration, saved)
    resumed = resumed_context.saves / "heavy-joined-437.sav"
    # The same-file configured catalogue is a new metadata family; all thirty existing physical owners remain byte-exact through both ordinary and own-resave readers.
    original_families = chunks(source)[1]
    for candidate in (saved, resumed):
        candidate_families = chunks(candidate)[1]
        for family in (*EXACT, b"CDIM"):
            assert original_families[family] == candidate_families[family], (proof["case"], family)
    target = context.reports / "heavy-joined-reload.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CD; ordinary and own-resave joined admission; integrated next-action/network acceptance remains9D.",
                                     configuration=configuration, case=proof["case"], proof=identity(proof_path), source=proof["after"], ordinary=identity(saved), resumed=identity(resumed), assertions=assertions,
                                     native_reports=[identity(p) for owner in (context, resumed_context) for p in sorted(owner.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Execute explicit small native/reload scopes; no full regression or newest-artifact selection is permitted."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("focused",) + tuple("native-" + k for k in GROUPS) + tuple("reload-" + k for k in GROUPS), required=True)
    parser.add_argument("--selector", nargs="+", type=Path); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists()
    if args.scope == "focused":
        reports = [focused(args.configuration)]
        args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CD; focused active contract membership.", configuration=args.configuration,
                                              scope=args.scope, complete=True, reports=[p.relative_to(ROOT).as_posix() for p in reports]), indent=2) + "\n", encoding="utf-8"); return
    mode, group = args.scope.split("-", 1); states = {}; reports = []
    # Only explicit complete current native scope membership may supply ordinary inputs; old indexes terminate at entry() above.
    for selector in args.selector or []:
        selected = read(selector); assert selected["complete"] and selected["configuration"] == args.configuration and selected["scope"].startswith("native-")
        for member in selected["reports"]:
            path = ROOT / member; proof = read(path); assert proof["case"] not in states; states[proof["case"]] = path
    for case in GROUPS[group]:
        reports.append(verify(args.configuration, case) if mode == "native" else reload(args.configuration, states[case])); print(args.configuration, args.scope, case, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CD; bounded actual original saved-state membership.", configuration=args.configuration,
                                          scope=args.scope, complete=True, reports=[p.relative_to(ROOT).as_posix() for p in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
