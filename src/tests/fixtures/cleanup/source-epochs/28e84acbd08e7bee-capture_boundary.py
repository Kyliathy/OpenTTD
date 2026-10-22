"""Capture read-only pre-migration facts for r2fc-file-cleanup3.md Group1.

@file capture_boundary.py Disposable observation aid; regenerate into test-cache/cleanup.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group1.
"""

import hashlib
import json
import os
from pathlib import Path
import stat
import subprocess
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "test-cache/cleanup/boundary.json"


def git(*arguments):
    """Read Git metadata without optional index refresh locks.

    @param arguments Read-only Git arguments passed separately to the process.
    @return Successful UTF-8 command output.
    """
    return subprocess.check_output(["git", "-C", str(ROOT), *arguments],
        env={**os.environ, "GIT_OPTIONAL_LOCKS": "0"}, text=True, encoding="utf-8")


def identity(relative):
    """Hash an existing protected input before any consumed executable is relinked.

    @param relative Exact repository-relative input spelling.
    @return Path, full SHA256 and file size, or an explicit missing status.
    """
    path = ROOT / relative
    if not path.is_file():
        return dict(path=relative, status="missing")
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        # Stream large application images so observation does not duplicate their memory.
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return dict(path=relative, sha256=digest.hexdigest(), bytes=path.stat().st_size)


def main():
    """Observe storage and explicit protected epochs without exporting or deleting inputs.

    @return None; publish one reproducible observation report under test-cache.
    """
    result = dict(_documentation=__doc__, captured_utc=datetime.now(timezone.utc).isoformat(),
        root=str(ROOT), head=git("rev-parse", "HEAD").strip(),
        worktrees=git("worktree", "list", "--porcelain"),
        status=git("status", "--porcelain=v1", "-uall"),
        untracked=git("ls-files", "--others", "--exclude-standard", "-z").split("\0")[:-1],
        ignored=git("ls-files", "--others", "--ignored", "--exclude-standard", "-z").split("\0")[:-1],
        sizes={}, nested_repositories=[], reparse_points=[], build_roots=[], configurations=[])
    pending = [ROOT]
    # Refuse to traverse redirected trees. Nested repositories retain independent
    # ownership; list them while still measuring ordinary files without Git mutations.
    while pending:
        directory = pending.pop()
        for entry in os.scandir(directory):
            path = Path(entry.path)
            relative = path.relative_to(ROOT).as_posix()
            metadata = entry.stat(follow_symlinks=False)
            if metadata.st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT:
                result["reparse_points"].append(relative)
                continue
            if entry.is_dir(follow_symlinks=False):
                pending.append(path)
                continue
            if entry.name == "CMakeCache.txt":
                result["build_roots"].append(path.parent.relative_to(ROOT).as_posix())
            if entry.name == ".git":
                result["nested_repositories"].append(relative)
            owner = relative.split("/")[0]
            bucket = result["sizes"].setdefault(owner, dict(bytes=0, files=0))
            bucket["bytes"] += metadata.st_size
            bucket["files"] += 1
        # Git directories are included in sizes but flagged as independent when nested.
        if directory.name == ".git" and directory.parent != ROOT:
            result["nested_repositories"].append(directory.relative_to(ROOT).as_posix())
    bound_path = "logs/surface-entry-construction-cash-verification.json"
    bound = json.loads((ROOT / bound_path).read_text(encoding="utf-8"))
    protected = {bound_path, bound["products"]["path"], bound["source"]["path"]}
    protected.update(row["path"] for row in bound["historical_versions"])
    # Capture exact current saves, applications and source readers separately from
    # historical retained versions; historical mismatches are not silently rebased.
    for row in bound["configurations"]:
        receipt = json.loads((ROOT / row["receipt"]["path"]).read_text(encoding="utf-8"))
        operation = receipt["operation"]
        protected.update((row["receipt"]["path"], row["output"]["path"], operation["application"]["path"]))
        protected.update(item["path"] for item in operation["sources"])
        result["configurations"].append(dict(configuration=row["configuration"],
            receipt=identity(row["receipt"]["path"]), save=identity(row["output"]["path"]),
            application=identity(operation["application"]["path"]), observation=row["observation"]))
    products = json.loads((ROOT / bound["products"]["path"]).read_text(encoding="utf-8"))
    protected.update(row["source"]["path"] for row in products["products"])
    result["closed_policy"] = {key: bound[key] for key in (
        "actual_cash", "startup_reserve", "working_floor_policy", "capital_scope", "measured_cash_flow")}
    protected.update(("src/transport_tube_fleet.h", "src/transport_tube_fleet.cpp", "src/transport_tube_engine.cpp",
        "src/newgrf.cpp", "src/transport_tube.h", "src/CMakeLists.txt", "CMakeLists.txt", "src/tests/CMakeLists.txt"))
    protected.update("logs/underground-tubes/20261021-models-01/" + name
        for name in ("before.json", "before-definitions.json"))
    result["protected_identities"] = [identity(path) for path in sorted(protected)]
    result["active_writers"] = json.loads(subprocess.check_output([
        "powershell", "-NoProfile", "-Command",
        "@(Get-CimInstance Win32_Process | Where-Object { $_.Name -match '^(openttd.*|MSBuild|cl|link|cmake|ninja|python.*|powershell|pwsh|7z.*)\\.exe$' } | Select-Object ProcessId,ParentProcessId,Name,CommandLine) | ConvertTo-Json -Depth 2"
    ], text=True, encoding="utf-8"))
    result["status_at_close"] = git("status", "--porcelain=v1", "-uall")
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: result[key] for key in ("head", "sizes", "nested_repositories", "reparse_points", "build_roots")}, indent=2))
    print("protected inputs", len(result["protected_identities"]), "ignored files", len(result["ignored"]))


if __name__ == "__main__":
    main()
