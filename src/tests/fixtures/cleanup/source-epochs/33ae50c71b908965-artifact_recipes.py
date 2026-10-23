"""Validate bounded rebuild declarations in the existing producer catalogue.

@file artifact_recipes.py Inspection never reserves a run, builds or imports producers.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups6/14/17DE.
"""

import json
from pathlib import Path

from artifact_inventory import checked_path
from artifact_paths import contained, identifier, owned_root, repository_root
from artifact_space_graph import graph_cycles

STAGES = ("inspect", "build", "prepare", "verify")
EXECUTIONS = {"read_only", "pure_conversion", "native_fixture", "native_application"}


def load_catalogue(root=None):
    """Read the single producer/recipe authority without loading any producer code.

    @param root Optional explicit source checkout.
    @return Parsed catalogue with its original producer assignments unchanged.
    """
    root = repository_root(root)
    return json.loads((root / "mlm/review-producers.json").read_text(encoding="utf-8"))


def validate_selection(catalogue, name, root=None):
    """Validate only one child stage's exact declaration/dependency closure.

    @param catalogue Existing sole recipe authority, already fully checked by the supervisor.
    @param name Exact requested registered recipe ID, never an implicit/default selector.
    @param root Optional checkout for all selected source/input/role/containment checks.
    @return Validated selected recipe mapping; unrelated file trees are not inspected.
    @throws ValueError On ambiguous IDs, missing dependencies or any unsafe selected declaration.
    """
    index = {}
    # Identity ambiguity affects every selection even when the duplicated row
    # is unrelated. Reject it before discovering this exact bounded closure.
    for recipe in catalogue["recipes"]:
        key = identifier(recipe["id"])
        if key in index:
            raise ValueError("duplicate recipe ID: " + key)
        index[key] = recipe
    selected, pending = {}, [name]
    # A source leaf may share prerequisites. Discover each declaration once;
    # the original validator still checks cycles, roles, execution and every
    # selected real file. This never executes or silently enables a supplier.
    while pending:
        key = pending.pop()
        if key in selected:
            continue
        if key not in index:
            raise ValueError("unknown recipe dependency: " + key)
        recipe = index[key]
        selected[key] = recipe
        pending.extend(row["recipe"] for row in recipe["requires"])
    return validate_recipes(dict(catalogue, recipes=list(selected.values())), root)


