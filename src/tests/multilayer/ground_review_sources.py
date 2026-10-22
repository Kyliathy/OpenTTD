"""Resolve explicit original source views and retained worlds for Ground native reviews.

@file ground_review_sources.py Existing independent normalizers; no old proof-cache discovery.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20B.
"""
import copy
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "media/multilayer"))
from artifact_paths import RunContext, contained, identifier, resolve_input_path
from artifact_rebuild import identity
from artifact_recipes import load_catalogue
from sprite_expected_payload import packing_inputs
import pack_m1_art
from regenerate_ground_sources import bind_sources, load_facts, verify_poses
from tools import ground_art_handoff as handoff

WORLD_CONTRACT = "src/tests/fixtures/graphics/ground-review-worlds.json"
WORLD_CONTRACT_SHA256 = "e56a5e7ae3a601e3dafe152561700455c8244a88f539032354ce57a588965737"
M1_ROLES = {"vehicle.model-04": "m1-source-vehicle", "purchase.model-04": "m1-source-purchase",
            "facility.vent-collector": "m1-source-collector"}


def retained_world(mode, root=ROOT):
    """Select only an independently preserved original native review world.

    @param mode Explicit original collector, industry-states, purchases, rail, road or stations burst.
    @param root Checkout owning immutable original expectations and retained-first save aliases.
    @return Contract row and exact existing hash-verified retained save, never a historical executable.
    """
    path = root / WORLD_CONTRACT
    if identity(path, root)["sha256"] != WORLD_CONTRACT_SHA256:
        raise ValueError("original Ground viewport expectations changed; restore retained bytes, never regenerate a baseline")
    document = json.loads(path.read_text(encoding="utf-8"))
    if mode not in document["modes"]:
        raise ValueError("unknown original Ground viewport scope")
    row = document["modes"][mode]
    source = resolve_input_path(row["source"]["path"], root)
    # Each mode retains its original earned/art-fixture world. Neither a newer
    # convenient save nor a disposable alias may replace the preserved bytes.
    if (identity(source, root)["sha256"] != row["source"]["sha256"]
            or not any(source.is_relative_to((root / owner).resolve())
                       for owner in ("savegames/retained", "src/tests/fixtures"))):
        raise ValueError("Ground viewport requires its exact retained original world")
    return row, source


