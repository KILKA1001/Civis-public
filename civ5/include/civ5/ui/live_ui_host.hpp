#pragma once

#include "civ5/ui/minimap_panel.hpp"
#include "civ5/ui/bnw_major_tech_dirty_snapshot.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include "civ5/ui/live_ui_audio.hpp"
#include "civ5/ui/bnw_golden_age_gameplay_event.hpp"
#include "civ5/ui/bnw_game_turn_changed_event.hpp"
#include "civ5/ui/popup_screens.hpp"
#include "civ5/ui/progression_screens.hpp"
#include "civ5/ui/top_panel_live.hpp"
#include "civ5/ui/world_view.hpp"
#include <array>
#include <atomic>
#include <thread>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <utility>

namespace civ5::runtime {class BnwRetainedTurnCursor;class BnwResearchChoiceCommit;enum class BnwResearchChoiceDirty;}
namespace civ5::runtime {enum class BnwTechAwardContextEffect;class BnwMajorTechTransportRequest;class BnwMajorTechClientEndpoint;struct BnwMajorTechUiTransportEvent;}
namespace civ5::runtime {class BnwOwnedGameplayEventPumpLoan;class BnwSelectedStockLuaSourceOwner;}
namespace civ5::ui {

class BnwClientSelectionControl;
class BnwEngineUiTurnOwner;struct BnwEngineUiSummaryEvent;struct BnwEngineUiNotificationEffect;
class BnwOwnedNotificationPublicationOwner;struct BnwNotificationPublicationProgress;
class BnwTechAwardContextOwner;class BnwTechAwardInputDispatch;enum class BnwTechAwardInput;
class BnwClientFrameOwner;
class BnwPlotYieldRefreshUiAdapter;
class BnwPlotYieldRefreshOwnedUiContext;
class BnwPlotYieldRefreshOwnedUiStorage;
class BnwCityProductionUiBackend;
struct CityScreenSource;
struct SystemPediaEntry;

// Captured from the native TechPopup ScrollPanel after its Lua layout pass.
// Row stride does not establish wheel movement; only this owner observation
// can position or enable research-choice hits.
struct TechPopupScrollEvidence {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame = 0, event_token = 0;
    std::string selected_source_path;
    std::int32_t viewport_width = 0, viewport_height = 0;
    std::int32_t scroll_panel_height = 0, vertical_offset_pixels = -1;
    std::size_t choice_count = 0;
};

// The game owner publishes one immutable snapshot per UI frame. Authored
// sources and measured hit rectangles remain owned by the caller/renderer.
struct LiveUiSources {
    const WorldViewSkin *world_skin = nullptr;
    const TopPanelLayout *top_panel_layout = nullptr;
    const MinimapPanelLayout *minimap_layout = nullptr;
    std::array<const ProgressionScreenSource *, 4> progression{};
    TopPanelTextLocalizer top_panel_localizer;
    // Owner of BUTTONPOPUP_CHOOSETECH must acknowledge that popup and publish
    // BUTTONPOPUP_TECH_TREE. `performed` means both native events are observed.
    // A callback invocation is one-shot per owner token even on uncertainty.
    std::function<UiCommandResult(const ProgressionTechPopupEvent &)>
        tech_popup_open_tree;
    // The GameCore/Lua owner must report the currently active TECH_TREE event.
    // The callback dequeues that exact popup and emits PopupProcessed; only
    // after the reader stops exposing its token may local focus close.
    std::function<std::optional<ProgressionTechnologyTreeEvent>()>
        active_tech_tree_event;
    std::function<UiCommandResult(const ProgressionTechnologyTreeEvent &)>
        tech_tree_processed;
    std::function<std::optional<TechPopupScrollEvidence>(
        const UiFrameSnapshot &, std::int32_t, std::int32_t)>
        tech_popup_scroll;
    // BUTTONPOPUP_GOODY_HUT_REWARD is a display of an already awarded reward.
    // The reader returns the native queued event; processed must dequeue that
    // exact token and report an observed PopupProcessed result.
    std::function<std::optional<GoodyHutRewardOwnerEvidence>()>
        active_goody_hut_reward;
    std::function<UiCommandResult(const GoodyHutRewardOwnerEvidence &)>
        goody_hut_processed;
    // Borrow the actual mutable backend retained by the selected Services
    // caller. It serves both const choice queries and typed queue execution;
    // captured choices retain/check this exact backend identity before dispatch.
    // Backend/services must outlive this host; no const_cast or aggregate proxy.
    BnwCityProductionUiBackend *production_backend = nullptr;
    // Borrow the selected XML source used by UiScreenCompositor. Missing or
    // mismatched source disables production rows and scrolling.
    const CityScreenSource *production_popup_source = nullptr;
    // Immutable authored Civilopedia catalog selected by the active resource
    // and language mounts. The host publishes it in each UI frame.
    const std::vector<SystemPediaEntry> *pedia_entries = nullptr;
    // Borrowed selected-profile bridge. Only owner-supplied typed cues are
    // emitted; the observer receives unknown/blocked/sink-refused outcomes.
    // Async WinMM completion remains observable through bridge.device_status().
    LiveUiAudioBridge *audio_bridge = nullptr;
    //Own native-source Civis queue seam. Only actual cue publication/open
    //calls this; repeated frames are deduplicated by the retained queue.
    //Queued here is not device submission/completion; Camera pumps outside
    //render/mutation and observes real backend completion independently.
    std::function<LiveUiAudioResult(std::uint64_t,UiAudioKind,std::string_view)>
        civis_audio_transition;
    std::function<void(UiAudioKind, const UiAuthoredAudioCue &,
                       const LiveUiAudioResult &)> audio_observer;
    // Explicit SAME-controller local queue owner capability. Its proof
    // includes expected parent, current source/session and ALL base fences.
    // Absence preserves the existing separate external-owner contract.
    std::function<bool(const ProgressionTechnologyTreeEvent &)>
        tech_tree_local_dequeue_observed;
};

// Current Civis UI projection of genuine stored research/score fields.
// This is not a selected Original Lua listener or updateScore invocation.
struct BnwResearchChoiceUiSnapshot {
    std::int64_t player=-1,team=-1;std::uint64_t revision=0;
    std::vector<std::int64_t> research_queue;
    std::optional<std::int64_t> player_score,team_score;
};
struct LiveUiFrame {
    UiFrameSnapshot snapshot;
    std::uint64_t sequence = 0;
    const void *owner = nullptr;
    std::optional<WorldViewPlan> world_view;
    std::optional<TopPanelRenderPlan> top_panel_bars;
    std::optional<TopPanelPresentation> top_panel;
    std::optional<MinimapPanelPlan> minimap;
    std::optional<ProgressionRenderPlan> progression;
    struct PolicyScrollView {
        std::int64_t player_id = -1;
        std::int32_t turn = -1;
        std::int32_t viewport_width = 0, viewport_height = 0;
        std::string selected_source_path;
        std::map<std::string, std::int32_t> offsets;
    };
    std::optional<PolicyScrollView> policy_scroll;
    std::optional<TechPopupScrollEvidence> tech_popup_scroll;
    enum class IdeologyOverlay { none, tenets, confirmation };
    struct IdeologyView {
        std::int64_t player_id = -1, branch_id = -1;
        std::int32_t turn = -1, viewport_width = 0, viewport_height = 0;
        std::uint64_t event_token = 0;
        std::string selected_source_path;
        std::int32_t list_offset = 0, tenet_offset = 0;
        IdeologyOverlay overlay = IdeologyOverlay::none;
        // RegisterCollapseBehavior starts expanded and retains panel hide
        // state while this popup owner remains alive.
        std::array<bool, 3> tenet_collapsed{};
    };
    std::optional<IdeologyView> ideology_view;
    bool technology_tree_transition_ready = false;
    bool technology_tree_close_ready = false;
};

struct PolicyScrollRegion {
    WorldViewRect bounds;
    std::string panel_id, selected_source_path;
    std::int32_t offset = 0, max_offset = 0;
};

struct TechPopupScrollRegion {
    WorldViewRect bounds;
    std::string selected_source_path;
    std::int64_t player_id = -1;
    std::int32_t turn = -1, viewport_width = 0, viewport_height = 0;
    std::uint64_t frame = 0, event_token = 0;
    std::int32_t scroll_panel_height = 0, vertical_offset_pixels = -1;
    std::size_t choice_count = 0;
};

struct IdeologyScrollRegion {
    WorldViewRect bounds;
    std::int64_t player_id = -1;
    std::int32_t turn = -1, viewport_width = 0, viewport_height = 0;
    std::uint64_t event_token = 0;
    std::string selected_source_path;
    LiveUiFrame::IdeologyOverlay overlay = LiveUiFrame::IdeologyOverlay::none;
    std::int32_t offset = 0, max_offset = 0;
};

// TopPanel text widths and popup parent transforms are measured by the 2D
// presenter. It returns screen-space hit rectangles with their typed command.
// The host never invents these coordinates from a diagnostic preview.
struct LiveUiHit {
    WorldViewRect bounds;
    std::string context_id;
    UiCommand command;
    bool enabled = true;
    std::string unavailable_reason;
};

struct LiveUiInputResult {
    bool consumed = false;
    UiCommandResult command;
};

class LiveUiHost {
  public:
    using FrameProvider = std::function<UiFrameSnapshot()>;

