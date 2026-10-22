"""Reconstruct selected Ground originals through their actual source producers.

@file regenerate_ground_sources.py Source candidates never publish shared media.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups17DB-17DDC.
Frozen descriptor and pixel expectations describe historical physical artwork;
current native qualification remains a separate executable-producing operation.
"""

from contextlib import contextmanager
import copy
from functools import partial
import importlib
import json
from pathlib import Path
from types import ModuleType

from artifact_paths import contained
from artifact_paths import RunContext
from evidence import identity
import regenerate_art_sources as shared
from ground_source_facts import load_facts, pose_digest, verify_facts
from tools import ground_art_handoff as handoff
from tools import ground_mine_contract, ground_processor_contract
from tools import ground_station_contract, ground_terminal_contract

ROOT = shared.base.ROOT
PROCESSORS = {"facility.metal-refinery": "processor", "facility.chemical-works": "chemical",
              "facility.battery-works": "battery", "facility.electronics-works": "electronics"}
CONVENTIONAL = {"vehicle.model-05": "ground_wagon", "vehicle.model-06": "sealed_wagon",
                "vehicle.model-07": "mine_truck", "vehicle.model-08": "sealed_road"}
CAPSULES = tuple("vehicle.model-" + str(model).zfill(2) for model in range(9, 14))
STATIONS = {"facility.rail-station": "station", "facility.road-terminal": "terminal"}
TUBE_INFRA = {"tube.straight": "art", "tube.bend45": "curves", "tube.junction": "junctions",
              "tube.end": "ends", "tube.crossing": "crossings", "tube.conduit": "conduits",
              "facility.tube-terminal": "terminals", "facility.tube-service": "facilities",
              "facility.tube-interchange": "interchanges"}
ENERGY = {"geothermal.vent": "vent", "facility.vent-powerhouse": "generator",
          "facility.magma-powerhouse": "generator", "facility.utility-riser": "riser",
          "geothermal.conduit": "utility", "control.tube": "feature_controls", "control.geothermal": "feature_controls"}
CONTROLS = ("control.tube", "control.geothermal")


def read_scope():
    """Check the original Ground scope's pinned encoder and serializer.

    @return Exact original scope; a changed workstation tool cannot rebase pixels.
    """
    scope = json.loads(handoff.SCOPE.read_text(encoding="utf-8"))
    # Both serializers affect the independent original PNG/report identities.
    if scope["toolchain"]["python"] != shared.sys.version.split()[0] or scope["toolchain"]["pillow"] != shared.PIL.__version__:
        raise ValueError("Ground conversion toolchain differs from pinned versions")
    return scope


def selected_path(reference, context, role):
    """Require an exact current-run generated identity and its declared role.

    @param reference Concrete path and SHA256 returned by the actual producer.
    @param context Explicit invocation consuming the source result.
    @param role Required images, reports or data owner.
    @return Verified contained file; foreign runs and changed bytes refuse.
    """
    path = contained(ROOT / reference["path"], ROOT / "mlm/reviews")
    parts = path.relative_to(ROOT / "mlm/reviews").parts
    # Related source leaves share the explicit invocation while retaining their
    # separate feature/scenario owners; older runs cannot supply prerequisites.
    if len(parts) != 5 or parts[2] != context.run_id or parts[3] != role:
        raise ValueError("Ground source has a foreign invocation or generated role")
    if identity(path) != reference:
        raise ValueError("Ground selected source identity changed")
    return path


