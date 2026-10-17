"""Verify one bounded original orbital factory work/completion boundary.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8
Consumes immutable Group7 Surface-delivered stock evidence; no new procurement or input fixture is introduced.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from runner import current_run_context, run_native
from space_preview_checks import ROOT, chunks, identity

MODELS = (15, 19, 20, 16, 17)
PHASES = ("focused", "halfway", "before", "complete", "repeat", "outage", "restore", "cancel", "company", "depart", "second")
EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]
ENTRY = ROOT / "logs/manufacturing-reservation-verification.json"
ENTRY_SHA256 = "44aa6aa8862405f8df0ee32b4a08a6d59d9392c3288f82f0c9922a593a913a33"


def native(context, configuration, phase, model, source, corrupt=None, reject=False):
    """Run one original loaded-world operation with exact immutable inputs and bounded actual native execution.

    @param context Explicit invocation-owned native runtime, logs and artifacts.
    @param configuration Matching completed native game/test configuration.
    @param phase Exact work, completion, retry or lifecycle boundary.
    @param model Original immutable requested recipe.
    @param source Genuine original native checkpoint for this operation.
    @param corrupt Optional malformed owned copy for independent preview refusal.
    @param reject Whether startup must refuse source before its actual observer can execute.
    @return Accepted actual output checkpoint/assertion count, or None after an expected refusal.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("manufacturing_materialization_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_MATERIALIZATION_SOURCE=str(source), OTTD_MATERIALIZATION_ACTION=phase,
                       OTTD_MATERIALIZATION_MODEL=str(model), OTTD_TEST_CONFIGURATION=configuration)
    environment.pop("OTTD_MATERIALIZATION_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_MATERIALIZATION_CORRUPT"] = str(corrupt)
    log = context.logs / "native-manufacturing-materialization.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.manufacturing-materialization-native]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        # Native family validation must refuse the actual malformed stream; an unrelated startup or test failure cannot masquerade as acceptance.
        assert result.returncode != 0 and "All tests passed" not in evidence and "Broken savegame" in evidence, log
        assert any(family in evidence.lower() for family in ("orbital", "manufactur", "launch assembly")), log
        return None
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    assert result.returncode == 0 and passed and "MANUFACTURE_MATERIALIZATION native=true" in evidence, log
    if corrupt is not None:
        assert "preview_rejected=true unchanged=true" in evidence, log
        return None
    assert "phase=" + phase in evidence and "model=" + str(model) in evidence, log
    return context.saves / f"manufacture-{phase}-{model}-427.sav", int(passed.group(1))


def verify(configuration, phase, model=15, predecessor=None):
    """Bind one accepted current work/materialization boundary to its original explicit producer.

    @param configuration Completed native configuration.
    @param phase Focused contracts or one bounded original-world operation.
    @param model Supported original canonical recipe.
    @param predecessor Exact accepted predecessor report; absent only for focused contracts.
    @return Accepted hash-bound phase report path.
    """
    context = current_run_context("space", f"manufacturing-materialization-verify-{phase}-{model}")
    if phase == "focused":
        assert predecessor is None
        log = context.logs / "manufacturing-materialization-focused.log"
        with log.open("wb") as sink:
            run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[manufacturing-materialization]"],
                       cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
        passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8"))
        assert passed and int(passed[2]) == 6, "All six actual current-group cases must exist; an absent fixture cannot borrow another tag's successful result."
        result = dict(configuration=configuration, phase=phase, model=model, assertions=int(passed[1]), cases=int(passed[2]), log=identity(log))
    else:
        assert predecessor is not None
        previous = json.loads(predecessor.read_text(encoding="utf-8")); assert previous["configuration"] == configuration and previous["model"] == model
        expected = {"halfway": ("reserved",), "before": ("halfway",), "complete": ("before",), "repeat": ("complete",),
                    "outage": ("halfway",), "restore": ("outage",), "cancel": ("halfway",), "company": ("complete",),
                    "depart": ("complete",), "second": ("depart",)}
        assert previous["phase"] in expected[phase]
        if phase == "halfway":
            # Preserve the dated426 public producer boundary; its old live source/executable paths legitimately change in this427 implementation.
            assert identity(ENTRY)["sha256"] == ENTRY_SHA256 and previous["save_version"] == 426
            accepted = json.loads(ENTRY.read_text(encoding="utf-8"))
            records = next(row for row in accepted["configurations"] if row["configuration"] == configuration)["reports"]
            assert identity(predecessor) in records
        else:
            assert previous["save_version"] == 427
        if phase in ("depart", "second"):
            assert model == 19
        selected = previous["after"]; source = ROOT / selected["path"]; assert identity(source) == selected
        after, assertions = native(context, configuration, phase, model, source)
        assert chunks(after)[0] == 427 and identity(source) == selected
        result = dict(configuration=configuration, phase=phase, model=model, assertions=assertions,
                      predecessor=identity(predecessor), source=selected, after=identity(after))
    result.update(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8; original Surface-delivered reservation, actual common-grant paid work and once-only native materialization. Starting Surface stock was declared in dated Group7 evidence; no new procurement/factory-chain claim.",
                  save_version=427, surface_input_fixture=True, procurement_acceptance=False,
                  native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    report = context.reports / "manufacturing-materialization.json"
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one explicit bounded original phase/model/configuration and print its accepted proof path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--phase", choices=PHASES, required=True)
    parser.add_argument("--model", choices=MODELS, type=int, default=15)
    parser.add_argument("--predecessor", type=Path)
    args = parser.parse_args()
    print(verify(args.configuration, args.phase, args.model, args.predecessor.resolve() if args.predecessor else None).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
