"""Qualify checked-in underground payload packaging with GRF generation disabled.

@file ground_payload_distribution_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79E.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Use the actual CMake cached route and two separate60second native observations.
Restore the user's original generator-discovery setting even when a check fails.
"""

import argparse
import json
import re
import sys

from ground_pack_check_support import ROOT, handoff
from ground_payload_build_checks import PROOF, execute, identity, preserved
from ground_payload_fixture import checked_build
import ground_native_payload_checks as native

OUTPUT = PROOF.parent / "distribution-proof.json"


def cache():
    """Read actual configured generator discovery without changing the build tree.

    @return CMake cache key/value mapping, including an absent discovery override.
    """
    return dict(re.findall(r"^([^#/:=]+):[^=]+=(.*)$", (ROOT / "build/CMakeCache.txt").read_text(encoding="utf-8"), re.MULTILINE))


def main():
    """Copy and load the exact current checked-in payload without invoking its generator.

    @return None; packed public metadata is not published by packaging qualification.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", required=True, help="Repository logs directory for exact packaging/native results")
    args = parser.parse_args()
    output = handoff.bound(args.run)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("ground distribution diagnostics belong in repository logs")
    output.mkdir(parents=True, exist_ok=True)
    checked_build()
    _scope, before, _public = handoff.read_scope()
    preserved(before)
    values = cache()
    cmake, setting = values["CMAKE_COMMAND"], "CMAKE_DISABLE_FIND_PACKAGE_Grfcodec"
    previous = values.get(setting)
    payload = identity(ROOT / "media/baseset/openttd.grf")
    metadata = None
    try:
        configured = execute([cmake, "-S", str(ROOT), "-B", str(ROOT / "build"), "-D" + setting + "=ON"],
                             ROOT, output / "cached-configure.log")
        project = ROOT / "build/media/baseset/baseset_files.vcxproj"
        if cache().get(setting) != "ON" or "CreateGRF.cmake" in project.read_text(encoding="utf-8"):
            # Disabled discovery must remove encoding from the actual target;
            # cached executable paths alone do not prove generator-free delivery.
            raise ValueError("ground generator-disabled packaging still contains encoding work")
        # Invalidate only the exact disposable build copy and its copy stamp.
        # Resolve and check every target before deletion; tracked source and
        # retained predecessor/qualified copies are never candidates here.
        temporary = [(ROOT / relative).resolve() for relative in
                     ("build/baseset/openttd.grf", "build/media/baseset/openttd.grf.timestamp")]
        if any(not path.is_relative_to((ROOT / "build").resolve()) for path in temporary):
            raise ValueError("ground packaging copy invalidation escapes the build owner")
        for path in temporary:
            path.unlink(missing_ok=True)
        copied = execute([cmake, "--build", str(ROOT / "build"), "--config", "RelWithDebInfo", "--target", "baseset_files",
                          "--", "/nodeReuse:false"], ROOT, output / "cached-copy.log")
        text = (output / "cached-copy.log").read_text(encoding="utf-8", errors="replace")
        if ("Copying openttd.grf baseset file" not in text or "Generating openttd.grf" in text
                or identity(ROOT / "media/baseset/openttd.grf") != payload
                or identity(ROOT / "build/baseset/openttd.grf")["sha256"] != payload["sha256"]):
            # A real copy rule must preserve this exact qualified container;
            # stale build data or regeneration cannot count as cached packaging.
            raise ValueError("ground generator-disabled packaging did not copy the qualified source payload")
        observations = {}
        for configuration in ("Debug", "RelWithDebInfo"):
            directory = output / configuration.lower()
            directory.mkdir(parents=True, exist_ok=True)
            command = execute([sys.executable, str(ROOT / "src/tests/multilayer/ground_native_payload_checks.py"), "--mode", "observe",
                               "--configuration", configuration, "--run", directory.relative_to(ROOT).as_posix(), "--cached"],
                              ROOT, directory / "observer-command.log")
            receipt = PROOF.parent / ("cached-native-" + configuration + ".json")
            observations[configuration] = {"receipt": identity(receipt), "command": command,
                                          "observation": handoff.read_json(receipt.relative_to(ROOT).as_posix())}
        metadata = {"generator_discovery_disabled": True, "native_loading_verified": True, "sha256": payload["sha256"],
                    "configure": configured, "copy": copied, "copied_payload": identity(ROOT / "build/baseset/openttd.grf"),
                    "native": observations}
    finally:
        # Exact restoration includes the previous absence of an override, so
        # later normal builds retain the user's original discovery policy.
        arguments = [cmake, "-S", str(ROOT), "-B", str(ROOT / "build")]
        arguments += ["-U", setting] if previous is None else ["-D" + setting + "=" + previous]
        restored = execute(arguments, ROOT, output / "generator-restore.log")
        if cache().get(setting) != previous:
            raise ValueError("ground generator discovery was not restored")
    preserved(before)
    metadata.update(restored_discovery=previous, restore=restored)
    primary = {}
    for configuration in ("Debug", "RelWithDebInfo"):
        receipt = PROOF.parent / ("native-" + configuration + ".json")
        primary[configuration] = {"receipt": identity(receipt), "observation": handoff.read_json(receipt.relative_to(ROOT).as_posix())}
    proof = {"_documentation": __doc__, "schema_version": 1, "passed": True, "producer": identity(native.Path(__file__)),
             "build_proof": identity(PROOF), "payload": payload, "native": primary, "generator_free": metadata,
             "native_loading_verified": True, "runtime_activated": False}
    OUTPUT.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"passed": True, "native_configurations": list(primary), "generator_free": True,
                      "restored_discovery": previous, "sha256": payload["sha256"]}))


if __name__ == "__main__":
    main()
