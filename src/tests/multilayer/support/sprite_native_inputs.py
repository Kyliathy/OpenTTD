"""Bind explicit source sprite expectations to one actual native configuration.

@file sprite_native_inputs.py No native execution, cache fallback or source publication.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups18B/18C.
The shared runner records the exact non-save input supplied to the child process.
"""

import json
from pathlib import Path

from artifact_paths import contained
from artifact_rebuild import identity
from sprite_expected_payload import checked_payload, recipe_family, selected_path

PREFIX = "OTTD_TEST_EXPECTED_SPRITES"
FIELDS = (PREFIX, PREFIX + "_SHA256", PREFIX + "_FAMILY", PREFIX + "_CONFIGURATION",
          PREFIX + "_MANIFEST", PREFIX + "_MANIFEST_SHA256")
CANDIDATE_PREFIX = "OTTD_TEST_CANDIDATE_BASESET"
CANDIDATE_FIELDS = (CANDIDATE_PREFIX, CANDIDATE_PREFIX + "_SHA256")


def bind_candidate_baseset(context, binding, configuration, environment=None):
    """Bind an already prepared process-local candidate independently of expected pixels.

    @param context Actual consuming process/run owner.
    @param binding Explicit manifest returned by this context's runtime preparation owner.
    @param configuration Actual independently compiled native configuration.
    @param environment Optional existing expectation-bound environment.
    @return Checked child environment; actual loading remains the native decoder's observation.
    """
    path = contained(binding, context.data)
    result = context.environment(environment)
    if result.get("OTTD_TEST_CONFIGURATION") not in (None, configuration):
        raise ValueError("candidate configuration conflicts with the existing native binding")
    result.update({CANDIDATE_PREFIX: str(path), CANDIDATE_PREFIX + "_SHA256": identity(path, context.root)["sha256"],
                   "OTTD_TEST_CONFIGURATION": configuration})
    observe_candidate_baseset(context, result, configuration)
    return result


def observe_candidate_baseset(context, environment, configuration):
    """Verify the actual candidate, checksum, descriptor and original base files around one child.

    @param context Exact native process whose runtime copies were prepared.
    @param environment Explicit complete candidate-manifest binding; no default candidate is inferred.
    @param configuration Actual executable configuration inferred by the existing runner.
    @return Separate actual candidate input/provenance record, or None for a non-candidate process.
    """
    if not any(key in environment for key in CANDIDATE_FIELDS):
        return None
    if not all(environment.get(key) for key in CANDIDATE_FIELDS):
        raise ValueError("candidate baseset needs its complete manifest identity")
    path = contained(environment[CANDIDATE_PREFIX], context.data)
    reference = identity(path, context.root)
    if reference["sha256"] != environment[CANDIDATE_PREFIX + "_SHA256"]:
        raise ValueError("candidate binding manifest changed")
    document = json.loads(path.read_text(encoding="utf-8"))
    # A prepared runtime never certifies itself as loaded. Run/process/build
    # identities stay independent of the reusable expected source pixels.
    if (document.get("kind") != "candidate_baseset_binding" or document.get("schema_version") != 1
            or document.get("run_id") != context.run_id or document.get("process") != context.process
            or configuration not in ("Debug", "RelWithDebInfo") or document.get("configuration") != configuration
            or environment.get("OTTD_TEST_CONFIGURATION") != configuration
            or document.get("runtime") != context.runtime_dir.relative_to(context.root).as_posix()
            or document.get("native_loading_verified") is not False or document.get("publishes_source") is not False):
        raise ValueError("candidate baseset run/process/configuration/status disagrees")
    for source_key, runtime_key, basename in (("candidate_source", "runtime_grf", "openttd.grf"),
                                             ("checksum_source", "runtime_checksum", "openttd.grf.hash")):
        source = document[source_key]
        source_path = contained(context.root / source["path"], context.root / "mlm/reviews")
        parts = source_path.relative_to(context.root / "mlm/reviews").parts
        runtime = document[runtime_key]
        runtime_path = contained(context.root / runtime["path"], context.runtime_dir / "baseset")
        # Actual source bytes and this process's exact copied basename are one
        # input. Equal expected pixels cannot excuse a foreign or changed GRF.
        if (len(parts) != 5 or parts[2] != context.run_id or parts[3] != "data" or source_path.name != basename
                or runtime_path != context.runtime_dir / "baseset" / basename
                or identity(source_path, context.root) != source or identity(runtime_path, context.root) != runtime
                or source["sha256"] != runtime["sha256"]):
            raise ValueError("candidate source/runtime/companion identity disagrees")
    expected = {"orig_dos.obg", "trg1.grf", "trgc.grf", "trgh.grf", "trgi.grf", "trgt.grf"}
    if {Path(row["path"]).name.lower() for row in document["companions"]} != expected:
        raise ValueError("candidate baseset descriptor/original companion set disagrees")
    for row in document["companions"]:
        companion = contained(context.root / row["path"], context.runtime_dir / "baseset")
        if identity(companion, context.root) != row:
            raise ValueError("candidate baseset descriptor/original companion changed")
    return dict(role="candidate_grf", configuration=configuration, **document["candidate_source"],
                checksum_source=document["checksum_source"], runtime_grf=document["runtime_grf"],
                runtime_checksum=document["runtime_checksum"], source_manifest=reference)


