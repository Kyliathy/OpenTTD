/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file industry_sl.cpp Code handling saving and loading of industries. @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1B, independent actual-native preview identity. */

#include "../stdafx.h"

#include "saveload.h"
#include "saveload_error.hpp"
#include "compat/industry_sl_compat.h"

#include "../industry.h"
#include "../fios.h"
#include "../geothermal_riser.h"
#include "newgrf_sl.h"

#include "../safeguards.h"

static OldPersistentStorage _old_ind_persistent_storage;

class SlIndustryAcceptedHistory : public DefaultSaveLoadHandler<SlIndustryAcceptedHistory, Industry::AcceptedCargo> {
public:
	static inline const SaveLoad description[] = {
		SaveLoad::Variable<VarFileType::U16>("accepted", SLE_OBJECT_ADDRESS(Industry::AcceptedHistory, accepted)),
		SaveLoad::Variable<VarFileType::U16>("waiting", SLE_OBJECT_ADDRESS(Industry::AcceptedHistory, waiting)),
	};
	static inline const SaveLoadCompatTable compat_description = _industry_produced_history_sl_compat;

	void Save(Industry::AcceptedCargo *a) const override
	{
		if (!IsValidCargoType(a->cargo) || a->history == nullptr) {
			/* Don't save any history if cargo slot isn't used. */
			SlSetStructListLength(0);
			return;
		}

		SlSetStructListLength(a->history->size());

		for (auto &h : *a->history) {
			SlObject(&h, this->GetDescription());
		}
	}

	void Load(Industry::AcceptedCargo *a) const override
	{
		size_t len = SlGetStructListLength(UINT32_MAX);
		if (len == 0) return;

		auto &history = a->GetOrCreateHistory();
		for (auto &h : history) {
			if (--len > history.size()) break; // unsigned so wraps after hitting zero.
			SlObject(&h, this->GetLoadDescription());
		}
	}
};

class SlIndustryAccepted : public VectorSaveLoadHandler<SlIndustryAccepted, Industry, Industry::AcceptedCargo, INDUSTRY_NUM_INPUTS> {
public:
	static inline const SaveLoad description[] = {
		SaveLoad::Variable<VarFileType::U8>("cargo", SLE_OBJECT_ADDRESS(Industry::AcceptedCargo, cargo)),
		SaveLoad::Variable<VarFileType::U16>("waiting", SLE_OBJECT_ADDRESS(Industry::AcceptedCargo, waiting)),
		SaveLoad::Variable<VarFileType::I32>("last_accepted", SLE_OBJECT_ADDRESS(Industry::AcceptedCargo, last_accepted)),
		SaveLoad::Variable<VarFileType::U32>("accumulated_waiting", SLE_OBJECT_ADDRESS(Industry::AcceptedCargo, accumulated_waiting), SaveLoadVersion::IndustryAcceptedHistory),
		SaveLoad::StructList<SlIndustryAcceptedHistory>("history", SaveLoadVersion::IndustryAcceptedHistory),
	};
	static inline const SaveLoadCompatTable compat_description = _industry_accepts_sl_compat;

	std::vector<Industry::AcceptedCargo> &GetVector(Industry *i) const override { return i->accepted; }

	/** @{
	 * Old array structures used by INDYChunkHandler for savegames before SaveLoadVersion::IndustryCargoReorganise. */
	static inline std::array<CargoType, INDUSTRY_NUM_INPUTS> old_cargo;
	static inline std::array<uint16_t, INDUSTRY_NUM_INPUTS> old_waiting;
	static inline std::array<TimerGameEconomy::Date, INDUSTRY_NUM_INPUTS> old_last_accepted;
	/** @} */

	static void ResetOldStructure()
	{
		SlIndustryAccepted::old_cargo.fill(INVALID_CARGO);
		SlIndustryAccepted::old_waiting.fill(0);
		SlIndustryAccepted::old_last_accepted.fill(TimerGameEconomy::Date{});
	}
};