    LiveUiHost(UiContextRegistry registry, FrameProvider provider,
               UiGameplayBackend *backend, LiveUiSources sources = {});
    // The caller retains this controller beyond the host. Popup owners that
    // queue through it must share the same focus and visibility stack.
    LiveUiHost(UiScreenController &screens, FrameProvider provider,
               UiGameplayBackend *backend, LiveUiSources sources = {});
    LiveUiHost(const LiveUiHost &) = delete;
    LiveUiHost &operator=(const LiveUiHost &) = delete;
    LiveUiHost(LiveUiHost &&) = delete;
    LiveUiHost &operator=(LiveUiHost &&) = delete;

    // Typed button/key command from this SAME actual issued modal frame.
    // It reserves an opaque once token; only the real SAMEVM SourceOwner can
    // execute it. Completion requires a newly issued actual closed UI frame.
    std::shared_ptr<const BnwTechAwardInputDispatch> prepare_tech_award_input(const LiveUiFrame&,BnwTechAwardInput);
    void complete_tech_award_input_after_frame(const std::shared_ptr<const BnwTechAwardInputDispatch>&,const LiveUiFrame&);
    // Camera catch boundary after dispatch: denial only, never grants completion.
    // Authenticates actual issued token/control and SAME UiHost without callbacks.
    std::vector<BnwEngineUiSummaryEvent> engine_ui_summaries()const;
    std::vector<BnwEngineUiNotificationEffect> engine_ui_notifications()const;
    std::optional<BnwNotificationPublicationProgress> notification_publication_progress()const;
    std::vector<BnwEngineUiGoldenAgeGameplayEvent> engine_ui_applied_golden_age_events()const;
    std::vector<BnwGameTurnChangedEvent> engine_ui_applied_game_turn_changed_events()const;
    std::optional<std::int32_t> engine_ui_applied_game_turn()const;
    void retire_tech_award_input_after_failure(const std::shared_ptr<const BnwTechAwardInputDispatch>&) noexcept;
    LiveUiFrame begin_frame(std::int32_t viewport_width, std::int32_t viewport_height);
    UiCommandResult dispatch(const UiCommand &command, const LiveUiFrame &frame);
    UiCommandResult search_civilopedia(std::string_view query, const LiveUiFrame &frame);
    LiveUiInputResult wheel_civilopedia_article(const LiveUiFrame &frame,
        std::int32_t x, std::int32_t y, std::int32_t wheel_delta,
        WorldViewRect scroll_panel);
    UiCommandResult edit_popup_text(std::string_view text, const LiveUiFrame &frame);
    UiCommandResult dispatch_top_panel(TopPanelAction action, const LiveUiFrame &frame);
    LiveUiInputResult click(const LiveUiFrame &frame, std::int32_t x, std::int32_t y,
                            std::span<const LiveUiHit> measured_hits = {});
    // The presenter supplies the selected ScrollPanel's measured screen bounds.
    // Positive wheel delta scrolls up, negative scrolls down.
    LiveUiInputResult wheel_production_popup(const LiveUiFrame &frame,
        std::int32_t x, std::int32_t y, std::int32_t wheel_delta,
        WorldViewRect measured_scroll_panel);
    LiveUiInputResult wheel_popup(const LiveUiFrame &frame,
        std::int32_t x, std::int32_t y, std::int32_t wheel_delta,
        WorldViewRect scroll_panel);
    LiveUiInputResult wheel_policy(const LiveUiFrame &frame,
        std::int32_t wheel_delta, const PolicyScrollRegion &region);
    LiveUiInputResult wheel_tech_popup(const LiveUiFrame &frame,
        std::int32_t x, std::int32_t y, std::int32_t wheel_delta,
        const TechPopupScrollRegion &region);
    LiveUiInputResult wheel_ideology(const LiveUiFrame &frame,
        std::int32_t x, std::int32_t y, std::int32_t wheel_delta,
        const IdeologyScrollRegion &region);

