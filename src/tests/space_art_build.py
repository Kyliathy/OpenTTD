"""Bounded final Space GRF reproducibility, cached packaging and actual decoder proof.

@file space_art_build.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG
Only the seven accepted Space sources are derived; earlier M0/M1 source reports
remain dated producer evidence and their original decoded pixels are checked.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import struct
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import pack_space_art as art
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context

CONTEXT = current_run_context("space-art", "packing")
PROOF = ROOT / "logs/proofs/mlm/native-art/space/packed-space-build-proof.json"
GRF = ROOT / "media/baseset/openttd.grf"


def digest(path):
    """Fingerprint an actual repository artifact.

    @param path Existing source, payload or native executable.
    @return Whole-file SHA256.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def execute(arguments, name, directory=ROOT):
    """Retain one bounded subprocess result beneath the owning repository logs.

    @param arguments Exact native tool argument vector, without shell expansion.
    @param name Basename for this independently inspectable log.
    @param directory Required repository working directory.
    @return Captured diagnostics after successful completion within110 seconds.
    """
    path = CONTEXT.logs / name
    environment = CONTEXT.environment()
    environment["CL_MPCount"] = "2"
    with path.open("wb") as sink:
        subprocess.run(arguments, cwd=directory, env=environment, stdout=sink, stderr=subprocess.STDOUT,
                       creationflags=subprocess.CREATE_NO_WINDOW, check=True, timeout=110)
    return path.read_text(encoding="utf-8", errors="replace")


def read_proof():
    """Load this append's independent evidence without changing earlier producer records.

    @return Existing successful modes, or an explicitly owned empty record.
    """
    if PROOF.exists():
        return json.loads(PROOF.read_text(encoding="utf-8"))
    return {"_documentation": "Owner: r2mlvs10-asteroids-and-mining-probes.md Group14BG; final seven-family append; ordinary-game review remains14BH."}


def write_proof(proof):
    """Publish only evidence from a completed successful mode.

    @param proof Current original-source/payload/native observations.
    """
    PROOF.parent.mkdir(parents=True, exist_ok=True)
    PROOF.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


def build(cmake, config, target, name):
    """Execute one actual GRF or cached-copy build with the user's compiler bound.

    @param cmake Verified local CMake executable.
    @param config Existing native configuration.
    @param target Existing generator or packaging target.
    @param name Diagnostic log basename.
    @return Actual build diagnostics.
    """
    return execute([cmake, "--build", "build", "--config", config, "--target", target, "--parallel", "2",
                    "--", "/p:CL_MPCount=2", "/nodeReuse:false"], name)


def reproducible(cmake, config):
    """Compare two independent accepted-source conversions and actual GRF builds.

    @param cmake Verified local CMake executable.
    @param config Existing native configuration.
    """
    cache = (ROOT / "build/CMakeCache.txt").read_text(encoding="utf-8")
    # Executable banners, rather than directory names, establish the actual pinned packer versions for both payload builds.
    for key, banner, arguments in [("GRFCODEC_EXECUTABLE", "GRFCodec 6.2.0", []), ("NFORENUM_EXECUTABLE", "NFORenum 6.2.0", ["--version"])]:
        executable = re.search(r"^" + key + r":FILEPATH=(.+)$", cache, re.MULTILINE).group(1)
        result = subprocess.run([executable, *arguments], cwd=ROOT / "build", capture_output=True, text=True,
                                creationflags=subprocess.CREATE_NO_WINDOW, timeout=10)
        assert banner in result.stdout + result.stderr, "unexpected actual packer version"
        (CONTEXT.logs / (key.lower() + ".log")).write_text(result.stdout + result.stderr, encoding="utf-8")
    first, files = art.derive()
    second, other_files = art.derive()
    assert first == second, "independent source metadata differs"
    assert files == other_files, "independent conversions differ"
    hashes, checksums, logs = [], [], []
    # Refresh only this append's PNG/NFO so both native generator invocations really run. An unchanged native range header must not trigger another C++ build.
    for attempt in (1, 2):
        for path, content in files.items():
            if path.suffix == ".h":
                assert path.read_bytes() == content
            else:
                path.write_bytes(content)
        name = f"space-repro-{attempt}.log"
        assert "Generating openttd.grf" in build(cmake, config, "openttd.grf", name)
        hashes.append(digest(GRF))
        checksums.append(Path(str(GRF) + ".hash").read_text().strip())
        logs.append((CONTEXT.logs / name).relative_to(ROOT).as_posix())
    assert hashes[0] == hashes[1], "actual native GRF builds differ"
    assert checksums[0] == checksums[1], "actual GRFID checksums differ"
    proof = read_proof()
    proof["reproducible"] = dict(sha256=hashes, grfid_md5=checksums, logs=logs,
                                source_outputs={p.relative_to(ROOT).as_posix(): hashlib.sha256(b).hexdigest() for p, b in files.items()})
    write_proof(proof)
    print("Two independent final Space conversions and actual GRF builds match", flush=True)


