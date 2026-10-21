#pragma once
void research_turn_tests() {
    using namespace civ5::runtime;
    struct Host final : ResearchProgressServices,ResearchTurnServices {
        std::vector<std::string> events;std::function<void()> ai,acquire,ui;
        std::int64_t science_times100(std::int64_t) override {throw std::logic_error("Injected science used instead of shared economy");}
        void game_data_and_score_dirty(std::int64_t) override {events.push_back("dirty");}
        void acquire_technology(std::int64_t,std::int64_t,std::int64_t) override {events.push_back("acquire");if(acquire)acquire();else throw ResearchTurnUnavailable("world required");}
        void city_specializations_dirty(std::int64_t) override {events.push_back("specializations");}
        std::int64_t culture_per_tech(std::int64_t) override {events.push_back("culture");return 0;}
        void add_culture(std::int64_t,std::int64_t) override {throw std::logic_error("No culture expected");}
        void choose_research_ai(std::int64_t) override {events.push_back("ai");if(ai)ai();else throw ResearchTurnUnavailable("AI required");}
        void choose_research_ui(std::int64_t) override {events.push_back("ui");if(ui)ui();}
        void research_buttons_dirty(std::int64_t) override {events.push_back("buttons");}
        void unlock_research_achievement(std::int64_t,ResearchAchievement a) override {events.push_back(a==ResearchAchievement::all_technologies?"all":"pony");}
    } host;
    struct GoldHost final : TreasuryTurnServices {
        void treasury_dirty(std::int64_t,TreasuryDirty) override {}
        void deficit(std::int64_t,TechnologyProfile) override {
            throw TreasuryUnavailable("Deficit requires the live world");
        }
        void unlock_trade_gold_achievement(std::int64_t) override {}
    } gold_host;
    for(bool vp:{false,true}) {
        const auto profile=vp?TechnologyProfile::vp542:TechnologyProfile::bnw;
        RulesDatabase db;research_context_schema(db);
        db.apply_sql("ALTER TABLE HandicapInfos ADD COLUMN TechNumOptionsConsidered INTEGER DEFAULT 2;"
            "ALTER TABLE HandicapInfos ADD COLUMN TechChoiceCutoffThreshold INTEGER DEFAULT 50;");
        db.apply_sql("UPDATE HandicapInfos SET ResearchPercent=100,ResearchPerEraModifier=0,AIResearchPercent=100,AIResearchPerEraModifier=0;"
            "UPDATE GameSpeeds SET ResearchPercent=100;UPDATE Eras SET ResearchPercent=100;"
            "INSERT INTO Defines VALUES('NUM_OR_TECH_PREREQS',1),('TECH_COST_TOTAL_KNOWN_TEAM_MODIFIER',0),"
            "('TECH_COST_KNOWN_PREREQ_MODIFIER',0),('NUM_CITIES_COST_MOD_SCALING',100),('RESEARCH_AGREEMENT_MOD',0),"
            "('AI_RESEARCH_WEIGHT_BASE_MOD',0.001),('AI_RESEARCH_WEIGHT_MOD_PER_TURN_LEFT',0.00001),"
            "('UNIT_MAINTENANCE_GAME_MULTIPLIER',8),('UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR',7),('INITIAL_FREE_OUTSIDE_UNITS',2),('INITIAL_OUTSIDE_UNIT_GOLD_PERCENT',50);");
        const auto defs=db.cache_global_defines(vp?GlobalDefineProfile::vp542:GlobalDefineProfile::bnw);
        const auto rules=load_research_rules(db,profile);
        TechnologyRules techs;techs.profile=profile;techs.collection.slots=2;
        for(int i=0;i<2;++i){auto &t=techs.entries[i];t.integers={{"Cost",10000},{"EurekaPerMillion",0}};t.booleans["Repeat"]=false;t.or_prerequisites.extent=1;t.or_prerequisites.default_value=-1;}
        ResearchContext setup;PlayerResearchContext players;ScienceContext science;TreasuryContext treasury;MaintenanceContext maintenance;
        CityYieldContext cities;TreasuryRules tr;CityYieldRules cr;MaintenanceRules mr;ResearchTurnState state;
        setup.profile=players.profile=science.profile=treasury.profile=maintenance.profile=cities.profile=tr.profile=cr.profile=mr.profile=state.profile=profile;
        if(vp){ensure_vp_maintenance_rules_profile(mr);ensure_vp_maintenance_context_profile(maintenance);}
        setup.speed=setup.start_era=setup.game_handicap=0;setup.world_percent=100;setup.game_turn=10;
        setup.map_random_seed=123456;
        for(std::int64_t id=0;id<4;++id){setup.players[id]={id==2?1:id==3?0:id,0,id==0?3:1,0,id!=2};setup.team_members[setup.players[id].team]=1;
            players.players[id]={};
            if(vp)players.players[id].profile_extensions.emplace<VpPlayerResearchProfileState>();
            players.players[id].cities[0]={};science.players[id]={};treasury.players[id].balance_times100=100000;
            if(vp)ensure_vp_science_profile(science.players[id]);
            if(vp){ensure_vp_treasury_income_profile(treasury.players[id]);ensure_vp_treasury_minor_profile(treasury.players[id]);ensure_vp_treasury_religion_profile(treasury.players[id]);}
            cities.players[id].excess_happiness=0;maintenance.players[id]={};
            if(vp)ensure_vp_maintenance_profile(maintenance.players[id],profile);
            state.players[id].leader_type="LEADER_TEST";}
        players.major_players={0,1,2,3};players.civ_teams={0,1};players.teams[0].members={0,3};players.teams[1].members={1,2};
        players.teams[0].alive_count=players.teams[1].alive_count=1;
        science.research_agreements={{0,{1}},{1,{}}};cities.yield_slots=17;cities.no_happiness=true;
        auto &city=cities.players[0].cities[0];city.id=0;city.owner=0;if(vp)vp_city_cached_yields(cities,0,0)[3]=1000;city.yields[3].terrain=10;
        maintenance.default_end_turn=maintenance.estimate_end_turn=500;maintenance.elapsed_turns=4;maintenance.team_eras={{0,0},{1,0}};
        mr.handicaps[0]={0,0,100,100,100,100,100,100,100,100,0};mr.era_building_percent[0]=100;
        std::vector<TeamTechnologies> teams;teams.emplace_back(profile,0,2);teams.emplace_back(profile,1,2);
        CustomModOptions options;options.prefetch.all_succeeded=true;
        MaintenanceWorld economy{setup,players,cities,teams};
        ResearchProgressContext progress{players,setup,rules,techs,defs,vp?&options:nullptr,teams,host,0,false};
        ResearchTurnContext c{state,progress,science,economy,treasury,maintenance,tr,cr,mr,host,0,{}};
        auto &p=players.players[0];auto &s=state.players[0];
        const auto reset=[&]{host.events.clear();host.ai={};host.acquire={};host.ui={};s.cities_founded=1;s.founded_first_city=true;s.researching={0,1};
            p.queue={0};p.overflow_times100=155;s.agreement_counters={{0,123},{1,7},{2,11},{3,17},{5000000000LL,19}};
            teams[0].set_progress_times100(0,0);teams[0].set_has_tech(0,false);
            if(vp)vp_science_profile(science.players[0]).last_turn_times100=555;
            if(vp)vp_city_cached_yields(cities,0,0)[3]=1000;city.yields[3].terrain=10;maintenance.elapsed_turns=4;};
        reset();
        {
            LuaScriptHost scripts;scripts.complete_loading();
            LuaResearchSelectionServices selection_host(scripts,[&](auto team){host.research_buttons_dirty(team);});
            ResearchSelectionState selection_state;selection_state.profile=profile;
            selection_state.players[0].civilization="CIV_TEST";
            selection_state.disabled_technologies["CIV_TEST"]={};
            ResearchSelectionContext selection{selection_state,state,players,setup,techs,
                vp?&options:nullptr,teams,maintenance.team_eras,{0,0},selection_host,0,false};
            ResearchAiWeights ai_weights;ai_weights.profile=profile;
            auto &saved=ai_weights.players[0];saved.weight_slots=2;saved.weights={{0,500},{1,200}};
            if(vp)saved.profile_extensions.emplace<VpResearchAiVector>();
            techs.entries[0].booleans["Disable"]=techs.entries[1].booleans["Disable"]=false;
            techs.entries[1].era=1;
            const auto scored=build_research_ai_candidates(selection,c,ai_weights,0,false);
            check(scored.choices.size()==2&&scored.choices[0].technology==0&&
                  scored.choices[1].technology==1&&scored.choices[0].turns>0&&
                  scored.choices[0].adjusted_weight>0&&
                  scored.choices[0].adjusted_weight<scored.choices[0].saved_weight,
                  "Research AI must enumerate loaded SQL technologies and score real turn estimates");
            const auto free=build_research_ai_candidates(selection,c,ai_weights,0,true);
            check(free.choices.size()==2&&(vp?free.prefers_expensive:
                  (free.choices[0].adjusted_weight==500&&free.choices[1].adjusted_weight==200)),
                  "Only VP reweights a free technology and forces the expensive preference");
            SimulationRandom gameplay_random(vp?RandomAlgorithm::vp_reference:RandomAlgorithm::bnw,{17,0,0});
            ResearchAiChoiceContext choice_context{selection,c,ai_weights,db,scripts,gameplay_random};
            p.queue={0};s.researching={0};players.players[3].queue={1};
            auto chosen=choose_research_ai(choice_context,0);
            check(chosen.teammate&&chosen.technology==1&&p.queue==std::vector<std::int64_t>{1}&&
                  gameplay_random.state().calls==0,
                  "Research AI must adopt a living teammate's project without consuming gameplay random");
            players.players[3].queue.clear();
            chosen=choose_research_ai(choice_context,0);
            check(chosen.weighted&&(chosen.technology==0||chosen.technology==1)&&
                  p.current_research()==chosen.technology&&gameplay_random.state().calls==(vp?0U:1U),
                  "Research AI must select a scored technology and update the shared queue");
            if(vp){
                options.entries["EVENTS_AI_OVERRIDE_TECH"].value=1;
                scripts.execute("choice-mod","GameEvents.AiOverrideChooseNextTech.Add(function(player, free) return 1 end)",
                    "=research-choice-mod.lua");
                scripts.complete_loading();
                const auto before=gameplay_random.state();
                chosen=choose_research_ai(choice_context,0);
                check(chosen.script_override&&chosen.technology==1&&p.current_research()==1&&
                      gameplay_random.state()==before,
                      "VP Lua override must select the live technology before weighted fallback");
                scripts.execute("choice-mod","GameEvents.AiOverrideChooseNextTech.Add(function(player, free) return 98 end)",
                    "=research-choice-invalid.lua");
                scripts.complete_loading();
                chosen=choose_research_ai(choice_context,0);
                check(!chosen.script_override&&chosen.weighted&&p.current_research()==chosen.technology,
                      "Out-of-range VP Lua result must fall back to the active TechAI profile");
                options.entries["EVENTS_AI_OVERRIDE_TECH"].value=0;
                ResearchAiCandidates tied;tied.choices={{0,0,0,0},{1,0,0,0}};
                check(vp_choose_research_ai_candidate(tied,50,123456,10,0,0)==1,
                      "VP zero-weight fallback must match the pinned CvSeeder sample");
                ResearchAiCandidates cutoff;cutoff.choices={{0,0,100,0},{1,0,20,0}};
                check(vp_choose_research_ai_candidate(cutoff,50,123456,10,0,0)==0,
                      "VP threshold must exclude a technology below half the best weight");
            }
            p.queue={0};s.researching={0};
            if(vp){
                auto &trait=saved.profile_extensions.find<VpResearchAiVector>()->permanent_yields_decrease_every_era;
                trait=true;
                const auto penalized=build_research_ai_candidates(selection,c,ai_weights,0,false);
                check(penalized.choices[1].adjusted_weight==
                      std::max<std::int64_t>(1,scored.choices[1].adjusted_weight/2)&&
                      penalized.choices[0].adjusted_weight==scored.choices[0].adjusted_weight,
                      "VP future-era trait must halve only future technology scores");
                trait=false;
            }
            scripts.execute("test-mod","GameEvents.PlayerCanResearch.Add(function(player,tech) return tech ~= 1 end)","=research-ai-mod.lua");
            scripts.complete_loading();
            const auto vetoed=build_research_ai_candidates(selection,c,ai_weights,0,false);
            check(vetoed.choices.size()==1&&vetoed.choices[0].technology==0,
                  "Lua mod veto must remove a technology before AI scoring");
            saved.weight_slots=0;
            rejects([&]{build_research_ai_candidates(selection,c,ai_weights,0,false);});
        }
        reset();
        auto estimate=research_turns_left(c,0,0,true);
        check(estimate.research_rate_times100==1000&&estimate.overflow_adjustment==(vp?155:1)&&
              estimate.player_cost>0&&estimate.team_progress==0&&estimate.turns.has_value()&&
              estimate.turns_times100==std::max<std::int64_t>(1,
                  (estimate.remaining*(vp?100:10000)+999)/1000),
              "Research-turn estimate must use real science and the profile's overflow units");
        auto without_overflow=research_turns_left(c,0,0,false);
        check(without_overflow.remaining==estimate.remaining+estimate.overflow_adjustment&&
              without_overflow.turns>=estimate.turns,"Overflow must reduce only the requested estimate");
        p.queue.clear();
        check(research_turns_left(c,0,0,true).research_rate_times100==1000,
              "Owning player contributes science even when not researching the candidate");
        p.queue={0};
        auto &teammate=cities.players[3].cities[0];teammate.id=0;teammate.owner=3;
        teammate.yields[3].terrain=10;
        if(vp)vp_city_cached_yields(cities,3,0)[3]=1000;
        players.players[3].queue={0};
        check(research_turns_left(c,0,0,true).research_rate_times100==2000,
              "Living teammate contributes only when researching the same technology");
        players.players[3].overflow_times100=255;
        check(research_turns_left(c,0,0,true).overflow_adjustment==(vp?410:3),
              "Matching teammate contributes overflow in the selected GameCore units");
        players.players[3].queue={1};
        check(research_turns_left(c,0,0,true).research_rate_times100==1000,
              "Teammate on another technology must not contribute science");
        cities.players[3].cities.clear();players.players[3].queue.clear();players.players[3].overflow_times100=0;
        if(vp)vp_city_cached_yields(cities,0,0)[3]=0;city.yields[3].terrain=0;
        check(!research_turns_left(c,0,0,true).turns.has_value(),
              "Zero research rate must remain unbounded without an x86 INT_MAX sentinel");
        reset();techs.entries[0].integers["Cost"]=100000000000LL;
        check(research_turns_left(c,0,0,true).turns.value()>INT32_MAX,
              "Large modded research costs must allow turn estimates beyond int32");
        techs.entries[0].integers["Cost"]=10000;
        reset();
        {
            auto &balance=treasury.players[0].balance_times100;
            const auto saved_balance=balance;
            balance=500000000001LL;
            city.yields[2].terrain=2;
            if(vp)vp_city_cached_yields(cities,0,0)[2]=200;
            TreasuryTurnContext gold{treasury,economy,maintenance,tr,cr,mr,defs,
                vp?&options:nullptr,gold_host,0,{}};
            auto detached=treasury;
            TreasuryTurnContext wrong{detached,economy,maintenance,tr,cr,mr,defs,
                vp?&options:nullptr,gold_host,0,{}};
            std::int64_t after_gold_calls=0;
            GameCoreAfterGoldStep after_gold{profile,[&](TreasuryTurnContext &,std::int64_t player){
                check(player==0,"GameCore after-gold hook must receive the active player");
                ++after_gold_calls;
            }};
            rejects([&]{(void)do_player_economy_research_stage(wrong,c,0,after_gold);});
            GameCoreAfterGoldStep wrong_profile{vp?TechnologyProfile::bnw:TechnologyProfile::vp542,
                after_gold.run};
            rejects([&]{(void)do_player_economy_research_stage(gold,c,0,wrong_profile);});
            check(balance==500000000001LL&&teams[0].progress_times100(0)==0,
                  "Mismatched economy/research contexts must fail before changing either world");
            const auto staged=do_player_economy_research_stage(gold,c,0,after_gold);
            check(staged.gold.net_times100==200&&balance==500000000201LL&&
                  staged.research.beakers_times100==1155&&teams[0].progress_times100(0)==1155&&after_gold_calls==1,
                  "Gold and research must run in source order on one wide live world");
            reset();
            city.yields[2].terrain=0;
            if(vp)vp_city_cached_yields(cities,0,0)[2]=0;
            maintenance.players[0].base_building=2;
            balance=200;
            const auto science_before=science_times100(science,economy,treasury,maintenance,tr,cr,mr,
                defs,vp?&options:nullptr,0).total_times100;
            const auto spent=do_player_economy_research_stage(gold,c,0,after_gold);
            check(science_before==1000&&spent.gold.net_times100==-200&&balance==0&&
                  spent.research.beakers_times100==955&&teams[0].progress_times100(0)==955&&after_gold_calls==2,
                  "Research must see DoGold's updated treasury when budget deficit reduces science");
            balance=saved_balance;
            maintenance.players[0].base_building=0;
        }
        reset();auto q=do_research(c,0);
        check(q.beakers_times100==1155&&p.overflow_times100==0&&teams[0].progress_times100(0)==1155&&q.science_queries==(vp?2:3),"Research turn must use real science and include stored overflow with source query count");
        check(s.agreement_counters.at(1)==(vp?1007:17)&&s.agreement_counters.at(2)==(vp?1011:21)&&s.agreement_counters.at(3)==17&&s.agreement_counters.at(0)==123,
            "Agreements must update dead major teammates in profile units, but skip self/same-team counters");
        check((vp?vp_science_profile(science.players[0]).last_turn_times100==1000:
               !science.players[0].profile_extensions.find<VpScienceProfile>())&&
              s.agreement_counters.at(5000000000LL)==19,
              "Only VP updates last-turn science; out-of-domain counter retained");
        science.research_agreements[0].clear();update_research_agreements(c,0,-7);check(!s.agreement_counters.contains(1)&&!s.agreement_counters.contains(2),"Expired agreement counters must reset to zero");science.research_agreements[0]={1};
        s.agreement_counters[1]=INT64_MAX;rejects([&]{update_research_agreements(c,0,1);});
        reset();science.no_science=progress.no_science=true;q=do_research(c,0);check(q.disabled&&host.events.empty()&&p.overflow_times100==155,"NO_SCIENCE must skip progress, choices and achievements");science.no_science=progress.no_science=false;
        s.cities_founded=0;s.founded_first_city=false;q=do_research(c,0);check(!q.research_enabled&&q.science_queries==0&&p.overflow_times100==155,"Research eligibility depends on founded state, not owned cities");
        s.cities_founded=vp?0:10;s.founded_first_city=vp?true:false;q=do_research(c,0);check(!q.research_enabled,"BNW founded-first flag and VP city-founded count must remain distinct");
        reset();p.queue.clear();q=do_research(c,0);
        check(q.overflow_added==10&&p.overflow_times100==165&&host.events==std::vector<std::string>{"ui"},"No-tech overflow must preserve source divide-by-100 and UI choice ordering");
        check(q.agreement_science.has_value()==vp&&q.science_queries==(vp?3:2),"No-tech RA update/query order differs between profiles");
        reset();p.queue.clear();host.ui=[&]{p.queue={0};};q=do_research(c,0);
        check(vp?(q.technology==-1&&p.overflow_times100==165):(q.technology==0&&p.overflow_times100==0),"UI queue changes must affect the correct before/after stage");
        reset();p.queue.clear();maintenance.elapsed_turns=5;host.ai=[&]{p.queue={0};s.researching={0};};
        if(vp)options.entries["AI_CONTROL_TECH_CHOICE"].value=1;
        q=do_research(c,0);
        check(vp?(p.queue==std::vector<std::int64_t>{0}&&!q.forced_choice&&teams[0].progress_times100(0)==0):
            (p.queue.empty()&&s.researching.empty()&&q.forced_choice&&teams[0].progress_times100(0)==1155),"BNW forced AI choice must clear queue after progress; VP AI choice follows progress");
        check(host.events==(vp?std::vector<std::string>{"ai"}:std::vector<std::string>{"ui","ai","dirty","buttons","dirty"}),"Research choice and queue dirty callbacks have wrong order");
        options.entries["AI_CONTROL_TECH_CHOICE"].value=0;
        reset();teams[0].set_progress_times100(0,999900);rejects([&]{do_research(c,0);});
        check(p.overflow_times100==1055&&teams[0].progress_times100(0)==1001055&&
              (vp?vp_science_profile(science.players[0]).last_turn_times100==555:
                  !science.players[0].profile_extensions.find<VpScienceProfile>())&&
              s.agreement_counters.at(1)==7&&host.events.back()=="acquire",
            "Unimplemented technology effects must stop before RA/last-science changes, retaining prior overflow/progress");
        reset();teams[0].set_progress_times100(0,999900);host.acquire=[&]{teams[0].set_has_tech(0,true);p.queue={1};if(vp)vp_city_cached_yields(cities,0,0)[3]=2000;city.yields[3].terrain=20;};
        q=do_research(c,0);check(q.progress_change->completed&&q.agreement_science==(vp?2000:20)&&p.queue==std::vector<std::int64_t>{1},"Post-completion RA must query changed world, not reuse pre-completion science");
        reset();p.overflow_times100=5000000001LL;techs.entries[0].integers["Cost"]=10000000000LL;
        q=do_research(c,0);check(q.beakers_times100==5000001001LL&&teams[0].progress_times100(0)==5000001001LL,"Research accumulation must not inherit the x86 INT_MAX cap");
        {
            LuaScriptHost scripts;
            scripts.execute("research-mod","GameEvents.PlayerCanEverResearch.Add(function(player, tech) return tech == 1 end)","=research-mod.lua");
            scripts.complete_loading();
            LuaResearchSelectionServices selection_host(scripts,[&](auto team){host.research_buttons_dirty(team);host.game_data_and_score_dirty(team);});
            reset();p.queue.clear();s.researching.clear();
            techs.entries[0].booleans["Disable"]=techs.entries[1].booleans["Disable"]=false;
            ResearchSelectionState selection_state;selection_state.profile=profile;
            selection_state.players[0].civilization="CIV_TEST";selection_state.disabled_technologies["CIV_TEST"]={};
            ResearchSelectionContext selection{selection_state,state,players,setup,techs,vp?&options:nullptr,teams,
                maintenance.team_eras,{0,0},selection_host,0,false};
            check(!push_research(selection,0,0)&&p.queue.empty(),"Executed Lua veto must prevent queue insertion before real science accrual");
            check(push_research(selection,0,1)&&p.current_research()==1&&s.researching==std::set<std::int64_t>{1},
                "Research selection must update the same live queue and flags used by the turn");
            q=do_research(c,0);
            check(q.technology==1&&q.beakers_times100==1155&&teams[0].progress_times100(1)==1155&&p.overflow_times100==0,
                "Real science must accrue to the technology selected through the shared queue API");
        }
        // Sparse achievement scan must not iterate billions of absent slots.
        techs.collection.slots=5000000001LL;teams.clear();teams.emplace_back(profile,0,techs.collection.slots);teams.emplace_back(profile,1,techs.collection.slots);
        progress.teams=teams; // economy is reconstructed after vector reallocation.
        MaintenanceWorld wide_world{setup,players,cities,teams};ResearchTurnContext wide_c{state,progress,science,wide_world,treasury,maintenance,tr,cr,mr,host,0,{}};
        options.entries["ENABLE_ACHIEVEMENTS"].value=1;s.leader_type="LEADER_CATHERINE";techs.entries[0].base.type="TECH_HORSEBACK_RIDING";
        teams[0].set_has_tech(0,true);host.events.clear();check_research_achievements(wide_c,0);check(host.events==std::vector<std::string>{},"Same-team major player counts separately for Catherine's achievement");
        players.major_players.erase(3);host.events.clear();check_research_achievements(wide_c,0);check(host.events==std::vector<std::string>{"pony"},"Catherine achievement must use owning team knowledge");
        s.leader_type="LEADER_TEST";techs.entries[1].booleans["Repeat"]=true;p.queue={1};teams[0].set_has_tech(1,true);teams[0].increment_count(1);host.events.clear();
        check_research_achievements(wide_c,0);check(host.events==std::vector<std::string>{"all"},"All-tech achievement must handle sparse wide catalog");
        setup.game_type=3;host.events.clear();check_research_achievements(wide_c,0);check(host.events.empty(),"PBEM must suppress research achievements");
        // Import founded eligibility, signed/wide counters, real leader types,
        // raw VP flags and BNW last-duplicate-wins hash remapping.
        RulesDatabase imported_db;imported_db.apply_sql("CREATE TABLE Technologies(ID,Type);INSERT INTO Technologies VALUES(0,'TECH_A'),(1,'TECH_B');"
            "CREATE TABLE Leaders(ID,Type);INSERT INTO Leaders VALUES(0,'LEADER_TEST');CREATE TABLE MinorCivilizations(ID,Type);");
        const auto types=imported_db.cache_info_types();TechnologyRules imported_techs;imported_techs.profile=profile;imported_techs.collection.slots=2;
        ResearchContext imported_setup;imported_setup.profile=profile;imported_setup.players[0]={};
        civ5::saves::SavedSessionSnapshot snapshot{};const auto layout=vp?civ5::saves::GameStateLayout::vp542_corpus:civ5::saves::GameStateLayout::expansion2_version1;
        snapshot.all_players.emplace().layout=layout;snapshot.pregame.emplace().layout=layout;
        snapshot.pregame->archive.flag_arrays["minor_nation_civs"]={false};snapshot.pregame->archive.string_arrays["minor_civ_types"]={""};
        snapshot.pregame->slots.string_arrays["leader_keys"]={"LEADER_TEST"};
        civ5::saves::SavedPlayerObservation saved{};saved.saved_player_id=0;saved.core.layout=saved.knowledge.layout=layout;
        const auto field=[&](const char *name,std::vector<std::int64_t> values){civ5::saves::PlayerCoreField f{};f.name=name;f.values=values;saved.core.fields.push_back(f);};
        field("m_iNumCitiesFounded",{7});field("m_bFoundedFirstCity",{1});field("m_aiResearchAgreementCounter",{0,-7,5000000001LL});
        civ5::saves::PlayerKnowledgeBlock block{};block.name="player_techs";
        if(vp){civ5::saves::PlotArrayObservation flags{};flags.name="researching_tech";flags.values={1,0};block.arrays={flags};}
        else {civ5::saves::TeamHashedArray flags{};flags.name="researching_tech";flags.rows={{0,{}},{saved_info_type_hash("TECH_A"),{1}},
            {saved_info_type_hash("TECH_B"),{1}},{saved_info_type_hash("TECH_A"),{0}},{UINT32_MAX,{1}}};block.hashes={flags};}
        saved.knowledge.blocks={block};snapshot.all_players->players={saved};
        auto imported=restore_research_turn(snapshot,imported_setup,imported_techs,types,imported_db);const auto &ip=imported.players.at(0);
        check(ip.cities_founded==7&&ip.founded_first_city==!vp&&ip.agreement_counters==YieldValues({{1,-7},{2,5000000001LL}})&&ip.leader_type=="LEADER_TEST",
            "Research turn import lost eligibility, leader or wide signed agreement counters");
        check(ip.researching==(vp?std::set<std::int64_t>{0}:std::set<std::int64_t>{1})&&ip.unmapped_researching==(vp?0U:1U),
            "Researching flags must use direct VP rows or BNW hash remapping, with last false duplicate clearing a flag");
        snapshot.all_players->players.push_back(saved);rejects([&]{restore_research_turn(snapshot,imported_setup,imported_techs,types,imported_db);});
        snapshot.all_players->players.pop_back();snapshot.pregame->slots.string_arrays["leader_keys"]={"REMOVED_LEADER"};
        check(!restore_research_turn(snapshot,imported_setup,imported_techs,types,imported_db).players.at(0).leader_type,"Removed leader must stay unresolved rather than become a guessed Info");
    }
    {
        ResearchContext roster;roster.profile=TechnologyProfile::vp542;
        constexpr std::int64_t id=5000000001LL;
        roster.players[id]={};roster.players[id].policies={0,1,2,3};
        roster.players[id].policies_resolved=true;
        civ5::saves::SavedSessionSnapshot snapshot;snapshot.pregame.emplace();
        snapshot.pregame->layout=civ5::saves::GameStateLayout::vp542_corpus;
        snapshot.pregame->world.values.integers["num_cities_policy_cost_mod"]=500;
        snapshot.all_players.emplace();
        snapshot.all_players->layout=civ5::saves::GameStateLayout::vp542_corpus;
        civ5::saves::SavedPlayerObservation saved;saved.saved_player_id=id;
        saved.core.layout=saved.strategy.layout=civ5::saves::GameStateLayout::vp542_corpus;
        const auto core=[&](const char *name,std::int64_t value){
            civ5::saves::PlayerCoreField field{};field.name=name;field.values={value};saved.core.fields.push_back(field);
        };
        core("m_iNumPolicies",4);core("m_iNumFreePolicies",2);
        core("m_iNumFreePoliciesEver",3);core("m_iNumFreeTenets",1);
        core("m_iPolicyCostModifier",25);core("m_iNumCitiesPolicyCostDiscount",-10);
        civ5::saves::PlayerStrategyBlock policies{};policies.name="policies";
        civ5::saves::PlotArrayObservation free{};free.name="free_policy";
        free.columns=1;free.values={0,1,0,0};
        civ5::saves::PlotArrayObservation owned{};owned.name="has_policy";
        owned.columns=1;owned.values={1,1,1,1};
        civ5::saves::PlotArrayObservation branches{};branches.name="branch_unlocked";
        branches.columns=1;branches.values={0,1};
        policies.arrays={free,owned,branches};saved.strategy.blocks={policies};
        snapshot.all_players->players={saved};
        restore_vp_policy_cost_saved_profile(snapshot,roster);
        const auto *profile=find_vp_policy_cost_saved_profile(roster);
        check(profile&&profile->players.at(id).free_policies_ever==3&&
              profile->players.at(id).free_tenets==1&&
              profile->players.at(id).city_cost_discount==-10&&
              profile->players.at(id).branch_unlocked==std::vector<std::uint8_t>({0,1})&&
              profile->world_city_modifier_times100==500,
              "VP policy-cost inputs must remain in the profile extension");
        RulesDatabase policy_rules;
        policy_rules.apply_sql("CREATE TABLE Defines(Name,Value);"
            "INSERT INTO Defines VALUES('NUM_CITIES_COST_MOD_SCALING',20);"
            "INSERT INTO Defines VALUES('POLICY_COST_INCREASE_TO_BE_EXPONENTED',3);"
            "INSERT INTO Defines VALUES('POLICY_COST_EXTRA_VALUE',0);"
            "INSERT INTO Defines VALUES('POLICY_COST_EXPONENT',2);"
            "INSERT INTO Defines VALUES('BASE_POLICY_COST',25);"
            "INSERT INTO Defines VALUES('POLICY_COST_VISIBLE_DIVISOR',5);"
            "INSERT INTO Defines VALUES('AI_HANDICAP',6);"
            "CREATE TABLE GameSpeeds(ID INTEGER,CulturePercent INTEGER);"
            "INSERT INTO GameSpeeds VALUES(4,150);"
            "CREATE TABLE HandicapInfos(ID INTEGER,PolicyPercent INTEGER,"
            "PolicyPerEraModifier INTEGER,AIPolicyPercent INTEGER,"
            "AIPolicyPerEraModifier INTEGER,PolicyCatchUpMod INTEGER,"
            "AIPolicyCatchUpMod INTEGER);"
            "INSERT INTO HandicapInfos VALUES(6,90,0,100,0,0,0),"
            "(7,120,5,100,0,10,0),(8,100,0,80,-10,0,20);"
            "CREATE TABLE PolicyBranchTypes(ID INTEGER,Type TEXT,PurchaseByLevel INTEGER);"
            "INSERT INTO PolicyBranchTypes VALUES(1,'IDEOLOGY',1);"
            "CREATE TABLE Policies(ID INTEGER,IsDummy INTEGER,Level INTEGER,PolicyBranchType TEXT);"
            "INSERT INTO Policies VALUES(0,0,2,'IDEOLOGY'),(1,0,0,NULL),"
            "(2,1,0,NULL),(3,0,1,'IDEOLOGY'),(4,0,0,NULL);");
        const auto count=vp_paid_policy_count(roster,policy_rules,id);
        check(count.owned==4&&count.free==1&&count.dummy==1&&count.paid==2,
              "VP paid-policy count must honor selected SQL dummy and saved free flags");
        PlayerResearchContext live_cities;live_cities.profile=TechnologyProfile::vp542;
        roster.speed=4;
        live_cities.players[id].cities[0]={};
        live_cities.players[id].cities[1].puppet=true;
        live_cities.players[id].cities[2].ignore_happiness=true;
        const auto policy_defines=policy_rules.cache_global_defines(GlobalDefineProfile::vp542);
        const auto vp_city_cost=vp_policy_city_modifier_times100(
            roster,live_cities,policy_defines,true,id);
        check(vp_city_cost.effective_cities==2&&vp_city_cost.policy_cities==2&&
              vp_city_cost.base_times100==450&&vp_city_cost.scaling_times100==18&&
              vp_city_cost.pairs==1&&vp_city_cost.total_times100==918,
              "VP policy city cost must include puppets and omit ignored cities");
        check(vp_policy_city_modifier_times100(roster,live_cities,policy_defines,false,id)
                  .total_times100==450,
              "CP policy city cost must exclude the first effective city");
        const auto subtotal=vp_policy_cost_subtotal(roster,live_cities,policy_rules,
            policy_defines,true,id);
        check(subtotal.counted_policies==2&&subtotal.charged_policies==2&&
              subtotal.exponent_input==6&&subtotal.exponent_cost==36&&
              subtotal.base_cost==61&&subtotal.after_cities==66&&
              subtotal.after_player_modifier==82&&subtotal.speed_percent==150&&
              subtotal.after_speed==123,
              "VP policy cost subtotal must preserve source truncation after each multiplier");
        const auto ideology=vp_policy_ideology_cost(roster,policy_rules,policy_defines,true,
            id,subtotal.after_speed);
        check(ideology.tier1==1&&ideology.tier2==1&&ideology.tier3==0&&
              ideology.penalty_percent==6&&ideology.after_ideology==130,
              "VP ideology must count owned tenets by selected branch and level");
        check(vp_policy_ideology_cost(roster,policy_rules,policy_defines,false,id,
                  subtotal.after_speed).after_ideology==subtotal.after_speed,
              "CP branch must not impose VP ideology policy penalty");
        constexpr std::int64_t other=id+1;
        roster.current_era=2;roster.game_handicap=8;
        roster.players[id].alive=true;roster.players[id].slot_status=3;
        roster.players[id].handicap=7;
        roster.players[other].alive=true;
        roster.players[other].policies={0,1,2,3,4};
        roster.players[other].policies_resolved=true;
        ensure_vp_policy_cost_saved_profile(roster).players[other].free_policy={0,0,0,0,0};
        live_cities.major_players={id,other};
        const auto human_cost=vp_policy_final_cost(roster,live_cities,policy_rules,
            policy_defines,false,id,ideology.after_ideology,subtotal.counted_policies);
        check(human_cost.major&&human_cost.human_for_handicap&&
              human_cost.player_handicap==7&&human_cost.player_percent==120&&
              human_cost.player_era_factor==110&&human_cost.after_player_handicap==171&&
              human_cost.higher_players==1&&human_cost.possible_players==1&&
              human_cost.catchup_factor==80&&human_cost.after_catchup==136&&
              human_cost.visible_divisor==5&&human_cost.cost==135,
              "VP human policy cost must apply selected handicap, catch-up and visible rounding");
        roster.players[id].slot_status=2;
        const auto ai_cost=vp_policy_final_cost(roster,live_cities,policy_rules,
            policy_defines,false,id,ideology.after_ideology,subtotal.counted_policies);
        check(!ai_cost.human_for_handicap&&ai_cost.game_handicap==8&&
              ai_cost.ai_percent==80&&ai_cost.ai_era_factor==80&&
              ai_cost.after_ai_handicap==108&&ai_cost.catchup_modifier==30&&
              ai_cost.catchup_factor==40&&ai_cost.after_catchup==43&&ai_cost.cost==40,
              "VP AI policy cost must apply game handicap after player handicap");
        roster.players[id].slot_status=3;
        const auto human_as_ai=vp_policy_final_cost(roster,live_cities,policy_rules,
            policy_defines,true,id,ideology.after_ideology,subtotal.counted_policies);
        check(!human_as_ai.human_for_handicap&&human_as_ai.player_handicap==6&&
              human_as_ai.after_player_handicap==117&&human_as_ai.game_handicap==8,
              "VP human-uses-AI-handicap option must select AI Info and AI modifiers");
        live_cities.major_players.erase(id);
        check(vp_policy_final_cost(roster,live_cities,policy_rules,policy_defines,
                  false,id,ideology.after_ideology,subtotal.counted_policies).cost==130,
              "Non-major VP player must bypass handicap and catch-up but retain rounding");
        live_cities.major_players.insert(id);
        roster.players[id].team=7;
        live_cities.teams[7].master=-1;
        auto &policy_culture=ensure_vp_culture_history_profile(roster).players[id];
        policy_culture.next_policy_cost=135;
        policy_culture.balance_times100=13000;
        CustomModOptions policy_options;
        const auto safe_phase=vp_policy_phase_without_host(
            roster,live_cities,policy_rules,policy_options,id);
        check(safe_phase.outer_guard&&safe_phase.human_policy_choice&&
              safe_phase.human_diplomacy&&safe_phase.ideology_branch==1&&
              !safe_phase.affordable&&safe_phase.safe_no_effect,
              "Chosen ideology and unaffordable policy must prove human VP policy phase has no effect");
        policy_culture.balance_times100=13500;
        check(!vp_policy_phase_without_host(roster,live_cities,policy_rules,
                  policy_options,id).safe_no_effect,
              "Affordable VP policy must wait for the notification service");
        policy_culture.balance_times100=13000;
        ensure_vp_policy_cost_saved_profile(roster).players[id].branch_unlocked[1]=0;
        check(!vp_policy_phase_without_host(roster,live_cities,policy_rules,
                  policy_options,id).safe_no_effect,
              "Unchosen VP ideology must wait for eligibility and choice logic");
        ensure_vp_policy_cost_saved_profile(roster).players[id].branch_unlocked[1]=1;
        live_cities.teams[7].master=8;
        check(!vp_policy_phase_without_host(roster,live_cities,policy_rules,
                  policy_options,id).safe_no_effect,
              "Human vassal policy phase must wait for ideology-switch effects");
        live_cities.teams[7].master=-1;
        policy_options.entries["AI_CONTROL_POLICY_CHOICE"].value=1;
        check(!vp_policy_phase_without_host(roster,live_cities,policy_rules,
                  policy_options,id).safe_no_effect,
              "AI-controlled VP policy phase must not be silently skipped");
        policy_options.entries["AI_CONTROL_POLICY_CHOICE"].value=0;
        policy_options.entries["AI_CONTROL_DIPLOMACY"].value=1;
        check(!vp_policy_phase_without_host(roster,live_cities,policy_rules,
                  policy_options,id).safe_no_effect,
              "AI-controlled diplomacy can switch ideology even for human policy choice");
        policy_options.entries["AI_CONTROL_DIPLOMACY"].value=0;
        roster.players[id].alive=false;
        check(vp_policy_phase_without_host(roster,live_cities,policy_rules,
                  policy_options,id).safe_no_effect,
              "Dead VP player must bypass the policy phase's outer guard");
        roster.players[id].alive=true;
        ensure_vp_policy_cost_saved_profile(roster).world_city_modifier_times100=5000000000LL;
        check(vp_policy_city_modifier_times100(roster,live_cities,policy_defines,true,id)
                  .total_times100==9000000018LL,
              "VP policy city modifier must not inherit source i32 multiplication limits");
        ensure_vp_policy_cost_saved_profile(roster).world_city_modifier_times100=500;
        ensure_vp_policy_cost_saved_profile(roster).world_city_modifier_times100=500000000000LL;
        check(vp_policy_cost_subtotal(roster,live_cities,policy_rules,policy_defines,true,id)
                  .after_cities>INT32_MAX,
              "VP policy cost subtotal must accept results above source i32 capacity");
        ensure_vp_policy_cost_saved_profile(roster).world_city_modifier_times100=500;
        roster.speed=99;
        rejects([&]{(void)vp_policy_cost_subtotal(roster,live_cities,policy_rules,
            policy_defines,true,id);});
        roster.speed=4;
        roster.players[id].policies_resolved=false;
        rejects([&]{(void)vp_paid_policy_count(roster,policy_rules,id);});
        roster.players[id].policies_resolved=true;
        auto absent_rules=RulesDatabase{};
        absent_rules.apply_sql("CREATE TABLE Policies(ID INTEGER,IsDummy INTEGER);"
            "INSERT INTO Policies VALUES(0,0),(1,0),(2,1);");
        rejects([&]{(void)vp_paid_policy_count(roster,absent_rules,id);});
        snapshot.all_players->players[0].strategy.blocks.clear();
        rejects([&]{restore_vp_policy_cost_saved_profile(snapshot,roster);});
        check(find_vp_policy_cost_saved_profile(roster)->players.at(id).free_policy.size()==4,
              "Failed VP policy-cost import must preserve the prior profile");
        ResearchContext vanilla;rejects([&]{(void)ensure_vp_policy_cost_saved_profile(vanilla);});
    }
    {
        ResearchContext roster;roster.profile=TechnologyProfile::vp542;roster.players[5000000001LL]={};
        civ5::saves::SavedSessionSnapshot snapshot;snapshot.all_players.emplace();
        snapshot.all_players->layout=civ5::saves::GameStateLayout::vp542_corpus;
        civ5::saves::SavedPlayerObservation saved;saved.saved_player_id=5000000001LL;
        saved.core.layout=saved.trade_culture.layout=civ5::saves::GameStateLayout::vp542_corpus;
        civ5::saves::PlayerCoreField lifetime{};lifetime.name="m_lJONSCultureEverGeneratedTimes100";
        lifetime.values={5000000000LL};saved.core.fields={lifetime};
        const auto culture_field=[&](const char *name,std::int64_t value){
            civ5::saves::PlayerCoreField field{};field.name=name;field.values={value};saved.core.fields.push_back(field);
        };
        culture_field("m_iJONSCultureTimes100",100);
        culture_field("m_iCostNextPolicy",100);
        culture_field("m_iAnarchyNumTurns",0);
        civ5::saves::PlayerTradeCultureRecord culture{};culture.name="player_culture";
        culture.fields={{"m_lLastTurnLifetimeCultureTimes100",100},{"m_iLastTurnCPT",17}};
        saved.trade_culture.blocks={culture};snapshot.all_players->players={saved};
        restore_vp_culture_history_profile(snapshot,roster);
        auto policy_state=roster;
        ensure_vp_culture_history_profile(policy_state).players.at(5000000001LL)
            .balance_times100=15000;
        check(apply_vp_culture_income(policy_state,5000000001LL,100,{false,true,false})
                  .timer_blocked,
              "Timed VP culture must compare against the prior policy cost");
        VpPolicyFinalQuote next_policy{};next_policy.cost=200;
        const auto policy_update=do_vp_policy_cost_after_culture(
            policy_state,5000000001LL,next_policy);
        check(policy_update.before==100&&policy_update.after==200&&
              find_vp_culture_history_profile(roster)->players.at(5000000001LL)
                  .next_policy_cost==100,
              "VP policy update must change only the disposable profile state");
        const auto after_policy=apply_vp_culture_income(
            policy_state,5000000001LL,100,{false,true,false});
        check(!after_policy.timer_blocked&&after_policy.balance_after_times100==15100,
              "The following timed turn must use the recalculated VP policy cost");
        auto copied=roster;
        const auto delta=do_vp_culture_history_after_gold(copied,5000000001LL);
        check(delta==4999999900LL&&delta>INT32_MAX&&
              find_vp_culture_history_profile(copied)->players.at(5000000001LL).last_lifetime_times100==5000000000LL&&
              find_vp_culture_history_profile(roster)->players.at(5000000001LL).last_lifetime_times100==100,
              "VP culture history must widen saved values and mutate only its copied profile");
        check(do_vp_culture_history_after_gold(copied,5000000001LL)==0,
              "Repeated VP culture snapshot must see the new lifetime baseline");
        const auto credited=apply_vp_culture_income(copied,5000000001LL,3000000000LL,{});
        check(credited.credited_times100==3000000000LL&&credited.balance_after_times100==3000000100LL&&
              credited.lifetime_after_times100==8000000000LL,
              "VP culture accrual must widen bank and lifetime beyond original 32-bit storage");
        check(do_vp_culture_history_after_gold(copied,5000000001LL)==3000000000LL,
              "The next turn must observe the culture credited after the prior history snapshot");
        const auto timed=apply_vp_culture_income(copied,5000000001LL,100,{false,true,false});
        check(timed.timer_blocked&&timed.balance_after_times100==3000000100LL,
              "End-turn timer must stop culture accrual once a policy is affordable");
        const auto disabled=apply_vp_culture_income(copied,5000000001LL,100,{true,false,false});
        check(disabled.credited_times100==0&&disabled.balance_after_times100==3000000100LL,
              "No-policies option must suppress VP culture income");
        auto &extreme=ensure_vp_culture_history_profile(copied).players.at(5000000001LL);
        extreme.anarchy_turns=1;
        check(apply_vp_culture_income(copied,5000000001LL,100,{}).credited_times100==0,
              "Anarchy must suppress VP culture income");
        extreme.anarchy_turns=0;
        extreme.balance_times100=1000;
        check(apply_vp_culture_income(copied,5000000001LL,50,{false,false,true}).balance_after_times100==50&&
              extreme.lifetime_times100==8000000000LL,
              "Minor culture getter must expose zero without fabricating lifetime culture");
        extreme.balance_times100=0;
        extreme.lifetime_times100=INT64_MAX;extreme.last_lifetime_times100=INT64_MIN;
        rejects([&]{(void)do_vp_culture_history_after_gold(copied,5000000001LL);});
        check(extreme.last_lifetime_times100==INT64_MIN,
              "An unrepresentable culture delta must fail before mutating history");
        rejects([&]{(void)apply_vp_culture_income(copied,5000000001LL,1,{});});
        check(extreme.balance_times100==0&&extreme.lifetime_times100==INT64_MAX,
              "Culture accrual overflow must not partially mutate the VP player");
        ResearchContext vanilla;rejects([&]{(void)ensure_vp_culture_history_profile(vanilla);});
        snapshot.all_players->players[0].trade_culture.blocks[0].fields.pop_back();
        rejects([&]{restore_vp_culture_history_profile(snapshot,roster);});
        check(find_vp_culture_history_profile(roster)->players.at(5000000001LL).last_cpt_times100==17,
              "Failed VP culture import must preserve the prior profile");
    }
    {
        VpCultureIncomeInputs inputs;
        inputs.cities_times100=10000;inputs.happiness_times100=500;inputs.traits_times100=300;
        inputs.free_times100=200;inputs.minors_times100=120;inputs.annexed_minors_times100=80;
        inputs.incoming_espionage_times100=60;inputs.outgoing_espionage_times100=40;
        inputs.vassals_times100=1000;inputs.religion_modifier=25;
        inputs.temporary_bonus=inputs.conquest_bonus=inputs.golden_age=true;
        inputs.temporary_boost_modifier=33;inputs.conquest_modifier=7;inputs.golden_age_modifier=20;
        const auto quote=vp_culture_income_times100(inputs);
        check(quote.before_modifiers_times100==12220&&quote.religion_extra_times100==3055&&
              quote.bonus_turns_times100==4887&&quote.golden_age_times100==3444&&
              quote.total_times100==23606,
              "VP culture must keep source component, modifier and Golden Age vassal-carry order");
        inputs.golden_age_disabled=true;
        check(vp_culture_income_times100(inputs).golden_age_times100==0,
              "Disabled Golden Age culture must suppress both percent and source carry");
        inputs.no_policies=true;inputs.cities_times100=INT64_MAX;
        check(vp_culture_income_times100(inputs).total_times100==0,
              "No-policies option must return before reading culture components");
        inputs.no_policies=false;inputs.anarchy=true;
        check(vp_culture_income_times100(inputs).total_times100==0,
              "Anarchy must return before calculating VP culture");
        inputs={};inputs.cities_times100=10000000000LL;inputs.vassals_times100=3000000000LL;
        inputs.golden_age=true;inputs.golden_age_modifier=20;
        check(vp_culture_income_times100(inputs).total_times100==18600000000LL,
              "VP culture arithmetic must exceed every original signed 32-bit capacity");
        inputs.cities_times100=INT64_MAX;
        rejects([&]{(void)vp_culture_income_times100(inputs);});
    }
    {
        ResearchContext roster;roster.profile=TechnologyProfile::vp542;roster.players[5000000002LL]={};
        civ5::saves::SavedSessionSnapshot snapshot;snapshot.all_players.emplace();
        snapshot.all_players->layout=civ5::saves::GameStateLayout::vp542_corpus;
        civ5::saves::SavedPlayerObservation saved;saved.saved_player_id=5000000002LL;
        saved.core.layout=saved.traits.layout=saved.tail.layout=civ5::saves::GameStateLayout::vp542_corpus;
        const auto core=[&](const char *name,std::vector<std::int64_t> values){
            civ5::saves::PlayerCoreField f{};f.name=name;f.values=std::move(values);saved.core.fields.push_back(f);
        };
        const auto traits=[&](const char *name,std::vector<std::int64_t> values){
            civ5::saves::PlayerTraitsField f{};f.name=name;f.values=std::move(values);saved.traits.fields.push_back(f);
        };
        const auto tail=[&](const char *name,std::vector<std::int64_t> values){
            civ5::saves::PlayerTailNode f{};f.name=name;f.values=std::move(values);saved.tail.fields.push_back(f);
        };
        core("m_iJONSCulturePerTurnForFree",{3000000000LL});
        core("m_aiYieldFromMinors",{0,0,0,0,7});
        core("m_piYieldPerTurnFromAnnexedMinorsTimes100",{0,0,0,0,12345});
        core("m_iHappinessToCulture",{2});core("m_iCultureBonusTurns",{1});
        core("m_iCultureBonusTurnsConquest",{2});core("m_iGoldenAgeCultureBonusDisabledCount",{1});
        tail("m_aiIncomingEspionageYields",{0,0,0,0,2});
        tail("m_aiOutgoingEspionageYields",{0,0,0,0,1});
        traits("m_iYieldChangePerTradePartner",{0,0,0,0,3});
        traits("m_iYieldFromExport",{0,0,0,0,4});traits("m_iYieldFromImport",{0,0,0,0,5});
        traits("m_iCultureBonusModifierConquest",{25});
        snapshot.all_players->players={saved};
        restore_vp_culture_income_saved_profile(snapshot,roster,4);
        const auto &p=find_vp_culture_income_saved_profile(roster)->players.at(5000000002LL);
        check(p.free_times100==300000000000LL&&p.jfd_minors_times100==700&&
              p.annexed_minors_times100==12345&&p.incoming_espionage_times100==200&&
              p.outgoing_espionage_times100==100&&p.per_export==4&&p.per_import==5&&
              p.per_trade_partner==3&&p.conquest_modifier==25&&p.golden_age_disabled,
              "VP culture source inputs must use selected yield Info and wide owned values");
        auto copied=roster;ensure_vp_culture_income_saved_profile(copied).players.at(5000000002LL).free_times100=1;
        check(p.free_times100==300000000000LL,"Copied VP culture income profile must be independent");
        snapshot.all_players->players[0].core.fields[1].values.pop_back();
        rejects([&]{restore_vp_culture_income_saved_profile(snapshot,roster,4);});
        ResearchContext vanilla;rejects([&]{(void)ensure_vp_culture_income_saved_profile(vanilla);});
    }
}
