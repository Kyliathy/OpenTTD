"""Prove the portal/TBM plans' runtime Completion Conditions without granting master human acceptance.

@file target_completion_audit.py
@see r2mlvs3-sealed-portals-and-deep-rail.md Completion Conditions and Group18.
@see r2mlvs4-tunnel-boring-machines.md Completion Conditions and Group22.
One guarded child at a time; exact existing integration evidence retains its dated meaning.
"""

import argparse
import json
import struct
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from bounded_performance import observe, verify_observation
from ground_final_boundaries import HANDOFFS
from ground_performance_acceptance import strict_reference, dictionaries
from ground_performance import parse_fields
from support.save_tables import chunks, read_stream

CONDITIONS = {
    "portal-solid-and-rail-only": ("[portal-build]", "[portal-routing]", "[portal-work-state]", "[portal-tbm-access]", "[portal-road]"),
    "first-entry-retreat-without-depot": ("[tbm-commission]", "[tbm-movement]", "[tbm-loop]"),
    "deep-calendar-and-real-crossing": ("[portal-fleet-date]",),
    "reachable-area-and-45-degree-sweeps": ("[tbm-area]", "[tbm-route]", "[tbm-coverage]", "[tbm-backing]"),
    "real-wagons-paid-work-and-holds": ("[tbm-work]", "[tbm-support]", "[tbm-conservation]"),
    "restricted-acquisition": ("[tbm-acquisition]", "[portal-script-commission]"),
    "ordinary-freight-and-final-land-use": ("[portal-movement]", "[tbm-reuse]"),
}
LEGACY = "src/tests/fixtures/portals/legacy-original-connected-370.sav"
LEGACY_SHA = "6585485b0e52a9cfbf3eac9513b15f212b83237fb7383a1267d8924958f68e2c"


def portal_snapshot(path):
    """Decode original portal geometry/owner bytes independently of native loading."""
    version, stream = read_stream(path)
    tables = {row["name"]: row for row in chunks(stream)}
    dimensions = struct.unpack(">IIB", tables[b"MAPS"]["rows"][1])
    types, owners, fields = (tables[key]["payload"] for key in (b"MAPT", b"MAPO", b"MAP5"))
    portals = [(index, value >> 4, owners[index], fields[index]) for index, value in enumerate(types) if value >> 4 == 11]
    assert portals
    return version, dimensions, portals, len(tables)


def prepare_legacy(context):
    """Select the existing ordinary load observer on the save's actual256map;no synthetic geometry/profile."""
    context.config_path.write_text("[game_creation]\ngameplay_profile = classic\n[gui]\npause_on_newgame = true\nautosave_on_exit = false\n", encoding="utf-8")
    (context.runtime_dir / "scripts/game_start.scr").write_text("ground_performance_start\n", encoding="utf-8")