def generator_free(cmake, config):
    """Force generator-disabled packaging to copy the checked-in exact payload.

    @param cmake Verified local CMake executable.
    @param config Existing native configuration.
    """
    sha = digest(GRF)
    try:
        execute([cmake, "-S", ".", "-B", "build", "-DCMAKE_DISABLE_FIND_PACKAGE_Grfcodec=ON"], "space-cached-configure.log")
        assert "CreateGRF.cmake" not in (ROOT / "build/media/baseset/baseset_files.vcxproj").read_text()
        # Remove only these disposable repository build copies and their copy-rule stamp, so stale output cannot impersonate a generator-free packaging operation.
        for relative in ("build/baseset/openttd.grf", "build/media/baseset/openttd.grf.timestamp"):
            (ROOT / relative).unlink(missing_ok=True)
        output = build(cmake, config, "baseset_files", "space-cached-copy.log")
        assert "Generating openttd.grf" not in output
        assert digest(GRF) == sha
        assert digest(ROOT / "build/baseset/openttd.grf") == sha
        proof = read_proof()
        proof["generator_free_packaging"] = dict(sha256=sha, generator_discovery_disabled=True,
                                                copied_payload="build/baseset/openttd.grf",
                                                log=(CONTEXT.logs / "space-cached-copy.log").relative_to(ROOT).as_posix())
        write_proof(proof)
        print("Generator-disabled packaging copies the exact checked-in final payload", flush=True)
    finally:
        execute([cmake, "-S", ".", "-B", "build", "-DCMAKE_DISABLE_FIND_PACKAGE_Grfcodec=OFF"], "space-generator-restore.log")


def native(config, mode):
    """Run one focused selector or actual decoder boundary in its own native process.

    @param config Already compiled native configuration.
    @param mode Select, load-space or load-prefix boundary; no broad regression.
    """
    executable = ROOT / "build" / config / "openttd_test.exe"
    filters = {"select": "[space-art]", "load-space": "[.space-packed-art-load]", "load-prefix": "[.m1-packed-art-load]"}
    sha = digest(GRF)
    # Decoder acceptance must use the exact retained generator-disabled copy; a newly generated or replaced payload cannot inherit earlier packaging evidence.
    if mode != "select":
        cached = read_proof()["generator_free_packaging"]
        assert cached["sha256"] == sha
        assert digest(ROOT / cached["copied_payload"]) == sha
    CONTEXT.prepare_runtime()
    startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
    original = startup.read_bytes() if startup.exists() else None
    try:
        startup.write_text("# @see r2mlvs10-asteroids-and-mining-probes.md14BG; this isolated native observer owns startup.\n", encoding="utf-8")
        name = f"space-{mode}-{config}.log"
        output = execute([str(executable), filters[mode]], name, ROOT / "build")
        assert "All tests passed" in output, "native boundary did not pass"
        if mode == "load-space":
            assert "SPACE_PACKED_ART_LOADED sprites=1579 final=1568 protected_preview=11 native_start=3588 native_bound=5167 pixel_hashes=exact" in output
        if mode == "load-prefix":
            assert "M1_PACKED_ART_LOADED" in output
        assert digest(GRF) == sha
        proof = read_proof()
        proof.setdefault("native", {}).setdefault(config, {})[mode] = dict(sha256=sha, executable_sha256=digest(executable),
                                                                         log=(CONTEXT.logs / name).relative_to(ROOT).as_posix())
        write_proof(proof)
        print(f"{config}: {mode} passed", flush=True)
    finally:
        if original is None:
            startup.unlink(missing_ok=True)
        else:
            startup.write_bytes(original)


