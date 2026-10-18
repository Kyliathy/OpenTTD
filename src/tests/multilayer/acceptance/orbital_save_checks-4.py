"""Select and bind the bounded ten-kind orbital construction persistence matrix.

@file orbital_save_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group14.1
Actual paid construction and declared current-format codec variants are reported
separately. This matrix supplies no Surface procurement or end-to-end gameplay claim.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("orbital_save_checks-2.py")))
read, execute = SHARED["read"], SHARED["execute"]
KINDS = {"core": 6, "dock": 7, "storage": 8, "assembly": 9, "steel": 10, "goods": 11,
         "power": 12, "fuel": 13, "refinery": 16, "ice": 17}
REVISION = "3"  # Earlier diagnostics predate exact empty IDs and competing unfunded-site selection.


def producer(configuration, name):
    """Select exactly one matching original configuration in a dated producer index.

    @param configuration Actual original native configuration. @param name Exact accepted index stem.
    @return Index identity and its original selected configuration row.
    """
    path = ROOT / "logs" / (name + ".json")
    (row,) = [row for row in read(path)["configurations"] if row["configuration"] == configuration]
    return identity(path), row


def accepted_report(reference):
    """Admit an exact original report identity before selecting a checkpoint.

    @param reference Explicit frozen report identity. @return Its unchanged original JSON.
    """
    assert identity(ROOT / reference["path"]) == reference
    return read(ROOT / reference["path"])


def checkpoint(configuration, case):
    """Resolve one explicit original paid milestone or declared same-recipe codec input.

    @param configuration Matching original producer configuration.
    @param case Canonical kind followed by partial or complete. @return Producer/source/kind/declaration tuple.
    """
    name, milestone = case.split("-")
    assert milestone in ("partial", "complete")
    partial = milestone == "partial"
    kind, declared = KINDS[name], False
    if name == "core":
        path = ROOT / "logs/orbital-core-verification.json"
        reference = read(path)["configurations"][configuration]["reports"][0]
        report = accepted_report(reference)
        source = report["completed"]
        if partial:
            # The real retained415 first delivery precedes the original Core
            # producer's paid frame. Select its exact original sibling rather
            # than rewriting any header or reconstructing a first station.
            source = identity((ROOT / report["input"]["path"]).with_name("construction-delivery-415.sav"))
        owner = identity(path)
    elif name == "storage":
        path = ROOT / "logs/orbital-assembly-verification.json"
        reference = read(path)["reports"][configuration]["preparation"]
        report = accepted_report(reference)
        source = report["checkpoints"]["partial"] if partial else report["frame"]
        owner = identity(path)
    elif name == "dock":
        path = ROOT / "logs/orbital-handoff-debug-verification.json"
        references = read(path)["reports"]
        # The handoff contains both old Release and Debug report paths. Match
        # each actual report's configuration, never its directory timestamp.
        candidates = [identity(ROOT / relative) for relative in references if relative.endswith("/orbital-dock-goods.json")]
        if configuration == "RelWithDebInfo":
            dated = ROOT / "logs/orbital-dock-verification.json"
            candidates += [identity(ROOT / relative) for relative in read(dated)["reports"] if relative.endswith("/orbital-dock-goods.json")]
        reference, = [ref for ref in candidates if accepted_report(ref)["configuration"] == configuration]
        report = accepted_report(reference)
        source = report["source"] if partial else report["complete"]
        owner = identity(path)
    elif name in ("power", "fuel", "assembly"):
        index = "orbital-art-paid-manufacture-verification" if name == "assembly" else "orbital-art-paid-construction-verification"
        owner, row = producer(configuration, index)
        stage = name + ("-partial" if partial else "-frame")
        # Functional Power and Assembly have explicit paid activation; FuelStore
        # stays a spent inert frame because its operating role is unsupported.
        if not partial and name != "fuel":
            stage = name + "-active"
        source = row["checkpoints"][stage]
    else:
        owner, row = producer(configuration, "orbital-processor-verification")
        stage = "native/steel" if partial else "native/" + name + "-active"
        if name == "ice":
            stage = "native/steel" if partial else "native/steel-frame"
        # Only these missing partial-kind transport cases borrow the identical
        # inactive copied processor recipe. Their report cannot claim that a
        # command paid for that substituted role or that an ice factory ran.
        declared = name == "ice" or (partial and name in ("goods", "refinery"))
        source = row["checkpoints"][stage]
    assert identity(ROOT / source["path"]) == source
    return owner, source, kind, declared


def member(configuration, case):
    """Run one exact matrix member or reuse its explicit previously accepted membership.

    @param configuration Actual native product configuration. @param case Exact original matrix case.
    @return Immutable membership identity; successful cases are never rerun merely by binding.
    """
    label = "release" if configuration == "RelWithDebInfo" else "debug"
    output = ROOT / "logs" / ("orbital-save-" + label + "-" + case + "-" + REVISION + ".json")
    if output.exists():
        result = read(output)["report"]
        accepted_report(result)
        return identity(output)
    owner, source, kind, declared = checkpoint(configuration, case)
    result = execute(configuration, source, owner, kind, declared)
    output.write_text(json.dumps(dict(_documentation=__doc__, case=case, report=result), indent=2) + "\n", encoding="utf-8")
    print(configuration, case, result["path"], flush=True)
    return identity(output)


def bind():
    """Admit all ten kinds' partial/completed states without upgrading codec fixtures to paid evidence.

    @return Complete two-configuration construction persistence matrix and exact original economic counters.
    """
    configurations = []
    # Each build must reconstruct every original or explicitly declared kind
    # itself. Equal output in one build cannot replace another process's owner
    # checks, original packet references, capital or activation countdown.
    for configuration in ("RelWithDebInfo", "Debug"):
        cases = []
        for name, kind in KINDS.items():
            for phase in ("partial", "complete"):
                case = name + "-" + phase
                label = "release" if configuration == "RelWithDebInfo" else "debug"
                member_path = ROOT / "logs" / ("orbital-save-" + label + "-" + case + "-" + REVISION + ".json")
                proof = accepted_report(read(member_path)["report"])
                owner, source, expected_kind, declared = checkpoint(configuration, case)
                assert proof["configuration"] == configuration and proof["complete"]
                assert proof["original"] == source and proof["producer"] == owner
                assert proof["declared_codec_fixture"] == declared and proof["paid_kind_delivery_acceptance"] != declared
                for reference in [proof["source"], proof["after"], proof["application"], proof["terminal"], *proof["sources"], *proof["native_processes"]]:
                    assert identity(ROOT / reference["path"]) == reference
                state = proof["construction"]
                assert state["kind"] == kind == expected_kind
                if phase == "partial":
                    assert state["phase"] == 0 and state["consumed"] == [0, 0] and state["module"] is None
                    assert sum(slot["quantity"] for slot in state["inputs"]) > 0
                else:
                    assert state["phase"] == 1 and all(state["consumed"])
                    assert all(slot["quantity"] == 0 and not slot["references"] for slot in state["inputs"])
                    if name in ("storage", "fuel", "ice"):
                        assert state["module"] is None
                    else:
                        assert state["module"] is not None and state["module"]["activation"] == 0
                cases.append(dict(case=case, membership=identity(member_path), report=read(member_path)["report"],
                                  declared_codec_fixture=declared, construction=state, exact_families=proof["exact_families"],
                                  native_legacy_upgrades=proof["native_legacy_upgrades"]))
        configurations.append(dict(configuration=configuration, cases=cases))
    return dict(_documentation=__doc__, complete=True, module_kinds=10, cases_per_configuration=20, configurations=configurations,
                declared_codec_cases=["goods-partial", "refinery-partial", "ice-partial", "ice-complete"],
                initial_content_procurement_acceptance=False,
                unsupported_functionality={"FuelStore": "frame only", "IceProcessor": "codec fixture only; original paid Refinery PICE evidence stays separate"})


def main():
    """Run at most four sequential cases per bounded burst or bind the completed explicit matrix."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--cases", nargs="+")
    parser.add_argument("--bind", type=Path)
    args = parser.parse_args()
    if args.bind:
        assert args.configuration is None and args.cases is None
        output = (ROOT / args.bind).resolve()
        assert output.is_relative_to(ROOT / "logs") and not output.exists()
        output.write_text(json.dumps(bind(), indent=2) + "\n", encoding="utf-8")
        print(identity(output), flush=True)
    else:
        assert args.configuration is not None and args.cases is not None and 1 <= len(args.cases) <= 4
        # Bounded batches preserve sequential source ownership and keep each
        # child under90s; the caller can checkpoint between focused bursts.
        for case in args.cases:
            member(args.configuration, case)


if __name__ == "__main__":
    main()
