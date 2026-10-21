#pragma once
#include "civ5/runtime/bnw_unit_support_death.hpp"
#include "civ5/content/global_defines.hpp"

namespace civ5::runtime {

enum class BnwDeathXpDomain { land,sea,other,hover };
struct BnwDeathXpUnitRule {
    std::int64_t unit_type=-1;
    std::uint32_t unit_hash=0;
    BnwDeathXpDomain domain=BnwDeathXpDomain::other;
};
struct BnwDeathXpRules {
    std::int64_t loss_percent=0;
    std::map<std::int64_t,BnwDeathXpUnitRule> units;
};

BnwDeathXpRules load_bnw_death_xp_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const BnwUnitClassDeathRules &class_rules);

struct BnwDeathXpQuote {
    BnwUnitSupportDeath support;
    std::int64_t killer=-1,unit_experience=0,loss=0;
    bool triggered=false,naval=false,setter_branch_unported=false;
    std::int64_t current_before=0,current_after=0,
                 lifetime_before=0,lifetime_after=0;
};

// Read-only Expansion2 CvUnit::kill(false) XP-loss branch. A changed current
// XP invokes CvPlayer::set*CombatExperience, which may create a Great Person
// and draw RNG. This quote never crosses that boundary.
BnwDeathXpQuote quote_bnw_unit_death_xp(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitSupportDeath &support,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules,
    const BnwDeathXpRules &xp_rules,
    std::int64_t killer);

// Only the proven no-setter-change branch may update lifetime XP on a detached
// snapshot. This is not a completed kill transition.
saves::SavedSessionSnapshot project_bnw_unit_death_xp_safe(
    const saves::SavedSessionSnapshot &source,
    const BnwDeathXpQuote &quote,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules,
    const BnwDeathXpRules &xp_rules);

} // namespace civ5::runtime
