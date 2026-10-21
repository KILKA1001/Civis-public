#pragma once
#include "civ5/runtime/vp_espionage_checkpoint.hpp"

void vp_espionage_checkpoint_tests(){
    using namespace civ5;
    using namespace civ5::runtime;
    constexpr std::int64_t owner=5000000001LL,city_id=6000000002LL;
    ResearchContext setup;setup.profile=content::TechnologyProfile::vp542;
    setup.players[owner];
    auto &player=ensure_vp_espionage_profile(setup).players[owner];
    player.extra_rank_bonus=3;player.offensive_strength_modifier=-10;
    player.network_points_bonus=9;player.policy_faster_modifier=25;
    player.security_modifier=7;player.cultural_influence_times100[owner]=3000000000LL;
    VpSpy spy;spy.name="Wide spy";spy.rank=4;spy.experience=5000000003LL;
    spy.city_x=12;spy.city_y=41;spy.state=3;spy.focus=6;
    spy.revive_counter=2;spy.vassal_diplomat_player=-1;
    spy.counterspy_mission_turn=22;spy.active_mission_turn=24;
    spy.passive=true;player.spies.push_back(spy);
    VpSpyNotification notice;notice.city_x=12;notice.city_y=41;
    notice.attacking_player=owner;notice.amount_stolen=5000000004LL;
    notice.mission=7;notice.great_work=8;player.spy_messages.push_back(notice);
    VpIntrigue intrigue;intrigue.discovering_player=owner;intrigue.source_player=2;
    intrigue.target_player=3;intrigue.city_x=12;intrigue.city_y=41;
    intrigue.spy_name=std::string("S\0py",4);intrigue.shared=true;
    intrigue.spy_id=5000000005LL;player.intrigue.push_back(intrigue);
    CityYieldContext cities;cities.profile=setup.profile;
    cities.players[owner].cities[city_id].owner=owner;
    cities.players[owner].cities[city_id].id=city_id;
    auto &activity=ensure_vp_city_espionage(cities,owner,city_id);
    activity.security_modifier=8;activity.security_per_x_pop=2;
    activity.previous_spy_missions=4;activity.happiness_delta=-5;
    activity.fields["m_aiAmount"][owner]=5000000010LL;
    activity.fields["m_aiMaxAmount"][owner]=5000000010LL;
    activity.next_passive_bonus[owner]={true,6000000000LL};
    const SessionCheckpointIdentity identity{std::string(64,'a'),std::string(64,'b')};
    const auto section=make_vp_espionage_checkpoint(setup,cities,identity);
    auto restored_setup=setup;auto restored_cities=cities;
    auto &old_player=ensure_vp_espionage_profile(restored_setup).players.at(owner);
    old_player.network_points_bonus=0;old_player.spies[0].experience=0;
    old_player.spy_messages.clear();old_player.intrigue.clear();
    auto &old_city=ensure_vp_city_espionage(restored_cities,owner,city_id);
    old_city.set("m_aiAmount",owner,0);old_city.security_modifier=0;
    restore_vp_espionage_checkpoint(section,restored_setup,restored_cities,identity);
    const auto &live_player=find_vp_espionage_profile(restored_setup)->players.at(owner);
    const auto &live_city=*find_vp_city_espionage(restored_cities,owner,city_id);
    check(live_player.network_points_bonus==9&&live_player.spies[0].experience==5000000003LL&&
          live_player.cultural_influence_times100.at(owner)==3000000000LL&&
          live_player.spy_messages.size()==1&&
          live_player.spy_messages[0].amount_stolen==5000000004LL&&
          live_player.intrigue.size()==1&&live_player.intrigue[0].spy_id==5000000005LL&&
          live_player.intrigue[0].spy_name==std::string("S\0py",4)&&
          live_city.value("m_aiAmount",owner)==5000000010LL&&
          live_city.next_passive_bonus.at(owner).threshold==6000000000LL&&
          live_city.security_modifier==8,
          "VP espionage checkpoint must restore wide live spy and city state");
    auto changed=section;changed.bytes.push_back(0);
    rejects([&]{restore_vp_espionage_checkpoint(changed,restored_setup,restored_cities,identity);});
    check(find_vp_espionage_profile(restored_setup)->players.at(owner).network_points_bonus==9&&
          find_vp_city_espionage(restored_cities,owner,city_id)->value("m_aiAmount",owner)==5000000010LL,
          "Invalid VP espionage checkpoint must leave both live profiles unchanged");
    rejects([&]{restore_vp_espionage_checkpoint(section,restored_setup,restored_cities,
        {std::string(64,'c'),identity.rules_sha256});});
    VpEspionageCheckpointBudget small;small.entries=1;
    rejects([&]{(void)make_vp_espionage_checkpoint(setup,cities,identity,small);});
    ResearchContext bnw;CityYieldContext bnw_cities;
    rejects([&]{(void)make_vp_espionage_checkpoint(bnw,bnw_cities,identity);});
}
