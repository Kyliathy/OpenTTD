"""Read original actual paid queue/power worlds and compare every full native chunk.

@file tube_tbm_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16D.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Each bounded invocation reuses one serial runtime home for three isolated native children.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
from construction_provider_return import ROOT, identity
from tube_tbm_queue import SOURCES, STAGES
from tube_tbm_useful import OWNER_KEYS, native_loaded
from tube_tbm_delivery import USEFUL_KEYS
from runner import current_run_context
from save_tables import chunks, read_stream

QUEUE = ("logs/tubes/tbm-final-use/20261019-native-01/queue-verification.json",
         "8f52c4afddab198b36f44b48cfba2ca3ccffe59a4878ae44adc569a4ab86ebe0")


def complete_world(path):
    """Read all original native saved authority without filtering fields or unknown chunks.

    @param path Existing genuine native434 original or ordinary output.
    @return Complete unique-name raw chunks;any unsupported version or missing owner refuses.
    """
    version, stream = read_stream(path)
    assert version == 434
    rows = chunks(stream)
    result = {row["name"].decode("ascii"): row["raw"] for row in rows}
    assert len(result) == len(rows) == 126
    return result


def equal_world(first, second):
    """Require exact whole native ownership,including diagnostic and reservation transport.

    @param first Explicit immutable original or uninterrupted baseline.
    @param second Actual ordinary native output from the selected reader.
    @return Complete126-chunk equality summary;no revision or gameplay field is ignored.
    """
    original, loaded = complete_world(first), complete_world(second)
    assert original.keys() == loaded.keys()
    different = [name for name in original if original[name] != loaded[name]]
    assert not different, (str(first), str(second), different)
    return {"complete_equal_chunks": 126,
            "chunk_sha256": {name: hashlib.sha256(raw).hexdigest() for name, raw in loaded.items()}}


def run_stage(configuration, queue_report, stage):
    """Accept one actual original phase through repeated/opposite-reader/full-next native files.

    @param configuration Current reader,opposite the accepted queue/recovery writer.
    @param queue_report Explicit original report selected by the sealed16D.1 handoff.
    @param stage Actual queued,starved,restored or recovered native boundary.
    @return Phase report only after three isolated children and every complete native comparison pass.
    """
    receipt_path = ROOT / QUEUE[0]
    receipt_ref = identity(receipt_path)
    assert receipt_ref["sha256"] == QUEUE[1]
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    selected = identity(queue_report)
    record = next(row for row in receipt["records"].values() if row["report"] == selected)
    assert record["configuration"] != configuration
    original, baseline = record["outputs"][stage], record["outputs"][stage + "-next"]
    assert identity(ROOT / original["path"]) == original
    assert identity(ROOT / baseline["path"]) == baseline
    participants = dict(record["useful_participants"])
    participants["source"] = record["observations"]["old_source" if stage in ("queued", "starved") else "fresh_source"]
    context = current_run_context("tubes", "tbm-final-use")
    tag = "[.tube-tbm-queue-file]"
    cases, first_output = [], None
    # Three independent real processes read the original,resume its ordinary
    # resave,and execute the untouched next opportunity. One serial runtime
    # owner avoids copying the same native distribution for every cold child.
    for action in ("read", "reload", "next"):
        source_ref = first_output if action == "reload" else original
        source = ROOT / source_ref["path"]
        label = f"file-{stage}-{action}"
        steps = int(action == "next")
        environment = {"OTTD_TUBE_TBM_INPUT": str(source),
                       "OTTD_TUBE_TBM_OWNERS": ",".join(str(record["original_owners"][key]) for key in OWNER_KEYS),
                       "OTTD_TUBE_TBM_USEFUL": ",".join(str(participants[key]) for key in USEFUL_KEYS),
                       "OTTD_TUBE_TBM_FILE": f"{record['observations']['peer']},{steps}",
                       "OTTD_TUBE_TBM_FILE_LABEL": label}
        raw, proof = native_loaded(context, configuration, tag, environment, label)
        assert f"TUBE_TBM_FILE label={label} step={steps} full_native_owners=1" in raw
        saved = re.search(rf"TUBE_TBM_USEFUL_SAVE stage={label} path=([^\r\n]+)", raw)
        assert saved is not None
        output = Path(saved[1]).resolve()
        assert output == context.output("saves", f"tube-tbm-{label}.sav").resolve()
        expected = baseline if steps else original
        comparisons = {"original_or_full_next": equal_world(ROOT / expected["path"], output)}
        repeat_ref = None
        if steps == 0:
            repeated = re.search(rf"TUBE_TBM_USEFUL_SAVE stage={label}-repeat path=([^\r\n]+)", raw)
            assert repeated is not None
            repeat = Path(repeated[1]).resolve()
            assert repeat == context.output("saves", f"tube-tbm-{label}-repeat.sav").resolve()
            comparisons["pure_repeat"] = equal_world(output, repeat)
            repeat_ref = identity(repeat)
        assert identity(source) == source_ref
        output_ref = identity(output)
        if action == "read":
            first_output = output_ref
        cases.append({"action": action, "input": source_ref, "output": output_ref, "repeat": repeat_ref,
                      "native": proof, "comparisons": comparisons})
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md16D.2;ordinary original/opposite-reader/reloaded/pure-repeat/full-next acceptance of actual paid queue/power/packet/reservation worlds;all126chunks retained exactly without exception",
        "configuration": configuration, "focus": stage, "tag": tag, "native_version": 434,
        "queue_receipt": receipt_ref, "queue_report": selected, "original": original, "baseline": baseline,
        "cases": cases, "native_cases": 3, "native_assertions": sum(case["native"]["assertions"] for case in cases),
        "complete126_chunk_comparisons": sum(len(case["comparisons"]) for case in cases),
        "controller": identity(__file__), "native_loader": identity(ROOT / "src/tests/multilayer/tube_tbm_useful.py"),
        "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
    }
    path = context.output("reports", f"tube-tbm-{configuration}-files-{stage}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Select one bounded native file phase and its exact accepted opposite original.

    @return None;altered originals,missing owners,unequal packets/grants/reservations or next steps refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--queue-report", type=Path, required=True)
    parser.add_argument("--stage", choices=STAGES, required=True)
    args = parser.parse_args()
    print(run_stage(args.config, args.queue_report, args.stage), flush=True)


if __name__ == "__main__":
    main()
