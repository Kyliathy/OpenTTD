# R2MLVS12 M0 execution record - ETI feasibility and native decision

Owner: [r2mlvs12-solar-farms-mirrors-and-eti.md](r2mlvs12-solar-farms-mirrors-and-eti.md), initial feasibility/draft-contract area. Consumer: [r2mlvs1 Content Freeze](r2mlvs1-gameplay-contracts-and-content.md#public-handoff-contract). Architecture: [archi-newgrf.md](../../architecture/newgrf/archi-newgrf.md). This record selects a guaranteed first-party route; it does not certify ETI runtime startup, production, GS behavior or redistribution permission.

## Pinned source fixture, 2026-10-04

The GitHub repository API and raw pinned files were retrieved read-only. Commit **09da329c29e78eeb1f26aab6b8e6492e7c06daf7**, 2026-09-27T16:47:23Z, tree **9b020c79d7ffc43ccc4109a3c44f78373179457a** reproduces the planning revision; VERSION is 14. No third-party file was installed or retained in the game/content package. The tree contains a 76,200-byte compiled GRF with Git blob **aaefe03b8b6b1ba61f9dac3c4bb56255124168f1**. Raw-source UTF-8 SHA256 evidence:

| Pinned file | SHA256 |
| --- | --- |
| VERSION | 9a92adbc0cee38ef658c71ce1b1bf8c65668f166bfb213644c895ccb1ad07a25 |
| README.md | e316c927bce1a3136b4fbf51de2403f6250337efdd1199624af9713eefa2abff |
| energy_transition.nml | 61d8046532608bc4ad1d92bd71846375b5224290ac0342a5a25f41294dd030ad |
| energy_transition_gs/info.nut | 58ed2c77a4e445aed884b711c037c705f4d0e8e7bd7b9a88df181e59bd876a78 |
| energy_transition_gs/main.nut | d0dfd9603699ae52e2232aa097d38d7a72a27f7170822cb453e77c5919d07273 |

The recursive pinned tree has no license/copying declaration; repository metadata reports no detected license. Distribution/modification terms for the GRF, NML, generated sprites and GS remain **unverified**; public access is not a distribution approval. No author contact was made. Source: [pinned tree](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/tree/09da329c29e78eeb1f26aab6b8e6492e7c06daf7), [repository metadata](https://api.github.com/repos/nathanpitman/Open-TTD-Renewable-Energy-Mod).

## Source-level incompatibilities and runtime status

[Pinned NML](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/blob/09da329c29e78eeb1f26aab6b8e6492e7c06daf7/energy_transition.nml) reproduces `RE01`, `POWR` slot/bit 32 and `URAN` 33. Native MLVI already supplies the one sealed `URAN`; ETI calls its version Bulk. Loading both does not constitute an approved mapping, even if they spell the same label. Native exact content validation refuses RE01 as an unsupported external package before world activation. The proposed 32/33 indices are ETI source choices, never fixed native runtime cargo slots. The solar local-ID/layout/year/base-rate observations remain source evidence, not executed values.

POWR is a normal cargo with an Express class and town growth effect in the source. Therefore simply emitting more POWR and separately crediting native electricity would risk freight/grid double reward; no nontransported adapter exists. [Pinned GS](https://github.com/nathanpitman/Open-TTD-Renewable-Energy-Mod/blob/09da329c29e78eeb1f26aab6b8e6492e7c06daf7/energy_transition_gs/main.nut) identifies producers by POWR, uses Manhattan proximity and last-month production, then writes town growth; it does not filter authoritative layers. Its update sleeps configured intervals x 74 ticks, with an initial 100-tick sleep. Version/API metadata is 14/API 13; startup API behavior, economy/calendar mode behavior, nuclear accepted-unit conversion, solar monthly accounting and save/reload are **not run/unverified**.

## Completed feasibility decision and remaining adapter work

**Selected route: original first-party MLVI cargo/native solar/useful power, no ETI adapter and no required Game Script.** Version-1 town authority is Native; existing focused content/identity and power-effect fixtures pass. The source/terms precheck already fails required adapter conditions, so M0 does not install/execute ETI just to complete optional acceptance. Original runtime tasks remain unchecked in Plan 12, available for a later explicit adapter effort; no failed/unrun check is turned into a pass. This is the fallback decision expressly permitted by task 5.1, consumed by Content Freeze, rather than evidence that ETI works.

Compatible third-party revision/hash: **none approved**. The SHA/tree above pin rejected investigation evidence only. Refusal remains the existing profile diagnostic for unsupported external content; missing/changed MLVI has its separate exact-identity refusal. Native active cargos stay 30 (31-slot original-preserving envelope); industry types/tiles at most 64/283, logical vehicles 275. No POWR slot, duplicate Uranium, imported art or GS distribution is added.

The public solar header publishes **draft M0 types/signature only**, with one source-side 4x ceiling, units/remainders, stable target references and invalid-target policy. Its compile fixture does not invoke undeveloped operational mirror/grid code. Plan 12's Useful Power Handoff still depends on completed Content Freeze/Schema Foundation; actual publish/consume/release/export/town integration remains unchecked.
