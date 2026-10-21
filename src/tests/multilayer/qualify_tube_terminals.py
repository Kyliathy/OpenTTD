"""Join both actual native tube terminal runs and retain their tested images.

@file qualify_tube_terminals.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md task10.5.
This boundary proof covers real station/depot admission, paid capsule return and
packet/payment handling. It does not close five-model, final-art or TBM-cleared
cross-layer useful-route acceptance.
"""

import json
import shutil
from pathlib import Path

from ground_pack_check_support import ROOT, handoff
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import reference
from tube_terminal_checks import BASELINE

RUN = ROOT / "logs/underground-tubes/20261021-terminal-01"
TAGS = {"transport-tube-model", "transport-tube-construction", "transport-tube-readiness",
        "transport-tube-platform-geometry", "transport-tube-meter", "transport-tube-interchange-geometry",
        "transport-tube-interchange", "transport-tube-interchange-delivery"}


def exact(row):
    """Reject a changed source, executable or terminal before reusing its proof.

    @param row Exact repository path/checksum reference from its actual execution.
    @return Resolved current repository path, only after full checksum equality.
    """
    path = handoff.bound(row["path"])
    if reference(path) != row:
        raise ValueError("tube qualification identity changed: " + row["path"])
    return path


def retain(path, target):
    """Keep the executed source/image without overwriting a previous epoch.

    @param path Verified current repository input or executable.
    @param target Exact repository logs destination for this qualified epoch.
    @return Exact retained path/checksum reference.
    """
    if target.exists():
        if reference(target)["sha256"] != reference(path)["sha256"]:
            raise ValueError("retained tube epoch already differs: " + str(target))
    else:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, target)
    return reference(target)


def main():
    """Require both complete focused native runs, then publish their bounded handoff.

    @return None; refuse missing/partial/stale observations before creating a successful receipt.
    """
    transitions, count = preserved(BASELINE)
    results, sources = {}, {}
    # Each configuration must independently execute the exact active provider
    # and cargo matrix. Matching counts cannot substitute for missing tags or
    # a process that never reached native Catch completion.
    for configuration in ("Debug", "RelWithDebInfo"):
        receipt = RUN / ("final-" + configuration.lower()) / "run.json"
        run = json.loads(receipt.read_text(encoding="utf-8"))
        observation = run["observation"]
        selected = {tag.strip("[]") for tag in observation["filter"].split(",")}
        valid = (run["passed"] is True and run["configuration"] == configuration and run["mode"] == "run"
                 and observation["exit_code"] == 0 and observation["cases"] == 24
                 and observation["assertions"] == 8687 and selected == TAGS)
        if not valid:
            raise ValueError("incomplete tube terminal native matrix: " + configuration)
        application = exact(run["application"])
        exact(observation["terminal"])
        # Preserve all executed feature/fixture inputs once. The original
        # before snapshot separately protects existing unrelated games/art and
        # records the narrowly permitted old-owner edits.
        for row in run["inputs"]:
            source = exact(row)
            sources[row["path"]] = retain(source, RUN / "retained/source" / row["path"])
        results[configuration] = {"execution": reference(receipt), "observation": observation,
                                  "tested_image": retain(application, RUN / "retained" / configuration / application.name)}
    proof = {"_documentation": __doc__, "passed": True, "configurations": results,
             "inputs": [reference(Path(__file__)), reference(handoff.bound(BASELINE))], "retained_sources": sources,
             "preserved_file_count": count, "transitions": transitions,
             "transfer_capacity_per_cargo": 1024, "native_packet_owner_only": True,
             "paid_native_capsule_empty_return": True, "rail_and_road_final_income_once": True,
             "fixture_boundaries": ["supplied excavated floor and commissioned inlet", "customer packets and positive freight rate",
                                    "final conventional carrier placed at Surface to isolate payment"],
             "operating_models": [9], "all_five_models": False, "final_art": False,
             "tbm_cleared_cross_layer_useful_route": False, "whole_underground_complete": False}
    (RUN / "qualification.json").write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"passed": True, "assertions_per_configuration": 8687, "cases_per_configuration": 24,
                      "protected_previous_files": count}))


if __name__ == "__main__":
    main()
