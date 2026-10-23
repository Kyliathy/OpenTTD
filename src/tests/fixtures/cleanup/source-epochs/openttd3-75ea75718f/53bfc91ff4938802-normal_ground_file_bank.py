"""Select exact immutable normal-world originals for ordinary native file acceptance.

@file normal_ground_file_bank.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DDC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Selection consumes dated outer receipts; later builds do not rewrite their original source bindings.
"""

import argparse
import json
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from save_tables import saved_table

HOME = "logs/acceptance/bootstrap/20261019-native-01/"
RECEIPTS = {
    "normal-start": "244583e6c91754ecb51949d23c212a0fcea36f0f07e833a2e9540074ec4d5cdc",
    "standard-machinery": "ef5789972ab288e7d94053ad36a260913502f6efcd6185004ff38474a79e1196",
    "standard-work": "01fd7e457f48e68dec9233b6572e919f37a4649251a54432d4d6bf79d4cb3245",
    "deep-machinery": "7796f578643d6bbb419d479ed6da1f38e5bcd61e2a0d6137734ec8c990a2564a",
    "deep-work": "bbfd942ca5a9c0057aa4d21d5f984d54f310556d7f379e607f39e885b27c90a0",
    "irregular-readiness": "db0f76085ab86af039845793a8a6bcd1612126a76baff8d38504f93671c808a5",
    "irregular-work": "fc09a953514dbec4748ed5d9defe77f8b45820cdc3769cd6edf9ad15e326ae54",
    "modal-layout": "adf17a20f1787d4d9cf8b516ff0afa93eda79d1f75538736db45cf25ddfa4624",
    "useful-supply": "38b5f0f1d8042aac697f7e33b368c93ebc38c2cfdd6fe98e70a8c9efb7ddb120",
    "useful-operation": "af16a8ef34eea33803a5f713118c133e4e9da2e8d27a8953523ed13170efda6f",
}


def make_bank(checkpoint_path, checkpoint_sha, output):
    """Bind originals/configs/next baselines solely to their exact accepted predecessor receipts."""
    records = {cfg: {} for cfg in ("Debug", "RelWithDebInfo")}
    refs = {}
    def select(cfg, phase, original, config, receipt, baseline=None):
        """Require physical original identity and retain its actual native clock/ID/cash metadata."""
        path = verify_reference(original)
        verify_reference(config)
        date = saved_table(path, b"DATE")[0]
        company = saved_table(path, b"PLYR")[0]
        assert date["pause_mode"] == 1
        row = {"original": original, "config": config, "predecessor": receipt,
               "tick": date["tick_counter"], "id": date["id"], "cash": company["money"], "loan": company["current_loan"]}
        if baseline:
            verify_reference(baseline)
            row["baseline"] = baseline
        assert phase not in records[cfg]
        records[cfg][phase] = row
    for name, digest in RECEIPTS.items():
        path = ROOT / (HOME + name + "-verification.json")
        reference = identity(path)
        assert reference["sha256"] == digest
        refs[name] = reference
        receipt = json.loads(path.read_text(encoding="utf-8"))
        for cfg, record in receipt["records"].items():
            if name == "normal-start":
                # The initial generation save is deliberately unpaused. This
                # bank selects the original lineage's paused physical stages,
                # retaining the generation receipt as their identity ancestor.
                continue
            if name == "useful-operation":
                first = record["records"][0]["actual"]
                select(cfg, "loaded-useful", first["saves"]["progress"], first["config"], reference)
                last = record["records"][-1]["actual"]
                select(cfg, "final-useful", record["settled_input"], last["config"], reference)
            elif name.endswith("work"):
                select(cfg, "settled-" + name.removesuffix("-work"), record["settled_input"], record["records"][-1]["actual"]["config"], reference)
            else:
                phase = {"normal-start": "normal-start", "standard-machinery": "sealed-standard", "deep-machinery": "sealed-deep",
                         "irregular-readiness": "sealed-irregular", "modal-layout": "paid-layout", "useful-supply": "ready-useful"}[name]
                original = record["layout_input"] if name == "modal-layout" else record["ready_input"]
                select(cfg, phase, original, record["actual"]["config"], reference)
    checkpoints = identity(ROOT / checkpoint_path)
    assert checkpoints["sha256"] == checkpoint_sha
    checkpoint_receipt = json.loads(verify_reference(checkpoints).read_text(encoding="utf-8"))
    assert checkpoint_receipt["classification"] == "normal_start_working_files"
    refs["working-files"] = checkpoints
    for cfg, record in checkpoint_receipt["records"].items():
        for phase in ("working", "loaded"):
            select(cfg, "working-bore" if phase == "working" else "loaded-spoil", record["checkpoints"][phase]["original"], record["config"], checkpoints,
                   record["checkpoints"][phase + "-next"]["original"])
    assert set(records["Debug"]) == set(records["RelWithDebInfo"]) and len(records["Debug"]) == 12
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md6DDC;exact original normal-world file selection,not native acceptance",
              "classification": "normal_start_file_bank", "bootstrap_claim": False, "controller": identity(__file__),
              "receipts": refs, "records": records, "original_files": 24}
    target = ROOT / output
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"bank": identity(target), "original_files": 24}), flush=True)


def main():
    """Select the immutable completed checkpoint receipt explicitly."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--checkpoints", required=True)
    parser.add_argument("--checkpoints-sha", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    make_bank(args.checkpoints, args.checkpoints_sha, args.output)


if __name__ == "__main__":
    main()
