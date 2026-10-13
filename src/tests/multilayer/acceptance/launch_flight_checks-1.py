"""Verify one paid rocket phase interruption in a bounded native flight burst.

@file launch_flight_checks-1.py Paid launch/deployment/recovery continuation acceptance.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups9-11.
Uses the existing native logistics and flight observers. This checks one selected
phase, rather than claiming all phases or completed orbital facility delivery.
"""
import argparse
import json
from launch_flight_checks import native, logistics_native
from space_preview_checks import ROOT, chunks, identity
from runner import current_run_context


def main():
    """Prepare paid native assets and compare one interrupted flight with uninterrupted completion."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--phase", default="delivery", choices=("preparing", "ascent", "delivery", "return", "turnaround"))
    args = parser.parse_args()
    context = current_run_context("space", "flight-verify")
    retained = ROOT / "src/tests/fixtures/demos/space/space-launch-hub-1990-408.sav"
    retained_identity = identity(retained)
    logistics = context.for_feature("space", "logistics-prepare")
    logistics_native(logistics, args.configuration, "prepare", retained)
    bootstrap = logistics.saves / "logistics-complete-414.sav"
    assert chunks(bootstrap)[0] == 415
    prepare = context.for_feature("space", "flight-prepare")
    partial = native(prepare, args.configuration, "prepare", bootstrap, args.phase)
    original_partial = identity(partial)
    assert chunks(partial)[0] == 415
    continued = native(context.for_feature("space", "flight-resume"), args.configuration, "continue", partial, args.phase)
    uninterrupted = chunks(prepare.saves / "flight-complete-414.sav")[1]
    version, resumed = chunks(continued)
    assert version == 415
    exact = (b"SPST", b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF", b"SPOR",
             b"ASTR", b"VEHS", b"CAPA", b"PLYR", b"STNN", b"LCTR", b"LCHB", b"LHND")
    # The same real hull, carried probe, packet owners, receipts and money must
    # continue identically across the selected interruption, including paid recovery.
    for family in exact:
        assert uninterrupted[family] == resumed[family], ("interrupted native rocket", args.phase, family)
    assert identity(retained) == retained_identity
    assert identity(partial) == original_partial
    report = context.reports / "launch-flight-focused-verification.json"
    report.write_text(json.dumps({
        "configuration": args.configuration, "save_version": version, "phase": args.phase,
        "retained": retained_identity, "paid_bootstrap": identity(bootstrap),
        "interrupted": original_partial, "continued": identity(continued),
        "exact_owner_families": [family.decode() for family in exact],
        "native_observer": "paid deployment, fuel-once, same-hull reuse and lost-home recovery assertions",
        "remaining_scope": "other phase interruptions, orbital facility delivery, final art and full acceptance",
    }, indent=2) + "\n", encoding="utf-8")
    print(f"Focused paid native rocket continuation verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
