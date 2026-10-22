"""Retain full original pixel expectations for actual launch/fleet and prior native loading.

@file launch_fleet_payload_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group18; explicit current source expectations.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62I.2.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
The fixture contains exact source index bytes and anchors, never a GRF decoder.
It preserves existing M0/M1/Space/orbital expectations and adds1648qualified poses.
"""

import hashlib
import json
from pathlib import Path
import struct
import sys

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT/"src/tests"))
sys.path.insert(0, str(ROOT/"media/multilayer"))
from multilayer_packed_art_fixture import pixel_hash, write_selected_payload
import pack_launch_fleet_art as packing
from tools import cargo_rocket_art as art
from tools import launch_fleet_handoff as handoff

OUTPUT = ROOT/"logs/proofs/mlm/native-art/expected/launch-fleet-packed-sprites.bin"
SUMMARY = ROOT/"logs/proofs/mlm/native-art/space/launch-fleet-packing-v1/full-pixel-expectations.json"


def source_cells(stem):
    """Read original published source rectangles without interpreting any packed GRF.

    @param stem Exact protected native PNG/NFO source basename.
    @return Ordered source width/height/anchor/index-byte records in unchanged NFO order.
    @throws ValueError On a malformed rectangle or non-indexed original source.
    """
    directory = ROOT/"media/baseset/openttd"
    with Image.open(directory/(stem+".png")) as image:
        if image.mode != "P":
            raise ValueError("protected native source is not indexed: "+stem)
        rows = [line.split() for line in (directory/(stem+".nfo")).read_text(encoding="utf-8").splitlines()
                if "sprites/"+stem+".png 8bpp" in line]
        result = []
        # The protected NFO owns actual source crops and contacts. Source
        # pixels, including transparent holes, remain independent of the loader.
        for row in rows:
            x, y, width, height, ax, ay = map(int, row[3:9])
            if not (0 <= x < x+width <= image.width and 0 <= y < y+height <= image.height):
                raise ValueError("protected native source cell leaves its image")
            result.append((width, height, ax, ay, image.crop((x, y, x+width, y+height)).tobytes()))
    return result


def protected_records(name, magic, header_size, count, stems, path=None):
    """Match unchanged historical expected geometry/hashes to full protected source bytes.

    @param name Existing retained binary fixture basename, never regenerated here.
    @param magic Exact preceding producer's eight-byte fixture version.
    @param header_size Preceding producer's actual fixed header byte count.
    @param count Exact previously qualified published sprite count.
    @param stems Protected source sheets in that historical fixture's native order.
    @param path Explicit selected source-derived prefix; None retains historical authoring lookup.
    @return Full prior native offset/geometry/anchor/hash/pixel-byte records.
    @throws ValueError On lost historical expectations or any altered prior source pixel.
    """
    path = OUTPUT.parent/(name+"-packed-sprites.bin") if path is None else path
    payload = path.read_bytes()
    cells = [cell for stem in stems for cell in source_cells(stem)]
    record = struct.Struct("<HHHhhI")
    # Historical expected records are preserved verbatim. A freshly rendered
    # atlas cannot silently redefine any accepted old pixel/contact expectation.
    if (payload[:8] != magic or struct.unpack_from("<H", payload, 8)[0] != count
            or len(payload) != header_size+count*record.size or len(cells) != count):
        raise ValueError("protected native fixture coverage/version changed: "+name)
    result = []
    for index, cell in enumerate(cells):
        offset, width, height, ax, ay, digest = record.unpack_from(payload, header_size+index*record.size)
        sw, sh, sx, sy, pixels = cell
        # Source dimensions, world anchor and complete indexed material are
        # compared against the actual retained prior executable-test expectations.
        if (cell[:4] != (width, height, ax, ay) or len(pixels) != width*height or pixel_hash(pixels) != digest):
            raise ValueError("protected original source/expectation changed: "+name+"/"+str(offset))
        result.append((offset, sw, sh, sx, sy, digest, pixels))
    return result