def converter(asset):
    """Select only the existing original producer for one dispatched material.

    @param asset Existing public mine, processing-facility or fleet body identity.
    @return Actual converter module and its original generation record path.
    """
    if asset in ENERGY:
        name = ENERGY[asset]
        module = importlib.import_module("convert_" + name + ("" if name == "feature_controls" else "_art"))
        return module, module.RECORD
    if asset in TUBE_INFRA:
        module = importlib.import_module("convert_tube_" + TUBE_INFRA[asset])
        return module, importlib.import_module("convert_tube_art").RECORD
    if asset in STATIONS:
        module = importlib.import_module("convert_" + STATIONS[asset] + "_states")
        owner = module.loader if asset == "facility.rail-station" else module.terminal
        return module, owner.RECORD
    if asset.startswith("mine."):
        if asset.removeprefix("mine.") not in ground_mine_contract.SELECTED:
            raise ValueError("Ground material source is outside the selected mine scope")
        module = importlib.import_module("convert_mine_states" if asset == "mine.COAL" else "convert_material_mine_art")
        return module, module.mine.RECORD
    if asset not in PROCESSORS:
        if asset in CONVENTIONAL:
            module = importlib.import_module("convert_" + CONVENTIONAL[asset] + "_states")
            return module, module.fleet.RECORD
        if asset in CAPSULES:
            module = importlib.import_module("convert_tube_capsule_states")
            return module, module.tube.RECORD
        raise ValueError("Ground source is outside the selected material/fleet scope")
    module = importlib.import_module("convert_" + PROCESSORS[asset] + "_states")
    return module, module.plant.RECORD


def read_record(path):
    """Read the actual retained original generation record selected by its producer.

    @param path Existing original media record, never a generated review document.
    @return Parsed original selection/component ancestry without modifying any hash.
    """
    return json.loads(contained(path, ROOT / "media/multilayer/source").read_text(encoding="utf-8"))


def native_references(report):
    """Read actual native atlas identities from the original explicit producer schema.

    @param report Actual physical-parts or body/purchase atlas-list source document.
    @return Existing asset IDs mapped to actual current source image identities.
    """
    if "atlases" in report:
        result = {row["asset_id"]: {key: row[key] for key in ("path", "sha256")} for row in report["atlases"]}
        if len(result) != len(report["atlases"]):
            raise ValueError("Ground source has duplicate native atlas identities")
        return result
    # The actual feature-control owner emits two separate source documents.
    # Its selected paired envelope names both original image identities.
    if report.get("asset_id") in CONTROLS and "native_sources" in report:
        return report["native_sources"]
    if "path" in report:
        return {report["asset_id"]: {key: report[key] for key in ("path", "sha256")}}
    return {report["asset_id"]: {"path": report["source"], "sha256": report["source_sha256"]}}


def build_source(module, record, family, public, context, dependencies=None):
    """Call the actual original bounded producer and its existing contact-page owner.

    @param module Selected original material or fleet converter module.
    @param record Actual immutable generation record, including original component ancestry.
    @param family Selected canonical public source family; publication flags stay unchanged.
    @param public Complete canonical public document needed by paired fleet producers.
    @param context Explicit current output owner already selected by the shared adapter.
    @param dependencies Exact selected current prerequisite report identities for review context.
    @return Actual original report and generated byte map, with no publisher call.
    """
    asset = family["asset_id"]
    if asset in ENERGY:
        return build_energy_source(module, record, public, asset, context, dependencies=dependencies)
    if asset in TUBE_INFRA:
        return build_tube_source(module, record, asset, context)
    if asset in STATIONS:
        return module.build(record, family, source_only=True)
    if asset in CONVENTIONAL:
        return module.build(document=record, public=public)
    if asset not in CAPSULES:
        return module.build(record, family)
    report, files = module.build(document=record, asset_id=asset)
    # The capsule owner normally adds hashes/contacts in its publishing entry
    # point. Invoke those actual pure producers here, keeping publication separate.
    for atlas in report["atlases"]:
        atlas["sha256"] = shared.hashlib.sha256(files[ROOT / atlas["path"]]).hexdigest()
    report["contact_pages"] = []
    for name, encoded in module.contact_pages(report, files).items():
        path = contained(context.images / name, context.images)
        files[path] = encoded
        report["contact_pages"].append(dict(path=path.relative_to(ROOT).as_posix(), sha256=shared.hashlib.sha256(encoded).hexdigest()))
    expected = load_facts()["source_pose_contracts"][asset]["original_report"]
    path = context.reports / Path(expected["path"]).name
    files[path] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def build_tube_source(module, record, asset, context):
    """Run an actual tube facility/geometry producer and its original contact owner.

    @param module Original selected converter, never a publisher or old-report reader.
    @param record Exact retained tube model/component/reuse provenance.
    @param asset Existing public tube infrastructure family.
    @param context Selected current images/reports owner already routed by the shared adapter.
    @return Actual source report, indexed atlas and original contact-page bytes.
    """
    builder = module.build_straight if asset == "tube.straight" else module.build
    report, encoded = builder(document=record)
    if asset == "tube.junction":
        pages = module.review_pages(report, encoded)
    elif asset == "facility.tube-terminal":
        pages = module.contact_pages(report, encoded)
    elif asset == "facility.tube-interchange":
        pages = module.terminals.contact_pages(report, encoded)
    elif asset == "tube.straight":
        pages = {"source": module.straight_review(report, encoded)}
    else:
        pages = {"source": module.contact_board(report, encoded)}
    return source_files(report, encoded, pages, context)


