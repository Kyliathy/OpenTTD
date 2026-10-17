"""Execute bounded clone scenarios sequentially and retain explicit immutable selectors.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BB
Each native child remains bounded; this dispatcher never fabricates gameplay state or selects a latest artifact.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("manufacturing_clone_checks-1.py")))


def execute(configuration):
    """Exercise all real roles and independent source lifecycle branches in task order.

    @param configuration Matching completed game/test configuration.
    @return Explicit accepted current invocation report paths, including focused contracts.
    """
    verify = SHARED["verify"]
    entry = json.loads(SHARED["ENTRY"].read_text(encoding="utf-8"))
    accepted = next(row for row in entry["configurations"] if row["configuration"] == configuration)
    sources = {}
    # Only immutable dated producer members authorize starting a new scenario; no directory search may substitute another source hull or paid job.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member
        proof = json.loads(path.read_text(encoding="utf-8"))
        # Ordinary reconstruction reports retain phase labels too; only original427 native checkpoints carry the actual producer schema and source hull.
        if proof.get("phase") in ("complete", "before") and proof.get("save_version") == 427 and "after" in proof:
            sources[proof["phase"], proof["model"]] = path
    reports = [verify(configuration, "focused")]; states = {}

    def step(phase, model, parent, variant="main"):
        """Run one separately bounded native child and retain its exact accepted predecessor graph.

        @param phase Actual command/work/navigation boundary.
        @param model Original canonical paid model.
        @param parent Exact accepted predecessor report path.
        @param variant Distinct artifact owner for a source lifecycle branch.
        @return Accepted child report path.
        """
        result = verify(configuration, phase, model, parent, variant)
        reports.append(result); states[variant, phase, model] = result
        print(configuration, variant, model, phase, flush=True)
        return result

    # Each role receives a distinct wholly Surface-delivered bill through real flights before its actual ordinary clone request and normal shared work.
    for model in SHARED["MODELS"]:
        parent = sources["complete", model]
        phases = ["prepared", "clear-core", "steel", "goods"]
        if model in (16, 17):
            phases += ["parts", "electronics"]
        elif model == 20:
            phases += ["service-metal", "service-electronics"]
        phases += ["depart", "queued", "reserved", "before", "complete", "repeat"]
        for phase in phases:
            parent = step(phase, model, parent)
    parent = states["main", "queued", 15]
    # A renamed/reordered source cannot rewrite the frozen intent; its branch must reach a separate independent completion and repeat.
    for phase in ("mutate", "reserved", "before", "complete", "repeat"):
        parent = step(phase, 15, parent, "mutated")
    # Removal and genuine company takeover are distinct historical-source lifecycles; neither can transfer source assets into the new hull.
    for model, variant, operation in ((19, "lost", "source-loss"), (20, "transferred", "transfer")):
        parent = states["main", "before", model]
        for phase in (operation, "complete", "repeat"):
            parent = step(phase, model, parent, variant)
    step("cancel", 16, states["main", "before", 16], "cancelled")
    step("guards", 15, states["main", "depart", 15], "guards")
    earth = step("earth", 15, states["main", "depart", 15], "earth")
    step("earth-complete", 15, earth, "earth")
    step("upgrade", 16, sources["complete", 16], "legacy")
    step("legacy-complete", 16, sources["before", 16], "legacy")
    return reports


def reconstruct(configuration, selectors):
    """Check relevant ordinary save boundaries and all eight interrupted lifecycle completions.

    @param configuration Matching completed native configuration.
    @param selectors Explicit immutable accepted scenario report paths.
    @return Accepted ordinary/resumed reports.
    """
    verify = runpy.run_path(str(Path(__file__).with_name("manufacturing_clone_checks-2.py")))["verify"]
    proofs = [(path, json.loads(path.read_text(encoding="utf-8"))) for path in selectors]
    completions = {(proof["variant"], proof["model"]): path for path, proof in proofs if proof["phase"] == "complete"}
    result = []
    # Every persisted admission/work/product and destructive lifecycle boundary gets ordinary reconstruction; only actual immediately preceding last-tick saves claim resumed convergence.
    for path, proof in proofs:
        if proof["phase"] not in ("queued", "reserved", "before", "complete", "mutate", "source-loss", "transfer", "cancel", "earth", "earth-complete", "upgrade", "legacy-complete"):
            continue
        completion = None
        if proof["phase"] in ("before", "source-loss", "transfer"):
            completion = completions[proof["variant"], proof["model"]]
        result.append(verify(configuration, path, completion))
        print(configuration, "ordinary", proof["variant"], proof["model"], proof["phase"], flush=True)
    return result


def main():
    """Dispatch one sequential current-group burst and write its explicit selector under repository logs."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--mode", choices=("native", "reload"), required=True)
    parser.add_argument("--selectors", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    if args.mode == "native":
        assert args.selectors is None
        reports = execute(args.configuration)
    else:
        selected = json.loads(args.selectors.read_text(encoding="utf-8")); assert selected["configuration"] == args.configuration
        reports = reconstruct(args.configuration, [ROOT / path for path in selected["reports"]])
    output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8BB; explicit sequential current invocation selectors, no latest lookup.",
                                     configuration=args.configuration, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
