"""Verify one bounded paid manufacturing request or Surface-freight phase.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group7
Each native child is bounded to110 seconds and consumes an exact predecessor report.
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
PHASES = ("focused", "setup", "steel", "goods", "robot", "electronics", "queued", "reserved", "outage", "restore", "cancel", "company")
EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]


def native(context, configuration, phase, model, source, corrupt=None, reject=False):
    """Run one actual loaded-world observer; setup declares Surface test content before actual flights, without granting orbital stock, hulls or receipts.

    @param context Exact invocation-owned runtime, log and artifact paths.
    @param configuration Matching completed native product/test configuration.
    @param phase One actual freight, request, reservation or lifecycle operation.
    @param model Immutable supported recipe model; shared freight phases use15.
    @param source Immutable accepted native checkpoint for this exact operation.
    @param corrupt Optional malformed copy for independent preview refusal.
    @param reject Whether full startup must refuse source before the observer runs.
    @return Produced checkpoint and assertion count, or None after expected refusal.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("manufacturing_reservation_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_MANUFACTURE_SOURCE=str(source), OTTD_MANUFACTURE_ACTION=phase,
                       OTTD_MANUFACTURE_MODEL=str(model), OTTD_TEST_CONFIGURATION=configuration)
    environment.pop("OTTD_MANUFACTURE_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_MANUFACTURE_CORRUPT"] = str(corrupt)
    log = context.logs / "native-manufacturing-reservation.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.manufacturing-reservation-native]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        # Only the actual native orbital loader's refusal proves the boundary; a startup/assertion failure cannot count as malformed-save acceptance.
        assert result.returncode != 0 and "All tests passed" not in evidence and "Broken savegame" in evidence and "orbital" in evidence.lower(), log
        return None
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    assert result.returncode == 0 and passed and "MANUFACTURE_RESERVATION native=true" in evidence, log
    if corrupt is not None:
        assert "preview_rejected=true unchanged=true" in evidence, log
        return None
    assert "phase=" + phase in evidence and "model=" + str(model) in evidence, log
    output = "ready" if phase == "setup" else phase
    return context.saves / f"manufacture-{output}-{model}-426.sav", int(passed.group(1))


def verify(configuration, phase, model=15, predecessor=None):
    """Bind one actual native phase to its explicit accepted producer and immutable artifact identities.

    @param configuration Completed native configuration.
    @param phase Focused contracts or one real paid-world operation.
    @param model Supported recipe model; Surface delivery/setup phases require15.
    @param predecessor Exact preceding accepted report, required for every native operation.
    @return Accepted bounded phase report path.
    """
    context = current_run_context("space", f"manufacturing-reservation-verify-{phase}-{model}")
    if phase == "focused":
        assert predecessor is None
        log = context.logs / "manufacturing-reservation-focused.log"
        with log.open("wb") as sink:
            run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"),
                "[manufacturing-reservation]"],
                cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
        passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8"))
        assert passed, log
        assert int(passed.group(2)) == 4, "The active group's four cases must exist; unrelated matching tags cannot mask an absent fixture."
        result = dict(configuration=configuration, phase=phase, model=model, assertions=int(passed.group(1)), cases=int(passed.group(2)), log=identity(log))
    else:
        assert predecessor is not None
        previous = json.loads(predecessor.read_text(encoding="utf-8")); assert previous["configuration"] == configuration
        expected = {"setup": ("connect",), "steel": ("setup",), "goods": ("steel",), "robot": ("goods",),
                    "electronics": ("robot",), "queued": ("electronics",), "reserved": ("queued",), "outage": ("queued",),
                    "restore": ("outage",), "cancel": ("reserved", "restore"), "company": ("reserved", "restore")}
        assert previous["phase"] in expected[phase], (phase, previous["phase"])
        if phase in ("setup", "steel", "goods", "robot", "electronics"):
            assert model == 15
        if phase == "setup":
            assert previous["save_version"] == 425
        else:
            assert previous["save_version"] == 426
            # Shared Surface-stock preparation belongs to model15; only the later typed request selects its actual recipe branch.
            if phase != "queued":
                assert previous["model"] == model
        selected = previous["after"]; source = ROOT / selected["path"]; assert identity(source) == selected
        after, assertions = native(context, configuration, phase, model, source)
        assert chunks(after)[0] == 426 and identity(source) == selected
        result = dict(configuration=configuration, phase=phase, model=model, assertions=assertions,
                      predecessor=identity(predecessor), source=selected, after=identity(after))
    result.update(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group7; declared starting Surface test content, then actual rocket freight, paid queued request and grant-qualified reservation; no procurement/factory-chain, work or materialization claim.",
                  surface_input_fixture=True, procurement_acceptance=False,
                  save_version=426, native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    report = context.reports / "manufacturing-reservation.json"
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one explicit bounded phase/model/configuration and print its accepted report path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--phase", choices=PHASES, required=True)
    parser.add_argument("--model", type=int, choices=MODELS, default=15)
    parser.add_argument("--predecessor", type=Path)
    args = parser.parse_args()
    print(verify(args.configuration, args.phase, args.model, args.predecessor.resolve() if args.predecessor else None).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
