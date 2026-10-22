"""Focused reproducible-build and generator-free actual-loader checks for M1 art.

@file multilayer_packed_build.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.6
Run each mode separately. All logs, configuration and generated data stay in the
repository. The generator-free mode restores the original CMake configuration.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import pack_m1_art as art

PROOF = ROOT / "media/multilayer/sprites/m1/packed-build-proof.json"
GRF = ROOT / "media/baseset/openttd.grf"


def digest(path):
    """Fingerprint a complete distributed artifact.

    @param path Existing repository-owned artifact.
    @return Lowercase SHA256 digest of all file bytes.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def execute(arguments, name, directory=ROOT):
    """Execute one bounded native build or isolated loader, retaining its evidence.

    @param arguments Exact subprocess argument vector, without shell interpolation.
    @param name Owning-plan log basename inside the repository logs directory.
    @param directory Repository working directory needed by the native tool.
    @return Complete captured diagnostic text; failed or overdue commands refuse.
    """
    path = ROOT / "logs" / name
    with path.open("wb") as sink:
        subprocess.run(arguments, cwd=directory, stdout=sink, stderr=subprocess.STDOUT,
                       creationflags=subprocess.CREATE_NO_WINDOW, check=True, timeout=110)
    return path.read_text(encoding="utf-8", errors="replace")