class SlIndustryProducedHistory : public DefaultSaveLoadHandler<SlIndustryProducedHistory, Industry::ProducedCargo> {
public:
	static inline const SaveLoad description[] = {
		SaveLoad::Variable<VarFileType::U16>("production", SLE_OBJECT_ADDRESS(Industry::ProducedHistory, production)),
		SaveLoad::Variable<VarFileType::U16>("transported", SLE_OBJECT_ADDRESS(Industry::ProducedHistory, transported)),
	};
	static inline const SaveLoadCompatTable compat_description = _industry_produced_history_sl_compat;

	void Save(Industry::ProducedCargo *p) const override
	{
		if (!IsValidCargoType(p->cargo)) {
			/* Don't save any history if cargo slot isn't used. */
			SlSetStructListLength(0);
			return;
		}

		SlSetStructListLength(p->history.size());

		for (auto &h : p->history) {
			SlObject(&h, this->GetDescription());
		}
	}

	void Load(Industry::ProducedCargo *p) const override
	{
		size_t len = SlGetStructListLength(p->history.size());

		for (auto &h : p->history) {
			if (--len > p->history.size()) break; // unsigned so wraps after hitting zero.
			SlObject(&h, this->GetLoadDescription());
		}
	}
};

class SlIndustryProduced : public VectorSaveLoadHandler<SlIndustryProduced, Industry, Industry::ProducedCargo, INDUSTRY_NUM_OUTPUTS> {
public:
	static inline const SaveLoad description[] = {
		SaveLoad::Variable<VarFileType::U8>("cargo", SLE_OBJECT_ADDRESS(Industry::ProducedCargo, cargo)),
		SaveLoad::Variable<VarFileType::U16>("waiting", SLE_OBJECT_ADDRESS(Industry::ProducedCargo, waiting)),
		SaveLoad::Variable<VarFileType::U8>("rate", SLE_OBJECT_ADDRESS(Industry::ProducedCargo, rate)),
		SaveLoad::StructList<SlIndustryProducedHistory>("history"),
	};
	static inline const SaveLoadCompatTable compat_description = _industry_produced_sl_compat;

	std::vector<Industry::ProducedCargo> &GetVector(Industry *i) const override { return i->produced; }

	/** @{
	 * Old array structures used by INDYChunkHandler for savegames before SaveLoadVersion::IndustryCargoReorganise. */
	static inline std::array<CargoType, INDUSTRY_NUM_OUTPUTS> old_cargo;
	static inline std::array<uint16_t, INDUSTRY_NUM_OUTPUTS> old_waiting;
	static inline std::array<uint8_t, INDUSTRY_NUM_OUTPUTS> old_rate;
	static inline std::array<uint16_t, INDUSTRY_NUM_OUTPUTS> old_this_month_production;
	static inline std::array<uint16_t, INDUSTRY_NUM_OUTPUTS> old_this_month_transported;
	static inline std::array<uint16_t, INDUSTRY_NUM_OUTPUTS> old_last_month_production;
	static inline std::array<uint16_t, INDUSTRY_NUM_OUTPUTS> old_last_month_transported;
	/** @} */

	static void ResetOldStructure()
	{
		SlIndustryProduced::old_cargo.fill(INVALID_CARGO);
		SlIndustryProduced::old_waiting.fill(0);
		SlIndustryProduced::old_rate.fill(0);
		SlIndustryProduced::old_this_month_production.fill(0);
		SlIndustryProduced::old_this_month_transported.fill(0);
		SlIndustryProduced::old_last_month_production.fill(0);
		SlIndustryProduced::old_this_month_production.fill(0);
	}
};

