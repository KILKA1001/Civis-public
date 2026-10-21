#pragma once
void game_leagues_tests() {
    using saves::GameStateLayout;
    struct Fixture {
        std::string bytes;
        std::vector<std::size_t> flags, versions, counts;
        std::size_t production_count = 0;
    };
    const auto fixture = [](bool vp, unsigned lv = 14, unsigned ev = 9, unsigned gv = 5, unsigned rv = 2,
                            unsigned av = 2) {
        Fixture f;
        const auto num = [&](std::uint32_t v) { u32(f.bytes, v); };
        const auto ver = [&](unsigned v) {
            if (!vp) {
                f.versions.push_back(f.bytes.size());
                num(v);
            }
        };
        const auto count = [&](unsigned v) {
            f.counts.push_back(f.bytes.size());
            num(v);
        };
        const auto flag = [&] {
            f.flags.push_back(f.bytes.size());
            f.bytes += char(1);
        };
        const auto vote = [&] {
            ver(1);
            num(70000);
            num(0x80000000u);
            num(0xffffffffu);
        };
        const auto decision = [&](bool proposer) {
            ver(1);
            num(123);
            ver(1);
            if (!proposer)
                count(2);
            vote();
            if (!proposer)
                vote();
        };
        const auto effects = [&] {
            ver(ev);
            flag();
            if (vp || ev >= 2)
                flag();
            num(101);
            num(102);
            flag();
            if (vp || ev >= 3)
                num(103);
            num(104);
            num(105);
            flag();
            flag();
            flag();
            num(106);
            num(107);
            if (vp || ev >= 4) {
                num(108);
                num(109);
            }
            if (vp || ev >= 5)
                flag();
            if (vp || ev >= 6) {
                num(110);
                num(111);
            }
            if (vp || ev >= 9)
                num(112);
            if (vp || ev >= 6) {
                num(113);
                num(114);
            }
            if (vp || ev >= 7) {
                num(115);
                num(116);
            }
            if (vp || ev >= 8) {
                num(117);
                num(118);
            }
            if (vp) {
                flag();
                flag();
                flag();
                num(119);
                num(120);
                num(0x80000000u);
                flag();
                num(122);
                num(123);
                flag();
            }
        };
        const auto resolution = [&](unsigned kind) {
            ver(rv);
            if (vp || rv >= 2)
                num(0x7fffffffu);
            num(90000);
            num(70000);
            effects();
            decision(false);
            decision(true);
            if (kind != 2) {
                ver(1);
                num(70000);
            }
            ver(kind == 2 ? av : 1);
            if (kind == 2) {
                if (!vp && av < 2)
                    num(0xfedcba98u);
                num(0xffffffffu);
            } else if (kind == 1) {
                num(0x7fffffffu);
                decision(false);
            }
        };
        const auto resolutions = [&](unsigned kind) {
            count(1);
            resolution(kind);
        };
        ver(gv);
        if (vp || gv >= 4)
            num(0x7fffffffu);
        count(1);
        ver(lv);
        num(70000);
        if (vp || lv >= 4)
            flag();
        if (vp || lv >= 2)
            flag();
        if (vp)
            flag();
        num(0x80000000u);
        num(0x7fffffffu);
        resolutions(0);
        resolutions(1);
        resolutions(2);
        count(2);
        for (unsigned i = 0; i < 2; ++i) {
            num(70000);
            if (vp || lv >= 9)
                num(0xffffffffu);
            if (vp || lv >= 10)
                string(f.bytes, std::string("a\0\xff", 3));
            flag();
            if (vp || lv >= 12)
                num(65537);
            num(0x80000000u);
            if (vp || lv >= 13)
                num(0xffffffffu);
            if (vp || lv >= 14) {
                flag();
                flag();
            }
        }
        if (vp || lv >= 3)
            num(70000);
        if (vp || lv >= 5) {
            count(1);
            num(90000);
            f.production_count = f.bytes.size();
            count(3);
            num(0x80000000u);
            num(0xffffffffu);
            num(0x7fffffffu);
            if (vp || lv >= 6)
                flag();
            if (vp || lv >= 11)
                flag();
        }
        if (vp || lv >= 7) {
            num(65537);
            num(90000);
            f.bytes.append(128, char(0xff));
        }
        if (vp)
            num(0x80000000u);
        if (vp || lv >= 8) {
            num(90000);
            num(0xffffffffu);
            resolutions(0);
            resolutions(1);
        }
        if (vp) {
            resolutions(0);
            resolutions(1);
        }
        if (vp || gv >= 2)
            num(65537);
        if (vp || gv >= 3)
            num(70000);
        if (vp || gv >= 5)
            num(90000);
        return f;
    };
    const auto observe = [](const std::string &bytes, bool vp, saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader reader(stream, bytes.size());
        return saves::observe_game_leagues(
            reader, vp ? GameStateLayout::vp542_corpus : GameStateLayout::expansion2_version1, budget);
    };
    const auto replace32 = [](std::string &bytes, std::size_t offset, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i)
            bytes[offset + i] = char(value >> (8 * i));
    };
    const auto effect = [](const auto &r, std::string_view name) {
        for (const auto &v : r.effects.integers)
            if (v.name == name)
                return v.value;
        throw std::runtime_error("Missing named effect");
    };
    for (bool vp : {false, true}) {
        const auto f = fixture(vp);
        const auto s = observe(f.bytes, vp);
        check(s.start_offset == 0 && s.end_offset == f.bytes.size() && s.generated_id_count == INT32_MAX &&
                  s.leagues.size() == 1 && s.leagues_ever_founded == 65537 && s.diplomatic_victor == 70000 &&
                  s.last_era_trigger == 90000,
              "League system state or boundaries changed");
        const auto &l = s.leagues[0];
        check(l.id == 70000 && l.united_nations && l.in_session &&
                  (vp ? l.host_proposing == true : !l.host_proposing) && l.turns_until_session == INT32_MIN &&
                  l.resolutions_ever_enacted == INT32_MAX && l.host == 70000 &&
                  l.consecutive_hosted_sessions == 65537 && l.assigned_name == 90000 &&
                  l.custom_name == std::vector<std::uint8_t>(128, 255) &&
                  (vp ? l.last_session_ended_turn == INT32_MIN : !l.last_session_ended_turn) &&
                  l.last_special_session == 90000 && l.current_special_session == -1,
              "League IDs, raw name or session fields normalized");
        for (const auto *group : {&l.enact_proposals, &l.repeal_proposals, &l.active_resolutions,
                                  &l.enact_proposals_on_hold, &l.repeal_proposals_on_hold}) {
            check(group->size() == 1, "Resolution group lost");
            const auto &r = group->front();
            check(r.id == INT32_MAX && r.type == 90000 && r.league == 70000 && r.voter_decision.type == 123 &&
                      r.voter_decision.votes.size() == 2 && r.proposer_decision.vote.choice == -1 &&
                      r.proposer_decision.vote.player == 70000 && r.proposer_decision.vote.votes == INT32_MIN,
                  "Resolution identity or decisions altered");
            for (const auto &v : r.voter_decision.votes)
                check(v.player == 70000 && v.votes == INT32_MIN && v.choice == -1,
                      "Duplicate, negative or unknown vote normalized");
            check(r.effects.flags.size() == (vp ? 12 : 7) && r.effects.integers.size() == (vp ? 23 : 18) &&
                      effect(r, "one_time_gold") == 101 && effect(r, "religion_spread_strength_mod") == 112 &&
                      effect(r, "landmark_culture") == 118,
                  "Resolution effect order or values changed");
            if (vp)
                check(effect(r, "is_world_war") == INT32_MIN && effect(r, "change_tourism") == 122,
                      "VP integer effect coerced to boolean");
        }
        check(l.active_resolutions[0].turn_enacted == -1 && !l.active_resolutions[0].proposal_player &&
                  l.enact_proposals[0].proposal_player == 70000 && !l.enact_proposals[0].turn_enacted &&
                  l.repeal_proposals[0].target_resolution_id == INT32_MAX &&
                  l.repeal_proposals[0].repeal_decision->votes.size() == 2,
              "Resolution subtype state lost");
        check(l.last_turn_enact_proposals.size() == (vp ? 1u : 0u) &&
                  l.last_turn_repeal_proposals.size() == (vp ? 1u : 0u),
              "VP last turn proposals lost");
        check(l.members.size() == 2 && l.projects.size() == 1 &&
                  l.projects[0].production == std::vector<std::int64_t>{INT32_MIN, -1, INT32_MAX} &&
                  l.projects[0].type == 90000 && l.projects[0].complete &&
                  l.projects[0].progress_warning_sent,
              "Members or project contributions altered");
        for (const auto &m : l.members)
            check(m.player == 70000 && m.extra_votes == -1 && m.proposals == 65537 && m.votes == INT32_MIN &&
                      m.abstained_votes == -1 && m.may_propose && m.ever_been_host && m.always_been_host &&
                      m.vote_sources == std::vector<std::uint8_t>{'a', 0, 255},
                  "Member fields changed");
        for (std::size_t n = 0; n < f.bytes.size(); ++n)
            rejects([&] { observe(f.bytes.substr(0, n), vp); });
        for (auto offset : f.flags) {
            auto bad = f.bytes;
            bad[offset] = 2;
            rejects([&] { observe(bad, vp); });
        }
        for (auto offset : f.versions)
            for (auto v : {0u, 99u}) {
                auto bad = f.bytes;
                replace32(bad, offset, v);
                rejects([&] { observe(bad, vp); });
            }
        for (auto offset : f.counts) {
            auto bad = f.bytes;
            replace32(bad, offset, UINT32_MAX);
            rejects([&] { observe(bad, vp); });
        }
        rejects([&] { observe(f.bytes, vp, {2, 1000, 10000}); });
        rejects([&] { observe(f.bytes, vp, {10000, 127, 10000}); });
        rejects([&] { observe(f.bytes, vp, {10000, 1000, 129}); });
        check(observe(f.bytes + "trade", vp).end_offset == f.bytes.size(), "Leagues consumed trade data");
    }
    for (unsigned lv = 1; lv <= 14; ++lv) {
        const auto f = fixture(false, lv, 1, 1, 1, 1);
        const auto s = observe(f.bytes, false);
        const auto &l = s.leagues[0];
        check(s.end_offset == f.bytes.size() && s.version == 1 && s.generated_id_count == 0 &&
                  s.leagues_ever_founded == 0 && s.diplomatic_victor == -1 && s.last_era_trigger == -1,
              "Legacy system defaults or missing generated counter wrong");
        check(l.version == lv && l.united_nations == (lv >= 4) && l.in_session == (lv >= 2) &&
                  l.host == (lv >= 3 ? 70000 : -1) && l.projects.size() == (lv >= 5 ? 1u : 0u) &&
                  l.custom_name.size() == (lv >= 7 ? 128u : 0u) &&
                  l.enact_proposals_on_hold.size() == (lv >= 8 ? 1u : 0u),
              "Legacy league fields wrong");
        const auto &m = l.members[0];
        check(m.extra_votes == (lv >= 9 ? -1 : 0) && m.vote_sources.size() == (lv >= 10 ? 3u : 0u) &&
                  m.proposals == (lv >= 12 ? 65537 : 0) && m.abstained_votes == (lv >= 13 ? -1 : 0) &&
                  m.ever_been_host == (lv >= 14) && m.always_been_host,
              "Legacy member defaults wrong");
        if (lv >= 5)
            check(l.projects[0].complete == (lv >= 6) && l.projects[0].progress_warning_sent == (lv >= 11),
                  "Legacy project flags wrong");
        check(!l.enact_proposals[0].id && !l.active_resolutions[0].id &&
                  l.active_resolutions[0].legacy_active_id == -19088744 &&
                  effect(l.enact_proposals[0], "league_project_enabled") == -1,
              "Legacy ID invented or discarded active ID lost");
    }
    for (unsigned ev = 1; ev <= 9; ++ev) {
        const auto s = observe(fixture(false, 14, ev).bytes, false);
        const auto &r = s.leagues[0].enact_proposals[0];
        check(r.effects.version == ev && effect(r, "league_project_enabled") == (ev >= 3 ? 103 : -1) &&
                  effect(r, "culture_per_wonder") == (ev >= 4 ? 108 : 0) &&
                  effect(r, "holy_city_tourism") == (ev >= 6 ? 111 : 0) &&
                  effect(r, "religion_spread_strength_mod") == (ev >= 9 ? 112 : 0) &&
                  effect(r, "artsy_great_person_rate_mod") == (ev >= 7 ? 115 : 0) &&
                  effect(r, "landmark_culture") == (ev >= 8 ? 118 : 0),
              "Legacy effect defaults wrong");
    }
    for (unsigned gv = 1; gv <= 5; ++gv) {
        const auto s = observe(fixture(false, 14, 9, gv).bytes, false);
        check(s.generated_id_count == (gv >= 4 ? INT32_MAX : 0) &&
                  s.leagues_ever_founded == (gv >= 2 ? 65537 : 0) &&
                  s.diplomatic_victor == (gv >= 3 ? 70000 : -1) &&
                  s.last_era_trigger == (gv >= 5 ? 90000 : -1),
              "Legacy system version field order wrong");
    }
    const auto f = fixture(true);
    auto wide = f.bytes;
    replace32(wide, f.production_count, 65537);
    wide.insert(f.production_count + 4 + 12, 4 * (65537 - 3), '\0');
    check(observe(wide, true).leagues[0].projects[0].production.size() == 65537,
          "Original player capacity leaked into project storage");
    class ShiftedStream final : public std::streambuf {
        const std::string &bytes_;
        std::uint64_t pos_ = 0;
        pos_type seekpos(pos_type pos, std::ios_base::openmode) override {
            pos_ = static_cast<std::uint64_t>(off_type(pos));
            return pos;
        }
        std::streamsize xsgetn(char *out, std::streamsize count) override {
            const auto local = pos_ - 0x100000123ull;
            if (local > bytes_.size() || static_cast<std::uint64_t>(count) > bytes_.size() - local)
                return 0;
            std::copy_n(bytes_.data() + local, count, out);
            pos_ += static_cast<std::uint64_t>(count);
            return count;
        }

      public:
        explicit ShiftedStream(const std::string &bytes) : bytes_(bytes) {}
    } shifted(wide);
    std::istream stream(&shifted);
    runtime::BinaryReader reader(stream, 0x100000123ull + wide.size());
    reader.seek(0x100000123ull);
    const auto moved = saves::observe_game_leagues(reader, GameStateLayout::vp542_corpus);
    check(moved.start_offset == 0x100000123ull && moved.end_offset == moved.start_offset + wide.size(),
          "League stream offsets narrowed");
    rejects([&] { saves::observe_game_leagues(reader, static_cast<GameStateLayout>(99)); });
}
