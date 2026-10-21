#pragma once
void research_selection_tests() {
    using namespace civ5::runtime;
    struct Host final:ResearchSelectionServices {
        std::vector<std::string> events;std::set<std::int64_t> denied;bool unavailable=false,veto_current=false;
        std::optional<bool> test_all(std::string_view event,std::int64_t,std::int64_t tech) override {
            events.push_back(std::string(event)+":"+std::to_string(tech));
            if(unavailable)throw ResearchSelectionUnavailable("Lua required");
            if(denied.contains(tech)||(veto_current&&event=="PlayerCanResearch"))return false;
            return {};
        }
        void research_queue_dirty(std::int64_t) override {events.push_back("dirty");}
    } host;
    for(bool vp:{false,true}) {
        const auto profile=vp?TechnologyProfile::vp542:TechnologyProfile::bnw;
        TechnologyRules techs;techs.profile=profile;techs.collection.slots=5000000002LL;
        const auto add=[&](std::int64_t id){auto &p=techs.entries[id];p.booleans={{"Disable",false},{"Repeat",false}};
            p.and_prerequisites.extent=p.or_prerequisites.extent=10000000000ULL;
            p.and_prerequisites.default_value=p.or_prerequisites.default_value=-1;p.era=2;};
        for(auto id:{0LL,1LL,2LL,3LL,4LL,5LL,6LL,5000000001LL})add(id);
        ResearchContext setup;setup.profile=profile;setup.players[5000000000LL].team=7000000000LL;
        PlayerResearchContext players;players.profile=profile;auto &p=players.players[5000000000LL];
        ResearchTurnState turn;turn.profile=profile;auto &r=turn.players[5000000000LL];r.founded_first_city=true;r.cities_founded=1;
        ResearchSelectionState state;state.profile=profile;state.players[5000000000LL].civilization="CIV_TEST";state.disabled_technologies["CIV_TEST"]={};
        CustomModOptions options;options.prefetch.all_succeeded=true;
        std::vector<TeamTechnologies> teams;teams.emplace_back(profile,7000000000LL,techs.collection.slots);
        std::map<std::int64_t,std::int64_t> eras{{7000000000LL,1}};
        ResearchSelectionContext c{state,turn,players,setup,techs,vp?&options:nullptr,teams,eras,{2,2},host,7000000000LL,false};
        const std::int64_t player=5000000000LL;
        host={};check(!can_ever_research(c,player,-1)&&!can_research(c,player,99)&&host.events.empty(),"Missing technology must reject before script hooks");
        techs.entries[0].booleans["Disable"]=true;check(!can_ever_research(c,player,0)&&host.events.empty(),"Disabled Info must reject without scripts");techs.entries[0].booleans["Disable"]=false;
        state.disabled_technologies["CIV_TEST"]={0};check(!can_ever_research(c,player,0)&&host.events.empty(),"Civilization restriction must reject without scripts");state.disabled_technologies["CIV_TEST"].clear();
        options.entries["BALANCE_ERA_RESTRICTION"].value=1;
        check(can_ever_research(c,player,0)==!vp,"Era restriction only applies to VP");options.entries["BALANCE_ERA_RESTRICTION"].value=0;
        host.events.clear();check(can_research(c,player,0)&&host.events==std::vector<std::string>{"PlayerCanEverResearch:0","PlayerCanResearch:0"},"Availability must run both test-all hooks in source order");
        host.veto_current=true;check(!can_research_for_free(c,player,0),"Free research must respect current-research Lua veto");host.veto_current=false;
        host.events.clear();host.denied={0};check(!can_research(c,player,0)&&host.events==std::vector<std::string>{"PlayerCanEverResearch:0"},"Ever veto must skip current hook");host.denied.clear();
        c.no_science=true;check(can_ever_research(c,player,0)==!vp,"BNW CanEverResearch must not inherit VP NO_SCIENCE check");c.no_science=false;
        r.founded_first_city=false;r.cities_founded=0;check(!can_research(c,player,0),"Cityless ordinary research must reject");
        state.players[player].advanced_start_points=0;check(can_research(c,player,0)==!vp,"Only BNW advanced start bypasses city founding");r.founded_first_city=true;r.cities_founded=1;
        teams[0].set_has_tech(0,true);techs.entries[0].booleans["Repeat"]=true;
        check(!can_research(c,player,0),"HasTech rejects even a repeatable Info");teams[0].set_has_tech(0,false);
        techs.entries[2].or_prerequisites.values={{0,0},{1,1}};
        check(!can_research(c,player,2),"OR requires at least one known prerequisite");teams[0].set_has_tech(0,true);teams[0].set_no_trade(0,true);
        check(can_research(c,player,2)&&!can_research(c,player,2,true),"No-trade prerequisite blocks trade only");teams[0].set_has_tech(1,true);
        check(can_research(c,player,2,true),"Second tradable OR must permit trade");
        techs.entries[2].and_prerequisites.values={{1,0}};check(!can_research(c,player,2,true),"AND scan must see entries beyond a hole and honor no-trade");
        check(!can_research_for_free(c,player,-1),"Unassigned free technology must reject");
        host.unavailable=true;rejects([&]{can_ever_research(c,player,1);});host.unavailable=false;
        state.players[player].civilization.reset();rejects([&]{can_ever_research(c,player,1);});state.players[player].civilization="CIV_TEST";
        for(auto &t:techs.entries){t.second.and_prerequisites.values.clear();t.second.or_prerequisites.values.clear();}
        teams[0].set_has_tech(0,false);teams[0].set_has_tech(1,false);host.events.clear();p.queue={1};r.researching={1};
        check(push_research(c,player,1,true)&&p.queue==std::vector<std::int64_t>{1}&&host.events.empty(),"Already queued must return before clear and hooks");
        teams[0].set_has_tech(0,true);check(push_research(c,player,0,true)&&p.queue==std::vector<std::int64_t>{1},"Known target must not clear queue");teams[0].set_has_tech(0,false);
        techs.entries[2].and_prerequisites.values={{0,0},{1,1}};host.denied={1};host.events.clear();
        check(!push_research(c,player,2,true)&&p.queue==std::vector<std::int64_t>{0}&&r.researching==std::set<std::int64_t>{0},"Failed AND must retain earlier clear and prerequisite insertion");
        check(host.events==std::vector<std::string>{"PlayerCanEverResearch:2","dirty","PlayerCanEverResearch:0","dirty","PlayerCanEverResearch:1"},"Queue callbacks must preserve partial mutation order");host.denied.clear();
        clear_research_queue(c,player);techs.entries[3].or_prerequisites.values={{0,1},{1,0}};
        check(push_research(c,player,3)&&p.queue==std::vector<std::int64_t>({1,3}),"Equal OR paths must choose first source relation");
        clear_research_queue(c,player);techs.entries[3].and_prerequisites.values={{0,0}};host.denied={1};state.disabled_technologies["CIV_TEST"]={0};
        check(!push_research(c,player,3)&&p.queue.empty(),"Unavailable AND must fail before OR selection");state.disabled_technologies["CIV_TEST"].clear();
        techs.entries[3].or_prerequisites.values={{0,1}};
        check(!push_research(c,player,3)&&p.queue==std::vector<std::int64_t>{0},"Missing eligible OR retains appended AND");host.denied.clear();
        p.queue={0,0,1};r.researching={0,1};pop_research(c,player,0);
        check(p.queue==std::vector<std::int64_t>({0,1})&&!r.researching.contains(0)&&research_queue_position(p,1)==2,"Pop removes first duplicate and clears flag even with another duplicate");
        host.events.clear();pop_research(c,player,6);check(host.events==std::vector<std::string>{"dirty"}&&research_queue_position(p,6)==-1,"Missing pop still marks active team UI dirty");
        c.active_team=-1;host.events.clear();clear_research_queue(c,player);check(host.events.empty()&&p.queue.empty()&&r.researching.empty(),"Other team queue changes must not dirty active UI");c.active_team=7000000000LL;
        check(push_research(c,player,5000000001LL)&&p.current_research()==5000000001LL,"Queue must preserve player/team/technology IDs beyond uint32");
        clear_research_queue(c,player);
        techs.entries[2].and_prerequisites.values={{0,0},{1,1}};techs.entries[1].and_prerequisites.values={{0,0}};
        check(research_path_length(c,player,2)==(vp?3:4),"VP counts shared prerequisites once; BNW counts both paths");
        r.researching={0};check(research_path_length(c,player,2)==(vp?3:2),"Only BNW path cost treats queued flags as known");r.researching.clear();
        techs.entries[4].and_prerequisites.values={{1,0}};check(research_path_length(c,player,4)==(vp?1:2),"VP path stops on the first missing prerequisite while BNW skips holes");
        techs.entries[4].and_prerequisites.values.clear();techs.entries[4].or_prerequisites.values={{0,0},{1,1}};teams[0].set_has_tech(0,true);
        check(research_path_length(c,player,4)==1,"Known first OR preserves empty VP best set and BNW zero path");teams[0].set_has_tech(0,false);
        techs.entries[4].or_prerequisites.values={{0,1},{1,0}};teams[0].set_has_tech(0,true);
        check(research_path_length(c,player,4)==(vp?2:1),"VP known later OR does not replace the earlier best set");teams[0].set_has_tech(0,false);
        for(std::int64_t i=100;i<=134;++i){add(i);if(i>100)techs.entries[i].and_prerequisites.values={{0,i-1},{1,i-1}};}
        check(research_path_length(c,player,134)==(vp?23:34359738367LL),"Path counts must remove INT_MAX while retaining VP look-ahead semantics");
        check(push_research(c,player,134)&&p.queue.size()==35,"Shared DAG prerequisites must be enqueued once");clear_research_queue(c,player);
        // Deep modded trees use heap frames for insertion, not native recursion.
        for(std::int64_t i=1000;i<5096;++i){add(i);if(i>1000)techs.entries[i].and_prerequisites.values={{0,i-1}};}
        check(research_path_length(c,player,5095)==(vp?23:4096),"Deep path walk must not consume unbounded native stack");
        check(push_research(c,player,5095)&&p.queue.size()==4096,"Deep queue insertion must preserve all modded prerequisites");clear_research_queue(c,player);
        techs.entries[5].and_prerequisites.values={{0,6}};techs.entries[6].and_prerequisites.values={{0,5}};
        rejects([&]{push_research(c,player,5);});if(!vp)rejects([&]{research_path_length(c,player,5);});else check(research_path_length(c,player,5)==2,"VP bounded path must preserve cyclic look-ahead behavior");
        // Import uses saved stable civilization keys and current SQL IDs.
        RulesDatabase db;db.apply_sql("CREATE TABLE Civilizations(ID,Type);INSERT INTO Civilizations VALUES(900,'CIV_TEST');"
            "CREATE TABLE Technologies(ID,Type);INSERT INTO Technologies VALUES(5000000001,'TECH_WIDE');"
            "CREATE TABLE Civilization_DisableTechs(CivilizationType,TechType);INSERT INTO Civilization_DisableTechs VALUES('CIV_TEST','TECH_WIDE'),('CIV_TEST','REMOVED');");
        civ5::saves::SavedSessionSnapshot snap{};const auto layout=vp?civ5::saves::GameStateLayout::vp542_corpus:civ5::saves::GameStateLayout::expansion2_version1;
        snap.pregame.emplace().layout=layout;snap.pregame->slots.string_arrays["civilization_keys"]={"CIV_TEST"};snap.all_players.emplace().layout=layout;
        civ5::saves::SavedPlayerObservation saved{};saved.saved_player_id=0;saved.core.layout=layout;
        civ5::saves::PlayerCoreField field{};field.name="m_iAdvancedStartPoints";field.values={5000000001LL};saved.core.fields={field};snap.all_players->players={saved};
        ResearchContext imported_setup;imported_setup.profile=profile;imported_setup.players[0]={};
        auto imported=restore_research_selection(snap,imported_setup,db);
        check(imported.players[0].civilization=="CIV_TEST"&&imported.players[0].advanced_start_points==(vp?-1:5000000001LL)&&imported.disabled_technologies.at("CIV_TEST")==std::set<std::int64_t>{5000000001LL},"Import must bind stable civilization key and wide SQL restrictions");
        snap.pregame->slots.string_arrays["civilization_keys"]={"REMOVED"};check(!restore_research_selection(snap,imported_setup,db).players[0].civilization,"Missing civilization must remain unresolved");
        snap.all_players->players.push_back(saved);rejects([&]{restore_research_selection(snap,imported_setup,db);});
    }
}
