#pragma once
void science_tests() {
    using namespace civ5::runtime;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES"
        "('RESEARCH_AGREEMENT_MOD',10),('RESEARCH_AGREEMENT_PLAYER_AVERAGE_YIELD_PERCENT',35),"
        "('RESEARCH_AGREEMENT_BOOST_DIVISOR',3),('RESEARCH_AGREEMENT_PER_TURN_YIELD_PERCENT',40),"
        "('MINOR_CIV_SCIENCE_BONUS_MULTIPLIER',25),('VASSALAGE_FREE_YIELD_FROM_VASSAL_PERCENT',20),"
        "('UNHAPPY_THRESHOLD',50),('UNIT_MAINTENANCE_GAME_MULTIPLIER',8),('UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR',7);");
    const auto vd=db.cache_global_defines(GlobalDefineProfile::vp542),bd=db.cache_global_defines(GlobalDefineProfile::bnw);
    ResearchContext setup;PlayerResearchContext dip;CityYieldContext cities;TreasuryContext treasury;
    MaintenanceContext maintenance;ScienceContext s;TreasuryRules tr;CityYieldRules cr;MaintenanceRules mr;
    setup.profile=dip.profile=cities.profile=treasury.profile=maintenance.profile=s.profile=tr.profile=cr.profile=mr.profile=TechnologyProfile::vp542;
    ensure_vp_maintenance_rules_profile(mr);
    ensure_vp_maintenance_context_profile(maintenance);
    setup.start_era=0;setup.game_handicap=0;cities.yield_slots=17;maintenance.default_end_turn=maintenance.estimate_end_turn=500;
    mr.handicaps[0]={0,0,100,100,100,100,100,100,100,100,0};mr.era_building_percent[0]=100;
    std::vector<TeamTechnologies> techs;
    for(std::int64_t i=0;i<4;++i) {
        setup.players[i]={i==3?1:i,0,3,0,i!=3};dip.players[i]={};cities.players[i]={};
        cities.players[i].excess_happiness=0;treasury.players[i]={};maintenance.players[i]={};s.players[i]={};
        ensure_vp_maintenance_profile(maintenance.players[i],maintenance.profile);
        ensure_vp_treasury_income_profile(treasury.players[i]);
        ensure_vp_treasury_minor_profile(treasury.players[i]);
        ensure_vp_treasury_religion_profile(treasury.players[i]);
        ensure_vp_science_profile(s.players[i]);
        treasury.players[i].balance_times100=100000;
        if(i<3) {
            dip.teams[i].members={i};dip.teams[i].alive_count=1;maintenance.team_eras[i]=2;s.research_agreements[i]={};
            techs.emplace_back(TechnologyProfile::vp542,i,10);
            auto &c=cities.players[i].cities[i*10];c.id=i*10;c.owner=i;c.population=1;vp_city_cached_yields(cities,i,i*10)[3]=i==0?1001:i==1?2003:400;
            c.yields[3].terrain=i==0?10:i==1?20:4;cities.players[i].capital=i*10;
            ensure_vp_science_profile(s.players[i]).passive_spy_percent[i*10]={};
            ensure_vp_treasury_minor_profile(treasury.players[i]).city_majorities[i*10]=-1;
        }
    }
    dip.teams[1].members={1,3};dip.major_players={0,1,3};dip.civ_teams={0,1,2};dip.barbarian_team=63;
    dip.players[2].pregame_minor=true;cities.players[2].minor=true;s.minor_players=treasury.minor_players={2};
    ensure_vp_treasury_minor_profile(treasury.players[2]).minor_type=7;treasury.players[2].ally=0;ensure_vp_treasury_minor_profile(treasury.players[2]).friends={0};
    tr.minors[7].friends[3]={{0,100}};tr.minors[7].allies[3]={{0,200}};
    auto &p=s.players[0];auto &vp_player=ensure_vp_science_profile(p);
    p.happiness_to_science=10;vp_player.cached_happiness_total=50;p.science_from_players={2};
    vp_player.last_turn_times100=1001;p.median_tech_percent=50;
    ensure_vp_science_profile(s.players[1]).last_turn_times100=2003;
    ensure_vp_science_profile(s.players[3]).last_turn_times100=501;
    vp_player.jfd_minors=2;vp_player.annexed_minors_times100=25;
    vp_player.incoming_espionage=3;vp_player.outgoing_espionage=1;
    vp_player.passive_spy_modifier=25;vp_player.spies={{true,{{1,10}}},{false,{{1,10}}}};
    ensure_vp_science_profile(s.players[1]).passive_spy_percent[10][0]=33;
    s.research_agreements[0]={1};CustomModOptions options;options.prefetch.all_succeeded=true;
    options.entries["BALANCE_VP"].value=options.entries["BALANCE_CORE_JFD"].value=1;
    MaintenanceWorld world{setup,dip,cities,techs};
    const auto query=[&](std::int64_t id=0,bool exclude=false) {return science_times100(s,world,treasury,maintenance,tr,cr,mr,vd,&options,id,exclude);};
    auto q=query();
    check(q.cities==1001&&q.other_players==100&&q.happiness==100&&q.agreements==217&&vp_science_quote(q).minors==300&&vp_science_quote(q).passive_spies==1650&&q.total_times100==3793,
        "Full VP science must preserve RA integer stages, dead RA member, duplicate/dead spy rows and minor bonuses");
    auto missing_profile=s;missing_profile.players[0].profile_extensions.values.clear();
    rejects([&]{science_times100(missing_profile,world,treasury,maintenance,tr,cr,mr,vd,&options,0);});
    q=query(0,true);check(q.agreements==0&&q.total_times100==3576,"VP exclusion must remove only RA income");
    vp_player.spies[0].alive=false;check(vp_science_quote(query()).passive_spies==0,"No alive spies must bypass all passive rows");vp_player.spies[0].alive=true;
    vp_player.cached_happiness_total=-1;check(query().happiness==0,"Negative VP cached happiness must suppress conversion");vp_player.cached_happiness_total=50;
    ensure_vp_science_profile(s.players[2]).cached_happiness_total=-100;s.players[2].happiness_to_science=10;
    check(query().other_players==110,"Minor happiness must use the native threshold instead of its saved total");s.players[2].happiness_to_science=0;
    setup.players[2].alive=false;q=query();check(q.other_players==100&&vp_science_quote(q).minors==0,"Legacy science gifts have no alive guard; VP minor yield scan does");setup.players[2].alive=true;
    p.science_from_players.clear();p.minor_science_allies=true;
    check(query().other_players==100,"Minor science allies policy must activate legacy science share");
    treasury.players[2].ally=-1;ensure_vp_treasury_minor_profile(treasury.players[2]).permanent_ally=0;ensure_vp_treasury_minor_profile(treasury.players[2]).no_ally=true;
    check(query().other_players==0,"NoAlly must block the permanent-alliance branch");
    treasury.players[2].ally=0;check(query().other_players==100,"NoAlly must not block an explicit ally");
    dip.teams[1].master=0;q=query();check(vp_science_quote(q).vassals==400&&q.total_times100==4193,"Vassal science must call the full child query and truncate each share");
    vp_player.vassal_modifier=5;check(vp_science_quote(query()).vassals==500,"Vassal science modifier must add to native free-yield percent");vp_player.vassal_modifier=0;
    s.research_agreements[1]={0};
    check(vp_science_quote(query(0,true)).vassals>400,"Excluding own RA must not exclude nested vassal RA");s.research_agreements[1].clear();
    s.no_science=true;q=query();check(vp_science_quote(q).vassals==0&&q.total_times100==3793,"NO_SCIENCE gates vassal income, not the whole getter");s.no_science=false;
    dip.teams[0].master=1;rejects([&]{query();});dip.teams[0].master=-1;dip.teams[1].master=-1;
    maintenance.players[0].base_building=2;treasury.players[0].balance_times100=-50;
    q=query();check(q.gold_rate_times100==-200&&q.budget_deficit==-250&&q.total_times100==3543,"Budget science penalty must include saved balance plus current net gold");
    treasury.players[0].balance_times100=-999999;check(query().total_times100==0,"Negative total science must clamp only after all income components");
    treasury.players[0].balance_times100=0;maintenance.players[0].base_building=0;
    options.entries["BALANCE_VP"].value=0;vp_player.per_trade_partner=2;
    civ5::saves::TradeRouteObservation route{};route.origin_owner=0;route.destination_owner=1;route.origin_x=route.origin_y=route.destination_x=route.destination_y=0;
    treasury.trade_routes={route,route};check(query().traits==200,"CP science traits count distinct partners without gold's era scaling");
    options.entries["BALANCE_VP"].value=1;check(query().traits==0,"VP science trait getter explicitly returns zero");
    vp_player.annexed_minors_times100=5000000001LL;check(query().total_times100>UINT32_MAX,"Science amounts must exceed 32-bit range");vp_player.annexed_minors_times100=25;
    treasury.players[0].anarchy=true;maintenance.players[0].cached_unit_expense=123;
    auto saved_cities=cities.players;cities.players.clear();q=query();
    check(q.anarchy&&q.total_times100==0&&!q.gold_rate_times100&&maintenance.players[0].cached_unit_expense==123,"Anarchy must avoid world income and expense-cache mutations");
    cities.players=std::move(saved_cities);treasury.players[0].anarchy=false;
    vp_player.annexed_minors_times100=INT64_MAX;rejects([&]{query();});vp_player.annexed_minors_times100=25;
    {
        auto large_setup=setup;auto large_dip=dip;auto large_cities=cities;
        auto large_treasury=treasury;auto large_maintenance=maintenance;auto large_science=s;
        ensure_vp_science_profile(large_science.players[0]).vassal_modifier=80; // 100% pass-through for the depth test.
        std::int64_t master=0;
        for(std::int64_t i=0;i<256;++i) {
            const auto id=5000000000LL+i;
            large_setup.players[id]={id,0,3,0,true};large_dip.players[id]={};
            large_dip.teams[id].alive_count=1;large_dip.teams[id].members={id};large_dip.teams[id].master=master;
            large_dip.major_players.insert(id);large_cities.players[id]={};large_treasury.players[id]={};
            ensure_vp_treasury_income_profile(large_treasury.players[id]);
            ensure_vp_treasury_minor_profile(large_treasury.players[id]);
            ensure_vp_treasury_religion_profile(large_treasury.players[id]);
            large_treasury.players[id].balance_times100=100000;large_maintenance.players[id]={};
            ensure_vp_maintenance_profile(large_maintenance.players[id],large_maintenance.profile);
            large_maintenance.team_eras[id]=0;
            ensure_vp_science_profile(large_science.players[id]).vassal_modifier=80;
            large_science.research_agreements[id]={};master=id;
        }
        ensure_vp_science_profile(large_science.players[master]).annexed_minors_times100=9000000000LL;
        MaintenanceWorld large_world{large_setup,large_dip,large_cities,techs};
        auto deep=science_times100(large_science,large_world,large_treasury,large_maintenance,tr,cr,mr,vd,&options,0);
        check(vp_science_quote(deep).vassals==9000000000LL,"Science dependency stack must handle 256 nested wide-ID players without original player or int32 limits");
    }
    // BNW shares science with minors even without VP's major-team gate.
    setup.profile=dip.profile=cities.profile=treasury.profile=maintenance.profile=s.profile=tr.profile=cr.profile=mr.profile=TechnologyProfile::bnw;
    const auto bnw=[&]{return science_times100(s,world,treasury,maintenance,tr,cr,mr,bd,nullptr,0);};
    q=bnw();check(q.cities==1000&&q.happiness==100&&q.agreements==100&&q.other_players==100&&q.total_times100==1300&&!q.profile_extensions.find<VpScienceQuote>(),"BNW full science must use live city formula and base RA percent only");
    cities.players[0].excess_happiness.reset();rejects([&]{bnw();});cities.players[0].excess_happiness=0;
    treasury.players[2].ally=-1;check(bnw().other_players==0,"BNW must not use VP permanent allies");treasury.players[2].ally=0;
    treasury.players[0].anarchy=true;check(bnw().total_times100==0,"BNW science, unlike BNW net gold, stops during anarchy");
    rejects([&]{science_times100(s,world,treasury,maintenance,tr,cr,mr,bd,nullptr,0,true);});
    for(bool vp:{false,true}) {
        using namespace civ5::saves;
        const auto profile=vp?TechnologyProfile::vp542:TechnologyProfile::bnw;
        const auto layout=vp?GameStateLayout::vp542_corpus:GameStateLayout::expansion2_version1;
        ResearchContext rs;PlayerResearchContext rd;CityYieldContext cy;
        rs.profile=rd.profile=cy.profile=profile;rs.players[0]={};rd.players[0]={};rd.teams[0]={};cy.players[0]={};
        rs.game_options={{"GAMEOPTION_NO_SCIENCE",1}};MaintenanceWorld view{rs,rd,cy,{}};
        SavedSessionSnapshot snap{};snap.all_players.emplace();snap.teams.emplace();snap.map_plots.emplace();
        snap.all_players->layout=snap.teams->layout=snap.map_plots->layout=snap.map_header.layout=layout;
        snap.teams->dimensions.major=0;snap.teams->dimensions.civ_teams=1;
        TeamObservation team{};team.fields={{"id",0}};team.arrays={{"research_agreement",1,{1}}};snap.teams->teams={team};
        SavedPlayerObservation saved{};saved.saved_player_id=0;
        saved.core.layout=saved.strategy.layout=saved.espionage.layout=saved.traits.layout=saved.tail.layout=layout;
        const auto field=[&](const char *name,std::vector<std::int64_t> values){PlayerCoreField f{};f.name=name;f.values=values;saved.core.fields.push_back(f);};
        field("m_iHappinessToScience",{10});
        field("m_iMinorScienceAlliesCount",{1});field("m_iMedianTechPercentage",{50});field("m_pabGetsScienceFromPlayer",{0,1});
        if(vp) {
            field("m_iHappinessTotal",{-33});field("m_iVassalYieldBonusModifier",{44});
            for(const auto *name:{"m_aiLastTurnYieldsTimes100","m_aiYieldFromMinors","m_piYieldPerTurnFromAnnexedMinorsTimes100"})field(name,{0,0,0,5000000001LL});
            PlayerTraitsField trade{};trade.name="m_iYieldChangePerTradePartner";trade.values={0,0,0,7};saved.traits.fields={trade};
            for(const auto *name:{"m_aiIncomingEspionageYields","m_aiOutgoingEspionageYields"}){PlayerTailNode f{};f.name=name;f.values={0,0,0,9};saved.tail.fields.push_back(f);}
            PlayerStrategyBlock policies{};policies.name="policies";policies.start_offset=100;policies.end_offset=400;
            std::vector<std::int64_t> mods(60);mods[43]=-25;policies.arrays={{"policy_modifiers",1,mods}};saved.strategy.blocks={policies};
            saved.espionage.start_offset=500;saved.espionage.end_offset=1000;
            PlayerEspionageRecord espionage{};espionage.name="player_espionage";
            const auto spy=[&](std::int64_t x,std::int64_t y,std::int64_t state){PlayerEspionageRecord a{};a.name="spy";a.fields={{"m_iCityX",x},{"m_iCityY",y},{"m_eSpyState",state}};espionage.children.push_back(a);};
            spy(2,0,8);spy(1,0,0);spy(-2,0,9);spy(-2147483647LL,0,0);saved.espionage.blocks={espionage};
            snap.map_header.grid={2,1};snap.map_header.wrap_x=true;
            PlotObservation center{};center.fields={{"is_city",1},{"owning_city_owner",0},{"owning_city_id",10}};
            PlotObservation worked=center;worked.fields[0].value=0;snap.map_plots->plots={center,worked};
            cy.players[0].cities[10]={};SavedCityObservation city{};city.core.layout=city.tail.layout=layout;
            CityCoreField cid{};cid.name="m_iID";cid.values={10};CityCoreField owner{};owner.name="m_eOwner";owner.values={0};city.core.fields={cid,owner};
            CityTailNode esp{};esp.name="espionage";CityTailNode percentages{};percentages.name="m_aiSciencePassivePerTurn";percentages.values={12};esp.children={percentages};city.tail.blocks={esp};saved.cities.cities={city};
        }
        snap.all_players->players={saved};auto restored=restore_science(snap,rs,view,db);const auto &p=restored.players.at(0);
        check(restored.no_science&&restored.minor_players.contains(0)&&restored.research_agreements.at(0).contains(0)&&p.science_from_players.contains(1)&&p.minor_science_allies,
            "Science import must retain source relation bits and selected loop domains");
        if(vp) {
            const auto &vp_state=vp_science_profile(p);
            check(vp_state.source_policy_modifiers==160&&vp_state.passive_spy_modifier==-25&&
                vp_state.last_turn_times100==5000000001LL&&vp_state.passive_spy_percent.at(10).at(0)==12,
                "Science import must bind saved policy/spy data and wide core values");
            check(vp_state.spies.size()==4&&!vp_state.spies[0].alive&&
                vp_state.spies[0].city==std::pair<std::int64_t,std::int64_t>{0,10}&&
                vp_state.spies[1].alive&&!vp_state.spies[1].city&&
                !vp_state.spies[2].alive&&!vp_state.spies[2].city&&!vp_state.spies[3].city,
                "Spy resolution must preserve source wrapping, dead states, invalid sentinel and actual city-center gate");
            snap.all_players->players[0].strategy.blocks[0].arrays[0].values.pop_back();rejects([&]{restore_science(snap,rs,view,db);});
        } else check(!p.profile_extensions.find<VpScienceProfile>(),"BNW must not consume VP-only science state");
        snap.all_players->players={saved,saved};rejects([&]{restore_science(snap,rs,view,db);});
    }
}
