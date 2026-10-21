#pragma once
// Included after quote/bytes_hex inside inspect_main's anonymous namespace.
namespace leagues_json {
void value(bool v) { std::cout << (v ? "true" : "false"); }
void value(std::int64_t v) { std::cout << v; }
void value(std::uint64_t v) { std::cout << v; }
template <class T> void value(const std::optional<T> &v) {
    if (v)
        value(*v);
    else
        std::cout << "null";
}
template <class T> void field(const char *name, const T &v) {
    std::cout << ',' << quote(name) << ':';
    value(v);
}
template <class T, class Write> void list(const char *name, const std::vector<T> &v, Write write) {
    std::cout << ',' << quote(name) << ":[";
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i)
            std::cout << ',';
        write(v[i]);
    }
    std::cout << ']';
}
template <class T> void begin(const T &v) {
    std::cout << "{\"version\":";
    value(v.version);
}
void vote(const civ5::saves::LeagueVoteObservation &v) {
    begin(v);
    field("player", v.player);
    field("votes", v.votes);
    field("choice", v.choice);
    std::cout << '}';
}
void voter(const civ5::saves::LeagueVoterDecisionObservation &v) {
    begin(v);
    field("base_version", v.base_version);
    field("type", v.type);
    list("votes", v.votes, vote);
    std::cout << '}';
}
void resolution(const civ5::saves::ResolutionObservation &r) {
    begin(r);
    field("id", r.id);
    field("type", r.type);
    field("league", r.league);
    field("proposal_version", r.proposal_version);
    field("proposal_player", r.proposal_player);
    field("kind_version", r.kind_version);
    field("legacy_active_id", r.legacy_active_id);
    field("turn_enacted", r.turn_enacted);
    field("target_resolution_id", r.target_resolution_id);
    std::cout << ",\"effects\":";
    begin(r.effects);
    std::cout << ",\"integers\":{";
    for (std::size_t i = 0; i < r.effects.integers.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &f = r.effects.integers[i];
        std::cout << quote(f.name) << ':';
        value(f.value);
    }
    std::cout << "},\"flags\":{";
    for (std::size_t i = 0; i < r.effects.flags.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &f = r.effects.flags[i];
        std::cout << quote(f.name) << ':';
        value(f.value);
    }
    std::cout << "}},\"voter_decision\":";
    voter(r.voter_decision);
    std::cout << ",\"proposer_decision\":";
    begin(r.proposer_decision);
    field("base_version", r.proposer_decision.base_version);
    field("type", r.proposer_decision.type);
    std::cout << ",\"vote\":";
    vote(r.proposer_decision.vote);
    std::cout << "},\"repeal_decision\":";
    if (r.repeal_decision)
        voter(*r.repeal_decision);
    else
        std::cout << "null";
    std::cout << '}';
}
void member(const civ5::saves::LeagueMemberObservation &m) {
    std::cout << "{\"vote_sources_hex\":" << quote(bytes_hex(m.vote_sources));
    field("player", m.player);
    field("extra_votes", m.extra_votes);
    field("may_propose", m.may_propose);
    field("proposals", m.proposals);
    field("votes", m.votes);
    field("abstained_votes", m.abstained_votes);
    field("ever_been_host", m.ever_been_host);
    field("always_been_host", m.always_been_host);
    std::cout << '}';
}
void project(const civ5::saves::LeagueProjectObservation &p) {
    std::cout << "{\"type\":";
    value(p.type);
    list("production", p.production, [](auto v) { value(v); });
    field("complete", p.complete);
    field("progress_warning_sent", p.progress_warning_sent);
    std::cout << '}';
}
void league(const civ5::saves::LeagueObservation &l) {
    begin(l);
    field("id", l.id);
    field("united_nations", l.united_nations);
    field("in_session", l.in_session);
    field("host_proposing", l.host_proposing);
    field("turns_until_session", l.turns_until_session);
    field("resolutions_ever_enacted", l.resolutions_ever_enacted);
    list("enact_proposals", l.enact_proposals, resolution);
    list("repeal_proposals", l.repeal_proposals, resolution);
    list("active_resolutions", l.active_resolutions, resolution);
    list("members", l.members, member);
    field("host", l.host);
    list("projects", l.projects, project);
    field("consecutive_hosted_sessions", l.consecutive_hosted_sessions);
    field("assigned_name", l.assigned_name);
    std::cout << ",\"custom_name_hex\":" << quote(bytes_hex(l.custom_name));
    field("last_session_ended_turn", l.last_session_ended_turn);
    field("last_special_session", l.last_special_session);
    field("current_special_session", l.current_special_session);
    list("enact_proposals_on_hold", l.enact_proposals_on_hold, resolution);
    list("repeal_proposals_on_hold", l.repeal_proposals_on_hold, resolution);
    list("last_turn_enact_proposals", l.last_turn_enact_proposals, resolution);
    list("last_turn_repeal_proposals", l.last_turn_repeal_proposals, resolution);
    std::cout << '}';
}
} // namespace leagues_json
void game_leagues_json(const civ5::saves::GameLeaguesObservation &state) {
    using namespace leagues_json;
    begin(state);
    std::cout << ",\"layout\":"
              << quote(state.layout == civ5::saves::GameStateLayout::vp542_corpus ? "vp542_corpus_comparison"
                                                                                  : "expansion2_version1")
              << ",\"next_field\":\"game_trade\",\"saved_content_profile_verified\":false,\"load_side_"
                 "effects_applied\":false";
    field("start_offset", state.start_offset);
    field("end_offset", state.end_offset);
    field("generated_id_count", state.generated_id_count);
    field("leagues_ever_founded", state.leagues_ever_founded);
    field("diplomatic_victor", state.diplomatic_victor);
    field("last_era_trigger", state.last_era_trigger);
    list("leagues", state.leagues, league);
    std::cout << '}';
}