def bind():
    """Publish current packed identity only after both exact native boundaries pass.

    Existing source provenance and gameplay definition offsets remain with their
    original owners. The old decoded prefix authorizes only a new container hash,
    never a refresh of the historical source graph or ordinary-game acceptance.
    """
    report, files = art.derive()
    proof = read_proof()
    sha = digest(GRF)
    checksum = Path(str(GRF) + ".hash").read_text().strip()
    assert proof["reproducible"]["sha256"] == [sha, sha]
    assert proof["reproducible"]["grfid_md5"] == [checksum, checksum]
    assert re.fullmatch("[0-9a-f]{32}", checksum)
    assert proof["generator_free_packaging"]["sha256"] == sha
    assert proof["generator_free_packaging"]["generator_discovery_disabled"]
    assert digest(ROOT / "build/baseset/openttd.grf") == sha
    # Every distributed append input must still match both clean actual builds; an edited sheet or range header cannot inherit an earlier decoded result.
    for path, content in files.items():
        assert path.read_bytes() == content
        assert proof["reproducible"]["source_outputs"][path.relative_to(ROOT).as_posix()] == hashlib.sha256(content).hexdigest()
    # Both current binaries independently prove pure selectors and the actual old/new decoded boundaries. Replaced binaries or edited diagnostic logs refuse publication.
    for config in ("RelWithDebInfo", "Debug"):
        executable = ROOT / "build" / config / "openttd_test.exe"
        for mode in ("select", "load-space", "load-prefix"):
            observation = proof["native"][config][mode]
            assert observation["sha256"] == sha
            assert observation["executable_sha256"] == digest(executable)
            output = (ROOT / observation["log"]).read_text(encoding="utf-8", errors="replace")
            assert "All tests passed" in output
            if mode == "load-space":
                assert "SPACE_PACKED_ART_LOADED sprites=1579 final=1568 protected_preview=11 native_start=3588 native_bound=5167 pixel_hashes=exact" in output
            if mode == "load-prefix":
                assert "M1_PACKED_ART_LOADED" in output
    fixture = (ROOT / "logs/proofs/mlm/native-art/expected/m1-packed-sprites.bin").read_bytes()
    count, bound = struct.unpack_from("<HH", fixture, 8)
    assert fixture[:8] == b"MLVART1\0"
    assert bound == 3588
    protected = {struct.unpack_from("<H", fixture, 12 + i * 14)[0] for i in range(count)}
    public = json.loads(art.PUBLIC.read_text(encoding="utf-8"))
    rows = {row["asset_id"]: row for row in report["families"]}
    refreshed = []
    # The native prefix decoder proves old pixels/anchors unchanged. Refresh only each covered row's whole-container fingerprint; its immutable source report, IDs and offsets stay original.
    for family in public["assets"]:
        packed = family["packed"]
        if family["asset_id"] in rows:
            row = rows[family["asset_id"]]
            packed.update(payload="media/baseset/openttd.grf", sha256=sha, native_offset=row["native_offset"], sprite_count=row["sprite_count"])
            family["runtime"]["status"] = "native_selection_bound_pending_ordinary_game_review"
        elif packed.get("payload") == "media/baseset/openttd.grf":
            offsets = set(range(packed["native_offset"], packed["native_offset"] + packed["sprite_count"]))
            assert offsets <= protected, "old container fingerprint has an unverified native range"
            refreshed.append(dict(asset_id=family["asset_id"], prior_sha256=packed["sha256"]))
            packed["sha256"] = sha
    report["packed_grf"] = dict(path="media/baseset/openttd.grf", sha256=sha, grfid_md5=checksum,
                                proof=PROOF.relative_to(ROOT).as_posix(), final_sprites=1568,
                                protected_decoded_sprites=count + 11, native_bound=5167)
    proof["bound_payload"] = dict(sha256=sha, prefix_container_hash_updates=refreshed,
                                 source_provenance_unchanged=True, ordinary_game_review_complete=False)
    (ROOT / "logs/proofs/mlm/native-art/space/packed-space-manifest.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    art.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
    write_proof(proof)
    print(f"Bound seven final Space families and current container hash for {len(refreshed)} unchanged decoded prefix families; ordinary-game review remains14BH", flush=True)


def main():
    """Dispatch separate bounded proof modes; native compilation is a consolidated prior batch."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("mode", choices=["reproducible", "generator-free", "select", "load-space", "load-prefix", "bind"])
    parser.add_argument("--config", default="RelWithDebInfo")
    parser.add_argument("--cmake")
    args = parser.parse_args()
    if args.mode == "reproducible":
        reproducible(args.cmake, args.config)
    elif args.mode == "generator-free":
        generator_free(args.cmake, args.config)
    elif args.mode == "bind":
        bind()
    else:
        native(args.config, args.mode)


if __name__ == "__main__":
    main()
