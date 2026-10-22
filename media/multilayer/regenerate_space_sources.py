"""Regenerate selected Space source candidates through their original producers.

@file regenerate_space_sources.py Current sources retain historical approval as facts.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups17B/17BB/17BC.
The public media manifest and original generation records remain authoritative.
Selected reports/images are disposable; no publisher or native game is called.
"""

import copy
import importlib
import json
from pathlib import Path

import convert_space_art as art
from evidence import identity
from artifact_paths import RunContext, contained, identifier
from regenerate_art_sources import write_document


def read_scope(collection):
    """Use the existing selected packing owner's original tool/prefix checks.

    @param collection Explicit space, orbital or launch packing/source collection.
    @return Original immutable packing scope after its existing validation.
    """
    if collection not in ("space", "orbital", "launch"):
        raise ValueError("unknown selected Space source collection")
    module = importlib.import_module("pack_launch_fleet_art" if collection == "launch" else "pack_" + collection + "_art")
    if collection == "launch":
        scope, accepted = module.read_scope(current_sources=True)
        return dict(scope, protected_outputs=accepted["protected_outputs"],
                    source_reviews={asset: accepted["sources"][asset]["report"] for asset in scope["asset_ids"]})
    return module.read_scope()


def selected_path(reference, context, role):
    """Resolve a current source identity in the exact recipe invocation.

    @param reference Explicit current path/SHA256 record.
    @param context Selected recipe RunContext, shared across source leaves.
    @param role Required reports or images owner.
    @return Exact existing artifact after ownership and byte verification.
    """
    if context.root.resolve() != art.ROOT:
        raise ValueError("selected source belongs to another source checkout")
    path = contained(art.ROOT / reference["path"], art.ROOT / "mlm/reviews")
    parts = path.relative_to(art.ROOT / "mlm/reviews").parts
    # A source cannot borrow a previous report/image or an unrelated data role,
    # even when its pixels or basename happen to match the current selection.
    if len(parts) != 5 or parts[2] != context.run_id or parts[3] != role:
        raise ValueError("selected Space source has a foreign invocation or role")
    if identity(path) != reference:
        raise ValueError("selected Space source bytes changed")
    return path


