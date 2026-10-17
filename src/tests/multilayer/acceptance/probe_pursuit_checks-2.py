"""Freeze current station-free paid pursuit, independent saved geometry and exact interrupted continuation.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group9
Controlled edge/loss components do not claim paid procurement; actual original Earth deployment supplies the native route.
"""
import argparse
import json
from pathlib import Path
import re
import runpy

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "probe_pursuit_checks-1.py"))
AUDIT = LEAF["AUDIT"]
TABLES = runpy.run_path(str(HERE / "heavy_joined_checks-2.py"))["table_rows"]
SOURCES = tuple(dict.fromkeys((*AUDIT["SOURCES"], "src/space_vehicle.cpp", "src/space_vehicle.h", "src/space_orders.cpp", "src/space_orders.h",
                             "src/space_approach.cpp", "src/space.cpp", "src/asteroid.cpp", "src/saveload/asteroid_sl.cpp", "src/saveload/vehicle_sl.cpp",
                             "src/tests/excavation_fixture.h", "src/tests/mock_environment.h", "src/tests/probe_pursuit.cpp", "src/tests/probe_pursuit_native.cpp",
                             "src/tests/probe_shell.cpp", "src/tests/asteroid_motion.cpp", "src/tests/space_approach.cpp",
                             "src/tests/multilayer/acceptance/heavy_joined_checks-2.py",
                             *(f"src/tests/multilayer/acceptance/probe_pursuit_checks-{n}.py" for n in (1, 2)))))


def snapshot(saved):
    """Independently join original paid hull, native mission and current finite moving-body geometry.

    @param saved Exact ordinary438 checkpoint identity.
    @return Original paid receipt, exclusive lease, saved deadline and actual body/craft points.
    """
    assert identity(ROOT / saved["path"]) == saved; version, raw = read_stream(ROOT / saved["path"]); assert version == 438

    def integer(position, width, signed=False):
        """Read one original bounded network-order scalar.

        @param position Same-file field position.
        @param width Native field width.
        @param signed Whether a fixed-point coordinate retains its signed meaning.
        @return Detached scalar, without allocating any gameplay owner.
        """
        return int.from_bytes(raw[position:position + width], "big", signed=signed)

    _, start, _, end = riff_location(raw, b"SVST"); assert start + 1 + raw[start] * 68 == end
    receipt, = [p for p in range(start + 1, end, 68) if integer(p + 41, 2) == 15]
    vehicle, generation = integer(receipt, 4), integer(receipt + 4, 8)
    assert integer(receipt + 43, 4) == 100000 and integer(receipt + 39, 2) == 2 and raw[receipt + 48] == 1
    row, = [r for r in TABLES(raw, b"VEHS") if r["index"] == vehicle]
    fields = {re.sub(r"\[\d+\]", "", f["path"]): f for f in row["fields"]}

    def field(name):
        """Read this actual original native hull's own scalar.

        @param name Exact normalized Space field path.
        @return Actual original native value.
        """
        f = fields["space." + name]; return integer(f["position"], f["end"] - f["position"])

    assert fields["space.common.cargo.packets"]["count"] == 0
    target = field("mission.target"); _, start, _, end = riff_location(raw, b"ASTR"); assert target < integer(start + 27, 4)
    body = start + 51 + 40 * target; assert integer(body, 4) == target and body + 40 <= end
    point = [integer(body + 6, 4, True), integer(body + 10, 4, True)]
    return dict(vehicle=vehicle, generation=generation, model=15, paid_capital=100000, content=2,
                home=[integer(receipt + 13, 2), integer(receipt + 15, 8)], target=target,
                phase=field("mission.phase"), deadline=field("mission.approach_ticks_left"), work=field("mission.work_ticks"),
                position=[field("position.x"), field("position.y")],
                offset=[integer(fields["space.mission.attachment_offset." + axis]["position"], 4, True) for axis in ("x", "y")],
                body_position=point, reserve=integer(body + 28, 4), lease=integer(body + 36, 4), elapsed=integer(start + 18, 8))


