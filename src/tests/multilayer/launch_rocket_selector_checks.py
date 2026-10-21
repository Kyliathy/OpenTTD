"""Qualify actual native rocket selectors against the immutable accepted source policy.

@file launch_rocket_selector_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62J.
Each actual process has a60second timeout and a reserved repository invocation.
No paid world is loaded. Source-era receipts and original applications remain
unchanged; current results live in a distinct rocket-selector proof directory.
"""

import argparse
from collections import Counter
import hashlib
import itertools
import json
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
from tools import cargo_rocket_art as art
from tools import cargo_rocket_states as source
from support.artifact_paths import create_run_context
from launch_fleet_payload_checks import preserved_applications

OUTPUT = ROOT / "logs/proofs/mlm/native-art/space/launch-rocket-selectors-v1"
FIXTURE = ROOT / "src/tests/multilayer/launch-rocket-native-owner-fixture.json"
HANDOFF = ROOT / "media/multilayer/source/space/launch-fleet-source-handoff-v1.json"
STATES = ("ready", "preparing", "ascent", "orbit", "recovery", "blocked", "service")
SOURCES = ("src/launch_sprite.h", "src/launch_sprite.cpp", "src/space_vehicle.cpp",
           "src/tests/launch_fleet_sprite.cpp", "src/CMakeLists.txt", "src/tests/CMakeLists.txt",
           "CMakeLists.txt", "src/tests/multilayer/launch_rocket_selector_checks.py")


def protected_boundary(native_station_draw=False):
    """Verify source-era owners and accepted pixels independently of new native observations.

    @param native_station_draw Explicit62JB qualification of only the new launch draw branch; default preserves the original62J gate.
    @return Frozen policy plus immutable inputs used by this current qualification.
    @throws ValueError On changed source contracts, historical outputs or simulation code.
    """
    preserved_applications()
    fixture = art.read_json(FIXTURE)
    reference = fixture["source_era_policy"]
    if art.source.identity(ROOT / reference["path"]) != reference:
        raise ValueError("frozen rocket source policy changed")
    policy = art.read_json(ROOT / reference["path"])
    # Twenty-one original producer files stay exact. Only the original hull's
    # image-dispatch body/header receives this group's native implementation.
    for owner in policy["native_inputs"]:
        # The later facility gate may change only the original launch draw
        # branch/include. Reconstructing its exact source-era bytes preserves
        # all ordinary station graphics and gameplay; this is no hash rebase.
        if native_station_draw and owner["path"] == "src/station_cmd.cpp":
            station = (ROOT / owner["path"]).read_text(encoding="utf-8")
            if station.count('#include "launch_facility_sprite.h"\n') != 1 or station.count('DrawLaunchFacilityTile(*ti);') != 1:
                raise ValueError("native station renderer changed beyond its explicit facility branch")
            station = station.replace('#include "launch_facility_sprite.h"\n', "")
            station = station.replace('DrawLaunchFacilityTile(*ti);', 'DrawGroundSprite(SPR_CONCRETE_GROUND, PAL_NONE); // Temporary native pad; final building family remains Plan14.')
            if hashlib.sha256(station.encode("utf-8")).hexdigest() != owner["sha256"]:
                raise ValueError("native station changed outside its qualified launch draw branch")
            continue
        if owner["path"] != "src/space_vehicle.cpp" and art.source.identity(ROOT / owner["path"]) != owner:
            raise ValueError("original native rocket phase/fuel/custody/geometry owner changed")
    vehicle = (ROOT / "src/space_vehicle.cpp").read_text(encoding="utf-8")
    vehicle = vehicle.replace('#include "launch_sprite.h"\n', "")
    vehicle = vehicle.replace(" * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62J; original cargo-rocket flight graphics.\n", "")
    vehicle, count = re.subn(r"^void SpaceVehicle::GetImage\([^\n]*\) const\n\{.*?^\}",
                             "<GetImage presentation body>", vehicle, count=1, flags=re.S | re.M)
    # A source-era fixture binds every remaining byte after LF normalization;
    # matching only declarations would conceal changed physics or accounting.
    if count != 1 or hashlib.sha256(vehicle.encode("utf-8")).hexdigest() != fixture["vehicle_outside_get_image_lf_sha256"]:
        raise ValueError("native hull changed outside the authorized image-dispatch boundary")
    if art.source.identity(HANDOFF)["sha256"] != "0c57c404169fb45a627c8dbbceabfc5fd44bc2cc361a4aa790467dbf37b97c67":
        raise ValueError("qualified source handoff changed")
    handoff = art.read_json(HANDOFF)
    # Follow only the frozen rocket's actual recipe chain to bind the original
    # pure Python selector. Current expectations cannot come from edited code.
    recipe_ref = handoff["sources"]["vehicle.model-14"]["source_contract"]["source"]["composition_recipe"]
    if art.source.identity(ROOT / recipe_ref["path"]) != recipe_ref:
        raise ValueError("qualified rocket source recipe changed")
    recipe = art.read_json(ROOT / recipe_ref["path"])
    qualification_ref = recipe["state_qualification"]
    if art.source.identity(ROOT / qualification_ref["path"]) != qualification_ref:
        raise ValueError("original rocket phase qualification changed")
    qualification = art.read_json(ROOT / qualification_ref["path"])
    proof_ref = qualification["proof"]
    if art.source.identity(ROOT / proof_ref["path"]) != proof_ref:
        raise ValueError("original rocket phase proof changed")
    source_proof = art.read_json(ROOT / proof_ref["path"])
    # Frozen source reports/atlases and protected packing outputs remain the
    # original evidence; current C++ cannot authorize repainting their pixels.
    references = list(handoff["protected_outputs"])
    references.extend((recipe_ref, qualification_ref, proof_ref, source_proof["helper"], source_proof["tests"], source_proof["fixture"]))
    for family in handoff["sources"].values():
        references.extend(family[key] for key in ("report", "qualification", "atlas"))
    for item in references:
        if art.source.identity(ROOT / item["path"]) != item:
            raise ValueError("accepted source pixels or protected packing output changed")
    # The actual loader-qualified append is unchanged during selector work.
    if art.source.identity(ROOT / "media/baseset/openttd.grf")["sha256"] != "1a9a5f5550155580a6574c68bb98bed8c450ce1c503d683cbf1c1f32f9c36f97":
        raise ValueError("qualified launch/fleet packed pixels changed")
    retained = art.read_json(ROOT / "logs/proofs/mlm/native-art/space/launch-fleet-packing-v1/qualified-native-applications.json")
    # Current selector binaries may change; the exact earlier decoder binaries
    # and their original successful receipts must remain available unchanged.
    for row in retained["applications"]:
        for item in (row["retained_application"], row["qualification"]):
            if art.source.identity(ROOT / item["path"]) != item:
                raise ValueError("historical native application or receipt changed")
    return policy, dict(fixture=art.source.identity(FIXTURE), handoff=art.source.identity(HANDOFF),
                        source_policy=reference, unchanged_native_owners=20 if native_station_draw else 21,
                        qualified_station_draw_branch=native_station_draw, source_and_packing_outputs=references,
                        current_sources=[art.source.identity(ROOT / name) for name in SOURCES])


