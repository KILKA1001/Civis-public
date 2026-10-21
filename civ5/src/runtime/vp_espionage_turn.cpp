#include "civ5/runtime/vp_espionage_turn.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <array>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
template<class T> const T &named(const std::vector<T> &rows,std::string_view name) {
    const T *found=nullptr;
    for(const auto &row:rows)if(row.name&&std::string_view(row.name)==name) {
        if(found)throw std::runtime_error("Duplicate VP espionage field: "+std::string(name));
        found=&row;
    }
    if(!found)throw std::runtime_error("Missing VP espionage field: "+std::string(name));
    return *found;
}
std::int64_t field(const saves::PlayerEspionageRecord &record,std::string_view name) {
    return named(record.fields,name).value;
}
template<class T> std::int64_t scalar(const std::vector<T> &rows,std::string_view name) {
    const auto &values=named(rows,name).values;
    if(values.size()!=1)throw std::runtime_error("Invalid VP espionage scalar: "+std::string(name));
    return values.front();
}
VpSpy spy_from_save(const saves::PlayerEspionageRecord &record) {
    VpSpy spy;
    const auto &bytes=named(record.blobs,"m_sName").bytes;
    spy.name.assign(bytes.begin(),bytes.end());
    spy.rank=field(record,"m_eRank");
    spy.experience=field(record,"m_iExperience");
    spy.city_x=field(record,"m_iCityX");
    spy.city_y=field(record,"m_iCityY");
    spy.state=field(record,"m_eSpyState");
    spy.focus=field(record,"m_eSpyFocus");
    spy.revive_counter=field(record,"m_iReviveCounter");
    spy.diplomat=field(record,"m_bIsDiplomat")!=0;
    spy.vassal_diplomat_player=field(record,"m_eVassalDiplomatPlayer");
    spy.evaluate_reassignment=field(record,"m_bEvaluateReassignment")!=0;
    spy.passive=field(record,"m_bPassive")!=0;
    spy.counterspy_mission_turn=field(record,"m_iTurnCounterspyMissionChanged");
    spy.active_mission_turn=field(record,"m_iTurnActiveMissionConducted");
    if(spy.state<0||spy.state>9)
        throw std::runtime_error("Unrecognized VP spy state");
    return spy;
}
VpSpyNotification notification_from_save(const saves::PlayerEspionageRecord &record) {
    VpSpyNotification message;
    message.city_x=field(record,"m_iCityX");message.city_y=field(record,"m_iCityY");
    message.attacking_player=field(record,"m_eAttackingPlayer");
    message.spy_result=field(record,"m_iSpyResult");
    message.amount_stolen=field(record,"m_iAmountStolen");
    message.stolen_tech=field(record,"m_eStolenTech");
    message.mission=field(record,"m_iMission");message.great_work=field(record,"m_iGWID");
    return message;
}
VpIntrigue intrigue_from_save(const saves::PlayerEspionageRecord &record) {
    VpIntrigue message;
    message.discovering_player=field(record,"m_eDiscoveringPlayer");
    message.source_player=field(record,"m_eSourcePlayer");
    message.target_player=field(record,"m_eTargetPlayer");
    message.diplomacy_player=field(record,"m_eDiplomacyPlayer");
    message.building=field(record,"m_eBuilding");message.project=field(record,"m_eProject");
    message.unit=field(record,"m_eUnit");message.type=field(record,"m_iIntrigueType");
    message.turn=field(record,"m_iTurnNum");
    message.city_x=field(record,"m_iCityX");message.city_y=field(record,"m_iCityY");
    const auto &name=named(record.blobs,"m_strSpyName").bytes;
    message.spy_name.assign(name.begin(),name.end());
    message.shared=field(record,"m_bShared")!=0;
    message.spy_id=field(record,"iSpyID");
    return message;
}
}
VpEspionageProfileState &ensure_vp_espionage_profile(ResearchContext &world) {
    if(world.profile!=vp)throw std::runtime_error("VP espionage requires VP GameCore profile");
    if(auto *profile=world.extensions.find<VpEspionageProfileState>())return *profile;
    return world.extensions.emplace<VpEspionageProfileState>();
}
const VpEspionageProfileState *find_vp_espionage_profile(const ResearchContext &world) {
    return world.profile==vp?world.extensions.find<VpEspionageProfileState>():nullptr;
}
void restore_vp_espionage_profile(const saves::SavedSessionSnapshot &snapshot,ResearchContext &world) {
    if(world.profile!=vp||!snapshot.all_players||snapshot.all_players->layout!=saves::GameStateLayout::vp542_corpus)
        throw std::runtime_error("VP espionage import requires VP players");
    VpEspionageProfileState imported;
    for(const auto &saved:snapshot.all_players->players) {
        const auto id=saved.saved_player_id;
        if(!world.players.contains(id)||saved.espionage.layout!=saves::GameStateLayout::vp542_corpus)
            throw std::runtime_error("VP espionage player is not in one VP roster");
        const auto &record=named(saved.espionage.blocks,"player_espionage");
        VpEspionagePlayer player;
        player.saved_record=record;
        player.extra_rank_bonus=scalar(saved.traits.fields,"m_iSpyExtraRankBonus");
        player.offensive_strength_modifier=scalar(saved.traits.fields,
            "m_iSpyOffensiveStrengthModifier");
        player.network_points_bonus=scalar(saved.core.fields,"m_iEspionageNetworkPoints");
        player.security_modifier=scalar(saved.core.fields,"m_iSpySecurityModifier");
        const auto &policies=named(saved.strategy.blocks,"policies");
        const auto &modifiers=named(policies.arrays,"policy_modifiers");
        // Pinned VP 5.4.2 source enum, including the HH slot at index two.
        if(modifiers.columns!=1||modifiers.values.size()!=60)
            throw std::runtime_error("Unverified VP espionage policy wire layout");
        player.policy_faster_modifier=modifiers.values.at(48);
        const auto &culture=named(saved.trade_culture.blocks,"player_culture");
        const auto &influence=named(culture.arrays,"m_aiCulturalInfluenceTimes100");
        if(influence.columns!=1)throw std::runtime_error("Invalid VP culture influence wire layout");
        for(std::size_t i=0;i<influence.values.size();++i) {
            if(i>static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()))
                throw std::runtime_error("VP culture influence index exceeds host identity range");
            player.cultural_influence_times100.emplace(static_cast<std::int64_t>(i),
                influence.values[i]);
        }
        for(const auto &child:record.children) {
            const std::string_view kind=child.name?child.name:"";
            if(kind=="spy")player.spies.push_back(spy_from_save(child));
            else if(kind=="spy_notification")player.spy_messages.push_back(notification_from_save(child));
            else if(kind=="intrigue")player.intrigue.push_back(intrigue_from_save(child));
        }
        if(!imported.players.emplace(id,std::move(player)).second)
            throw std::runtime_error("Duplicate VP espionage player");
    }
    if(imported.players.size()!=world.players.size())
        throw std::runtime_error("Incomplete VP espionage roster");
    ensure_vp_espionage_profile(world)=std::move(imported);
}
VpSpyMessageDispatch vp_add_spy_message(ResearchContext &world,std::int64_t defender,
    VpSpyNotification message,const content::CustomModOptions &options,LuaScriptHost *scripts) {
    if(!options.all_succeeded())throw std::runtime_error("VP spy message requires loaded GameCore options");
    const auto *profile=find_vp_espionage_profile(world);
    if(!profile||!world.players.contains(defender)||!profile->players.contains(defender))
        throw std::runtime_error("Unresolved VP spy message defender");
    VpSpyMessageDispatch result;
    if(options.enabled("EVENTS_ESPIONAGE")) {
        if(!scripts)throw std::runtime_error("VP espionage gameplay event host is not bound");
        const std::array<ScriptValue,7> args{
            message.city_x,message.city_y,message.attacking_player,defender,
            message.spy_result,message.stolen_tech,message.amount_stolen};
        result.hook_errors=scripts->call_hook("EspionageNotificationData",args).errors;
    }
    ensure_vp_espionage_profile(world).players.at(defender).spy_messages.push_back(message);
    return result;
}
bool vp_update_spy_message(ResearchContext &world,std::int64_t defender,
    std::int64_t city_x,std::int64_t city_y,std::int64_t attacking_player,
    std::int64_t stolen_tech,std::int64_t amount_stolen,std::int64_t great_work) {
    const auto *profile=find_vp_espionage_profile(world);
    if(!profile||!world.players.contains(defender)||!profile->players.contains(defender))
        throw std::runtime_error("Unresolved VP spy message defender");
    auto &messages=ensure_vp_espionage_profile(world).players.at(defender).spy_messages;
    for(auto &message:messages)if(message.city_x==city_x&&message.city_y==city_y&&
        message.attacking_player==attacking_player) {
        message.stolen_tech=stolen_tech;
        message.amount_stolen=amount_stolen;
        message.great_work=great_work;
        return true;
    }
    return false;
}
bool vp_recent_building_army_intrigue(const ResearchContext &world,std::int64_t discoverer,
    std::int64_t source,bool amphibious) {
    const auto *profile=find_vp_espionage_profile(world);
    if(!profile||!world.players.contains(discoverer)||!profile->players.contains(discoverer))
        throw std::runtime_error("Unresolved VP building-army intrigue discoverer");
    // Release-5.4.2 CvIntrigueType: building land army=1, amphibious army=2.
    const std::int64_t type=amphibious?2:1;
    for(const auto &message:profile->players.at(discoverer).intrigue)
        if(message.source_player==source&&message.target_player==-1&&message.type==type&&
           wide::subtract(world.game_turn,message.turn)<30)
            return true;
    return false;
}
bool vp_recent_diplomatic_intrigue(const ResearchContext &world,std::int64_t discoverer,
    std::int64_t source,std::int64_t target,bool coop_war,
    std::int64_t city_x,std::int64_t city_y) {
    const auto *profile=find_vp_espionage_profile(world);
    if(!profile||!world.players.contains(discoverer)||!profile->players.contains(discoverer))
        throw std::runtime_error("Unresolved VP diplomatic intrigue discoverer");
    // Release-5.4.2 CvIntrigueType: deception=0, cooperative war=6.
    const std::int64_t type=coop_war?6:0;
    for(const auto &message:profile->players.at(discoverer).intrigue) {
        if(message.source_player!=source||message.target_player!=target||message.type!=type||
           wide::subtract(world.game_turn,message.turn)>=30)continue;
        if(!coop_war||
           (message.building==-1&&message.project==-1&&message.unit==-1&&
            message.city_x==city_x&&message.city_y==city_y))return true;
    }
    return false;
}
VpEspionageTurnFrontier vp_espionage_turn_frontier(const ResearchContext &world,std::int64_t player) {
    const auto *profile=find_vp_espionage_profile(world);
    if(!profile||!world.players.contains(player)||!profile->players.contains(player))
        throw std::runtime_error("Unresolved VP espionage player");
    const auto &saved=profile->players.at(player);
    VpEspionageTurnFrontier result;
    result.spy_count=saved.spies.size();
    result.queued_messages=saved.spy_messages.size();
    result.queued_intrigue=saved.intrigue.size();
    if(result.queued_messages) {
        result.unresolved="VP spy notification host is not bound";
        return result;
    }
    for(std::size_t i=0;i<saved.spies.size();++i) {
        const auto state=saved.spies[i].state;
        // Release-5.4.2 CvSpyState: 0 unassigned, 9 terminated.
        if(state!=0&&state!=9) {
            result.first_spy_needing_host=i;
            result.first_spy_state=state;
            result.unresolved="VP spy state processing requires map, city and event hosts";
            return result;
        }
    }
    result.safe_no_gameplay_effect=true;
    return result;
}
} // namespace civ5::runtime