def produce(report=None, image_path=None, destination=None, prefixes=None):
    """Write reproducible full-byte expectations for6116old and1648new native identities.

    @param report Explicit current packing report; None selects historical authoring only.
    @param image_path Selected indexed sheet; required together with report/destination/prefixes.
    @param destination Current binary data output; never a canonical GRF destination.
    @param prefixes Actual selected m1/space/orbital source expectation binary paths.
    @return Full expected binary facts and independently retained source/proof bindings.
    @throws ValueError On any source, native identity, packed-crop or historical fixture drift.
    """
    selected = any(value is not None for value in (report, image_path, destination, prefixes))
    # Half-selected inputs must never reopen old authoring paths. All four
    # explicit roles form one current source-only operation, independent of decoding.
    if selected and any(value is None for value in (report, image_path, destination, prefixes)):
        raise ValueError("launch expectations require complete explicit current inputs/output")
    if selected:
        if set(prefixes) != {"m1", "space", "orbital"}:
            raise ValueError("launch expectations require exact source prefix families")
        scope, accepted = packing.read_scope(current_sources=True)
        files = {}
    else:
        scope, accepted = packing.read_scope()
        report, files = packing.derive()
    # The native process must observe the exact staged PNG/NFO/header, rather
    # than an unpublished in-memory derivation or a neighboring source atlas.
    for path, data in files.items():
        if path.read_bytes() != data:
            raise ValueError("stage exact qualified launch/fleet inputs before full-pixel expectations")
    old = protected_records("m1", b"MLVART1\0", 12, 3561, ("map_layers", "map_layers_m1"), None if not selected else prefixes["m1"])
    old += protected_records("space", b"SPCART1\0", 14, 1579, ("space_preview", "space_final"), None if not selected else prefixes["space"])
    old += protected_records("orbital", b"ORBART1\0", 14, 976, ("orbital_final",), None if not selected else prefixes["orbital"])
    required_old = set(range(66)) | set(range(3588, 3599))
    # Frozen public ranges own all preceding native IDs; reserved/unselected
    # slots remain absent instead of becoming fabricated transparent coverage.
    for packed in accepted["prior_public_ranges"].values():
        if "native_offset" in packed:
            required_old.update(range(packed["native_offset"], packed["native_offset"]+packed["sprite_count"]))
    observed_old = [row[0] for row in old]
    if len(observed_old) != 6116 or len(set(observed_old)) != 6116 or set(observed_old) != required_old:
        raise ValueError("protected native IDs differ from qualified frozen public ranges")
    records = list(old)
    with Image.open(image_path if selected else ROOT/scope["png"]) as image:
        # Every new logical slot retains its own identity, even when an alias
        # shares physical pixels or the original back/front part is wholly empty.
        for row in report["sprites"]:
            x, y, width, height = row["box"]
            pixels = image.crop((x, y, x+width, y+height)).tobytes()
            if hashlib.sha256(pixels).hexdigest() != row["pixel_sha256"]:
                raise ValueError("qualified launch/fleet packed crop changed")
            records.append((row["native_offset"], width, height, *row["anchor"], pixel_hash(pixels), pixels))
    records.sort(key=lambda row: row[0])
    offsets = [row[0] for row in records]
    # Exact prior/new source domains must fit the current header bound without
    # duplicates, overlaps or activation of old reserved gaps.
    if (len(records) != 7764 or len(set(offsets)) != len(records)
            or set(offsets) != required_old | set(range(6143, 7791))):
        raise ValueError("complete native full-pixel expectation coverage changed")
    chunks = [b"LNFART2\0", struct.pack("<HHHH", len(records), 7791, 6143, 1648)]
    # Full pixel bytes supplement historical portable fingerprints: the actual
    # native cache must now match every old/new transparent and material byte.
    for offset, width, height, ax, ay, digest, pixels in records:
        chunks.extend((struct.pack("<HHHhhI", offset, width, height, ax, ay, digest), pixels))
    payload = b"".join(chunks)
    if selected:
        write_selected_payload(destination, payload)
        return dict(total_sprites=7764, protected_sprites=6116, new_sprites=1648, native_loading_verified=False)
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(payload)
    summary = dict(_documentation=__doc__, fixture=art.source.identity(OUTPUT), producer=art.source.identity(Path(__file__)),
                   source_handoff=scope["source_handoff"], source_qualification=scope["source_qualification"],
                   native_bound=7791, protected_sprites=6116, new_sprites=1648, total_sprites=7764,
                   protected_expected_fixtures=[art.source.identity(OUTPUT.parent/(name+"-packed-sprites.bin")) for name in ("m1", "space", "orbital")],
                   source_outputs=report["outputs"], native_loading_verified=False, runtime=False)
    SUMMARY.parent.mkdir(parents=True, exist_ok=True)
    SUMMARY.write_text(json.dumps(summary, indent=2)+"\n", encoding="utf-8")
    return summary


def produce_selected(report, image_path, destination, prefixes=None):
    """Execute the original complete-byte producer using explicit current source roles.

    @param report Checked selected launch/fleet packing report.
    @param image_path Exact current original source-derived launch packing sheet.
    @param destination Explicit current binary data output.
    @param prefixes Exact selected m1/space/orbital original expectation binary paths.
    @return Complete7764source expectation count; no native loading or publication occurs.
    """
    return produce(report, image_path, destination, prefixes)["total_sprites"]


def main():
    """Build source expectations only; the real native loader owns acceptance.

    @return Zero when all retained and new source expectations match their original owners.
    """
    summary = produce()
    print(json.dumps({name: summary[name] for name in ("protected_sprites", "new_sprites", "total_sprites", "native_bound", "native_loading_verified")}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
