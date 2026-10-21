#pragma once
void treasury_tests(){
    using namespace civ5::runtime;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('CITY_STATE_SCALE_PER_CITY_MOD',10),"
        "('UNIT_MAINTENANCE_GAME_MULTIPLIER',8),('UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR',7),"
        "('INITIAL_FREE_OUTSIDE_UNITS',2),('INITIAL_OUTSIDE_UNIT_GOLD_PERCENT',50),"
        "('TEMPORARY_CULTURE_BOOST_MOD',33),('GOLDEN_AGE_CULTURE_MODIFIER',20),"
        "('VASSALAGE_FREE_YIELD_FROM_VASSAL_PERCENT',20);");
    const auto vp_defs=db.cache_global_defines(GlobalDefineProfile::vp542),bnw_defs=db.cache_global_defines(GlobalDefineProfile::bnw);
    ResearchContext setup;PlayerResearchContext dip;CityYieldContext cities;MaintenanceContext m;TreasuryContext s;
    TreasuryRules rules;CityYieldRules city_rules;MaintenanceRules maintenance_rules;
    setup.profile=dip.profile=cities.profile=m.profile=s.profile=rules.profile=city_rules.profile=maintenance_rules.profile=TechnologyProfile::vp542;
    ensure_vp_maintenance_rules_profile(maintenance_rules);
    ensure_vp_maintenance_context_profile(m);
    setup.start_era=0;setup.game_handicap=0;cities.yield_slots=17;cities.no_happiness=true;
    for(std::int64_t id=0;id<4;++id){
        setup.players[id]={id==3?0:id,0,id==0?3:1,0,true};dip.players[id]={};
        cities.players[id]={};m.players[id]={};ensure_vp_maintenance_profile(m.players[id],m.profile);
        ensure_vp_treasury_income_profile(s.players[id]);
        ensure_vp_treasury_minor_profile(s.players[id]);
        ensure_vp_treasury_religion_profile(s.players[id]);
    }
    dip.major_players={0,1,3};dip.barbarian_team=63;dip.teams[0].members={0,3};dip.teams[0].alive_count=2;
    dip.teams[1].members={1};dip.teams[1].alive_count=1;dip.teams[1].master=0;dip.teams[2].members={2};dip.teams[2].alive_count=1;
    m.team_eras={{0,3},{1,2},{2,3}};m.default_end_turn=m.estimate_end_turn=500;
    maintenance_rules.handicaps[0]={0,0,100,100,100,100,100,100,100,100,0};maintenance_rules.era_building_percent[0]=100;
    m.players[0].base_building=3;m.players[0].base_improvement=2;m.players[0].diplomacy_gold=2;
    ensure_vp_maintenance_profile(m.players[0],m.profile).vassal_taxes_times100=250;
    m.players[1].cached_unit_expense=11;ensure_vp_maintenance_profile(m.players[1],m.profile).vassal_taxes_times100=199;
    auto &c=cities.players[0].cities[10];c.owner=0;c.id=10;c.population=7;vp_city_cached_yields(cities,0,10)[2]=1001;c.yields[2].terrain=10;
    cities.players[0].capital=10;cities.players[0].civilization=5;
    cities.players[2].cities[20].population=3;cities.players[2].cities[21].population=1;cities.players[2].capital=20;
    std::vector<TeamTechnologies> techs;techs.emplace_back(TechnologyProfile::vp542,0,10);
    MaintenanceWorld w{setup,dip,cities,techs};CustomModOptions o;o.prefetch.all_succeeded=true;
    for(const auto *name:{"BALANCE_VP","BALANCE_CORE_JFD","BALANCE_CITY_STATE_SCALE"})o.entries[name].value=1;
    auto &p=s.players[0];auto &vp_income=ensure_vp_treasury_income_profile(p);
    p.balance_times100=500000000001LL;p.connections_times100=137;vp_income.internal_trade=4;vp_income.incoming_espionage=2;
    vp_income.jfd_minors=3;vp_income.annexed_minors_times100=99;p.gold_per_partner=2;
    vp_income.gold_per_export=5;vp_income.gold_per_import=7;
    auto &pm=ensure_vp_treasury_minor_profile(p);pm.trait_cs_modifier=10;pm.cs_modifier=15;auto &vp_religion=ensure_vp_treasury_religion_profile(p);
    vp_religion.state_religion=1;vp_religion.owns_state_religion=true;pm.city_majorities[10]=1;
    vp_income.exported={{8,3},{7,9}};vp_income.imported={{8,1}};
    vp_income.from_minors={{8,1},{4294967300LL,2}};rules.luxury_resources={8,4294967300LL};
    auto &cs=s.players[2];auto &csm=ensure_vp_treasury_minor_profile(cs);csm.minor_type=7;csm.friends={0};cs.ally=0;csm.city_majorities={{20,1},{21,1}};s.minor_players={2};
    rules.minors[7].friends[2]={{0,100},{2,150},{3,100}};rules.minors[7].allies[2]={{2,250},{3,-20}};
    rules.minors[7].type="MINOR_A";rules.minors[4].type="MINOR_Z";
    rules.beliefs[1]={2,4,20,-1,true,false,false,false,false};
    TreasuryReligion religion;religion.religion=religion.belief_religion=1;religion.founder=0;religion.holy_plot_owner=0;religion.holy_city={{0,10}};religion.beliefs={1};religion.belief_lookup={1};s.religions={religion};
    const auto route=[&](std::int64_t a,std::int64_t b,bool empty=false){civ5::saves::TradeRouteObservation r{};r.origin_owner=a;r.destination_owner=b;
        r.origin_x=r.origin_y=r.destination_x=r.destination_y=empty?-1:0;s.trade_routes.push_back(r);};
    route(0,1);route(1,0);route(2,0);route(0,3,true);route(0,0);route(1,3);
    auto minor=minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2);
    check(minor.friends==100&&minor.allies==230&&minor.modifier==45&&minor.total_times100==525,"VP minor yield era accumulation, religion modifier or sequential truncation differs");
    auto missing_religion=s;missing_religion.players[0].profile_extensions.values.erase(std::type_index(typeid(VpTreasuryReligionProfile)));
    rejects([&]{minor_yield_times100(missing_religion,w,m,rules,vp_defs,o,2,0,2);});
    auto missing_minor=s;missing_minor.players[2].profile_extensions.values.erase(std::type_index(typeid(VpTreasuryMinorProfile)));
    rejects([&]{minor_yield_times100(missing_minor,w,m,rules,vp_defs,o,2,0,2);});
    auto copied_religion=s;ensure_vp_treasury_religion_profile(copied_religion.players[0]).state_religion=3;
    check(vp_treasury_religion_profile(s.players[0]).state_religion==1,"Copied VP treasury religion must own independent profile state");
    auto copied_minor=s;ensure_vp_treasury_minor_profile(copied_minor.players[2]).friends.clear();
    check(vp_treasury_minor_profile(s.players[2]).friends.contains(0),"Copied VP minor state must own independent friends");
    auto gross=gross_gold_times100(s,w,m,rules,city_rules,vp_defs,&o,0);
    check(gross.cities==1001&&gross.traits==6900&&vp_treasury_income_quote(gross).minors==525&&vp_treasury_income_quote(gross).vassal_taxes==100&&gross.total_times100==9862,
        "VP gross gold must include source trade partners, unique luxury types, minors, taxes and all saved sources");
    struct TaxServices final:TreasuryTurnServices {
        void treasury_dirty(std::int64_t,TreasuryDirty) override {}
        void deficit(std::int64_t,TechnologyProfile) override {}
        void unlock_trade_gold_achievement(std::int64_t) override {}
    } tax_services;
    TreasuryTurnContext tax_turn{s,w,m,rules,city_rules,maintenance_rules,vp_defs,&o,tax_services,0,{}};
    auto &saved_culture=ensure_vp_culture_income_saved_profile(setup).players[0];
    saved_culture.per_trade_partner=11;saved_culture.per_export=5;saved_culture.per_import=7;
    saved_culture.jfd_minors_times100=300;
    const auto culture_traits=vp_culture_resource_traits_times100(tax_turn,0);
    check(culture_traits.trade_partners==0&&culture_traits.exported_luxuries==1&&
          culture_traits.imported_luxuries==2&&culture_traits.export_times100==1500&&
          culture_traits.import_times100==4200&&culture_traits.total_times100==5700,
          "VP culture traits must use selected luxury IDs and era without partner yield");
    o.entries["BALANCE_VP"].value=0;
    const auto cp_culture_traits=vp_culture_resource_traits_times100(tax_turn,0);
    check(cp_culture_traits.trade_partners==2&&cp_culture_traits.partner_times100==2200&&
          cp_culture_traits.total_times100==7900,
          "CP culture traits must count distinct live trade partners");
    o.entries["BALANCE_VP"].value=1;
    const auto culture_minors=vp_culture_minor_income_times100(tax_turn,0,2);
    check(culture_minors.city_states_times100==525&&culture_minors.jfd_times100==300&&
          culture_minors.total_times100==825,
          "VP culture minor income must combine live city-state yield and saved JFD extension");
    o.entries["BALANCE_CORE_JFD"].value=0;
    check(vp_culture_minor_income_times100(tax_turn,0,2).total_times100==525,
          "Disabled JFD option must omit its saved VP minor yield");
    o.entries["BALANCE_CORE_JFD"].value=1;
    saved_culture.per_export=50000000;
    check(vp_culture_resource_traits_times100(tax_turn,0).export_times100==15000000000LL,
          "VP culture trait multiplication must not inherit i32 limits");
    saved_culture.per_export=5;
    rejects([&]{(void)vp_culture_minor_income_times100(tax_turn,0,-1);});
    auto wrong_culture_state=s;wrong_culture_state.profile=TechnologyProfile::bnw;
    TreasuryTurnContext wrong_culture_turn{wrong_culture_state,w,m,rules,city_rules,
        maintenance_rules,vp_defs,&o,tax_services,0,{}};
    rejects([&]{(void)vp_culture_resource_traits_times100(wrong_culture_turn,0);});
    saved_culture.happiness_to_culture=25;
    check(vp_culture_happiness_times100(tax_turn,0)==0,
          "VP must ignore the CP happiness-to-culture branch without resolved happiness");
    cities.no_happiness=false;
    check(vp_culture_happiness_times100(tax_turn,0)==0,
          "VP happiness suppression must precede the missing empire value");
    o.entries["BALANCE_VP"].value=0;
    rejects([&]{(void)vp_culture_happiness_times100(tax_turn,0);});
    cities.players[0].excess_happiness=7;
    check(vp_culture_happiness_times100(tax_turn,0)==175,
          "CP happiness-to-culture already has hundredths units in the source getter");
    cities.players[0].excess_happiness=-7;
    check(vp_culture_happiness_times100(tax_turn,0)==0,
          "Nonpositive excess happiness must grant no culture");
    cities.players[0].excess_happiness=7;saved_culture.happiness_to_culture=-25;
    check(vp_culture_happiness_times100(tax_turn,0)==-175,
          "Nonzero negative source coefficient must retain its sign");
    cities.players[0].excess_happiness.reset();saved_culture.happiness_to_culture=0;
    check(vp_culture_happiness_times100(tax_turn,0)==0,
          "Zero happiness coefficient must not demand an unrelated cache");
    saved_culture.happiness_to_culture=25;cities.no_happiness=true;
    check(vp_culture_happiness_times100(tax_turn,0)==0,
          "NO_HAPPINESS must suppress CP culture without a resolved empire value");
    saved_culture.happiness_to_culture=0;o.entries["BALANCE_VP"].value=1;
    auto &culture_history=ensure_vp_culture_history_profile(setup).players[0];
    cities.players[0].golden_age=true;
    saved_culture.temporary_bonus_turns=2;saved_culture.conquest_bonus_turns=3;
    saved_culture.conquest_modifier=17;
    const auto controls=vp_culture_turn_controls(tax_turn,0,false);
    check(!controls.no_policies&&!controls.anarchy&&controls.golden_age&&
          !controls.golden_age_disabled&&controls.temporary_bonus&&controls.conquest_bonus&&
          controls.temporary_boost_modifier==33&&controls.conquest_modifier==17&&
          controls.golden_age_modifier==20,
          "VP culture controls must read live Golden Age, saved bonus turns and selected Defines");
    saved_culture.golden_age_disabled=true;
    check(vp_culture_turn_controls(tax_turn,0,false).golden_age_modifier==0,
          "Saved VP Golden Age suppression must skip its modifier");
    saved_culture.golden_age_disabled=false;culture_history.anarchy_turns=1;
    const auto anarchy_controls=vp_culture_turn_controls(tax_turn,0,false);
    check(anarchy_controls.anarchy&&!anarchy_controls.temporary_bonus&&
          anarchy_controls.temporary_boost_modifier==0,
          "Anarchy must short-circuit VP culture modifiers");
    culture_history.anarchy_turns=0;
    const auto no_policy_controls=vp_culture_turn_controls(tax_turn,0,true);
    check(no_policy_controls.no_policies&&!no_policy_controls.golden_age&&
          no_policy_controls.golden_age_modifier==0,
          "No-policies option must short-circuit VP culture modifiers");
    cities.players[0].golden_age=false;saved_culture.temporary_bonus_turns=0;
    saved_culture.conquest_bonus_turns=0;saved_culture.conquest_modifier=0;
    ScienceContext culture_science;culture_science.profile=TechnologyProfile::vp542;
    ensure_vp_science_profile(culture_science.players[0]).vassal_modifier=25;
    rejects([&]{(void)vp_culture_vassals_times100(tax_turn,culture_science,0,false);});
    rejects([&]{(void)vp_culture_vassals_times100(tax_turn,culture_science,0,false,
        [](std::int64_t)->std::optional<std::int64_t>{return {};});});
    const auto vassal_culture=vp_culture_vassals_times100(tax_turn,culture_science,0,false,
        [](std::int64_t vassal)->std::optional<std::int64_t>{
            if(vassal!=1)throw std::runtime_error("Unexpected culture vassal");
            return 6000000000LL;
        });
    check(vassal_culture.modifier==45&&vassal_culture.by_vassal_times100.at(1)==2700000000LL&&
          vassal_culture.total_times100==2700000000LL,
          "VP culture vassals must use complete quoted income, selected rate and wide arithmetic");
    check(vp_culture_vassals_times100(tax_turn,culture_science,0,true).total_times100==0,
          "No-policies culture must skip the missing vassal income service");
    dip.teams[1].master=-1;
    check(vp_culture_vassals_times100(tax_turn,culture_science,0,false).total_times100==0,
          "No live vassal must not require a culture income service");
    dip.teams[1].master=0;
    RulesDatabase culture_religion_db;
    culture_religion_db.apply_sql("CREATE TABLE Beliefs(ID INTEGER PRIMARY KEY,PlayerCultureModifier INTEGER,RequiresPeace INTEGER);"
        "INSERT INTO Beliefs VALUES(1,35,1),(2,15,0),(3,3000000000,0);");
    const auto culture_religion_rules=load_vp_culture_religion_rules(culture_religion_db);
    const auto religion_culture=vp_culture_religion(tax_turn,culture_religion_rules,0);
    check(religion_culture.owned_religion==1&&religion_culture.base_times100==0&&
          religion_culture.modifier==35&&religion_culture.by_belief.at(1)==35,
          "VP 5.4.2 religion has zero flat culture and a peace-gated founder modifier");
    dip.teams[0].at_war.insert(1);
    check(vp_culture_religion(tax_turn,culture_religion_rules,0).modifier==0,
          "RequiresPeace belief must stop modifying culture during war");
    dip.teams[0].at_war.clear();
    s.religions[0].holy_plot_owner=1;
    check(vp_culture_religion(tax_turn,culture_religion_rules,0).modifier==0,
          "VP founder culture modifier follows live holy-plot ownership");
    s.religions[0].holy_plot_owner=0;
    rules.beliefs[2].follower=true;s.religions[0].beliefs.push_back(2);
    check(vp_culture_religion(tax_turn,culture_religion_rules,0).modifier==50,
          "Holy-city majority must permit a follower culture modifier");
    pm.city_majorities[10]=2;
    check(vp_culture_religion(tax_turn,culture_religion_rules,0).modifier==35,
          "Wrong holy-city majority must suppress its follower modifier");
    pm.city_majorities[10]=1;s.religions[0].beliefs.pop_back();
    s.religions[0].beliefs.push_back(1);
    check(vp_culture_religion(tax_turn,culture_religion_rules,0).modifier==70,
          "VP religion belief list preserves source duplicates");
    s.religions[0].beliefs.pop_back();
    rules.beliefs[3].civilization=-1;s.religions[0].beliefs.push_back(3);
    check(vp_culture_religion(tax_turn,culture_religion_rules,0).modifier==3000000035LL,
          "VP belief culture modifier must not inherit source i32 limits");
    s.religions[0].beliefs.pop_back();rules.beliefs.erase(3);
    o.entries["BALANCE_UNIQUE_BELIEFS_ONLY_FOR_CIV"].value=1;rules.beliefs[1].civilization=6;
    check(vp_culture_religion(tax_turn,culture_religion_rules,0).modifier==0,
          "Selected unique-belief option must honor the rule civilization");
    rules.beliefs[1].civilization=-1;o.entries["BALANCE_UNIQUE_BELIEFS_ONLY_FOR_CIV"].value=0;
    rejects([&]{(void)load_vp_culture_religion_rules(db);});
    ensure_vp_culture_income_saved_profile(setup).players[1].free_times100=6000000000LL;
    ensure_vp_culture_history_profile(setup).players[1];
    const auto full_culture=vp_resolve_culture_income_times100(
        tax_turn,culture_science,culture_religion_rules,2,0,false);
    check(full_culture.quotes_by_player.size()==2&&
          full_culture.quotes_by_player.at(1).total_times100==6000000000LL&&
          full_culture.inputs_by_player.at(0).vassals_times100==2700000000LL&&
          full_culture.inputs_by_player.at(0).religion_modifier==35&&
          full_culture.quote.before_modifiers_times100==2700007526LL&&
          full_culture.quote.total_times100==3645010160LL,
          "VP culture resolver must quote a vassal first and retain 64-bit source-order income");
    dip.teams[0].master=1;
    rejects([&]{(void)vp_resolve_culture_income_times100(
        tax_turn,culture_science,culture_religion_rules,2,0,false);});
    rejects([&]{(void)do_vp_culture_income_after_gold(setup,tax_turn,culture_science,
        culture_religion_rules,2,0,{});});
    check(culture_history.balance_times100==0&&culture_history.lifetime_times100==0,
          "Unresolved VP culture dependency must not credit any culture");
    dip.teams[0].master=-1;
    const auto culture_disabled=vp_resolve_culture_income_times100(
        tax_turn,culture_science,culture_religion_rules,2,0,true);
    check(culture_disabled.quote.total_times100==0&&culture_disabled.quotes_by_player.size()==1,
          "No-policies VP culture must short-circuit before vassal dependencies");
    auto unrelated_culture_world=setup;
    rejects([&]{(void)do_vp_culture_income_after_gold(unrelated_culture_world,tax_turn,
        culture_science,culture_religion_rules,2,0,{});});
    check(culture_history.balance_times100==0&&culture_history.lifetime_times100==0,
          "VP culture income must not credit a different imported world");
    check(do_vp_culture_history_after_gold(setup,0)==0,
          "VP culture income follows the saved-history update");
    const auto credited_culture=do_vp_culture_income_after_gold(setup,tax_turn,culture_science,
        culture_religion_rules,2,0,{});
    check(credited_culture.resolved.quote.total_times100==3645010160LL&&
          credited_culture.accrual.credited_times100==3645010160LL&&
          credited_culture.accrual.balance_after_times100==3645010160LL&&
          credited_culture.accrual.lifetime_after_times100==3645010160LL,
          "Complete 64-bit VP culture income must credit VP-owned balance and lifetime state");
    auto &tax_rates=ensure_vp_vassal_tax_profile(setup).rates_by_master_team;
    tax_rates[0][1]=25;
    s.players[1].connections_times100=10000000000LL;
    const auto vassal_gross=gross_gold_times100(s,w,m,rules,city_rules,vp_defs,&o,1).total_times100;
    const auto tax=do_vp_vassal_tax_after_gold(tax_turn,1);
    check(tax.vassal&&tax.master==0&&tax.gross_times100==vassal_gross&&tax.rate==25&&
          tax.expense_times100==wide::multiply_divide(vassal_gross,25,100)&&
          tax.expense_times100>INT32_MAX&&vp_maintenance_profile(m.players[1]).vassal_taxes_times100==tax.expense_times100,
          "VP after-gold vassal tax must use live gross income and wide master rate state");
    tax_rates[0].erase(1);
    rejects([&]{(void)do_vp_vassal_tax_after_gold(tax_turn,1);});
    tax_rates[0][1]=25;
    s.players[1].connections_times100=-5000000000LL;
    check(do_vp_vassal_tax_after_gold(tax_turn,1).expense_times100==0&&
          vp_maintenance_profile(m.players[1]).vassal_taxes_times100==0,
          "Nonpositive VP gross income must clear the vassal tax expense");
    s.players[1].connections_times100=0;
    check(!do_vp_vassal_tax_after_gold(tax_turn,0).vassal&&
          vp_maintenance_profile(m.players[0]).vassal_taxes_times100==0,
          "A non-vassal must clear its old VP tax expense without a master rate");
    ensure_vp_maintenance_profile(m.players[0],m.profile).vassal_taxes_times100=250;
    ensure_vp_maintenance_profile(m.players[1],m.profile).vassal_taxes_times100=199;
    {
        civ5::saves::SavedSessionSnapshot snapshot;snapshot.teams.emplace();
        snapshot.teams->layout=civ5::saves::GameStateLayout::vp542_corpus;
        snapshot.teams->dimensions.major=2;
        civ5::saves::TeamObservation master;master.fields={{"id",0}};
        master.arrays={{"vassal_tax",1,{0,25}}};
        civ5::saves::TeamObservation subordinate;subordinate.fields={{"id",1}};
        subordinate.arrays={{"vassal_tax",1,{0,0}}};
        snapshot.teams->teams={master,subordinate};
        ResearchContext imported;imported.profile=TechnologyProfile::vp542;
        imported.team_members={{0,1},{1,1}};
        restore_vp_vassal_tax_profile(snapshot,imported);
        check(find_vp_vassal_tax_profile(imported)->rates_by_master_team.at(0).at(1)==25,
              "VP saved team tax array must populate profile-owned rates");
        auto copied=imported;ensure_vp_vassal_tax_profile(copied).rates_by_master_team[0][1]=5000000001LL;
        check(find_vp_vassal_tax_profile(imported)->rates_by_master_team.at(0).at(1)==25,
              "Wide owned VP tax rates must copy independently");
        ResearchContext vanilla;rejects([&]{(void)ensure_vp_vassal_tax_profile(vanilla);});
        snapshot.teams->teams.push_back(master);
        rejects([&]{restore_vp_vassal_tax_profile(snapshot,imported);});
        snapshot.teams->teams.pop_back();
        snapshot.teams->teams[0].arrays[0].values.pop_back();
        rejects([&]{restore_vp_vassal_tax_profile(snapshot,imported);});
    }
    auto missing_income=s;missing_income.players[0].profile_extensions.values.clear();
    rejects([&]{gross_gold_times100(missing_income,w,m,rules,city_rules,vp_defs,&o,0);});
    auto net=net_gold_times100(s,w,m,rules,city_rules,maintenance_rules,vp_defs,&o,0);
    check(net.expenses&&net.expenses->total==8&&net.net_times100==9062&&p.balance_times100==500000000001LL,"Net treasury quote must subtract expenses without changing balance");
    const auto report=civ5::validation::treasury_report(s,w,m,rules,city_rules,maintenance_rules,vp_defs,&o);
    auto shuffled=rules;auto shuffled_state=s;
    shuffled.minors.clear();shuffled.minors[4000000001LL]=rules.minors.at(4);shuffled.minors[4000000000LL]=rules.minors.at(7);
    ensure_vp_treasury_minor_profile(shuffled_state.players[2]).minor_type=4000000000LL;
    check(report==civ5::validation::treasury_report(shuffled_state,w,m,shuffled,city_rules,maintenance_rules,vp_defs,&o),
        "Random SQL ID reorder must preserve every treasury rule, player type binding and quote");
    shuffled.minors[4000000000LL].friends[2][3]++;
    check(report!=civ5::validation::treasury_report(shuffled_state,w,m,shuffled,city_rules,maintenance_rules,vp_defs,&o),
        "Type-based comparison must still detect a changed minor bonus");
    shuffled.minors[4000000000LL]=rules.minors.at(7);ensure_vp_treasury_minor_profile(shuffled_state.players[2]).minor_type=4000000001LL;
    check(report!=civ5::validation::treasury_report(shuffled_state,w,m,shuffled,city_rules,maintenance_rules,vp_defs,&o),
        "Type-based comparison must still detect an incorrect player binding");
    p.anarchy=true;m.players[0].cached_unit_expense=123;
    auto missing_rules=rules;missing_rules.minors.clear();
    net=net_gold_times100(s,w,m,missing_rules,city_rules,maintenance_rules,vp_defs,&o,0);
    check(net.anarchy&&!net.expenses&&net.net_times100==0&&m.players[0].cached_unit_expense==123,"VP anarchy must skip income dependencies and expense cache mutations");p.anarchy=false;
    dip.teams[0].members={3,0};check(vp_treasury_income_quote(gross_gold_times100(s,w,m,rules,city_rules,vp_defs,&o,0)).vassal_taxes==99,"Tax remainder must follow stored leader order");dip.teams[0].members={0,3};
    setup.players[2].alive=false;check(vp_treasury_income_quote(gross_gold_times100(s,w,m,rules,city_rules,vp_defs,&o,0)).minors==0,"Dead source minor must not contribute");setup.players[2].alive=true;
    csm.friends.clear();cs.ally=-1;csm.permanent_ally=0;
    check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).allies==230,"Permanent alliance must grant ally yield");
    csm.no_ally=true;check(minor_yield_times100(s,w,m,missing_rules,vp_defs,o,2,0,2).total_times100==0,"Unused minor Info must remain lazy");
    cs.ally=0;check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).allies==230,"NoAlly must not cancel explicit saved ally");csm.friends={0};
    pm.trait_cs_modifier=-50;pm.cs_modifier=-20;
    check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).total_times100==363,"Nonpositive minor modifier must not reduce yield");pm.trait_cs_modifier=10;pm.cs_modifier=15;
    o.entries["BALANCE_UNIQUE_BELIEFS_ONLY_FOR_CIV"].value=1;rules.beliefs[1].civilization=6;
    check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).modifier==25,"Unique belief must honor civilization restriction");rules.beliefs[1].civilization=-1;
    s.religions[0].holy_plot_owner=1;
    check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).modifier==25,"Founder belief must use live holy-plot ownership");s.religions[0].holy_plot_owner=0;
    rules.beliefs[1].founder=false;rules.beliefs[1].follower=true;pm.city_majorities[10]=2;
    check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).modifier==25,"Follower belief must check the holy city's actual majority");
    pm.city_majorities[10]=1;rules.beliefs[1].follower=false;rules.beliefs[1].reformation=true;
    s.religions[0].holy_plot_owner=2;s.religions[0].holy_city={{2,20}};
    check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).modifier==45,"Reformation belief permits matching state religion without owning the holy city");
    rules.beliefs[1].reformation=false;rules.beliefs[1].pantheon=true;s.religions[0].holy_plot_owner=0;s.religions[0].holy_city={{0,10}};
    pm.city_majorities[10]=2;TreasuryReligion pantheon;pantheon.religion=pantheon.belief_religion=0;pantheon.founder=0;pantheon.pantheon=true;pantheon.belief_lookup={1};s.religions.push_back(pantheon);
    check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).modifier==45,"Merged pantheon membership must use both saved lookup tables");
    s.religions[0].belief_lookup.clear();
    check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).modifier==25,"Pantheon belief list must not replace HasBelief lookup membership");
    s.religions[0].belief_lookup={1};s.religions.pop_back();pm.city_majorities[10]=1;rules.beliefs[1].pantheon=false;rules.beliefs[1].founder=true;
    s.religions[0].beliefs={1,1};
    check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).modifier==65,"VP belief getter must preserve duplicate list entries");s.religions[0].beliefs={1};
    o.entries["BALANCE_VP"].value=0;vp_religion.owns_state_religion=false;
    check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).modifier==45,"CP ownership must use founded religion and actual holy city instead of VP cached flag");
    o.entries["BALANCE_VP"].value=1;vp_religion.owns_state_religion=true;
    vp_religion.state_religion=0;check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).modifier==25,"Pantheon ownership must not count as owned VP religion");vp_religion.state_religion=1;
    rules.minors[7].friends[2]={{4294967301LL,9000000000LL}};m.team_eras[0]=4294967302LL;
    check(minor_yield_times100(s,w,m,rules,vp_defs,o,2,0,2).friends==9000000000LL,"Wide era/amount must not require a bounded array or dense era scan");m.team_eras[0]=3;
    p.connections_times100=INT64_MAX;rejects([&]{gross_gold_times100(s,w,m,rules,city_rules,vp_defs,&o,0);});p.connections_times100=137;
    // BNW uses signed diplomacy and native founder benefits, with an injectable
    // Lua accumulator. It does not apply VP's anarchy short circuit.
    setup.profile=dip.profile=cities.profile=m.profile=s.profile=rules.profile=city_rules.profile=maintenance_rules.profile=TechnologyProfile::bnw;
    p.anarchy=true;m.players[0].diplomacy_gold=-3;c.religions={{1,5,20}};
    cities.players[2].cities[20].religions={{1,3,9},{1,99,0}}; // first matching row supplies follower count
    cities.players[2].cities[21].religions={{1,1,4}};
    gross=gross_gold_times100(s,w,m,rules,city_rules,bnw_defs,nullptr,0);
    check(gross.religion==800&&gross.traits==400&&gross.diplomacy==-300&&gross.total_times100==2037&&
        !gross.profile_extensions.find<VpTreasuryIncomeQuote>(),
        "BNW religion must count alive cities, first follower row and distinct beliefs");
    net=net_gold_times100(s,w,m,rules,city_rules,maintenance_rules,bnw_defs,nullptr,0);
    check(!net.anarchy&&net.net_times100==1537,"BNW net gold must not inherit VP anarchy gate");
    s.religions[0].holy_plot_owner=1;check(gross_gold_times100(s,w,m,rules,city_rules,bnw_defs,nullptr,0).religion==0,"BNW lost holy plot must remove native founder benefits");
    TreasuryServices hooks;std::int64_t calls=0;hooks.founder_benefits_religion=[&](auto who)->std::optional<std::int64_t>{check(who==0,"Wrong founder hook player");++calls;return 1;};
    check(gross_gold_times100(s,w,m,rules,city_rules,bnw_defs,nullptr,0,hooks).religion==800&&calls==1,"Lua accumulator override must precede native eligibility");
    hooks.founder_benefits_religion=[](auto)->std::optional<std::int64_t>{return -1;};
    check(gross_gold_times100(s,w,m,rules,city_rules,bnw_defs,nullptr,0,hooks).religion==0,"Lua NO_RELIGION is an override, not missing handler");
    LuaScriptHost unloaded;
    rejects([&]{(void)lua_treasury_services(unloaded);});
    unloaded.complete_loading();
    auto no_handlers=lua_treasury_services(unloaded);
    s.religions[0].holy_plot_owner=0;
    check(!no_handlers.founder_benefits_religion(0)&&
          gross_gold_times100(s,w,m,rules,city_rules,bnw_defs,nullptr,0,no_handlers).religion==800,
          "No Lua listener must retain native founder selection and holy-plot eligibility");
    s.religions[0].holy_plot_owner=1;
    LuaScriptHost scripted;
    scripted.execute("treasury-mod",R"lua(
        GameEvents.GetFounderBenefitsReligion.Add(function(player)
            if player == 0 then return 1 end
            if player == 5000000001 then return 5000000002 end
        end)
    )lua","=treasury-mod.lua");
    scripted.complete_loading();
    auto lua_hooks=lua_treasury_services(scripted);
    check(gross_gold_times100(s,w,m,rules,city_rules,bnw_defs,nullptr,0,lua_hooks).religion==800&&
          lua_hooks.founder_benefits_religion(5000000001LL)==5000000002LL,
          "Data-driven Lua founder hook must alter BNW gold without narrowing wide IDs");
    scripted.execute("treasury-mod","GameEvents.GetFounderBenefitsReligion.Add(function() error('failed mod') end)",
                     "=treasury-failure.lua");
    rejects([&]{(void)lua_hooks.founder_benefits_religion(0);});
    scripted.complete_loading();
    rejects([&]{(void)gross_gold_times100(s,w,m,rules,city_rules,bnw_defs,nullptr,0,lua_hooks);});
    rejects([&]{gross_gold_times100(s,w,m,rules,city_rules,vp_defs,&o,0);});
    // SQL cache keeps duplicate bonuses and wide sparse Info IDs.
    RulesDatabase info_db;info_db.apply_sql("CREATE TABLE Beliefs(ID,Type,GoldPerFollowingCity,GoldPerXFollowers);INSERT INTO Beliefs VALUES(0,'B',2,4);"
        "CREATE TABLE Resources(ID,Type,ResourceUsage);INSERT INTO Resources VALUES(4294967300,'LUX',2);"
        "CREATE TABLE MinorCivilizations(ID,Type,MinorCivTrait);INSERT INTO MinorCivilizations VALUES(4294967301,'MINOR','TRAIT');"
        "CREATE TABLE Eras(ID,Type);INSERT INTO Eras VALUES(4294967302,'ERA');CREATE TABLE Yields(ID,Type);INSERT INTO Yields VALUES(2,'GOLD');"
        "CREATE TABLE MinorCivilizationTraits_FriendYieldBonuses(MinorCivTraitType,EraType,YieldType,Yield);"
        "INSERT INTO MinorCivilizationTraits_FriendYieldBonuses VALUES('TRAIT','ERA','GOLD',150),('TRAIT','ERA','GOLD',-20);"
        "CREATE TABLE MinorCivilizationTraits_AllyYieldBonuses(MinorCivTraitType,EraType,YieldType,Yield);");
    const auto loaded=load_treasury_rules(info_db,TechnologyProfile::vp542,info_db.cache_info_types());
    check(loaded.luxury_resources.contains(4294967300LL)&&loaded.minors.at(4294967301LL).friends.at(2).at(4294967302LL)==130,"Treasury SQL cache lost wide IDs or additive duplicate rows");
    const auto types=info_db.cache_info_types();
    for(bool vp:{false,true}){
        using namespace civ5::saves;
        const auto profile=vp?TechnologyProfile::vp542:TechnologyProfile::bnw;
        const auto layout=vp?GameStateLayout::vp542_corpus:GameStateLayout::expansion2_version1;
        const auto rr=load_treasury_rules(info_db,profile,types);
        ResearchContext rs;PlayerResearchContext rd;CityYieldContext cy;rs.profile=rd.profile=cy.profile=profile;
        rs.players[0]={};rd.players[0]={};cy.players[0]={};MaintenanceWorld view{rs,rd,cy,{}};
        SavedSessionSnapshot snap{};snap.all_players.emplace();snap.pregame.emplace();snap.map_plots.emplace();
        snap.all_players->layout=snap.pregame->layout=snap.map_plots->layout=snap.trade.layout=snap.religion_culture.layout=layout;
        snap.pregame->archive.string_arrays["minor_civ_types"]={"MINOR"};snap.map_header.grid={1,1};
        PlotObservation plot{};plot.fields={{"owner",0},{"plot_city_owner",-1},{"plot_city_id",-1},{"is_city",0},{"owning_city_owner",0},{"owning_city_id",123}};snap.map_plots->plots={plot};
        SavedPlayerObservation saved{};saved.saved_player_id=0;
        saved.core.layout=saved.traits.layout=saved.administration.layout=saved.tail.layout=saved.knowledge.layout=saved.planning.layout=layout;
        saved.core.start_offset=10;saved.core.end_offset=20;saved.administration.start_offset=30;saved.administration.end_offset=40;
        saved.traits.version=6;
        const auto core=[&](const char *name,std::vector<std::int64_t> values){PlayerCoreField f{};f.name=name;f.values=values;saved.core.fields.push_back(f);};
        core("m_iAnarchyNumTurns",{2});core("m_iCSYieldBonusModifier",{7});
        for(const auto *name:{"m_aiYieldFromMinors","m_piYieldPerTurnFromAnnexedMinorsTimes100","m_paiResourceExport","m_paiResourceImportFromMajor","m_paiResourceFromMinors","m_paiResourceFromCSAlliances","m_paiResourcesSiphoned"})core(name,{0,5,6});
        for(const auto *name:{"m_iYieldChangePerTradePartner","m_iYieldFromExport","m_iYieldFromImport","m_iCityStateBonusModifier"}){
            PlayerTraitsField f{};f.name=name;f.values=std::string_view(name)=="m_iCityStateBonusModifier"?std::vector<std::int64_t>{8}:std::vector<std::int64_t>{0,0,9};saved.traits.fields.push_back(f);}
        PlayerAdministrationRecord tr{};tr.name="treasury";tr.fields={{"m_iGold",500000000001LL},{"m_iCityConnectionGoldTimes100",137},{"m_iCityConnectionTradeRouteGoldModifier",-4294967301LL},{"m_iCityConnectionTradeRouteGoldChange",4294967302LL},{"m_iInternalTradeGoldBonus",3},{"m_iLifetimeGrossGoldIncome",6000000001LL}};
        PlotArrayObservation balances{},incomes{};balances.name="m_GoldBalanceForTurnTimes100";balances.values={500000000001LL,99};
        incomes.name="m_GoldChangeForTurnTimes100";incomes.values={10000000001LL,-234};tr.arrays={balances,incomes};saved.administration.blocks={tr};
        PlayerTailNode incoming{};incoming.name="m_aiIncomingEspionageYields";incoming.values={0,0,4};saved.tail.fields={incoming};
        PlayerKnowledgeBlock rel{};rel.name="player_religions";rel.fields={{"state_religion",1},{"owns_state_religion",1}};saved.knowledge.blocks={rel};
        PlayerPlanningRecord minor{};minor.name="minor_civ_ai";minor.fields={{"m_eAlly",2},{"m_ePermanentAlly",3},{"m_bNoAlly",1}};
        PlotArrayObservation friends{};friends.name="m_abFriends";friends.values={0,1};minor.arrays={friends};saved.planning.blocks={minor};saved.planning.dimensions={0,1,1};
        snap.all_players->players={saved};ReligionObservation rel_saved{};rel_saved.religion=1;rel_saved.founder=0;rel_saved.holy_city_x=rel_saved.holy_city_y=0;
        rel_saved.beliefs.religion=1;rel_saved.beliefs.belief_ids={1,1};rel_saved.beliefs.belief_lookup={0,1,2};rel_saved.beliefs.belief_hashes={saved_info_type_hash("B"),saved_info_type_hash("B")};snap.religion_culture.religions={rel_saved};
        auto restored=restore_treasury(snap,view,rr,types);const auto &rp=restored.players.at(0);
        check(rp.connection_modifier==-4294967301LL&&rp.connection_change==4294967302LL,"Treasury import preserves wide signed connection changes and modifiers in both profiles");
        check(rp.lifetime_gross_gold==6000000001LL&&rp.balance_history_times100==balances.values&&rp.income_history_times100==incomes.values,"Treasury import lost wide signed histories/lifetime income");
        check(rp.balance_times100==500000000001LL&&rp.connections_times100==137&&rp.anarchy&&rp.gold_per_partner==(vp?9:0)&&rp.source_core==std::pair<std::uint64_t,std::uint64_t>{10,20},"Treasury restoration lost source offsets, wide balance or legacy trait defaults");
        check(vp?(vp_treasury_minor_profile(rp).minor_type==4294967301LL&&vp_treasury_minor_profile(rp).friends.contains(1)&&
            vp_treasury_religion_profile(rp).state_religion==1&&vp_treasury_religion_profile(rp).owns_state_religion&&
            vp_treasury_income_profile(rp).siphoned.at(1)==5&&
            vp_treasury_income_profile(rp).incoming_espionage==4&&restored.minor_players.contains(0)):
            (!rp.profile_extensions.find<VpTreasuryMinorProfile>()&&
             !rp.profile_extensions.find<VpTreasuryIncomeProfile>()&&
             !rp.profile_extensions.find<VpTreasuryReligionProfile>()),
            "Treasury VP inputs must use saved minor type/relations and resource arrays");
        check(restored.religions[0].holy_plot_owner==0&&!restored.religions[0].holy_city&&
            (vp?(restored.religions[0].beliefs.size()==2&&restored.religions[0].belief_lookup==std::set<std::int64_t>{1}):(restored.religions[0].belief_lookup==std::set<std::int64_t>{0})),
            "Religion restoration must preserve VP duplicate belief list, exact lookup==1, and BNW unique hash membership");
        snap.all_players->players.push_back(saved);rejects([&]{restore_treasury(snap,view,rr,types);});
    }
}
