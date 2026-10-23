"""Quote genuine additional Goods capacity through native owners at zero ticks.

@file space_goods_capacity.py Explicit Plan11GZVEEI query and successor validator.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group36.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEEI.
@see docs/zz-reach2/architecture/archi-historical-debug.md Checkpoints, cleanup and transfer.
The native producer owns availability, packets, topology and cost estimates.
This adapter binds its actual process, unchanged saves and ordinary reload.
"""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import create_run_context
from runner import run_native
from space_checkpoint import CONFIGURATIONS, POLICY, ORIGINAL_OBSERVER, load_foundation, verify_foundation, publish_successor
from space_checkpoint_foundation import read, validate_build_document
from space_checkpoint_load import checked, identity, compare_clocks, compare_revision
from space_checkpoint_operation import _scope
from space_operation_restore import execute_restore
from space_preview_checks import chunks

TASK = "Plan11/15CSGZVEEI"
PHASE = "goods-capacity-query"
TAG = "[.space-goods-capacity]"
SCENARIO = "goods-capacity"
SELF = "src/tests/multilayer/acceptance/space_goods_capacity.py"
SOURCE_FILES = ("src/tests/space_goods_capacity.h", "src/tests/surface_entry_productive_admission.cpp",
                "src/train_cmd.cpp", "src/vehicle.cpp",
                "src/tests/surface_entry_resupply.cpp", "src/tests/surface_entry_module_capital.h",
                "src/tests/surface_entry_income_capitalization.h", SELF,
                "src/tests/multilayer/acceptance/space_operation_restore.py",
                "src/tests/multilayer/acceptance/space_checkpoint_operation.py")
BOOLS = set("available callback_free quoted accepts source_connected receiver_connected quote_complete year_boundary fits funded spending_permission inflation age_limited".split())
SCHEMAS = {
    "SPACE_GOODS_PACKET": set("station id next first source_tile quantity".split()),
    "SPACE_GOODS_STATION": set("station tile available packet_quantity packets accepts platform owner".split()),
    "SPACE_GOODS_EXISTING": set("head parts capacity held length remaining_days".split()),
    "SPACE_GOODS_EXPECTED": set("engine available callback_free quoted error invoice capacity".split()),
    "SPACE_GOODS_WAGON": set("engine quoted error invoice capacity annual length life_days age_limited".split()),
    "SPACE_GOODS_HEAD": set("engine quoted error invoice annual source_connected receiver_connected length life_days".split()),
    "SPACE_GOODS_BUDGET": set("cash debt depot ticks days months upkeep interest station capital_floor operating_floor head wagon head_invoice wagon_invoice head_upkeep wagon_upkeep wagon_capacity head_length wagon_length platform_length quote_complete year_boundary future_fares refunds credit paid".split()),
    "SPACE_GOODS_LOT": set("wagons capacity invoice added_upkeep existing_operating target cash length platform_length fits quote_complete funded spending_permission".split()),
    "SURFACE_RESUPPLY_INCOME_FIXED_PRICES": set("ticks inflation callback_free priced_bodies year_boundary days months upkeep interest station_charge".split()),
    "SURFACE_RESUPPLY_CAPITAL_SERVICE": set("vehicle engine annual256 pending remaining profit_last profit_this orders".split()),
}


def parse_row(line, fields):
    """Decode one exact native diagnostic row without accepting duplicate fields.

    @param line Actual terminal line, including its known marker prefix.
    @param fields Exact field names owned by that marker's native producer.
    @return Canonical integer/boolean fields; malformed or ambiguous data refuses.
    """
    tokens = [value.split("=", 1) for value in line.split()[1:]]
    if any(len(token) != 2 for token in tokens):
        raise ValueError("malformed native Goods diagnostic field")
    raw = dict(tokens)
    # Duplicate or missing fields can hide a discounted invoice or an earlier
    # refusal behind a later value; the complete native row must be unambiguous.
    if len(raw) != len(tokens) or set(raw) != fields:
        raise ValueError("native Goods diagnostic fields disagree")
    result = {}
    # Station availability measures waiting units; model availability is a
    # Boolean admission decision. Preserve those distinct native meanings.
    booleans = BOOLS - {"available"} if line.startswith("SPACE_GOODS_STATION ") else BOOLS
    # Decode quantities exactly. Strings, floats and numeric Boolean substitutes
    # cannot turn a refusal into funding or alter the native financial totals.
    for name, value in raw.items():
        if name in booleans:
            if value not in ("true", "false"):
                raise ValueError("native Goods Boolean is not canonical")
            result[name] = value == "true"
        else:
            if re.fullmatch(r"-?(?:0|[1-9][0-9]*)", value) is None:
                raise ValueError("native Goods quantity is not a canonical integer")
            result[name] = int(value)
    return result


