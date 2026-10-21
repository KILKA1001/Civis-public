#include "civ5/runtime/bnw_fresh_player_uninit_prefix.hpp"
#include "civ5/runtime/bnw_fresh_trade_ai_uninit.hpp"
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
    const std::vector<civ5::content::GameplayDefine> constants{
        {"MAX_MAJOR_CIVS",3}};
    auto input = make_bnw_fresh_player_uninit_prefix_input(constants);
    check(input.has_value(),"selected native constants required");
    const auto prefix = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(*input));
    check(prefix.detached_effects_complete && prefix.trade.member_effect_complete &&
          !prefix.archive_traversal_verified,
          "TradeAI Uninit requires source-ordered PlayerTrade predecessor");

    const auto fresh = make_bnw_fresh_trade_ai_constructor_state();
    check(!fresh.player_bound && !fresh.removable_value,
          "CvTradeAI constructor nulls player but not removable value");
    const auto fresh_draft = project_bnw_fresh_trade_ai_uninit(
        objects,prefix.archive,prefix.policies,prefix.economic,
        prefix.military,prefix.city_specialization,prefix.wonder,
        prefix.grand_strategy,prefix.diplomacy,prefix.religions,
        prefix.religion_ai,prefix.espionage,prefix.espionage_ai,
        prefix.trade,"selected-sha",fresh);
    check(fresh_draft.member_effect_complete && fresh_draft.state == fresh &&
          fresh_draft.source_order == std::vector<std::string_view>{"m_pPlayer"} &&
          !fresh_draft.archive_predecessor_verified &&
          !fresh_draft.remaining_services_complete &&
          !fresh_draft.initialized_by_gamecore,
          "fresh TradeAI Uninit must preserve scalar and readiness blockers");

    BnwFreshTradeAiServiceState populated;
    populated.player_bound = true;
    populated.removable_value = 17;
    auto draft = project_bnw_fresh_trade_ai_uninit(
        objects,prefix.archive,prefix.policies,prefix.economic,
        prefix.military,prefix.city_specialization,prefix.wonder,
        prefix.grand_strategy,prefix.diplomacy,prefix.religions,
        prefix.religion_ai,prefix.espionage,prefix.espionage_ai,
        prefix.trade,"selected-sha",populated);
    check(draft.member_effect_complete && !draft.state.player_bound &&
          draft.state.removable_value == 17 && populated.player_bound,
          "Uninit must null player but retain removable value");

    auto wrong = project_bnw_fresh_trade_ai_uninit(
        objects,prefix.archive,prefix.policies,prefix.economic,
        prefix.military,prefix.city_specialization,prefix.wonder,
        prefix.grand_strategy,prefix.diplomacy,prefix.religions,
        prefix.religion_ai,prefix.espionage,prefix.espionage_ai,
        prefix.trade,"wrong-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must fail closed");
    auto missing = prefix.trade;
    missing.member_effect_complete = false;
    wrong = project_bnw_fresh_trade_ai_uninit(
        objects,prefix.archive,prefix.policies,prefix.economic,
        prefix.military,prefix.city_specialization,prefix.wonder,
        prefix.grand_strategy,prefix.diplomacy,prefix.religions,
        prefix.religion_ai,prefix.espionage,prefix.espionage_ai,
        missing,"selected-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "missing PlayerTrade predecessor must fail closed");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard detached TradeAI state");
}