def validate_recipes(catalogue, root=None):
    """Validate exact recipe roles, prerequisites, modes and disposable destinations.

    @param catalogue Existing producer catalogue including its recipe extension.
    @param root Optional checkout used only for source/input existence checks.
    @return Recipe-ID mapping after all dependency and ownership checks pass.
    @throws ValueError For unknown/cyclic dependencies, ambiguous roles or unsafe outputs.
    """
    root = repository_root(root)
    if catalogue.get("recipe_schema_version") != 1:
        raise ValueError("unsupported recipe declaration schema")
    result = {}
    # Each leaf has one explicit identity and one original owner. A duplicate ID
    # would make prerequisites depend on catalogue ordering rather than intention.
    for recipe in catalogue["recipes"]:
        name = identifier(recipe["id"])
        if name in result:
            raise ValueError(f"duplicate recipe ID: {name}")
        result[name] = recipe
        if recipe["execution"] not in EXECUTIONS:
            raise ValueError(f"unknown recipe execution: {name}")
        if recipe["publishes_source"]:
            raise ValueError(f"routine recipe cannot publish source: {name}")
        if not checked_path(recipe["producer"], root).is_file():
            raise ValueError(f"original recipe producer is absent: {recipe['producer']}")
        # Stage adapters are actual implementation sources, not another asset
        # authority. Missing code cannot become an executable declaration.
        for relative in recipe.get("implementation_sources", []):
            if not checked_path(relative, root).is_file():
                raise ValueError(f"recipe implementation source is absent: {relative}")
        identifier(recipe["feature"])
        identifier(recipe["scenario"])
        if recipe["stages"] != list(STAGES):
            raise ValueError(f"unknown stage order: {name}")
        native = recipe["execution"] in {"native_fixture", "native_application"}
        # Native proof belongs to exactly one real configuration. Pure conversion
        # can be independent only when the explicit comparison mode says so.
        if native and (not recipe["configuration_required"] or recipe["configuration_reuse"] != "forbid"):
            raise ValueError(f"native cross-configuration reuse is forbidden: {name}")
        if recipe["configuration_reuse"] not in {"forbid", "source_bytes_only"}:
            raise ValueError(f"unknown configuration reuse policy: {name}")
        if recipe["configuration_reuse"] == "source_bytes_only" and recipe["comparison"] != "independent_source_bytes":
            raise ValueError(f"cross-configuration source comparison is not declared: {name}")
        # Declaration groups precede implementation/sealing by design. A disabled
        # leaf names its supplying groups explicitly and can never be executed.
        if not recipe["enabled"] and not recipe.get("pending_groups"):
            raise ValueError(f"disabled recipe has no supplying task group: {name}")
        if native:
            target = recipe.get("target")
            if not target:
                raise ValueError(f"native recipe lacks an isolated target: {name}")
            cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
            if f"add_executable({target}" not in cmake:
                raise ValueError(f"native target is not declared: {target}")
            # A selected fixture must finish independently of agent polling and
            # leave stopped writers for cleanup; builds have a separate time budget.
            if not 0 < recipe["native_timeout_seconds"] <= 60:
                raise ValueError(f"native fixture exceeds its bounded process cap: {name}")
            # Empty filters can look successful without running the intended
            # business boundary, so enabled native leaves need real selectors.
            if recipe["execution"] == "native_fixture" and recipe["enabled"] and not recipe["test_tags"]:
                raise ValueError(f"enabled native recipe lacks actual test tags: {name}")
            # A future native leaf must identify its supplying interface rather
            # than pretending an empty test selection could ever prove success.
            if recipe["execution"] == "native_fixture" and not recipe["test_tags"] and not recipe.get("required_interface"):
                raise ValueError(f"native declaration lacks actual tags or a supplying interface: {name}")
            # Ordinary application screenshots have no Catch test tag. Their
            # real existing game target and startup arguments identify the
            # observer; inventing a hidden test would misstate the producer.
            if recipe["execution"] == "native_application":
                arguments = recipe["original_parameters"].get("native_arguments", [])
                if target != "openttd" or recipe["test_tags"] or arguments != ["-g", "-c", "-v"]:
                    raise ValueError(f"ordinary native capture lacks its actual game/argument contract: {name}")
            # Tags come from actual named source owners, never invented aggregate
            # suites. Planned load-only interfaces intentionally declare no tag yet.
            sources = "\n".join(checked_path(path, root).read_text(encoding="utf-8") for path in recipe.get("tag_sources", []))
            for tag in recipe["test_tags"]:
                if tag not in sources:
                    raise ValueError(f"native tag has no original source: {name}: {tag}")
        roles, destinations = set(), set()
        # Output names are part of the producer contract. Two roles cannot race
        # to overwrite the same file even when their semantic labels differ.
        for output in recipe["outputs"]:
            role = identifier(output["role"])
            if role in roles:
                raise ValueError(f"duplicate output role: {name}: {role}")
            roles.add(role)
            sample = output["path"].format(configuration="Debug")
            owner = owned_root(root, "mlm/reviews")
            run = contained(owner / recipe["feature"] / recipe["scenario"] / "schema-inspection", owner)
            destination = contained(checked_path(run / sample, root), run)
            spelling = str(destination).casefold()
            if spelling in destinations:
                raise ValueError(f"duplicate output destination: {name}: {sample}")
            destinations.add(spelling)
            # Required evidence must stay in the review run, with only the named
            # role folders. Logs/runtime/build/source paths cannot masquerade as data.
            if destination.relative_to(run).parts[0] not in {"images", "data", "reports", "evidence", "scratch"}:
                raise ValueError(f"recipe output is outside generated review roles: {name}: {sample}")
        input_roles = set()
        # Retained roles are independently supplied immutable/source inputs;
        # each must have one unambiguous selected path or a named supplying group.
        for declared in recipe["retained_inputs"]:
            role = identifier(declared["role"])
            if role in input_roles:
                raise ValueError(f"duplicate retained input role: {name}: {role}")
            input_roles.add(role)
            path = checked_path(declared["path"], root)
            # Missing retained material needs an exact supplying task, not an old
            # workstation fallback. Enabled leaves must have their real inputs.
            if not path.is_file() and (recipe["enabled"] or not declared.get("provider_group")):
                raise ValueError(f"missing required input {declared['path']} for {name}; supplying group {declared.get('provider_group')}")
    adjacency = {}
    # Connect roles only after all leaves exist, so forward declarations remain
    # deterministic and an unknown dependency cannot silently become optional.
    for name, recipe in result.items():
        adjacency[(name, "recipe", "dependency")] = []
        aliases = {row["role"] for row in recipe["retained_inputs"]}
        for requirement in recipe["requires"]:
            dependency = requirement["recipe"]
            if dependency not in result:
                raise ValueError(f"unknown recipe dependency: {name} -> {dependency}")
            if requirement["role"] not in {row["role"] for row in result[dependency]["outputs"]}:
                raise ValueError(f"unknown supplying role: {name} -> {dependency}/{requirement['role']}")
            alias = identifier(requirement["as"])
            if alias in aliases:
                raise ValueError(f"duplicate prerequisite input role: {name}: {alias}")
            aliases.add(alias)
            adjacency[(name, "recipe", "dependency")].append((dependency, "recipe", "dependency"))
    if graph_cycles(adjacency):
        raise ValueError("cyclic recipe prerequisites")
    return result


