#pragma once

#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include "civ5/graphics/bnw_unit_setup_graphical_transport.hpp"
#include "civ5/graphics/bnw_rendered_fog_frame.hpp"
#include "civ5/graphics/bnw_clear_hex_highlight_owner.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <stdexcept>
#include <atomic>
#include <thread>
#include <utility>

namespace civ5::graphics { class BnwPendingUnitVisualOwner; class BnwSelectedPendingUnitVisualLoan;class BnwTrainPresentedFrameProof; class BnwInitialCenterUnitRendererStorage; class BnwInitialCenterUnitRendererLoan; class BnwInitialUnitShouldDimRendererLoan; class BnwUnitShouldDimQueue; }
namespace civ5::runtime { class BnwOwnedGameplayEventPumpLoan; class BnwSelectedStockLuaBootstrap; class BnwStockLuaAttemptLease; class BnwSelectedStockLuaSourceOwner; class BnwNativeUnitAllocatorCore; class BnwUnitMovePending; }
namespace civ5::graphics {class BnwUnitDamageCommandOwner;class BnwInitialUnitSetupGraphicalRendererLoan;class BnwInitialUnitSetupGraphicalEnqueueProof;class BnwUnitSetupGraphicalQueue;class BnwUnitSetupGraphicsState;}

namespace civ5::ui {
class BnwOwnedCameraGuiScreen;
class BnwPlotYieldRefreshUiAdapter;
class BnwPlotYieldRefreshOwnedUiContext;
class BnwPlotYieldRefreshOwnedUiStorage;

// Consumer boundary for the current immutable GPU scene. Publishing a live
// frame does not update terrain/art buffers. Every draw, swap and input must
// retain the receipt of the scene actually prepared by the renderer.
class BnwClientSceneFrame {
  public:
    static std::shared_ptr<BnwClientSceneFrame> create_selected_screen_scene(
        std::shared_ptr<const SelectedBnwPreviewHost> parent,
        const graphics::BnwRendererSceneReceipt& receipt,const LiveSessionUiSources& sources) {
        auto scene=std::shared_ptr<BnwClientSceneFrame>(new BnwClientSceneFrame(std::move(parent),receipt,sources));
        scene->gui_self_=scene;
        scene->setup_graphical_storage_=graphics::BnwUnitSetupGraphicalQueue::bind_actual_scene(*scene);
        scene->setup_graphical_queue_=scene->setup_graphical_storage_;
        return scene;
    }
    BnwClientSceneFrame(const runtime::LiveSession &session,
                       const graphics::BnwRendererSceneReceipt &receipt,
                       std::int64_t player, LiveSessionUiSources sources)
        : session_(session), receipt_(receipt),
          selected_unit_(sources.selected_unit), selected_city_(sources.selected_city),
          owner_(session, receipt.scene_file_sha256, player, std::move(sources)) {}

    // Real selected-host lifetime/source join; this does not attest EARLY,
    // native App/UIManager/Label initialization or GPU upload. Those issuers
    // retain their own private guards. Legacy session/file-SHA route is intact.
    BnwClientSceneFrame(std::shared_ptr<const SelectedBnwPreviewHost> parent,
                       const graphics::BnwRendererSceneReceipt &receipt,
                       const LiveSessionUiSources &sources)
        : selected_host_(require_selected_parent(std::move(parent),receipt)),
          session_(selected_host_->session()), receipt_(receipt),
          selected_unit_(sources.selected_unit), selected_city_(sources.selected_city),
          owner_(selected_host_,checked_selected_sources(
                     selected_host_,receipt_,scene_state_,scene_state_generation_,sources)) {}

