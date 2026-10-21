#include "civ5/ui/bnw_alpha_progression_backend.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include <algorithm>
#include <stdexcept>
#include <thread>

namespace civ5::ui {
namespace {
void need(bool ok,const char* why) {if(!ok)throw std::runtime_error(why);}
template<class A,class B>bool same_control(const A& a,const B& b) {
    return a&&b&&a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
}
UiCommandResult blocked(const char* reason) {return {UiCommandStatus::unavailable,reason};}
}
struct BnwAlphaProgressionBackend::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::weak_ptr<const BnwAlphaProgressionBackend> self;
    std::weak_ptr<runtime::BnwSelectedStockLuaSourceOwner> source;
    std::weak_ptr<const SelectedBnwPreviewHost> host;
    std::weak_ptr<BnwClientSceneFrame> scene;
    BnwAlphaResearchUiPhase phase=BnwAlphaResearchUiPhase::idle;
    std::shared_ptr<const runtime::BnwResearchChoiceCommit> commit;
    std::optional<BnwClientFrameStamp> before;
    std::optional<runtime::BnwResearchChoiceCommand> command;
    // This backend's selected GUI Scene is issued by create_selected_screen_scene.
    // The distinct owned PlotYield storage authority remains unchanged.
    static void require_gui_scene(const std::shared_ptr<BnwClientSceneFrame>& scene,
                                  const std::shared_ptr<const SelectedBnwPreviewHost>& host) {
        need(scene&&scene->gui_thread_==std::this_thread::get_id(),
             "Alpha GUI Scene absent or foreign creator");
        {
            const auto actual=scene->gui_self_.lock();
            need(same_control(actual,scene),"Alpha GUI Scene is not its factory-issued canonical owner");
        } // Release checker pin BEFORE the outside-owner fence.
        need(host&&same_control(scene->selected_host_,host)&&
             &scene->session_==&host->session()&&scene->scene_state_==&scene->session_.state()&&
             scene->scene_state_generation_==scene->session_.state_generation(),
             "Alpha GUI Scene selected Host/state generation changed");
        // Every caller holds exactly two earned Scene pins. Compositor-held
        // pins are counted separately; neither can replace an outside facade.
        need(scene->gui_self_.use_count()>static_cast<long>(scene->gui_publication_pins_.load()+2)&&
             !scene->gui_self_.expired()&&!scene->gui_self_.owner_before(scene)&&
             !scene.owner_before(scene->gui_self_),
             "Alpha GUI Scene outside owner retired");
    }
    static std::shared_ptr<BnwClientSceneFrame> pin_gui_scene(
        const std::shared_ptr<BnwClientSceneFrame>& supplied,
        const std::shared_ptr<const SelectedBnwPreviewHost>& host) {
        need(supplied&&supplied->gui_thread_==std::this_thread::get_id(),
             "Alpha GUI Scene pin absent or foreign creator");
        auto actual=supplied->gui_self_.lock();
        need(same_control(actual,supplied),"Alpha research forged scene control");
        require_gui_scene(actual,host);return actual;
    }
    static void verify_gui_scene(const std::shared_ptr<BnwClientSceneFrame>& scene,
                                 const std::shared_ptr<const SelectedBnwPreviewHost>& host) {
        require_gui_scene(scene,host);
        scene->verify_owned(); // Preserve genuine PlotYield finality when present.
        require_gui_scene(scene,host);
    }
    static void require_gui_ui(const std::shared_ptr<BnwClientSceneFrame>& scene,
        const std::shared_ptr<const SelectedBnwPreviewHost>& host,const std::shared_ptr<LiveUiHost>& ui) {
        require_gui_scene(scene,host);
        need(ui&&ui->plot_yield_ui_thread_==std::this_thread::get_id(),
             "Alpha GUI UI host absent or foreign creator");
        {
            const auto actual=ui->selected_owner_lifetime_.lock();
            need(same_control(actual,ui),"Alpha GUI UI host control is not canonical");
        }
        scene->owner_.require_same_gui_ui_host(ui);
        require_gui_scene(scene,host);
    }
    static std::shared_ptr<LiveUiHost> pin_gui_ui(const std::shared_ptr<BnwClientSceneFrame>& scene,
        const std::shared_ptr<const SelectedBnwPreviewHost>& host,LiveUiHost& supplied) {
        require_gui_scene(scene,host);
        need(supplied.plot_yield_ui_thread_==std::this_thread::get_id(),"Alpha GUI UI pin foreign creator");
        auto ui=supplied.selected_owner_lifetime_.lock();
        need(ui&&ui.get()==&supplied,"Alpha research refresh belongs to another registered UI host");
        require_gui_ui(scene,host,ui);return ui;
    }
    static bool ready_gui_scene(const std::shared_ptr<BnwClientSceneFrame>& scene,
                               const std::shared_ptr<const SelectedBnwPreviewHost>& host) {
        require_gui_scene(scene,host);const auto state=host->session().read_state_lease();
        need(bool(state),"Alpha GUI Scene state allocation retired");
        const bool ready=scene->ready_to_draw();require_gui_scene(scene,host);return ready;
    }
    static bool current_gui_frame(const std::shared_ptr<BnwClientSceneFrame>& scene,
        const std::shared_ptr<const SelectedBnwPreviewHost>& host,const std::shared_ptr<const BnwClientFrame>& frame) {
        require_gui_scene(scene,host);const auto state=host->session().read_state_lease();
        need(bool(state),"Alpha GUI frame state allocation retired");
        const bool current=scene->owner().is_current(frame);require_gui_scene(scene,host);return current;
    }
    static bool current_gui_ui(const std::shared_ptr<BnwClientSceneFrame>& scene,
        const std::shared_ptr<const SelectedBnwPreviewHost>& host,const std::shared_ptr<LiveUiHost>& ui,
        const LiveUiFrame& frame) {
        require_gui_ui(scene,host,ui);const auto state=host->session().read_state_lease();
        need(bool(state),"Alpha GUI UI state allocation retired");
        const bool current=ui->is_current_frame(frame);require_gui_ui(scene,host,ui);return current;
    }
};
BnwAlphaProgressionBackend::BnwAlphaProgressionBackend(std::unique_ptr<Impl> impl):impl_(std::move(impl)) {}
BnwAlphaProgressionBackend::~BnwAlphaProgressionBackend()=default;
std::shared_ptr<BnwAlphaProgressionBackend> BnwAlphaProgressionBackend::bind(
    std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner> source,
    std::shared_ptr<const SelectedBnwPreviewHost> host,std::shared_ptr<BnwClientSceneFrame> supplied_scene) {
    need(source&&host&&supplied_scene,"Alpha research binding needs genuine SourceOwner/Host/scene");
    // Only SourceOwner can call bind; it supplies its factory-issued canonical
    // self. Authenticate actual registered scene and canonical selected parent.
    const auto scene=Impl::pin_gui_scene(supplied_scene,host);
    need(same_control(scene,supplied_scene),"Alpha research forged scene control");
    Impl::verify_gui_scene(scene,host);
    need(same_control(scene->selected_host_,host),"Alpha research scene belongs to another canonical Host");
    auto impl=std::make_unique<Impl>();impl->source=source;impl->host=host;impl->scene=scene;
    auto result=std::shared_ptr<BnwAlphaProgressionBackend>(new BnwAlphaProgressionBackend(std::move(impl)));
    result->impl_->self=result;Impl::verify_gui_scene(scene,host);return result;
}
std::shared_ptr<const BnwAlphaProgressionBackend> BnwAlphaProgressionBackend::retain_self()const {
    need(impl_->thread==std::this_thread::get_id(),"Alpha research foreign thread before mutable UI/world borrow");
    const auto self=impl_->self.lock();need(self&&self.get()==this,"Alpha research canonical backend retired");return self;
}
BnwAlphaResearchUiPhase BnwAlphaProgressionBackend::phase()const {(void)retain_self();return impl_->phase;}
std::shared_ptr<const runtime::BnwResearchChoiceCommit> BnwAlphaProgressionBackend::last_commit()const {
    const auto self=retain_self();return self->impl_->commit;
}
UiCommandResult BnwAlphaProgressionBackend::availability(
    const UiGameplayCommand& command,const UiFrameSnapshot& ui)const {
    try {
        const auto self=retain_self();const auto& p=*self->impl_;
        if(p.phase==BnwAlphaResearchUiPhase::executing||p.phase==BnwAlphaResearchUiPhase::pending_refresh)
            return blocked("Research choice is awaiting actual next-frame publication");
        if(p.phase==BnwAlphaResearchUiPhase::uncertain)
            return blocked("Research outcome is uncertain; reconcile actual owners before another command");
        const auto* choice=command.as<ProgressionChooseTechnology>();
        if(command.source_id()!="TechTree"||!choice||choice->technology<0)
            return {UiCommandStatus::invalid,"Alpha research backend accepts only typed TechTree technology choices"};
        const auto host=p.host.lock();const auto source=p.source.lock();const auto original_scene=p.scene.lock();
        if(!host||!source||!original_scene)return blocked("Actual alpha research source/scene owner expired");
        const auto scene=Impl::pin_gui_scene(original_scene,host);
        need(same_control(scene,original_scene),"Actual alpha research scene allocation retired");Impl::verify_gui_scene(scene,host);
        need(same_control(scene->selected_host_,host),"Actual alpha research canonical selected parent differs");
        const auto frame=scene->frame();
        if(!frame||!Impl::ready_gui_scene(scene,host)||!Impl::current_gui_frame(scene,host,frame))
            return blocked("Alpha research requires the actual current selected client frame");
        const auto* stamp=ui.find<BnwClientFrameStamp>();
        if(!stamp||stamp!=frame->ui.find<BnwClientFrameStamp>()||stamp->identity!=frame->stamp.identity||
            stamp->selection!=frame->stamp.selection||stamp->viewport!=frame->stamp.viewport||
            ui.frame()!=frame->ui.frame()||ui.active_player_id()!=frame->ui.active_player_id()||
            ui.turn()!=frame->ui.turn())return blocked("Alpha technology button uses a stale/foreign frame");
        const auto* progression=progression_for_active_player(ui);
        const auto state_pin=host->session().read_state_lease();
        need(bool(state_pin),"Alpha research session state retired");
        const auto& live=*state_pin;
        if(!progression||progression->player_id<0||progression->player_id>=64||
            progression!=frame->ui.find<ProgressionFrameState>()||
            progression->player_id!=host->active_player_id()||progression->team_id!=stamp->identity.team||
            progression->player_id!=runtime::detail::bnw_game_active_player_read(live)||
            progression->team_id!=runtime::detail::bnw_game_active_team_read(live)||
            stamp->identity.ui_source_revision!=live.ui_source_revision||stamp->identity.turn!=live.setup.game_turn)
            return blocked("Alpha progression publication differs from the actual active player");
        const auto* technology=host->technologies().find(static_cast<std::uint64_t>(choice->technology));
        const auto item=std::find_if(progression->technologies.begin(),progression->technologies.end(),
            [&](const auto& value){return value.id==choice->technology;});
        const auto player=live.setup.players.find(progression->player_id);
        const auto research=live.player_research.players.find(progression->player_id);
        const auto turn=live.research_turn.players.find(progression->player_id);
        const auto selection=live.research_selection.players.find(progression->player_id);
        const auto team=std::find_if(live.technologies.begin(),live.technologies.end(),
            [&](const auto& value){return value.team()==progression->team_id;});
        if(!technology||item==progression->technologies.end()||item->type!=technology->base.type||
            player==live.setup.players.end()||!player->second.alive||research==live.player_research.players.end()||
            turn==live.research_turn.players.end()||selection==live.research_selection.players.end()||team==live.technologies.end())
            return blocked("Alpha technology ID or genuine research player is unavailable");
        if(host->technologies().profile!=content::TechnologyProfile::bnw||!host->technologies().all_succeeded()||
            host->defines().profile()!=content::GlobalDefineProfile::bnw||
            live.setup.profile!=content::TechnologyProfile::bnw||
            live.research_selection.profile!=content::TechnologyProfile::bnw)
            return blocked("Alpha choice requires complete activated BNW research rules");
        const bool queued=std::find(research->second.queue.begin(),research->second.queue.end(),choice->technology)!=research->second.queue.end();
        const auto current=research->second.current_research();
        if(item->researched!=team->has_tech(choice->technology)||item->queued!=queued||
            progression->current_research!=(current<0?std::optional<std::int64_t>{}:std::optional<std::int64_t>{current})||
            item->researched||queued||turn->second.researching.contains(choice->technology))
            return blocked("Alpha technology is already owned/queued or its button state is stale");
        if((!turn->second.founded_first_city&&selection->second.advanced_start_points<0)||
            !runtime::detail::bnw_player_human_read(live,static_cast<std::int32_t>(progression->player_id),host->database())||
            technology->booleans.at("Disable")||!selection->second.civilization||
            live.research_selection.disabled_technologies.at(*selection->second.civilization).contains(choice->technology)||
            !runtime::research_prerequisites_met(host->technologies(),*team,choice->technology,
                runtime::research_prerequisite_limits(host->defines()),false))
            return blocked("Alpha first choice supports only currently researchable technologies");
        Impl::verify_gui_scene(scene,host);
        if(host->session().read_state_lease()!=state_pin||scene->frame()!=frame||!Impl::current_gui_frame(scene,host,frame))
            return blocked("Alpha button frame changed during source checks");
        // Native eligibility only. Genuine SourceOwner performs authoritative
        // SAME-VM TestAll during execute; hover does not dispatch gameplay Lua.
        return {UiCommandStatus::ready,{}};
    } catch(const std::exception& error) {return {UiCommandStatus::unavailable,error.what()};}
}
UiCommandResult BnwAlphaProgressionBackend::execute(const UiGameplayCommand& command,const UiFrameSnapshot& ui) {
    const auto self=retain_self();auto& p=*self->impl_;
    const auto checked=availability(command,ui);if(checked.status!=UiCommandStatus::ready)return checked;
    const auto source=p.source.lock();const auto host=p.host.lock();const auto supplied_scene=p.scene.lock();
    if(!source||!host||!supplied_scene)return blocked("Actual alpha research owner expired before execute");
    const auto scene=Impl::pin_gui_scene(supplied_scene,host);need(same_control(scene,supplied_scene),"Alpha research canonical scene retired");
    Impl::verify_gui_scene(scene,host);const auto old=scene->frame();
    need(old&&Impl::current_gui_frame(scene,host,old),"Alpha research publication changed before command conversion");
    need(ui.find<BnwClientFrameStamp>()==old->ui.find<BnwClientFrameStamp>()&&
        ui.active_player_id()==old->ui.active_player_id()&&ui.turn()==old->ui.turn(),
        "Alpha research input frame changed after availability");
    const auto* choice=command.as<ProgressionChooseTechnology>();
    runtime::BnwResearchChoiceCommand input{old->stamp.identity.player,choice->technology,old->stamp.identity.turn,
        host->session().state_generation(),old->stamp.identity.ui_source_revision,choice->clear_queue};
    p.before=old->stamp;p.command=input;
    const auto previous=p.commit; // Retain old callback owners across current operation.
    p.phase=BnwAlphaResearchUiPhase::executing;p.commit.reset();
    try {
        // Mandatory genuine SourceOwner method, supplied by graphics. No public
        // research services/callbacks or generic transaction are injected here.
        p.commit=source->run_same_research_choice(input);
        Impl::verify_gui_scene(scene,host);
        if(!p.commit||p.commit->phase()!=runtime::BnwResearchChoicePhase::complete) {
            p.phase=BnwAlphaResearchUiPhase::uncertain;
            return {UiCommandStatus::uncertain,"Research action did not complete its genuine owner tail; do not retry"};
        }
        p.commit->require_stored_current(*host);Impl::verify_gui_scene(scene,host);
        if(p.commit->player()!=input.player||p.commit->before_revision()!=input.expected_ui_source_revision||
            p.commit->after_revision()!=input.expected_ui_source_revision+1||
            host->session().state_generation()!=input.expected_generation||
            host->session().state().ui_source_revision!=p.commit->after_revision())
            throw std::runtime_error("Genuine research result belongs to another command/source revision");
        p.phase=BnwAlphaResearchUiPhase::pending_refresh;
        return {UiCommandStatus::pending_refresh,"Research committed; awaiting actual fresh scene and UI frame"};
    } catch(const runtime::BnwResearchChoiceSourceFailure& error) {
        // SourceOwner may fail renderer/source bookends AFTER successful core
        // stores. Keep its actual opaque receipt: assignment from the throwing
        // run_same_research_choice expression never reached the ordinary path.
        p.phase=BnwAlphaResearchUiPhase::uncertain;
        p.commit=error.receipt();
        return {UiCommandStatus::uncertain,"Research stores may be committed; source/UI outcome is uncertain; do not retry"};
    } catch(const std::exception& error) {
        p.phase=BnwAlphaResearchUiPhase::uncertain;
        return {UiCommandStatus::uncertain,std::string("Research operation is uncertain; do not retry: ")+error.what()};
    } catch(...) {p.phase=BnwAlphaResearchUiPhase::uncertain;return {UiCommandStatus::uncertain,"Research operation is uncertain; do not retry"};}
}
UiCommandResult BnwAlphaProgressionBackend::complete_after_refresh(
    BnwClientSceneFrame& supplied_scene,LiveUiHost& supplied_ui,const LiveUiFrame& refreshed) {
    const auto self=retain_self();auto& p=*self->impl_;
    if(p.phase!=BnwAlphaResearchUiPhase::pending_refresh||!p.commit||!p.before||!p.command)
        return blocked("No alpha research command is awaiting frame publication");
    // Consume the UI completion attempt BEFORE any callbackful guard. Reentry
    // cannot repeat SourceOwner completion or the already committed action.
    // The core phase stays untouched: source/UI uncertainty is a separate state.
    p.phase=BnwAlphaResearchUiPhase::uncertain;
    try {
        const auto host=p.host.lock();const auto expected_scene=p.scene.lock();const auto source=p.source.lock();
        need(host&&expected_scene&&source,"Actual alpha research owner expired before publication");
        const auto scene=Impl::pin_gui_scene(expected_scene,host);
        need(same_control(scene,expected_scene)&&scene.get()==&supplied_scene&&same_control(scene->selected_host_,host),
            "Alpha research refresh belongs to another registered scene/Host");
        const auto ui=Impl::pin_gui_ui(scene,host,supplied_ui);
        need(ui&&ui.get()==&supplied_ui,"Alpha research refresh belongs to another registered UI host");
        Impl::verify_gui_scene(scene,host);ui->require_issued_frame_finality(refreshed);Impl::require_gui_ui(scene,host,ui);
        p.commit->require_stored_current(*host);Impl::verify_gui_scene(scene,host);
        need(p.commit->phase()==runtime::BnwResearchChoicePhase::complete,
            "Actual research commit became uncertain before fresh publication");
        const auto frame=scene->frame();const auto* stamp=refreshed.snapshot.find<BnwClientFrameStamp>();
        need(frame&&Impl::ready_gui_scene(scene,host)&&Impl::current_gui_frame(scene,host,frame)&&Impl::current_gui_ui(scene,host,ui,refreshed)&&
            stamp&&stamp==frame->ui.find<BnwClientFrameStamp>()&&stamp->identity==frame->stamp.identity&&
            stamp->selection==frame->stamp.selection&&stamp->viewport==frame->stamp.viewport,
            "Actual refreshed research scene/UI publication is unavailable");
        const auto& now=frame->stamp.identity;const auto& old=p.before->identity;
        need(now.owner==old.owner&&now.frame>old.frame&&now.session==old.session&&now.player==old.player&&
            now.team==old.team&&now.turn==old.turn&&now.map_guid==old.map_guid&&
            now.selection_revision==old.selection_revision&&frame->stamp.selection==p.before->selection&&
            now.ui_source_revision==p.commit->after_revision()&&
            host->session().state_generation()==p.command->expected_generation,
            "Research refresh did not publish the exact new source/selection revision");
        const auto* progression=progression_for_active_player(refreshed.snapshot);
        const auto& research=host->session().state().player_research.players.at(now.player);
        const auto current=research.current_research();
        need(progression&&progression==frame->ui.find<ProgressionFrameState>()&&
            progression->player_id==now.player&&progression->team_id==now.team&&
            progression->current_research==(current<0?std::optional<std::int64_t>{}:std::optional<std::int64_t>{current})&&
            std::any_of(progression->technologies.begin(),progression->technologies.end(),[&](const auto& row) {
                return row.id==p.command->technology&&row.queued&&!row.researched;
            }),"Fresh technology button model does not show actual committed research");
        // Only actual UiScreenCompositor issuance installs this weak Scene
        // link. Lock BEFORE callbackful retain_owner; do not borrow a raw facade
        // which a guard can destroy, or accept an arbitrary caller GUI object.
        const auto issued_gui=scene->gui_screen_.lock();
        need(bool(issued_gui),"Research refresh has no actual compositor-issued GUI publication");
        const auto gui=issued_gui->retain_owner();
        need(same_control(gui,issued_gui),"Research GUI uses forged/noncanonical owner control");
        const auto require_publication=[&] {
            Impl::verify_gui_scene(scene,host);ui->require_issued_frame_finality(refreshed);Impl::require_gui_ui(scene,host,ui);
            p.commit->require_stored_current(*host);Impl::verify_gui_scene(scene,host);
            const bool frame_current=Impl::current_gui_frame(scene,host,frame);
            const bool ui_current=Impl::current_gui_ui(scene,host,ui,refreshed);
            // Final callback-free membership checks follow ALL guard callbacks;
            // an observer cannot replace a previously checked GUI/frame link.
            need(frame_current&&ui_current&&same_control(scene->gui_screen_.lock(),gui)&&
                same_control(scene->frame_,frame)&&same_control(scene->selected_host_,host)&&
                host->session().state_generation()==p.command->expected_generation&&
                host->session().state().ui_source_revision==p.commit->after_revision(),
                "Research actual Scene/GUI publication changed during completion");
        };
        require_publication();gui->require_same_selected_host(*host);require_publication();
        gui->require_same_research_choice(*p.commit);require_publication();
        gui->require_current();require_publication();
        source->complete_same_research_choice_after_refresh(p.commit,gui);
        require_publication();gui->require_same_research_choice(*p.commit);require_publication();
        gui->require_current();require_publication();
        p.phase=BnwAlphaResearchUiPhase::complete;
        return {UiCommandStatus::performed,"Research choice and actual next-frame publication completed"};
    } catch(const std::exception& error) {
        p.phase=BnwAlphaResearchUiPhase::uncertain;
        return {UiCommandStatus::uncertain,std::string("Research committed but refresh is uncertain; do not retry: ")+error.what()};
    } catch(...) {p.phase=BnwAlphaResearchUiPhase::uncertain;return {UiCommandStatus::uncertain,"Research committed but refresh is uncertain; do not retry"};}
}
}