def indexed(rows, key, expected=None):
    """Require one native owner row for each declared identity.

    @param rows Actual parsed diagnostic rows of one role.
    @param key Native owner/model/lot identity field.
    @param expected Optional exact required identity set.
    @return Identity-indexed rows without duplicates or omitted required owners.
    """
    result = {row[key]: row for row in rows}
    # Multiple claims for one owner or a missing requested lot would make the
    # proposal incomplete; no convenient subset may replace the full query.
    if len(result) != len(rows) or expected is not None and set(result) != set(expected):
        raise ValueError("native Goods owner/lot coverage disagrees")
    return result


def parse_proposal(text):
    """Validate actual native Goods demand and whole-capital proposal arithmetic.

    @param text Complete successful selected native transcript.
    @return Measured proposal/refusal, retaining all real models and packet origins.
    """
    rows = {name: [] for name in SCHEMAS}
    # Keep every task-owned row and the existing native full-company cost rows.
    # Other unchanged historical helper diagnostics supply no extra permission.
    for line in text.splitlines():
        marker = line.split(" ", 1)[0]
        if marker in rows:
            rows[marker].append(parse_row(line, SCHEMAS[marker]))
        elif marker.startswith("SPACE_GOODS_"):
            raise ValueError("unknown native Goods diagnostic role")
    stations = indexed(rows["SPACE_GOODS_STATION"], "station", {13, 15})
    existing = indexed(rows["SPACE_GOODS_EXISTING"], "head", {49, 52})
    expected = indexed(rows["SPACE_GOODS_EXPECTED"], "engine", {24, 32})
    heads = indexed(rows["SPACE_GOODS_HEAD"], "engine")
    wagons = indexed(rows["SPACE_GOODS_WAGON"], "engine")
    packets = indexed(rows["SPACE_GOODS_PACKET"], "id")
    lots = indexed(rows["SPACE_GOODS_LOT"], "wagons", {2, 4, 8, 12})
    services = indexed(rows["SURFACE_RESUPPLY_CAPITAL_SERVICE"], "vehicle")
    # One actual full-interval cost owner must explain the complete proposal;
    # mixed transcripts or absent company charges cannot qualify any lot.
    if len(rows["SPACE_GOODS_BUDGET"]) != 1 or len(rows["SURFACE_RESUPPLY_INCOME_FIXED_PRICES"]) != 1:
        raise ValueError("Goods proposal needs one whole-company interval")
    budget = rows["SPACE_GOODS_BUDGET"][0]
    bill = rows["SURFACE_RESUPPLY_INCOME_FIXED_PRICES"][0]
    required = dict(cash=182302, debt=5000000, depot=1638, ticks=2000, capital_floor=100000,
                    operating_floor=40000, future_fares=0, refunds=0, credit=0, paid=0)
    if any(budget[name] != value for name, value in required.items()):
        raise ValueError("Goods proposal changed the original cash/closed financial policy")
    if any(budget[name] < 0 for name in ("upkeep", "interest", "station", "head_invoice", "wagon_invoice", "head_upkeep", "wagon_upkeep")):
        raise ValueError("Goods proposal contains negative cost credit")
    # Existing service ownership, pending ticks, interest and station charges
    # come from the unchanged full-cost native owner, never a new local forecast.
    if (bill["inflation"] is not False or bill["callback_free"] is not True or not services
            or bill["priced_bodies"] != len(services) or bill["station_charge"] != budget["station"]
            or any(bill[name] != budget[name] for name in ("ticks", "days", "months", "upkeep", "interest", "year_boundary"))):
        raise ValueError("Goods proposal omitted or changed existing company costs")
    # Packet identities/counts conserve real waiting stock. Acceptance and
    # native platform lengths remain observations, with no invented new demand.
    for station_id, station in stations.items():
        owned = [row for row in packets.values() if row["station"] == station_id]
        quantity = sum(row["quantity"] for row in owned)
        if (station["packets"] != len(owned) or station["packet_quantity"] != quantity
                or not 0 <= station["available"] <= quantity or station["platform"] <= 0):
            raise ValueError("Goods proposal packet/platform observation disagrees")
    if any(row["station"] not in stations or row["quantity"] <= 0 for row in packets.values()):
        raise ValueError("Goods packet has no observed owner/quantity")
    complete = budget["head"] in heads and budget["wagon"] in wagons
    if budget["quote_complete"] != complete:
        raise ValueError("Goods proposal invented a missing model quote")
    if budget["platform_length"] != min(station["platform"] for station in stations.values()) * 16:
        raise ValueError("Goods proposal replaced actual platform fit")
    if complete:
        head, wagon = heads[budget["head"]], wagons[budget["wagon"]]
        # The powered head must remain useful throughout the first interval.
        # Native wagon age has no independent lifetime/reliability limit;
        # disconnected cheap heads or invented wagon limits provide no service.
        if (not head["quoted"] or not wagon["quoted"] or not head["source_connected"] or not head["receiver_connected"]
                or head["life_days"] <= budget["days"] or wagon["age_limited"] is not False
                or head["invoice"] != budget["head_invoice"] or wagon["invoice"] != budget["wagon_invoice"]
                or head["length"] != budget["head_length"] or wagon["length"] != budget["wagon_length"]
                or wagon["capacity"] != budget["wagon_capacity"] or budget["head_upkeep"] < 2 or budget["wagon_upkeep"] < 2):
            raise ValueError("Goods proposal selected an unavailable/disconnected/unpriced model")
    operating = budget["upkeep"] + budget["interest"] + budget["station"]
    # Reconcile each requested lot against native body quotes and the complete
    # existing/new operating bills. Future income and capital floors stay fixed.
    for count, lot in lots.items():
        invoice = budget["head_invoice"] + count * budget["wagon_invoice"]
        added = budget["head_upkeep"] + count * budget["wagon_upkeep"]
        target = 100000 + operating + invoice + added
        length = budget["head_length"] + count * budget["wagon_length"]
        fits = length <= budget["platform_length"]
        funded = complete and fits and stations[13]["available"] > 0 and stations[15]["accepts"] and budget["cash"] >= target
        actual = dict(capacity=count * budget["wagon_capacity"], invoice=invoice, added_upkeep=added,
                      existing_operating=operating, target=target, cash=budget["cash"], length=length,
                      platform_length=budget["platform_length"], fits=fits, quote_complete=complete,
                      funded=funded, spending_permission=False)
        if any(lot[name] != value for name, value in actual.items()):
            raise ValueError("Goods lot omitted whole cost, reserve, demand or physical fit")
    funded_lots = [count for count, row in lots.items() if row["funded"]]
    if funded_lots:
        count = min(funded_lots)
        next_proposal = dict(kind="fully_funded_capacity_proposal", head=budget["head"], wagon=budget["wagon"],
                             wagons=count, target=lots[count]["target"], cash=budget["cash"], spending_permission=False)
    else:
        next_proposal = dict(kind="missing_capacity_prerequisite", models_quoted=complete,
                             demand=stations[13]["available"], accepts=stations[15]["accepts"],
                             fitting_lots=[count for count, row in lots.items() if row["fits"]],
                             cash=budget["cash"], targets={str(count): row["target"] for count, row in lots.items()},
                             spending_permission=False)
    return dict(stations=stations, packets=list(packets.values()), existing=existing, expected_models=expected,
                head_models=heads, wagon_models=wagons, existing_services=services, budget=budget,
                lots=lots, next_proposal=next_proposal)