def configuration(name, selectors, references):
    """Qualify exact current native membership and actual original/resumed interception convergence.

    @param name Current native executable configuration.
    @param selectors Four exact native/own/resume/focused selectors.
    @param references Checked direct identities; historical index graphs remain dated boundaries.
    @return Qualified current process counts, exact checkpoints and independently decoded geometry.
    """
    rows = {}; proofs = []; focused = None
    for scope, path in zip(LEAF["SCOPES"], selectors):
        selected = LEAF["read"](path); AUDIT["inspect"](selected, references)
        assert selected["complete"] and selected["scope"] == scope and selected["configuration"] == name
        assert len(selected["reports"]) == len(LEAF["SCOPES"][scope])
        for phase, relative in zip(LEAF["SCOPES"][scope], selected["reports"]):
            path = ROOT / relative; proof = LEAF["read"](path); AUDIT["inspect"](proof, references)
            assert proof["complete"] and proof["configuration"] == name and proof["phase"] == phase and proof["scope"] == scope and not proof["added_stock"]
            assert proof["dated_index"] == identity(LEAF["ENTRY"]) and len(proof["native_reports"]) == 1
            process = LEAF["read"](ROOT / proof["native_reports"][0]["path"]); AUDIT["inspect"](process, references)
            assert process["exit_status"] == 0 and process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
            assert process["argv"][1:] == [LEAF["TAGS"] if scope == "focused" else "[.probe-pursuit-native]"]
            terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
            assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
            proof["_path"] = identity(path); proofs.append(proof)
            if scope == "focused":
                focused = proof; assert proof["cases"] == 10; continue
            assert proof["source"] in process["inputs"] and proof["after"] in process["artifacts"]
            parent = rows.get("native/assigned") if scope == "own" or (scope == "native" and phase == "intercepted") else rows.get("own/assigned") if scope == "resume" else None
            if parent:
                assert proof["parent"] == parent["_path"] and proof["source"] == parent["after"]
            else:
                assert proof["parent"] is None and proof["source"] == LEAF["predecessor"](name)[1]
            rows[scope + "/" + phase] = proof
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 5 and focused
    AUDIT["compare"](rows["native/assigned"]["after"], rows["own/assigned"]["after"])
    AUDIT["compare"](rows["native/intercepted"]["after"], rows["resume/intercepted"]["after"])
    checkpoints = {k: p["after"] for k, p in rows.items()}; decoded = {k: snapshot(v) for k, v in checkpoints.items()}
    before, after = decoded["native/assigned"], decoded["native/intercepted"]
    assert before["phase"] == 1 and before["deadline"] > 0 and after["phase"] == 2 and after["deadline"] == 0
    assert before["body_position"] != after["body_position"] and before["elapsed"] < after["elapsed"] <= before["elapsed"] + before["deadline"]
    assert [x // 65536 for x in before["body_position"]] != [x // 65536 for x in after["body_position"]]
    assert after["position"] == [x + offset for x, offset in zip(after["body_position"], after["offset"])] and after["lease"] == after["vehicle"]
    assert before["reserve"] == after["reserve"] and before["work"] == after["work"] == 0
    assert all(before[k] == after[k] for k in ("vehicle", "generation", "home", "paid_capital", "content", "target"))
    return dict(configuration=name, processes=5, focused_assertions=focused["assertions"], focused_cases=10,
                native_assertions=sum(p["assertions"] for p in proofs if p is not focused), selectors=[identity(p) for p in selectors], reports=[p["_path"] for p in proofs],
                checkpoints=checkpoints, decoded_checkpoints=decoded, physical_owner_comparisons=60, metadata_comparisons=4)


def main():
    """Freeze both native builds only after real paid route geometry and all interrupted owners agree."""
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
                                          cross_configuration_physical_comparisons=120, cross_configuration_metadata_comparisons=8,
                                          native_station_free_paid_pursuit=True, inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