def source_files(report, encoded, pages, context):
    """Bind actual scalar/atlas-list source and contact bytes to selected file roles.

    @param report Original producer's actual source document, never a historical cached report.
    @param encoded Actual producer's canonical indexed native atlas bytes.
    @param pages Actual original contact/geometry producer's encoded review pages.
    @param context Exact selected images/reports owner shared by this source leaf.
    @return Current source report and generated byte map without publication.
    """
    asset = report["asset_id"]
    if "atlases" not in report:
        report["sha256"] = shared.hashlib.sha256(encoded).hexdigest()
    native = native_references(report)[asset]
    files = {ROOT / native["path"]: encoded}
    report["contact_pages"] = []
    # Reuse real source-scale diagnostic producers for every topology/axis;
    # selected filenames change ownership only, never composition or pixels.
    for name, pixels in pages.items():
        if isinstance(name, Path):
            path = contained(name, context.images)
        else:
            path = contained(context.images / (asset.replace(".", "-") + "-" + str(name) + "-contact.png"), context.images)
        files[path] = pixels
        report["contact_pages"].append(dict(path=path.relative_to(ROOT).as_posix(), sha256=shared.hashlib.sha256(pixels).hexdigest()))
    expected = load_facts()["source_pose_contracts"][asset]["original_report"]
    path = context.reports / Path(expected["path"]).name
    files[path] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def build_energy_source(module, record, public, asset, context, dependencies=None):
    """Execute actual energy/utility/control source and geometry/contact producers.

    @param module Original selected source owner with unchanged physical algorithms.
    @param record Its retained original candidate/component/selection provenance.
    @param public Complete canonical public contract, never modified or published.
    @param asset Existing energy, utility or primary paired-control source identity.
    @param context Explicit current source destinations already selected by the shared adapter.
    @param dependencies Explicit current prerequisite reports; the riser guide needs the conduit leaf.
    @return Actual source document and native/guide/contact byte map.
    """
    if asset in CONTROLS:
        reports, encoded = module.build(document=record, public=public)
        files, native = {}, {}
        # The original control producer creates both distinct controls. Preserve
        # each original report and pixel identity while sharing its real contact.
        for selected, report in reports.items():
            report["sha256"] = shared.hashlib.sha256(encoded[selected]).hexdigest()
            native[selected] = {key: report[key] for key in ("path", "sha256")}
            files[ROOT / report["path"]] = encoded[selected]
            original = load_facts()["source_pose_contracts"][selected]["original_report"]
            files[context.reports / Path(original["path"]).name] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
        path = context.images / "feature-controls-contact.png"
        files[path] = module.contact_board(reports, encoded)
        report = reports[asset]
        report.update(native_sources=native, contact_pages=[dict(path=path.relative_to(ROOT).as_posix(),
                                                               sha256=shared.hashlib.sha256(files[path]).hexdigest())])
        return report, files
    if ENERGY[asset] == "generator":
        report, encoded = module.build_generator(asset, document=record, public=public)
        pages = module.generator_contact_pages(report, encoded)
    elif asset == "geothermal.vent":
        report, encoded = module.build(document=record)
        pages = {"source": module.contact_page(report, encoded)}
    elif asset == "facility.utility-riser":
        report, encoded = module.build(document=record, public=public)
        _family, models, _component, _phases = module.source_inputs(record, public)
        whole, pieces, _original = models["ready"]
        reference = (dependencies or {}).get("conduit-source")
        if reference is None:
            raise ValueError("riser guide requires selected ground-source-geothermal-conduit input")
        prior = json.loads(selected_path(reference, context, "reports").read_text(encoding="utf-8"))
        expected = load_facts()["source_pose_contracts"]["geothermal.conduit"]
        # An explicit file identity alone cannot borrow another family's source
        # authority. The guide needs this actual approved physical conduit leaf.
        if prior.get("kind") != "selected_ground_source" or prior.get("asset_id") != "geothermal.conduit" or prior["native_source"]["sha256"] != expected["original_atlas"]["sha256"]:
            raise ValueError("riser guide selected a foreign conduit source family")
        raw = selected_path(prior["raw_source_report"], context, "reports")
        atlas = selected_path(prior["native_source"], context, "images")
        conduit = json.loads(raw.read_text(encoding="utf-8"))
        pages = {"state": module.state_contact(report, encoded),
                 "geometry": module.geometry_contact(whole, pieces, conduit_report=conduit, conduit_atlas=atlas)}
    else:
        report, encoded = module.build(document=record, public=public)
        pages = {"source": module.contact_board(report, encoded), "originals": module.original_board(record, public),
                 "geometry": module.geometry_board(report, encoded)}
    return source_files(report, encoded, pages, context)


