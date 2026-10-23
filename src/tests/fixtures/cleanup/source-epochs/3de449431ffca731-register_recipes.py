"""Register bounded declarations before original Space sealing, without running producers.

@file register_recipes.py Explicit catalogue edit for r2fc-file-cleanup3.md Group6.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups6/8-20.
"""

import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_recipes import validate_recipes


def declaration(name, producer, execution="pure_conversion", pending=(17,), target=None, tags=(), sources=()):
    """Create one explicitly disabled leaf using its actual existing original owner.

    @param name Stable bounded recipe ID.
    @param producer Existing source tool/owner; no invented executable entry point.
    @param execution Read-only metadata, pure conversion or isolated native fixture.
    @param pending Exact supplying task groups before this declaration can execute.
    @param target Existing original native target, pending isolation where declared.
    @param tags Actual original test selectors, empty only for a pending new interface.
    @param sources Existing source files containing each actual declared native tag.
    @return Mutable declaration; validation and publication happen only after graph closure.
    """
    native = execution == "native_fixture"
    return dict(id=name, producer=producer, feature="space" if name.startswith(("space", "orbital", "launch")) else "multilayer-art",
        scenario=name, target=target, test_tags=list(tags), tag_sources=list(sources),
        execution=execution, enabled=False, pending_groups=list(pending),
        publishes_source=False, stages=["inspect", "build", "prepare", "verify"],
        configuration_required=native, configuration_reuse="forbid" if native or execution == "read_only" else "source_bytes_only",
        comparison="current_native_observation" if native else "historical_metadata" if execution == "read_only" else "independent_source_bytes",
        tools=["native-build", "runtime-distribution"] if native else ["python", "pillow"] if execution == "pure_conversion" else ["python"],
        retained_inputs=[dict(role="tool-requirements", path="src/tests/fixtures/cleanup/tool-requirements.json")],
        requires=[], outputs=[dict(role="manifest", path="reports/current-{configuration}.json" if native else "reports/derived.json")],
        original_parameters={}, native_timeout_seconds=60 if native else None)


def require(recipe, dependency, role="manifest", alias=None):
    """Connect one explicit supplying role without adding unrelated producer families.

    @param recipe Mutable consuming leaf.
    @param dependency Exact supplying recipe ID.
    @param role Declared output role from that dependency.
    @param alias Unique consuming input role, defaulting to the dependency ID.
    @return None; mutate only this in-memory declaration.
    """
    recipe["requires"].append(dict(recipe=dependency, role=role, **{"as": alias or dependency}))


