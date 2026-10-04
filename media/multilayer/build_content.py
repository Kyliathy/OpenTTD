"""Build first-party cargo definitions and their exact compiled identity together.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md.
@see docs/zz-reach2/architecture/newgrf/archi-newgrf.md.
The JSON catalogue is the single definition source. This generator emits the
staged-loader package and a C++ requirement manifest from those same bytes.
Industry, vehicle, utility and original-art behavior belongs to the feature plans.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct


# These are format bit positions from CargoClass, not a second runtime cargo registry.
CLASSES = {"Bulk": 4, "PieceGoods": 5, "Liquid": 6, "Refrigerated": 7,
           "Hazardous": 8, "Covered": 9, "Oversized": 10, "Potable": 13, "NonPotable": 14}
FORMS = {"dry_bulk": "DryBulk", "covered_granules": "CoveredGranules", "sealed_drums": "SealedDrums",
         "ingots": "Ingots", "liquid": "Liquid", "packaged": "Packaged", "frozen_bulk": "FrozenBulk"}


def word(value):
    """Encode a bounded NewGRF word.

    @param value Unsigned 16-bit integer.
    @return Two bytes; struct.pack rejects out-of-range quantities.
    """
    return struct.pack("<H", value)


def dword(value):
    """Encode a bounded NewGRF double word.

    @param value Unsigned 32-bit integer.
    @return Four bytes.
    """
    return struct.pack("<I", value)


def action0(feature, local_id, properties):
    """Encode one definition with ordered Action 0 properties.

    @param feature NewGRF feature number.
    @param local_id Definition ID below 255, owned by the package.
    @param properties Ordered property-number/encoded-value pairs.
    @return Staged-loader pseudo sprite payload.
    """
    return bytes([0, feature, len(properties), 1, local_id]) + b"".join(
        bytes([prop]) + value for prop, value in properties)


def validate_catalogue(catalogue):
    """Reject ambiguous labels, unsupported forms and over-budget definitions.

    @param catalogue Decoded first-party source record.
    @return Ordered required labels, including preserved Classic meanings.
    """
    labels = catalogue["baseline_labels"] + [cargo["label"] for cargo in catalogue["cargoes"]]
    if len(labels) != len(set(labels)):
        raise ValueError("profile labels must be unique")
    if len(catalogue["cargoes"]) + 12 > 64:
        raise ValueError("new definitions exceed the original-preserving cargo envelope")
    # Required labels must resolve identically in the package, compiled manifest and saved identity.
    for label in labels + [catalogue["package_id"]]:
        if len(label.encode("ascii")) != 4:
            raise ValueError(f"not a four-character identity: {label}")
    # A liquid cannot become a packaged refit just because a container vehicle exists.
    for cargo in catalogue["cargoes"]:
        if len(cargo["classes"]) != len(set(cargo["classes"])):
            raise ValueError("cargo classes must not repeat")
        if cargo["form"] not in FORMS:
            raise ValueError("cargo form must have an explicit transport meaning")
        unknown = set(cargo["classes"]) - CLASSES.keys()
        if unknown:
            raise ValueError(f"unsupported cargo classes: {unknown}")
        if cargo["form"] == "liquid":
            if cargo["unit"] != "kilolitres":
                raise ValueError("liquid quantities must use kilolitres")
            if "Liquid" not in cargo["classes"]:
                raise ValueError("liquid cargo requires tanker compatibility")
            if "PieceGoods" in cargo["classes"]:
                raise ValueError("liquid cargo cannot silently become packaged goods")
        if cargo["label"] in ("ERTH", "ROCK"):
            if cargo["payment"] != 0:
                raise ValueError("spoil must not earn unrestricted freight income")
    return labels


def build_package(catalogue):
    """Compile only the cargo-definition family owned by the first content contract.

    @param catalogue Validated source catalogue, preserving original slots 0..11.
    @return Complete container-v1 package bytes.
    """
    package_id = catalogue["package_id"].encode("ascii")
    sprites = [b"\x08\x08" + package_id
               + b"Multilayer Industry\0First-party cargo definitions; R2MLVS1\0"]
    # FileScan stops at Action 8, so version metadata must precede the identifying Action 8 record.
    # Action 14 pins the definition version independently of GRF container version.
    version = catalogue["package_version"]
    sprites.insert(0, b"\x14CINFO" + b"BVRSN" + word(4) + dword(version)
                   + b"BMINV" + word(4) + dword(version) + b"\0\0")
    # Add one definition per label outside the original cargo envelope. Baseline entries are untouched.
    for offset, cargo in enumerate(catalogue["cargoes"]):
        slot = 12 + offset
        base_string = 0xD000 + offset * 5
        names = [cargo["name"], cargo["name"], cargo["unit"],
                 f'{cargo["unit"]} of {cargo["name"]}', cargo["label"][:2]]
        # The quantity name consumes the numeric parameter through the real NewGRF DWORD formatter.
        encoded_names = [name.encode("ascii") for name in names]
        encoded_names[3] = b"\x7b " + encoded_names[3]
        sprites.append(bytes([4, 11, 255, len(names)]) + word(base_string)
                       + b"".join(name + b"\0" for name in encoded_names))
        classes = sum(1 << CLASSES[name] for name in cargo["classes"])
        # Base cargo icons make draft content legible; original icon art is still owned by r2mlvs13.
        sprites.append(action0(11, slot, [
            (0x08, bytes([slot])), (0x17, cargo["label"].encode("ascii")),
            (0x09, word(base_string)), (0x0A, word(base_string + 1)),
            (0x1B, word(base_string + 2)), (0x1C, word(base_string + 3)),
            (0x0D, word(base_string + 4)), (0x0E, word(4309)),
            (0x0F, b"\x10"), (0x10, b"\x09"), (0x11, b"\xff"),
            (0x12, dword(cargo["payment"])), (0x13, b"\xb8"), (0x14, b"\xb8"),
            (0x15, b"\x01"), (0x16, word(classes)), (0x18, b"\xff"),
            (0x1D, word(256)),
        ]))
    records = [dword(len(sprites))] + sprites
    return b"".join(word(len(record)) + b"\xff" + record for record in records) + word(0)


def build_manifest(catalogue, package):
    """Bind the compiled requirement to the exact bytes produced in this build.

    @param catalogue Single authoritative content source.
    @param package Compiled definition package whose digest is pinned.
    @return C++ declarations included only by the profile implementation.
    """
    labels = validate_catalogue(catalogue)
    digest = ", ".join(str(value) for value in hashlib.md5(package).digest())
    rows = ",\n".join(f'\tCargoLabel{{"{label}"}}' for label in labels)
    forms = ",\n".join(
        f'\t{{CargoLabel{{"{cargo["label"]}"}}, MultiLayerCargoForm::{FORMS[cargo["form"]]}, "{cargo["unit"]}"}}'
        for cargo in catalogue["cargoes"])
    return ("/** @file multilayer_content_manifest.h Generated immutable requirements. "
            "@see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */\n"
            f'static constexpr uint16_t _multilayer_profile_version = {catalogue["profile_version"]};\n'
            f"static constexpr CargoLabel _multilayer_required_labels[] = {{\n{rows}\n}};\n"
            f"static constexpr MultiLayerCargoContract _multilayer_cargo_contracts[] = {{\n{forms}\n}};\n"
            f'static constexpr GrfID _multilayer_package_id{{"{catalogue["package_id"]}"}};\n'
            f'static constexpr uint32_t _multilayer_package_version = {catalogue["package_version"]};\n'
            f"static constexpr uint8_t _multilayer_package_digest[] = {{{digest}}};\n")


def main():
    """Emit the package and C++ manifest inside the Git repository.

    @return Process exit code; unsafe paths and malformed catalogues fail before writing.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--catalogue", type=Path, required=True)
    parser.add_argument("--package", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    catalogue = json.loads(args.catalogue.read_text(encoding="utf-8"))
    validate_catalogue(catalogue)
    package = build_package(catalogue)
    manifest = build_manifest(catalogue, package)
    # Build products must not write to user-wide game or content directories.
    for target in (args.package, args.manifest):
        if not target.resolve().is_relative_to(root):
            raise ValueError("content build output must remain inside the repository")
    args.package.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.package.write_bytes(package)
    args.manifest.write_text(manifest, encoding="utf-8")
    print(f"Multilayer Industry: {len(catalogue['cargoes'])} additional cargos; {len(package)} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
