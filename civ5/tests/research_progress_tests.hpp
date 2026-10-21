#pragma once
// The service double tests dispatch/ordering, not implementation of the
// complete world's setHasTech, science production or culture mechanics.
struct ResearchProgressTestWorld final : civ5::runtime::ResearchProgressServices {
    std::vector<civ5::runtime::TeamTechnologies> *teams = nullptr;
    civ5::runtime::PlayerResearchContext *players = nullptr;
    std::vector<std::string> events;
    std::map<std::int64_t,std::int64_t> bonuses, culture;
    std::int64_t science = 100, seen_overflow = 0, seen_progress = 0;
    bool throw_on_acquire = false, reset_repeat = false;
    std::int64_t science_times100(std::int64_t p) override { events.push_back("science:"+std::to_string(p)); return science; }
    void game_data_and_score_dirty(std::int64_t t) override {
        events.push_back("dirty:"+std::to_string(t)); seen_progress=teams->front().progress_times100(0);
    }
    void acquire_technology(std::int64_t t, std::int64_t tech, std::int64_t p) override {
        events.push_back("acquire:"+std::to_string(t)+":"+std::to_string(tech)+":"+std::to_string(p));
        seen_overflow=players->players.at(p).overflow_times100;
        if (throw_on_acquire) throw std::runtime_error("world completion failed");
        for (auto &team : *teams) if (team.team()==t) {
            check(!team.at(tech).no_trade,"NoTrade must follow world acquisition");
            if (reset_repeat) { team.increment_count(tech); team.set_progress_times100(tech,0); }
            else team.set_has_tech(tech,true);
        }
    }
    void city_specializations_dirty(std::int64_t p) override { events.push_back("specialize:"+std::to_string(p)); }
    std::int64_t culture_per_tech(std::int64_t p) override { events.push_back("culture_get:"+std::to_string(p)); return bonuses[p]; }
    void add_culture(std::int64_t p, std::int64_t amount) override {
        events.push_back("culture_add:"+std::to_string(p)+":"+std::to_string(amount)); culture[p]+=amount;
    }
};
void research_progress_tests() {
    using namespace civ5::runtime;
    const auto parsed=civ5::validation::parse_research_probe("4294967300,9223372036854775807,-100,0");
    check(parsed.player==4294967300LL && parsed.tech==INT64_MAX && parsed.change==-100 && parsed.science==0,"Research probe narrowed signed input");
    for (const auto *invalid : {"", "0,1,2", "0,1,2,3,4", "0,,2,3", "0,1,2,3x", "0,1,2,9223372036854775808"})
        rejects([&] { civ5::validation::parse_research_probe(invalid); });
    RulesDatabase option_db;
    option_db.apply_sql("CREATE TABLE GameOptions(Type,\"Default\"); INSERT INTO GameOptions VALUES('GAMEOPTION_NO_SCIENCE',1);");
    ResearchContext option_context;
    check(option_context.game_option_value(option_db,"GAMEOPTION_NO_SCIENCE")==1 && option_context.game_options.empty(),"Turn zero lookup must not append option");
    option_context.game_turn=1;
    check(option_context.game_option_value(option_db,"GAMEOPTION_NO_SCIENCE")==1 && option_context.game_options.size()==1,"Option default must cache after turn zero");
    option_context.game_options={{std::string("GAMEOPTION_NO_SCIENCE\0suffix",27),0},{"GAMEOPTION_NO_SCIENCE",1}};
    check(option_context.game_option_value(option_db,"GAMEOPTION_NO_SCIENCE")==0,"No-science option takes first saved C-string match");
    option_context.game_options[0].value=2;
    check(option_context.game_option_value(option_db,"GAMEOPTION_NO_SCIENCE")==2,"Raw game option must not convert nonzero values to one");
    for (auto profile : {TechnologyProfile::bnw,TechnologyProfile::vp542}) {
        const bool vp=profile==TechnologyProfile::vp542;
        RulesDatabase db; research_context_schema(db);
        db.apply_sql("UPDATE HandicapInfos SET ResearchPercent=100,ResearchPerEraModifier=0,AIResearchPercent=100,AIResearchPerEraModifier=0;"
                     "UPDATE GameSpeeds SET ResearchPercent=100; UPDATE Eras SET ResearchPercent=100;"
                     "INSERT INTO Defines VALUES('NUM_OR_TECH_PREREQS',1),('TECH_COST_TOTAL_KNOWN_TEAM_MODIFIER',30),"
                     "('TECH_COST_KNOWN_PREREQ_MODIFIER',20),('NUM_CITIES_COST_MOD_SCALING',100);");
        const auto defs=db.cache_global_defines(vp?GlobalDefineProfile::vp542:GlobalDefineProfile::bnw);
        auto rules=load_research_rules(db,profile);
        TechnologyRules techs; techs.profile=profile; techs.collection.slots=2;
        for (int id=0; id<2; ++id) {
            techs.entries[id].integers={{"Cost",101},{"EurekaPerMillion",0}};
            techs.entries[id].or_prerequisites.extent=1; techs.entries[id].or_prerequisites.default_value=-1;
        }
        techs.entries[0].or_prerequisites.values={{0,1}};
        ResearchContext base; base.profile=profile; base.speed=base.start_era=base.game_handicap=0; base.world_percent=100;
        base.players={{0,{10,0,3,0,true}},{1,{10,0,1,0,false}},{2,{11,0,3,0,true}},{3,{10,0,1,0,true}}};
        base.team_members={{10,1},{11,2}};
        PlayerResearchContext state; state.profile=profile; state.world_city_modifier=vp?500:5;
        state.major_players={0,1,2}; // player 3 deliberately outside the source major loop
        state.teams[10].members={0,1,3}; state.teams[11].members={2};
        for (int id=0; id<4; ++id){
            state.players[id]={};
            if(vp)state.players[id].profile_extensions.emplace<VpPlayerResearchProfileState>();
        }
        state.players[0].cities[0]={};
        std::vector<TeamTechnologies> teams; teams.emplace_back(profile,10,2); teams.emplace_back(profile,11,2);
        teams[0].set_has_tech(1,true);
        CustomModOptions options;
        ResearchProgressTestWorld world; world.teams=&teams; world.players=&state; world.bonuses={{0,3},{1,-2},{2,100},{3,999}};
        ResearchProgressContext c{state,base,rules,techs,defs,vp?&options:nullptr,teams,world,10,false};
        const auto reset=[&] {
            teams[0].set_progress_times100(0,0); teams[0].set_has_tech(0,false); teams[0].set_no_trade(0,false);
            teams[1].set_progress_times100(0,0); teams[1].set_has_tech(0,false); teams[1].set_no_trade(0,false);
            state.players[0].overflow_times100=0; state.players[2].overflow_times100=0;
            world.events.clear(); world.culture.clear(); world.throw_on_acquire=false; world.reset_repeat=false; world.science=100;
            c.no_science=false; c.active_team=10;
        };
        const auto threshold=vp?8800:8836;
        auto q=player_research_quote(state,base,rules,techs,defs,c.options,teams,0,0);
        check(q.completion_threshold_times100==threshold && q.cost==(vp?88:89),"Completion must use unrounded BNW threshold");
        auto r=set_research_progress_times100(c,10,0,threshold-1,0);
        check(r.changed && !r.completed && r.threshold==threshold && teams[0].progress_times100(0)==threshold-1 &&
              world.seen_progress==threshold-1 && world.events==(vp?std::vector<std::string>{"dirty:10"}:
              std::vector<std::string>{"dirty:10","science:0"}),"Partial progress write/order differs from source");
        world.events.clear();
        r=set_research_progress_times100(c,10,0,threshold-1,0);
        check(!r.changed && world.events.empty(),"Unchanged progress must not query science/cost or emit effects");
        state.players[0].overflow_times100=7;
        r=change_research_progress_times100(c,10,0,1,0);
        auto expected=std::vector<std::string>{"dirty:10"}; if (!vp) expected.push_back("science:0");
        expected.insert(expected.end(),{"acquire:10:0:0","specialize:0","culture_get:0","culture_add:0:3","culture_get:1","culture_add:1:-2"});
        check(r.completed && r.overflow_granted==0 && world.seen_overflow==7 && teams[0].has_tech(0) && teams[0].at(0).no_trade &&
              world.culture[0]==3 && world.culture[1]==-2 && world.events==expected,
              "Completion effects must use source order and include dead teammates in major domain");
        reset(); world.reset_repeat=true;
        r=set_research_progress_times100(c,10,0,threshold,0);
        check(r.completed && teams[0].progress_times100(0)==0 && teams[0].at(0).count==1 && !teams[0].has_tech(0) && teams[0].at(0).no_trade,
              "Progress layer overwrote repeat reset or synthesized a normal HasTech outside world handler");
        reset(); world.throw_on_acquire=true;
        rejects([&] { set_research_progress_times100(c,10,0,threshold+17,0); });
        check(teams[0].progress_times100(0)==threshold+17 && state.players[0].overflow_times100==17 && !teams[0].at(0).no_trade &&
              world.events.back()=="acquire:10:0:0" && world.culture.empty(),"Callback failure must retain prior writes and stop later effects");
        reset(); c.active_team=11;
        r=set_research_progress_times100(c,10,0,-17,0);
        check(r.changed && r.assigned==(vp?0:-17) && teams[0].progress_times100(0)==(vp?0:-17) &&
              world.events==(vp?std::vector<std::string>{}:std::vector<std::string>{"science:0"}),
              "VP lower clamp, BNW raw negative or inactive-team dirty behavior differs");
        reset(); r=set_research_progress_times100(c,10,0,200000,0);
        check(r.overflow_granted==(vp?191200:10100),"BNW overflow cap or VP uncapped overflow differs");
        reset(); world.science=5000;
        r=set_research_progress_times100(c,10,0,200000,0);
        check(r.overflow_granted==(vp?191200:25000),"BNW cap must use max(raw base cost times100, five turns of science)");
        reset(); c.no_science=true;
        if (vp) {
            r=change_research_progress_times100(c,-999,-999,1,-999);
            check(r.skipped && world.events.empty(),"VP NO_SCIENCE change must bypass all state lookup");
        } else rejects([&] { change_research_progress_times100(c,-999,-999,1,-999); });
        set_player_research_overflow_times100(c,0,77);
        change_player_research_overflow_times100(c,0,13);
        check(state.players[0].overflow_times100==(vp?0:90),"NO_SCIENCE gates only VP player overflow setters");
        r=set_research_progress_times100(c,10,0,threshold+17,0);
        check(r.completed && r.overflow_granted==(vp?0:17) && state.players[0].overflow_times100==(vp?0:107),
              "Direct VP progress setter bypasses NO_SCIENCE, but its overflow setter remains gated");
        reset();
        r=set_research_progress(c,10,0,3,0);
        check(r.assigned==300,"Whole-beaker setter must convert to hundredths");
        r=change_research_progress(c,10,0,2,0);
        check(r.assigned==500,"Whole-beaker increment must convert before addition");
        reset(); teams[0].set_progress_times100(0,199);
        auto granted=change_research_progress_percent(c,10,0,-1,0);
        check(granted==-1 && teams[0].progress_times100(0)==(vp?98:99),"Negative percent uses profile-specific team remaining units");
        reset(); techs.entries[0].integers["Cost"]=75;
        granted=change_research_progress_percent(c,10,0,1,0);
        check(granted==0 && teams[0].progress_times100(0)==(vp?75:0),"VP fractional percentage must change progress even when whole return is zero");
        reset(); c.no_science=true;
        granted=change_research_progress_percent(c,10,0,10,0);
        check(granted==7 && teams[0].progress_times100(0)==(vp?0:700),"VP percent return remains computed even when change is gated");
        reset(); techs.entries[0].integers["Cost"]=101;
        check(change_research_progress_percent(c,-999,-999,0,-999)==0,"Zero percent must skip lookup");
        teams[0].set_has_tech(0,true);
        check(change_research_progress_percent(c,10,0,50,-999)==0 && world.events.empty(),"Known technology percent must not evaluate player or callbacks");
        reset();
        check(max_research_overflow(c,99,0)==0 && world.events==std::vector<std::string>{"science:0"},
              "Missing Info overflow limit must query source science then return zero");
        reset();
        r=set_research_progress_times100(c,10,0,1,2);
        check(r.threshold==(vp?15100:10605),"Credited foreign player: BNW uses target team cost, VP uses player's team cost");
        reset(); state.players[0].overflow_times100=INT64_MAX;
        r=set_research_progress_times100(c,10,0,1,0);
        check(r.assigned==1 && !r.completed,"Cost threshold must not query combined player progress or overflow");
        reset();
        if (vp) {
            r=set_research_progress_times100(c,10,0,threshold+17,0,11,150);
            check(r.overflow_granted==13,"Overflow above player's contribution scales only that contribution");
            reset(); r=set_research_progress_times100(c,10,0,threshold+11,0,11,150);
            check(r.overflow_granted==7,"Overflow at/below contribution uses the alternate rounded branch");
            reset(); r=set_research_progress_times100(c,10,0,threshold-1,0,11,0);
            check(!r.completed,"Unused overflow divisor must not reject partial research");
            rejects([&] { set_research_progress_times100(c,10,0,threshold,0,11,0); });
            check(teams[0].progress_times100(0)==threshold && state.players[0].overflow_times100==0 && !teams[0].has_tech(0),
                  "Invalid used divisor must retain earlier progress write, without later completion");
            reset();
            check(set_research_progress_times100(c,-999,-1,1,0).skipped &&
                  set_research_progress_times100(c,-999,0,1,-1).skipped && world.events.empty(),"VP NO_PLAYER/NO_TECH failsafe differs");
        }
        reset();
        techs.entries[0].integers["Cost"]=5000000000LL;
        techs.entries[0].or_prerequisites.values.clear(); state.world_city_modifier=0;
        state.players[0].overflow_times100=4294967400LL;
        r=set_research_progress_times100(c,10,0,500000000000LL+4294967400LL,0);
        check(r.completed && r.threshold==500000000000LL && r.overflow_granted==4294967400LL &&
              state.players[0].overflow_times100==8589934800LL && teams[0].progress_times100(0)>INT32_MAX,
              "Progress, threshold or overflow retained an x86 INT_MAX clamp");
        reset(); teams[0].set_progress_times100(0,INT64_MAX);
        rejects([&] { change_research_progress_times100(c,10,0,1,0); });
        check(teams[0].progress_times100(0)==INT64_MAX && world.events.empty(),"Overflowing increment must not silently wrap or issue callbacks");
        reset();
        rejects([&] { set_research_progress_times100(c,10,-2,1,0); });
        rejects([&] { set_research_progress_times100(c,10,2,1,0); });
        auto broken_rules=rules; broken_rules.speeds.report.error="not loaded";
        ResearchProgressContext invalid{state,base,broken_rules,techs,defs,c.options,teams,world,10,false};
        rejects([&] { set_research_progress_times100(invalid,10,0,1,0); });
        check(teams[0].progress_times100(0)==0 && world.events.empty(),"Invalid rules must be rejected before progress mutation");
    }
}
