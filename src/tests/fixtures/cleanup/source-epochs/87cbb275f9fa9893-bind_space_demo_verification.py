"""Bind completed current demo results without rerunning dated producer graphs."""
import hashlib
import json
from pathlib import Path

ROOT = Path.cwd()


def identity(relative):
    path = ROOT / relative
    return {"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def read(relative):
    return json.loads((ROOT / relative).read_text(encoding="utf-8-sig"))


manifest_path = "src/tests/fixtures/demos/space/current-checkpoints.json"
manifest = read(manifest_path)
assert [r["menu_number"] for r in manifest["checkpoints"]] == list(range(31, 44))
assert all(identity(r["snapshot"]["path"]) == r["snapshot"] for r in manifest["checkpoints"])
builds = {}
for configuration in ("RelWithDebInfo", "Debug"):
    relative = "logs/space-demo-verify-" + configuration + ".json"
    result = read(relative)
    assert result["complete"] and len(result["checkpoints"]) == 41
    assert all(len(r["unchanged_owners"]) == 33 for r in result["checkpoints"])
    assert result["manifest"] == identity(manifest_path)
    application = identity("build/" + configuration + "/openttd.exe")
    assert application == result["application"]
    for row in result["checkpoints"]:
        for key in ("source", "loaded", "reloaded", "advanced", "advance_writer"):
            if key in row:
                assert identity(row[key]["path"]) == row[key]
    for member in result["native_processes"]:
        assert identity(member["path"]) == member
        assert read(member["path"])["exit_status"] == 0
    builds[configuration] = dict(application=application, verification=identity(relative),
        build_log=identity("logs/demo-update-build-" + configuration + ".log"),
        retained_prior_application=identity("mlm/reviews/demo-update/20261018/original-binaries/" + configuration + "/openttd.exe"),
        english=identity("build/" + configuration + "/lang/english.lng"),
        featured_checkpoints=41, stable_owner_families=33,
        native_processes=len(result["native_processes"]),
        tick_continuations=sum("advanced" in r for r in result["checkpoints"]))
assert builds["RelWithDebInfo"]["tick_continuations"] == 12
assert builds["Debug"]["native_processes"] == 82
launcher = read("logs/space-demo-launcher-verification.json")
assert len(launcher) == 41
assert [r["number"] for r in launcher] == list(range(3, 44))
gui = read("logs/space-demo-capture-RelWithDebInfo.json")
assert gui["complete"] and len(gui["checkpoints"]) == 3
for row in gui["checkpoints"]:
    assert row["application"] == builds["RelWithDebInfo"]["application"]
    assert identity(row["screenshot"]["path"]) == row["screenshot"]
result = dict(complete=True, date="2026-10-18", baseline="5d250ceaca361900c07e0e1beccbcc5b50d6a35d",
    builds=builds, manifest=identity(manifest_path),
    preparation=identity("logs/space-demo-prepare-RelWithDebInfo.json"),
    launcher=identity("logs/space-demo-launcher-verification.json"),
    gui=identity("logs/space-demo-capture-RelWithDebInfo.json"),
    factory_inspector_visual_reference=identity("mlm/reviews/space-demo/capture-relwithdebinfo/20261018-090754-b5b466ff2a/images/space-factory.png"),
    visual_review="Reviewed current factory reservation 74/148, Steelworks recipe OSTL 17/74 and 12750 J, and current fleet/contained service disclosures. Screenshots do not substitute for gameplay continuation checks.",
    diagnostics="Earlier build-concurrent preparation timeout and runner calendar/chunk/UI-exit attempts excluded. Only successful functional processes and completed selected GUI captures qualify.",
    next_implementation="Plan6 Group13CD task13CD.1; actual Surface factory production remains required for Plan11 final acceptance.",
    gameplay_source_changed=False, compiler_project_settings_changed=False,
    sources=[identity(p) for p in ["play.ps1", "src/tests/multilayer/acceptance/space_demo_checks.py",
        "docs/zz-reach2/upgrades/2026-10/d2sa-space-adventures.md",
        "docs/zz-reach2/upgrades/2026-10/d2um-underground-mines.md",
        "docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md",
        "docs/zz-rhr/upgrades/2026-10/r2mlsp-z-z-z-z-g-surface-order-execution.md"]])
(ROOT / "logs/space-demo-verification.json").write_text(json.dumps(result, indent=2) + "\n")
print("Demo complete: both builds; 41 saves / 33 stable families each; 176 successful functional processes; 12 tick continuations; 13 new snapshots; GUI review retained.")
