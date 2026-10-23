"""Run the bounded native physical/owner service interruption burst after originals.

@file first_plant_service_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group9B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FC.
"""
import argparse
import json
from pathlib import Path
from first_plant_service_checks import ROOT, identity, complete_chunks, native, saved, saved_table, current_run_context


def main():
    """Verify original physical loss and paid repair in isolated native processes.

    @return None; retain exact parent proof, child reports and interrupted originals.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-report", type=Path, required=True)
    parser.add_argument("--part", choices=("physical", "owners"), required=True, help="One sub-two-minute isolated interruption burst.")
    args = parser.parse_args()
    parent = json.loads(args.original_report.read_text())
    config = parent["configuration"]
    context = current_run_context("geothermal", "first-plant-service")
    connected, complete = ROOT / parent["connected"]["path"], ROOT / parent["complete"]["path"]
    reports, boundaries = [], {}
    def run(action, source=None, boundary=None, focus=None):
        """Retain one actual isolated successful native interruption or owner boundary.

        @param action Native file action, None for an existing focused owner tag.
        @param source Unchanged actual original from the parent or preceding native case.
        @param boundary One explicit actual physical/owner interruption.
        @param focus One isolated existing producer case tag.
        @return Successful native child report, never a replacement fixture world.
        """
        report, proof = native(config, action, source, boundary, focus)
        reports.append(proof)
        return report
    # Each interrupted clone retains its own ordinary save. Start total source
    # loss from an already useful interval to verify real bonus expiry.
    cases = ("path", "sealed", "terminal", "source") if args.part == "physical" else ("company", "town", "custom", "all-sources")
    for boundary in cases:
        case = run("boundary", complete if boundary == "all-sources" else connected, boundary)
        blocked = saved(case, "blocked")
        inspected = run("inspect", blocked)
        if boundary == "company":
            # The normal loader supplies the missing human player once; the
            # shared old-company tombstones and all other native owners stay
            # exact. A second ordinary load must change no complete chunk.
            assert inspected["input"]["native_new_human_company"] == [0]
            old = dict(complete_chunks(blocked)[1]); after = dict(complete_chunks(saved(inspected, "inspect"))[1])
            assert {name for name in old if old[name] != after[name]} == {"DATE", "PLYR"}
            second = run("inspect", saved(inspected, "inspect"))
            assert complete_chunks(saved(inspected, "inspect")) == complete_chunks(saved(second, "inspect"))
        else:
            assert complete_chunks(blocked) == complete_chunks(saved(inspected, "inspect"))
        boundaries[boundary] = identity(blocked)
        if boundary in ("path", "sealed"):
            repaired = run("repair", blocked)
            assert saved_table(saved(repaired, "complete"), b"SSOR") == saved_table(ROOT / parent["input"]["path"], b"SSOR"), "Repair changed paid material history."
    focuses = ("town-effects", "utility-company", "generator-waits", "generator-zero", "generator-service") if args.part == "owners" else ()
    for focus in focuses:
        run(None, focus=focus)
    report = {"_documentation": "r2mlvs12 Group9B; r2mlvs18 Group8FC; isolated native physical/owner interruption and paid repair",
              "configuration": config, "part": args.part, "parent": identity(args.original_report), "controller": identity(__file__),
              "reports": reports, "boundaries": boundaries}
    path = context.output("reports", f"first-plant-service-{config}-{args.part}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Surface service boundaries accepted in {config}: {len(reports)} isolated reports\n{path}", flush=True)


if __name__ == "__main__":
    main()
