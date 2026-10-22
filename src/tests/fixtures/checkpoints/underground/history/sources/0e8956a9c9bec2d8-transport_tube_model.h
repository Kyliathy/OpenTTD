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

struct LayerVehicleCapability;

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

#endif /* TRANSPORT_TUBE_MODEL_H */
