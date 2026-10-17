"""Verify exact native paid replacement boundaries from immutable accepted traffic inputs.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC
No new stock, cash, hull, paid work or pose is granted. The native same-model age-policy fixture is reported independently of economic/calendar simulation.
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

MODELS = (15, 19, 20, 16, 17)
PHASES = ("focused", "purchase-boundary", "prepared", "automatic", "queued", "reserved", "before", "complete", "repeat", "cancel", "mutate", "transfer", "source-loss", "depart-source", "return-source",
          "earth-prepared", "earth-queued", "earth-before", "earth-complete", "earth-cancel", "upgrade-clone", "legacy-clone-complete")
VARIANTS = ("main", "automatic", "edited", "transferred", "lost", "cancelled", "departed", "earth", "earth-cancelled")
EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"] + (b"ENGN", b"EIDS", b"ERNW", b"GRPS")
ENTRY = ROOT / "logs/manufacturing-clone-verification.json"
ENTRY_SHA256 = "d94f2bd7a327463043762c99bb066b2503325eaf89e799b324bd130f73e26c7b"


def original_paid_rows(source):
    """Read exact original428 paid hull receipts without selecting a newest or coincidental stored body.

    @param source Immutable accepted native428 checkpoint.
    @return Original fixed67-byte receipt rows after strict family/count admission.
    """
    version, families = chunks(source); assert version == 428
    data = families[b"SVST"][3:]; count = data[0]; assert len(data) == 1 + count * 67
    return [data[1 + index * 67:1 + (index + 1) * 67] for index in range(count)]


def native(context, configuration, phase, model, source, original, corrupt=None, reject=False):
    """Run one exact bounded native original/job/settlement observer.

    @param context Explicit unique output/native data-home owner.
    @param configuration Matching current game/test build.
    @param phase One supported bounded native operation.
    @param model Original and requested same-role paid recipe.
    @param source Exact immutable native predecessor save.
    @param original Exact original paid hull incarnation captured by its accepted producer.
    @param corrupt Optional independent malformed save for copied-preview refusal while source remains open.
    @param reject Whether full startup must refuse source before its observer can run.
    @return Output save, assertion count and parsed exact original/product/asset/age-fixture metadata.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("manufacturing_replacement_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_REPLACEMENT_SOURCE=str(source), OTTD_REPLACEMENT_ACTION=phase,
                       OTTD_REPLACEMENT_MODEL=str(model), OTTD_REPLACEMENT_ORIGINAL_ID=str(original["vehicle"]),
                       OTTD_REPLACEMENT_ORIGINAL_GENERATION=str(original["generation"]), OTTD_TEST_CONFIGURATION=configuration)
    environment.pop("OTTD_REPLACEMENT_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_REPLACEMENT_CORRUPT"] = str(corrupt)
    log = context.logs / "native-manufacturing-replacement.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.manufacturing-replacement-native]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    if reject:
        assert result.returncode != 0 and "All tests passed" not in evidence and "Broken savegame" in evidence and any(owner in evidence.lower() for owner in ("replacement", "orbital", "launch assembly")), log
        return None
    if corrupt is not None:
        assert result.returncode == 0 and passed and "preview_rejected=true unchanged=true" in evidence, log
        return None
    marker = re.search(r"MANUFACTURE_REPLACEMENT native=true phase=(\S+) model=(\d+) original_id=(\d+) original_generation=(\d+) original_alive=(true|false) assets=(\d+) product_id=(\d+) product_generation=(\d+) age_fixture_days=(\d+) procurement=false", evidence)
    # An unrelated successful test, absent tag or observer that never reached actual owned-state checks cannot provide this boundary's evidence.
    assert result.returncode == 0 and passed and int(passed[2]) == 1 and marker, log
    assert (marker[1], int(marker[2])) == (phase, model), log
    observed = dict(vehicle=int(marker[3]), generation=int(marker[4])); assert observed == original, log
    metadata = dict(original=observed, original_alive=marker[5] == "true", asset_digest=int(marker[6]),
                    product=dict(vehicle=int(marker[7]), generation=int(marker[8])), age_fixture_days=int(marker[9]))
    return context.saves / f"replacement-{phase}-{model}-429.sav", int(passed[1]), metadata


