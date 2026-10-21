"""Publish only fully decoded and natively loaded selected underground artwork bindings.

@file ground_payload_binding.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79E.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Source records, definition-local IDs and dated predecessor reports remain exact.
The62 prior native families receive only their proven shared-container checksum.
"""

import argparse
import copy
import json
import re

from tools import ground_art_handoff as handoff

ROOT = handoff.ROOT
DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-packing-v1"
PROOF = DIRECTORY / "distribution-proof.json"
PUBLISHED = ROOT / "logs/proofs/mlm/native-art/ground/packed-ground-payload.json"
QUALIFICATION = DIRECTORY / "binding-verification.json"
MARKER = "GROUND_PACKED_ART_LOADED sprites=22018 protected=7764 new=14254 families=47 native_start=7791 native_bound=22045 full_pixel_bytes=exact"
TRANSITIONS = {"CMakeLists.txt", "media/baseset/openttd/CMakeLists.txt", "media/baseset/openttd/openttd.nfo",
               "src/table/sprites.h", "media/baseset/openttd.grf", "media/baseset/openttd.grf.hash",
               "media/multilayer/contracts/multilayer_sprite_manifest.json"}


def reference(path):
    """Identify actual repository bytes without trusting a declared success flag.

    @param path Existing absolute repository artifact.
    @return Relative path and exact SHA256.
    """
    return {"path": path.relative_to(ROOT).as_posix(), "sha256": handoff.digest(path)}


def checked_reference(row):
    """Require an exact current repository artifact for a qualified proof reference.

    @param row Required repository-relative path/SHA256 object.
    @return Absolute verified artifact path.
    """
    path = handoff.bound(row["path"])
    if reference(path) != row:
        raise ValueError("ground packed qualification artifact changed: " + row["path"])
    return path


def native_shape(observation, cached):
    """Reject partial or substituted observation metadata before expensive byte verification.

    @param observation Actual retained native application/process receipt.
    @param cached Required generator-disabled observation status.
    @return None when complete actual-process scope is declared exactly.
    """
    # Only the actual full-byte case can authorize packed identities. Partial
    # coverage, paid-world claims or cached/source observation substitution must
    # refuse before metadata can make them look like accepted runtime delivery.
    if (observation.get("complete") is not True or observation.get("cases") != 1
            or observation.get("assertions") != 462407 or observation.get("exit_code") != 0
            or observation.get("protected_sprites") != 7764 or observation.get("new_sprites") != 14254
            or observation.get("total_sprites") != 22018 or observation.get("native_bound") != 22045
            or observation.get("full_pixel_bytes_exact") is not True or observation.get("anchors_exact") is not True
            or observation.get("paid_world_loaded") is not False
            or observation.get("generator_disabled_observation") is not cached):
        raise ValueError("ground complete actual native observation is absent or partial")


