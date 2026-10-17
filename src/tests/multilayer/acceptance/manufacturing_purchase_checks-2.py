"""Compare one exact ordinary/replayed native purchase job or preserved Earth hull.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8B
Reconstruction retains real paid owner identities without replaying requests, inputs, work or hull creation.
"""
import argparse
import json
import os
from pathlib import Path
import runpy

from runner import current_run_context
from space_preview_checks import ROOT, chunks, identity, ordinary

shared = runpy.run_path(str(Path(__file__).with_name("manufacturing_purchase_checks-1.py")))
native, EXACT = shared["native"], shared["EXACT"]


def verify(configuration, proof_path, completion=None):
    """Require exact ordinary native owner preservation and optional resumed last-tick purchase completion.

    @param configuration Matching completed native product/test configuration.
    @param proof_path Exact accepted current boundary report with its genuine after checkpoint.
    @param completion Exact uninterrupted complete proof for a before checkpoint; absent for ordinary-only comparison.
    @return Accepted independent ordinary/replay report path.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8")); assert proof["configuration"] == configuration and proof["save_version"] == 427
    selected = proof["after"]; source = ROOT / selected["path"]; assert identity(source) == selected
    model = proof["model"]; scope = f'{proof["phase"]}-{model}'
    context = current_run_context("space", "manufacturing-purchase-reload-" + scope)
    owner = context.for_feature("space", "manufacturing-purchase-ordinary-" + scope)
    previous_configuration = os.environ.get("OTTD_TEST_CONFIGURATION"); os.environ["OTTD_TEST_CONFIGURATION"] = configuration
    try:
        ordinary(owner, {"name": "purchase-" + scope, "snapshot": selected}, configuration)
    finally:
        if previous_configuration is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous_configuration
    resaved = owner.saves / "ordinary-resave.sav"
    expected, actual = chunks(source)[1], chunks(resaved)[1]
    # A paused ordinary reconstruction preserves all existing input/job/capital/berth/hull owners, including the unchanged original Earth path.
    for family in EXACT:
        assert expected[family] == actual[family], (scope, family)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8B; ordinary/replayed native purchase ownership, no duplicate paid request or product.",
                  configuration=configuration, phase=proof["phase"], model=model, proof=identity(proof_path), source=selected,
                  ordinary=identity(resaved), exact_owner_families=len(EXACT))
    owners = (owner,)
    if completion is not None:
        accepted = json.loads(completion.read_text(encoding="utf-8"))
        assert proof["phase"] == "before" and accepted["phase"] == "complete" and accepted["configuration"] == configuration
        assert accepted["model"] == model and accepted["predecessor"] == identity(proof_path)
        assert identity(ROOT / accepted["after"]["path"]) == accepted["after"]
        resumed_owner = context.for_feature("space", "manufacturing-purchase-resumed-" + str(model))
        resumed, assertions, additions = native(resumed_owner, configuration, "complete", model, resaved)
        assert not additions
        expected, actual = chunks(ROOT / accepted["after"]["path"])[1], chunks(resumed)[1]
        # The actual ordinary resave, then one real work opportunity, must converge with uninterrupted final input consumption and one canonical new product.
        for family in EXACT:
            assert expected[family] == actual[family], ("resumed", model, family)
        result.update(completion=identity(completion), resumed=identity(resumed), resumed_input=identity(resaved), assertions=assertions)
        owners += (resumed_owner,)
    result["native_reports"] = [identity(path) for item in owners for path in sorted(item.reports.glob("native-*.json"))]
    assert identity(source) == selected
    report = context.reports / "manufacturing-purchase-reload.json"
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one exact current purchase checkpoint and optional matching uninterrupted completion proof."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--completion", type=Path)
    args = parser.parse_args()
    print(verify(args.configuration, args.proof.resolve(), args.completion.resolve() if args.completion else None).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
