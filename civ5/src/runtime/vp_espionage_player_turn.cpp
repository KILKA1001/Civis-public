#include "civ5/runtime/vp_espionage_player_turn.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
std::int64_t define(const content::GlobalDefineCache &defines,std::string_view name) {
    if(defines.profile()!=content::GlobalDefineProfile::vp542)
        throw std::runtime_error("VP espionage define profile mismatch");
    const auto *entry=defines.find(name);
    if(!entry||!entry->lookup.error.empty())
        throw std::runtime_error("Unresolved VP espionage define: "+std::string(name));
    const auto *value=std::get_if<std::int64_t>(&entry->lookup.value);
    if(!value)throw std::runtime_error("VP espionage define is not an integer");
    return *value;
}
}
VpEspionagePlayerTurnSegment process_vp_espionage_player_turn_segment(
    ResearchContext &setup,CityYieldContext &cities,
    const PlayerResearchContext &diplomacy,std::span<const TeamTechnologies> technologies,
    const TreasuryContext &treasury,const TreasuryRules &treasury_rules,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    const content::CustomModOptions &options,std::int64_t player) {
    constexpr auto vp=content::TechnologyProfile::vp542;
    if(setup.profile!=vp||cities.profile!=vp||diplomacy.profile!=vp||
       treasury.profile!=vp||treasury_rules.profile!=vp||
       defines.profile()!=content::GlobalDefineProfile::vp542||!options.all_succeeded())
        throw std::runtime_error("VP espionage turn requires one selected VP world and rules");
    const auto *profile=find_vp_espionage_profile(setup);
    if(!profile||!setup.players.contains(player)||!profile->players.contains(player))
        throw std::runtime_error("Unresolved VP espionage turn player");
    VpEspionagePlayerTurnSegment result;
    if(!profile->players.at(player).spy_messages.empty()) {
        result.unresolved="VP spy notification host is not bound";
        return result;
    }
    auto &spies=ensure_vp_espionage_profile(setup).players.at(player).spies;
    const auto stop=[&](std::size_t index,std::int64_t state,std::string why) {
        result.blocking_spy=index;
        result.blocking_state=state;
        result.unresolved=std::move(why);
    };
    for(std::size_t i=0;i<spies.size();++i) {
        const auto state=spies[i].state;
        try {
            switch(state) {
            case 0: // SPY_STATE_UNASSIGNED: ProcessSpy returns immediately.
            case 9: // SPY_STATE_TERMINATED: no gameplay mutation.
                break;
            case 1: { // SPY_STATE_TRAVELLING before arrival.
                result.travelling_steps.push_back(process_vp_travelling_without_arrival_host(
                    setup,cities,player,i));
                if(!result.travelling_steps.back().unresolved.empty()) {
                    stop(i,state,result.travelling_steps.back().unresolved);
                    return result;
                }
                break;
            }
            case 3: { // SPY_STATE_GATHERING_INTEL with BALANCE_VP.
                const MaintenanceWorld world{setup,diplomacy,cities,technologies};
                result.gathering_steps.push_back(process_vp_gathering_intel_without_effect_host(
                    world,cities,treasury,treasury_rules,database,defines,options,player,i));
                break;
            }
            case 7: { // SPY_STATE_SCHMOOZE with BALANCE_VP.
                const MaintenanceWorld world{setup,diplomacy,cities,technologies};
                result.diplomat_steps.push_back(process_vp_schmooze_without_effect_host(
                    world,cities,treasury,treasury_rules,database,defines,options,player,i));
                if(!result.diplomat_steps.back().unresolved.empty()) {
                    stop(i,state,result.diplomat_steps.back().unresolved);
                    return result;
                }
                break;
            }
            case 5: // SPY_STATE_COUNTER_INTEL: only optional experience.
                if(options.enabled("BALANCE_VP")&&
                   define(defines,"ESPIONAGE_XP_PER_TURN_COUNTERSPY")!=0)
                    throw std::runtime_error("VP counterspy experience requires promotion host");
                break;
            case 8: { // SPY_STATE_DEAD: revive only after the selected timer.
                const auto next=wide::add(spies[i].revive_counter,1);
                if(next>=define(defines,"BALANCE_SPY_RESPAWN_TIMER"))
                    throw std::runtime_error("VP spy revival requires name and notification hosts");
                spies[i].revive_counter=next;
                break;
            }
            default:
                throw std::runtime_error("VP spy state requires its GameCore world effects");
            }
        } catch(const std::exception &error) {
            stop(i,state,error.what());
            return result;
        }
        ++result.processed_spies;
    }
    result.complete=true;
    return result;
}
} // namespace civ5::runtime
