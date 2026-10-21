#pragma once
void vp_unit_trait_tests(){
    using namespace civ5::runtime;using namespace civ5::saves;constexpr auto vp=TechnologyProfile::vp542;
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);CREATE TABLE Technologies(ID,Type);INSERT INTO Technologies VALUES(1,'T1'),(2,'T2');"
        "CREATE TABLE Beliefs(ID,Type);INSERT INTO Beliefs VALUES(1,'B1'),(2,'B2');CREATE TABLE Policies(ID,Type);INSERT INTO Policies VALUES(1,'P1'),(2,'P2');"
        "CREATE TABLE Traits(ID,Type,ObsoleteTech,PrereqTech,ObsoleteBelief,PrereqBelief,ObsoletePolicy,PrereqPolicy);"
        "INSERT INTO Traits VALUES(0,'TRAIT0',NULL,NULL,NULL,NULL,NULL,NULL),(1,'TRAIT1','T2','T1','B2','B1','P2','P1'),(4,'TRAIT4',NULL,NULL,NULL,NULL,NULL,NULL);"
        "CREATE TABLE Leaders(ID,Type);INSERT INTO Leaders VALUES(0,'L0'),(1,'L1');CREATE TABLE Leader_Traits(LeaderType,TraitType);"
        "INSERT INTO Leader_Traits VALUES('L0','TRAIT4'),('L0','TRAIT1'),('L0','TRAIT1'),('L1','TRAIT0');");
    const auto types=db.cache_info_types();const auto rules=load_vp_unit_trait_rules(db,types);
    check(rules.trait_slots==5&&rules.leader_slots==2&&rules.traits.at(1).obsolete_tech==2&&rules.traits.at(1).prereq_belief==1,
        "VP trait production loads active gates and sparse trait slots");
    check(rules.leader_traits.at("L0")==std::set<std::int64_t>{1,4}&&rules.leader_traits.at("L1")==std::set<std::int64_t>{0},
        "Leader traits use active relation, de-duplicate rows and retain trait ID order");
    ResearchContext players;players.profile=vp;auto &p=players.players[5];p.alive=true;p.team=7;
    players.players[63].alive=true;ResearchTurnState turn;turn.profile=vp;turn.players[5].leader_type="L0";turn.players[63]={};
    PlayerResearchContext dip;dip.profile=vp;dip.major_players={5};restore_vp_unit_traits(rules,players,turn,dip);
    check(vp_potential_traits(players,5)==std::vector<std::int64_t>{1,4}&&vp_potential_traits(players,63).empty(),
        "Only an alive major has the source-ordered potential trait cache; barbarian does not");
    auto copied_players=players;ensure_vp_unit_trait_profile(copied_players).potential_by_player[5].clear();
    check(vp_potential_traits(players,5)==std::vector<std::int64_t>{1,4}&&vp_potential_traits(copied_players,5).empty(),
        "Copying the shared roster copies VP trait state independently");
    CityYieldContext cities;cities.profile=vp;vp_player_state_religion(cities.players[5])=2;vp_player_state_religion(cities.players[63])=-1;
    CityYieldReligionBeliefs owned;owned.religion=2;owned.founder=5;ensure_vp_religion_beliefs(owned,vp);vp_religion_beliefs(owned).lookup[1]=1;cities.religions={owned};
    SavedMapProjection map{{1,1}};map.cells.resize(1);map.cells[0].index=0;PlotPassability pass;PlotWaterRules wr;wr.profile=vp;
    PlotWaterState ws;ws.layout=GameStateLayout::vp542_corpus;const auto defines=db.cache_global_defines(GlobalDefineProfile::vp542);
    CustomModOptions options;options.entries["BALANCE_VP"].value=1;PlotWaterContext water{map,ws,wr,types,pass,players,cities,defines,&options};
    VpPlotReligionRules rr;VpPlotReligionContext religion{rr,water};TreasuryContext treasury;treasury.profile=vp;
    ensure_vp_treasury_religion_profile(treasury.players[5])={2,true};ensure_vp_treasury_religion_profile(treasury.players[63]).state_religion=-1;
    VpCityReligionMinorContext relation{religion,treasury};std::vector<TeamTechnologies> teams;teams.emplace_back(vp,7,3);
    VpUnitTraitContext c{rules,players,turn,dip,teams,relation};
    check(vp_potential_unit_traits(c,5)==std::vector<std::int64_t>{1,4}&&vp_unit_trait_active(c,5,4),
        "Potential trait positions are independent of sparse trait IDs");
    teams.clear();check(vp_unit_trait_active(c,5,4),"Trait without a technology gate does not require a team-tech lookup");
    teams.emplace_back(vp,7,3);
    check(!vp_unit_trait_active(c,5,1)&&vp_unit_trait_player_has_belief(c,5,1),"Prerequisite gates read owned belief lookup and live policy/tech");
    cities.religions[0].extensions={};rejects([&]{vp_unit_trait_player_has_belief(c,5,1);});cities.religions={owned};
    p.policies.insert(1);teams[0].set_has_tech(1,true);check(vp_unit_trait_active(c,5,1),"Trait activates when policy, belief and technology are present");
    ensure_vp_policy_branch_profile(players).players[5]={3};check(vp_unit_trait_active(c,5,1),"HasPolicy ignores blocked branches as in source HasTrait");
    p.policies.insert(2);check(!vp_unit_trait_active(c,5,1),"Obsolete policy suppresses trait");p.policies.erase(2);
    vp_religion_beliefs(owned).lookup[2]=1;cities.religions={owned};check(!vp_unit_trait_active(c,5,1),"Obsolete belief suppresses trait");
    vp_religion_beliefs(owned).lookup.erase(2);cities.religions={owned};teams[0].set_has_tech(2,true);check(!vp_unit_trait_active(c,5,1),"Obsolete technology suppresses trait");
    teams[0].set_has_tech(2,false);check(vp_unit_trait_active(c,5,1),"Removing obsolescence restores current trait result");
    ensure_vp_treasury_religion_profile(treasury.players[5]).owns_state_religion=false;check(!vp_unit_trait_active(c,5,1),"Without owned or created religion, prerequisite belief is absent");
    auto pantheon=owned;pantheon.religion=0;vp_religion_beliefs(pantheon).pantheon=true;vp_religion_beliefs(pantheon).holy_x=vp_religion_beliefs(pantheon).holy_y=-1;cities.religions={pantheon};
    check(vp_unit_trait_player_has_belief(c,5,1)&&vp_unit_trait_active(c,5,1),"HasBelief falls back to a created personal pantheon");
    cities.religions={owned};ensure_vp_treasury_religion_profile(treasury.players[5]).owns_state_religion=true;p.alive=false;
    check(vp_unit_trait_active(c,5,1),"Potential trait cache remains until an explicit Init/Read rebuild");
    rebuild_vp_unit_traits(rules,players,turn,dip,5);check(vp_potential_traits(players,5).empty()&&!vp_unit_trait_active(c,5,1),"Rebuild removes dead leader traits");
    p.alive=true;turn.players[5].leader_type="L1";rebuild_vp_unit_traits(rules,players,turn,dip,5);
    check(vp_potential_traits(players,5)==std::vector<std::int64_t>{0}&&vp_unit_trait_active(c,5,0),"Leader change takes effect on trait rebuild");
    turn.players[5].leader_type="MISSING";rejects([&]{rebuild_vp_unit_traits(rules,players,turn,dip,5);});
    check(vp_potential_traits(players,5)==std::vector<std::int64_t>{0},"Failed VP trait rebuild preserves previous cache");
    turn.players[5].leader_type="L0";rebuild_vp_unit_traits(rules,players,turn,dip,5);
    rejects([&]{vp_unit_trait_active(c,5,99);});ensure_vp_unit_trait_profile(players).potential_by_player.erase(5);rejects([&]{vp_potential_unit_traits(c,5);});
    ResearchContext vanilla;vanilla.players[5].team=9;
    check(!find_vp_unit_trait_profile(vanilla),"BNW research roster has no VP trait cache");
    rejects([&]{ensure_vp_unit_trait_profile(vanilla);});
}
