#pragma once
void lua_pregame_tests(){
    using namespace civ5::runtime;
    RulesDatabase db;db.apply_sql("CREATE TABLE Civilizations(ID INTEGER,Type TEXT);"
        "INSERT INTO Civilizations VALUES(5000000001,'CIV_A'),(5000000002,'CIV_B');"
        "CREATE TABLE Leaders(ID INTEGER,Type TEXT); INSERT INTO Leaders VALUES(5000000003,'LEADER_A'),(5000000005,'LEADER_B');"
        "CREATE TABLE PlayerColors(ID INTEGER,Type TEXT); INSERT INTO PlayerColors VALUES(5000000006,'PLAYERCOLOR_A'),(5000000007,'PLAYERCOLOR_B');"
        "CREATE TABLE MinorCivilizations(ID INTEGER,Type TEXT);"
        "INSERT INTO MinorCivilizations VALUES(5000000004,'MINOR_A');");
    civ5::saves::PregameObservation saved{};saved.slots_version=3;saved.archive_version=6;
    saved.archive.integers["active_player"]=128;
    saved.archive.integer_arrays["slot_status"]=std::vector<std::int64_t>(129,1);
    saved.archive.flag_arrays["minor_nation_civs"]=std::vector<bool>(129,false);
    saved.archive.flag_arrays["minor_nation_civs"][128]=true;
    saved.archive.string_arrays["minor_civ_types"]=std::vector<std::string>(129);
    saved.archive.string_arrays["minor_civ_types"][128]=std::string("MINOR_A\0ignored",15);
    saved.archive.string_arrays["player_colors"]=std::vector<std::string>(129);
    saved.archive.string_arrays["player_colors"][128]="PLAYERCOLOR_A";
    saved.slots.integer_arrays["slot_status"]=std::vector<std::int64_t>(129,2);
    saved.slots.integer_arrays["civilizations"]=std::vector<std::int64_t>(129,77);
    auto &keys=saved.slots.string_arrays["civilization_keys"];keys=std::vector<std::string>(129,"CIV_A");
    keys[0]="";keys[1]="MISSING";keys[2]="civ_a";keys[3]=std::string("CIV_B\0ignored",13);
    auto &leader_keys=saved.slots.string_arrays["leader_keys"];
    leader_keys=std::vector<std::string>(129);leader_keys[128]="LEADER_A";
    for(const auto layout:{civ5::saves::GameStateLayout::expansion2_version1,civ5::saves::GameStateLayout::vp542_corpus}){
        saved.layout=layout;const auto roster=restore_pregame_roster(saved,db);
        check(roster.slots.size()==129&&roster.active_player==128&&roster.slots.at(128).civilization==5000000001LL&&
              roster.slots.at(128).leader==5000000003LL&&roster.slots.at(128).leader_key=="LEADER_A"&&roster.slots.at(128).minor&&
              roster.slots.at(128).minor_type==5000000004LL&&
              roster.slots.at(128).player_color==5000000006LL&&roster.slots.at(128).player_color_key=="PLAYERCOLOR_A"&&
              !roster.slots.at(1).minor&&roster.slots.at(1).minor_type==-1&&roster.slots.at(1).leader==-1,
              "Pregame binds source keys to wide current IDs, beyond the original slot capacity");
        check(roster.slots.at(128).status==1&&roster.slots.at(0).civilization==-1&&roster.slots.at(1).civilization==-1&&
              roster.slots.at(2).civilization==-1&&roster.slots.at(3).civilization==5000000002LL,
              "Archive status wins; empty, missing, case-sensitive and C-string keys follow source lookup");
    }
    auto legacy=saved;legacy.slots.string_arrays.at("civilization_keys").clear();
    legacy.slots.string_arrays.at("leader_keys").clear();
    legacy.archive.string_arrays.at("minor_civ_types")[128].clear();
    legacy.archive.string_arrays.at("player_colors").clear();
    RulesDatabase no_schema;check(restore_pregame_roster(legacy,no_schema).slots.at(128).civilization==77,
        "Empty source key list uses numeric indices without requiring a database lookup");
    auto bad=saved;bad.slots.string_arrays.at("civilization_keys").pop_back();rejects([&]{restore_pregame_roster(bad,db);});
    bad=legacy;bad.slots.integer_arrays.at("civilizations").clear();rejects([&]{restore_pregame_roster(bad,db);});
    bad=saved;bad.archive.flag_arrays.at("minor_nation_civs").pop_back();rejects([&]{restore_pregame_roster(bad,db);});
    bad=saved;bad.archive.string_arrays.at("minor_civ_types").pop_back();rejects([&]{restore_pregame_roster(bad,db);});
    bad=saved;bad.archive.string_arrays.at("player_colors").pop_back();rejects([&]{restore_pregame_roster(bad,db);});
    bad=saved;bad.archive_version=7;rejects([&]{restore_pregame_roster(bad,db);});
    rejects([&]{restore_pregame_roster(saved,no_schema);});
    PregameRoster roster;roster.active_player=5000000001LL;
    roster.slots={{0,{3,8}},{2,{1,9}},{5000000001LL,{4,5000000002LL}}};
    auto provider=lua_pregame_services(roster);
    LuaScriptHost host({}, {},{},{},{},provider);
    host.execute("A",R"lua(
        assert(Game.GetActivePlayer()==5000000001)
        assert(PreGame.GetSlotStatus(5000000001)==SlotStatus.SS_OBSERVER)
        assert(PreGame.GetCivilization(5000000001)==5000000002)
        assert(PreGame.GetSlotStatus(-1)==2 and PreGame.GetCivilization(-1)==-1)
        assert(PreGame.GetSlotStatus(999)==2 and PreGame.GetCivilization(999)==-1)
        assert(PreGame.GetSlotStatus()==3 and PreGame.GetCivilization('not a number')==8)
        assert(PreGame.GetCivilization('2')==9 and PreGame.GetCivilization(1.6)==9)
        assert(PreGame.GetPlayerColor(5000000001)==-1)
        assert(select('#',PreGame.GetCivilization(0,4))==1)
        assert(SlotStatus.SS_OPEN==0 and SlotStatus.SS_COMPUTER==1 and SlotStatus.SS_CLOSED==2
            and SlotStatus.SS_TAKEN==3 and SlotStatus.SS_MAX_SLOT_STATUS==5)
        assert(not pcall(PreGame.GetSlotStatus,9007199254740994))
        SlotStatus.SS_TAKEN=99
    )lua","=pregame-api");
    roster.active_player=-1;roster.slots.at(5000000001LL)={3,5000000003LL};
    host.execute("B","assert(Game.GetActivePlayer()==-1 and SlotStatus.SS_TAKEN==3)","=pregame-second-context");
    host.execute("A","assert(Game.GetActivePlayer()==-1 and PreGame.GetSlotStatus(5000000001)==3 and PreGame.GetCivilization(5000000001)==5000000003)","=pregame-live-state");
    host.complete_loading();check(host.loading_complete(),"Contexts must share live roster while retaining local enum tables");
    roster.slots.at(5000000001LL).leader=5000000003LL;
    ResearchContext world;world.players[5000000001LL].team=0;
    std::vector<TeamTechnologies> teams;const auto types=db.cache_info_types();
    LuaScriptHost mutable_host({}, {},{},lua_gameplay_services(db,world,teams,types,nullptr,&roster),{},
        lua_pregame_services(roster,db));
    mutable_host.execute("mutator",R"lua(
        assert(Players[5000000001]:GetLeaderType()==5000000003)
        assert(select('#',PreGame.SetLeaderType(5000000001,5000000005))==0)
        assert(Players[5000000001]:GetLeaderType()==5000000005)
        assert(select('#',PreGame.SetPlayerColor(5000000001,5000000007))==0)
        assert(PreGame.GetPlayerColor(5000000001)==5000000007)
    )lua","=pregame-leader-mutation");
    check(roster.slots.at(5000000001LL).leader_key=="LEADER_B",
          "Lua leader mutation retains its stable active-rules key in the shared roster");
    check(roster.slots.at(5000000001LL).player_color_key=="PLAYERCOLOR_B",
          "Lua player color mutation retains its stable active-rules key in the shared roster");
    mutable_host.execute("mutator","PreGame.SetLeaderType(999999,5000000003); PreGame.SetLeaderType(5000000001,999999); assert(Players[5000000001]:GetLeaderType()==-1)",
        "=pregame-invalid-leader");
    check(roster.slots.at(5000000001LL).leader_key.empty(),
          "Unknown leader ID resets shared pregame selection like native bindLeaderKeys");
    mutable_host.execute("mutator","PreGame.SetPlayerColor(999999,5000000006); PreGame.SetPlayerColor(5000000001,999999); assert(PreGame.GetPlayerColor(5000000001)==-1)",
        "=pregame-invalid-color");
    check(roster.slots.at(5000000001LL).player_color_key.empty(),
          "Unknown player color resets its saved key without changing another slot");
    mutable_host.complete_loading();check(mutable_host.loading_complete(),
        "Pregame leader updates reach the same roster read by gameplay Lua");
    LuaScriptHost absent;absent.execute("no-setup","assert(PreGame==nil and SlotStatus==nil and Game==nil)","=missing-setup");
    absent.complete_loading();check(absent.loading_complete(),"Missing pregame provider does not synthesize setup");
    auto partial=provider;partial.civilization={};rejects([&]{LuaScriptHost invalid({}, {},{},{},{},partial);});
    auto failing=provider;failing.slot_status=[](std::int64_t)->std::int64_t{throw std::runtime_error("lost setup");};
    LuaScriptHost failure({}, {},{},{},{},failing);
    failure.execute("failed","assert(not pcall(PreGame.GetSlotStatus,0))","=provider-failure");rejects([&]{failure.complete_loading();});
    roster.active_player=INT64_MAX;
    LuaScriptHost wide_error({}, {},{},{},{},provider);
    wide_error.execute("failed","assert(not pcall(Game.GetActivePlayer))","=inexact-id");rejects([&]{wide_error.complete_loading();});
}
void lua_pregame_asset_tests(const std::filesystem::path &path){
    using namespace civ5::runtime;
    std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("Cannot read original Inca script");
    const std::string source{std::istreambuf_iterator<char>(file),{}};
    if(file.bad())throw std::runtime_error("Original Inca script read failed");
    RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name TEXT,Value INTEGER);INSERT INTO Defines VALUES('MAX_MAJOR_CIVS',4);"
        "CREATE TABLE Civilizations(ID INTEGER,Type TEXT);INSERT INTO Civilizations VALUES(5000000001,'CIVILIZATION_INCA');"
        "CREATE TABLE Improvements(ID INTEGER,Type TEXT);INSERT INTO Improvements VALUES(5000000002,'IMPROVEMENT_MOUNTAIN_CITY');");
    ResearchContext players;players.players[0].team=0;players.game_turn=88;
    std::vector<TeamTechnologies> teams;const auto types=db.cache_info_types();
    PregameRoster roster;roster.active_player=0;roster.slots={{0,{3,7}},{1,{2,5000000001LL}},{2,{4,5000000001LL}},{3,{0,5000000001LL}}};
    for(const auto status:{0,1,3}){
        roster.slots.at(3).status=status;
        LuaScriptHost host({}, {},{},lua_gameplay_services(db,players,teams,types),{},lua_pregame_services(roster));
        host.execute("inca",source,"@CBP_IncaFunctions.lua");
        host.execute("inca",std::string("assert(GameEvents.PlayerCityFounded.Count()==")+(status==0?"0":"1")+
            "); assert(JFD_IsCivilisationActive(5000000001)=="+(status==0?"false":"true")+
            "); assert(not JFD_IsCivilisationActive(999)); assert(Game.GetGameTurn()==88)","=inca-registration");
        host.complete_loading();check(host.loading_complete(),"Original Inca initialization must select computer/taken slots, excluding closed/observer/open");
    }
}