    std::shared_ptr<const BnwClientFrame> begin(ClientViewport viewport,
                                              terrain::CameraControllerState camera) {
        const auto retained=pin_owned();
        verify_owned();
        rendered_fog_.reset();
        if(!selected_host_) {
            const auto selected_unit=selected_unit_,selected_city=selected_city_;
            verify_owned();
            const auto unit=selected_unit?selected_unit():std::nullopt;verify_owned();
            const auto city=selected_city?selected_city():std::nullopt;verify_owned();
            owner_.select({unit,city});verify_owned();
        }
        const auto captured=owner_.capture(viewport,std::move(camera));verify_owned();
        frame_=captured;
        if (!frame_) return {};
        if (!ready_to_draw()) {
            const auto reason = graphics::bnw_renderer_scene_revision_mismatch(
                receipt_, frame_->world, frame_->stamp.identity.turn,
                frame_->stamp.identity.player);
            owner_.notify_world_changed();
            throw std::runtime_error("BNW renderer cannot draw live frame: " +
                reason.value_or("live owner changed or scene visibility is stale"));
        }
        return frame_;
    }
    bool ready_to_draw() const {
        const auto retained=pin_owned();const auto actual_frame=frame_;
        const auto &state = session_.state();
        if (&state != scene_state_ || session_.state_generation()!=scene_state_generation_) return false;
        if (!owner_.is_current(actual_frame)) return false;verify_owned();
        const auto player = state.setup.players.find(actual_frame->stamp.identity.player);
        if (!state.plot_visibility.current || player == state.setup.players.end() ||
            !player->second.alive || player->second.team != actual_frame->stamp.identity.team ||
            player->second.team < 0 || static_cast<std::uint64_t>(player->second.team) !=
                receipt_.active_team) return false;
        if (actual_frame->stamp.selection.unit) {
            const auto army = state.maintenance.players.find(actual_frame->stamp.identity.player);
            if (army == state.maintenance.players.end() ||
                !army->second.units.contains(*actual_frame->stamp.selection.unit)) return false;
        }
        if (actual_frame->stamp.selection.city) {
            const auto cities = state.cities.players.find(actual_frame->stamp.identity.player);
            if (cities == state.cities.players.end() ||
                !cities->second.cities.contains(*actual_frame->stamp.selection.city)) return false;
        }
        if (!graphics::matches_bnw_renderer_scene_receipt(receipt_, actual_frame->world,
                actual_frame->stamp.identity.turn, actual_frame->stamp.identity.player) ||
            !graphics::matches_bnw_renderer_scene_receipt(receipt_, state.world,
                state.setup.game_turn, actual_frame->stamp.identity.player)) return false;
        // Visibility is an owned derived cache. A retained mutable reference
        // can change it without changing the decoded world or publication ID.
        // Match the actual live logical fog against the latched UI/scene fog.
        try {
            const auto &cache = runtime::bnw_live_plot_visibility_cache(state);
            if (cache.grid.size() != actual_frame->visibility.plots.size() ||
                receipt_.active_team_art.size() != actual_frame->visibility.plots.size() ||
                !state.world.map_projection) return false;
            for (std::size_t plot = 0; plot < cache.grid.size(); ++plot) {
                const auto &cell = state.world.map_projection->cells.at(plot);
                const auto count = cache.counts.at(plot * cache.wire_slots +
                    static_cast<std::uint64_t>(player->second.team));
                if (count < 0 || !cell.revealed_teams) return false;
                const auto fog = count > 0 ? WorldViewPlotFog::visible :
                    cell.revealed_teams->contains(player->second.team)
                        ? WorldViewPlotFog::explored : WorldViewPlotFog::hidden;
                if (fog != actual_frame->visibility.plots[plot]) return false;
                // Without a bound native journal, retain the immutable fog baseline.
                if (native_fog_) continue;
                // This consumer has the immutable map/fog GPU baseline. A new
                // live cache alone cannot attest a corresponding FOW texture
                // update. A future journal consumer must supply its actually
                // applied native receipt before accepting changed logical fog.
                const auto &art = receipt_.active_team_art[plot];
                if (art.plot_index != plot || !art.revealed) return false;
                const auto rendered_fog = art.visibility_count > 0 ? WorldViewPlotFog::visible :
                    *art.revealed ? WorldViewPlotFog::explored : WorldViewPlotFog::hidden;
                if (rendered_fog != actual_frame->visibility.plots[plot]) return false;
            }
        } catch (const std::exception &) { return false; }
        const bool valid=!native_fog_ || native_fog_->matches(state,receipt_,*actual_frame);
        verify_owned();return valid;
    }
    bool current() const {
        const auto retained=pin_owned();const auto actual_frame=frame_;
        const bool ready=ready_to_draw();verify_owned();
        const bool valid=ready && (!native_fog_ || (rendered_fog_ &&
            rendered_fog_->matches(*actual_frame,receipt_,session_.state())));
        verify_owned();return valid;
    }
    void bind_native_fog(const runtime::BnwDeferredFogOwner &owner,
        const materials::FogOfWarState &native, const graphics::GlFogOfWarMap &map,
        const runtime::SessionCheckpointIdentity &identity,
        const runtime::BnwGameActivationOwner *activation=nullptr) {
        bind_native_fog(graphics::BnwNativeFogFrameBinding{&owner,&native,&map,identity,activation});
    }
    void bind_native_fog(graphics::BnwNativeFogFrameBinding binding) {
        const auto retained=pin_owned();
        if (native_fog_) throw std::logic_error("BNW native fog owner already bound");
        if (!binding.same_outer_owner() || !binding.owner || !binding.native || !binding.map)
            throw std::invalid_argument("BNW native fog binding owner is missing or detached");
        const auto &owner=*binding.owner;const auto &native=*binding.native;
        if (!owner.queue_known || !owner.published || owner.pending ||
            !owner.queued_plots.empty() || owner.published->sequence()!=0 || native.dirty())
            throw std::invalid_argument("BNW native fog binding requires its actual initial baseline");
        native_fog_=std::move(binding);
        rendered_fog_.reset(); owner_.notify_world_changed();
    }

