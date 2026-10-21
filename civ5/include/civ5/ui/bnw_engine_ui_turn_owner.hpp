#pragma once
#include "civ5/ui/detail/bnw_label_system_font_language_data.hpp"
#include "civ5/ui/bnw_golden_age_gameplay_event.hpp"
#include "civ5/ui/bnw_game_turn_changed_event.hpp"
#include "civ5/ui/bnw_engine_ui_end_turn_queue.hpp"
namespace civ5::runtime {class BnwPlayerTurnActiveDirtyIssuer;class BnwSelectedWorldViewContextOwner;}
namespace civ5::ui {class BnwNotificationOwnerModelLoan;}
namespace civ5::runtime {class BnwOwnedGameplayEventPumpLoan;class BnwNativeAttritionPrefix;}
#include <cstdint>
#include <array>
#include <utility>
#include <filesystem>
#include <string_view>
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace civ5::runtime {class BnwPolicyAdoptionPending;struct BnwPolicyBranchNotificationHistory;class BnwSelectedStockLuaSourceOwner;class BnwRetainedTurnCursor;class BnwSelectedStockLuaBootstrap;struct SavedContentProfile;struct LiveSessionState;struct SessionCheckpointIdentity;struct ScriptLanguage;}
namespace civ5::content {struct Catalog;}
namespace civ5::runtime {class BnwStockLuaAttemptLease;class BnwOwnedGameplayEventPumpLoan;}
namespace civ5::runtime {class BnwLivePlayerGoldPending;enum class TreasuryDirty;}
namespace civ5::runtime {class BnwAttritionCityInfoDirtyLoan;class BnwAttritionUnitInfoDirtyLoan;class BnwAttritionUnitSelectedRead;class BnwAttritionSelectedUnitInfoDirtyLoan;class BnwAttritionSelectionPlotRead;class BnwAttritionPlotListDirtyLoan;}
namespace civ5::ui {
class LiveUiHost;class SelectedBnwPreviewHost;struct LiveUiFrame;class BnwOwnedNotificationPublicationOwner;
// Actual new Civis client storage, not an imported retail UI cache or a
// public action capability. It persists in its genuine LiveUiHost.
struct BnwEngineUiRepetitionSnapshot {
    std::int32_t start_diplo_repeat_count=0,offer_trade_repeat_count=0;
};
// Real captured operands only. Missing native comparator/aggregate/localized
// field producers remain optional; this snapshot is never a completion grant.
struct BnwEngineUiCitySummaryOperands {
 std::int32_t owner=-1,city=-1,population=0,strength_times100=0;
 std::string source_name;
 std::uint64_t generation=0,world_revision=0;
};
struct BnwEngineUiNetGoldSnapshot {
 std::int32_t actor=-1,net_times100=0,base_net_gold=0;
 std::uint64_t generation=0,world_revision=0,occurrence=0;
};
struct BnwEngineUiDeficitPending {
 std::int32_t actor=-1,turn=-1,net_times100=0;
 std::uint32_t native_type=0xbe9725c6;
 std::string message_key="TXT_KEY_NTFN_INCOME_DEFICIT",summary_key="TXT_KEY_NTFN_INCOME_DEFICIT_S";
 std::uint64_t generation=0,world_revision=0,occurrence=0;
 // Actual source request. Only the private selected-text/captured-define formatter may consume it.
 // It is not a created CvNotifications slot, UI broadcast, FX or minimap ACK.
};
// Semantic 5EA0C0 payload: explicitly excludes native padding8A3.
struct BnwEngineUiCityRow {
 std::array<char,128> name{};
 std::array<std::uint16_t,8> fields{};
 bool operator==(const BnwEngineUiCityRow&)const=default;
};
struct BnwEngineUiOpponentRow {
 std::array<char,128> name{};
 std::uint16_t status=0,score=0;
 bool operator==(const BnwEngineUiOpponentRow&)const=default;
};
struct BnwEngineUiSummary {
 std::array<BnwEngineUiCityRow,7> cities{};
 std::array<BnwEngineUiOpponentRow,9> opponents{};
 // Constructor final memset zeroes [0,894); earlier row sentinels DO NOT survive.
 std::uint16_t ever_alive=UINT16_MAX,alive_capitals=UINT16_MAX,apollo_teams=UINT16_MAX;
 std::uint8_t max_apollo=UINT8_MAX,home_un=0;
 std::uint16_t secured_votes=UINT16_MAX,voting_teams=UINT16_MAX,votes_needed=UINT16_MAX;
 std::uint8_t max_finished_branches=UINT8_MAX;
 std::uint32_t turns_left=UINT32_MAX;
 bool operator==(const BnwEngineUiSummary&)const=default;
};
struct BnwEngineUiSummaryEvent {
 std::uint32_t native_message=0xa8;
 std::int32_t actor=-1;
 std::uint64_t generation=0,world_revision=0,occurrence=0;
 BnwEngineUiSummary value;
 // Genuine retained UiHost message. Publication is not physical paint/present.
};
enum class BnwEngineUiNotificationEffectKind {remove_notification,add_notification,gameplay_fx_null,gameplay_minimap_notification};
struct BnwEngineUiNotificationEffect {
 BnwEngineUiNotificationEffectKind kind=BnwEngineUiNotificationEffectKind::add_notification;
 std::int32_t actor=-1,lookup=-1,turn=-1,x=-1,y=-1,game_data_index=-1,extra_game_data=-1;
 std::uint32_t type=0xbe9725c6;
 std::uint64_t generation=0,world_revision=0,occurrence=0;
 std::string message,summary;
 bool operator==(const BnwEngineUiNotificationEffect&)const=default;
 // Actual canonical Civis UI/DLL effect submission, not physical FX or paint.
};
class BnwEngineUiTurnOwner final {
public:
    ~BnwEngineUiTurnOwner();
    BnwEngineUiTurnOwner(const BnwEngineUiTurnOwner&)=delete;
    BnwEngineUiTurnOwner&operator=(const BnwEngineUiTurnOwner&)=delete;
    BnwEngineUiRepetitionSnapshot repetition_snapshot()const;
    std::vector<BnwEngineUiCitySummaryOperands> active_city_operands()const;
    std::vector<BnwEngineUiNetGoldSnapshot> net_gold_queries()const;
    std::vector<BnwEngineUiDeficitPending> pending_deficits()const;
    std::vector<BnwEngineUiSummaryEvent> queued_summaries()const;
    std::vector<BnwEngineUiNotificationEffect> queued_notifications()const;
    std::vector<BnwEngineUiGoldenAgeGameplayEvent> queued_golden_age_gameplay_events()const;
    std::vector<BnwGameTurnChangedEvent> queued_game_turn_changed_events()const;
private:
    friend class BnwEngineUiEndTurnQueue;
    friend class LiveUiHost;
    friend class BnwOwnedNotificationPublicationOwner;
    friend class runtime::BnwRetainedTurnCursor;
    friend class runtime::BnwPolicyAdoptionPending;
    friend class runtime::BnwSelectedStockLuaSourceOwner;
    friend class runtime::BnwSelectedStockLuaBootstrap;
    friend class runtime::BnwSelectedWorldViewContextOwner;
    friend class runtime::BnwNativeAttritionPrefix;
    struct Impl;struct EndTurnPin;
    class EndTurnCursorPin final {
      friend class runtime::BnwRetainedTurnCursor;
      std::shared_ptr<Impl> storage_;
      EndTurnCursorPin(const BnwEngineUiTurnOwner&,const LiveUiHost&,const runtime::LiveSessionState&);
      ~EndTurnCursorPin();
      EndTurnCursorPin(const EndTurnCursorPin&)=delete;
      EndTurnCursorPin&operator=(const EndTurnCursorPin&)=delete;
    };
    std::shared_ptr<Impl> impl_;
    explicit BnwEngineUiTurnOwner(std::shared_ptr<Impl>);
    static std::shared_ptr<BnwEngineUiTurnOwner> bind(const LiveUiHost&,
        std::shared_ptr<const SelectedBnwPreviewHost>);
    void require_current()const;
    static void consume_attrition_city_info_dirty_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,const runtime::BnwAttritionCityInfoDirtyLoan&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
    static void consume_attrition_unit_info_dirty_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,const runtime::BnwAttritionUnitInfoDirtyLoan&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
    static bool consume_attrition_unit_selected_read_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,const runtime::BnwAttritionUnitSelectedRead&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
    static void consume_attrition_selected_unit_info_dirty_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,const runtime::BnwAttritionSelectedUnitInfoDirtyLoan&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
    static std::optional<std::int32_t> consume_attrition_selection_plot_read_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,const runtime::BnwAttritionSelectionPlotRead&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
    static void consume_attrition_plot_list_dirty_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,const runtime::BnwAttritionPlotListDirtyLoan&,runtime::LiveSessionState&,const runtime::BnwStockLuaAttemptLease&);
    static std::shared_ptr<BnwEngineUiTurnOwner> retain_for_actual_policy_ui(const std::shared_ptr<const LiveUiHost>&,std::shared_ptr<const SelectedBnwPreviewHost>);
    void consume_live_gold_dirty(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwLivePlayerGoldPending>&,runtime::TreasuryDirty,const runtime::BnwStockLuaAttemptLease&);
    void consume_policy_process_dirty(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,const runtime::BnwStockLuaAttemptLease&);
    void consume_outer_branch_notifications(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,runtime::BnwSelectedStockLuaBootstrap&,const runtime::BnwStockLuaAttemptLease&);
    runtime::BnwPolicyBranchNotificationHistory require_outer_branch_notification_return(const runtime::BnwPolicyAdoptionPending&)const;
    bool add_outer_branch_native(runtime::LiveSessionState&,std::string_view,std::string_view,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,const runtime::BnwStockLuaAttemptLease&);
    void consume_free_policy_notification(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,runtime::BnwSelectedStockLuaBootstrap&);
    void require_free_policy_notification_return(const runtime::BnwPolicyAdoptionPending&)const;
    bool add_free_policy_native(runtime::LiveSessionState&,std::string_view,std::string_view,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&);