static const SaveLoad _industry_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("location.tile", SLE_OBJECT_ADDRESS(Industry, location.tile), SaveLoadVersion::MinVersion, SaveLoadVersion::MultipleRoadStops),
	SaveLoad::Variable<VarFileType::U32>("location.tile", SLE_OBJECT_ADDRESS(Industry, location.tile), SaveLoadVersion::MultipleRoadStops),
	SaveLoad::Variable<VarFileType::U8>("location.w", SLE_OBJECT_ADDRESS(Industry, location.w)),
	SaveLoad::Variable<VarFileType::U8>("location.h", SLE_OBJECT_ADDRESS(Industry, location.h)),
	SaveLoad::Reference<SLRefType::Town>("town", SLE_OBJECT_ADDRESS(Industry, town)),
	SaveLoad::Reference<SLRefType::Station>("neutral_station", SLE_OBJECT_ADDRESS(Industry, neutral_station), SaveLoadVersion::ServeNeutralIndustries),
	SaveLoad::Array<VarFileType::U8, INDUSTRY_ORIGINAL_NUM_OUTPUTS>("produced_cargo", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_cargo), SaveLoadVersion::StoreIndustryCargo, SaveLoadVersion::ExtendIndustryCargoSlots),
	SaveLoad::Array<VarFileType::U8, INDUSTRY_NUM_OUTPUTS>("produced_cargo", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_cargo), SaveLoadVersion::ExtendIndustryCargoSlots, SaveLoadVersion::IndustryCargoReorganise),
	SaveLoad::Array<VarFileType::U16, INDUSTRY_ORIGINAL_NUM_INPUTS>("incoming_cargo_waiting", SLE_GLOBAL_ADDRESS(SlIndustryAccepted::old_waiting), SaveLoadVersion::CargoPaymentOverflow, SaveLoadVersion::ExtendIndustryCargoSlots),
	SaveLoad::Array<VarFileType::U16, INDUSTRY_NUM_INPUTS>("incoming_cargo_waiting", SLE_GLOBAL_ADDRESS(SlIndustryAccepted::old_waiting), SaveLoadVersion::ExtendIndustryCargoSlots, SaveLoadVersion::IndustryCargoReorganise),
	SaveLoad::Array<VarFileType::U16, INDUSTRY_ORIGINAL_NUM_OUTPUTS>("produced_cargo_waiting", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_waiting), SaveLoadVersion::MinVersion, SaveLoadVersion::ExtendIndustryCargoSlots),
	SaveLoad::Array<VarFileType::U16, INDUSTRY_NUM_OUTPUTS>("produced_cargo_waiting", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_waiting), SaveLoadVersion::ExtendIndustryCargoSlots, SaveLoadVersion::IndustryCargoReorganise),
	SaveLoad::Array<VarFileType::U8, INDUSTRY_ORIGINAL_NUM_OUTPUTS>("production_rate", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_rate), SaveLoadVersion::MinVersion, SaveLoadVersion::ExtendIndustryCargoSlots),
	SaveLoad::Array<VarFileType::U8, INDUSTRY_NUM_OUTPUTS>("production_rate", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_rate), SaveLoadVersion::ExtendIndustryCargoSlots, SaveLoadVersion::IndustryCargoReorganise),
	SaveLoad::Array<VarFileType::U8, INDUSTRY_ORIGINAL_NUM_INPUTS>("accepts_cargo", SLE_GLOBAL_ADDRESS(SlIndustryAccepted::old_cargo), SaveLoadVersion::StoreIndustryCargo, SaveLoadVersion::ExtendIndustryCargoSlots),
	SaveLoad::Array<VarFileType::U8, INDUSTRY_NUM_INPUTS>("accepts_cargo", SLE_GLOBAL_ADDRESS(SlIndustryAccepted::old_cargo), SaveLoadVersion::ExtendIndustryCargoSlots, SaveLoadVersion::IndustryCargoReorganise),
	SaveLoad::Variable<VarFileType::U8>("prod_level", SLE_OBJECT_ADDRESS(Industry, prod_level)),
	SaveLoad::Array<VarFileType::U16, INDUSTRY_ORIGINAL_NUM_OUTPUTS>("this_month_production", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_this_month_production), SaveLoadVersion::MinVersion, SaveLoadVersion::ExtendIndustryCargoSlots),
	SaveLoad::Array<VarFileType::U16, INDUSTRY_NUM_OUTPUTS>("this_month_production", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_this_month_production), SaveLoadVersion::ExtendIndustryCargoSlots, SaveLoadVersion::IndustryCargoReorganise),
	SaveLoad::Array<VarFileType::U16, INDUSTRY_ORIGINAL_NUM_OUTPUTS>("this_month_transported", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_this_month_transported), SaveLoadVersion::MinVersion, SaveLoadVersion::ExtendIndustryCargoSlots),
	SaveLoad::Array<VarFileType::U16, INDUSTRY_NUM_OUTPUTS>("this_month_transported", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_this_month_transported), SaveLoadVersion::ExtendIndustryCargoSlots, SaveLoadVersion::IndustryCargoReorganise),
	SaveLoad::Array<VarFileType::U16, INDUSTRY_ORIGINAL_NUM_OUTPUTS>("last_month_production", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_last_month_production), SaveLoadVersion::MinVersion, SaveLoadVersion::ExtendIndustryCargoSlots),
	SaveLoad::Array<VarFileType::U16, INDUSTRY_NUM_OUTPUTS>("last_month_production", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_last_month_production), SaveLoadVersion::ExtendIndustryCargoSlots, SaveLoadVersion::IndustryCargoReorganise),
	SaveLoad::Array<VarFileType::U16, INDUSTRY_ORIGINAL_NUM_OUTPUTS>("last_month_transported", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_last_month_transported), SaveLoadVersion::MinVersion, SaveLoadVersion::ExtendIndustryCargoSlots),
	SaveLoad::Array<VarFileType::U16, INDUSTRY_NUM_OUTPUTS>("last_month_transported", SLE_GLOBAL_ADDRESS(SlIndustryProduced::old_last_month_transported), SaveLoadVersion::ExtendIndustryCargoSlots, SaveLoadVersion::IndustryCargoReorganise),

	SaveLoad::Variable<VarFileType::U16>("counter", SLE_OBJECT_ADDRESS(Industry, counter)),

	SaveLoad::Variable<VarFileType::U8>("type", SLE_OBJECT_ADDRESS(Industry, type)),
	SaveLoad::Variable<VarFileType::U8>("owner", SLE_OBJECT_ADDRESS(Industry, owner)),
	SaveLoad::Variable<VarFileType::U8>("random_colour", SLE_OBJECT_ADDRESS(Industry, random_colour)),
	SaveLoad::Variable<VarFileType::U8>("last_prod_year", SLE_OBJECT_ADDRESS(Industry, last_prod_year), SaveLoadVersion::MinVersion, SaveLoadVersion::BigDates),
	SaveLoad::Variable<VarFileType::I32>("last_prod_year", SLE_OBJECT_ADDRESS(Industry, last_prod_year), SaveLoadVersion::BigDates),
	SaveLoad::Variable<VarFileType::U8>("was_cargo_delivered", SLE_OBJECT_ADDRESS(Industry, was_cargo_delivered)),
	SaveLoad::Variable<VarFileType::U8>("ctlflags", SLE_OBJECT_ADDRESS(Industry, ctlflags), SaveLoadVersion::GSIndustryControl),

	SaveLoad::Variable<VarFileType::U8>("founder", SLE_OBJECT_ADDRESS(Industry, founder), SaveLoadVersion::CargoPaymentOverflow),
	SaveLoad::Variable<VarFileType::I32>("construction_date", SLE_OBJECT_ADDRESS(Industry, construction_date), SaveLoadVersion::CargoPaymentOverflow),
	SaveLoad::Variable<VarFileType::U8>("construction_type", SLE_OBJECT_ADDRESS(Industry, construction_type), SaveLoadVersion::CargoPaymentOverflow),
	SaveLoad::Variable<VarFileType::I32>("last_cargo_accepted_at[0]", SLE_GLOBAL_ADDRESS(SlIndustryAccepted::old_last_accepted[0]), SaveLoadVersion::CargoPaymentOverflow, SaveLoadVersion::ExtendIndustryCargoSlots),
	SaveLoad::Array<VarFileType::I32, 16>("last_cargo_accepted_at", SLE_GLOBAL_ADDRESS(SlIndustryAccepted::old_last_accepted), SaveLoadVersion::ExtendIndustryCargoSlots, SaveLoadVersion::IndustryCargoReorganise),
	SaveLoad::Variable<VarFileType::U8>("selected_layout", SLE_OBJECT_ADDRESS(Industry, selected_layout), SaveLoadVersion::NewGRFIndustryLayout),
	SaveLoad::Variable<VarFileType::U8>("exclusive_supplier", SLE_OBJECT_ADDRESS(Industry, exclusive_supplier), SaveLoadVersion::GSIndustryControl),
	SaveLoad::Variable<VarFileType::U8>("exclusive_consumer", SLE_OBJECT_ADDRESS(Industry, exclusive_consumer), SaveLoadVersion::GSIndustryControl),

	SaveLoad::Array<VarFileType::U32, 16>("storage", SLE_GLOBAL_ADDRESS(_old_ind_persistent_storage.storage), SaveLoadVersion::NewGRFPersistentStorage, SaveLoadVersion::PersistentStoragePool),
	SaveLoad::Reference<SLRefType::Storage>("psa", SLE_OBJECT_ADDRESS(Industry, psa), SaveLoadVersion::PersistentStoragePool),

	SaveLoad::Variable<VarFileType::U16>("random", SLE_OBJECT_ADDRESS(Industry, random), SaveLoadVersion::NewGRFIndustryRandomTriggers),
	SaveLoad::String("text", SLE_OBJECT_ADDRESS(Industry, text), StringValidationSetting::AllowControlCode, SaveLoadVersion::IndustryText),

	SaveLoad::Variable<VarFileType::U64>("valid_history", SLE_OBJECT_ADDRESS(Industry, valid_history), SaveLoadVersion::IndustryNumValidHistory),

	SaveLoad::StructList<SlIndustryAccepted>("accepted", SaveLoadVersion::IndustryCargoReorganise),
	SaveLoad::StructList<SlIndustryProduced>("produced", SaveLoadVersion::IndustryCargoReorganise),
};

