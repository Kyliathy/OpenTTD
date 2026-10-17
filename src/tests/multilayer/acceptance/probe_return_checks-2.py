"""Qualify shared mining departure, actual return-circle entry and separately declared refusal fixtures.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group11
Only original paid packets enter native travel checkpoints; controlled foreign/full inputs emit no candidate world.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream
from launch_center_checks import riff_location

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "probe_return_checks-1.py"))
BASE = runpy.run_path(str(HERE / "probe_extraction_checks-2.py"))
AUDIT = LEAF["AUDIT"]
SOURCES = tuple(dict.fromkeys((*BASE["SOURCES"], "src/space_departure.h", "src/space_approach.cpp", "src/space_descent.cpp", "src/launch_center.cpp",
                             "src/tests/probe_return_native.cpp", "src/tests/orbital_release_geometry.cpp", "src/tests/space_logistics.cpp", "src/tests/space_descent.cpp",
                             *(f"src/tests/multilayer/acceptance/probe_return_checks-{n}.py" for n in (1, 2)))))


def snapshot(saved):
    """Decode original retained cargo and the exact same-file authorized Surface anchor.

    @param saved Original ordinary438 return checkpoint identity.
    @return Actual packet provenance, current body/craft points and independent circle distance.
    """
    state = BASE["snapshot"](saved); _, raw = read_stream(ROOT / saved["path"])
    _, start, _, end = riff_location(raw, b"LCTR"); count = int.from_bytes(raw[start + 8:start + 10], "big"); assert start + 10 + count * 15 == end
    row, = [p for p in range(start + 10, end, 15) if int.from_bytes(raw[p:p + 2], "big") == 4 and int.from_bytes(raw[p + 2:p + 10], "big") == 3]
    tile = int.from_bytes(raw[row + 10:row + 14], "big"); _, start, _, _ = riff_location(raw, b"ASTR")
    width = int.from_bytes(raw[start + 2:start + 6], "big"); height = int.from_bytes(raw[start + 6:start + 10], "big")
    assert tile < width * height; anchor = [tile % width * 65536, tile // width * 65536]
    state.update(anchor=anchor, distance_squared=sum((p - a)**2 for p, a in zip(state["position"], anchor)))
    return state


def configuration(name, selectors, references):
    """Qualify exact native route membership, unchanged resaves and explicit unsaved boundary controls.

    @param name Current native executable configuration.
    @param selectors Five exact native/own/resume/focused/boundaries selectors.
    @param references Checked direct identities; historical indexes remain immutable boundaries only.
    @return Qualified native/fixture process counts, original saves and independent retained-cargo geometry.
    """
    rows = {}; proofs = []; focused = boundary = None
    for scope, path in zip(LEAF["SCOPES"], selectors):
        selected = LEAF["read"](path); AUDIT["inspect"](selected, references)
        assert selected["complete"] and selected["configuration"] == name and selected["scope"] == scope and len(selected["reports"]) == len(LEAF["SCOPES"][scope])
        for phase, relative in zip(LEAF["SCOPES"][scope], selected["reports"]):
            path = ROOT / relative; proof = LEAF["read"](path); AUDIT["inspect"](proof, references)
            assert proof["complete"] and proof["configuration"] == name and proof["scope"] == scope and proof["phase"] == phase
            assert proof["added_stock"] == (scope == "boundaries") and proof["dated_index"] == identity(LEAF["ENTRY"]) and len(proof["native_reports"]) == 1
            process = LEAF["read"](ROOT / proof["native_reports"][0]["path"]); AUDIT["inspect"](process, references)
            assert process["exit_status"] == 0 and process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
            assert process["argv"][1:] == [LEAF["TAGS"] if scope == "focused" else "[.probe-return-native]"]
            terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
            assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
            proof["_path"] = identity(path); proofs.append(proof)
            if scope == "focused":
                focused = proof; assert proof["cases"] == 19; continue
            assert proof["source"] in process["inputs"]
            parent = rows.get("native/" + LEAF["PARENTS"].get(phase, "")) if scope == "native" else rows["native/" + phase] if scope in ("own", "boundaries") else rows["own/" + LEAF["PARENTS"][phase]]
            if parent:
                assert proof["parent"] == parent["_path"] and proof["source"] == parent["after"]
            else:
                assert phase == "exit" and proof["parent"] is None and proof["source"] == LEAF["predecessor"](name)[1]
            if scope == "boundaries":
                boundary = proof; assert proof["after"] is None
                assert "PROBE_RETURN boundaries=true foreign=true full=true supplied_fixture_units=" in terminal and "no_save=true" in terminal
                assert not any(p["path"].endswith(".sav") for p in process["artifacts"]); continue
            assert proof["after"] in process["artifacts"]; rows[scope + "/" + phase] = proof
            if scope == "own":
                AUDIT["compare"](proof["source"], proof["after"])
            elif scope == "resume":
                AUDIT["compare"](rows["native/" + phase]["after"], proof["after"])
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 8 and focused and boundary
    checkpoints = {k: p["after"] for k, p in rows.items()}; decoded = {k: snapshot(saved) for k, saved in checkpoints.items()}
    initial = snapshot(LEAF["predecessor"](name)[1]); original_packets = [(p["packet"], p["quantity"], p["tile"]) for p in initial["packets"]]
    # No return/quote may recreate a packet, renew allowance or change production coordinates. Only original actual travel and cargo aging change while the full twenty-unit hold stays on the same hull.
    for key, state in decoded.items():
        phase = key.split("/")[1]
        assert state["quantity"] == state["extracted"] == 20 and state["reserve"] == 1740 and state["work"] == 0 and state["lease"] == 1048575
        assert [(p["packet"], p["quantity"], p["tile"]) for p in state["packets"]] == original_packets
        assert all(b["age"] >= a["age"] for a, b in zip(initial["packets"], state["packets"]))
        assert state["anchor"] == initial["anchor"] and state["phase"] == (4 if phase == "region" else 3)
        assert (state["distance_squared"] <= (20 * 65536)**2) == (phase == "region")
        assert state["position"] != state["anchor"]
        if phase == "outbound":
            assert sum((p - b)**2 for p, b in zip(state["position"], state["body_position"])) >= (2 * 65536)**2
    return dict(configuration=name, processes=8, paid_route_processes=6, controlled_boundary_processes=1, focused_assertions=focused["assertions"], focused_cases=19,
                native_and_boundary_assertions=sum(p["assertions"] for p in proofs if p is not focused), selectors=[identity(p) for p in selectors], reports=[p["_path"] for p in proofs],
                physical_owner_comparisons=90, metadata_comparisons=6, checkpoints=checkpoints, decoded_checkpoints=decoded,
                boundary_fixture=dict(report=boundary["_path"], supplied_capacity_stock=True, foreign_identity_only=True, saved_world=False))


def main():
    """Freeze both native builds after retained-cargo travel, independent boundaries and all saved next actions qualify."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=5, required=True); parser.add_argument("--debug", type=Path, nargs=5, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists(); references = {}
    release = configuration("RelWithDebInfo", [p.resolve() for p in args.release], references)
    debug = configuration("Debug", [p.resolve() for p in args.debug], references)
    for key, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][key])
    assert release["decoded_checkpoints"] == debug["decoded_checkpoints"]
    args.output.write_text(json.dumps(dict(_documentation=__doc__, save_version=438, configurations=[release, debug], sources=[identity(ROOT / p) for p in SOURCES],
                                          direct_references=list(references.values()), dated_producer_indexes=[identity(LEAF["ENTRY"])],
                                          cross_configuration_physical_comparisons=180, cross_configuration_metadata_comparisons=12,
                                          shared_native_mining_departure=True, native_paid_circle_entry=True, boundary_fixtures_saved=False,
                                          inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
