#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/bnw_civilopedia_native_runtime_owner.hpp"
#include <iostream>
#include <functional>
#include <type_traits>
#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "../src/ui/bnw_native_completed_color_query.hpp"
#include "civ5/graphics/gl_context.hpp"
#include "civ5/ui/win32_viewport_transaction.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include <thread>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define NOMINMAX
#include <windows.h>

using namespace civ5;
using namespace civ5::runtime;
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F> void rejects(F&& f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}need(rejected,"Invalid persistent stock owner use accepted");}
struct Window {
    HWND handle=CreateWindowExW(0,L"STATIC",L"Civis stock owner CPU test",WS_OVERLAPPEDWINDOW,
        0,0,840,620,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    std::uint64_t revision=9;
    Window(){need(handle&&!IsWindowVisible(handle),"Actual hidden owned test window unavailable");}
    ~Window(){if(handle)DestroyWindow(handle);}
    BnwStockLuaClientExtentProvider provider(){return bnw_stock_lua_win32_client_extent(
        reinterpret_cast<std::uintptr_t>(handle),"owned-stock-join-client",[this]{return revision;});}
};
std::shared_ptr<BnwSelectedStockLuaSourceOwner> owner(const std::filesystem::path& root,
    const content::Catalog& catalog,const SavedContentProfile& profile,
    const SessionCheckpointIdentity& identity){
    auto o=std::make_shared<BnwSelectedStockLuaSourceOwner>(root,catalog,profile,identity);
    need(o->evidence().selected_profile,"Actual selected source preparation failed");return o;
}
void journal(const BnwStockLuaOperationEvidence& e){
    for(std::size_t i=0;i<e.operations.size();++i)
        need(e.operations[i].serial==i+1,"Operation journal serial reused/skipped");
}
void uncertain(const BnwStockLuaOperationEvidence& e){
    journal(e);
    need(e.state==BnwStockLuaOperationState::uncertain&&!e.operations.empty()&&
         e.operations.back().outcome==BnwStockLuaOperationOutcome::uncertain&&
         e.operations.back().vm_retained&&e.operations.back().host_retained,
         "Failed genuine operation discarded VM or reported a successful return");
}
void first(const BnwStockLuaBootstrapProgress& p){
    need(p.selected_sources_verified&&p.first_prefix_executed&&p.prefix_executions==1&&
         p.manager_bindings==33&&p.invalidation_listeners==2&&p.pending_api=="UIManager.GetScreenSizeVal"&&
         !p.loading_complete&&!p.gameplay_callbacks_ready,"Owned genuine prefix did not suspend at screen getter");
}
void retained(const std::filesystem::path& root,const std::filesystem::path& assets,
    const content::Catalog& catalog,const SavedContentProfile& profile,
    const std::shared_ptr<ui::SelectedBnwPreviewHost>& host,Window& window){
    static_assert(!std::is_constructible_v<BnwSelectedStockLuaBootstrap,
        std::filesystem::path,const content::Catalog&,const SavedContentProfile&,
        const SessionCheckpointIdentity&,std::shared_ptr<const LiveSessionState>,
        const content::RulesDatabase&,ScriptMapProvider>);
    auto lease=host->session().read_state_lease();
    auto types=host->database().cache_info_types();
    auto provider=lua_gameplay_read_services(host->database(),lease,types);
    need(provider.game_turn()==lease->setup.game_turn&&provider.game_speed()==lease->setup.speed,
        "Stock read provider did not consume SAME current setup");
    const auto actual_ids=provider.ids(false);
    need(actual_ids.size()==lease->setup.players.size(),"Stock read roster differs");
    for(const auto id:actual_ids){
        need(provider.player_team(id)==lease->setup.players.at(id).team&&
             provider.player_alive(id)==lease->setup.players.at(id).alive,
             "Stock read provider copied a different roster");
    }
    need(provider.player_human(0,0)==detail::bnw_player_human_read(*lease,0,host->database()),
        "Stock read IsHuman differs from existing native string read");
    std::weak_ptr<const LiveSessionState> retained_lease=lease;lease.reset();
    need(!retained_lease.expired()&&provider.game_turn()==host->session().state().setup.game_turn,
        "Stock read closures failed to retain actual state");
    // Detached read-math retention only; never passed to a production issuer.
    std::weak_ptr<const LiveSessionState> detached_weak;
    std::function<std::int64_t()> only_turn;
    {
        auto detached=std::make_shared<LiveSessionState>(host->session().state());
        auto numeric=lua_gameplay_read_services(host->database(),detached,types);
        detached_weak=detached;only_turn=std::move(numeric.game_turn);
    }
    need(!detached_weak.expired()&&only_turn()==host->session().state().setup.game_turn,
        "One extracted read closure did not retain its own world allocation");
    only_turn={};need(detached_weak.expired(),"Numeric read closure leaked world allocation");
    rejects([&]{(void)lua_gameplay_read_services(host->database(),{},types);});
    static_assert(!std::is_default_constructible_v<BnwStockLuaAttemptLease>);
    static_assert(!std::is_aggregate_v<BnwStockLuaAttemptLease>);
    static_assert(!std::is_constructible_v<BnwSelectedStockLuaBootstrap,
        std::filesystem::path,const content::Catalog&,const SavedContentProfile&,
        const SessionCheckpointIdentity&,std::shared_ptr<const LiveSessionState>,
        const content::RulesDatabase&,ScriptMapProvider,BnwStockLuaAttemptLease>);
    auto o=owner(root,catalog,profile,host->identity());
    need(!o->stock_bootstrap_progress(),"Source evidence manufactured a persistent VM");
    rejects([&]{(void)o->advance_stock_bootstrap();});
    rejects([&]{(void)o->attach_stock_client_extent(window.provider());});
    first(o->start_stock_bootstrap(host,assets));
    static_assert(!std::is_default_constructible_v<ui::detail::BnwNativeCompletedColorQuery>);
    static_assert(!std::is_copy_constructible_v<ui::detail::BnwNativeCompletedColorQuery>);
    // This actual CPU host has no adopted Early Parent and cannot issue a
    // genuine Color leaf. Absence refusal is negative coverage ONLY.
    const auto before_color_absence=o->stock_operation_evidence();
    rejects([&]{o->attach_stock_completed_color_query({});});
    const auto after_color_absence=o->stock_operation_evidence();
    need(after_color_absence.operations.size()==before_color_absence.operations.size()&&
         after_color_absence.state==before_color_absence.state,
         "Absent actual Color Parent attempted source callbacks or poisoned unrelated CPU VM");
    auto initial=o->stock_operation_evidence();journal(initial);
    need(initial.state==BnwStockLuaOperationState::returned_suspended&&initial.operations.size()==1&&
         initial.operations[0].kind==BnwStockLuaOperationKind::prepare&&
         initial.operations[0].outcome==BnwStockLuaOperationOutcome::incomplete_suspended&&
         initial.operations[0].vm_retained&&initial.operations[0].host_retained&&
         initial.operations[0].suspended_before=="UIManager.GetScreenSizeVal",
         "First planned yield was treated as completed gameplay or an empty VM");
    const auto* original=o->stock_bootstrap_progress();
    need(original,"Persistent VM owner missing");
    first(o->advance_stock_bootstrap());
    auto waiting=o->stock_operation_evidence();journal(waiting);
    need(waiting.state==BnwStockLuaOperationState::returned_suspended&&waiting.operations.size()==2&&
         waiting.operations.back().kind==BnwStockLuaOperationKind::advance&&
         waiting.operations.back().outcome==BnwStockLuaOperationOutcome::incomplete_suspended,
         "Planned dependency wait did not return a distinct incomplete operation");
    rejects([&]{(void)o->start_stock_bootstrap(host,assets);});
    need(o->stock_bootstrap_progress()==original,"Prefix VM recreated by rejected repeated start");
    need(!o->attach_stock_client_extent({}),"Missing client attached");
    need(o->attach_stock_client_extent(window.provider()),"Real current-process client not attached");
    auto screen=o->advance_stock_bootstrap();
    need(screen.native_screen_queries==1&&screen.pending_api=="Controls.ScrollBar.GetSize",
         "Owned actual coroutine did not execute its stock screen query");
    need(!o->attach_stock_client_extent(window.provider()),"Bound source locals rebound");
    const auto bar=o->advance_stock_bootstrap();
    need(bar.native_scrollbar_queries==1&&bar.pending_api=="Controls.ScrollPanel.GetSize",
         "Owned actual coroutine did not advance to panel getter");
    const auto panel=o->advance_stock_bootstrap();
    need(panel.native_panel_queries==1&&panel.pending_api=="Controls.ScrollBar.SetSize"&&
         !panel.native_scrollbar_resizes&&!panel.gameplay_callbacks_ready,"Unsupported UI setter bypassed");
    for(int i=0;i<2;++i){const auto held=o->advance_stock_bootstrap();
        need(held.prefix_executions==1&&held.native_screen_queries==1&&held.native_scrollbar_queries==1&&
             held.native_panel_queries==1&&held.pending_api=="Controls.ScrollBar.SetSize"&&
             !held.native_scrollbar_resizes&&!held.loading_complete&&!held.gameplay_callbacks_ready,
             "Retained coroutine replayed source or manufactured an epoch");}
    auto draft=host->session().state();
    need(!o->fork_at(draft).epoch,"Partial stock bootstrap opened gameplay fork");
    need(!o->publish({},draft).committed,"Partial stock bootstrap published gameplay world");
    o->retire_stock_bootstrap();rejects([&]{(void)o->advance_stock_bootstrap();});
    rejects([&]{(void)o->stock_bootstrap_progress();});
    rejects([&]{(void)o->start_stock_bootstrap(host,assets);});
}
// The installed stock prefix has already written globals, listener closures
// and real coroutine locals. Fail INSIDE its next actual Lua native getter,
// after successful native preflight, rather than just before coroutine.resume.
void failure_after_stock_writes(const std::filesystem::path& root,const std::filesystem::path& assets,
    const content::Catalog& catalog,const SavedContentProfile& profile,
    const std::shared_ptr<ui::SelectedBnwPreviewHost>& host,Window& window){
    auto o=owner(root,catalog,profile,host->identity());
    const auto prefix=o->start_stock_bootstrap(host,assets);first(prefix);
    need(prefix.includes_executed==std::vector<std::string>{"Assets/UI/InstanceManager.lua",
         "Assets/UI/IconSupport.lua","Assets/Gameplay/Lua/FLuaVector.lua"},
         "Actual prefix writes were not from the selected include closure");
    auto native=window.provider();std::size_t calls=0,fail_on=0;
    need(o->attach_stock_client_extent({[&,native]{
        ++calls;const auto extent=native.current();
        need(extent.has_value(),"Actual native extent unavailable before stock-resume failure");
        if(fail_on&&calls==fail_on)
            throw std::runtime_error("Actual GetClientRect completed; fail inside stock Lua panel GetSize");
        return extent;
    }}),"Actual late-resume provider not attached");
    const auto screen=o->advance_stock_bootstrap();
    need(screen.native_screen_queries==1&&screen.pending_api=="Controls.ScrollBar.GetSize"&&calls==1,
         "Real screenSizeY local was not returned by stock coroutine");
    const auto bar=o->advance_stock_bootstrap();
    need(bar.native_scrollbar_queries==1&&bar.pending_api=="Controls.ScrollPanel.GetSize"&&calls==3&&
         bar.prefix_executions==1&&bar.invalidation_listeners==2,
         "Real scrollBarWidth local/selected globals/listeners were not retained before failure");
    const auto before=o->stock_operation_evidence();journal(before);
    need(before.state==BnwStockLuaOperationState::returned_suspended&&!before.operations.empty()&&
         before.operations.back().same_vm_identity&&!before.operations.back().host_failed&&
         before.operations.back().retained_lua_bytes>0,
         "Before-failure record did not refer to the actual changed VM");
    fail_on=calls+2; // one real native preflight, then real Lua GetSize callback
    rejects([&]{(void)o->advance_stock_bootstrap();});
    need(calls==fail_on,"Failure occurred outside the intended Lua resume callback");
    const auto failed=o->stock_operation_evidence();uncertain(failed);
    const auto& recorded=failed.operations.back();
    need(failed.operations.size()==before.operations.size()+1&&
         recorded.serial==before.operations.back().serial+1&&
         recorded.kind==BnwStockLuaOperationKind::advance&&recorded.same_vm_identity&&
         recorded.host_failed&&recorded.retained_lua_bytes>0&&
         recorded.host_failure_epoch>=before.operations.back().host_failure_epoch&&
         recorded.suspended_before=="Controls.ScrollPanel.GetSize"&&
         failed.error.find("fail inside stock Lua panel GetSize")!=std::string::npos,
         "Actual error-after-stock-writes discarded VM/stamp or swallowed its original reason");
    // No mutable progress/VM exposure is needed to inspect retained metadata.
    rejects([&]{(void)o->stock_bootstrap_progress();});
    rejects([&]{(void)o->advance_stock_bootstrap();});
    rejects([&]{(void)o->start_stock_bootstrap(host,assets);});
    rejects([&]{(void)o->attach_stock_client_extent(window.provider());});
    const auto repeated=o->stock_operation_evidence();uncertain(repeated);
    need(calls==fail_on&&repeated.operations.size()==failed.operations.size()&&
         repeated.operations.back().serial==recorded.serial&&
         repeated.operations.back().same_vm_identity&&repeated.operations.back().host_failed&&
         repeated.operations.back().retained_lua_bytes==recorded.retained_lua_bytes&&
         repeated.error==failed.error,"Forbidden repeat reexecuted source, replaced VM or erased uncertainty");
    auto draft=host->session().state();
    need(!o->fork_at(draft).epoch&&!o->publish({},draft).committed,
         "Retained uncertain heap was treated as a gameplay epoch");
}
// Same existing explicit camera resource graph used by the actual owned-camera
// fixture. This is the compositor's source table, not native UISkin completion.
assets::ResourceMountTable renderer_gui_resources(const std::filesystem::path& root){
    const auto loose=std::make_shared<assets::LooseSource>(root);
    assets::ResourceMountTable table(assets::resource_lookup(loose),
        assets::resource_lookup_all(loose),assets::resource_keys(loose));
    const assets::AssetEnvironment environment(root);std::uint64_t id=UINT64_MAX;
    for(const auto* name:{"Resource/DX9/UITextures.fpk","Resource/DX9/MiscTextures.fpk",
            "Resource/DX9/Expansion1UITextures.fpk","Resource/DX9/Expansion2UITextures.fpk"}){
        const auto opened=environment.open_fpk_archive(name);
        const auto* source=std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);
        need(source&&*source,"Actual renderer GUI archive unavailable");
        table.mount({id--,0,0,assets::resource_lookup(*source),true,true,
            assets::resource_lookup_all(*source),assets::resource_keys(*source),*source});
    }
    const auto skin=std::make_shared<assets::LooseResourceIndex>(root);
    skin->scan({root/"Assets/DLC/Expansion2/UI/InGame",{},false});
    need(skin->indexed_files()!=0,"Actual renderer BNW UI source unavailable");
    table.mount({id,0,0,assets::resource_lookup(skin),true,true,
        assets::resource_lookup_all(skin),assets::resource_keys(skin),skin});
    return table;
}
// Opt-in real renderer fixture. The original three-argument CPU scenario
// remains unchanged; this mode requires actual WGL and the Window1+2+3 producer.
// Initial physical Window is issued BEFORE Early/SelectedHost construction.
void renderer_binding_regression(const char* save,const std::filesystem::path& root,
    const std::filesystem::path& assets,std::string_view mode){
    need(mode=="renderer_good"||mode=="renderer_alias"||mode=="renderer_drop"||mode=="renderer_resize"||
         mode=="renderer_foreign_release",
         "Unknown actual renderer regression mode");
    auto context=std::make_shared<graphics::GlContext>(840,620,false);
    const auto initial=context->native_identity();
    need(initial.window&&initial.dc&&initial.context&&initial.owner_thread,"Actual renderer native identity absent");
    ui::ClientWindowInput input;
    auto viewport=std::make_unique<ui::Win32ViewportTransaction>(reinterpret_cast<HWND>(initial.window),input,
        [](const ui::Win32ViewportTransaction::Retirement&){},[](const ui::ClientWindowEvent&){});
    auto renderer=viewport->capture_renderer_window_owner(context);
    need(renderer!=nullptr,"Actual producer did not issue physical Window");
    std::uint64_t revision=0;std::uintptr_t occurrence=0;
    renderer->with_actual_window_loan([&](const graphics::BnwOwnedCameraRendererWindowLease::Loan& loan){
        revision=loan.viewport_revision();occurrence=loan.window_occurrence();
        need(loan.window()==initial.window&&loan.dc()==initial.dc&&loan.wgl_context()==initial.context,
             "Genuine initial physical loan changed actual renderer handles");
    });
    auto selected=ui::bootstrap_selected_bnw_preview(save,root,0,true);
    need(static_cast<bool>(selected.preview),"Actual renderer selected Early bootstrap failed");
    auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(selected.preview.host));
    const auto catalog=content::discover_packages({root/"Assets/DLC"});
    need(catalog.diagnostics.empty(),"Actual renderer catalog unavailable");
    auto o=owner(root,catalog,selected.profile,host->identity());
    first(o->start_stock_bootstrap(host,assets));
    const auto prefix=o->stock_operation_evidence();
    auto native=bnw_stock_lua_win32_client_extent(initial.window,"genuine-renderer-stock-join",[revision]{return revision;});
    std::size_t reads=0;
    BnwStockLuaClientExtentProvider provider{[&]{
        ++reads;const auto value=native.current();
        need(value.has_value(),"Actual renderer GetClientRect unavailable");
        if(mode=="renderer_drop")context.reset();
        return value;
    }};
    if(mode=="renderer_alias"){
        // Same facade address, distinct control. No client callback may run.
        std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> forged(renderer.get(),[](const auto*){});
        need(forged.get()==renderer.get()&&(forged.owner_before(renderer)||renderer.owner_before(forged)),
             "No-op alias negative fixture accidentally uses canonical control");
        rejects([&]{o->attach_stock_renderer_window_owner(forged,provider);});
        need(reads==0,"Forged control reached real client callback");
        uncertain(o->stock_operation_evidence());
        rejects([&]{o->attach_stock_renderer_window_owner(renderer,provider);});
        need(reads==0,"Forged-control failure replayed native attachment");
        return;
    }
    if(mode=="renderer_drop"){
        rejects([&]{o->attach_stock_renderer_window_owner(renderer,provider);});
        need(!context&&reads==1,"Actual GlContext facade retirement did not occur after real extent read");
        uncertain(o->stock_operation_evidence());
        rejects([&]{(void)o->advance_stock_bootstrap();});
        need(reads==1,"Retired renderer callback was replayed");
        viewport.reset(); // Detach borrowed native router before physical teardown.
        return;
    }
    o->attach_stock_renderer_window_owner(renderer,provider);
    const auto attached=o->stock_operation_evidence();journal(attached);
    need(reads==1&&attached.operations.size()==prefix.operations.size()+1&&
         attached.operations.back().kind==BnwStockLuaOperationKind::attach_renderer_window&&
         attached.operations.back().same_vm_identity&&!attached.operations.back().host_failed&&
         attached.operations.back().outcome==BnwStockLuaOperationOutcome::incomplete_suspended,
         "Genuine renderer attach changed VM or pretended activation completion");
    std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> canonical=renderer;
    renderer.reset();need(!canonical.expired(),"Native binding retained only caller/raw facade");
    if(mode=="renderer_foreign_release"){
        // Binding is real but native child graphs have not been constructed.
        // This tests exactly the new physical Window consumption lifetime slice.
        const auto storage=o->stock_native_window_storage();
        std::weak_ptr<BnwSelectedStockLuaSourceOwner> source=o;
        need(!storage.expired()&&!canonical.expired()&&
             GetPropW(reinterpret_cast<HWND>(initial.window),L"Civis.StockNativeUI.WindowObserver")!=nullptr,
             "Foreign-release fixture has no actual native registration/storage");
        std::thread foreign([last=std::move(o)]()mutable{last.reset();});
        foreign.join(); // Synchronizes final facade/state destruction, no pump yet.
        need(source.expired()&&storage.expired()&&canonical.expired(),
             "Foreign final release retained source/native binding/canonical producer cycle");
        // Window3's real final-release path posts its actual creator cleanup.
        // Pump the creator queue; no forged message/ack or user window close.
        MSG message{};unsigned dispatched=0;
        while(dispatched<64&&PeekMessageW(&message,reinterpret_cast<HWND>(initial.window),0,0,PM_REMOVE)){
            TranslateMessage(&message);DispatchMessageW(&message);++dispatched;
        }
        need(dispatched>0&&GetPropW(reinterpret_cast<HWND>(initial.window),L"Civis.StockNativeUI.WindowObserver")==nullptr&&
             GetPropW(reinterpret_cast<HWND>(initial.window),L"Civis.OwnedRenderer.InitialWindow")==nullptr,
             "Creator pump did not remove actual consumer/producer registrations");
        viewport.reset(); // Actual borrowed input/router is still alive until detach.
        context.reset(); // Last real facade; deferred physical storage was drained.
        need(!IsWindow(reinterpret_cast<HWND>(initial.window)),
             "Creator cleanup left physical HWND/storage retained after facade teardown");
        return;
    }
    if(mode=="renderer_resize"){
        need(SetWindowPos(reinterpret_cast<HWND>(initial.window),nullptr,0,0,900,680,
                          SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE)!=FALSE,"Actual native resize unavailable");
        const auto held=reads;
        rejects([&]{(void)o->advance_stock_bootstrap();});
        need(reads==held,"Stale renderer reached client provider before currentness refusal");
        uncertain(o->stock_operation_evidence());
        rejects([&]{(void)o->advance_stock_bootstrap();});
        need(reads==held,"Stale renderer operation replayed");
        return;
    }
    // Physical Window attachment alone cannot serve GUI dimensions. Execute
    // the real selected Scene/frame/UI/compositor producer before resuming Lua.
    // Negative Window modes above remain independent of this GUI publication.
    const auto preview_input=host->preview_input();
    const auto& decoded=host->session().source();
    need(decoded.snapshot.pregame&&decoded.snapshot.map_plots&&decoded.snapshot.map_projection,
         "Actual renderer saved map source unavailable");
    const auto art=saves::observe_saved_map_team_art(*decoded.snapshot.pregame,*decoded.snapshot.map_plots);
    auto receipt=graphics::capture_bnw_renderer_scene_receipt(save,
        runtime::identify_file(save).sha256,decoded,art);
    need(receipt.has_value(),"Actual renderer source receipt unavailable");
    auto projection=*decoded.snapshot.map_projection;
    need(graphics::reconcile_bnw_renderer_scene_projection(projection,*receipt,host->session().state()),
         "Actual renderer scene projection differs from selected Host");
    const auto ticket=viewport->request();
    need(ticket&&reinterpret_cast<std::uintptr_t>(ticket->window())==initial.window&&
         ticket->version()==revision,"Actual renderer viewport ticket changed before GUI composition");
    const auto resources=renderer_gui_resources(assets);
    const auto registry=ui::UiContextRegistry::from_resources(resources);
    const auto scene=ui::BnwClientSceneFrame::create_selected_screen_scene(host,*receipt,preview_input.frame_sources);
    const auto actual_ui=scene->create_selected_ui_host(registry,scene->ui_provider(),nullptr);
    const auto actual_frame=scene->begin(ticket->viewport(),{});
    need(actual_frame&&scene->current(),"Actual renderer selected Scene did not publish a current frame");
    const auto ui_frame=actual_ui->begin_frame(ticket->viewport().framebuffer_width,
        ticket->viewport().framebuffer_height);
    need(scene->accepts(*actual_ui,ui_frame),"Actual renderer UI frame differs from selected Scene");
    const ui::UiScreenCompositor compositor(resources,registry,nullptr);
    std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> gui;
    {
        const auto physical=canonical.lock();
        need(static_cast<bool>(physical),"Actual renderer canonical Window expired before composition");
        physical->with_actual_window_loan([&](const auto& loan){
            need(loan.window()==reinterpret_cast<std::uintptr_t>(ticket->window())&&
                 loan.viewport_revision()==ticket->version(),
                 "Actual GUI frame and canonical Window have different viewport tickets");
        });
        gui=compositor.compose_owned_current_screen(scene,actual_ui,physical,ui_frame);
    } // Factory's temporary canonical Window owner is released here.
    need(static_cast<bool>(gui),"Actual compositor did not issue GUI screen");
    o->attach_stock_gui_screen(gui);
    need(o->advance_stock_bootstrap().pending_api=="Controls.ScrollBar.GetSize"&&
         o->advance_stock_bootstrap().pending_api=="Controls.ScrollPanel.GetSize"&&
         o->advance_stock_bootstrap().pending_api=="Controls.ScrollBar.SetSize",
         "Canonical Window did not serve reached actual source getters");
    const auto held=o->advance_stock_bootstrap();
    const auto frame=o->stock_native_runtime_frame();
    need(frame&&frame->initial_renderer_window_occurrence==occurrence&&
         frame->initial_renderer_dc==initial.dc&&frame->initial_renderer_wgl==initial.context&&
         frame->initial_renderer_viewport_revision==revision&&frame->box&&
         frame->box->pending_color_token=="0,0,0,160"&&!frame->box->source_append_completed,
         "Actual Window binding did not retain SAME stopped Box receiver/source");
    need(held.pending_api=="Controls.ScrollBar.SetSize"&&!held.native_scrollbar_resizes&&
         !held.loading_complete&&!held.gameplay_callbacks_ready&&
         !frame->production_window_lease_bound&&!frame->native_slider_setter_complete,
         "Physical Window binding bypassed Color/native setter or Lua activation");
    const auto repeated=o->advance_stock_bootstrap();
    need(repeated.prefix_executions==1&&!repeated.native_scrollbar_resizes&&
         repeated.pending_api==held.pending_api&&!repeated.gameplay_callbacks_ready,
         "Stopped same-VM source replayed or became active after Window binding");
    gui.reset(); // The stock owner must release its own final GUI/Window storage.
    o->retire_stock_bootstrap();o.reset();
    need(canonical.expired(),"Retired native binding leaked canonical renderer owner");
}
void invalidation(const std::filesystem::path& root,const std::filesystem::path& assets,
    const content::Catalog& catalog,const SavedContentProfile& profile,
    const std::shared_ptr<ui::SelectedBnwPreviewHost>& host,Window& window){
    {
        auto o=owner(root,catalog,profile,host->identity());first(o->start_stock_bootstrap(host,assets));
        const auto cp=host->session().checkpoint();host->session().restore(cp);
        rejects([&]{(void)o->advance_stock_bootstrap();});
        need(!o->fork_at(host->session().state()).epoch,"Stale source allocation opened epoch");
    }
    {
        auto o=owner(root,catalog,profile,host->identity());first(o->start_stock_bootstrap(host,assets));
        const auto cp=host->session().checkpoint();
        (void)host->session().mutable_state(); // no field change; private Read owner invalidates
        rejects([&]{(void)o->advance_stock_bootstrap();});
        host->session().restore(cp);rejects([&]{(void)o->advance_stock_bootstrap();});
    }
    for(int mode=0;mode<5;++mode){
        auto o=owner(root,catalog,profile,host->identity());first(o->start_stock_bootstrap(host,assets));
        auto native=window.provider();std::size_t calls=0;const auto cp=host->session().checkpoint();
        need(o->attach_stock_client_extent({[&,native]{
            ++calls;
            if(mode==0)(void)host->session().mutable_state();
            else if(mode==1)o->retire_stock_bootstrap();
            else if(mode==2)rejects([&]{(void)o->advance_stock_bootstrap();});
            else if(mode==3){
                need(native.current().has_value(),"Actual native extent unavailable before injected late throw");
                throw std::runtime_error("Failure after actual owned GetClientRect observation");
            }
            else rejects([&]{(void)o->start_stock_bootstrap(host,assets);});
            return native.current();
        }}),"Adversarial actual client provider not attached");
        need(calls==0,"Current bootstrap attachment unexpectedly invoked client provider");
        rejects([&]{(void)o->advance_stock_bootstrap();});
        need(calls==1,"Late stale/reentrant native callback repeated");
        const auto failed=o->stock_operation_evidence();uncertain(failed);
        need(!failed.error.empty(),"Original uncertainty reason was swallowed");
        rejects([&]{(void)o->advance_stock_bootstrap();});need(calls==1,"Poisoned callback replayed");
        const auto repeated=o->stock_operation_evidence();uncertain(repeated);
        need(repeated.operations.size()==failed.operations.size()&&
             repeated.operations.back().serial==failed.operations.back().serial&&
             repeated.error==failed.error,"Forbidden replay erased original VM journal/error");
        rejects([&]{(void)o->attach_stock_client_extent(window.provider());});
        if(mode==0)host->session().restore(cp);
    }
    {
        auto o=owner(root,catalog,profile,host->identity());first(o->start_stock_bootstrap(host,assets));
        bool rejected=false;std::thread other([&]{try{(void)o->advance_stock_bootstrap();}
            catch(const std::exception&){rejected=true;}});other.join();
        need(rejected,"Foreign thread used live stock coroutine");
        rejects([&]{(void)o->stock_bootstrap_progress();});
    }
    {
        auto o=owner(root,catalog,profile,host->identity());
        bool rejected=false;std::thread other([&]{try{(void)o->start_stock_bootstrap(host,assets);}
            catch(const std::exception&){rejected=true;}});other.join();
        need(rejected,"First stock VM start migrated owner creation thread");
        rejects([&]{(void)o->start_stock_bootstrap(host,assets);});
    }
    {
        auto o=owner(root,catalog,profile,host->identity());first(o->start_stock_bootstrap(host,assets));
        auto native=window.provider();std::size_t calls=0;
        need(o->attach_stock_client_extent({[&,native]{++calls;o.reset();return native.current();}}),
             "Self-retiring actual client provider not attached");
        auto* borrowed=o.get();rejects([&]{(void)borrowed->advance_stock_bootstrap();});
        need(!o&&calls==1,"Self-retired outer owner callback not safely rejected");
    }
}
}
int main(int argc,char** argv){
    try {
        need(argc==4||argc==5,"Requires actual BNW save, source/rules root, canonical asset root and optional renderer regression mode");
        const auto before=identify_file(argv[1]);
        const std::filesystem::path root=argv[2],assets=argv[3];
        if(argc==5){
            renderer_binding_regression(argv[1],root,assets,argv[4]);
            need(identify_file(argv[1]).sha256==before.sha256,"Original save changed during renderer fixture");
            std::cout<<"PASS actual canonical Window/SAME retained Lua join mode "<<argv[4]<<"; activation remains unavailable\n";
            return 0;
        }
        auto selected=ui::bootstrap_selected_bnw_preview(argv[1],root,0,true);
        need(static_cast<bool>(selected.preview),"Actual selected host bootstrap failed");
        auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(selected.preview.host));
        const auto catalog=content::discover_packages({root/"Assets/DLC"});
        need(catalog.diagnostics.empty(),"Actual selected catalog failed");
        const auto map=host->session().state().random.map.state();
        const auto game=host->session().state().random.gameplay.state();
        Window window;
        {
            auto wrong=host->identity();wrong.payload_sha256[0]=wrong.payload_sha256[0]=='a'?'b':'a';
            auto o=owner(root,catalog,selected.profile,wrong);
            rejects([&]{(void)o->start_stock_bootstrap(host,assets);});
            need(!o->stock_bootstrap_progress(),"Foreign payload bound persistent VM");
        }
        retained(root,assets,catalog,selected.profile,host,window);
        failure_after_stock_writes(root,assets,catalog,selected.profile,host,window);
        invalidation(root,assets,catalog,selected.profile,host,window);
        need(host->session().state().random.map.state()==map&&host->session().state().random.gameplay.state()==game,
             "Owned partial bootstrap changed live RNG");
        need(!host->session().readiness().can_continue(),"Bootstrap join opened complete gameplay readiness");
        std::weak_ptr<ui::SelectedBnwPreviewHost> lease=host;
        auto held=owner(root,catalog,selected.profile,host->identity());first(held->start_stock_bootstrap(host,assets));
        host.reset();need(!lease.expired(),"Persistent VM lost genuine selected host lifetime");
        first(held->advance_stock_bootstrap());
        // A real sealed rules writer attempt retires its SAME Impl even though
        // the host's cached identity string remains unchanged. Exercise after
        // a native observation; the wrapper must quarantine the whole VM step.
        auto actual=lease.lock();need(actual!=nullptr,"Actual retained host lost");
        const auto identity=actual->identity();auto native=window.provider();std::size_t calls=0;
        auto idle=owner(root,catalog,selected.profile,identity);
        first(idle->start_stock_bootstrap(actual,assets));
        const auto current=actual->database().immutable_preview_fingerprint_currentness();
        need(current&&current(),"Actual immutable rules producer unavailable");
        need(held->attach_stock_client_extent({[&,native]{
            ++calls;const auto extent=native.current();need(extent.has_value(),"Actual native extent unavailable");
            auto attempted=actual->database().script_query("UPDATE Defines SET Value=Value WHERE Name='MAX_HIT_POINTS'",{});
            need(!attempted&&!current(),"Actual rules write attempt did not retire immutable scope");
            return extent;
        }}),"Actual late-rules provider not attached");
        rejects([&]{(void)held->advance_stock_bootstrap();});
        uncertain(held->stock_operation_evidence());
        need(calls==1&&actual->identity().rules_sha256==identity.rules_sha256,
             "Rules retirement missed real callback or rewrote cached identity");
        rejects([&]{(void)held->advance_stock_bootstrap();});need(calls==1,"Retired rules callback replayed");
        // Independent idle VM was not callback-poisoned: its next step must
        // reject actual rules retirement before it would return a screen wait.
        rejects([&]{(void)idle->advance_stock_bootstrap();});
        auto after_retirement=owner(root,catalog,selected.profile,identity);
        rejects([&]{(void)after_retirement->start_stock_bootstrap(actual,assets);});
        need(!after_retirement->stock_bootstrap_progress(),"Retired DB published a new owned VM");
        held.reset();idle.reset();actual.reset();need(lease.expired(),"Retired VM leaked selected host");
        need(identify_file(argv[1]).sha256==before.sha256,"Original BNW save changed");
        std::cout<<"PASS actual owned selected stock coroutine/host join; full epoch/fork/publish unavailable\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
