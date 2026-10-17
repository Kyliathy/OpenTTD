"""Convert genuine natural-body originals and approved local surface parts.

@file convert_space_asteroids.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Groups14BF/14BFB/14BFC
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups67-69
Only retained original pixels, uniform BOX reduction, declared physical-part assembly
and the established DOS quantizer supply art. This module grants no runtime state.
"""
import argparse
import copy
from fractions import Fraction
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw
import convert_space_art as art

SIZES = ("small", "medium", "large")
DIRECTIONS = ("N", "NE", "E", "SE", "S", "SW", "W", "NW")
STATES = ("free", "attached", "mining", "depleted")
SCALE_DIVISORS = {"small": 4, "medium": 2, "large": 1}
FRAME_ALIASES = {"free": [0, 0, 0, 0], "attached": [0, 0, 0, 0],
                 "mining": [0, 1, 0, 1], "depleted": [0, 0, 0, 0]}


def digest(value):
    """Bind exact reviewed metadata independently of JSON formatting.

    @param value JSON-serializable original registration or physical-part policy.
    @return Stable SHA256 over sorted metadata.
    """
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode("utf-8")).hexdigest()


def size_signature(family):
    """Seal the actual saved enum and fixed body registration contract.

    @param family Original natural-body family definition.
    @return Hash covering canvas, anchor, saved sizes/diameters and honest frame aliases.
    """
    return digest({key: family[key] for key in ("public_native_size", "public_anchor", "sizes",
                   "visual_diameters_q16", "source_frame_contract", "native_art_envelope")})


def scaled_original(candidate, direction, size):
    """Derive a detached uniform saved-size registration from its sealed Large original.

    @param candidate Verified original with the approved Large transform; never modified.
    @param direction Actual original view.
    @param size Native saved Small/Medium/Large spelling, after family validation.
    @return Detached candidate whose only changed transform field is uniform scale.
    """
    result = copy.deepcopy(candidate)
    transform = result["native_transforms"][direction]
    ratio = Fraction(*transform["uniform_scale"]) / SCALE_DIVISORS[size]
    transform["uniform_scale"] = [ratio.numerator, ratio.denominator]
    return result


def surface_signature(candidate, direction, family, base):
    """Bind one own authored surface to its protected body and all three physical masks.

    @param candidate Retained local mining/depleted source with measured Large registration.
    @param direction Genuine body yaw present in both originals.
    @param family Own body family and exact per-size surface masks.
    @param base Selected immutable free original for that direction.
    @return Approval hash; shifted crops, phases, masks or substituted bodies invalidate it.
    """
    return digest(dict(original=candidate["sha256"], state=candidate["state"], phase=candidate["phase"],
                       transform=candidate["native_transforms"][direction],
                       base=art.registration_signature(base, direction), size_policy=size_signature(family),
                       masks={size: family["surface_masks"][size][direction] for size in SIZES}))


