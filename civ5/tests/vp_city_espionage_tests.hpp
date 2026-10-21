#pragma once
#include "civ5/runtime/vp_city_espionage.hpp"

void vp_city_espionage_tests() {
    using namespace civ5;
    using namespace civ5::runtime;
    constexpr auto layout=saves::GameStateLayout::vp542_corpus;
    saves::SavedSessionSnapshot snapshot;
    snapshot.all_players.emplace();snapshot.all_players->layout=layout;
    saves::SavedPlayerObservation saved{};
    saved.saved_player_id=0;
    saves::SavedCityObservation city{};
    city.core.layout=city.tail.layout=layout;
    const auto core=[&](const char *name,std::int64_t value) {
        saves::CityCoreField f{};f.name=name;f.values={value};city.core.fields.push_back(f);
    };
    core("m_iID",5000000001LL);core("m_eOwner",0);core("m_iX",12);core("m_iY",7);
    core("m_iSpySecurityModifier",7);core("m_iSpySecurityModifierPerXPop",3);
    core("m_iNumPreviousSpyMissions",2);core("m_iHappinessDelta",-4);
    saves::CityTailNode espionage{};espionage.name="espionage";
    constexpr const char *names[]={
        "m_aiSpyAssignment","m_aiAmount","m_aiMaxAmount","m_aiNextPassiveBonus",
        "m_aiRate","m_aiGoal","m_aiLastProgress","m_aiLastPotential",
        "m_aiLastBasePotential","m_aiResult","m_aiNumTimesCityRobbed",
        "m_aiPendingEventsForPlayer","m_aiDiplomatTradeBonus",
        "m_abDiplomatReceiveIntrigues","m_abDiplomatRevealTrueApproaches",
        "m_aiDiplomatVoteTradePercent","m_aiSciencePassivePerTurn",
        "m_abRevealCityScreen","m_aiVisionBonus"};
    for(const auto *name:names) {
        saves::CityTailNode field{};field.name=name;field.values={0};
        if(std::string_view(name)=="m_aiAmount")field.values={100};
        if(std::string_view(name)=="m_aiMaxAmount")field.values={120};
        if(std::string_view(name)=="m_aiRate")field.values={8};
        if(std::string_view(name)=="m_aiGoal")field.values={-1};
        if(std::string_view(name)=="m_aiNextPassiveBonus")field.values={-1};
        espionage.children.push_back(std::move(field));
    }
    city.tail.blocks={espionage};saved.cities.cities={city};snapshot.all_players->players={saved};
    CityYieldContext world;world.profile=content::TechnologyProfile::vp542;
    CityYieldState live{};live.owner=0;live.id=5000000001LL;live.x=12;live.y=7;
    world.players[0].cities.emplace(live.id,live);
    restore_vp_city_espionage(snapshot,world);
    const auto bound=vp_spy_city_binding(world,0,0,12,7);
    check(bound&&bound->city==5000000001LL&&bound->amount==100&&
          bound->max_amount==120&&bound->rate==8&&bound->goal==-1,
          "VP spy must bind to the same live city and saved activity");
    const auto *imported=find_vp_city_espionage(world,0,live.id);
    check(imported->security_modifier==7&&imported->security_per_x_pop==3&&
          imported->previous_spy_missions==2&&imported->happiness_delta==-4&&
          !imported->next_passive_bonus.at(0).initialized&&
          !imported->fields.contains("m_aiNextPassiveBonus"),
          "VP passive bonus wire sentinel must be normalized at import");
    auto copy=world;
    auto &activity=ensure_vp_city_espionage(copy,0,live.id);
    const auto changed=process_vp_city_spy_activity(activity,0,3000000000LL);
    check(changed.before==100&&changed.after==3000000100LL&&changed.max_after==3000000100LL&&
          activity.value("m_aiLastProgress",0)==3000000000LL&&
          find_vp_city_espionage(world,0,live.id)->value("m_aiAmount",0)==100,
          "VP city spy progress must use wide state and isolate copied worlds");
    activity.set("m_aiSpyAssignment",0,1);
    rejects([&]{(void)vp_spy_city_binding(copy,0,0,12,7);});
    check(!vp_spy_city_binding(copy,0,0,99,99),"Absent VP spy target city must remain absent");
    snapshot.all_players->players[0].cities.cities[0].tail.blocks[0].children.pop_back();
    rejects([&]{restore_vp_city_espionage(snapshot,world);});
    check(find_vp_city_espionage(world,0,live.id)->value("m_aiAmount",0)==100,
          "Failed VP city import must preserve existing live city activity");
    snapshot.all_players->players[0].cities.cities[0].tail.blocks[0].children.push_back(
        {"m_aiVisionBonus","counted_i32",0,0,{}, {0}, {}});
    snapshot.all_players->players[0].cities.cities[0].tail.blocks[0].children[3].values={2147483647};
    restore_vp_city_espionage(snapshot,world);
    const auto &no_more=find_vp_city_espionage(world,0,live.id)->next_passive_bonus.at(0);
    check(no_more.initialized&&!no_more.threshold,
          "Original INT_MAX passive sentinel must not become a live 32-bit ceiling");
    CityYieldContext vanilla;
    rejects([&]{(void)ensure_vp_city_espionage(vanilla,0,live.id);});
    check(find_vp_city_espionage(vanilla,0,live.id)==nullptr,
          "Vanilla city must not require VP espionage activity");
}
