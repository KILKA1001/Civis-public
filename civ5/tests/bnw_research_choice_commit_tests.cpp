#include "civ5/runtime/bnw_research_choice_commit.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <type_traits>

namespace civ5::tests {
namespace {
void need(bool ok,const char* why) {if(!ok)throw std::runtime_error(why);}
template<class F>void refuses(F&& f) {
    bool refused=false;try {f();}catch(const std::exception&) {refused=true;}
    need(refused,"Expected research action refusal missing");
}
template<class T>concept PublicIssue=requires(std::shared_ptr<const ui::SelectedBnwPreviewHost> h,
    runtime::BnwStockLuaAttemptLease lease,typename T::TestAll test,typename T::AcceptCommit accept,typename T::Dirty dirty) {
    T::issue(h,lease,test,accept,dirty);
};
static_assert(!PublicIssue<runtime::BnwResearchChoiceHooks>);
static_assert(!std::is_default_constructible_v<runtime::BnwResearchChoiceHooks>);
static_assert(!std::is_default_constructible_v<runtime::BnwResearchChoiceCommit>);
}
// Root's genuine selected-client integration fixture calls this with an actual
// activated SourceOwner issuer and a CURRENT owned scene. No local issuer,
// scripted result, synthetic world, or fabricated renderer receipt is supplied.
void verify_actual_bnw_research_choice_commit(
    const std::shared_ptr<ui::SelectedBnwPreviewHost>& host,
    ui::BnwClientFrameOwner& scene,
    const std::shared_ptr<const runtime::BnwResearchChoiceHooks>& hooks,
    const runtime::BnwResearchChoiceCommand& command) {
    using namespace runtime;
    need(host&&hooks,"Actual research integration dependencies absent");
    auto& session=host->session();
    const auto before=scene.current_frame();
    need(before&&scene.is_current(before),"Actual selected scene is not current");
    const auto state=session.read_state_lease();const auto generation=session.state_generation();
    const auto visibility=bnw_live_plot_visibility_cache(*state);
    const auto map_rng=state->random.map.state(),game_rng=state->random.gameplay.state();
    const auto old_queue=state->player_research.players.at(command.player).queue;
    const auto source_bytes=session.source().source.bytes;
    const auto flags_before=state->research_turn.players.at(command.player).researching;
    need(!flags_before.contains(command.technology),"Genuine positive must select a new technology");
    need(command.expected_generation==generation&&command.expected_turn==before->stamp.identity.turn&&
        command.expected_ui_source_revision==before->stamp.identity.ui_source_revision&&
        command.player==before->stamp.identity.player,"Actual typed action and presented scene differ");

    // A no-op-deleter alias to a still-live genuine issuer must fail BEFORE
    // consuming it or touching the selected world. This is a control fault,
    // never the positive source of gameplay permissions.
    const std::shared_ptr<const BnwResearchChoiceHooks> forged(hooks.get(),[](const auto*){});
    refuses([&] {(void)BnwResearchChoiceCommit::execute(session,forged,command);});
    need(session.read_state_lease()==state&&state->player_research.players.at(command.player).queue==old_queue&&
        state->ui_source_revision==command.expected_ui_source_revision,
        "Forged hooks changed genuine world before canonical admission");
    auto stale=command;stale.expected_generation=0;
    refuses([&] {(void)BnwResearchChoiceCommit::execute(session,hooks,stale);});
    bool foreign_refused=false;
    std::thread foreign([&] {
        try {(void)BnwResearchChoiceCommit::execute(session,hooks,command);}
        catch(const std::exception&) {foreign_refused=true;}
    });
    foreign.join();
    need(foreign_refused&&state->player_research.players.at(command.player).queue==old_queue&&
        state->ui_source_revision==command.expected_ui_source_revision,
        "Foreign command entered actual VM/world or consumed the owner hook");

    const auto commit=BnwResearchChoiceCommit::execute(session,hooks,command);
    need(commit&&commit->phase()==BnwResearchChoicePhase::complete,
        "Actual SourceOwner/dirty tail did not complete; positive action NOT accepted");
    commit->require_stored_current(*host);
    need(session.read_state_lease()==state&&session.state_generation()==generation,
        "Research choice replaced the real state allocation/generation");
    need(state->player_research.players.at(command.player).queue!=old_queue&&
        state->research_turn.players.at(command.player).researching.contains(command.technology)&&
        state->ui_source_revision==command.expected_ui_source_revision+1&&
        commit->queue()==state->player_research.players.at(command.player).queue&&
        commit->completed_dirty_calls()>0&&commit->completed_dirty_calls()%3==0,
        "Actual typed input did not change research queue with ordered dirty tail");
    const auto& after_visibility=bnw_live_plot_visibility_cache(session.state());
    need(after_visibility.current&&after_visibility.counts==visibility.counts&&
        after_visibility.source_raw==visibility.source_raw&&
        after_visibility.team_slots==visibility.team_slots&&after_visibility.wire_slots==visibility.wire_slots&&
        state->random.map.state()==map_rng&&state->random.gameplay.state()==game_rng&&
        session.source().source.bytes==source_bytes,
        "Research selection changed visibility, RNG or original save");
    need(!scene.is_current(before),"Old presented frame remained input-current after real queue write");
    const auto next=scene.capture(before->stamp.viewport,before->camera);
    need(next&&scene.is_current(next)&&next->stamp.identity.ui_source_revision==state->ui_source_revision&&
        next->visibility.plots==before->visibility.plots&&next->visibility.team_id==before->visibility.team_id&&
        next->visibility.player_id==before->visibility.player_id,
        "Real action could not publish a fresh frame with the SAME visibility");
    refuses([&] {(void)BnwResearchChoiceCommit::execute(session,hooks,command);});
    need(scene.is_current(next)&&session.read_state_lease()==state&&
        state->player_research.players.at(command.player).queue==commit->queue(),
        "Same operation replay invalidated the accepted next frame");
}
}

#ifndef CIVIS_RESEARCH_CHOICE_ACTUAL_CLIENT_FIXTURE
int main(int argc,char** argv) {
    try {
        if(argc!=3)throw std::runtime_error("Usage: actual-save actual-installation");
        auto boot=civ5::ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0,true);
        if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
        if(!boot.preview.host)throw std::runtime_error("Actual BNW Host absent");
        auto& session=boot.preview.host->session();
        const auto state=session.read_state_lease();
        const auto rng=state->random.gameplay.state();
        civ5::tests::refuses([&] {(void)civ5::runtime::BnwResearchChoiceCommit::execute(session,{},{});});
        civ5::tests::need(session.read_state_lease()==state&&state->random.gameplay.state()==rng,
            "Missing genuine hooks mutated the actual imported state");
        std::cout<<"UNAVAILABLE: genuine selected SourceOwner/VM issuer and current scene positive fixture required\n";
        return 2; // Null-only negative coverage is NOT a passed gameplay gate.
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
#endif
