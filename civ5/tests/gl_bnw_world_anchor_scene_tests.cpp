#include "civ5/graphics/gl_bnw_world_anchor_scene.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
using namespace civ5;
void check(bool valid,const char *reason) {
    if(!valid) throw std::runtime_error(reason);
}
struct MockDrawables final:graphics::GlBnwWorldAnchorDrawBackend {
    const graphics::GlContext *gl=nullptr;
    runtime::BnwWorldAnchorGraphicsIdentity identity;
    bool present=true;
    bool throw_after_remove=false;
    unsigned removed=0;
    const graphics::GlContext *context() const noexcept override {return gl;}
    runtime::BnwWorldAnchorGraphicsIdentity world_identity() const override {
        return identity;
    }
    bool has_drawable(std::uint64_t plot,std::int32_t anchor,
                      std::int32_t x,std::int32_t y) const override {
        return present && plot==635 && anchor==0 && x==35 && y==15;
    }
    void remove_drawable(std::uint64_t plot,std::int32_t anchor,
                         std::int32_t x,std::int32_t y,
                         std::int32_t event_data) override {
        check(has_drawable(plot,anchor,x,y) && event_data==-1,
              "Mock received an incorrect SDK anchor event");
        present=false;
        ++removed;
        if(throw_after_remove)
            throw std::runtime_error("Mock draw backend failed after removal");
    }
};
void selected(const std::filesystem::path &save,
              const std::filesystem::path &game_root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    const auto &state=bootstrap.preview.host->session().state();
    const auto &map=state.world.map_header;
    auto disconnected=std::make_shared<graphics::GlBnwWorldAnchorScene>(state,nullptr);
    check(disconnected->retained_anchor_count()==1 &&
          disconnected->current_world()==runtime::BnwWorldAnchorGraphicsIdentity{
              map.guid,map.grid.width,map.grid.height} &&
          !disconnected->supports_gameplay_world_anchor() &&
          !disconnected->publication_context_current() &&
          !runtime::bind_bnw_world_anchor_graphics(state,disconnected).ready(),
          "Gandhi scene claimed a missing GL anchor renderer");

    auto gl=std::make_shared<graphics::GlContext>(64,64,false);
    auto undrawn=std::make_shared<graphics::GlBnwWorldAnchorScene>(state,gl);
    check(undrawn->publication_context_current() &&
          !undrawn->supports_gameplay_world_anchor() &&
          !runtime::bind_bnw_world_anchor_graphics(state,undrawn).ready(),
          "GL context alone falsely claimed anchor rendering");
    auto mock=std::make_shared<MockDrawables>();
    mock->gl=gl.get();
    mock->identity={map.guid,map.grid.width,map.grid.height};
    auto mismatched=std::make_shared<MockDrawables>(*mock);
    ++mismatched->identity.map_guid[0];
    auto foreign=std::make_shared<graphics::GlBnwWorldAnchorScene>(
        state,gl,mismatched);
    check(!foreign->supports_gameplay_world_anchor() &&
          !runtime::bind_bnw_world_anchor_graphics(state,foreign).ready(),
          "Graphics backend for another map was accepted");
    mock->present=false;
    auto missing=std::make_shared<graphics::GlBnwWorldAnchorScene>(state,gl,mock);
    check(!missing->supports_gameplay_world_anchor() &&
          !runtime::bind_bnw_world_anchor_graphics(state,missing).ready(),
          "Undrawn source anchor was accepted");
    mock->present=true;
    auto scene=std::make_shared<graphics::GlBnwWorldAnchorScene>(state,gl,mock);
    check(scene->supports_gameplay_world_anchor(),
          "Complete mock drawable roster was rejected");
    auto binding=runtime::bind_bnw_world_anchor_graphics(state,scene);
    check(binding.ready(),"GL scene could not bind the guarded SDK emitter");
    auto draft=state;
    runtime::BnwWorldAnchorJournal journal(std::move(binding.emitter));
    check(runtime::stage_bnw_world_anchor_clear(draft,635,journal).outcome==
              runtime::LiveActionOutcome::committed && mock->removed==0,
          "Graphics event escaped before game-turn commit");
    check(journal.publish(draft).status==
              runtime::BnwWorldAnchorPublicationStatus::complete &&
          mock->removed==1 && scene->retained_anchor_count()==0 &&
          !scene->publication_uncertain(),
          "Mock draw backend missed selected Gandhi anchor removal");
    check(journal.publish(draft).status==
              runtime::BnwWorldAnchorPublicationStatus::complete &&
          mock->removed==1,
          "Published anchor event was repeated");
    auto failure_backend=std::make_shared<MockDrawables>();
    failure_backend->gl=gl.get();
    failure_backend->identity=mock->identity;
    failure_backend->throw_after_remove=true;
    auto failure_scene=std::make_shared<graphics::GlBnwWorldAnchorScene>(
        state,gl,failure_backend);
    auto failure_binding=runtime::bind_bnw_world_anchor_graphics(
        state,failure_scene);
    check(failure_binding.ready(),"Failure backend setup was not ready");
    auto failure_draft=state;
    runtime::BnwWorldAnchorJournal failure_journal(
        std::move(failure_binding.emitter));
    check(runtime::stage_bnw_world_anchor_clear(
        failure_draft,635,failure_journal).outcome==
              runtime::LiveActionOutcome::committed,
          "Failure journal could not stage Gandhi anchor");
    check(failure_journal.publish(failure_draft).status==
              runtime::BnwWorldAnchorPublicationStatus::uncertain &&
          failure_scene->publication_uncertain() &&
          !failure_scene->supports_gameplay_world_anchor() &&
          failure_backend->removed==1 &&
          failure_journal.publish(failure_draft).status==
              runtime::BnwWorldAnchorPublicationStatus::uncertain &&
          failure_backend->removed==1,
          "Post-effect GL anchor failure was retried or declared complete");
    std::cout << "PASS Gandhi GL anchor scene: one retained source anchor; "
                 "no production drawable backend; mock removal once\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and game root");
        selected(argv[1],argv[2]);
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL Gandhi GL anchor scene: " << error.what() << '\n';
        return 1;
    }
}
