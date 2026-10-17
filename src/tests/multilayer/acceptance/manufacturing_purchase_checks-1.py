"""Verify one bounded ordinary paid Space purchase/facility/callback boundary.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8B
Heavy/Earth preparations explicitly declare initial Surface test content; actual traffic and paid production never claim procurement.
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
PHASES = ("focused", "prepared", "steel", "goods", "parts", "electronics", "depart", "queued", "reserved", "before", "complete", "repeat", "cancel", "guards", "earth", "earth-complete")
EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]
ENTRY = ROOT / "logs/manufacturing-materialization-verification.json"
ENTRY_SHA256 = "a52bdfb62a387d6a351256e8704273cc76cb70f51a09c73a2fdefcbde0a0e557"


def native(context, configuration, phase, model, source):
    """Run one actual loaded-world command boundary with bounded native execution and exact existing input ownership.

    @param context Explicit invocation runtime/log/save owner.
    @param configuration Matching completed native product/test configuration.
    @param phase One preparation, real flight, paid request/completion, refusal or preserved Earth boundary.
    @param model Existing immutable canonical role requested through ordinary purchase.
    @param source Exact accepted native predecessor; loading never fabricates a producer.
    @return Actual output checkpoint, accepted assertion count and explicitly declared initial Surface additions.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("manufacturing_purchase_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_PURCHASE_SOURCE=str(source), OTTD_PURCHASE_ACTION=phase,
                       OTTD_PURCHASE_MODEL=str(model), OTTD_TEST_CONFIGURATION=configuration)
    log = context.logs / "native-manufacturing-purchase.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.manufacturing-purchase-native]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    assert result.returncode == 0 and passed and "MANUFACTURE_PURCHASE native=true" in evidence, log
    assert "phase=" + phase in evidence and "model=" + str(model) in evidence, log
    additions = [dict(cargo_index=int(cargo), quantity=int(quantity)) for cargo, quantity in
                 re.findall(r"PURCHASE_SURFACE_FIXTURE cargo=(\d+) added=(\d+)", evidence)]
    # Only declared preparation branches can add initial Surface content; every other operation uses actual already saved physical stock.
    assert not additions or phase == "earth" or (phase == "prepared" and model in (16, 17)), log
    return context.saves / f"purchase-{phase}-{model}-427.sav", int(passed.group(1)), additions


def verify(configuration, phase, model=15, predecessor=None):
    """Bind one current ordinary purchase boundary to an explicit accepted original producer or current predecessor.

    @param configuration Completed matching native configuration.
    @param phase Focused contracts or one bounded actual native operation.
    @param model Existing canonical native role; first heavy mappings come from genuine completed producer saves.
    @param predecessor Exact accepted producer/current report; absent only for focused contracts.
    @return Accepted hash-bound report path.
    """
    context = current_run_context("space", f"manufacturing-purchase-verify-{phase}-{model}")
    if phase == "focused":
        assert predecessor is None
        log = context.logs / "manufacturing-purchase-focused.log"
        with log.open("wb") as sink:
            run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[manufacturing-purchase]"],
                       cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
        passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8"))
        assert passed and int(passed[2]) == 4, "All four actual current purchase cases must be present; an absent tag cannot borrow unrelated assertions."
        result = dict(configuration=configuration, phase=phase, model=model, assertions=int(passed[1]), cases=int(passed[2]), log=identity(log))
    else:
        assert predecessor is not None
        previous = json.loads(predecessor.read_text(encoding="utf-8"))
        assert previous["configuration"] == configuration and previous["save_version"] == 427
        if phase == "prepared":
            # The dated completed producer pins real paid factories/hulls/Engine mappings. Its overwritten live source/executable paths remain historical metadata.
            assert identity(ENTRY)["sha256"] == ENTRY_SHA256
            accepted = next(row for row in json.loads(ENTRY.read_text(encoding="utf-8"))["configurations"] if row["configuration"] == configuration)
            assert identity(predecessor) in accepted["reports"]
            expected = ("cancel", 16) if model in (15, 20) else ("depart", 19) if model == 19 else ("complete", model)
            assert (previous["phase"], previous["model"]) == expected
        else:
            assert previous["model"] == model
            expected = {"steel": "prepared", "goods": "steel", "parts": "goods", "electronics": "parts", "depart": "electronics",
                        "queued": "depart" if model in (16, 17) else "prepared", "reserved": "queued", "before": "reserved", "complete": "before",
                        "repeat": "complete", "cancel": "before", "guards": "prepared", "earth": "prepared", "earth-complete": "earth"}
            assert previous["phase"] == expected[phase]
        if phase in ("steel", "goods", "parts", "electronics", "depart"):
            assert model in (16, 17)
        if phase in ("guards", "earth", "earth-complete"):
            assert model == 15
        selected = previous["after"]; source = ROOT / selected["path"]; assert identity(source) == selected
        after, assertions, additions = native(context, configuration, phase, model, source)
        assert chunks(after)[0] == 427 and identity(source) == selected
        result = dict(configuration=configuration, phase=phase, model=model, assertions=assertions,
                      predecessor=identity(predecessor), source=selected, after=identity(after),
                      surface_fixture_added=additions,
                      additional_surface_starting_content=bool(additions) or previous.get("additional_surface_starting_content", False))
    result.update(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8B; ordinary command, actual paid facility/request and canonical product. Declared initial Surface test stock, then real freight/work; no procurement/factory-chain claim.",
                  save_version=427, surface_input_fixture=True, procurement_acceptance=False,
                  native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    report = context.reports / "manufacturing-purchase.json"
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one explicit bounded actual purchase phase/model/configuration with its exact accepted predecessor."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--phase", choices=PHASES, required=True)
    parser.add_argument("--model", type=int, choices=MODELS, default=15)
    parser.add_argument("--predecessor", type=Path)
    args = parser.parse_args()
    print(verify(args.configuration, args.phase, args.model, args.predecessor.resolve() if args.predecessor else None).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
