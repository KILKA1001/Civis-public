#include "civ5/ui/bnw_unit_panel_stat_rows_feed.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include "civ5/ui/bnw_info_default_form.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5;
namespace {
void check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
std::shared_ptr<const ui::TopPanelLocalizationCatalog> catalog(const std::filesystem::path &root,bool russian) {
    auto base=std::make_shared<assets::LooseResourceIndex>(root);
    auto expansion=std::make_shared<assets::LooseResourceIndex>(root);
    auto bnw=std::make_shared<assets::LooseResourceIndex>(root);
    const auto language=russian ? "RU_RU" : "en_US";
    base->scan({root/"Assets/Gameplay/XML/NewText"/(russian ? "RU_RU" : "EN_US"),{},false});
    expansion->scan({root/"Assets/DLC/Expansion/Gameplay/XML/Text"/language,{},false});
    bnw->scan({root/"Assets/DLC/Expansion2/Gameplay/XML/Text"/language,{},false});
    assets::ResourceMountTable resources(assets::resource_lookup(base),{},assets::resource_keys(base));
    resources.mount({1,0,0,assets::resource_lookup(expansion),true,true,{},assets::resource_keys(expansion)});
    resources.mount({2,0,0,assets::resource_lookup(bnw),true,true,{},assets::resource_keys(bnw)});
    return std::make_shared<const ui::TopPanelLocalizationCatalog>(ui::TopPanelLocalizationCatalog::read_active_bnw_resources(
        resources,russian ? "Language_RU_RU" : "Language_en_US"));
}
void run(int argc,char **argv) {
    check(argc==3,"Expected actual selected save and original game root");
    auto bootstrap=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(bootstrap.preview.host && !bootstrap.preview.blocker,"Actual selected host unavailable");
    auto &host=*bootstrap.preview.host;
    const auto readiness=host.session().readiness().blockers;
    const runtime::BnwUnitOrderKey selected{0,16385};
    for (const bool russian : {false,true}) {
        auto input=host.preview_input();
        auto sources=input.frame_sources;
        auto selection=runtime::start_bnw_unit_presentation_client(host.session().state(),host.identity(),host.database());
        runtime::select_bnw_unit_presentation(host.session().state(),selection,host.database(),selected,selection.client_revision());
        sources.selected_unit=[&] { return selection.selected_unit() ?
            std::optional<std::int64_t>(selection.selected_unit()->second) : std::nullopt; };
        const auto language=russian ? "ru_RU" : "en_US";
        auto localized=catalog(argv[2],russian);
        // Production camera supplies this selected catalog owner before the
        // row binder; a missing localized unit name must stay unselected.
        sources.localize_info=[retained=localized](std::string_view key) {
            return ui::bnw_info_default_form(*retained,key);
        };
        ui::install_bnw_unit_panel_stat_rows_feed(sources,host.session(),0,localized,language);
        localized.reset(); // Binder must retain the actual catalog.
        const auto read=sources.selected_unit_stat_rows(host.session(),1,selected.second);
        check(read && read->locale==language && read->rows.movement.value=="2/2 [ICON_MOVES]" &&
            read->rows.strength.value=="8 [ICON_STRENGTH]" &&
            sources.selected_unit_stat_rows_current(host.session(),*read,selected.second,1),
            "Installed actual native rows not current");
        auto wrong=*read;++wrong.frame_number;
        check(!sources.selected_unit_stat_rows_current(host.session(),wrong,selected.second,1),"Wrong frame stamp accepted");
        check(!sources.selected_unit_stat_rows_current(host.session(),*read,selected.second+1,1),"Wrong selected unit accepted");
        const auto captured=ui::capture_live_session_ui_frame(host.session(),1,0,sources);
        if (!captured.find<ui::BnwUnitPanelReadRows>() || !ui::ui_owners_current(captured)) {
            const auto *world=captured.find<ui::WorldViewState>();
            std::cerr<<"consumer_rows="<<bool(captured.find<ui::BnwUnitPanelReadRows>())
                <<" selection="<<bool(world && world->selected_unit)<<" generation="<<host.session().state_generation()<<'\n';
            if (const auto *fences=captured.find<ui::UiOwnerFences>())
                for (std::size_t i=0;i<fences->checks.size();++i)
                    std::cerr<<"fence="<<i<<" current="<<fences->checks[i]()<<'\n';
        }
        check(captured.find<ui::BnwUnitPanelReadRows>(),"Installed rows packet not accepted by real consumer");
        check(ui::ui_owners_current(captured),"Installed rows frame has stale owner fence");
        const auto checkpoint=host.session().checkpoint();
        const auto generation=host.session().state_generation();
        host.session().restore(checkpoint);
        check(host.session().state_generation()!=generation &&
            !sources.selected_unit_stat_rows_current(host.session(),*read,selected.second,1),
            "Current validator rebound a replaced native source");
        check(!ui::ui_owners_current(captured),"Replacement accepted old retained consumer frame");
        // Legitimate new lifecycle can restart at frame1; this does not grant
        // old retained frames a new generation seal even if rows are identical.
        const auto rebound=sources.selected_unit_stat_rows(host.session(),1,selected.second);
        check(rebound && sources.selected_unit_stat_rows_current(host.session(),*rebound,selected.second,1) &&
            !ui::ui_owners_current(captured),"Capture rebind failed or repaired old consumer generation");
        const auto newer=sources.selected_unit_stat_rows(host.session(),2,selected.second);
        check(newer && !sources.selected_unit_stat_rows_current(host.session(),*read,selected.second,1),
            "Old capture accepted after latest source publication");
        check(!sources.selected_unit_stat_rows(host.session(),3,-1),"Absent selection fabricated native rows");
        std::cout<<(russian ? "RU" : "EN")<<" actual installed rows/current/capture generation PASS; no_render_claim=1\n";
    }
    ui::LiveSessionUiSources absent;
    bool rejected=false;
    try {ui::install_bnw_unit_panel_stat_rows_feed(absent,host.session(),0,catalog(argv[2],false),"en_US");}
    catch(const std::invalid_argument &) {rejected=true;}
    check(rejected && !absent.selected_unit_stat_rows,"Missing actual native source owner accepted");
    check(host.session().readiness().blockers==readiness && !host.session().readiness().can_continue(),
        "Rows binder changed fullturn readiness");
}
}
int main(int argc,char **argv) {try {run(argc,argv);return 0;} catch(const std::exception &error) {
    std::cerr<<error.what()<<'\n';return 1;
}}