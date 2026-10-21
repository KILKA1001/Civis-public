#include "civ5/runtime/bnw_world_anchor_graphics_adapter.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
struct Scene final:BnwWorldAnchorGraphicsOwner {
    bool supported=true,current=true;
    BnwWorldAnchorGraphicsIdentity identity;
    std::vector<std::tuple<std::int32_t,bool,std::int32_t,std::int32_t,std::int32_t>> calls;
    bool supports_gameplay_world_anchor() const override {return supported;}
    BnwWorldAnchorGraphicsIdentity current_world() const override {return identity;}
    bool publication_context_current() const override {return current;}
    void gameplay_world_anchor(std::int32_t anchor,bool add,
        std::int32_t x,std::int32_t y,std::int32_t data) override {
        calls.emplace_back(anchor,add,x,y,data);
    }
};
void selected(const std::filesystem::path &save,const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"Selected BNW host unavailable");
    const auto &source=bootstrap.preview.host->session().state();
    const auto &map=source.world.map_header;
    auto scene=std::make_shared<Scene>();
    scene->identity={map.guid,map.grid.width,map.grid.height};
    check(!bind_bnw_world_anchor_graphics(source,{}).ready(),
          "Missing BNW graphics owner claimed GameplayWorldAnchor capability");
    scene->supported=false;
    check(!bind_bnw_world_anchor_graphics(source,scene).ready(),
          "LAND-only graphics owner claimed world-anchor capability");
    scene->supported=true;
    ++scene->identity.map_guid[0];
    check(!bind_bnw_world_anchor_graphics(source,scene).ready(),
          "Graphics owner from another map was accepted");
    --scene->identity.map_guid[0];
    scene->current=false;
    check(!bind_bnw_world_anchor_graphics(source,scene).ready(),
          "Inactive graphics publication context was accepted");
    scene->current=true;
    auto binding=bind_bnw_world_anchor_graphics(source,scene);
    check(binding.ready(),"Matching graphics world-anchor owner did not bind");
    auto invalid=*plan_bnw_world_anchor_clear(source,635).removal;
    invalid.data=8;
    bool rejected=false;
    try {binding.emitter(invalid);} catch(const std::runtime_error &) {rejected=true;}
    check(rejected && scene->calls.empty(),
          "Graphics adapter accepted saved old anchor data as event data");

    auto draft=source;
    BnwWorldAnchorJournal journal(std::move(binding.emitter));
    check(stage_bnw_world_anchor_clear(draft,635,journal).outcome==
              LiveActionOutcome::committed && scene->calls.empty(),
          "Graphics event escaped before game-turn commit");
    check(journal.publish(draft).status==BnwWorldAnchorPublicationStatus::complete &&
          scene->calls.size()==1 &&
          scene->calls[0]==std::tuple<std::int32_t,bool,std::int32_t,std::int32_t,std::int32_t>{
              0,false,35,15,-1},
          "Selected Gandhi graphics adapter did not forward exact SDK call");

    auto stale_draft=source;
    auto stale_binding=bind_bnw_world_anchor_graphics(source,scene);
    BnwWorldAnchorJournal stale(std::move(stale_binding.emitter));
    check(stage_bnw_world_anchor_clear(stale_draft,635,stale).outcome==
              LiveActionOutcome::committed,
          "Selected Gandhi stale-owner setup failed");
    ++scene->identity.map_guid[0];
    check(stale.publish(stale_draft).status==
              BnwWorldAnchorPublicationStatus::uncertain &&
          scene->calls.size()==1 &&
          stale.publish(stale_draft).status==
              BnwWorldAnchorPublicationStatus::uncertain &&
          scene->calls.size()==1,
          "Stale graphics owner received or replayed a world-anchor event");
    std::cout<<"selected BNW graphics GameplayWorldAnchor(0,false,35,15,-1) forwarded once\n";
}
} // namespace

int main(int argc,char **argv) {
    check(argc==3,"Supply selected Gandhi BNW save and game root");
    selected(argv[1],argv[2]);
}
