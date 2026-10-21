"""Audit the delivered orbital handoff without replaying historical native worlds.

@file orbital_handoff_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group16.
This documentation/identity audit reuses accepted runtime boundaries. It cannot
close the unfinished genuine Surface-funded chain or authorize another interval.
"""
import argparse
import json
from pathlib import Path
import re
import launch_facility_selector_checks as selectors
import launch_fleet_payload_checks as payload

ROOT = selectors.ROOT
PLAN = ROOT / "docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md"
EXPORTS = {
    "src/orbital_station.h": ("OrbitalConstructionJob", "OrbitalModule",
        "ReserveOrbitalConstructionInputs", "CommitOrbitalConstructionStep"),
    "src/orbital_manufacturing.h": ("AppendOrbitalManufacturingRequests",
        "TickOrbitalManufacturingReservations", "TickOrbitalManufacturingWork",
        "QueueOrbitalReplacement", "ValidateOrbitalManufacturingProductionLinks"),
    "src/orbital_recipe.h": ("OrbitalRecipeInventory", "OrbitalRecipeState",
        "TickOrbitalRecipes", "ConnectOrbitalRecipePower", "ValidateOrbitalRecipeRecords"),
    "src/space_vehicle.h": ("SpaceVehicle",),
    "src/space_orders.h": ("SpaceOrderQueue", "TickSpaceOrders"),
    "src/multilayer_space_content.h": ("GetSpaceConstructionDefinitions",
        "GetSpaceVehicleConstruction", "IsSupportedSpaceVehicleContentVersion"),
    "src/orbital_freight.h": ("GetOrbitalModuleFreightBlock", "CommitOrbitalModuleFreight"),
    "src/launch_assembly_gui.h": ("ShowLaunchCraftWindow", "GetLaunchCraftWindowSelection",
        "GetLaunchPodWindowSelection"),
    "src/space_survey_gui.h": ("GetSpaceSurveySelection",),
}
RECEIPTS = ("orbital-handoff-debug-verification.json", "manufacturing-replacement-verification.json",
    "orbital-runtime-verification.json", "orbital-processor-verification.json",
    "heavy-role-runtime-verification.json", "heavy-network-verification.json",
    "processor-freight-verification.json", "processor-returns-verification.json",
    "orbital-art-ordinary-verification.json", "orbital-art-refusal-verification.json",
    "orbital-construction-save-verification.json", "orbital-materialization-save-verification.json",
    "orbital-processing-save-verification.json", "surface-entry-joint-cycle-reserve-verification.json",
    "proofs/mlm/native-art/space/launch-presentation-v1/verification.json")


def section(path, heading):
    """Select only the current handoff section governed by this documentation task.

    @param path Actual owning Markdown file.
    @param heading Exact current owner heading, excluding historical reading queues.
    @return Current section text through the next heading at the same level.
    @throws ValueError When the documented current owner section is absent.
    """
    text = path.read_text(encoding="utf-8")
    start = text.index(heading)
    end = text.find("\n## ", start + len(heading))
    return text[start:] if end < 0 else text[start:end]