def main():
    """Publish validated pending recipes while preserving every original producer row.

    @return None; no recipe is enabled and no source/native producer is executed.
    """
    path = ROOT / "mlm/review-producers.json"
    catalogue = json.loads(path.read_text(encoding="utf-8"))
    originals = json.dumps(catalogue["producers"], sort_keys=True)
    recipes = []
    checkpoint = declaration("space-checkpoint", "src/tests/multilayer/acceptance/surface_constructor_retirement_ledger.py",
        "native_fixture", (8, 11, 12, 13, 15), "openttd_surface_capital_test")
    checkpoint.update(required_interface="Group11 pure zero-tick load tag and Group12 typed checkpoint adapter; original observer listed until that actual adapter exists",
        original_parameters=dict(configuration="explicit", checkpoint="explicit retained manifest", gameplay_ticks=0))
    checkpoint["retained_inputs"].append(dict(role="checkpoint-contract", path="src/tests/fixtures/checkpoints/space/checkpoint.json", provider_group=8))
    checkpoint["outputs"] += [dict(role="native-evidence", path="evidence/checkpoint-{configuration}.txt")]
    recipes.append(checkpoint)
    tube = declaration("tube-checkpoint", "src/tests/multilayer/qualify_tube_terminals.py", "read_only", (9, 15))
    tube.update(original_parameters=dict(scope="completed Group9 terminal metadata only; Group10 remains provisional"))
    tube["retained_inputs"].append(dict(role="tube-checkpoint-contract", path="src/tests/fixtures/checkpoints/underground/checkpoint.json", provider_group=9))
    recipes.append(tube)
    stages = ("base", "terrain", "portal", "vehicle", "purchase", "facility", "control", "hardness")
    # Source stages are independent selectable leaves. Packing names exactly the
    # source prerequisites it consumes; it cannot trigger an acceptance directory.
    for stage in stages:
        recipe = declaration("m1-source-" + stage, "media/multilayer/regenerate_art_sources.py")
        recipe["original_parameters"] = dict(stage=stage, entry_point="main", publication=False)
        recipe["outputs"].append(dict(role="source-data", path="data/" + stage, kind="directory"))
        if stage != "base":
            require(recipe, "m1-source-base")
        recipes.append(recipe)
    # Each original pure packer retains its own scope and three named candidate
    # outputs. Candidate bytes are reviewed/generated, never implicitly published.
    for name, module in (("m1", "m1"), ("space", "space"), ("orbital", "orbital"), ("launch", "launch_fleet"), ("ground", "ground")):
        scope = json.loads((ROOT / f"media/multilayer/{name}-pack-scope.json").read_text(encoding="utf-8"))
        recipe = declaration(name + "-derived-inputs", f"media/multilayer/pack_{module}_art.py")
        recipe["original_parameters"] = dict(symbol="derive", publication=False, scope=f"media/multilayer/{name}-pack-scope.json")
        recipe["retained_inputs"] += [dict(role="pack-scope", path=f"media/multilayer/{name}-pack-scope.json"),
            dict(role="source-approval-contract", path="src/tests/fixtures/graphics/source-contracts/checkpoint.json", provider_group=9)]
        recipe["outputs"] += [dict(role=role, path="data/" + Path(scope[role]).name) for role in ("png", "nfo", "header")]
        if name == "m1":
            for stage in stages:
                require(recipe, "m1-source-" + stage)
        recipes.append(recipe)
    graphics = json.loads((ROOT / "src/tests/fixtures/cleanup/graphics-dependencies.json").read_text(encoding="utf-8"))
    # The fixture inventory supplies the actual original generator/loader pairs;
    # all output-only adapters are pending rather than fictional working commands.
    for family in graphics["expected_payloads"]:
        name = "launch" if family["id"] == "launch-fleet" else family["id"]
        fixture = declaration(name + "-expected-payload", family["producer"], pending=(9, 17, 18))
        fixture["original_parameters"] = dict(symbol="main" if name in {"m1", "space", "orbital"} else "produce", output_only_adapter_required=True)
        fixture["outputs"].append(dict(role="expected-payload", path="data/" + Path(family["old_output"]).name))
        require(fixture, name + "-derived-inputs")
        if name == "launch":
            for prefix in ("m1", "space", "orbital"):
                require(fixture, prefix + "-expected-payload", "expected-payload", prefix + "-prefix")
        recipes.append(fixture)
        native = declaration(name + "-native-loader", family["native_reader"], "native_fixture", (13, 18),
            family["target"], (family["native_tag"],), (family["native_reader"],))
        require(native, fixture["id"], "expected-payload", "expected-payload")
        native["outputs"].append(dict(role="native-evidence", path="evidence/decoder-{configuration}.txt"))
        recipes.append(native)
    current = declaration("launch-current-producers", "src/tests/multilayer/launch_fleet_runtime_checks.py", "native_fixture", (13, 19),
        "openttd_space_art_test", ("[launch-site]", "[launch-assembly-contract]", "[space-logistics-contract]", "[launch-facility-art]", "[launch-rocket-art]"),
        ("src/tests/launch_site.cpp", "src/tests/launch_production.cpp", "src/tests/space_logistics.cpp", "src/tests/launch_facility_sprite.cpp", "src/tests/launch_fleet_sprite.cpp"))
    require(current, "launch-native-loader")
    current["original_parameters"] = dict(configuration="--configuration <exact>", cases=20, output_adapter_required=True)
    recipes.append(current)
    for item in graphics["supplemental_expected_inputs"]:
        recipe = declaration(item["id"], item["producer"].split(":")[0], pending=(17, 18))
        recipe["original_parameters"] = dict(symbol=item["producer"].split(":")[-1], output_adapter_required=True)
        if item["id"] != "ground-generated-header":
            recipe["outputs"].append(dict(role="expected-payload", path="data/" + Path(item["old_input"]).name))
        require(recipe, "ground-derived-inputs")
        recipes.append(recipe)
    catalogue["recipe_schema_version"] = 1
    catalogue["recipe_stages"] = dict(inspect=dict(read_only=True, reserves_run_id=False),
        build=dict(writes="declared build owner only"), prepare=dict(writes="selected generated run only"),
        verify=dict(native_only_when_selected=True, closed_paid_work=False))
    catalogue["recipes"] = recipes
    catalogue["_documentation"] += " Bounded recipe declarations: r2fc-file-cleanup3.md Group6; disabled pending leaves are not executable or proof of regeneration."
    validate_recipes(catalogue, ROOT)
    if json.dumps(catalogue["producers"], sort_keys=True) != originals:
        raise ValueError("original catalogue producer assignments changed")
    path.write_text(json.dumps(catalogue, indent=2) + "\n", encoding="utf-8")
    print("Registered disabled bounded recipes:",len(recipes),"original producers preserved:",len(catalogue["producers"]))


if __name__ == "__main__":
    main()