    void accept_rendered_fog(graphics::BnwFogRenderedFrameReceipt rendered) {
        const auto retained=pin_owned();
        if (!native_fog_ || !ready_to_draw() ||
            !rendered.belongs_to(*native_fog_) ||
            !rendered.matches(*frame_,receipt_,session_.state()))
            throw std::invalid_argument("BNW GPU fog receipt is stale or belongs to another frame");
        rendered_fog_=std::move(rendered);
        if (!current()) throw std::invalid_argument("BNW GPU fog publication changed");
    }
    const graphics::BnwFogRenderedFrameReceipt *fog_receipt() const {
        const auto retained=pin_owned();
        return native_fog_ && current() && rendered_fog_ ? &*rendered_fog_ : nullptr;
    }
    bool accepts(const LiveUiHost &host, const LiveUiFrame &ui_frame) const {
        const auto retained=pin_owned();const auto retained_host=pin_owned_host(host);
        const auto actual_frame=frame_;const auto actual_ui=ui_frame;verify_owned();
        const auto *stamp=actual_ui.snapshot.find<BnwClientFrameStamp>();
        const bool valid=current() && host.is_current_frame(actual_ui) && stamp && actual_frame &&
            stamp->identity==actual_frame->stamp.identity && stamp->selection==actual_frame->stamp.selection &&
            stamp->viewport==actual_frame->stamp.viewport;
        verify_owned();
        return valid && frame_==actual_frame && owner_.current_frame()==actual_frame;
    }
    // UI helpers route several command types. A successful helper or uncertain
    // exception invalidates the publication before a second helper can run.
    UiCommandResult dispatch(LiveUiHost &host, const LiveUiFrame &ui_frame,
                             const std::function<UiCommandResult()> &action) {
        const auto retained=pin_owned();const auto retained_host=pin_owned_host(host);
        const auto actual_ui=ui_frame;const auto actual_action=action;verify_owned();
        if (!accepts(host,actual_ui) || !actual_action)
            return {UiCommandStatus::invalid,"BNW rendered client frame is stale"};
        try {
            const auto result=actual_action();verify_owned();
            const bool host_current=host.is_current_frame(actual_ui);verify_owned();
            if(result.status==UiCommandStatus::performed || result.status==UiCommandStatus::pending_refresh || result.status==UiCommandStatus::uncertain || result.status==UiCommandStatus::navigation ||
               !host_current)owner_.notify_world_changed();
            return result;
        } catch (...) { owner_.notify_world_changed();throw; }
    }
    bool present(const std::function<void(const BnwClientFrame &)> &compose) const {
        const auto retained=pin_owned();const auto actual_frame=frame_;
        const auto actual_compose=compose;verify_owned();
        if(!current())return false;verify_owned();
        const bool presented=owner_.present(actual_frame,actual_compose);verify_owned();
        return presented && current();
    }
    // Bind the renderer's SAME screen controller and projected provider to
    // the ONE canonical FrameOwner. This is a Civis client input binding.
    std::shared_ptr<LiveUiHost> create_selected_ui_host(UiScreenController& screens,
        LiveUiHost::FrameProvider provider,UiGameplayBackend* backend,LiveUiSources sources={}) {
        const auto retained=pin_owned();verify_owned();
        if(!selected_host_)throw std::logic_error("Legacy scene cannot bind genuine selected UI");
        auto host=std::make_shared<LiveUiHost>(screens,std::move(provider),backend,std::move(sources));
        verify_owned();owner_.register_created_selected_ui_host(host);verify_owned();
        return host;
    }
    std::shared_ptr<LiveUiHost> create_selected_ui_host(UiContextRegistry registry,
        LiveUiHost::FrameProvider provider,UiGameplayBackend* backend,LiveUiSources sources={}) {
        const auto retained=pin_owned();verify_owned();
        if(!selected_host_)throw std::logic_error("Legacy scene cannot bind genuine selected UI");
        auto host=std::make_shared<LiveUiHost>(std::move(registry),std::move(provider),backend,std::move(sources));
        verify_owned();owner_.register_created_selected_ui_host(host);verify_owned();
        return host;
    }
    UiCommandResult dispatch(LiveUiHost& host,const LiveUiFrame& ui,const UiCommand& command) {
        const auto retained=pin_owned();const auto retained_host=pin_owned_host(host);
        const auto actual_frame=frame_;
        const auto actual_ui=ui;const auto actual_command=command;verify_owned();
        return dispatch(host,actual_ui,std::function<UiCommandResult()>{[this,&host,actual_ui,actual_frame,actual_command] {
            return owner_.dispatch(host,actual_ui,actual_frame,actual_command);
        }});
    }
    LiveUiHost::FrameProvider ui_provider() const { return owner_.ui_provider(); }
    const BnwClientFrameOwner &owner() const { return owner_; }
    const graphics::BnwRendererSceneReceipt &receipt() const { return receipt_; }
    const std::shared_ptr<const BnwClientFrame> &frame() const { return frame_; }
    // The native picker retains the exact publication whose depth it sampled.
    void select_plot(std::optional<std::int64_t> plot,
                     std::shared_ptr<const BnwClientFrame> picked_frame) {
        const auto retained=pin_owned();verify_owned();
        if(!picked_frame || picked_frame!=frame_ || !current())
            throw std::invalid_argument("World pick frame is stale or foreign");
        verify_owned();owner_.select_plot(plot,picked_frame);verify_owned();
        // Retire paint/fog even for repeated selection. The next begin() captures
        // the canonical plot along with the same owned source/session/rules.
        owner_.notify_world_changed();rendered_fog_.reset();frame_.reset();
    }
    void invalidate() { owner_.notify_world_changed(); }