def audit(output):
    """Record each actual condition's focused native proof and original370 compatibility on both current readers."""
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    receipts = {}
    for key, (path, expected) in {**HANDOFFS, "final-boundaries": (
            "logs/performance/ground-maps/20261019-native-01/final-boundaries-verification.json",
            "36e2d85892055cef1c4fbd31aea0b619f3c1eda43959acc8166ae714c4ee5fac")}.items():
        reference = identity(ROOT / path)
        assert reference["sha256"] == expected
        receipts[key] = {"reference": reference, "actual": json.loads(strict_reference(reference).read_text(encoding="utf-8"))}
    performance = receipts["performance"]["actual"]
    production = {ref["path"]: ref for ref in performance["current_production_and_physical_artifacts"] if ref["path"].startswith("src/") and not ref["path"].startswith("src/tests/")}
    for ref in production.values():
        strict_reference(ref)
    selectors = tuple(dict.fromkeys(selector for values in CONDITIONS.values() for selector in values))
    cpp = {path: path.read_text(encoding="utf-8-sig") for path in (ROOT / "src/tests").glob("*.cpp")}
    sources = {*production, "src/tests/ground_performance_fixture.h"}
    for selector in selectors:
        matches = [path for path, text in cpp.items() if selector in text]
        assert matches, selector
        sources.update(path.relative_to(ROOT).as_posix() for path in matches)
    records, cases, assertions = [], 0, 0
    for cfg in ("Debug", "RelWithDebInfo"):
        for selector in selectors:
            context, actual, baseline = observe(cfg, selector, {}, sources, "target-completion", __file__)
            report = context.output("reports", "condition.json")
            report.write_text(json.dumps(actual, indent=2) + "\n", encoding="utf-8")
            verify_observation(actual, baseline, selector, {})
            records.append({"configuration": cfg, "selector": selector, "report": identity(report), "actual": actual})
            cases += actual["native"]["cases"]
            assertions += actual["native"]["assertions"]
            print(json.dumps({"configuration": cfg, "selector": selector, "cases": actual["native"]["cases"], "assertions": actual["native"]["assertions"]}), flush=True)
    legacy_ref = identity(ROOT / LEGACY)
    assert legacy_ref["sha256"] == LEGACY_SHA
    old_version, dimensions, portals, old_chunks = portal_snapshot(strict_reference(legacy_ref))
    assert old_version == 370 and dimensions == (256, 256, 4)
    compatibility = []
    for cfg in ("Debug", "RelWithDebInfo"):
        selectors_legacy = {"OTTD_GROUND_PERFORMANCE_PROFILE": "classic", "OTTD_GROUND_PERFORMANCE_INPUT": str(ROOT / LEGACY)}
        context, actual, baseline = observe(cfg, "[.ground-performance-map]", selectors_legacy, sources, "target-legacy-compatibility", __file__, prepare_legacy)
        report = context.output("reports", "legacy-compatibility.json")
        report.write_text(json.dumps(actual, indent=2) + "\n", encoding="utf-8")
        text = verify_observation(actual, baseline, "[.ground-performance-map]", selectors_legacy)
        assert len(parse_fields(text, "GROUND_PERFORMANCE_TICK")) == 640
        saves = {name: identity(context.saves / (name + ".sav")) for name in ("initial-world", "initial-repeat", "final-world", "final-repeat")}
        version, actual_dimensions, actual_portals, owner_chunks = portal_snapshot(strict_reference(saves["initial-world"]))
        assert version == 434 and actual_dimensions == dimensions and actual_portals == portals and owner_chunks == 126
        for phase in ("initial", "final"):
            assert all(saves[phase + "-world"][field] == saves[phase + "-repeat"][field] for field in ("bytes", "sha256"))
        compatibility.append({"configuration": cfg, "report": identity(report), "actual": actual, "saves": saves,
                              "source": legacy_ref, "original_version": old_version, "original_chunk_count": old_chunks,
                              "current_version": version, "current_owner_chunks": owner_chunks, "dimensions": dimensions, "unchanged_portal_endpoints": portals})
        cases += actual["native"]["cases"]
        assertions += actual["native"]["assertions"]
    assert identity(ROOT / LEGACY) == legacy_ref and len(records) == 2 * len(selectors)
    result = {"_documentation": "r2mlvs3mp-master-plan.md12.1/12.2;runtime conditions belong to Plans3/4;humanmaster11.7/demo12.6remainseparate",
              "controller": identity(__file__), "accepted_runtime_conditions": True, "conditions": CONDITIONS,
              "fresh_condition_records": records, "current_original370_compatibility": compatibility,
              "closed_integration_receipts": {key: row["reference"] for key, row in receipts.items()},
              "inherited_recovery_and_controls": "final-boundaries:current9recoverycases+wholelifecontrols+exchange/industrylifecycleeachbuild",
              "inherited_files": "performance:file-reconstruction28freshcases/14originalinputs;complete126owners/firstresumedwork/independentactivefinals",
              "inherited_consumers": "genuine-bootstrap:paidnormalStandard/Deep;technicalart:69selectedfamilies;multiplayer:60actualgroundtube/thermalpeercases",
              "native_cases": cases, "native_assertions": assertions, "isolated_invocations": len(records) + len(compatibility),
              "remaining_master_gates": ["11.7 human art feedback", "12.6 refreshed demo feedback and gated fixture selection"],
              "production_sources": list(production.values())}
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "cases": cases, "assertions": assertions, "invocations": result["isolated_invocations"]}), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True)
    audit(parser.parse_args().output)
