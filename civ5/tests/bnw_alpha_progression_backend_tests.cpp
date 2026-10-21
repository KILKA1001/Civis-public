#include "civ5/ui/bnw_alpha_progression_backend.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace civ5::tests {
namespace {
void need(bool ok,const char* why) {if(!ok)throw std::runtime_error(why);}
template<class T>concept PublicBind=requires(
    std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner> source,
    std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::shared_ptr<ui::BnwClientSceneFrame> scene) {
    T::bind(source,host,scene);
};
static_assert(!PublicBind<ui::BnwAlphaProgressionBackend>);
static_assert(!std::is_default_constructible_v<ui::BnwAlphaProgressionBackend>);
static_assert(!std::is_constructible_v<runtime::BnwResearchChoiceSourceFailure,
    std::shared_ptr<const runtime::BnwResearchChoiceCommit>,std::string>);
}
// Genuine selected-client fixture, not a local callback supplier. Root's joined
// graphics test provides actual SourceOwner-bound backend, canonical scene/UI,
// real selected compositor and an actual CURRENT already displayed TechTree.
std::shared_ptr<const runtime::BnwResearchChoiceCommit> verify_actual_alpha_progression_technology_button(
    const std::shared_ptr<ui::BnwAlphaProgressionBackend>& backend,
    const std::shared_ptr<ui::SelectedBnwPreviewHost>& host,
    const std::shared_ptr<ui::BnwClientSceneFrame>& scene,
    const std::shared_ptr<ui::LiveUiHost>& ui_host,
    const ui::UiScreenCompositor& compositor,const ui::LiveUiFrame& before,
    std::int64_t technology) {
    using namespace ui;
    need(backend&&host&&scene&&ui_host,"Genuine alpha progression client dependencies absent");
    need(scene->accepts(*ui_host,before)&&ui_host->screens().focused_screen()==std::optional<std::string>{"TechTree"},
        "Actual displayed TechTree is unavailable");
    const auto old=scene->frame();const auto state=host->session().read_state_lease();
    const auto generation=host->session().state_generation();const auto viewport=old->stamp.viewport;
    const auto map_rng=state->random.map.state(),game_rng=state->random.gameplay.state();
    const auto old_queue=state->player_research.players.at(host->active_player_id()).queue;
    const auto command=progression_choose_technology(technology);
    const auto& choice=std::get<UiGameplayCommand>(command);
    need(backend->availability(choice,before.snapshot).status==UiCommandStatus::ready,
        "Actual alpha fixture must choose a naturally available technology");
    const auto composed=compositor.compose(before,ui_host->screens(),viewport.framebuffer_width,viewport.framebuffer_height);
    const auto hit=std::find_if(composed.hits.begin(),composed.hits.end(),[&](const LiveUiHit& value) {
        const auto* gameplay=std::get_if<UiGameplayCommand>(&value.command);
        const auto* selected=gameplay?gameplay->as<ProgressionChooseTechnology>():nullptr;
        return value.context_id=="TechTree"&&gameplay&&gameplay->source_id()=="TechTree"&&selected&&
            selected->technology==technology&&value.enabled&&value.bounds.width>0&&value.bounds.height>0;
    });
    need(hit!=composed.hits.end(),"Actual compositor did not create this typed technology button");
    const auto x=hit->bounds.x+hit->bounds.width/2,y=hit->bounds.y+hit->bounds.height/2;
    bool consumed=false;
    const auto result=scene->dispatch(*ui_host,before,std::function<UiCommandResult()>{[&] {
        const auto clicked=click_composed_ui(*ui_host,before,composed,x,y,x,y);
        consumed=clicked.consumed;return clicked.command;
    }});
    need(consumed&&result.status==UiCommandStatus::pending_refresh&&
        backend->phase()==BnwAlphaResearchUiPhase::pending_refresh,
        "Technology click must commit as pending, never report success before refreshed frame");
    const auto commit=backend->last_commit();
    need(commit&&commit->phase()==runtime::BnwResearchChoicePhase::complete,
        "Real clicked technology did not finish its genuine core/UI handlers");
    commit->require_stored_current(*host);
    need(host->session().read_state_lease()==state&&host->session().state_generation()==generation&&
        state->player_research.players.at(host->active_player_id()).queue!=old_queue&&
        state->research_turn.players.at(host->active_player_id()).researching.contains(technology)&&
        state->ui_source_revision==old->stamp.identity.ui_source_revision+1&&
        state->random.map.state()==map_rng&&state->random.gameplay.state()==game_rng,
        "Actual button failed to change only live research without state replacement/RNG");
    need(backend->availability(choice,before.snapshot).status==UiCommandStatus::unavailable,
        "Pending research publication did not block a repeated button");
    const auto repeated=backend->execute(choice,before.snapshot);
    need(repeated.status==UiCommandStatus::unavailable&&backend->last_commit()==commit&&
        state->ui_source_revision==commit->after_revision(),"Repeated pending click executed another mutation");

    // Actual production renderer MUST now finish its normal paint/present
    // loop. Do not mint a renderer-lifetime token or publication callback here.
    // Return the genuine receipt for the after-presentation assertion helper.
    return commit;
}
// Call AFTER the production camera presenter has naturally called
// complete_presented_research_refresh. That caller owns the genuine renderer
// sink, publishes its new GUI and invokes backend completion after paint/present.
void verify_actual_alpha_progression_presented_completion(
    const std::shared_ptr<ui::BnwAlphaProgressionBackend>& backend,
    const std::shared_ptr<ui::SelectedBnwPreviewHost>& host,
    const std::shared_ptr<ui::BnwClientSceneFrame>& scene,
    const std::shared_ptr<ui::LiveUiHost>& ui_host,
    const std::shared_ptr<const ui::BnwClientFrame>& before,
    const std::shared_ptr<const runtime::BnwResearchChoiceCommit>& commit,
    const std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>& actual_presented_gui,
    const ui::LiveUiFrame& refreshed,std::int64_t technology) {
    using namespace ui;
    need(backend&&host&&scene&&ui_host&&before&&commit&&actual_presented_gui,
        "Actual before/after research presentation fixture is absent");
    need(backend->phase()==BnwAlphaResearchUiPhase::complete&&backend->last_commit()==commit,
        "Real production presenter did not complete the research outcome");
    const auto gui=actual_presented_gui->retain_owner();
    need(gui.get()==actual_presented_gui.get()&&!gui.owner_before(actual_presented_gui)&&
        !actual_presented_gui.owner_before(gui),"Actual renderer GUI control differs from canonical issuer");
    gui->require_same_selected_host(*host);gui->require_same_research_choice(*commit);gui->require_current();
    commit->require_stored_current(*host);
    const auto next=scene->frame();
    need(next&&scene->owner().is_current(next)&&next->visibility.plots==before->visibility.plots&&
        next->visibility.team_id==before->visibility.team_id&&
        next->stamp.identity.ui_source_revision==commit->after_revision()&&ui_host->is_current_frame(refreshed)&&
        next->stamp.identity.frame>before->stamp.identity.frame,
        "Actual new research frame/UI did not finish the pending gameplay action");
    const auto* progression=progression_for_active_player(refreshed.snapshot);
    need(progression&&std::any_of(progression->technologies.begin(),progression->technologies.end(),
        [&](const auto& row){return row.id==technology&&row.queued&&!row.researched;}),
        "Actual presented technology button does not show committed research");
    const auto state=host->session().read_state_lease();
    need(backend->complete_after_refresh(*scene,*ui_host,refreshed).status==UiCommandStatus::unavailable&&
        backend->last_commit()==commit&&state->player_research.players.at(host->active_player_id()).queue==commit->queue(),
        "Completed publication replayed gameplay or returned another success");
    const auto choice=std::get<UiGameplayCommand>(progression_choose_technology(technology));
    need(backend->availability(choice,before->ui).status==UiCommandStatus::unavailable,
        "Old technology button became current again after refresh");
}
// Run in a separate genuine clicked client fixture. The input is its actual
// old issued frame, not a fabricated bad frame or callback that reports failure.
void verify_actual_alpha_progression_stale_refresh_is_uncertain(
    ui::BnwAlphaProgressionBackend& backend,ui::SelectedBnwPreviewHost& host,
    ui::BnwClientSceneFrame& scene,ui::LiveUiHost& ui_host,const ui::LiveUiFrame& old) {
    need(backend.phase()==ui::BnwAlphaResearchUiPhase::pending_refresh,
        "Stale-refresh fixture must first perform the real typed button action");
    const auto commit=backend.last_commit();
    need(commit&&commit->phase()==runtime::BnwResearchChoicePhase::complete,
        "Stale-refresh fixture has no genuine committed research");
    commit->require_stored_current(host);
    const auto queue=host.session().state().player_research.players.at(commit->player()).queue;
    need(!queue.empty(),"Actual research commit has no queued target");
    const auto revision=host.session().state().ui_source_revision;
    const auto result=backend.complete_after_refresh(scene,ui_host,old);
    need(result.status==ui::UiCommandStatus::uncertain&&
        backend.phase()==ui::BnwAlphaResearchUiPhase::uncertain&&backend.last_commit()==commit&&
        host.session().state().player_research.players.at(commit->player()).queue==queue&&
        host.session().state().ui_source_revision==revision,
        "Old actual frame falsely completed or replayed the research command");
    const auto command=std::get<ui::UiGameplayCommand>(ui::progression_choose_technology(queue.back()));
    need(backend.availability(command,old.snapshot).status==ui::UiCommandStatus::unavailable&&
        backend.execute(command,old.snapshot).status==ui::UiCommandStatus::unavailable&&
        host.session().state().ui_source_revision==revision,
        "Uncertain refresh allowed a second gameplay execution");
}
// Separate genuine pending fixture: publish a real new Scene/UI frame but
// intentionally do NOT invoke the compositor's owned-GUI factory. Real frames
// and source revision alone must never acknowledge the completed UI tail.
void verify_actual_alpha_progression_frame_without_gui_refuses(
    ui::BnwAlphaProgressionBackend& backend,ui::SelectedBnwPreviewHost& host,
    ui::BnwClientSceneFrame& scene,ui::LiveUiHost& ui_host,
    const ui::LiveUiFrame& actual_old_frame,
    const std::shared_ptr<const ui::BnwClientFrame>& actual_old_scene_frame) {
    need(backend.phase()==ui::BnwAlphaResearchUiPhase::pending_refresh,
        "Missing-GUI fixture must first perform the genuine typed button action");
    const auto receipt=backend.last_commit();
    need(receipt&&receipt->phase()==runtime::BnwResearchChoicePhase::complete,
        "Missing-GUI fixture has no actual completed core receipt");
    receipt->require_stored_current(host);
    const auto* old_stamp=actual_old_frame.snapshot.find<ui::BnwClientFrameStamp>();
    need(bool(old_stamp),"Missing-GUI fixture has no actual original scene stamp");
    need(actual_old_scene_frame&&old_stamp==actual_old_scene_frame->ui.find<ui::BnwClientFrameStamp>(),
        "Missing-GUI fixture has no retained matching actual original Scene frame");
    const auto queue=host.session().state().player_research.players.at(receipt->player()).queue;
    const auto revision=host.session().state().ui_source_revision;
    const auto next=scene.begin(old_stamp->viewport,actual_old_scene_frame->camera);
    const auto fresh_ui=ui_host.begin_frame(old_stamp->viewport.framebuffer_width,old_stamp->viewport.framebuffer_height);
    need(next&&scene.owner().is_current(next)&&ui_host.is_current_frame(fresh_ui),
        "Missing-GUI fixture did not genuinely publish fresh Scene/UI frames");
    need(backend.complete_after_refresh(scene,ui_host,fresh_ui).status==ui::UiCommandStatus::uncertain&&
        backend.phase()==ui::BnwAlphaResearchUiPhase::uncertain&&backend.last_commit()==receipt&&
        host.session().state().ui_source_revision==revision&&
        host.session().state().player_research.players.at(receipt->player()).queue==queue,
        "Fresh frame without compositor GUI falsely completed or changed research stores");
}
// Root's separate genuine client fixture provokes an ACTUAL post-core source
// or renderer bookend failure through its real owner lifetime. No local throw
// callback or manufactured BnwResearchChoiceSourceFailure is injected here.
void verify_actual_alpha_progression_post_commit_source_failure(
    ui::BnwAlphaProgressionBackend& backend,ui::SelectedBnwPreviewHost& host,
    const ui::LiveUiFrame& actual_old_frame) {
    need(backend.phase()==ui::BnwAlphaResearchUiPhase::uncertain,
        "Actual post-core source failure did not consume the UI operation");
    const auto receipt=backend.last_commit();
    need(receipt&&receipt->phase()==runtime::BnwResearchChoicePhase::complete,
        "Actual post-core SourceOwner failure lost its completed opaque receipt");
    receipt->require_stored_current(host);
    const auto state=host.session().read_state_lease();
    const auto queue=state->player_research.players.at(receipt->player()).queue;
    const auto revision=state->ui_source_revision;
    need(!queue.empty(),"Actual failed source operation did not preserve committed research stores");
    const auto command=std::get<ui::UiGameplayCommand>(ui::progression_choose_technology(queue.back()));
    need(backend.availability(command,actual_old_frame.snapshot).status==ui::UiCommandStatus::unavailable&&
        backend.execute(command,actual_old_frame.snapshot).status==ui::UiCommandStatus::unavailable&&
        backend.last_commit()==receipt&&host.session().read_state_lease()==state&&
        state->ui_source_revision==revision&&state->player_research.players.at(receipt->player()).queue==queue,
        "Actual post-core SourceOwner failure replayed/rolled back committed research");
}
}

#ifndef CIVIS_ALPHA_PROGRESSION_ACTUAL_CLIENT_FIXTURE
int main() {
    std::cout<<"UNAVAILABLE: invoke actual selected SourceOwner/client/button fixture; no synthetic issuer supplied\n";
    return 2; // No null-only or fabricated success may satisfy the alpha gate.
}
#endif
