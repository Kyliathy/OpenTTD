"""Independently qualify finite native extraction, actual packet provenance and interrupted next actions.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group10
Original paid probe/body/packet owners remain authoritative; component vetoes claim no native heap exhaustion or procurement.
"""
import argparse
import json
from pathlib import Path
import re
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream
from launch_center_checks import riff_location

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "probe_extraction_checks-1.py"))
PURSUIT = runpy.run_path(str(HERE / "probe_pursuit_checks-2.py"))
AUDIT = LEAF["AUDIT"]; TABLES = PURSUIT["TABLES"]
EXPECTED = {"stopped": (0, 0), "partial": (0, 37), "first": (2, 0), "second-partial": (2, 37), "loaded": (20, 0)}
SOURCES = tuple(dict.fromkeys((*PURSUIT["SOURCES"], "src/space_mining_inventory.cpp", "src/space_mining_inventory.h", "src/space_mining_tuning.cpp",
                             "src/asteroid_motion.cpp", "src/cargopacket.cpp", "src/cargopacket.h", "src/saveload/cargopacket_sl.cpp",
                             "src/tests/probe_extraction.cpp", "src/tests/probe_extraction_native.cpp", "src/tests/heavy_mining_runtime.cpp",
                             "src/tests/manufacturing_fixture.h",
                             *(f"src/tests/multilayer/acceptance/probe_extraction_checks-{n}.py" for n in (1, 2)))))


def snapshot(saved):
    """Join the original paid receipt, finite body and actual native hold through same-file cargo meanings.

    @param saved Exact retained ordinary438 checkpoint identity.
    @return Independently decoded ready/partial work, reserve, physical quantities and packet provenance.
    """
    assert identity(ROOT / saved["path"]) == saved; version, raw = read_stream(ROOT / saved["path"]); assert version == 438

    def integer(position, width, signed=False):
        """Read an original bounded scalar.

        @param position Exact native field start.
        @param width Actual original field width.
        @param signed Whether a fixed-point coordinate retains its signed meaning.
        @return Detached network-order value.
        """
        return int.from_bytes(raw[position:position + width], "big", signed=signed)

    _, start, _, end = riff_location(raw, b"SVST"); assert start + 1 + 68 * raw[start] == end
    receipt, = [p for p in range(start + 1, end, 68) if integer(p + 41, 2) == 15]
    assert integer(receipt, 4) == 16 and integer(receipt + 4, 8) == 2 and integer(receipt + 43, 4) == 100000 and integer(receipt + 39, 2) == 2
    row, = [r for r in TABLES(raw, b"VEHS") if r["index"] == 16]; fields = {re.sub(r"\[\d+\]", "", f["path"]): f for f in row["fields"]}

    def field(name):
        """Read one original miner field.

        @param name Exact normalized original Space path.
        @return Same-file native scalar.
        """
        f = fields["space." + name]; return integer(f["position"], f["end"] - f["position"])

    _, start, _, end = riff_location(raw, b"CDIM")
    ore, = [raw[p] for p in range(start + 1, end, 6) if raw[p + 1:p + 5] == b"NORE" and raw[p + 5] == 16]
    assert field("common.cargo_type") == ore and field("common.cargo_cap") == 20
    packet_rows = {}
    for r in TABLES(raw, b"CAPA"):
        packet_rows[r["index"]] = {f["path"]: integer(f["position"], f["end"] - f["position"]) for f in r["fields"]}
    references = fields["space.common.cargo.packets"]; refs = [integer(p, 4) - 1 for p in range(references["position"], references["end"], 4)]
    assert len(refs) == len(set(refs)); held = []
    # Actual positive packet quantities, rather than a cached hold total, finance this one miner. Every packet retains asteroid0, original Space coordinates/age, invalid pickup and zero feeder credit.
    for reference in refs:
        packet = packet_rows[reference]; assert packet["count"] > 0 and packet["source_type"] == 3 and packet["source_id"] == 0
        assert packet["source"] == 65535 and packet["feeder_share"] == 0
        held.append(dict(packet=reference, quantity=packet["count"], tile=packet["source_xy"], age=packet["periods_in_transit"], source_type=3, source_id=0))
    _, start, _, end = riff_location(raw, b"ASTR"); body = start + 51
    assert integer(start + 27, 4) > 0 and integer(body, 4) == field("mission.target") == 0 and body + 40 <= end
    return dict(vehicle=16, generation=2, phase=field("mission.phase"), work=field("mission.work_ticks"), extracted=field("extracted_units"),
                quantity=sum(p["quantity"] for p in held), packets=held, reserve=integer(body + 28, 4), lease=integer(body + 36, 4),
                body_position=[integer(body + 6, 4, True), integer(body + 10, 4, True)],
                position=[field("position.x"), field("position.y")],
                offset=[integer(fields["space.mission.attachment_offset." + axis]["position"], 4, True) for axis in ("x", "y")], elapsed=integer(start + 18, 8))