def read_proof():
    """Read the bounded retained evidence record, if a prior mode supplied it.

    @return Current proof fields with an explicit owner header.
    """
    if PROOF.exists():
        return json.loads(PROOF.read_text(encoding="utf-8"))
    return {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B; actual build/load evidence, runtime bindings pending"}


def write_proof(proof):
    """Publish evidence only after all checks for the executing mode succeed.

    @param proof Complete read-only source/build observations for this mode.
    @return None; writes the repository-owned packing evidence record.
    """
    PROOF.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


def reproducible(cmake, config, prefix):
    """Compare two clean source derivations and two actual native GRF builds.

    @param cmake Verified local CMake executable path.
    @param config Existing completed native configuration.
    @param prefix Repository log prefix preserving earlier evidence.
    @return None; retain matching source/payload/checksum hashes on success.
    """
    cache = (ROOT / "build/CMakeCache.txt").read_text()
    versions = {}
    version_output = []
    # Verify executable banners rather than trusting directory labels or cached
    # version strings; both clean conversions must use the pinned toolchain.
    for key, name, arguments in [("GRFCODEC_EXECUTABLE", "GRFCodec", []), ("NFORENUM_EXECUTABLE", "NFORenum", ["--version"])]:
        executable = re.search(r"^" + key + r":FILEPATH=(.+)$", cache, re.MULTILINE).group(1)
        result = subprocess.run([executable, *arguments], cwd=ROOT / "build", capture_output=True, text=True,
                                creationflags=subprocess.CREATE_NO_WINDOW, timeout=10)
        text = result.stdout + result.stderr
        assert name + " 6.2.0" in text, "native packing tool differs from pinned version: " + name
        versions[name] = "6.2.0"
        version_output.append(text)
    (ROOT / f"logs/{prefix}-tool-versions.log").write_text("\n".join(version_output), encoding="utf-8")
    first_report, first_files = art.derive()
    second_report, second_files = art.derive()
    assert first_report == second_report, "clean selected-source metadata differs"
    assert first_files == second_files, "clean selected-source conversion differs"
    # The source converter writes both independent inputs anew. Their mtimes
    # require the normal build graph to rerun NFORenum and GRFCodec each time.
    hashes, checksums, logs = [], [], []
    for attempt in (1, 2):
        for path, content in first_files.items():
            path.write_bytes(content)
        name = f"{prefix}-repro-pack-{attempt}.log"
        output = execute([cmake, "--build", "build", "--config", config, "--target", "openttd.grf", "-j", "1", "--", "/p:CL_MPCount=2", "/nodeReuse:false"], name)
        assert "Generating openttd.grf" in output, "native generator did not run"
        hashes.append(digest(GRF))
        checksums.append(Path(str(GRF) + ".hash").read_text().strip())
        logs.append("logs/" + name)
    assert hashes[0] == hashes[1], "actual clean GRF payload differs"
    assert checksums[0] == checksums[1], "GRFID container checksum differs"
    proof = read_proof()
    proof.update(reproducible={"sha256": hashes, "grfid_md5": checksums, "logs": logs, "tool_versions": versions,
                              "source_outputs": {str(path.relative_to(ROOT)).replace("\\", "/"): hashlib.sha256(data).hexdigest()
                                                 for path, data in first_files.items()}})
    write_proof(proof)
    print("Two clean selected conversions and actual GRF builds are byte-identical", flush=True)


def generator_free(cmake, config, prefix):
    """Build and actually load cached artwork with package discovery disabled.

    @param cmake Verified local CMake executable path.
    @param config Existing completed native configuration.
    @param prefix Repository log prefix preserving earlier evidence.
    @return None; retain successful cached native load and restore generator discovery.
    """
    before = digest(GRF)
    # Package discovery is disabled through CMake's public switch. Cached tool
    # locations can remain present, but no native generator target may execute.
    try:
        execute([cmake, "-S", ".", "-B", "build", "-DCMAKE_DISABLE_FIND_PACKAGE_Grfcodec=ON"],
                f"{prefix}-generator-free-configure.log")
        project = ROOT / "build/media/baseset/baseset_files.vcxproj"
        assert "CreateGRF.cmake" not in project.read_text(), "cached packaging still invokes the generator"
        # Remove only the disposable build copy so the cached source-copy rule
        # must execute; an already present binary cannot impersonate packaging.
        (ROOT / "build/baseset/openttd.grf").unlink(missing_ok=True)
        # The repository's add_custom_command_timestamp tracks this owned
        # copy stamp, not the copied payload. Invalidate both so an already
        # built configuration really exercises cached packaging again.
        (ROOT / "build/media/baseset/openttd.grf.timestamp").unlink(missing_ok=True)
        output = execute([cmake, "--build", "build", "--config", config, "--target", "baseset_files", "-j", "1", "--", "/p:CL_MPCount=2", "/nodeReuse:false"],
                         f"{prefix}-generator-free-build.log")
        assert "Generating openttd.grf" not in output, "generator executed during cached build"
        assert digest(GRF) == before, "generator-free build changed distributed source payload"
        assert digest(ROOT / "build/baseset/openttd.grf") == before, "cached packaging copied stale artwork"
        name = f"{prefix}-generator-free-load.log"
        loaded = execute([str(ROOT / "build" / config / "openttd_test.exe"), "[.m1-packed-art-load]"], name, ROOT / "build")
        assert "M1_PACKED_ART_LOADED sprites=2409 native_bound=2436 protected=66 selected=2343 pixel_hashes=exact" in loaded
        assert "All tests passed" in loaded, "native pixel/anchor observer did not pass"
        proof = read_proof()
        proof.update(generator_free={"sha256": before, "selected_sprites": 2343, "protected_sprites": 66,
                                     "native_bound": 2436, "loader_log": "logs/" + name,
                                     "generator_discovery_disabled": True})
        write_proof(proof)
        print("Generator-free checked-in packaging loads all 2409 source-exact native sprites", flush=True)
    finally:
        execute([cmake, "-S", ".", "-B", "build", "-DCMAKE_DISABLE_FIND_PACKAGE_Grfcodec=OFF"],
                f"{prefix}-generator-restore.log")


def main():
    """Dispatch one focused proof mode to keep native checks below two minutes.

    @return None; each mode records only successful independent evidence.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("mode", choices=["reproducible", "generator-free"])
    parser.add_argument("--cmake", required=True)
    parser.add_argument("--config", default="RelWithDebInfo")
    parser.add_argument("--prefix", default="r2tbmf-z-art")
    args = parser.parse_args()
    assert Path(args.prefix).name == args.prefix
    if args.mode == "reproducible":
        reproducible(args.cmake, args.config, args.prefix)
    else:
        generator_free(args.cmake, args.config, args.prefix)


if __name__ == "__main__":
    main()
