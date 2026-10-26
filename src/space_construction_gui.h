/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_construction_gui.h Shared read-only construction wording for native views.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group8.
 */
#ifndef SPACE_CONSTRUCTION_GUI_H
#define SPACE_CONSTRUCTION_GUI_H
#include <cstdint>
#include <string>
#include <vector>

struct SpaceVehicle;
struct SpaceStructure;
enum class OrbitalConstructionBlock : uint8_t;

/** @param reason Original construction owner's read-only prerequisite. @return Localized status and player remedy. */
std::string GetSpaceConstructionReasonText(OrbitalConstructionBlock reason);
/** @param builder Actual selected native builder. @return Localized actual instruction/target and readiness; queries grant no work. */
std::vector<std::string> GetSpaceConstructionBuilderText(const SpaceVehicle &builder);
/** @param site Actual selected job. @param builder Optional actual carrier; its hold is separate from site inputs. @return Required, held, missing, consumed and carrier quantities from original owners. */
std::vector<std::string> GetSpaceConstructionMaterialText(const SpaceStructure &site, const SpaceVehicle *builder);
#endif /* SPACE_CONSTRUCTION_GUI_H */