def bind_expected_payload(context, family, configuration, manifest, environment=None):
    """Prepare an exact source payload for one selected native process configuration.

    @param context Explicit consuming native invocation sharing the source prerequisite run.
    @param family Existing source binary protocol family selected by the native test tag.
    @param configuration Actual Debug or RelWithDebInfo build configuration.
    @param manifest Actual current typed source-expectation report identity.
    @param environment Optional existing child environment; contradictory configuration refuses.
    @return Complete child environment, including exact payload/report identities and family.
    """
    if configuration not in ("Debug", "RelWithDebInfo"):
        raise ValueError("sprite native input requires an explicit supported configuration")
    recipe_family(family)
    result = context.environment(environment)
    # A current process cannot borrow another build's proof while consuming
    # otherwise reusable source bytes. The binding fixes one actual configuration.
    if result.get("OTTD_TEST_CONFIGURATION") not in (None, configuration):
        raise ValueError("sprite native input configuration conflicts with the child environment")
    payload = checked_payload(family, manifest, context)
    report = selected_path(manifest, context, "reports")
    reference = identity(payload, context.root)
    result.update({PREFIX: str(payload), PREFIX + "_SHA256": reference["sha256"],
        PREFIX + "_FAMILY": family, PREFIX + "_CONFIGURATION": configuration,
        PREFIX + "_MANIFEST": str(report), PREFIX + "_MANIFEST_SHA256": manifest["sha256"],
        "OTTD_TEST_CONFIGURATION": configuration})
    return result


def observe_expected_payload(context, environment, configuration):
    """Validate one exact non-save sprite input immediately before or after native execution.

    @param context Actual native invocation owner.
    @param environment Exact environment supplied to the child, after shared routing fields.
    @param configuration Actual configuration inferred from the executable or explicit environment.
    @return Bound input/provenance record, or None when no sprite binding was supplied.
    """
    if not any(key in environment for key in FIELDS):
        return None
    if not all(environment.get(key) for key in FIELDS):
        raise ValueError("sprite native input requires a complete explicit payload/report binding")
    # Source bytes may be reused, but each native process must agree with its
    # selected executable configuration before the child starts or proof is sealed.
    if (configuration not in ("Debug", "RelWithDebInfo")
            or environment[PREFIX + "_CONFIGURATION"] != configuration
            or environment.get("OTTD_TEST_CONFIGURATION") != configuration):
        raise ValueError("sprite native input configuration disagrees with the actual executable")
    family = environment[PREFIX + "_FAMILY"]
    recipe_family(family)
    report = contained(environment[PREFIX + "_MANIFEST"], context.root / "mlm/reviews")
    manifest = dict(path=report.relative_to(context.root).as_posix(), sha256=environment[PREFIX + "_MANIFEST_SHA256"])
    payload = checked_payload(family, manifest, context)
    requested = contained(environment[PREFIX], context.root / "mlm/reviews")
    reference = identity(payload, context.root)
    # Payload location, actual bytes and typed source report remain one role.
    # Setting a path-only environment variable cannot substitute another input.
    if requested != payload or environment[PREFIX + "_SHA256"] != reference["sha256"]:
        raise ValueError("sprite native input payload identity disagrees with its source report")
    return dict(role="expected_sprites", family=family, configuration=configuration,
                **reference, source_manifest=manifest)
