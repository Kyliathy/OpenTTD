"""Refresh exact original selected geothermal cases through existing native observers.

@file selected_geothermal_refresh.py
@see r2mlvs14-remaining-assets-and-runtime-integration.md Group84FC.
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DEC.
One native observation runs at a time;each case keeps its own explicit progress/output.
"""

import argparse
import json
from pathlib import Path
import sys
import uuid

sys.dont_write_bytecode = True
from selected_geothermal_gallery import ROOT, CASES, capture, identity
from geothermal_machinery_raster import verify_art_references


def refresh(output, resume=False):
    """Run or resume exact named original observations without rediscovering or recreating paid worlds."""
    output = (ROOT / output).resolve()
    assert output.is_relative_to(ROOT / "logs")
    if output.exists():
        assert resume
        document = json.loads(output.read_text(encoding="utf-8"))
        assert document["controller"] == identity(__file__) and not document["complete"]
    else:
        document = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84FC;explicit original case refresh index",
                    "controller": identity(__file__), "reports": [], "complete": False}
        output.parent.mkdir(parents=True, exist_ok=True)
    done = set()
    for name in document["reports"]:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_art_references(report, {})
        assert report["complete"] and report["controller"] == identity(ROOT / "src/tests/multilayer/selected_geothermal_gallery.py")
        done.add((report["configuration"], report["case"]))
    output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    for cfg in ("Debug", "RelWithDebInfo"):
        for case in CASES:
            if (cfg, case) in done:
                continue
            target = output.parent / f"geothermal-{cfg}-{case}.json"
            native_report = None
            if target.exists():
                partial = json.loads(target.read_text(encoding="utf-8"))
                if partial.get("native_report"):
                    ref = partial["native_report"]
                    assert identity(ROOT / ref["path"]) == ref
                    native_report = ref["path"]
                target = target.with_name(target.stem + "-recheck-" + uuid.uuid4().hex[:8] + ".json")
            print(json.dumps({"phase": "original_geothermal_case", "configuration": cfg, "case": case, "reused_native": native_report}), flush=True)
            capture(cfg, case, target.relative_to(ROOT).as_posix(), native_report)
            document["reports"].append(target.relative_to(ROOT).as_posix())
            output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    assert len(document["reports"]) == 14
    document["complete"] = True
    output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(output), "complete_cases": 14}), flush=True)


def main():
    """Select one explicit absent/resumable case index;never use an implicit latest native report."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True)
    parser.add_argument("--resume", action="store_true")
    args = parser.parse_args()
    refresh(args.output, args.resume)


if __name__ == "__main__":
    main()
