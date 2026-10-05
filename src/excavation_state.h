/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_state.h Geology save transport and staged owner publication; never a second live pool. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 4 */

#ifndef EXCAVATION_STATE_H
#define EXCAVATION_STATE_H

#include "excavation.h"

/** Bounded descriptor transport; metadata owns seed/profile, Map owns live geometry, geology owns actual records. */
struct GeologyStateSnapshot {
	uint16_t generator_version = 0; ///< Zero is absent legacy terrain, version 1 is delivered generation.
	uint32_t width = 0; ///< Copied geometry checked against Map, never another mutable geometry owner.
	uint32_t height = 0; ///< Plane height.
	uint32_t next_deposit_id = 0; ///< Monotonic identity cursor survives deletion.
	bool freeform_edges = false; ///< Copied fence policy checked against the saved world settings.
	std::vector<uint8_t> packed; ///< Exactly two rock-plane bitsets, or empty inactive geology.
	std::vector<GeologyDeposit> deposits; ///< Canonical authority; spans are serialized as compact separate rows.
	std::vector<GeothermalVentField> vents; ///< Independent bounded thermal fields, never freight inventory.
	uint32_t next_vent_id = 0; ///< Natural source cursor, separate from mineral and utility IDs.
	std::vector<ExcavationJob> jobs; ///< Actual sparse route/progress authority, excluding rebuilt exclusive lookup.
	uint32_t next_job_id = 0; ///< Saved monotonic cursor survives closed-history retirement.
};

/** @return Snapshot of accepted geological owners; no bucket pointer or compiler padding is serialized. */
GeologyStateSnapshot CaptureGeologyState();

/**
 * Prepare a fully validated restoration without changing live terrain/profile/seed/RNG.
 * @param snapshot Actual bounded save transport.
 * @param restored Output candidate assigned only after all structure/content/reference checks pass.
 * @param check_company_references Validate actual loaded Company pool, after all chunks have been read.
 * @return Refusal or complete prepared candidate; no references are invented or repaired into reserve.
 */
GeologyError PrepareGeologyRestoration(const GeologyStateSnapshot &snapshot, GeologyWorldState &restored, bool check_company_references = true);

/**
 * Stage a complete feature candidate after actual chunk/reference validation.
 * @param state Validated candidate; accepted profile/content is still unpublished during chunk fixups.
 */
void StageLoadedGeologyState(GeologyWorldState state);

/** @return Whether a valid feature candidate exists; required bit/chunk absence cannot silently become legacy terrain. */
bool HasStagedGeologyState();

/** Publish staged geological authority only after the shared metadata/profile acceptance succeeds. */
void AcceptLoadedGeologyState();

/** Clear save transport during InitializeGame; ResetGeologyWorldState also clears its pending candidate without generating terrain. */
void ResetGeologyLoadStaging();

/** Reset bounded job-wire staging during world initialization; preview state remains independent. */
void ResetExcavationLoadStaging();

/**
 * Complete bounded job transport after every chunk exists, before combined geological/reference validation.
 * @param snapshot Actual staged geological transport; assigned jobs are still unpublished.
 */
void CompleteExcavationLoad(GeologyStateSnapshot &snapshot);

/** Validate independent job preview presence/counts/feature identity without touching live or staged geology. */
void ValidateExcavationLoadPreview();

#endif /* EXCAVATION_STATE_H */
