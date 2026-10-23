"""Query supported original rail productivity without changing physical owners.

@file surface_entry_rail_productivity_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZV.
Actual native demand, command invoices and full operating costs supply evidence;
neither a pure quote nor an unbanked asset authorizes purchase or earned work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_rail_productivity_native import execute as native
from surface_original_evidence import verify

VERSIONS = ROOT / "logs/surface-entry-rail-productivity-before-edit.json"

HERE = Path(__file__).resolve().parent
RAIL = runpy.run_path(str(HERE / "surface_entry_goods_rail_delivery_checks-1.py"))
observe = RAIL["observe"]
PHASE = "rail-productivity-query"


def execute(configuration, recover_run=None):
    """Inspect one exact current original through the isolated native rail query.

    @param configuration Original independently linked native build configuration.
    @param recover_run Exact previous native invocation to inspect without repeating it.
    @return Actual coupled capacity, supported refits, connectivity and unchanged whole-company bill.
    @throws AssertionError On unqualified source changes, native mutation or inconsistent real capacity.
    """
    receipt = ROOT / f"logs/surface-entry-productive-admission-{configuration}.json"
    previous = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
    assert previous["earned_ticks"] == 0 and not previous["productive_consumer_admitted"]
    # Only the explicitly archived prior isolated translation unit may differ;
    # old observers, financial sources and original application bytes stay exact.
    for ref in previous["sources"]:
        verify(ref, [VERSIONS])
    archive = ROOT / "logs/surface-entry-productive-admission-before-edit.json"
    protected = json.loads(archive.read_text(encoding="utf-8"))["files"]
    assert all(identity(ROOT / ref["path"])["sha256"] == ref["sha256"] for ref in protected
               if ref["path"].endswith(".exe"))
    files = [ROOT / ref["path"] for ref in previous["sources"] if verify(ref, [VERSIONS])] + [
        Path(__file__), VERSIONS, ROOT / "src/tests/surface_entry_rail_productivity.h"]
    attempt = ROOT / f"logs/surface-entry-rail-productivity-{configuration}-attempt.json"
    if recover_run is None:
        assert not attempt.exists(), "recover the original query invocation instead of repeating it"
        attempt.write_text(json.dumps(dict(receipt=identity(receipt), native_invocations=1, earned_ticks=0),
            indent=2)+"\n", encoding="utf-8")
    else:
        assert attempt.is_file()
    operation = native(configuration, PHASE, receipt, observe,
        dict(source="RAIL_PRODUCTIVITY_SOURCE", trains="RAIL_PRODUCTIVITY_TRAIN", parts="RAIL_PRODUCTIVITY_PART",
             refits="RAIL_PRODUCTIVITY_REFIT", edges="RAIL_PRODUCTIVITY_EDGE",
             wagon_lots="RAIL_PRODUCTIVITY_WAGONS"), files, recover_run=recover_run)
    assert json.loads(json.dumps(operation["initial"])) == previous["final"]
    assert operation["initial"] == operation["final"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    assert source["cash"] == 45080 and source["reserve"] == 40000 and source["debt"] == 5000000
    assert source["paid"] == source["elapsed"] == 0
    assert source["target"] == source["operating"]+40000 and source["funded"]
    assert len(operation["rows"]["parts"]) == 35
    assert {row["head"] for row in operation["rows"]["trains"]} == {42, 49, 52}
    # Whole native train summaries must agree with their actual coupled parts.
    # A positive GUI estimate remains distinct from ordinary depot admission,
    # currently held freight and a genuinely funded complete productive route.
    for train in operation["rows"]["trains"]:
        parts = [row for row in operation["rows"]["parts"] if row["head"] == train["head"]]
        assert train["parts"] == len(parts)
        assert train["capacity"] == sum(row["capacity"] for row in parts)
        assert train["held"] == sum(row["held"] for row in parts)
    assert all(row["supported"] == (row["gui_available"] and row["capacity"] > 0)
               and not row["work_permission"] for row in operation["rows"]["refits"])
    assert all(row["target"] == 40000+row["existing_operating"]+row["invoice"]+row["added_upkeep"]
               and row["funded"] == (row["target"] <= 45080) and not row["work_permission"]
               and row["future_income_counted"] == row["refund_counted"] == 0
               for row in operation["rows"]["wagon_lots"])
    operation.update(before_edit=identity(archive), earned_ticks=0, paid=0,
        protected_original_applications=[ref for ref in protected if ref["path"].endswith(".exe")],
        historical_versions=[identity(VERSIONS)],
        supported_changes=[row for row in operation["rows"]["refits"] if row["supported"]
                           and row["requested_cargo"] != row["original_cargo"]],
        funded_wagon_lots=[row for row in operation["rows"]["wagon_lots"] if row["funded"]],
        productive_consumer_admitted=False, startup_reserve=40000)
    return operation


def main():
    """Retain exactly one original pure productive admission or recover its exact native invocation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    output = ROOT / f"logs/surface-entry-rail-productivity-{args.configuration}.json"
    assert not output.exists()
    operation = execute(args.configuration, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase=PHASE, operation=operation), indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), "supported changes", operation["supported_changes"], flush=True)


if __name__ == "__main__":
    main()