class GroundSourceViews:
    """One independently checked current source view for all47Ground and3retained M1 families.

    @param root Checkout containing canonical original media and selected current source reports.
    @param source_run Explicit invocation supplying all pure source candidates.
    @param inputs Ground packing plus three exact original M1 source report roles.
    """

    def __init__(self, root, source_run, inputs):
        """Bind original approved sources through their existing independent normalizers.

        @param self Owned source view and open-atlas cache for this bounded review.
        @param root Explicit checkout; source producers must belong to this same checkout.
        @param source_run Explicit current source invocation, separate from native capture output.
        @param inputs Exact ground-packing and m1-source-vehicle/purchase/collector report identities.
        """
        if root.resolve() != ROOT:
            raise ValueError("Ground review sources belong to another checkout")
        identifier(source_run)
        self.root, self.source_run = root, source_run
        self.images, self.documents, self.sources = {}, {}, {}
        self.inputs = copy.deepcopy(inputs)
        context = RunContext(root, "multilayer-art", "ground-source-views", source_run)
        packed, _sheet, _declaration = packing_inputs("ground", inputs["ground-packing"], context)
        self.packed = packed
        self.scope, self.before, self.public = handoff.read_scope(current_sources=True)
        self.public, self.scope, self.before = bind_sources(self.public, self.scope, self.before,
                                                          context, packed["source_reviews"])
        recipes = {row["id"]: row for row in load_catalogue(root)["recipes"]}
        families = {row["asset_id"]: row for row in self.public["assets"]}
        aliases = {}
        # The three retained M1 families are existing current owners. Select
        # their original registered producer roles, not another similarly named
        # report or a dated manifest behind the canonical public alias.
        for asset, role in M1_ROLES.items():
            reference = inputs[role]
            path = contained(root / reference["path"], root / "mlm/reviews")
            parts = path.relative_to(root / "mlm/reviews").parts
            declaration = recipes[role]
            if (len(parts) != 5 or tuple(parts[:3]) != (declaration["feature"], declaration["scenario"], source_run)
                    or parts[3] != "reports" or identity(path, root) != reference):
                raise ValueError("Ground review selected another M1 producer/run/storage role")
            aliases[families[asset]["authoring"]["atlas_manifest"]] = reference
        m1_scope = json.loads(pack_m1_art.SCOPE.read_text(encoding="utf-8"))
        self.documents.update(pack_m1_art.selected_sources(self.public, dict(m1_scope, asset_ids=list(M1_ROLES)),
                                                         context, aliases))
        self.families = {row["asset_id"]: row for row in self.public["assets"]}
        self.allowed = set(self.scope["asset_ids"]) | set(M1_ROLES)
        self.facts = load_facts()
        references = [*self.inputs.values(), *packed["source_reviews"].values()]
        # Indirect current reports/images are actual inputs too. Preserve their
        # original identities across native drawing, not only their parent JSON.
        for row in self.scope["source_rows"]:
            references.extend([dict(path=row["manifest"], sha256=row["manifest_sha256"]),
                               dict(path=row["source_atlas"], sha256=row["source_sha256"])])
        for asset in M1_ROLES:
            authoring = self.families[asset]["authoring"]
            references.append(dict(path=authoring["native_atlas"], sha256=authoring["native_sha256"]))
        self.witnesses = {row["path"]: row for row in references}
        try:
            # Complete independent physical pose contracts remain prerequisites
            # even when the next bounded camera displays only a few families.
            for asset in sorted(self.allowed):
                self.poses(asset)
        except BaseException:
            self.close()
            raise

    def verify_current(self):
        """Close every direct/indirect original source identity after native drawing.

        @param self Current source view retaining the exact selected parent/child input identities.
        @return None; a changed generated source cannot inherit an earlier approved in-memory atlas.
        """
        # Cached decoded images are useful for all frames, but cannot conceal a
        # removed/replaced source file while a native observer was executing.
        for reference in self.witnesses.values():
            if identity(self.root / reference["path"], self.root) != reference:
                raise ValueError("Ground current source input changed during native review")

    def poses(self, asset):
        """Return original normalized source poses and their exact approved indexed atlas.

        @param self Current detached source/atlas owner; canonical source metadata stays untouched.
        @param asset One of the exact47Ground or3retained M1 source families.
        @return Original pose-key dictionary and owned immutable source image.
        """
        if asset not in self.allowed:
            raise ValueError("Ground viewport requested an unselected or later external source family")
        if asset not in self.sources:
            family = self.families[asset]
            if asset in M1_ROLES:
                self.sources[asset] = pack_m1_art.source_poses(family, self.documents, self.images)
            else:
                self.sources[asset] = handoff.source_poses(family, self.scope, self.before, self.documents, self.images)
                verify_poses(asset, self.sources[asset][0], self.facts)
        return self.sources[asset]

    def close(self):
        """Release only atlases opened by this explicit source view, including failure paths.

        @param self Current owned image cache; original files are never modified or removed.
        @return None; no source/expected pixel bytes are changed.
        """
        # Multiple families may share one source file; closing this owned cache
        # does not affect another producer/process or rewrite its original bytes.
        for image in self.images.values():
            image.close()
        self.images.clear()

    def __enter__(self):
        """Use this owned original source view for one bounded review.

        @param self Fully constructed source view.
        @return This same checked source/pose owner.
        """
        return self

    def __exit__(self, exception_type, exception, traceback):
        """Release atlas handles without hiding native/source/pixel failures.

        @param self Current bounded source view.
        @param exception_type Active exception type, or None after successful use.
        @param exception Active exception value, preserved for the caller.
        @param traceback Original failure traceback, preserved for the caller.
        @return False so every refusal still propagates unchanged.
        """
        self.close()
        return False
