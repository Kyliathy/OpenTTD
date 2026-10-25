"""Read-only verification of the original command oracles and complete merged traits.

@file merge_command_identity_checks.py
@see docs/zz-reach2/architecture/commands/archi-commands.md
@see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
The independent revision2 fixture supplies expectations; current code never
generates a baseline. Git is read-only. Reports live in test-cache/merge-supply.
"""
import argparse
from functools import lru_cache
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def clean(text):
    """Remove comments before checking declarations rather than matching prose.

    @param text Original or current C++ source text.
    @return Source tokens with comments removed and whitespace folded.
    """
    return re.sub(r"\s+", " ", re.sub(r"/\*.*?\*/|//[^\n]*", " ", text, flags=re.S)).strip()


def command_values(text):
    """Decode the actual explicit or sequential native command enum.

    @param text Source containing the one Commands declaration.
    @return Name/value map, including the non-executable End sentinel.
    """
    body, = re.findall(r"enum class Commands\s*:\s*uint8_t\s*\{(.*?)\};", clean(text))
    result, value = {}, 0
    # Every original identity participates, including unrelated ordinary commands;
    # a missing prefix entry cannot be hidden by checking only the imported tail.
    for entry in body.split(","):
        if not entry.strip():
            continue
        name, explicit = re.fullmatch(r"\s*(\w+)\s*(?:=\s*(\d+))?\s*", entry).groups()
        if explicit is not None:
            value = int(explicit)
        assert name not in result
        result[name] = value
        value += 1
    return result


def arguments(text, function):
    """Read exactly one declared function tuple, retaining native type and argument names.

    @param text Header from the selected original epoch or current destination.
    @param function Exact oracle-declared function name.
    @return Canonical whitespace only; no type widening or alias normalization.
    """
    args, = re.findall(r"\b" + re.escape(function) + r"\s*\(([^)]*)\)\s*;", clean(text))
    return re.sub(r"\s*,\s*", ", ", args).strip()


@lru_cache(maxsize=None)
def original(repository, head, path):
    """Read an immutable original blob without writing or checking out Git state.

    @param repository Source checkout whose object database retains the pinned epoch.
    @param head Exact original40-character commit recorded by the independent oracle.
    @param path Repository-relative original declaration or implementation.
    @return Original blob text; unavailable evidence fails instead of borrowing live code.
    """
    return subprocess.check_output(["git", "-C", repository, "show", f"{head}:{path}"],
                                   env=dict(os.environ, GIT_OPTIONAL_LOCKS="0"), text=True, encoding="utf-8")


def verify(donor):
    """Verify all original source-labelled facts and the complete merged dispatch prefix.

    @param donor Read-only original B checkout; A is the current repository's object database.
    @return Compact independent qualification receipt, never a runtime registry.
    """
    fixture = ROOT / "src/tests/fixtures/multilayer/merge-command-identity-sequential.json"
    expected = json.loads(fixture.read_text(encoding="utf-8"))
    prior_path = fixture.with_name(expected["prior_allocation"]["file"])
    assert hashlib.sha256(prior_path.read_bytes()).hexdigest() == expected["prior_allocation"]["sha256"]
    prior = {row["name"]: row for row in json.loads(prior_path.read_text(encoding="utf-8"))["commands"]}
    current = command_values((ROOT / "src/command_type.h").read_text(encoding="utf-8"))
    assert expected["allocation_revision"] == 2
    assert current == {**{r["name"]: r["merged_value"] for r in expected["commands"]}, "End": expected["merged_end"]}
    originals = {}
    # Source-labelled tuples stay tied to their actual original declarations and
    # bodies; the superseded unpublished tail may never redefine live expectations.
    for source, repository in (("a", ROOT), ("b", donor)):
        head = expected["source_heads"][source.upper()]
        names = command_values(original(str(repository), head, "src/command_type.h"))
        checked = 0
        for row in expected["commands"]:
            fact = row["original_" + source]
            assert fact == prior[row["name"]]["original_" + source]
            if fact is None:
                continue
            name = row["name"]
            # The two donor supply actions deliberately route to the canonical
            # destination owner; original B numeric names still remain evidence.
            if source == "b":
                name = {"PlaceSurfaceSupplyOrder": "OrderSurfaceSupply", "CancelLegacySurfaceSupplyOrder": "CancelSurfaceSupply"}.get(name, name)
            assert names[name] == fact["value"]
            declaration = original(str(repository), head, fact["declaration"])
            assert arguments(declaration, fact["function"]) == fact["arguments"], (source, name)
            assert re.search(r"\b" + fact["function"] + r"\s*\(", clean(original(str(repository), head, fact["implementation"])))
            checked += 1
        assert checked == names["End"]
        originals[source.upper()] = {"head": head, "commands": checked}
    # Every live trait and its exact source tuple must match the independently
    # allocated map, so native/network template dispatch instantiate one prefix.
    for row in expected["commands"]:
        fact = row["original_a"] or row["original_b"]
        declaration = (ROOT / fact["declaration"]).read_text(encoding="utf-8")
        trait, = re.findall(r"DEF_CMD_TRAIT\(Commands::" + row["name"] + r",\s*(\w+)\s*,", clean(declaration))
        assert arguments(declaration, trait) == row["merged_arguments"], row["name"]
        assert re.search(r"\b" + trait + r"\s*\(", clean((ROOT / row["implementation"]).read_text(encoding="utf-8")))
    return {"_documentation": __doc__, "complete": True, "commands": expected["merged_end"],
            "originals": originals, "prior_sha256": expected["prior_allocation"]["sha256"],
            "expectation_sha256": hashlib.sha256(fixture.read_bytes()).hexdigest()}


def main():
    """Run the read-only source audit and publish its rebuildable cache receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--donor", type=Path, default=ROOT.with_name("OpenTTD3"))
    args = parser.parse_args()
    report = verify(args.donor)
    path = ROOT / "test-cache/merge-supply/group47-command-identity.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Verified {report['commands']} merged command identities and unchanged original A/B facts.")


if __name__ == "__main__":
    main()
