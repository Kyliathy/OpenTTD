/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_model.h Canonical physical capsule geometry before model art or engine activation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group10B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups49-53.
 */

#ifndef TRANSPORT_TUBE_MODEL_H
#define TRANSPORT_TUBE_MODEL_H

#include <cstdint>
#include <optional>
#include "newgrf_config.h"

#include "command_type.h"
#include "cargo_type.h"
#include "engine_type.h"

struct LayerVehicleCapability;
struct Train;

/**
 * Immutable physical source contract over a borrowed canonical content model.
 * All five capsules use one full native rail part; distinct hull/load art and
 * model balance do not allocate another movement, cargo or capability owner.
 * A descriptor may precede its native engine definition and never activates it.
 */
struct TubeVehicleGeometry {
	const LayerVehicleCapability *catalogue = nullptr; ///< Borrowed immutable profile row; lifetime is the canonical catalogue's.
	uint8_t vehicle_length_units = 0; ///< Native spacing; VEHICLE_LENGTH is one full part, independent of sprite pixels.
	uint8_t native_parts = 0; ///< One physical self-propelled capsule part; source art cannot add articulated vehicles.
};

/**
 * Resolve physical source geometry from the sole logical content catalogue.
 * @param model_id Immutable profile model identity, never a runtime EngineID.
 * @return Borrowed-capability geometry for a canonical tube design, or absence
 * for unknown/ordinary/orbital designs. No engine, vehicle, funds, cargo or
 * utility state is allocated or changed; dates and activation remain separate.
 */
std::optional<TubeVehicleGeometry> GetTubeVehicleGeometry(uint16_t model_id);

/** Native train tuning only; logical dates/cargos/layers and metered energy retain their canonical owners. */
struct TubeVehicleDefinition {
	uint16_t model_id; ///< Canonical immutable profile model identity, independent of the runtime EIDS slot.
	uint16_t max_speed; ///< Native train speed units, read by the normal movement owner.
	uint16_t traction_hp; ///< Native acceleration power; this never grants electrical joules.
	uint16_t empty_weight_t; ///< Real empty weight used by native consist/acceleration caches.
	uint8_t capacity; ///< Native whole cargo units, before the ordinary refit owner applies the selected cargo policy.
	uint8_t purchase_factor; ///< Native train purchase factor; actual Money comes from the normal price owner.
	uint8_t running_factor; ///< Native electric running factor; paid utility use is invoiced separately.
};

/**
 * Borrow the first-party capsule's native physics/pricing definition without installing it.
 * @param model_id Existing canonical tube model, never an engine-pool identity.
 * @return Immutable native tuning, or nullptr for an unknown/ordinary/orbital design.
 * Model9 preserves its delivered prototype terms; other models have distinct capacity/speed/cost envelopes.
 */
const TubeVehicleDefinition *GetTubeVehicleDefinition(uint16_t model_id);

/**
 * Refuse a native capsule refit/replacement that changes loaded cargo meaning or loses real packets.
 * @param current Actual existing native train part, including its current cargo and reservations.
 * @param target Proposed native/ordinary engine; movement domains cannot be mixed by replacement.
 * @param cargo Exact cargo retained/refitted on that target.
 * @param retained_capacity Actual operation-specific retention bound; refit passes native refit_cap, replacement uses the target's capacity by default.
 * @return Pure admission or refusal before temporary vehicles, stock truncation, cache changes or charges.
 * Empty carriers may change their allowed cargo form; loaded carriers must retain exact labels and sufficient capacity.
 */
CommandCost CheckTubeVehicleCargoChange(const Train &current, EngineID target, CargoType cargo, uint16_t retained_capacity = UINT16_MAX);

/**
 * Install admitted native tube content during normal NewGRF polishing, independently of saved object presence.
 * @param activated Exact active content configuration; foreign/missing required content cannot create definitions.
 * @return Whether the canonical registry and all five EIDS models are available.
 * The registry remains hidden from ordinary rail menus; actual profile/date/floor and paid power admission remain separate.
 */
bool InitialiseTubeContentDefinitions(const GRFConfigList &activated);

#endif /* TRANSPORT_TUBE_MODEL_H */
