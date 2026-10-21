#include "civ5/runtime/bnw_fresh_pregame_roster.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>
#include <type_traits>

namespace {
template<class T>concept PublicWorldReset=requires(T& owner,const civ5::runtime::BnwFreshPregameSpeed& pg) {
    owner.reset_selected_world_slots_from_pregame(pg);
};
template<class T>concept PublicWorldCache=requires(T& owner,const civ5::runtime::BnwFreshPregameSpeed& pg,
                                                  const civ5::content::RulesRowView& row) {
    owner.cache_world_from_selected_row(pg,row);
};
static_assert(!PublicWorldReset<civ5::runtime::BnwFreshPregameRoster>);
static_assert(!PublicWorldCache<civ5::runtime::BnwFreshPregameRoster>);
static_assert(static_cast<int>(civ5::runtime::BnwFreshSlotStatus::computer)==1);
static_assert(static_cast<int>(civ5::runtime::BnwFreshSlotStatus::taken)==3);
static_assert(static_cast<int>(civ5::runtime::BnwFreshSlotStatus::observer)==4);
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_init_players_pregame_prefix() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const auto rules_sha = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    const std::vector<civ5::content::GameplayDefine> constants{
        {"MAX_TEAMS",64},{"MAX_PLAYERS",64},{"MAX_CIV_PLAYERS",63},
        {"MAX_MAJOR_CIVS",22},{"BARBARIAN_PLAYER",63},
        {"BARBARIAN_TEAM",63}};
    std::vector<BnwFreshPregameSlot> pregame(64);
    for (std::size_t i = 0; i < pregame.size(); ++i)
        pregame[i].team_id = static_cast<std::int32_t>(i);
    pregame[0].status = BnwFreshSlotStatus::taken;
    pregame[1].status = BnwFreshSlotStatus::computer;
    pregame[2].status = BnwFreshSlotStatus::observer;
    const auto before = pregame;
    auto draft = draft_bnw_fresh_pregame_roster(
        selected, rules_sha, constants, pregame, 2, 62);
    check(draft.slot_prefix_complete && !draft.initialized_by_gamecore &&
              draft.has_blocker(BnwFreshRosterBlockerCode::major_colors_pending) &&
              draft.has_blocker(BnwFreshRosterBlockerCode::player_ai_init_pending) &&
              draft.teams_reset.size() == 64 &&
              draft.source_order.size() == 70 &&
              draft.source_order[0] ==
                  BnwFreshRosterEvent{BnwFreshRosterEventKind::team_reset,0} &&
              draft.source_order[63] ==
                  BnwFreshRosterEvent{BnwFreshRosterEventKind::team_reset,63} &&
              draft.source_order[64] ==
                  BnwFreshRosterEvent{BnwFreshRosterEventKind::major_color_pass_pending,-1},
          "Teams reset first, with color and player init effects still blocked");
    check(draft.slots[2].team_id == 62 &&
              draft.slots[22].status == BnwFreshSlotStatus::computer &&
              draft.slots[23].status == BnwFreshSlotStatus::computer &&
              draft.slots[24].status == BnwFreshSlotStatus::closed &&
              draft.slots[63].status == BnwFreshSlotStatus::computer &&
              draft.slots[63].team_id == 63 && pregame == before &&
              draft.source_order[65] ==
                  BnwFreshRosterEvent{BnwFreshRosterEventKind::observer_team,2} &&
              draft.source_order[68] ==
                  BnwFreshRosterEvent{BnwFreshRosterEventKind::barbarian_team,63},
          "Observer, minor and barbarian pregame writes follow SDK player order");
    auto bad_constants = constants;
    bad_constants.push_back({"MAX_TEAMS",64});
    const auto mismatch = draft_bnw_fresh_pregame_roster(
        selected, rules_sha, bad_constants, pregame, 2, 62);
    check(mismatch.has_blocker(BnwFreshRosterBlockerCode::native_constants_unverified) &&
              mismatch.slots.empty(),
          "Ambiguous native constants cannot produce a roster draft");
    const auto unresolved_observer = draft_bnw_fresh_pregame_roster(
        selected, rules_sha, constants, pregame, 2);
    check(unresolved_observer.has_blocker(
              BnwFreshRosterBlockerCode::pregame_slots_incomplete) &&
              unresolved_observer.slots.empty(),
          "Observer team must be resolved before any detached slot write");
    draft.rollback();
    check(!draft.slot_prefix_complete && !draft.initialized_by_gamecore &&
              draft.slots.empty() && draft.teams_reset.empty() &&
              draft.source_order.empty() && pregame == before,
          "Rollback leaves caller-owned pregame slots untouched");
}

