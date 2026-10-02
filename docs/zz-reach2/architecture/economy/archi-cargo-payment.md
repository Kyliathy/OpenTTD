# Cargo payment

This is the arithmetic behind a delivery. The stop, the ledger, and the subsidy lifecycle are in [archi-economy.md](archi-economy.md). Link-graph routing decides keep, transfer, or deliver before any of the functions below run. This file prices the decision those systems already made.

`CargoSpec::current_payment` is the inflated rate. `RecomputePrices` sets it from the cargo's `initial_payment` and `_economy.inflation_payment`:

```790:792:src/economy.cpp
	for (CargoSpec *cs : CargoSpec::Iterate()) {
		cs->current_payment = (cs->initial_payment * (int64_t)_economy.inflation_payment) >> 16;
	}
```

`initial_payment` and `transit_periods[2]` are cargo data. Defaults are the `ip`, `td1`, and `td2` arguments of `MK` in `src/table/cargo_const.h`. A NewGRF may replace `initial_payment` from action 0 (`src/newgrf/newgrf_act0_cargo.cpp`).

## When the amount is computed

| Moment | Function | What is stored |
| --- | --- | --- |
| Arrival, inside `VehicleCargoList::Stage` | `CargoPayment::PayTransfer` | Feeder credit on the packet and on `visual_transfer`. Company cash is unchanged |
| Each unload slice of a deliver-marked packet | `CargoPayment::PayFinalDelivery` → `DeliverGoods` | Added to `route_profit` and, minus feeder share, to `visual_profit` |
| `Vehicle::LeaveStation` deletes the `CargoPayment` | `CargoPayment::~CargoPayment` | `SubtractMoneyFromCompany` of `-route_profit` |

`Stage` pays the transfer for the whole packet before any of it is moved:

```531:537:src/cargopacket.cpp
			case MoveToAction::Transfer:
				this->packets.push_front(cp);
				/* Add feeder share here to allow reusing field for next station. */
				share = payment->PayTransfer(cargo, cp, cp->count, current_tile);
				cp->AddFeederShare(share);
				this->feeder_share += share;
				cp->next_hop = cargo_next;
```

`CargoDelivery` prices the slice that is actually leaving the vehicle:

```113:118:src/cargoaction.cpp
bool CargoDelivery::operator()(CargoPacket *cp)
{
	uint remove = this->Preprocess(cp);
	this->source->RemoveFromMeta(cp, VehicleCargoList::MoveToAction::Deliver, remove);
	this->payment->PayFinalDelivery(this->cargo, cp, remove, this->current_tile);
	return this->Postprocess(cp, remove);
}
```

`PayFinalDelivery` passes `cp->GetDistance(current_tile)` and `cp->GetPeriodsInTransit()` into `DeliverGoods` (`src/economy.cpp` 1216–1223). `GetFeederShare(count)` prorates the packet's stored feeder share by `count / packet count`.

## Distance

`GetDistance` is called while the packet is still on the vehicle. It finishes the travelled vector against the current tile locally and does not write the packet:

```213:246:src/cargopacket.h
	inline uint GetDistance(TileIndex current_tile) const
	{
		assert(this->source_xy != INVALID_TILE);
#ifdef WITH_ASSERT
		assert(this->in_vehicle);
#endif /* WITH_ASSERT */

		/* Distance is always requested when the cargo is still inside the
		 * vehicle. So first finish the calculation for travelled to
		 * become a vector. */
		auto local_travelled = travelled;
		local_travelled.x -= TileX(current_tile);
		local_travelled.y -= TileY(current_tile);

		/* Cargo-movement is a vector that indicates how much the cargo has
		 * actually traveled in a vehicle. This is the distance you get paid
		 * for. However, one could construct a route where this vector would
		 * be really long. To not overpay the player, cap out at the distance
		 * between source and destination.
		 *
		 * This way of calculating is to counter people moving cargo for free
		 * and instantly in stations, where you deliver it in one part of the
		 * station and pick it up in another. By using the actual distance
		 * traveled in a vehicle, using this trick doesn't give you more money.
		 *
		 * However, especially in large networks with large transfer station,
		 * etc, one could actually make the route a lot longer. In that case,
		 * use the actual distance between source and destination.
		 */

		uint distance_travelled = abs(local_travelled.x) + abs(local_travelled.y);
		uint distance_source_dest = DistanceManhattan(this->source_xy, current_tile);
		return std::min(distance_travelled, distance_source_dest);
	}
```

`travelled` gains `TileX`/`TileY` of the load tile in `UpdateLoadingTile` and loses the unload tile in `UpdateUnloadingTile` (`src/cargopacket.h` 104–136). The first load also sets `source_xy`. The paid distance is the smaller of the in-vehicle Manhattan vector and the Manhattan distance from the first pickup tile to the tile where payment is computed. The comment above the `min` states why both exist: the vehicle vector ignores cargo that was walked across a station without a trip, and the source-to-destination cap limits a deliberately long route.

Transfer uses the same `GetDistance` at stage time, on the vehicle's cargo tile. Final delivery uses it again at unload time, on that slice's tile.

## Cargo age

