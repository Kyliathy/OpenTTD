/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_cmd.cpp Owner-checked network commands delegate all payment/quantity changes to tube producers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups12B/6BDB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "stdafx.h"
#include "transport_tube_cmd.h"
#include "transport_tube.h"
#include "transport_tube_geothermal.h"
#include "geothermal_generator.h"
#include "command_func.h"
#include "train.h"
#include "table/strings.h"
#include "safeguards.h"

/**
 * Translate actual service refusals without charging the normal command engine a second time.
 * @param error Existing physical/service owner's exact result.
 * @param geothermal Native original-source command; its admission must not tell the player to build an external portal route or buy another lease.
 * @return Localized refusal or zero extra cost; direct source invoices remain the sole payment authority.
 */
static CommandCost TubeServiceCommandResult(TubePowerError error, bool geothermal = false)
{
	switch (error) {
		case TubePowerError::None: return CommandCost(ExpensesType::Other);
		case TubePowerError::InvalidOwner: return CommandCost(geothermal ? STR_ERROR_TUBE_POWER_REFUSED : STR_ERROR_TUBE_POWER_OWNER);
		case TubePowerError::Funds: return CommandCost(geothermal ? STR_ERROR_TUBE_POWER_REFUSED : STR_ERROR_TUBE_POWER_FUNDS);
		case TubePowerError::Unavailable: return CommandCost(STR_ERROR_TUBE_POWER_UNAVAILABLE);
		case TubePowerError::InvalidTopology: return CommandCost(geothermal ? STR_ERROR_TUBE_POWER_REFUSED : STR_ERROR_TUBE_POWER_TOPOLOGY);
		case TubePowerError::Limit: return CommandCost(STR_ERROR_TUBE_POWER_LIMIT);
		case TubePowerError::Ledger:
		case TubePowerError::Refused: return CommandCost(STR_ERROR_TUBE_POWER_REFUSED);
	}
	return CommandCost(STR_ERROR_TUBE_POWER_REFUSED);
}

/** @copydoc CmdConnectTubePower */
CommandCost CmdConnectTubePower(DoCommandFlags flags, TileIndex portal, TileIndex interchange, const TubePowerCommandPath &path)
{
	if (path.count > path.cells.size()) return CommandCost(STR_ERROR_TUBE_POWER_LIMIT);
	return TubeServiceCommandResult(ConnectTubeExternalPower(portal, interchange,
		std::span{path.cells}.first(path.count), flags.Test(DoCommandFlag::Execute)).error);
}

/** @copydoc CmdCloseTubePower */
CommandCost CmdCloseTubePower(DoCommandFlags flags, UtilityProducerID source)
{
	return TubeServiceCommandResult(CloseTubeExternalPower(source, flags.Test(DoCommandFlag::Execute)));
}

/** @copydoc CmdSetTubeVehiclePower */
CommandCost CmdSetTubeVehiclePower(DoCommandFlags flags, VehicleID vehicle, UtilityProducerID source, bool reconnect)
{
	const Train *train = Train::GetIfValid(vehicle);
	if (train == nullptr) return CommandCost(STR_ERROR_TUBE_POWER_REFUSED);
	if (CommandCost owner = CheckOwnership(train->owner); owner.Failed()) return owner;
	const auto result = reconnect ? ReconnectTubePowerVehicle(vehicle, source, flags.Test(DoCommandFlag::Execute)) :
		BindTubePowerVehicle(vehicle, source, flags.Test(DoCommandFlag::Execute));
	return TubeServiceCommandResult(result.has_value() ? TubePowerError::None : TubePowerError::Refused);
}

/** @copydoc CmdConnectTubeGeothermalPower */
CommandCost CmdConnectTubeGeothermalPower(DoCommandFlags flags, IndustryID generator, TileIndex inlet, UtilityProducerID expected_output)
{
	const auto *plant = GetGeothermalGeneratorState(generator);
	// A queued placement names the original acquired output as well as the
	// recyclable native pool ID; replacement machinery needs a fresh selection.
	if (plant == nullptr || expected_output.base() == UINT32_MAX || plant->electrical_producer != expected_output) return CommandCost(STR_ERROR_TUBE_POWER_REFUSED);
	return TubeServiceCommandResult(ConnectTubeGeothermalPower(generator, inlet, flags.Test(DoCommandFlag::Execute)).error, true);
}

/** @copydoc CmdCloseTubeGeothermalPower */
CommandCost CmdCloseTubeGeothermalPower(DoCommandFlags flags, TubeGeothermalSourceID source)
{
	return TubeServiceCommandResult(CloseTubeGeothermalPower(source, flags.Test(DoCommandFlag::Execute)), true);
}

/** @copydoc CmdSetTubeVehicleGeothermalPower */
CommandCost CmdSetTubeVehicleGeothermalPower(DoCommandFlags flags, VehicleID vehicle, TubeGeothermalSourceID source, bool reconnect)
{
	const Train *train = Train::GetIfValid(vehicle);
	if (train == nullptr) return CommandCost(STR_ERROR_TUBE_POWER_REFUSED);
	if (CommandCost owner = CheckOwnership(train->owner); owner.Failed()) return owner;
	const auto result = reconnect ? ReconnectTubeGeothermalPowerVehicle(vehicle, source, flags.Test(DoCommandFlag::Execute)) :
		BindTubeGeothermalPowerVehicle(vehicle, source, flags.Test(DoCommandFlag::Execute));
	return TubeServiceCommandResult(result.has_value() ? TubePowerError::None : TubePowerError::Refused, true);
}
