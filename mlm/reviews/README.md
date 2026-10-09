# Multi-layer review artifacts

Owner: [cleanup plan](../../docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md). Shared decision: [Development artifact layout](../../docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout).

## Routing contract

The layout migration and focused native/cold verification are complete. Reusable tests live under `src/tests/`; this directory contains generated evidence. The [producer catalogue](../review-producers.json) records native writers and wrapper entry points. Each caller selects feature/scenario explicitly; mixed tests share an invocation ID while using separate feature directories.

Use `<feature>/<scenario>/<run-id>/images/`, `saves/`, `reports/` and `scratch/`. Reports record producer, native writer, feature, scenario, run ID, build configuration, input identities and artifact paths/hashes. Logs use repository `logs/<feature>/<scenario>/<run-id>/`. Runtime homes use `mlm/runtime/<profile>/<run-id>/<process>/`, separately for host/client.

Inputs are immutable fixtures or explicitly selected prior checkpoints. Do not overwrite them, infer the latest historical run, or create a docs data home. Reproducible proof images/reports/logs belong under ignored `logs/proofs/mlm/native-art/`, with explicit source identities and regeneration commands. A validator dependency alone does not justify committing generated output. Immutable compatibility/demo inputs stay in purpose-named fixture directories. Historical `import-20261009` artifacts preserve original bytes/provenance and do not count as new passes. Fixture pruning remains gated by master task 12.6.

## Producer catalogue

The machine-readable [producer catalogue](../review-producers.json) lists current entry points, explicit feature/scenario assignments and actual native test tags. Mixed control capture spans construction, geology, portals and vehicles; terrain capture spans geology and portals. Native spoil-loop capture uses separate Deep/Underground scenarios. Manual art capture explicitly routes portal/traffic, terrain and construction UI frames to their respective features.

Use the owning Python capture runner or its documented native test tag. Each command prints its run-local log/evidence path. PPM-to-PNG conversion records exact container and native-process report hashes in `capture-conversions.jsonl`, and verifies unchanged pixels before disposing of a temporary PPM. Accepted feedback continuation, motion replay/pacing continuation and partial geology/profile verification require `--prior-run` with an exact earlier invocation; no latest-run discovery is permitted. An imported helper receives the caller's context when it generates output for that caller.
