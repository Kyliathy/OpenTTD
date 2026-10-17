"""Dispatch focused probe action acceptance sequentially with explicit resumable report membership.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8B
Every child has its own bound and original save. This invocation owns its progress selector; completed earlier selectors and producer reports remain immutable.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("probe_role_checks-2.py")))
PERSISTENCE = runpy.run_path(str(Path(__file__).with_name("probe_role_checks-3.py")))


def selected(selector, configuration):
    """Resolve explicit already accepted native membership without a latest-output lookup.

    @param selector Exact existing current/partial invocation selector path.
    @param configuration Matching native build whose accepted reports are required.
    @return Report paths keyed by actual independent branch and phase.
    """
    result = {}; content = json.loads(selector.read_text(encoding="utf-8")); assert content["configuration"] == configuration
    # Only explicit report membership supplies a predecessor. A coincident phase in another native invocation or ordinary-resave report cannot substitute for this original.
    for name in content["reports"]:
        path = ROOT / name; proof = json.loads(path.read_text(encoding="utf-8")); assert proof["configuration"] == configuration
        key = proof["branch"], proof["phase"]; assert key not in result; result[key] = path
    return result


def write_selector(output, configuration, scope, reports, complete):
    """Publish only this current invocation's progress membership after an accepted child.

    @param output Explicit new invocation-owned selector, never a completed older selector.
    @param configuration Matching actual build.
    @param scope Native, ordinary/resumed or malformed acceptance dispatch.
    @param reports Actual already accepted current child reports.
    @param complete Whether every required child in this explicit scope passed.
    """
    value = dict(_documentation="Owner: r2mlvs10-asteroids-and-mining-probes.md Group8B; explicit sequential current accepted membership, with per-child process bounds and no latest lookup.",
                 configuration=configuration, scope=scope, complete=complete, reports=[path.relative_to(ROOT).as_posix() for path in reports])
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def execute(configuration, output, resume=None):
    """Exercise actual whole freight, rocket resupply, two paid returns and original service/pause/loss owners.

    @param configuration Matching current completed test/game build.
    @param output New current invocation-owned native progress selector.
    @param resume Optional explicit earlier partial selector; exact accepted predecessor membership is revalidated before reuse.
    @return All required actual accepted report paths.
    """
    states = selected(resume, configuration) if resume else {}; reports = list(states.values())

    def step(phase, parent=None, branch="main"):
        """Run one independently bounded actual native child and preserve its exact original predecessor graph.

        @param phase One actual focused/navigation/packet/service operation.
        @param parent Exact immutable accepted original/current predecessor, empty only for declared producer entry.
        @param branch Independent main, paused or lost-center action owner.
        @return Actual accepted report path.
        """
        key = branch, phase
        if key in states:
            proof = json.loads(states[key].read_text(encoding="utf-8"))
            if parent is not None:
                assert proof["predecessor"] == identity(parent)
            if "after" in proof:
                assert identity(ROOT / proof["after"]["path"]) == proof["after"]
            return states[key]
        result = SHARED["verify"](configuration, phase, parent, branch); reports.append(result); states[key] = result
        write_selector(output, configuration, "native", reports, False)
        print(configuration, branch, phase, result.relative_to(ROOT).as_posix(), flush=True)
        return result

    step("focused"); step("unload-orbital")
    parent = None
    # All parts arrive through one actual paid reusable-rocket flight before the original two paid pods descend and exhaust the original allowance. No direct orbital stock or service budget is granted.
    for phase in ("service-supply", "first-pending", "first-return", "mine-again", "second-pending", "service-partial", "service-complete", "repeat"):
        parent = step(phase, parent)
    parent = states["main", "service-partial"]
    for phase in ("service-pause", "service-resume", "service-complete", "repeat"):
        parent = step(phase, parent, "paused")
    parent = states["main", "second-pending"]
    for phase in ("service-center-loss", "service-after-loss"):
        parent = step(phase, parent, "lost")
    return reports


def reloads(configuration, selector, output):
    """Verify each active original boundary through ordinary reconstruction and its exact next action where applicable.

    @param configuration Matching current completed native build.
    @param selector Exact completed focused/native invocation selector.
    @param output New invocation-owned ordinary/resumed progress selector.
    @return Actual accepted ordinary/resumed reports.
    """
    states = selected(selector, configuration); reports = []
    checkpoints = (("main", "unload-orbital", None), ("main", "service-supply", None), ("main", "first-pending", "first-return"),
                   ("main", "first-return", "mine-again"), ("main", "mine-again", "second-pending"), ("main", "second-pending", "service-partial"),
                   ("main", "service-partial", "service-complete"), ("main", "service-complete", None),
                   ("paused", "service-pause", "service-resume"), ("paused", "service-resume", "service-complete"), ("paused", "service-complete", None),
                   ("lost", "service-center-loss", "service-after-loss"), ("lost", "service-after-loss", None))
    # Every resumed child loads its own actual ordinary resave; pending descent, partial/paused service and lost-center history cannot borrow the uninterrupted source.
    for branch, phase, completion in checkpoints:
        reports.append(PERSISTENCE["reload"](configuration, states[branch, phase], states[branch, completion] if completion else None))
        write_selector(output, configuration, "reloads", reports, False); print(configuration, branch, phase, flush=True)
    return reports


def rejects(configuration, selector, output):
    """Reject each independently malformed authority/progress/version boundary in separate full and copied-preview children.

    @param configuration Matching current completed native build.
    @param selector Exact completed native invocation containing actual partial onboard work.
    @param output New invocation-owned rejection progress selector.
    @return Actual accepted paired refusal reports.
    """
    states = selected(selector, configuration); reports = []
    # One original paid partial-service checkpoint supplies every independent mutation; no previous corrupted copy or open-world owner can supply another rejection's authority.
    for kind in PERSISTENCE["KINDS"]:
        reports.append(PERSISTENCE["reject"](configuration, states["main", "service-partial"], kind))
        write_selector(output, configuration, "rejects", reports, False); print(configuration, kind, flush=True)
    return reports


def main():
    """Run one current acceptance scope, retaining partial accepted membership without overwriting any older invocation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("native", "reloads", "rejects"), required=True)
    parser.add_argument("--selector", type=Path)
    parser.add_argument("--resume", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = args.output.resolve(); assert not output.exists()
    if args.scope == "native":
        assert args.selector is None; reports = execute(args.configuration, output, args.resume.resolve() if args.resume else None)
    else:
        assert args.selector is not None and args.resume is None
        runner = reloads if args.scope == "reloads" else rejects
        reports = runner(args.configuration, args.selector.resolve(), output)
    write_selector(output, args.configuration, args.scope, reports, True)
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