def related_converters(module):
    """Follow the actual original converter imports sharing generated destinations.

    @param module Selected pure producer whose nested composition must stay in this run.
    @return Original converter modules, excluding the shared base already routed by its owner.
    """
    pending, seen, related = [module], set(), []
    # Composition modules bind OUTPUT independently at import. Following their
    # actual imports prevents nested byte-map lookups from escaping this run.
    while pending:
        current = pending.pop()
        if current in seen:
            continue
        seen.add(current)
        related.append(current)
        for value in vars(current).values():
            # Only repo-owned original converters participate in this routing;
            # test, library and source-policy modules retain their own owners.
            if isinstance(value, ModuleType) and value.__name__.startswith("convert_") and value is not shared.base:
                contained(Path(value.__file__), ROOT / "media/multilayer")
                pending.append(value)
    return related


@contextmanager
def historical_geometry(asset=None, related=()):
    """Select the independently frozen physical geometry for original source calls.

    @param asset Optional explicit station/terminal family selecting original geometry aliases.
    @param related Actual original converter imports whose bound geometry readers share this role.
    @yield None; defaults are restored before any current native consumer can run.
    """
    owners = [ground_mine_contract, ground_processor_contract]
    changes = []
    if asset in STATIONS:
        owner = ground_station_contract if asset == "facility.rail-station" else ground_terminal_contract
        owners.append(owner)
        name = STATIONS[asset] + "_contract"
        changes.append((owner, name, partial(getattr(owner, name), source_only=True)))
        modules = list(related)
        if asset == "facility.rail-station":
            modules.append(importlib.import_module("create_station_art_guides"))
        reader = owner.load_geometry
        # Original station views/guides import this reader directly. Select the
        # same explicit historical role for those exact aliases, without ever
        # substituting historical hashes for current source_digest measurements.
        for module in modules:
            if getattr(module, "load_geometry", None) is reader:
                changes.append((module, "load_geometry", partial(reader, source_only=True)))
    changes.extend((owner, "load_geometry", partial(owner.load_geometry, source_only=True)) for owner in owners)
    previous = [(owner, name, getattr(owner, name)) for owner, name, _ in changes]
    try:
        # The original converters call these owners dynamically. Explicit
        # source-only selection preserves descriptor bytes and default native gates.
        for owner, name, value in changes:
            setattr(owner, name, value)
        yield
    finally:
        # A serial leaf must never leak historical geometry authority into a
        # later native qualification, including when source conversion refuses.
        for owner, name, value in reversed(previous):
            setattr(owner, name, value)


