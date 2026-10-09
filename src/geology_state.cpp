/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geology_state.cpp Snapshot/reference validation before geological authority resumes. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 4 */

#include "stdafx.h"
#include "excavation_state.h"
#include "settings_type.h"
#include "company_base.h"

#include "safeguards.h"

/** @return Canonical owner snapshot, excluding spatial buckets and allocation addresses. */
GeologyStateSnapshot CaptureGeologyState()
{
	GeologyStateSnapshot snapshot;
	const auto &state = GetGeologyWorldState();
	snapshot.generator_version = state.generator_version;
	snapshot.hardness_version = state.hardness_version;
	snapshot.hardness_seed = state.hardness_seed;
	snapshot.width = Map::SizeX();
	snapshot.height = Map::SizeY();
	snapshot.next_deposit_id = state.deposits.NextID().base();
	snapshot.freeform_edges = _settings_game.construction.freeform_edges;
	snapshot.packed.assign(state.terrain.Data().begin(), state.terrain.Data().end());
	snapshot.deposits.assign(state.deposits.Records().begin(), state.deposits.Records().end());
	snapshot.vents = state.vents;
	snapshot.next_vent_id = state.next_vent_id;
	snapshot.jobs = state.jobs;
	snapshot.next_job_id = state.next_job_id;
	return snapshot;
}

/**
 * Restore one candidate through actual geometry, shape, content and reference owners before publication.
 * @param snapshot Descriptor transport, with no cached addresses.
 * @param restored Candidate output, changed only on complete acceptance.
 * @param check_company_references Require actual loaded claimants rather than only bounded wire IDs.
 * @return Specific geological refusal or complete preparation.
 */
GeologyError PrepareGeologyRestoration(const GeologyStateSnapshot &snapshot, GeologyWorldState &restored, bool check_company_references)
{
	// Copied geometry/fence metadata must agree with the actual saved Map/settings owners.
	if (snapshot.width != Map::SizeX() || snapshot.height != Map::SizeY() || snapshot.freeform_edges != _settings_game.construction.freeform_edges) return GeologyError::UnsupportedMap;
	GeologyWorldState prepared;
	// Unsupported algorithms and hidden regional state in a legacy/absent
	// world must refuse before accepted jobs or their measured difficulty change.
	if (snapshot.hardness_version > GEOLOGY_HARDNESS_VERSION ||
			(snapshot.hardness_version == 0 && snapshot.hardness_seed != 0) ||
			(snapshot.generator_version == 0 && snapshot.hardness_version != 0)) return GeologyError::InvalidShape;
	if (snapshot.generator_version == 0) {
		// Old/Classic terrain cannot acquire a hidden packed reserve or cursor through an inactive feature header.
		if (!snapshot.packed.empty() || !snapshot.deposits.empty() || snapshot.next_deposit_id != 0 || !snapshot.vents.empty() || snapshot.next_vent_id != 0 || !snapshot.jobs.empty() || snapshot.next_job_id != 0) return GeologyError::InvalidShape;
		restored = std::move(prepared);
		return GeologyError::None;
	}
	if (snapshot.generator_version != GEOLOGY_GENERATOR_VERSION) return GeologyError::InvalidShape;
	if (!CheckGeologyGenerationSize(snapshot.width, snapshot.height)) return GeologyError::UnsupportedMap;
	if (!MultiLayerMap::IsValidLayer(MapLayer::DeepUnderground)) return GeologyError::UnsupportedMap;
	if (!prepared.terrain.Restore(Map::PlaneSize(), snapshot.packed)) return GeologyError::InvalidShape;
	// Companies are resolved after every chunk is read; invalid claimants never become fabricated owners with fresh ore.
	for (const auto &deposit : snapshot.deposits) {
		if (!check_company_references) continue;
		if (deposit.claimant == CompanyID::Invalid()) continue;
		if (!Company::IsValidID(deposit.claimant)) return GeologyError::ClaimConflict;
	}
	const auto error = prepared.deposits.RestoreRecords(snapshot.deposits, snapshot.next_deposit_id);
	if (error != GeologyError::None) return error;
	const auto vent_error = ValidateGeothermalVentFields(snapshot.vents, snapshot.next_vent_id);
	if (vent_error != GeologyError::None) return vent_error;
	prepared.vents = snapshot.vents;
	prepared.next_vent_id = snapshot.next_vent_id;
	prepared.generator_version = snapshot.generator_version;
	prepared.hardness_version = snapshot.hardness_version;
	prepared.hardness_seed = snapshot.hardness_seed;
	prepared.jobs = snapshot.jobs;
	prepared.next_job_id = snapshot.next_job_id;
	// Rebuild exclusive unfinished cells and reject every orphan active packed bit before publishing any owner state.
	const auto job_error = RebuildExcavationJobClaims(prepared);
	if (job_error != GeologyError::None) return job_error;
	// Optional bindings resolve against actual loaded owners; explicit removed-company pauses retain recoverable state.
	for (const auto &job : prepared.jobs) {
		if (check_company_references && !ValidateExcavationJobReferences(job)) return GeologyError::ClaimConflict;
	}
	restored = std::move(prepared);
	return GeologyError::None;
}