def source_snapshot(asset, scope):
    """Protect original source ancestry, public state and canonical packing bytes.

    @param asset Exact existing public family selected for source conversion.
    @param scope Existing immutable packing scope, including protected prefix files.
    @return Exact before/after identities; historical review paths are not followed.
    """
    paths = {art.RECORD, art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"}
    paths.update(art.ROOT / scope[key] for key in ("png", "nfo", "header"))
    paths.update(art.ROOT / row["path"] for row in scope["protected_outputs"])
    document = art.read_record()
    # Preserve original candidates/receipts/input images for this family only.
    # Other families' historical templates do not become hidden prerequisites.
    for candidate in document["candidates"]:
        if candidate["asset_id"] != asset:
            continue
        # Launch originals record actual prompt inputs with explicit roles;
        # older Space originals use the equivalent path/hash reference list.
        inputs = candidate.get("references", candidate.get("reference_inputs", []))
        references = [{key: candidate[key] for key in ("path", "sha256")}, candidate["generation_receipt"], *inputs]
        # Every actual image/prompt input remains an original media dependency.
        for reference in references:
            paths.add(art.source_reference(reference, document))
    return {path: identity(path)["sha256"] for path in paths}


def produce_source(asset, collection, context):
    """Run the actual selected craft/natural-body source converter without publication.

    @param asset Exact existing public asset ID in the selected collection.
    @param collection Explicit space or orbital source/packing scope.
    @param context Exact shared invocation receiving source images/reports/data.
    @return Measured preservation/pixel facts and the selected generated source report.
    """
    scope = read_scope(collection)
    if asset not in scope["asset_ids"]:
        raise ValueError("source family is outside the selected packing scope")
    if context.root.resolve() != art.ROOT:
        raise ValueError("Space source context belongs to another checkout")
    # Source adapters join an existing invocation and cannot allocate another
    # run or redirect original producers outside their declared generated role.
    for value in (context.feature, context.scenario, context.run_id):
        identifier(value)
    public = json.loads((art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == asset)
    approval = scope["source_reviews"][asset]
    authoring = family["authoring"]
    # This is the existing original approval binding, retained as historical
    # metadata. A fresh report never changes that approval or its pixel hash.
    if authoring["atlas_manifest"] != approval["path"] or authoring["atlas_manifest_sha256"] != approval["sha256"]:
        raise ValueError("original source approval binding differs from its packing scope")
    before = source_snapshot(asset, scope)
    try:
        if collection == "launch":
            producer = importlib.import_module("convert_launch_center_source" if asset == "facility.launch-center" else "convert_cargo_rocket_source")
            producer.derive(context.images, context=context)
            raw = identity(context.reports / "source-manifest.json")
        elif asset in art.ASTEROID_ASSETS:
            import convert_space_asteroids as producer
            raw = producer.derive(asset, context.images, context=context)
        elif asset.startswith("module."):
            owners = {"module.nucleus": "module", "module.dock": "dock", "module.storage": "storage",
                      "module.power": "power", "module.fuel-store": "fuel_store",
                      "module.workshop": "assembly", "module.processor": "processor"}
            producer = importlib.import_module("convert_space_" + owners[asset] + "_states")
            raw = producer.review(context.images, context=context)
        else:
            import convert_space_family_art as producer
            raw = producer.derive(context.images, asset=asset, context=context)
        raw_path = selected_path(raw, context, "reports")
        document = json.loads(raw_path.read_text(encoding="utf-8"))
        candidates = document.get("atlases", []) + document.get("artifacts", [])
        if "atlas" in document:
            candidates.append(document["atlas"])
        native = [row for row in candidates if Path(row["path"]).name == Path(authoring["native_atlas"]).name]
        if len(native) != 1:
            raise ValueError("actual source producer omitted its canonical native atlas")
        native = {key: native[0][key] for key in ("path", "sha256")}
        selected_path(native, context, "images")
        if native["sha256"] != authoring["native_sha256"]:
            raise ValueError("fresh Space source differs from original approved native pixels")
        if collection == "launch":
            from PIL import Image
            # Actual saved source decoding checks independent original fixtures,
            # cell ownership, physical absence and alias policy; source flags or
            # historical success counters never certify current conversion.
            with Image.open(selected_path(native, context, "images")) as atlas:
                decoded = producer.decode(document, atlas=atlas, context=context)
            document["current_source_decode"] = dict(logical_pose_count=len(decoded),
                                                    producer=identity(Path(producer.__file__)), native_proof=False)
        document.update(kind="selected_space_source", asset_id=asset, collection=collection,
                        run_id=context.run_id, historical_approval=approval, native_source=native,
                        raw_source_report=raw, source_producer=identity(Path(producer.__file__)))
        destination = context.output("reports", "derived.json")
        write_document(destination, document)
        inventory = dict(_documentation=__doc__, schema_version=1, kind="selected_space_source_files",
                         asset_id=asset, run_id=context.run_id, native_source=native,
                         raw_source_report=raw, manifest=identity(destination), publishes_source=False)
        write_document(context.output("data", "source-data.json"), inventory)
    finally:
        # Neither an unsuccessful converter nor a successful source candidate
        # can advance original authoring flags or replace canonical payloads.
        for path, digest in before.items():
            if identity(path)["sha256"] != digest:
                raise ValueError("selected Space conversion changed original source: " + str(path))
    return dict(checks=["actual original source/pose/contact producer completed",
                        "approved original native pixels reproduced exactly",
                        "source/public/canonical bytes preserved"], native_source=native,
                manifest=identity(destination), source_files=len(before), publishes_source=False)


def bind_sources(public, scope, context, manifests):
    """Bind current source reports while keeping original approval metadata separate.

    @param public Detached canonical public manifest, including original pixel identities.
    @param scope Existing immutable selected source/packing scope.
    @param context Exact current recipe invocation supplying every source leaf.
    @param manifests Existing public asset IDs mapped to selected current report identities.
    @return Detached relocated public view, actual current source references and documents.
    """
    if set(manifests) != set(scope["asset_ids"]):
        raise ValueError("selected Space source closure is incomplete or ambiguous")
    public = copy.deepcopy(public)
    families = {row["asset_id"]: row for row in public["assets"]}
    documents = {}
    # Old hashes remain approval facts. Current reports must independently bind
    # real source output, its original pixels and the explicitly selected run.
    for asset in scope["asset_ids"]:
        original = scope["source_reviews"][asset]
        authoring = families[asset]["authoring"]
        # The original public/scope pair owns approval; current output cannot
        # revise either its historical report identity or approved pixel hash.
        if authoring["atlas_manifest"] != original["path"] or authoring["atlas_manifest_sha256"] != original["sha256"]:
            raise ValueError("original source approval binding changed")
        reference = manifests[asset]
        document = json.loads(selected_path(reference, context, "reports").read_text(encoding="utf-8"))
        # A current source must identify this family/run and its original
        # approval role; a relocated historical report cannot satisfy the gate.
        if document.get("kind") != "selected_space_source" or document.get("asset_id") != asset or document.get("run_id") != context.run_id or document.get("historical_approval") != original:
            raise ValueError("current source report has a foreign asset/run/approval role")
        native = document["native_source"]
        selected_path(native, context, "images")
        if native["sha256"] != authoring["native_sha256"]:
            raise ValueError("current source differs from original approved pixels")
        authoring.update(atlas_manifest=reference["path"], atlas_manifest_sha256=reference["sha256"], native_atlas=native["path"])
        documents[reference["path"]] = document
    return public, manifests, documents


def execute_stage(request, root):
    """Dispatch one registered bounded Space source producer or selected packer stage.

    @param request Exact shared executor request with declared scope/input/output roles.
    @param root Explicit source checkout; no prior workstation selection is discovered.
    @return Actual measured source or packing checks for the shared dispatcher.
    """
    if root.resolve() != art.ROOT:
        raise ValueError("Space source recipe belongs to another checkout")
    # Routine source recovery grants neither authored publication nor gameplay.
    if request["publishes_source"] is not False or request["paid_work_authorized"] is not False:
        raise ValueError("selected source recipe cannot publish or perform paid work")
    parameters = request["declaration"]["original_parameters"]
    collection = parameters["collection"]
    read_scope(collection)
    if request["stage"] == "prepare":
        return dict(checks=["original pinned source/packing inputs and protected prefix verified"])
    context = RunContext(root, request["feature"], request["scenario"], request["run_id"])
    if parameters.get("stage") == "packing":
        recipes = {row["id"]: row for row in json.loads((root / "mlm/review-producers.json").read_text(encoding="utf-8"))["recipes"]}
        manifests = {}
        # Dependency declarations carry existing public IDs; no private asset
        # catalogue or old filename search determines the packing source set.
        for row in request["declaration"]["requires"]:
            asset = recipes[row["recipe"]]["original_parameters"]["asset"]
            manifests[asset] = request["inputs"][row["as"]]
        return produce_packing(collection, context, manifests)
    return produce_source(parameters["asset"], collection, context)


def produce_packing(collection, context, manifests):
    """Derive selected packing bytes through the actual existing Space packer.

    @param collection Explicit existing Space/orbital packing scope.
    @param context Exact recipe invocation receiving disposable candidate data.
    @param manifests Complete current source report identities keyed by public asset.
    @return Measured canonical/source preservation and actual candidate identities.
    """
    if context.root.resolve() != art.ROOT:
        raise ValueError("selected packing belongs to another source checkout")
    scope = read_scope(collection)
    before = source_snapshot(scope["asset_ids"][0], scope)
    try:
        packer = importlib.import_module("pack_launch_fleet_art" if collection == "launch" else "pack_" + collection + "_art")
        report, files = packer.derive(source_manifests=manifests, context=context)
        context.data.mkdir(parents=True, exist_ok=True)
        # Existing shipping PNG/NFO/header bytes are independent expectations.
        # The newly derived candidate cannot become its own approval baseline.
        for role in ("png", "nfo", "header"):
            target = context.data / Path(scope[role]).name
            if files[target] != (art.ROOT / scope[role]).read_bytes():
                raise ValueError("selected Space packing differs from canonical bytes: " + role)
        # Only independently compared candidates enter the disposable data owner.
        for path, encoded in files.items():
            contained(path, context.data).write_bytes(encoded)
        destination = context.output("reports", "derived.json")
        write_document(destination, report)
    finally:
        # Actual candidate production changes only the selected generated owner;
        # original source approval, distribution and protected prefix stay exact.
        for path, digest in before.items():
            if identity(path)["sha256"] != digest:
                raise ValueError("selected packing changed original source: " + str(path))
    return dict(checks=["actual selected original packing producer completed",
                        "canonical PNG/NFO/header bytes matched independently",
                        "original approval and source/public state preserved"],
                manifest=identity(destination), artifacts=[identity(path) for path in files], publishes_source=False)