class AsteroidSources:
    """One detached, bounded source conversion; caches never outlive this exact document.

    Original Large registration is validated before a smaller detached transform is
    used. The source family has no physics, resource, lease or company authority.
    """

    def __init__(self, asset, document=None):
        """Freeze one explicit natural-body source definition for this conversion.

        @param asset One of the existing three public natural-body asset IDs.
        @param document Optional detached generation record for focused refusal checks.
        @throws ValueError On invented sizes, canvas, company masks, spin or unsealed policy.
        """
        self._document = copy.deepcopy(art.read_record() if document is None else document)
        if asset not in art.ASTEROID_ASSETS:
            raise ValueError("not a natural asteroid source family")
        self.asset = asset
        self._family = self._document["families"][asset]
        f = self._family
        # Existing saved size meanings and fixed world registration must survive art publication; aliases cannot invent a spin clock or duplicate a separately rendered native probe.
        if (f["public_native_size"] != [96, 80] or f["public_anchor"] != [-48, -48]
                or f["sizes"] != list(SIZES) or f["visual_diameters_q16"] != [32768, 65536, 131072]
                or f["directions"] != list(DIRECTIONS) or f["states"] != list(STATES)
                or f["frames"] != 4 or f["company_colors"] != "none"
                or f["source_frame_contract"] != FRAME_ALIASES):
            raise ValueError("natural asteroid size/frame/registration contract changed")
        if f.get("approved_size_policy") != size_signature(f):
            raise ValueError("unapproved asteroid saved-size policy")
        self._bodies = {}
        self._surfaces = {}

    def body(self, direction, size):
        """Read one actual body view at a saved size without scaling an indexed preview.

        @param direction One genuine approved original yaw.
        @param size Actual saved size spelling.
        @return Detached indexed pixels and measured original/size registration evidence.
        @throws ValueError On absent yaw/size or changed approved Large registration.
        """
        # A missing native size or view cannot silently alias a neighboring source.
        if direction not in DIRECTIONS or size not in SIZES:
            raise ValueError("missing native asteroid size or genuine direction")
        key = (direction, size)
        if key not in self._bodies:
            source_key = self._family["selections"]["free/" + direction]
            candidate = art.source_candidate(source_key, self._document)
            approved = self._family["approved_base_registration"].get(direction)
            if approved != art.registration_signature(candidate, direction):
                raise ValueError("unapproved asteroid Large original registration")
            # Large approval must pass against the original transform before deriving a detached smaller scale; approving the rescaled transform would weaken the frozen source boundary.
            large_key = (direction, "large")
            if large_key not in self._bodies:
                pixels, evidence = art.native_view(source_key, direction, self._document)
                self._bodies[large_key] = (pixels, dict(evidence, size="large", candidate=source_key))
            if size != "large":
                raw, evidence = art.source_pixels(scaled_original(candidate, direction, size), direction,
                                                  self._family["native_art_envelope"], (96, 80))
                self._bodies[key] = (art.base.quantize(raw, {}), dict(evidence, size=size, candidate=source_key))
        pixels, evidence = self._bodies[key]
        return pixels.copy(), copy.deepcopy(evidence)

    def surface(self, direction, size, state, phase):
        """Assemble only the approved own ore surface over unchanged original body pixels.

        @param direction Genuine body and local-part yaw.
        @param size Actual saved body size; all original parts receive the same uniform ratio.
        @param state Mining or depleted, without authority to select either in a game.
        @param phase Actual authored mining0/1 or static depleted0.
        @return Indexed assembled body and outside-part/opacity/readability evidence.
        @throws ValueError On substituted ancestry, changed masks, empty cues or silhouette loss.
        """
        # Only two original mining surface phases and one worked surface exist; accepting another phase would manufacture source coverage.
        if state not in ("mining", "depleted") or phase not in ((0, 1) if state == "mining" else (0,)):
            raise ValueError("unsupported original asteroid surface phase")
        cache_key = (direction, size, state, phase)
        if cache_key in self._surfaces:
            pixels, evidence = self._surfaces[cache_key]
            return pixels.copy(), copy.deepcopy(evidence)
        body, body_evidence = self.body(direction, size)
        candidate_key = self._family["surface_sources"][state][str(phase)][direction]
        candidate = art.source_candidate(candidate_key, self._document)
        base = art.source_candidate(body_evidence["candidate"], self._document)
        # Surface art belongs to this exact natural body and actual phase; another family's convincing ore patch cannot satisfy its source obligation.
        if (candidate["asset_id"] != self.asset or candidate.get("kind") != "authored_ore_surface_part"
                or candidate["state"] != state or candidate["phase"] != phase
                or art.identity(art.ROOT / base["path"]) not in candidate["references"]):
            raise ValueError("asteroid surface has different body/phase ancestry")
        signature = surface_signature(candidate, direction, self._family, base)
        if self._family["approved_surface_registration"][state][str(phase)].get(direction) != signature:
            raise ValueError("unapproved asteroid surface registration or physical mask")
        # Complete-view alpha/separation is checked by source_pixels before any declared ordinary part rectangle. Validate the sealed Large part before deriving smaller uniform registrations.
        large_key = (direction, "large", state, phase)
        if large_key not in self._surfaces and size != "large":
            self.surface(direction, "large", state, phase)
        source, registration = art.source_pixels(scaled_original(candidate, direction, size), direction,
                                                 self._family["native_art_envelope"], (96, 80))
        part = art.base.quantize(source, {})
        points = self._family["surface_masks"][size][direction]
        mask = {tuple(point) for point in points}
        # Physical patches are explicit, nonempty, unique and bounded inside existing stone; no background pixel may grow a new silhouette or erase the old one.
        if not mask or len(mask) != len(points) or any(not (0 <= x < 96 and 0 <= y < 80) for x, y in mask):
            raise ValueError("invalid asteroid physical surface mask")
        result = body.copy()
        # Only actual visible part pixels can overpaint existing stone. Transparent part margins leave the original untouched; they never erase stone or repair source alpha. All pixels outside the physical patch remain byte-identical.
        for x, y in mask:
            if body.getpixel((x, y)) == 0:
                raise ValueError("asteroid surface would grow original stone")
            if part.getpixel((x, y)) != 0:
                result.putpixel((x, y), part.getpixel((x, y)))
        changed = sum(a != b for a, b in zip(body.tobytes(), result.tobytes()))
        colors = result.convert("RGB")
        # Uniform Small reduction blends a gold chip with its silver/gray host. A retained warm red/green excess distinguishes that authored cue from the original cool stone without recoloring any pixel.
        amber = sum(red >= 140 and red - blue >= 40 and green - blue >= 20
                    for red, green, blue in (colors.getpixel(point) for point in mask))
        # A phase must communicate actual mining at every saved size. Missing authored pixels cannot be repaired by procedural recoloring or a decorative clock.
        if changed == 0 or (state == "mining" and amber == 0):
            raise ValueError("asteroid authored state loses its native visible cue")
        original_colors = body.convert("RGB")
        # Exhaustion must replace every original bright ore pixel in the physical seam, even if a part's transparent margin preserves adjacent ordinary stone.
        if state == "depleted" and any(min(original_colors.getpixel(point)) >= 205
                                       and min(colors.getpixel(point)) >= 205 for point in mask):
            raise ValueError("depleted asteroid retains pristine ore in its worked seam")
        if [v != 0 for v in result.tobytes()] != [v != 0 for v in body.tobytes()]:
            raise ValueError("asteroid original silhouette changed")
        evidence = dict(direction=direction, size=size, state=state, phase=phase,
                        base=body_evidence, surface=registration, candidate=candidate_key,
                        physical_mask=points, changed_pixels=changed, amber_pixels=amber,
                        protected_pixels=96 * 80 - len(mask), silhouette_preserved=True)
        self._surfaces[cache_key] = (result, evidence)
        return result.copy(), copy.deepcopy(evidence)

    def pose(self, direction, size, state, frame):
        """Resolve a public logical frame through honest static/two-phase source aliases.

        @param direction Genuine original yaw, never velocity-driven body rotation.
        @param size Native saved size spelling.
        @param state Public free/attached/mining/depleted source state.
        @param frame Public logical frame0..3; runtime selection remains with native owners.
        @return Detached indexed original pose and alias/registration evidence.
        """
        # The source declares exactly four logical frames; extra states or frames cannot imply additional hardware, spin or work authority.
        if state not in STATES or frame not in range(4):
            raise ValueError("missing public asteroid source pose")
        phase = FRAME_ALIASES[state][frame]
        if state in ("free", "attached"):
            pixels, evidence = self.body(direction, size)
        else:
            pixels, evidence = self.surface(direction, size, state, phase)
        return pixels, dict(evidence, state=state, logical_frame=frame, actual_phase=phase)


