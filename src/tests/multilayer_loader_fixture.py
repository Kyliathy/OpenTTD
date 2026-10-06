"""Reproducible ore/processor/carrier package for the R2MLVS1 loader experiment.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md, Group 3.
@see docs/zz-reach2/architecture/newgrf/archi-newgrf.md.
The fixture deliberately uses existing base sprites; it does not deliver final art
or all-input production. It uses the real staged NewGRF loader and saved mappings.
All generated artifacts remain inside the repository. No external content tools
or user-wide content stores are used.
"""

import argparse
import json
import os
from pathlib import Path
import re
import struct
import subprocess

from multilayer_state_fixture import save_path


def word(value):
    """Encode an unsigned little-endian word.

    @param value Integer in the 16-bit range.
    @return Exactly two bytes, rejecting overflow through struct.pack.
    """
    return struct.pack("<H", value)


def dword(value):
    """Encode an unsigned little-endian double word.

    @param value Integer in the 32-bit range.
    @return Exactly four bytes.
    """
    return struct.pack("<I", value)


def action0(feature, local_id, properties):
    """Define one entity through the engine's Action 0 property loader.

    @param feature NewGRF feature number.
    @param local_id Local entity ID below 255.
    @param properties Ordered pairs of property number and encoded bytes.
    @return Pseudo sprite payload.
    """
    return bytes([0, feature, len(properties), 1, local_id]) + b"".join(
        bytes([key]) + value for key, value in properties
    )


def package_bytes():
    """Build a v1 package with one additional ore, processor and rail carrier.

    @return GRF container bytes, including sprite count and terminator.
    """
    # Stable probe identity; it is not the production profile's package identity.
    sprites = [b"\x08\x08MLVP" + b"Multilayer loader probe\0R2MLVS1 delivery experiment\0"]
    names = b"Copper Ore\0Copper Ore\0tonnes\0tonnes of Copper Ore\0CO\0Probe Processor\0"
    sprites.append(bytes([4, 10, 255, 6]) + word(0xD000) + names)
    # Cargo slot 12 appends after the preserved 12-slot original envelope.
    sprites.append(action0(11, 12, [
        (0x08, b"\x0c"), (0x17, b"CORE"), (0x09, word(0xD000)),
        (0x0A, word(0xD001)), (0x1B, word(0xD002)), (0x1C, word(0xD003)),
        (0x0D, word(0xD004)), (0x0E, word(4309)), (0x0F, b"\x10"),
        (0x10, b"\x09"), (0x11, b"\xff"), (0x12, dword(5120)),
        (0x13, b"\xb8"), (0x14, b"\xb8"), (0x15, b"\x01"),
        (0x16, word((1 << 4) | (1 << 14))), (0x18, b"\xff"), (0x1D, word(256)),
    ]))
    # An explicit translation list keeps entity references label-based, including appended CORE.
    labels = [b"PASS", b"COAL", b"MAIL", b"OIL_", b"LVST", b"GOOD", b"GRAI",
              b"WOOD", b"IORE", b"STEL", b"VALU", b"VOID", b"CORE"]
    sprites.append(bytes([0, 8, 1, len(labels), 0, 0x09]) + b"".join(labels))
    # A steel-mill substitute supplies a tested base layout; it does not override that mill.
    sprites.append(action0(10, 0, [
        (0x08, b"\x08"), (0x26, bytes([2, 12, 1])), (0x25, bytes([1, 9])),
        (0x17, b"\x00"), (0x18, b"\x00"), (0x1F, word(0xD005)),
        (0x1A, dword(0)),
    ]))
    # Local train 116 has no original counterpart, so the dynamic engine manager appends it.
    sprites.append(action0(0, 116, [
        (0x00, word(10958)), (0x03, b"\x1e"), (0x04, b"\xff"),
        (0x06, b"\x01"), (0x05, b"\x00"), (0x09, word(64)),
        (0x0B, word(300)), (0x12, b"\x04"), (0x14, b"\x20"),
        (0x15, b"\x0c"), (0x16, b"\x2f"), (0x17, b"\x07"),
        (0x19, b"\x00"), (0x2C, bytes([1, 12])),
    ]))
    # Serialize every action once in staged-loader order; there is no external registration step.
    records = [dword(len(sprites))] + sprites
    return b"".join(word(len(record)) + b"\xff" + record for record in records) + word(0)