def verify(configuration, phase, model=15, predecessor=None, variant="main"):
    """Bind one current focused/native boundary to its exact immutable accepted predecessor.

    @param configuration Matching current completed game/test build.
    @param phase Focused contracts or actual bounded loaded-world operation.
    @param model Actual paid canonical role.
    @param predecessor Exact accepted original/current report; no latest-path lookup.
    @param variant Independent edited/transferred/lost/cancelled branch owner.
    @return Exact accepted hash-qualified proof report.
    """
    assert phase in PHASES and model in MODELS and variant in VARIANTS
    context = current_run_context("space", f"manufacturing-replacement-verify-{variant}-{phase}-{model}")
    report = context.reports / "manufacturing-replacement.json"; assert not report.exists()
    if phase in ("focused", "purchase-boundary"):
        assert predecessor is None
        log = context.logs / f"manufacturing-replacement-{phase}.log"
        tag = "[manufacturing-replacement]" if phase == "focused" else "[manufacturing-purchase]"
        with log.open("wb") as sink:
            run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag],
                       cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
        passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8"))
        assert passed and int(passed[2]) == (9 if phase == "focused" else 4), "Every requested actual focused boundary contract must be compiled."
        result = dict(configuration=configuration, phase=phase, model=model, variant=variant, assertions=int(passed[1]), cases=int(passed[2]), log=identity(log))
    else:
        assert predecessor is not None
        previous = json.loads(predecessor.read_text(encoding="utf-8")); assert previous["configuration"] == configuration and previous["model"] == model
        original = previous["original"]; selected = previous["after"]; source = ROOT / selected["path"]; assert identity(source) == selected
        if phase in ("prepared", "earth-prepared", "upgrade-clone", "legacy-clone-complete"):
            # Freeze traversal at the accepted dated clone report identity. Its current executable/source paths legitimately change during replacement implementation.
            entry_phase = {"prepared": "depart", "earth-prepared": "earth-complete", "upgrade-clone": "complete", "legacy-clone-complete": "before"}[phase]
            assert previous["phase"] == entry_phase and previous["save_version"] == 428
            assert identity(ENTRY)["sha256"] == ENTRY_SHA256
            accepted = next(row for row in json.loads(ENTRY.read_text(encoding="utf-8"))["configurations"] if row["configuration"] == configuration)
            assert identity(predecessor) in accepted["reports"]
            if phase == "earth-prepared":
                # The accepted Earth completion can also contain an older stored bootstrap miner. Follow its actual preceding Assembling receipt, then require that same incarnation completed; pool order/newest heuristics cannot choose a different paid source.
                staged = ROOT / previous["source"]["path"]; assert identity(staged) == previous["source"]
                candidates = [row for row in original_paid_rows(staged) if row[12] == 1 and int.from_bytes(row[41:43], "big") == model]
                assert len(candidates) == 1
                row = candidates[0]; original = dict(vehicle=int.from_bytes(row[:4], "big"), generation=int.from_bytes(row[4:12], "big"))
                completed = [item for item in original_paid_rows(source) if item[:12] == row[:12]]
                assert len(completed) == 1 and completed[0][12] == 2
        else:
            assert previous["save_version"] == 429
        after, assertions, metadata = native(context, configuration, phase, model, source, original)
        assert chunks(after)[0] == 429 and identity(source) == selected
        assert metadata["age_fixture_days"] == 0 or phase in ("prepared", "earth-prepared")
        result = dict(configuration=configuration, phase=phase, model=model, variant=variant, assertions=assertions,
                      predecessor=identity(predecessor), source=selected, after=identity(after), **metadata)
    result.update(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8BC; actual paid replacement, original whole assets and once-only native retirement. Explicit native age-policy fixture is not century-long economy/calendar acceptance.",
                  save_version=429, procurement_acceptance=False, native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    assert not report.exists(); report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one exact phase/model/branch/build and optional immutable predecessor."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--phase", choices=PHASES, required=True)
    parser.add_argument("--model", type=int, choices=MODELS, default=15)
    parser.add_argument("--variant", choices=VARIANTS, default="main")
    parser.add_argument("--predecessor", type=Path)
    args = parser.parse_args()
    print(verify(args.configuration, args.phase, args.model, args.predecessor.resolve() if args.predecessor else None, args.variant).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
