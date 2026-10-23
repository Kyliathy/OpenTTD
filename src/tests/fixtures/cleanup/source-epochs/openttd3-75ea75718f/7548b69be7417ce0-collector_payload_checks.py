"""Decode the actual collector GRF and compare every selected/protected source pixel.

@file collector_payload_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group79C.2
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
GRFCodec decoding verifies payload inclusion independently of source packing.
Compiled application loading and ordinary-game rendering remain Group79C.4.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
PACKED = ROOT / "logs/proofs/mlm/native-art/derived/packed-manifest.json"
PROOF = ROOT / "logs/proofs/mlm/native-art/packed-build-proof.json"
GRF = ROOT / "media/baseset/openttd.grf"


def digest(path):
    """Fingerprint the complete actual input/output container.

    @param path Existing file selected by the explicit current invocation.
    @return Lowercase SHA256 of its exact bytes.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def extended_word(values, position):
    """Read Action5's byte-or-extended-word count/offset encoding.

    @param values Decoded pseudo-sprite bytes, including its action and type.
    @param position Index of the count or offset to decode.
    @return Value and first unread byte index; reject truncated metadata.
    """
    if position >= len(values):
        raise ValueError("truncated native Action5 value")
    value = values[position]
    if value != 255:
        return value, position + 1
    if position + 2 >= len(values):
        raise ValueError("truncated extended native Action5 value")
    return values[position + 1] | values[position + 2] << 8, position + 3


def decoded_native_rows(path):
    """Read only real native-map-layer replacement blocks from GRFCodec output.

    @param path Current GRFCodec version6 NFO emitted with -xx beside its PNGs.
    @return Native-offset keyed decoded image/crop/anchor records.
    """
    text = path.read_text(encoding="latin-1")
    if "(Info version 6)" not in text[:200]:
        raise ValueError("decoder geometry requires explicit version6 NFO")
    lines, rows = text.splitlines(), {}
    action = re.compile(r"\s*(\d+)\s+\*\s+(\d+)\s+(05\s+(?:1C|9C)\s+[0-9A-F\s]+)", re.IGNORECASE)
    # Actual Action5 blocks, rather than the source converter's assumed order,
    # determine which decoded sprite owns every native map-layer offset.
    for index, line in enumerate(lines):
        match = action.fullmatch(line)
        if match is None:
            continue
        values = [int(value, 16) for value in match[3].split()]
        if len(values) != int(match[2]):
            raise ValueError("decoded native Action5 byte count differs")
        count, position = extended_word(values, 2)
        offset = 0
        if values[1] & 0x80:
            offset, position = extended_word(values, position)
        if position != len(values) or count == 0 or index + count >= len(lines):
            # A replacement block must describe exactly its complete following
            # real sprites; malformed metadata cannot hide missing native poses.
            raise ValueError("invalid decoded native Action5 extent")
        for local in range(count):
            fields = lines[index + local + 1].rsplit(maxsplit=7)
            if len(fields) != 8:
                raise ValueError("native Action5 contains a non-real sprite")
            number, name = fields[0].split(maxsplit=1)
            if int(number) != int(match[1]) + local + 1:
                raise ValueError("decoded native sprite sequence differs")
            x, y = map(int, fields[1:3])
            int(fields[3], 16)  # Version6 compression flags; PNG bytes own pixel meaning.
            height, width, rx, ry = map(int, fields[4:])
            image = Path(name).resolve()
            if not image.is_relative_to(path.parent.resolve()):
                raise ValueError("decoded sprite image leaves its current output directory")
            native_offset = offset + local
            if native_offset in rows:
                raise ValueError("duplicate decoded native offset")
            rows[native_offset] = {"path": image, "box": [x, y, width, height], "anchor": [rx, ry]}
    return rows


def source_native_rows(report):
    """Resolve exact protected M0 and selected M1 source crops without decoding GRF.

    @param report Current independently generated selected source packing report.
    @return Native-offset keyed original source image/crop/anchor expectations.
    """
    directory = ROOT / "media/baseset/openttd"
    rows = {}
    legacy = (directory / "map_layers.nfo").read_text(encoding="utf-8")
    declarations = [line.split() for line in legacy.splitlines() if "sprites/map_layers.png 8bpp" in line]
    if len(declarations) != 66:
        raise ValueError("protected native M0 source count differs")
    # The old source NFO supplies every protected anchor verbatim; generated
    # collector metadata cannot redefine the retained prefix's geometry.
    for offset, declaration in enumerate(declarations):
        x, y, width, height, rx, ry = map(int, declaration[3:9])
        rows[offset] = {"path": directory / "map_layers.png", "box": [x, y, width, height], "anchor": [rx, ry]}
    # Every selected family pose contributes its own source crop and world
    # anchor; no decoded sprite can satisfy another omitted state or cell.
    for sprite in report["sprites"]:
        offset = sprite["native_offset"]
        if offset in rows:
            raise ValueError("selected source overlaps a protected native offset")
        rows[offset] = {"path": directory / "map_layers_m1.png", "box": sprite["box"], "anchor": sprite["anchor"]}
    return rows


