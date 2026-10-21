#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/bnw_plot_yield_refresh_owned_ui_context.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include <climits>
#include "civ5/ui/ui_owner_fences.hpp"
#include <atomic>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::ui {
namespace {
std::atomic<std::uint64_t> next_owner{1};
const SelectedBnwPreviewHost& checked_client_parent(
    const std::shared_ptr<const SelectedBnwPreviewHost>& host) {
    if(!host)throw std::invalid_argument("Actual selected client parent absent");
    return *host;
}
void increment(std::uint64_t &value) {
    if (value == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("BNW client publication counter exhausted");
    ++value;
}
} // namespace
struct BnwClientFrameOwner::Publication {
    std::uint64_t owner = next_owner.fetch_add(1), revision = 1, frame = 0;
    std::string session;
    std::uint64_t state_generation=0;
    const runtime::LiveSession* borrowed_session=nullptr;
    std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage> owned_ui_sources;
    // Internal identity only: retain_owned_scene compares against the genuine
    // weak Scene child. This pointer never issues ownership or a selected epoch.
    const BnwClientFrameOwner* scene_identity=nullptr;
    BnwClientSelection selection;
    std::shared_ptr<const SelectedBnwPreviewHost> selected_host;
    std::shared_ptr<BnwClientSelectionControl> city_selection;
    std::shared_ptr<LiveUiHost> client_ui;
    std::shared_ptr<const BnwClientFrame> current;
    bool capturing = false;
    bool source_current(const BnwClientFrame& frame)const {
        if(city_selection){
            city_selection->require_current();
            if(city_selection->revision()!=frame.stamp.identity.selection_revision)return false;
        }
        const auto& session=selected_host?selected_host->session():*borrowed_session;
        return state_generation==session.state_generation()&&
            frame.stamp.identity.ui_source_revision==session.state().ui_source_revision;
    }
};

BnwClientFrameOwner::BnwClientFrameOwner(const runtime::LiveSession &session,
    std::string identity, std::int64_t player, LiveSessionUiSources sources)
    : session_(session), player_(player), sources_(std::move(sources)),
      publication_(std::make_shared<Publication>()) {
    if (identity.empty() || player < 0)
        throw std::invalid_argument("BNW client requires a session and active player");
    publication_->borrowed_session=&session;
    publication_->scene_identity=this;
    publication_->session = std::move(identity);
    validate_selection({});
}
BnwClientFrameOwner::~BnwClientFrameOwner() {
    if(publication_->city_selection)publication_->city_selection->retire();
}
void BnwClientFrameOwner::retain_owned_ui_sources(std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage> storage){publication_->owned_ui_sources=std::move(storage);}
BnwClientFrameOwner::BnwClientFrameOwner(
    std::shared_ptr<const SelectedBnwPreviewHost> host,LiveSessionUiSources sources)
    : BnwClientFrameOwner(checked_client_parent(host).session(),
        checked_client_parent(host).identity().payload_sha256,
        checked_client_parent(host).active_player_id(),std::move(sources)) {
    auto control=BnwClientSelectionControl::create(host);
    const auto player=control->active_player();
    if(player<0)throw std::invalid_argument("Actual client Game active player unavailable");
    if(host->active_player_id()!=player)
        throw std::invalid_argument("Diagnostic preview actor differs from actual Game UI actor");
    publication_->selected_host=std::move(host);
    publication_->city_selection=std::move(control);
}
std::unique_ptr<BnwClientFrameOwner> BnwClientFrameOwner::from_selected_host(
    std::shared_ptr<const SelectedBnwPreviewHost> host,LiveSessionUiSources sources) {
    return std::unique_ptr<BnwClientFrameOwner>(new BnwClientFrameOwner(std::move(host),std::move(sources)));
}
void BnwClientFrameOwner::register_created_selected_ui_host(const std::shared_ptr<LiveUiHost>& host) {
    const auto publication=publication_;
    if(!publication->city_selection||!host)throw std::logic_error("Borrowed client cannot bind genuine UI");
    publication->city_selection->require_current(false);
    if(publication->client_ui)throw std::logic_error("Actual selected client UI can bind only once");
    host->city_selection_control_=publication->city_selection;
    host->selected_owner_lifetime_=host;
    publication->city_selection->bind_actual_ui_host();
    publication->client_ui=host;
}
std::shared_ptr<LiveUiHost> BnwClientFrameOwner::create_selected_ui_host(
    UiContextRegistry registry,UiGameplayBackend* backend,LiveUiSources sources) {
    const auto publication=publication_;
    if(!publication->city_selection)throw std::logic_error("Borrowed client cannot bind genuine UI");
    publication->city_selection->require_current(false);
    if(publication->client_ui)throw std::logic_error("Actual selected client UI can bind only once");
    auto host=std::make_shared<LiveUiHost>(std::move(registry),ui_provider(),backend,std::move(sources));
    register_created_selected_ui_host(host);
    return host;
}
std::vector<runtime::BnwCityDirtyEvent> BnwClientFrameOwner::take_city_dirty_events() {
    const auto publication=publication_;
    const auto storage=publication->owned_ui_sources;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    if(!publication->city_selection)throw std::logic_error("Borrowed client cannot consume genuine dirty queue");
    const auto result=publication->city_selection->take_events();
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    return result;
}

BnwClientUnitDirtySnapshot BnwClientFrameOwner::unit_dirty_state()const{
    const auto p=publication_;
    const auto storage=p->owned_ui_sources;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    if(!p->city_selection)throw std::logic_error("Borrowed client has no genuine unit dirty storage");
    const auto result=p->city_selection->unit_dirty_state();
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    return result;
}
std::vector<BnwClientUnitDirtyEvent> BnwClientFrameOwner::take_unit_dirty_events(){
    const auto p=publication_;
    const auto storage=p->owned_ui_sources;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    if(!p->city_selection)throw std::logic_error("Borrowed client cannot consume genuine unit dirty events");
    const auto result=p->city_selection->take_unit_dirty_events();
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    return result;
}
void BnwClientFrameOwner::validate_selection(BnwClientSelection selection) const {
    const auto publication=publication_;
    const auto storage=publication->owned_ui_sources;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);