def current_source_view(family, scope, facts, report, raw):
    """Select a location-only current source view while preserving original physical authority.

    @param family Canonical public source row; it is never mutated or published.
    @param scope Original Ground adapter and physical absence contract.
    @param facts Independently sealed original pose/pixel expectations.
    @param report Actual producer metadata with its current image paths.
    @param raw Identity of the actual producer report under current reports.
    @return Detached family, one-family scope and original source comparison view.
    """
    asset = family["asset_id"]
    native = native_references(report)[asset]
    current = copy.deepcopy(family)
    reference = copy.deepcopy(next(row for row in scope["source_rows"] if row["asset_id"] == asset))
    reference.update(manifest=raw["path"], manifest_sha256=raw["sha256"],
                     source_atlas=native["path"], source_sha256=native["sha256"])
    current.setdefault("authoring", {}).update(native_atlas=native["path"], native_sha256=native["sha256"],
                                atlas_manifest=raw["path"], atlas_manifest_sha256=raw["sha256"])
    selected = dict(scope, source_rows=[reference])
    original = copy.deepcopy(facts["selected_public_facts"][asset])
    if asset in facts["derived_source_inputs"]:
        historical = facts["derived_source_inputs"][asset]["historical_derived_source"]
        # Four original public source spellings name generated atlases. Only
        # their explicitly frozen role permits a location-only current view;
        # exact original pixels and all other physical/source facts stay fixed.
        if {key: original["source"][key] for key in ("path", "sha256")} != historical or native["sha256"] != historical["sha256"]:
            raise ValueError("Ground historical generated-source role or original pixels changed")
        current["source"]["path"] = native["path"]
        original["source"]["path"] = native["path"]
    before = {"public": {"assets": [original]}}
    return current, selected, before


def verify_poses(asset, poses, facts):
    """Require the complete independently captured original physical pose matrix.

    @param asset Existing selected public family identity.
    @param poses Actual unchanged normalizer output from selected current source bytes.
    @param facts Independently preserved original physical expectations.
    @return None; generated actuals can never become their own new expected baseline.
    """
    expected = facts["source_pose_contracts"][asset]
    # Original pixels alone do not prove cell ownership, state aliases, native
    # signed anchors, intentional absence or company-mask positions.
    if len(poses) != expected["pose_count"] or pose_digest(poses) != expected["pose_contract_sha256"]:
        raise ValueError("Ground source differs from independent original physical poses")


def decode_source(family, scope, facts, report, raw):
    """Run the unchanged Ground normalizer against independent original physical expectations.

    @param family Canonical public source row, never mutated or published.
    @param scope Original explicit Ground source adapters and absence contracts.
    @param facts Independently frozen original source/pose/prefix expectations.
    @param report Actual selected producer envelope with explicit current native identities.
    @param raw Exact current original producer report identity for this physical family.
    @return Complete independently compared pose count; no current native proof is inferred.
    """
    asset = family["asset_id"]
    current, selected, before = current_source_view(family, scope, facts, report, raw)
    images = {}
    try:
        poses, _image = handoff.source_poses(current, selected, before, {}, images)
        verify_poses(asset, poses, facts)
        return len(poses)
    finally:
        # The original normalizer caches open atlases; release only images
        # created by this bounded source check, even on a physical refusal.
        for image in images.values():
            image.close()


