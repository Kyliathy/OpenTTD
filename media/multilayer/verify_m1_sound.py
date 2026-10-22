"""Verify native TBM sound-event reuse against actual controller-selected PCM playback.

@file verify_m1_sound.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group16.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md task83.4
@see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md task16.2
Publication contains mappings and fingerprints only; no selected baseset sample is copied.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re

from activate_m1_art import read_evidence
from evidence import EvidenceSet

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "media/multilayer/m1-sound-events.json"


def validate(manifest, evidence, capture):
    """Validate exact mapping authority, existing sample ownership and actual phase/mixer evidence.

    @param manifest Four-event published candidate; source table remains authoritative.
    @param evidence Real application log containing actual selected/decoded/mixed event records.
    @param capture Corresponding actual loop/source review metadata.
    @return Actual event records tied to their native sample IDs and nonempty PCM.
    @throws AssertionError If a mapping, provider, source or actual playback record is stale/missing.
    """
    header = (ROOT / manifest["source"]).read_text(encoding="utf-8")
    table = re.findall(r"\{MultiLayerTBMSoundEvent::(\w+),\s*(SND_\w+),\s*(\d+)\}", header)
    assert table == [(row["event"], row["symbol"], str(row["interval_ticks"])) for row in manifest["events"]], "sound mapping differs from native authority"
    assert "All tests passed" in evidence and "M1_SOUND_COMPLETE events=4 actual_decoder_and_mixer=true owners_unchanged=true" in evidence
    records = re.findall(r"M1_SOUND_EVENT event=(\d+) sample=(\d+) tick=(\d+) rate=(\d+) bytes=(\d+) nonzero_pcm=(\d+) pcm_hash=(\d+) actual_owner=true", evidence)
    assert len(records) == 4 and {int(row[0]) for row in records} == set(range(4)), "actual sound phase coverage is incomplete"
    # The normal game loop selects all four states. A declared/queued sample
    # without decoded data and actual mixer output cannot establish playback.
    for row in records:
        event, sample, _tick, rate, size, nonzero, _hash = map(int, row)
        assert sample == manifest["events"][event]["sound_id"], "actual sound ID differs from published mapping"
        assert rate > 0 and size > 0 and nonzero > 0, "sound was dispatched without real decoded/mixed data"
    source = ROOT / capture["source"]
    assert hashlib.sha256(source.read_bytes()).hexdigest() == capture["source_sha256"], "actual sound source save changed"
    assert capture["simulation_owners_unchanged_during_render"]
    # This retained original provider is used only by the isolated playback
    # fixture. The distributed mapping works through the selected baseset and
    # includes neither this licensed CAT nor any copied sound waveform.
    cat = ROOT / "build/baseset/sample.cat"
    provider = (ROOT / "media/baseset/orig_dos.obs").read_text(encoding="utf-8")
    required = re.search(r"SAMPLE.CAT\s*=\s*([0-9a-f]{32})", provider).group(1)
    assert hashlib.md5(cat.read_bytes()).hexdigest() == required, "actual playback provider differs from its retained descriptor"
    return [{"event": manifest["events"][int(row[0])]["event"], "sound_id": int(row[1]), "game_tick": int(row[2]),
             "sample_rate": int(row[3]), "decoded_bytes": int(row[4]), "nonzero_pcm": int(row[5]), "pcm_fingerprint": row[6]} for row in records]


def selected_sound_inputs(selection, capture_path, evidence, selection_log, output=None):
    """Resolve sound's direct consumer edges through the graphics selection.

    @param selection Explicit schema2 EvidenceSet for the recipe invocation.
    @param capture_path Exact selected semantic capture path, absolute or relative.
    @param evidence Required native sound transcript's repository-relative path.
    @param selection_log Required owner/refusal transcript's repository-relative path.
    @param output Optional explicit destination, which must equal the selected owner.
    @return Validated capture and generated sound report destination.
    """
    destination = selection.output("sound")
    # Caller overrides can repeat the explicit owner, but cannot redirect
    # required evidence into sources, diagnostics or an unrelated invocation.
    if output is not None and (selection.root / output).resolve() != destination:
        raise ValueError("sound output differs from the explicit recipe selection")
    capture_path = (selection.root / capture_path).resolve()
    matches = [role for role in selection.document["reports"] if selection.selected("reports", role) == capture_path]
    if len(matches) != 1:
        raise ValueError("sound capture is not an unambiguous selected current report")
    capture = selection.report(matches[0])
    assert selection.capture_terminal(capture) == evidence, "sound and actual loop evidence disagree"
    assert selection.terminal("sound-selection") == selection_log, "sound selection transcript differs from required evidence"
    return capture, destination


def main():
    """Validate actual reuse/playback and optionally publish its exact retained evidence.

    @return None; write only mapping/evidence metadata with --publish.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--evidence", required=True)
    parser.add_argument("--capture", required=True)
    parser.add_argument("--selection-log", required=True)
    parser.add_argument("--evidence-manifest", type=Path, required=True,
                        help="exact schema2 graphics selection; diagnostics cannot supply required evidence")
    parser.add_argument("--publish", action="store_true")
    parser.add_argument("--cache-only", action="store_true", help="write generated evidence without changing source status")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    selection = EvidenceSet(args.evidence_manifest)
    capture, output = selected_sound_inputs(selection, args.capture, args.evidence, args.selection_log, args.output)
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    evidence_path = ROOT / args.evidence
    evidence = read_evidence(evidence_path)
    assert "All tests passed" in read_evidence(ROOT / args.selection_log), "sound owner/refusal tests failed"
    records = validate(manifest, evidence, capture)
    # Missing actual output must refuse even when a caller reports a selected
    # sample. This bounded corruption retains every other actual log record.
    broken = re.sub(r"nonzero_pcm=\d+", "nonzero_pcm=0", evidence, count=1)
    try:
        validate(manifest, broken, capture)
    except AssertionError:
        pass
    else:
        raise AssertionError("silent dispatched sound was accepted as playback")
    # Both explicit publication modes write the selected generated proof; only
    # the separately requested source publication may advance authored status.
    if args.publish or args.cache_only:
        report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md83.4; real native phase/selected baseset decode/mixer proof, no sample bytes redistributed",
                  "schema_version": 1, "provider": "original_dos (isolated fixture)", "provider_descriptor": "media/baseset/orig_dos.obs",
                  "runtime_provider": "player-selected sounds baseset", "evidence": args.evidence, "selection_log": args.selection_log,
                  "capture": args.capture, "source": capture["source"], "source_sha256": capture["source_sha256"],
                  "events": records, "simulation_owners_unchanged": True, "user_review": "deferred_until_integration"}
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        manifest.update(runtime_verified=True, evidence=output.relative_to(ROOT).as_posix())
        if not args.cache_only:
            MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("M1 sound handoff: four actual idle/boring/backing/service events select the declared existing slots and produce decoded/mixed PCM; silent-output corruption refused; simulation owners unchanged")


if __name__ == "__main__":
    main()
