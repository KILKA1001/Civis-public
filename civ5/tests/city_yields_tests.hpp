#pragma once
void city_yields_tests() {
    using namespace civ5::runtime;
    RulesDatabase db;
    db.apply_sql("CREATE TABLE Defines(Name,Value); INSERT INTO Defines VALUES"
        "('PUPPET_SCIENCE_MODIFIER',-25),('PUPPET_GOLD_MODIFIER',-25),('CITY_MAX_NUM_BUILDINGS',1),"
        "('VERY_UNHAPPY_PRODUCTION_PENALTY_PER_UNHAPPY',-2),('VERY_UNHAPPY_MAX_PRODUCTION_PENALTY',-40),"
        "('VERY_UNHAPPY_GOLD_PENALTY_PER_UNHAPPY',-2),('VERY_UNHAPPY_MAX_GOLD_PENALTY',-40);"
        "CREATE TABLE Yields(ID,Type,GoldenAgeYieldMod); INSERT INTO Yields VALUES(1,'PROD',0),(3,'SCI',5),(4294967300,'WIDE',7);"
        "CREATE TABLE Beliefs(ID,Type); INSERT INTO Beliefs VALUES(0,'A'),(1,'B');"
        "CREATE TABLE Belief_MaxYieldModifierPerFollower(BeliefType,YieldType,Max);"
        "INSERT INTO Belief_MaxYieldModifierPerFollower VALUES('A','SCI',4),('A','SCI',5),('B','SCI',3);"
        "CREATE TABLE Buildings(ID,Type,CityStateTradeRouteProductionModifier); INSERT INTO Buildings VALUES(1,'B1',5),(2,'B2',9);"
        "CREATE TABLE BuildingClasses(ID,Type,DefaultBuilding); INSERT INTO BuildingClasses VALUES(0,'BC0','B1'),(4,'BC4','B2');"
        "CREATE TABLE Civilizations(ID,Type); INSERT INTO Civilizations VALUES(0,'CIV'),(2,'CIV2');"
        "CREATE TABLE Civilization_BuildingClassOverrides(CivilizationType,BuildingClassType,BuildingType);"
        "INSERT INTO Civilization_BuildingClassOverrides VALUES('CIV2','BC4',NULL);");
    auto rules=load_city_yield_rules(db,TechnologyProfile::bnw);
    const auto defs=db.cache_global_defines(GlobalDefineProfile::bnw);
    check(rules.golden_age_modifiers.at(4294967300LL)==7 && rules.belief_follower_maximum.at(0).at(3)==5,
        "Yield Info slot or linked last-write semantics narrowed/changed");
    check(rules.civilization_buildings.at(2).at(4)==-1 && rules.civilization_buildings.at(0).at(4)==2,
        "Civilization building default/NULL override differs");
    CityYieldContext state; state.yield_slots=4294967301ULL;
    auto &player=state.players[0]; player.capital=10; player.civilization=0;
    auto &city=player.cities[10]; city.id=10; city.owner=0; city.x=2; city.y=3; city.area=5; city.population=7;
    auto &science=city.yields[3];
    science.terrain=1; science.buildings=2; science.specialists=3; science.misc=4; science.religion=5;
    science.per_population=33; science.per_religion=17; science.rate_modifier=10; science.resource_modifier=-3;
    city.religions={{-1,2,1},{0,1,3},{1,4,10},{2,1,20}};
    state.religions={{1,-1,{0,1}}}; // half the total followers; max 8, actual first matching row 4
    state.area_modifiers[{5,0}][3]=11; player.modifiers[3]=13; player.capital_modifiers[3]=7; player.golden_age=true;
    civ5::saves::TradeRouteObservation route; route.origin_owner=route.destination_owner=0;
    route.origin_x=route.destination_x=2; route.origin_y=route.destination_y=3;
    route.origin_yields={0,21,0,29,0,0}; route.destination_yields={0,13,0,-7,0,0};
    state.trade_routes={route};
    auto q=city_yield_times100(state,rules,defs,0,10,3);
    check(q.base_times100==1765 && q.modifier==147 && q.trade_times100==22 && q.total_times100==2616,
        "BNW city population/religion/modifiers/trade calculation differs");
    city.puppet=true;
    q=city_yield_times100(state,rules,defs,0,10,3,true);
    check(q.modifier==97 && q.total_times100==1712 && q.trade_times100==0,"BNW science puppet must include gold switch fall-through");
    city.resistance=true;
    q=city_yield_times100(state,rules,defs,0,10,3);
    check(q.suppressed && q.total_times100==0,"Resistance must suppress science before process/modifiers/trade");
    city.resistance=false; city.razing=true;
    check(city_yield_times100(state,rules,defs,0,10,3).suppressed,"Razing must suppress BNW science");
    city.razing=false; city.puppet=false;
    science.process_modifier=25; city.yields[1].terrain=8;
    rejects([&]{city_yield_times100(state,rules,defs,0,10,3);});
    player.excess_happiness=-30; // explicitly supplied by a live happiness subsystem in a full world
    q=city_yield_times100(state,rules,defs,0,10,3,true);
    check(q.process_times100==128 && q.total_times100==2722,"Science process must use production WITH trade and capped unhappiness");
    state.players[1].minor=true;
    auto minor_route=route; minor_route.origin_owner=0; minor_route.destination_owner=1;
    minor_route.origin_x=minor_route.origin_y=minor_route.destination_x=minor_route.destination_y=-1;
    state.trade_routes.push_back(minor_route); // empty still counts for Hanse's separate getter
    city.real_buildings={{1,1},{2,1}};
    check(city_yield_times100(state,rules,defs,0,10,1).modifier==69,"Hanse last matching class must replace, not sum, and count empty routes");
    player.civilization=2;
    check(city_yield_times100(state,rules,defs,0,10,1).modifier==65,"NULL civilization override must remove building contribution");
    player.civilization=0;
    city.real_buildings[2]=-1; city.free_buildings[2]=1;
    check(city_yield_times100(state,rules,defs,0,10,1).modifier==69,"CITY_MAX_NUM_BUILDINGS <= 1 uses max(real,free)");
    db.apply_sql("UPDATE Defines SET Value=2 WHERE Name='CITY_MAX_NUM_BUILDINGS'");
    const auto sum_defs=db.cache_global_defines(GlobalDefineProfile::bnw);
    check(city_yield_times100(state,rules,sum_defs,0,10,1).modifier==65,"Stackable buildings use real+free count");
    state.no_happiness=true; player.excess_happiness.reset();
    check(city_yield_times100(state,rules,defs,0,10,1).modifier==109,"NO_HAPPINESS must bypass unresolved happiness");
    city.yields[1].process_modifier=1;
    rejects([&]{city_yield_times100(state,rules,defs,0,10,1);});
    city.yields[1].process_modifier=0; science.process_modifier=0;
    science.rate_modifier=-1000;
    q=city_yield_times100(state,rules,defs,0,10,3);
    check(q.modifier==0 && q.total_times100==22,"Modifier clamps at zero but trade stays outside it");
    // Pressure selects a different religion when follower counts tie; count
    // includes NO_RELIGION, and exactly half is sufficient. Duplicate belief
    // membership is a set, while duplicate city religion records stay ordered.
    science={}; science.terrain=1; player.golden_age=false; player.modifiers.clear(); player.capital_modifiers.clear();
    state.area_modifiers.clear(); state.trade_routes.clear();
    city.religions={{1,4,10},{2,4,20}}; state.religions={{1,-1,{0}},{2,-1,{1}}};
    check(city_yield_times100(state,rules,defs,0,10,3).modifier==103,"Majority must break follower tie by pressure");
    city.religions={{2,1,0},{2,4,10},{-1,3,0}};
    check(city_yield_times100(state,rules,defs,0,10,3).modifier==101,"Follower bonus must use first duplicate religion row");
    city.religions={{0,7,0}}; state.religions={{0,1,{1}},{0,0,{0}}};
    check(city_yield_times100(state,rules,defs,0,10,3).modifier==105,"Pantheon religion must match the city's owner");
    city.religions={{1,2,0},{-1,7,0}}; state.religions={{1,-1,{0}}};
    check(city_yield_times100(state,rules,defs,0,10,3).modifier==100,"Non-majority religion must not contribute");
    city.yields[4294967300LL].terrain=5000000000LL;
    check(city_yield_times100(state,rules,defs,0,10,4294967300LL,true).total_times100==500000000000LL,"City yield IDs/values retained x86 capacity");
    science.terrain=INT64_MAX;
    rejects([&]{city_yield_times100(state,rules,defs,0,10,3);});
    science.terrain=1;
    check(science_from_cities_times100(state,rules,defs,0)==100,"City science aggregation differs");
    // VP's cached overload does not re-run resistance/puppet/Anarchy checks.
    auto vp_rules=load_city_yield_rules(db,TechnologyProfile::vp542);
    const auto vp_defs=db.cache_global_defines(GlobalDefineProfile::vp542);
    rejects([&]{vp_city_cached_yields(state,0,10)[3]=1;});
    check(!state.extensions.find<VpCityYieldCacheProfileState>(),"BNW city must not acquire VP GameCore cache");
    state.profile=TechnologyProfile::vp542; city.resistance=city.razing=city.puppet=true;
    vp_city_cached_yields(state,0,10)[3]=4294967400LL;
    q=city_yield_times100(state,vp_rules,vp_defs,0,10,3);
    check(q.cached && !q.suppressed && q.total_times100==4294967400LL && q.base_times100==0,"VP default getter must return raw wide cached yield");
    rejects([&]{city_yield_times100(state,vp_rules,vp_defs,0,10,3,true);});
    rejects([&]{city_yield_times100(state,rules,defs,0,10,3);});
    rejects([&]{city_yield_times100(state,vp_rules,vp_defs,0,10,-1);});
    check(science_from_cities_times100(state,vp_rules,vp_defs,0)==4294967400LL,"VP city science must use its cached overload");
    auto copied=state;vp_city_cached_yields(copied,0,10)[3]=5;
    check(vp_city_cached_yield(state,0,10,3)==4294967400LL &&
          vp_city_cached_yield(copied,0,10,3)==5,"VP cache must copy with its owning context");
    // Exercise restoration with an inhabited BNW fixture: local original BNW
    // control saves begin before any cities have been founded.
    db.apply_sql("CREATE TABLE GameOptions(Type,\"Default\");INSERT INTO GameOptions VALUES('GAMEOPTION_NO_HAPPINESS',0);");
    const auto types=db.cache_info_types();
    for(bool is_vp:{false,true}) {
        using namespace civ5::saves;
        const auto layout=is_vp?GameStateLayout::vp542_corpus:GameStateLayout::expansion2_version1;
        SavedSessionSnapshot snap{};snap.game_state.layout=layout;snap.trade.layout=layout;snap.trade.dimensions.yield_slots=6;
        TradeRouteObservation imported_route;imported_route.id=4294967301LL;imported_route.origin_owner=imported_route.destination_owner=0;
        imported_route.origin_x=imported_route.origin_y=imported_route.destination_x=imported_route.destination_y=-1;
        imported_route.origin_yields=imported_route.destination_yields=std::vector<std::int64_t>(6);imported_route.plots={{4294967302LL,4294967303LL}};snap.trade.routes={imported_route};
        snap.pregame.emplace();snap.pregame->layout=layout;snap.pregame->slots.integer_arrays["civilizations"]={0};
        snap.pregame->archive.flag_arrays["minor_nation_civs"]={false};
        snap.map_header.grid={1,1};snap.map_plots.emplace();snap.map_plots->layout=layout;
        PlotObservation plot{};plot.fields={{"area",4294967400LL}};snap.map_plots->plots.push_back(plot);
        snap.map_regions.emplace();snap.map_regions->layout=layout;snap.map_regions->dimensions=RegionWireDimensions{1,1,1,6};
        MapRegionRecord area{};area.fields={{"id",4294967400LL}};area.arrays={{"yield_modifiers",6,{0,0,0,10,0,0}}};
        MapRegionContainer areas{};areas.name="areas";areas.records.push_back(area);snap.map_regions->containers.push_back(areas);
        ReligionObservation religion{};religion.religion=1;religion.founder=0;
        religion.beliefs.belief_hashes={saved_info_type_hash("A"),saved_info_type_hash("A"),saved_info_type_hash("UNKNOWN")};
        snap.religion_culture.religions.push_back(religion);
        snap.all_players.emplace();snap.all_players->layout=layout;
        SavedPlayerObservation saved{};saved.saved_player_id=0;saved.core.layout=layout;saved.cities.layout=layout;
        const auto pf=[&](const char *name,std::vector<std::int64_t> v){PlayerCoreField f{};f.name=name;f.values=std::move(v);saved.core.fields.push_back(std::move(f));};
        pf("m_iCapitalCityID",{4294967300LL});pf("m_iGoldenAgeTurns",{1});pf("m_aiYieldRateModifier",{0,0,0,20,0,0});pf("m_aiCapitalYieldRateModifier",{0,0,0,30,0,0});
        if(!is_vp)pf("m_iTotalPopulation",{4}); // explicit native Read counter for the inhabited BNW record
        SavedCityObservation saved_city{};saved_city.core.layout=saved_city.production.layout=saved_city.tail.layout=layout;
        saved_city.core.version=6;saved_city.core.start_offset=10;saved_city.core.end_offset=20;
        const auto cf=[&](const char *name,std::vector<std::int64_t> v){CityCoreField f{};f.name=name;f.values=std::move(v);saved_city.core.fields.push_back(std::move(f));};
        cf("m_eOwner",{0});cf("m_iID",{4294967300LL});cf("m_iX",{0});cf("m_iY",{0});cf("m_iPopulation",{4});
        cf("m_iResistanceTurns",{0});cf("m_iRazingTurns",{0});cf("m_bPuppet",{0});cf("m_aiStaticCityYield",{0,0,0,9876,0,0});
        cf("m_aiBaseYieldRateFromTerrain",{0,0,0,11,0,0});cf("m_aiYieldPerPop",{0,0,0,3,0,0});cf("m_aiYieldPerReligion",{0,0,0,5,0,0});
        for(const auto *name:{"m_aiBaseYieldRateFromBuildings","m_aiBaseYieldRateFromSpecialists","m_aiBaseYieldRateFromMisc",
            "m_aiBaseYieldRateFromReligion","m_aiYieldRateModifier","m_aiResourceYieldRateModifier","m_aiProductionToYieldModifier"})cf(name,{0,0,0,0,0,0});
        CityTailNode rel{};rel.name="religion";
        for(const auto &[name,v]:std::vector<std::pair<const char *,std::int64_t>>{{"m_eReligion",1},{"m_iFollowers",2},{"m_iPressure",10}}){CityTailNode n{};n.name=name;n.values={v};rel.children.push_back(n);}
        CityTailNode status{};status.name="m_ReligionStatus";status.children={rel};CityTailNode block{};block.name="religions";block.children={status};saved_city.tail.blocks={block};
        CityProductionRecord buildings{};buildings.name="buildings";
        for(const auto *name:{"m_paiNumRealBuilding","m_paiNumFreeBuilding"}) {
            CityProductionRecord counts{};counts.name=name;
            for(auto n:{1,0}){CityProductionRecord row{};row.name="row";row.fields={{"hash",saved_info_type_hash("B1")}};row.arrays={{"values",1,{n}}};counts.children.push_back(row);}
            buildings.children.push_back(counts);
        }
        saved_city.production.blocks={buildings};saved.cities.cities={saved_city};snap.all_players->players={saved};
        ResearchContext base;base.profile=is_vp?TechnologyProfile::vp542:TechnologyProfile::bnw;base.players[0]={};
        auto restored=restore_city_yields(snap,base,db,is_vp?vp_rules:rules,types);
        check((restored.extensions.find<VpCityYieldCacheProfileState>()!=nullptr)==is_vp,
              "Only VP restoration may install the VP city yield cache");
        if(!is_vp)check(restored.players.at(0).total_population==4,"BNW import lost saved player total population");
        check(restored.trade_routes.size()==1&&restored.trade_routes[0].id==4294967301LL&&restored.trade_routes[0].plots[0].y==4294967303LL,"Both profiles restore shared routes, including empty slots and wide path coordinates");
        const auto &c=restored.players.at(0).cities.at(4294967300LL);
        check(c.source_core==std::optional(std::pair<std::uint64_t,std::uint64_t>{10,20}) && c.owner==0,"Restored city identity or provenance differs");
        check(science_from_cities_times100(restored,is_vp?vp_rules:rules,is_vp?vp_defs:defs,0)==(is_vp?9876:1865),"Restored city science lost saved components or area/religion/Info binding");
        if(!is_vp) {
            check(c.real_buildings.at(1)==0 && restored.religions[0].beliefs==std::vector<std::int64_t>{0,0},"Building duplicate reset or ordered belief hash restoration differs");
            snap.all_players->players[0].cities.cities[0].core.version=3;
            auto old=restore_city_yields(snap,base,db,rules,types);
            check(science_from_cities_times100(old,rules,defs,0)==1857,"Pre-v4 BNW city retains zero yield-per-religion reset");
            snap.map_regions->dimensions.reset();rejects([&]{restore_city_yields(snap,base,db,rules,types);});snap.map_regions->dimensions=RegionWireDimensions{1,1,1,6};
        } else {
            auto &fields=snap.all_players->players[0].cities.cities[0].core.fields;
            std::size_t cache=0;
            while(cache<fields.size() && std::string_view(fields[cache].name)!="m_aiStaticCityYield")++cache;
            check(cache<fields.size(),"VP cache fixture field missing");
            fields.push_back(fields[cache]);
            rejects([&]{restore_city_yields(snap,base,db,vp_rules,types);});
            fields.pop_back();
            fields[cache].values.pop_back();
            rejects([&]{restore_city_yields(snap,base,db,vp_rules,types);});
            fields[cache].values.push_back(0);
        }
        snap.all_players->players.push_back(saved);rejects([&]{restore_city_yields(snap,base,db,is_vp?vp_rules:rules,types);});
        snap.all_players->players.pop_back();snap.all_players->players[0].cities.cities.push_back(saved_city);
        rejects([&]{restore_city_yields(snap,base,db,is_vp?vp_rules:rules,types);});
    }
}
