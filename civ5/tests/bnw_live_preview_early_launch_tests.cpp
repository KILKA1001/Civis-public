#include "civ5/ui/bnw_live_preview_launch.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include <functional>
#include <iostream>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

using namespace civ5;
namespace {
void need(bool value,const char *why) { if(!value) throw std::runtime_error(why); }
void blocked(const ui::BnwLivePreviewOutcome &out,const char *why) {
    need(!out.rendered && out.blocker.has_value(),why);
}
struct PreparedClient {
    HWND hwnd=CreateWindowExW(0,L"STATIC",L"Civis EARLY launch CPU source gate",
        WS_OVERLAPPEDWINDOW,0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    std::uint64_t revision=1;
    std::function<void()> callback;
    std::shared_ptr<const ui::BnwNativeEarlyInputLease> lease;
    std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner> package;
    PreparedClient(const std::filesystem::path &save,const std::filesystem::path &game,
                   const std::filesystem::path &canonical) {
        need(hwnd!=nullptr,"Missing actual owned CPU test window");
        const auto native=runtime::bnw_stock_lua_win32_client_extent(
            reinterpret_cast<std::uintptr_t>(hwnd),"early-launch-cpu-window",[this]{return revision;});
        runtime::BnwStockLuaClientExtentProvider provider{
            [this,native]() -> std::optional<runtime::BnwStockLuaClientExtent> {
                // Pin executing callback: it may replace the member itself.
                const auto pinned=callback;
                if(pinned) pinned();
                return native.current();
            }};
        lease=ui::BnwNativeEarlyInputLease::acquire(save,game,canonical,
            reinterpret_cast<std::uintptr_t>(hwnd),std::move(provider));
        package=ui::BnwNativePackageDiscoveryOwner::create(lease);
    }
    ~PreparedClient() { if(hwnd) DestroyWindow(hwnd); }
    void retire() { ++revision; }
    PreparedClient(const PreparedClient&)=delete;
};
struct SourceScene {
    graphics::BnwRendererSceneReceipt receipt;
    saves::SavedMapProjection projection;
    explicit SourceScene(const std::filesystem::path &save) {
        // Existing BNW source-session parser and receipt producer. Real EARLY
        // lease/bootstrap independently pin the original BNW source profile.
        // This is a CPU source-scene gate, not GPU/native pixel evidence.
        const auto decoded=saves::read_saved_session(save,saves::SavedSessionDecodeContext{});
        need(decoded.snapshot.pregame && decoded.snapshot.map_plots &&
             decoded.snapshot.map_projection,"Actual saved map was not decoded");
        const auto art=saves::observe_saved_map_team_art(
            *decoded.snapshot.pregame,*decoded.snapshot.map_plots);
        auto captured=graphics::capture_bnw_renderer_scene_receipt(save,
            runtime::identify_file(save).sha256,decoded,art);
        need(captured.has_value(),"Actual independently decoded source receipt unavailable");
        receipt=std::move(*captured);
        projection=*decoded.snapshot.map_projection;
        need(!receipt.projection_reconciled,"Source receipt unexpectedly already reconciled");
    }
    bool prepare(const ui::LiveBnwPreviewInput *input,
                 graphics::BnwRendererSceneReceipt &restored) {
        need(input!=nullptr,"Missing borrowed current input");
        // Never set a success bit by hand. This applies all real source-cell,
        // city-load/nearby-city and active-team art checks before publication.
        return graphics::reconcile_bnw_renderer_scene_projection(
            projection,restored,input->session.state());
    }
};
ui::BnwPrepareRendererScene preparer(SourceScene &scene) {
    return [&scene](const ui::LiveBnwPreviewInput *input,
                   graphics::BnwRendererSceneReceipt &receipt) {
        return scene.prepare(input,receipt);
    };
}
struct CopyProbe {
    bool active=false;
    unsigned copies=0,calls=0;
    std::function<void()> effect;
};
struct CopyActiveObserver {
    std::shared_ptr<CopyProbe> probe;
    explicit CopyActiveObserver(std::shared_ptr<CopyProbe> p):probe(std::move(p)) {}
    CopyActiveObserver(const CopyActiveObserver &other):probe(other.probe) {
        if(probe->active) { ++probe->copies; probe->effect(); }
    }
    void operator()(const ui::SelectedBnwBootstrapResult &b) const {
        need(b.preview.host!=nullptr,"Copied profile observer lost actual host");
        ++probe->calls;
    }
};
struct CopyActiveRender {
    std::shared_ptr<CopyProbe> probe;
    explicit CopyActiveRender(std::shared_ptr<CopyProbe> p):probe(std::move(p)) {}
    CopyActiveRender(const CopyActiveRender &other):probe(other.probe) {
        if(probe->active) { ++probe->copies; probe->effect(); }
    }
    void operator()(const ui::LiveBnwPreviewInput *input) const {
        need(input!=nullptr,"Copied renderer lost actual input");
        ++probe->calls;
    }
};
}
int main(int argc,char **argv) {
    try {
        need(argc==4,"Expected SAVE ORIGINAL_GAME CANONICAL_ASSETS");
        const std::filesystem::path save=argv[1],game=argv[2],canonical=argv[3];
        ui::BnwLivePreviewRequest request{save,game,0};
        unsigned renders=0;
        const std::function<void(const ui::LiveBnwPreviewInput*)> render=
            [&](const ui::LiveBnwPreviewInput *input) {
                need(input && input->scene_identity && input->active_player_id==0 &&
                     !input->session.readiness().can_continue(),"Invalid actual CPU launch input");
                ++renders;
            };

        // Named route never silently falls back to a legacy host.
        blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
            request,save,{},render),"Missing real package accepted");
        {
            PreparedClient client(save,game,canonical);
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                request,save,client.package,render),"Missing actual scene receipt accepted");
            need(!client.package->current().primary_dx9_registration_complete && renders==0,
                "Missing receipt began resource boot/render");
            SourceScene source(save);
            auto wrong=request; wrong.game_root=game.parent_path();
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                wrong,save,client.package,render,&source.receipt,preparer(source)),
                "Different lease/request game accepted");
            auto bad=source.receipt; bad.scene_file_sha256.assign(64,'0');
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                request,save,client.package,render,&bad,preparer(source)),
                "Stale independently supplied scene accepted");
            need(renders==0,"A refused preflight rendered");
        }
        {
            PreparedClient client(save,game,canonical);
            SourceScene source(save);
            client.retire();
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                request,save,client.package,render,&source.receipt,preparer(source)),
                "Retired initial client ticket was refreshed/accepted");
            need(renders==0,"Retired initial lease rendered");
        }
        {
            PreparedClient client(save,game,canonical);
            SourceScene source(save);
            bool refused=false;
            client.callback=[&] {
                client.callback={};
                try { (void)ui::launch_selected_bnw_camera_preview(request,save,render); }
                catch(const std::exception &) { refused=true; }
            };
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                request,save,client.package,render,&source.receipt,preparer(source)),
                "Legacy route escaped caught EARLY callback reentry guard");
            need(refused && renders==0 && !client.package->current().primary_dx9_registration_complete,
                "Recursive legacy route began a second host/outer boot");
        }
        {
            PreparedClient client(save,game,canonical);
            SourceScene source(save);
            bool nested=false;
            client.callback=[&] {
                client.callback={};
                const auto out=ui::launch_selected_bnw_camera_preview_from_early_package(
                    request,save,client.package,render,&source.receipt,preparer(source));
                blocked(out,"Nested launch inside first lease callback accepted");
                nested=true;
            };
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                request,save,client.package,render,&source.receipt,preparer(source)),
                "Caught lease callback recursion did not retire outer launch");
            need(nested && renders==0 && !client.package->current().primary_dx9_registration_complete,
                "Provider recursion started nested/outer boot");
        }
        {
            PreparedClient client(save,game,canonical);
            SourceScene source(save);
            bool nested=false;
            client.callback=[&] {
                client.callback={};
                std::thread foreign([&] {
                    blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                        request,save,client.package,render,&source.receipt,preparer(source)),
                        "Foreign-thread active launch accepted");
                    nested=true;
                });
                foreign.join();
            };
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                request,save,client.package,render,&source.receipt,preparer(source)),
                "Caught foreign-thread launch did not retire outer operation");
            need(nested && renders==0,"Foreign-thread launch rendered");
        }
        {
            PreparedClient client(save,game,canonical);
            SourceScene source(save);
            auto observed=request;
            bool observed_once=false;
            observed.observe_selected_profile=[&](const ui::SelectedBnwBootstrapResult &b) {
                need(b.preview.host!=nullptr,"Missing observed adopted host");
                client.package->require_same_selected_host(*b.preview.host);
                const auto checkpoint=b.preview.host->session().checkpoint();
                b.preview.host->session().restore(checkpoint); // identical semantics, new generation
                observed_once=true;
            };
            const auto outcome=ui::launch_selected_bnw_camera_preview_from_early_package(
                observed,save,client.package,render,&source.receipt,preparer(source));
            blocked(outcome,"Identical restore inside profile callback accepted");
            if(!observed_once && outcome.blocker)
                std::cerr<<"Restore observer was not reached; earlier launch blocker: "
                         <<outcome.blocker->reason<<'\n';
            need(observed_once,"Restore observer was not reached");
            need(renders==0,"Observer restore reached renderer");
        }
        {
            PreparedClient client(save,game,canonical);
            SourceScene source(save);
            auto observed=request;
            observed.observe_selected_profile=[](const ui::SelectedBnwBootstrapResult &b) {
                auto &world=b.preview.host->session().mutable_state().world;
                need(world.map_projection && !world.map_projection->cells.empty(),"Missing actual cells");
                auto &terrain=world.map_projection->cells[0].terrain_type;
                terrain=terrain.value_or(0)+1;
            };
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                observed,save,client.package,render,&source.receipt,preparer(source)),
                "Same-allocation live world mutation accepted");
            need(renders==0,"Observer world mutation reached renderer");
        }
        {
            PreparedClient client(save,game,canonical);
            SourceScene source(save);
            bool prepared=false;
            const ui::BnwPrepareRendererScene prepare=[&](const ui::LiveBnwPreviewInput *input,
                    graphics::BnwRendererSceneReceipt &receipt) {
                prepared=source.prepare(input,receipt);
                client.retire();
                return prepared;
            };
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                request,save,client.package,render,&source.receipt,prepare),
                "Retirement after actual scene reconciliation accepted");
            need(prepared && renders==0,"Preparation retirement was not tested at its boundary");
        }
        {
            PreparedClient client(save,game,canonical);
            SourceScene source(save);
            unsigned closed_renders=0;
            const auto closing=[&](const ui::LiveBnwPreviewInput *input) {
                render(input); ++closed_renders;
                const auto hwnd=client.hwnd;
                client.hwnd=nullptr;
                need(DestroyWindow(hwnd)!=0,"Actual test window was not retired");
                client.retire();
            };
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                request,save,client.package,closing,&source.receipt,preparer(source)),
                "Callback return after real HWND retirement published rendered");
            need(closed_renders==1,"Real retirement did not occur during render");
        }
        {
            PreparedClient client(save,game,canonical);
            SourceScene source(save);
            bool nested=false;
            const auto recursive_render=[&](const ui::LiveBnwPreviewInput *input) {
                render(input);
                const auto out=ui::launch_selected_bnw_camera_preview_from_early_package(
                    request,save,client.package,render,&source.receipt,preparer(source));
                blocked(out,"Nested launch inside renderer accepted");
                nested=true;
            };
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                request,save,client.package,recursive_render,&source.receipt,preparer(source)),
                "Caught render recursion published outer success");
            need(nested,"Renderer recursion was not reached");
            const auto before=renders;
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                request,save,client.package,render,&source.receipt,preparer(source)),
                "Already registered/adopted package was silently rebooted");
            need(renders==before,"Repeated boot reached renderer");
        }
        {
            // std::function copying itself can legally invoke user code.
            // This observer copy destroys the borrowed optional receipt BEFORE
            // any profile/prepare/render callback; the pinned receipt must live.
            PreparedClient client(save,game,canonical);
            SourceScene source(save);
            auto exported=std::optional{source.receipt};
            auto selected_scene=save;
            auto observed=request;
            auto probe=std::make_shared<CopyProbe>();
            observed.observe_selected_profile=CopyActiveObserver(probe);
            probe->effect=[&] {
                exported.reset(); selected_scene.clear();
                observed.save_path.clear(); observed.game_root.clear();
                observed.active_player_id=-1;
            };
            probe->active=true;
            unsigned copy_renders=0;
            const auto out=ui::launch_selected_bnw_camera_preview_from_early_package(
                observed,selected_scene,client.package,
                [&](const ui::LiveBnwPreviewInput *input) { render(input); ++copy_renders; },
                &*exported,preparer(source));
            need(out.rendered && !out.blocker && !exported && selected_scene.empty() &&
                 probe->copies==1 && probe->calls==1 && copy_renders==1,
                 "Legal observer copy invalidated borrowed receipt/path or was recopied");
        }
        {
            PreparedClient client(save,game,canonical);
            SourceScene source(save);
            auto probe=std::make_shared<CopyProbe>();
            std::function<void(const ui::LiveBnwPreviewInput*)> active_render=CopyActiveRender(probe);
            bool refused=false;
            probe->effect=[&] {
                const auto out=ui::launch_selected_bnw_camera_preview_from_early_package(
                    request,save,client.package,render,&source.receipt,preparer(source));
                blocked(out,"Callable copy recursion accepted");
                refused=true;
            };
            probe->active=true;
            blocked(ui::launch_selected_bnw_camera_preview_from_early_package(
                request,save,client.package,active_render,&source.receipt,preparer(source)),
                "Caught callable copy recursion continued launch");
            need(refused && probe->copies==1 && probe->calls==0 &&
                 !client.package->current().primary_dx9_registration_complete,
                 "Poisoned copy authorized another borrowed read/resource boot/render");
        }
        {
            // Positive CPU lifecycle: same window/table/adopted host retained
            // through final outcome. Parent closes only after its result checks.
            PreparedClient client(save,game,canonical);
            SourceScene source(save);
            const auto *table=&client.package->resources();
            const runtime::LiveSession *same_session=nullptr;
            auto observed=request;
            std::function<void(const ui::LiveBnwPreviewInput*)> pinned_render;
            ui::BnwPrepareRendererScene pinned_prepare=preparer(source);
            auto external=client.package;
            client.package.reset(); // this variable supplies the last public package owner
            const std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> weak=external;
            observed.observe_selected_profile=[&](const ui::SelectedBnwBootstrapResult &b) {
                same_session=&b.preview.host->session();
                const auto held=weak.lock();
                need(held!=nullptr,"Observed adopted package was destroyed");
                held->require_same_selected_host(*b.preview.host);
            };
            pinned_render=[&](const ui::LiveBnwPreviewInput *input) {
                need(input && &input->session==same_session && IsWindow(client.hwnd),
                    "CPU renderer received a substituted host/window");
                const auto held=weak.lock();
                need(held && &held->resources()==table,"Initial table/package lifetime was lost");
                external.reset();
                observed={};
                pinned_prepare={};
                pinned_render={}; // executing function is pinned by named route
                render(input);
            };
            const auto before=renders;
            const auto out=ui::launch_selected_bnw_camera_preview_from_early_package(
                observed,save,external,pinned_render,&source.receipt,pinned_prepare);
            if(out.blocker) std::cerr<<"CPU initial launch blocker: "<<out.blocker->reason<<'\n';
            need(out.rendered && !out.blocker && renders==before+1 && IsWindow(client.hwnd),
                "Actual source-scene CPU initial launch did not complete under live lease");
            need(!out.readiness.can_continue() && weak.expired(),
                "Launch raised game readiness or leaked adopted package/host");
        }
        std::cout<<"PASS EARLY named CPU launch: actual receipt/reconciliation, same lease/table/host, "
                     "reentry/thread/restore/world/preparation/real-close guards and last-owner retention; "
                     "no GPU/native Label/App/UIManager/full-game readiness claim\n";
        return 0;
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