def run_roundtrips(root):
    """Exercise real dedicated new-world and save reloads for both candidates.

    @param root Resolved repository root containing the configured Debug build.
    @return Dictionary of checked candidate diagnostics.
    """
    area = root / "docs/zz-reach2/upgrades/2026-10"
    fixtures = root / "src/tests/fixtures/content"
    startup = root / "build/scripts/game_start.scr"
    original = startup.read_bytes()
    config_proxy = root / "build/grftest.cfg"
    original_config = config_proxy.read_bytes()
    package_proxy = root / "build/newgrf/test_road_type.grf"
    original_package = package_proxy.read_bytes()
    runner = area / "tools/run_dedicated.sh"
    package = fixtures / "multilayer_probe.grf"
    package.write_bytes(package_bytes())
    # Saved GRFs are rediscovered by identity from the runtime newgrf search directory.
    # Reuse and restore the existing local test package; an arbitrary absolute path is insufficient on reload.
    package_proxy.write_bytes(package.read_bytes())
    results = {}
    try:
        # Each candidate is regenerated then reloaded once; there is no broad world regression.
        for mode in ("native", "bundled"):
            config = fixtures / f"r2mlvs1-b-{mode}.cfg"
            config.write_text(
                "# @see r2mlvs1-gameplay-contracts-and-content.md, Group 3 dedicated fixture.\n"
                "[game_creation]\nmap_x = 6\nmap_y = 6\nlandscape = temperate\n"
                "[network]\nserver_advertise = false\nserver_bind_addresses = 127.0.0.1\n"
                + ("[newgrf]\n" + package_proxy.name + " =\n" if mode == "bundled" else ""),
                encoding="utf-8",
            )
            # A bare -c path retains the existing build's local baseset and scripts.
            # Preserve the existing proxy rather than creating scratch configuration outside the plan.
            config_proxy.write_bytes(config.read_bytes())
            before = None
            # The second load reconstructs definitions using saved engine and industry mappings.
            for phase in ("new", "reload"):
                saved = save_path(f"r2mlvs1-b-{mode}-{phase}.sav")
                saved.unlink(missing_ok=True)
                script = fixtures / f"r2mlvs1-b-{mode}-{phase}.scr"
                log = area / f"r2mlvs1-b-{mode}-{phase}.log"
                script.write_text(
                    "# @see r2mlvs1-gameplay-contracts-and-content.md, Group 3 saved-content fixture.\n"
                    f'dump_info cargotypes\nsave "{saved.with_suffix("").as_posix()}"\nquit\n',
                    encoding="utf-8",
                )
                startup.write_text(f'exec "{script.as_posix()}"\n', encoding="utf-8")
                env = dict(os.environ, OTTD_MULTILAYER_LOADER_PROBE=mode,
                           OTTD_TIMEOUT="60", OTTD_DEBUG="misc=5,net=2,sl=2,grf=2,script=2")
                arguments = ["-G", "1234"] if phase == "new" else [
                    "-g", save_path(f"r2mlvs1-b-{mode}-new.sav").as_posix()]
                command = ["C:/Program Files/Git/bin/bash.exe", runner.as_posix(),
                           (root / "build").as_posix(), "Debug", log.as_posix(),
                           "-X", "-c", config_proxy.name, *arguments]
                subprocess.run(command, check=True, env=env, timeout=70)
                evidence = log.read_text(encoding="utf-8", errors="replace")
                assert saved.is_file(), f"fixture did not save: {saved}"
                messages = re.findall(r"MLVS_PROBE (.*)", evidence)
                # Report lines can occur during the title-world load as well; final identities must agree.
                cargo = next(line for line in reversed(messages) if line.startswith("cargo "))
                processor = next(line for line in reversed(messages) if line.startswith("processor "))
                carrier = next(line for line in reversed(messages) if line.startswith("carrier "))
                actual = (cargo, processor, carrier)
                assert "slot=12 sprite=true" in cargo, actual
                assert "inputs=12,1 output=9" in processor, actual
                assert "id=256 local=116 cargo=12 capacity=32 image=2" in carrier, actual
                if before is None:
                    before = actual
                else:
                    assert before == actual, f"saved identities changed: {before!r} -> {actual!r}"
                print(f"{mode} {phase}: {actual}", flush=True)
            results[mode] = before
    finally:
        # Preserve the pre-existing developer startup script byte for byte, including after failure.
        startup.write_bytes(original)
        config_proxy.write_bytes(original_config)
        package_proxy.write_bytes(original_package)
        # Generated command scripts are disposable; retain the supplied saves and the actual run evidence.
        for mode in ("native", "bundled"):
            for phase in ("new", "reload"):
                (fixtures / f"r2mlvs1-b-{mode}-{phase}.scr").unlink(missing_ok=True)
    (area / "r2mlvs1-b-loader-results.txt").write_text(
        "R2MLVS1 Group 3: dedicated definition round-trip evidence\n"
        + "\n".join(f"{mode}: {value!r}" for mode, value in results.items()) + "\n",
        encoding="utf-8",
    )
    return results


