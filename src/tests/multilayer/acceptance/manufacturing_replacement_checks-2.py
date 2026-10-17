"""Compare exact ordinary reconstruction and actual resumed final replacement settlement.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC
Every interrupted completion loads the actual ordinary resave, retaining native Engine/rule/group owners as well as physical production and packets.
"""
import argparse
import json
import os
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("manufacturing_replacement_checks-1.py")))


def verify(configuration, proof_path, completion=None):
    """Reconstruct one exact paid request/history and optionally settle its real resaved last tick.

    @param configuration Matching current completed game/test build.
    @param proof_path Exact current native checkpoint report.
    @param completion Optional matching uninterrupted completion with this checkpoint as its exact predecessor.
    @return Independent accepted ordinary/resumed report path.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8"))
    assert proof["configuration"] == configuration and proof["save_version"] == 429 and "after" in proof
    scope = f'{proof["variant"]}-{proof["phase"]}-{proof["model"]}'
    context = current_run_context("space", "manufacturing-replacement-reload-" + scope)
    report = context.reports / "manufacturing-replacement-reload.json"; assert not report.exists()
    selected = proof["after"]; source = ROOT / selected["path"]; assert identity(source) == selected
    owner = context.for_feature("space", "manufacturing-replacement-ordinary-" + scope)
    previous_configuration = os.environ.get("OTTD_TEST_CONFIGURATION"); os.environ["OTTD_TEST_CONFIGURATION"] = configuration
    try:
        ordinary(owner, {"name": "replacement-" + scope, "snapshot": selected}, configuration)
    finally:
        if previous_configuration is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous_configuration
    resaved = owner.saves / "ordinary-resave.sav"; expected, actual = chunks(source)[1], chunks(resaved)[1]
    # Native group/rule/Engine identities and all original/new physical owners must reconstruct exactly; unchanged Space digests alone cannot prove ordinary native accounting.
    for family in SHARED["EXACT"]:
        assert expected[family] == actual[family], (scope, family)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8BC; exact ordinary reconstruction and resaved once-only final native replacement retirement.",
                  configuration=configuration, save_version=429, phase=proof["phase"], model=proof["model"], variant=proof["variant"],
                  proof=identity(proof_path), source=selected, ordinary=identity(resaved), exact_owner_families=len(SHARED["EXACT"]))
    owners = [owner]
    if completion is not None:
        accepted = json.loads(completion.read_text(encoding="utf-8"))
        phase = {"earth-before": "earth-complete", "depart-source": "return-source"}.get(proof["phase"], "complete")
        assert proof["phase"] in ("before", "earth-before", "transfer", "depart-source") and accepted["phase"] == phase
        assert accepted["configuration"] == configuration and accepted["model"] == proof["model"] and accepted["variant"] == proof["variant"]
        assert accepted["predecessor"] == identity(proof_path)
        resumed_owner = context.for_feature("space", f'manufacturing-replacement-resumed-{proof["variant"]}-{proof["model"]}')
        resumed, assertions, metadata = SHARED["native"](resumed_owner, configuration, phase, proof["model"], resaved, proof["original"])
        assert metadata["original_alive"] == accepted["original_alive"] and metadata["product"] == accepted["product"] and metadata["asset_digest"] == accepted["asset_digest"]
        target = ROOT / accepted["after"]["path"]; assert identity(target) == accepted["after"]
        expected, actual = chunks(target)[1], chunks(resumed)[1]
        # One actual final opportunity after ordinary reconstruction must converge with the uninterrupted work/material/retirement and native financial/group owners.
        for family in SHARED["EXACT"]:
            assert expected[family] == actual[family], ("resumed", scope, family)
        result.update(completion=identity(completion), resumed=identity(resumed), resumed_input=identity(resaved), assertions=assertions); owners.append(resumed_owner)
    result["native_reports"] = [identity(path) for item in owners for path in sorted(item.reports.glob("native-*.json"))]
    assert identity(source) == selected; report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select an exact native checkpoint and optional uninterrupted counterpart, without latest-path lookup."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--completion", type=Path)
    args = parser.parse_args()
    print(verify(args.configuration, args.proof.resolve(), args.completion.resolve() if args.completion else None).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