void test_native_namespace_and_reset_player_prefix() {
    using namespace civ5::runtime;
    using W=BnwFreshPregameRosterWrite;
    static_assert(!std::is_default_constructible_v<BnwFreshPregameRoster>);
    static_assert(!std::is_copy_constructible_v<BnwFreshPregameRoster>);
    static_assert(!std::is_constructible_v<BnwFreshPregameRoster,
                  std::shared_ptr<const BnwCreationConfig>,const BnwFreshPregameSpeed&>);
    const auto slots=detail::bnw_pregame_namespace_slots();
    check(slots.size()==64,"Native MAX_PLAYERS is63+1, not80");
    for(const auto& s:slots) {
        check(s.civilization==0 && s.leader==0 && s.minor_civ==0 &&
              s.team==0 && s.handicap==0 && s.last_human_handicap==0 &&
              s.player_color==0 && s.art_style==0 && s.status==0 &&
              s.claim==0 && s.net_id==0,"Native numeric namespace vectors value-initialize0/OPEN0");
        check(!s.white_flag && !s.civilization_available && !s.civilization_playable &&
              !s.leader_available && !s.playable && !s.minor && !s.ready,
              "Native boolean namespace vectors false");
        check(s.leader_name.empty() && s.civilization_description.empty() &&
              s.civilization_short_description.empty() && s.civilization_adjective.empty() &&
              s.civilization_password.empty() && s.email.empty() &&
              s.civilization_key.empty() && s.leader_key.empty() &&
              s.nickname.empty() && !s.display_nickname && s.available_dlc.empty(),
              "Native string/list namespace constructors empty");
        for(const auto b:s.civilization_package) check(b==0,"Civilization GUID value-initialized0");
        for(const auto b:s.leader_package) check(b==0,"Leader GUID value-initialized0");
    }
    auto input=slots[0];
    input.leader_name="leader"; input.civilization_description="long";
    input.civilization_short_description="short"; input.civilization_adjective="adj";
    input.civilization_password="pass"; input.email="email";
    input.white_flag=true; input.civilization_key="civ"; input.leader_key="lead";
    input.civilization_package.fill(255); input.leader_package.fill(255);
    // Later operands/stores are deliberately varied in DETACHED diagnostics.
    input.handicap=7; input.last_human_handicap=8; input.player_color=9;
    input.art_style=10; input.status=3; input.claim=2; input.net_id=11;
    input.playable=true; input.minor=true; input.ready=true;
    input.nickname="source"; input.display_nickname="display";
    input.available_dlc.push_back(input.civilization_package);
    const auto original=input;
    const auto q=detail::quote_bnw_pregame_reset_player_to_handicap(input,12);
    const auto& s=q.slot;
    check(s.leader_name.empty() && s.civilization_description.empty() &&
          s.civilization_short_description.empty() && s.civilization_adjective.empty() &&
          s.civilization_password.empty() && s.email.empty() && !s.white_flag &&
          s.civilization==-1 && s.leader==-1 && s.minor_civ==-1 && s.team==12 &&
          s.civilization_key.empty() && s.leader_key.empty() &&
          s.civilization_available && s.civilization_playable && s.leader_available,
          "ResetPlayer strings/white/sentinel key bind+fallback/minor/team source prefix");
    for(const auto b:s.civilization_package) check(b==0,"Civilization ClearGUID exact0");
    for(const auto b:s.leader_package) check(b==0,"Leader ClearGUID exact0");
    check(s.handicap==7 && s.last_human_handicap==8 && s.player_color==9 &&
          s.art_style==10 && s.status==3 && s.claim==2 && s.net_id==11 &&
          s.playable && s.minor && s.ready && s.nickname=="source" &&
          s.display_nickname=="display" && s.available_dlc==original.available_dlc,
          "Mandatory handicap frontier precedes ALL later native stores and observer override");
    check(q.writes==std::vector<W>{W::leader_name,W::civilization_description,
          W::civilization_short_description,W::civilization_adjective,
          W::civilization_password,W::email,W::white_flag,W::civilization,
          W::civilization_key,W::civilization_package,W::civilization_available,
          W::civilization_playable,W::civilization,W::civilization_key,
          W::civilization_package,W::civilization_available,W::civilization_playable,
          W::leader,W::leader_key,W::leader_package,W::leader_available,
          W::leader,W::leader_available,W::minor_civ,W::team},
          "Native25 stores preserve duplicate civilization fallback and distinct leader fallback");
    check(input==original && slots[0].civilization==0 && slots[63].team==0,
          "Detached quote leaves caller and barbarian constructor slots untouched");
    for(const std::int32_t player:{-1,64}) {
        bool refused=false;
        try { (void)detail::quote_bnw_pregame_reset_player_to_handicap(input,player); }
        catch(const BnwFreshPregameRosterRefusal& e) {
            refused=e.code()==BnwFreshPregameRosterError::native_source_unavailable;
        }
        check(refused,"Detached invalid native player index cannot issue a prefix");
    }
}

void test_native_handicap_setter_suffix() {
    using namespace civ5::runtime;using W=BnwFreshPregameRosterWrite;
    auto slot=detail::bnw_pregame_namespace_slots()[0];slot.status=3;
    slot.handicap=7;slot.last_human_handicap=8;slot.nickname="old";slot.display_nickname="old";
    slot.available_dlc.push_back(slot.civilization_package);
    const auto before=slot;
    const auto q=detail::quote_bnw_pregame_reset_player_after_handicap(slot,83);
    check(q.writes==std::vector<W>{W::handicap,W::last_human_handicap,W::last_human_handicap,
        W::player_color,W::art_style,W::status,W::claim,W::playable,W::minor,W::net_id,W::ready,
        W::display_nickname,W::nickname,W::available_dlc},"TAKEN setter caches handicap before explicit NO_HANDICAP overwrites it");
    check(q.slot.handicap==83 && q.slot.last_human_handicap==-1 && q.slot.status==2 && q.slot.net_id==-1 &&
        q.slot.player_color==-1 && q.slot.art_style==-1 && q.slot.display_nickname && q.slot.display_nickname->empty() &&
        q.slot.nickname.empty() && q.slot.available_dlc.empty() && slot==before,"Suffix exact fields/display insert/DLC clear, detached input unchanged");
    for(const auto status:{0,1,2,4}) {
        slot.status=status;const auto closed=detail::quote_bnw_pregame_reset_player_after_handicap(slot,-2147483647-1);
        check(closed.writes.size()==13 && closed.writes[0]==W::handicap && closed.writes[1]==W::last_human_handicap &&
            closed.slot.handicap==(-2147483647-1),"Non-TAKEN native status suppresses only human side write; signed cached value is not reinterpreted Info ID");
    }
}

} // namespace

int main() { test_init_players_pregame_prefix(); test_native_namespace_and_reset_player_prefix(); test_native_handicap_setter_suffix(); }
