/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file source_type.h Native cargo provenance and persistent non-paying company-supply purpose.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group15; self-contained asteroid provenance, with live use gated on CAPA migration.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CB.
 */

#ifndef SOURCE_TYPE_H
#define SOURCE_TYPE_H

#include "company_type.h"
#include "cargo_type.h"
#include "industry_type.h"
#include "news_type.h"
#include "strings_type.h"
#include "town_type.h"
#include <optional>

/** Types of cargo source and destination */
enum class SourceType : uint8_t {
	Industry = 0, ///< Source/destination is an industry
	Town = 1, ///< Source/destination is a town
	Headquarters = 2, ///< Source/destination are company headquarters
	Asteroid = 3, ///< Typed body/mineral origin, or explicitly legacy 16-bit transport before migration; never an industry/town ID.
	CompanySupply = 4, ///< Bought company material; retains non-paying purpose even after the original buyer disappears.
};

using SourceID = uint16_t; ///< Contains either industry ID, town ID or company ID (or Source::Invalid)
static_assert(sizeof(SourceID) >= sizeof(CompanyID));
static_assert(sizeof(SourceID) >= sizeof(IndustryID));
static_assert(sizeof(SourceID) >= sizeof(TownID));

/** Immutable natural-resource provenance carried with the original packet after its body retires; no live resource owner is implied. */
struct AsteroidCargoOrigin {
	uint32_t body = UINT32_MAX; ///< Full issued body identity; the invalid sentinel is never a real origin.
	CargoLabel mineral{}; ///< Stable mineral meaning, independent of runtime cargo slot configuration.
	/** @param other Original provenance to compare. @return Full body/mineral equality and ordering for native merge admission. */
	auto operator<=>(const AsteroidCargoOrigin &other) const = default;
};

/** Native cargo provenance; company-supply purpose remains attached until the material is consumed. */
struct Source {
public:
	static constexpr SourceID Invalid = 0xFFFF; ///< Invalid/unknown index of source

	SourceID id = Source::Invalid; ///< Original 16-bit native/legacy identity; modern asteroid origins always normalize this to Invalid.
	SourceType type = SourceType::Industry; ///< Persistent provenance/purpose; default is a canonical unknown ordinary source.

private:
	std::optional<AsteroidCargoOrigin> asteroid_origin; ///< Value-owned typed provenance; only the factory can create it.

public:

	/** Default construction supplies canonical unknown ordinary provenance, never uninitialized identity bits. */
	constexpr Source() = default;
	/** @param id Original typed native identity. @param type Its persistent provenance/purpose meaning. */
	constexpr Source(ConvertibleThroughBase auto id, SourceType type) : id(static_cast<SourceID>(id.base())), type(type)
	{
		// Legacy constructors cannot wrap a wide natural-body ID onto another
		// body. Full-width provenance must enter through the explicit factory.
		if (type == SourceType::Asteroid && id.base() >= Source::Invalid) this->id = Source::Invalid;
	}
	/** @param id Original serialized identity. @param type Its persistent provenance/purpose meaning. */
	constexpr Source(SourceID id, SourceType type) : id(id), type(type) {}

	/** @param body Exact 32-bit body identity. @param mineral Stable nonempty mineral label; field/cargo semantics are validated separately. @return Canonical typed origin, or invalid Asteroid provenance for a sentinel/empty input. This factory never resolves or creates a body. */
	static constexpr Source FromAsteroid(uint32_t body, CargoLabel mineral)
	{
		Source source{Source::Invalid, SourceType::Asteroid};
		// Missing identity or meaning cannot become a modern packet origin. The
		// invalid result retains its natural-source domain without a fake body ID.
		if (body == UINT32_MAX || mineral.Empty()) return source;
		source.asteroid_origin = AsteroidCargoOrigin{body, mineral};
		return source;
	}
	/** @return Whether tag, legacy scalar and private payload form one unambiguous representation; this grants no live/issued-body or cargo rights. */
	constexpr bool IsCanonical() const noexcept
	{
		if (this->type > SourceType::CompanySupply) return false;
		if (!this->asteroid_origin) return true;
		return this->type == SourceType::Asteroid && this->id == Source::Invalid &&
			this->asteroid_origin->body != UINT32_MAX && !this->asteroid_origin->mineral.Empty();
	}
	/** @return A value copy of complete typed provenance, or absence for legacy/ordinary/malformed sources. Copying avoids dangling references when CargoPacket::GetSource returns a temporary. */
	constexpr std::optional<AsteroidCargoOrigin> GetAsteroidOrigin() const noexcept
	{
		return this->IsCanonical() ? this->asteroid_origin : std::nullopt;
	}

	/** @return Original company identity for headquarters or bought supplies; neither implies the company still exists. */
	constexpr CompanyID ToCompanyID() const
	{
		// Both company-backed purposes use the native company domain; interpreting an industry/body as a buyer would alias unrelated ownership.
		assert(this->type == SourceType::Headquarters || this->IsCompanySupply());
		return static_cast<CompanyID>(this->id);
	}
	/** @return Original industry identity; callers must first establish industry provenance. */
	constexpr IndustryID ToIndustryID() const { assert(this->type == SourceType::Industry); return static_cast<IndustryID>(this->id); }
	/** @return Original town identity; callers must first establish town provenance. */
	constexpr TownID ToTownID() const { assert(this->type == SourceType::Town); return static_cast<TownID>(this->id); }

	/** Explicitly invalidate identity/payload while retaining purpose. Company loss keeps bought supplies non-paying; asteroid retirement must never call this generic invalidation because its historical cargo origin remains valid. */
	constexpr void MakeInvalid() { this->id = Source::Invalid; this->asteroid_origin.reset(); }
	/** @param index Replacement ordinary native identity. Asteroid sources and malformed representations are unchanged; a generic 16-bit setter cannot rewrite typed or legacy asteroid provenance. */
	constexpr void SetIndex(SourceID index)
	{
		// Generic subsidy/town remapping is an ordinary-domain operation. It must
		// neither truncate a natural origin nor repair a contradictory source tag.
		if (this->type == SourceType::Asteroid || !this->IsCanonical()) return;
		this->id = index;
	}
	/** @param index Replacement typed ordinary identity; the same domain guard as the scalar setter applies. */
	constexpr void SetIndex(ConvertibleThroughBase auto index) { this->SetIndex(static_cast<SourceID>(index.base())); }

	/** @return Whether an original identity is retained; an invalid identity does not erase a non-paying purpose. */
	constexpr bool IsValid() const noexcept { return this->IsCanonical() && (this->asteroid_origin.has_value() || this->id != Source::Invalid); }
	/** @return Whether this material was bought as company supplies and therefore cannot earn freight income/subsidies. */
	constexpr bool IsCompanySupply() const noexcept { return this->type == SourceType::CompanySupply; }
	/** @param source Other original provenance. @return Exact identity and purpose ordering/equality for native packet merging. */
	auto operator<=>(const Source &source) const = default;

	/** @return Native news reference, or no link for sources without an industry/town destination. */
	NewsReference GetNewsReference() const;
	/** @return Native source-name format without treating a company/body identity as an industry/town. */
	StringID GetFormat() const;
};

#endif /* SOURCE_TYPE_H */
