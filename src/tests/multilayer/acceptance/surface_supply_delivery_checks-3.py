"""Bind exact original both-build purchased-construction evidence without native reruns.

@file surface_supply_delivery_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CD.3.
Prepared capital/floor remain declared. This closes the procurement boundary,
never the later powerhouse, useful electricity or unassisted factory chain.
"""

import argparse
import json
from pathlib import Path
import runpy

SHARED = runpy.run_path(str(Path(__file__).with_name("surface_supply_delivery_checks-2.py")))
ROOT, identity, chunks, FAMILIES = (SHARED[name] for name in ("ROOT", "identity", "chunks", "FAMILIES"))


def read(path):
    """@param path Explicit original JSON path. @return Parsed immutable receipt."""
    return json.loads(path.read_text(encoding="utf-8"))


def bind(specification):
    """Bind current drive products and original accepted loader/guard processes.

    @param specification Explicit membership and exact retained-product aliases.
    @return Complete procurement and build-retention receipts.
    """
    spec = read(specification)
    retained = {(row["original"]["path"], row["original"]["sha256"]): row["retained"]
                for row in spec["retained_products"]}
    processes = set()
    used_retention = set()

    def verify(reference):
        """Verify exact current bytes or an explicitly retained original test binary.

        @param reference Original path/digest at the actual execution boundary.
        @return Current original path or its explicit unchanged retained product.
        """
        expected = {key: reference[key] for key in ("path", "sha256")}
        path = ROOT / reference["path"]
        if path.is_file() and identity(path) == expected:
            return path
        # Only explicitly archived products may replace a rebuilt path. A
        # changed source, missing receipt or arbitrary same-name file refuses.
        key = (reference["path"], reference["sha256"])
        assert key in retained, reference
        replacement = retained[key]
        target = ROOT / replacement["path"]
        assert identity(target) == replacement and replacement["sha256"] == reference["sha256"]
        used_retention.add(key)
        return target

    def process(reference):
        """Validate an actual native child and its original output memberships.

        @param reference Unchanged runner-generated native process report.
        """
        report = read(verify(reference))
        assert report["exit_status"] == 0
        verify(report["executable"])
        for item in report["inputs"] + report["artifacts"]:
            verify(item)
        processes.add(reference["path"])

    def proof(path):
        """Verify immediate original evidence without replaying historical graphs.

        @param path Exact current original proof path.
        @return Parsed proof with verified sources, products and native children.
        """
        value = read(path)
        assert value.get("complete", True)
        for key in ("executable", "application", "terminal", "harness", "source", "malformed"):
            if value.get(key):
                verify(value[key])
        for reference in value.get("sources", []):
            verify(reference)
        for reference in value.get("native_processes", []):
            process(reference)
        return value

    def game_process(value):
        """Verify one ordinary application operation and its exact physical input/output.

        @param value Accepted complete game operation, never a fixture callback.
        """
        for key in ("source", "output", "application", "terminal"):
            verify(value[key])
        for reference in value["native_processes"]:
            process(reference)

    configurations = []
    originals = []
    demo = read(ROOT / "logs/space-demo-verification.json")
    builds = []
    # Both independently linked builds must supply actual native purchases,
    # ordinary continuations, malformed refusals and the active command guards.
    for entry in spec["configurations"]:
        configuration = entry["configuration"]
        driven = proof(ROOT / entry["driven"])
        assert driven["configuration"] == configuration and driven["cases"] == 1 and driven["assertions"] == 33236
        outputs = {Path(item["path"]).name: item for item in driven["outputs"]}
        assert len(outputs) == 6
        for reference in outputs.values():
            verify(reference)
        producer = proof(ROOT / entry["producer"])
        assert producer["cases"] == 20 and producer["assertions"] == 3059
        ordinary = proof(ROOT / entry["ordinary"])
        assert len(ordinary["evidence"]) == 3
        for state in ordinary["evidence"]:
            assert state["owners"] == len(FAMILIES) and len(state["processes"]) == 4
            # The final change only adds fixture teardown. Its newly generated
            # six inputs must equal the already exercised original game inputs
            # byte for byte; matching totals or an older fixture cannot suffice.
            source = state["processes"][0]["source"]
            assert source["sha256"] == outputs[f"delivery-{state['state']}-440.sav"]["sha256"]
            for operation in state["processes"]:
                game_process(operation)
        refusals = proof(ROOT / entry["refusals"])
        assert [row["fault"] for row in refusals["evidence"]] == ["price", "pending-packet", "missing-station-field"]
        for fault in refusals["evidence"]:
            verify(fault["malformed"])
            for reference in fault["reports"]:
                proof(verify(reference))
        guards = proof(ROOT / entry["guards"])
        component = proof(verify(guards["evidence"]))
        assert component["cases"] == 32 and component["assertions"] == 2223
        verify(demo["builds"][configuration]["application"])
        build_log = ROOT / entry["build_log"]
        terminal = build_log.read_text(encoding="utf-16", errors="replace")
        assert "error C" not in terminal and "openttd_surface_supply_test.exe" in terminal
        builds.append(dict(configuration=configuration, build_log=identity(build_log),
                           current_focused_application=driven["executable"],
                           unchanged_ordinary_application=demo["builds"][configuration]["application"]))
        configurations.append(dict(configuration=configuration, driven=identity(ROOT / entry["driven"]),
                                   producer=identity(ROOT / entry["producer"]), ordinary=identity(ROOT / entry["ordinary"]),
                                   refusals=identity(ROOT / entry["refusals"]), guards=identity(ROOT / entry["guards"]),
                                   driven_assertions=33236, guard_cases=32, guard_assertions=2223,
                                   producer_cases=20, producer_assertions=3059, ordinary_processes=12,
                                   full_copied_refusal_pairs=3))
        originals.append((outputs, ordinary))
    assert [row["configuration"] for row in configurations] == ["RelWithDebInfo", "Debug"]
    comparisons = 0
    all_native_comparisons = 0
    # Compare each actual independently produced checkpoint across builds,
    # including all native chunks; no generated expected ledger participates.
    for name in originals[0][0]:
        first, second = [chunks(verify(item[0][name]))[1] for item in originals]
        assert first == second, name
        all_native_comparisons += len(first)
        comparisons += len(FAMILIES)
    for first_state, second_state in zip(originals[0][1]["evidence"], originals[1][1]["evidence"]):
        assert first_state["state"] == second_state["state"]
        for first_operation, second_operation in zip(first_state["processes"], second_state["processes"]):
            first, second = [chunks(verify(item["output"]))[1] for item in (first_operation, second_operation)]
            for family in FAMILIES:
                assert first[family] == second[family], (first_state["state"], family)
            comparisons += len(FAMILIES)
    bounded = proof(ROOT / spec["bounded_ordinary"])
    for operation in bounded["evidence"]["processes"]:
        game_process(operation)
    assert len(processes) == 46
    result = dict(_documentation=__doc__, complete=True, specification=identity(specification),
                  configurations=configurations, native_processes=len(processes),
                  tracked_owner_families=len(FAMILIES), cross_configuration_comparisons=comparisons,
                  all_native_chunk_comparisons=all_native_comparisons,
                  bounded_ordinary=identity(ROOT / spec["bounded_ordinary"]),
                  accepted_units=dict(STEL=12, RFMT=8, ELEC=4, BATY=2),
                  retained_units=dict(STEL=1, RFMT=1, ELEC=1, BATY=1), charged_once=6470,
                  freight_income=0, prepared_floor=True, prepared_capital=True,
                  final_surface_factory_acceptance=False, unassisted=False,
                  sources=[identity(Path(__file__))])
    retention = dict(_documentation=__doc__, complete=True, target="openttd_surface_supply_test",
                     configurations=builds, retained_products=spec["retained_products"],
                     used_retained_products=[dict(path=path, sha256=digest) for path, digest in sorted(used_retention)],
                     game_application_relinked=False, compiler_settings_changed=False, no_full_regression=True)
    return result, retention


def main():
    """Publish fresh procurement/build receipts from an explicit original membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--specification", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--build-output", type=Path, required=True)
    args = parser.parse_args()
    outputs = [(ROOT / value).resolve() for value in (args.output, args.build_output)]
    assert all(path.is_relative_to(ROOT / "logs") and not path.exists() for path in outputs)
    result, retention = bind((ROOT / args.specification).resolve())
    for path, value in zip(outputs, (result, retention)):
        path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
        print(path.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