`periods_in_transit` counts aging periods, not economy days. `VehicleCargoList::AgeCargo` increments every packet that is below `UINT16_MAX` and adds `count` to the list's cached period-sum (`src/cargopacket.cpp` 406–414).

The tick loop decrements `cargo_age_counter` and calls `AgeCargo` when it hits 0, then reloads the counter from `cached_cargo_age_period` (`src/vehicle.cpp` 1021–1026). That cached period is the engine property (`PROP_TRAIN_CARGO_AGE_PERIOD` and the road, ship, and aircraft equivalents), falling back to `Engine::info.cargo_age_period`. A new engine sets `cargo_age_period` to `Ticks::CARGO_AGING_TICKS`:

```80:81:src/timer/timer_game_tick.h
	static constexpr TimerGameTick::Ticks CARGO_AGING_TICKS = 185; ///< Cycle duration for aging cargo.
```

`CargoPacket::GetPeriodsInTransit` documents the default as 2.5 days because `CARGO_AGING_TICKS` is 185 and `Ticks::DAY_TICKS` is 74 (`src/cargopacket.h` 177–186). A vehicle NewGRF replaces the period through the cargo-age property. Station cargo does not age in this function; `AgeCargo` runs on the vehicle list.

Inside the income function the stored periods are scaled before either the callback or the built-in time factor sees them:

```963:964:src/economy.cpp
	/* Scale transit periods according to the game setting. We also pass this scaled value to the NewGRF callback. */
	transit_periods = ScaleByPercentage<uint16_t, uint32_t>(transit_periods, _settings_game.economy.cargo_aging_rate);
```

```377:389:src/core/math_func.hpp
template <typename T, typename U>
constexpr T ScaleByPercentage(U num, uint16_t percentage)
{
	U scaled;
	/* We might not need to do anything. */
	if (percentage == 100) {
		scaled = num;
	} else {
		scaled = (num * static_cast<U>(percentage)) / 100;
	}

	/* Make sure the value fits resulting type T. */
	return ClampTo<T>(scaled);
}
```

## GetTransportedGoodsIncome

```955:1017:src/economy.cpp
Money GetTransportedGoodsIncome(uint num_pieces, uint dist, uint16_t transit_periods, CargoType cargo_type)
{
	const CargoSpec *cs = CargoSpec::Get(cargo_type);
	if (!cs->IsValid()) {
		/* User changed newgrfs and some vehicle still carries some cargo which is no longer available. */
		return 0;
	}

	/* Scale transit periods according to the game setting. We also pass this scaled value to the NewGRF callback. */
	transit_periods = ScaleByPercentage<uint16_t, uint32_t>(transit_periods, _settings_game.economy.cargo_aging_rate);

	/* Use callback to calculate cargo profit, if available */
	if (cs->callback_mask.Test(CargoCallbackMask::ProfitCalc)) {
		uint32_t var18 = ClampTo<uint16_t>(dist) | (ClampTo<uint8_t>(num_pieces) << 16) | (ClampTo<uint8_t>(transit_periods) << 24);
		uint16_t callback = GetCargoCallback(CBID_CARGO_PROFIT_CALC, 0, var18, cs);
		if (callback != CALLBACK_FAILED) {
			int result = GB(callback, 0, 14);

			/* Simulate a 15 bit signed value */
			if (HasBit(callback, 14)) result -= 0x4000;

			/* "The result should be a signed multiplier that gets multiplied
			 * by the amount of cargo moved and the price factor, then gets
			 * divided by 8192." */
			return result * num_pieces * cs->current_payment / 8192;
		}
	}

	static const int MIN_TIME_FACTOR = 31;
	static const int MAX_TIME_FACTOR = 255;
	static const int TIME_FACTOR_FRAC_BITS = 4;
	static const int TIME_FACTOR_FRAC = 1 << TIME_FACTOR_FRAC_BITS;

	const int periods1 = cs->transit_periods[0];
	const int periods2 = cs->transit_periods[1];
	const int periods_over_periods1 = std::max(transit_periods - periods1, 0);
	const int periods_over_periods2 = std::max(periods_over_periods1 - periods2, 0);
	int periods_over_max = MIN_TIME_FACTOR - MAX_TIME_FACTOR;
	if (periods2 > -periods_over_max) {
		periods_over_max += transit_periods - periods1;
	} else {
		periods_over_max += 2 * (transit_periods - periods1) - periods2;
	}

	/*
	 * The time factor is calculated based on the time it took
	 * (transit_periods) compared two cargo-depending values. The
	 * range is divided into four parts:
	 *
	 *  - constant for fast transits
	 *  - linear decreasing with time with a slope of -1 for medium transports
	 *  - linear decreasing with time with a slope of -2 for slow transports
	 *  - after hitting MIN_TIME_FACTOR, the time factor will be asymptotically decreased to a limit of 1 with a scaled 1/(x+1) function.
	 *
	 */
	if (periods_over_max > 0) {
		const int time_factor = std::max(2 * MIN_TIME_FACTOR * TIME_FACTOR_FRAC * TIME_FACTOR_FRAC / (periods_over_max + 2 * TIME_FACTOR_FRAC), 1); // MIN_TIME_FACTOR / (x/(2 * TIME_FACTOR_FRAC) + 1) + 1, expressed as fixed point with TIME_FACTOR_FRAC_BITS.
		return BigMulS(dist * time_factor * num_pieces, cs->current_payment, 21 + TIME_FACTOR_FRAC_BITS);
	} else {
		const int time_factor = std::max(MAX_TIME_FACTOR - periods_over_periods1 - periods_over_periods2, MIN_TIME_FACTOR);
		return BigMulS(dist * time_factor * num_pieces, cs->current_payment, 21);
	}
}
```

