# Merged command and peer compatibility

Related architecture: [native commands](../../architecture/commands/archi-commands.md),
[native saves](../../architecture/savegame/archi-savegame.md) and
[Plan3 Group47](m2o23-3-come-space.md#group-47---verify-command-abi-and-incompatible-peer-refusal).
Status: all Group47 tasks verified; live two-process joining remains Plan4.

## Independent command identity

`src/tests/merge_command_identity_checks.py` reads both immutable original Git
epochs through `git show`, without checking out or writing Git state. It verifies
the retained Group7 oracle SHA256
`94e17438558c2695ba1a856c0f5de5b96c163031386862b1880bd3e116fc5fcd`,
unchanged original A211/B217 names, values, declaration tuples and owning bodies,
and all223 current traits against the separately frozen revision2 fixture. Only
the old unpublished merged tail is superseded; source-labelled facts remain exact.
Run `python -B src/tests/merge_command_identity_checks.py`; the default read-only
donor is the sibling OpenTTD3 checkout, overridable by `--donor`.
The rebuildable receipt is `test-cache/merge-supply/group47-command-identity.json`.

Native codec checks use independently written byte vectors for StationID U16,
RFMT label semantics, quantity U16, intent U64 and checked legacy cancellation.
The donor's raw four-label-byte adapter is reproduced only in the test: it is
not registered as another executable command. A's semantic big-endian label
integer still uses the ordinary little-endian command writer; a donor U32
purchase packet is not A's U64 intent tuple. Existing bounded-path and typed
external/geothermal provider codecs retain their native owners and identities.

## Peer admission

`GetNetworkRevisionString` now reserves suffix space inside the existing33-byte
native revision field. `+m441c223p1` is derived from the existing merged save
version, Commands::End and required package version, rather than a new registry.
The ordinary Git hash and debug-mode distinctions remain in the revision. Tagged
builds retain the same explicit merged suffix. `IsNetworkCompatibleVersion`
requires that suffix before full or hash-alias compatibility; the retained hash
comparison also includes it. Original A/B peers therefore refuse even if their
dirty checkout shares the same source hash. Their own unchanged readers likewise
cannot equate the new hash suffix with their original one.

The existing server ReceiveClientJoin calls this gate and the native NewGRF
protocol-version check before SendAuthRequest, client identification, map transfer
or command delivery. The normal NewGRF negotiation and loaded-profile activation
still require exact active package identity/version/digest. The native engine
installer independently checks the required package and canonical EIDS/model
identities; it does not reinterpret another executable's capsule definitions.
No socket protocol field, packet width or command numbering was repurposed.

## Ready native interfaces and refused history

| Interface | Current supported use | Explicit boundary |
| --- | --- | --- |
| CommandTraits/Command and native network dispatch | All223 sequential revision2 command tuples; native U64 purchase/cancellation and typed provider references. | Original B tail IDs and immediate-stock purchase semantics are never executable aliases. |
| GetNetworkRevisionString / IsNetworkCompatibleVersion | Merged revision/content-generation admission before synchronized work; same-build peer qualification can use the normal join path. | Original and stale protocol identities refuse before mixed command execution. Live two-process joining remains Plan4 acceptance. |
| SaveOrLoad and SaveWithFilter | Full/copy native441 loading and sole merged writer, with all128 independently required families. | Original ambiguous saves need the exact source-digest SaveOriginHint; no persistent lineage preference or header rewrite. |
| Capture owner APIs and ComputeLiveMultiLayerStateDigest | Complete installed-state diagnosis after ordinary adoption; real account/packet/RNG observation remains separately testable. | Detached candidates never borrow a running world's geometry or owner roots. |
| InitialiseTubeContentDefinitions and content activation | One canonical capsule catalogue, physical EIDS preservation and exact package checks. | Original provisional tuning and retired provider rights are not alternate live definitions. |
| Historical numeric command logs | Preserve original source facts as evidence. | No automatic log converter or historical replay API is exported. A name/ID map alone does not authorize replay or reproduce B's supply timing. |

The generic low-level EndianBufferReader is a native scalar codec, not a historical
log admission API. These checks do not claim that decoding an arbitrary old byte
array proves its lineage or authorizes executing the resulting tuple.

## Focused proof

Native tests are registered in existing focused/ordinary CMake owners. The build
log is `test-cache/merge-supply/group47-command-peer-build-2.log`. Run the focused
`[merged-command-codec]` and `[merged-peer-identity]` cases beside existing provider,
bounded external-path, required-content and engine-registration checks. The
source audit is independent of all native outputs.

Post-load estimate observation is an explicit mode of the existing machinery
producer: `python -B src/tests/machinery_lineage_native_checks.py --binding-phase 0
--roundtrip --canonical --command-estimates`. It loads the immutable B434 original,
writes and fully reloads441, then quotes valid source closure and stale output/
supply identities through the real command kernel. Neither mode grants energy,
retires a source, creates an identity or changes packets, accounts, both RNGs,
clock, camera or complete canonical state. Its Group47-prefixed outputs remain in
`test-cache/merge-supply`; isolated runtime/process receipts retain the existing
producer's `mlm/runtime` and `mlm/reviews` owners. No protected continuation ticks
or paid windows are authorized by this component test.

Final focused result: eight peer/codec/content/engine/provider cases369assertions,
one checked legacy codec case2assertions, and one actual post-load estimate case
361assertions: ten cases/732assertions. The native migration pair also compares
every128-family byte exactly. `group47-command-compatibility-verification.json`
in the same cache directory binds source audit, native report, three focused
transcripts, current sources and both executables. The first build exposed that
A intentionally has no donor raw-label command overload; the test now reproduces
that immutable original adapter locally without adding a historical runtime API.
