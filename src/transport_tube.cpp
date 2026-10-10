/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube.cpp Exact first-party rail capability in the existing canonical railtype registry.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups1/4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md pinned identity
 */

#include "stdafx.h"
#include "transport_tube.h"
#include "rail.h"
#include "multilayer_content.h"

#include "safeguards.h"

/** @copydoc IsTubeRailType */
bool IsTubeRailType(RailType type)
{
	if (type >= RAILTYPE_END) return false;
	const auto &info = *GetRailTypeInfo(type);
	// Tube meaning belongs to an admitted canonical row. Broad foreign masks,
	// a familiar label or an alternate-label alias cannot create this domain.
	return info.native_tube && info.label == RAILTYPE_LABEL_TUBE;
}

/** @copydoc GetTubeRailType */
RailType GetTubeRailType()
{
	const RailType type = GetRailTypeByLabel(RAILTYPE_LABEL_TUBE, false);
	return IsTubeRailType(type) ? type : INVALID_RAILTYPE;
}

/** @copydoc InitialiseTubeRailType */
RailType InitialiseTubeRailType(const GRFConfigList &activated)
{
	const auto &profile = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	if (CheckMultiLayerContentIdentity(*profile.required_content, activated) != MultiLayerContentIdentityError::None) return INVALID_RAILTYPE;
	const RailType claimed = GetRailTypeByLabel(RAILTYPE_LABEL_TUBE, true);
	// A prior native row is idempotent. Every other main/alternate claim refuses
	// before allocation, preserving external content and saved rail identities.
	if (claimed != INVALID_RAILTYPE) return IsTubeRailType(claimed) ? claimed : INVALID_RAILTYPE;
	const RailType type = AllocateRailType(RAILTYPE_LABEL_TUBE);
	if (type == INVALID_RAILTYPE) return type;
	auto &info = *const_cast<RailTypeInfo *>(GetRailTypeInfo(type));
	info.native_tube = true;
	info.powered_railtypes = RailTypes{type};
	info.compatible_railtypes = RailTypes{type};
	info.introduces_railtypes = RailTypes{type};
	info.introduction_required_railtypes = {};
	info.introduction_date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1970}, 0, 1);
	// Existing base guideway art supports this prototype. Authoring/packing
	// later replaces its presentation; no sprite defines physical tube access.
	info.fallback_railtype = RAILTYPE_MAGLEV;
	info.flags.Set(RailTypeFlag::NoLevelCrossing);
	// Tube guideways admit straight and traversed 45-degree steps. A direct sharp
	// 90-degree step remains unavailable even when ordinary rail allows it; TBM cutting geometry has its own owner.
	info.flags.Set(RailTypeFlag::Disallow90Deg);
	info.flags.Set(RailTypeFlag::Hidden); // Construction/UI activation waits for actual admission and capsule separation.
	return type;
}
