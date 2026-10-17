"""Run one bounded real paid Assembly facility phase or its focused public contracts.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group6B
Each continuation consumes an exact preceding report, preserving immutable original inputs.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from runner import current_run_context, run_native

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]
OUTPUT = {"steel": "assembly-steel-425.sav", "goods": "assembly-active-425.sav", "activation": "assembly-active-425.sav",
          "connect": "assembly-connected-425.sav", "outage": "assembly-restored-425.sav",
          "disconnect": "assembly-removed-425.sav", "company": "assembly-company-425.sav"}


def native(context, configuration, phase, source, corrupt=None, reject=False):
    """Execute an actual paid-world boundary without fixture cargo, positions or construction receipts.

    @param context Exact invocation/runtime/artifact owner.
    @param configuration Completed native build configuration.
    @param phase Real material, activation, connection, outage or lifecycle operation.
    @param source Immutable original or immediately preceding native checkpoint.
    @param corrupt Optional malformed input for independent copied-preview refusal.
    @param reject Whether full startup must refuse source before running the observer.
    @return Completed native checkpoint and assertion count, or None after expected rejection.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("assembly_facility_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_ASSEMBLY_FACILITY_SOURCE=str(source), OTTD_ASSEMBLY_FACILITY_ACTION=phase)
    environment.pop("OTTD_ASSEMBLY_FACILITY_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_ASSEMBLY_FACILITY_CORRUPT"] = str(corrupt)
    log = context.logs / "native-assembly-facility.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.assembly-facility-native]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        # Only an actual native orbital loader error counts; unrelated test/startup failures cannot prove schema or ownership refusal.
        assert result.returncode != 0 and "All tests passed" not in evidence and "Broken savegame" in evidence and "orbital" in evidence.lower(), log
        return None
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    assert result.returncode == 0 and passed and "ASSEMBLY_FACILITY native=true" in evidence, log
    if corrupt is not None:
        assert "preview_rejected=true unchanged=true" in evidence, log
        return None
    assert "phase=" + phase in evidence, log
    return context.saves / OUTPUT[phase], int(passed.group(1))


def verify(configuration, phase, predecessor=None):
    """Bind one ordered physical producer phase to exact input and native process evidence.

    @param configuration Completed game/test build configuration.
    @param phase Focused contracts or one real paid-world operation.
    @param predecessor Exact prior accepted report, required after the initial Steel delivery.
    @return Accepted bounded phase report path.
    """
    context = current_run_context("space", "assembly-facility-verify-" + phase)
    if phase == "focused":
        log = context.logs / "assembly-facility-focused.log"
        with log.open("wb") as sink:
            run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"),
                "[assembly-facility],[orbital-activation-contract],[orbital-dock-contract],[orbital-service-contract],[orbital-assembly-contract],[space-construction-contract]"],
                cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
        passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8"))
        assert passed, log
        result = {"configuration": configuration, "phase": phase, "assertions": int(passed.group(1)), "cases": int(passed.group(2)), "log": identity(log)}
    else:
        if phase == "steel":
            assert predecessor is None
            source = ROOT / "src/tests/fixtures/demos/space/space-module-frame-418.sav"
        else:
            assert predecessor is not None
            previous = json.loads(predecessor.read_text(encoding="utf-8"))
            assert previous["configuration"] == configuration
            expected = {"goods": ("steel",), "activation": ("goods",), "connect": ("goods", "activation"),
                        "outage": ("connect",), "disconnect": ("connect",), "company": ("connect",)}
            assert previous["phase"] in expected[phase]
            selected = previous["partial"] if phase == "activation" else previous["after"]
            source = ROOT / selected["path"]; assert identity(source) == selected
        original = identity(source)
        after, assertions = native(context, configuration, phase, source)
        assert chunks(after)[0] == 425 and identity(source) == original
        result = {"configuration": configuration, "phase": phase, "assertions": assertions, "source": original, "after": identity(after)}
        if predecessor is not None:
            result["predecessor"] = identity(predecessor)
        if phase == "goods":
            result["partial"] = identity(context.saves / "assembly-activating-425.sav")
            result["frame"] = identity(context.saves / "assembly-frame-425.sav")
        if phase == "outage":
            result["unpowered"] = identity(context.saves / "assembly-unpowered-425.sav")
    result.update(_documentation="Owner: r2mlvs11 Group6B; actual paid Assembly activation/source boundary, no vehicle manufacturing claim.",
                  save_version=425, native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    report = context.reports / ("assembly-facility-" + phase + ".json")
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one explicit configuration/phase and print its accepted report path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--phase", choices=("focused", *OUTPUT), required=True)
    parser.add_argument("--predecessor", type=Path)
    args = parser.parse_args()
    print(verify(args.configuration, args.phase, args.predecessor.resolve() if args.predecessor else None).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