def validate_operation(source, before, output, operation, root):
    """Validate the actual pure Goods proposal without inherited paid authority.

    @param source Explicit selected predecessor save.
    @param before Actual paused native before-query snapshot.
    @param output Actual native after-query snapshot, preserving every family.
    @param operation Typed current operation with its actual ordinary reload identity.
    @param root Checkout containing explicit native evidence and current sources.
    @return Measured Plan11 business facts accepted by the existing successor owner.
    """
    # This new business producer requires version-two ordinary reload binding;
    # a legacy transport fixture or another native case cannot stand in for it.
    if (operation["schema_version"] != 2 or operation["task"] != TASK or operation["phase"] != PHASE
            or operation["tag"] != TAG or "ordinary_result" not in operation):
        raise ValueError("select the actual Goods proposal and ordinary reload")
    text = checked(operation["terminal"], root).read_text(encoding="utf-8", errors="replace")
    native = _scope(text, operation)
    compare_clocks(source, before, output)
    compare_revision(source, before, native)
    versions, families = zip(*(chunks(path) for path in (source, before, output)))
    original, first, last = families
    # All121families survive loading and every before/after byte stays exact.
    # Only the established loader revision/pause normalization may precede before.
    if (versions != (440, 440, 440) or len(original) != 121 or original.keys() != first.keys()
            or first != last or any(first[name] != raw for name, raw in original.items() if name not in (b"DATE", b"GLOG"))
            or native["ticks_before"] != native["ticks_after"]):
        raise ValueError("Goods query changed an owner, clock or physical family")
    proposal = parse_proposal(text)
    return dict(kind="plan11_operation_validation", task=TASK, phase=PHASE, elapsed_ticks=0,
                authorizes_future_operations=False, facts=dict(unchanged_families=121, proposal=proposal))