def recipe_order(name, recipes):
    """Return only one selected leaf's ordered prerequisite closure.

    @param name Explicit registered recipe ID.
    @param recipes Already validated recipe mapping.
    @return Dependency-first ordered IDs; no unrelated producer family is selected.
    @throws ValueError For an unknown selected recipe.
    """
    if name not in recipes:
        raise ValueError(f"unknown recipe: {name}")
    ordered, visited, pending = [], set(), [(name, False)]
    # A dependency may be shared by several leaves. Prepare it once in this
    # invocation while preserving the declared order and bounded selected scope.
    while pending:
        current, closing = pending.pop()
        if current in visited:
            continue
        if closing:
            visited.add(current)
            ordered.append(current)
            continue
        pending.append((current, True))
        pending.extend((row["recipe"], False) for row in reversed(recipes[current]["requires"]))
    return ordered


def inspect_recipe(name, configuration=None, root=None):
    """Inspect declared prerequisites and pending implementation without reserving a run.

    @param name Explicit recipe ID; no latest-run discovery is used.
    @param configuration Optional exact native configuration selected by the caller.
    @param root Optional checkout whose catalogue and retained inputs are inspected.
    @return Read-only recipe plan with exact missing-input/provider and pending-group details.
    """
    root = repository_root(root)
    recipes = validate_recipes(load_catalogue(root), root)
    order = recipe_order(name, recipes)
    if configuration not in {None, "Debug", "RelWithDebInfo"}:
        raise ValueError("unsupported recipe configuration")
    selected = recipes[name]
    # Native proof cannot silently choose a default build and later be reused
    # as evidence for the other independently linked configuration.
    if selected["configuration_required"] and configuration is None:
        raise ValueError(f"recipe requires an explicit configuration: {name}")
    missing = [dict(recipe=item, **row) for item in order for row in recipes[item]["retained_inputs"]
        if not checked_path(row["path"], root).is_file()]
    pending = {item: recipes[item]["pending_groups"] for item in order if not recipes[item]["enabled"]}
    return dict(recipe=name, configuration=configuration, order=order, missing_inputs=missing,
        pending_groups=pending, executable=not missing and not pending,
        publishes_source=False, reserves_run_id=False)
