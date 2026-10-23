"""Small synthetic Goods diagnostics for adversarial business-contract tests.

@file goods_capacity_fixture.py No executable run or real gameplay qualification.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group36.
Numbers are a declared test invoice, independent of the current world's prices.
"""


def proposal_rows():
    """Construct a small independently declared invoice and real-role-shaped demand.

    @return Synthetic marker/field pairs; none represent an actual native process.
    """
    rows = [
        ("SPACE_GOODS_PACKET", dict(station=13, id=101, next=15, first=13, source_tile=1000, quantity=120)),
        ("SPACE_GOODS_PACKET", dict(station=13, id=102, next=15, first=13, source_tile=1000, quantity=120)),
        ("SPACE_GOODS_STATION", dict(station=13, tile=1000, available=240, packet_quantity=240, packets=2, accepts=False, platform=8, owner=0)),
        ("SPACE_GOODS_STATION", dict(station=15, tile=2000, available=0, packet_quantity=0, packets=0, accepts=True, platform=8, owner=0)),
        ("SPACE_GOODS_EXISTING", dict(head=49, parts=13, capacity=240, held=20, length=104, remaining_days=1000)),
        ("SPACE_GOODS_EXISTING", dict(head=52, parts=14, capacity=60, held=0, length=112, remaining_days=2000)),
        ("SPACE_GOODS_EXPECTED", dict(engine=24, available=True, callback_free=True, quoted=True, error=0, invoice=40000, capacity=0)),
        ("SPACE_GOODS_EXPECTED", dict(engine=32, available=True, callback_free=True, quoted=True, error=0, invoice=5000, capacity=30)),
        ("SPACE_GOODS_HEAD", dict(engine=24, quoted=True, error=0, invoice=40000, annual=2000, source_connected=True, receiver_connected=True, length=8, life_days=3650)),
        ("SPACE_GOODS_WAGON", dict(engine=32, quoted=True, error=0, invoice=5000, capacity=30, annual=0, length=8, life_days=0, age_limited=False)),
        ("SURFACE_RESUPPLY_CAPITAL_SERVICE", dict(vehicle=49, engine=24, annual256=256000, pending=12, remaining=1000, profit_last=50, profit_this=20, orders=2)),
        ("SURFACE_RESUPPLY_INCOME_FIXED_PRICES", dict(ticks=2000, inflation=False, callback_free=True, priced_bodies=1, year_boundary=False, days=28, months=1, upkeep=1000, interest=8334, station_charge=500)),
        ("SPACE_GOODS_BUDGET", dict(cash=182302, debt=5000000, depot=1638, ticks=2000, days=28, months=1, upkeep=1000, interest=8334, station=500, capital_floor=100000, operating_floor=40000, head=24, wagon=32, head_invoice=40000, wagon_invoice=5000, head_upkeep=20, wagon_upkeep=2, wagon_capacity=30, head_length=8, wagon_length=8, platform_length=128, quote_complete=True, year_boundary=False, future_fares=0, refunds=0, credit=0, paid=0)),
    ]
    # Four independently requested lots share this declared synthetic body and
    # company invoice. The first two fit the actual test cash; larger lots do not.
    for count, target in ((2, 159858), (4, 169862), (8, 189870), (12, 209878)):
        rows.append(("SPACE_GOODS_LOT", dict(wagons=count, capacity=30 * count, invoice=40000 + 5000 * count,
            added_upkeep=20 + 2 * count, existing_operating=9834, target=target, cash=182302,
            length=8 + 8 * count, platform_length=128, fits=True, quote_complete=True,
            funded=count in (2, 4), spending_permission=False)))
    return rows


def transcript(rows):
    """Encode synthetic diagnostics using the native integer/Boolean spelling.

    @param rows Explicit synthetic marker/field pairs supplied by the test.
    @return Text for grammar/business checks, explicitly not native execution proof.
    """
    lines = ["Synthetic diagnostic fixture; no native image was executed."]
    # Keep field order stable so tests can alter one declared business claim
    # without accidentally changing another owner's diagnostic fields.
    for marker, fields in rows:
        encoded = [f'{name}={str(value).lower() if isinstance(value, bool) else value}' for name, value in fields.items()]
        lines.append(marker + " " + " ".join(encoded))
    return "\n".join(lines) + "\n"