def verify_observations(terminal, policy):
    """Compare complete native phase observations with the original independent source producer.

    @param terminal Actual native combined stdout/stderr for the four focused cases.
    @param policy Immutable qualified source policy, with original enum ordering and cadence.
    @return Number of independently compared native observations.
    @throws ValueError On missing, duplicated, extra or source-incompatible native observations.
    """
    phases, blocks = policy["phases"], policy["blocks"]
    expected = Counter(itertools.product(range(7), range(7), range(4), (0, 1, 206, 214, 222, 223)))
    # Both funded legs also cover every positive native countdown with home
    # loss, including every frame boundary and its adjacent opportunities.
    expected.update((phase, 2, burns, remaining) for phase, burns in ((2, 1), (4, 3)) for remaining in range(1, 223))
    actual = Counter()
    rows = re.findall(r"^LAUNCH_ROCKET_POLICY (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)$", terminal, flags=re.M)
    # Compare each real native result with the separately authored Python
    # source selector; a matching count alone cannot establish phase parity.
    for raw in rows:
        phase, block, burns, remaining, state, frame = map(int, raw)
        key = (phase, block, burns, remaining)
        if key not in expected:
            raise ValueError("native rocket observation has an undeclared input")
        selected = source.source_pose(phases[phase], blocks[block], bool(burns & 1), bool(burns & 2), remaining, policy)
        if state >= len(STATES) or STATES[state] != selected["state"] or frame != selected["frame"]:
            raise ValueError("native rocket pose differs from the accepted source phase policy")
        actual[key] += 1
    if actual != expected:
        raise ValueError("native rocket phase observations are incomplete or duplicated")
    return len(rows)


def verify_refusals(terminal, policy):
    """Require the independent comparison to refuse broken native evidence.

    @param terminal Successful real native trace, used only for detached bad-input checks.
    @param policy Immutable original source policy.
    @return Count of rejected detached evidence mutations.
    @throws ValueError If any incomplete, duplicate or wrong-frame observation is accepted.
    """
    first = re.search(r"^LAUNCH_ROCKET_POLICY .*$", terminal, flags=re.M).group(0)
    bad_frame = first.rsplit(" ", 1)[0] + " 99"
    mutations = (terminal.replace(first, "", 1), terminal + "\n" + first,
                 terminal.replace(first, bad_frame, 1))
    # None of these detached mutations touches a proof file or source fixture;
    # successful refusal demonstrates that a plausible partial log cannot pass.
    for bad in mutations:
        try:
            verify_observations(bad, policy)
        except ValueError:
            continue
        raise ValueError("rocket evidence mutation unexpectedly accepted")
    return len(mutations)


