/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file engine_sl.cpp Native Engine history reconstruction and bounded legacy Space startup repair.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC; paid roles saved before native runtime startup existed retain their actual indices while their never-initialized catalogue history is completed once.
 */

#include "../stdafx.h"

#include "saveload.h"
#include "saveload_error.hpp"
#include "compat/engine_sl_compat.h"

#include "saveload_internal.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../space_production.h"
#include "../core/backup_type.hpp"
#include "../string_func.h"

#include "../safeguards.h"

static const SaveLoad _engine_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("intro_date", SLE_OBJECT_ADDRESS(Engine, intro_date), SaveLoadVersion::MinVersion, SaveLoadVersion::BigDates),
	SaveLoad::Variable<VarFileType::I32>("intro_date", SLE_OBJECT_ADDRESS(Engine, intro_date), SaveLoadVersion::BigDates),
	SaveLoad::Variable<VarFileType::U16>("age", SLE_OBJECT_ADDRESS(Engine, age), SaveLoadVersion::MinVersion, SaveLoadVersion::BigDates),
	SaveLoad::Variable<VarFileType::I32>("age", SLE_OBJECT_ADDRESS(Engine, age), SaveLoadVersion::BigDates),
	SaveLoad::Variable<VarFileType::U16>("reliability", SLE_OBJECT_ADDRESS(Engine, reliability)),
	SaveLoad::Variable<VarFileType::U16>("reliability_spd_dec", SLE_OBJECT_ADDRESS(Engine, reliability_spd_dec)),
	SaveLoad::Variable<VarFileType::U16>("reliability_start", SLE_OBJECT_ADDRESS(Engine, reliability_start)),
	SaveLoad::Variable<VarFileType::U16>("reliability_max", SLE_OBJECT_ADDRESS(Engine, reliability_max)),
	SaveLoad::Variable<VarFileType::U16>("reliability_final", SLE_OBJECT_ADDRESS(Engine, reliability_final)),
	SaveLoad::Variable<VarFileType::U16>("duration_phase_1", SLE_OBJECT_ADDRESS(Engine, duration_phase_1)),
	SaveLoad::Variable<VarFileType::U16>("duration_phase_2", SLE_OBJECT_ADDRESS(Engine, duration_phase_2)),
	SaveLoad::Variable<VarFileType::U16>("duration_phase_3", SLE_OBJECT_ADDRESS(Engine, duration_phase_3)),
	SaveLoad::Variable<VarFileType::U8>("flags", SLE_OBJECT_ADDRESS(Engine, flags)),
	SaveLoad::Variable<VarFileType::U16>("preview_asked", SLE_OBJECT_ADDRESS(Engine, preview_asked), SaveLoadVersion::RobustEnginePreview),
	SaveLoad::Variable<VarFileType::U8>("preview_company", SLE_OBJECT_ADDRESS(Engine, preview_company), SaveLoadVersion::RobustEnginePreview),
	SaveLoad::Variable<VarFileType::U8>("preview_wait", SLE_OBJECT_ADDRESS(Engine, preview_wait)),
	SaveLoad::Variable<VarFileType::U8>("company_avail", SLE_OBJECT_ADDRESS(Engine, company_avail), SaveLoadVersion::MinVersion, SaveLoadVersion::MoreCompanies),
	SaveLoad::Variable<VarFileType::U16>("company_avail", SLE_OBJECT_ADDRESS(Engine, company_avail), SaveLoadVersion::MoreCompanies),
	SaveLoad::Variable<VarFileType::U16>("company_hidden", SLE_OBJECT_ADDRESS(Engine, company_hidden), SaveLoadVersion::HideEnginesForCompany),
	SaveLoad::String("name", SLE_OBJECT_ADDRESS(Engine, name), {}, SaveLoadVersion::ReplaceCustomNameArray),
};

static TypedIndexContainer<std::vector<Engine>, EngineID> _temp_engine;

/**
 * Get temporary engine data for loading savegame engine information.
 * @param index Engine ID of data.
 * @param type Vehicle type of engine.
 * @param local_id The local index of the engine.
 * @return A temporary engine.
 */
Engine *GetTempDataEngine(EngineID index, VehicleType type, uint16_t local_id)
{
	if (index < _temp_engine.size()) {
		return &_temp_engine[index];
	} else if (index == _temp_engine.size()) {
		return &_temp_engine.emplace_back(index, type, local_id);
	} else {
		NOT_REACHED();
	}
}

