#include "civ5/runtime/bnw_happiness_receipt_types.hpp"
#include "civ5/runtime/bnw_team_tech_share_culture.hpp"
#include "civ5/runtime/bnw_team_tech_player_research.hpp"
#include "civ5/runtime/bnw_major_player_traits_owner.hpp"
#pragma once

#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/team_technologies.hpp"
#include <cstdint>
#include <string>

namespace civ5::runtime {

enum class BnwBarbarianTeamHasTechStatus {
    committed,
    rejected,
    uncertain,
};
struct BnwBarbarianTeamHasTechResult {
    BnwBarbarianTeamHasTechStatus status=BnwBarbarianTeamHasTechStatus::rejected;
    bool hook_handled=false;
    std::string reason;
};

// Expansion2 CvTeamTechs::SetHasTech for the first missing barbarian tech:
// set the bit, set last acquired tech, then call TeamSetHasTech(team,tech,true).
// The hook boolean does not veto the write. This does NOT implement the
// surrounding CvTeam::setHasTech effects and must not be bound directly as a
// complete game-teams handler. The host must expose the same authoritative
// TeamTechnologies object to Lua gameplay getters.
//
// If a Lua callback fails, source order has already changed the technology
// and the Lua VM may have external effects. The result is uncertain, and the
// caller must stop the turn instead of retrying or rolling back only the bit.
BnwBarbarianTeamHasTechResult write_bnw_barbarian_team_has_tech(
    TeamTechnologies &barbarians,std::int64_t technology,LuaScriptHost *host);

} // namespace civ5::runtime

#include <memory>
#include <stdexcept>