def verify_pixels(expected, decoded):
    """Compare actual decoded native pixels, geometry and complete offset coverage.

    @param expected Protected and selected source-owned sprite declarations.
    @param decoded Actual GRFCodec-decoded Action5 declarations and PNGs.
    @return Number of exact sprites and the decoded image paths fingerprinted.
    """
    if set(expected) != set(decoded):
        raise ValueError("actual decoded native coverage differs from source")
    images = {}
    # Every real sprite is compared, including transparency and both aligned
    # collector parts. A representative thumbnail cannot prove missing states,
    # a shifted source cell or accidental replacement of an existing family.
    for offset, source in expected.items():
        actual = decoded[offset]
        if actual["box"][2:] != source["box"][2:] or actual["anchor"] != source["anchor"]:
            # Sprite-sheet positions may differ after decoding, but native
            # dimensions/world anchors must preserve the exact source contract.
            raise ValueError(f"decoded native geometry differs at offset {offset}")
        pixels = []
        # Inspect the source and independently decoded canvases under the same
        # indexed-pixel rules before comparing their complete crop contents.
        for row in (source, actual):
            path = row["path"]
            if path not in images:
                images[path] = Image.open(path)
            image = images[path]
            if image.mode != "P":
                raise ValueError("decoded native source is not indexed artwork")
            x, y, width, height = row["box"]
            # PIL can pad an out-of-bounds crop with transparent pixels. Reject
            # such geometry so clipped hardware cannot pass as a valid sprite.
            if not (0 <= x < x + width <= image.width and 0 <= y < y + height <= image.height):
                raise ValueError(f"decoded native crop leaves image at offset {offset}")
            pixels.append(image.crop((x, y, x + width, y + height)).tobytes())
        if images[source["path"]].getpalette() != images[actual["path"]].getpalette() or pixels[0] != pixels[1]:
            # The DOS palette and every index, including transparent index0,
            # must agree; visually similar RGB output is insufficient evidence.
            raise ValueError(f"decoded native palette/pixels differ at offset {offset}")
    return len(expected), sorted({row["path"] for row in decoded.values()})


def main():
    """Decode the current packaged payload and retain exact non-runtime evidence.

    @return None; reject stale build/copy evidence or any decoded source mismatch.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--grfcodec", required=True)
    parser.add_argument("--run-dir", required=True)
    args = parser.parse_args()
    run = Path(args.run_dir).resolve()
    if not run.is_relative_to((ROOT / "logs").resolve()):
        raise ValueError("payload decoding output must stay beneath repository logs")
    run.mkdir(parents=True, exist_ok=True)
    directory = run / "decoded"
    directory.mkdir(exist_ok=True)
    report = json.loads(PACKED.read_text(encoding="utf-8"))
    proof = json.loads(PROOF.read_text(encoding="utf-8"))
    sha = digest(GRF)
    # One payload must match both clean encodings and the explicitly disabled
    # generator copy route; source-only agreement cannot prove distribution.
    if (proof["reproducible"]["sha256"] != [sha, sha] or proof["generator_free_packaging"]["sha256"] != sha
            or not proof["generator_free_packaging"]["generator_discovery_disabled"]):
        raise ValueError("actual payload differs from two clean builds or cached packaging")
    if digest(ROOT / "build/baseset/openttd.grf") != sha:
        raise ValueError("generator-free packaged copy is stale")
    # Fingerprint each source output used by the actual encodings. A changed
    # report/atlas cannot retroactively describe already packed native bytes.
    for name, source in (("png", report["png"]), ("nfo", report["nfo"]), ("header", report["header"])):
        if digest(ROOT / source["path"]) != source["sha256"]:
            raise ValueError("current packed source changed: " + name)
        if proof["reproducible"]["source_outputs"].get(source["path"]) != source["sha256"]:
            raise ValueError("actual encoding used another packed source: " + name)
    log = run / "independent-decode.log"
    command = [args.grfcodec, "-d", "-xx", "-o", "png", "-p", "1", str(GRF), str(directory)]
    with log.open("wb") as output:
        subprocess.run(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, check=True, timeout=60,
                       creationflags=subprocess.CREATE_NO_WINDOW)
    if digest(GRF) != sha:
        raise ValueError("payload changed during independent decoding")
    nfo = directory / "openttd.nfo"
    count, images = verify_pixels(source_native_rows(report), decoded_native_rows(nfo))
    if count != 66 + report["pose_count"]:
        raise ValueError("decoded protected/selected count differs")
    receipt = {"_documentation": "Plan13 Group79C.2 actual generated/cached GRF decode; compiled native loading/rendering remain79C.4",
               "verifier": {"path": Path(__file__).relative_to(ROOT).as_posix(), "sha256": digest(Path(__file__))},
               "sha256": sha, "protected_sprites": 66, "selected_sprites": report["pose_count"], "decoded_sprites": count,
               "native_bound": report["native_sprite_bound"], "all_source_pixels_palettes_dimensions_and_anchors_exact": True,
               "decoder_command": command, "decoder_sha256": digest(Path(args.grfcodec)), "decoder_log": log.relative_to(ROOT).as_posix(),
               "decoded_outputs": {path.relative_to(ROOT).as_posix(): digest(path) for path in [nfo, *images]},
               "source_outputs": proof["reproducible"]["source_outputs"], "native_loading_verified": False, "runtime_activated": False}
    target = run / "payload-verification.json"
    target.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    proof["independent_decode"] = {"sha256": sha, "selected_sprites": report["pose_count"], "native_bound": report["native_sprite_bound"],
                                   "receipt": target.relative_to(ROOT).as_posix(), "receipt_sha256": digest(target)}
    PROOF.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(f"Actual GRF decode: {count} exact native sprites, including all {report['pose_count']} selected source poses; no runtime activation claimed")


if __name__ == "__main__":
    main()
