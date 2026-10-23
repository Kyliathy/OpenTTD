"""Bind original paid tube queues and operating first-plant thermal/riser worlds.

@file ground_network_delivered_inputs.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group18C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Reads exact accepted originals;no stock,floor,plant,packet or utility account is generated.
"""

import argparse
import json
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT, HANDOFF, HANDOFF_SHA
from construction_provider_return import identity
from save_tables import chunks, read_stream, saved_table

THERMAL = "logs/geothermal/first-plant/20261017-native-01/same-world-power.json"
THERMAL_SHA = "93b7f77cce1680de227c75d3fd3cbd8c0c938264cbe3a664d2a15be05837a491"


def select_inputs(output_path):
    """Verify exact native inputs and retain the original consumed-resource owners.

    @param output_path New retained input index under repository logs.
    @return Paid tube/thermal references and authoritative original meter/clock observations.
    """
    assert identity(ROOT / HANDOFF)["sha256"] == HANDOFF_SHA
    assert identity(ROOT / THERMAL)["sha256"] == THERMAL_SHA
    tube = json.loads((ROOT / HANDOFF).read_text(encoding="utf-8"))
    thermal = json.loads((ROOT / THERMAL).read_text(encoding="utf-8"))
    selected = {"tube-queue": tube["records"]["Debug"]["outputs"]["queued"],
                "thermal-operating": thermal["configurations"]["Debug"]["partial"]}
    records = {}
    # Retained useful paid service must already exist before a network claim.
    # A newly prepared world or free output grant cannot replace this lineage.
    for scenario, reference in selected.items():
        path = ROOT / reference["path"]
        actual = identity(path)
        assert all(actual[key] == reference[key] for key in ("path", "bytes", "sha256"))
        version, stream = read_stream(path)
        count = len(list(chunks(stream)))
        record = {"input": actual, "source_version": version, "source_chunks": count,
                  "clock": saved_table(path, b"DATE")[0]["tick_counter"]}
        power = saved_table(path, b"UPLD")
        if scenario == "tube-queue":
            assert version == 434
            assert count == 126
            participants = saved_table(path, b"TUVD")
            assert {row["vehicle"] for row in participants.values()} == {11, 14}
            assert all(row["active"] == 1 for row in participants.values())
            meter = next(row for row in power.values() if row["type"] == 0 and row["id"] == 0)
            assert meter["active"] == 1
            assert meter["lifetime_consumed"] == 1429299
            connection = next(row for row in saved_table(path, b"TUPD").values() if row["kind"] == 0)
            assert connection["active"] == 1
            assert connection["invoice"] > 0
            record.update(participants=participants, power_meter=meter, connection=connection)
        else:
            assert version == 431
            assert count == 124
            generators = saved_table(path, b"GPGD")
            assert {row["model"] for row in generators.values()} == {0, 1}
            assert all(row["materials_committed"] == 1 for row in generators.values())
            assert all(row["active_service_ticks"] > 0 for row in generators.values())
            risers = saved_table(path, b"GRSD")
            assert {row["industry"] for row in risers.values()} == {4, 5}
            assert all(row["materials_committed"] == 1 for row in risers.values())
            assert all(row["portal_retired"] == 0 for row in risers.values())
            conversion = saved_table(path, b"UCVD")
            assert all(row["heat_j"] > 0 for row in conversion.values() if row["kind"] == 1)
            record.update(generators=generators, risers=risers, conversion=conversion,
                          collector=saved_table(path, b"GCBD"), utility=power)
        records[scenario] = record
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md18C;exact paid native434 tube queue and431 original operating Magma/VentTap history/two-riser world for real current peer joins;not network acceptance",
              "controller": identity(__file__), "tube_handoff": identity(ROOT / HANDOFF),
              "thermal_handoff": identity(ROOT / THERMAL), "records": records}
    target = (ROOT / output_path).resolve()
    # Selection is new diagnostic evidence;originals and earlier receipts stay
    # immutable even when a later native reader introduces current empty chunks.
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(target), "scenarios": list(records)}), flush=True)
    return result


def main():
    """Retain explicit delivered-family input identities without launching a native process.

    @return None;missing paid lineage,active references or consumed history refuses.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    select_inputs(args.output)


if __name__ == "__main__":
    main()
