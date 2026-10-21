#pragma once
void resource_visibility_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;
    constexpr std::int64_t wide=4294967300LL;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('BARBARIAN_CAMP_IMPROVEMENT',8);"
        "CREATE TABLE Policies(ID,Type);INSERT INTO Policies VALUES(0,'POLICY'),(4294967300,'WIDE_POLICY');"
        "CREATE TABLE Technologies(ID,Type);INSERT INTO Technologies VALUES(0,'TECH'),(4294967300,'ALT');"
        "CREATE TABLE Resources(ID,Type,TechReveal,PolicyReveal);INSERT INTO Resources VALUES(0,'IRON','TECH','POLICY'),"
        "(4294967300,'WIDE_RESOURCE','ALT','WIDE_POLICY'),(4294967300,'DUP','UNKNOWN',NULL);"
        "CREATE TABLE Improvements(ID,Type);INSERT INTO Improvements VALUES(8,'CAMP');");
    const auto types=db.cache_info_types();
    for(const auto profile:{TechnologyProfile::bnw,TechnologyProfile::vp542}){
        const bool vp=profile==TechnologyProfile::vp542;
        auto rules=load_resource_visibility_rules(db,types,profile);
        check(rules.policy_slots==wide+1&&rules.resources.at(wide).policy==wide&&rules.resources.at(wide+1).technology==-1,
            "Resource rules preserve wide logical slots, duplicate authored IDs and unknown reference fallback");
        auto defs=db.cache_global_defines(vp?GlobalDefineProfile::vp542:GlobalDefineProfile::bnw);
        ResearchContext players;players.profile=profile;
        for(const auto id:{std::int64_t{0},wide}){auto &p=players.players[id];p.team=wide;p.alive=true;p.policies_resolved=true;}
        PlayerResearchContext roster;roster.profile=profile;roster.teams[wide].members={0,wide};roster.teams[0].members={};
        ResourceVisibilityState state;state.profile=profile;state.teams[wide]={};state.teams[0]={};
        if(vp){auto &visibility_profile=ensure_vp_resource_visibility_profile(state);
            visibility_profile.observers[0]=visibility_profile.observers[wide]=false;
            visibility_profile.alternate_technologies[0]={};visibility_profile.alternate_technologies[wide]={};}
        std::vector<TeamTechnologies> techs;techs.emplace_back(profile,wide,wide+1);techs.emplace_back(profile,0,wide+1);
        SavedMapProjection map{{1,1}};map.cells.emplace_back();auto &plot=map.cells[0];
        plot.resource_type=0;plot.improvement_type=-1;plot.force_resource_reveal.emplace();
        ResourceVisibilityContext c{map,state,rules,players,roster,techs,types,defs,false};
        if(vp){auto extension=state.profile_extensions;state.profile_extensions={};
            rejects([&]{plot_resource_type(c,0,wide);});
            state.profile_extensions=std::move(extension);
        }else check(!state.profile_extensions.find<VpResourceVisibilityProfile>(),"BNW has no VP resource visibility state");
        for(int mask=0;mask<256;++mask){
            const bool known=mask&1,policy=mask&2,forced=mask&4,plot_forced=mask&8,debug=mask&16,
                observer=mask&32,camp=mask&64,ignore=mask&128;
            if(!vp&&ignore)continue;
            techs[0].set_has_tech(0,known);players.players[0].policies=policy?std::set<std::int64_t>{0}:std::set<std::int64_t>{};
            state.teams[wide].forced_resources=forced?std::vector<std::int64_t>{0}:std::vector<std::int64_t>{};
            *plot.force_resource_reveal=plot_forced?std::set<std::int64_t>{wide}:std::set<std::int64_t>{};
            if(vp)vp_resource_visibility_profile(state).observers.at(wide)=observer;
            c.debug_mode=debug;plot.improvement_type=camp?8:-1;
            const bool expected=vp?(!camp&&(debug||observer||ignore||(known&&policy)||forced||plot_forced)):
                (debug||(policy&&(known||forced||plot_forced)));
            check(plot_resource_type(c,0,wide,ignore)==(expected?0:-1),"Profile-specific resource visibility truth table");
            check(plot_resource_type(c,0)==0,"NO_TEAM bypasses every resource visibility gate, including barbarian camp");
        }
        c.debug_mode=false;state.teams[wide]={};if(vp)vp_resource_visibility_profile(state).observers.at(wide)=false;
        plot.improvement_type=-1;plot.force_resource_reveal->clear();
        players.players[0].policies.clear();players.players[wide].policies.clear();techs[0].set_has_tech(0,false);
        plot.resource_type=wide+1;
        check(plot_resource_type(c,0,0)==(vp?-1:wide+1),"VP needs a living member even with no technology or policy requirement; BNW does not");
        plot.resource_type=0;
        if(vp){
            // One player knows an alternative and another owns the policy.
            vp_resource_visibility_profile(state).alternate_technologies[0][0]={wide,123};techs[0].set_has_tech(wide,true);players.players[wide].policies.insert(0);
            check(!resource_revealed_by_team(c,wide,0),"VP may not combine one member's trait with another member's policy");
            players.players[0].policies.insert(0);check(resource_revealed_by_team(c,wide,0),"VP same-member alternative technology and policy");
            players.players[0].alive=false;check(!resource_revealed_by_team(c,wide,0),"Dead player cannot reveal a resource through a trait");
            players.players[0].slot_status=4;refresh_resource_observer(c,wide);
            check(vp_resource_visibility_profile(state).observers.at(wide)&&plot_resource_type(c,0,wide)==0,"Observer cache considers dead roster members too");
            players.players[0].slot_status=2;check(plot_resource_type(c,0,wide)==0,"Observer cache does not update merely because the slot changed");
            refresh_resource_observer(c,wide);check(plot_resource_type(c,0,wide)==-1,"Explicit team status refresh removes observer bypass");
            players.players[0].alive=true;vp_resource_visibility_profile(state).alternate_technologies[0][0].reveal=-1;
            check(!resource_revealed_by_team(c,wide,0),"Alternate NO_TECH means use the default technology");
            techs[0].set_has_tech(0,true);check(resource_revealed_by_team(c,wide,0),"Live technology discovery changes resource visibility");
            roster.teams[wide].members.clear();check(!resource_revealed_by_team(c,wide,0),"VP reads the stored member vector");
            roster.teams[wide].members={0,wide};
            check(!resource_revealed_by_team(c,wide,999),"Missing VP resource Info does not reveal a resource normally");
            plot.resource_type=999;state.teams[wide].forced_resources={999};
            check(plot_resource_type(c,0,wide)==999,"VP force vector can reveal an ID with missing resource Info");
            plot.resource_type=0;state.teams[wide].forced_resources.clear();
        }else{
            players.players[wide].policies.insert(0);techs[0].set_has_tech(0,true);
            check(plot_resource_type(c,0,wide)==0,"BNW policy search includes a player beyond the source roster capacity");
            players.players[wide].team=0;check(plot_resource_type(c,0,wide)==-1,"BNW policy search reads live team assignment");
            players.players[wide].team=wide;players.players[wide].alive=false;
            check(plot_resource_type(c,0,wide)==-1,"BNW policy search excludes dead players");players.players[wide].alive=true;
            rejects([&]{plot_resource_type(c,0,wide,true);});rejects([&]{refresh_resource_observer(c,wide);});
        }
        plot.resource_type=wide;players.players[0].policies.insert(wide);techs[0].set_has_tech(wide,true);
        check(plot_resource_type(c,0,wide)==wide,"Runtime resource, technology, policy and team identities are 64 bit");
        plot.resource_type.reset();plot.resource_type_hash=saved_info_type_hash("WIDE_RESOURCE");
        check(plot_resource_type(c,0,wide)==wide,"Resource hash remaps to shared live content identity");
        PlotPassability pass;pass.layout=vp?GameStateLayout::vp542_corpus:GameStateLayout::expansion2_version1;pass.plots.emplace_back();
        LuaScriptHost host({},{},{},{},{},{},with_resource_visibility(lua_map_services(map,pass),c));
        host.execute("resources","local p=Map.GetPlotByIndex(0);assert(p:GetResourceType()==4294967300);"
            "assert(p:GetResourceType(nil)==-1 and p:GetResourceType('0')==-1);"
            "assert(p:GetResourceType(4294967300)==4294967300);assert(p:GetResourceType(0,true)==-1)","=resource binding");
        check(host.memory_bytes()>0,"Lua resource binding preserves omitted versus nil and ignores extra arguments");
        plot.resource_type_hash=0;check(plot_resource_type(c,0,999999)==-1,"No-resource sentinel returns before team lookup");
        host.execute("resources","assert(Map.GetPlotByIndex(0):GetResourceType(999999)==-1)","=live resource removal");
        plot.resource_type_hash.reset();plot.resource_type=0;players.players[0].policies.insert(0);techs[0].set_has_tech(0,false);
        state.teams[wide]={};if(vp)vp_resource_visibility_profile(state).alternate_technologies[0].clear();plot.force_resource_reveal.reset();
        rejects([&]{plot_resource_type(c,0,wide);});
    }
    // Deserialize policy ownership and force flags independently of the runtime query.
    for(const auto layout:{GameStateLayout::expansion2_version1,GameStateLayout::vp542_corpus}){
        const bool vp=layout==GameStateLayout::vp542_corpus;const auto profile=vp?TechnologyProfile::vp542:TechnologyProfile::bnw;
        SavedSessionSnapshot snapshot;snapshot.teams.emplace();snapshot.teams->layout=layout;snapshot.all_players.emplace();snapshot.all_players->layout=layout;
        TeamObservation t;t.fields.push_back({"id",wide});
        const auto resource=vp?wide:static_cast<std::int64_t>(saved_info_type_hash("WIDE_RESOURCE"));
        t.arrays.push_back({"revealed_resources",1,{resource,resource,0}});snapshot.teams->teams.push_back(t);
        SavedPlayerObservation p;p.saved_player_id=wide;p.strategy.layout=p.traits.layout=layout;
        PlayerStrategyBlock policies{"policies"};
        if(vp){
            policies.arrays.push_back({"has_policy",1,{0,255}});
            PlayerTraitsField f{"m_aiiAlternateResourceTechs",PlayerTraitsEncoding::map_iii};f.columns=3;f.values={wide,wide,7,wide,0,9};p.traits.fields.push_back(f);
        }else policies.hashes.push_back({"has_policy",5,{{0,{}},{saved_info_type_hash("POLICY"),{1}},
            {saved_info_type_hash("POLICY"),{0}},{saved_info_type_hash("WIDE_POLICY"),{255}}}});
        p.strategy.blocks.push_back(policies);snapshot.all_players->players.push_back(p);
        ResearchContext players;players.profile=profile;players.players[wide]={};players.team_members[wide]=1;
        auto rules=load_resource_visibility_rules(db,types,profile);if(vp)rules.policy_slots=2;
        auto state=restore_resource_visibility(snapshot,players,rules,types);
        check(state.teams.at(wide).forced_resources==std::vector<std::int64_t>{wide,wide,vp?0:-1}&&
            (!vp||!vp_resource_visibility_profile(state).observers.at(wide))&&
            (vp||!state.profile_extensions.find<VpResourceVisibilityProfile>()),
            "Restore keeps force-vector duplicates and source sentinel, with a cold observer cache");
        check(players.players.at(wide).policies_resolved&&players.players.at(wide).policies==std::set<std::int64_t>{vp?1:wide},
            "VP byte flags and BNW last duplicate ownership restore to the shared policy set");
        if(vp)check(vp_resource_visibility_profile(state).alternate_technologies.at(wide).at(wide).reveal==wide&&
            vp_resource_visibility_profile(state).alternate_technologies.at(wide).at(wide).city_trade==7,
            "Alternate resource technology map preserves first duplicate and reveal/city-trade order");
        snapshot.all_players->players.push_back(p);rejects([&]{restore_resource_visibility(snapshot,players,rules,types);});
        MapHeaderObservation header{};header.layout=layout;header.grid={1,1};MapPlotsObservation plots{};plots.layout=layout;
        PlotObservation cell;cell.arrays.push_back({vp?"teams":"resource_force_reveal",vp?10u:1u,vp?std::vector<std::int64_t>(30):std::vector<std::int64_t>(3)});
        cell.arrays[0].values[vp?24:2]=1;plots.plots.push_back(cell);
        const auto projection=project_saved_map(header,plots);
        check(projection.cells[0].force_resource_reveal==std::set<std::int64_t>{2},"Shared map projection restores the exact source force flag column");
        plots.plots[0].arrays[0].values[vp?24:2]=2;rejects([&]{project_saved_map(header,plots);});
    }
}
