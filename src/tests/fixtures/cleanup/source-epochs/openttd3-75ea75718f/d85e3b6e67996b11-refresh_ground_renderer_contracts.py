"""Refresh conventional anchor provenance after the isolated geothermal survey draw hook.

@file refresh_ground_renderer_contracts.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15CDB.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups24B/25B.
@see docs/zz-reach2/architecture/gui/archi-viewport.md.
This does not approve new station/terminal pixels. Current native geometry must
pass, all physical contracts must remain identical, and removing only the new
opt-in vent draw hook must reproduce the previously approved renderer hash.
"""

import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.ground_station_contract import ROOT, source_digest, station_contract, validate_station_contract
from tools.ground_terminal_contract import terminal_contract, validate_terminal_contract

PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
VIEWPORT = "src/viewport.cpp"
VENT_OVERLAY_BLOCK = (
    "\t// Natural vent art is a complete tile-aligned overlay, unlike the older\n"
    "\t// offset mineral icon. Both borrow the same actual placement owner and\n"
    "\t// cannot reveal fields, paid floor or quantities after its cursor ends.\n"
    "\tif (const auto vent = GetUndergroundGeothermalSurveyOverlaySprite(ti->tile)) DrawGroundSprite(*vent, PAL_NONE);\n"
)  # The only admitted renderer delta; an anchor or unrelated draw change refuses.


def identity(path):
    """Identify an actual repository proof input.

    @param path Existing file strictly inside this repository.
    @return Relative path, byte length and SHA256.
    """
    path = Path(path).resolve()
    relative = path.relative_to(ROOT)
    return {"path": relative.as_posix(), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def prepare(native_report, binary):
    """Prepare a complete metadata-only candidate before any publication.

    @param native_report Successful current generator-operation geometry report.
    @param binary Exact current executable identified by that report.
    @return Original/candidate public documents and a bounded refresh receipt.
    @throws ValueError For stale proof, changed physical contracts or any renderer delta beyond the isolated vent hook.
    """
    report_path = Path(native_report).resolve()
    report = json.loads(report_path.read_text(encoding="utf-8"))
    # The refresh borrows successful actual geometry from the same current
    # executable, rather than blessing hashes copied from an earlier build.
    if report.get("phase") != "geometry" or report.get("binary") != identity(binary):
        raise ValueError("Renderer provenance requires the exact current native geometry executable")
    selected = {proof["producer"]: proof for proof in report["proofs"]}
    for name, counts in (("station", (4, 3355)), ("terminal", (5, 4850))):
        proof = selected.get(name)
        # Failed/partial evidence cannot authorize either source-contract row.
        if proof is None or (proof.get("cases"), proof.get("assertions")) != counts or proof.get("exit_code") != 0:
            raise ValueError("Incomplete native renderer geometry: " + name)
        log_path = ROOT / proof["log"]["path"]
        if identity(log_path) != proof["log"] or not log_path.read_text(encoding="utf-8-sig").rstrip().endswith("NATIVE_EXIT_CODE=0"):
            raise ValueError("Changed or unsuccessful native renderer proof: " + name)
    renderer = (ROOT / VIEWPORT).read_text(encoding="utf-8")
    if renderer.count(VENT_OVERLAY_BLOCK) != 1:
        raise ValueError("The isolated geothermal overlay delta is absent or ambiguous")
    previous_hash = hashlib.sha256(renderer.replace(VENT_OVERLAY_BLOCK, "", 1).encode("utf-8")).hexdigest()
    current_hash = source_digest(ROOT / VIEWPORT)
    original = json.loads(PUBLIC.read_text(encoding="utf-8"))
    candidate = copy.deepcopy(original)
    changed = []
    # Both authoritative contract builders validate their compiled native
    # descriptors. Only their renderer provenance may change; source pixels,
    # ranges, anchors, physical parents and gameplay ownership stay identical.
    for asset, build, validate in (("facility.rail-station", station_contract, validate_station_contract),
                                   ("facility.road-terminal", terminal_contract, validate_terminal_contract)):
        row = next(item for item in candidate["assets"] if item["asset_id"] == asset)
        expected = build()
        for key, value in expected.items():
            if key == "native_anchor_renderer_sha256":
                continue
            if row.get(key) != value:
                raise ValueError("Physical source contract changed: " + asset + "/" + key)
        hashes = row["native_anchor_renderer_sha256"]
        if set(hashes) != set(expected["native_anchor_renderer_sha256"]):
            raise ValueError("The declared anchor-renderer set changed: " + asset)
        # Every other renderer remains the exact approved original. The
        # viewport either already has this idempotent refresh or must equal
        # the full historical source with just our separate survey hook absent.
        for name, value in expected["native_anchor_renderer_sha256"].items():
            if name == VIEWPORT:
                if hashes.get(name) not in (previous_hash, current_hash):
                    raise ValueError("An unrelated viewport/anchor edit requires its own render acceptance")
            elif hashes.get(name) != value:
                raise ValueError("Another anchor renderer changed: " + name)
        if hashes != expected["native_anchor_renderer_sha256"]:
            row["native_anchor_renderer_sha256"] = expected["native_anchor_renderer_sha256"]
            changed.append(asset)
        validate(row)
    return original, candidate, {
        "_documentation": "r2mlvs18-geothermal-energy-and-industry.md Group15CDB; r2mlvs14-remaining-assets-and-runtime-integration.md Groups24B/25B; archi-viewport.md; scoped renderer-file provenance, not new conventional artwork acceptance",
        "native_report": identity(report_path), "binary": identity(binary), "renderer": VIEWPORT,
        "previous_renderer_sha256": previous_hash, "current_renderer_sha256": current_hash,
        "changed_assets": changed, "physical_contracts_unchanged": True,
        "delta": "Only the explicit same-owner opt-in geothermal survey DrawGroundSprite hook; every other renderer source byte is preserved",
    }


def main():
    """Publish one fully prepared public provenance update and print its receipt.

    @return None; no native/source/payload file is regenerated by this helper.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--geometry-report", type=Path, required=True)
    parser.add_argument("--binary", type=Path, required=True)
    args = parser.parse_args()
    original, candidate, receipt = prepare(args.geometry_report, args.binary)
    if candidate != original:
        PUBLIC.write_text(json.dumps(candidate, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
