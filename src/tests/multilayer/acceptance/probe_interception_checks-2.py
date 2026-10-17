"""Independently qualify reflected pursuit/attachment saves and exact native next decisions.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group15
Paid inputs retain their original resource owners; explicit edge staging is not earned paid travel.
"""
import argparse
import json
from pathlib import Path
import re
import runpy

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "probe_interception_checks-1.py"))
DECODER = runpy.run_path(str(HERE / "probe_repeat_checks-3.py"))
AUDIT = LEAF["AUDIT"]
SOURCES = tuple(dict.fromkeys((*DECODER["BASE"]["SOURCES"], "src/space_approach.h", "src/asteroid_generation.h",
                             "src/asteroid_generation.cpp", "src/saveload/space_order_sl.cpp", "src/tests/probe_interception_native.cpp",
                             "src/tests/multilayer/acceptance/probe_repeat_checks-3.py",
                             *(f"src/tests/multilayer/acceptance/probe_interception_checks-{n}.py" for n in (1, 2)))))


def snapshot(saved):
    """Decode original saved edge motion, actual mission deadline and paid physical owners.

    @param saved Exact retained ordinary438 checkpoint identity.
    @return Detached geometry, reflected remainders and original paid cargo/service history.
    """
    state = DECODER["snapshot"](saved)
    _, raw = read_stream(ROOT / saved["path"])

    def integer(position, width, signed=False):
        """Read one bounded original network-order field.

        @param position Exact same-file field start.
        @param width Published native scalar width.
        @param signed Whether signed velocity is preserved.
        @return Detached original integer.
        """
        return int.from_bytes(raw[position:position + width], "big", signed=signed)

    _, start, _, _ = riff_location(raw, b"ASTR")
    body = start + 51
    row, = [r for r in DECODER["TABLES"](raw, b"VEHS") if r["index"] == 16]
    fields = {re.sub(r"\[\d+\]", "", f["path"]): f for f in row["fields"]}
    deadline = fields["space.mission.approach_ticks_left"]
    state.update(max_x=(integer(start + 2, 4) - 3) * 65536,
                 velocity=[integer(body + 14, 4, True), integer(body + 18, 4, True)], remainder=[raw[body + 22], raw[body + 23]],
                 deadline=integer(deadline["position"], deadline["end"] - deadline["position"]))
    return state