def derive(asset, output, document=None):
    """Write a complete source atlas and native/4x contacts without activating runtime art.

    @param asset Exact selected natural-body asset.
    @param output Fresh repository-local native-art proof directory.
    @param document Optional exact detached generation record for focused checks.
    @return Identity of the reproducible source manifest, with packing/runtime explicitly false.
    """
    sources = AsteroidSources(asset, document)
    output = output.resolve()
    # Immutable review evidence stays in the established proof home and never replaces an earlier accepted conversion or shipped payload.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("asteroid review requires a fresh native-art proof directory")
    output.mkdir(parents=True)
    atlas = Image.new("P", (96 * 8, 80 * 4 * 4 * 3), 0)
    atlas.putpalette(art.base.PALETTE_BYTES)
    entries, artifacts = [], []
    # Public order is saved size, genuine direction, state, logical frame. Atlas rows group size/state/frame so native packing can consume an explicit rectangle without inferring an alias.
    for size_index, size in enumerate(SIZES):
        for direction_index, direction in enumerate(DIRECTIONS):
            for state_index, state in enumerate(STATES):
                for frame in range(4):
                    pixels, evidence = sources.pose(direction, size, state, frame)
                    x, y = direction_index * 96, (size_index * 16 + state_index * 4 + frame) * 80
                    atlas.paste(pixels, (x, y))
                    entries.append(dict(size=size, variant=size_index, direction=direction, state=state,
                                        frame=frame, box=[x, y, x + 96, y + 80], anchor=[-48, -48],
                                        pixel_sha256=hashlib.sha256(pixels.tobytes()).hexdigest(), evidence=evidence))
        # One contact per actual source phase makes all three sizes independently reviewable, without repeating static logical aliases as invented artwork.
        for state, phase in (("free", 0), ("attached", 0), ("mining", 0), ("mining", 1), ("depleted", 0)):
            contact = Image.new("RGB", (384 * 8, 432), (38, 43, 49))
            drawing = ImageDraw.Draw(contact)
            for index, direction in enumerate(DIRECTIONS):
                pixels, _evidence = sources.pose(direction, size, state, phase)
                rgba = pixels.convert("RGBA")
                contact.paste(rgba, (index * 384 + 144, 20), rgba)
                contact.paste(rgba.resize((384, 320), Image.Resampling.NEAREST), (index * 384, 108),
                              rgba.resize((384, 320), Image.Resampling.NEAREST))
                drawing.text((index * 384 + 8, 4), f"{size} {direction} {state}/{phase}", fill=(235, 235, 235))
            name = f"{size}-{state}-{phase}-contact.png"
            contact.save(output / name)
            artifacts.append(art.identity(output / name))
    atlas.save(output / "native.png", transparency=0)
    artifacts.append(art.identity(output / "native.png"))
    # Snapshot only this body's selected originals and immutable prompt/input receipts. Later unrelated source families must not rewrite the ancestry of this dated conversion.
    keys = set(sources._family["selections"].values())
    keys.update(key for states in sources._family["surface_sources"].values()
                for views in states.values() for key in views.values())
    originals = [copy.deepcopy(art.source_candidate(key, sources._document)) for key in sorted(keys)]
    report = output / "space-source-manifest.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, schema_version=1, asset_id=asset,
                                     logical_pose_count=len(entries), complete_source_coverage=True,
                                     actual_component_pose_count=96, explicit_source_alias_count=288,
                                     alias_breakdown=dict(static_frame=216, unchanged_attachment=24, repeated_mining_phase=48),
                                     new_original_source_count=len(originals), registered_source_instance_count=0,
                                     family_definition=sources._family, source_candidates=originals,
                                     packed=False, runtime=False, entries=entries, artifacts=artifacts), indent=2)
                      + "\n", encoding="utf-8")
    return art.identity(report)


