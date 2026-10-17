"""Qualify exact native population work, output-sensitive searches and finite probe transactions.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group21
Exact original operation counts govern acceptance; instrumented component timings remain diagnostic.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

LEAF = runpy.run_path(str(Path(__file__).with_name("asteroid_work_checks-1.py")))
READ = LEAF["read"]
SOURCES = (
    "src/CMakeLists.txt", "src/tests/CMakeLists.txt", "src/space_work_cost.h", "src/space.cpp", "src/space.h",
    "src/asteroid.cpp", "src/asteroid.h", "src/asteroid_motion.cpp", "src/asteroid_generation.cpp", "src/asteroid_generation.h",
    "src/space_approach.cpp", "src/space_approach.h", "src/space_vehicle.cpp", "src/space_vehicle.h", "src/space_vehicle_cmd.h",
    "src/space_mining_tuning.cpp", "src/space_mining_tuning.h", "src/space_mining_inventory.cpp", "src/space_mining_inventory.h",
    "src/space_orders.cpp", "src/space_orders.h", "src/multilayer_asteroid_catalogue.cpp", "src/multilayer_asteroid_catalogue.h",
    "src/map.cpp", "src/map_layer.cpp", "src/tests/asteroid_work_costs-1.cpp", "src/tests/asteroid_work_costs-2.cpp",
    "src/tests/asteroid_work_fixture.h", "src/tests/probe_mining_fixture.h", "src/tests/probe_extraction.cpp",
    "src/tests/space_preview.cpp", "src/tests/asteroid_generation.cpp", "src/tests/excavation_fixture.h",
    "src/tests/test_artifact_paths.cpp", "src/tests/test_artifact_paths.h", "src/tests/multilayer/support/artifact_paths.py",
    "src/tests/multilayer/support/runner.py", "src/tests/multilayer/acceptance/asteroid_work_checks-1.py",
    "src/tests/multilayer/acceptance/asteroid_work_checks-2.py",
)
GEOMETRY = {"small": (64, 64, 10), "normal": (256, 256, 160), "largest-normal": (4096, 4096, 10240),
            "largest-dense": (4096, 4096, 14336), "clustered": (4096, 4096, 14336),
            "narrow-tall": (64, 1024, 160), "narrow-wide": (1024, 64, 160)}


def inspect(value, references):
    """Validate every direct evidence identity without reopening dated producer graphs.

    @param value Current selector, process witness or native result structure.
    @param references Accumulator of checked repository path/hash records.
    """
    if isinstance(value, dict):
        # Only concrete path/hash identities authorize evidence membership; nested semantic facts are walked without treating arbitrary text as a path.
        if set(value) == {"path", "sha256"}:
            assert identity(ROOT / value["path"]) == value, value
            references[value["path"]] = value
        else:
            # Every direct process/report/input reference must still exist unchanged at final qualification.
            for child in value.values():
                inspect(child, references)
    elif isinstance(value, list):
        # Ordered native process/artifact membership is validated completely, without selecting a convenient subset.
        for child in value:
            inspect(child, references)


def semantic(value):
    """Remove only diagnostic timing and implementation-dependent native sort comparisons.

    @param value Native measurements and original finite resource/phase facts.
    @return Exact cross-build semantic structure; every other operation count remains required.
    """
    if isinstance(value, dict):
        return {key: semantic(child) for key, child in value.items() if key not in ("nanoseconds", "query_order_tests", "planner_order_tests")}
    if isinstance(value, list):
        return [semantic(child) for child in value]
    return value


def validate(data):
    """Independently enforce published population and native operation/resource bounds.

    @param data One actual native component result, after successful Catch assertions.
    """
    width, height, bodies = GEOMETRY[data["case"]]
    assert (data["width"], data["height"], data["bodies"]) == (width, height, bodies)
    assert data["seed"] == 12345 and data["clustered"] == (data["case"] == "clustered")
    if data["domain"] == "field":
        cells = ((width - 5) // 4 + 1) * ((height // 2 - 5) // 4 + 1)
        generation, motion, query = (data[name]["work"] for name in ("generation", "motion", "query"))
        assert data["grid_cells"] == generation["generation_cells"] == cells
        assert generation["generation_starter_candidates"] == 3 * cells - 3
        assert generation["generation_shuffle_swaps"] == 2 * (bodies - 3)
        assert generation["generation_bodies"] == generation["validation_bodies"] == bodies
        assert generation["validation_velocity_tests"] == 24 * bodies
        assert motion["body_steps"] == motion["index_updates"] == 32 * bodies
        assert motion["body_lookups"] == motion["query_calls"] == 0
        assert motion["index_unlinks"] <= 32 * bodies
        assert query["query_calls"] == 1 and 1 <= query["query_buckets"] <= 4
        assert query["query_candidates"] == data["query_results"] <= bodies
        assert query["query_order_tests"] <= data["sort_work_bound"]
        assert not data["surface_site_search_measured"]
        if data["clustered"]:
            assert motion["index_unlinks"] == motion["index_links"] == query["query_candidates"] == bodies
        assert data["resources"]["initial"] == data["resources"]["remaining"]
        return
    assert data["domain"] == "probes" and data["isolated_legacy_hulls"] and not data["whole_game_frame_rate_measured"]
    # Fresh native approach/mining worlds retain separate clocks and packet owners; only their actual original extraction may debit Nickel.
    for phase in ("approach", "mining"):
        record = data[phase]
        mining = phase == "mining"
        count = 1 if mining and data["clustered"] else 6 if bodies == 10 else 64
        steps, units = (74, 2 * count) if mining else (32, 0)
        assert record["mining"] == mining and record["probes"] == count and record["steps"] == steps
        field, approach, craft = (record[name]["work"] for name in ("field", "approach", "craft"))
        assert field["body_steps"] == field["index_updates"] == bodies * steps
        assert field["attachment_visits"] == craft["craft_ticks"] == count * steps
        assert approach["approach_craft_visits"] == 2 * count * steps
        assert approach["approach_occupant_checks"] == count * count * steps
        assert approach["planner_requests"] == count * steps
        assert approach["planner_slot_tests"] <= 3 * count * count * steps
        assert approach["planner_owner_tests"] <= count * count * steps
        assert approach["planner_order_tests"] <= 64 * count * count.bit_length() * steps
        assert craft["body_lookups"] <= 32 * count * steps and craft["query_candidates"] == 0
        assert craft["extraction_attempts"] == (count if mining else 0)
        assert craft["extraction_units"] == record["held"] == units
        assert record["money_debit"] == (500 if mining else 43) * count
        before, after = record["before_resources"], record["after_resources"]
        assert before["initial"] == after["initial"]
        assert [a - b for a, b in zip(before["remaining"], after["remaining"], strict=True)] == [units, 0, 0]


def configuration(name, selectors, references):
    """Qualify all seven field/probe cases and one directly relevant producer burst.

    @param name Current native build configuration.
    @param selectors Explicit fifteen selector paths in field, probes, focused order.
    @param references Checked direct identity accumulator.
    @return Complete measured cases, native processes and assertion totals.
    """
    expected = [(domain, case) for domain in ("field", "probes") for case in LEAF["CASES"]] + [("focused", "small")]
    measured, proofs, processes = {}, [], []
    totals = {"measured_assertions": 0, "focused_assertions": 0, "focused_cases": 0}
    # Exact scope membership prevents an inexpensive small field from substituting for the maximum or actual native hull measurements.
    for (domain, case), selector in zip(expected, selectors, strict=True):
        inspect(identity(selector.resolve()), references)
        selected = READ(selector)
        inspect(selected, references)
        proof = READ(ROOT / selected["report"]["path"])
        inspect(proof, references)
        assert selected["complete"] and proof["complete"]
        assert selected["configuration"] == proof["configuration"] == name
        assert selected["domain"] == proof["domain"] == domain and selected["case"] == proof["case"] == case
        native_id, = proof["native_reports"]
        native = READ(ROOT / native_id["path"])
        inspect(native, references)
        assert native["exit_status"] == 0 and native["configuration"] == name
        assert native["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
        tag = LEAF["TAGS"] if domain == "focused" else f"[.asteroid-{'field' if domain == 'field' else 'probe'}-work]"
        assert native["argv"][1:] == [tag]
        terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
        assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
        processes.append(native_id)
        proofs.append(selected["report"])
        if domain == "focused":
            assert proof["measurement"] is None
            totals["focused_assertions"], totals["focused_cases"] = proof["assertions"], proof["cases"]
        else:
            assert (ROOT / proof["measurement"]["path"]).parent == (ROOT / native_id["path"]).parent
            data = READ(ROOT / proof["measurement"]["path"])
            assert data["case"] == case and data["domain"] == domain and proof["cases"] == 1
            assert f"ASTEROID_WORK case={case} domain={domain} bodies={data['bodies']} native=true paid=false" in terminal
            validate(data)
            measured[f"{domain}/{case}"] = data
            totals["measured_assertions"] += proof["assertions"]
    assert len({record["path"] for record in processes}) == 15
    # The detached canonical field and the actual native approaching fleet consume the same original motion steps without extraction or a second resource owner.
    for case in LEAF["CASES"]:
        assert measured[f"field/{case}"]["final_digest"] == measured[f"probes/{case}"]["approach"]["final_digest"]
    return dict(configuration=name, reports=proofs, processes=processes, measurements=measured, **totals)


def main():
    """Freeze both successful current builds and exact semantic work comparisons."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=15, required=True)
    parser.add_argument("--debug", type=Path, nargs=15, required=True)
    parser.add_argument("--build-logs", type=Path, nargs=2, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    references = {}
    rows = [configuration(name, paths, references) for name, paths in (("RelWithDebInfo", args.release), ("Debug", args.debug))]
    assert semantic(rows[0]["measurements"]) == semantic(rows[1]["measurements"])
    sources = [identity(ROOT / source) for source in SOURCES]
    builds = [identity(path.resolve()) for path in args.build_logs]
    games = [identity(ROOT / "build" / name / "openttd.exe") for name in ("RelWithDebInfo", "Debug")]
    args.output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, sources=sources, references=list(references.values()),
                                         configurations=rows, semantic_case_comparisons=14, build_logs=builds, normal_games=games,
                                         timing_policy="Instrumented native nanoseconds are diagnostic; exact operations and output-sensitive query bounds govern acceptance",
                                         limitations="Isolated legacy hulls; no paid procurement, whole-game FPS or maximum-map Surface startup timing"), indent=2) + "\n", encoding="utf-8")
    print(f"passed both builds: 30 native processes, 14 exact semantic cases, {len(sources)} sources, {len(references)} direct references", flush=True)


if __name__ == "__main__":
    main()
