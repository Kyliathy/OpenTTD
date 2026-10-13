/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_orders.h Native Space order ownership; Plan8 Group6. Ordinary OrderList remains separate. */
#ifndef SPACE_ORDERS_H
#define SPACE_ORDERS_H
#include "space.h"
#include "vehicle_type.h"
#include <vector>
#include <span>

struct SpaceVehicle;
inline constexpr size_t MAX_SPACE_ORDERS = 32;
enum class SpaceOrderAction : uint8_t { Visit, Survey, Mine, Construct, Unload, ReturnService, End };
enum class SpaceOrderTargetKind : uint8_t { Body, Structure, Blueprint, LaunchCenter, Craft, End };
enum class SpaceOrderBlock : uint8_t { None, Completed, TargetRemoved, ProducerUnavailable, HoldIncompatible, Timeout, ReturnsPending, ServiceRequired, PodsMissing, End };
enum class SpaceOrderEdit : uint8_t { Insert, Replace, Erase, Clear, MoveUp, MoveDown, End };
struct SpaceOrderTarget {
	SpaceOrderTargetKind kind = SpaceOrderTargetKind::Body;
	uint32_t id = UINT32_MAX;
	uint64_t generation = 0; ///< Bodies are monotonic and use zero; reusable pool identities require nonzero generations.
	bool operator==(const SpaceOrderTarget &) const = default;
};
struct SpaceOrder {
	SpaceOrderAction action = SpaceOrderAction::Visit;
	SpaceOrderTarget target;
	bool operator==(const SpaceOrder &) const = default;
};
/** Owned directly by one native Vehicle. No shared-order or clone commissioning path. */
struct SpaceOrderQueue {
	std::vector<SpaceOrder> entries;
	uint8_t cursor = 0;
	bool started = false;
	SpaceOrderBlock blocked = SpaceOrderBlock::None;
	uint32_t timer_ticks = 0;
	uint16_t work_ticks = 0; ///< Reserved for action-owned partial work; unavailable producers may not accrue it.
	bool operator==(const SpaceOrderQueue &) const = default;
};
struct SpaceOrderRecord {
	VehicleID vehicle = VehicleID::Invalid();
	uint64_t generation = 0;
	SpaceOrderQueue queue;
};
bool IsSpaceOrderAllowed(uint16_t model, const SpaceOrder &order);
bool ValidateSpaceOrderRecords(std::span<const SpaceOrderRecord> records, uint64_t next_generation);
std::vector<SpaceOrderRecord> CaptureSpaceOrders();
bool InstallLoadedSpaceOrders(std::span<const SpaceOrderRecord> records, uint64_t next_generation);
bool ValidateNativeSpaceOrders();
bool RegisterSpaceVehicleIdentity(SpaceVehicle &vehicle);
uint64_t GetNextSpaceVehicleGeneration();
uint64_t ComputeSpaceOrdersDigest();
void ResetSpaceOrders();
void ResetSpaceOrderLoadStaging();
bool AcceptLoadedSpaceOrders();
void ValidateSpaceOrderLoadPreview();
/** True when the queue handled this tick; false delegates the current Mine to the one existing mining controller. */
bool TickSpaceOrders(SpaceVehicle &vehicle);
void CancelSpaceOrders(SpaceVehicle &vehicle);
/** Derived working-tick predicate, shared by survey execution and native upkeep. */
bool IsSpaceSurveyWorkTick(const SpaceVehicle &vehicle);
/** Shared current-position resolver for orders and rocket delivery; no alternate target registry. */
SpaceOrderBlock ResolveSpaceNavigationTarget(const SpaceVehicle &vehicle, const SpaceOrderTarget &target, SpacePoint &point);
/** Native paid mining budget; legacy unpaid allowances are never fabricated. */
SpaceOrderBlock GetSpaceMiningBlock(const SpaceVehicle &vehicle);
bool IsSpaceMiningServiceWait(const SpaceOrderQueue &queue);
bool IsSpaceMiningWorkTick(const SpaceVehicle &vehicle);
#endif
