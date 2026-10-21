"""Close the scoped launch presentation gate against actual native evidence.

@file launch_presentation_closure_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JDD.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md task16.2.
Historical observations retain their executing binaries. Current original
producers and the complete native pixel decoder run in separate focused cases.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
from support.artifact_paths import create_run_context
import launch_facility_selector_checks as selectors
import launch_fleet_payload_checks as payload

ROOT = selectors.ROOT
BASE = ROOT / "logs/proofs/mlm/native-art/space"
OUTPUT = BASE / "launch-presentation-v1"
CONFIGURATIONS = ("Debug", "RelWithDebInfo")
STAGES = ("launch-fleet-packing-v1/final-verification.json",
          "launch-facility-selectors-v1/final-verification.json",
          "launch-fleet-runtime-v1/publication-verification.json",
          "launch-fleet-paid-review-v1/viewport-queue-verification.json",
          "launch-craft-window-v1/publication-verification.json",
          "launch-probe-deployment-v1/verification.json",
          "launch-pod-review-v1/verification.json")


def checked(reference):
    """Verify one exact original artifact without rebasing its receipt.

    @param reference Explicit repository path and SHA256 from accepted evidence.
    @return Same reference after comparing the actual retained bytes.
    @throws ValueError On missing or changed evidence.
    """
    if selectors.art.source.identity(ROOT / reference["path"]) != reference:
        raise ValueError("changed launch presentation evidence: " + reference["path"])
    return reference


def original_window_sources(proof):
    """Prove the later pod GUI extension preserves the qualified craft path.

    @param proof Earlier exact-window case whose recorded source hashes are authoritative.
    @return Current source identities with narrowly reconstructed original bytes.
    @throws ValueError On any change beyond the reviewed pod disclosure/query/test registration.
    """
    sources = []
    # Strip only the explicit later passive-pod extension in memory. Every
    # earlier craft-window byte must still match its accepted source identity.
    for reference in proof["sources"]:
        path = ROOT / reference["path"]
        data = path.read_bytes()
        if reference["path"] == "src/launch_assembly_gui.cpp":
            start = data.index(b"        // A live passive descent")
            end = data.index(b"\t\tif (ResolveSpaceCraft(carrier)", start)
            data = data[:start] + data[end:]
            end = data.index(b"/** @copydoc GetLaunchPodWindowSelection */")
            data = data[:end].rstrip(b"\r\n") + b"\r\n"
        elif reference["path"] == "src/launch_assembly_gui.h":
            data = data.replace(b"/** Existing generation-qualified passive shell identity, separate from native Vehicles. */\r\nstruct SpacePodRef;\r\n", b"")
            start = data.index(b"/** Read one native workshop's actual surviving passive-shell selection.")
            end = data.index(b"#endif", start)
            data = data[:start] + data[end:]
        elif reference["path"] in ("CMakeLists.txt", "src/tests/CMakeLists.txt"):
            data = b"".join(line for line in data.splitlines(keepends=True)
                            if b"launch_pod_review.cpp" not in line
                            and b"# Passive shell/packet recovery and real UI selection:" not in line)
        if hashlib.sha256(data).hexdigest() != reference["sha256"]:
            raise ValueError("earlier craft-window source changed beyond the pod extension: " + reference["path"])
        sources.append(selectors.art.source.identity(path))
    return sources


def audited_case(reference, retained):
    """Check an immutable actual case through its images and executing archive.

    @param reference Accepted case receipt identity from its completed aggregate.
    @param retained Actual binary retained by that case's capture epoch.
    @return Verified case without treating an old build path as its current executable.
    @throws ValueError On substituted worlds, native observations, pixels or applications.
    """
    checked(reference)
    proof = selectors.art.read_json(ROOT / reference["path"])
    checked(retained)
    if proof["application"]["sha256"] != retained["sha256"]:
        raise ValueError("historical native case lost its actual executing application")
    for artifact in (proof["source"], proof["qualification"], proof["terminal"],
                     proof["observation"], *proof["images"]):
        checked(artifact)
    return proof


def current_native(configuration):
    """Qualify current original native producers and every protected/new decoded sprite.

    @param configuration Existing isolated Debug or RelWithDebInfo engine/art target.
    @return Two bounded actual process records bound to current binary and payload.
    @throws ValueError On stale payload, changed originals, failed or overdue native tests.
    """
    originals = payload.preserved_applications()
    application = ROOT / "build" / configuration / "openttd_space_art_test.exe"
    binary = selectors.art.source.identity(application)
    source = selectors.art.source.identity(ROOT / "media/baseset/openttd.grf")
    context = create_run_context("multilayer-art", "launch-presentation-closure", root=ROOT,
                                 profile="launch-fleet-art", process=configuration.lower())
    context.prepare_runtime()
    results = []
    filters = (("producers", "[launch-site],[launch-assembly-contract],[space-logistics-contract],[launch-facility-art],[launch-rocket-art]", 20, 14943),
               ("decoder", "[.launch-fleet-packed-art-load]", 1, 163073))
    # Each native scope stays separate and bounded. The complete original
    # producer set and native byte decoder prevent stale integration claims.
    for name, expression, cases, assertions in filters:
        terminal = ROOT / ("logs/launch-presentation-" + name + "-" + configuration + ".log")
        with terminal.open("w", encoding="utf-8") as output:
            result = subprocess.run([str(application), expression, "--durations", "yes"],
                                    cwd=ROOT / "build", env=context.environment(), stdout=output,
                                    stderr=subprocess.STDOUT, timeout=60, check=False,
                                    creationflags=subprocess.CREATE_NO_WINDOW)
        text = terminal.read_text(encoding="utf-8", errors="replace")
        match = re.search(r"All tests passed \(([\d,]+) assertions in (\d+) test cases?\)", text)
        if (result.returncode or match is None or int(match.group(1).replace(",", "")) != assertions
                or int(match.group(2)) != cases):
            raise ValueError("current launch presentation native scope failed: " + str(terminal))
        if name == "decoder" and payload.MARKER not in text:
            raise ValueError("complete native protected/new pixel decoding marker absent")
        results.append(dict(scope=name, cases=cases, assertions=assertions,
                            terminal=selectors.art.source.identity(terminal)))
    # Repository, build discovery and isolated runtime must load the same GRF;
    # unrelated original applications remain the immutable financial producers.
    for path in (ROOT / "media/baseset/openttd.grf", ROOT / "build/baseset/openttd.grf",
                 context.runtime_dir / "baseset/openttd.grf"):
        if selectors.art.source.identity(path)["sha256"] != source["sha256"]:
            raise ValueError("current launch decoder loaded different packed bytes")
    checked(binary)
    payload.preserved_applications()
    return dict(configuration=configuration, application=binary, payload=source,
                original_applications=originals, processes=results)


def qualify():
    """Verify every scoped presentation requirement before conditional plan closure.

    @return Complete two-configuration gate receipt; unrelated master acceptance stays open.
    @throws ValueError On stale stages, incomplete ordinary review or changed native ownership.
    """
    selectors.boundary()
    stages = [selectors.art.read_json(BASE / name) for name in STAGES]
    if any(not stage["complete"] for stage in stages):
        raise ValueError("launch presentation has an incomplete required stage")
    packing, facility, runtime, paid, windows, probe, pods = stages
    # Actual source/payload, occupied native raster, generic native selection,
    # paid deployment and passive recovery must all pass their independent gates.
    if (not facility["full_renderer_reviewed"] or not paid["visual_review_complete"]
            or not windows["published"] or not windows["visual_review_complete"]
            or not probe["actual_paid_deployment"] or not pods["visual_review_complete"]):
        raise ValueError("launch presentation still lacks actual native ordinary review")
    for reference in (packing["payload"], packing["packed_manifest"], packing["source_handoff"],
                      windows["public_manifest"], windows["public_before"], windows["runtime_manifest"]):
        checked(reference)
    for reference in paid["reviews"]:
        checked(reference)
        review = selectors.art.read_json(ROOT / reference["path"])
        for image in review["review_images"]:
            checked(image)
        for case in review["proofs"]:
            audited_case(case, review["retained_application"])
    current_sources = []
    # Each earlier generic-window case remains bound to its own real app, and
    # the current source reconstructs its exact reviewed craft-selection behavior.
    for reference in windows["evidence"]:
        checked(reference)
        if reference["path"].endswith("viewport-queue-verification.json"):
            continue
        proof = selectors.art.read_json(ROOT / reference["path"])
        retained = next(row["retained"] for row in windows["applications"]
                        if row["observed"] == proof["application"])
        audited_case(reference, retained)
        current_sources = original_window_sources(proof)
    native = []
    for configuration, review in zip(CONFIGURATIONS, pods["configurations"], strict=True):
        # Current pod captures bind every later GUI/source extension and its real
        # binary, including the one earned native atomic settlement opportunity.
        for reference in review["proofs"]:
            proof = audited_case(reference, review["retained_application"])
            for source in proof["sources"]:
                checked(source)
        for image in review["review_images"]:
            checked(image)
        current = current_native(configuration)
        if current["application"]["sha256"] != review["retained_application"]["sha256"]:
            raise ValueError("current native qualification differs from actual pod review app")
        native.append(current)
    selectors.boundary()
    receipt = dict(_documentation=__doc__, complete=True, group="62JDD", plan9_task="16.2",
                   stages=[selectors.art.source.identity(BASE / name) for name in STAGES],
                   current_native=native, current_craft_window_sources=current_sources,
                   requirements=dict(source_and_packed=True, native_layers=True,
                                     occupied_occlusion=True, exact_vehicle_selection=True,
                                     payload_phase_fuel_service_queue=True,
                                     actual_probe_deployment=True, passive_atomic_recovery=True),
                   final_gallery_or_user_acceptance=False, financial_scope_reopened=False,
                   verifier=selectors.art.source.identity(Path(__file__).resolve()))
    OUTPUT.mkdir(parents=True, exist_ok=True)
    (OUTPUT / "verification.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print("Plan9 task16.2 scoped native presentation gate verified in both configurations", flush=True)
    return receipt


if __name__ == "__main__":
    qualify()
