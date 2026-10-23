"""Prepare a read-only fixture retention review before the master12.6 human demo gate.

@file fixture_retention_inventory.py
@see r2mlvs3mp-master-plan.md12.6 and r2fc-file-cleanup.md.
Hashes/aliases/references and duplicate bytes inform review; none proves redundant coverage.
No file is deleted, moved, rewritten or selected for automatic removal.
"""

import argparse
from collections import defaultdict
import json
import subprocess
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity


def inventory(output):
    """Record current input identities and named occurrences,keeping ambiguous/dynamic consumers explicit."""
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    manifest_path = ROOT / "src/tests/fixtures/multilayer-inputs.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))["inputs"]
    paths = {path.relative_to(ROOT).as_posix() for path in (ROOT / "src/tests/fixtures/haulage").rglob("*") if path.is_file()}
    # Relocated equivalents retain their original stable logical alias. Other
    # fixture saves provide compatibility/demo context and remain out of scope.
    paths.update(path.relative_to(ROOT).as_posix() for path in (ROOT / "src/tests/fixtures").rglob("*.sav"))
    paths.update(value["path"] for key, value in manifest.items() if key.startswith("src/tests/fixtures/haulage/") and (ROOT / value["path"]).is_file())
    aliases, tokens = defaultdict(list), defaultdict(set)
    for key, value in manifest.items():
        if value["path"] in paths:
            aliases[value["path"]].append({"key": key, "recorded_sha256": value["sha256"]})
            tokens[key.rsplit("/", 1)[-1]].add(value["path"])
    for path in paths:
        tokens[path.rsplit("/", 1)[-1]].add(path)
    patterns = target.parent / "fixture-reference-patterns.txt"
    assert not patterns.exists()
    patterns.write_text("\n".join(sorted(tokens)) + "\n", encoding="utf-8")
    command = ["rg", "--json", "--fixed-strings", "--file", str(patterns), "--glob", "*.py", "--glob", "*.cpp",
               "--glob", "*.h", "--glob", "*.json", "--glob", "*.ps1", "--glob", "*.md", "--glob", "CMakeLists.txt",
               "src", "docs", "mlm/tools", "media/multilayer/tools", "play.ps1"]
    search = subprocess.run(command, cwd=ROOT, capture_output=True, encoding="utf-8", check=False)
    assert search.returncode in (0, 1), search.stderr
    raw_search = target.parent / "fixture-reference-matches.jsonl"
    assert not raw_search.exists()
    raw_search.write_text(search.stdout, encoding="utf-8")
    hits, sources = defaultdict(dict), {}
    for line in search.stdout.splitlines():
        row = json.loads(line)
        if row["type"] != "match":
            continue
        data = row["data"]
        source = data["path"]["text"].replace("\\", "/")
        if source == "src/tests/fixtures/multilayer-inputs.json":
            continue # A registry's own listing is not an executable consumer.
        if source not in sources:
            sources[source] = identity(ROOT / source)
        suffix = (ROOT / source).suffix.lower()
        kind = "executable-source-occurrence" if suffix in (".cpp", ".h", ".py", ".ps1") else "documentation-or-data-occurrence"
        for match in data["submatches"]:
            token = match["match"]["text"]
            for path in tokens[token]:
                hits[path][source, data["line_number"], token] = {"source": source, "line": data["line_number"], "token": token,
                                                               "kind": kind, "excerpt": data["lines"]["text"].strip()[:240]}
    records, same_bytes = [], defaultdict(list)
    for path in sorted(paths):
        reference = identity(ROOT / path)
        same_bytes[reference["sha256"]].append(path)
        with (ROOT / path).open("rb") as source:
            header = source.read(8)
        version = int.from_bytes(header[4:6], "big") if len(header) == 8 and header[:4] in (b"OTTX", b"OTTZ", b"OTTD", b"OTTL") else None
        in_scope = path.startswith("src/tests/fixtures/haulage/") or any(row["key"].startswith("src/tests/fixtures/haulage/") for row in aliases[path])
        references = list(hits[path].values())
        executable = sorted({row["source"] for row in references if row["kind"] == "executable-source-occurrence"})
        records.append({"file": reference, "in_haulage_review_scope": in_scope, "save_version": version,
                        "aliases": aliases[path], "alias_hash_mismatches": [row for row in aliases[path] if row["recorded_sha256"] != reference["sha256"]],
                        "named_executable_sources": executable, "references": references,
                        "reference_limit": "Occurrences identify named source/data dependencies,not proof that every branch executes;dynamic prefixes/globs require owning-runner inspection.",
                        "coverage_status": "manual distinct-purpose/compatibility/adverse-state review required before any replacement",
                        "disposition": "retain-until-human-demo-gate-and-consumer-review" if in_scope else "retain-out-of-scope-context"})
    result = {"_documentation": "r2mlvs3mp-master-plan.md12.6;read-only preparation,notcleanupcompletionorhumanacceptance",
              "controller": identity(__file__), "input_manifest": identity(manifest_path), "search_command": command,
              "patterns": identity(patterns), "raw_matches": identity(raw_search), "source_bindings": list(sources.values()),
              "records": records, "fixture_files": len(records), "haulage_scope_files": sum(row["in_haulage_review_scope"] for row in records),
              "scope_files_with_named_executable_occurrences": sum(row["in_haulage_review_scope"] and bool(row["named_executable_sources"]) for row in records),
              "identical_byte_groups": [{"sha256": key, "paths": values, "disposition": "candidate-only;identitydoesnotprovesamecoverage"} for key, values in same_bytes.items() if len(values) > 1],
              "protected": "All supplied/unique compatibility/malformed/demo/native-art inputs and active receipt dependencies remain retained. No missing direct reference authorizes deletion.",
              "deleted_files": [], "human_gate": "master12.6:reviewrefresheddemoandresolveTBMfix11.5feedbackbeforefixturepruning",
              "remaining": "Resolve dynamic consumers and distinct coverage per candidate;propose exact replacement mapping;waitforhumanreview;then affected consumer checks and per-file deletion limits."}
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"inventory": identity(target), "fixture_files": len(records), "haulage_scope_files": result["haulage_scope_files"],
                      "named_executable_scope_files": result["scope_files_with_named_executable_occurrences"], "identical_byte_groups": len(result["identical_byte_groups"]), "deleted_files": 0}), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True)
    inventory(parser.parse_args().output)
