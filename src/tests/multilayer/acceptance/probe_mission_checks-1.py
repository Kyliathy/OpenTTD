"""Run current paid scan/company and genuine network-dispatch mission boundaries sequentially.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md task8B.4
Every child loads an explicit accepted original. Network twins use original server command queues, while full socket integration remains with its owning later gate.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
from PIL import Image

from space_preview_checks import ROOT, chunks, identity
from runner import current_run_context, run_native, save_native_capture

ACTION_INDEX_SHA = "ba308b8be30c756efb57125a32c55992fd428bdd28e94742bd08fcdeceeb04a2"
REPLACEMENT_SHA = "6a6e1bc848c551c9523b2970043af302c8ff5759f1963d6bc347302eaf648878"


def entry(configuration, action):
    """Resolve an exact original without selecting a newest hull, matching filename or open-world pool alias.

    @param configuration Matching current native configuration.
    @param action Explicit legacy, mining-sequence or original scan entry.
    @return Accepted producer boundary, actual source identity and original hull incarnation.
    """
    if action == "legacy-unknown":
        source = ROOT / "src/tests/fixtures/demos/space/space-launch-hub-1990-408.sav"
        assert identity(source)["sha256"] == "2f9c50badca4d4582850569231ec1db47bb677a73a1a6ad85f0ab2d0e22ab611"
        version, owners = chunks(source); assert version == 408 and b"SPKN" not in owners
        return identity(source), identity(source), dict(vehicle=0, generation=0)
    index = ROOT / ("logs/probe-order-actions-verification.json" if action == "visit-mine-partial" else "logs/manufacturing-replacement-verification.json")
    assert identity(index)["sha256"] == (ACTION_INDEX_SHA if action == "visit-mine-partial" else REPLACEMENT_SHA)
    accepted = next(row for row in json.loads(index.read_text(encoding="utf-8"))["configurations"] if row["configuration"] == configuration)
    eligible = []
    # Only the completed accepted native predecessor can supply this physical original; ordinary reports or lifecycle forks cannot impersonate the paid entry.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member; proof = json.loads(path.read_text(encoding="utf-8"))
        if "after" not in proof:
            continue
        if action == "visit-mine-partial":
            good = proof.get("branch") == "main" and proof.get("phase") == "first-return"
        else:
            good = proof.get("variant") == "main" and proof.get("phase") == "complete" and proof.get("model") == 19
        if good:
            original = proof["original"] if action == "visit-mine-partial" else proof["product"]
            eligible.append((member, proof["after"], {key: original[key] for key in ("vehicle", "generation")}))
    assert len(eligible) == 1
    return eligible[0]


def native(context, configuration, action, source, original, network):
    """Exercise one actual loaded-world command/work boundary with strict process and original identity bounds.

    @param context Current invocation artifact/runtime owner.
    @param configuration Matching current completed native build.
    @param action Explicit paid sequence, scan, company lifecycle or old unknown migration.
    @param source Actual original/predecessor save, never an uninterrupted substitute for a resave.
    @param original Original paid hull incarnation, or explicit zero sentinel for the pre-knowledge world.
    @param network True dispatches actual network-queued edits and labels their continuation consistently.
    @return Actual assertion count and process marker.
    """
    context.prepare_runtime(); (context.runtime_dir / "scripts/game_start.scr").write_text("probe_mission_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_PROBE_MISSION_SOURCE=str(source),
                       OTTD_PROBE_MISSION_ACTION=action, OTTD_PROBE_MISSION_VEHICLE=str(original["vehicle"]), OTTD_PROBE_MISSION_GENERATION=str(original["generation"]))
    environment.pop("OTTD_PROBE_MISSION_NETWORK", None)
    if network:
        environment["OTTD_PROBE_MISSION_NETWORK"] = "1"
    log = context.logs / "probe-mission.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.probe-mission-native]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    marker = re.search(r"PROBE_MISSION native=true action=(\S+) network=(true|false) vehicle=(\d+) generation=(\d+) elapsed=(\d+) knowledge=(\d+)", evidence)
    assert result.returncode == 0 and passed and int(passed[2]) == 1 and marker, log
    assert marker[1] == action and (marker[2] == "true") == network and int(marker[3]) == original["vehicle"] and int(marker[4]) == original["generation"], log
    # Container encoding retains the exact native process provenance and pixel identity; it cannot invent a second render or change the company's disclosure view.
    for source_image in context.images.glob("*.ppm"):
        with Image.open(source_image) as raster:
            save_native_capture(context, raster, source_image, source_image.with_suffix(".png"))
    return int(passed[1]), dict(elapsed=int(marker[5]), knowledge=int(marker[6]))


def verify(configuration, action, parent=None, network=False):
    """Bind one actual current mission stage to its original paid or genuine legacy predecessor.

    @param configuration Matching current native configuration.
    @param action Explicit independently bounded native operation.
    @param parent Exact already accepted predecessor report, absent only for original entry.
    @param network Whether this is the actual server-queue twin.
    @return Current immutable accepted report path.
    """
    branch = "network" if network else "local"; context = current_run_context("space", f"probe-mission-{branch}-{action}")
    target = context.reports / "probe-mission.json"; assert not target.exists()
    if parent is None:
        assert action in ("legacy-unknown", "visit-mine-partial", "scan-partial"); predecessor, selected, original = entry(configuration, action)
    else:
        proof = json.loads(parent.read_text(encoding="utf-8")); assert proof["configuration"] == configuration and proof["branch"] == branch
        predecessor, selected, original = identity(parent), proof["after"], proof["original"]
    source = ROOT / selected["path"]; assert identity(source) == selected
    assertions, metadata = native(context, configuration, action, source, original, network)
    after = context.saves / f"probe-mission-{action}-430.sav"; assert chunks(after)[0] == 430 and identity(source) == selected
    result = dict(_documentation="Owner: r2mlvs10-asteroids-and-mining-probes.md task8B.4; actual original paid mission, company knowledge and native server-queue dispatch. No pose, cargo, service allowance or company funding grants.",
                  configuration=configuration, action=action, branch=branch, save_version=430, predecessor=predecessor, source=selected,
                  original=original, after=identity(after), assertions=assertions, **metadata,
                  images=[identity(path) for path in sorted(context.images.glob("*.png"))],
                  conversions=[identity(path) for path in sorted(context.reports.glob("capture-conversions.jsonl"))],
                  native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); return target


def execute(configuration, output):
    """Run original local phases first, then independent network twins, publishing only accepted current membership.

    @param configuration Matching current completed native build.
    @param output New invocation-owned progress selector.
    @return All thirteen accepted actual native reports.
    """
    reports = []; states = {}

    def step(action, parent=None, network=False):
        """Run one bounded sequential child and record its exact completed predecessor.

        @param action Current native operation.
        @param parent Exact accepted physical predecessor.
        @param network True selects the original server command queues.
        @return Current accepted proof path.
        """
        result = verify(configuration, action, parent, network); reports.append(result); states[network, action] = result
        value = dict(_documentation="Owner: r2mlvs10-asteroids-and-mining-probes.md task8B.4; explicit current accepted native membership, no latest lookup.",
                     configuration=configuration, complete=False, reports=[path.relative_to(ROOT).as_posix() for path in reports])
        output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(configuration, network, action, result.relative_to(ROOT).as_posix(), flush=True)
        return result

    step("legacy-unknown")
    # Local originals and network twins independently load identical accepted paid inputs. Each actual next action loads its own branch's saved state.
    for network in (False, True):
        step("visit-mine-next", step("visit-mine-partial", network=network), network)
        partial = step("scan-partial", network=network); step("scan-cancel", partial, network)
        complete = step("scan-complete", partial, network)
        if not network:
            step("company-reuse", step("scan-transfer", complete), False)
    value = json.loads(output.read_text(encoding="utf-8")); assert len(reports) == 13; value["complete"] = True
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); return reports


def main():
    """Select one explicit current native mission operation or a complete sequential bounded dispatch."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--action")
    parser.add_argument("--parent", type=Path)
    parser.add_argument("--network", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.output:
        output = args.output.resolve(); assert not output.exists() and args.action is None and args.parent is None and not args.network
        execute(args.configuration, output)
    else:
        assert args.action is not None
        print(verify(args.configuration, args.action, args.parent.resolve() if args.parent else None, args.network).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
