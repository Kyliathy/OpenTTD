"""Check ordinary commissioning reconstruction and genuine older pending producer migration.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9B
Only explicitly versioned SDEF and absent429 intent framing are normalized for old input comparison.
Paid receipt fields, physical packets, original work and pending source identities stay exact.
"""
import argparse
import json
import os
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream
from runner import current_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_commissioning_checks-1.py")))
AUDIT = runpy.run_path(str(Path(__file__).with_name("probe_role_checks-5.py")))
LAYOUT = runpy.run_path(str(Path(__file__).with_name("manufacturing_replacement_checks-3.py")))["layout"]
EXACT = AUDIT["EXACT"]
NEXT = dict(zip(SHARED["PHASES"][:6], SHARED["PHASES"][1:7]))


def read(path):
    """Read an explicitly retained JSON proof.

    @param path Exact repository proof path.
    @return Original parsed object.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def compare(left, right, label):
    """Compare all thirty unchanged physical owner families at a current commissioning boundary.

    @param left Exact uninterrupted native predecessor or next-action checkpoint.
    @param right Its independently reconstructed/resumed native twin.
    @param label Boundary identification for diagnostics.
    @return Number of exact physical owner comparisons.
    """
    before, after = chunks(left)[1], chunks(right)[1]
    # Every physical owner must agree, including packets, finances, engine/unit/group state and the complete once-only producer ledger.
    for family in EXACT:
        assert before[family] == after[family], (label, family, left, right)
    return len(EXACT)


def accepted_engine_upgrade(source, upgraded, configuration):
    """Match the exact pre429 unstarted-Engine repair already accepted by the original429 producer.

    @param source Genuine428 input whose actual Engine bytes equal the accepted original repair input.
    @param upgraded Actual current ordinary reconstruction; all repaired bytes must equal the retained429 output.
    @param configuration Matching dated producer configuration.
    @return Exact immutable producer/index/old/new references proving the identical bounded migration.
    """
    index = ROOT / "logs/manufacturing-replacement-verification.json"
    assert identity(index)["sha256"] == "6a6e1bc848c551c9523b2970043af302c8ff5759f1963d6bc347302eaf648878"
    accepted = next(row for row in read(index)["configurations"] if row["configuration"] == configuration); candidates = []
    # The retained original429 upgrade is the exact producer boundary; another newer engine history cannot authorize changes to an initialized or user-denied catalogue.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member; proof = read(path)
        if proof.get("variant") == "main" and proof.get("phase") == "upgrade-clone" and proof.get("model") == 16:
            candidates.append((member, proof))
    assert len(candidates) == 1; report, proof = candidates[0]
    original, repaired = ROOT / proof["source"]["path"], ROOT / proof["after"]["path"]
    assert identity(original) == proof["source"] and identity(repaired) == proof["after"]
    old, new = chunks(source)[1], chunks(upgraded)[1]; before, after = chunks(original)[1], chunks(repaired)[1]
    assert old[b"ENGN"] == before[b"ENGN"] and new[b"ENGN"] == after[b"ENGN"]
    assert old[b"EIDS"] == before[b"EIDS"] == after[b"EIDS"] == new[b"EIDS"]
    return dict(index=identity(index), proof=report, original=proof["source"], repaired=proof["after"])


def migrate_equal(source, upgraded, label, configuration):
    """Compare genuine older owners while admitting only explicit transport-version differences.

    @param source Actual immutable428/429/430 input.
    @param upgraded Its ordinary current431 reconstruction.
    @param label Explicit original producer scope.
    @param configuration Matching current and retained migration-producer configuration.
    @return Exact comparison count and optional identical already accepted pre429 Engine repair references.
    """
    old_version, before = chunks(source); new_version, after = chunks(upgraded); assert old_version in (428, 429, 430) and new_version == 431
    assert before[b"SDEF"][3:5] == bytes.fromhex("0002") and after[b"SDEF"][3:5] == bytes.fromhex("0003")
    assert before[b"SDEF"][5:] == bytes.fromhex("a1608eff08bf795b") and len(after[b"SDEF"]) == 13
    normalized = {}; engine_upgrade = None
    if old_version == 428:
        _, raw = read_stream(upgraded); owners, jobs, bodies = LAYOUT(raw)
        # Version429 appended one explicit absence byte to each body and present manufacture job. Remove only those known absent fields in memory; actual original67-byte receipts and job/clone inputs remain intact.
        for family, rows in ((b"SPST", jobs), (b"SVST", bodies)):
            owner = owners[family]; payload = bytearray(raw[owner["start"]:owner["end"]]); assert all(not row["present"] for row in rows)
            # Remove later original offsets first so normalizing one absent intent cannot shift or alter an earlier paid owner's bytes.
            for position in sorted((row["presence"] - owner["start"] for row in rows), reverse=True):
                assert payload[position] == 0; del payload[position]
            normalized[family] = bytes(payload)
    # Only the original pre429 never-started Engine migration may alter its catalogue history, and only with byte-identical original/repaired evidence from the accepted429 producer. Paid ownership and every initialized history remain unchanged.
    if old_version == 428 and before[b"ENGN"] != after[b"ENGN"]:
        engine_upgrade = accepted_engine_upgrade(source, upgraded, configuration)
    # SDEF deliberately identifies current definitions after ordinary resave. All original physical owners retain exact old fields; no receipt2 is rewritten to3.
    for family in EXACT:
        if family == b"SDEF" or (family == b"ENGN" and engine_upgrade is not None):
            continue
        actual = normalized[family] if family in normalized else after[family][3:] if family in (b"SPST", b"SVST") else after[family]
        expected = before[family][3:] if family in (b"SPST", b"SVST") else before[family]
        assert actual == expected, (label, family, source, upgraded)
    return dict(exact_owner_families=len(EXACT), engine_upgrade=engine_upgrade)


def ordinary_run(owner, configuration, source, label):
    """Run the real ordinary application while preserving the caller's configuration environment.

    @param owner Current invocation's physical ordinary-process owner.
    @param configuration Matching completed native game build.
    @param source Immutable native checkpoint to reconstruct.
    @param label Exact boundary label.
    @return Actual ordinary resave path.
    """
    previous = os.environ.get("OTTD_TEST_CONFIGURATION"); os.environ["OTTD_TEST_CONFIGURATION"] = configuration
    try:
        ordinary(owner, dict(name=label, snapshot=identity(source)), configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    result = owner.saves / "ordinary-resave.sav"; assert chunks(result)[0] == 431; return result


def reload(configuration, proof_path, completion=None):
    """Reconstruct an actual current milestone and resume its next action from its own ordinary resave.

    @param configuration Matching current completed native configuration.
    @param proof_path Explicit accepted current native milestone report.
    @param completion Optional accepted uninterrupted next action.
    @return Current immutable ordinary/resumed proof.
    """
    proof = read(proof_path); phase = proof["phase"]; assert proof["configuration"] == configuration
    context = current_run_context("space", "heavy-commissioning-reload-" + phase); owner = context.for_feature("space", "heavy-commissioning-ordinary-" + phase)
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    resaved = ordinary_run(owner, configuration, source, phase); compare(source, resaved, phase); owners = [owner]
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9B; all thirty actual physical owners and own-resave next action.",
                  configuration=configuration, phase=phase, proof=identity(proof_path), source=proof["after"], ordinary=identity(resaved), exact_owner_families=len(EXACT))
    if completion is not None:
        accepted = read(completion); assert accepted["predecessor"] == identity(proof_path)
        resumed_owner = context.for_feature("space", "heavy-commissioning-resumed-" + phase)
        assertions, marker = SHARED["native"](resumed_owner, configuration, accepted["phase"], resaved)
        resumed = resumed_owner.saves / f'heavy-commission-{accepted["phase"]}-431.sav'
        compare(ROOT / accepted["after"]["path"], resumed, ("resumed", phase)); owners.append(resumed_owner)
        result.update(completion=identity(completion), resumed_input=identity(resaved), resumed=identity(resumed), assertions=assertions, marker=marker)
    result["native_reports"] = [identity(path) for item in owners for path in sorted(item.reports.glob("native-*.json"))]
    target = context.reports / "heavy-commissioning-reload.json"; target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); return target


def legacy(configuration, scope):
    """Restore one real older pending clone/replacement or active mission without replaying its paid producer.

    @param configuration Matching completed current native build.
    @param scope Explicit original legacy producer family.
    @return Current immutable ordinary-upgrade report.
    """
    names = {"clone": ("manufacturing-clone", SHARED["ENTRY_SHA"]), "replacement": ("manufacturing-replacement", "6a6e1bc848c551c9523b2970043af302c8ff5759f1963d6bc347302eaf648878"),
             "mission": ("probe-mission", "b5c1086261f032fd93a2e8de658ea7020bb2b8b2f116b5f93a9c8fcae20b4936")}
    name, expected = names[scope]; index = ROOT / f"logs/{name}-verification.json"; assert identity(index)["sha256"] == expected
    accepted = next(row for row in read(index)["configurations"] if row["configuration"] == configuration); candidates = []
    # Only original native reports qualify; ordinary/resumed forks and matching model numbers cannot supply a different pending producer or active mission.
    for member in accepted["reports"]:
        path = ROOT / member["path"]; assert identity(path) == member; proof = read(path)
        match = (proof.get("variant") == "main" and proof.get("phase") == "before" and proof.get("model") == 16) if scope != "mission" else (proof.get("branch") == "local" and proof.get("action") == "scan-partial")
        if match and "after" in proof:
            candidates.append((member, proof["after"]))
    assert len(candidates) == 1; predecessor, selected = candidates[0]; source = ROOT / selected["path"]; assert identity(source) == selected
    context = current_run_context("space", "heavy-commissioning-legacy-" + scope); upgraded = ordinary_run(context, configuration, source, "legacy-" + scope)
    migration = migrate_equal(source, upgraded, scope, configuration); assert identity(source) == selected
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9B; genuine immutable older pending producer/active mission. Only explicit429 absence/SDEF transport is normalized; no physical content or old receipt version is changed.",
                  configuration=configuration, phase="legacy-" + scope, predecessor=predecessor, source=selected, upgraded=identity(upgraded), migration=migration, exact_owner_families=len(EXACT),
                  native_reports=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    target = context.reports / "heavy-commissioning-legacy.json"; target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); return target


def execute(configuration, selector, output, resume=False):
    """Verify all current boundaries plus three independent real old producer epochs in bounded sequential children.

    @param configuration Matching completed current native builds.
    @param selector Exact completed original commissioning membership.
    @param output New invocation-owned progress selector.
    @param resume Revalidate explicitly retained completed membership and run only missing boundaries.
    """
    content = read(selector); assert content["configuration"] == configuration and content["complete"] and len(content["reports"]) == 9
    states = {read(ROOT / member)["phase"]: ROOT / member for member in content["reports"]}; reports = []; existing = {}
    if resume:
        prior = read(output); assert prior["configuration"] == configuration and prior["scope"] == "reloads" and not prior["complete"]
        existing = {read(ROOT / member)["phase"]: ROOT / member for member in prior["reports"]}; assert len(existing) == len(prior["reports"])

    def record(path):
        """Persist exact completed membership before attempting another independent native boundary.

        @param path Actual accepted proof path, never an inferred latest output.
        """
        reports.append(path)
        value = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9B; explicit current reconstruction and real legacy acceptance membership.",
                     configuration=configuration, scope="reloads", complete=False, reports=[item.relative_to(ROOT).as_posix() for item in reports])
        output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    # Every current boundary gets its own ordinary save, and every actual next action loads that new save rather than borrowing uninterrupted state.
    for phase in SHARED["PHASES"]:
        if phase in existing:
            path = existing[phase]; proof = read(path); assert proof["configuration"] == configuration and proof["proof"] == identity(states[phase])
            compare(ROOT / proof["source"]["path"], ROOT / proof["ordinary"]["path"], ("retained ordinary", phase))
            if phase in NEXT:
                completion = read(states[NEXT[phase]]); assert proof["completion"] == identity(states[NEXT[phase]])
                compare(ROOT / completion["after"]["path"], ROOT / proof["resumed"]["path"], ("retained resumed", phase))
        else:
            path = reload(configuration, states[phase], states[NEXT[phase]] if phase in NEXT else None)
        record(path); print(configuration, "reload", phase, flush=True)
    for scope in ("clone", "replacement", "mission"):
        record(existing["legacy-" + scope] if "legacy-" + scope in existing else legacy(configuration, scope)); print(configuration, "legacy", scope, flush=True)
    value = read(output); assert len(reports) == 11; value["complete"] = True; output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def main():
    """Run only the active current/legacy commissioning reconstruction burst."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--selector", type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--resume", action="store_true")
    args = parser.parse_args(); assert args.resume or not args.output.exists(); execute(args.configuration, args.selector.resolve(), args.output.resolve(), args.resume)


if __name__ == "__main__":
    main()
