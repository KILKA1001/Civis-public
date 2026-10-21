#pragma once
void improvement_yield_calculation_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('MIN_WATER_SIZE_FOR_OCEAN',1),('LAKE_MAX_AREA_SIZE',0),('BARBARIAN_CAMP_IMPROVEMENT',8);"
        "CREATE TABLE Improvements(ID,Type);INSERT INTO Improvements VALUES(0,'FARM');"
        "CREATE TABLE Yields(ID,Type);INSERT INTO Yields VALUES(0,'FOOD');"
        "CREATE TABLE Beliefs(ID,Type,Pantheon);INSERT INTO Beliefs VALUES(7,'MAIN',0),(8,'SECOND',1),(9,'FIRST',1);"
        "CREATE TABLE Belief_ImprovementYieldChanges(BeliefType,ImprovementType,YieldType,Yield);"
        "INSERT INTO Belief_ImprovementYieldChanges VALUES('MAIN','FARM','FOOD',2),('MAIN','FARM','FOOD',151),('SECOND','FARM','FOOD',163),('FIRST','FARM','FOOD',157);");
    const auto types=db.cache_info_types();const auto beliefs=load_improvement_religion_rules(db);
    check(beliefs.beliefs.at(7).improvements.at(0).at(0)==151&&beliefs.beliefs.at(9).pantheon,"Belief yield relations use last row assignment and pantheon flags");
    for(const auto profile:{TechnologyProfile::bnw,TechnologyProfile::vp542}){
        const bool vp=profile==TechnologyProfile::vp542;const auto layout=vp?GameStateLayout::vp542_corpus:GameStateLayout::expansion2_version1;
        auto defs=db.cache_global_defines(vp?GlobalDefineProfile::vp542:GlobalDefineProfile::bnw);
        ResearchContext players;players.profile=profile;players.players[5]={9,-1,2,0,true,{},true};
        PlayerResearchContext roster;roster.profile=profile;roster.teams[9].members={5};
        std::vector<TeamTechnologies> techs;techs.emplace_back(profile,9,2);
        ResourceVisibilityRules resource_rules;resource_rules.profile=profile;resource_rules.resources[8]={-1,-1};
        ResourceVisibilityState resources;resources.profile=profile;resources.teams[9]={};
        if(vp){auto &visibility_profile=ensure_vp_resource_visibility_profile(resources);visibility_profile.observers[9]=false;visibility_profile.alternate_technologies[5]={};}
        SavedMapProjection map{{5,5}};PlotPassability pass;pass.layout=layout;
        for(std::uint64_t i=0;i<25;++i){SavedMapCellProjection p;p.index=i;p.x=i%5;p.y=i/5;p.plot_type=2;p.terrain_type=0;p.feature_type=-1;p.resource_type=-1;p.improvement_type=-1;
            p.improvement_pillaged=false;p.owner=5;p.artifact_era=1;p.trade_route_units=0;p.river_crossing_bits=0;p.rivers=SavedRiverPlot{};p.landmass=0;
            p.water_cache=PlotWaterCache{};p.route=SavedRoutePlot{};p.yield_city=SavedPlotCityReference{};p.plot_city=SavedPlotCityReference{};p.force_resource_reveal.emplace();
            map.cells.push_back(p);pass.plots.push_back({});}
        auto &center=map.cells[12];center.plot_type=1;center.resource_type=8;center.improvement_type=0;center.route->type=0;center.route->pillaged=true;
        center.rivers->is_river=true;center.water_cache->fresh=true;center.water_cache->adjacent_water=true;center.trade_route_units=1;center.yield_city=SavedPlotCityReference{5,44};
        map.cells[13].is_city=vp;map.cells[13].plot_type=0;map.cells[13].plot_city=SavedPlotCityReference{5,44};map.cells[13].owner=99;
        map.cells[16].terrain_type=7;map.cells[17].plot_type=3;map.cells[17].terrain_type=5;map.cells[17].landmass=1;
        map.cells[11].feature_type=3;map.cells[11].resource_type=8;map.cells[11].improvement_type=1;
        CityYieldContext cities;cities.profile=profile;cities.yield_slots=1;cities.players[5].golden_age=true;cities.players[5].second_pantheon_count=1;
        auto &city=cities.players[5].cities[44];city.id=44;city.owner=5;city.x=3;city.y=2;city.we_love_king_turns=2;city.religions={{1,6,20},{2,4,30}};
        cities.religions={{1,5,{7,7}},{2,5,{9,8}}};
        PlotWaterRules water_rules;water_rules.profile=profile;PlotWaterState water_state;water_state.layout=layout;water_state.landmasses[0]={24,false};water_state.landmasses[1]={1,true};
        CustomModOptions options;options.entries["BALANCE_VP"].value=1;
        PlotWaterContext water{map,water_state,water_rules,types,pass,players,cities,defs,vp?&options:nullptr};
        ResourceVisibilityContext visibility{map,resources,resource_rules,players,roster,techs,types,defs,false};
        ImprovementYieldRules rules;rules.profile=profile;rules.technology_slots=2;rules.policy_slots=2;rules.route_slots=2;
        auto &info=rules.improvements[0];const auto put=[&](ImprovementYieldKind k,std::int64_t n){info.yields[static_cast<std::size_t>(k)][0]=n;};
        put(ImprovementYieldKind::base,1);put(ImprovementYieldKind::per_era,2);put(ImprovementYieldKind::adjacent_city,11);put(ImprovementYieldKind::adjacent_mountain,13);
        put(ImprovementYieldKind::river,19);put(ImprovementYieldKind::coastal,23);put(ImprovementYieldKind::hills,29);put(ImprovementYieldKind::fresh_water,31);
        put(ImprovementYieldKind::golden_age,41);put(ImprovementYieldKind::wltkd,43);info.resources[8][0]=37;info.features[3][0]=47;info.adjacent_features[3][0]=53;info.adjacent_resources[8][0]=59;
        info.routes[0][0]=71;info.routes[1][0]=89;rules.routes[0][0]=73;
        info.technologies[0][0]=113;info.technologies[1][0]=127;info.fresh_technologies[0][0]=131;info.dry_technologies[0][0]=137;
        rules.policies[0][0][0]=139;rules.policies[1][0][0]=149;
        info.fractional_improvements[0][1]=YieldFraction(1,2);for(const auto terrain:{0,5,7})info.fractional_terrains[0][terrain]=YieldFraction(1,12);
        ImprovementYieldState state;state.profile=profile;state.players[5].own[0][0]=97;state.players[5].traits[0][0]=101;
        state.teams[9][0][0][0]=103;state.teams[9][1][0][0]=109;state.teams[9][2][0][0]=107;
        if(vp){auto &connections=ensure_vp_improvement_yield_profile(state).players[5];
            connections.connections={12};connections.industrial_connections={12};}
        std::map<std::int64_t,std::int64_t> eras{{9,5}};
        ImprovementYieldContext c{rules,state,water,visibility,eras,&beliefs};
        const auto query=[&](std::int64_t player=5,bool optimal=false,std::optional<std::int64_t> forced={}){
            return vp?vp_improvement_yield(c,12,0,0,player,0,3,8,&city,optimal,forced):bnw_improvement_yield(c,12,0,0,player,optimal,forced);};
        auto q=query();check(q.era==8&&q.coast==23&&q.hills==29&&q.adjacent_city==11&&q.resource==37,"Improvement geography uses actual neighboring city owner and visible resource");
        check(q.river==(vp?19:0)&&q.adjacent_mountain==(vp?26:0)&&q.fresh==(vp?31:0),"BNW/VP river, mountain-city and ordinary fresh-water branches differ");
        if(vp){check(q.fractional==1&&q.adjacent_features==53&&q.adjacent_resources==59&&q.feature==47&&q.golden_age==41&&q.wltkd==43,
                "VP full improvement helper includes fractional adjacency, feature/resource, golden age and WLTKD");
            check(q.trade_route==89&&q.connection==89&&q.route_intrinsic==73&&q.player==0&&q.traits==0&&q.team==0,
                "VP trade and industrial route bonuses stack while player/team modifiers belong to another helper");
            options.entries["BALANCE_VP"].value=0;q=query(5,false,1);check(q.trade_route==0&&q.connection==71&&q.route_intrinsic==73,"CP-only forced railroad falls back to road bonus");
            options.entries["BALANCE_VP"].value=1;q=query(5,false,-1);check(q.trade_route==0&&q.connection==0&&q.route_intrinsic==73,"Forced NO_ROUTE disables connections but preserves actual route's own yield");
            q=query(5,false,2);check(q.trade_route==0&&q.connection==0&&q.route_intrinsic==73,"VP ROUTE_ANY=2 differs from automatic NUM_ROUTE_TYPES=3");
            q=vp_improvement_yield(c,12,0,0,5,-1,3,8,&city,false,0);check(q.connection==71&&q.route_intrinsic==0,"Forced road supplies connection without a physical route");
            auto &connections=ensure_vp_improvement_yield_profile(state).players[5];
            connections.industrial_connections={12,1,13};connections.connections={12,1,13};
            check(query().connection==0,"Connection query preserves native binary-search behavior on unsorted saved vectors");
            connections.connections={12};connections.industrial_connections={12};
            center.owner=99;q=query();check(q.adjacent_features==0&&q.adjacent_resources==0&&q.fractional==1,"VP ownership gate applies only to feature/resource adjacency");center.owner=5;
            map.cells[11].improvement_pillaged=true;check(query().fractional==0,"Live adjacent pillage removes fractional improvement contribution");map.cells[11].improvement_pillaged=false;
        }else{check(q.player==97&&q.traits==101&&q.team==103&&q.team_water==107&&q.religion==308&&q.route==71&&q.total()==896,
                "BNW full formula reads saved player/team bonuses, deduplicated majority beliefs and first secondary pantheon");
            cities.religions[1].beliefs={8,9};check(query().religion==314,"Secondary pantheon follows source belief order, not sorted Info ID");cities.religions[1].beliefs={9,8};
            center.river_crossing_bits=2;check(query().river==19,"BNW river-side uses crossing bits rather than crossing count");center.river_crossing_bits=0;
            center.route->pillaged=false;check(query().route==71,"BNW helper does not filter pillaged routes itself");
            check(query(5,false,-1).route==0&&query(5,false,1).route==89,"BNW explicit hypothetical route replaces actual route");
        }
        q=query(5,true);check(q.route==89&&q.technologies==371&&q.policies==288&&q.fresh==31&&q.player==0&&q.team==0&&q.trade_route==0&&q.connection==0,
            "Optimal adds every technology/policy and best nonnegative route using actual fresh/dry status");
        if(!vp)check(q.total()==1196,"BNW optimal full sum includes resource and working-city religion");
        q=query(-1);check(q.era==0&&q.resource==0&&q.fractional==0&&q.technologies==371&&q.policies==288,"NO_PLAYER omits player terms but adds all technology and policy terms");
        center.artifact_era=-1;check(query().era==0,"No archaeological era suppresses per-era yields");center.artifact_era=1;
        put(ImprovementYieldKind::per_era,-2);check(query().era==0,"Nonpositive per-era yield never multiplies archaeological age");put(ImprovementYieldKind::per_era,2);
        put(ImprovementYieldKind::adjacent_city,-11);check(query().adjacent_city==-11,"Positive mountain bonus activates loop including negative city bonus");
        put(ImprovementYieldKind::adjacent_mountain,-13);check(query().adjacent_city==0&&query().adjacent_mountain==0,"Both nonpositive adjacency bonuses skip the source loop");
        put(ImprovementYieldKind::base,-10000);check(query().total()<0,"Improvement helper preserves negative totals for the outer plot calculation");
        put(ImprovementYieldKind::base,4294967300LL);check(query().total()>UINT32_MAX,"Complete improvement sum must not narrow to int32");
        put(ImprovementYieldKind::base,INT64_MAX);rejects([&]{query();});
        check((vp?vp_improvement_yield(c,-99,999,0,-1,-1,-1,-1):bnw_improvement_yield(c,-99,999,0)).total()==0,"Missing improvement Info returns zero before touching plot inputs");
    }
    // Imported matrices have different wire layouts. Use asymmetric rows to
    // detect swapped improvement/yield/water axes and preserve zero overwrites.
    constexpr std::int64_t large=4294967300LL;
    for(const auto layout:{GameStateLayout::expansion2_version1,GameStateLayout::vp542_corpus}){
        const bool vp=layout==GameStateLayout::vp542_corpus;const auto profile=vp?TechnologyProfile::vp542:TechnologyProfile::bnw;
        SavedSessionSnapshot snapshot;snapshot.teams.emplace();snapshot.teams->layout=layout;snapshot.all_players.emplace();snapshot.all_players->layout=layout;
        TeamObservation team;team.fields.push_back({"id",large});
        if(vp)team.arrays.push_back({"improvement_yields",6,{1,2,3,4,5,6,7,8,9,10,11,12}});
        else{const char *names[]={"improvement_yields","improvement_dry_yields","improvement_fresh_yields"};
            for(int k=0;k<3;++k)team.hashes.push_back({names[k],3,{{0,{}},{saved_info_type_hash("FARM"),{90,91}},
                {saved_info_type_hash("FARM"),{0,large+k}}}});}
        snapshot.teams->teams.push_back(team);
        SavedPlayerObservation p;p.saved_player_id=large;p.core.layout=p.traits.layout=p.administration.layout=p.planning.layout=layout;p.core.yield_slots=2;
        PlayerTraitsField traits{"m_ppaaiImprovementYieldChange",PlayerTraitsEncoding::yield_rows};traits.columns=2;traits.values={0,large,3,4};p.traits.fields.push_back(traits);
        PlayerCoreField pantheon{"m_iSecondReligionPantheonCount",PlayerCoreEncoding::i32};pantheon.values={large};p.core.fields.push_back(pantheon);
        if(vp){PlayerCoreField own{"m_ppaaiImprovementYieldChange",PlayerCoreEncoding::yield_rows};own.values={5,6,0,large};p.core.fields.push_back(own);
            PlayerPlanningRecord connections{"city_connections"};connections.arrays={{"plots_with_connection_to_capital",1,{large,1,large}},{"plots_with_industrial_connection_to_capital",1,{9,3}}};p.planning.blocks.push_back(connections);
        }else{PlayerAdministrationRecord own{"pre_city"};own.arrays.push_back({"m_ppaaiImprovementYieldChange",2,{5,6,0,large}});p.administration.blocks.push_back(own);}
        SavedCityObservation saved_city;CityCoreField id{"m_iID",CityCoreEncoding::i32},wltkd{"m_iWeLoveTheKingDayCounter",CityCoreEncoding::i32};id.values={large};wltkd.values={large+1};
        saved_city.core.fields={id,wltkd};p.cities.cities.push_back(saved_city);snapshot.all_players->players.push_back(p);
        CityYieldContext cities;cities.profile=profile;cities.yield_slots=2;cities.players[large].cities[large]={};
        ImprovementYieldRules rules;rules.profile=profile;rules.improvement_slots=2;
        const auto state=restore_improvement_yield_state(snapshot,rules,types,cities);
        check(state.players.at(large).traits.at(0).at(1)==large&&state.players.at(large).own.at(1).at(1)==large&&
            !state.players.at(large).own.at(1).contains(0),"Imported player/trait matrices retain wide values and sparse zeros");
        check(cities.players.at(large).second_pantheon_count==large&&cities.players.at(large).cities.at(large).we_love_king_turns==large+1,
            "Import restores counters into shared cities and players with wide IDs");
        if(vp){check(state.teams.at(large)[0].at(1).at(0)==7&&state.teams.at(large)[1].at(0).at(1)==4&&state.teams.at(large)[2].at(1).at(1)==12,
                "VP team cube is improvement then base/dry/fresh then yield");
            const auto &connections=vp_improvement_yield_player(state,large);
            check(connections.connections==std::vector<std::int64_t>{large,1,large}&&connections.industrial_connections==std::vector<std::int64_t>{9,3},
                "Connection restore retains source order and duplicates");
        }else{check(!state.teams.at(large)[0].at(0).contains(0)&&state.teams.at(large)[0].at(0).at(1)==large&&state.teams.at(large)[2].at(0).at(1)==large+2,
            "BNW repeated hash replaces the entire prior row, including zero values");
            check(!state.profile_extensions.find<VpImprovementYieldProfileState>(),
                "BNW improvement state must not acquire VP capital connections");}
        auto broken=snapshot;broken.all_players->players[0].traits.fields[0].values.pop_back();rejects([&]{restore_improvement_yield_state(broken,rules,types,cities);});
        broken=snapshot;broken.teams->teams.push_back(team);rejects([&]{restore_improvement_yield_state(broken,rules,types,cities);});
        if(!vp){broken=snapshot;broken.teams->teams[0].hashes[0].rows.back().type_hash=saved_info_type_hash("MISSING_IMPROVEMENT");
            rejects([&]{restore_improvement_yield_state(broken,rules,types,cities);});}
    }
}
