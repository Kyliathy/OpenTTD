#!/usr/bin/env python3
"""Real saved editor policy and exact-content/corrupt-metadata refusal fixtures.

@file multilayer_profile_save_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md, Group 21
"""

import hashlib
import argparse
import lzma
from pathlib import Path

from multilayer_state_fixture import AREA, ROOT, run_case


def metadata_variant(source, suffix, policy=None, schema_version=None, remove=False):
    """Change only an owned fixture's version-1 metadata transport, preserving original inputs.

    @param source Existing repository-owned 371 native fixture produced by the metadata harness.
    @param suffix Owning plan's lowercase evidence suffix.
    @param policy Optional checked raw policy flags for an explicit saved editor case.
    @param schema_version Optional deliberately unsupported schema revision.
    @param remove Whether to remove this owned fixture's metadata chunk entirely.
    @return New evidence save path, with unchanged source bytes.
    """
    original = source.read_bytes()
    if original[:8] != b"OTTX\x01\x73\x00\x00":
        raise ValueError("fixture requires actual LZMA/version-371 output")
    stream = bytearray(lzma.decompress(original[8:]))
    start = stream.index(b"MLMD")
    end = stream.index(b"CHTS", start + 4)
    row = end - 52
    # Version-1 MLMD has 50 scalar/array bytes plus the MD5 array's length byte and table terminator; refuse another codec shape.
    if stream[start + 4] != 3 or stream[row:row + 6] != b"\x00\x01\x01\x00\x01\x00" or stream[end - 1] != 0:
        raise ValueError("unexpected metadata wire layout")
    if policy is not None:
        stream[row + 5] = policy
    if schema_version is not None:
        stream[row:row + 2] = schema_version.to_bytes(2, "big")
    if remove:
        del stream[start:end]
    target = AREA / f"r2mlvs1-h-{suffix}-input.sav"
    target.write_bytes(original[:8] + lzma.compress(stream))
    return target


def main():
    """Run saved-policy/refusal boundaries and restore every existing package/config/script byte.

    @return None; failures retain their logs and restore the exact original inputs.
    """
    source = AREA / "r2mlvs14-b-native-new.sav"
    source_digest = hashlib.sha256(source.read_bytes()).hexdigest()
    package = ROOT / "build/newgrf/multilayer_industry.grf"
    held = AREA / "r2mlvs1-h-held-package.grf"
    original_package = package.read_bytes()
    startup = ROOT / "build/scripts/game_start.scr"
    config = ROOT / "build/grftest.cfg"
    original_script = startup.read_bytes()
    original_config = config.read_bytes()
    parser = argparse.ArgumentParser()
    parser.add_argument("--packages-only", action="store_true", help="repeat only the active saved-package refusal boundary")
    args = parser.parse_args()
    try:
        config.write_text("# @see r2mlvs1-gameplay-contracts-and-content.md, Group 21.\n"
                          "[game_creation]\nmap_x = 6\nmap_y = 6\nlandscape = temperate\n"
                          "gameplay_profile = multilayer_industry\n"
                          "[network]\nserver_advertise = false\nserver_bind_addresses = 127.0.0.1\n", encoding="utf-8")
        if not args.packages_only:
            editor = metadata_variant(source, "editor", policy=2)
            saved_editor = run_case("editor", ["-g", editor.as_posix()],
                                    "MLVS_METADATA schema=1 profile=1 version=1 legacy=false editor=true", prefix="r2mlvs1-h")
            run_case("editor-reload", ["-g", saved_editor.as_posix()],
                     "MLVS_METADATA schema=1 profile=1 version=1 legacy=false editor=true", prefix="r2mlvs1-h")
            future = metadata_variant(source, "future-schema", schema_version=2)
            missing_metadata = metadata_variant(source, "missing-metadata", remove=True)
            bad_policy = metadata_variant(source, "bad-policy", policy=4)
            # Each unsupported metadata candidate must refuse before startup/material or gameplay activation.
            for name, path, expected in (("future-schema", future, "Unsupported or invalid multilayer metadata"),
                                         ("missing-metadata", missing_metadata, "Missing required multilayer metadata chunk"),
                                         ("bad-policy", bad_policy, "Unknown multilayer policy flags")):
                run_case(name, ["-g", path.as_posix()], expected, accepted=False, prefix="r2mlvs1-h")
        if held.exists():
            raise ValueError("held package destination already exists")
        package.rename(held)
        try:
            run_case("missing-package", ["-g", source.as_posix()], "Required Multilayer content package is missing", accepted=False, prefix="r2mlvs1-h")
        finally:
            held.rename(package)
        package.write_bytes(original_package + b"\n")
        try:
            run_case("changed-package", ["-g", source.as_posix()], "Required Multilayer content package differs from its saved identity/version", accepted=False, prefix="r2mlvs1-h")
        finally:
            package.write_bytes(original_package)
    finally:
        startup.write_bytes(original_script)
        config.write_bytes(original_config)
        assert package.read_bytes() == original_package, "required package changed"
        assert hashlib.sha256(source.read_bytes()).hexdigest() == source_digest, "source fixture changed"
    # A partial rerun may report combined success only when earlier editor/metadata evidence still proves those five boundaries.
    if args.packages_only:
        previous = (("editor", "legacy=false editor=true", True),
                    ("editor-reload", "legacy=false editor=true", True),
                    ("future-schema", "Unsupported or invalid multilayer metadata", False),
                    ("missing-metadata", "Missing required multilayer metadata chunk", False),
                    ("bad-policy", "Unknown multilayer policy flags", False))
        for name, expected, accepted in previous:
            evidence = (AREA / f"r2mlvs1-h-{name}.log").read_text(encoding="utf-8", errors="replace")
            assert expected in evidence, (name, "prior boundary evidence missing")
            assert (AREA / f"r2mlvs1-h-{name}.sav").is_file() == accepted, (name, "prior save boundary differs")
    scope = "Two package refusals rerun; five earlier editor/metadata boundaries verified from retained evidence.\n" if args.packages_only else "All seven boundaries executed in this run.\n"
    (AREA / "r2mlvs1-h-saved-profile-results.txt").write_text(
        "R2MLVS1 Group 21: explicit saved editor policy/reload and five metadata/content refusals passed.\n" + scope +
        "Original package, configuration, startup and source save preserved.\n", encoding="utf-8")


if __name__ == "__main__":
    main()
