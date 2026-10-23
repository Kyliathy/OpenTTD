"""Verify a named selected gallery's published files, offline links and complete native canvases.

@file selected_gallery_delivery_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84ED.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Existing evidence, activation and gallery owners retain all source/range/consumer authority.
"""

import argparse
from html.parser import HTMLParser
import json
from pathlib import Path
import sys
from urllib.parse import unquote, urlsplit

from PIL import Image

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import activate_m1_art as activation
import review_m1_art as review
from tools import selected_gallery as gallery


class OfflineLinks(HTMLParser):
    """Collect actual navigation/image targets without executing gallery markup.

    @see Plan14-84ED.2; every local target must remain inside the checkout and exist.
    """

    def __init__(self):
        """Initialize an independent gallery link collection.

        @param self New parser for one exact published HTML artifact.
        @return None; parsing cannot access or modify linked artifacts.
        """
        super().__init__()
        self.targets = set()

    def handle_starttag(self, tag, attrs):
        """Retain explicit image and navigation attributes from parsed markup.

        @param self Current gallery parser.
        @param tag HTML element name supplied by the standard parser.
        @param attrs Parsed attribute name/value pairs, including optional values.
        @return None; duplicate targets remain one offline proof obligation.
        """
        # Both navigation and embedded previews must survive cleanup; captions
        # and styling attributes do not identify observation artifacts.
        for key, value in attrs:
            if key in ("href", "src"):
                if value:
                    self.targets.add(value)


def verify_links(path):
    """Require every published HTML link to resolve to a contained local artifact.

    @param path Exact published HTML file, already bound by the gallery record.
    @return Number of distinct successfully resolved offline targets.
    """
    links = OfflineLinks()
    links.feed(path.read_text(encoding="utf-8"))
    # Offline review must remain usable with retained PNG/SAV/JSON evidence;
    # an external URL or an escaping path cannot replace an absent local file.
    for name in links.targets:
        url = urlsplit(name)
        assert not url.scheme and not url.netloc, "selected gallery contains a nonlocal link"
        target = (path.parent / unquote(url.path)).resolve()
        assert target.is_relative_to(ROOT) and target.is_file(), "missing/escaping offline gallery target: " + name
    return len(links.targets)


def verify_canvases(record):
    """Compare each complete representative native canvas with its published sheet pixels.

    @param record Exact accepted gallery with existing family/frame mappings and sheet identity.
    @return Count and dimensions of complete unscaled native scenes on the sheet.
    """
    scenes = {}
    # Shared original frames represent multiple actual families. The published
    # three-column overview must preserve each chosen full canvas once, with
    # captions in separate padding rather than cropped or rescaled evidence.
    for row in record["families"]:
        if row["review_kind"] != "current_selected_native":
            continue
        preferred = [name for name in row["frames"] if "zoom-1-0" in name]
        name = (preferred or row["frames"])[0]
        scenes.setdefault(name, Image.open(ROOT / name).convert("RGB"))
    width = max(frame.width for frame in scenes.values())
    height = max(frame.height for frame in scenes.values()) + 64
    sheet = Image.open(ROOT / record["contact_sheet"]["path"]).convert("RGB")
    assert sheet.size == (3 * width, ((len(scenes) + 2) // 3) * height)
    # Check every source pixel, including background and occlusion, so a
    # visually plausible montage cannot hide clipped native canvas edges.
    for index, frame in enumerate(scenes.values()):
        x, y = (index % 3) * width, (index // 3) * height + 64
        observed = sheet.crop((x, y, x + frame.width, y + frame.height))
        assert observed.tobytes() == frame.tobytes(), "contact sheet changed a complete native canvas"
    return {"scenes": len(scenes), "size": list(sheet.size), "complete_rgb_exact": True}


def accept(evidence_manifest, check_logs, output):
    """Seal exact current publication after existing-owner and offline artifact checks.

    @param evidence_manifest Explicit immutable named selection, distinct from the mutable default.
    @param check_logs Exact focused check logs retained beside this review's diagnostics.
    @param output Absent receipt beneath repository logs; no source/runtime metadata is written.
    @return None; missing/stale source, frame, ancestor, publication or offline artifact refuses.
    """
    selection = activation.select_evidence(evidence_manifest)
    assert selection.path != activation.DEFAULT_MANIFEST.resolve(), "select the immutable named delivery"
    assert selection.path.read_bytes() == activation.DEFAULT_MANIFEST.read_bytes(), "default selection differs from named publication"
    output = (ROOT / output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    review.configure(selection)
    public, packed = gallery.read(activation.PUBLIC), gallery.read(activation.PACKED)
    bindings = activation.validate_binding_review(public, packed)
    rows, frames, summary = gallery.validate(public, packed, {"packed_payload": packed["packed_grf"], "bindings": bindings}, selection)
    record = gallery.read(selection.output("gallery"))
    assert record["families"] == rows and record["frames"] == frames
    # Recompute summary fields as well as visible frames: refreshed independent
    # Forge processes must not inherit the first gallery's duplicate-run count.
    for key, value in summary.items():
        assert record[key] == value, "published gallery summary differs: " + key
    cache = {}
    # The renderer/adapter that accepted these exact artifacts are part of the
    # proof; changing validation policy requires a fresh generated gallery.
    for role in ("html", "contact_sheet", "gallery_owner", "selected_adapter"):
        gallery.checked(record[role], cache)
    assert record["technical_acceptance"] == "verified"
    runtime = gallery.read(selection.output("runtime"))
    assert runtime["gallery_manifest"] == selection.output("gallery").relative_to(ROOT).as_posix()
    assert runtime["selected_native_closures"] == summary["closures"]
    assert runtime["active_consumers"] == 65 and not runtime["whole_profile_activated"]
    result = {
        "_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84ED; exact selected public/runtime/gallery technical delivery;human/network/operator/future-profile gates remain pending",
        "controller": activation.evidence_identity(Path(__file__)),
        "selection": activation.evidence_identity(selection.path),
        "default_selection": activation.evidence_identity(activation.DEFAULT_MANIFEST),
        "public_manifest": activation.evidence_identity(activation.PUBLIC),
        "packed_manifest": activation.evidence_identity(activation.PACKED),
        "runtime_manifest": activation.evidence_identity(selection.output("runtime")),
        "gallery": activation.evidence_identity(selection.output("gallery")),
        "html": record["html"], "contact_sheet": record["contact_sheet"],
        "summary": summary, "native_frames": len(frames),
        "offline_targets": verify_links(ROOT / record["html"]["path"]),
        "native_canvases": verify_canvases(record),
        "focused_checks": [activation.evidence_identity((ROOT / name).resolve()) for name in check_logs],
        "technical_acceptance": "verified", "user_feedback": "deferred",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": activation.evidence_identity(output), "offline_targets": result["offline_targets"], "native_canvases": result["native_canvases"]}), flush=True)


def main():
    """Select one exact current named delivery and absent retained verification output.

    @return None; this command is read-only except for the requested receipt.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence-manifest", type=Path, required=True)
    parser.add_argument("--checks", nargs="+", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.evidence_manifest, args.checks, args.output)


if __name__ == "__main__":
    main()