    // Local CityView selection belongs to this host and clears on Close.
    // The client frame owner reads it before publication; UI projection must
    // not silently select a different city after the world stamp is frozen.
    std::optional<std::pair<std::int64_t, std::int64_t>> selected_city() const;
    BnwClientUnitDirtySnapshot unit_dirty_state()const;
    std::vector<BnwClientUnitDirtyEvent> take_unit_dirty_events();
    const UiScreenController &screens() const { return screens_; }
    bool is_current_frame(const LiveUiFrame &frame) const { return current(frame); }

  private:
    friend class BnwPlotYieldRefreshUiAdapter;
    std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage> owned_plot_yield_ui_sources_;
    friend class BnwPlotYieldRefreshOwnedUiContext;
    friend class UiScreenGlRenderer;
    void require_issued_frame_finality(const LiveUiFrame&)const;
    // Own-C++ CityScreen dirty deliveries retain each source occurrence. The
    // adapter completes an entry only after this host's actual begin_frame
    // consumes a newly captured SAME SceneFrame/provider publication.
    struct PlotCityDirtyOccurrence {
        std::uint64_t operation=0,revision=0,old_sequence=0,new_sequence=0;
        std::int64_t plot=-1,player=-1,city=-1;
        std::uint8_t yield=0;
        bool consumed=false;
    };
    // Adapter-only private issue record. Legacy current()/sanitized-frame
    // contracts remain unchanged. Publish only the final real begin_frame value.
    std::shared_ptr<const UiFrameSnapshot> issued_plot_yield_snapshot_;
    std::uint64_t issued_plot_yield_sequence_=0;
    std::optional<std::pair<std::int64_t,std::int64_t>> issued_plot_yield_selection_;
    std::uint64_t plot_city_dirty_revision_=0;
    const std::thread::id plot_yield_ui_thread_=std::this_thread::get_id();
    std::atomic<bool> plot_yield_ui_busy_{false},plot_yield_ui_poisoned_{false};
    std::vector<PlotCityDirtyOccurrence> plot_city_dirty_queue_;
    friend class BnwTechAwardInputDispatch;
    friend class BnwClientFrameOwner;
    friend class BnwClientSceneFrame;
    friend class BnwOwnedCameraGuiScreen;
    friend class BnwAlphaProgressionBackend;
    friend class UiScreenCompositor;
    friend class runtime::BnwRetainedTurnCursor;
    friend class runtime::BnwSelectedStockLuaSourceOwner;
    friend class BnwEngineUiTurnOwner;
    friend class BnwOwnedNotificationPublicationOwner;
    friend class runtime::BnwOwnedGameplayEventPumpLoan;
    friend class runtime::BnwMajorTechClientEndpoint;
    friend class runtime::BnwMajorTechTransportRequest;
    std::weak_ptr<const runtime::BnwMajorTechClientEndpoint> major_tech_endpoint_;
    BnwMajorTechDirtySnapshot major_tech_dirty_;
    std::shared_ptr<const runtime::BnwMajorTechClientEndpoint> create_major_tech_endpoint(const LiveUiFrame&);
    void require_major_tech_receiver_current(const runtime::BnwMajorTechClientEndpoint&)const;
    void consume_major_tech_dirty(const runtime::BnwMajorTechUiTransportEvent&,const runtime::BnwMajorTechClientEndpoint&);
    void publish_major_tech_dirty(LiveUiFrame&);
 void publish_tech_award_lifecycle(LiveUiFrame&);
 std::weak_ptr<const BnwTechAwardContextOwner> tech_award_context_;
 std::uint64_t tech_award_context_occurrence_=0,tech_award_context_generation_=0;
 void bind_actual_tech_award_context(const runtime::BnwMajorTechTransportRequest&,
    const runtime::BnwMajorTechClientEndpoint&,const std::shared_ptr<const BnwTechAwardContextOwner>&);
 void publish_actual_tech_award_context(LiveUiFrame&);
 UiFrameSnapshot snapshot_for_major_tech_context(const runtime::BnwMajorTechClientEndpoint&)const;
 struct TechAwardInputSlot {
  std::uint64_t occurrence,generation,world_revision;
  std::string native_control;
 };
 std::vector<TechAwardInputSlot> tech_award_inputs_;
 std::weak_ptr<const BnwTechAwardInputDispatch> tech_award_pending_input_;
 std::uint64_t tech_award_input_occurrence_=0;
 void register_tech_award_input_slot(const runtime::BnwMajorTechTransportRequest&,
    const runtime::BnwMajorTechClientEndpoint&,std::string_view);
 // Complete event type is owned in runtime consumer TU; avoid a UI-header SCC.
 struct TechAwardLifecycleStorage;
 std::shared_ptr<TechAwardLifecycleStorage> tech_award_lifecycle_;
 void apply_tech_award_context_effect(const runtime::BnwMajorTechTransportRequest&,
    const runtime::BnwMajorTechClientEndpoint&,runtime::BnwTechAwardContextEffect);

