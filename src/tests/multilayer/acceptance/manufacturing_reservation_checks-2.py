"""Compare one exact ordinary or interrupted manufacturing reservation boundary.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group7
The ordinary product and native observer consume exact accepted proof identities.
"""
import argparse
import json
import os
from pathlib import Path
import runpy

from runner import current_run_context
from space_preview_checks import ROOT, chunks, identity, ordinary

shared = runpy.run_path(str(Path(__file__).with_name("manufacturing_reservation_checks-1.py")))
native, EXACT = shared["native"], shared["EXACT"]


def verify(configuration, proof_path, completion=None):
    """Require exact native owner bytes after ordinary loading and optionally resume a queued request.

    @param configuration Completed native configuration matching both explicit proofs.
    @param proof_path Exact accepted native phase report containing the selected after checkpoint.
    @param completion Exact uninterrupted reserved-phase proof when resuming a queued request; otherwise absent.
    @return Accepted independent ordinary/reload comparison report path.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8")); assert proof["configuration"] == configuration
    selected = proof["after"]; source = ROOT / selected["path"]; assert identity(source) == selected
    model = proof["model"]; scope = f'{proof["phase"]}-{model}'
    context = current_run_context("space", "manufacturing-reservation-reload-" + scope)
    owner = context.for_feature("space", "manufacturing-reservation-ordinary-" + scope)
    prior_configuration = os.environ.get("OTTD_TEST_CONFIGURATION")
    os.environ["OTTD_TEST_CONFIGURATION"] = configuration
    try:
        ordinary(owner, {"name": "manufacture-" + scope, "snapshot": selected}, configuration)
    finally:
        if prior_configuration is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = prior_configuration
    resaved = owner.saves / "ordinary-resave.sav"
    expected, actual = chunks(source)[1], chunks(resaved)[1]
    # Loading the real product may not move reserved packets, pay capital, reissue a serial/berth, consume electricity or construct a hull.
    for family in EXACT:
        assert expected[family] == actual[family], (scope, family)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group7; exact ordinary and interrupted queued/reserved native ownership.",
                  configuration=configuration, phase=proof["phase"], model=model, proof=identity(proof_path),
                  source=selected, ordinary=identity(resaved), exact_owner_families=len(EXACT))
    owners = (owner,)
    if completion is not None:
        accepted = json.loads(completion.read_text(encoding="utf-8"))
        assert proof["phase"] == "queued" and accepted["phase"] == "reserved"
        assert accepted["configuration"] == configuration and accepted["model"] == model and accepted["predecessor"] == identity(proof_path)
        assert identity(ROOT / accepted["after"]["path"]) == accepted["after"]
        resumed_owner = context.for_feature("space", "manufacturing-reservation-resumed-" + str(model))
        resumed, assertions = native(resumed_owner, configuration, "reserved", model, source)
        expected, actual = chunks(ROOT / accepted["after"]["path"])[1], chunks(resumed)[1]
        # The same single real allocation opportunity must converge with uninterrupted reservation, including every unrelated native family.
        for family in EXACT:
            assert expected[family] == actual[family], ("resumed", model, family)
        result.update(completion=identity(completion), resumed=identity(resumed), assertions=assertions)
        owners += (resumed_owner,)
    result["native_reports"] = [identity(path) for item in owners for path in sorted(item.reports.glob("native-*.json"))]
    assert identity(source) == selected
    report = context.reports / "manufacturing-reservation-reload.json"
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one explicit accepted checkpoint and optional exact uninterrupted reservation proof."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--completion", type=Path)
    args = parser.parse_args()
    print(verify(args.configuration, args.proof.resolve(), args.completion.resolve() if args.completion else None).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
