#pragma once
void treasury_turn_tests() {
    using namespace civ5::runtime;
    struct Host final : TreasuryTurnServices {
        std::vector<std::string> events;
        std::function<void()> on_deficit,on_dirty;
        void treasury_dirty(std::int64_t,TreasuryDirty kind) override {
            events.push_back(kind==TreasuryDirty::misc_buttons?"misc":kind==TreasuryDirty::selection_buttons?"selection":"data");
            if(on_dirty)on_dirty();
        }
        void deficit(std::int64_t,TechnologyProfile profile) override {
            events.push_back(profile==TechnologyProfile::vp542?"bankruptcy":"deficit");if(on_deficit)on_deficit();
        }
        void unlock_trade_gold_achievement(std::int64_t) override {events.push_back("achievement");}
    } host;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('DEFICIT_UNIT_DISBANDING_THRESHOLD',-5),"
        "('UNIT_MAINTENANCE_GAME_MULTIPLIER',8),('UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR',7),"
        "('INITIAL_FREE_OUTSIDE_UNITS',2),('INITIAL_OUTSIDE_UNIT_GOLD_PERCENT',50);");
    const auto vd=db.cache_global_defines(GlobalDefineProfile::vp542),bd=db.cache_global_defines(GlobalDefineProfile::bnw);
    ResearchContext setup;PlayerResearchContext dip;CityYieldContext cities;MaintenanceContext m;TreasuryContext state;
    TreasuryRules tr;CityYieldRules cr;MaintenanceRules mr;
    setup.profile=dip.profile=cities.profile=m.profile=state.profile=tr.profile=cr.profile=mr.profile=TechnologyProfile::vp542;
    ensure_vp_maintenance_rules_profile(mr);
    ensure_vp_maintenance_context_profile(m);
    setup.game_turn=1;setup.game_handicap=0;setup.start_era=0;setup.players[5000000000LL]={5000000001LL,0,2,0,true};
    constexpr std::int64_t id=5000000000LL,team=5000000001LL;
    dip.players[id]={};dip.teams[team].alive_count=1;dip.teams[team].members={id};dip.major_players={id};
    cities.yield_slots=17;cities.no_happiness=true;cities.players[id]={};cities.players[id].excess_happiness=0;
    auto &city=cities.players[id].cities[5];city.owner=id;city.id=5;vp_city_cached_yields(cities,id,5)[2]=1234;city.yields[2].terrain=12;
    m.default_end_turn=m.estimate_end_turn=500;m.team_eras[team]=0;m.players[id]={};
    ensure_vp_maintenance_profile(m.players[id],m.profile);
    mr.handicaps[0]={0,0,100,100,100,100,100,100,100,100,0};mr.era_building_percent[0]=100;
    std::vector<TeamTechnologies> teams;teams.emplace_back(TechnologyProfile::vp542,team,1);
    MaintenanceWorld world{setup,dip,cities,teams};CustomModOptions options;options.prefetch.all_succeeded=true;
    auto &p=state.players[id];ensure_vp_treasury_income_profile(p);
    p.balance_times100=500000000001LL;p.lifetime_gross_gold=6000000000LL;
    TreasuryTurnContext ctx{state,world,m,tr,cr,mr,vd,&options,host,id,{}};
    auto q=do_gold(ctx,id);
    check(q.net_times100==1234&&q.gross_gold==12&&p.balance_times100==500000001235LL&&p.lifetime_gross_gold==6000000012LL,
        "VP gold turn must mutate the wide shared treasury, retaining hundredths and whole lifetime income");
    check(p.balance_history_times100==std::vector<std::int64_t>{500000001235LL}&&p.income_history_times100==std::vector<std::int64_t>{1234},"Gold turn history differs");
    check(host.events==std::vector<std::string>{"misc","selection","data"},"Active-player dirty notifications have wrong order");
    change_gold(ctx,id,3);change_gold_times100(ctx,id,-90);
    check(p.income_history_times100.back()==1534&&p.balance_times100==500000001445LL,"VP instant income tracks positive changes only");
    const auto event_count=host.events.size();set_gold_times100(ctx,id,p.balance_times100);
    check(host.events.size()==event_count,"Unchanged assignment must not dirty UI");
    set_gold_times100(ctx,id,-1);check(p.balance_times100==0,"VP setter must clamp negatives");
    host.events.clear();change_gold_times100(ctx,id,-1);check(host.events.empty(),"VP negative change clamps before setter comparison");
    set_gold_times100(ctx,id,-1);check(host.events.size()==3,"VP negative direct assignment compares before clamp");
    setup.game_turn=0;p.income_history_times100.clear();change_gold_times100(ctx,id,50);
    check(p.income_history_times100==std::vector<std::int64_t>{50},"Turn-zero instant income must initialize an empty history safely");
    p.balance_times100=INT64_MAX;rejects([&]{change_gold_times100(ctx,id,1);});
    check(p.balance_times100==INT64_MAX,"Wide balance overflow must be rejected before mutation");
    rejects([&]{set_gold(ctx,id,INT64_MAX);});
    p.balance_times100=1000;p.income_history_times100={100};p.balance_history_times100={111};setup.game_turn=1;
    vp_city_cached_yields(cities,id,5)[2]=50;m.players[id].base_building=2;
    q=do_gold(ctx,id);
    check(q.net_times100==-150&&p.balance_times100==850&&p.income_history_times100==std::vector<std::int64_t>{-50}&&p.balance_history_times100==std::vector<std::int64_t>{111},
        "VP negative rate must adjust latest sample, without replacing already-recorded balance");
    p.balance_times100=0;host.events.clear();setup.game_turn=4;p.balance_history_times100.clear();p.income_history_times100.clear();
    q=do_gold(ctx,id);
    check(!q.deficit_called&&p.balance_times100==0&&p.income_history_times100==std::vector<std::int64_t>{-150}&&p.balance_history_times100.size()==1,
        "Below-threshold deficit clamps balance and appends one sample without filling gaps");
    vp_city_cached_yields(cities,id,5)[2]=0;m.players[id].base_building=5;p.balance_times100=0;p.income_history_times100.clear();host.events.clear();
    host.on_deficit=[&]{check(p.balance_times100==0&&p.income_history_times100.empty(),"Deficit callback must see zero balance before income history");vp_city_cached_yields(cities,id,5)[2]=321;change_gold(ctx,id,2);};
    q=do_gold(ctx,id);
    check(q.deficit_called&&q.gross_gold==3&&p.balance_times100==200&&p.income_history_times100==std::vector<std::int64_t>({200,-500}),
        "Bankruptcy must run at inclusive threshold and gross must observe callback changes; sparse history appends source samples");
    check(host.events==std::vector<std::string>({"bankruptcy","misc","selection","data"}),"Bankruptcy effects must precede later history updates");
    host.on_deficit=[&]{throw TreasuryUnavailable("unit world not loaded");};vp_city_cached_yields(cities,id,5)[2]=0;p.balance_times100=1;m.players[id].base_building=6;
    const auto lifetime=p.lifetime_gross_gold;const auto history=p.income_history_times100;
    rejects([&]{do_gold(ctx,id);});check(p.balance_times100==0&&p.income_history_times100==history&&p.lifetime_gross_gold==lifetime,"Failed deficit service must expose exact partial mutation boundary");
    host.on_deficit={};m.players[id].base_building=0;vp_city_cached_yields(cities,id,5)[2]=200;p.balance_times100=17;p.income_history_times100.clear();cities.players[id].minor=true;
    q=do_gold(ctx,id);check(p.balance_times100==0&&p.income_history_times100.empty()&&q.gross_gold==2,"VP minors zero gold and do not track discarded positive income");
    cities.players[id].minor=false;p.anarchy=true;p.balance_times100=123;m.players[id].cached_unit_expense=456;
    q=do_gold(ctx,id);check(q.net_times100==0&&q.gross_gold==2&&p.balance_times100==123&&m.players[id].cached_unit_expense==456,"VP anarchy still updates gross/history, with no maintenance query");
    p.anarchy=false;host.events.clear();ctx.active_player=-1;set_gold(ctx,id,12);check(host.events.empty(),"Inactive player must not dirty UI");ctx.active_player=id;
    // A second subsystem must see the changed treasury, without re-importing
    // the save or manually copying its balance into another model.
    ScienceContext science;science.profile=TechnologyProfile::vp542;
    ensure_vp_science_profile(science.players[id]);science.research_agreements[team]={};
    vp_city_cached_yields(cities,id,5)[2]=0;vp_city_cached_yields(cities,id,5)[3]=1000;m.players[id].base_building=2;p.balance_times100=0;
    const auto science_quote=[&]{return science_times100(science,world,state,m,tr,cr,mr,vd,&options,id);};
    check(science_quote().budget_deficit==-200,"Science setup must observe the shared treasury deficit");
    p.balance_times100=200;check(science_quote().budget_deficit==0,"Stored gold covers next-turn science deficit");
    do_gold(ctx,id);check(p.balance_times100==0&&science_quote().budget_deficit==-200,"Science must see DoGold expenditure on the same state");
    change_gold(ctx,id,5);check(science_quote().budget_deficit==0,"Science must immediately observe live gold changes");
    setup.players[id].slot_status=3;options.entries["ENABLE_ACHIEVEMENTS"].value=1;
    rejects([&]{do_gold(ctx,id);}); // VP ignore-trade cache rebuilding is still required.
    options.entries["ENABLE_ACHIEVEMENTS"].value=0;q=do_gold(ctx,id);
    check(!q.achievement_requested,"Disabled VP achievements must skip the unresolved city cache branch");setup.players[id].slot_status=2;
    p.income_history_times100={INT64_MAX};setup.game_turn=1;p.balance_times100=0;
    rejects([&]{change_gold_times100(ctx,id,1);});
    check(p.balance_times100==1&&p.income_history_times100.back()==INT64_MAX,"Income-history overflow must preserve source setter-before-history order");
    setup.game_turn=5000000000LL;p.income_history_times100.clear();change_gold_times100(ctx,id,1);
    check(p.income_history_times100==std::vector<std::int64_t>{1},"Wide turn numbers must append one sample without allocating up to the turn");
    host.on_dirty=[&]{throw TreasuryUnavailable("UI unavailable");};p.balance_times100=1;auto saved_hist=p.income_history_times100;
    rejects([&]{change_gold_times100(ctx,id,100);});check(p.balance_times100==101&&p.income_history_times100==saved_hist,"Setter dirty callback precedes VP instant-income accounting");host.on_dirty={};
    auto &anarchy=ensure_vp_culture_history_profile(setup).players[id];
    anarchy.anarchy_turns=2;p.anarchy=true;host.events.clear();
    check(science_quote().anarchy,"Science must observe the shared treasury anarchy flag");
    const auto first_anarchy=do_vp_anarchy_after_policy_cost(setup,ctx,id);
    check(first_anarchy.before==2&&first_anarchy.after==1&&first_anarchy.changed&&
          !first_anarchy.event_ended&&first_anarchy.ui_dirty&&p.anarchy&&
          host.events==std::vector<std::string>{"data"},
          "VP anarchy decrement must retain the science gate and dirty active UI");
    host.events.clear();
    rejects([&]{(void)do_vp_anarchy_after_policy_cost(setup,ctx,id);});
    check(anarchy.anarchy_turns==1&&p.anarchy&&host.events.empty(),
          "Ending VP anarchy must wait for the mod event host before mutation");
    bool ended=false;
    const auto last_anarchy=do_vp_anarchy_after_policy_cost(setup,ctx,id,
        [&](std::int64_t observed,bool start,std::int64_t turns) {
            check(observed==id&&!start&&turns==0&&anarchy.anarchy_turns==1&&p.anarchy,
                  "VP PlayerAnarchy hook must run before changing the counter");
            ended=true;
        });
    check(ended&&last_anarchy.after==0&&last_anarchy.event_ended&&
          !p.anarchy&&!science_quote().anarchy&&host.events==std::vector<std::string>{"data"},
          "Ending VP anarchy must release the science gate on the same live world");
    host.events.clear();
    check(!do_vp_anarchy_after_policy_cost(setup,ctx,id).changed&&host.events.empty(),
          "Zero VP anarchy must be a no-op without an event host");
    // BNW changes use signed balances; DoGold alone prevents a turn deficit.
    setup.profile=dip.profile=cities.profile=m.profile=state.profile=tr.profile=cr.profile=mr.profile=TechnologyProfile::bnw;
    TreasuryTurnContext bnw{state,world,m,tr,cr,mr,bd,nullptr,host,id,{}};
    p.balance_times100=1;p.income_history_times100.clear();p.balance_history_times100.clear();p.lifetime_gross_gold=0;setup.game_turn=1;m.players[id].base_building=0;
    set_gold_times100(bnw,id,-20);check(p.balance_times100==-20,"BNW negative setter must retain source assert-only semantics");
    change_gold(bnw,id,1);check(p.balance_times100==80&&p.income_history_times100.empty(),"BNW instant gold must not modify income history");
    cities.players[id].minor=true;q=do_gold(bnw,id);check(p.balance_times100==1280&&q.net_times100==1200&&p.income_history_times100==std::vector<std::int64_t>{1200},"BNW minors retain income and use per-turn history");
    cities.players[id].minor=false;p.anarchy=true;m.players[id].base_building=90;m.players[id].cached_unit_expense=987;setup.game_turn=2;
    q=do_gold(bnw,id);check(q.net_times100==0&&q.gross_gold==12&&p.balance_times100==1280&&m.players[id].cached_unit_expense==987&&p.income_history_times100.back()==0,
        "BNW DoGold uses PLAYER anarchy gate but still records gross and zero net history");
    p.anarchy=false;city.yields[2].terrain=0;p.balance_times100=0;m.players[id].base_building=5;host.events.clear();
    q=do_gold(bnw,id);check(q.deficit_called&&host.events==std::vector<std::string>{"deficit"},"BNW uses DoDeficit at inclusive threshold");
    // Achievement predicate and trade subtraction use actual city yields.
    m.players[id].base_building=0;setup.players[id].slot_status=3;city.x=1;city.y=2;
    civ5::saves::TradeRouteObservation route{};route.origin_owner=id;route.origin_x=1;route.origin_y=2;route.destination_owner=2;
    route.origin_yields={0,0,20099};cities.trade_routes={route};
    host.events.clear();q=do_gold(bnw,id);
    check(q.achievement_requested&&host.events.back()=="achievement","BNW achievement uses truncation and >=200 trade gold threshold after treasury update");
    setup.game_type=2;host.events.clear();q=do_gold(bnw,id);check(!q.achievement_requested,"Hotseat must not unlock gold achievement");setup.game_type=0;
    cities.trade_routes[0].origin_yields[2]=19999;q=do_gold(bnw,id);check(!q.achievement_requested,"199.99 trade gold must not unlock achievement");
    p.balance_times100=5;p.income_history_times100={1};setup.game_turn=-1;rejects([&]{do_gold(bnw,id);});check(p.balance_times100==5,"Negative host turn rejected before mutations");
}
