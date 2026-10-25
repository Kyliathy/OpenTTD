"""Rebuild bounded source/packing candidates and join native GRF bytes without publishing.

@file regenerate_art_sources.py Actual original converters supply every candidate.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17.
Selected source reports and images share the recipe invocation; old cache inputs
are never used by selected packing. Existing authoring commands remain separate.
"""

import argparse
from contextlib import contextmanager
from functools import partial
import hashlib
import importlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import uuid

import PIL
import convert_m1_art as base
from evidence import identity
from artifact_paths import RunContext, contained, create_run_context, identifier

STAGES = ("base", "terrain", "portal", "vehicle", "purchase", "facility", "control", "hardness", "collector", "packing")
SCOPE = base.ROOT / "media/multilayer/m1-pack-scope.json"


def write_document(path, document):
    """Write one explicitly selected generated JSON document in UTF8.

    @param path Concrete generated report/data destination chosen by the context.
    @param document Actual source conversion or generated-file inventory.
    @return None; never infers a destination from a previous run.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")


def check_tools():
    """Check the existing pack scope's pinned encoder/serializer requirements.

    @return Original M1 pack scope after exact Python/Pillow version validation.
    """
    scope = json.loads(SCOPE.read_text(encoding="utf-8"))
    # Reproducible candidate bytes require the same original serializer/encoder;
    # a newer workstation tool cannot silently advance the authored baseline.
    if scope["toolchain"]["python"] != sys.version.split()[0] or scope["toolchain"]["pillow"] != PIL.__version__:
        raise ValueError("M1 conversion toolchain differs from pinned versions")
    return scope


def source_directories(output, context=None):
    """Select source images/reports while preserving the original fresh-run rule.

    @param output Exact image directory requested by the original source producer.
    @param context Optional explicit recipe invocation; None is historical authoring mode.
    @return Image and report directories; current reports never live beside frames.
    """
    output = Path(output).resolve()
    if context is None:
        contained(output, base.ROOT / "logs/proofs/mlm/native-art")
        if output.exists():
            raise ValueError("historical source conversion requires a fresh proof directory")
        return output, output
    if context.root.resolve() != base.ROOT:
        raise ValueError("source output belongs to another checkout")
    # A recipe can supply only its own immutable context components; generated
    # source relocation cannot redirect output into another feature or run.
    for value in (context.feature, context.scenario, context.run_id):
        identifier(value)
    expected = (context.feature, context.scenario, context.run_id, "images")
    parts = contained(output, base.ROOT / "mlm/reviews").relative_to(base.ROOT / "mlm/reviews").parts
    if output != context.images or tuple(parts) != expected:
        raise ValueError("source output differs from the explicit recipe image owner")
    # Run reservation creates the empty images directory. Existing content
    # still refuses a retry that could overwrite an earlier real source result.
    if output.exists() and any(output.iterdir()):
        raise ValueError("selected source conversion requires fresh empty images")
    reports = contained(context.reports, context.directory)
    reports.mkdir(parents=True, exist_ok=True)
    return output, reports


def source_snapshot(stage, records=()):
    """Hash immutable source contracts, originals and canonical packing files.

    @param stage Exact source family, selecting the additional collector originals.
    @param records Additional original media records whose real source ancestry is protected.
    @return Path-to-SHA256 map used to detect accidental source publication.
    """
    scope = json.loads(SCOPE.read_text(encoding="utf-8"))
    paths = {base.PUBLIC, SCOPE, *(base.ROOT / scope[key] for key in ("png", "nfo", "header"))}
    paths.update(path for path in base.ASSETS.rglob("*") if path.is_file())
    records = list(records)
    if stage == "collector":
        records.append(base.ROOT / "media/multilayer/source/ground/geothermal-generation-record.json")
    # Related source producers protect their actual original media ancestry;
    # generated review identities stay historical and are never followed here.
    for record in records:
        paths.add(record)
        pending = [json.loads(record.read_text(encoding="utf-8"))]
        # Collector provenance includes original model and glass-component paths.
        # Protect those exact media inputs without following old review receipts.
        while pending:
            value = pending.pop()
            if isinstance(value, dict):
                pending.extend(value.values())
            elif isinstance(value, list):
                pending.extend(value)
            # External workstation provenance and generated review paths are
            # historical text, while actual original media files stay protected.
            elif isinstance(value, str) and value.startswith("media/"):
                candidate = contained(base.ROOT / value, base.ROOT / "media")
                if candidate.is_file():
                    paths.add(candidate)
    return {path: base.digest(path) for path in paths}


@contextmanager
def source_outputs(context, module, collector=False, inputs=None, related=None):
    """Select existing converter destination variables for one serial recipe leaf.

    @param context Exact current invocation owning all generated source products.
    @param module Actual original pure converter module, never a publisher adapter.
    @param collector Whether its existing tube helper owns the atlas destination.
    @param inputs Exact predecessor aliases; an empty map refuses every old cache read.
    @param related Optional original converter dependencies sharing this leaf's output owners.
    @yield None while original converter/check/guide functions see selected owners.
    """
    changes = [(base, "OUTPUT", context.images), (base, "REVIEW", context.images),
               (base, "repository_path", partial(base.repository_path, selected_inputs=inputs or {}, context=context))]
    if related is not None:
        # Nested Ground composition uses independently imported destination
        # globals. Route those same owners together without changing originals.
        for owner in dict.fromkeys((module, *related)):
            for name in ("OUTPUT", "REVIEW", "MANIFEST", "REPORT", "ATLAS"):
                if hasattr(owner, name):
                    original = getattr(owner, name)
                    destination = context.reports / original.name if name in ("MANIFEST", "REPORT") else context.images
                    if name == "ATLAS":
                        destination = context.images / original.name
                    changes.append((owner, name, destination))
    elif collector:
        changes.append((module.tube, "REVIEW", context.images))
    else:
        changes.append((module, "REPORT", context.reports / "derived.json"))
        if hasattr(module, "REVIEW"):
            changes.append((module, "REVIEW", context.images / module.REVIEW.name))
    previous = [(owner, name, getattr(owner, name)) for owner, name, _ in changes]
    try:
        # Only destinations owned by the original converters change. Sources,
        # palettes, public state and crop/anchor policy remain their originals.
        for owner, name, destination in changes:
            setattr(owner, name, destination)
        yield
    finally:
        # Serial execution must not leak selected destinations into another
        # source family or the existing independent authoring entry points.
        for owner, name, value in reversed(previous):
            setattr(owner, name, value)


def source_guides(context, report):
    """Use the existing M1 contact-sheet producer on actual generated source poses.

    @param context Current source leaf's explicit generated images owner.
    @param report Actual original converter metadata with exact atlas/cell geometry.
    @return None; review arrangement never changes native source pixels.
    """
    atlases = []
    # Supplemental atlases can contain multiple survey families. Their board
    # uses the actual recorded poses, with a label only for source inspection.
    for row in report["atlases"]:
        atlas = dict(row)
        atlas.setdefault("asset_id", atlas["sprites"][0].get("asset_id", "source.review"))
        atlases.append(atlas)
    base.make_review(atlases, context.images)


def produce_source(stage, context, dependencies=None):
    """Run one original source converter and its actual focused source checks.

    @param stage Exact base/detail/collector source producer identity.
    @param context Explicit recipe leaf sharing the selected invocation.
    @param dependencies Explicit selected predecessor source report identities.
    @return Actual source report and exact generated atlas identities.
    """
    inputs = {}
    # Purchase/vehicle/control originals refer to earlier base conversions.
    # Reconstruct those bytes in this run and keep every original alias/hash.
    for reference in (dependencies or {}).values():
        path = contained(base.ROOT / reference["path"], base.ROOT / "mlm/reviews")
        parts = path.relative_to(base.ROOT / "mlm/reviews").parts
        # A predecessor is an explicit current source report, never an earlier
        # workstation run or a manually substituted immutable/public document.
        if len(parts) != 5 or parts[2] != context.run_id or parts[3] != "reports":
            raise ValueError("source predecessor has a foreign invocation or role")
        if identity(path) != reference:
            raise ValueError("selected predecessor source manifest changed")
        document = json.loads(path.read_text(encoding="utf-8"))
        # Preserve each original predecessor spelling while binding the exact
        # regenerated atlas identity that replaces its former cache location.
        for atlas in document["atlases"]:
            inputs["logs/proofs/mlm/native-art/derived/" + Path(atlas["path"]).name] = {key: atlas[key] for key in ("path", "sha256")}
    if stage == "base":
        report = base.build(cache_only=True, context=context)
        base.check(context.reports / "atlas-manifest.json")
    else:
        module = importlib.import_module("convert_geothermal_art" if stage == "collector" else "convert_" + stage + "_art")
        with source_outputs(context, module, collector=stage == "collector", inputs=inputs):
            if stage == "collector":
                report, encoded = module.build_collector()
                module.validate_collector(report, encoded)
                report.update(sha256=hashlib.sha256(encoded).hexdigest(), contact_pages=[])
                files = {base.ROOT / report["path"]: encoded}
                # Preserve every genuine collector view/state through its
                # original contact-page producer, without binding public flags.
                for direction, pixels in module.collector_contact_pages(report, encoded).items():
                    path = context.output("images", "collector-" + direction.lower() + "-state-contact.png")
                    files[path] = pixels
                    report["contact_pages"].append(dict(direction=direction, path=path.relative_to(base.ROOT).as_posix(), sha256=hashlib.sha256(pixels).hexdigest()))
            else:
                # Facility's original default directory was bound at import;
                # pass it explicitly so no candidate can escape to the old cache.
                arguments = {"output_dir": context.images} if stage == "facility" else {}
                report, files = module.derive(**arguments)[-2:]
            # Only original source candidates enter this selected images owner;
            # their actual producer checks run before any packing can use them.
            for path, encoded in files.items():
                contained(path, context.images).write_bytes(encoded)
            write_document(context.reports / "derived.json", report)
            if hasattr(module, "check"):
                module.check(report, files)
            if stage != "collector":
                source_guides(context, report)
    atlases = report.get("atlases", [report])
    if inputs:
        report["selected_input_aliases"] = inputs
    return report, [identity(base.ROOT / row["path"]) for row in atlases]


def produce(stage, context, source_manifests=None, dependencies=None):
    """Produce selected candidates while preserving every relevant immutable source.

    @param stage Exact bounded source or packing stage.
    @param context Caller-selected RunContext; this function reserves no invocation.
    @param source_manifests Exact current source closure required for packing only.
    @param dependencies Explicit selected predecessor reports for source conversion.
    @return Measured source checks, report and generated file identities.
    """
    if stage not in STAGES:
        raise ValueError("unknown selected M1 conversion stage")
    if context.root.resolve() != base.ROOT:
        raise ValueError("source conversion context belongs to another checkout")
    # Identifiers and shared containment constrain the original converters'
    # temporary destination variables to this exact repository-local run.
    for value in (context.feature, context.scenario, context.run_id):
        identifier(value)
    for directory in (context.images, context.data, context.reports):
        contained(directory, base.ROOT / "mlm/reviews").mkdir(parents=True, exist_ok=True)
    scope = check_tools()
    before = source_snapshot(stage)
    try:
        if stage == "packing":
            import pack_m1_art as pack
            report, files = pack.derive(source_manifests=source_manifests, context=context)
            # Shipping bytes are an existing independent comparison, not an
            # expected image generated from the candidate under verification.
            for role in ("png", "nfo", "header"):
                actual = context.data / Path(scope[role]).name
                expected = base.ROOT / scope[role]
                if files[actual] != expected.read_bytes():
                    raise ValueError("selected packing differs from canonical shipping bytes: " + role)
            # Publish disposable data only after its existing independent
            # shipping comparison; canonical distribution files stay untouched.
            for path, encoded in files.items():
                contained(path, context.data).write_bytes(encoded)
            artifacts = [identity(path) for path in files]
        else:
            report, artifacts = produce_source(stage, context, dependencies)
        write_document(context.reports / "derived.json", report)
        inventory = dict(_documentation=__doc__, schema_version=1, kind="m1_generated_sources",
                         stage=stage, run_id=context.run_id, artifacts=artifacts,
                         manifest=identity(context.reports / "derived.json"), source_files=len(before), publishes_source=False)
        write_document(context.data / "source-data.json", inventory)
    finally:
        # A successful or failed candidate phase must preserve the original
        # media, public completion flags and canonical distribution artifacts.
        for path, digest in before.items():
            if base.digest(path) != digest:
                raise ValueError("candidate conversion changed immutable source: " + str(path))
    return dict(checks=["actual original converter and source checks completed",
                        "immutable source/public/canonical bytes preserved"], report=report, artifacts=artifacts,
                source_files=len(before), publishes_source=False)


def candidate_encoder_tools():
    """Discover actual encoder tools through PATH and enforce original graphics pins.

    @return Actual executable paths, versions and content hashes; no machine path is retained in source.
    """
    scope = check_tools()
    tools = {}
    # Actual binary probes, rather than old build-cache strings, establish the
    # encoder pair used for these bytes. Missing tools name their exact supplier.
    for name, arguments in (("cmake", ["--version"]), ("grfcodec", ["-v"]),
                            ("nforenum", ["--version"]), ("grfid", ["--version"])):
        executable = shutil.which(name)
        if executable is None:
            raise ValueError("candidate GRF requires PATH tool: " + name)
        probe = subprocess.run([executable, *arguments], capture_output=True, text=True, timeout=10)
        version = (probe.stdout + probe.stderr).strip()
        # GRFID has no consistent version option; its paired binary identity is
        # recorded, while the real checksum invocation must succeed below.
        if name in ("grfcodec", "nforenum") and scope["toolchain"][name] not in version:
            raise ValueError("candidate encoder differs from pinned version: " + name)
        path = Path(executable).resolve()
        tools[name] = {"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(), "version": version}
    return tools


def produce_candidate_grf(context, manifests):
    """Join five current packing suppliers using the existing CMake GRF owner.

    @param context Explicit disposable candidate invocation, never canonical media.
    @param manifests Exact M1/Space/orbital/launch/Ground packing report identities from this run.
    @return Source/tool/output identities and two independently encoded equal candidates.
    """
    families = ("m1", "space", "orbital", "launch", "ground")
    if set(manifests) != set(families):
        raise ValueError("candidate GRF requires exactly five current packing suppliers")
    tools = candidate_encoder_tools()
    before = source_snapshot("ground")
    native = base.ROOT / "media/baseset/openttd"
    protected = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in native.iterdir() if path.suffix in (".png", ".nfo")}
    protected.update({path: hashlib.sha256(path.read_bytes()).hexdigest() for path in
                      (base.ROOT / "media/baseset/openttd.grf", base.ROOT / "media/baseset/openttd.grf.hash")})
    selected, suppliers = {}, {}
    # Every family supplies its current PNG/NFO/header under the selected run.
    # Reports cannot substitute canonical media or another run's old products.
    for family in families:
        reference = manifests[family]
        path = contained(base.ROOT / reference["path"], base.ROOT / "mlm/reviews")
        parts = path.relative_to(base.ROOT / "mlm/reviews").parts
        if len(parts) != 5 or parts[1] != family + "-derived-inputs" or parts[2] != context.run_id or parts[3] != "reports":
            raise ValueError("candidate packing supplier has a foreign family/run/role: " + family)
        if identity(path) != reference:
            raise ValueError("candidate packing supplier changed: " + family)
        document = json.loads(path.read_text(encoding="utf-8"))
        scope = json.loads((base.ROOT / f"media/multilayer/{family}-pack-scope.json").read_text(encoding="utf-8"))
        suppliers[family] = dict(manifest=reference, files={})
        for role in ("png", "nfo", "header"):
            candidate = contained(path.parent.parent / "data" / Path(scope[role]).name, path.parent.parent / "data")
            actual = identity(candidate)
            recorded = document.get(role)
            if recorded is None:
                recorded = next((row for row in document["outputs"] if row["path"] == actual["path"]), None)
            if recorded is None or recorded["sha256"] != actual["sha256"]:
                raise ValueError("candidate packing output identity changed: " + family + "/" + role)
            suppliers[family]["files"][role] = actual
            if role != "header":
                selected[candidate.name] = candidate
    encoded = []
    context.data.mkdir(parents=True, exist_ok=True)
    try:
        # Both encodes use fresh isolated workspaces and the same original CMake
        # script. No cached GRF, decoder output or canonical destination participates.
        for repeat in range(2):
            workspace = context.data / ("encoder-" + str(repeat + 1) + "-" + uuid.uuid4().hex)
            source = workspace / "openttd"
            sprites = workspace / "sprites"
            source.mkdir(parents=True); sprites.mkdir()
            for original in protected:
                if original.parent != native:
                    continue
                actual = selected.get(original.name, original)
                shutil.copyfile(actual, source / original.name)
                if original.suffix == ".png":
                    shutil.copyfile(actual, sprites / original.name)
            output = workspace / "candidate.grf"
            command = [tools["cmake"]["path"], "-DGRF_SOURCE_FOLDER=" + str(source), "-DGRF_BINARY_FILE=" + str(output),
                       *["-D" + name.upper() + "_EXECUTABLE=" + tools[name]["path"] for name in ("grfcodec", "nforenum", "grfid")],
                       "-P", str(base.ROOT / "cmake/scripts/CreateGRF.cmake")]
            terminal = context.evidence / f"encoder-{repeat + 1}.txt"
            terminal.parent.mkdir(parents=True, exist_ok=True)
            with terminal.open("w", encoding="utf-8") as stream:
                child = subprocess.run(command, cwd=workspace, stdout=stream, stderr=subprocess.STDOUT, timeout=110)
            if child.returncode != 0:
                raise ValueError("candidate CMake GRF encoder failed; see " + str(terminal))
            encoded.append(dict(grf=identity(output), checksum=identity(Path(str(output) + ".hash")), terminal=identity(terminal)))
        if encoded[0]["grf"]["sha256"] != encoded[1]["grf"]["sha256"] or encoded[0]["checksum"]["sha256"] != encoded[1]["checksum"]["sha256"]:
            raise ValueError("repeated candidate GRF/hash bytes differ")
        # This merge changes native ownership, not selected pixels or offsets.
        # Original shipping bytes therefore remain an independent exact check.
        if encoded[0]["grf"]["sha256"] != protected[base.ROOT / "media/baseset/openttd.grf"]:
            raise ValueError("candidate GRF differs from independently retained shipping bytes")
        for role, name in (("grf", "openttd.grf"), ("checksum", "openttd.grf.hash")):
            shutil.copyfile(base.ROOT / encoded[0][role]["path"], context.data / name)
        report = dict(_documentation=__doc__, schema_version=1, kind="selected_merged_art_candidates", run_id=context.run_id,
                      suppliers=suppliers, tools=tools, encoder=identity(base.ROOT / "cmake/scripts/CreateGRF.cmake"), repeats=encoded,
                      candidate=identity(context.data / "openttd.grf"), checksum=identity(context.data / "openttd.grf.hash"),
                      protected_files=len(before) + len(protected), deterministic=True, publishes_source=False, native_proof=False,
                      checks=["all five existing source/packing suppliers are current and exact",
                              "existing CMake encoder produced two identical candidate GRF/hash pairs",
                              "independent original shipping GRF bytes match; originals and canonical media remain unchanged"])
        write_document(context.reports / "candidates.json", report)
    finally:
        # Success and failure preserve authored media, public status, canonical
        # shipping art and scope headers alike; disposable encoding grants no publication.
        for path, expected in {**before, **protected}.items():
            if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
                raise ValueError("candidate encoding changed immutable input: " + str(path))
    return report


def execute_stage(request, root):
    """Adapt the existing selected recipe executor to original M1 source producers.

    @param request Registered prepare/verify request with explicit inputs and outputs.
    @param root Exact selected source checkout.
    @return Actual measured preparation/conversion checks for the shared dispatcher.
    """
    if root.resolve() != base.ROOT:
        raise ValueError("M1 recipe belongs to another checkout")
    # Current cleanup recipes have no authoring publication or paid-work mode.
    if request["publishes_source"] is not False or request["paid_work_authorized"] is not False:
        raise ValueError("source recipe cannot publish or perform paid work")
    check_tools()
    if request["recipe"] == "merged-art-candidates":
        if request["stage"] == "prepare":
            candidate_encoder_tools()
            return dict(checks=["pinned existing encoder tools and explicit current packing suppliers admitted"])
        context = RunContext(root, request["feature"], request["scenario"], request["run_id"])
        return produce_candidate_grf(context, {row["as"]: request["inputs"][row["as"]] for row in request["declaration"]["requires"]})
    if request["stage"] == "prepare":
        return dict(checks=["original pinned encoders and explicit selected source roots verified"])
    stage = "packing" if request["recipe"] == "m1-derived-inputs" else request["declaration"]["original_parameters"]["stage"]
    context = RunContext(root, request["feature"], request["scenario"], request["run_id"])
    manifests = None
    if stage == "packing":
        public = json.loads(base.PUBLIC.read_text(encoding="utf-8"))
        families = {row["asset_id"]: row for row in public["assets"]}
        scope = check_tools()
        required = {families[name]["authoring"]["atlas_manifest"] for name in scope["asset_ids"]}
        manifests = {}
        # Every dependency's original alias is explicit in its declaration;
        # packing cannot guess a previous source by basename or newest run.
        recipes = {row["id"]: row for row in json.loads((root / "mlm/review-producers.json").read_text(encoding="utf-8"))["recipes"]}
        for dependency in request["declaration"]["requires"]:
            alias = recipes[dependency["recipe"]]["original_parameters"]["manifest_alias"]
            manifests[alias] = request["inputs"][dependency["as"]]
        if set(manifests) != required:
            raise ValueError("selected packing source closure is incomplete")
    dependencies = {row["as"]: request["inputs"][row["as"]] for row in request["declaration"]["requires"]}
    facts = produce(stage, context, manifests, dependencies)
    facts.pop("report")
    return facts


def main():
    """Run one explicit standalone source candidate or selected packing request.

    @return None; recipe children should use the shared structured dispatcher.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("stage", choices=STAGES)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--source-manifests", type=Path, help="exact alias-to-current-identity JSON for packing")
    parser.add_argument("--dependencies", type=Path, help="exact selected predecessor source report identities")
    args = parser.parse_args()
    manifests = json.loads(args.source_manifests.read_text(encoding="utf-8")) if args.source_manifests else None
    dependencies = json.loads(args.dependencies.read_text(encoding="utf-8")) if args.dependencies else None
    context = create_run_context("multilayer-art", "m1-source-" + args.stage, run_id=args.run_id)
    facts = produce(args.stage, context, manifests, dependencies)
    print(json.dumps(dict(run_id=context.run_id, checks=facts["checks"], artifacts=facts["artifacts"]), indent=2))


if __name__ == "__main__":
    main()
