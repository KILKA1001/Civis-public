#include "civ5/ui/bnw_unit_panel_stat_rows.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/bnw_info_default_form.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace civ5;
namespace {
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
ui::TopPanelLocalizationCatalog locale(const std::filesystem::path &root,bool russian) {
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
    return ui::TopPanelLocalizationCatalog::read_active_bnw_resources(resources,
        russian ? "Language_RU_RU" : "Language_en_US");
}
std::int64_t &scalar(saves::UnitCoreObservation &core,std::string_view name) {
    for (auto &field : core.fields) if (field.name && std::string_view(field.name)==name && field.values.size()==1)
        return field.values.front();
    throw std::runtime_error("Current selected unit scalar is absent");
}
void run(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected actual save/game root/optional original SDK root");
    for (const auto &source : ui::kBnwUnitPanelStatRowSources)
        check(runtime::identify_file(std::filesystem::path(argv[2])/source.path).sha256==source.sha256,
            "Original UnitPanel row source pin differs");
    if (argc==4) for (const auto &source : runtime::kBnwUnitMovementSources)
        check(runtime::identify_file(std::filesystem::path(argv[3])/source.path).sha256==source.sha256,
            "Native movement/secondary getter pin differs");
    auto bootstrap=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(bootstrap.preview.host && !bootstrap.preview.blocker,"Actual selected Gandhi host unavailable");
    auto &host=*bootstrap.preview.host;
    const runtime::BnwUnitOrderKey requested{0,16385};
    auto selection=runtime::start_bnw_unit_presentation_client(host.session().state(),host.identity(),host.database());
    runtime::select_bnw_unit_presentation(host.session().state(),selection,host.database(),requested,selection.client_revision());
    check(selection.selected_unit()==requested,"Actual native selection owner differs");
    ui::BnwUnitPanelStatRowsBridge bridge(host.session(),host.database(),host.types(),host.identity(),0);
    const auto readiness=host.session().readiness().blockers;
    const auto *fighter=host.types().find("UNIT_FIGHTER");
    check(fighter && fighter->table=="Units","Actual selected fighter Info unavailable");
    const auto air_info=ui::read_bnw_unit_panel_secondary_info(host.database(),host.types(),fighter->hash,2);
    // Actual original UI/XML/color source, separate from the explicit layout
    // receipt fixture below. This test does not attest native XP visibility.
    const auto game_root=std::filesystem::path(argv[2]);
    auto base=std::make_shared<assets::LooseResourceIndex>(game_root);
    auto expansion=std::make_shared<assets::LooseResourceIndex>(game_root);
    auto bnw=std::make_shared<assets::LooseResourceIndex>(game_root);
    base->scan({game_root/"Assets/UI/InGame",{},false});
    base->add_file(game_root/"Assets/UI/ColorAtlas.xml");
    expansion->scan({game_root/"Assets/DLC/Expansion/UI/InGame",{},false});
    bnw->scan({game_root/"Assets/DLC/Expansion2/UI/InGame",{},false});
    assets::ResourceMountTable resources(assets::resource_lookup(base),{},assets::resource_keys(base));
    resources.mount({1,0,0,assets::resource_lookup(expansion),true,true,{},assets::resource_keys(expansion)});
    resources.mount({2,0,0,assets::resource_lookup(bnw),true,true,{},assets::resource_keys(bnw)});
    const auto registry=ui::UiContextRegistry::from_resources(resources);
    const auto skin=ui::WorldViewSkin::from_resources(resources,registry);
    check(skin.unit_stat_layout().has_value(),"Actual anonymous UnitPanel stat parent layout absent");
    const auto *movement_control=skin.find("UnitPanel","UnitStatMovement");
    check(movement_control && movement_control->font=="TwCenMT20" && movement_control->font_style=="Shadow" &&
        movement_control->font_colors[0]==0xffffc8ffu && movement_control->font_colors[1]==0x000000ffu,
        "Actual UnitPanel TextButton Context/ColorSet inheritance differs");
    for (bool russian : {false,true}) {
        auto catalog=locale(argv[2],russian);
        const std::string language=russian ? "ru_RU" : "en_US";
        const auto rows=bridge.capture(host.session(),requested.second,41,catalog,language);
        check(rows && bridge.matches(host.session(),*rows,requested.second,41,catalog,language),
            "Current source row read contract unavailable");
        check(rows->rows.movement.shown==true && rows->rows.movement.value=="2/2 [ICON_MOVES]" &&
            rows->rows.movement.label==catalog.format("TXT_KEY_UPANEL_MOVEMENT") &&
            rows->rows.strength.shown==true && rows->rows.strength.value=="8 [ICON_STRENGTH]" &&
            rows->rows.strength.label==catalog.format("TXT_KEY_UPANEL_STRENGTH") &&
            rows->rows.strength.tooltip_text==catalog.format("TXT_KEY_UPANEL_STRENGTH_TT") &&
            rows->rows.strength.tooltip_blocker.empty(),"Actual positive native combat row/locale differs");
        check(!rows->rows.ranged.shown && !rows->rows.ranged.value && !rows->rows.ranged.visibility_blocker.empty(),
            "Known zero base-ranged improperly proved absent fallback/cargo branches");
        check(!rows->rows.movement.tooltip_text && rows->rows.movement.tooltip_key=="TXT_KEY_UPANEL_UNIT_MAY_MOVE" &&
            rows->rows.movement.tooltip_arguments.size()==1 &&
            rows->rows.movement.tooltip_arguments[0].numerator==120 &&
            rows->rows.movement.tooltip_arguments[0].denominator==60 &&
            !rows->rows.movement.tooltip_blocker.empty(),"Native movement fraction or locale-composition blocker lost");
        if (russian) check(rows->rows.movement.tooltip_blocker.find("plural")!=std::string::npos &&
            catalog.rows.at("TXT_KEY_UPANEL_UNIT_MAY_MOVE").find(": plural")!=std::string::npos,
            "Actual Russian plural expression was formatted by a placeholder-only helper");
        // Detached original Info fixture; no live AIR unit/frame/render is claimed.
        const auto air=ui::read_detached_bnw_air_stat_rows(air_info,catalog,language);
        check(air.movement.shown==true && air.movement.value==std::to_string(*air_info.range)+" [ICON_MOVES]" &&
            air.movement.label==catalog.format("TXT_KEY_UPANEL_RANGEMOVEMENT") &&
            air.strength.shown==true && air.strength.value==std::to_string(*air_info.base_ranged_combat)+" [ICON_STRENGTH]" &&
            air.strength.tooltip_text==catalog.format("TXT_KEY_UPANEL_STRENGTH_TT") &&
            !air.ranged.shown && !air.ranged.value && !air.ranged.visibility_blocker.empty() &&
            air.movement.tooltip_arguments.size()==2 && air.movement.tooltip_arguments[0].numerator==*air_info.range &&
            air.movement.tooltip_arguments[1].numerator==std::int64_t(*air_info.range)*(*air_info.rebase_range_multiplier) &&
            air.movement.tooltip_arguments[1].denominator==100 && !air.movement.tooltip_text,
            "Detached AIR native branch projection duplicated ranged row or lost rebase operand");
        if (russian) check(air.movement.tooltip_blocker.find("plural")!=std::string::npos,
            "AIR Russian plural composition dependency erased");
        ui::LiveSessionUiSources sources;
        sources.database=&host.database();sources.unit_stats_database=&host.database();
        sources.unit_stats_types=&host.types();sources.unit_stats_identity=&host.identity();
        sources.selected_unit=[&] { return selection.selected_unit() ?
            std::optional<std::int64_t>(selection.selected_unit()->second) : std::nullopt; };
        sources.localize_info=[&](std::string_view key) { return ui::bnw_info_default_form(catalog,key); };
        sources.selected_unit_stat_rows=[&](const runtime::LiveSession &session,std::uint64_t frame,std::int64_t id) {
            return bridge.capture(session,id,frame,catalog,language);
        };
        sources.selected_unit_stat_rows_current=[&](const runtime::LiveSession &session,
            const ui::BnwUnitPanelReadRows &feed,std::int64_t id,std::uint64_t frame) {
            return bridge.matches(session,feed,id,frame,catalog,language);
        };
        sources.selected_unit_stat_boxes=[&](const runtime::LiveSession &session,std::uint64_t frame) {
            // Explicit layout-only fixture: these booleans are not native XP or
            // gameplay attestation. Production uses its validated G0 receipt.
            return std::optional(ui::LiveWorldViewUnitStatBoxesFeed{0,requested.second,
                static_cast<std::int32_t>(session.state().setup.game_turn),frame,{true,true,true,false}});
        };
        const auto captured=ui::capture_live_session_ui_frame(host.session(),41,0,sources);
        const auto *world=captured.find<ui::WorldViewState>();
        check(world && world->selected_unit && world->selected_unit->stat_rows &&
            captured.find<ui::BnwUnitPanelReadRows>() && ui::ui_owners_current(captured),
            "Actual native stat rows not adopted/sealed by current frame consumer");
        for (const auto height : {925,960}) {
            const auto plan=ui::plan_world_view(skin,registry,captured,1274,height);
            const auto find_text=[&](std::string_view value,std::int32_t x,std::int32_t y) {
                return std::find_if(plan.paints.begin(),plan.paints.end(),[&](const auto &paint) {
                    return paint.kind==ui::WorldViewPaintKind::text && paint.text==value &&
                        paint.destination.x==x && paint.destination.y==y && paint.destination.width==0 &&
                        paint.destination.height==27 && paint.style=="TwCenMT20:right-center" &&
                        paint.font_style && paint.font_style->style=="Shadow" &&
                        paint.font_style->colors[0]==0xffffc8ffu && paint.font_style->colors[1]==0x000000ffu;
                })!=plan.paints.end();
            };
            check(find_text(*rows->rows.movement.label,402,height-88) &&
                find_text(*rows->rows.movement.value,482,height-88) &&
                find_text(*rows->rows.strength.label,402,height-61) &&
                find_text(*rows->rows.strength.value,482,height-61),
                "Actual XML stat text right/center origin or inherited paint style differs");
            check(plan.readonly_tooltips.size()==2 && std::all_of(plan.readonly_tooltips.begin(),plan.readonly_tooltips.end(),
                [&](const auto &region) { return region.context_id=="UnitPanel" && region.font_blocker.empty() &&
                    region.tooltip==*rows->rows.strength.tooltip_text && region.text.destination.width==0 &&
                    region.source_paint_index && *region.source_paint_index<plan.paints.size() &&
                    plan.paints[*region.source_paint_index].text==region.text.text &&
                    (region.control_id=="UnitStatStrength" || region.control_id=="UnitStatNameStrength"); }),
                "Static combat readonly tooltip request gained guessed hit bounds or dynamic text");
            check(std::none_of(plan.hits.begin(),plan.hits.end(),[](const auto &hit) {
                const auto *command=std::get_if<ui::UiGameplayCommand>(&hit.command);
                return command && command->source_id()=="UnitPanel";
            }),"Stat readonly tooltip gained a gameplay click action");
        }
        ui::LiveUiFrame live{.snapshot=captured,.sequence=7,.owner=&selection,
            .world_view=ui::plan_world_view(skin,registry,captured,1274,925),
            .top_panel_bars={},.top_panel={},.minimap={},.progression={},
            .policy_scroll={},.tech_popup_scroll={},.ideology_view={}};
        ui::UiScreenController screens(registry);
        ui::UiScreenCompositor compositor(resources,registry);
        const auto composed=compositor.compose(live,screens,1274,925);
        check(composed.frame_owner==live.owner && composed.frame_sequence==live.sequence &&
            composed.snapshot_frame==captured.frame() && composed.readonly_tooltips.size()==2 &&
            composed.readonly_tooltips[0].source_paint_index!=composed.readonly_tooltips[1].source_paint_index,
            "Composed readonly requests lost exact paint/frame identity");
        auto malformed=live;
        malformed.world_view->readonly_tooltips[0].source_paint_index=malformed.world_view->paints.size();
        malformed.world_view->readonly_tooltips[1].source_paint_index.reset();
        check(compositor.compose(malformed,screens,1274,925).readonly_tooltips.empty(),
            "Invalid/absent sourcepaint index gained readonly hover request");
        screens.dispatch(ui::UiOpenScreen{"GameMenu"},captured);
        check(screens.focused_screen()==std::optional<std::string>{"GameMenu"} &&
            compositor.compose(live,screens,1274,925).readonly_tooltips.empty(),
            "World readonly hover leaked through current focused screen");
        screens.dispatch(ui::UiCloseScreen{"GameMenu"},captured);
        auto bad=sources;
        bad.selected_unit_stat_rows=[&](const runtime::LiveSession &,std::uint64_t,std::int64_t) {
            auto stale=*rows;--stale.frame_number;return std::optional(stale);
        };
        const auto stale=ui::capture_live_session_ui_frame(host.session(),41,0,bad);
        check(!stale.find<ui::WorldViewState>()->selected_unit->stat_rows && !stale.find<ui::BnwUnitPanelReadRows>(),
            "Wrong native row stamp accepted by frame consumer");
        bad=sources;bad.selected_unit_stat_rows_current=[](const auto &,const auto &,auto,auto) { return false; };
        check(!ui::capture_live_session_ui_frame(host.session(),41,0,bad).find<ui::BnwUnitPanelReadRows>(),
            "Unattested current native row source accepted");
        bad=sources;bad.selected_unit_stat_boxes={};
        const auto missing_layout=ui::capture_live_session_ui_frame(host.session(),41,0,bad);
        const auto missing_plan=ui::plan_world_view(skin,registry,missing_layout,1274,925);
        check(missing_layout.find<ui::BnwUnitPanelReadRows>() && missing_plan.readonly_tooltips.empty() &&
            std::none_of(missing_plan.paints.begin(),missing_plan.paints.end(),[](const auto &paint) {
                return paint.style.ends_with(":right-center"); }),
            "Unknown XP visibility produced guessed native row positions");
        const auto clear_selection=[&] {
            // Native client startup/reset genuinely owns an empty selection;
            // no missing serialized original selection is inferred.
            selection=runtime::start_bnw_unit_presentation_client(host.session().state(),host.identity(),host.database());
        };
        const auto select_requested=[&] {
            runtime::select_bnw_unit_presentation(host.session().state(),selection,host.database(),
                requested,selection.client_revision());
        };
        clear_selection();
        check(!ui::ui_owners_current(captured) && ui::plan_world_view(skin,registry,captured,1274,925).paints.empty(),
            "Native empty selection accepted retained selected-unit row frame");
        const auto empty_selection=ui::capture_live_session_ui_frame(host.session(),41,0,sources);
        check(!empty_selection.find<ui::WorldViewState>()->selected_unit && ui::ui_owners_current(empty_selection),
            "Actual native empty selection was fabricated as a selected unit");
        select_requested();
        check(!ui::ui_owners_current(empty_selection),"No-selection publication lacked actual selector fence");
        bad=sources;
        bad.selected_unit_stat_rows_current=[&](const runtime::LiveSession &session,
            const ui::BnwUnitPanelReadRows &feed,std::int64_t id,std::uint64_t frame) {
            const bool current=bridge.matches(session,feed,id,frame,catalog,language);
            clear_selection();
            return current;
        };
        const auto switched=ui::capture_live_session_ui_frame(host.session(),41,0,bad);
        check(!switched.find<ui::BnwUnitPanelReadRows>() && !ui::ui_owners_current(switched),
            "Row current callback changed native selection but published current rows");
        select_requested();
        catalog.rows.at("TXT_KEY_UPANEL_STRENGTH_TT")+=" selected-locale-fault";
        check(!bridge.matches(host.session(),*rows,requested.second,41,catalog,language),
            "Changed selected locale tooltip accepted old row read" );
        check(!ui::ui_owners_current(captured) && ui::plan_world_view(skin,registry,captured,1274,925).paints.empty(),
            "Stale native row locale owner allowed WorldView composition" );
        check(compositor.compose(live,screens,1274,925).readonly_tooltips.empty(),
            "Stale current row owner kept cached composed readonly request");
        std::cout << (russian ? "RU" : "EN") << " actual land rows/static tooltip PASS; DETACHED_ACTUAL_DB_AIR rows/args PASS; no_render_claim=1\n";
    }
    const auto catalog=locale(argv[2],false);
    const auto original=bridge.capture(host.session(),requested.second,42,catalog,"en_US");
    check(host.session().readiness().blockers==readiness && !host.session().readiness().can_continue(),
        "Read-only stat rows changed runtime readiness" );
    auto &state=host.session().mutable_state();
    saves::UnitCoreObservation *core=nullptr;
    for (auto &owner : state.world.all_players->players) if (owner.saved_player_id==requested.first)
        for (auto &unit : owner.units.units) if (scalar(unit.core,"m_iID")==requested.second) core=&unit.core;
    check(core && original,"Actual selected mutation probe owner absent");
    const auto moves=scalar(*core,"m_iMoves");
    scalar(*core,"m_iMoves")=30; // Labeled actual current operand probe, not an executed movement action.
    const auto half=bridge.capture(host.session(),requested.second,42,catalog,"en_US");
    check(half && half->rows.movement.value=="0/2 [ICON_MOVES]" &&
        half->rows.movement.tooltip_arguments[0].numerator==30 && half->rows.movement.tooltip_arguments[0].denominator==60 &&
        !bridge.matches(host.session(),*original,requested.second,42,catalog,"en_US"),
        "Movement caption flooring erased actual fractional tooltip operand/current fence");
    scalar(*core,"m_iMoves")=moves;
    check(!host.session().readiness().can_continue(),
        "Row read granted full turn readiness");
    const auto checkpoint=host.session().checkpoint();host.session().restore(checkpoint);host.session().restore(checkpoint);
    check(!bridge.capture(host.session(),requested.second,43,catalog,"en_US") &&
        !bridge.matches(host.session(),*original,requested.second,42,catalog,"en_US"),"Replaced native source row reader accepted");
    ui::BnwUnitPanelStatRowsBridge rebound(host.session(),host.database(),host.types(),host.identity(),0);
    check(rebound.capture(host.session(),requested.second,43,catalog,"en_US").has_value() &&
        !host.session().readiness().can_continue(),"Explicit row reader rebind failed or full turn enabled");
    std::cout << "PASS actual source stat-row read contract EN/RU; native/cache/locale lifecycle guarded; no UI publication\n";
}
}
int main(int argc,char **argv) {
    try { run(argc,argv); } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