    struct ResearchHandlers {
        std::array<const UiContextDescriptor*,3> contexts{};
        std::shared_ptr<const BnwResearchChoiceUiSnapshot> snapshot;
        std::uint64_t sequence=0;
    };
    std::optional<ResearchHandlers> research_handlers_;
    void register_owned_research_handlers();
    void require_owned_research_handlers(const LiveUiFrame&,std::int64_t)const;
    void consume_owned_research_dirty(runtime::BnwResearchChoiceDirty,const LiveUiFrame&,
                                     const runtime::BnwResearchChoiceCommit&);
    std::shared_ptr<BnwClientSelectionControl> city_selection_control_;
    mutable std::shared_ptr<BnwEngineUiTurnOwner> engine_ui_turn_owner_;
    mutable std::shared_ptr<BnwOwnedNotificationPublicationOwner> notification_publication_owner_;
    mutable std::vector<BnwEngineUiGoldenAgeGameplayEvent> applied_golden_age_events_;
    mutable std::vector<BnwGameTurnChangedEvent> applied_game_turn_changed_events_;
    mutable std::optional<std::int32_t> applied_game_turn_;
    friend class BnwTechAwardAnimationWitness;
    friend class UiScreenGlRenderer;
    std::weak_ptr<LiveUiHost> selected_owner_lifetime_;
    void set_selected_city(std::optional<std::pair<std::int64_t,std::int64_t>>);
    bool current(const LiveUiFrame &frame) const;
    UiCommandResult route(const UiCommand &command, const LiveUiFrame &frame);
    LiveUiInputResult activate(const UiCommand &command, bool enabled,
                               std::string_view reason, const LiveUiFrame &frame);
    void emit_audio(UiAudioKind kind, const UiAuthoredAudioCue &cue);
    //Only actual same-owner UI publication ingress/Engine friend can issue
    //this shared stream. Native ring indices/Gameplay occurrences are not
    //audio sequence numbers. Counter reserves before callback, even failure.
    LiveUiAudioResult publish_civis_audio_cue(UiAudioKind,std::string_view native_id)const;
    mutable std::uint64_t civis_audio_source_sequence_=0;
    std::map<UiAudioKind,std::uint64_t> last_authored_audio_cue_;