def bind_sources(public, scope, before, context, manifests):
    """Bind exactly47current physical sources without reading historical reports.

    @param public Canonical public manifest; only a detached consumer view is changed.
    @param scope Independently preserved original ordered source/range/absence contract.
    @param before Compact original physical/prefix comparison view from retained facts.
    @param context Explicit packing invocation shared by all selected source producers.
    @param manifests Exact asset-to-current typed source report identities from declared suppliers.
    @return Detached current public, source scope and comparison view for the unchanged packer.
    """
    facts = load_facts()
    if set(manifests) != set(scope["asset_ids"]):
        raise ValueError("Ground packing requires every exact selected physical source report")
    current, selected, frozen = copy.deepcopy(public), copy.deepcopy(scope), copy.deepcopy(before)
    families = {row["asset_id"]: row for row in current["assets"]}
    originals = {row["asset_id"]: row for row in frozen["public"]["assets"]}
    documents, references = {}, []
    # Every source row retains its immutable native range/order. Paired source
    # leaves may share one typed envelope but preserve each raw producer schema.
    for asset in scope["asset_ids"]:
        path = selected_path(manifests[asset], context, "reports")
        if path not in documents:
            documents[path] = json.loads(path.read_text(encoding="utf-8"))
        document = documents[path]
        primary = asset.replace("purchase.", "vehicle.", 1)
        if asset in CONTROLS:
            primary = "control.tube"
        module, _record = converter(primary)
        approval = facts["source_pose_contracts"][primary]["original_report"]
        # A report from another family or an old schema cannot borrow current
        # source authority, even where two legitimate atlases have identical PNGs.
        if (document.get("kind") != "selected_ground_source" or document.get("collection") != "ground"
                or document.get("run_id") != context.run_id or document.get("asset_id") != primary
                or document.get("source_producer") != identity(Path(module.__file__))
                or document.get("historical_approval") != approval
                or document.get("source_coverage_complete") is not True
                or document.get("runtime_activated") is not False):
            raise ValueError("Ground selected source producer/family/approval changed")
        native = document["native_sources"][asset]
        image = selected_path(native, context, "images")
        raw = document["raw_source_reports"][asset]
        raw_path = selected_path(raw, context, "reports")
        # All actual outputs belong to the declared leaf, including paired art.
        # A same-run path in another scenario cannot stand in for its writer.
        if image.parent.parent != path.parent.parent or raw_path.parent.parent != path.parent.parent:
            raise ValueError("Ground selected source has a foreign writer scenario")
        if native["sha256"] != facts["source_pose_contracts"][asset]["original_atlas"]["sha256"]:
            raise ValueError("Ground selected source differs from original approved native pixels")
        family, source_scope, original = current_source_view(families[asset], scope, facts, document, raw)
        families[asset] = family
        references.append(source_scope["source_rows"][0])
        originals[asset] = original["public"]["assets"][0]
    current["assets"] = [families[row["asset_id"]] for row in current["assets"]]
    selected["source_rows"] = references
    frozen["public"]["assets"] = list(originals.values())
    return current, selected, frozen


