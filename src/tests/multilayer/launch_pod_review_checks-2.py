"""Qualify actual paid passive-pod captures and atomic cargo recovery.

@file launch_pod_review_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JDC.2.
Only explicit original paid snapshots and current native observations qualify.
Contact images retain native pixels; --reviewed records their actual inspection.
"""
import argparse
import json
from pathlib import Path
import shutil
from PIL import Image, ImageDraw
from support.artifact_paths import create_run_context
import launch_facility_selector_checks as selectors

ROOT = selectors.ROOT
OUTPUT = ROOT / "logs/proofs/mlm/native-art/space/launch-pod-review-v1"
CASES = ("release", "before-first", "first", "before-second", "second", "recover")
CONFIGURATIONS = ("Debug", "RelWithDebInfo")
LIVE = {"release": ((0, 1, 111), (1, 2, 148)),
        "before-first": ((0, 1, 1), (1, 2, 38)),
        "first": ((1, 2, 37),), "before-second": ((1, 2, 1),),
        "second": (), "recover": ((0, 1, 1), (1, 2, 38))}


def checked(reference):
    """Bind one observation to exact retained repository bytes.

    @param reference Explicit repository-relative path and SHA256.
    @return Original reference after checking its bytes.
    @throws ValueError When evidence is missing, substituted or stale.
    """
    if selectors.art.source.identity(ROOT / reference["path"]) != reference:
        raise ValueError("stale passive recovery evidence: " + reference["path"])
    return reference


