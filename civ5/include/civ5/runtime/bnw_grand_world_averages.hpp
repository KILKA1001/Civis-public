#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
#include "civ5/runtime/bnw_happiness_buildings.hpp"
#include "civ5/runtime/bnw_founder_religion_native.hpp"
#include "civ5/runtime/bnw_happiness_update_suffix.hpp"
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwCityCurrentBaseTourismUnavailable final:public std::runtime_error {public:using std::runtime_error::runtime_error;};
// Genuine per-city current read only. No all-empire query, cached freshness,
// culture strategy completion, activation, effects or continuation authority.
class BnwCityCurrentBaseTourism final {
 public:std::int32_t value()const;
 private:struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwCityCurrentBaseTourism(std::shared_ptr<Impl>)noexcept;void require_current()const;
 friend BnwCityCurrentBaseTourism inspect_bnw_city_current_base_tourism(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
BnwCityCurrentBaseTourism inspect_bnw_city_current_base_tourism(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
class BnwGrandWorldAverageRules {
 public:
    struct Building {std::int32_t enhanced_tourism=0,enhanced_tech=-1;bool theming_possible=false;};
    static BnwGrandWorldAverageRules load(const content::RulesDatabase &);
    const auto &identity()const noexcept{return buildings_.rules_sha256();}
    const auto &buildings()const noexcept{return buildings_;}
    const auto &entries()const noexcept{return entries_;}
    const auto &policy_tourism()const noexcept{return policy_;}
    const auto &types()const noexcept{return types_;}
    const auto &technologies()const noexcept{return tech_;}
    std::int32_t base_tourism()const noexcept{return base_;}
    std::int32_t temporary_boost()const noexcept{return temporary_;}
 private:
    BnwGrandWorldAverageRules()=default;
    BnwBuildingHappinessRules buildings_;
    content::InfoTypeCatalog types_;
    content::TechnologyRules tech_;
    std::map<std::int64_t,Building> entries_;
    std::map<std::int64_t,YieldValues> policy_;
    std::int32_t base_=0,temporary_=0;
};
struct BnwGrandCityTourism {
    std::int64_t city=-1;
    bool base_suppressed=false;
    std::int32_t great_works=0,base_before_modifiers=0,modifier=0,tourism=0;
    std::int32_t theming_classes_visited=0,enhanced_classes_visited=0,policy_classes_visited=0;
    std::int32_t majority=-1;
    bool get_religion_null=false,league_query_executed=false;
};
struct BnwGrandPlayerCultureTourism {
    std::int64_t player=-1;
    std::int32_t lifetime_culture=0,tourism=0;
    std::vector<BnwGrandCityTourism> cities;
};
struct BnwGrandPlayerTourismQuery {
    bool calculated=false;
    std::int64_t player=-1;
    std::vector<BnwGrandCityTourism> cities;
    std::optional<std::int32_t> total;
    std::string blocker;
};
// Actual CvPlayerCulture::GetTourism only. Reuses the SAME current native
// city/FLTA producer as world averages; no foreign culture/tech loop or RNG.
BnwGrandPlayerTourismQuery quote_bnw_player_tourism(const LiveSessionState &,
 const BnwGrandWorldAverageRules &,const SessionCheckpointIdentity &,std::int64_t);
class BnwCultureInfluenceRules {
 public:
    static BnwCultureInfluenceRules load(const content::RulesDatabase &);
    const auto &identity()const noexcept{return identity_;}
    // Native comparison order: dominant, influential, popular, familiar, exotic.
    const auto &thresholds()const noexcept{return thresholds_;}
 private:
    BnwCultureInfluenceRules()=default;
    std::string identity_;
    std::array<std::int32_t,5> thresholds_{};
};
struct BnwCultureInfluenceLevel {
    bool calculated=false,foreign_met=false;
    std::int64_t player=-1,foreign=-1;
    std::optional<std::int32_t> influence,lifetime_culture,percent,level;
    std::string blocker;
};
struct BnwCultureInfluentialCount {
    bool calculated=false;
    std::int64_t player=-1;
    std::size_t slots_visited=0;
    std::vector<BnwCultureInfluenceLevel> foreigners;
    std::optional<std::int32_t> count;
    std::string blocker;
};
// Major target query: foreignTeam.hasMet[ownTeam] BEFORE the own influence22
// and foreign lifetime getters. The met test is directional and not inferred.
BnwCultureInfluenceLevel quote_bnw_culture_influence_level(const LiveSessionState &,
 const BnwCultureInfluenceRules &,const SessionCheckpointIdentity &,
 std::int64_t player,std::int64_t foreign);
// Actual 22-slot GetNumCivsInfluentialOn: self/alive/nonminor filtering is
// separate from each qualifying target's native foreign-to-own met query.
BnwCultureInfluentialCount quote_bnw_num_civs_influential_on(const LiveSessionState &,
 const BnwCultureInfluenceRules &,const SessionCheckpointIdentity &,std::int64_t);
struct BnwGrandTeamKnownTechs {std::int64_t team=-1;std::int32_t known=0;};
struct BnwGrandWorldAverages {
    bool calculated=false;
    std::vector<BnwGrandPlayerCultureTourism> players;
    std::vector<BnwGrandTeamKnownTechs> teams;
    // No values are supplied for an incomplete native source query.
    std::optional<std::int32_t> culture_average,tourism_average,tech_average;
    std::string blocker;
};
// Read-only actual CvGrandStrategyAI::DoGuess 872..909. Native source order:
// all-alive-major lifetime culture / full city tourism, divide, alive-major-team
// HasTech count, divide. No met filter, no player/Grand completion receipt.
// Unsupported theming/landmark/religious/active league queries stay explicit.
BnwGrandWorldAverages quote_bnw_grand_world_averages(const LiveSessionState &,
 const BnwGrandWorldAverageRules &,const SessionCheckpointIdentity &);
struct BnwGrandCityCulture {
    std::int64_t city=-1;
    bool suppressed=false,league_query_executed=false;
    std::int32_t buildings=0,policies=0,specialists=0,great_works=0;
    std::int32_t terrain=0,traits=0,religion=0,leagues=0;
    std::int32_t base=0,modifier=0,culture=0,theming_classes_visited=0;
};
struct BnwGrandPlayerCityCulture {
    bool calculated=false;
    std::vector<BnwGrandCityCulture> cities;
    std::optional<std::int32_t> total;
    std::string blocker;
};
// Source GetJONSCulturePerTurnFromCities -> each native city JONS getter.
// Shares the actual current building/theming/FLTA owners with tourism.
// This is only the cities term: player culture/religion Lua is not completed.
BnwGrandPlayerCityCulture quote_bnw_jons_culture_from_cities(
 const LiveSessionState &,const BnwGrandWorldAverageRules &,
 const BnwHappinessSuffixRules &,const SessionCheckpointIdentity &,std::int64_t);
struct BnwGrandWorldAverageSourceFile {std::string_view path,sha256;};
inline constexpr std::array kBnwGrandWorldAverageSources{
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvGrandStrategyAI.cpp","1f815b0d1adde6735a5600535f2040e54bbae284bd0931489bfdeb6b4a5fec9e"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvPlayer.cpp","9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvPlayer.h","a08fbe82bb6a6709529e163467ff018c962dd76b61628b0c820946d9fa1284b4"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvPreGame.h","be6d9d3923cddf517273fb9c779ab4baac821b452c0e9394043c1407ef5ac635"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvPreGame.cpp","6fabb3a68f6eea348d0a232998ee6b4f81aa3f4a7b58aa0ae2f9ec32d9383946"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvCultureClasses.cpp","455e48e7055fc23f891b691b9a07691916f6e48ec28f2974f19086ec69a2651b"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvBuildingClasses.cpp","de5121283887ca9011da25b4c27398b7ec083ad70b577676e8fed5dce26940e4"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvReligionClasses.cpp","50d96e758204221392c5497b6322162869ca2352fec5277dacfc177881d81093"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvCity.cpp","15e14918392e0f8e9de7e37758a659ed0bf7a571144daa57e83b1b999a548fee"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvPolicyClasses.cpp","3fe2aca3c99961746d60346913fa59f72315e323285f8594f1cbed22f4eddb5d"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvVotingClasses.cpp","5c49da61bd5fe42f6aab75e94e96525e4472f959dd76fabc11bed8133657c322"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvTraitClasses.h","1750f445c977cec4ecd587a1cd0642f3edb1df1186ce23000b591c2f32254d81"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvTraitClasses.cpp","8e5c3fe9ad40e44e2ad9a16e9ea590ff13ced2b51925f96b280495d82fec8edc"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvTechClasses.cpp","61b8dfe64c20c71edc0db39d4445add2873bf710fbc97e394088213060df9dc2"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvTeam.h","eb5192d83d3e1f6bfafe9846743f7d6aef0e207bc2ef03d4174916ee128144e1"}
};
inline constexpr std::array kBnwCultureInfluenceSources{
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvCultureClasses.h","b5bc607f0e0db46b93ef5f7a95512147c52e4f09913a9094bf8cbc7e17aa4c62"}
};

}