def audit():
    """Bind actual exports, current documentation and remaining acceptance limits.

    @return Reproducible repository-local documentation handoff receipt.
    @throws ValueError On missing exports/links, incomplete evidence or false chain closure.
    """
    original_applications = payload.preserved_applications()
    exports = []
    # Public declarations remain the documented owner boundary. A mention in a
    # comment is insufficient; this audit never creates a substitute implementation.
    for filename, names in EXPORTS.items():
        path = ROOT / filename
        text = path.read_text(encoding="utf-8")
        for name in names:
            pattern = r"^\s*(?:(?:struct|class)\s+" + name + r"\b|[A-Za-z_][^\n/;]*\b" + name + r"\s*\()"
            if re.search(pattern, text, re.MULTILINE) is None:
                raise ValueError("documented public export absent: " + name)
        exports.append(dict(source=selectors.art.source.identity(path), declarations=names))
    receipts = []
    # Accepted aggregate identities retain their actual dated scope. This handoff
    # does not re-run old sources/executables or turn supplied stock into procurement.
    for filename in RECEIPTS:
        path = ROOT / "logs" / filename
        report = selectors.art.read_json(path)
        if report.get("complete") is False:
            raise ValueError("incomplete accepted handoff: " + filename)
        receipts.append(selectors.art.source.identity(path))
    financial = selectors.art.read_json(ROOT / "logs/surface-entry-joint-cycle-reserve-verification.json")
    # Documentation cannot spend a refused interval or erase the original funding
    # shortfall; those conditions keep the genuine-chain completion gate open.
    if (financial["actual_reserve_restored"] or financial["module_capital_complete"]
            or financial["working_gap"] != 52161 or financial["incomplete_module_base_gap"] != 462161):
        raise ValueError("closed financial/continuous-chain boundary changed")
    text = PLAN.read_text(encoding="utf-8")
    # Documentation can close independently. Genuine-chain and reserve tasks
    # remain unchecked until their own written physical acceptance passes.
    for task in ("15.1", "15.2", "15CS.2", "15CS.3"):
        if re.search(r"^- \[ \] " + re.escape(task) + r"\b", text, re.MULTILINE) is None:
            raise ValueError("unfinished actual chain task incorrectly closed: " + task)
    documents = []
    owners = ((PLAN, "## Current runtime handoff - 2026-10-20"),
        (ROOT / "docs/zz-reach2/architecture/vehicles/archi-vehicles.md", "## Native orbital manufacturing and current presentation"),
        (ROOT / "docs/zz-reach2/architecture/map-layers/archi-map-layers.md", "## Delivered orbital and launch presentation boundaries"))
    link_count = 0
    # Check only the current owner sections: older architecture provenance is
    # outside this scoped handoff and must not become a transitive audit queue.
    for path, heading in owners:
        current = section(path, heading)
        for target in re.findall(r"\[[^\]]*\]\(([^)]+)\)", current):
            destination = (path.parent / target.split("#", 1)[0]).resolve()
            if not destination.is_file():
                raise ValueError("missing current handoff link: " + target)
            link_count += 1
        documents.append(selectors.art.source.identity(path))
    payload.preserved_applications()
    receipt = dict(_documentation=__doc__, complete=True, group="16", exports=exports,
        accepted_aggregate_receipts=receipts, documents=documents, verified_local_links=link_count,
        original_applications=original_applications, native_processes_replayed=0,
        genuine_chain_complete=False, financial_scope_reopened=False,
        conditional_parent_completion_performed=False,
        remaining_tasks=["15.1", "15.2", "15CS.2", "15CS.3"],
        verifier=selectors.art.source.identity(Path(__file__).resolve()))
    output = ROOT / "logs/orbital-current-handoff-verification.json"
    output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print("current orbital handoff:", sum(len(names) for names in EXPORTS.values()),
          "exports,", len(receipts), "accepted aggregates,", link_count,
          "current links; continuous chain remains open", flush=True)
    return receipt


def reserve_proposal():
    """Price a single possible startup bridge from the closed original native quotes.

    @return Read-only two-build proposal; no changed floor, native work or authorization.
    @throws ValueError When original refusal, whole interval bill or counterpart books differ.
    """
    payload.preserved_applications()
    sources = []
    quotes = []
    # A proposed lower floor cannot borrow future fares or silently continue
    # the closed trial. Both original full-interval quotes must agree first.
    for configuration in ("RelWithDebInfo", "Debug"):
        path = ROOT / ("logs/surface-entry-joint-cycle-reserve-005-" + configuration + ".json")
        report = selectors.art.read_json(path)
        operation = report["operation"]
        scope = operation["working_scope"]
        quote = next(row for row in operation["rows"]["admission"] if row["ticks"] == 2000)
        if (not report["complete"] or not scope["ended"] or scope["end_reason"] != "funding_refused"
                or quote["cash"] != 47839 or quote["operating"] != 12712
                or quote["reserve"] != 40000 or quote["future_income_counted"] != 0
                or quote["funded"]):
            raise ValueError("closed native bridge price/refusal changed")
        sources.append(selectors.art.source.identity(path))
        quotes.append(quote)
    if quotes[0] != quotes[1]:
        raise ValueError("counterpart original whole-interval quotes differ")
    proposed_floor = 30000
    result = dict(_documentation=__doc__, read_only=True, authorized=False,
        sources=sources, original_quote=quotes[0], candidate_floor=proposed_floor,
        candidate_full_bill_plus_floor=quotes[0]["operating"] + proposed_floor,
        current_cash=quotes[0]["cash"],
        candidate_admission_margin=quotes[0]["cash"] - quotes[0]["operating"] - proposed_floor,
        proposed_tick_cap=2000, proposed_windows=1, stop_at_first_posted_rail_payment=True,
        future_income_counted=0, native_ticks_earned=0, current_default_reserve=40000,
        existing_scopes_reopened=False, repeat_authorized=False,
        verifier=selectors.art.source.identity(Path(__file__).resolve()))
    output = ROOT / "logs/surface-entry-reserve-bridge-proposal.json"
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("read-only bridge proposal: full bill12712 + candidate30000 =42712, cash47839, margin5127; unauthorized", flush=True)
    return result


def main():
    """Audit the current handoff or price one explicit read-only reserve proposal."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reserve-proposal", action="store_true")
    if parser.parse_args().reserve_proposal:
        reserve_proposal()
    else:
        audit()


if __name__ == "__main__":
    main()
