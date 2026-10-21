#pragma once
void player_research_tests() {
    using namespace civ5::runtime;
    using namespace civ5::saves;
    using wide::multiply_add_divide;
    check(multiply_add_divide(INT64_MIN,-1,-1,1)==INT64_MAX &&
          multiply_add_divide(INT64_MAX,100,50,100)==INT64_MAX &&
          multiply_add_divide(INT64_MIN,100,50,100)==INT64_MIN+1 &&
          multiply_add_divide(-2,3,7,2)==0 && multiply_add_divide(2,3,-9,2)==-1 &&
          multiply_add_divide(0,-3,INT64_MIN,1)==INT64_MIN,
          "Wide product/addition lost cancellation, sign or rounding");
    rejects([&] { multiply_add_divide(INT64_MAX,2,2,2); });
    rejects([&] { multiply_add_divide(1,1,0,0); });
    rejects([&] { multiply_add_divide(1,1,0,-1); });
#if defined(__SIZEOF_INT128__)
    __extension__ using OracleInteger = __int128;
    std::uint64_t seed = 0x9b05688c2b3e6c1fULL;
    const auto next = [&]() { seed ^= seed<<13; seed ^= seed>>7; seed ^= seed<<17; return seed; };
    const auto oracle = [&](std::int64_t a, std::int64_t b, std::int64_t c, std::int64_t d) {
        const OracleInteger expected=(static_cast<OracleInteger>(a)*b+c)/d;
        if (expected<INT64_MIN || expected>INT64_MAX) rejects([&] { multiply_add_divide(a,b,c,d); });
        else check(multiply_add_divide(a,b,c,d)==expected,"Product/add/divide differs from independent 128-bit oracle");
    };
    for (unsigned i=0; i<7000; ++i) {
        const auto a=std::bit_cast<std::int64_t>(next()), b=std::bit_cast<std::int64_t>(next());
        const auto c=std::bit_cast<std::int64_t>(next()), d=static_cast<std::int64_t>((next()&INT64_MAX)|1);
        oracle(a,b,c,d);
        if (i<200) oracle(a,i%3-1LL,c,d);
    }
    for (auto a : {INT64_MIN,INT64_MAX,std::int64_t{0},std::int64_t{1},std::int64_t{-1}})
        for (auto b : {INT64_MIN,INT64_MAX,std::int64_t{0},std::int64_t{1},std::int64_t{-1}})
            for (auto c : {INT64_MIN,INT64_MAX,std::int64_t{0},std::int64_t{1},std::int64_t{-1}})
                for (auto d : {INT64_MAX,std::int64_t{1},std::int64_t{2}}) oracle(a,b,c,d);
#endif
    PlayerResearchState city_player;
    city_player.profile_extensions.emplace<VpPlayerResearchProfileState>();
    check(bnw_max_effective_cities(city_player,true)==1 && vp_effective_cities(city_player,true)==1 &&
          vp_research_city_modifier_times100(city_player,500,100,true)==0,
          "Empty player has zero VP city cost but effective-city getter returns one");
    for (int id=0; id<5; ++id) city_player.cities[id]={};
    city_player.cities[0].puppet=true;
    city_player.cities[1].razing=true;
    city_player.cities[2].ignore_happiness=true;
    check(bnw_max_effective_cities(city_player,true)==3 && city_player.max_effective_cities==2 &&
          vp_effective_cities(city_player,false)==3 && vp_effective_cities(city_player,true)==4,
          "BNW history, VP razing or puppet/ignore counts differ");
    city_player.cities[0].razing=true;
    check(bnw_max_effective_cities(city_player,false)==2,"BNW historical maximum must survive city loss");
    city_player.max_effective_cities=-9;
    city_player.cities.erase(3); city_player.cities.erase(4);
    check(bnw_max_effective_cities(city_player,false)==-1,"BNW puppet and razing subtract independently; only zero becomes one");
    city_player={}; city_player.cities[0]={}; city_player.cities[1]={}; city_player.cities[2]={};
    city_player.profile_extensions.emplace<VpPlayerResearchProfileState>().city_cost_modifier=-1;
    check(vp_research_city_modifier_times100(city_player,500,100,true)==1440 &&
          vp_research_city_modifier_times100(city_player,500,100,true,-99)==0 &&
          vp_research_city_modifier_times100(city_player,500,100,true,1)==2080,
          "VP city scaling/offset or half-divisor rounding differs");
    check(vp_research_city_modifier_times100(city_player,0,13,true)==-261 &&
          vp_research_city_modifier_times100(city_player,500,-1,true)==1200,
          "Nonpositive map and signed scaling rounding differ");
    city_player={};
    city_player.profile_extensions.emplace<VpPlayerResearchProfileState>();
    check(vp_research_city_modifier_times100(city_player,1,0,true,4294967298LL)==4294967298LL,
          "Zero scaling must not impose an intermediate pair-count or int32 limit");

    for (auto profile : {TechnologyProfile::bnw,TechnologyProfile::vp542}) {
        const bool vp=profile==TechnologyProfile::vp542;
        RulesDatabase db; research_context_schema(db);
        db.apply_sql("ALTER TABLE HandicapInfos ADD COLUMN TechCatchUpMod; ALTER TABLE HandicapInfos ADD COLUMN AITechCatchUpMod;"
                     "UPDATE HandicapInfos SET ResearchPercent=100,ResearchPerEraModifier=0,AIResearchPercent=100,AIResearchPerEraModifier=0,TechCatchUpMod=3,AITechCatchUpMod=7;"
                     "UPDATE HandicapInfos SET TechCatchUpMod=11 WHERE ID=8; UPDATE GameSpeeds SET ResearchPercent=100; UPDATE Eras SET ResearchPercent=100;"
                     "INSERT INTO Defines VALUES('NUM_OR_TECH_PREREQS',4),('TECH_COST_TOTAL_KNOWN_TEAM_MODIFIER',30),"
                     "('TECH_COST_KNOWN_PREREQ_MODIFIER',20),('NUM_CITIES_COST_MOD_SCALING',100),('SCHOLAR_MINOR_ALLY_MULTIPLIER',2);");
        const auto defs=db.cache_global_defines(vp ? GlobalDefineProfile::vp542 : GlobalDefineProfile::bnw);
        auto rules=load_research_rules(db,profile);
        check(rules.handicaps.find(0)->tech_catch_up==(vp?3:0) && rules.handicaps.find(0)->ai_tech_catch_up==(vp?7:0),
              "Handicap catch-up fields must load only in VP");
        TechnologyRules techs; techs.profile=profile; techs.collection.slots=4;
        for (int id=0; id<4; ++id) {
            techs.entries[id].integers={{"Cost",101},{"EurekaPerMillion",0}};
            techs.entries[id].or_prerequisites.extent=4;
            techs.entries[id].or_prerequisites.default_value=-1;
        }
        techs.entries[0].or_prerequisites.values={{0,1},{1,1},{2,2},{3,-1}};
        ResearchContext base; base.profile=profile; base.speed=base.start_era=base.game_handicap=0;
        base.world_percent=100; base.current_era=2;
        PlayerResearchContext state; state.profile=profile; state.barbarian_team=63;
        std::vector<TeamTechnologies> teams;
        for (int id=0; id<5; ++id) {
            base.players[id]={id+10,0,3,0,true}; base.team_members[id+10]=1;
            state.players[id]={}; state.teams[id+10].members={id}; state.teams[id+10].alive_count=1;
            if(vp)state.players[id].profile_extensions.emplace<VpPlayerResearchProfileState>();
            state.civ_teams.insert(id+10); state.major_players.insert(id); teams.emplace_back(profile,id+10,4);
        }
        state.bnw_minor_scan_players={3,4}; state.players[3].pregame_minor=true;
        state.teams[14].alive_count=0;
        state.teams[10].met={10,11,13}; teams[1].set_has_tech(0,true); teams[0].set_has_tech(1,true);
        CustomModOptions options;
        auto *opts=vp ? &options : nullptr;
        const auto modifier=[&](std::int64_t tech=0) { return player_research_modifier(state,base,rules,techs,defs,opts,teams,0,tech); };
        const auto league_science=[&](){return vp?vp_player_research_profile(state.players[0]).science_from_league:0;};
        auto m=modifier();
        check(m.possible_teams==(vp?1:3) && m.known_teams==1 && m.known_or_prerequisites==2 &&
              m.catch_up==(vp?6:0) && m.value==(vp?176:150),
              "Known-team denominator/self, duplicate OR prerequisites, minor/dead or catch-up branches differ");
        check(player_research_modifier(state,base,rules,techs,defs,opts,teams,-999,-1).value==100,
              "NO_TECH returns 100 before looking up a player");
        rejects([&] { modifier(4); });
        auto broken=techs; broken.entries[0].or_prerequisites.extent=3;
        rejects([&] { player_research_modifier(state,base,rules,broken,defs,opts,teams,0,0); });
        auto wrong=base; wrong.profile=vp?TechnologyProfile::bnw:TechnologyProfile::vp542;
        rejects([&] { player_research_modifier(state,wrong,rules,techs,defs,opts,teams,0,0); });
        state.leagues={{{0,1,1},{10,15}},{{1,2},{90}},{{0,2},{70}}};
        if(vp){auto &x=vp_player_research_profile(state.players[0]);x.cs_allies=40;x.science_from_league=17;}
        options.entries["BALANCE_VP"].value=1;
        m=modifier();
        check(m.league==(vp?75:25) && m.value==(vp?308:187) && league_science()==(vp?50:0),
            "Saved league effects/member teams or capped VP ally cache differs");
        state.leagues.clear(); modifier();
        check(league_science()==(vp?50:0),"Zero league bonus must preserve cached science");
        state.leagues={{{0,1},{25}},{{0,1},{-25}}}; modifier();
        check(league_science()==(vp?50:0),"Cancelling leagues must not enter ally cache branch");
        state.leagues={{{0,1},{25}}}; if(vp)vp_player_research_profile(state.players[0]).cs_allies=0; modifier();
        check(league_science()==0,"No allies must clear VP cache only inside league branch");
        state.leagues.clear();
        if (vp) {
            base.players[0].slot_status=1;
            check(modifier().catch_up==20,"VP AI catch-up must include game handicap and current game era");
            base.players[0].slot_status=3; options.entries["HUMAN_USES_AI_HANDICAP"].value=1;
            check(modifier().catch_up==36,"Human override must use AI_HANDICAP and game AI catch-up");
            options.entries["HUMAN_USES_AI_HANDICAP"].value=0;
            base.players[0].slot_status=4;
            check(modifier().catch_up==0,"Observer team cannot receive major-civ catch-up");
            base.players[0].slot_status=3;
            base.players[0].handicap=999;
            check(modifier().missing_infos==std::vector<std::string>{"player_handicap"},"Missing VP handicap must diagnose fallback");
            base.players[0].handicap=0;
            options.entries["DIPLOMACY_TECH_BONUSES"].value=1;
            check(modifier().possible_teams==0,"Met is insufficient when VP diplomacy bonus requires embassy");
            state.teams[11].liberated_by=10;
            check(modifier().known_teams==1,"Liberation implies embassy when not at war");
            state.teams[10].at_war.insert(11);
            check(modifier().possible_teams==0,"War must cancel liberation embassy");
            state.teams[11].master=10;
            check(modifier().possible_teams==1,"Vassal embassy branch remains independent of war");
            vp_player_research_profile(state.players[0]).spies=1;
            check(modifier().possible_teams==0,"Any spy replaces embassy requirement with capital spy requirement");
            state.players[1].capital=4294967300LL;
            ensure_vp_research_city_profile(state.players[1].cities[4294967300LL]).assigned_spy_players.insert(0);
            base.players[0].alive=false; base.players[1].alive=false;
            check(modifier().known_teams==1,"Spy lookup uses city assignments and does not test player alive flags");
            state.major_players.erase(0);
            check(modifier().possible_teams==0,"Spy lookup must respect explicit major-player source domain");
            state.major_players.insert(0); base.players[0].alive=base.players[1].alive=true;
            vp_player_research_profile(state.players[0]).spies=0; options.entries["DIPLOMACY_TECH_BONUSES"].value=0;
        } else {
            base.players[3].team=base.players[4].team=13; state.players[3].pregame_minor=false; state.players[4].pregame_minor=true;
            check(!state.team_is_minor(base,13),"BNW must return first matching alive player's minor flag, not any flag");
            base.players[3].alive=false;
            check(state.team_is_minor(base,13),"BNW minor scan must skip dead players");
        }
        state.civ_teams.clear(); state.leagues.clear(); techs.entries[0].or_prerequisites.values={{0,1}};
        state.players[0].cities[0]={}; state.world_city_modifier=vp?500:5;
        state.players[0].overflow_times100=99;
        auto q=player_research_quote(state,base,rules,techs,defs,opts,teams,0,0);
        check(q.modifier.value==120 && q.team_cost==101 && q.cost==(vp?88:89) && q.progress==(vp?99:0),
              "Player cost must use profile-specific intermediate rounding and overflow units");
        db.apply_sql("CREATE TABLE ResearchTechIDs(ID,Type); INSERT INTO ResearchTechIDs VALUES(0,'RESEARCH_A');");
        auto saved_progress=saved_team_techs(profile,{saved_info_type_hash("RESEARCH_A")});
        saved_progress.fields[3].value=10; saved_progress.arrays[3].values={199};
        teams[0]=restore_team_technologies(saved_progress,techs,db.cache_info_types()); teams[0].set_has_tech(1,true);
        q=player_research_quote(state,base,rules,techs,defs,opts,teams,0,0);
        check(q.progress==(vp?298:1),"BNW truncates team progress and overflow separately; VP adds raw hundredths");
        state.leagues={{{0,1},{-200}}};
        q=player_research_quote(state,base,rules,techs,defs,opts,teams,0,0);
        check(q.modifier.value==-120 && q.cost==10605,"Player cost must clamp negative research modifier to one");
        state.leagues.clear();
        techs.entries[0].integers["Cost"]=1;
        q=player_research_quote(state,base,rules,techs,defs,opts,teams,0,0);
        check(q.cost==(vp?0:1),"VP player cost has no final minimum, BNW rounds fractional costs upward");
        if (vp) {
            techs.entries[0].integers["Cost"]=101;
            q=player_research_quote(state,base,rules,techs,defs,opts,teams,0,0,true,999);
            check(q.cost==84 && q.city_modifier_times100==0,"Ignore-city option must bypass city cost and offset");
        } else rejects([&] { player_research_quote(state,base,rules,techs,defs,opts,teams,0,0,true); });
        // Sparse wide IDs remain valid in owned worlds, independent of source wire domains.
        constexpr std::int64_t wide_id=4294967301LL;
        base.players.clear(); base.players[wide_id]={wide_id,0,3,0,true}; base.team_members.clear(); base.team_members[wide_id]=1;
        state.players.clear(); state.players[wide_id].queue={wide_id,wide_id}; state.players[wide_id].overflow_times100=wide_id;
        if(vp)state.players[wide_id].profile_extensions.emplace<VpPlayerResearchProfileState>();
        state.teams.clear(); state.teams[wide_id].members={wide_id}; state.teams[wide_id].alive_count=1;
        state.major_players={wide_id}; state.bnw_minor_scan_players.clear();
        techs.entries.clear(); techs.collection.slots=wide_id+1; techs.entries[wide_id].integers={{"Cost",wide_id},{"EurekaPerMillion",0}};
        techs.entries[wide_id].or_prerequisites.extent=4; techs.entries[wide_id].or_prerequisites.default_value=-1;
        teams.clear(); teams.emplace_back(profile,wide_id,wide_id+1);
        q=player_research_quote(state,base,rules,techs,defs,opts,teams,wide_id,wide_id);
        check(q.team_cost==wide_id && state.players.at(wide_id).current_research()==wide_id &&
              q.progress==(vp?wide_id:wide_id/100),"Player research narrowed an owned ID/cost/queue/progress");

        auto snapshot=research_snapshot();
        snapshot.teams->dimensions.major=1;
        const auto layout=vp?GameStateLayout::vp542_corpus:GameStateLayout::expansion2_version1;
        snapshot.game_state.layout=snapshot.pregame->layout=snapshot.teams->layout=snapshot.all_players->layout=layout;
        snapshot.pregame->world.values.integers["num_cities_tech_cost_mod"]=vp?650:7;
        snapshot.pregame->archive.flag_arrays["minor_nation_civs"]={false,false,true};
        auto &saved_team=snapshot.teams->teams[0];
        saved_team.fields.push_back({"alive_count",2}); saved_team.fields.push_back({"liberated_by",42}); saved_team.fields.push_back({"master",-1});
        for (const auto *name : {"has_met","embassy",vp?"at_war_final":"at_war"}) {
            PlotArrayObservation flags{}; flags.name=name; flags.values.resize(43); flags.values[42]=1; saved_team.arrays.push_back(flags);
        }
        const auto field=[](auto &fields, const char *name, std::int64_t value) {
            typename std::remove_reference_t<decltype(fields)>::value_type f{}; f.name=name; f.values={value}; fields.push_back(f);
        };
        for (auto &saved : snapshot.all_players->players) {
            saved.core.layout=layout; saved.core.version=6;
            field(saved.core.fields,"m_iOverflowResearch",199); field(saved.core.fields,"m_iCapitalCityID",77);
            field(saved.core.fields,"m_iMaxEffectiveCities",9); field(saved.core.fields,"m_iTechCostXCitiesModifier",-2);
            field(saved.core.fields,"m_iCSAllies",3); field(saved.core.fields,"m_iScienceRateFromLeague",11);
            PlayerAdministrationRecord admin{}; admin.name="pre_city";
            PlotArrayObservation queue{}; queue.name="m_researchQueue"; queue.values={3,1,3,-1}; admin.arrays.push_back(queue);
            saved.administration.blocks.push_back(admin);
            PlayerEspionageRecord esp{}; esp.name="player_espionage";
            PlayerEspionageRecord spy{}; spy.name="spy"; esp.children.push_back(spy); saved.espionage.blocks.push_back(esp);
            SavedCityObservation city{};
            field(city.core.fields,"m_iID",77); field(city.core.fields,"m_bPuppet",1); field(city.core.fields,"m_iRazingTurns",2);
            field(city.core.fields,"m_bIgnoreCityForHappiness",1);
            CityTailNode city_esp{}; city_esp.name="espionage";
            CityTailNode assignments{}; assignments.name="m_aiSpyAssignment"; assignments.values={-1,0,7};
            city_esp.children.push_back(assignments); city.tail.blocks.push_back(city_esp); saved.cities.cities.push_back(city);
        }
        base=restore_research_context(snapshot,db);
        auto restored=restore_player_research_context(snapshot,base,defs);
        const auto &p=restored.players.at(2);
        const auto *vp_saved=p.profile_extensions.find<VpPlayerResearchProfileState>();
        const auto *vp_city=find_vp_research_city_profile(p.cities.at(77));
        check(p.queue==std::vector<std::int64_t>({3,1,3,-1}) && p.current_research()==3 && p.overflow_times100==199 &&
              p.pregame_minor && p.capital==77 && p.max_effective_cities==(vp?1:9) &&
              ((vp&&vp_saved&&vp_saved->city_cost_modifier==-2&&vp_saved->cs_allies==3&&
                vp_saved->science_from_league==11)||(!vp&&!vp_saved)) &&
              (!vp || (vp_saved&&vp_saved->spies==1)) && p.cities.at(77).ignore_happiness==vp && p.cities.at(77).razing &&
              ((vp&&vp_city&&vp_city->assigned_spy_players==std::set<std::int64_t>{1,2})||(!vp&&!vp_city)) &&
              restored.world_city_modifier==(vp?650:7) && restored.civ_teams==std::set<std::int64_t>{42} &&
              restored.major_players==std::set<std::int64_t>{0} && restored.bnw_minor_scan_players==std::set<std::int64_t>{1,2},
              "Player restoration changed raw queue order/IDs, source fields, domains or profile defaults");
        check(snapshot.all_players->players[0].core.fields.back().values[0]==11,"Restoration mutated source observation");
        if (!vp) {
            snapshot.all_players->players[0].core.version=5;
            check(restore_player_research_context(snapshot,base,defs).players.at(2).max_effective_cities==1,
                  "Earlier BNW version must retain constructor maximum of one");
        }
        auto bad=snapshot; bad.all_players->players.push_back(bad.all_players->players[0]);
        rejects([&] { restore_player_research_context(bad,base,defs); });
        bad=snapshot; bad.all_players->players[0].cities.cities.push_back(bad.all_players->players[0].cities.cities[0]);
        rejects([&] { restore_player_research_context(bad,base,defs); });
        bad=snapshot; bad.all_players->players[0].administration.blocks.clear();
        rejects([&] { restore_player_research_context(bad,base,defs); });
        bad=snapshot; bad.pregame->archive.flag_arrays["minor_nation_civs"].resize(1);
        rejects([&] { restore_player_research_context(bad,base,defs); });
        bad=snapshot; bad.teams->dimensions.civ_teams=UINT64_MAX;
        rejects([&] { restore_player_research_context(bad,base,defs); });
        bad=snapshot; bad.teams->layout=vp?GameStateLayout::expansion2_version1:GameStateLayout::vp542_corpus;
        rejects([&] { restore_player_research_context(bad,base,defs); });
        auto wrong_roster=base; wrong_roster.players[999]={};
        rejects([&] { restore_player_research_context(snapshot,wrong_roster,defs); });
    }
}