namespace civ5::ui { class SelectedBnwReadOnlyPreview; class SelectedBnwPreviewHost; }
namespace civ5::runtime {
class BnwSelectedStockLuaSourceOwner;
class BnwSelectedUserOptionsOwner;
class BnwMajorTeamHasTechUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
// A direct nonrepeat CvTeam::setHasTech acquisition request, not a claim that
// research completed. Fixed bNewValue=true; before values come only from SAME
// actual raw/owned state. Research completion must supply its own preceding
// progress/overflow/dirty effects before using this entry.
struct BnwMajorTeamHasTechCommand {
 std::int64_t team=-1,technology=-1,player=-1,expected_turn=-1;
 std::uint64_t expected_generation=0,expected_ui_source_revision=0;
 // Preserve actual SDK caller arguments for the mandatory later suffix.
 // These are inputs, never predecessor/readiness attestations.
 bool first=true,announce=true;
};
enum class BnwMajorTeamHasTechPhase {
 team_set_has_tech_pending, uncertain, city_strength_pending, city_info_dirty_pending, resource_reveal_pending, player_traits_pending, policy_cost_pending,
 first_trait_unit_pending, remaining_process_players_pending,
 resource_plot_yields_pending, city_routes_pending, team_tech_researched_pending,
 ends_game_pending, artifact_notifications_pending, era_change_pending,
 map_reveal_pending, best_route_pending, player_research_pending,
 first_research_reward_pending, technology_announcement_pending, technology_share_pending,
 resource_announcement_pending, owed_culture_building_pending, research_choice_notification_pending,
 espionage_research_pending, major_happiness_pending,
 happiness_founder_pending, happiness_foreign_founder_pending, happiness_suffix_pending, technology_ui_pending, reward_option_pending, gameplay_tech_acquired_pending, technology_transport_pending
};
// Genuine paired HasTech + lastTech native prefix ONLY. Its state-owned once
// registry forbids replay/overlap even if every caller drops the receipt.
// TeamSetHasTech, city strength, resource and processTech intervals remain
// mandatory. The privately reached player_traits_pending cursor proves only
// the zero-effect process prefix; it never completes Traits or technology.
struct BnwMajorTeamCityStrengthUpdate {
 std::int32_t owner=-1,city=-1,before=0,after=0,known_technologies=0;
 std::int64_t plot=-1;std::uint64_t dirty_occurrence=0;
};
struct BnwMajorProcessTechPrefixObservation {
 std::uint64_t resource_rules=0,map_plots=0,reveal_rules=0,trade_rules=0;
 std::uint64_t routes=0,builds=0,buildings=0,improvements=0,promotions=0;
 std::int32_t first_traits_player=-1;
};
struct BnwMajorAfterResearchEffect {
 bool ends_game_before=false,ends_game_after=false,best_route_stored=false;
 std::int64_t current_era=-1,requested_era=-1,missing_earlier_technology=-1;
 std::int64_t best_route_before=-1,best_route_after=-1,winning_build=-1,route_value=0;
 std::uint64_t builds=0;
};

// Owned port of DLLUI::AddMessage. Text stays a localization request until the
// genuine UI formatter consumes it; this never claims displayed text or a GPU
// frame. All native arguments and ordered localization operands are retained.
struct BnwMajorTechMessage {
 std::int64_t message_id=0,player=-1,team=-1,technology=-1,duration=-1;
 bool force=false;
 std::int64_t turn=-1;
 std::uint64_t generation=0;
 std::string tag;
 std::vector<std::string> operands;
 bool operator==(const BnwMajorTechMessage&)const=default;
};
struct BnwMajorTechMessageQueue {
 std::vector<BnwMajorTechMessage> messages;
 // Queue publication is complete, UI formatting/consumption is independent.
 std::size_t written=0;
};
struct BnwMajorAnnouncementShareEffect {
 std::vector<BnwMajorTechMessage> messages;
 std::vector<BnwTeamTechShareCheck> sharing;
 std::uint64_t resources_checked=0,plots_checked=0,culture_cities_checked=0;
 bool announcement_complete=false,sharing_complete=false,culture_complete=false;
 std::int64_t recursive_team=-1,owed_culture_player=-1;
 std::uint64_t before_revision=0,after_revision=0;
};

// Actual native cache mirror for the selected player's BuildStealableTechList.
// Data-only, not an issuer or a statement that a mandatory Lua call occurred.
struct BnwMajorTechEspionageCache {
 std::int64_t player=-1;
 std::uint64_t generation=0;
 std::vector<std::vector<std::int64_t>> lists;
 std::vector<std::int64_t> maximum_cost,number_to_steal;
};
struct BnwMajorEspionagePrefixEffect {
 std::int64_t player=-1,first_hook_target=-1,first_hook_technology=-1;
 std::uint64_t targets_cleared=0,targets_completed=0,known_technologies_checked=0;
 std::uint64_t before_revision=0,after_revision=0;
 bool complete=false;
};

// Readback of genuine DoUpdateHappiness stores; these data never issue a hook.
struct BnwMajorHappinessPrefixEffect {
 std::int64_t player=-1;
 std::int32_t initial_happiness=0;
 std::vector<BnwHappinessNativeStore> stores;
 std::uint64_t before_revision=0,after_revision=0,founder_callbacks=0;
 std::optional<std::int32_t> founder_religion;
 std::optional<BnwReligionHappinessTerms> religion;
 bool religion_complete=false;
};

// Genuine engine dirty-bit transport. This is neither a UI callback receipt nor
// an assertion that an achievement was sent to Steam. Excess is absent when
// the SDK short-circuit did not call GetExcessHappiness for a nonlocal player.
struct BnwMajorHappinessDirtyEvent {
 std::int64_t player=-1;
 std::int32_t happiness=0,connections=0;
 std::optional<std::int32_t> excess;
 std::uint64_t generation=0,occurrence=0;
 BnwTeamTechResearchDirtyBit bit=BnwTeamTechResearchDirtyBit::game_data;
 bool dirty=true,achievement_xp2_45_requested=false;
 bool operator==(const BnwMajorHappinessDirtyEvent&)const=default;
};
struct BnwMajorHappinessPublication {
 std::vector<BnwMajorHappinessDirtyEvent> events;
 std::size_t written=0;
 bool pending_ui_consumption=true;
};
struct BnwMajorHappinessSuffixEffect {
 std::int64_t player=-1;
 std::vector<BnwHappinessSuffixStore> stores;
 std::int32_t connection_before=0,connection_after=0;
 std::optional<std::int32_t> excess;
 bool achievement_requested=false;
 std::uint64_t before_revision=0,after_revision=0;
};


// Ordered engine/UI port requests. Publication never asserts displayed text,
// a changed CvNotifications ring, actual GPU consumption or full continuation.
enum class BnwMajorTechUiTransportKind { dirty, notification_add, popup_add, gameplay_tech_acquired };
struct BnwMajorTechUiTransportEvent {
 BnwMajorTechUiTransportKind kind=BnwMajorTechUiTransportKind::dirty;
 std::int64_t team=-1,technology=-1,player=-1,turn=-1;
 std::uint64_t generation=0,occurrence=0;
 std::string native_name;
 bool dirty=false;
 // Exact CvPopupInfo ctor flags/options defaults and data1/data2/data3.
 std::int32_t flags=0;bool option1=false,option2=false;
 std::int64_t data1=0,data2=0,data3=0,x=-1,y=-1,game_data_index=0,extra_game_data=0;
 // Notification message and summary use the same native localization request.
 std::string localization_tag;
 std::vector<std::string> localization_operands;
 bool operator==(const BnwMajorTechUiTransportEvent&)const=default;
};
struct BnwMajorTechUiTransportQueue {
 std::vector<BnwMajorTechUiTransportEvent> events;
 std::size_t written=0;
 bool pending_consumer=true;
};

class BnwMajorTeamHasTechPending final {
 public:
 ~BnwMajorTeamHasTechPending();
 BnwMajorTeamHasTechPending(const BnwMajorTeamHasTechPending&)=delete;
 BnwMajorTeamHasTechPending& operator=(const BnwMajorTeamHasTechPending&)=delete;
 static std::shared_ptr<const BnwMajorTeamHasTechPending> begin_nonrepeat_acquisition(
     const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechCommand&);
 BnwMajorTeamHasTechPhase phase()const;
 std::vector<BnwMajorTeamCityStrengthUpdate> city_strength_updates()const;
 std::optional<BnwMajorProcessTechPrefixObservation> process_prefix_observation()const;
 std::optional<BnwMajorPlayerTraitsResetEffect> traits_reset_effect()const;
 std::optional<BnwMajorPolicyFreeUnitEffect> policy_free_unit_effect()const;
 bool team_tech_researched_completed()const;
 std::optional<BnwMajorAfterResearchEffect> after_research_effect()const;
 std::vector<BnwTeamTechPlayerResearchEffect> player_research_effects()const;
 std::optional<BnwMajorAnnouncementShareEffect> announcement_share_effect()const;
 std::optional<BnwMajorEspionagePrefixEffect> espionage_prefix_effect()const;
 std::optional<BnwMajorHappinessPrefixEffect> happiness_prefix_effect()const;
 std::vector<BnwMajorHappinessSuffixEffect> happiness_suffix_effects()const;
 std::vector<BnwMajorTechUiTransportEvent> technology_transport_events()const;
 const BnwMajorTeamHasTechCommand& command()const;
 std::uint64_t after_revision()const;
 void require_stored_current(const ui::SelectedBnwPreviewHost&)const;
 private:
 friend class BnwSelectedStockLuaSourceOwner;
 // Genuine private consumer owns canonical SelectedHost already; no public
 // Host constructor or reconstructed Preview can issue this command.
 static std::shared_ptr<const BnwMajorTeamHasTechPending> begin_for_source_host(
     std::shared_ptr<const ui::SelectedBnwPreviewHost>,const BnwMajorTeamHasTechCommand&,
     std::shared_ptr<const BnwMajorPlayerTraitsOwner>* issued_traits_owner=nullptr);
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwMajorTeamHasTechPending(std::shared_ptr<Impl>);
 // Only SourceOwner may bracket the real SAME-VM CallHook with these. The
 // private receiver adopts only this actual paired-store revision beforehand.
 // Begin stamps uncertain BEFORE callback; no callback/boolean ack factory.
 void begin_team_set_has_tech_dispatch(const ui::SelectedBnwPreviewHost&)const;
 void finish_team_set_has_tech_dispatch(const ui::SelectedBnwPreviewHost&)const;
 // Actual SDK loop: paired strength store then owned CityInfo dirty queue
 // publication BEFORE the next city. setDirty does not synchronously draw or
 // call Lua. The next real UI frame consumes the retained queue/revision.
 std::optional<BnwMajorTeamCityStrengthUpdate> advance_city_strength(const ui::SelectedBnwPreviewHost&)const;
 // SDK full resource map scan and proven zero processTech prefix through the
 // first actual ever-alive member's native Traits Reset/Init boundary.
 void advance_resource_and_process_prefix(const ui::SelectedBnwPreviewHost&)const;
 // Exact existing native Reset/Init after the true processTech prefix.
 void advance_first_player_traits(const ui::SelectedBnwPreviewHost&)const;
 void advance_policy_and_post_player_prefix(const ui::SelectedBnwPreviewHost&)const;
 // Real SAME-VM CallHook(team,tech,+1), bracketed only by SourceOwner.
 void begin_team_tech_researched_dispatch(const ui::SelectedBnwPreviewHost&)const;
 void finish_team_tech_researched_dispatch(const ui::SelectedBnwPreviewHost&)const;
 void advance_after_team_tech_researched(const ui::SelectedBnwPreviewHost&)const;
 void advance_player_research_and_first_reward(const ui::SelectedBnwPreviewHost&)const;
 void advance_announcements_and_sharing(const ui::SelectedBnwPreviewHost&)const;
 void advance_espionage_prefix(const ui::SelectedBnwPreviewHost&)const;
 void advance_happiness_prefix(const ui::SelectedBnwPreviewHost&)const;
 void advance_happiness_suffix(const ui::SelectedBnwPreviewHost&)const;
 void advance_technology_ui_prefix(const ui::SelectedBnwPreviewHost&)const;
 void advance_technology_reward_ui(const ui::SelectedBnwPreviewHost&,const BnwSelectedUserOptionsOwner&)const;
 void advance_gameplay_tech_acquired(const ui::SelectedBnwPreviewHost&)const;
 std::int64_t begin_happiness_founder_dispatch(const ui::SelectedBnwPreviewHost&)const;
 void finish_happiness_founder_dispatch(const ui::SelectedBnwPreviewHost&,std::optional<std::int64_t>)const;
};
}