def configuration(name, selectors, references):
    """Qualify exact paid family inputs, geometry stages and actual ordinary/resumed next decisions.

    @param name Current native build configuration.
    @param selectors Explicit native/own/resume/focused selection in prerequisite order.
    @param references Verified direct identities; dated producer graphs remain immutable boundaries.
    @return Qualified process counts, saved checkpoints and independently decoded original owners.
    """
    rows = {}
    proofs = []
    focused = None
    # Explicit selector membership prevents a repeated approach result from substituting the distinct latched reserve/work/custody boundary.
    for scope, selector in zip(LEAF["SCOPES"], selectors):
        selected = LEAF["read"](selector)
        AUDIT["inspect"](selected, references)
        assert selected["complete"] and selected["configuration"] == name and selected["scope"] == scope
        assert len(selected["reports"]) == len(LEAF["SCOPES"][scope])
        # Each child must have its own actual native invocation, exact original input and retained output, rather than merely a successful selector.
        for phase, relative in zip(LEAF["SCOPES"][scope], selected["reports"]):
            path = ROOT / relative
            proof = LEAF["read"](path)
            AUDIT["inspect"](proof, references)
            assert proof["complete"] and proof["configuration"] == name and proof["scope"] == scope and proof["phase"] == phase
            assert proof["geometry_fixture"] and not proof["earned_paid_travel"] and not proof["added_stock"]
            assert proof["dated_index"] == LEAF["predecessor"](name, phase)[0] and len(proof["native_reports"]) == 1
            process = LEAF["read"](ROOT / proof["native_reports"][0]["path"])
            AUDIT["inspect"](process, references)
            assert process["exit_status"] == 0 and process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
            assert process["argv"][1:] == [LEAF["TAGS"] if scope == "focused" else "[.probe-interception-native]"]
            terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
            assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
            proof["_path"] = identity(path)
            proofs.append(proof)
            if scope == "focused":
                focused = proof
                continue
            assert proof["source"] in process["inputs"] and proof["after"] in process["artifacts"]
            parent = rows.get("native/" + LEAF["PARENTS"].get(phase, "")) if scope == "native" else rows["native/" + phase] if scope == "own" else rows["own/" + LEAF["PARENTS"][phase]]
            if parent:
                assert proof["parent"] == parent["_path"] and proof["source"] == parent["after"]
            else:
                assert phase.endswith("stage") and proof["parent"] is None and proof["source"] == LEAF["predecessor"](name, phase)[1]
            rows[scope + "/" + phase] = proof
            if scope == "own":
                AUDIT["compare"](proof["source"], proof["after"])
            elif scope == "resume":
                AUDIT["compare"](rows["native/" + phase]["after"], proof["after"])
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 9 and focused
    checkpoints = {key: proof["after"] for key, proof in rows.items()}
    decoded = {key: snapshot(saved) for key, saved in checkpoints.items()}
    # Reflection is independently literal: (16384+73)/74 earns222 Q16 units with remainder29, then the edge reverses X. Only the real next opportunity advances time/work.
    for key, state in decoded.items():
        phase = key.split("/")[1]
        latched = phase.startswith("latched")
        next_step = phase.endswith("next")
        initial = snapshot(LEAF["predecessor"](name, phase)[1])
        assert state["body_position"] == [state["max_x"] - (222 if next_step else 0), initial["body_position"][1]]
        assert state["velocity"] == [-16384 if next_step else 16384, 0] and state["remainder"] == [29 if next_step else 73, 0]
        assert state["elapsed"] == initial["elapsed"] + int(next_step)
        assert state["phase"] == (2 if latched else 1) and state["lease"] == (16 if latched else 1048575)
        assert state["work"] == ((37 + int(next_step)) if latched else 0)
        assert state["quantity"] == state["extracted"] == (2 if latched else 0) and state["reserve"] == (1758 if latched else 1760)
        assert state["manifests"] == initial["manifests"] and state["pods"] == initial["pods"] and not state["descents"]
        assert state["next_pod"] == initial["next_pod"] and state["next_descent"] == initial["next_descent"] and state["conserved_ore"] == initial["conserved_ore"]
        assert [(p["packet"], p["quantity"], p["tile"]) for p in state["packets"]] == [(p["packet"], p["quantity"], p["tile"]) for p in initial["packets"]]
        assert all(b["age"] >= a["age"] for a, b in zip(initial["packets"], state["packets"]))
        if latched:
            assert state["offset"] == initial["offset"] and state["position"] == [p + offset for p, offset in zip(state["body_position"], state["offset"])]
        else:
            assert state["deadline"] == initial["deadline"] - int(next_step)
    return dict(configuration=name, processes=9, focused_assertions=focused["assertions"], focused_cases=focused["cases"],
                native_assertions=sum(p["assertions"] for p in proofs if p is not focused), selectors=[identity(p) for p in selectors],
                reports=[p["_path"] for p in proofs], checkpoints=checkpoints, decoded_checkpoints=decoded,
                physical_owner_comparisons=120, metadata_comparisons=8)


def main():
    """Freeze current reflected continuations only when both builds preserve exact physical owners and next decisions."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=4, required=True)
    parser.add_argument("--debug", type=Path, nargs=4, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    references = {}
    release = configuration("RelWithDebInfo", [p.resolve() for p in args.release], references)
    debug = configuration("Debug", [p.resolve() for p in args.debug], references)
    # Every original stage, reflected step and ordinary/resumed counterpart must agree across builds, including SPOR admission, native VEHS decisions and actual packet owners.
    for key, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][key])
    assert release["decoded_checkpoints"] == debug["decoded_checkpoints"]
    args.output.write_text(json.dumps(dict(_documentation=__doc__, save_version=438, configurations=[release, debug], sources=[identity(ROOT / p) for p in SOURCES],
                                         direct_references=list(references.values()), dated_producer_indexes=[identity(ROOT / row[0]) for row in LEAF["ENTRIES"].values()],
                                         cross_configuration_physical_comparisons=240, cross_configuration_metadata_comparisons=16,
                                         geometry_fixture=True, earned_paid_travel=False, added_stock=False, inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
