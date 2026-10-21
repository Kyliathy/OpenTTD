"""Qualify actual eleven-family native industry state viewports and exact accepted source pixels.

@file qualify_ground_industry_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHB.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native state/stock/clock staging is explicit. Rendering proves no paid transport,
earned mining/processing or human visual acceptance.
"""

import json

from ground_pack_check_support import ROOT, handoff
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference

RUN = ROOT / "logs/graphics/selected-ground-runtime/20261021-industry-01"
DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1"


def qualify():
    """Join actual current state-owner executions, exact source compositions and verifier refusal evidence.

    @return Bounded native industry view qualification with transport/gameplay acceptance explicitly excluded.
    """
    fixture_path = DIRECTORY / "industry-fixture-transport.json"
    fixture = handoff.read_json(fixture_path.relative_to(ROOT).as_posix())
    if fixture.get("native_checkpoint_reload_verified") is not True:
        raise ValueError("industry state views lack actual native fixture transport")
    for row in fixture["native"]:
        checked_reference(row["retained_application"])
    refusal_path = RUN / "pixel-verifier-cases/verification.json"
    refusal = handoff.read_json(refusal_path.relative_to(ROOT).as_posix())
    # A fresh hash cannot legitimize wrong source bytes, phase, footprint or
    # partial process output; require the focused semantic corruption boundary.
    if refusal.get("passed") is not True or refusal["cases"] != 7:
        raise ValueError("industry pixel verifier semantic refusal proof is incomplete")
    for row in refusal["inputs"] + [refusal["terminal"], refusal["native_execution"]]:
        checked_reference(row)
    dispatch = handoff.read_json("media/multilayer/contracts/ground-consumer-dispatch.json")
    families = sorted(row["asset_id"] for row in dispatch["families"] if row.get("binding_header") in
                      ("src/ground_mine_sprite.h", "src/ground_processor_sprite.h"))
    expected = set()
    # Atomic static blocked/depleted/ready states use source phase0 aliases;
    # native construction and working phases retain all four authored variants.
    for asset in families:
        expected.update((asset, state, frame) for state in ("construction", "working") for frame in range(4))
        expected.add((asset, "blocked", 0))
        expected.add((asset, "depleted" if asset.startswith("mine.") else "ready", 0))
    native = []
    # Both configurations must reproduce every reachable selected state with
    # actual loaded pixels, immutable input identity and zero drawing side effects.
    for configuration in ("Debug", "RelWithDebInfo"):
        execution_path = RUN / ("states-" + configuration.lower()) / "states.json"
        execution = handoff.read_json(execution_path.relative_to(ROOT).as_posix())
        observed = execution["observation"]
        if (execution.get("passed") is not True or execution["mode"] != "states" or observed["exit_code"] != 0
                or observed["assertions"] != 1403 or observed["full_state_coverage"] is not True or observed["native_scenes"] != 110):
            raise ValueError("native eleven-family source-state execution is incomplete")
        for row in execution["inputs"] + observed["captures"] + [observed["source"], observed["application"], observed["terminal"], observed["runtime_payload"]]:
            checked_reference(row)
        pixel_path = RUN / ("states-pixels-" + configuration.lower() + ".json")
        pixels = handoff.read_json(pixel_path.relative_to(ROOT).as_posix())
        # Exhaust exact native state/cell/part compositions. Missing scenes or
        # a convenient nonempty screenshot cannot satisfy the full selected scope.
        if (pixels.get("passed") is not True or pixels["execution"] != reference(execution_path)
                or pixels["native_scenes"] != 110 or pixels["matched_source_pixels"] != 1496709
                or {(row["asset_id"], row["state"], row["frame"]) for row in pixels["frames"]} != expected):
            raise ValueError("native selected industry source-pixel coverage differs from the full state matrix")
        for row in pixels["frames"]:
            for key in ("source_manifest", "source_atlas", "native_indices", "native_rgb"):
                checked_reference(row[key])
        depth_control = sum(row["reversed_depth_different_pixels"] for row in pixels["frames"])
        if depth_control != 355492:
            raise ValueError("native source pixel proof lacks its real cell-depth overlap discrimination")
        application = checked_reference(observed["application"])
        retained = DIRECTORY / "industry-states-bin" / configuration / application.name
        retained.parent.mkdir(parents=True, exist_ok=True)
        if retained.exists() and retained.read_bytes() != application.read_bytes():
            raise ValueError("retained native state observer already has another identity")
        if not retained.exists():
            retained.write_bytes(application.read_bytes())
        native.append({"configuration": configuration, "execution": reference(execution_path), "pixels": reference(pixel_path),
                       "executed_application": observed["application"], "retained_application": reference(retained),
                       "native_assertions": 1403, "native_viewports": 110, "matched_source_pixels": 1496709,
                       "wrong_depth_different_pixels": depth_control})
    baseline = (RUN / "before.json").relative_to(ROOT).as_posix()
    transitions, count = preserved(baseline)
    inputs = ["src/tests/ground_industry_runtime.cpp", "src/tests/ground_industry_runtime_fixture.h",
              "src/tests/multilayer/ground_runtime_checks-1.py", "src/tests/multilayer/ground_industry_pixel_checks.py",
              "src/tests/multilayer/qualify_ground_industry_states.py", "src/industry_cmd.cpp", "CMakeLists.txt"]
    result = {"_documentation": __doc__, "passed": True, "families": families, "native": native,
              "inputs": [reference(handoff.bound(path)) for path in inputs], "fixture_transport": reference(fixture_path),
              "verifier_refusals": reference(refusal_path), "baseline": reference(handoff.bound(baseline)),
              "preserved_file_count": count, "transitions": transitions, "actual_native_viewports": True,
              "full_native_state_coverage": True, "source_pixels_and_anchors_exact": True,
              "native_cell_depth_and_back_front_composition_exact": True, "native_front_height": 20,
              "fixture_floor_capital_construction_inputs_depletion": True, "drawing_owners_unchanged": True,
              "human_acceptance": "pending", "transported_bootstrap_claimed": False}
    (DIRECTORY / "industry-states.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    print(json.dumps({key: value for key, value in qualify().items() if key in
          ("passed", "families", "full_native_state_coverage", "source_pixels_and_anchors_exact")}))