def run(configuration):
    """Run one actual focused application process and bind current source/executable identities.

    @param configuration Debug or RelWithDebInfo from the existing isolated art target.
    @return Newly qualified current proof; source-era receipts remain untouched.
    @throws ValueError On native failure, timeout, stale inputs or source-policy disagreement.
    """
    policy, boundary = protected_boundary()
    application = ROOT / "build" / configuration / "openttd_space_art_test.exe"
    executable = art.source.identity(application)
    context = create_run_context("multilayer-art", "launch-rocket-selectors", run_id="rocket-selectors-" + str(time.time_ns()),
                                 root=ROOT, profile="launch-fleet-art", process=configuration.lower())
    terminal = ROOT / ("logs/launch-rocket-selectors-" + configuration + ".log")
    arguments = [str(application), "[launch-rocket-art]", "--reporter", "console", "--durations", "yes"]
    started = time.monotonic()
    with terminal.open("w", encoding="utf-8") as output:
        result = subprocess.run(arguments, cwd=ROOT / "build", env=context.environment(), stdout=output,
                                stderr=subprocess.STDOUT, timeout=60, check=False, creationflags=subprocess.CREATE_NO_WINDOW)
    text = terminal.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \(([\d,]+) assertions in 4 test cases\)", text)
    if result.returncode != 0 or passed is None:
        raise ValueError("focused native rocket selectors failed; see " + str(terminal.relative_to(ROOT)))
    observations = verify_observations(text, policy)
    refusals = verify_refusals(text, policy)
    # Observation cannot silently change its executable, protected owners or
    # original art. Rechecking the same boundary binds all post-process bytes.
    if art.source.identity(application) != executable or protected_boundary()[1] != boundary:
        raise ValueError("rocket selector inputs changed during native observation")
    proof = dict(_documentation=__doc__, complete=True, configuration=configuration, application=executable,
                 terminal=art.source.identity(terminal), arguments=arguments[1:], cases=4,
                 assertions=int(passed.group(1).replace(",", "")), source_policy_comparisons=observations,
                 detached_refusals=refusals, elapsed_seconds=round(time.monotonic() - started, 3),
                 paid_world_loaded=False, simulation_ticks=0, surface_visibility_activated=False, boundary=boundary)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    (OUTPUT / ("native-" + configuration + ".json")).write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(configuration + ": " + str(proof["assertions"]) + " assertions; " + str(observations) + " source comparisons; " + str(refusals) + " refusals", flush=True)
    return proof


def main():
    """Qualify one explicitly selected native configuration without broad regression work.

    @return Zero after actual native selectors and independent source comparisons pass.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    run(parser.parse_args().configuration)
    finalize_pair()
    return 0


def finalize_pair():
    """Publish combined evidence only when both actual native configurations match current inputs.

    @return Whether the complete pair was available; one focused run may precede the other.
    @throws ValueError On altered successful logs or disagreement with the source policy.
    """
    policy, boundary = protected_boundary()
    paths = [OUTPUT / ("native-" + config + ".json") for config in ("Debug", "RelWithDebInfo")]
    if not all(path.is_file() for path in paths):
        return False
    observations = []
    # Each actual executable must independently supply the same current-source
    # scope. A stale preceding configuration cannot close the group's pair.
    for path in paths:
        proof = art.read_json(path)
        if proof["boundary"] != boundary or art.source.identity(ROOT / proof["application"]["path"]) != proof["application"]:
            return False
        terminal = ROOT / proof["terminal"]["path"]
        if art.source.identity(terminal) != proof["terminal"]:
            raise ValueError("qualified rocket selector log changed")
        count = verify_observations(terminal.read_text(encoding="utf-8"), policy)
        if not proof["complete"] or proof["cases"] != 4 or count != proof["source_policy_comparisons"] or proof["detached_refusals"] != 3:
            raise ValueError("qualified rocket selector receipt changed")
        observations.append(art.source.identity(path))
    receipt = dict(_documentation=__doc__, complete=True, boundary=boundary, native_proofs=observations,
                   configurations=["Debug", "RelWithDebInfo"], source_policy_comparisons_per_configuration=1620,
                   focused_cases_per_configuration=4, paid_world_loaded=False, surface_visibility_activated=False,
                   public_runtime_activated=False, next_group="62JB.1")
    (OUTPUT / "final-verification.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    return True


if __name__ == "__main__":
    raise SystemExit(main())
