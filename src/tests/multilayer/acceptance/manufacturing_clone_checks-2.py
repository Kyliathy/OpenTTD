"""Compare one exact ordinary/resaved paid clone and its independent original source owners.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BB
An interrupted last-tick continuation loads the actual ordinary resave, never substitutes the original before checkpoint.
"""
import argparse
import json
import os
from pathlib import Path
import runpy

from runner import current_run_context
from space_preview_checks import ROOT, chunks, identity, ordinary

shared = runpy.run_path(str(Path(__file__).with_name("manufacturing_clone_checks-1.py")))
native, EXACT = shared["native"], shared["EXACT"]


def verify(configuration, proof_path, completion=None):
    """Preserve exact paid request/configuration/source ownership and optionally finish one actual resaved final milestone.

    @param configuration Matching current completed game/test configuration.
    @param proof_path Exact accepted current checkpoint report with its original actual after save.
    @param completion Exact matching uninterrupted completion, including its distinct source lifecycle variant.
    @return Accepted independent ordinary/resumed report path.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8"))
    # Focused-only reports contain no world; only a genuine current native checkpoint can claim exact reconstruction or last-tick continuation.
    assert proof["configuration"] == configuration and proof["save_version"] == 428 and "after" in proof
    scope = f'{proof["variant"]}-{proof["phase"]}-{proof["model"]}'
    context = current_run_context("space", "manufacturing-clone-reload-" + scope)
    report = context.reports / "manufacturing-clone-reload.json"; assert not report.exists()
    selected = proof["after"]; source = ROOT / selected["path"]; assert identity(source) == selected
    owner = context.for_feature("space", "manufacturing-clone-ordinary-" + scope)
    previous_configuration = os.environ.get("OTTD_TEST_CONFIGURATION"); os.environ["OTTD_TEST_CONFIGURATION"] = configuration
    try:
        ordinary(owner, {"name": "clone-" + scope, "snapshot": selected}, configuration)
    finally:
        if previous_configuration is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous_configuration
    resaved = owner.saves / "ordinary-resave.sav"
    expected, actual = chunks(source)[1], chunks(resaved)[1]
    # Original input, captured source/configuration, actual cargo/service packets, paid work and product receipt remain the same owners after ordinary reconstruction.
    for family in EXACT:
        assert expected[family] == actual[family], (scope, family)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8BB; exact ordinary native clone/source/configuration ownership and actual resaved once-only final completion.",
                  configuration=configuration, save_version=428, phase=proof["phase"], model=proof["model"], variant=proof["variant"],
                  proof=identity(proof_path), source=selected, ordinary=identity(resaved), exact_owner_families=len(EXACT))
    owners = (owner,)
    if completion is not None:
        accepted = json.loads(completion.read_text(encoding="utf-8"))
        assert proof["phase"] in ("before", "source-loss", "transfer") and accepted["phase"] == "complete"
        assert accepted["configuration"] == configuration and accepted["model"] == proof["model"] and accepted["variant"] == proof["variant"]
        assert accepted["predecessor"] == identity(proof_path) and identity(ROOT / accepted["after"]["path"]) == accepted["after"]
        resumed_owner = context.for_feature("space", f'manufacturing-clone-resumed-{proof["variant"]}-{proof["model"]}')
        resumed, assertions, additions, original, alive, assets = native(resumed_owner, configuration, "complete", proof["model"], resaved, proof["original"])
        assert not additions and original == proof["original"] and alive == accepted["original_alive"] and assets == accepted["original_asset_digest"]
        expected, actual = chunks(ROOT / accepted["after"]["path"])[1], chunks(resumed)[1]
        # One actual final opportunity after the ordinary resave must converge with uninterrupted name/order application, full recipe consumption and one real new hull.
        for family in EXACT:
            assert expected[family] == actual[family], ("resumed", scope, family)
        result.update(completion=identity(completion), resumed=identity(resumed), resumed_input=identity(resaved), assertions=assertions)
        owners += (resumed_owner,)
    result["native_reports"] = [identity(path) for item in owners for path in sorted(item.reports.glob("native-*.json"))]
    assert identity(source) == selected
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one exact accepted current checkpoint and optional matching uninterrupted final completion."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--completion", type=Path)
    args = parser.parse_args()
    print(verify(args.configuration, args.proof.resolve(), args.completion.resolve() if args.completion else None).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
