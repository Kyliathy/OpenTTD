"""Verify current mission reconstruction, local/network equivalence and player disclosure boundaries.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md task8B.4
Resumed children load their own ordinary resave. Static disclosure evidence supplements actual company-specific native inspector captures.
"""
import argparse
import json
import os
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("probe_mission_checks-1.py")))
AUDIT = runpy.run_path(str(Path(__file__).with_name("probe_role_checks-5.py")))
EXACT = AUDIT["EXACT"]
NEXT = {("local", "visit-mine-partial"): "visit-mine-next", ("network", "visit-mine-partial"): "visit-mine-next",
        ("local", "scan-partial"): "scan-complete", ("network", "scan-partial"): "scan-complete",
        ("local", "scan-complete"): "scan-transfer", ("local", "scan-transfer"): "company-reuse"}


def selected(selector, configuration):
    """Resolve exact completed native membership without inspecting arbitrary output directories.

    @param selector Exact accepted current native selector.
    @param configuration Matching native build.
    @return Reports keyed by real local/network branch and operation.
    """
    content = json.loads(selector.read_text(encoding="utf-8")); assert content["configuration"] == configuration and content["complete"] and len(content["reports"]) == 13
    result = {}
    # Each accepted original/action appears once. A partial invocation or unrelated output cannot supply a missing company/work/migration boundary.
    for member in content["reports"]:
        path = ROOT / member; proof = json.loads(path.read_text(encoding="utf-8")); assert proof["configuration"] == configuration
        key = proof["branch"], proof["action"]; assert key not in result; result[key] = path
    return result


def compare(left, right, label):
    """Compare all physical owners, with useful boundary identification on refusal.

    @param left Exact accepted original/uninterrupted checkpoint.
    @param right Its actual ordinary/resumed or local/network twin.
    @param label Explicit comparison boundary for diagnostics.
    @return Number of exact native owner comparisons.
    """
    before, after = chunks(left)[1], chunks(right)[1]
    # Company receipts, finite bodies, packets, leases, work, costs, engine/group/rule state and physical producer identity must all converge, rather than just a queue digest.
    for family in EXACT:
        assert before[family] == after[family], (label, family, left, right)
    return len(EXACT)