/** Native machinery/inventory transport; current389 preview copies real pool identities without creating a running industry or a second stock owner. */
struct INDYChunkHandler : ChunkHandler {
	/** Register the existing authoritative industry table. */
	INDYChunkHandler() : ChunkHandler("INDY", ChunkType::Table) {}

	/** Save every actual industry and its native accepted/produced cargo exactly once. */
	void Save() const override
	{
		SlTableHeader(_industry_desc);

		/* Write the industries */
		for (Industry *ind : Industry::Iterate()) {
			SlSetArrayIndex(ind->index);
			SlObject(ind, _industry_desc);
		}
	}

	/** @param len Older formats retain their ordinary skip policy; current389 copies bounded native identities without constructing any industry or inventory. */
	void LoadCheck(size_t len) const override
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::GeothermalCollectors)) { ChunkHandler::LoadCheck(len); return; }
		static const SaveLoad preview_desc[] = {
			SaveLoad::Variable<VarFileType::U32>("location.tile", SLE_OBJECT_ADDRESS(NativeIndustryPreview, tile)),
			SaveLoad::Variable<VarFileType::U8>("location.w", SLE_OBJECT_ADDRESS(NativeIndustryPreview, width)),
			SaveLoad::Variable<VarFileType::U8>("location.h", SLE_OBJECT_ADDRESS(NativeIndustryPreview, height)),
			SaveLoad::Variable<VarFileType::U8>("type", SLE_OBJECT_ADDRESS(NativeIndustryPreview, type)),
		};
		const bool machinery = !IsSavegameVersionBefore(SaveLoadVersion::GeothermalRisers);
		const auto desc = machinery ? SlTableHeader(preview_desc) : SlTableHeader({});
		if (machinery) {
			// Native identity/shape fields must come from this file; defaults
			// cannot hide a paid facility behind a missing type or anchor column.
			for (const auto &field : preview_desc) {
				if (std::ranges::find(desc, field.name, &SaveLoad::name) == desc.end()) SlErrorCorrupt("Missing native industry preview field");
			}
		}
		auto &ids = _load_check_data.native_industry_preview_ids;
		// Copy only real serialized native pool identities. Unknown native fields
		// are skipped by the descriptor reader, never decoded into running pools.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (index >= 64000 || (!ids.empty() && index <= ids.back())) SlErrorCorrupt("Invalid native industry preview identity"); // Native pool bounds and canonical order prevent duplicate/oversized construction references.
			NativeIndustryPreview row; row.id = static_cast<uint16_t>(index);
			SlObject(machinery ? &row : nullptr, desc); ids.push_back(row.id);
			if (machinery) _load_check_data.native_industry_preview_rows.push_back(row); // Copy native shape/type only; accepted stocks/history stay in their actual transport and full-load validation.
		}
	}

	/** @param i Actual loading native industry. @param inputs Legacy input slot count. @param outputs Legacy output slot count. Move old serialized stock/history into the same native inventory owner. */
	void LoadMoveAcceptsProduced(Industry *i, uint inputs, uint outputs) const
	{
		i->accepted.reserve(inputs);
		for (uint j = 0; j != inputs; ++j) {
			auto &a = i->accepted.emplace_back();
			a.cargo = SlIndustryAccepted::old_cargo[j];
			a.waiting = SlIndustryAccepted::old_waiting[j];
			a.last_accepted = SlIndustryAccepted::old_last_accepted[j];
		}

		i->produced.reserve(outputs);
		for (uint j = 0; j != outputs; ++j) {
			auto &p = i->produced.emplace_back();
			p.cargo = SlIndustryProduced::old_cargo[j];
			p.waiting = SlIndustryProduced::old_waiting[j];
			p.rate = SlIndustryProduced::old_rate[j];
			p.history[THIS_MONTH].production = SlIndustryProduced::old_this_month_production[j];
			p.history[THIS_MONTH].transported = SlIndustryProduced::old_this_month_transported[j];
			p.history[LAST_MONTH].production = SlIndustryProduced::old_last_month_production[j];
			p.history[LAST_MONTH].transported = SlIndustryProduced::old_last_month_transported[j];
		}
	}

	/** Restore actual native pool/inventory records; construction side-state resolves these existing owners after normal fixup. */
	void Load() const override
	{
		const std::vector<SaveLoad> slt = SlCompatTableHeader(_industry_desc, _industry_sl_compat);

		int index;

		SlIndustryAccepted::ResetOldStructure();
		SlIndustryProduced::ResetOldStructure();

		while ((index = SlIterateArray()) != -1) {
			Industry *i = Industry::CreateAtIndex(IndustryID(index));
			SlObject(i, slt);

			/* Before savegame version 161, persistent storages were not stored in a pool. */
			if (IsSavegameVersionBefore(SaveLoadVersion::PersistentStoragePool) && !IsSavegameVersionBefore(SaveLoadVersion::NewGRFPersistentStorage)) {
				/* Store the old persistent storage. The GRFID will be added later. */
				assert(PersistentStorage::CanAllocateItem());
				i->psa = PersistentStorage::Create(GrfID{}, GrfSpecFeature::Invalid, TileIndex{});
				std::copy(std::begin(_old_ind_persistent_storage.storage), std::end(_old_ind_persistent_storage.storage), std::begin(i->psa->storage));
			}
			if (IsSavegameVersionBefore(SaveLoadVersion::ExtendIndustryCargoSlots)) {
				LoadMoveAcceptsProduced(i, INDUSTRY_ORIGINAL_NUM_INPUTS, INDUSTRY_ORIGINAL_NUM_OUTPUTS);
			} else if (IsSavegameVersionBefore(SaveLoadVersion::IndustryCargoReorganise)) {
				LoadMoveAcceptsProduced(i, INDUSTRY_NUM_INPUTS, INDUSTRY_NUM_OUTPUTS);
			}

			if (IsSavegameVersionBefore(SaveLoadVersion::IndustryNumValidHistory)) {
				/* The last month has always been recorded. */
				size_t oldest_valid = LAST_MONTH;
				if (!IsSavegameVersionBefore(SaveLoadVersion::ProductionHistory)) {
					/* History was extended but we did not keep track of valid history, so assume it from the oldest non-zero value. */
					for (const auto &p : i->produced) {
						if (!IsValidCargoType(p.cargo)) continue;
						for (size_t n = LAST_MONTH; n < std::size(p.history); ++n) {
							if (p.history[n].production == 0 && p.history[n].transported == 0) continue;
							oldest_valid = std::max(oldest_valid, n);
						}
					}
				}
				/* Set mask bits up to and including the oldest valid record. */
				i->valid_history = (std::numeric_limits<uint64_t>::max() >> (std::numeric_limits<uint64_t>::digits - (oldest_valid + 1 - LAST_MONTH))) << LAST_MONTH;
			}

			Industry::industries[i->type].insert(i->index);
		}
	}

	void FixPointers() const override
	{
		for (Industry *i : Industry::Iterate()) {
			SlObject(i, _industry_desc);
		}
	}
};

