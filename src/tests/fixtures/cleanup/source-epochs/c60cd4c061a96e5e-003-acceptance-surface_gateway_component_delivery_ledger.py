"""Conserve the actual additional factory batch through native launch handling.

@file surface_gateway_component_delivery_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CG/15CIC/15CIE/15CK/15CL/15CM/15CN/15CP/15CSGB/15CSGF.
Completed15CFowns fixed acquired36Steel/50Goods. Original native containers own
their transport; this observer never treats an emptied carrier as lost supply.
"""
from surface_gateway_construction_ledger import observe as construction
from surface_gateway_components_ledger import CAPACITIES
from surface_regional_ledger import TABLE
from surface_propellant_ledger import FACTORY


def observe(path, allow_abandonment=False, allow_salt_window=False, allow_salt_delivery=False, allow_chemical_pickup=False, allow_chemical_delivery=False, allow_propellant_pickup=False, allow_propellant_delivery=False, allow_capital_service=False, allow_goods_income=False, allow_farm_capacity=False):
    """@param path Actual original additional-material delivery checkpoint.
    @param allow_abandonment Explicit known after-boundary eightSteel/twoGoods ordinary-offer attrition; earlier defaults remain lossless.
    @param allow_salt_window Explicit later paid electric two-part Salt consist only; original delivery defaults still require exactly35rail bodies.
    @param allow_salt_delivery Explicit original60Salt unloading/production; defaults still prohibit new chemical input or production.
    @param allow_chemical_pickup Explicit renewed27capacity chemical pickup only; default chemical carrier remains empty.
    @param allow_chemical_delivery Explicit subsequent PCHMdelivery/production, with original launch12PROPuntouched.
    @param allow_propellant_pickup Explicit actual Source6custody on original reserve2only; original twelve launch units stay at center21.
    @param allow_propellant_delivery Explicit completed fixed26fuel receipt, preserving quantity through member0/native reverse jobs/center21.
    @return Fixed acquisition and actual retained/abandoned quantities, native custody and original paid Space owners.
    @param allow_capital_service Explicit later single paid Town0PASSbus; every historical default keeps its exact original roles.
    @param allow_goods_income Explicit later reuse of only real empty140/cap18reserves3/25; new Source11trade stays outside original fiftyGoods and old defaults remain strict.
    @param allow_farm_capacity Explicit later one31LVST25tail after exact original Farm13bodies; defaults reject any extra body.
    """
    result = construction(path, allow_component_delivery=True, allow_component_abandonment=allow_abandonment, allow_salt_delivery=allow_salt_delivery, allow_chemical_pickup=allow_chemical_pickup, allow_chemical_delivery=allow_chemical_delivery, allow_propellant_pickup=allow_propellant_pickup, allow_propellant_delivery=allow_propellant_delivery, allow_capital_service=allow_capital_service, allow_goods_income=allow_goods_income)
    assert result["operational"] and result["fuel_burned"] == 30
    assert result["flight"]["started"] == result["flight"]["completed"] == 2
    assert result["orbital_consumed"] == {"STEL": 16, "GOOD": 24, "PROP": 0}
    abandoned = result["cargo_abandoned"]
    acquired_propellant = result["additional_collected"]["PROP"]
    assert result["totals"] == {"STEL": 40 - abandoned["STEL"], "GOOD": 52 - abandoned["GOOD"], "PROP": 12 + acquired_propellant} and result["goods_holds"] == {54: 0, 38: 0}
    stocks = {station: {label: 0 for label in ("STEL", "GOOD", "PROP")} for station in (0, 21)}
    # Read quantities only from actual globally unique packet owners. The
    # original twelve fuel units stay in center storage throughout delivery.
    for packet in result["cohort"]:
        owner = tuple(packet["owner"])
        if owner[0] == "station":
            stocks[owner[1]][packet["label"]] += packet["count"]
        if packet["label"] == "PROP":
            # Only the explicit later pickup may place new Source6fuel aboard
            # original reserve2. Earlier defaults keep all twelve at center21.
            # A completed later acquisition may additionally move only through
            # actual member0and its generation-qualified native reverse job.
            # The unchanged global material observer checks every sole owner.
            delivery_owner = allow_propellant_delivery and (owner == ("station", 0) or owner[0] == "handling")
            assert owner == ("station", 21) or (allow_propellant_pickup and owner == ("vehicle", 2)) or delivery_owner
    shipments = result["shipments"]
    for name, capacity in CAPACITIES.items():
        assert shipments[name]["capacity"] == capacity
    for name in ("Surface Propellant reserve", "Surface Propellant refill"):
        if allow_propellant_pickup and name == "Surface Propellant reserve":
            # This one actual original carrier performs the new pickup; the
            # other reserve remains empty/stopped at its original depot.
            assert shipments[name]["held"] <= 26
            continue
        assert shipments[name]["held"] == 0 and shipments[name]["home"] and shipments[name]["tile"] == 2443
    returned = all(shipments[name]["held"] == 0 and shipments[name]["home"] and shipments[name]["tile"] == 2443 for name in CAPACITIES)
    result["delivery_stocks"] = stocks
    result["component_carriers_returned"] = returned
    result["components_at_member"] = returned and not result["handling"]["jobs"] and stocks[0] == {"STEL": 36 - abandoned["STEL"], "GOOD": 50 - abandoned["GOOD"], "PROP": 0}
    # Original Steel/Goods completion stays separate from the explicit later
    # fuel target. Its new fixed26receipt is independently conserved above and
    # receives its own physical center/member completion flags below.
    original_components = stocks[21]["STEL"] == 40 - abandoned["STEL"] and stocks[21]["GOOD"] == 52 - abandoned["GOOD"]
    # Later reassigned trucks do not become returned carriers. Only their
    # explicit income consumer may rely on the original fixed stock already
    # delivered to this exact center; no new trade packet enters that stock.
    custody_complete = returned or allow_goods_income
    result["components_at_center"] = custody_complete and not result["handling"]["jobs"] and original_components and (allow_propellant_delivery or stocks[21]["PROP"] == 12)
    if allow_goods_income:
        assert original_components and stocks[21] == {"STEL": 32, "GOOD": 50, "PROP": 38}
        assert stocks[0] == {"STEL": 0, "GOOD": 0, "PROP": 0} and not result["handling"]["jobs"]
    vehicles = TABLE(path, b"VEHS")
    extra = 0
    if allow_salt_window:
        # Only this explicit later producer may own a new two-part Salt train;
        # original paid materials, historical factory inputs and all eleven
        # useful commercial services retain their earlier strict contracts.
        prefix = "train[0].common[0]."
        selected = [(key, row) for key, row in vehicles.items() if row["type"] == 0
                    and bytes.fromhex(row[prefix + "name"]).decode("utf-8") == "Surface Salt electric refill"]
        assert len(selected) <= 1
        if selected:
            head, row = selected[0]
            wagon_id = row[prefix + "next"] - 1
            wagon = vehicles[wagon_id]
            assert wagon["type"] == 0 and wagon[prefix + "next"] == 0
            assert row[prefix + "owner"] == wagon[prefix + "owner"] == 0
            assert row[prefix + "engine_type"] == 24 and wagon[prefix + "engine_type"] == 261
            assert row[prefix + "cargo_cap"] == 0 and wagon[prefix + "cargo_cap"] == 60
            assert FACTORY["observe_factory"](path)["cargo_labels"][wagon[prefix + "cargo_type"]] == "SALT"
            packets = TABLE(path, b"CAPA")
            carried = [dict(packet=reference - 1, **packets[reference - 1]) for reference in wagon[prefix + "cargo.packets"]]
            assert all(packet["source_type"] == 0 and packet["source_id"] == 14 for packet in carried)
            held = sum(packet["count"] for packet in carried)
            assert held == sum(wagon[prefix + "cargo.action_counts"]) <= 60
            result["salt_consist"] = dict(head=head, wagon=wagon_id, engine=24, wagon_engine=261,
                                          capacity=60, held=held, packets=carried, tile=row[prefix + "tile"],
                                          home=bool(row[prefix + "vehstatus"] & 2) and row["train[0].track"] == 128 and row[prefix + "tile"] == 1373)
            extra = 2
    # Only the explicitly named actual head and linked sealed hopper may
    # extend the original35bodies. Global packet ownership is already checked
    # by the unchanged material observer; no generic extra-fleet waiver exists.
    added = 0
    if allow_farm_capacity:
        assert allow_goods_income and allow_capital_service and allow_propellant_delivery
        result["farm_capacity"] = farm_capacity(path, vehicles)
        added = result["farm_capacity"]["added"]
    assert sum(row["type"] == 0 for row in vehicles.values()) == 35 + extra + added
    return result


