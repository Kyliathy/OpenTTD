"""Map already native-owned rocket phase progress to read-only original-source poses.

@file cargo_rocket_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62E.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md saved native flight contract.
This is source policy, not a second native record validator or runtime selector.
It never advances simulation, burns fuel, changes custody or exposes Hidden hulls.
"""

from tools import cargo_rocket_art as art

POLICY = art.ROOT / "media/multilayer/source/space/cargo-rocket-state-model-v1.json"
FIXTURE = art.ROOT / "src/tests/multilayer/cargo-rocket-state-fixture.json"


def checked_policy(policy):
    """Bind immutable source choices to actual native producers and ready originals.

    @param policy Explicit immutable model14 source-state/part contract.
    @return None.
    @throws ValueError On stale producer inputs or changed body/part/state geometry.
    """
    expected = art.read_json(FIXTURE)
    # The pre-effect checkpoint is immutable. Later authors bind selected
    # genuine parts in a successor descriptor rather than changing this policy.
    if (art.source.identity(POLICY) != expected["policy"]
            or art.semantic_digest(policy) != expected["policy_semantic_sha256"]
            or policy["ready_qualification"] != expected["ready_qualification"]):
        raise ValueError("immutable rocket phase source policy changed")
    for reference in [policy["ready_qualification"], *policy["native_inputs"]]:
        if art.source.identity(art.ROOT / reference["path"]) != reference:
            raise ValueError("rocket source policy owner/ready input changed")
    # Both real narrow native runs must retain their exact source/binary/log
    # identities. Source mapping cannot bless an uncompiled native phase edit.
    for configuration, reference in policy["native_proofs"].items():
        if art.source.identity(art.ROOT / reference["path"]) != reference:
            raise ValueError("rocket phase native receipt changed")
        proof = art.read_json(art.ROOT / reference["path"])
        if (proof["configuration"] != configuration or not proof["complete"]
                or proof["assertions"] != 183 or proof["cases"] != 7
                or proof["paid_world_loaded"] or proof["simulation_ticks"] != 0
                or proof["sources"] != policy["native_inputs"]):
            raise ValueError("rocket phase native producer scope changed")
        for actual in [proof["application"], proof["terminal"], *proof["sources"]]:
            if art.source.identity(art.ROOT / actual["path"]) != actual:
                raise ValueError("rocket phase actual producer input/output changed")
    art.checked_model(art.read_json(art.MODEL))


def source_pose(phase, blocked="None", outbound_burned=False, recovery_burned=False, remaining_ticks=0, policy=None):
    """Select a source pose from fields that remain owned/validated by SpaceTransition.

    @param phase Actual saved native phase name, or None for dormant/no transition.
    @param blocked Actual native flight block; Approach denotes continuing movement.
    @param outbound_burned Actual once-only paid outbound leg receipt.
    @param recovery_burned Actual once-only paid recovery leg receipt.
    @param remaining_ticks Saved native controller countdown; never changed here.
    @param policy Optional already checked immutable source contract for batch conversion.
    @return State, logical frame and declared original engine-part key or no effect.
    @throws ValueError On unknown enum names or non-native countdown input.
    """
    policy = art.read_json(POLICY) if policy is None else policy
    # Source enum drift must stop conversion. This does not validate native
    # references, custody or save acceptance; those remain native responsibilities.
    if (phase is not None and phase not in policy["phases"]) or blocked not in policy["blocks"]:
        raise ValueError("unknown native rocket phase/block")
    if type(remaining_ticks) is not int:
        raise ValueError("rocket source countdown must be a native integer")
    state, frame, effect = "ready", 0, None
    # A dormant hull and ordinary internal loading share the same sealed ready
    # exterior; source art never exposes or creates the packets inside it.
    if phase is None or phase == "GroundLoading":
        return dict(state=state, frame=frame, effect=effect)
    flight_ticks = policy["native_budget"]["flight_ticks"]
    if phase == "Ascent":
        # Target/home loss does not stop the active native ascent countdown.
        # Draw only its already funded leg; a missing receipt cannot create fire.
        funded = outbound_burned and not recovery_burned and 0 < remaining_ticks <= flight_ticks
        state = "ascent" if funded else "blocked"
    elif phase == "ReturnRecovery":
        # The original recovery continues moving until native admission decides
        # its terminal wait. A HomeLost flag alone cannot freeze this funded leg.
        funded = outbound_burned and recovery_burned and 0 < remaining_ticks <= flight_ticks
        state = "recovery" if funded else "blocked"
    elif phase == "Holding" or blocked not in ("None", "Approach"):
        state = "blocked"
    else:
        state = {"Preparing": "preparing", "OrbitalDelivery": "orbit", "Turnaround": "service"}[phase]
    if state in policy["active_states"]:
        frame = ((flight_ticks-remaining_ticks)//policy["frame_rule"]["opportunities_per_frame"]) % 2
        effect = "engine/frame"+str(frame)
    return dict(state=state, frame=frame, effect=effect)
