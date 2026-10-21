#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/vp_city_espionage.hpp"
#include <optional>
#include <vector>

namespace civ5::runtime {
struct VpSpyPassiveBonusRule {
    std::int64_t id=0,threshold=0,science_percent=0,vision_tiles=0;
    bool reveal_city_screen=false;
};
struct VpSpyMissionThreshold {
    std::int64_t id=0,threshold=0;
};
struct VpNetworkPointRules {
    bool selected_rules_loaded=false;
    std::int64_t train_percent=100;
    std::vector<VpSpyPassiveBonusRule> passive;
    std::vector<VpSpyMissionThreshold> missions;
};
// These inputs and the rate formula belong only to the selected VP GameCore.
// A missing optional input is unresolved, never an implicit zero from a save.
struct VpCounterspyNetworkReduction {
    std::int64_t focus_reduction=0, effective_rank=0;
};
struct VpNetworkPointRateInputs {
    bool balance_vp=false, city_present=false;
    std::int64_t spy_state=0;
    // No rank is valid for VP's theoretical city-selection query (spy index -1).
    std::optional<std::int64_t> effective_spy_rank;
    std::optional<std::int64_t> influence_level, target_known_techs, own_known_techs;
    std::optional<std::int64_t> player_bonus, religion_bonus;
    std::optional<std::int64_t> policy_faster_modifier, trait_strength_modifier;
    // Only gathering intel consults these city-specific outputs. A resolved
    // counterspy lookup may legitimately contain no counterspy.
    bool counterspy_resolved=false;
    std::optional<VpCounterspyNetworkReduction> counterspy;
    std::optional<std::int64_t> city_security;
};
struct VpNetworkPointRateRules {
    bool selected_rules_loaded=false;
    std::int64_t base=0, per_spy_rank=0, per_influence_level=0;
    std::int64_t per_technology_behind=0, max_technologies_behind=0;
    std::int64_t reduction_per_security_point=0;
};
struct VpNetworkPointRateQuote {
    std::int64_t before_security=0, security_reduction_percent=0, rate=0;
};
VpNetworkPointRateRules load_vp_network_point_rate_rules(const content::GlobalDefineCache &defines);
// VP Release-5.4.2 CvPlayerEspionage::CalcNetworkPointsPerTurn. Caller resolves
// saved/profile state, policy, religion and city effects before invoking it.
VpNetworkPointRateQuote calculate_vp_network_point_rate(const VpNetworkPointRateInputs &inputs,
    const VpNetworkPointRateRules &rules);
// Selected SQL rules, including XML/SQL mod rows. The source's optional
// NetworkPointsScaling uses GameSpeeds.TrainPercent for both table families.
VpNetworkPointRules load_vp_network_point_rules(const content::RulesDatabase &database,
    const ResearchContext &world);

struct VpNetworkPointPreview {
    std::int64_t before=0,after=0,max_before=0,max_after=0,rate=0;
    std::optional<std::int64_t> next_passive_bonus;
    std::vector<std::int64_t> passive_bonuses_unlocked,missions_unlocked;
    bool safe_without_effect_host=false;
};
VpNetworkPointPreview preview_vp_network_points(const VpCityEspionageState &city,
    std::int64_t player,std::int64_t rate,const VpNetworkPointRules &rules);
// Advances the source's rate/last-progress/amount/max/next-threshold fields
// only when no passive bonus, mission notification or sight effect can fire.
// The caller must still apply XP, wonders and later spy operations separately.
VpNetworkPointPreview apply_vp_network_points_without_effect_host(VpCityEspionageState &city,
    std::int64_t player,std::int64_t rate,const VpNetworkPointRules &rules);

struct VpDiplomatNetworkPointRules {
    bool selected_rules_loaded=false;
    std::vector<VpSpyMissionThreshold> passive;
};
struct VpDiplomatNetworkPointPreview {
    std::int64_t before=0,after=0,max_before=0,max_after=0,rate=0;
    std::optional<std::int64_t> next_passive_bonus;
    std::vector<std::int64_t> passive_bonuses_unlocked;
    bool safe_without_effect_host=false;
};
// Diplomat bonuses use a different SQL table and compare the previous Amount
// with the new MaxAmount. They never unlock offensive espionage missions.
VpDiplomatNetworkPointRules load_vp_diplomat_network_point_rules(
    const content::RulesDatabase &database,const ResearchContext &world);
VpDiplomatNetworkPointPreview preview_vp_diplomat_network_points(
    const VpCityEspionageState &city,std::int64_t player,std::int64_t rate,
    const VpDiplomatNetworkPointRules &rules);
VpDiplomatNetworkPointPreview apply_vp_diplomat_network_points_without_effect_host(
    VpCityEspionageState &city,std::int64_t player,std::int64_t rate,
    const VpDiplomatNetworkPointRules &rules);
} // namespace civ5::runtime
