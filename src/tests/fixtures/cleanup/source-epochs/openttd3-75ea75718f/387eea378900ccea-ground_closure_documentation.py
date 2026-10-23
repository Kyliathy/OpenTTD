"""Verify the selected technical closure's current documents, links, source owners and exact receipts.

@file ground_closure_documentation.py
@see r2mlvs3mp-master-plan.md12.3-12.5.
Read-only Git inspection; never changes checkboxes, evidence, Git history or human acceptance.
"""

import argparse
import ast
import json
import re
import subprocess
import sys
from urllib.parse import unquote

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from ground_final_boundaries import HANDOFFS
from ground_performance_acceptance import strict_reference

PLANS = "docs/zz-reach2/upgrades/2026-10/"
PENDING = {"r2mlvs3mp-master-plan.md": {"11.7", "12.1", "12.2", "12.6"},
           "r2mlvs3-sealed-portals-and-deep-rail.md": {"18.2"}, "r2mlvs4-tunnel-boring-machines.md": {"22.2"}}


def git(*arguments):
    """Read repository state without staging, committing or checking out any object."""
    return subprocess.run(["git", *arguments], cwd=ROOT, check=True, capture_output=True, encoding="utf-8").stdout


def anchors(path):
    """Return ordinary GitHub-style heading anchors, including duplicate suffixes."""
    found, duplicates = set(), {}
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        match = re.match(r"^#{1,6}\s+(.+?)\s*#*\s*$", line)
        if not match:
            continue
        title = re.sub(r"\[([^]]+)\]\([^)]*\)", r"\1", match[1]).lower()
        slug = re.sub(r"[^\w\- ]", "", title).replace(" ", "-")
        suffix = duplicates.get(slug, 0)
        duplicates[slug] = suffix + 1
        found.add(slug + ("-" + str(suffix) if suffix else ""))
    return found


def added_markdown():
    """Inspect changed Markdown links, preserving unrelated historical unavailable references."""
    result, current = {}, None
    for line in git("diff", "--no-ext-diff", "--unified=0", "--", "docs").splitlines():
        if line.startswith("+++ b/"):
            current = line[6:]
        elif current and current.endswith(".md") and line.startswith("+"):
            result.setdefault(current, []).append(line[1:])
    for name in git("ls-files", "--others", "--exclude-standard", "--", "docs").splitlines():
        if name.endswith(".md"):
            result[name] = (ROOT / name).read_text(encoding="utf-8-sig").splitlines()
    return result


def verify_links(changed):
    """Require every added local target and Markdown heading to resolve in the actual workspace."""
    checked, cache = [], {}
    for name, lines in changed.items():
        for target in re.findall(r"!?\[[^\]]*\]\((<[^>]+>|[^)\n]+)\)", "\n".join(lines)):
            target = target.strip().strip("<>")
            if re.match(r"(?:https?|mailto|app|plugin):", target):
                continue
            destination, separator, fragment = unquote(target).partition("#")
            path = ((ROOT / name).parent / destination).resolve() if destination else ROOT / name
            assert path.exists(), (name, target)
            if separator and fragment and path.suffix.lower() == ".md":
                if path not in cache:
                    cache[path] = anchors(path)
                assert fragment in cache[path], (name, target, "missing heading")
            checked.append({"source": name, "target": target})
    return checked


def verify_pending():
    """Keep exact human/final-signoff tasks open while requiring all target implementation rows complete."""
    actual = {}
    for name, expected in PENDING.items():
        text = (ROOT / PLANS / name).read_text(encoding="utf-8-sig")
        remaining = set(re.findall(r"^- \[ \] ([0-9A-Z]+(?:\.[0-9A-Z]+)*)\s", text, re.M))
        assert remaining == expected, (name, remaining, expected)
        actual[name] = sorted(remaining)
    parent = (ROOT / PLANS / "r2mlvs-multilayer-vehicles-and-structures.md").read_text(encoding="utf-8-sig")
    for name in ("r2mlvs3-sealed-portals-and-deep-rail.md", "r2mlvs4-tunnel-boring-machines.md"):
        assert any(line.startswith("| [~] |") and name in line for line in parent.splitlines())
    plan = (ROOT / PLANS / "r2mlvs16-integrated-acceptance-and-balance.md").read_text(encoding="utf-8-sig")
    for task in ("21C.1", "21DB.1", "21DC.1", "21DDB.1", "21DDC.1", "21DDD.1", "21EB.1", "21EC.1", "21F.1", "21F.2",
                 "21.1G", "21.2G", "21.3G", "22.1G", "22.2G", "22.3G", "23.1G", "23.2G"):
        assert re.search(r"^- \[X\] " + re.escape(task) + r"\s", plan, re.M), task
    return actual


def verify_python():
    """Parse changed Python sources without writing bytecode or creating synthetic tests."""
    files = set(git("diff", "--name-only", "--", "src/tests").splitlines())
    files.update(git("ls-files", "--others", "--exclude-standard", "--", "src/tests").splitlines())
    names = sorted(name for name in files if name.endswith(".py") and (ROOT / name).is_file())
    for name in names:
        ast.parse((ROOT / name).read_text(encoding="utf-8-sig"), filename=name)
    return names


def audit(output):
    """Seal only complete actual document/source checks and exact final integration evidence."""
    pending = verify_pending()
    changed = added_markdown()
    links = verify_links(changed)
    syntax = verify_python()
    git("diff", "--check")
    receipts = {}
    for key, (path, expected) in {**HANDOFFS, "final-boundaries": (
            "logs/performance/ground-maps/20261019-native-01/final-boundaries-verification.json",
            "36e2d85892055cef1c4fbd31aea0b619f3c1eda43959acc8166ae714c4ee5fac")}.items():
        reference = identity(ROOT / path)
        assert reference["sha256"] == expected
        strict_reference(reference)
        receipts[key] = reference
    performance = json.loads((ROOT / receipts["performance"]["path"]).read_text(encoding="utf-8"))
    production = [ref for ref in performance["current_production_and_physical_artifacts"] if ref["path"].startswith("src/") and not ref["path"].startswith("src/tests/")]
    assert production
    for reference in production:
        strict_reference(reference)
    boundary = json.loads((ROOT / receipts["final-boundaries"]["path"]).read_text(encoding="utf-8"))
    assert boundary["complete"] is True and boundary["isolated_invocations"] == 24 and boundary["native_cases"] == 76
    for record in boundary["records"]:
        actual = record["actual"]
        assert actual["children_reaped"] is True and actual["failure"] is None
        for reference in (actual["controller"], actual["binary"], actual["native"]["log"], actual["config"], actual["memory_samples"], *actual["sources"]):
            strict_reference(reference)
    result = {"_documentation": "r2mlvs3mp-master-plan.md12.3-12.5;current technical closure with human gates preserved",
              "controller": identity(__file__), "accepted": True, "pending": pending, "receipts": receipts,
              "current_production": production, "documents": [identity(ROOT / name) for name in sorted(changed)],
              "added_local_links": links, "parsed_python_files": syntax, "git_diff_check": "passed",
              "remaining": "human art11.7/full target signoff12.1-12.2;verified demo/feedback12.6;whole unrelated orbital/profile work"}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "documents": len(changed), "links": len(links), "python_sources": len(syntax), "production_owners": len(production)}), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True)
    audit(parser.parse_args().output)