def produce_source(asset, context, dependencies=None):
    """Generate one actual original source family or paired model without publication.

    @param asset Existing public material/fleet body identity; no unselected family is accepted.
    @param context Explicit current invocation owning generated images/reports/data.
    @param dependencies Explicit current M1 reports for genuine reused parts and review-floor context.
    @return Actual source identities and independent preservation/physical comparison facts.
    """
    scope = read_scope()
    facts = load_facts()
    verify_facts()
    module, record_path = converter(asset)
    shared.source_directories(context.images, context=context)
    context.images.mkdir(parents=True, exist_ok=True)
    context.data.mkdir(parents=True, exist_ok=True)
    document = json.loads(handoff.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in document["assets"] if row["asset_id"] == asset)
    record = read_record(record_path)
    inputs = {}
    # Genuine Earth receiver/control/cargo/hardness inputs are reconstructed prerequisites,
    # never old atlases promoted into originals or silently read from logs.
    for reference in (dependencies or {}).values():
        path = selected_path(reference, context, "reports")
        dependency = json.loads(path.read_text(encoding="utf-8"))
        if dependency.get("kind") == "selected_ground_source":
            # Ground source prerequisites pass their own typed current identity
            # to the actual guide owner, rather than masquerading as M1 aliases.
            for native in dependency["native_sources"].values():
                selected_path(native, context, "images")
            continue
        for atlas in dependency["atlases"]:
            selected_path({key: atlas[key] for key in ("path", "sha256")}, context, "images")
            inputs["logs/proofs/mlm/native-art/derived/" + Path(atlas["path"]).name] = {key: atlas[key] for key in ("path", "sha256")}
    before = shared.source_snapshot("ground", records=[record_path])
    # Ground canonical files and frozen prefixes remain independent immutable
    # shipping comparisons, never destinations of the selected source producer.
    protected = [ROOT / scope[key] for key in ("png", "nfo", "header")]
    protected.extend(ROOT / row["path"] for row in facts["protected_prefix_files"])
    # Only material source composition consumes these historical descriptors;
    # fleet/tube artwork cannot borrow their native provenance for another owner.
    historical_descriptors = facts["original_native_descriptors"] if asset.startswith("mine.") or asset in PROCESSORS else []
    if asset in STATIONS:
        import station_source_facts as station_facts
        original = station_facts.load_originals()["families"][asset]
        protected.extend((station_facts.FIXTURE, ROOT / original["descriptor"]["path"]))
        historical_descriptors = [original["descriptor"]]
    before.update({path: shared.base.digest(path) for path in protected})
    try:
        related = related_converters(module)
        with historical_geometry(asset=asset, related=related), shared.source_outputs(context, module, inputs=inputs, related=related):
            report, files = build_source(module, record, family, document, context, dependencies=dependencies)
        atlases = native_references(report)
        required = {asset, asset.replace("vehicle.", "purchase.", 1)} if asset.startswith("vehicle.") else {asset}
        if asset in CONTROLS:
            required = set(CONTROLS)
        if set(atlases) != required:
            raise ValueError("Ground source is missing its complete body/purchase atlas ownership")
        native = atlases[asset]
        expected = facts["source_pose_contracts"][asset]
        # Fleet leaves own both actual body and purchase art. Every returned
        # atlas must reproduce its independently approved original pixel identity.
        for actual_asset, reference in atlases.items():
            if reference["sha256"] != facts["source_pose_contracts"][actual_asset]["original_atlas"]["sha256"]:
                raise ValueError("Ground source differs from original approved native pixels")
        raw_paths = {}
        # Original pure producers return their genuine contact/GIF/atlas bytes.
        # Only their JSON destination changes role; source pixels stay untouched.
        for path, encoded in files.items():
            contained(path, context.directory)
            if path.suffix == ".json":
                target = contained(context.reports / path.name, context.reports)
                # Paired fleet art shares one original report; paired controls
                # retain two. Bind each actual family to its exact producer schema.
                for selected in atlases:
                    if path.name == Path(facts["source_pose_contracts"][selected]["original_report"]["path"]).name:
                        if selected in raw_paths:
                            raise ValueError("Ground original producer returned ambiguous source reports")
                        raw_paths[selected] = target
            else:
                target = contained(path, context.images)
            target.write_bytes(encoded)
        if set(raw_paths) != set(atlases):
            raise ValueError("Ground original producer omitted its source report")
        raw_reports = {selected: identity(path) for selected, path in raw_paths.items()}
        raw = raw_reports[asset]
        counts = {}
        # Paired fleet purchase art retains its own original UI geometry/masks;
        # decoding only the body would conceal a changed availability preview.
        for actual_asset, reference in atlases.items():
            selected_path(reference, context, "images")
            actual_family = next(row for row in document["assets"] if row["asset_id"] == actual_asset)
            counts[actual_asset] = decode_source(actual_family, scope, facts, report, raw_reports[actual_asset])
        count = counts[asset]
        report.update(kind="selected_ground_source", asset_id=asset, collection="ground", run_id=context.run_id,
                      historical_approval=expected["original_report"], native_source=native,
                      native_sources=atlases,
                      raw_source_report=raw, raw_source_reports=raw_reports, source_producer=identity(Path(module.__file__)),
                      current_source_decode={"logical_pose_count": count, "family_pose_counts": counts, "native_proof": False},
                      historical_native_descriptors=historical_descriptors)
        destination = context.reports / "derived.json"
        shared.write_document(destination, report)
        shared.write_document(context.data / "source-data.json",
                              dict(_documentation=__doc__, schema_version=1, kind="selected_ground_source_files",
                                   asset_id=asset, run_id=context.run_id, native_source=native, raw_source_report=raw,
                                   manifest=identity(destination), publishes_source=False, native_proof=False))
    finally:
        # Failed conversion cannot advance canonical/public source state any
        # more than successful conversion; preserve every protected original.
        for path, digest in before.items():
            if shared.base.digest(path) != digest:
                raise ValueError("Ground conversion changed immutable source: " + str(path))
    return dict(manifest=identity(destination), native_source=native, source_files=len(before),
                pose_count=count, family_pose_counts=counts, publishes_source=False, native_proof=False,
                checks=["actual original source/guide/contact producer completed",
                        "independent approved pixels and complete physical poses reproduced",
                        "original media/public/canonical bytes preserved"])


