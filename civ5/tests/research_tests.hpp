#pragma once
void research_tests() {
    using namespace civ5::runtime;
    using wide::multiply_divide;
    check(multiply_divide(INT64_MAX,100,100) == INT64_MAX &&
          multiply_divide(INT64_MIN,100,100) == INT64_MIN &&
          multiply_divide(INT64_MIN,-1,2) == (1LL<<62) &&
          multiply_divide(INT64_MAX,INT64_MAX,INT64_MAX) == INT64_MAX &&
          multiply_divide(INT64_MIN,INT64_MAX,INT64_MAX) == INT64_MIN &&
          multiply_divide(-7,5,3) == -11 && multiply_divide(7,-5,3) == -11 &&
          multiply_divide(-7,-5,3) == 11, "Wide multiply/divide loses high product or signed truncation");
    rejects([&] { multiply_divide(1,1,0); }); rejects([&] { multiply_divide(1,1,-1); });
    rejects([&] { multiply_divide(INT64_MIN,-1,1); });
    rejects([&] { multiply_divide(INT64_MIN,INT64_MIN,INT64_MAX); });
    check(wide::add(INT64_MIN,INT64_MAX) == -1 && wide::subtract(INT64_MIN,INT64_MIN) == 0,
          "Wide addition/subtraction rejected cancellation");
    rejects([&] { wide::add(INT64_MAX,1); }); rejects([&] { wide::add(INT64_MIN,-1); });
    rejects([&] { wide::subtract(INT64_MIN,1); }); rejects([&] { wide::subtract(INT64_MAX,-1); });
#if defined(__SIZEOF_INT128__)
    // Independent compiler-wide oracle; production uses only portable uint64.
    __extension__ using OracleInteger = __int128;
    std::uint64_t seed = 0x510e527fade682d1ULL;
    const auto next = [&]() { seed ^= seed<<13; seed ^= seed>>7; seed ^= seed<<17; return seed; };
    for (unsigned trial=0; trial<6000; ++trial) {
        const auto a=std::bit_cast<std::int64_t>(next()), b=std::bit_cast<std::int64_t>(next());
        auto d=static_cast<std::int64_t>(next()&INT64_MAX);
        if (!d) d=1;
        const OracleInteger expected=static_cast<OracleInteger>(a)*b/d;
        if (expected<INT64_MIN || expected>INT64_MAX) rejects([&] { multiply_divide(a,b,d); });
        else check(multiply_divide(a,b,d)==expected,"Portable wide arithmetic diverged from 128-bit oracle");
    }
#endif
    const auto bnw=TechnologyProfile::bnw, vp=TechnologyProfile::vp542;
    RulesDatabase db; technology_schema(db);
    db.apply_sql("ALTER TABLE Technologies ADD COLUMN EurekaPerMillion;"
                 "INSERT INTO Technologies(ID,Type,Cost,EurekaPerMillion) VALUES(0,'A',1000,100000),(1,'B',1000,0),(3,'C',10001,0);"
                 "CREATE TABLE Defines(Name,Value); INSERT INTO Defines VALUES('NUM_AND_TECH_PREREQS',4),('NUM_OR_TECH_PREREQS',4);");
    const auto types=db.cache_info_types();
    auto bnw_rules=load_technology_rules(db,types,bnw,5), vp_rules=load_technology_rules(db,types,vp,5);
    auto bnw_save=saved_team_techs(bnw,{saved_info_type_hash("A")});
    auto vp_save=saved_team_techs(vp,{saved_info_type_hash("A")});
    bnw_save.arrays[3].values[0]=199; vp_save.arrays[3].values[0]=199;
    auto b=restore_team_technologies(bnw_save,bnw_rules,types), v=restore_team_technologies(vp_save,vp_rules,types);
    TeamResearchFactors f;
    const auto reset_factors=[&]{f={};ensure_vp_research_cost_profile(f);};
    reset_factors();
    check(team_research_cost(bnw_rules,b,0,f)==1000 && team_research_cost(vp_rules,v,0,f)==1000 &&
          bnw_research_left(bnw_rules,b,0,f)==999 && vp_research_left_times100(vp_rules,v,0,f)==99801 &&
          team_research_cost(bnw_rules,b,2,f)==0 && team_research_cost(vp_rules,v,2,f)==0,
          "Team cost baseline, missing entry or source progress units changed");
    f.world_percent=3; f.speed_percent=150;
    check(team_research_cost(bnw_rules,b,0,f)==45 && team_research_cost(vp_rules,v,0,f)==40,
          "BNW rounds costs but VP rounds modifiers before applying base cost");
    reset_factors(); f.team_handicap_percent=100;
    auto &vp_handicap=ensure_vp_research_cost_profile(f);
    vp_handicap.team_handicap_per_era_modifier=-10; vp_handicap.current_era=3;
    vp_handicap.game_ai_handicap_percent=50;vp_handicap.game_ai_per_era_modifier=-20;
    vp_handicap.human_for_handicap=false;
    check(team_research_cost(vp_rules,v,0,f)==140 && team_research_cost(bnw_rules,b,0,f)==1000,
          "Era/AI handicap ordering or BNW profile separation changed");
    vp_handicap.human_for_handicap=true;
    check(team_research_cost(vp_rules,v,0,f)==700,"Human team received AI handicap");
    f.team_handicap_percent.reset(); vp_handicap.human_for_handicap=false;
    check(team_research_cost(vp_rules,v,0,f)==1000,"Absent team handicap must also skip AI adjustments");
    reset_factors(); f.team_handicap_percent=73; f.world_percent=115; f.speed_percent=67;
    f.start_era_percent=143; f.members=3; f.extra_team_member_modifier=50;
    check(team_research_cost(bnw_rules,b,0,f)==1606 && team_research_cost(vp_rules,v,0,f)==1560,
          "Handicap/world/speed/start-era/member stages changed order or rounding");
    reset_factors(); f.team_handicap_percent=100;
    ensure_vp_research_cost_profile(f).team_handicap_per_era_modifier=-100;
    ensure_vp_research_cost_profile(f).current_era=2;
    check(team_research_cost(vp_rules,v,0,f)==1 && team_research_cost(bnw_rules,b,0,f)==1000,
          "Negative era factor was not clamped before multiplication");
    reset_factors(); f.extra_team_member_modifier=-25; f.members=3;
    check(team_research_cost(bnw_rules,b,0,f)==500 && team_research_cost(vp_rules,v,0,f)==1000,
          "BNW and VP member factor clamp placement changed");
    f.members=0; f.extra_team_member_modifier=50;
    check(team_research_cost(bnw_rules,b,0,f)==500 && team_research_cost(vp_rules,v,0,f)==1000,
          "Empty team cost or negative member delta changed");
    f.members=4;
    check(team_research_cost(bnw_rules,b,0,f)==2500 && team_research_cost(vp_rules,v,0,f)==2500,
          "Extra team member cost modifier lost");
    reset_factors(); ensure_vp_research_cost_profile(f).eurekas_enabled=true; f.members=2; vp_team_set_eureka(v,0,3);
    check(team_research_cost(vp_rules,v,0,f)==850 && vp_eureka_discount(vp_rules,v,0)==70,
          "Eureka cost uses per-member effect but discount getter does not");
    f.world_percent=150;
    check(team_research_cost(vp_rules,v,0,f)==1350,"Eureka must subtract percentage points after other factors");
    vp_team_set_eureka(v,0,100);
    check(team_research_cost(vp_rules,v,0,f)==500 && vp_eureka_discount(vp_rules,v,0)==0,
          "Eureka clamp or additive modifier changed");
    vp_team_set_eureka(v,0,-2); f.world_percent=100;
    check(team_research_cost(vp_rules,v,0,f)==1100 && vp_eureka_discount(vp_rules,v,0)==120,
          "Negative source eureka counter was clamped");
    ensure_vp_research_cost_profile(f).eurekas_enabled=false;
    check(team_research_cost(vp_rules,v,0,f)==1000,"Disabled eureka still affected cost");
    ensure_vp_research_cost_profile(f).eurekas_enabled=true; f.members=0; vp_team_set_eureka(v,0,3);
    check(team_research_cost(vp_rules,v,0,f)==700 && vp_eureka_discount(vp_rules,v,2)==0,
          "Empty-team eureka divisor or missing-tech discount changed");
    rejects([&] { vp_eureka_discount(vp_rules,v,-1); });
    reset_factors(); bnw_save.arrays[3].values[0]=200001; vp_save.arrays[3].values[0]=200001;
    check(bnw_research_left(bnw_rules,restore_team_technologies(bnw_save,bnw_rules,types),0,f)==0 &&
          vp_research_left_times100(vp_rules,restore_team_technologies(vp_save,vp_rules,types),0,f)==0,
          "Research-left query returned negative beakers after completion");
    reset_factors(); f.world_percent=125;
    check(team_research_cost(vp_rules,v,3,f)==12501,"VP x86 overflow workaround still truncates base>=10000");
    vp_rules.entries.at(0).integers["Cost"]=INT64_MAX;
    reset_factors();
    check(team_research_cost(vp_rules,v,0,f)==INT64_MAX,"Research multiplication narrowed a fitting 64-bit result");
    rejects([&] { vp_research_left_times100(vp_rules,v,0,f); });
    vp_rules.entries.at(0).integers["Cost"]=5000000001LL;
    f.world_percent=200;
    check(team_research_cost(vp_rules,v,0,f)==10000000002LL,"Technology cost retained x86 ceiling");
    for (const auto cost : {0LL,-100LL}) {
        vp_rules.entries.at(0).integers["Cost"]=cost;
        check(team_research_cost(vp_rules,v,0,f)==1,"Final minimum technology cost changed");
    }
    rejects([&] { team_research_cost(vp_rules,v,-1,f); });
    rejects([&] { team_research_cost(vp_rules,v,0,TeamResearchFactors{}); });
    check(team_research_cost(bnw_rules,b,0,TeamResearchFactors{})==1000,
          "BNW research cost unexpectedly requires a VP profile extension");
    rejects([&] { team_research_cost(vp_rules,b,0,f); });
    rejects([&] { bnw_research_left(vp_rules,v,0,f); });
    rejects([&] { vp_eureka_discount(bnw_rules,b,0); });
    auto partial=bnw_rules; partial.errors=1;
    rejects([&] { team_research_cost(partial,b,0,f); });
    for (auto *rules : {&bnw_rules,&vp_rules}) {
        auto &a=rules->entries.at(0);
        a.or_prerequisites.values={{1,1}}; a.and_prerequisites.values={{0,3}};
        auto &team=rules->profile==bnw?b:v;
        const ResearchPrerequisiteLimits limits{4,4};
        check(!research_prerequisites_met(*rules,team,0,limits),"Missing OR prerequisite accepted");
        team.set_has_tech(1,true);
        check(!research_prerequisites_met(*rules,team,0,limits),"OR success bypassed required AND technology");
        team.set_has_tech(3,true); team.set_no_trade(1,true);
        check(research_prerequisites_met(*rules,team,0,limits) && !research_prerequisites_met(*rules,team,0,limits,true),
              "Research and trading prerequisite gates conflated");
        team.set_no_trade(1,false); team.set_no_trade(3,true);
        check(!research_prerequisites_met(*rules,team,0,limits,true),"AND no-trade gate ignored");
        team.set_no_trade(3,false);
        check(research_prerequisites_met(*rules,team,0,limits,true) && !research_prerequisites_met(*rules,team,2,limits),
              "Valid trade prerequisites or missing tech entry failed");
        check(research_prerequisites_met(*rules,team,0,{-1,0}),"Negative source loop limit was turned into unsigned work");
        rejects([&] { research_prerequisites_met(*rules,team,0,{5,4}); });
        check(research_prerequisites_met(*rules,team,0,{4,INT64_MAX}),"OR early success must precede array boundary rejection");
        a.or_prerequisites.values.clear();
        rejects([&] { research_prerequisites_met(*rules,team,0,{4,INT64_MAX}); });
        a.or_prerequisites.extent=static_cast<std::uint64_t>(INT64_MAX);
        check(research_prerequisites_met(*rules,team,0,{4,INT64_MAX}),"Sparse prerequisite walk iterated empty ID range");
        const auto cache=db.cache_global_defines(rules->profile==bnw?GlobalDefineProfile::bnw:GlobalDefineProfile::vp542);
        const auto defined=research_prerequisite_limits(cache);
        check(defined.and_count==4 && defined.or_count==4,"Prerequisite limits bypassed selected define cache");
    }
    // An empty AND condition makes VP's AI helper true even when OR is unmet.
    auto &a=vp_rules.entries.at(0);
    a.and_prerequisites.values.clear(); a.or_prerequisites.values={{0,1}};
    v.set_has_tech(1,false); v.set_has_tech(3,false);
    check(vp_ai_prerequisites_met(vp_rules,v,0,{4,4}) && !research_prerequisites_met(vp_rules,v,0,{4,4}),
          "VP AI helper was incorrectly replaced by player CanResearch prerequisites");
    a.and_prerequisites.values={{0,3}};
    check(!vp_ai_prerequisites_met(vp_rules,v,0,{4,4}) && vp_ai_prerequisites_met(vp_rules,v,0,{4,4},{1}) &&
          vp_ai_prerequisites_met(vp_rules,v,0,{4,4},{3}),"VP AI OR combination or extra-tech membership changed");
    a.and_prerequisites.values={{1,3}}; a.or_prerequisites.values.clear();
    check(vp_ai_prerequisites_met(vp_rules,v,0,{4,4}),"VP AI did not stop at first NO_TECH");
    // Four levels, non-contiguous IDs and a cycle: frontier includes exactly
    // the first two layers, using one fixed first-layer set for the second pass.
    TechnologyRules chain; chain.profile=vp; chain.collection.slots=5000000005ULL;
    for (const auto id : {0ULL,1ULL,2ULL,3ULL,5000000004ULL}) {
        TechnologyInfo info;
        info.and_prerequisites={6,-1,{}}; info.or_prerequisites={6,-1,{}};
        if (id>0 && id<4) info.and_prerequisites.values[0]=static_cast<std::int64_t>(id-1);
        if (id==5000000004ULL) info.and_prerequisites.values[0]=5000000004LL;
        chain.entries.emplace(id,std::move(info));
    }
    TeamTechnologies state(vp,0,chain.collection.slots);
    check(vp_tech_frontier(chain,state,{6,6})==std::vector<std::int64_t>{0,1},"VP frontier leaked more than two layers or a cycle");
    state.set_has_tech(0,true);
    check(vp_tech_frontier(chain,state,{6,6})==std::vector<std::int64_t>{1,2},"VP frontier included known tech or omitted second layer");
    rejects([&] { vp_tech_frontier(bnw_rules,b,{4,4}); });
    // Dense source loops vs sparse traversal, with gaps, explicit sentinels,
    // repeated dependencies and independently varied trade restrictions.
    std::uint64_t query_seed=0x1f83d9abfb41bd6bULL;
    const auto draw=[&]() { query_seed^=query_seed<<13; query_seed^=query_seed>>7; query_seed^=query_seed<<17; return query_seed; };
    for (unsigned trial=0; trial<512; ++trial) {
        TechnologyRules rules; rules.profile=vp; rules.collection.slots=8;
        TechnologyInfo info; info.and_prerequisites={8,-1,{}}; info.or_prerequisites={8,-1,{}};
        std::array<std::int64_t,8> ands,ors; std::array<bool,8> owned,trade,extra;
        TeamTechnologies team(vp,0,8); std::set<std::int64_t> extras;
        for (std::size_t i=0; i<8; ++i) {
            ands[i]=static_cast<std::int64_t>(draw()%9)-1; ors[i]=static_cast<std::int64_t>(draw()%9)-1;
            if (ands[i]!=-1 || draw()%2) info.and_prerequisites.values[i]=ands[i];
            if (ors[i]!=-1 || draw()%2) info.or_prerequisites.values[i]=ors[i];
            owned[i]=draw()%2; trade[i]=draw()%2; extra[i]=draw()%2;
            team.set_has_tech(static_cast<std::int64_t>(i),owned[i]); team.set_no_trade(static_cast<std::int64_t>(i),trade[i]);
            if (extra[i]) extras.insert(static_cast<std::int64_t>(i));
        }
        rules.entries.emplace(0,std::move(info));
        const auto na=draw()%9, no=draw()%9;
        for (const bool trading : {false,true}) {
            bool possible=false, any=false, all=true;
            for (std::size_t i=0; i<no; ++i) if (ors[i]!=-1) {
                possible=true; if (owned[ors[i]] && (!trading || !trade[ors[i]])) { any=true; break; }
            }
            for (std::size_t i=0; i<na; ++i) if (ands[i]!=-1 && (!owned[ands[i]] || (trading && trade[ands[i]]))) { all=false; break; }
            check(research_prerequisites_met(rules,team,0,{static_cast<std::int64_t>(na),static_cast<std::int64_t>(no)},trading)==
                  ((!possible||any)&&all),"Sparse player prerequisite traversal diverged from source loops");
        }
        bool all=true, any=false;
        for (std::size_t i=0; i<na && ands[i]!=-1; ++i) if (!owned[ands[i]] && !extra[ands[i]]) { all=false; break; }
        for (std::size_t i=0; i<no && ors[i]!=-1; ++i) if (owned[ors[i]] || extra[ors[i]]) { any=true; break; }
        check(vp_ai_prerequisites_met(rules,team,0,{static_cast<std::int64_t>(na),static_cast<std::int64_t>(no)},extras)==(all||any),
              "Sparse VP AI prerequisite traversal diverged from source loops");
    }
}