def configuration(name, selectors, references):
    """Qualify exact current native stages and unchanged original/resumed extraction owners.

    @param name Current native executable build.
    @param selectors Four exact native/own/resume/focused selectors.
    @param references Checked current direct identities, without reopening historical executable graphs.
    @return Qualified processes, independently decoded material flow and exact save membership.
    """
    rows = {}; proofs = []; focused = None
    for scope, path in zip(LEAF["SCOPES"], selectors):
        selected = LEAF["read"](path); AUDIT["inspect"](selected, references)
        assert selected["complete"] and selected["configuration"] == name and selected["scope"] == scope and len(selected["reports"]) == len(LEAF["SCOPES"][scope])
        for phase, relative in zip(LEAF["SCOPES"][scope], selected["reports"]):
            path = ROOT / relative; proof = LEAF["read"](path); AUDIT["inspect"](proof, references)
            assert proof["complete"] and proof["configuration"] == name and proof["scope"] == scope and proof["phase"] == phase and not proof["added_stock"]
            assert proof["dated_index"] == identity(LEAF["ENTRY"]) and len(proof["native_reports"]) == 1
            process = LEAF["read"](ROOT / proof["native_reports"][0]["path"]); AUDIT["inspect"](process, references)
            assert process["exit_status"] == 0 and process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
            assert process["argv"][1:] == [LEAF["TAGS"] if scope == "focused" else "[.probe-extraction-native]"]
            terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
            assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
            proof["_path"] = identity(path); proofs.append(proof)
            if scope == "focused":
                focused = proof; assert proof["cases"] == 11; continue
            assert proof["source"] in process["inputs"] and proof["after"] in process["artifacts"]
            parent = rows.get("native/" + LEAF["PARENTS"].get(phase, "")) if scope == "native" else rows["native/" + phase] if scope == "own" else rows["own/" + LEAF["PARENTS"][phase]]
            if parent:
                assert proof["parent"] == parent["_path"] and proof["source"] == parent["after"]
            else:
                assert phase == "stopped" and proof["parent"] is None and proof["source"] == LEAF["predecessor"](name)[1]
            rows[scope + "/" + phase] = proof
            if scope == "own":
                AUDIT["compare"](proof["source"], proof["after"])
            elif scope == "resume":
                AUDIT["compare"](rows["native/" + phase]["after"], proof["after"])
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 11 and focused
    checkpoints = {k: p["after"] for k, p in rows.items()}; decoded = {k: snapshot(saved) for k, saved in checkpoints.items()}
    original_stock = chunks(ROOT / LEAF["predecessor"](name)[1]["path"])[1][b"SPST"]
    # Every output is the exact difference from the same1760-unit native resource, never a fixture grant. Partial work commits no fraction of a packet and stopped attachment keeps the current body-relative transform.
    for key, state in decoded.items():
        phase = key.split("/")[1]; quantity, work = EXPECTED[phase]
        assert state["quantity"] == state["extracted"] == quantity and state["reserve"] == 1760 - quantity and state["work"] == work
        assert state["position"] == [x + offset for x, offset in zip(state["body_position"], state["offset"])]
        assert state["phase"] == (3 if phase == "loaded" else 2) and state["lease"] == (1048575 if phase == "loaded" else 16)
        assert chunks(ROOT / checkpoints[key]["path"])[1][b"SPST"] == original_stock
    first = decoded["native/first"]["packets"]; later = decoded["native/second-partial"]["packets"]
    assert [(p["packet"], p["quantity"], p["tile"]) for p in first] == [(p["packet"], p["quantity"], p["tile"]) for p in later]
    assert all(b["age"] >= a["age"] for a, b in zip(first, later))
    return dict(configuration=name, processes=11, focused_assertions=focused["assertions"], focused_cases=11,
                native_assertions=sum(p["assertions"] for p in proofs if p is not focused), selectors=[identity(p) for p in selectors], reports=[p["_path"] for p in proofs],
                physical_owner_comparisons=150, metadata_comparisons=10, checkpoints=checkpoints, decoded_checkpoints=decoded)


def main():
    """Freeze both native builds only after actual finite quantities, provenance and interrupted owners agree."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=4, required=True); parser.add_argument("--debug", type=Path, nargs=4, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists(); references = {}
    release = configuration("RelWithDebInfo", [p.resolve() for p in args.release], references)
    debug = configuration("Debug", [p.resolve() for p in args.debug], references)
    for key, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][key])
    assert release["decoded_checkpoints"] == debug["decoded_checkpoints"]
    args.output.write_text(json.dumps(dict(_documentation=__doc__, save_version=438, configurations=[release, debug], sources=[identity(ROOT / p) for p in SOURCES],
                                          direct_references=list(references.values()), dated_producer_indexes=[identity(LEAF["ENTRY"])],
                                          cross_configuration_physical_comparisons=300, cross_configuration_metadata_comparisons=20,
                                          native_paid_finite_extraction=True, allocation_refusal_kind="additional component admission veto; actual pool capacity remains mandatory",
                                          inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