def produce_packing(context, manifests):
    """Run the original Ground packer on the explicit complete current source closure.

    @param context Exact current packing output owner, sharing source invocation identity.
    @param manifests All47physical source report identities supplied by declared source recipes.
    @return Actual canonical comparison/preservation facts and selected candidate identities.
    """
    import pack_ground_art as packing
    scope = read_scope()
    facts = load_facts()
    records = set()
    # Protect each original source owner once, including shared paired models
    # and controls. Generated source images stay disposable current inputs.
    for asset in scope["asset_ids"]:
        primary = asset.replace("purchase.", "vehicle.", 1)
        if asset in CONTROLS:
            primary = "control.tube"
        records.add(converter(primary)[1])
    before = shared.source_snapshot("ground", records=records)
    protected = [ROOT / scope[key] for key in ("png", "nfo", "header")]
    protected.extend(ROOT / row["path"] for row in facts["protected_prefix_files"])
    before.update({path: shared.base.digest(path) for path in protected})
    try:
        report, files = packing.derive(source_manifests=manifests, context=context)
        # Shipping bytes predate this candidate. They remain an independent
        # expectation; a new actual can never update its own PNG/NFO/header baseline.
        for key in ("png", "nfo", "header"):
            destination = context.data / Path(scope[key]).name
            if files[destination] != (ROOT / scope[key]).read_bytes():
                raise ValueError("Ground packing differs from canonical shipping " + key)
        context.data.mkdir(parents=True, exist_ok=True)
        context.reports.mkdir(parents=True, exist_ok=True)
        # Only independently compared candidates become current disposable data;
        # the original packer's publisher/CLI/canonical destinations are unused.
        for path, encoded in files.items():
            contained(path, context.data).write_bytes(encoded)
        destination = context.reports / "derived.json"
        shared.write_document(destination, report)
    finally:
        # Failed binding/packing cannot alter source any more than success.
        # Every original record/pixel/public/prefix/canonical byte remains exact.
        for path, digest in before.items():
            if shared.base.digest(path) != digest:
                raise ValueError("Ground packing changed immutable source: " + str(path))
    return dict(manifest=identity(destination), pose_count=report["pose_count"],
                native_start=report["native_start"], native_sprite_bound=report["native_sprite_bound"],
                native_proof=False, publishes_source=False,
                checks=["actual original Ground normalizer/native anchor/packing producers completed",
                        "all47independent original physical pose matrices preserved",
                        "PNG/NFO/header exactly match canonical shipping bytes",
                        "original media/public/prefix/shipping bytes preserved"])


def execute_stage(request, root):
    """Dispatch an enabled bounded Ground source leaf through the shared executor.

    @param request Exact registered current invocation with explicit prerequisite identities.
    @param root Selected checkout owning original media and current generated destinations.
    @return Actual measured input/preservation checks, never a publication or paid operation.
    """
    if root.resolve() != ROOT:
        raise ValueError("Ground source recipe belongs to another checkout")
    # Source recovery cannot authorize canonical publication or financial work.
    if request["publishes_source"] is not False or request["paid_work_authorized"] is not False:
        raise ValueError("Ground source recipe cannot publish or perform paid work")
    read_scope()
    verify_facts()
    if request["stage"] == "prepare":
        return dict(checks=["original physical/source/prefix facts and pinned encoder verified"])
    context = RunContext(root, request["feature"], request["scenario"], request["run_id"])
    if request["declaration"]["original_parameters"].get("stage") == "packing":
        recipes = {row["id"]: row for row in json.loads((root / "mlm/review-producers.json").read_text(encoding="utf-8"))["recipes"]}
        manifests = {}
        # Coverage comes from the existing declared original producers. Paired
        # source outputs share their actual manifest; no filename inference or
        # private asset catalogue selects missing packing inputs.
        for requirement in request["declaration"]["requires"]:
            reference = request["inputs"][requirement["as"]]
            for asset in recipes[requirement["recipe"]]["original_parameters"]["source_assets"]:
                if asset in manifests:
                    raise ValueError("Ground packing source coverage is ambiguous")
                manifests[asset] = reference
        return produce_packing(context, manifests)
    dependencies = {row["as"]: request["inputs"][row["as"]] for row in request["declaration"]["requires"]}
    return produce_source(request["declaration"]["original_parameters"]["asset"], context, dependencies=dependencies)
