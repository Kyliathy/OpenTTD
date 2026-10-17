"""Execute bounded native replacement scenarios sequentially and retain explicit immutable selectors.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC
Each native child has its own bound and original checkpoint. Dispatch never fabricates gameplay owners or selects a latest directory.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("manufacturing_replacement_checks-1.py")))


def execute(configuration, resume=None):
    """Exercise actual native requests, whole assets, final retirement and independent lifecycle branches.

    @param configuration Matching current completed game/test build.
    @param resume Optional explicit partial accepted selector; completed exact predecessor boundaries are reused without rerunning their native children.
    @return Exact accepted report paths for all current focused/native operations.
    """
    verify = SHARED["verify"]; entry = SHARED["ENTRY"]; assert identity(entry)["sha256"] == SHARED["ENTRY_SHA256"]
    accepted = next(row for row in json.loads(entry.read_text(encoding="utf-8"))["configurations"] if row["configuration"] == configuration)
    sources = {}
    # Only explicit immutable dated producer members authorize inputs; ordinary reload reports and non-main lifecycle branches cannot replace another original hull or paid bill.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member; proof = json.loads(path.read_text(encoding="utf-8"))
        # Native Earth clone completion is an explicit separate producer branch; the original main branch supplies orbital traffic and its independent full bills.
        eligible = (proof.get("variant") == "main" and proof.get("phase") in ("depart", "before", "complete")) or (proof.get("variant") == "earth" and proof.get("phase") == "earth-complete")
        if eligible and proof.get("save_version") == 428 and "after" in proof:
            sources[proof["phase"], proof["model"]] = path
    states = selected(resume) if resume is not None else {}
    reports = list(states.values())
    if ("main", "focused", 15) not in states:
        reports.append(verify(configuration, "focused"))

    def step(phase, model, parent, variant="main"):
        """Run one independently bounded native child and retain its exact accepted predecessor graph.

        @param phase One actual native policy/request/work/settlement operation.
        @param model Actual original paid canonical role.
        @param parent Exact immutable accepted predecessor report.
        @param variant Independent lifecycle branch/output owner.
        @return Current accepted report path.
        """
        key = variant, phase, model
        if key in states:
            proof = json.loads(states[key].read_text(encoding="utf-8"))
            assert proof["configuration"] == configuration and proof["predecessor"] == identity(parent)
            assert identity(ROOT / proof["after"]["path"]) == proof["after"]
            return states[key]
        result = verify(configuration, phase, model, parent, variant); reports.append(result); states[key] = result
        print(configuration, variant, model, phase, result.relative_to(ROOT).as_posix(), flush=True)
        return result

    # Every role consumes its own real separately delivered bill from the accepted clone-era traffic boundary, in an independent branch. Native same-model age-policy fixture is reported before payment.
    for model in SHARED["MODELS"]:
        parent = sources["depart", model]
        for phase in ("prepared", "queued", "reserved", "before", "complete", "repeat"):
            parent = step(phase, model, parent)
    parent = states["main", "before", 20]
    for phase in ("mutate", "complete", "repeat"):
        parent = step(phase, 20, parent, "edited")
    parent = states["main", "before", 20]
    for phase in ("transfer", "complete", "repeat"):
        parent = step(phase, 20, parent, "transferred")
    parent = states["main", "before", 19]
    for phase in ("source-loss", "cancel"):
        parent = step(phase, 19, parent, "lost")
    step("cancel", 16, states["main", "before", 16], "cancelled")
    parent = states["main", "prepared", 15]
    for phase in ("automatic", "reserved", "before", "complete", "repeat"):
        parent = step(phase, 15, parent, "automatic")
    parent = states["main", "before", 20]
    for phase in ("depart-source", "return-source", "repeat"):
        parent = step(phase, 20, parent, "departed")
    parent = sources["earth-complete", 15]
    for phase in ("earth-prepared", "earth-queued", "earth-before", "earth-complete"):
        parent = step(phase, 15, parent, "earth")
    step("earth-cancel", 15, states["earth", "earth-before", 15], "earth-cancelled")
    step("upgrade-clone", 16, sources["complete", 16])
    step("legacy-clone-complete", 16, sources["before", 16])
    return reports


def selected(selector):
    """Resolve exact accepted current native reports from an explicit selector.

    @param selector Previously accepted native invocation selector path.
    @return Mapping keyed by actual lifecycle branch, phase and canonical role.
    """
    result = {}
    # Selector membership is explicit; no glob/latest-path lookup may substitute a later invocation when an interrupted or lifecycle comparison expects an exact predecessor.
    for name in json.loads(selector.read_text(encoding="utf-8"))["reports"]:
        path = ROOT / name; proof = json.loads(path.read_text(encoding="utf-8")); result[proof["variant"], proof["phase"], proof["model"]] = path
    return result


def reloads(configuration, selector):
    """Compare current ordinary reconstruction and actual resaved final milestones in focused independent children.

    @param configuration Matching current completed game/test build.
    @param selector Exact original native invocation reports.
    @return Current ordinary/resumed proof reports.
    """
    verify = runpy.run_path(str(Path(__file__).with_name("manufacturing_replacement_checks-2.py")))["verify"]; states = selected(selector); reports = []
    # Main queued/partial/final owners cover every recipe; each last-tick child resumes its own actual ordinary resave rather than borrowing the uninterrupted input.
    for model in SHARED["MODELS"]:
        for phase in ("queued", "before", "complete"):
            reports.append(verify(configuration, states["main", phase, model], states["main", "complete", model] if phase == "before" else None))
    for variant, phase, model, completion in (("edited", "mutate", 20, None), ("transferred", "transfer", 20, "complete"), ("lost", "source-loss", 19, None),
                                             ("cancelled", "cancel", 16, None), ("automatic", "before", 15, "complete"), ("departed", "depart-source", 20, "return-source"),
                                             ("earth", "earth-queued", 15, None), ("earth", "earth-before", 15, "earth-complete"), ("earth", "earth-complete", 15, None),
                                             ("earth-cancelled", "earth-cancel", 15, None)):
        reports.append(verify(configuration, states[variant, phase, model], states[variant, completion, model] if completion else None))
    return reports


def rejects(configuration, selector):
    """Require independent full/copied-preview refusal for both actual containing producer families.

    @param configuration Matching current completed game/test build.
    @param selector Exact original native invocation reports.
    @return Accepted two-process malformed ownership/framing reports.
    """
    gate = runpy.run_path(str(Path(__file__).with_name("manufacturing_replacement_checks-3.py"))); states = selected(selector); reports = []
    # The same strict intent fields belong to different real physical owners. Validate each independently; malformed full load never stands in for copied-preview refusal.
    for owner, proof in (("orbital", states["main", "complete", 20]), ("earth", states["earth", "earth-before", 15])):
        for kind in gate["KINDS"]:
            reports.append(gate["verify"](configuration, proof, owner, kind)); print(configuration, owner, kind, flush=True)
    return reports


def main():
    """Run one explicit current acceptance scope and write its invocation-owned selector without overwriting another proof."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--scope", choices=("native", "reloads", "rejects", "legacy"), required=True)
    parser.add_argument("--selector", type=Path)
    parser.add_argument("--resume", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); args.output = args.output.resolve(); assert not args.output.exists()
    if args.scope == "native":
        assert args.selector is None; reports = execute(args.configuration, args.resume.resolve() if args.resume else None)
    else:
        assert args.selector is not None and args.resume is None
        if args.scope == "reloads":
            reports = reloads(args.configuration, args.selector.resolve())
        elif args.scope == "rejects":
            reports = rejects(args.configuration, args.selector.resolve())
        else:
            legacy = runpy.run_path(str(Path(__file__).with_name("manufacturing_replacement_checks-3.py")))["legacy"]; states = selected(args.selector.resolve())
            reports = [legacy(args.configuration, states["main", "upgrade-clone", 16]), legacy(args.configuration, states["earth", "earth-prepared", 15])]
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8BC; explicit sequential current invocation selectors, no latest lookup.",
                                          configuration=args.configuration, scope=args.scope, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")
    print(args.output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