def checked_native(observation, sha, cached):
    """Validate complete real-process native coverage and terminal evidence.

    @param observation Actual retained native application/process receipt.
    @param sha Qualified complete shared container identity.
    @param cached Required generator-disabled observation status.
    @return None after complete current pixel/anchor/process identity checks.
    """
    native_shape(observation, cached)
    for name in ("application", "terminal", "fixture", "expectations", "build_proof", "native_test", "producer",
                 "payload", "copied_payload", "runtime_payload"):
        checked_reference(observation[name])
    if any(observation[name]["sha256"] != sha for name in ("payload", "copied_payload", "runtime_payload")):
        raise ValueError("ground native process used a different shared payload")
    text = handoff.bound(observation["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
    if MARKER not in text or "All tests passed (462407 assertions in 1 test case)" not in text:
        # Actual Catch completion and the full native marker must agree; a
        # caller-supplied boolean cannot replace the real terminal observation.
        raise ValueError("ground native complete-byte terminal evidence is absent")


def public_transition(public, before, scope, report, sha):
    """Prepare exactly47 new bindings and62 proven container-only checksum joins.

    @param public Detached current public manifest, never mutated by this pure validator.
    @param before Frozen complete prepacking public/byte snapshot.
    @param scope Qualified source selection and exact native append order.
    @param report Qualified own source/range report, without a rewritten historical epoch.
    @param sha Current fully qualified shared payload SHA256.
    @return Proposed exact public document and changed family identities.
    """
    proposed = copy.deepcopy(public)
    original = before["public"]
    rows = {row["asset_id"]: row for row in proposed["assets"]}
    frozen = {row["asset_id"]: row for row in original["assets"]}
    if (len(rows) != len(proposed["assets"]) or set(rows) != set(frozen)
            or {key: value for key, value in proposed.items() if key != "assets"}
            != {key: value for key, value in original.items() if key != "assets"}):
        # Graphics publication cannot add/remove gameplay definitions or change
        # root schemas, saved identity envelopes or profile requirements.
        raise ValueError("ground public family set or definition envelope changed")
    prefix = {row["asset_id"] for row in before["native_prefix"]}
    appended = {row["asset_id"]: row for row in report["families"]}
    changed = []
    # Compare complete rows, including authoring/source/geometry and local IDs.
    # The only allowed differences are exact own bindings or a prior shared
    # container checksum, leaving unrelated and deferred families byte-semantic exact.
    for asset_id, previous in frozen.items():
        row = rows[asset_id]
        expected = copy.deepcopy(previous)
        if asset_id in appended:
            native = appended[asset_id]
            expected["packed"].update(route=scope["native_route"], native_offset=native["native_offset"],
                                       sprite_count=native["sprite_count"], source_payload=PUBLISHED.relative_to(ROOT).as_posix(),
                                       payload="media/baseset/openttd.grf", sha256=sha)
        elif asset_id in prefix:
            expected["packed"]["sha256"] = sha
        if row != previous and row != expected:
            # Idempotent publication accepts only the complete old/new row;
            # partial staged metadata cannot hide a changed native range or source.
            raise ValueError("ground unrelated public source/geometry/binding changed: " + asset_id)
        rows[asset_id].clear()
        rows[asset_id].update(expected)
        if expected != previous:
            changed.append(asset_id)
    return proposed, changed


def prepare(proof=None, public=None):
    """Validate complete current build/native/cache/transition facts without writing files.

    @param proof Optional detached distribution proof for focused refusal fixtures.
    @param public Optional detached public manifest for source/ID preservation fixtures.
    @return Own packed handoff document and proposed narrowly scoped public bindings.
    """
    scope, before, public = handoff.read_scope(public)
    proof = handoff.read_json(PROOF.relative_to(ROOT).as_posix()) if proof is None else proof
    cached = proof["generator_free"]
    configurations = {"Debug", "RelWithDebInfo"}
    if (proof.get("passed") is not True or proof.get("native_loading_verified") is not True
            or cached.get("generator_discovery_disabled") is not True or cached.get("native_loading_verified") is not True
            or set(proof.get("native", {})) != configurations or set(cached.get("native", {})) != configurations):
        # Both distinct actual configurations and actual generator-free startup
        # are required; one successful source process cannot qualify distribution.
        raise ValueError("ground current native/generator-disabled qualification is incomplete")
    payload = proof["payload"]
    sha = payload["sha256"]
    report = handoff.read_json(scope["report"])
    proposed, updates = public_transition(public, before, scope, report, sha)
    for configuration in ("Debug", "RelWithDebInfo"):
        native_shape(proof["native"][configuration]["observation"], False)
        native_shape(cached["native"][configuration]["observation"], True)
    build = handoff.read_json(proof["build_proof"]["path"])
    checksum = (ROOT / "media/baseset/openttd.grf.hash").read_text(encoding="utf-8").strip()
    if (len(build["builds"]) != 2 or [row["payload"]["sha256"] for row in build["builds"]] != [sha, sha]
            or [row["grfid_md5"] for row in build["builds"]] != [checksum, checksum]
            or not re.fullmatch(r"[0-9a-f]{32}", checksum) or cached["sha256"] != sha):
        # Identical complete encodings and the format-specific checksum must
        # bind this actual container, never whole-file MD5 or an older payload.
        raise ValueError("ground reproducible complete payload/checksum qualification differs")
    checked_reference(payload)
    checked_reference(proof["producer"])
    checked_reference(proof["build_proof"])
    for row in build["inputs"]:
        checked_reference(row)
    for row in build["builds"]:
        checked_reference(row["payload"])
        checked_reference(row["build"]["log"])
        checked_reference(row["decode"]["log"])
    for name in ("configure", "copy", "restore"):
        checked_reference(cached[name]["log"])
        if cached[name]["exit_code"] != 0:
            raise ValueError("ground actual generator-disabled packaging command failed")
    checked_reference(cached["copied_payload"])
    values = dict(re.findall(r"^([^#/:=]+):[^=]+=(.*)$", (ROOT / "build/CMakeCache.txt").read_text(encoding="utf-8"), re.MULTILINE))
    if values.get("CMAKE_DISABLE_FIND_PACKAGE_Grfcodec") != cached["restored_discovery"]:
        raise ValueError("ground normal generator discovery was not restored")
    invocations = set()
    for configuration in ("Debug", "RelWithDebInfo"):
        primary = proof["native"][configuration]
        fallback = cached["native"][configuration]
        for entry, is_cached in ((primary, False), (fallback, True)):
            checked_reference(entry["receipt"])
            observation = entry["observation"]
            if handoff.read_json(entry["receipt"]["path"]) != observation:
                raise ValueError("ground embedded observation differs from its actual process receipt")
            if observation["configuration"] != configuration:
                raise ValueError("ground native configurations are mislabeled")
            checked_native(observation, sha, is_cached)
            invocations.add(observation["invocation"])
    if len(invocations) != 4:
        raise ValueError("ground cached/native qualifications reused a process invocation")
    transitions = []
    # Only the seven named build/container/public integration paths can have
    # changed. All other source artifacts, old private reports and applications
    # still identify their exact current predecessor bytes.
    for row in before["files"]:
        actual = reference(handoff.bound(row["path"]))
        if actual != row:
            if row["path"] not in TRANSITIONS:
                raise ValueError("ground packed publication changed an unrelated file: " + row["path"])
            transitions.append({"before": row, "after": actual})
    checked_reference(before["predecessor_payload"])
    packed = {"_documentation": __doc__, "schema_version": 1, "route": scope["native_route"],
              "source_manifest": reference(handoff.bound(scope["report"])), "scope": reference(handoff.SCOPE),
              "baseline": reference(handoff.bound(scope["baseline"])),
              "native_start": 7791, "native_sprite_bound": 22045, "pose_count": 14254,
              "families": report["families"], "preserved_family_count": 62,
              "packed_grf": {**payload, "grfid_md5": checksum, "proof": reference(PROOF),
                             "protected_sprites": 7764, "new_sprites": 14254, "total_sprites": 22018,
                             "authorized_prefix_checksum_updates": [row["asset_id"] for row in before["native_prefix"]]},
              "authorized_public_updates": updates, "transitions": transitions,
              "native_loading_verified": True, "runtime_activated": False}
    return packed, proposed


def publish():
    """Publish qualified selected bindings after current focused transition/refusal checks pass.

    @return Own packed handoff; source records and historical prefix reports remain unchanged.
    """
    checks = handoff.read_json(QUALIFICATION.relative_to(ROOT).as_posix())
    if checks.get("passed") is not True or checks.get("cases") != 8:
        # Actual focused corruption/transition checks, not a hand-written
        # success flag, precede publication of the new shared-container bindings.
        raise ValueError("ground focused packed-binding qualification is absent")
    for row in checks["inputs"]:
        checked_reference(row)
    packed, public = prepare()
    PUBLISHED.write_text(json.dumps(packed, indent=2) + "\n", encoding="utf-8")
    handoff.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")
    final, repeated = prepare()
    if repeated != public:
        raise ValueError("ground actual published document differs from its qualified transition")
    # Retain the actual after-publication identities and complete seven-path
    # transition audit separately from the unchanged source-conversion epoch.
    publication = {"_documentation": __doc__, "passed": True, "public": reference(handoff.PUBLIC),
                   "packed_handoff": reference(PUBLISHED), "qualification": reference(QUALIFICATION),
                   "producer": reference(handoff.bound("media/multilayer/tools/ground_payload_binding.py")),
                   "transitions": final["transitions"], "new_families": 47, "prefix_checksum_joins": 62,
                   "native_loading_verified": True, "runtime_activated": False}
    (DIRECTORY / "publication.json").write_text(json.dumps(publication, indent=2) + "\n", encoding="utf-8")
    return packed


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--publish", action="store_true", help="Publish only after current focused qualification")
    args = parser.parse_args()
    result = publish() if args.publish else prepare()[0]
    print(json.dumps({key: result[key] for key in ("pose_count", "native_start", "native_sprite_bound", "native_loading_verified")}))
