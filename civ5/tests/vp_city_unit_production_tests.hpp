#pragma once
void vp_city_unit_production_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    constexpr auto vp=TechnologyProfile::vp542;constexpr auto layout=GameStateLayout::vp542_corpus;
    constexpr std::int64_t big=4294967300LL;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES"
        "('MAX_UNIT_SUPPLY_PRODMOD',70),('PRODUCTION_PENALTY_PER_UNIT_OVER_SUPPLY',5),('CITY_MAX_NUM_BUILDINGS',1),"
        "('LOCAL_UNHAPPY_COMBAT_UNIT_PRODUCTION_PENALTY',10),('LOCAL_UNHAPPY_SETTLER_PRODUCTION_PENALTY',10),"
        "('UNHAPPY_MAX_UNIT_PRODUCTION_PENALTY',-75),('UNHAPPY_THRESHOLD',50),"
        "('GLOBAL_SETTLER_PRODUCTION_PENALTY_PER_UNHAPPY',2.5);");
    const auto types=db.cache_info_types();auto defines=db.cache_global_defines(GlobalDefineProfile::vp542);
    CustomModOptions options;options.entries["BALANCE_VP"].value=1;
    ResearchContext players;players.profile=vp;players.players[5].team=9;ensure_vp_unit_trait_profile(players).potential_by_player[5]={};
    PlayerResearchContext dip;dip.profile=vp;dip.major_players={5};dip.teams[9].members={5};
    CityYieldContext world;world.profile=vp;auto &p=world.players[5];p.capital=42;ensure_vp_great_work_profile(world).players[5]={};ensure_vp_great_work_profile(world).cities[{5,42}]={};p.second_pantheon_count=0;
    auto &growth_profile=ensure_vp_city_growth_profile(world);growth_profile.players[5]={};auto &city_growth=growth_profile.cities[{5,42}];
    auto &corporation_profile=ensure_vp_corporation_profile(world);corporation_profile.players[5]={};
    auto &production_profile=ensure_vp_general_production_profile(world);production_profile.players[5]={};production_profile.cities[{5,42}]={};
    auto &profile=ensure_vp_unit_production_profile(world);profile.players[5]={};profile.city_players[5]={};profile.cities[{5,42}]={};
    auto &city=p.cities[42];city.owner=5;city.id=42;city.x=city.y=0;ensure_vp_city_trait_yield_profile(world).cities[{5,42}]={};ensure_vp_city_happiness_profile(world).cities[{5,42}]={};ensure_vp_city_happiness_profile(world).players[5]={};vp_city_majority_religion(city)=-1;city.production_orders=std::vector<CityYieldState::ProductionOrder>{{0,8}};
    city.building_counts_resolved=true;city.real_buildings[2]=2;
    MaintenanceContext units;units.profile=vp;ensure_vp_unit_supply_profile(units).world_city_supply_modifier=0;units.team_eras[9]=0;units.players[5]={};auto &unit_supply=ensure_vp_unit_supply_profile(units).players[5];unit_supply.base_cache=100;unit_supply.war_cache=100;
    units.players[5].units[1];profile.military_units[{5,1}]=3;
    VpUnitSupplyRules sr;std::vector<TeamTechnologies> techs;techs.emplace_back(vp,9,5);
    VpUnitSupplyContext supply{units,players,dip,world,techs,sr,db,defines,options};VpCityFoodRules fr;VpCityFoodContext food{world,players,units.team_eras,fr,defines};
    VpCityProductionRules pr;VpCityProductionContext production{supply,food,pr};
    SavedMapProjection map{{1,1}};map.cells.resize(1);map.cells[0].index=0;map.cells[0].x=0;map.cells[0].y=0;map.cells[0].units=std::vector<SavedPlotUnitReference>{{5,1},{5,1}};
    PlotPassability pass;PlotWaterRules wr;wr.profile=vp;PlotWaterState ws;ws.layout=layout;PlotWaterContext water{map,ws,wr,types,pass,players,world,defines,&options};
    VpPlotReligionRules rr;VpPlotReligionContext religion{rr,water};TreasuryContext treasury;treasury.profile=vp;treasury.players[5]={};
    VpCityReligionMinorContext minor{religion,treasury};VpUnitTraitRules tr;ResearchTurnState turn;turn.profile=vp;turn.players[5]={};VpUnitTraitContext traits{tr,players,turn,dip,techs,minor};
    VpPlayerUnitProductionRules ur;ur.units[7].military=true;ur.units[8].found=true;VpPlayerUnitProductionContext player{ur,production,traits};
    CityYieldRules cr;cr.profile=vp;VpGreatWorkRules gr;struct Services:VpGreatWorkServices{void unlock_capital_theming_achievement(std::int64_t)override{throw std::runtime_error("Unexpected achievement");}} services;
    VpGreatWorkContext great{world,cr,gr,religion,services};ScienceContext science;science.profile=vp;
    ensure_vp_science_profile(science.players[5]).cached_happiness_total=40;
    VpCityGrowthRules growth_rules;VpCityGrowthContext growth{religion,dip,science,growth_rules};
    VpCityUnitProductionRules rules;auto &military=rules.units[7];military.military=true;military.unit_class=3;military.domain=2;military.combat=4;military.strength=10;military.required_resources[1]=1;military.buildings[2]=5;
    auto &settler=rules.units[8];settler.found=true;settler.found_mid=true;settler.domain=2;
    rules.buildings.insert(2);rules.corporations[6][1]=11;corporation_profile.players.at(5).founded=6;
    auto &ps=profile.city_players.at(5);auto &cs=profile.cities.at({5,42});
    ps.classes[3]=7;ps.pledge_domains[2]=4;ps.capital_settler=9;
    cs.domains[2]=6;cs.combats[4]=8;cs.military=12;city_growth.happiness_delta=2;
    VpCityUnitProductionContext context{rules,player,great,growth};
    auto q=vp_city_unit_production(context,city,7);
    check(q.unit_class==7&&q.domain==6&&q.combat==8&&q.corporation==11&&q.unhappiness==-45&&q.military_city==12&&q.military_units==6&&q.buildings==10&&q.total==15,
        "City unit modifiers combine active rules and use queued settler for local/global happiness");
    check(vp_city_unit_production(context,city,7,true).unhappiness==0,"Ignore-happiness mode omits only the queue penalty");
    city.production_orders->front().item=7;q=vp_city_unit_production(context,city,8);
    check(q.capital_settler==9&&q.unhappiness==-20,"Queried settler receives capital bonus while queued combat unit selects combat unhappiness");
    city.production_orders->front().type=1;check(vp_city_unit_production(context,city,7).unhappiness==0,"Non-unit first order does not impose unit happiness penalty");city.production_orders->front().type=0;
    auto &minor_player=world.players[22];minor_player.minor=true;profile.city_players[22].protected_by.insert(5);
    check(vp_city_unit_production(context,city,7).pledges==4,"Each protected minor supplies the trait domain modifier");
    profile.city_players[22].protected_by.clear();
    military.required_resources.clear();military.total_resources[1]=1;options.entries["UNITS_RESOURCE_QUANTITY_TOTALS"].value=1;
    check(vp_city_unit_production(context,city,7).corporation==11,"Quantity-total option enables corporation resource production");
    options.entries["UNITS_RESOURCE_QUANTITY_TOTALS"].value=0;
    check(vp_city_unit_production(context,city,7).corporation==0,"Inactive quantity-total option leaves no corporation bonus");
    military.required_resources[1]=1;
    military.spaceship=3;production_profile.cities.at({5,42}).space=5;production_profile.players.at(5).space=6;
    CityYieldContext::League league;league.members={5};world.leagues.push_back(league);profile.league_space_ship_production.push_back({7,-2});
    q=vp_city_unit_production(context,city,7);
    check(q.space_city==5&&q.space_player==6&&q.space_leagues==5,"Spaceship production combines city, player and member-league resolutions");
    world.leagues[0].members.clear();check(vp_city_unit_production(context,city,7).space_leagues==0,"Nonmember congress resolutions do not modify spaceship production");
    military.spaceship=-1;
    CityYieldReligionBeliefs religion_state;religion_state.religion=1;religion_state.founder=5;religion_state.beliefs={0};ensure_vp_religion_beliefs(religion_state,vp);vp_religion_beliefs(religion_state).belief_religion=1;vp_religion_beliefs(religion_state).holy_x=0;vp_religion_beliefs(religion_state).holy_y=0;
    world.religions={religion_state};vp_city_majority_religion(city)=1;map.cells[0].owner=5;rr.beliefs[0].follower=true;
    rules.beliefs[0].unit=13;rules.beliefs[0].combats[4]=3;
    q=vp_city_unit_production(context,city,7);
    check(q.religion_unit==13&&q.religion_combat==3,"Majority belief grants general combat and class production separately");
    vp_city_majority_religion(city)=-1;world.religions.clear();
    ps.classes[3]=big;check(vp_city_unit_production(context,city,7).unit_class==big,"City unit production supports modifier values above signed 32-bit range");
    ps.classes[3]=INT64_MAX;rejects([&]{vp_city_unit_production(context,city,7);});ps.classes[3]=7;
    check(vp_city_unit_production(context,city,999).total==0,"Missing Unit Info returns zero before general city/player modifiers");
    auto detached=city;rejects([&]{vp_city_unit_production(context,detached,7);});
}