    std::optional<UiScreenController> owned_screens_;
    UiScreenController &screens_;
    FrameProvider provider_;
    UiGameplayBackend *backend_;
    LiveUiSources sources_;
    std::uint64_t sequence_ = 0;
    struct ProductionScrollState {
        std::int64_t player_id = -1, city_id = -1;
        std::string selected_source_path;
        std::size_t first_visible = 0;
    };
    std::optional<ProductionScrollState> production_scroll_;
    // UI selection is kept outside the immutable game snapshot. The city ID
    // is revalidated against each newly published live frame.
    std::optional<std::pair<std::int64_t, std::int64_t>> selected_city_;
    struct PopupScrollState {
        std::string context_id;
        std::int64_t player_id = -1, event_id = -1;
        std::size_t choice_count = 0, first_visible = 0;
        std::int32_t viewport_width = 0, viewport_height = 0;
    };
    std::optional<PopupScrollState> popup_scroll_;
    std::optional<PantheonConfirmView> pantheon_confirm_;
    std::optional<ReligionSelectionView> religion_selection_;
    std::int32_t viewport_width_ = 0, viewport_height_ = 0;
    std::optional<LiveUiFrame::PolicyScrollView> policy_scroll_;
    std::optional<LiveUiFrame::IdeologyView> ideology_view_;
    std::optional<std::pair<std::int64_t, std::uint64_t>> attempted_tech_tree_event_;
    std::optional<std::pair<std::int64_t, std::uint64_t>> attempted_tech_tree_close_;
    std::optional<std::pair<std::int64_t, std::uint64_t>> attempted_goody_hut_close_;
    std::optional<PopupTextView> popup_text_;
    std::optional<std::string> selected_pedia_id_;
    std::size_t pedia_page_ = 0;
    std::uint8_t pedia_category_tab_ = 0;
    std::string pedia_search_;
    std::vector<std::string> pedia_history_;
    std::size_t pedia_history_index_ = 0;
    std::size_t pedia_article_first_line_ = 0;
};

} // namespace civ5::ui