def current_build(reference, root, configuration):
    """Require actual current selected images and the complete live source inventory.

    @param reference Explicit completed native-build report identity.
    @param root Selected development checkout.
    @param configuration Actual independent native configuration.
    @return Verified current build document; no historical image fallback.
    """
    from native_build import source_epoch
    build = read(reference, root)
    epoch = read(build["source_epoch"], root)
    validate_build_document(build, epoch, read(build["tool_profile"], root), configuration, root, True)
    if epoch != source_epoch(root):
        raise ValueError("Goods build source inventory changed, including added/removed paths")
    return build


def execute_query(foundation, configuration, build_reference, run_id):
    """Run one separately selected Goods query and its ordinary reload.

    @param foundation Explicit typed selected checkpoint foundation from Group35.
    @param configuration Independent Debug/RelWithDebInfo original.
    @param build_reference Exact current two-image build report.
    @param run_id Fresh explicit query identifier; existing evidence is never overwritten.
    @return Completed version-two operation identity, with no paid permission.
    """
    root = foundation.root
    # Native proof is configuration-specific. A different selected foundation,
    # closed task or stale path must refuse before reserving a mutable runtime.
    if configuration not in CONFIGURATIONS or foundation.configuration not in (None, configuration):
        raise ValueError("Goods query disagrees with its typed configuration")
    parent = verify_foundation(foundation)
    if parent["next_task"] != TASK:
        raise ValueError("selected foundation does not dispatch the Goods query")
    build = current_build(build_reference, root, configuration)
    source_reference = parent["configurations"][configuration]["source"]
    source = checked(source_reference, root)
    application = checked(build["targets"]["openttd_surface_capital_test"], root)
    context = create_run_context("space", SCENARIO, root=root, run_id=run_id)
    # A pure query still owns its before/output/terminal once. Keeping failed
    # attempts intact prevents mixed proof from a repeated invocation identifier.
    if any(path.is_file() for role in ("images", "saves", "data", "reports", "evidence", "scratch") for path in (context.directory / role).rglob("*")):
        raise ValueError("Goods query invocation already contains output")
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("space_goods_capacity\n", encoding="utf-8")
    environment = {key: value for key, value in os.environ.items() if not key.startswith("OTTD_")}
    environment.update(OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source))
    log = context.output("logs", "native.log")
    with log.open("wb") as sink:
        result = run_native(context, [str(application), TAG, "--durations", "yes"], cwd=context.runtime_dir,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=60, env=environment, native_producer=SELF)
    if result.returncode:
        raise ValueError(f"native Goods query failed: {log}")
    reports = list(context.reports.glob("native-*.json"))
    if len(reports) != 1:
        raise ValueError("Goods query needs its sole actual native process")
    process = json.loads(reports[0].read_text(encoding="utf-8"))
    if len(process["transcripts"]) != 1:
        raise ValueError("Goods query needs its actual native terminal")
    before, output = context.saves / "before.sav", context.saves / "after.sav"
    initial, final = [json.loads(json.dumps(ORIGINAL_OBSERVER.read(path))) for path in (before, output)]
    operation = dict(_documentation=__doc__, schema_version=2, kind="space_operation_result", complete=True,
        configuration=configuration, task=TASK, phase=PHASE, tag=TAG, source=source_reference,
        before=identity(before, root), output=identity(output, root), application=identity(application, root),
        terminal=process["transcripts"][0], native_process=identity(reports[0], root), build_result=build_reference,
        source_files=[identity(root / name, root) for name in SOURCE_FILES], validator_source=identity(root / SELF, root),
        observer_contract=ORIGINAL_OBSERVER.contract_id, initial=initial, final=final, financial_policy=POLICY,
        scenario=SCENARIO, run_id=context.run_id)
    operation["ordinary_result"] = execute_restore(operation, root, build)
    validation = validate_operation(source, before, output, operation, root)
    current_build(build_reference, root, configuration)
    target = context.output("reports", "operation.json")
    target.write_text(json.dumps(operation, indent=2) + "\n", encoding="utf-8")
    proposal = context.output("reports", "proposal.json")
    proposal.write_text(json.dumps(dict(_documentation=__doc__, operation=identity(target, root),
                                       facts=validation["facts"]), indent=2) + "\n", encoding="utf-8")
    return identity(target, root)


