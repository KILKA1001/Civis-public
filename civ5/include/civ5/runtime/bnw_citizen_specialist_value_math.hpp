#pragma once
#include <array>
#include <cstdint>
#include "civ5/saves/team_state.hpp"
namespace civ5::runtime::detail {
// Supplied checked arithmetic only: no source, count, allocation or writer grant.
std::int32_t bnw_specialist_focus_yield(std::int32_t)noexcept;
std::int32_t bnw_specialist_weighted_yield(std::int32_t weight,std::int32_t yield);
std::int32_t bnw_specialist_total(std::array<std::int32_t,6> focused,
 bool avoid,std::int32_t focus,std::int32_t population,std::int32_t gp,std::int32_t happiness);
struct BnwSpecialistBuildingValue {std::int32_t adjusted=0,unmodified=0;};
BnwSpecialistBuildingValue bnw_specialist_building_value(std::int32_t allowed,std::int32_t value);
// Supplied logical hash projection only. A writer must separately prove the
// genuine native Info hash and its current Reset/Read count before using it.
saves::TeamHashedArray bnw_specialist_known_hash_projection_math(
 const saves::TeamHashedArray&,std::uint32_t hash,std::int32_t next);
}
