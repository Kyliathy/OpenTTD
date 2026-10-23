"""Seal the two actual paid-world peer transport records without claiming full traces.

@file ground_network_report.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group16B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The original paid queue remains immutable; later groups own complete agreed-frame state.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT, HANDOFF, HANDOFF_SHA, CONTENT_SHA, SOURCES
from construction_provider_return import identity
from save_tables import chunks, read_stream


def verify_reference(reference):
    """Verify one exact repository-owned retained input or output identity.

    @param reference Path, bytes and SHA256 record emitted by the native controller.
    @return Existing file after exact content verification.
    """
    path = ROOT / reference["path"]
    assert identity(path) == reference, f"Retained evidence changed: {path}"
    return path


def accept(report_paths, output_path):
    """Validate independent native configurations and publish a transport handoff.

    @param report_paths Exactly one actual transport report for each native configuration.
    @param output_path New immutable JSON receipt beneath repository logs.
    @return Receipt containing exact native/diagnostic/content/input identities.
    """
    assert identity(ROOT / HANDOFF)["sha256"] == HANDOFF_SHA
    accepted = json.loads((ROOT / HANDOFF).read_text(encoding="utf-8"))
    records = {}
    # Both independently launched native applications must transfer the same
    # accepted paid world. Reusing one peer log cannot establish both builds.
    for name in report_paths:
        path = (ROOT / name).resolve()
        report = json.loads(path.read_text(encoding="utf-8"))
        cfg = report["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo")
        assert cfg not in records, "Duplicate native configuration"
        assert report["native_version"] == 434
        assert report["children_reaped"] is True
        assert report["handoff"] == identity(ROOT / HANDOFF)
        assert report["original_input"] == accepted["records"][cfg]["outputs"]["queued"]
        assert report["application"]["path"] == f"build/{cfg}/openttd.exe"
        assert report["content"]["sha256"] == CONTENT_SHA
        assert [row["path"] for row in report["sources"]] == list(SOURCES)
        references = [report["controller"], report["original_input"], report["application"],
                      report["console_projection"], report["content"], report["joined_snapshot"],
                      *report["sources"], *report["diagnostics"].values(), *report["configs"].values()]
        # Every retained diagnostic and code/content identity is checked from
        # bytes; a passing boolean alone cannot seal stale or mixed-run proof.
        for reference in references:
            verify_reference(reference)
        server = verify_reference(report["diagnostics"]["server"]).read_text(encoding="utf-8", errors="replace")
        client = verify_reference(report["diagnostics"]["client"]).read_text(encoding="utf-8", errors="replace")
        assert "status = ACTIVE" in server
        assert "Client::ReceiveServerMapDone()" in client
        assert "Sync error detected" not in client
        assert "sync_err:" not in client
        frames = [int(value) for value in re.findall(r"Sent ACK at (\d+)", client)]
        assert frames[0] == report["first_ack"]
        assert frames[-1] == report["last_ack"]
        assert frames[-1] - frames[0] == report["acknowledged_frames"] >= 444
        assert report["initial_feature_digests"]["server"] == report["initial_feature_digests"]["client"]
        version, stream = read_stream(verify_reference(report["joined_snapshot"]))
        assert version == 434
        assert len(list(chunks(stream))) == 126
        # The active-client hook belongs to the actual host and saves before
        # native unpause; isolated configs must bind locally without advertising.
        scripts = {}
        for role, home in report["runtime_homes"].items():
            assert role in ("server", "client")
            config = verify_reference(report["configs"][role]).read_text(encoding="utf-8")
            assert "server_bind_addresses = 127.0.0.1" in config
            assert "server_game_type = local" in config
            assert "pause_on_join = true" in config
            if role == "server":
                hook = ROOT / home / "scripts/on_server_connect.scr"
                expected = f'save "{(ROOT / report["joined_snapshot"]["path"]).with_suffix("").as_posix()}"\nunpause\n'
                assert hook.read_text(encoding="utf-8") == expected
                scripts["active_client_hook"] = identity(hook)
                scripts["loaded_world_hook"] = identity(ROOT / home / "scripts/game_start.scr")
        records[cfg] = {"report": identity(path), "actual": report, "startup_scripts": scripts}
    assert set(records) == {"Debug", "RelWithDebInfo"}
    assert records["Debug"]["actual"]["original_input"]["sha256"] == records["RelWithDebInfo"]["actual"]["original_input"]["sha256"]
    entry = ROOT / "logs/network/paid-ground/20261019-083348-8777d3b8aa/schema-entry.log"
    assert "All tests passed (17 assertions in 1 test case)" in entry.read_text(encoding="utf-8")
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md16B;real native host/client snapshot transfer and ACK continuation;complete native simulation/commands are subsequent scope",
              "controller": identity(__file__), "schema_entry": identity(entry), "schema_assertions": 17,
              "handoff": identity(ROOT / HANDOFF), "records": records,
              "remaining": "Plan15-16.2/17-19 and selected21:complete native agreed-frame state and commands,different view/render settings,active excavation/wagon-wait joins,time modes and relevant failures;orbital combined matrix remains separate."}
    target = (ROOT / output_path).resolve()
    # Transport acceptance cannot replace a historical receipt or publish into
    # a source/fixture directory merely because its basename looks appropriate.
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(target), "frames": {cfg: row["actual"]["acknowledged_frames"] for cfg, row in records.items()}}), flush=True)
    return result


def main():
    """Seal explicitly named native reports rather than searching historical runs.

    @return None; missing or stale evidence refuses publication.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.report, args.output)


if __name__ == "__main__":
    main()
