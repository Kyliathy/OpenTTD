"""Compare one exact interrupted/ordinary Assembly boundary in a separate bounded burst.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group6B
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context

shared = runpy.run_path(str(Path(__file__).with_name("assembly_facility_checks-1.py")))
native, EXACT = shared["native"], shared["EXACT"]


def verify(configuration, proof_path, boundary):
    """Require exact persisted owners at one accepted native boundary and optionally resume partial activation.

    @param configuration Completed product/test configuration matching the proof.
    @param proof_path Exact accepted material/activation/connection/lifecycle report.
    @param boundary Named proof checkpoint: after, frame, partial or unpowered.
    @return Accepted ordinary/reload comparison report path.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8")); assert proof["configuration"] == configuration
    selected = proof[boundary]; source = ROOT / selected["path"]; assert identity(source) == selected
    scope = proof["phase"] + "-" + boundary
    context = current_run_context("space", "assembly-facility-reload-" + scope)
    owner = context.for_feature("space", "assembly-facility-ordinary-" + scope)
    ordinary(owner, {"name": "assembly-" + boundary, "snapshot": selected}, configuration)
    resaved = owner.saves / "ordinary-resave.sav"
    expected, actual = chunks(source)[1], chunks(resaved)[1]
    # Every native family retains original packet, vehicle, money, work and electrical authority; a paused ordinary load may not replay any production event.
    for family in EXACT:
        assert expected[family] == actual[family], ("ordinary", boundary, family)
    result = {"_documentation": "Owner: r2mlvs11 Group6B; exact ordinary/reloaded native Assembly owners.",
              "configuration": configuration, "proof": identity(proof_path), "boundary": boundary, "source": selected,
              "ordinary": identity(resaved), "exact_owner_families": len(EXACT)}
    owners = (owner,)
    if boundary == "partial":
        assert proof["phase"] == "goods"
        resumed_owner = context.for_feature("space", "assembly-facility-resumed")
        resumed, assertions = native(resumed_owner, configuration, "activation", source)
        expected, actual = chunks(ROOT / proof["after"]["path"])[1], chunks(resumed)[1]
        # The same37 remaining activation opportunities must converge to uninterrupted completion, including unrelated owners and company charges.
        for family in EXACT:
            assert expected[family] == actual[family], ("activation", family)
        result.update(resumed=identity(resumed), assertions=assertions)
        owners += (resumed_owner,)
    result["native_reports"] = [identity(path) for item in owners for path in sorted(item.reports.glob("native-*.json"))]
    assert identity(source) == selected
    report = context.reports / "assembly-facility-reload.json"
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one explicitly accepted proof/checkpoint and print its bounded comparison report."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--boundary", choices=("after", "frame", "partial", "unpowered"), default="after")
    args = parser.parse_args()
    print(verify(args.configuration, args.proof.resolve(), args.boundary).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
