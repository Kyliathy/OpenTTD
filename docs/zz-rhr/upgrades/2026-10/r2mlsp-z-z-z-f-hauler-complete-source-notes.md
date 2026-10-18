# R2MLSP - Complete orbital hauler source handoff

Owner: [Space master](../../../zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md), [Plan11 Group13D](../../../zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md#group-13d---deliver-orbital-cargo-hauler-source), [Plan14 Group66D](../../../zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md#group-66d---publish-the-complete-cargo-hauler-source-matrix).

## Dispatch

Groups66B/66C/66D, original66.3 and Plan11/master13D are complete. Continue **Plan11 Group13E / Plan14 Group70.1, actual fixed3x3 Core source**, in order. Packing, native selection and ordinary review remain13K/L. No rebuild, Git write or subagent was used for this source handoff.

The active goal was explicitly resumed by the user. All hauler conversion/test handles are terminal. The completed13D has2compactions;66D has1. Do not increment completed groups when later context compacts. No Core raster or source registration has been started at this handoff.

## Accepted source boundary

- [Full verification](../../../../logs/space-hauler-source-verification.json) SHA256 **aa78f0245dcddf82587200fac86c4c6b213ed37ea2421f9f0d089f55d719643e**:160logical poses,96actual compositions,56explicit static frame aliases. Both service frames in all8cameras are exact empty-source aliases because actual model17 has no service producer.
- Primary source manifest: `logs/proofs/mlm/native-art/space/hauler-complete-v1/space-source-manifest.json`, SHA256 **512874a1051fa152e34f966b691ef1c2561e2f87d19cfb03ed6360b3b8f3f820**. Native atlas `vehicle-model-17-authored.png`, SHA256 **b21e4b0b00769e1260429cdd0c782251790652d823e847a9b6e5c66ebf0788ba**. Actual512x960atlas retains64x48cells and anchor[-32,-32]. Native atlas and both frame contacts inspected.
- Independent source manifest: `logs/proofs/mlm/native-art/space/hauler-complete-repro-v1/space-source-manifest.json`, SHA256 **e2c933d76f64cb2129e37792329ea11b7cb8c274ddb3d5d51924bdd85c5b3931**. Atlas and both contacts are byte-identical to the primary; source decisions, sprite registrations and pose evidence match exactly. Primary42.062s, reproduction34.156s; each child timeout110s.
- [Focused family tests](../../../../logs/space-hauler-family-tests-passing.log):15cases/14.047s wrapper/exit0. Complete matrix equals accepted load/motion/blocked producer atlases. Signed policy corruption cannot erase quantity, animate stopped freight or invent service. Foreign/ambiguous/changed receivers refuse before image IO. Local caching rechecks raw/receipt bytes, rejects changed metadata and missing cameras, and returns independent pixel/evidence copies. Two direct prior-role samples remain exact. An initial test used the wrong load-atlas row; corrected to its actual empty/partial/full layout and rerun successfully.
- Twelve own generated originals and four explicitly registered gas instances from four existing approved originals. Source coverage is complete; public17 is authored with original provenance and exact derived-manifest/native identities. Packed offset/hash remain null; runtime activation remains false. No cargo units, capacity, native movement, service admission or extra sprite part is created.
- All9previous source families,147previous candidate records,147other public rows and the exact shippedGRF remain unchanged. GRF SHA256 **021baffd5857e892cc7d91af939bfe36b50a96bf2cc901ab282458ab09e0e51f**. Protection owner `logs/space-hauler-protected-boundary.json`, SHA256 **2178a536d954ae20fc7d58ca044bd74f13e8aaa7835363e7125060261c2a0869**.

## Implementation and previous evidence

`convert_space_family_art.py` now has an explicit17profile and metadata-only `validate_state_request`. `state_pose` retains prior defaults and optionally consumes the conversion-owned `HaulerSourceBank`. Each full derive resolves96actual compositions once; every logical lookup revalidates metadata and immutable ancestry. There is no global pixel cache. Other roles retain their existing paths.

`convert_space_hauler_parts.py` owns the local bank and exact source-kind mapping. Original load, pulse and display assembly remains unchanged. `src/tests/space_art_hauler_family.py` owns the full-matrix/refusal checks. All three active files pass AST, owning-plan headers, class/function documentation and every argument's `@param` documentation.

The [preceding blocked handoff](r2mlsp-z-z-z-e-hauler-blocked-source-notes.md) contains the precise source receipts, excluded wrong SE view, tiny dark display masks and completed component boundaries. Its statement that the17profile/full matrix was absent is historical. Consume the final verification above; do not replay previous full source families or old native executable graphs. Raw originals, source receipts, selected body registrations and bay/load/nozzle/display geometry remain immutable.

## Next mandatory reads

Read Plan11 Global, Presentation Area and Remaining individual orbital sources Local; Plan14 Global, Source generation Area and **Actual fixed Core source Local immediately above Group70**. Read only public `module.nucleus`, `module_contract`, actual Core footprint/world-tile/connector/berth owners and the existing source packing interface needed for that family. Core is3x3 with fixed native orientation and nine tile variants, not a rotating vehicle or a2x2 module. Confirm actual front/back/world anchors before raster authoring.

Before new raster work, read the canonical Plan13 framed ImageGen/extraction rules and installed imagegen skill. Use built-in generation, immutable originals/receipts and ordinary approved crops/scales/assembly only. Source geometry grants no Core inventory, power, service slot or native clock.

Keep focused child tests under110s; test waits at least50000ms and each blocking wait at most60000ms. Use explicitUTF8 and repository logs. No source/test/JSON filename may start r2. New auxiliary Markdown remains in this tracked archive with next prefix `r2mlsp-z-z-z-g-`. No Git writes or rebuild before the authorized native integration stage.
