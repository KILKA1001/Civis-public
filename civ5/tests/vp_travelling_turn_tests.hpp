#pragma once
#include "civ5/runtime/vp_travelling_turn.hpp"

void vp_travelling_turn_tests() {
    using namespace civ5;
    using namespace civ5::runtime;
    constexpr std::int64_t city_id=5000000001LL;
    ResearchContext setup;setup.profile=content::TechnologyProfile::vp542;
    setup.players[0].alive=true;setup.players[1].alive=true;
    VpSpy spy{};spy.state=1;spy.city_x=12;spy.city_y=7;
    ensure_vp_espionage_profile(setup).players[0].spies.push_back(spy);
    CityYieldContext cities;cities.profile=setup.profile;
    CityYieldState city{};city.owner=1;city.id=city_id;city.x=12;city.y=7;
    cities.players[1].cities.emplace(city_id,city);
    auto &activity=ensure_vp_city_espionage(cities,1,city_id);
    activity.fields["m_aiSpyAssignment"][0]=0;
    activity.fields["m_aiAmount"][0]=3000000000LL;
    activity.fields["m_aiMaxAmount"][0]=3000000000LL;
    activity.fields["m_aiRate"][0]=1000000000LL;
    activity.fields["m_aiGoal"][0]=6000000000LL;
    activity.fields["m_aiLastProgress"][0]=77;
    const auto step=process_vp_travelling_without_arrival_host(setup,cities,0,0);
    check(step.city==city_id&&step.before==3000000000LL&&step.after==4000000000LL&&
          step.max_after==4000000000LL&&!step.arrival_pending&&step.unresolved.empty()&&
          activity.value("m_aiAmount",0)==4000000000LL&&
          activity.value("m_aiMaxAmount",0)==4000000000LL&&
          activity.value("m_aiLastProgress",0)==77&&
          ensure_vp_espionage_profile(setup).players.at(0).spies.at(0).state==1,
          "VP travel must advance wide saved city progress without changing unrelated spy activity");
    activity.set("m_aiGoal",0,5000000000LL);
    const auto arriving=process_vp_travelling_without_arrival_host(setup,cities,0,0);
    check(arriving.arrival_pending&&arriving.after==5000000000LL&&
          !arriving.unresolved.empty()&&activity.value("m_aiAmount",0)==4000000000LL,
          "Reaching travel goal must await mission transition without partial mutation");
    activity.set("m_aiSpyAssignment",0,1);
    rejects([&]{(void)process_vp_travelling_without_arrival_host(setup,cities,0,0);});
    check(activity.value("m_aiAmount",0)==4000000000LL,
          "Mismatched saved spy assignment must not move city activity");
    ResearchContext vanilla;
    rejects([&]{(void)process_vp_travelling_without_arrival_host(vanilla,cities,0,0);});
}
