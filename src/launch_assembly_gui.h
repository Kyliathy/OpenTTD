/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_assembly_gui.h Exact native craft context for the existing paid workshop window.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JDB
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md task16.2
 */
#ifndef LAUNCH_ASSEMBLY_GUI_H
#define LAUNCH_ASSEMBLY_GUI_H

/** Actual generation-qualified native hull identity, without a second selection registry. */
struct SpaceCraftRef;
/** Existing generation-qualified native workshop incarnation. */
struct LaunchCenterRef;
/** Existing generation-qualified passive shell identity, separate from native Vehicles. */
struct SpacePodRef;

/** Open the exact original home workshop and select its surviving paid hull.
 * @param craft Original native incarnation selected through a map/list action.
 * @return Whether its actual supported non-orbital hull and exact owned home supplied the native window.
 * Only transient GUI selection changes. Stale/lost homes never pick a replacement facility or craft.
 */
bool ShowLaunchCraftWindow(SpaceCraftRef craft);

/** Read the actual live native selection from one surviving workshop window.
 * @param center Exact window/facility incarnation; an old station ID cannot select a replacement window.
 * @return Current resolved native hull, or an empty reference for closed/stale windows or retired hulls.
 */
SpaceCraftRef GetLaunchCraftWindowSelection(LaunchCenterRef center);

/** Read one native workshop's actual surviving passive-shell selection.
 * @param center Exact original facility/window incarnation.
 * @return Current resolved paid shell, or an empty reference after closure, retirement or atomic recovery.
 */
SpacePodRef GetLaunchPodWindowSelection(LaunchCenterRef center);

#endif /* LAUNCH_ASSEMBLY_GUI_H */