/** Canonical native industry mappings, copied independently only when version400 dependent machinery needs definition identity. */
struct IIDSChunkHandler : NewGRFMappingChunkHandler {
	/** Register the existing native allocator's mapping family without changing its on-disk format. */
	IIDSChunkHandler() : NewGRFMappingChunkHandler("IIDS", _industry_mngr) {}
	/** @param len Older files retain ordinary skip behavior; current preview copies exact riser mappings without mutating the running native manager. */
	void LoadCheck(size_t len) const override
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::GeothermalRisers)) { ChunkHandler::LoadCheck(len); return; }
		static const SaveLoad copied[] = {
			SaveLoad::Variable<VarFileType::Label>("grfid", SLE_OBJECT_ADDRESS(EntityIDMapping, grfid)),
			SaveLoad::Variable<VarFileType::U16>("entity_id", SLE_OBJECT_ADDRESS(EntityIDMapping, entity_id)),
			SaveLoad::Variable<VarFileType::U16>("substitute_id", SLE_OBJECT_ADDRESS(EntityIDMapping, substitute_id)),
		};
		const auto desc = SlTableHeader(copied);
		if (desc.size() != std::size(copied)) SlErrorCorrupt("Invalid native industry preview mapping fields");
		// Exact existing mapping columns prevent default local IDs from hiding
		// omitted construction or impersonating a different native definition.
		for (const auto &field : copied) {
			if (std::ranges::find(desc, field.name, &SaveLoad::name) == desc.end()) SlErrorCorrupt("Missing native industry mapping field");
		}
		const auto grfid = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package.grfid;
		int previous = -1;
		// Native IDs and exact package/local meanings resolve from this file's
		// mapping, so a missing machinery row cannot hide behind a cleared feature.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (index <= previous || index >= NUM_INDUSTRYTYPES) SlErrorCorrupt("Invalid native industry preview mapping");
			EntityIDMapping row{}; SlObject(&row, desc); previous = index;
			if (row.grfid == grfid && row.entity_id == GEOTHERMAL_RISER_LOCAL_ID) {
				if (!_load_check_data.native_riser_preview_types.empty()) SlErrorCorrupt("Duplicate native riser definition mapping");
				_load_check_data.native_riser_preview_types.push_back(static_cast<uint8_t>(index)); // Copied definition identity only; preview installs no catalogue or native pool.
			}
		}
	}
};

