#pragma once
void bnw_unhappiness_tests() {
    using namespace civ5::runtime;
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES"
        "('UNHAPPINESS_PER_CITY',3),('UNHAPPINESS_PER_CAPTURED_CITY',5),('UNHAPPINESS_PER_POPULATION',1),"
        "('UNHAPPINESS_PER_OCCUPIED_POPULATION',1.34),('FREE_UNIT_HAPPINESS',20),('DEFAULT_SPECIALIST',0),"
        "('VERY_UNHAPPY_PRODUCTION_PENALTY_PER_UNHAPPY',-2),('VERY_UNHAPPY_MAX_PRODUCTION_PENALTY',-40);"
        "CREATE TABLE HandicapInfos(ID,Type,NumCitiesUnhappinessMod,PopulationUnhappinessMod,AIUnhappinessPercent);"
        "INSERT INTO HandicapInfos VALUES(0,'P',80,90,73),(1,'G',100,100,50);"
        "CREATE TABLE Specialists(ID,Type);INSERT INTO Specialists VALUES(0,'DEFAULT'),(1,'SCIENTIST'),(4294967300,'WIDE');");
    const auto defs=db.cache_global_defines(GlobalDefineProfile::bnw);auto rules=load_bnw_unhappiness_rules(db);
    check(rules.specialist_slots==4294967301ULL && rules.handicaps.at(0).cities==80 && rules.handicaps.at(1).ai==50,
        "Unhappiness Info must preserve wide sparse slots and per-player/game handicaps");
    ResearchContext setup;setup.game_handicap=1;setup.players[0]={7,0,3,0,true};
    CityYieldContext world;world.yield_slots=6;world.players[0].capital=10;
    BnwUnhappinessContext s;s.world_cities_percent=60;auto &p=s.players[0];
    p.happiness=40;p.units=199;p.units_modifier=50;p.population_modifier=-20;p.cities_modifier=-10;
    p.occupied_modifier=-25;p.capital_modifier=-10;p.trait_cities_modifier=20;p.trait_population_modifier=10;
    p.public_opinion=3;p.half_specialists=true;
    for(auto [id,population]:std::vector<std::pair<std::int64_t,std::int64_t>>{{10,9},{11,5},{12,5},{13,4}}) {
        auto &city=world.players[0].cities[id];city.id=id;city.owner=0;city.population=population;p.cities[id]={};
    }
    p.cities[10].specialists={{0,100},{1,3}};p.cities[11].specialists={{1,1}};p.cities[12].specialists={{1,2}};
    p.cities[11].occupied=true;world.players[0].cities[11].puppet=true;p.cities[12].occupied=true;
    p.cities[13].occupied=true;p.cities[13].no_occupied_unhappiness=true;
    auto q=bnw_unhappiness(s,setup,world,rules,defs,0);
    check(q.normal_cities_times100==475 && q.captured_cities_times100==264 && q.normal_population_times100==1132 &&
          q.occupied_population_times100==361 && q.units_times100==268 && q.before_ai==28 && q.total==28 && q.excess_happiness==12 && !q.ai_discount_applied,
          "BNW total unhappiness rounding, specialist, occupation or traits differ");
    world.no_happiness=true;
    check(bnw_unhappiness(s,setup,world,rules,defs,0).total==28,"GetUnhappiness itself must not gate NO_HAPPINESS");
    world.no_happiness=false;
    setup.players[0].slot_status=1;
    q=bnw_unhappiness(s,setup,world,rules,defs,0);
    check(q.total==14 && q.ai_discount_applied,"AI percentage must use game handicap after public opinion");
    setup.players[9]={7,0,3,0,true};
    check(!bnw_unhappiness(s,setup,world,rules,defs,0).ai_discount_applied,"Living human teammate exempts AI");
    setup.players[9].alive=false;
    check(bnw_unhappiness(s,setup,world,rules,defs,0).total==14,"Dead human must not exempt AI teammate");
    setup.players[9].alive=true;setup.players[9].team=8;
    check(bnw_unhappiness(s,setup,world,rules,defs,0).total==14,"Human on another team must not exempt AI");
    setup.players.erase(9);setup.players[0].slot_status=3;
    p.cities[10].ignore=true;
    q=bnw_unhappiness(s,setup,world,rules,defs,0);
    check(q.normal_cities_times100==316 && q.normal_population_times100==633,"Ignored city must be excluded from count and population");
    BnwUnhappinessAssumptions preview;preview.annexed=UnhappinessCityKey{0,10};
    q=bnw_unhappiness(s,setup,world,rules,defs,0,preview);
    check(q.captured_cities_times100==528 && q.occupied_population_times100==994,"Annex preview overrides ignored/capital status");
    preview.puppeted=UnhappinessCityKey{0,10};
    q=bnw_unhappiness(s,setup,world,rules,defs,0,preview);
    check(q.normal_cities_times100==475 && q.captured_cities_times100==264 && q.normal_population_times100==633 && q.occupied_population_times100==994,
          "Count/population must preserve different precedence when both preview pointers match");
    preview.annexed.reset();q=bnw_unhappiness(s,setup,world,rules,defs,0,preview);
    check(q.total==28,"Puppet preview overrides ignored state and restores normal population");
    preview.puppeted=UnhappinessCityKey{1,10};
    check(bnw_unhappiness(s,setup,world,rules,defs,0,preview).normal_cities_times100==316,"Foreign preview city must not match by ID alone");
    p.cities[10].ignore=false;
    p.cities[12].specialists[1]=5;world.players[0].cities[12].population=1;
    check(bnw_unhappiness(s,setup,world,rules,defs,0).occupied_population_times100==-180,"Occupied negative population product must truncate toward zero without clamping");
    p.cities[12].specialists[1]=2;world.players[0].cities[12].population=5;
    p.units=10;p.units_modifier=-200;
    check(bnw_unhappiness(s,setup,world,rules,defs,0).units_times100==0,"Free-unit subtraction clamps BEFORE unit modifier");
    p.units=199;
    check(bnw_unhappiness(s,setup,world,rules,defs,0).units_times100==-179,"Negative unit modifier must not get a second clamp");
    p.units_modifier=50;
    CityYieldRules yield_rules;
    world.players[0].cities[10].yields[1].terrain=10;
    rejects([&]{city_yield_times100(world,yield_rules,defs,0,10,1);});
    p.happiness=8;
    auto bound=bind_bnw_unhappiness(s,setup,world,rules,defs);
    check(bound.at(0).excess_happiness==-20 && world.players[0].excess_happiness==-20 &&
          city_yield_times100(world,yield_rules,defs,0,10,1).total_times100==600,
          "Bound empire unhappiness must enable actual city production penalty");
    setup.players[1]={8,0,3,0,true};world.players[1]={};s.players[1]={};setup.players[1].handicap=99;
    world.players[0].excess_happiness=77;
    rejects([&]{bind_bnw_unhappiness(s,setup,world,rules,defs);});
    check(world.players[0].excess_happiness==77 && !world.players[1].excess_happiness,"Unresolved player must prevent partial happiness binding");
    setup.players.erase(1);world.players.erase(1);s.players.erase(1);
    p={};p.happiness=5000000100LL;s.world_cities_percent=100;rules.handicaps[0]={100,100,100};world.players[0].cities.clear();
    auto &wide_city=world.players[0].cities[4294967300LL];wide_city.id=4294967300LL;wide_city.owner=0;wide_city.population=5000000000LL;
    p.cities[4294967300LL].occupied=true;
    q=bnw_unhappiness(s,setup,world,rules,defs,0);
    check(q.occupied_population_times100==670000000000LL && q.total==6700000005LL && q.excess_happiness==-1699999905LL,
          "Occupied city and total unhappiness retained 32-bit capacity");
    wide_city.population=INT64_MAX;
    rejects([&]{bnw_unhappiness(s,setup,world,rules,defs,0);});
    wide_city.population=0;
    p.cities[4294967300LL].occupied=false;p.half_specialists=true;p.cities[4294967300LL].specialists={{4294967300LL,3}};
    check(bnw_unhappiness(s,setup,world,rules,defs,0).normal_population_times100==-200,"Wide specialist IDs must count without source array capacities");
    db.apply_sql("UPDATE Defines SET Value=1.01 WHERE Name='UNHAPPINESS_PER_OCCUPIED_POPULATION'");
    const auto float_defs=db.cache_global_defines(GlobalDefineProfile::bnw);
    wide_city.population=1;p.half_specialists=false;p.cities[4294967300LL].occupied=true;
    check(bnw_unhappiness(s,setup,world,rules,float_defs,0).occupied_population_times100==101,
        "Occupied per-population multiply must round at float precision BEFORE promotion to double");
    db.apply_sql("UPDATE Defines SET Value=1.34 WHERE Name='UNHAPPINESS_PER_OCCUPIED_POPULATION'");
    world.profile=TechnologyProfile::vp542;rejects([&]{bnw_unhappiness(s,setup,world,rules,defs,0);});
    // Inhabited saved-state restoration: local real BNW controls are turn zero.
    using namespace civ5::saves;
    constexpr auto layout=GameStateLayout::expansion2_version1;
    SavedSessionSnapshot snap{};snap.game_state.layout=layout;snap.pregame.emplace();snap.pregame->layout=layout;
    snap.pregame->world.values.integers["num_cities_unhappiness_percent"]=60;
    snap.all_players.emplace();snap.all_players->layout=layout;
    SavedPlayerObservation saved{};saved.saved_player_id=4294967310LL;
    saved.core.layout=saved.traits.layout=saved.trade_culture.layout=saved.cities.layout=layout;
    for(const auto &[name,v]:std::vector<std::pair<const char *,std::int64_t>>{
        {"m_iHappiness",5000000000LL},{"m_iUnhappinessFromUnits",199},{"m_iUnhappinessFromUnitsMod",50},
        {"m_iUnhappinessMod",-20},{"m_iCityCountUnhappinessMod",-10},{"m_iOccupiedPopulationUnhappinessMod",-25},
        {"m_iCapitalUnhappinessMod",-10},{"m_iHalfSpecialistUnhappinessCount",1}}) {
        PlayerCoreField f{};f.name=name;f.values={v};saved.core.fields.push_back(f);
    }
    for(const auto &[name,v]:std::vector<std::pair<const char *,std::int64_t>>{
        {"m_iCityUnhappinessModifier",20},{"m_iPopulationUnhappinessModifier",10}}) {
        PlayerTraitsField f{};f.name=name;f.values={v};saved.traits.fields.push_back(f);
    }
    PlayerTradeCultureRecord culture{};culture.name="player_culture";
    culture.fields={{"version",3},{"m_iOpinionUnhappiness",3}};saved.trade_culture.blocks={culture};
    SavedCityObservation sc{};sc.core.layout=sc.management.layout=layout;
    for(const auto &[name,v]:std::vector<std::pair<const char *,std::int64_t>>{
        {"m_eOwner",4294967310LL},{"m_iID",4294967300LL},{"m_bOccupied",1},{"m_iNoOccupiedUnhappinessCount",0}}) {
        CityCoreField f{};f.name=name;f.values={v};sc.core.fields.push_back(f);
    }
    CityManagementBlock citizens{};citizens.name="citizens";TeamHashedArray counts{};counts.name="m_aiSpecialistCounts";
    counts.rows={{saved_info_type_hash("DEFAULT"),{100}},{saved_info_type_hash("SCIENTIST"),{9}},
        {saved_info_type_hash("WIDE"),{2}},{saved_info_type_hash("SCIENTIST"),{0}},
        {saved_info_type_hash("UNKNOWN"),{7}},{0,{}}};
    citizens.hashes={counts};sc.management.blocks={citizens};saved.cities.cities={sc};snap.all_players->players={saved};
    ResearchContext restored_setup;restored_setup.players[4294967310LL]={7,0,3,0,true};restored_setup.game_handicap=1;
    CityYieldContext restored_world;auto &rc=restored_world.players[4294967310LL].cities[4294967300LL];
    rc.owner=4294967310LL;rc.id=4294967300LL;rc.population=5;
    const auto types=db.cache_info_types();
    auto restored=restore_bnw_unhappiness(snap,restored_setup,restored_world,rules,types);
    const auto &rp=restored.players.at(4294967310LL);const auto &extra=rp.cities.at(4294967300LL);
    check(restored.world_cities_percent==60 && rp.happiness==5000000000LL && rp.half_specialists &&
        rp.public_opinion==3 && extra.occupied && !extra.no_occupied_unhappiness && !extra.ignore &&
        extra.specialists==YieldValues{{0,100},{1,0},{4294967300LL,2}},
        "Saved unhappiness inputs, specialist duplicate zero/unknown hash or wide identities differ");
    q=bnw_unhappiness(restored,restored_setup,restored_world,rules,defs,4294967310LL);
    check(q.captured_cities_times100==330 && q.occupied_population_times100==402 && q.units_times100==268 &&
        q.total==13 && q.excess_happiness==4999999987LL,"Restored occupied city calculation differs");
    auto &saved_player=snap.all_players->players[0];
    saved_player.trade_culture.blocks[0].fields={{"version",2}};
    restored=restore_bnw_unhappiness(snap,restored_setup,restored_world,rules,types);
    check(restored.players.at(4294967310LL).public_opinion==0,"Pre-v3 culture must retain reset public opinion");
    saved_player.trade_culture.blocks[0].fields={{"version",3}};
    rejects([&]{restore_bnw_unhappiness(snap,restored_setup,restored_world,rules,types);});
    saved_player.trade_culture.blocks={culture};
    saved_player.cities.cities[0].management.blocks[0].hashes[0].rows[0].values={1,2};
    rejects([&]{restore_bnw_unhappiness(snap,restored_setup,restored_world,rules,types);});
    saved_player.cities.cities={sc};
    auto outside_rules=rules;outside_rules.specialist_slots=2;
    restored=restore_bnw_unhappiness(snap,restored_setup,restored_world,outside_rules,types);
    check(!restored.players.at(4294967310LL).cities.at(4294967300LL).specialists.contains(4294967300LL),
        "Out-of-domain source specialist hash must be ignored");
    saved_player.cities.cities.push_back(sc);
    rejects([&]{restore_bnw_unhappiness(snap,restored_setup,restored_world,rules,types);});saved_player.cities.cities.pop_back();
    saved_player.cities.layout=GameStateLayout::vp542_corpus;
    rejects([&]{restore_bnw_unhappiness(snap,restored_setup,restored_world,rules,types);});saved_player.cities.layout=layout;
    saved_player.core.fields.push_back(saved_player.core.fields.front());
    rejects([&]{restore_bnw_unhappiness(snap,restored_setup,restored_world,rules,types);});saved_player.core.fields.pop_back();
    restored_world.players[4294967310LL].cities[99]={};
    rejects([&]{restore_bnw_unhappiness(snap,restored_setup,restored_world,rules,types);});
}