def reload(configuration, proof_path, completion=None):
    """Reconstruct one original boundary and optionally execute its actual next action from that new save.

    @param configuration Matching current game/test build.
    @param proof_path Exact accepted current mission report.
    @param completion Exact uninterrupted next-action report with proof_path as its original predecessor.
    @return Current immutable accepted ordinary/resumed report.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8")); assert proof["configuration"] == configuration
    scope = proof["branch"] + "-" + proof["action"]; context = current_run_context("space", "probe-mission-reload-" + scope)
    owner = context.for_feature("space", "probe-mission-ordinary-" + scope)
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    previous = os.environ.get("OTTD_TEST_CONFIGURATION"); os.environ["OTTD_TEST_CONFIGURATION"] = configuration
    try:
        ordinary(owner, dict(name=scope, snapshot=proof["after"]), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    resaved = owner.saves / "ordinary-resave.sav"; assert chunks(resaved)[0] == 430; compare(source, resaved, scope)
    result = dict(_documentation="Owner: r2mlvs10-asteroids-and-mining-probes.md task8B.4; actual ordinary and own-resave native continuation across30 original native owners.",
                  configuration=configuration, branch=proof["branch"], action=proof["action"], proof=identity(proof_path), source=proof["after"], ordinary=identity(resaved), exact_owner_families=len(EXACT))
    owners = [owner]
    if completion is not None:
        accepted = json.loads(completion.read_text(encoding="utf-8")); assert accepted["predecessor"] == identity(proof_path) and accepted["original"] == proof["original"]
        resumed_owner = context.for_feature("space", "probe-mission-resumed-" + scope)
        assertions, marker = SHARED["native"](resumed_owner, configuration, accepted["action"], resaved, proof["original"], proof["branch"] == "network")
        resumed = resumed_owner.saves / f'probe-mission-{accepted["action"]}-430.sav'; compare(ROOT / accepted["after"]["path"], resumed, ("resumed", scope))
        assert marker["elapsed"] == accepted["elapsed"] and marker["knowledge"] == accepted["knowledge"]
        result.update(completion=identity(completion), resumed_input=identity(resaved), resumed=identity(resumed), assertions=assertions); owners.append(resumed_owner)
    assert identity(source) == proof["after"]
    result["native_reports"] = [identity(path) for item in owners for path in sorted(item.reports.glob("native-*.json"))]
    result["images"] = [identity(path) for item in owners for path in sorted(item.images.glob("*.png"))]
    result["conversions"] = [identity(path) for item in owners for path in sorted(item.reports.glob("capture-conversions.jsonl"))]
    target = context.reports / "probe-mission-reload.json"; assert not target.exists(); target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); return target


def reloads(configuration, selector, output):
    """Verify all thirteen original boundaries and six independently resaved next actions in bounded sequential children.

    @param configuration Matching current completed native build.
    @param selector Exact completed native invocation selector.
    @param output New invocation-owned progress selector.
    """
    states = selected(selector, configuration); reports = []
    # Local and dispatched branches each reconstruct their own original company/work/body state; neither may borrow the other's uninterrupted input.
    for key, proof in states.items():
        completion = states[key[0], NEXT[key]] if key in NEXT else None
        reports.append(reload(configuration, proof, completion))
        value = dict(_documentation="Owner: r2mlvs10-asteroids-and-mining-probes.md task8B.4; explicit ordinary/resumed current progress membership.",
                     configuration=configuration, complete=False, reports=[path.relative_to(ROOT).as_posix() for path in reports])
        output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(configuration, key, flush=True)
    value["complete"] = True; output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def disclosure_audit():
    """Bind the sole orbital player quantity consumer and the absence of raw reserve access in script APIs.

    @return Hash-qualified actual disclosure/query/presentation sources and checked scope counts.
    """
    gui = ROOT / "src/asteroid_gui.cpp"; content = gui.read_text(encoding="utf-8")
    assert content.count("GetAsteroidQuantityDisclosure(_local_company, body.id)") == 1
    assert "if (quantity) draw(GetString(STR_SPACE_BODY_RESERVE" in content and "else draw(GetString(STR_SPACE_BODY_UNKNOWN" in content
    assert "body.initial_reserve" not in content and "body.remaining_reserve" not in content
    presentations = []; scripts = list((ROOT / "src/script").rglob("*.cpp")) + list((ROOT / "src/script").rglob("*.hpp")) + list((ROOT / "src/script").rglob("*.h"))
    # Inspect every script/API translation unit: no raw asteroid reserve or disclosure struct is exported to bypass the company receipt gate.
    for path in scripts:
        text = path.read_text(encoding="utf-8")
        assert not any(name in text for name in ("initial_reserve", "remaining_reserve", "AsteroidQuantityDisclosure")), path
    # Orbital GUI, viewport and tooltip consumers must use the one disclosure query. Existing underground deposit views have distinct terrestrial authority and remain outside this orbital gate.
    for path in (ROOT / "src").glob("*.cpp"):
        if "gui" not in path.name and "viewport" not in path.name:
            continue
        text = path.read_text(encoding="utf-8")
        if "GetAsteroidQuantityDisclosure(" in text:
            presentations.append(identity(path)); assert path == gui
        if path.name not in ("industry_gui.cpp", "underground_gui.cpp"):
            assert "initial_reserve" not in text and "remaining_reserve" not in text, path
    assert len(presentations) == 1
    return dict(player_quantity_consumers=presentations, script_units_checked=len(scripts),
                sources=[identity(ROOT / name) for name in ("src/asteroid.cpp", "src/asteroid.h", "src/asteroid_gui.cpp", "src/lang/english.txt")])


def main():
    """Run current ordinary/resumed acceptance only, keeping final dual-configuration aggregation a separate read-only operation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--selector", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = args.output.resolve(); assert not output.exists()
    reloads(args.configuration, args.selector.resolve(), output)


if __name__ == "__main__":
    main()
