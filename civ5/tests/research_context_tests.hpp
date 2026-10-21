#pragma once
void research_context_schema(RulesDatabase &db) {
    db.apply_sql("CREATE TABLE HandicapInfos(ID,Type,ResearchPercent,ResearchPerEraModifier,AIResearchPercent,AIResearchPerEraModifier);"
                 "INSERT INTO HandicapInfos VALUES(0,'H0',100,0,100,0),(1,'H1',80,-10,90,-5),(2,'H2',73,-10,85,-5),"
                 "(3,'H3',90,0,80,-5),(5,'H5',100,0,70,-5),(6,'H6',100,0,50,-20),(8,'H8',100,0,40,-10);"
                 "CREATE TABLE GameSpeeds(ID,Type,ResearchPercent); INSERT INTO GameSpeeds VALUES(0,'S0',67);"
                 "CREATE TABLE Eras(ID,Type,ResearchPercent); INSERT INTO Eras VALUES(0,'E0',143);"
                 "CREATE TABLE Defines(Name,Value); INSERT INTO Defines VALUES('STANDARD_HANDICAP',1),('AI_HANDICAP',8),('TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER',50);");
}
civ5::saves::SavedSessionSnapshot research_snapshot() {
    using namespace civ5::saves;
    SavedSessionSnapshot s;
    const auto layout = GameStateLayout::vp542_corpus;
    s.game_state.layout = layout;
    s.game_state.controls = {{"handicap",6},{"game_era",3}};
    auto &p = s.pregame.emplace();
    p.layout = layout;
    p.archive.integers = {{"game_type",0},{"game_speed",0},{"era",0},{"game_turn",42},
        {"map_random_seed",4294967295LL}};
    p.archive.integer_arrays = {{"team_types",{42,42,42}},{"handicaps",{1,8,3}},
        {"slot_status",{3,3,1}},{"slot_claims",{0,0,0}}};
    p.world.values.integers["research_percent"] = 115;
    p.slots.integer_arrays["handicaps"] = {8,8,8}; // archive must win
    s.teams.emplace().layout = layout;
    TeamObservation team;
    team.index = 0;
    team.fields = {{"id",42},{"members_count",4},{"current_era",6}};
    PlotArrayObservation members;
    members.name = "members"; members.values = {0,1,2,2};
    team.arrays.push_back(members);
    s.teams->teams.push_back(team);
    s.all_players.emplace().layout = layout;
    for (auto id : {2,0,1}) {
        SavedPlayerObservation player;
        player.saved_player_id = id;
        player.index = s.all_players->players.size();
        player.core.layout = layout;
        PlayerCoreField f;
        f.name = "m_eID"; f.encoding = PlayerCoreEncoding::i32; f.values = {id};
        player.core.fields.push_back(f);
        f.name = "m_bAlive"; f.encoding = PlayerCoreEncoding::boolean; f.values = {id != 1};
        player.core.fields.push_back(f);
        f.name = "m_bEverAlive"; f.values = {true};
        player.core.fields.push_back(f);
        s.all_players->players.push_back(player);
    }
    return s;
}
void research_context_tests() {
    using namespace civ5::runtime;
    using namespace civ5::saves;
    const auto vp = TechnologyProfile::vp542, bnw = TechnologyProfile::bnw;
    RulesDatabase db; research_context_schema(db);
    const auto rules = load_research_rules(db, vp), bnw_rules = load_research_rules(db, bnw);
    const auto defs = db.cache_global_defines(GlobalDefineProfile::vp542);
    const auto bdefs = db.cache_global_defines(GlobalDefineProfile::bnw);
    check(rules.all_succeeded() && rules.handicaps.report.slots == 9 && rules.handicaps.entries.size() == 7 &&
          rules.handicaps.find(4) == nullptr && rules.handicaps.find(-1) == nullptr &&
          rules.handicaps.find(2)->per_era_modifier == -10 && bnw_rules.handicaps.find(2)->per_era_modifier == 0,
          "Research Info subset/profile/sparse span differs");
    auto snapshot = research_snapshot();
    auto context = restore_research_context(snapshot, db);
    check(context.players.at(0).handicap == 1 && !context.players.at(1).alive && context.players.at(2).handicap == 3 &&
          context.team_members.at(42) == 4 && context.current_era == 3 && context.world_percent == 115 &&
          context.map_random_seed == UINT32_MAX && !context.pitboss,
          "Research setup binding used indexes, slots preamble, team era, alive count or a DB world default");
    check(context.players.at(1).ever_alive && !context.players.at(1).alive,
          "A defeated Civ V player retains the independently saved ever-alive flag");
    CustomModOptions options;
    TeamTechnologies team(vp,42,1), bteam(bnw,42,1);
    auto bound = bind_team_research(context,rules,defs,&options,team);
    check(bound.handicap_id == 2 && bound.alive_players == 2 && bound.missing_infos.empty() &&
          vp_research_cost_profile(bound.factors).human_for_handicap && bound.factors.team_handicap_percent == 73 &&
          vp_research_cost_profile(bound.factors).game_ai_handicap_percent == 100 && bound.factors.speed_percent == 67 &&
          bound.factors.start_era_percent == 143, "Saved research factors not resolved through alive roster and Info slots");
    TechnologyRules techs; techs.profile = vp; techs.collection.slots = 1;
    techs.entries[0].integers = {{"Cost",1000},{"EurekaPerMillion",100000}};
    check(team_research_cost(techs,team,0,bound.factors) == 1350, "Bound VP cost differs from sequential source arithmetic");
    options.entries["HUMAN_USES_AI_HANDICAP"].value = 1;
    bound = bind_team_research(context,rules,defs,&options,team);
    check(bound.handicap_id == 5 && !vp_research_cost_profile(bound.factors).human_for_handicap &&
          vp_research_cost_profile(bound.factors).game_ai_handicap_percent == 50 &&
          vp_research_cost_profile(bound.factors).game_ai_per_era_modifier == -20 &&
          team_research_cost(techs,team,0,bound.factors) == 520,
          "Human AI override must change both team difficulty average and human-for-handicap predicate");
    options.entries["HUMAN_USES_AI_HANDICAP"].value = 2;
    options.entries["CIV6_EUREKAS"].value = 1;
    bound = bind_team_research(context,rules,defs,&options,team);
    vp_team_set_eureka(team,0,4);
    check(bound.handicap_id == 2 && vp_research_cost_profile(bound.factors).human_for_handicap &&
          vp_research_cost_profile(bound.factors).eurekas_enabled &&
          team_research_cost(techs,team,0,bound.factors) == 1250, "Research option booleans or per-member eureka binding differs");
    context.players.at(0).alive = false;
    bound = bind_team_research(context,rules,defs,&options,team);
    check(bound.handicap_id == 3 && bound.alive_players == 1 &&
          !vp_research_cost_profile(bound.factors).human_for_handicap && bound.factors.members == 4,
          "Roster mutation must affect next query without replacing saved member count");
    context.players.at(2).alive = false;
    bound = bind_team_research(context,rules,defs,&options,team);
    check(bound.handicap_id == 1 && bound.alive_players == 0, "Empty team must use STANDARD_HANDICAP");
    context = restore_research_context(snapshot,db);
    context.players.at(0).handicap = context.players.at(2).handicap = 4;
    context.game_handicap = 999;
    bound = bind_team_research(context,rules,defs,&options,team);
    check(!bound.factors.team_handicap_percent && bound.missing_infos == std::vector<std::string>{"team_handicap"},
          "Absent team handicap skips game AI handicap lookup");
    context.players.at(0).handicap = context.players.at(2).handicap = 2;
    context.players.at(0).slot_status = 1;
    bound = bind_team_research(context,rules,defs,&options,team);
    check(vp_research_cost_profile(bound.factors).game_ai_handicap_percent == 0 &&
          bound.missing_infos == std::vector<std::string>{"game_handicap"},
          "Missing VP game handicap should expose source default Info fallback");
    context.speed = context.start_era = 999;
    bound = bind_team_research(context,rules,defs,&options,team);
    check(bound.factors.speed_percent == 0 && bound.factors.start_era_percent == 0 && bound.missing_infos.size() == 3,
          "Missing VP speed/era uses diagnosed zero-valued source Info");
    context.profile = bnw;
    rejects([&] { bind_team_research(context,bnw_rules,bdefs,nullptr,bteam); });
    context = restore_research_context(snapshot,db); context.profile = bnw;
    bound = bind_team_research(context,bnw_rules,bdefs,nullptr,bteam);
    auto btechs = techs; btechs.profile = bnw;
    check(!bound.factors.profile_extensions.find<VpResearchCostProfile>() &&
          bound.factors.team_handicap_percent.has_value() &&
          team_research_cost(btechs,bteam,0,bound.factors) == 2007, "BNW binding inherited VP rules");
    rejects([&] { bind_team_research(context,rules,defs,&options,team); });
    context.profile = vp;
    rejects([&] { bind_team_research(context,rules,defs,nullptr,team); });
    auto invalid_options = options; invalid_options.errors = 1;
    rejects([&] { bind_team_research(context,rules,defs,&invalid_options,team); });
    auto incomplete = rules; incomplete.speeds.report.error = "failed";
    rejects([&] { bind_team_research(context,incomplete,defs,&options,team); });
    // Wide object and rule IDs remain sparse. Average can exceed int32 and
    // its mathematical sum can exceed int64 without overflowing the result.
    context.team_members.clear(); context.team_members[4294967300LL] = 4294967298LL;
    context.players.clear();
    context.players[4294967301LL] = {4294967300LL,INT64_MAX,1,0,true};
    context.players[4294967302LL] = {4294967300LL,INT64_MAX,1,0,true};
    TeamTechnologies wide_team(vp,4294967300LL,1);
    bound = bind_team_research(context,rules,defs,&options,wide_team);
    check(bound.handicap_id == INT64_MAX && bound.factors.members == 4294967298LL, "Research binding narrowed IDs/count/handicap sum");
    context.players.begin()->second.handicap = INT64_MIN;
    check(bind_team_research(context,rules,defs,&options,wide_team).handicap_id == 0, "Signed average must truncate total toward zero");
#if defined(__SIZEOF_INT128__)
    __extension__ using OracleInteger = __int128;
    std::uint64_t seed = 0xa54ff53a5f1d36f1ULL;
    const auto next = [&]() { seed ^= seed<<13; seed ^= seed>>7; seed ^= seed<<17; return seed; };
    context.players[4294967303LL] = {4294967300LL,0,1,0,true};
    for (unsigned trial=0; trial<700; ++trial) {
        OracleInteger sum = 0;
        for (auto &[id,player] : context.players) { player.handicap = std::bit_cast<std::int64_t>(next()); sum += player.handicap; }
        check(bind_team_research(context,rules,defs,&options,wide_team).handicap_id == sum/3,
              "Wide handicap average differs from independent 128-bit oracle");
    }
#endif
    // Native human classification, including lazy PitBoss default and ordered
    // duplicate string options (first wins, opposite to CustomMods cache).
    snapshot.pregame->archive.integer_arrays["slot_status"][0] = 0;
    rejects([&] { restore_research_context(snapshot,db); }); // required GameOptions table is missing
    snapshot.pregame->archive.integers["game_type"] = 2;
    context = restore_research_context(snapshot,db);
    check(context.is_human(context.players.at(0)) && !context.pitboss, "Hotseat open slot should short circuit DB lookup");
    snapshot.pregame->archive.integers["game_type"] = 0;
    snapshot.pregame->game_options = {{"GAMEOPTION_PITBOSS",0},{"GAMEOPTION_PITBOSS",1}};
    context = restore_research_context(snapshot,db);
    check(!context.is_human(context.players.at(0)), "Pregame string option must take first duplicate");
    snapshot.pregame->game_options[0].name += std::string("\0ignored",8);
    snapshot.pregame->game_options[0].value = -1;
    context = restore_research_context(snapshot,db);
    check(context.is_human(context.players.at(0)), "Pregame option uses C string and nonzero integer semantics");
    snapshot.pregame->game_options.clear();
    db.apply_sql("CREATE TABLE GameOptions(Type,\"Default\"); INSERT INTO GameOptions VALUES('GAMEOPTION_PITBOSS',1);");
    context = restore_research_context(snapshot,db);
    check(context.is_human(context.players.at(0)) && context.game_options.size() == 1 && snapshot.pregame->game_options.empty(),
          "PitBoss default must be cached in owned game options without changing the raw observation");
    db.apply_sql("DELETE FROM GameOptions;");
    context = restore_research_context(snapshot,db);
    check(!context.is_human(context.players.at(0)) && context.game_options.empty(), "Missing PitBoss default is false without invented saved option");
    context.game_type = 3;
    check(context.is_human(context.players.at(0)), "Email open slot should be human");
    for (auto status : {1,2,3,4,99}) for (auto claim : {0,1,2}) {
        ResearchPlayerState player; player.slot_status=status; player.slot_claim=claim;
        check(context.is_human(player) == (status==3 || (status==4 && claim==2)), "Slot/claim human classification differs");
    }
    auto bad = research_snapshot(); bad.pregame.reset(); rejects([&] { restore_research_context(bad,db); });
    bad = research_snapshot(); bad.pregame->archive.integers["map_random_seed"]=-1;
    rejects([&] { restore_research_context(bad,db); });
    bad = research_snapshot(); bad.teams->teams.push_back(bad.teams->teams[0]); rejects([&] { restore_research_context(bad,db); });
    bad = research_snapshot(); bad.all_players->players.push_back(bad.all_players->players[0]); rejects([&] { restore_research_context(bad,db); });
    bad = research_snapshot(); bad.all_players->players[0].core.fields[0].values[0]=0; rejects([&] { restore_research_context(bad,db); });
    bad = research_snapshot(); bad.pregame->archive.integer_arrays["handicaps"].resize(1); rejects([&] { restore_research_context(bad,db); });
    bad = research_snapshot(); bad.all_players->players[0].core.fields[1].values[0]=2; rejects([&] { restore_research_context(bad,db); });
    bad = research_snapshot(); bad.all_players->players[0].core.fields[2].values[0]=2; rejects([&] { restore_research_context(bad,db); });
    bad = research_snapshot(); bad.game_state.controls.push_back({"game_era",4}); rejects([&] { restore_research_context(bad,db); });
    // Info collection duplicates append a slot; missing columns convert to zero.
    db.apply_sql("INSERT INTO GameSpeeds VALUES(0,'DUP','4294967297'),(4294967297,'WIDE',NULL);");
    const auto sparse = load_research_rules(db,vp);
    check(sparse.speeds.entries.size()==3 && sparse.speeds.report.slots==4294967298ULL &&
          sparse.speeds.find(1)->percent==4294967297LL && sparse.speeds.find(4294967297LL)->percent==0,
          "Research Info duplicates, sparse wide IDs or SQLite GetInt conversion changed");
    RulesDatabase missing;
    const auto none = load_research_rules(missing,vp);
    check(!none.all_succeeded() && !none.handicaps.report.error.empty(), "Missing research tables cannot report complete");
    missing.apply_sql("CREATE TABLE HandicapInfos(ID,Type); CREATE TABLE GameSpeeds(ID,Type); CREATE TABLE Eras(ID,Type);"
                      "INSERT INTO HandicapInfos VALUES(0,'H'); INSERT INTO GameSpeeds VALUES(0,'S'); INSERT INTO Eras VALUES(0,'E');");
    const auto zero = load_research_rules(missing,vp);
    check(zero.all_succeeded() && zero.handicaps.find(0)->percent==0 && zero.speeds.find(0)->percent==0,
          "Missing Info numeric columns have native zero defaults");
    rejects([&] { load_research_rules(db,vp,{1}); });
}
