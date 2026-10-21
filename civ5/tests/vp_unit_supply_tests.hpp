#pragma once
void vp_unit_supply_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    constexpr auto vp=TechnologyProfile::vp542;constexpr auto layout=GameStateLayout::vp542_corpus;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('AI_HANDICAP',2),('UNIT_SUPPLY_BASE_TECH_REDUCTION_PER_ERA',100),('UNIT_SUPPLY_CITIES_TECH_REDUCTION_MULTIPLIER',50),('UNIT_SUPPLY_POPULATION_TECH_REDUCTION_MULTIPLIER',100),('PUPPET_YIELD_AND_SUPPLY_MODIFIER_MULTIPLICATIVE',30),('UNIT_SUPPLY_WAR_WEARINESS_PERCENT_REDUCTION',50),('MINOR_CIV_UNIT_SUPPLY_MODIFIER_MILITARISTIC',20),('MINOR_CIV_UNIT_SUPPLY_MODIFIER_NEUTRAL',-10),('MINOR_CIV_UNIT_SUPPLY_MULTIPLIER_PER_EXTRA_CITY',50);CREATE TABLE GameOptions(Type,\"Default\");INSERT INTO GameOptions VALUES('GAMEOPTION_NO_CHANGING_WAR_PEACE',0),('GAMEOPTION_ALWAYS_WAR',0),('GAMEOPTION_ALWAYS_PEACE',0);CREATE TABLE HandicapInfos(ID,Type,UnitSupplyBase,ProductionFreeUnits,UnitSupplyPerCity,ProductionFreeUnitsPerCity,UnitSupplyPopulationPercent,ProductionFreeUnitsPopulationPercent,AIUnitSupplyBonusPercent,AIUnitSupplyPercent);INSERT INTO HandicapInfos VALUES(1,'HANDICAP',10,3,2,4,20,5,30,6);CREATE TABLE Eras(ID,Type,UnitSupplyBase);INSERT INTO Eras VALUES(2,'ERA',3);CREATE TABLE MinorCivTraits(ID,Type);INSERT INTO MinorCivTraits VALUES(1,'MILITARISTIC');CREATE TABLE MinorCivilizations(ID,Type,MinorCivTrait);INSERT INTO MinorCivilizations VALUES(7,'MINOR','MILITARISTIC');");
    db.apply_sql("INSERT INTO Defines VALUES('MAX_UNIT_SUPPLY_GROWTH_MOD',70),('GROWTH_PENALTY_PER_UNIT_OVER_SUPPLY',5),('UNHAPPY_THRESHOLD',50);");
    auto types=db.cache_info_types();auto defines=db.cache_global_defines(GlobalDefineProfile::vp542);auto rules=load_vp_unit_supply_rules(db,types);const auto &loaded=rules.handicaps.at(1);
    check(loaded.player.base==13&&loaded.player.city==6&&loaded.player.population==25&&loaded.ai.percent==36&&rules.era_base.at(2)==3&&rules.minor_traits.at(7)==1&&rules.minor_types.at(7)=="MINOR","Supply rules add all four legacy difficulty columns and bind named minor trait");
    {
        using namespace civ5::validation;auto a=rules;a.minor_traits[8]=2;a.minor_types[8]="SECOND";auto b=a;
        b.minor_traits={{4294967300LL,1},{4,2}};b.minor_types={{4294967300LL,"MINOR"},{4,"SECOND"}};
        check(vp_unit_supply_minor_rules_report(a)==vp_unit_supply_minor_rules_report(b),"Randomized minor Info IDs preserve every reported supply rule by Type");
        check(vp_unit_supply_minor_type_report(a,7)==vp_unit_supply_minor_type_report(b,4294967300LL),"Minor player binding survives reshuffled wide Info IDs");
        b.minor_traits[4]=3;check(vp_unit_supply_minor_rules_report(a)!=vp_unit_supply_minor_rules_report(b),"Canonical supply report still detects a changed minor trait");
        check(vp_unit_supply_minor_type_report(a,7)!=vp_unit_supply_minor_type_report(b,4),"Canonical supply report still detects a changed player type");
        check(vp_unit_supply_minor_type_report(a,-1)=="null"&&vp_unit_supply_minor_type_report(a,9)=="{\"unresolved_id\":9}","Unknown minor IDs are not silently normalized away");
    }
    rules.handicaps[1].player={10,2,20,1,5,10};rules.handicaps[1].ai={4,1,10,2,3,20};rules.handicaps[1].minor={6,3,40,1,2,30};rules.handicaps[2].player={20,3,30,2,6,20};
    CustomModOptions options;options.entries["BALANCE_VP"].value=1;
    ResearchContext setup;setup.profile=vp;setup.start_era=2;setup.current_era=2;setup.game_handicap=1;setup.game_turn=10;
    for(auto id:{5LL,6LL}){auto &p=setup.players[id];p.team=id+4;p.handicap=1;p.alive=true;p.slot_status=id==5?3:1;}
    PlayerResearchContext dip;dip.profile=vp;dip.barbarian_team=63;dip.major_players={5,6};dip.civ_teams={9,10};
    for(auto id:{5LL,6LL}){dip.players[id]={};auto &t=dip.teams[id+4];t.alive_count=1;t.members={id};t.war_peace.emplace();}
    dip.teams[9].met={10};dip.teams[9].at_war={10};
    CityYieldContext cities;cities.profile=vp;auto &pc=cities.players[5];auto &city_supply=ensure_vp_city_supply_profile(cities);
    for(auto cid:{42LL,43LL}){auto &city=pc.cities[cid];city.id=cid;city.owner=5;city.population=cid==42?10:7;city.puppet=cid==43;city_supply.cities[{5,cid}]={cid==42?1:2,cid==42?10:20};dip.players[5].cities[cid]={};}
    MaintenanceContext units;units.profile=vp;ensure_vp_unit_supply_profile(units).world_city_supply_modifier=5;units.team_eras[9]=3;units.players[5]={};auto &s=ensure_vp_unit_supply_profile(units).players[5];
    s.military=30;s.free=2;s.extra_flat=1;s.city_flat=1;s.extra_population=5;s.expended=2;s.puppet=10;s.trait_base=2;s.trait_city=1;s.trait_population=5;s.trait_puppet=5;
    s.war_value=500;s.war_modifier=10;s.trait_war_modifier=10;s.war_weariness[6]=100;s.minor_type=7;s.personality=1;
    std::vector<TeamTechnologies> techs;techs.emplace_back(vp,9,10);vp_team_change_known_count(techs.back(),4);
    VpUnitSupplyContext context{units,setup,dip,cities,techs,rules,db,defines,options};
    auto components=vp_unit_supply_components(context,5);check(components.handicap==14&&components.cities==6&&components.population==3&&components.expended==2,"Supply components preserve era, puppet fractions and technology reductions");
    components=vp_unit_supply_components(context,5,true);check(components.handicap==17&&components.cities==8&&components.population==5,"Ignore-reduction mode preserves unmodified component totals");
    auto q=vp_unit_supply(context,5);check(q.base==27&&q.capacity==24&&q.war_percent==8&&q.units==28&&q.over==4&&!q.cached,"Fresh supply applies world, difficulty and war modifiers with intermediate truncation");
    check(vp_unit_supply(context,5,false).capacity==27&&vp_unit_supply(context,5).cached,"Both cache variants are populated by the first query");
    options.entries["BALANCE_VP"].value=0;invalidate_vp_unit_supply(units,5);q=vp_unit_supply(context,5);
    check(q.capacity==27&&q.war_percent==0&&vp_highest_war_weariness_percent(context,5)==0,"Community Patch mode retains supply and omits VP war weariness reduction");
    options.entries["BALANCE_VP"].value=1;invalidate_vp_unit_supply(units,5);vp_unit_supply(context,5);
    pc.cities[42].population=100;check(vp_unit_supply(context,5).capacity==24,"Source supply cache persists until explicit invalidation");invalidate_vp_unit_supply(units,5);check(vp_unit_supply(context,5).capacity>24,"Invalidating supply observes live city population");pc.cities[42].population=10;
    invalidate_vp_unit_supply(units,5);setup.players[5].slot_status=1;q=vp_unit_supply(context,5);check(q.base==49&&q.capacity==45&&q.components.handicap==22&&q.components.cities==8&&q.components.population==4,"AI supply combines own and game difficulty bonuses before scaling");
    setup.players[5].slot_status=3;options.entries["HUMAN_USES_AI_HANDICAP"].value=1;invalidate_vp_unit_supply(units,5);q=vp_unit_supply(context,5);
    check(q.components.handicap==34&&q.empire_modifier==58,"Human-AI option selects AI difficulty entry and AI supply path");options.entries["HUMAN_USES_AI_HANDICAP"].value=0;
    setup.players[5].handicap=999;check(vp_unit_supply_components(context,5).handicap==2,"Missing player handicap retains VP empty Info fallback");setup.players[5].handicap=1;
    setup.start_era=999;check(vp_unit_supply_components(context,5).handicap==11,"Missing start era retains empty EraInfo supply zero");setup.start_era=2;
    check(vp_highest_war_weariness_percent(context,5)==16,"Highest war weariness includes both player and trait modifiers");
    setup.players[6].alive=false;check(vp_highest_war_weariness_percent(context,5)==16,"Weariness has no alive-player filter");
    dip.teams[10].war_peace->locked_turns[9]=1;check(!vp_can_change_war_peace(context,9,10)&&vp_highest_war_weariness_percent(context,5)==0,"Opposing team's locked war excludes that war weariness");
    dip.teams[9].at_war.clear();check(vp_highest_war_weariness_percent(context,5)==16,"Peace-time weariness survives a war-change restriction");dip.teams[9].at_war={10};dip.teams[10].war_peace->locked_turns.clear();
    dip.teams[9].war_peace->permanent.insert(10);check(vp_highest_war_weariness_percent(context,5)==0,"Own permanent-war relation suppresses wartime weariness");dip.teams[9].war_peace->permanent.clear();
    dip.teams[10].war_peace->permanent.insert(9);check(vp_highest_war_weariness_percent(context,5)==0,"Opposing permanent relation is checked too");dip.teams[10].war_peace->permanent.clear();
    dip.teams[9].master=10;check(vp_highest_war_weariness_percent(context,5)==0,"Vassal cannot independently change war or peace");dip.teams[9].master=-1;
    for(const char *name:{"GAMEOPTION_NO_CHANGING_WAR_PEACE","GAMEOPTION_ALWAYS_WAR","GAMEOPTION_ALWAYS_PEACE"}){
        setup.game_options.insert(setup.game_options.begin(),{name,1});check(!vp_can_change_war_peace(context,9,10),"Each global war/peace option is enforced");setup.game_options.erase(setup.game_options.begin());}
    check(setup.game_options.size()==3&&!vp_can_change_war_peace(context,9,9)&&!vp_can_change_war_peace(context,9,-1),"Source lazy game-option defaults and self/NO_TEAM checks are retained");
    dip.teams[9].met.clear();check(vp_highest_war_weariness_percent(context,5)==0,"Unmet opponents do not contribute weariness");dip.teams[9].met={10};
    dip.players[6].pregame_minor=true;check(vp_highest_war_weariness_percent(context,5)==0,"Nonmajor opponent in major source domain is filtered by live team status");dip.players[6].pregame_minor=false;
    s.war_value=0;check(vp_highest_war_weariness_percent(context,5)==100,"War value denominator has a one-unit floor and final percent cap");s.war_value=500;
    s.war_modifier=200;check(vp_highest_war_weariness_percent(context,5)==0,"Combined weariness reduction is capped at 100 percent");s.war_modifier=10;
    dip.players[5].pregame_minor=true;invalidate_vp_unit_supply(units,5);q=vp_unit_supply(context,5);
    check(q.capacity==33&&q.war_percent==0&&q.components.handicap==5&&q.components.cities==5&&q.components.population==4,"Ordinary minor supply excludes trait puppet effects and applies trait/personality and extra-city modifiers");
    options.entries["BALANCE_MINOR_UNIT_SUPPLY_HANDICAP"].value=1;invalidate_vp_unit_supply(units,5);q=vp_unit_supply(context,5);
    check(q.capacity==36&&q.components.handicap==8&&q.components.cities==6&&q.components.population==4&&q.components.expended==0,"Minor handicap option bypasses tech, city effects and expended-GP supply in the proper branches");
    s.personality=99;invalidate_vp_unit_supply(units,5);q=vp_unit_supply(context,5);check(q.minor_unready&&q.capacity==0&&q.over==28&&!s.base_cache&&!s.war_cache,"Unknown minor personality returns zero without populating caches");s.personality=1;
    rules.minor_traits[7]=99;check(vp_unit_supply(context,5).minor_unready,"Unknown minor trait follows original unready branch");rules.minor_traits[7]=1;
    dip.players[5].pregame_minor=false;options.entries["BALANCE_MINOR_UNIT_SUPPLY_HANDICAP"].value=0;
    units.players[5].barbarian=true;q=vp_unit_supply(context,5);check(q.unlimited&&q.over==0&&!s.base_cache,"Barbarians have semantic unlimited supply, without int32 capacity");units.players[5].barbarian=false;
    setup.players[5].alive=false;check(vp_unit_supply(context,5).capacity==0&&vp_unit_supply(context,5).over==0&&vp_unit_supply_components(context,5).expended==0,"Dead player has no supply or penalty");setup.players[5].alive=true;
    // A population component of 199 hundredths reduced by 50% of tech
    // progression gives one unit. Truncating before reduction would give zero.
    auto saved=s;auto saved_rules=rules;auto saved_cities=pc.cities;auto saved_city_supply=city_supply.cities;auto saved_research=dip.players[5].cities;
    s={};s.minor_type=7;s.personality=1;rules.handicaps[1].player={0,0,199,0,0,0};rules.era_base[2]=0;pc.cities.erase(43);city_supply.cities.erase({5,43});dip.players[5].cities.erase(43);auto &single=pc.cities[42];single.population=1;single.puppet=false;city_supply.cities[{5,42}]={};
    vp_team_change_known_count(techs[0],1); // 50% of the active technology span; no has-tech flags set.
    check(vp_unit_supply_components(context,5).population==1,"Population hundredths survive until after technology reduction, using saved known counter");
    constexpr std::int64_t wide=4294967300LL;single.population=wide;rules.handicaps[1].player.population=100;
    check(vp_unit_supply_components(context,5,true).population==wide,"Population supply preserves values beyond uint32");
    s=saved;rules=saved_rules;pc.cities=saved_cities;city_supply.cities=saved_city_supply;dip.players[5].cities=saved_research;vp_team_change_known_count(techs[0],-1);
    {
        ScienceContext science;science.profile=vp;
        ensure_vp_science_profile(science.players[5]).cached_happiness_total=50;
        VpCityGrowthRules gr;VpPlotReligionRules rr;SavedMapProjection map{{1,1}};PlotPassability pass;PlotWaterRules wr;wr.profile=vp;PlotWaterState ws;ws.layout=layout;
        PlotWaterContext water{map,ws,wr,types,pass,setup,cities,defines,&options};VpPlotReligionContext religion{rr,water};VpCityGrowthContext growth{religion,dip,science,gr};
        auto &city=pc.cities.at(42);vp_player_state_religion(pc)=-1;pc.second_pantheon_count=0;auto &growth_profile=ensure_vp_city_growth_profile(cities);growth_profile.players[5]={};growth_profile.cities[{5,42}]={};vp_city_majority_religion(city)=-1;city.we_love_king_turns=0;
        invalidate_vp_unit_supply(units,5);check(vp_city_growth(growth,context,city).supply==-20,"Actual city growth obtains the current excess from shared supply");
        s.free=30;check(vp_city_growth(growth,context,city).supply==0,"Live unit exemption changes growth without a stale saved penalty");s.free=2;
        auto detached=cities;VpUnitSupplyContext wrong{units,setup,dip,detached,techs,rules,db,defines,options};rejects([&]{vp_city_growth(growth,wrong,city);});
    }
    s.military=wide;s.free=1;invalidate_vp_unit_supply(units,5);check(vp_unit_supply(context,5).over==wide-25,"Actual unit count and over-supply remain 64-bit");
    // Move the entire player/team/city identity to wide sparse IDs.
    auto player_node=setup.players.extract(5);player_node.key()=wide;player_node.mapped().team=wide+1;setup.players.insert(std::move(player_node));
    auto city_node=cities.players.extract(5);city_node.key()=wide;for(auto &[id,c]:city_node.mapped().cities)c.owner=wide;cities.players.insert(std::move(city_node));
    for (const auto id:{42LL,43LL}) {auto node=city_supply.cities.extract(std::pair{5LL,id});node.key()={wide,id};city_supply.cities.insert(std::move(node));}
    auto unit_node=units.players.extract(5);unit_node.key()=wide;units.players.insert(std::move(unit_node));
    auto supply_node=ensure_vp_unit_supply_profile(units).players.extract(5);supply_node.key()=wide;ensure_vp_unit_supply_profile(units).players.insert(std::move(supply_node));
    auto rp=dip.players.extract(5);rp.key()=wide;dip.players.insert(std::move(rp));auto team_node=dip.teams.extract(9);team_node.key()=wide+1;team_node.mapped().members={wide};dip.teams.insert(std::move(team_node));dip.major_players.erase(5);dip.major_players.insert(wide);
    units.team_eras[wide+1]=3;techs.clear();techs.emplace_back(vp,wide+1,10);vp_team_change_known_count(techs.back(),4);context.technologies=techs;
    invalidate_vp_unit_supply(units,wide);check(vp_unit_supply(context,wide).over==wide-25,"Supply player and team IDs use sparse wide identities");
    vp_unit_supply_player(units,wide).extra_flat=INT64_MAX;invalidate_vp_unit_supply(units,wide);rejects([&]{vp_unit_supply(context,wide);});
    ensure_vp_unit_supply_profile(units).players.erase(wide);rejects([&]{vp_unit_supply(context,wide);});
    {
        SavedSessionSnapshot snap;snap.pregame.emplace();snap.pregame->layout=layout;snap.pregame->world.values.integers["num_cities_unit_supply_mod"]=wide;
        auto &names=snap.pregame->archive.string_arrays["minor_civ_types"];names.resize(6);names[5]="MINOR";
        snap.all_players.emplace();snap.all_players->layout=layout;snap.all_players->players.emplace_back();auto &sp=snap.all_players->players[0];sp.saved_player_id=5;
        sp.core.layout=sp.traits.layout=sp.planning.layout=sp.cities.layout=layout;
        const auto core=[&](const char *name,std::vector<std::int64_t> v){PlayerCoreField f{name,PlayerCoreEncoding::i32};f.values=std::move(v);sp.core.fields.push_back(f);};
        core("m_eID",{5});
        for(const char *name:{"m_iNumMilitaryUnits","m_iSupplyFreeUnits","m_iExtraSupplyFlat","m_iCitySupplyFlatGlobal","m_iExtraSupplyPerPopulation","m_iUnitSupplyFromExpendedGP","m_iPuppetYieldAndSupplyModifierChange","m_iCachedCurrentWarValue","m_iWarWearinessModifier"})core(name,{wide});
        core("m_aiWarWeariness",{0,0,0,0,0,0,wide,999});
        for(const char *name:{"m_iWarWearinessModifier","m_iExtraSupply","m_iExtraSupplyPerCity","m_iExtraSupplyPerPopulation","m_iPuppetYieldAndSupplyModifierChange"}){PlayerTraitsField f{name,PlayerTraitsEncoding::i32};f.values={wide};sp.traits.fields.push_back(f);}
        PlayerPlanningRecord block{"minor_civ_ai"};block.fields={{"m_ePersonality",1}};sp.planning.blocks={block};
        sp.cities.cities.emplace_back();auto &sc=sp.cities.cities[0];sc.core.layout=layout;sc.core.start_offset=wide;sc.core.end_offset=wide+100;
        const auto field=[&](const char *name,std::int64_t n){CityCoreField f{name,CityCoreEncoding::i32};f.values={n};sc.core.fields.push_back(f);};
        field("m_iID",42);field("m_eOwner",5);field("m_iCitySupplyFlat",wide);field("m_iCitySupplyModifier",7);
        snap.teams.emplace();snap.teams->layout=layout;TeamObservation team;team.fields={{"id",9}};std::vector<std::int64_t> locks(12),permanent(12);locks[9]=wide;locks[11]=888;permanent[9]=1;permanent[11]=7;
        team.arrays={{"turns_locked_at_war",1,locks},{"permanent_war_peace",1,permanent}};snap.teams->teams={team};
        ResearchContext base;base.profile=vp;base.players[5].team=9;PlayerResearchContext roster;roster.profile=vp;roster.major_players={5,6};roster.teams[9]={};
        MaintenanceContext m;m.profile=vp;m.players[5]={};CityYieldContext cy;cy.profile=vp;auto &cc=cy.players[5].cities[42];cc.id=42;cc.owner=5;cc.source_core={{wide,wide+100}};
        restore_vp_unit_supply(snap,types,base,roster,cy,m);const auto &p=vp_unit_supply_player(m,5);
        check(p.military==wide&&p.free==wide&&p.trait_base==wide&&p.war_value==wide&&p.minor_type==7&&p.personality==1&&p.war_weariness==YieldValues{{6,wide}}&&!p.base_cache&&!p.war_cache,"Supply import binds wide values, pregame minor type and personality, excludes wire tail, and resets unsaved caches");
        const auto &city_state=vp_city_supply(cy,5,42);check(city_state.flat==wide&&city_state.population_modifier==7&&vp_world_city_supply_modifier(m)==wide&&roster.teams[9].war_peace->locked_turns==YieldValues{{9,wide}}&&roster.teams[9].war_peace->permanent==std::set<std::int64_t>{9},"Supply import binds profile city cache, world and team relations without wire tail slots");
        auto copied=m;vp_unit_supply_player(copied,5).military=0;
        check(vp_unit_supply_player(m,5).military==wide&&vp_unit_supply_player(copied,5).military==0,
            "Copying maintenance state keeps VP supply inputs independent");
        auto broken=snap;broken.all_players->players[0].cities.cities[0].core.start_offset++;rejects([&]{restore_vp_unit_supply(broken,types,base,roster,cy,m);});
        broken=snap;broken.all_players->players[0].core.fields.back().values.resize(6);rejects([&]{restore_vp_unit_supply(broken,types,base,roster,cy,m);});
        broken=snap;broken.teams->teams[0].arrays[0].values.resize(9);rejects([&]{restore_vp_unit_supply(broken,types,base,roster,cy,m);});
        check(vp_unit_supply_player(m,5).military==wide&&vp_world_city_supply_modifier(m)==wide,
            "Failed VP supply import preserves the previous complete profile");
    }
    MaintenanceContext vanilla;vanilla.players[5]={};
    check(!find_vp_unit_supply_profile(vanilla),"BNW maintenance has no VP supply profile");
    rejects([&]{ensure_vp_unit_supply_profile(vanilla);});
}