/** Original native per-Engine availability/reliability history, staged independently of physical hull production and content reconstruction. */
struct ENGNChunkHandler : ChunkHandler {
	/** Register the existing native Engine-history table without adding another availability or vehicle owner. */
	ENGNChunkHandler() : ChunkHandler("ENGN", ChunkType::Table) {}

	/** Serialize each existing native Engine's history at its actual retained index. */
	void Save() const override
	{
		SlTableHeader(_engine_desc);

		// Retain the complete actual catalogue history, including explicit company denial/hiding; a later paid replacement may not infer availability from hull ownership alone.
		for (Engine *e : Engine::Iterate()) {
			SlSetArrayIndex(e->index);
			SlObject(e, _engine_desc);
		}
	}

	/** Stage original native history before content definitions exist; final publication remains CopyTempEngineData's sole responsibility. */
	void Load() const override
	{
		const std::vector<SaveLoad> slt = SlCompatTableHeader(_engine_desc, _engine_sl_compat);

		/* As engine data is loaded before engines are initialized we need to load
		 * this information into a temporary array. This is then copied into the
		 * engine pool after processing NewGRFs by CopyTempEngineData(). */
		int index;
		while ((index = SlIterateArray()) != -1) {
			Engine *e = GetTempDataEngine(static_cast<EngineID>(index));
			SlObject(e, slt);

			if (IsSavegameVersionBefore(SaveLoadVersion::RobustEnginePreview)) {
				/* preview_company_rank was replaced with preview_company and preview_asked.
				 * Just cancel any previews. */
				e->flags.Reset(EngineFlag{2}); // ENGINE_OFFER_WINDOW_OPEN
				e->preview_company = CompanyID::Invalid();
				e->preview_asked.Set();
			}
		}
	}
};

/** Finish only a legacy canonical Space definition that never received native startup, preserving every initialized/denied/preview history and its saved user-hidden flags.
 * @param engine Actual existing native definition after all of its own saved fields have been copied; this allocates no mapping, hull or material owner.
 */
static void RestoreUnstartedSpaceEngine(Engine &engine)
{
	// Only pre429 canonical mappings with the entire untouched constructor/startup pattern qualify. Native availability denial retains initialized flags/reliability phases, so it cannot be overwritten by a migration or a replacement consumer.
	if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceReplacementSettlement) || engine.type != VehicleType::Space ||
		GetSpaceEngine(engine.VehInfo<SpaceVehicleInfo>().model) != engine.index || engine.flags.Any() || engine.company_avail.Any() || engine.age != 0 ||
		engine.reliability_start != 0 || engine.reliability_max != 0 || engine.reliability_final != 0 || engine.reliability_spd_dec != 0 ||
		engine.duration_phase_1 != 0 || engine.duration_phase_2 != 0 || engine.duration_phase_3 != 0 ||
		engine.preview_asked.Any() || engine.preview_company != CompanyID::Invalid() || engine.preview_wait != 0) return;
	AutoRestoreBackup hidden(engine.company_hidden); // User catalogue hiding is saved preference, independent of this missing native startup; restore it exactly after the original startup owner runs.
	StartupNewEngine(&engine);
}

/** Copy all saved native history before repairing only proven never-initialized legacy Space definitions; no physical producer, company money or hull identity changes. */
void CopyTempEngineData()
{
	// Reconstruct each original native Engine at its retained index. Complete initialized saved history takes precedence over runtime definition defaults and is never reset by later replacement admission.
	for (Engine *e : Engine::Iterate()) {
		if (e->index >= _temp_engine.size()) break;

		const Engine *se = GetTempDataEngine(e->index);
		e->intro_date          = se->intro_date;
		e->age                 = se->age;
		e->reliability         = se->reliability;
		e->reliability_spd_dec = se->reliability_spd_dec;
		e->reliability_start   = se->reliability_start;
		e->reliability_max     = se->reliability_max;
		e->reliability_final   = se->reliability_final;
		e->duration_phase_1    = se->duration_phase_1;
		e->duration_phase_2    = se->duration_phase_2;
		e->duration_phase_3    = se->duration_phase_3;
		e->flags               = se->flags;
		e->preview_asked       = se->preview_asked;
		e->preview_company     = se->preview_company;
		e->preview_wait        = se->preview_wait;
		e->company_avail       = se->company_avail;
		e->company_hidden      = se->company_hidden;
		e->name                = se->name;
		RestoreUnstartedSpaceEngine(*e);
	}

	ResetTempEngineData();
}

