"""Bind both actual native order-persistence builds and their complete file evidence.

@file surface_supply_checks-7.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCC
Read-only verification binds original products, processes, sources and files.
Declared metadata proves persistence only; paid commands and delivery remain open.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

COMPONENTS = runpy.run_path(str(Path(__file__).with_name("surface_supply_checks-4.py")))
FILES = runpy.run_path(str(Path(__file__).with_name("surface_supply_checks-5.py")))
FAULTS = runpy.run_path(str(Path(__file__).with_name("surface_supply_checks-6.py")))


def read(path):
    """@param path Explicit repository-owned JSON. @return Its original parsed content."""
    return json.loads(path.read_text(encoding="utf-8"))


def original(reference):
    """@param reference Original path/SHA pair. @return Existing exact original file path."""
    path = ROOT / reference["path"]
    assert identity(path) == reference
    return path


def file_report(reference, application, configuration):
    """Admit one real terminal native-file invocation and every original artifact it owns.

    @param reference Explicit original native report identity.
    @param application This configuration's tested current focused executable.
    @param configuration Required independently linked build.
    @return Complete original report; no declared receipt acquires procurement authority.
    """
    proof = read(original(reference))
    assert proof["complete"] and proof["configuration"] == configuration
    assert proof["application"] == application and proof["declared_metadata_fixture"]
    assert not proof["runtime_procurement_acceptance"] and len(proof["native_processes"]) == 1
    references = [proof["application"], proof["source"], proof["terminal"], *proof["native_processes"], *proof["sources"]]
    if proof["malformed"] is not None:
        references.append(proof["malformed"])
    # Every native source, terminal and process must still be the exact tested
    # original. A fresh unrelated product cannot stand in for a dated report.
    for item in references:
        original(item)
    # Independent decoding checks complete receipt preservation in actual native
    # outputs; a scalar success flag cannot replace these original saved bytes.
    for output in proof["outputs"].values():
        assert FILES["orders"](original(output["file"])) == output["orders"]
    return proof


def configuration_proof(configuration, suffix, revision, application):
    """Join one configuration's valid boundaries and all full/copied malformed pairs.

    @param configuration Actual native build.
    @param suffix Explicit release/debug evidence suffix.
    @param revision Final valid-file/refusal member revision.
    @param application Exact focused native application admitted by component proof.
    @return Original member identities and independently counted actual invocations.
    """
    members = []
    valid = {}
    # Each separate process restores from its own explicit immutable input.
    # Empty migration and own resave do not advance paid dispatch or manufacture stock.
    for name, phase in (("prepare", "prepare"), ("empty", "inspect-empty"), ("restore", "inspect"), ("resave", "inspect")):
        member = ROOT / f"logs/surface-supply-order-persistence-{name}-{suffix}-{revision}.json"
        reference = read(member)["report"]
        proof = file_report(reference, application, configuration)
        assert proof["phase"] == phase
        valid[name] = proof
        members.append(dict(member=identity(member), report=reference))
    fixture = valid["prepare"]["outputs"]["surface-supply-order-fixture-440.sav"]
    history_path = ROOT / "logs/orbital-processor-verification.json"
    history, = [row for row in read(history_path)["configurations"] if row["configuration"] == configuration]
    predecessor_ref, = [ref for ref in history["reports"] if ref["path"].endswith("/orbital-processor-inspected.json")]
    predecessor = read(original(predecessor_ref))
    original(predecessor["after"])
    assert valid["prepare"]["source"]["sha256"] == predecessor["after"]["sha256"]
    expected = fixture["orders"]
    assert expected["next_id"] == 5 and [row["state"] for row in expected["rows"]] == [0, 1, 2, 3]
    assert not valid["prepare"]["outputs"]["surface-supply-empty-upgrade-440.sav"]["orders"]["rows"]
    assert FILES["orders"](original(valid["restore"]["source"])) == expected
    assert FILES["orders"](original(valid["resave"]["source"])) == expected
    refusals = []
    # Every malformed fact requires both original read-only refusal and actual
    # full-loader refusal in fresh processes; neither mode may borrow the other.
    for case in FAULTS["CASES"]:
        member = ROOT / f"logs/surface-supply-order-persistence-refusal-{case}-{suffix}-{revision}.json"
        proof = read(member)
        assert proof["complete"] and proof["configuration"] == configuration and proof["case"] == case
        assert not proof["runtime_procurement_acceptance"]
        original(proof["source"])
        original(proof["malformed"])
        # An earlier declared fixture may live at another immutable path while
        # preserving identical original bytes; only exact content equivalence qualifies.
        assert proof["source"]["sha256"] == fixture["file"]["sha256"]
        for source in proof["sources"]:
            original(source)
        phases = []
        for reference in proof["reports"]:
            terminal = file_report(reference, application, configuration)
            assert terminal["source"] == proof["source"] and terminal["malformed"] == proof["malformed"]
            phases.append(terminal["phase"])
        assert phases == ["preview-refusal", "full-refusal"]
        refusals.append(dict(case=case, member=identity(member), reports=proof["reports"]))
    return dict(configuration=configuration, valid=members, refusals=refusals,
                genuine_older_source=dict(index=identity(history_path), report=predecessor_ref, saved=predecessor["after"]),
                native_file_processes=4 + 2 * len(refusals), declared_orders=expected)


def bind():
    """@return Complete both-build persistence proof; this function executes no tests or gameplay."""
    component_members = [ROOT / "logs/surface-supply-order-persistence-components-release-4.json",
                         ROOT / "logs/surface-supply-order-persistence-components-debug-1.json"]
    components = COMPONENTS["bind"](component_members)
    configurations = []
    # Both compiled configurations prove the same native family independently;
    # one successful executable cannot supply the other build's missing boundary.
    for name, suffix, revision in (("RelWithDebInfo", "release", 2), ("Debug", "debug", 1)):
        component, = [row for row in components["configurations"] if row["configuration"] == name]
        configurations.append(configuration_proof(name, suffix, revision, component["executable"]))
    assert configurations[0]["declared_orders"] == configurations[1]["declared_orders"]
    previous = read(ROOT / "logs/surface-supply-order-model-build-verification.json")
    retained = read(ROOT / "logs/surface-supply-order-persistence-retained-tests.json")
    game_applications = []
    # Native persistence tests do not replace playable applications or original
    # completed model products. Their exact retained bytes remain reviewable.
    for row in previous["configurations"]:
        original(row["game_application_unchanged"])
        original(row["test_application"])
        game_applications.append(row["game_application_unchanged"])
    for row in retained["configurations"]:
        assert row["retained"]["sha256"] == row["original"]["sha256"]
        original(row["retained"])
    return dict(_documentation=__doc__, complete=True, components=components, configurations=configurations,
                native_processes=2 + sum(row["native_file_processes"] for row in configurations),
                retained_tests=identity(ROOT / "logs/surface-supply-order-persistence-retained-tests.json"),
                unchanged_game_applications=game_applications,
                sources=[identity(Path(__file__))], native_order_persistence_implemented=True,
                mutable_supply_commands_enabled=False, runtime_procurement_acceptance=False,
                ordinary_game_application_acceptance=False, no_full_regression=True,
                open_consumers=["Plan6 13CCD paid command/dispatch/lifecycle", "Plan6 13CD driven delivery/ordinary application",
                                "Plan11 15B actual Surface entry and master closure"])


def main():
    """Write one fresh final evidence index after all original boundaries qualify."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = bind()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(identity(target), flush=True)


if __name__ == "__main__":
    main()