An invalid cargo spec returns 0. That is the path for a vehicle still carrying cargo from a NewGRF that is no longer loaded.

When `CargoCallbackMask::ProfitCalc` is set and `GetCargoCallback(CBID_CARGO_PROFIT_CALC, ...)` does not return `CALLBACK_FAILED`, the callback result replaces the time factor. `var18` packs distance in the low 16 bits (clamped to `uint16_t`), piece count in the next 8 (clamped to `uint8_t`), and the already-scaled transit periods in the top 8 (clamped to `uint8_t`). Bits 0–13 of the callback are the magnitude. Bit 14 sets the sign by subtracting `0x4000`. The return is the comment's formula: that signed multiplier times `num_pieces` times `current_payment`, divided by 8192. A failed callback falls through to the built-in time factor.

The comment above the `periods_over_max` test describes the four regions of the built-in factor, using the scaled `transit_periods` against the cargo's two thresholds. `periods1` is `transit_periods[0]`, `periods2` is `transit_periods[1]`. `periods_over_periods1` is how far the scaled age is past the first threshold, or 0. `periods_over_periods2` is how far that excess is past the second threshold, or 0. The fast region is the `else` branch: `time_factor = max(MAX_TIME_FACTOR - periods_over_periods1 - periods_over_periods2, MIN_TIME_FACTOR)`, which is 255 while age is still inside the first threshold (both excesses are 0), then falls by 1 per period through the second threshold and by 2 per period after it, and stops at 31. The slow region is the `periods_over_max > 0` branch. Its comment on the division says the fixed-point value is `MIN_TIME_FACTOR / (x/(2 * TIME_FACTOR_FRAC) + 1) + 1` with `TIME_FACTOR_FRAC_BITS` (4) fractional bits, and the code takes at least 1.

Both built-in returns go through `BigMulS`:

```84:87:src/economy.cpp
static inline int32_t BigMulS(const int32_t a, const int32_t b, const uint8_t shift)
{
	return (int32_t)((int64_t)a * (int64_t)b >> shift);
}
```

The first argument is `dist * time_factor * num_pieces`. The second is `cs->current_payment`. The shift is 21 in the fast/medium branch and `21 + TIME_FACTOR_FRAC_BITS` (25) in the asymptotic branch, where `time_factor` still carries 4 fractional bits. The product is computed in `int64_t`, shifted, and returned as `int32_t`. The function parameters are `int32_t`, so the call narrows both arguments to 32 bits before that multiply.

`DeliverGoods` calls this with `accepted_total`, not with the raw unloaded count (`src/economy.cpp` 1116). A zero accepted count multiplies through to a zero income. After that return, `DeliverGoods` may apply the subsidy multiplier documented in [archi-economy.md](archi-economy.md). `PayTransfer` calls `GetTransportedGoodsIncome` directly and does not apply that multiplier.

## Feeder share

```1234:1247:src/economy.cpp
Money CargoPayment::PayTransfer(CargoType cargo, const CargoPacket *cp, uint count, TileIndex current_tile)
{
	/* Pay transfer vehicle the difference between the payment for the journey from
	 * the source to the current point, and the sum of the previous transfer payments */
	Money profit = -cp->GetFeederShare(count) + GetTransportedGoodsIncome(
			count,
			cp->GetDistance(current_tile),
			cp->GetPeriodsInTransit(),
			cargo);

	profit = profit * _settings_game.economy.feeder_payment_share / 100;

	this->visual_transfer += profit; // accumulate transfer profits for whole vehicle
	return profit; // account for the (virtual) profit already made for the cargo packet
}
```

The virtual profit is the income for the distance and age so far, minus the feeder share already stored on the packet, then multiplied by `economy.feeder_payment_share` and divided by 100. The setting is an integer percentage, default 75, range 0–100 (`src/table/settings/economy_settings.ini`). `GetFeederShare(part)` is `feeder_share * part / count` (`src/cargopacket.h` 172–175).

`PayFinalDelivery` keeps the full `DeliverGoods` profit in `route_profit` and shows `profit - GetFeederShare(count)` as `visual_profit` (`src/economy.cpp` 1216–1223). The destructor credits `route_profit` to the company and adds `(visual_profit + visual_transfer) << 8` to `profit_this_year` (`src/economy.cpp` 1192–1193). A pure transfer therefore updates the vehicle profit statistic and the packet's `feeder_share`, and leaves company cash unchanged because `route_profit` is still 0 and `SubtractMoneyFromCompany` returns on a zero cost.