    if(publication_->city_selection)publication_->city_selection->require_current();
    const auto &state = session_.state();
    const auto player = state.setup.players.find(player_);
    if (player == state.setup.players.end() || !player->second.alive || player->second.team < 0)
        throw std::invalid_argument("BNW client active player is unavailable");
    if (selection.unit) {
        const auto army = state.maintenance.players.find(player_);
        if (*selection.unit < 0 || army == state.maintenance.players.end() ||
            !army->second.units.contains(*selection.unit))
            throw std::invalid_argument("BNW client selected unit is not owned");
    }
    if (selection.city) {
        const auto cities = state.cities.players.find(player_);
        if(publication_->city_selection){
            if(*selection.city<0||*selection.city>INT32_MAX||player_>INT32_MAX||
                !runtime::resolve_bnw_city_registry(state.world,state.cities,
                    static_cast<std::int32_t>(player_),static_cast<std::int32_t>(*selection.city)))
                throw std::invalid_argument("Actual client selected canonical city unavailable");
            BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
            if(publication->city_selection)publication->city_selection->require_current();
            return;
        }
        if (*selection.city < 0 || cities == state.cities.players.end() ||
            !cities->second.cities.contains(*selection.city))
            throw std::invalid_argument("BNW client selected city is not owned");
    }
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    if(publication->city_selection)publication->city_selection->require_current();
}
void BnwClientFrameOwner::select(BnwClientSelection selection) {
    const auto publication=publication_;
    const auto storage=publication->owned_ui_sources;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);

    // Unit/city input never clears or changes the canonical picked plot.
    if(publication_->city_selection){
        const auto plot=publication_->city_selection->selection_plot();
        selection.plot=plot?std::optional{static_cast<std::int64_t>(*plot)}:std::nullopt;
    }else selection.plot=publication_->selection.plot;
    validate_selection(selection);
    if(publication_->city_selection){
        publication_->city_selection->select_input(selection.city
            ?std::optional{std::pair{player_,*selection.city}}:std::nullopt,selection.unit);
        const auto city=publication_->city_selection->city();
        selection.city=city?std::optional{city->second}:std::nullopt;
    }
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    if(publication->city_selection)publication->city_selection->require_current();
    if (publication_->selection == selection) return;
    publication_->selection = selection;
    publication_->current.reset();
}
UiCommandResult BnwClientFrameOwner::cycle_selection(
    const std::shared_ptr<const BnwClientFrame>& frame,
    const BnwManualCycleSelectionCommand& command) {
    const auto p=publication_;
    if(!p->city_selection||!p->selected_host)
        return {UiCommandStatus::unavailable,"Manual cycle requires the actual selected client"};
    p->city_selection->require_current();
    if(!is_current(frame))
        return {UiCommandStatus::invalid,"Manual cycle frame is stale"};
    const auto& host=*p->selected_host;
    const auto* moves=host.update_moves_owner_.get();
    const auto& busy=host.activation_owner().busy_state;
    if(!moves||!busy)
        return {UiCommandStatus::unavailable,"Manual cycle requires retained original-import movement and busy owners; Civis transients are not restored"};
    const auto unit=p->city_selection->unit();
    if(unit!=frame->stamp.selection.unit)
        return {UiCommandStatus::invalid,"Manual cycle canonical unit differs from its frame"};
    const auto observation=runtime::observe_bnw_game_unit_cycle(
        host.session().state(),*moves,*busy,host.identity(),player_,
        unit?std::optional<runtime::BnwUnitOrderKey>{{player_,*unit}}:std::nullopt,
        command.direction,command.filter);
    // In the original source this dispatch precedes selection. A manual
    // client command cannot skip it and then report a changed selection.
    if(observation.send_auto_moves_required)
        return {UiCommandStatus::unavailable,"Manual cycle requires the actual sendAutoMoves owner before selection"};
    p->city_selection->require_current();
    if(!is_current(frame))
        return {UiCommandStatus::invalid,"Manual cycle world changed during observation"};
    if(observation.next){
        if(observation.next->first!=player_)
            throw std::logic_error("Manual cycle returned a foreign next unit");
        select({observation.next->second,std::nullopt});
        return {UiCommandStatus::performed,"Manual client unit selected; original engine selection and camera effects remain unavailable"};
    }
    if(observation.clear_selection_required){
        select({});
        return {UiCommandStatus::performed,"Manual client selection cleared; original engine selection effects remain unavailable"};
    }
    return {UiCommandStatus::performed,"Manual cycle has no eligible unit; selection unchanged"};
}