struct TIDSChunkHandler : NewGRFMappingChunkHandler {
	TIDSChunkHandler() : NewGRFMappingChunkHandler("TIDS", _industile_mngr) {}
};

/** Description of the data to save and load in #IndustryBuildData. */
static const SaveLoad _industry_builder_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("wanted_inds", SLE_GLOBAL_ADDRESS(_industry_builder.wanted_inds)),
};

/** Industry builder. */
struct IBLDChunkHandler : ChunkHandler {
	IBLDChunkHandler() : ChunkHandler("IBLD", ChunkType::Table) {}

	void Save() const override
	{
		SlTableHeader(_industry_builder_desc);

		SlSetArrayIndex(0);
		SlGlobList(_industry_builder_desc);
	}

	void Load() const override
	{
		const std::vector<SaveLoad> slt = SlCompatTableHeader(_industry_builder_desc, _industry_builder_sl_compat);

		if (!IsSavegameVersionBefore(SaveLoadVersion::RiffToArray) && SlIterateArray() == -1) return;
		SlGlobList(slt);
		if (!IsSavegameVersionBefore(SaveLoadVersion::RiffToArray) && SlIterateArray() != -1) SlErrorCorrupt("Too many IBLD entries");
	}
};

/** Description of the data to save and load in #IndustryTypeBuildData. */
static const SaveLoad _industrytype_builder_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("probability", SLE_OBJECT_ADDRESS(IndustryTypeBuildData, probability)),
	SaveLoad::Variable<VarFileType::U8>("min_number", SLE_OBJECT_ADDRESS(IndustryTypeBuildData, min_number)),
	SaveLoad::Variable<VarFileType::U16>("target_count", SLE_OBJECT_ADDRESS(IndustryTypeBuildData, target_count)),
	SaveLoad::Variable<VarFileType::U16>("max_wait", SLE_OBJECT_ADDRESS(IndustryTypeBuildData, max_wait)),
	SaveLoad::Variable<VarFileType::U16>("wait_count", SLE_OBJECT_ADDRESS(IndustryTypeBuildData, wait_count)),
};