def verify(configuration):
    """Verify original shell/packet identities and their native source selections.

    @param configuration Actual isolated native build owning the six observations.
    @return Ordered proof and observation pairs with exact current inputs.
    @throws ValueError On changed custody, invented progress, stale input or duplicate shells.
    """
    records = []
    index = selectors.art.read_json(ROOT / "logs/descent-custody-verification.json")
    # Independent original phase expectations preserve two distinct consumed
    # shells. Art cannot replace either identity or manufacture inventory stock.
    for case in CASES:
        path = OUTPUT / (configuration + "-" + case + ".json")
        proof = selectors.art.read_json(path)
        for reference in (proof["application"], proof["source"], proof["qualification"],
                          proof["index"], proof["terminal"], proof["observation"],
                          *proof["sources"], *proof["images"]):
            checked(reference)
        # Both accepted provenance and pure observation are needed before the
        # ordinary game can establish a paid, unmodified passive-pod presentation.
        if (not proof["complete"] or proof["source_pose_grants"]
                or proof["source"] not in index["artifacts"]
                or proof["qualification"] not in index["reports"]):
            raise ValueError("passive review lacks original earned qualification")
        native = selectors.art.read_json(ROOT / proof["observation"]["path"])
        if native["source_pose_grants"] or native["capture_only_ticks"]:
            raise ValueError("passive observation assigned work or source state")
        active = tuple((row["pod"], row["generation"], row["descent"]["remaining"])
                       for row in native["before"] if row["drawable_space_only"])
        if active != LIVE[case]:
            raise ValueError("original paid shell/countdown identities changed")
        # Original stored inventory retains all eight paid unused shell
        # incarnations and contributes no free-standing Space/Surface sprite.
        stored = tuple((row["pod"], row["generation"], row["custody"])
                       for row in native["before"] if not row["drawable_space_only"])
        if stored != tuple((pod, pod + 1, 1) for pod in range(2, 10)):
            raise ValueError("stored passive shell ownership changed")
        # The native release point stays fixed. Only actual remaining work
        # chooses authored descent/recovery-ready frames; there is no art clock.
        for row in native["before"]:
            if not row["drawable_space_only"]:
                continue
            job = row["descent"]
            expected_state = 2 if job["remaining"] <= 1 else 1
            expected_frame = 1 if expected_state == 2 else (job["remaining"] // 8) % 2
            if (row["custody"] != 5 or row["position"] != job["release_point"]
                    or job["packets"] != 10 or job["quantity"] != 10
                    or job["cargo"] != 16 or job["center"] != 4
                    or job["center_generation"] != 2 or job["blocked"] != 0
                    or row["art"]["state"] != expected_state
                    or row["art"]["frame"] != expected_frame):
                raise ValueError("native pod art differs from original packet/work owners")
        if case == "recover":
            settlement = native["recovery"]
            expected = dict(assigned_work_or_cargo=False, deleted_exact_shell=True,
                            earned_ticks=1, quantity=10, spent_generation=1,
                            spent_pod=0, stale_selection_empty=True,
                            station_after=10, station_before=0)
            if settlement != expected:
                raise ValueError("native atomic packet settlement changed")
            if any(row["pod"] == 0 for row in native["after"]):
                raise ValueError("spent original shell remains in the native pool")
        # Snapshot capture cannot advance work. The separate recovery case is
        # the sole normal earned opportunity and is qualified explicitly above.
        elif native["before"] != native["after"] or native["recovery"] is not None:
            raise ValueError("passive snapshot observation advanced native owners")
        records.append((case, path, proof, native))
    return records


def contacts(configuration, records):
    """Build contact sheets from the actual captured viewport and public widget.

    @param configuration Native build label for these retained review images.
    @param records Fully verified original-case observations.
    @return Native contact image identities under the feature review directory.
    """
    context = create_run_context("space", "launch-pod-review-contact", root=ROOT,
                                 profile="launch-fleet-art", process=configuration.lower())
    sheet = Image.new("RGB", (1280, 6 * 390), "#252525")
    draw = ImageDraw.Draw(sheet)
    # Show each genuine native phase beside the actual selected-shell inventory;
    # fixed-size camera crops preserve the source/native pixels without repainting.
    for index, (case, _, proof, _) in enumerate(records):
        directory = (ROOT / proof["observation"]["path"]).parents[1] / "images"
        for column, name in enumerate(("space-before.png", "pod-selected-before.png")):
            with Image.open(directory / name) as pixels:
                sheet.paste(pixels, (column * 640, index * 390 + 22))
            draw.text((column * 640 + 4, index * 390 + 4), case + " / " + name, fill="white")
    output = context.images / (configuration + "-pod-phases.png")
    sheet.save(output)
    recovery = next(proof for case, _, proof, _ in records if case == "recover")
    directory = (ROOT / recovery["observation"]["path"]).parents[1] / "images"
    sheet = Image.new("RGB", (1280, 780), "#252525")
    draw = ImageDraw.Draw(sheet)
    # Compare the same open native window across actual shell consumption, plus
    # original Surface placement. This cannot substitute a persistent recovered asset.
    for index, name in enumerate(("surface-before.png", "surface-after.png",
                                  "pod-selected-before.png", "pod-spent-after.png")):
        with Image.open(directory / name) as pixels:
            sheet.paste(pixels, (index % 2 * 640, index // 2 * 390 + 22))
        draw.text((index % 2 * 640 + 4, index // 2 * 390 + 4), name, fill="white")
    settlement = context.images / (configuration + "-pod-settlement.png")
    sheet.save(settlement)
    return [selectors.art.source.identity(path) for path in (output, settlement)]


def qualify(reviewed=False):
    """Bind both actual native corpora and optionally close their visual review.

    @param reviewed Whether the generated native image sheets have actually been inspected.
    @return Receipt preserving exact original paid sources and executing applications.
    @throws ValueError On mismatched builds, provenance, pixels or unqualified inputs.
    """
    selectors.boundary()
    corpora = [verify(configuration) for configuration in CONFIGURATIONS]
    ppm_pairs = indexed_pairs = 0
    # Counterpart worlds and native windows must agree byte for byte between
    # builds, independently of their distinct actual executing application hashes.
    for left, right in zip(*corpora, strict=True):
        if left[3] != right[3] or left[2]["source"] != right[2]["source"]:
            raise ValueError("counterpart native passive owners differ")
        images = [{Path(row["path"]).name: row["sha256"] for row in item[2]["images"]
                   if Path(row["path"]).suffix in (".ppm", ".indexed")} for item in (left, right)]
        if images[0] != images[1]:
            raise ValueError("counterpart passive viewport/window pixels differ")
        ppm_pairs += sum(name.endswith(".ppm") for name in images[0])
        indexed_pairs += sum(name.endswith(".indexed") for name in images[0])
    reviews = []
    for configuration, records in zip(CONFIGURATIONS, corpora, strict=True):
        images = contacts(configuration, records)
        binary = records[0][2]["application"]
        if any(record[2]["application"] != binary for record in records):
            raise ValueError("passive cases executed different native applications")
        # Retain the real app used by every case before later isolated-target
        # extensions change its build path. Historical receipts remain verifiable.
        retained = OUTPUT / ("qualified-pod-review-" + configuration + ".exe")
        shutil.copyfile(ROOT / binary["path"], retained)
        reviews.append(dict(configuration=configuration, assertions=sum(row[2]["assertions"] for row in records),
                            cases=len(records), retained_application=selectors.art.source.identity(retained),
                            proofs=[selectors.art.source.identity(row[1]) for row in records], review_images=images))
    result = dict(_documentation=__doc__, complete=reviewed, visual_review_complete=reviewed,
                  configurations=reviews, exact_ppm_counterparts=ppm_pairs,
                  exact_indexed_counterparts=indexed_pairs, original_paid_sources=True,
                  source_pose_grants=False, earned_recovery_ticks_per_configuration=1,
                  persistent_spent_shell=False, stored_shells_free_visible=False,
                  verifier=selectors.art.source.identity(Path(__file__).resolve()))
    name = "verification.json" if reviewed else "preview.json"
    (OUTPUT / name).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(name, [(row["configuration"], row["assertions"], row["cases"]) for row in reviews],
          "exact native counterparts", ppm_pairs, indexed_pairs, flush=True)
    return result


def main():
    """Verify current native passive review, then record actual image inspection."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reviewed", action="store_true")
    qualify(parser.parse_args().reviewed)


if __name__ == "__main__":
    main()
