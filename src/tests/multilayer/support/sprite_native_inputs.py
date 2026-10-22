"""Bind explicit source sprite expectations to one actual native configuration.

@file sprite_native_inputs.py No native execution, cache fallback or source publication.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups18B/18C.
The shared runner records the exact non-save input supplied to the child process.
"""

from artifact_paths import contained
from artifact_rebuild import identity
from sprite_expected_payload import checked_payload, recipe_family, selected_path

PREFIX = "OTTD_TEST_EXPECTED_SPRITES"
FIELDS = (PREFIX, PREFIX + "_SHA256", PREFIX + "_FAMILY", PREFIX + "_CONFIGURATION",
          PREFIX + "_MANIFEST", PREFIX + "_MANIFEST_SHA256")


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