def publish_query(foundation, operations, next_task, run_id):
    """Bind both real query/reload operations through the existing successor owner.

    @param foundation Explicit selected qualified predecessor.
    @param operations Exact configuration-to-current-operation identities.
    @param next_task Explicit next Plan11producer or missing-prerequisite task.
    @param run_id Fresh paired publication identifier.
    @return Typed generated successor; Group37exports its compact durable closure.
    """
    # Both exact builds must still describe this checkout. Agreement between
    # worlds cannot certify a subsequently edited source or replace an image.
    for configuration, reference in operations.items():
        operation = read(reference, foundation.root)
        current_build(operation["build_result"], foundation.root, configuration)
    context = create_run_context("space", "goods-capacity-publication", root=foundation.root, run_id=run_id)
    return publish_successor(foundation, operations, next_task, context.reports / "foundation.json", validate_operation)


def main():
    """Expose explicit pure run and paired publication commands, without a work mode.

    @return None; failures retain their actual review artifacts and child status.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    run = commands.add_parser("run")
    run.add_argument("--foundation", type=Path, required=True)
    run.add_argument("--configuration", choices=CONFIGURATIONS, required=True)
    run.add_argument("--build-result", type=Path, required=True)
    run.add_argument("--run-id", required=True)
    publish = commands.add_parser("publish")
    publish.add_argument("--foundation", type=Path, required=True)
    publish.add_argument("--operation", nargs=2, action="append", required=True, metavar=("CONFIGURATION", "REPORT"))
    publish.add_argument("--next-task", required=True)
    publish.add_argument("--run-id", required=True)
    arguments = parser.parse_args()
    foundation = load_foundation(arguments.foundation)
    if arguments.command == "run":
        result = execute_query(foundation, arguments.configuration, identity(arguments.build_result, ROOT), arguments.run_id)
    else:
        selections = dict(arguments.operation)
        # Repeated configuration flags must not silently replace an earlier
        # selected operation and leave the other world unverified.
        if len(selections) != len(arguments.operation) or set(selections) != set(CONFIGURATIONS):
            parser.error("publish requires exactly one operation for each configuration")
        selected = {name: identity(Path(path), ROOT) for name, path in selections.items()}
        successor = publish_query(foundation, selected, arguments.next_task, arguments.run_id)
        result = identity(successor.manifest_path, ROOT)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
