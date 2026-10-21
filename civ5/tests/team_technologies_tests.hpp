#pragma once
using civ5::runtime::TeamTechnologies;
using civ5::runtime::TeamTechnologyValue;
using civ5::runtime::restore_team_technologies;
civ5::saves::TeamObservation saved_team_techs(TechnologyProfile profile, std::vector<std::int64_t> hashes) {
    civ5::saves::TeamObservation saved;
    saved.index = 4294967400ULL;
    saved.fields = {{"tech_version", 1}, {"last_tech", -1}, {"saved_techs", static_cast<std::int64_t>(hashes.size())}};
    saved.fields.push_back({"id",4294967500LL}); // Deliberately different from saved.index.
    const bool vp = profile == TechnologyProfile::vp542;
    if (vp) {
        saved.fields.push_back({"tech_mod_version",151}); saved.fields.push_back({"tech_sentinel",0xdeadbeef});
        saved.fields.push_back({"num_techs",0});
    }
    saved.arrays.push_back({"tech_hashes",1,hashes});
    for (const auto *name : {"has_tech", "no_trade_tech", vp ? "research_progress_times100" : "research_progress", "tech_count"})
        saved.arrays.push_back({name,1,std::vector<std::int64_t>(hashes.size())});
    if (vp) saved.arrays.push_back({"eureka_counter",1,std::vector<std::int64_t>(hashes.size())});
    return saved;
}
void team_technologies_tests() {
    const auto bnw = TechnologyProfile::bnw, vp = TechnologyProfile::vp542;
    const auto h = [](const char *type) -> std::int64_t { return saved_info_type_hash(type); };
    RulesDatabase db;
    technology_schema(db);
    db.apply_sql("INSERT INTO Technologies(ID,Type) VALUES(0,'A'),(2,'B');"
                 "CREATE TABLE OtherTypes(ID,Type); INSERT INTO OtherTypes VALUES(1,'HOLE'),(7,'OUTSIDE'),(-2,'NEGATIVE'),(-1,'NONE');");
    const auto types = db.cache_info_types();
    for (const auto profile : {bnw,vp}) {
        const auto rules = load_technology_rules(db, types, profile, 5);
        auto saved = saved_team_techs(profile, {h("B"), h("A"), h("A"), 0, h("MISSING"), h("HOLE"), h("OUTSIDE"), h("NEGATIVE"), h("NONE")});
        saved.fields[1].value = 2;
        if (profile == vp) saved.fields.back().value = 4294967321LL;
        saved.arrays[1].values = {0,1,0,1,1,1,1,1,1};
        saved.arrays[2].values = {1,1,0,1,1,0,1,1,1};
        saved.arrays[3].values = {4294967401LL,999,-199,888,777,456,333,222,111};
        saved.arrays[4].values = {2,9,0,7,6,3,5,4,3};
        if (profile == vp) saved.arrays[5].values = {4294967307LL,8,0,6,5,4,3,2,1};
        auto state = restore_team_technologies(saved, rules, types);
        const auto &r = state.remap();
        check(state.team() == 4294967500LL && state.slots() == 3 && state.last_tech() == 2 &&
              r.saved == 9 && r.mapped == 4 && r.duplicates == 1 && r.sentinels == 1 &&
              r.unknown == 2 && r.outside == 2 && r.holes == 1 && !r.all_mapped() && !r.last_tech_outside,
              "Team tech hash remap diagnostics or raw last-tech ID changed");
        check(!state.has_tech(0) && !state.at(0).no_trade && state.progress(0) == -1 && state.at(0).count == 0 &&
              state.has_tech(1) && !state.has_tech(2) && state.at(2).no_trade && state.at(2).count == 2 &&
              state.progress_times100(2) == 4294967401LL && state.progress(2) == 42949674LL && state.occupied_slots() == 3,
              "Remap lost source units, wide progress, holes or last-duplicate defaults");
        check(state.has_tech(-1) && state.progress(-1) == 0 && state.progress_times100(-1) == 0,
              "Team tech NO_TECH getters changed");
        check(state.known_count() == (profile == vp ? 4294967321LL : 1) && state.all_researched() == (profile == vp),
              "VP stored known counter replaced by flag count or BNW counted repeat count as ownership");
        unsigned events = 0;
        const auto hook = [&](std::int64_t team, std::int64_t tech, bool owned) {
            ++events;
            check(team == state.team() && tech == 0 && state.has_tech(tech) == owned && state.last_tech() == 0,
                  "TeamSetHasTech hook fired before state update or with wrong wide IDs");
        };
        check(!state.set_has_tech(0,false,hook) && events == 0, "Unchanged HasTech fired hook");
        check(state.set_has_tech(0,true,hook) && events == 1 && state.all_researched(), "Changed flag or BNW repeat completion lost");
        check(state.set_has_tech(0,false,hook) && events == 2 && state.last_tech() == 0, "Removing technology reset last-tech");
        rejects([&] { state.set_has_tech(0,true, [](auto,auto,auto) { throw std::runtime_error("Hook failed"); }); });
        check(state.has_tech(0) && state.last_tech() == 0, "Hook failure rolled back source mutation");
        if (profile == vp) {
            check(vp_team_eureka(state,-1) == 0 && vp_team_eureka(state,2) == 4294967307LL && state.known_count() == 4294967321LL,
                  "VP eureka narrowed or SetHasTech mutated separate known counter");
            vp_team_set_eureka(state,-1, 999); vp_team_set_eureka(state,0,INT64_MIN);
            check(vp_team_eureka(state,0) == INT64_MIN, "VP eureka introduced clamp");
            vp_team_change_known_count(state,-4294967322LL);
            check(state.known_count() == -1 && !state.all_researched(), "Negative VP counter clamped or compared as unsigned");
        } else {
            rejects([&] { (void)vp_team_eureka(state,0); }); rejects([&] { vp_team_set_eureka(state,0,1); });
            rejects([&] { vp_team_change_known_count(state,0); });
        }
        state.set_no_trade(0,true); state.increment_count(0); state.set_last_tech(2);
        check(state.at(0).no_trade && state.at(0).count == 1 && state.last_tech() == 2, "Low-level team tech mutation failed");
        rejects([&] { (void)state.has_tech(-2); }); rejects([&] { (void)state.at(-1); });
        rejects([&] { (void)state.at(3); }); rejects([&] { state.set_has_tech(-1,true); });
        rejects([&] { state.set_last_tech(-1); }); rejects([&] { state.increment_count(3); });
        rejects([&] { restore_team_technologies(saved, rules, types,8); });
        auto bad = saved; bad.fields[0].value = 9; rejects([&] { restore_team_technologies(bad,rules,types); });
        bad = saved; bad.fields[2].value = -1; rejects([&] { restore_team_technologies(bad,rules,types); });
        bad = saved; bad.fields.push_back(saved.fields[1]); rejects([&] { restore_team_technologies(bad,rules,types); });
        bad = saved; bad.fields.erase(bad.fields.begin()); rejects([&] { restore_team_technologies(bad,rules,types); });
        bad = saved; bad.arrays.pop_back(); rejects([&] { restore_team_technologies(bad,rules,types); });
        bad = saved; bad.arrays.push_back(saved.arrays[0]); rejects([&] { restore_team_technologies(bad,rules,types); });
        bad = saved; bad.arrays[0].columns = 2; rejects([&] { restore_team_technologies(bad,rules,types); });
        bad = saved; bad.arrays[0].values.pop_back(); rejects([&] { restore_team_technologies(bad,rules,types); });
        for (const auto invalid : {-1LL, 4294967296LL}) {
            bad = saved; bad.arrays[0].values[0] = invalid; rejects([&] { restore_team_technologies(bad,rules,types); });
        }
        for (const auto which : {1,2}) {
            bad = saved; bad.arrays[which].values[3] = 2; rejects([&] { restore_team_technologies(bad,rules,types); });
        }
        bad = saved; bad.fields[1].value = 99;
        const auto raw = restore_team_technologies(bad,rules,types);
        check(raw.last_tech() == 99 && raw.remap().last_tech_outside, "Raw invalid last-tech was remapped or silently repaired");
        auto zero = saved_team_techs(profile,{});
        auto empty = restore_team_technologies(zero,rules,types,0);
        check(empty.remap().all_mapped() && empty.occupied_slots() == 0 && empty.known_count() == 0 &&
              !empty.all_researched() && empty.at(2) == TeamTechnologyValue{}, "Empty save failed reset defaults");
        civ5::saves::TeamsObservation teams;
        teams.layout = profile == vp ? civ5::saves::GameStateLayout::vp542_corpus : civ5::saves::GameStateLayout::expansion2_version1;
        teams.teams = {saved,saved}; teams.teams.back().index++;
        teams.teams.back().fields[3].value = -1;
        const auto all = restore_team_technologies(teams,rules,types,18);
        check(all.size() == 2 && all[0].team() == 4294967500LL && all[1].team() == -1,
              "Session team restore lost raw IDs or used stream position as owning team ID");
        rejects([&] { restore_team_technologies(teams,rules,types,17); });
        teams.teams.back().index = saved.index; rejects([&] { restore_team_technologies(teams,rules,types); });
        teams.teams.clear(); teams.layout = static_cast<civ5::saves::GameStateLayout>(99);
        rejects([&] { restore_team_technologies(teams,rules,types); });
        auto partial = rules; partial.errors = 1; rejects([&] { restore_team_technologies(saved,partial,types); });
        auto overwritten = saved_team_techs(profile,{h("A"),h("A")});
        overwritten.arrays[1].values[0] = 1; overwritten.arrays[3].values[0] = INT64_MAX;
        if (profile == vp) overwritten.arrays[5].values[0] = 7;
        auto defaults = restore_team_technologies(overwritten,rules,types);
        check(defaults.remap().duplicates == 1 && defaults.occupied_slots() == 0 && defaults.at(0) == TeamTechnologyValue{},
              "Default duplicate did not erase earlier imported value");
        teams.layout = profile == vp ? civ5::saves::GameStateLayout::vp542_corpus : civ5::saves::GameStateLayout::expansion2_version1;
        teams.teams.assign(129,zero);
        for (std::size_t i = 0; i < teams.teams.size(); ++i) teams.teams[i].index = i;
        check(restore_team_technologies(teams,rules,types,129).size() == 129, "Team restore retained original team-count ceiling");
        rejects([&] { restore_team_technologies(teams,rules,types,128); });
        teams.teams.clear(); rejects([&] { restore_team_technologies(teams,partial,types); });
        if (profile == vp) {
            bad = saved; bad.fields[4].value = 150; rejects([&] { restore_team_technologies(bad,rules,types); });
            bad = saved; bad.fields[5].value = 0; rejects([&] { restore_team_technologies(bad,rules,types); });
        }
    }
    {
        RulesDatabase wide_db; technology_schema(wide_db);
        wide_db.apply_sql("INSERT INTO Technologies(ID,Type) VALUES(4294967301,'WIDE'),(9223372036854775807,'LAST');");
        const auto wide_types = wide_db.cache_info_types();
        auto wide_rules = load_technology_rules(wide_db,wide_types,vp,5);
        auto saved = saved_team_techs(vp,{h("WIDE"),h("LAST")});
        saved.arrays[1].values = {1,1}; saved.arrays[3].values = {INT64_MIN,INT64_MAX};
        saved.arrays[4].values = {INT64_MAX,INT64_MIN};
        saved.fields.back().value = INT64_MAX;
        auto state = restore_team_technologies(saved,wide_rules,wide_types);
        check(state.remap().all_mapped() && state.slots() == (1ULL<<63) && state.occupied_slots() == 2 &&
              state.has_tech(4294967301LL) && state.has_tech(INT64_MAX) && !state.has_tech(0) &&
              state.progress_times100(4294967301LL) == INT64_MIN && state.progress_times100(INT64_MAX) == INT64_MAX &&
              !state.all_researched(), "Wide technology slots allocate MAX(ID), narrow or wrap");
        rejects([&] { state.increment_count(4294967301LL); });
        check(state.at(4294967301LL).count == INT64_MAX, "Counter overflow changed state");
        rejects([&] { vp_team_change_known_count(state,1); });
        check(state.known_count() == INT64_MAX, "Known overflow changed state");
        vp_team_change_known_count(state,-INT64_MAX); vp_team_change_known_count(state,INT64_MIN);
        rejects([&] { vp_team_change_known_count(state,-1); });
        check(state.known_count() == INT64_MIN, "Known underflow changed state");
        state.increment_count(INT64_MAX); state.set_has_tech(INT64_MAX,false); state.set_has_tech(INT64_MAX,true);
        check(state.at(INT64_MAX).count == INT64_MIN+1 && state.last_tech() == INT64_MAX,
              "Wide mutation inherited x86 ID or counter ceiling");
    }
    // Independent dense-array oracle: apply the original loop to a small span,
    // including duplicate hashes, holes, sentinels and skipped unknown IDs.
    std::uint64_t seed = 0x6a09e667f3bcc909ULL;
    const auto next = [&]() { seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17; return seed; };
    for (const auto profile : {bnw,vp}) {
        const auto rules = load_technology_rules(db,types,profile,5);
        const std::array<std::int64_t,6> hashes{h("A"),h("B"),h("HOLE"),0,h("MISSING"),h("OUTSIDE")};
        const std::array<int,6> indices{0,2,1,-1,-1,-1};
        for (unsigned trial = 0; trial < 64; ++trial) {
            auto saved = saved_team_techs(profile,std::vector<std::int64_t>(128));
            std::array<TeamTechnologyValue,3> expected{};
            std::array<std::int64_t,3> expected_eureka{};
            for (std::size_t i = 0; i < 128; ++i) {
                const auto choice = next() % hashes.size(); saved.arrays[0].values[i] = hashes[choice];
                TeamTechnologyValue v{bool(next()%2),bool(next()%2),static_cast<std::int64_t>(next()%10000000000ULL)-5000000000LL,
                    static_cast<std::int64_t>(next()%5)-2};
                const auto eureka = profile == vp ? static_cast<std::int64_t>(next()%10000000000ULL) : 0;
                saved.arrays[1].values[i] = v.has_tech; saved.arrays[2].values[i] = v.no_trade;
                saved.arrays[3].values[i] = v.progress_times100; saved.arrays[4].values[i] = v.count;
                if (profile == vp) saved.arrays[5].values[i] = eureka;
                if (indices[choice] >= 0) {
                    expected[indices[choice]] = v;
                    expected_eureka[indices[choice]] = eureka;
                }
            }
            auto state = restore_team_technologies(saved,rules,types);
            for (std::int64_t i = 0; i < 3; ++i) {
                check(state.at(i) == expected[i], "Sparse remap diverged from independent dense oracle");
                if (profile == vp)
                    check(vp_team_eureka(state,i) == expected_eureka[i], "VP eureka remap diverged from dense oracle");
            }
        }
    }
    {
        TeamTechnologies ordinary(bnw,0,4), vp_state(vp,0,4);
        vp_team_set_eureka(vp_state,2,4294967307LL);
        check(ordinary.occupied_indices().empty() && vp_state.nondefault_values().empty() &&
              vp_state.occupied_indices() == std::vector<std::uint64_t>{2} &&
              vp_state.occupied_slots() == 1 && vp_team_eureka(vp_state,2) == 4294967307LL,
              "VP-only eureka leaked into shared technology values or disappeared from diagnostics");
        auto copy = vp_state; vp_team_set_eureka(copy,2,0);
        check(copy.occupied_slots() == 0 && vp_team_eureka(vp_state,2) == 4294967307LL,
              "Copied VP extension shared mutable eureka state");
    }
    TeamTechnologies empty(bnw,0,0), sparse(vp,0,UINT64_MAX);
    check(empty.all_researched() && empty.has_tech(-1) && !sparse.all_researched(), "Zero or unsigned-wide span failed");
    sparse.set_no_trade(0,true); sparse.set_no_trade(0,false); sparse.set_has_tech(0,true); sparse.set_has_tech(0,false);
    check(sparse.occupied_slots() == 0 && sparse.last_tech() == 0, "Sparse default restoration retained allocation or lost last-tech");
    rejects([] { TeamTechnologies invalid(static_cast<TechnologyProfile>(99),0,0); });
}