void BnwClientFrameOwner::select_plot(std::optional<std::int64_t> plot) {
    select_plot(plot,publication_->current);
}
void BnwClientFrameOwner::select_plot(std::optional<std::int64_t> plot,
    std::shared_ptr<const BnwClientFrame> picked_frame) {
    const auto publication=publication_;
    if(!publication->city_selection)throw std::logic_error("Borrowed client cannot issue canonical plot input");
    publication->city_selection->require_current();
    if(!is_current(picked_frame))throw std::invalid_argument("Canonical plot input requires exact current picked frame");
    publication->city_selection->select_plot(plot);
    const auto canonical=publication->city_selection->selection_plot();
    publication->selection.plot=canonical?std::optional{static_cast<std::int64_t>(*canonical)}:std::nullopt;
    if(publication->city_selection->revision()!=picked_frame->stamp.identity.selection_revision)
        publication->current.reset();
}
void BnwClientFrameOwner::notify_world_changed() {
    if(publication_->city_selection)publication_->city_selection->require_thread();
    publication_->current.reset();
    increment(publication_->revision);
}
std::shared_ptr<const BnwClientFrame> BnwClientFrameOwner::capture(
    ClientViewport viewport, terrain::CameraControllerState camera) {
    const auto publication=publication_;
    const auto storage=publication->owned_ui_sources;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);

    if(publication->city_selection)publication->city_selection->require_current();
    const auto& session=publication->selected_host?publication->selected_host->session():session_;
    const auto player=player_;
    auto selection=publication->selection;
    std::uint64_t selection_revision=0;
    std::optional<BnwClientUnitDirtySnapshot> unit_dirty;
    if(publication->city_selection){
        publication->city_selection->require_current();
        const auto city=publication->city_selection->city();
        if(city&&city->first!=player)throw std::logic_error("Current client selection actor differs");
        selection.city=city?std::optional{city->second}:std::nullopt;
        selection.unit=publication->city_selection->unit();
        const auto plot=publication->city_selection->selection_plot();
        selection.plot=plot?std::optional{static_cast<std::int64_t>(*plot)}:std::nullopt;
        unit_dirty=publication->city_selection->unit_dirty_state();
        selection_revision=publication->city_selection->revision();
    }
    if (publication->capturing) throw std::logic_error("Reentrant BNW frame capture");
    publication->current.reset();
    if (!viewport.drawable()) return {};
    validate_selection(selection);
    const auto &state = session.state();
    if (state.world.game_state.layout != saves::GameStateLayout::expansion2_version1 ||
        !state.world.map_projection || !state.plot_visibility.current ||
        state.setup.game_turn < 0 || state.setup.game_turn > std::numeric_limits<std::int32_t>::max())
        throw std::invalid_argument("BNW client requires the current live map and visibility");
    const auto &map = *state.world.map_projection;
    if (map.grid.width != state.world.map_header.grid.width ||
        map.grid.height != state.world.map_header.grid.height ||
        map.cells.size() != map.grid.size())
        throw std::invalid_argument("BNW client map shape differs");
    increment(publication->frame);
    const BnwClientFrameStamp stamp{{publication->owner, publication->revision,
        publication->frame, publication->session, player, state.setup.players.at(player).team,
        static_cast<std::int32_t>(state.setup.game_turn), state.world.map_header.guid,selection_revision,state.ui_source_revision},
        selection, viewport};
    const auto captured_state_lease=session.read_state_lease();
    const auto *captured_state = captured_state_lease.get();
    const auto captured_generation=session.state_generation();
    auto world = state.world;
    auto sources = sources_;
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    sources.selected_city = [selected = stamp.selection.city] { return selected; };
    sources.selected_unit = [selected = stamp.selection.unit] { return selected; };
    publication->capturing = true;
    struct ClearCapture { bool &value; ~ClearCapture() { value = false; } } clear{publication->capturing};
    auto ui = capture_live_session_ui_frame(session, stamp.identity.frame, player, sources);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    if(publication->city_selection)publication->city_selection->require_current();
    if (publication->revision != stamp.identity.revision || (publication->city_selection?publication->city_selection->unit()!=stamp.selection.unit:publication->selection.unit!=stamp.selection.unit) ||
        (publication->city_selection?publication->city_selection->revision()!=stamp.identity.selection_revision:publication->selection!=stamp.selection) || &session.state() != captured_state ||
        session.state().setup.game_turn != stamp.identity.turn ||
        session.state_generation()!=captured_generation ||
        session.state().ui_source_revision!=stamp.identity.ui_source_revision)
        throw std::logic_error("BNW owner changed during frame capture");
    const auto *world_view = ui.find<WorldViewState>();
    if (!world_view || !world_view->map_visibility)
        throw std::invalid_argument("BNW frame has no current UI visibility");
    const auto &visibility = *world_view->map_visibility;
    if (visibility.player_id != player || visibility.team_id != stamp.identity.team ||
        visibility.width != map.grid.width || visibility.height != map.grid.height ||
        visibility.plots.size() != map.cells.size())
        throw std::invalid_argument("BNW scene and UI visibility differ");
    if(publication->selected_host) {
        BnwResearchChoiceUiSnapshot research;
        research.player=stamp.identity.player;research.team=stamp.identity.team;
        research.revision=stamp.identity.ui_source_revision;
        const auto queue=state.player_research.players.find(research.player);
        if(queue!=state.player_research.players.end())research.research_queue=queue->second.queue;
        if(state.world.game_state.roster){
            const auto& arrays=state.world.game_state.roster->arrays;
            // Proven store_player_score/store_team_scores in bnw_game_score.cpp.
            if(research.player>=0&&static_cast<std::size_t>(research.player)<arrays[3].size())
                research.player_score=arrays[3][static_cast<std::size_t>(research.player)];
            if(research.team>=0&&static_cast<std::size_t>(research.team)<arrays[6].size())
                research.team_score=arrays[6][static_cast<std::size_t>(research.team)];
        }
        ui=ui.with(std::make_shared<const BnwResearchChoiceUiSnapshot>(std::move(research)));
    }
    ui = ui.with(std::make_shared<const BnwClientFrameStamp>(stamp));
    if(unit_dirty)ui=ui.with(std::make_shared<const BnwClientUnitDirtySnapshot>(std::move(*unit_dirty)));
    auto frame = std::make_shared<const BnwClientFrame>(BnwClientFrame{
        stamp, std::move(world), visibility, std::move(camera), std::move(ui)});
    publication->state_generation=captured_generation;
    publication->current = frame;
    if (sources.augment_snapshot && (!ui_owners_current(frame->ui) || publication->current!=frame ||
        publication->revision != stamp.identity.revision || (publication->city_selection?publication->city_selection->unit()!=stamp.selection.unit:publication->selection.unit!=stamp.selection.unit) ||
        !publication->source_current(*frame) || (publication->city_selection?publication->city_selection->revision()!=stamp.identity.selection_revision:publication->selection!=stamp.selection))) {
        if (publication->current == frame) publication->current.reset();
        throw std::logic_error("BNW augmented publication changed during final owner checks");
    }
    if(!publication->source_current(*frame))throw std::logic_error("BNW world changed before publication");
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    if(publication->city_selection)publication->city_selection->require_current();
    if(!publication->source_current(*frame))throw std::logic_error("BNW source changed at final owned publication check");
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    return frame;
}
void BnwClientFrameOwner::require_plot_tail_publication_identity(
 const std::shared_ptr<const BnwClientFrame>& frame,
 const std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>& storage,
 const std::shared_ptr<const SelectedBnwPreviewHost>& host,
 const std::shared_ptr<LiveUiHost>& ui,
 const std::shared_ptr<const runtime::LiveSessionState>& state,
 std::uint64_t generation,std::uint64_t expected_world_revision)const{
 const auto p=publication_;
 const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(storage,this);
 BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
 if(!retained||!storage||p->owned_ui_sources!=storage||p->scene_identity!=this||
    !host||p->selected_host!=host||!ui||p->client_ui!=ui||
    !p->city_selection||p->city_selection!=ui->city_selection_control_||
    p->borrowed_session!=&host->session()||&session_!=&host->session()||
    !frame||p->current!=frame||p->capturing)
  throw std::logic_error("Plot tail SAME private retained Publication absent");
 p->city_selection->require_current();
 const auto& session=host->session();
 const auto& stamp=frame->stamp;const auto& identity=stamp.identity;
 const auto city=p->city_selection->city();const auto unit=p->city_selection->unit();
 const auto plot=p->city_selection->selection_plot();
 if(session.read_state_lease()!=state||session.state_generation()!=generation||
    p->state_generation!=generation||!state||state->ui_source_revision!=expected_world_revision||
    identity.ui_source_revision>expected_world_revision||identity.owner!=p->owner||
    identity.revision!=p->revision||identity.frame!=p->frame||identity.session!=p->session||
    identity.session!=host->identity().payload_sha256||identity.player!=player_||
    identity.player!=host->active_player_id()||identity.turn!=state->setup.game_turn||
    identity.map_guid!=state->world.map_header.guid||
    !state->setup.players.contains(player_)||!state->setup.players.at(player_).alive||
    identity.team!=state->setup.players.at(player_).team||
    p->city_selection->active_player()!=player_||
    identity.selection_revision!=p->city_selection->revision()||
    stamp.selection.city!=(city?std::optional<std::int64_t>{city->second}:std::nullopt)||
    (city&&city->first!=player_)||stamp.selection.unit!=unit||
    stamp.selection.plot!=(plot?std::optional<std::int64_t>{static_cast<std::int64_t>(*plot)}:std::nullopt)||
    p->current!=frame)
  throw std::logic_error("Plot tail raw Publication/source/selection identity changed");
 BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
 // No source_current/current_frame: the actual pending just changed the world.
}
std::shared_ptr<const BnwClientFrame> BnwClientFrameOwner::current_frame() const {
    const auto publication=publication_;const auto storage=publication->owned_ui_sources;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
    if(publication->city_selection)publication->city_selection->require_current();
    const auto frame=publication->current;
    const bool current=frame&&publication->source_current(*frame)&&publication->current==frame;
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    return current?frame:std::shared_ptr<const BnwClientFrame>{};
}
bool BnwClientFrameOwner::is_current(const std::shared_ptr<const BnwClientFrame> &frame) const {
    const auto publication=publication_;
    const auto storage=publication->owned_ui_sources;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);

    const auto actual_frame=frame;
    if(publication->city_selection)publication->city_selection->require_current();
    if(publication->city_selection){
        publication->city_selection->require_current();
        if(!actual_frame||publication->city_selection->revision()!=actual_frame->stamp.identity.selection_revision)return false;
    }
    if (!actual_frame || publication->current != actual_frame ||
        !publication->source_current(*actual_frame)) return false;
    // Source resolution observers can enter application code while a borrowed
    // owner is checked. Authenticate publication again after those callbacks.
    const bool owners=ui_owners_current(actual_frame->ui);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    if(publication->city_selection){
        publication->city_selection->require_current();
        if(publication->city_selection->revision()!=actual_frame->stamp.identity.selection_revision)return false;
    }
    const bool result=owners&&publication->current==actual_frame&&publication->source_current(*actual_frame);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    return result;
}
LiveUiHost::FrameProvider BnwClientFrameOwner::ui_provider() const {
    return [weak = std::weak_ptr<Publication>(publication_)] {
        const auto owner = weak.lock();
        if(!owner)throw std::logic_error("BNW client UI owner retired");
        const auto storage=owner->owned_ui_sources;
        const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(storage,owner->scene_identity);
        BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);
        if(owner->city_selection)owner->city_selection->require_current();
        if(!owner->current||!owner->source_current(*owner->current))
            throw std::logic_error("BNW client UI frame is unpublished or stale");
        const auto actual_frame=owner->current;
        const auto result=actual_frame->ui;
        BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
        if(owner->city_selection)owner->city_selection->require_current();
        if(owner->current!=actual_frame||!owner->source_current(*actual_frame))throw std::logic_error("BNW UI provider changed during final owner checks");
        BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
        return result;
    };
}
UiCommandResult BnwClientFrameOwner::dispatch(LiveUiHost &host,
    const LiveUiFrame &ui_frame, const std::shared_ptr<const BnwClientFrame> &frame,
    const UiCommand &command) {
    const auto publication=publication_;
    const auto storage=publication->owned_ui_sources;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);

    const auto retained_host=BnwPlotYieldRefreshOwnedUiContext::retain_owned_host(storage,&host);
    const auto actual_frame=frame;const auto actual_ui=ui_frame;const auto actual_command=command;
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    if(publication->city_selection)publication->city_selection->require_current();
    if(publication->city_selection&&publication->client_ui.get()!=&host)
        return {UiCommandStatus::invalid,"Actual selected client UI owner differs"};
    const auto *stamp = actual_ui.snapshot.find<BnwClientFrameStamp>();
    if (!is_current(actual_frame) || !stamp || stamp->identity != actual_frame->stamp.identity ||
        stamp->selection != actual_frame->stamp.selection || stamp->viewport != actual_frame->stamp.viewport ||
        !host.is_current_frame(actual_ui))
        return {UiCommandStatus::invalid, "BNW client frame or selection is stale"};
    try {
        validate_selection(actual_frame->stamp.selection);
        if(const auto* gameplay=std::get_if<UiGameplayCommand>(&actual_command)){
            if(const auto* cycle=gameplay->as<BnwManualCycleSelectionCommand>()){
                if(gameplay->source_id()!="WorldView")
                    return {UiCommandStatus::invalid,"Manual cycle must originate from WorldView"};
                // Reuse the real controller's source/visibility/modal checks.
                // The copy is used ONLY for this typed gameplay branch: it
                // cannot open/close screens or replace the actual focus owner.
                struct CycleBackend final:UiGameplayBackend{
                    std::function<UiCommandResult()> execute_cycle;
                    UiCommandResult availability(const UiGameplayCommand&,
                        const UiFrameSnapshot&)const override{
                        return {UiCommandStatus::ready,{}};
                    }
                    UiCommandResult execute(const UiGameplayCommand&,
                        const UiFrameSnapshot&)override{return execute_cycle();}
                } backend;
                backend.execute_cycle=[&]{return cycle_selection(actual_frame,*cycle);};
                auto screens=host.screens();
                const auto result=screens.dispatch(actual_command,actual_ui.snapshot,&backend);
                if(result.status==UiCommandStatus::performed)
                    return {UiCommandStatus::navigation,result.reason};
                return result;
            }
        }
        const auto result = host.dispatch(actual_command, actual_ui);
        BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
        if(publication->city_selection)publication->city_selection->require_current();
        const bool host_current=host.is_current_frame(actual_ui);
        BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
        if(publication->city_selection)publication->city_selection->require_current();
        if (result.status == UiCommandStatus::performed || result.status == UiCommandStatus::pending_refresh || result.status == UiCommandStatus::uncertain) {
            publication->current.reset();increment(publication->revision);
        }
        else if (result.status == UiCommandStatus::navigation || !host_current)
            publication->current.reset();
        return result;
    } catch (...) {
        // A throwing gameplay/native callback may already have changed state.
        publication->current.reset();increment(publication->revision);
        throw;
    }
}
bool BnwClientFrameOwner::present(const std::shared_ptr<const BnwClientFrame> &frame,
    const std::function<void(const BnwClientFrame &)> &compose) const {
    const auto publication=publication_;
    const auto storage=publication->owned_ui_sources;
    const auto retained=BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(storage,this);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_current(storage);

    const auto actual_frame=frame;const auto actual_compose=compose;
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
    if(publication->city_selection)publication->city_selection->require_current();
    const auto current=[&]{
        if(publication->city_selection){
            publication->city_selection->require_current();
            if(!actual_frame||publication->city_selection->revision()!=actual_frame->stamp.identity.selection_revision)return false;
        }
        if(!actual_frame||publication->current!=actual_frame||!publication->source_current(*actual_frame))return false;
        const bool owners=ui_owners_current(actual_frame->ui);
        BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
        if(publication->city_selection){
            publication->city_selection->require_current();
            if(publication->city_selection->revision()!=actual_frame->stamp.identity.selection_revision)return false;
        }
        const bool result=owners&&publication->current==actual_frame&&publication->source_current(*actual_frame);
        BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);
        return result;
    };
    if (!actual_compose||!current()) return false;
    actual_compose(*actual_frame);
    BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(storage);

    return current();
}
} // namespace civ5::ui

namespace civ5::ui {
void BnwClientFrameOwner::require_same_gui_ui_host(const std::shared_ptr<LiveUiHost>& host)const {
    const auto& actual=publication_->client_ui;
    if(!publication_->selected_host||!publication_->city_selection||!actual||!host||
       actual.get()!=host.get()||actual.owner_before(host)||host.owner_before(actual))
        throw std::runtime_error("GUI screen requires the actual registered FrameOwner UI host");
}
}