def audit_layout(report_path, output):
    """Measure complete original separation independently of selected part rectangles.

    @param report_path Exact reviewed source report with immutable candidate snapshots.
    @param output Fresh JSON evidence path beneath the repository logs directory.
    @return Identity of the audit; every model pixel must belong to a complete view.
    @throws ValueError On changed ancestry, separator intrusion, clipping or stray model pixels.
    """
    report = json.loads(report_path.read_text(encoding="utf-8"))
    output = output.resolve()
    # Layout evidence belongs to a fresh repository log; a repeated audit cannot overwrite the concrete original result already cited by a source consumer.
    if not output.is_relative_to(art.ROOT / "logs") or output.exists():
        raise ValueError("original layout audit requires fresh repository log evidence")
    originals = []
    # Whole originals are audited once, including the frame/gutters outside all selected rectangles. A clean local ore crop alone cannot hide an overlapping or clipped full subject.
    for candidate in report["source_candidates"]:
        image = Image.open(art.ROOT / candidate["path"]).convert("RGBA")
        if art.identity(art.ROOT / candidate["path"])["sha256"] != candidate["sha256"]:
            raise ValueError("asteroid layout original changed")
        coverage = Image.new("L", image.size)
        drawing = ImageDraw.Draw(coverage)
        views, boxes = [], []
        # Complete view rectangles must be disjoint before coverage is combined; overlapping windows cannot count the same subject pixels as two genuine source views.
        for direction, transform in candidate["native_transforms"].items():
            left, top, right, bottom = transform["crop_box"]
            if any(max(left, box[0]) < min(right, box[2]) and max(top, box[1]) < min(bottom, box[3]) for box in boxes):
                raise ValueError("asteroid original view rectangles overlap")
            boxes.append((left, top, right, bottom))
            crop = image.crop((left, top, right, bottom))
            bounds = crop.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
            clearance = min(bounds[0], bounds[1], crop.width - bounds[2], crop.height - bounds[3]) if bounds else 0
            separator_count = sum(art.source_separator(pixel, candidate.get("separator_color_policy"))
                                  for pixel in crop.get_flattened_data())
            # Every complete source view retains measured subject bounds and real transparent clearance; source separators never enter artwork even if a later part mask would hide them.
            if clearance < 16 or list(bounds) != transform["measured_subject_bbox_in_crop"] or separator_count:
                raise ValueError("asteroid whole original view fails separation")
            drawing.rectangle((left, top, right - 1, bottom - 1), fill=255)
            views.append(dict(direction=direction, crop_box=transform["crop_box"],
                              subject_bounds=list(bounds), minimum_alpha16_clearance=clearance,
                              visible_separator_pixels_inside=separator_count))
        outside = ImageChops.subtract(image.getchannel("A").point([0] * 16 + [255] * 240), coverage)
        count = sum(value != 0 and not art.source_separator(image.getpixel((index % image.width, index // image.width)),
                                                           candidate.get("separator_color_policy"))
                    for index, value in enumerate(outside.get_flattened_data()))
        if count:
            raise ValueError(f"asteroid original has {count} unregistered visible model pixels")
        originals.append(dict(key=candidate["key"], original=art.identity(art.ROOT / candidate["path"]),
                              receipt=candidate["generation_receipt"], views=views,
                              visible_nonseparator_pixels_outside=0, source_alpha_modified=False))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(dict(_documentation=__doc__, source_review=art.identity(report_path),
                                     originals=originals, alpha_threshold=16, accepted=True), indent=2)
                      + "\n", encoding="utf-8")
    return art.identity(output)


def publish(asset, report_path):
    """Publish reviewed source coverage through the existing public Sprite Contract.

    @param asset Exact natural-body family whose native contacts were reviewed.
    @param report_path Explicit complete conversion report; no latest-file discovery.
    @return Published source report identity; native packed/runtime fields remain pending.
    @throws ValueError On missing poses, changed original ancestry, artifacts or native pixels.
    """
    report_path = report_path.resolve()
    report = json.loads(report_path.read_text(encoding="utf-8"))
    # A source publication must bind this exact family and complete matrix; a partial review or packed/runtime claim cannot silently stand in for the later native integration gates.
    if (not report_path.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art")
            or report["asset_id"] != asset or report["logical_pose_count"] != 384
            or not report["complete_source_coverage"] or report["packed"] or report["runtime"]):
        raise ValueError("incomplete or mismatched asteroid source publication")
    # Every concrete atlas/contact remains the same artifact that was reviewed; metadata cannot authorize a replaced native image.
    for artifact in report["artifacts"]:
        if art.identity(art.ROOT / artifact["path"]) != artifact:
            raise ValueError("asteroid source artifact changed")
    atlas_record = next(row for row in report["artifacts"] if Path(row["path"]).name == "native.png")
    atlas = Image.open(art.ROOT / atlas_record["path"])
    expected = {(size, direction, state, frame) for size in SIZES for direction in DIRECTIONS
                for state in STATES for frame in range(4)}
    actual = set()
    # Decode every explicit public rectangle before publication; a plausible count or source label alone cannot cover an absent native pose.
    for row in report["entries"]:
        key = (row["size"], row["direction"], row["state"], row["frame"])
        left, top, right, bottom = row["box"]
        pixels = atlas.crop(row["box"])
        if (key in actual or key not in expected or (right - left, bottom - top) != (96, 80)
                or row["anchor"] != [-48, -48]
                or hashlib.sha256(pixels.tobytes()).hexdigest() != row["pixel_sha256"]):
            raise ValueError("invalid or duplicated asteroid source rectangle")
        actual.add(key)
    if actual != expected:
        raise ValueError("missing asteroid source pose")
    document = art.read_record()
    sources = AsteroidSources(asset, document)
    # Exact per-family original snapshots must agree with the current producer. A later substitution or approval change cannot borrow the earlier reviewed atlas.
    for candidate in report["source_candidates"]:
        if art.source_candidate(candidate["key"], document) != candidate:
            raise ValueError("asteroid source publication ancestry changed")
    family = document["families"][asset]
    if family != report["family_definition"]:
        raise ValueError("asteroid reviewed family definition changed")
    proof = art.identity(report_path)
    family.update(coverage_complete=True, complete_source_review=proof)
    art.RECORD.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    manifest_path = art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    row = next(row for row in manifest["assets"] if row["asset_id"] == asset)
    original = art.source_candidate(family["selections"]["free/N"], document)
    row["source"] = dict(status="authored", path=original["path"], sha256=original["sha256"],
                         provenance="first_party_original_builtin_image_gen", rights="first_party_generated_original")
    row["authoring"] = dict(date="2026-10-17", status="source_coverage_verified_packing_runtime_pending",
                            source_record=art.RECORD.relative_to(art.ROOT).as_posix(),
                            originals=[candidate["path"] for candidate in report["source_candidates"]],
                            atlas_manifest=proof["path"], atlas_manifest_sha256=proof["sha256"],
                            native_atlas=atlas_record["path"], native_sha256=atlas_record["sha256"],
                            staged_pose_count=384, required_pose_count=384, missing_pose_count=0,
                            coverage_complete=True, runtime_activated=False, actual_component_pose_count=96,
                            explicit_source_alias_count=288, new_original_source_count=len(report["source_candidates"]),
                            registered_source_instance_count=0, size_policy_sha256=size_signature(sources._family))
    row["runtime"] = dict(status="pending_packed_delivery_and_native_field_render_review",
                          consumer=dict(source="src/asteroid.h", symbol="AsteroidBody/AsteroidSize; read-only field selection14BG"))
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return proof


def main():
    """Convert one explicit asteroid source family into a fresh review directory."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asset", required=True, choices=art.ASTEROID_ASSETS)
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument("--output", type=Path)
    selection.add_argument("--publish", type=Path)
    args = parser.parse_args()
    print(json.dumps(publish(args.asset, args.publish) if args.publish else derive(args.asset, args.output)))


if __name__ == "__main__":
    main()
