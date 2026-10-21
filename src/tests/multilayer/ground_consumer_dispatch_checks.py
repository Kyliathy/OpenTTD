"""Audit exact selected-family dispatch and real native export locations without behavior claims.

@file ground_consumer_dispatch_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79F.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Rebuild the read-only dispatch receipt under logs/graphics/selected-ground-consumers.
Native signature compilation and producer execution remain each dispatched group's gate.
"""

import argparse
from collections import Counter
import json
from pathlib import Path
import re
import subprocess

from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import checked_reference, reference

DISPATCH = ROOT / "media/multilayer/contracts/ground-consumer-dispatch.json"


def audit():
    """Verify all50 public selected shapes, native ranges and actual producer/draw/test locations.

    @return Read-only exact contract/export inventory; runtime behavior is explicitly unqualified here.
    """
    scope, before, public = handoff.read_scope()
    contract = handoff.read_json(DISPATCH.relative_to(ROOT).as_posix())
    checked_reference(contract["source_scope"])
    checked_reference(contract["packed_handoff"])
    rows = contract["families"]
    if [row["asset_id"] for row in rows] != scope["selected_asset_ids"]:
        raise ValueError("ground consumer dispatch omits or duplicates a selected family")
    counts = Counter(row["dispatch"] for row in rows)
    # Current implementation groups may promote only their original21 pending
    # consumers. Three retained sources and26 later-owned producers keep their
    # original disposition; an art binding never closes a later gameplay gate.
    if (set(counts) - {"current_unbound", "current_bound", "current_existing", "later_owner"}
            or counts["current_unbound"] + counts["current_bound"] != 21
            or counts["current_existing"] != 3 or counts["later_owner"] != 26):
        raise ValueError("ground current/later consumer boundary changed without dispatch revision")
    current = {row["asset_id"]: row for row in public["assets"]}
    files, tags, pose_count = {}, set(), 0
    # Every selected source row retains its actual public Cartesian geometry
    # and packed native identity. Complete art cannot silently promote a later
    # tube/generator/control producer to a delivered current consumer.
    for row in rows:
        family = current[row["asset_id"]]
        for key in ("consumer", "states", "frames", "directions", "tile_variants", "parts"):
            if row[key] != family[key]:
                raise ValueError("ground consumer source geometry changed: " + row["asset_id"])
        for key in ("native_offset", "sprite_count"):
            if row[key] != family["packed"][key]:
                raise ValueError("ground consumer native range changed: " + row["asset_id"])
        pose_count += row["sprite_count"]
        owner = handoff.bound(row["owner"])
        files[owner] = reference(owner)
        text = owner.read_text(encoding="utf-8")
        for symbol in row["symbols"]:
            if re.search(r"\b" + re.escape(symbol) + r"\s*\(", text) is None:
                raise ValueError("ground consumer's actual public export is absent: " + symbol)
        if "draw_owner" in row:
            draw = handoff.bound(row["draw_owner"])
            files[draw] = reference(draw)
            if row["draw_symbol"] not in draw.read_text(encoding="utf-8"):
                raise ValueError("ground actual draw consumer is absent: " + row["draw_symbol"])
        tags.update(row.get("producer_tests", []))
        if row["dispatch"] == "current_bound":
            qualification = checked_reference(row["binding_qualification"])
            accepted = handoff.read_json(qualification.relative_to(ROOT).as_posix())
            if accepted.get("passed") is not True or row["asset_id"] not in accepted["families"]:
                raise ValueError("ground current binding lacks its exact native family qualification")
            header = handoff.bound(row["binding_header"])
            if row["binding_symbol"] not in header.read_text(encoding="utf-8"):
                raise ValueError("ground declared native binding export is absent")
            files[header] = reference(header)
    if pose_count != 15488:
        raise ValueError("ground selected native pose coverage differs from the source closure")
    # Locate only the named current producer fixtures with rg. This records
    # real executable entry points without loading unrelated historical suites
    # or confusing an exported symbol with a passed behavior qualification.
    command = ["rg", "-l", "-F", "-g", "*.cpp"]
    for tag in sorted(tags):
        command.extend(("-e", tag))
    result = subprocess.run(command + ["src/tests"], cwd=ROOT, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        raise ValueError("ground named producer test entry points were not found")
    discovered = {}
    for relative in result.stdout.splitlines():
        path = handoff.bound(relative)
        text = path.read_text(encoding="utf-8")
        discovered[path] = {tag for tag in tags if tag in text}
        files[path] = reference(path)
    if set().union(*discovered.values()) != tags:
        raise ValueError("ground a declared producer test tag is absent")
    return {"_documentation": __doc__, "passed": True, "contract": reference(DISPATCH),
            "source_scope": contract["source_scope"], "packed_handoff": contract["packed_handoff"],
            "selected_families": len(rows), "selected_poses": pose_count, "dispatch_counts": dict(counts),
            "source_exports_and_fixtures": list(files.values()), "producer_tags": sorted(tags),
            "signature_compilation_claimed": False, "producer_behavior_verified": False, "runtime_activated": False}


def main():
    """Write reproducible read-only dispatch evidence into an explicit repository logs run.

    @return None; this audit marks no consumer selection or native behavior complete.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", required=True, help="Repository logs audit receipt")
    args = parser.parse_args()
    path = handoff.bound(args.report)
    if not path.is_relative_to(ROOT / "logs"):
        raise ValueError("ground consumer audit belongs in repository logs")
    report = audit()
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: report[key] for key in ("passed", "selected_families", "selected_poses", "dispatch_counts")}))


if __name__ == "__main__":
    main()