def observe_retained(path):
    """@param path Actual original world after the hash-qualified ordinary-offer abandonment boundary. @return Remaining real stock plus the fixed actual eightSteel/twoGoods lost, never spendable inventory."""
    return observe(path, allow_abandonment=True)


def observe_salt_window(path):
    """@param path Actual later paid electric-Salt world. @return Explicit two-part Salt custody plus unchanged strict original Space/material receipts; no chemical delivery is admitted."""
    return observe(path, allow_abandonment=True, allow_salt_window=True)


def observe_salt_delivery(path):
    """@param path Actual original finite-Salt delivery/empty-sale world. @return Bounded actual Salt/chemical receipt and preserved original owners; earlier observers remain strict."""
    result = observe(path, allow_abandonment=True, allow_salt_window=True, allow_salt_delivery=True)
    # The original newly procured60units remain either in the paid hopper or
    # delivered to Industry15. No empty carrier or production history grants
    # missing finite material, including after a genuine empty-train sale.
    assert result.get("salt_consist", {}).get("held", 0) + result["salt_delivery"]["delivered"] == 60
    return result


def observe_chemical_pickup(path):
    """@param path Actual original renewed chemical pickup world. @return Literal Source15custody and native Salt/chemical production, retaining strict original PCHM/fuel and all useful owners."""
    result = observe(path, allow_abandonment=True, allow_salt_window=True, allow_salt_delivery=True, allow_chemical_pickup=True)
    assert "salt_consist" not in result and result["salt_delivery"]["delivered"] == 60
    return result