  private:
    friend class graphics::BnwInitialUnitSetupGraphicalRendererLoan;
    friend class graphics::BnwUnitSetupGraphicalQueue;
    std::weak_ptr<graphics::BnwUnitSetupGraphicsState> setup_graphics_state_;
    std::shared_ptr<graphics::BnwUnitSetupGraphicalQueue> setup_graphical_storage_;
    std::weak_ptr<graphics::BnwUnitSetupGraphicalQueue> setup_graphical_queue_;
    mutable const graphics::BnwInitialUnitSetupGraphicalRendererLoan* setup_graphical_renderer_action_=nullptr;
    mutable std::uint64_t setup_graphical_renderer_serial_=0;
    mutable bool setup_graphical_renderer_failed_=false;
    void with_actual_initial_unit_setup_graphical_renderer(
        const runtime::BnwNativeUnitAllocatorCore&,const runtime::BnwUnitMovePending&,
        const runtime::BnwStockLuaAttemptLease&,
        const std::function<void(const graphics::BnwInitialUnitSetupGraphicalRendererLoan&)>&)const;
    friend class graphics::BnwPendingUnitVisualOwner;
    friend class graphics::BnwSelectedPendingUnitVisualLoan;
    friend class graphics::BnwTrainPresentedFrameProof;
    friend class graphics::BnwInitialCenterUnitRendererStorage;
    friend class graphics::BnwInitialCenterUnitRendererLoan;
    friend class graphics::BnwInitialUnitShouldDimRendererLoan;
    friend class runtime::BnwOwnedGameplayEventPumpLoan;
    friend class runtime::BnwSelectedStockLuaBootstrap;
    void with_actual_initial_unit_should_dim_renderer(
        const runtime::BnwNativeUnitAllocatorCore&,const runtime::BnwUnitMovePending&,
        const runtime::BnwStockLuaAttemptLease&,
        const std::function<void(const graphics::BnwInitialUnitShouldDimRendererLoan&)>&)const;
    std::weak_ptr<graphics::BnwUnitShouldDimQueue> should_dim_queue_;
    mutable const graphics::BnwInitialUnitShouldDimRendererLoan* should_dim_renderer_action_=nullptr;
    mutable std::uint64_t should_dim_renderer_serial_=0;
    mutable bool should_dim_renderer_failed_=false;
    // Only actual successful paint/present creates this renderer association.
    std::weak_ptr<graphics::BnwInitialCenterUnitRendererStorage> center_unit_renderer_;
    void with_actual_initial_center_unit_renderer(
        const runtime::BnwNativeUnitAllocatorCore&,const runtime::BnwUnitMovePending&,
        const runtime::BnwStockLuaAttemptLease&,
        const std::function<void(const graphics::BnwInitialCenterUnitRendererLoan&)>&) const;
    mutable const graphics::BnwInitialCenterUnitRendererLoan* center_unit_renderer_action_=nullptr;
    mutable std::uint64_t center_unit_renderer_serial_=0;
    mutable bool center_unit_renderer_failed_=false;
    friend class runtime::BnwSelectedStockLuaSourceOwner;
    void with_actual_pending_unit_visual_loan(
        const std::shared_ptr<const BnwOwnedCameraGuiScreen>&,
        const runtime::BnwStockLuaAttemptLease&,
        const std::function<void(const graphics::BnwSelectedPendingUnitVisualLoan&)>&) const;
    mutable const graphics::BnwSelectedPendingUnitVisualLoan* pending_visual_action_=nullptr;
    mutable std::uint64_t pending_visual_action_serial_=0;
    mutable bool pending_visual_action_failed_=false;
    // Actual renderer receiver owns only weak Scene/source links.
    std::shared_ptr<graphics::BnwPendingUnitVisualOwner> pending_unit_visuals_;
    friend class graphics::BnwUnitDamageCommandOwner;
    std::shared_ptr<graphics::BnwUnitDamageCommandOwner> unit_damage_commands_;
    friend class UiScreenCompositor;
    friend class BnwOwnedCameraGuiScreen;
    friend class BnwOwnedNotificationPlotFxReceiver;
    friend class BnwAlphaProgressionBackend;
    mutable std::atomic<std::size_t> gui_publication_pins_{0};
    // Only the actual compositor mints this association; no strong Scene cycle.
    std::weak_ptr<const BnwOwnedCameraGuiScreen> gui_screen_;
    friend class ::civ5::graphics::BnwClearHexHighlightOwner;
    //Minted only by actual selected UiRenderer PRE, after real constructor.
    std::shared_ptr<graphics::BnwClearHexHighlightOwner> clear_hex_highlights_;
    std::weak_ptr<BnwClientSceneFrame> gui_self_;
    const std::thread::id gui_thread_=std::this_thread::get_id();
    std::shared_ptr<BnwClientSceneFrame> pin_owned()const;
    std::shared_ptr<LiveUiHost> pin_owned_host(const LiveUiHost&)const;
    void verify_owned()const;
    friend class BnwPlotYieldRefreshUiAdapter;
    std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage> owned_plot_yield_ui_sources_;
    friend class BnwPlotYieldRefreshOwnedUiContext;
    friend class UiScreenGlRenderer;
    void require_publication_finality(const std::shared_ptr<const BnwClientFrame>& frame)const {
        if(plot_yield_ui_thread_!=std::this_thread::get_id()||!frame||frame_!=frame||owner_.current_frame()!=frame)
            throw std::runtime_error("Owned UI final actual publication changed");
    }
    const std::thread::id plot_yield_ui_thread_=std::this_thread::get_id();
    std::atomic<bool> plot_yield_ui_busy_{false},plot_yield_ui_poisoned_{false};
    static std::shared_ptr<const SelectedBnwPreviewHost> require_selected_parent(
        std::shared_ptr<const SelectedBnwPreviewHost> parent,
        const graphics::BnwRendererSceneReceipt &receipt) {
        // Plain source/player authentication precedes function target copies,
        // source callbacks and construction of the ONE real FrameOwner.
        if(!parent) throw std::invalid_argument("Selected renderer parent absent");
        if(!receipt.projection_reconciled)
            throw std::invalid_argument("Selected renderer source projection not reconciled");
        if(receipt.restored_payload_sha256.empty() ||
           receipt.restored_payload_sha256!=parent->identity().payload_sha256)
            throw std::invalid_argument("Selected renderer payload differs from actual host");
        if(runtime::identify_bytes(parent->session().source().source.bytes).sha256!=receipt.restored_payload_sha256)
            throw std::invalid_argument("Selected renderer payload differs from retained source bytes");
        if(parent->active_player_id()<0 || receipt.active_player!=
           static_cast<std::uint64_t>(parent->active_player_id()))
            throw std::invalid_argument("Selected renderer player differs from actual host");
        return parent;
    }
    static LiveSessionUiSources checked_selected_sources(
        const std::shared_ptr<const SelectedBnwPreviewHost>& parent,
        const graphics::BnwRendererSceneReceipt &pinned_receipt,
        const runtime::LiveSessionState* initial_state,std::uint64_t initial_generation,
        const LiveSessionUiSources &borrowed_sources) {
        // Borrowed callable storage must remain valid during its own copy.
        // Receipt and genuine parent are already retained before these copies.
        auto sources=borrowed_sources;
        if(&parent->session().state()!=initial_state ||
           parent->session().state_generation()!=initial_generation)
            throw std::logic_error("Selected renderer state replaced during source copy");
        (void)require_selected_parent(parent,pinned_receipt);
        return sources;
    }
    // Declared BEFORE borrowed session/state fields so callbacks cannot release
    // the last actual host while this scene owner still holds borrowed state.
    std::shared_ptr<const SelectedBnwPreviewHost> selected_host_;
    const runtime::LiveSession &session_;
    // Native scene owners borrow load-derived contexts at construction. A
    // restore replacing that owner requires rebuilding those GPU/native owners.
    const runtime::LiveSessionState *scene_state_ = &session_.state();
    const std::uint64_t scene_state_generation_ = session_.state_generation();
    const graphics::BnwRendererSceneReceipt receipt_;
    std::function<std::optional<std::int64_t>()> selected_unit_, selected_city_;
    BnwClientFrameOwner owner_;
    std::shared_ptr<const BnwClientFrame> frame_;
    std::optional<graphics::BnwNativeFogFrameBinding> native_fog_;
    std::optional<graphics::BnwFogRenderedFrameReceipt> rendered_fog_;
};
} // namespace civ5::ui
