#include "civ5/runtime/bnw_fresh_diplomacy_uninit.hpp"
#include <algorithm>
#include <stdexcept>

namespace {
using namespace civ5::runtime;
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
BnwFreshFirstPlayerObjects first_player() {
    BnwFreshFirstPlayerObjects result;
    result.player_id = 0;
    result.selected_rules_sha256 = "selected-sha";
    result.constructors_complete = true;
    result.blockers.push_back({
        BnwFreshFirstPlayerObjectBlockerCode::reset_sync_archive_and_uninit_pending,
        "fixture"});
    return result;
}
} // namespace

int main() {
    const auto objects = first_player();
    const auto archive = project_bnw_fresh_full_archive_reset(
        objects,"selected-sha",make_bnw_fresh_archive_defaults());
    const auto policies = project_bnw_fresh_policies_uninit(
        objects,archive,"selected-sha",
        make_bnw_fresh_policies_constructor_state());
    const auto economic = project_bnw_fresh_economic_uninit(
        objects,archive,policies,"selected-sha",
        make_bnw_fresh_economic_constructor_state());
    const auto military = project_bnw_fresh_military_uninit(
        objects,archive,policies,economic,"selected-sha",
        make_bnw_fresh_military_constructor_state());
    const auto city = project_bnw_fresh_city_specialization_uninit(
        objects,archive,policies,economic,military,"selected-sha",
        make_bnw_fresh_city_specialization_constructor_state());
    const auto wonder = project_bnw_fresh_wonder_uninit(
        objects,archive,policies,economic,military,city,"selected-sha",
        make_bnw_fresh_wonder_constructor_state());
    const auto grand = project_bnw_fresh_grand_strategy_uninit(
        objects,archive,policies,economic,military,city,wonder,"selected-sha",
        make_bnw_fresh_grand_strategy_constructor_state());
    check(grand.member_effect_complete && !archive.aggregate_traversal_verified,
          "Diplomacy Uninit needs detached GrandStrategy predecessor");

    const auto names = bnw_fresh_diplomacy_uninit_aliases();
    check(names.size() == 134 &&
          names.front() == "m_paDiploLogStatementTurnCountScratchPad" &&
          names.back() == "m_ppaDiploStatementsLog" &&
          std::find(names.begin(),names.end(),
                    "m_ppaaeOtherPlayerMajorCivOpinion") == names.end(),
          "ordered SDK alias list must preserve its omitted field");
    const auto fresh = make_bnw_fresh_diplomacy_constructor_state();
    check(!fresh.owned_diplo_data &&
          fresh.null_written_aliases.size() == names.size() &&
          std::all_of(fresh.null_written_aliases.begin(),
                      fresh.null_written_aliases.end(),
                      [](auto alias) {
                          return alias == BnwFreshDiplomacyAliasState::null_alias;
                      }) &&
          fresh.other_player_major_opinion_alias ==
              BnwFreshDiplomacyAliasState::null_alias,
          "fresh Diplomacy constructor must have null data and aliases");
    const auto fresh_draft = project_bnw_fresh_diplomacy_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        "selected-sha",fresh);
    check(fresh_draft.member_effect_complete && fresh_draft.state == fresh &&
          fresh_draft.source_order.size() == names.size() + 2 &&
          fresh_draft.source_order.front() == names.front() &&
          fresh_draft.source_order[names.size()-1] == names.back() &&
          fresh_draft.source_order[names.size()] == "delete m_pDiploData" &&
          fresh_draft.source_order.back() == "m_pDiploData" &&
          !fresh_draft.archive_predecessor_verified &&
          !fresh_draft.remaining_services_complete &&
          !fresh_draft.initialized_by_gamecore,
          "fresh Uninit must preserve SDK order and readiness blockers");

    auto populated = fresh;
    populated.owned_diplo_data = std::vector<std::byte>{
        std::byte{0x12},std::byte{0x34}};
    std::fill(populated.null_written_aliases.begin(),
              populated.null_written_aliases.end(),
              BnwFreshDiplomacyAliasState::bound_to_diplo_data);
    populated.other_player_major_opinion_alias =
        BnwFreshDiplomacyAliasState::bound_to_diplo_data;
    auto draft = project_bnw_fresh_diplomacy_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        "selected-sha",populated);
    check(draft.member_effect_complete && !draft.state.owned_diplo_data &&
          std::all_of(draft.state.null_written_aliases.begin(),
                      draft.state.null_written_aliases.end(),
                      [](auto alias) {
                          return alias == BnwFreshDiplomacyAliasState::null_alias;
                      }) &&
          draft.state.other_player_major_opinion_alias ==
              BnwFreshDiplomacyAliasState::dangling_after_delete &&
          populated.owned_diplo_data.has_value(),
          "Uninit must null 134 aliases and delete owner without fixing omission");

    auto wrong = project_bnw_fresh_diplomacy_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        "wrong-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must fail closed");
    auto missing_grand = grand;
    missing_grand.member_effect_complete = false;
    wrong = project_bnw_fresh_diplomacy_uninit(
        objects,archive,policies,economic,military,city,wonder,missing_grand,
        "selected-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "missing GrandStrategy predecessor must fail closed");
    auto invalid = fresh;
    invalid.null_written_aliases.front() =
        BnwFreshDiplomacyAliasState::bound_to_diplo_data;
    wrong = project_bnw_fresh_diplomacy_uninit(
        objects,archive,policies,economic,military,city,wonder,grand,
        "selected-sha",invalid);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "bound alias without data owner must fail closed");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard detached Diplomacy effect");
}
