"""Qualify three complete paid resource missions and separately declared inclusive drop geometry.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group18
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group19
Original paid receipts, resource equations and exclusive packet owners finance every hold/descent/Surface result.
"""
import argparse
import json
from pathlib import Path
import re
import runpy

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "probe_class_mission_checks-1.py"))
FLOW = runpy.run_path(str(HERE / "probe_conservation_checks-3.py"))
BASE = runpy.run_path(str(HERE / "probe_conservation_checks-2.py"))
AUDIT = LEAF["AUDIT"]
SOURCES = tuple(dict.fromkeys((*BASE["SOURCES"], "src/multilayer_asteroid_catalogue.cpp", "src/tests/probe_class_mission_native.cpp",
                             *(f"src/tests/multilayer/acceptance/probe_class_mission_checks-{n}.py" for n in (1, 2)))))


def snapshot(saved, target, require_packet_conservation=True):
    """Join the original paid hull, class-specific cargo, finite resource and actual home/descent owners.

    @param saved Exact original438 checkpoint identity.
    @param target Original starter body/class0,1,2.
    @param require_packet_conservation True requires packets to cover every finite debit; false leaves final native Industry intake/history reconciliation to the caller, retaining all original packet ownership checks.
    @return Detached mission/paid custody/geometry and original resource quantities; Surface consumers must add their independently read actual receiver before claiming a complete equation.
    """
    flow = FLOW["inventory"](saved, require_packet_conservation=require_packet_conservation)
    _, raw = read_stream(ROOT / saved["path"])

    def integer(position, width, signed=False):
        """Read an original bounded network-order field.

        @param position Exact same-file field start.
        @param width Published original scalar width.
        @param signed Whether an original Q16 point retains its signed meaning.
        @return Detached original integer.
        """
        return int.from_bytes(raw[position:position + width], "big", signed=signed)

    _, start, _, end = riff_location(raw, b"SVST")
    receipt, = [p for p in range(start + 1, end, 68) if integer(p, 4) == 16]
    assert integer(receipt + 4, 8) == 2 and integer(receipt + 41, 2) == 15 and integer(receipt + 43, 4) == 100000 and integer(receipt + 39, 2) == 2
    home = [integer(receipt + 13, 2), integer(receipt + 15, 8)]
    row, = [r for r in FLOW["TABLES"](raw, b"VEHS") if r["index"] == 16]
    fields = {re.sub(r"\[\d+\]", "", f["path"]): f for f in row["fields"]}

    def field(name):
        """Read one original paid native hull field.

        @param name Exact normalized Space path.
        @return Original scalar, without live-world lookup.
        """
        f = fields["space." + name]
        return integer(f["position"], f["end"] - f["position"])

    _, start, _, end = riff_location(raw, b"CDIM")
    cargo, = [raw[p] for p in range(start + 1, end, 6) if raw[p + 1:p + 5].decode("ascii") == FLOW["LABELS"][target] and raw[p + 5] == 16]
    assert field("mission.target") == target and field("common.cargo_type") == cargo and field("common.cargo_cap") == 20
    _, start, _, _ = riff_location(raw, b"ASTR")
    width, elapsed = integer(start + 2, 4), integer(start + 18, 8)
    _, start, _, end = riff_location(raw, b"LCTR")
    anchor_row, = [p for p in range(start + 10, end, 15) if [integer(p, 2), integer(p + 2, 8)] == home]
    tile = integer(anchor_row + 10, 4)
    anchor = [tile % width * 65536, tile // width * 65536]
    position = [field("position.x"), field("position.y")]
    _, start, _, end = riff_location(raw, b"SPSU")
    p = start + 1
    manifest = None
    # Walk every original variable-width manifest before selecting the paid miner; another craft's supplies/pods cannot finance this class's recovery.
    for _ in range(raw[start]):
        craft = integer(p, 4)
        count = raw[p + 60]
        record = dict(generation=integer(p + 4, 8), service_ticks=integer(p + 38, 2), quota=raw[p + 40], pending=raw[p + 42], services=integer(p + 52, 8),
                      pods=[[integer(q, 2), integer(q + 2, 8)] for q in range(p + 61, p + 61 + 10 * count, 10)])
        p += 61 + 10 * count
        # Original supply references retain their own native packets. This scenario purchases no service or replacement allowance.
        for _ in range(3):
            p += 1 + 6 * raw[p]
        if craft == 16:
            assert manifest is None
            manifest = record
    assert p == end and manifest and manifest["generation"] == 2
    _, start, _, end = riff_location(raw, b"SPDJ")
    p = start + 10
    descents = []
    # Actual jobs preserve immutable sender/class dimensions and original release geometry; class quantities cannot be borrowed from a Nickel-only decoder.
    for _ in range(integer(start + 8, 2)):
        assert integer(p + 36, 4) == 16 and integer(p + 40, 8) == 2 and raw[p + 48] == cargo
        assert integer(p + 49, 2) == 20 and integer(p + 56, 2) == 15 and raw[p + 58] == 16 and integer(p + 59, 2) == 320
        descents.append(dict(pod=[integer(p, 2), integer(p + 2, 8)], job=integer(p + 10, 8), center=[integer(p + 18, 2), integer(p + 20, 8)],
                             release=[integer(p + 28, 4, True), integer(p + 32, 4, True)], remaining=integer(p + 51, 2)))
        p += 62 + 6 * raw[p + 61]
    assert p == end
    return dict(flow=flow, phase=field("mission.phase"), work=field("mission.work_ticks"), extracted=field("extracted_units"),
                manifest=manifest, descents=descents, anchor=anchor, position=position, home=home, elapsed=elapsed,
                distance_squared=sum((value - origin)**2 for value, origin in zip(position, anchor)))


def configuration(name, selectors, references):
    """Qualify exact earned class paths, controlled-boundary forks and unchanged recovered resaves.

    @param name Current native executable configuration.
    @param selectors Twelve exact class0/1/2 mine/return/boundary/own selectors in order.
    @param references Checked direct evidence without historical producer graph requalification.
    @return Qualified current process counts and independently decoded saved physical owners.
    """
    expected = [(target, phase) for target in range(3) for phase in LEAF["PHASES"]]
    assert len(selectors) == len(expected)
    rows = {}
    proofs = []
    checkpoints = {}
    decoded = {}
    # A boundary fork must use its own class's earned hold. Explicit ordering prevents one successful Nickel mission from substituting Water Ice or Rare Earth.
    for (target, phase), selector in zip(expected, selectors):
        selected = LEAF["read"](selector)
        AUDIT["inspect"](selected, references)
        assert selected["complete"] and selected["configuration"] == name and selected["target"] == target and selected["phase"] == phase
        path = ROOT / selected["report"]["path"]
        proof = LEAF["read"](path)
        AUDIT["inspect"](proof, references)
        assert proof["complete"] and proof["configuration"] == name and proof["target"] == target and proof["phase"] == phase
        assert not proof["added_stock"] and proof["boundary_geometry"] == (phase == "boundary") and len(proof["native_reports"]) == 1
        assert proof["dated_index"] == LEAF["predecessor"](name)[0]
        process = LEAF["read"](ROOT / proof["native_reports"][0]["path"])
        AUDIT["inspect"](process, references)
        assert process["exit_status"] == 0 and process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
        assert process["argv"][1:] == ["[.probe-class-mission-native]"] and proof["source"] in process["inputs"]
        terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
        assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
        if phase == "mine":
            assert proof["parent"] is None and proof["source"] == LEAF["predecessor"](name)[1]
        else:
            parent = rows[(target, "return" if phase == "own" else "mine")]
            assert proof["parent"] == parent["_path"] and proof["source"] == parent["checkpoints"]["recovered" if phase == "own" else "loaded"]
        assert tuple(proof["checkpoints"]) == LEAF["PHASES"][phase]
        # Each original saved stage must retain exactly twenty units of only its selected resource, under one native hold, descent or Surface stock owner.
        for stage, saved in proof["checkpoints"].items():
            assert saved in process["artifacts"]
            state = snapshot(saved, target)
            assert state["work"] == 0 and state["extracted"] == 20 and state["manifest"]["services"] == 1
            resource = state["flow"]["resources"][FLOW["LABELS"][target]]
            holding = "space_hold" if stage in ("loaded", "region") else "descent" if stage == "pending" else "surface_stock"
            assert resource[holding] == 20 and resource["initial"] - resource["remaining"] == 20
            assert all(body["initial"] - body["remaining"] == (20 if body_id == target else 0) for body_id, body in state["flow"]["bodies"].items())
            assert state["manifest"]["quota"] == (1 if stage == "recovered" else 2)
            assert state["manifest"]["pending"] == int(stage == "pending") and len(state["manifest"]["pods"]) == (2 if stage in ("loaded", "region") else 1)
            if stage == "region":
                assert 0 < state["distance_squared"] <= (20 * 65536)**2
                if phase == "boundary":
                    assert state["position"] == [state["anchor"][0] + 20 * 65536, state["anchor"][1]]
                else:
                    assert state["phase"] == 4
            if stage == "pending":
                assert len(state["descents"]) == 1 and state["descents"][0]["remaining"] == 148
                assert state["descents"][0]["release"] == decoded[f"{target}/{phase}/region"]["position"]
                assert state["descents"][0]["center"] == state["home"]
            elif stage == "recovered":
                assert not state["descents"]
            key = f"{target}/{phase}/{stage}"
            checkpoints[key] = saved
            decoded[key] = state
        if phase == "own":
            AUDIT["compare"](proof["source"], proof["checkpoints"]["recovered"])
        proof["_path"] = identity(path)
        rows[(target, phase)] = proof
        proofs.append(proof)
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 12
    return dict(configuration=name, processes=12, native_assertions=sum(p["assertions"] for p in proofs), selectors=[identity(p) for p in selectors],
                reports=[p["_path"] for p in proofs], checkpoints=checkpoints, decoded_checkpoints=decoded,
                physical_owner_comparisons=90, metadata_comparisons=6)


def main():
    """Freeze all three real full missions only after exact native cargo owners and declared geometry agree across builds."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=12, required=True)
    parser.add_argument("--debug", type=Path, nargs=12, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    references = {}
    release = configuration("RelWithDebInfo", [p.resolve() for p in args.release], references)
    debug = configuration("Debug", [p.resolve() for p in args.debug], references)
    # Original earned routes, separate boundary experiments and ordinary resaves must preserve identical resources, packets, paid shells and native decisions in both builds.
    for key, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][key])
    assert release["decoded_checkpoints"] == debug["decoded_checkpoints"]
    args.output.write_text(json.dumps(dict(_documentation=__doc__, save_version=438, configurations=[release, debug], sources=[identity(ROOT / p) for p in SOURCES],
                                         direct_references=list(references.values()), dated_producer_indexes=[identity(LEAF["ENTRY"])],
                                         cross_configuration_physical_comparisons=720, cross_configuration_metadata_comparisons=48,
                                         actual_earned_class_missions=3, controlled_boundary_scenarios=3, added_stock=False,
                                         inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
