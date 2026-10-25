/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_cmd.h Synchronized player requests over existing paid tube service and binding owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups12B/6BDB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef TRANSPORT_TUBE_CMD_H
#define TRANSPORT_TUBE_CMD_H

#include "command_type.h"
#include "tile_type.h"
#include "vehicle_type.h"
#include "industry_type.h"
#include "utility_power.h"
#include "transport_tube.h"
#include "misc/endian_buffer.hpp"

/** Bounded command transport for the existing physical utility path, without a saved/network inventory owner. */
struct TubePowerCommandPath {
	uint16_t count = 0; ///< Explicit request length; an over-limit value is retained for unchanged command refusal.
	std::array<TileIndex, MAX_TUBE_POWER_PATH> cells{}; ///< Fixed transport bound; only the declared admitted prefix is used.

	/** Create an empty request for ordinary network deserialization. */
	TubePowerCommandPath() = default;
	/** @param path Explicit world cells; oversized requests retain refusal length without copying their cells. */
	explicit TubePowerCommandPath(std::span<const TileIndex> path) : count(static_cast<uint16_t>(std::min<size_t>(path.size(), UINT16_MAX)))
	{
		if (path.size() > this->cells.size()) return;
		std::ranges::copy(path, this->cells.begin());
	}
	/** @param other Native command request to compare. @return Whether length and transported cells agree. */
	bool operator==(const TubePowerCommandPath &other) const = default;
};

/**
 * Serialize only the bounded request prefix; oversized lengths remain explicit refusals.
 * @tparam Tcont Existing native command storage container.
 * @tparam Titer Existing writer iterator over that container.
 * @param buffer Existing native little-endian command writer.
 * @param path Explicit player request, never inferred from a viewport.
 * @return The same writer after its bounded request bytes.
 */
template <typename Tcont, typename Titer>
inline EndianBufferWriter<Tcont, Titer> &operator<<(EndianBufferWriter<Tcont, Titer> &buffer, const TubePowerCommandPath &path)
{
	buffer << path.count;
	if (path.count > path.cells.size()) return buffer;
	// Only real requested cells travel on the wire. The shared physical limit
	// bounds both network work and the later source topology preflight.
	for (uint16_t index = 0; index < path.count; ++index) buffer << path.cells[index];
	return buffer;
}

/**
 * Decode a bounded physical request without allocating from an untrusted path length.
 * @param buffer Existing native command reader.
 * @param path Output transport; oversized lengths remain invalid rather than being truncated into valid routes.
 * @return The same reader after the admitted bounded prefix.
 */
inline EndianBufferReader &operator>>(EndianBufferReader &buffer, TubePowerCommandPath &path)
{
	path = {};
	buffer >> path.count;
	if (path.count > path.cells.size()) return buffer;
	// Match the writer's complete prefix exactly; ordinary command validation
	// then checks ownership, adjacency, layer and excavated endpoint topology.
	for (uint16_t index = 0; index < path.count; ++index) buffer >> path.cells[index];
	return buffer;
}

/**
 * Request one actual same-layer paid service through its atomic source owner.
 * @param flags Pure estimate or synchronized execution.
 * @param portal Actual commissioned rock-side portal endpoint.
 * @param interchange Actual owned tube depot/station receiving service.
 * @param path Explicit bounded excavated adjacent utility path, including its real endpoints.
 * @return Native refusal or zero additional command cost; the source owner alone debits its lease/invoice.
 */
CommandCost CmdConnectTubePower(DoCommandFlags flags, TileIndex portal, TileIndex interchange, const TubePowerCommandPath &path);

/**
 * Retire one exact owned inlet without renewing IDs or refunding settled energy.
 * @param flags Pure estimate or synchronized execution.
 * @param source Actual ledger-issued physical connection identity.
 * @return Native refusal or zero additional command cost; closing settlement remains source-owned.
 */
CommandCost CmdCloseTubePower(DoCommandFlags flags, UtilityProducerID source);

/**
 * Explicitly bind/reconnect a real owned capsule to a selected physically reachable source.
 * @param flags Pure estimate or synchronized execution.
 * @param vehicle Actual native front capsule; its packets/orders and work remain vehicle-owned.
 * @param source Exact installed paid connection, never coordinate-inferred electricity.
 * @param reconnect Whether to replace an existing held binding through its recovery owner rather than make its first binding.
 * @return Permission/refusal without a command fee; no action creates a grant or transfers cargo.
 */
CommandCost CmdSetTubeVehiclePower(DoCommandFlags flags, VehicleID vehicle, UtilityProducerID source, bool reconnect);

/**
 * Attach an existing original native generator to an actual owned tube facility through purchased electrical cells.
 * @param flags Pure estimate or synchronized execution.
 * @param generator Original completed native plant, never a caller-provided electrical output quantity.
 * @param inlet Actual owned tube depot/platform at the end of its installed cable.
 * @param expected_output Original selected plant's nonreused shared output; a recycled industry ID cannot adopt the request.
 * @return Complete admission/refusal with zero extra command fee; existing generator service and cable construction retain payment authority.
 */
CommandCost CmdConnectTubeGeothermalPower(DoCommandFlags flags, IndustryID generator, TileIndex inlet, UtilityProducerID expected_output);

/**
 * Close only an original geothermal inlet and its unused forwarding rights.
 * @param flags Pure estimate or synchronized execution.
 * @param source Independently issued original inlet incarnation, distinct from the generator's shared producer identity.
 * @return Complete owned closure/refusal, without refunding energy, deleting machinery or closing another inlet.
 */
CommandCost CmdCloseTubeGeothermalPower(DoCommandFlags flags, TubeGeothermalSourceID source);

/**
 * Explicitly select an original geothermal provider for an actual native capsule.
 * @param flags Pure estimate or synchronized execution.
 * @param vehicle Actual owned native front with unchanged cargo/order custody.
 * @param source Original inlet incarnation whose real paid path and owned guideway reach the capsule.
 * @param reconnect True replaces its existing held binding; false performs initial stopped-depot admission.
 * @return Complete bind/switch or unchanged refusal; the owning transaction alone retires unused rights and moves the sole demand fraction.
 */
CommandCost CmdSetTubeVehicleGeothermalPower(DoCommandFlags flags, VehicleID vehicle, TubeGeothermalSourceID source, bool reconnect);

DEF_CMD_TRAIT(Commands::ConnectTubePower, CmdConnectTubePower, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::CloseTubePower, CmdCloseTubePower, {}, CommandType::OtherManagement)
DEF_CMD_TRAIT(Commands::SetTubeVehiclePower, CmdSetTubeVehiclePower, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::ConnectTubeGeothermalPower, CmdConnectTubeGeothermalPower, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::CloseTubeGeothermalPower, CmdCloseTubeGeothermalPower, {}, CommandType::OtherManagement)
DEF_CMD_TRAIT(Commands::SetTubeVehicleGeothermalPower, CmdSetTubeVehicleGeothermalPower, {}, CommandType::VehicleManagement)

#endif /* TRANSPORT_TUBE_CMD_H */