def run_profiles(root):
    """Compare real Classic/Multilayer activation and explicit dedicated refusal paths.

    @param root Resolved repository root with the built first-party package and Debug executable.
    @return Checked label-to-slot maps for both supported Temperate profiles.
    """
    area = root / "docs/zz-reach2/upgrades/2026-10"
    fixtures = root / "src/tests/fixtures/content"
    startup = root / "build/scripts/game_start.scr"
    config_proxy = root / "build/grftest.cfg"
    package = root / "build/newgrf/multilayer_industry.grf"
    original_script = startup.read_bytes()
    original_config = config_proxy.read_bytes()
    original_package = package.read_bytes()
    held_package = fixtures / "r2mlvs1-c-held-package.grf"
    catalogue = json.loads((root / "media/multilayer/cargo_catalogue.json").read_text(encoding="utf-8"))
    expected_classic = {label: slot for slot, label in enumerate(catalogue["baseline_labels"])}
    expected_multilayer = dict(expected_classic)
    expected_multilayer.update({cargo["label"]: 12 + offset for offset, cargo in enumerate(catalogue["cargoes"])})
    results = {}
    cases = (
        ("classic", "classic", "temperate", "", None),
        ("multilayer", "multilayer_industry", "temperate", "", None),
        ("arctic", "multilayer_industry", "arctic", "", "currently supports the Temperate climate"),
        ("external", "multilayer_industry", "temperate", "[newgrf]\ntest_road_type.grf =\n", "selected external set has no supported adapter"),
        ("missing", "multilayer_industry", "temperate", "", "content package is missing"),
        ("changed", "multilayer_industry", "temperate", "", "required identity and version"),
    )
    try:
        # Each case exercises one activation/refusal boundary; no broad map or vehicle regression runs here.
        for name, profile, climate, extra, refusal in cases:
            script = fixtures / f"r2mlvs1-c-{name}.scr"
            saved = save_path(f"r2mlvs1-c-{name}.sav")
            log = area / f"r2mlvs1-c-{name}.log"
            config = fixtures / f"r2mlvs1-c-{name}.cfg"
            saved.unlink(missing_ok=True)
            config.write_text(
                "# @see r2mlvs1-gameplay-contracts-and-content.md, Group 4 profile fixture.\n"
                f"[game_creation]\nmap_x = 6\nmap_y = 6\nlandscape = {climate}\ngameplay_profile = {profile}\n"
                "[network]\nserver_advertise = false\nserver_bind_addresses = 127.0.0.1\n" + extra,
                encoding="utf-8",
            )
            config_proxy.write_bytes(config.read_bytes())
            script.write_text(
                "# @see r2mlvs1-gameplay-contracts-and-content.md, Group 4 activation fixture.\n"
                f'dump_info cargotypes\nsave "{saved.with_suffix("").as_posix()}"\nquit\n',
                encoding="utf-8",
            )
            startup.write_text(f'exec "{script.as_posix()}"\n', encoding="utf-8")
            # Refuse missing or altered content before gameplay; preserve the exact built package in every outcome.
            if name == "missing":
                if held_package.exists():
                    raise ValueError("held-package evidence path is already occupied")
                package.rename(held_package)
            if name == "changed":
                package.write_bytes(original_package + b"\n")
            try:
                env = dict(os.environ, OTTD_TIMEOUT="60", OTTD_DEBUG="misc=5,net=2,sl=1,grf=2,script=2")
                env.pop("OTTD_MULTILAYER_LOADER_PROBE", None)
                command = ["C:/Program Files/Git/bin/bash.exe", (area / "tools/run_dedicated.sh").as_posix(),
                           (root / "build").as_posix(), "Debug", log.as_posix(),
                           "-X", "-c", config_proxy.name, "-G", "1234"]
                subprocess.run(command, check=True, env=env, timeout=70)
                evidence = log.read_text(encoding="utf-8", errors="replace")
                if refusal is not None:
                    assert refusal in evidence, f"missing specific refusal for {name}"
                    assert not saved.exists(), f"unsupported {name} world was accepted"
                    print(f"{name}: refused explicitly", flush=True)
                    continue
                assert saved.is_file(), f"supported {name} world was not saved"
                actual = {label: int(slot) for slot, label in re.findall(
                    r"\s(\d{2}) Bit:\s*\d+, Label: ([A-Z_]{4}),", evidence)}
                expected = expected_classic if name == "classic" else expected_multilayer
                assert actual == expected, (name, actual, expected)
                expected_profile = "id=0 version=0" if name == "classic" else "id=1 version=1"
                assert "MLVS_PROFILE " + expected_profile in evidence
                results[name] = actual
                print(f"{name}: {len(actual)} active cargo meanings; {expected_profile}", flush=True)
            finally:
                if name == "missing":
                    held_package.rename(package)
                if name == "changed":
                    package.write_bytes(original_package)
    finally:
        startup.write_bytes(original_script)
        config_proxy.write_bytes(original_config)
        # Keep evidence and saves; disposable generated console source is reconstructed by this harness.
        for name, *_ in cases:
            (fixtures / f"r2mlvs1-c-{name}.scr").unlink(missing_ok=True)
    (area / "r2mlvs1-c-profile-results.txt").write_text(
        "R2MLVS1 Group 4: dedicated profile activation and four refusal cases\n"
        + json.dumps(results, indent=2) + "\n", encoding="utf-8")
    return results


def main():
    """Write the bounded repository-local probe package.

    @return Process exit code; invalid destinations raise before any write.
    """
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", type=Path)
    parser.add_argument("--run", action="store_true", help="run two focused dedicated round trips")
    parser.add_argument("--profiles", action="store_true", help="run profile activation and refusal fixtures")
    args = parser.parse_args()
    if args.profiles:
        run_profiles(root)
        return 0
    if args.run:
        run_roundtrips(root)
        return 0
    if args.out is None:
        parser.error("--out, --run or --profiles is required")
    target = args.out.resolve()
    # The spike must not install or overwrite user-wide packages while testing delivery choices.
    if not target.is_relative_to(root):
        raise ValueError("probe output must remain inside the repository")
    target.parent.mkdir(parents=True, exist_ok=True)
    data = package_bytes()
    target.write_bytes(data)
    print(f"{target}: {len(data)} bytes, GRFID MLVP")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