/** Industry-type build data. */
struct ITBLChunkHandler : ChunkHandler {
	ITBLChunkHandler() : ChunkHandler("ITBL", ChunkType::Table) {}

	void Save() const override
	{
		SlTableHeader(_industrytype_builder_desc);

		for (int i = 0; i < NUM_INDUSTRYTYPES; i++) {
			SlSetArrayIndex(i);
			SlObject(_industry_builder.builddata + i, _industrytype_builder_desc);
		}
	}

	void Load() const override
	{
		const std::vector<SaveLoad> slt = SlCompatTableHeader(_industrytype_builder_desc, _industrytype_builder_sl_compat);

		for (IndustryType it = 0; it < NUM_INDUSTRYTYPES; it++) {
			_industry_builder.builddata[it].Reset();
		}
		int index;
		while ((index = SlIterateArray()) != -1) {
			if ((uint)index >= NUM_INDUSTRYTYPES) SlErrorCorrupt("Too many industry builder datas");
			SlObject(_industry_builder.builddata + index, slt);
		}
	}
};

static const INDYChunkHandler INDY;
static const IIDSChunkHandler IIDS;
static const TIDSChunkHandler TIDS;
static const IBLDChunkHandler IBLD;
static const ITBLChunkHandler ITBL;
static const ChunkHandlerRef industry_chunk_handlers[] = {
	INDY,
	IIDS,
	TIDS,
	IBLD,
	ITBL,
};

extern const ChunkHandlerTable _industry_chunk_handlers(industry_chunk_handlers);
