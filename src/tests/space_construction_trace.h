/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_construction_trace.h Focused native construction-boundary observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups4-7.
 */
#ifndef TESTS_SPACE_CONSTRUCTION_TRACE_H
#define TESTS_SPACE_CONSTRUCTION_TRACE_H

#include <string_view>
#include <cstdint>
#include <functional>

struct SpaceVehicle;
struct SpaceStructure;
enum class SpaceOrderAction : uint8_t;
enum class OrbitalConstructionBlock : uint8_t;

/** @param craft Actual builder. @param expected Original owner's explanation. @param unpaused Inspect running prerequisites rather than the actual pause state. */
void CheckConstructionReadiness(const SpaceVehicle &craft, OrbitalConstructionBlock expected, bool unpaused = true);
/** @param ticks Actual simulation opportunities. @param craft Observed original builder. @param site Original job. @param report Emit the final boundary. */
void AdvanceConstructionTrace(uint ticks, const SpaceVehicle &craft, const SpaceStructure &site, bool report = true);
/** @param craft Original builder. @param site Original job. @param reached Read-only endpoint. @param phase Logged endpoint name. */
void AwaitConstructionBoundary(const SpaceVehicle &craft, const SpaceStructure &site, const std::function<bool()> &reached, std::string_view phase);
/** @param craft Real command recipient. @param target Exact paid site. @param action Explicit Visit or Construct instruction. */
void OrderConstructionCraft(SpaceVehicle &craft, const SpaceStructure &target, SpaceOrderAction action);
/** @param builder Keep this selected builder; send other paid constructors on actual Visits to free the Core berth. */
void MoveOtherConstructionCraftAway(const SpaceVehicle &builder);
/** @param scenario Checkpoint/reload stage, or the shared-worker continuation case; all mutations use native commands/ticks. */
void RunSpaceConstructionReplay(std::string_view scenario);
/** Exercise two real supplied builders on distinct adjacent jobs sharing one original Core construction slot. */
void RunSpaceConstructionWorkers();
/** @param builder Exact selected hull. @param site Exact selected job/frame. @param phase Repository-local capture label. Verify native view bounds and presentation purity. */
void CaptureConstructionViews(const SpaceVehicle &builder, const SpaceStructure &site, std::string_view phase);

/**
 * Trace actual queue clicks and normal construction opportunities in a loaded demo.
 * @param scenario "waiting"/"working" characterize materials; "queue" checks edits on supplied work;
 * "service" checks actual service custody; "removed" checks a genuinely removed/replaced empty blueprint.
 * "delivery" checks actual final-input timing; "interruptions" checks retained fractional work and readiness.
 * This test grants no materials, work, hulls or utility energy outside native owners.
 */
void RunSpaceConstructionTrace(std::string_view scenario);

#endif /* TESTS_SPACE_CONSTRUCTION_TRACE_H */