    std::uint64_t reset_fresh_end_turn_flags();
    std::uint64_t reset_end_turn_flags_owned(const runtime::BnwStockLuaAttemptLease&);
    std::uint64_t reset_end_turn_flags_impl(const runtime::BnwStockLuaAttemptLease*);
    void ensure_end_turn_queue_owned();
    void consume_player_turn_dirty(const std::shared_ptr<const runtime::BnwPlayerTurnActiveDirtyIssuer>&,const runtime::BnwStockLuaAttemptLease&);
    friend class BnwNotificationOwnerModelLoan;
    void require_notification_model_origin_without_callbacks(const BnwOwnedNotificationPublicationOwner&,const runtime::BnwOwnedGameplayEventPumpLoan&,const runtime::BnwStockLuaAttemptLease&)const;
    BnwEngineUiNotificationEffect notification_model_head_without_callbacks(const BnwOwnedNotificationPublicationOwner&)const;
    std::weak_ptr<BnwEngineUiTurnOwner> end_turn_engine_for_bootstrap_owned()const;
    void attach_end_turn_consumer_owned(std::shared_ptr<const BnwEngineUiEndTurnQueue::Consumer>,const runtime::BnwStockLuaAttemptLease&);
    static void attach_end_turn_consumer_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,
        std::shared_ptr<const BnwEngineUiEndTurnQueue::Consumer>,const runtime::BnwStockLuaAttemptLease&);
    void drain_end_turn_queue_owned(const runtime::BnwOwnedGameplayEventPumpLoan&,const runtime::BnwStockLuaAttemptLease&);
    static void drain_end_turn_queue_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,
        const runtime::BnwOwnedGameplayEventPumpLoan&,const runtime::BnwStockLuaAttemptLease&);
    void require_end_turn_queue_identity(const std::weak_ptr<const LiveUiHost>&,
        const std::weak_ptr<const SelectedBnwPreviewHost>&,const std::weak_ptr<const runtime::LiveSessionState>&)const;
    void require_fresh_end_turn_flags_return(std::uint64_t)const;
    void consume_outer_policy_dirty(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,const runtime::BnwStockLuaAttemptLease&);
    void consume_free_tech_text(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,runtime::BnwSelectedStockLuaBootstrap&,const runtime::BnwStockLuaAttemptLease&);
    void require_free_tech_text_return(const runtime::BnwPolicyAdoptionPending&)const;
    void consume_free_tech_dirty(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,const runtime::BnwStockLuaAttemptLease&);
    void consume_free_tech_notification(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,const runtime::BnwStockLuaAttemptLease&);
    void require_free_tech_notification_return(const runtime::BnwPolicyAdoptionPending&)const;
    bool add_free_tech_native(runtime::LiveSessionState&,std::string_view,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&);
    void consume_reformation_notification(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,runtime::BnwSelectedStockLuaBootstrap&,const runtime::BnwStockLuaAttemptLease&);
    void consume_resource_policy_layout(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,const runtime::BnwStockLuaAttemptLease&);
    void consume_happiness_dirty(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,const runtime::BnwStockLuaAttemptLease&);
    void require_reformation_notification_return(const runtime::BnwPolicyAdoptionPending&)const;
    bool add_reformation_native(runtime::LiveSessionState&,std::string_view,std::string_view,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,const runtime::BnwStockLuaAttemptLease&);
    void consume_golden_age_dirty(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,const runtime::BnwStockLuaAttemptLease&);
    void consume_golden_age_replay(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,runtime::BnwSelectedStockLuaBootstrap&,const runtime::BnwStockLuaAttemptLease&);
    void require_golden_age_replay_return(const runtime::BnwPolicyAdoptionPending&)const;
    void consume_golden_age_gameplay(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,const runtime::BnwStockLuaAttemptLease&);
    void require_golden_age_gameplay_return(const runtime::BnwPolicyAdoptionPending&)const;
    void consume_golden_age_notification(runtime::LiveSessionState&,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&,runtime::BnwSelectedStockLuaBootstrap&,const runtime::BnwStockLuaAttemptLease&);
    void require_golden_age_notification_return(const runtime::BnwPolicyAdoptionPending&)const;
    bool add_golden_age_native(runtime::LiveSessionState&,std::string_view,std::string_view,const std::shared_ptr<const runtime::BnwPolicyAdoptionPending>&);
    static void drain_notification_publication_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,
        const runtime::BnwOwnedGameplayEventPumpLoan&,const runtime::BnwStockLuaAttemptLease&);
    void drain_notification_publication_owned(const runtime::BnwOwnedGameplayEventPumpLoan&,const runtime::BnwStockLuaAttemptLease&);
    static void merge_notification_frame_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,const LiveUiHost&,LiveUiFrame&);
    void merge_notification_frame_owned(const LiveUiHost&,LiveUiFrame&);
    void drain_golden_age_gameplay_owned(const runtime::BnwOwnedGameplayEventPumpLoan&,const runtime::BnwStockLuaAttemptLease&);
    static void publish_game_turn_changed_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,runtime::BnwRetainedTurnCursor&,runtime::LiveSessionState&,const runtime::BnwSelectedStockLuaSourceOwner&,const runtime::BnwStockLuaAttemptLease&);
    void publish_game_turn_changed_owned(runtime::BnwRetainedTurnCursor&,runtime::LiveSessionState&,const runtime::BnwSelectedStockLuaSourceOwner&,const runtime::BnwStockLuaAttemptLease&);
    void require_game_turn_changed_return(const runtime::BnwRetainedTurnCursor&,const runtime::LiveSessionState&,std::uint64_t,std::uint64_t,std::uint64_t)const;
    static void drain_game_turn_changed_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,const runtime::BnwOwnedGameplayEventPumpLoan&,const runtime::BnwStockLuaAttemptLease&);
    void drain_game_turn_changed_owned(const runtime::BnwOwnedGameplayEventPumpLoan&,const runtime::BnwStockLuaAttemptLease&);
    static void drain_golden_age_gameplay_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,
        const runtime::BnwOwnedGameplayEventPumpLoan&,const runtime::BnwStockLuaAttemptLease&);
    void reset_repetition_counters();
    void capture_active_city_operands(std::int32_t);
    BnwEngineUiSummary build_active_summary(std::int32_t)const;
    void publish_active_summary(std::int32_t);
    bool has_selected_city_name_keys(std::int32_t)const;
    void bind_selected_localization(const std::filesystem::path&,const runtime::SavedContentProfile&,const content::Catalog&,const std::shared_ptr<const runtime::LiveSessionState>&,const runtime::SessionCheckpointIdentity&);
    void require_selected_localization_current()const;
    std::string localized_city_name(std::string_view)const;
    std::vector<BnwEngineUiNetGoldSnapshot> prepare_net_gold_observation(std::int32_t,std::int32_t,std::uint64_t)const;
    void commit_net_gold_observation(std::vector<BnwEngineUiNetGoldSnapshot>&)noexcept;
    void format_pending_deficit();
    std::string selected_localization_text(std::string_view)const;
    static void require_counted_localization_owner(const EndTurnPin&);
    static std::string selected_localization_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,std::string_view);
    static std::string selected_action_info_lookup_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&,std::string_view);
    static runtime::ScriptLanguage selected_localization_language_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&);
    static detail::BnwLabelSystemFontLanguageData system_font_language_data_from_owned_weak(const std::weak_ptr<BnwEngineUiTurnOwner>&);
    // Private actual selected Localization database, never raw XML/defaults.
    static std::pair<std::string,std::uint8_t> selected_localization_extended_font_from_owned_weak(
        const std::weak_ptr<BnwEngineUiTurnOwner>&);
    bool add_formatted_deficit(runtime::LiveSessionState&);
    void record_pending_deficit(std::int32_t,std::int32_t,std::int32_t,std::uint64_t);
};
}
