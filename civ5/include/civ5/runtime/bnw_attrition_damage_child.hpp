#pragma once
#include <cstdint>
#include <optional>
namespace civ5::runtime {
enum class BnwAttritionDamageBoundary {localized_text_owner,garrison_owner,death_query_owner};
// Copy of retained physical facts only. No public constructor/receipt can
// issue the private child or authorize a store, replay, checkpoint or effect.
struct BnwAttritionDamageProgress {
 bool issued=false,localization_entered=false,localization_returned=false;
 bool prepared=false,attempted=false,executed=false,failed=false;
 bool last_full_health_written=false,damage_assignment_invoked=false,damage_changed=false;
 std::optional<std::int32_t>unit,before_damage,requested_damage,after_damage,max_hit_points;
 std::optional<std::int32_t>before_last_full_health,after_last_full_health;
 std::uint64_t previous_source_serial=0,source_serial=0,before_revision=0,after_revision=0;
 BnwAttritionDamageBoundary boundary=BnwAttritionDamageBoundary::localized_text_owner;
};
}