/** Clear detached native Engine-history staging after publication or world teardown; this resets no live catalogue definition. */
void ResetTempEngineData()
{
	_temp_engine.clear();
	_temp_engine.shrink_to_fit();
}

/** Read-only historical custom-name transport; original names join the existing staged Engine history rather than creating new definitions. */
struct ENGSChunkHandler : ChunkHandler {
	/** Register the original legacy name family for compatibility only. */
	ENGSChunkHandler() : ChunkHandler("ENGS", ChunkType::ReadOnly) {}

	/** Restore original legacy custom names into their matching detached Engine indices. */
	void Load() const override
	{
		/* Load old separate String ID list into a temporary array. This
		 * was always 256 entries. */
		TypedIndexContainer<std::array<StringID, 256>, EngineID> names{};

		SlCopy<VarFileType::StringID>(names);

		/* Copy each string into the temporary engine array. */
		for (EngineID engine = EngineID::Begin(); engine < std::size(names); ++engine) {
			Engine *e = GetTempDataEngine(engine);
			e->name = CopyFromOldName(names[engine]);
		}
	}
};

/** Save and load the mapping between the engine id in the pool, and the grf file it came from. */
static const SaveLoad _engine_id_mapping_desc[] = {
	SaveLoad::Variable<VarFileType::Label>("grfid", SLE_OBJECT_ADDRESS(EngineIDMapping, grfid)),
	SaveLoad::Variable<VarFileType::U16>("internal_id", SLE_OBJECT_ADDRESS(EngineIDMapping, internal_id)),
	SaveLoad::Variable<VarFileType::U8>("type", SLE_OBJECT_ADDRESS(EngineIDMapping, type)),
	SaveLoad::Variable<VarFileType::U8>("substitute_id", SLE_OBJECT_ADDRESS(EngineIDMapping, substitute_id)),
};

/** Native package/local-role-to-Engine mapping transport; actual retained indices remain authoritative across load and paid replacement. */
struct EIDSChunkHandler : ChunkHandler {
	/** Register the sole original native Engine mapping table. */
	EIDSChunkHandler() : ChunkHandler("EIDS", ChunkType::Table) {}

	/** Serialize complete native role mappings in actual Engine-index order without allocating replacement definitions. */
	void Save() const override
	{
		SlTableHeader(_engine_id_mapping_desc);

		/* Count total entries needed for combined list. */
		size_t total = 0;
		for (const auto &mapping : _engine_mngr.mappings) {
			total += std::size(mapping);
		}

		/* Combine per-type mappings into single list for all types. */
		std::vector<EngineIDMapping> temp;
		temp.reserve(total);
		for (const auto &mapping : _engine_mngr.mappings) {
			temp.insert(std::end(temp), std::begin(mapping), std::end(mapping));
		}

		/* Sort combined list by EngineID */
		std::ranges::sort(temp, std::less{}, &EngineIDMapping::engine);

		for (EngineIDMapping &eid : temp) {
			SlSetArrayIndex(eid.engine);
			SlObject(&eid, _engine_id_mapping_desc);
		}
	}

	/** Restore only admitted saved native vehicle-family mappings, leaving physical commissioning to its separate paid producer. */
	void Load() const override
	{
		const std::vector<SaveLoad> slt = SlCompatTableHeader(_engine_id_mapping_desc, _engine_id_mapping_sl_compat);

		_engine_mngr.mappings = {};

		int index;
		while ((index = SlIterateArray()) != -1) {
			EngineIDMapping eid;
			SlObject(&eid, slt);
			// A mapping cannot manufacture another vehicle family or introduce Space identities into a format predating their actual native owner.
			if (!IsCompanyVehicleType(eid.type) || (eid.type == VehicleType::Space && IsSavegameVersionBefore(SaveLoadVersion::SpaceMiningVehicles))) SlErrorCorrupt("Invalid engine vehicle type");
			_engine_mngr.SetID(eid.type, eid.internal_id, eid.grfid, eid.substitute_id, static_cast<EngineID>(index));
		}
	}
};

static const EIDSChunkHandler EIDS;
static const ENGNChunkHandler ENGN;
static const ENGSChunkHandler ENGS;
static const ChunkHandlerRef engine_chunk_handlers[] = {
	EIDS,
	ENGN,
	ENGS,
};

extern const ChunkHandlerTable _engine_chunk_handlers(engine_chunk_handlers);
