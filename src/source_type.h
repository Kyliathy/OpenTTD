/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file source_type.h Native cargo provenance and persistent non-paying company-supply purpose.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CB.
 */

#ifndef SOURCE_TYPE_H
#define SOURCE_TYPE_H

#include "company_type.h"
#include "industry_type.h"
#include "news_type.h"
#include "strings_type.h"
#include "town_type.h"

/** Types of cargo source and destination */
enum class SourceType : uint8_t {
	Industry = 0, ///< Source/destination is an industry
	Town = 1, ///< Source/destination is a town
	Headquarters = 2, ///< Source/destination are company headquarters
	Asteroid = 3, ///< Stable bounded orbital body ID; never an industry/town ID.
	CompanySupply = 4, ///< Bought company material; retains non-paying purpose even after the original buyer disappears.
};

using SourceID = uint16_t; ///< Contains either industry ID, town ID or company ID (or Source::Invalid)
static_assert(sizeof(SourceID) >= sizeof(CompanyID));
static_assert(sizeof(SourceID) >= sizeof(IndustryID));
static_assert(sizeof(SourceID) >= sizeof(TownID));

/** Native cargo provenance; company-supply purpose remains attached until the material is consumed. */
struct Source {
public:
	static constexpr SourceID Invalid = 0xFFFF; ///< Invalid/unknown index of source

	SourceID id; ///< Original industry/town/company/body identity, Source::Invalid if unknown/retired.
	SourceType type; ///< Type of \c source_id.

	/** Default construction leaves initialization policy with the containing native owner. */
	Source() = default;
	/** @param id Original typed native identity. @param type Its persistent provenance/purpose meaning. */
	Source(ConvertibleThroughBase auto id, SourceType type) : id(id.base()), type(type) {}
	/** @param id Original serialized identity. @param type Its persistent provenance/purpose meaning. */
	Source(SourceID id, SourceType type) : id(id), type(type) {}

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

	/** Retire only the original identity; loss of a buyer must never turn bought supplies into paying freight. */
	constexpr void MakeInvalid() { this->id = Source::Invalid; }
	/** @param index Replacement original identity, preserving its provenance/purpose meaning. */
	constexpr void SetIndex(SourceID index) { this->id = index; }
	/** @param index Replacement typed original identity, preserving its provenance/purpose meaning. */
	constexpr void SetIndex(ConvertibleThroughBase auto index) { this->id = index.base(); }

	/** @return Whether an original identity is retained; an invalid identity does not erase a non-paying purpose. */
	constexpr bool IsValid() const noexcept { return this->id != Source::Invalid; }
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
