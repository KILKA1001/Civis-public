#include "civ5/runtime/bnw_unit_kill_frontier.hpp"
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *s) {
    return s?std::string_view(s):std::string_view();
}
template<class Player> auto &lifetime(Player &player) {
    using Field=std::conditional_t<std::is_const_v<Player>,
        const saves::PlayerCoreField,saves::PlayerCoreField>;
    Field *found=nullptr;
    for(auto &field:player.core.fields)
        if(name(field.name)=="m_iLifetimeCombatExperience") {
            require(!found,"Duplicate BNW kill lifetime XP field");found=&field;
        }
    require(found && found->encoding==saves::PlayerCoreEncoding::i32 &&
            found->values.size()==1 && !found->wire_count,
            "BNW kill lifetime XP field unavailable");
    return found->values[0];
}
void same_removal(const BnwUnitRemoval &a,const BnwUnitRemoval &b) {
    require(a.player_index==b.player_index && a.unit_index==b.unit_index &&
            a.plot_index==b.plot_index && a.area_index==b.area_index &&
            a.owner==b.owner && a.id==b.id && a.area_id==b.area_id,
            "BNW kill frontier removal is stale");
}
void same_entry(const BnwUnitKillEntry &a,const BnwUnitKillEntry &b) {
    same_removal(a.removal,b.removal);
    const auto &x=a.lua_arguments,&y=b.lua_arguments;
    require(x.owner==y.owner && x.unit_id==y.unit_id &&
            x.unit_type==y.unit_type && x.x==y.x && x.y==y.y &&
            x.delayed==y.delayed && x.killer==y.killer &&
            a.army_id==b.army_id &&
            a.requires_script_system_query &&
            b.requires_script_system_query && a.requires_can_save_unit &&
            b.requires_can_save_unit && a.requires_selected_ui_query &&
            b.requires_selected_ui_query &&
            a.requires_army_service==b.requires_army_service &&
            a.blocker==
                BnwUnitKillEntryBlocker::script_system_and_can_save_unit_unresolved &&
            b.blocker==a.blocker &&
            !a.can_advance && !b.can_advance,
            "BNW kill entry callback boundary is stale");
}
void same_class(const BnwUnitClassDeath &a,const BnwUnitClassDeath &b) {
    same_removal(a.removal,b.removal);
    require(a.team==b.team && a.unit_class==b.unit_class &&
            a.team_count_before==b.team_count_before &&
            a.player_count_before==b.player_count_before,
            "BNW kill frontier class stage is stale");
}
void same_support(const BnwUnitSupportDeath &a,
                  const BnwUnitSupportDeath &b) {
    same_class(a.class_death,b.class_death);
    require(a.builder_delta==b.builder_delta &&
            a.unhappiness_delta==b.unhappiness_delta &&
            a.extra_cost_delta==b.extra_cost_delta &&
            a.nuke_delta==b.nuke_delta &&
            a.military_delta==b.military_delta && a.before==b.before &&
            a.needs_active_player_ui_check==b.needs_active_player_ui_check,
            "BNW kill frontier support stage is stale");
}
void same_capture(const BnwUnitCaptureDeathQuote &a,
                  const BnwUnitCaptureDeathQuote &b) {
    require(a.old_player==b.old_player &&
            a.original_owner==b.original_owner &&
            a.old_type==b.old_type &&
            a.capturing_player==b.capturing_player &&
            a.capture_type==b.capture_type && a.x==b.x && a.y==b.y &&
            a.religion==b.religion &&
            a.religious_strength==b.religious_strength &&
            a.spreads_left==b.spreads_left &&
            a.embarked==b.embarked &&
            a.captured_as_is==b.captured_as_is &&
            a.capture_valid==b.capture_valid &&
            !a.can_commit_kill && !b.can_commit_kill,
            "BNW kill frontier capture quote is stale");
}
void same_xp(const BnwDeathXpQuote &a,const BnwDeathXpQuote &b) {
    same_support(a.support,b.support);
    require(a.killer==b.killer &&
            a.unit_experience==b.unit_experience && a.loss==b.loss &&
            a.triggered==b.triggered && a.naval==b.naval &&
            a.setter_branch_unported==b.setter_branch_unported &&
            a.current_before==b.current_before &&
            a.current_after==b.current_after &&
            a.lifetime_before==b.lifetime_before &&
            a.lifetime_after==b.lifetime_after,
            "BNW kill frontier XP quote is stale");
}
} // namespace

BnwUnitKillFrontier plan_bnw_unit_kill_frontier(
    const saves::SavedSessionSnapshot &source,
    std::uint64_t player_index,std::uint64_t unit_index,
    std::int64_t killer,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules,
    const BnwDeathXpRules &xp_rules) {
    BnwUnitKillFrontier plan;
    plan.removal=plan_bnw_unit_removal(source,player_index,unit_index);
    plan.entry=plan_bnw_unit_kill_entry(source,plan.removal,killer);
    plan.class_death=plan_bnw_unit_class_death(
        source,plan.removal,class_rules);
    plan.support_death=plan_bnw_unit_support_death(
        source,plan.class_death,class_rules,support_rules);
    plan.xp=quote_bnw_unit_death_xp(
        source,plan.support_death,class_rules,support_rules,xp_rules,killer);
    plan.capture=quote_bnw_unit_capture_death_no_player(
        source,plan.xp,class_rules,support_rules,xp_rules);
    return plan;
}

saves::SavedSessionSnapshot project_bnw_unit_kill_frontier_diagnostic(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitKillFrontier &frontier,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules,
    const BnwDeathXpRules &xp_rules) {
    require(!frontier.can_commit_kill && !frontier.capture.capture_valid,
            "BNW kill frontier cannot publish a gameplay kill");
    const auto fresh=plan_bnw_unit_kill_frontier(
        source,frontier.removal.player_index,frontier.removal.unit_index,
        frontier.xp.killer,class_rules,support_rules,xp_rules);
    same_removal(frontier.removal,fresh.removal);
    same_entry(frontier.entry,fresh.entry);
    same_class(frontier.class_death,fresh.class_death);
    same_support(frontier.support_death,fresh.support_death);
    same_capture(frontier.capture,fresh.capture);
    same_xp(frontier.xp,fresh.xp);
    same_xp(frontier.capture.xp,fresh.capture.xp);
    require(!frontier.xp.setter_branch_unported,
            "BNW kill XP setter/Great Person effects unported");
    // The support projection revalidates its own nested source-bound plan.
    // Only one full world copy is needed for this atomic detached result.
    auto counters=project_bnw_unit_support_death(
        source,frontier.support_death,class_rules,support_rules);
    const auto owner=frontier.removal.player_index;
    auto &counter=lifetime(counters.all_players->players[owner]);
    require(counter==lifetime(source.all_players->players[owner]),
            "BNW kill support stage overlapped lifetime XP");
    require(counter==frontier.xp.lifetime_before || !frontier.xp.triggered,
            "BNW kill XP field changed before detached merge");
    if(frontier.xp.triggered) counter=frontier.xp.lifetime_after;
    // FFreeListTrashArray::RemoveAt follows the missing setXY/removeUnit and
    // resource effects in SDK order. This is a detached final-state diagnostic.
    return project_bnw_unit_removal(counters,frontier.removal);
}
} // namespace civ5::runtime
