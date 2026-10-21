#include "civ5/runtime/bnw_deficit_selection.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::int64_t neutral=1;
void require(bool value,const char *message) {
    if (!value) throw std::runtime_error(message);
}
std::int64_t scalar(const saves::UnitCoreObservation &core,
                    std::string_view name) {
    const saves::UnitCoreField *found=nullptr;
    for (const auto &field:core.fields)
        if (field.name && std::string_view(field.name)==name) {
            require(!found,"Duplicate BNW deficit unit field");found=&field;
        }
    require(found && found->values.size()==1,
            "Missing BNW deficit unit field");
    return found->values.front();
}
struct Unit {
    BnwDeficitCandidate candidate;
    std::int64_t x=0,y=0,moves=0,base_combat=0;
};
std::int64_t power(std::int64_t intrinsic,std::int64_t level,
                   std::int64_t damage,std::int64_t max_hp) {
    require(intrinsic>=0 && intrinsic<=INT32_MAX && level>=0 &&
            level<=(INT32_MAX-1000)/125 &&
            damage>=0 && damage<=max_hp && max_hp>0,
            "Invalid BNW deficit power inputs");
    const auto modifier=1000+level*125;
    require(modifier>=0 && modifier<=INT32_MAX &&
            intrinsic*modifier<=INT32_MAX,
            "BNW deficit power exceeds source int range");
    const auto adjusted=intrinsic*modifier/1000;
    require(adjusted*(max_hp-damage)<=INT32_MAX,
            "BNW deficit health power exceeds source int range");
    return adjusted*(max_hp-damage)/max_hp;
}
} // namespace

BnwDeficitSelection choose_bnw_deficit_candidate(
    const saves::SavedSessionSnapshot &snapshot,std::int64_t player,
    const BnwDeficitRules &rules,const BnwDeficitMilitaryContext &military,
    SimulationRandom &gameplay_random) {
    require(snapshot.game_state.layout==bnw && snapshot.all_players &&
            snapshot.all_players->layout==bnw &&
            gameplay_random.algorithm()==RandomAlgorithm::bnw,
            "BNW deficit requires matching decoded world and gameplay RNG");
    require(rules.max_hit_points>0 && rules.max_hit_points<=INT32_MAX &&
            military.land_defense && military.naval_defense &&
            *military.land_defense>=-1 && *military.land_defense<=3 &&
            *military.naval_defense>=-1 && *military.naval_defense<=3,
            "BNW deficit requires resolved rules and live military AI state");
    const saves::SavedPlayerObservation *owner=nullptr;
    for (const auto &candidate:snapshot.all_players->players)
        if (candidate.saved_player_id==player) {
            require(!owner,"Duplicate BNW deficit player");owner=&candidate;
        }
    require(owner && owner->units.layout==bnw && owner->cities.layout==bnw,
            "BNW deficit player units or cities unavailable");

    BnwDeficitSelection result;
    result.cities=static_cast<std::int64_t>(owner->cities.cities.size());
    std::vector<Unit> units;
    units.reserve(owner->units.units.size());
    std::set<std::int64_t> ids;
    for (const auto &saved:owner->units.units) {
        require(saved.core.layout==bnw,"BNW deficit unit profile mismatch");
        const auto id=scalar(saved.core,"m_iID");
        const auto unit_owner=scalar(saved.core,"m_eOwner");
        const auto x=scalar(saved.core,"m_iX"),y=scalar(saved.core,"m_iY");
        const auto base=scalar(saved.core,"m_iBaseCombat");
        const auto moves=scalar(saved.core,"m_iMoves");
        const auto level=scalar(saved.core,"m_iLevel");
        const auto damage=scalar(saved.core,"m_iDamage");
        const auto hash=scalar(saved.core,"idUnitType");
        require(id>=0 && unit_owner==player && x>=0 && y>=0 &&
                base>=0 && hash>0 && hash<=UINT32_MAX &&
                ids.insert(id).second,
                "Invalid BNW deficit unit identity or motion");
        Unit unit;
        unit.candidate.unit_id=id;unit.candidate.source_index=saved.index;
        if (base>0) {
            const auto info=rules.units.find(static_cast<std::uint32_t>(hash));
            require(info!=rules.units.end(),"Unresolved BNW deficit unit Info hash");
            unit.candidate.power=power(info->second.intrinsic_power,level,damage,
                                       rules.max_hit_points);
            unit.candidate.domain=info->second.domain;
        }
        unit.x=x;unit.y=y;unit.moves=moves;unit.base_combat=base;
        units.push_back(unit);
        if (base>0) ++result.combat_units;
    }
    if (result.combat_units<=result.cities) return result;

    // Forced disband skips obsolete-tech, trainability, army and upgrade tests.
    // Validate a requested plot against the live fighting-state binding before
    // committing the local RNG copy.
    const auto eligible=[&](const Unit &unit,BnwDeficitDomain domain) {
        return unit.base_combat>0 && unit.candidate.domain==domain;
    };
    const auto candidate=[&](BnwDeficitDomain domain)
        ->std::optional<BnwDeficitCandidate> {
        std::optional<BnwDeficitCandidate> best;
        if ((domain==BnwDeficitDomain::land?*military.land_defense:
             *military.naval_defense)>neutral) return best;
        for (const auto &unit:units) {
            if (!eligible(unit,domain)) continue;
            const auto plot=military.plot_unit_fighting.find({unit.x,unit.y});
            require(plot!=military.plot_unit_fighting.end(),
                    "BNW deficit lacks live plot fighting state");
            if (plot->second || unit.moves<=0 || rules.unit_delete_disabled)
                continue;
            if (unit.candidate.power<INT32_MAX &&
                (!best || unit.candidate.power<best->power))
                best=unit.candidate; // source iteration order breaks ties
        }
        return best;
    };
    auto next_random=gameplay_random;
    const auto draw=next_random.draw(100); // exactly one getJonRandNum call
    result.random_drawn=true;
    result.random_allows_disband=draw<50;
    if (result.random_allows_disband) {
        const auto land=candidate(BnwDeficitDomain::land);
        const auto naval=candidate(BnwDeficitDomain::sea);
        result.land=land;result.naval=naval;
        if (land && (*military.land_defense<=*military.naval_defense ||
                     !naval || land->power<=naval->power))
            result.selected=land;
        else if (naval) result.selected=naval;
    }
    gameplay_random=std::move(next_random);
    return result;
}

} // namespace civ5::runtime
