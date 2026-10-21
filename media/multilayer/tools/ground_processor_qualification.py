"""Qualify current native processor code against the immutable original geometry.

@file ground_processor_qualification.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group41B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original geometry and completed artwork retain their historical identities.
"""
import json
import re

from tools.ground_station_contract import ROOT, digest, source_digest

QUALIFICATION = ROOT / "media/multilayer/contracts/native-processor-current-qualification.json"
BUILD_OWNERS = ("CMakeLists.txt", "src/tests/test_main.cpp", "src/tests/test_artifact_paths.cpp",
                "src/tests/mock_spritecache.cpp", "src/tests/multilayer_bootstrap.cpp",
                "src/tests/processor_fixture.h", "src/tests/mine_fixture.h", "src/tests/excavation_fixture.h")
_verified = {}  # Process-local immutable-file identities, always checked against current metadata.


def check_file(reference, cold=False):
    """Verify an exact repository file, reusing SHA reads only while all metadata agrees.

    @param reference Repository-relative path and exact SHA256 identity.
    @param cold Permit omitted ignored proof artifacts in a source-only checkout.
    @return Whether the actual file is present and byte-verified.
    @throws ValueError For escaped paths, malformed identities or changed evidence.
    """
    path = (ROOT / reference.get("path", "")).resolve()
    expected = reference.get("sha256", "")
    # Evidence must identify an actual repository child and a complete hash;
    # neither a different checkout nor an empty path can authorize native art.
    if not path.is_relative_to(ROOT) or path == ROOT or not re.fullmatch(r"[a-f0-9]{64}", expected):
        raise ValueError("invalid processor qualification file identity")
    if not path.exists():
        if cold:
            return False
        raise ValueError("missing processor qualification authority: " + reference["path"])
    if not path.is_file():
        raise ValueError("processor qualification authority is not a file")
    stat = path.stat()
    token = (stat.st_dev, stat.st_ino, stat.st_size, stat.st_mtime_ns, stat.st_ctime_ns)
    key = (path, expected)
    if _verified.get(key) != token:
        if digest(path) != expected:
            raise ValueError("changed processor qualification evidence: " + reference["path"])
        # A concurrently rewritten proof cannot become an immutable cached
        # authority merely because one read happened to match its old hash.
        after = path.stat()
        if (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns, after.st_ctime_ns) != token:
            raise ValueError("processor evidence changed during verification")
        _verified[key] = token
    return True


def validate_qualification(document, owners, candidate=None):
    """Require a real current seven-case proof of precisely the historical processor contract.

    @param document Original geometry descriptor supplied to its existing validator.
    @param owners Exact existing native source-owner names.
    @param candidate Explicit qualification for focused refusal checks; None reads the published qualification.
    @return Checked current qualification, without changing original geometry or art.
    @throws ValueError For stale owners, foreign geometry, false or changed native proof.
    """
    if candidate is None and not QUALIFICATION.is_file():
        # Changed native owners remain blocked until the separately targeted
        # current executable has actually produced and published its proof.
        raise ValueError("changed native processors require current qualification")
    value = json.loads(QUALIFICATION.read_text(encoding="utf-8")) if candidate is None else candidate
    # An in-progress or differently defined epoch cannot unblock selected
    # originals; source authoring requires the completed exact proof schema.
    if value.get("schema_version") != 1 or value.get("complete") is not True:
        raise ValueError("incomplete current processor qualification")
    descriptor = value.get("historical_descriptor", {})
    if descriptor.get("path") != "media/multilayer/contracts/native-processor-geometry.json":
        raise ValueError("processor qualification names a foreign historical descriptor")
    check_file(descriptor)
    historical = json.loads((ROOT / descriptor["path"]).read_text(encoding="utf-8"))
    # Only this exact immutable descriptor and its complete ordered geometry
    # are qualified. A stale-hash escape cannot authorize altered machinery.
    if historical != document or value.get("families") != historical.get("families"):
        raise ValueError("processor qualification changed historical geometry or owners")
    current = value.get("native_source_sha256", {})
    if set(current) != set(owners):
        raise ValueError("processor qualification source-owner set changed")
    # Every current implementation owner must match the code used by the
    # focused build; an unrelated successful executable cannot qualify edits.
    for name in owners:
        if current[name] != source_digest(ROOT / name):
            raise ValueError("native processor owner changed: " + name)
    build = value.get("build_source_sha256", {})
    if set(build) != set(BUILD_OWNERS):
        raise ValueError("processor qualification build-source set changed")
    for name in BUILD_OWNERS:
        if build[name] != source_digest(ROOT / name):
            raise ValueError("processor qualification build owner changed: " + name)
    proof = value.get("native_proof", {})
    # Every original definition/status/batch case must finish successfully;
    # a convenient partial test filter grants no current source authority.
    if proof.get("exit_code") != 0 or proof.get("test_cases") != 7 or proof.get("assertions", 0) <= 0:
        raise ValueError("incomplete or failed current processor native proof")
    for field in ("binary", "log", "process"):
        check_file(proof.get(field, {}), cold=True)
    # Retained native process identity and the seven-case summary must agree
    # even when ignored proof files are omitted from a cold source checkout.
    if proof.get("process_executable") != proof.get("executed_binary"):
        raise ValueError("processor native executable identity mismatch")
    if proof.get("binary", {}).get("sha256") != proof.get("executed_binary", {}).get("sha256"):
        raise ValueError("retained processor binary differs from executed binary")
    process_path = ROOT / proof["process"]["path"]
    if process_path.exists():
        process = json.loads(process_path.read_text(encoding="utf-8"))
        # The retained report must name the successful actual application,
        # rather than pairing another process with a plausible passing log.
        if process.get("exit_status") != 0 or process.get("executable") != proof["executed_binary"]:
            raise ValueError("processor native process did not execute its qualified application")
    log_path = ROOT / proof["log"]["path"]
    if log_path.exists():
        output = log_path.read_text(encoding="utf-8-sig")
        summary = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", output)
        traces = [json.loads(line.removeprefix("PROCESSOR_ART_GEOMETRY ")) for line in output.splitlines()
                  if line.startswith("PROCESSOR_ART_GEOMETRY ")]
        # Exactly one successful run must cover every original family in
        # native order; partial or concatenated logs cannot promote geometry.
        if (summary is None or int(summary[1]) != proof["assertions"] or int(summary[2]) != 7
                or output.count("All tests passed") != 1 or output.count("NATIVE_EXIT_CODE=") != 1
                or not output.rstrip().endswith("NATIVE_EXIT_CODE=0") or traces != historical["families"]):
            raise ValueError("partial, foreign or failed processor native trace")
    return value