def observe_chemical_delivery(path):
    """@param path Actual original chemical-to-PCHMworld. @return Actual27CHEMreceipt/at-most26new PROP production, preserving original launch stock and every useful paid owner."""
    result = observe(path, allow_abandonment=True, allow_salt_window=True, allow_salt_delivery=True, allow_chemical_pickup=True, allow_chemical_delivery=True)
    assert "salt_consist" not in result and result["salt_delivery"]["delivered"] == 60
    return result


def observe_propellant_pickup(path):
    """@param path Actual original Source6fuel-pickup world. @return Only current real reserve2fuel custody added to the unchanged original twelve, with all finite inputs and paid owners conserved."""
    result = observe(path, allow_abandonment=True, allow_salt_window=True, allow_salt_delivery=True, allow_chemical_pickup=True, allow_chemical_delivery=True, allow_propellant_pickup=True)
    assert "salt_consist" not in result and result["salt_delivery"]["delivered"] == 60
    assert result["delivery_stocks"][21]["PROP"] == 12 and result["delivery_stocks"][0]["PROP"] == 0
    assert result["plant"]["additional_produced"] == 26 and result["chemical_carrier"]["held"] == 0
    return result


def observe_propellant_delivery(path, allow_capital_service=False, allow_goods_income=False, allow_farm_capacity=False):
    """@param allow_capital_service Explicit later additional paid bus, with strict defaults retained.
    @param allow_goods_income Explicit later reuse of only real empty140/cap18reserves3/25; new Source11trade stays outside original fiftyGoods and old defaults remain strict.
    @param allow_farm_capacity Explicit later one actual31LVST25tail; all earlier fuel/material defaults retain exact original35rail bodies.
    @param path Actual fixed26Source6launch-supply transport world. @return Sole packet custody through original reserve/member/job/center, strict total38fuel and actual physical storage flags."""
    result = observe(path, allow_abandonment=True, allow_salt_window=True, allow_salt_delivery=True, allow_chemical_pickup=True,
                     allow_chemical_delivery=True, allow_propellant_pickup=True, allow_propellant_delivery=True, allow_capital_service=allow_capital_service, allow_goods_income=allow_goods_income, allow_farm_capacity=allow_farm_capacity)
    assert result["additional_collected"] == {"STEL": 36, "GOOD": 50, "PROP": 26}
    assert result["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    # Historical fuel-delivery consumers retain their completed five-carrier
    # return contract. Only the explicit later income consumer reassigns3/25.
    if not allow_goods_income:
        assert result["component_carriers_returned"]
    assert "salt_consist" not in result and result["chemical_carrier"]["held"] == 0
    member, center = result["delivery_stocks"][0]["PROP"], result["delivery_stocks"][21]["PROP"]
    assert 12 <= center <= 38 and member <= 26
    road = result["propellant_carrier"]
    empty_home = road["home"] and road["tile"] == 2443 and road["held"] == 0
    result["new_fuel_at_member"] = empty_home and member == 26 and center == 12 and not result["handling"]["jobs"]
    result["new_fuel_at_center"] = empty_home and member == 0 and center == 38 and not result["handling"]["jobs"]
    return result


def farm_capacity(path, vehicles):
    """Validate only an actual extra Livestock tail after the immutable original Farm chain.

    @param path Native original carrying its sole physical packet references.
    @param vehicles Same-save decoded native vehicle pool.
    @return Actual added body, full capacities and packet provenance; zero before its native purchase.
    """
    prefix = "train[0].common[0]."
    expected = [52] + list(range(12, 24))
    key, original, seen, livestock, grain = 52, [], set(), 0, 0
    # Saved next references are one-based. Preserve every original body in its
    # exact order before recognizing a new native tail, including valid ID0.
    for original_id in expected:
        assert key == original_id and key not in seen
        seen.add(key)
        row = vehicles[key]
        assert row["type"] == 0 and row[prefix + "owner"] == 0
        position = len(original)
        if position == 0:
            assert row[prefix + "engine_type"] == 24 and row[prefix + "cargo_cap"] == 0
        elif position <= 6:
            assert row[prefix + "engine_type"] == 31 and row[prefix + "cargo_type"] == 4 and row[prefix + "cargo_cap"] == 25
            livestock += 25
        else:
            assert row[prefix + "engine_type"] == 33 and row[prefix + "cargo_type"] == 6 and row[prefix + "cargo_cap"] == 30
            grain += 30
        original.append(key)
        following = row[prefix + "next"]
        key = following - 1 if following else None
    added = None
    if key is not None:
        assert key not in seen
        row = vehicles[key]
        assert row["type"] == 0 and row[prefix + "owner"] == 0 and row[prefix + "next"] == 0
        assert row[prefix + "engine_type"] == 31 and row[prefix + "cargo_type"] == 4 and row[prefix + "cargo_cap"] == 25
        packets = TABLE(path, b"CAPA")
        cargo = [dict(packet=reference - 1, **packets[reference - 1]) for reference in row[prefix + "cargo.packets"]]
        assert all(packet["source_type"] == 0 and packet["source_id"] == 7 for packet in cargo)
        held = sum(packet["count"] for packet in cargo)
        assert held == sum(row[prefix + "cargo.action_counts"]) <= 25
        added = dict(vehicle=key, engine=31, capacity=25, held=held, packets=cargo)
        livestock += 25
    return dict(original=original, added=int(added is not None), body=added, livestock_capacity=livestock, grain_capacity=grain)
