#include "civis_frontend_party_routing_owner.hpp"
#include "civ5/runtime/detail/bnw_civis_application_state_owner.hpp"
#include "civis_frontend_party_scene_source.hpp"
#include "civis_frontend_party_world_source.hpp"
#include "civis_frontend_party_material_storage.hpp"
#include "civis_frontend_alpha_land_frame.hpp"
#include "frontend_load_progress.hpp"
#include "../../../validation/material_world_validation.hpp"
#include "../../../validation/material_preview_settings.hpp"
// These fragments declare the actual LAND/water/Camera dependencies in order.
#include "../../../validation/water_camera_replay.hpp"
#include "../../../validation/water_world_scene.hpp"
#include "../../../validation/scheduled_material_preview.hpp"
#include "../../../validation/gl_bnw_warrior_owned_scene.hpp"
#include "../../../validation/camera_preview.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_owned_camera_ui_source_input.hpp"
#include "civ5/terrain/terrain_source_rivers.hpp"
#include <atomic>
#include <chrono>
#include <exception>

namespace civ5::ui::detail {
namespace {
void party_need(bool v,const char* message){if(!v)throw std::runtime_error(message);}
std::filesystem::path party_fresh_output(const std::filesystem::path& parent){
    party_need(!parent.empty(),"Party output descriptor is absent");
    std::filesystem::create_directories(parent);
    const auto clock=std::chrono::steady_clock::now().time_since_epoch().count();
    const auto child=parent/("saved-world-own-alpha-"+std::to_string(GetCurrentProcessId())+"-"+
        std::to_string(GetCurrentThreadId())+"-"+std::to_string(clock));
    party_need(std::filesystem::create_directory(child),"Party output occurrence already exists; refusing overwrite");
    party_need(std::filesystem::is_empty(child),"Party output occurrence is not empty");return child;
}
}
struct CivisFrontendPartyRoutingOwner::State {
    const DWORD creator=GetCurrentThreadId();
    const std::shared_ptr<graphics::GlContext> context;
    const HWND window;
    const std::shared_ptr<runtime::BnwCivisApplicationStateOwner> application;
    const std::shared_ptr<PreviewInput> input=std::make_shared<PreviewInput>();
    std::shared_ptr<const Win32ClientWindowBinding::ExistingInputLoan> binding;
    std::shared_ptr<PreparedCameraViewer> viewer;
    std::atomic<bool> alive{true},failed{false};
    bool routing=false,executing=false;
    std::size_t backend_facades=0;
    explicit State(std::shared_ptr<graphics::GlContext> c)
      :context(std::move(c)),window(static_cast<HWND>(context->native_window_handle())),
       application(runtime::BnwCivisApplicationStateOwner::create_menu_owned(reinterpret_cast<std::uintptr_t>(window))){}
    void first()const {
        party_need(GetCurrentThreadId()==creator,"Party input belongs to another thread");
        party_need(alive.load()&&!failed.load(),"Party input owner retired or quarantined");
        if(binding){binding->require_same_input(input->router);
            party_need(binding->window()==window,"Party input window changed");}
    }
    void idle()const{first();party_need(binding!=nullptr,"Party binding has not attached");binding->require_idle();first();}
};
CivisFrontendPartyRoutingOwner::CivisFrontendPartyRoutingOwner(std::shared_ptr<State> s):state_(std::move(s)){}
std::shared_ptr<CivisFrontendPartyRoutingOwner> CivisFrontendPartyRoutingOwner::create(
    std::shared_ptr<graphics::GlContext> context){
    party_need(context!=nullptr,"Party input needs the existing GL owner");
    auto state=std::make_shared<State>(std::move(context));state->first();
    return std::shared_ptr<CivisFrontendPartyRoutingOwner>(new CivisFrontendPartyRoutingOwner(std::move(state)));
}
CivisFrontendPartyRoutingOwner::~CivisFrontendPartyRoutingOwner(){
    if(GetCurrentThreadId()!=state_->creator)std::terminate();
    // An attached owner must retire while its actual binding still lives.
    // Relying on facade destruction after local binding teardown is invalid.
    if(state_->binding)std::terminate();state_->alive.store(false);
}
void CivisFrontendPartyRoutingOwner::retire_owned()noexcept{
    const auto s=state_;
    if(GetCurrentThreadId()!=s->creator||s->routing||s->executing||s->backend_facades)std::terminate();
    if(!s->alive.load())return;
    if(s->binding){try{s->binding->require_retirement_idle_owned(s->input->router);}catch(...){s->failed.store(true);std::terminate();}}
    s->alive.store(false); // FIRST. No new route can use retained targets.
    s->application->retire_owned();
    s->viewer.reset();s->binding.reset();
}
ClientWindowInput& CivisFrontendPartyRoutingOwner::window_input()const{
    state_->first();return state_->input->router;
}
void CivisFrontendPartyRoutingOwner::attach_existing_binding(Win32ClientWindowBinding& actual){
    const auto s=state_;s->first();party_need(!s->binding&&!s->viewer&&!s->routing&&!s->executing,"Party binding attachment repeated");
    auto loan=actual.retain_existing_input_owned();loan->require_same_input(s->input->router);
    party_need(loan->window()==s->window,"Party binding is not on the existing GL window");loan->require_idle();
    try{
        auto viewer=std::shared_ptr<PreparedCameraViewer>(new PreparedCameraViewer(s->input));
        viewer->context=s->context;viewer->window=s->window;viewer->existing_menu_binding=true;
        viewer->width=viewer->height=0; // Set from the actual existing client rectangle below.
        party_need(GetClientRect(s->window,&viewer->client_before_layout)!=FALSE,"Party client measurement failed");
        viewer->client_after_layout=viewer->client_before_layout;
        viewer->client_width=viewer->client_before_layout.right;
        viewer->client_height=viewer->client_before_layout.bottom;
        party_need(viewer->client_width>0&&viewer->client_height>0,"Party actual client extent is empty");
        viewer->width=viewer->client_width;viewer->height=viewer->client_height; // Same HWND extent, no resize.
        viewer->dpi_before_layout=viewer->dpi_after_layout=preview_window_dpi(s->window);
        viewer->initial_wgl=wglGetCurrentContext();viewer->initial_dc=wglGetCurrentDC();
        const std::weak_ptr<PreparedCameraViewer> weak=viewer;
        viewer->viewport_transaction=std::unique_ptr<Win32ViewportTransaction>(new Win32ViewportTransaction(
            loan,s->input->router,
            [weak](const auto& retired){
                const auto v=weak.lock();party_need(v&&v->alive,"Party retirement target disappeared");
                v->viewport_version=retired.version;v->last_retirement_reason=retired.reason;v->viewport_retired_pending=true;
                if(v->initial_ticket)v->initial_epoch_retired=true;
                if(retired.reason==Win32ViewportTransaction::RetirementReason::close)v->native_window_close_retired=true;
                v->input.cancel=true;v->input.pointer_valid=false;v->retire_client_audio();
                {const auto metadata=v->retire_metadata;if(metadata)metadata();}
                party_need(v->alive,"Party retired during metadata target destruction");
            },
            [weak](const ClientWindowEvent& event){
                const auto v=weak.lock();party_need(v&&v->alive,"Party input target disappeared");
                if(event.kind==ClientWindowEventKind::close&&v->stock_source_owner){
                    party_need(event.worldview_input&&!v->native_close_occurrence,"Party native close occurrence repeated");
                    v->native_close_occurrence=event.worldview_input;
                }
                camera_preview_event(v->window,v->input,event);
                // Native TechAward capture ran before Return; actual world-plot
                // provider/GL work is now outside every input receipt.
                {const auto pick=v->world_plot_input;if(pick)pick(event);}
                party_need(v->alive,"Party event target retired during callback destruction");
            }));
        viewer->drain_existing_=[](Win32ViewportTransaction& actual){actual.drain_existing_events_owned();};
        viewer->initial_ticket=viewer->viewport_transaction->request();viewer->viewport_retired_pending=false;
        viewer->require_initial_ticket();loan->require_idle();s->first();
        s->binding=std::move(loan);s->viewer=std::move(viewer);s->idle();
    }catch(...){s->failed.store(true);throw;}
}
bool CivisFrontendPartyRoutingOwner::route(const ClientWindowEvent& event){
    const auto s=state_;
    party_need(GetCurrentThreadId()==s->creator,"Party native route foreign creator");
    if(!s->alive.load())return false; // Native cancellation during real retirement goes to the original menu sink.
    s->first();
    if(!s->executing)return false; // Menu's existing sink receives the event.
    party_need(s->binding&&s->viewer,"Party route lacks its actual binding/viewer");
    if(s->routing){s->failed.store(true);throw std::runtime_error("Party input routing reentry");}
    s->routing=true;struct Exit{State& s;~Exit(){s.routing=false;}} exit{*s};
    try{
        s->binding->claim_sink_event_owned(event);
        // No idle check: this is the SAME real native sink. Its genuine entered
        // receipts authenticate capture; all full/GL work drains after Return.
        s->viewer->viewport_transaction->capture_existing_event_owned(event);
        {const auto capture=s->viewer->native_pointer_capture;if(capture)capture(event);}
        s->first();return true;
    }catch(...){s->failed.store(true);throw;}
}
void CivisFrontendPartyRoutingOwner::begin_camera_input_owned(const runtime::BnwSelectedStockLuaSourceOwner& source){
    const auto s=state_;s->idle();
    party_need(s->viewer&&s->viewer->stock_source_owner.get()==&source&&!s->executing&&
        s->viewer->viewport_transaction,"Party route requires actual bound Camera Source");
    s->viewer->viewport_transaction->require_worldview_input_source_owned(s->viewer->stock_source_owner);
    s->idle();s->executing=true;
}

// WorldSource now has a real per-request saved-input/storage producer.
class CivisFrontendPartyRendererBackend final : public FrontendBackend {
    const DWORD creator_=GetCurrentThreadId();
    const FrontendCatalogContentAdapter content_;
    const std::shared_ptr<CivisFrontendPartyRoutingOwner> routing_;
    const std::filesystem::path game_root_,payload_root_,output_root_;
    std::filesystem::path load_output_;
    const std::shared_ptr<FrontendLoadProgress> progress_;
    runtime::BnwStockLuaClientExtentProvider loading_client(
        runtime::BnwStockLuaClientExtentProvider original) const {
        const auto progress = progress_;
        return {[original = std::move(original), progress] {
            // Cooperative OS dispatch stays on the actual creator thread.
            // The original extent provider runs AFTER it and rejects any
            // changed viewport/context; this callback supplies no authority.
            progress->poll();
            return original.current();
        }};
    }
    // ONE real Source26/material/frame occurrence from the selected request.
    std::shared_ptr<CivisFrontendPartyWorldSource> world_source_;
    std::shared_ptr<const CivisSavedWorldLoadingOwner> loading_source_;
    std::unique_ptr<CivisFrontendPartyMaterialStorage> material_storage_;
    std::unique_ptr<CivisFrontendAlphaLandFrame> alpha_frame_;
    std::shared_ptr<assets::CivisResourceBankOwner> bank_;
    std::unique_ptr<CivisFrontendPartySceneSource> scene_;
    bool busy_=false,attempted_=false,failed_=false;
    void first()const{
        party_need(GetCurrentThreadId()==creator_,"Party backend belongs to another thread");
        party_need(!failed_,"Party backend quarantined");routing_->state_->idle();
    }
    UiCommandResult unavailable(const FrontendRequest& request)const{
        if(request.operation!="load_game")return {UiCommandStatus::unavailable,"This action has no retained game owner"};
        if(attempted_||busy_)return {UiCommandStatus::unavailable,"This retained load occurrence is closed"};
        const auto& rows=content_.data().saves;
        const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto& row){
            return request.draft.save&&row.id==*request.draft.save&&(!request.save||request.save->id==row.id);});
        if(found==rows.end())return {UiCommandStatus::unavailable,"Select an owned save first"};
        if(found->availability.status!=UiCommandStatus::ready)return found->availability;
        // Immutable descriptor only on paint. Real selected/save/Bank/window
        // validation and the one source birth run inside execute(load_game).
        return {UiCommandStatus::ready,"Own saved-world render; validation occurs on load"};
    }
    // Actual GL/LAND/Camera body. This is not an injected render callback.
    // Instantiation requires the genuine upstream owner above; no public caller
    // can supply raw primitive fields or anonymous texture IDs as authority.
    template<class ActualWorldSource>UiCommandResult execute_actual_owned(
        const FrontendRequest& request,const ActualWorldSource& source){
        const auto camera_output=load_output_/"camera-client";
        party_need(std::filesystem::create_directory(camera_output)&&std::filesystem::is_empty(camera_output),
            "Actual Camera output already exists; refusing overwrite");first();
        auto selected=content_.prepare_selected_save_input(request);selected.require_files_current();first();
        source.require_selected_owned(selected,*routing_->state_->context);first();
        auto world=source.world_owned();
        party_need(world!=nullptr,"Party renderer requires actual owned MaterialWorld");
        source.require_selected_owned(selected,*routing_->state_->context);selected.require_files_current();first();
        const auto viewer=routing_->state_->viewer;viewer->require_initial_ticket();
        party_need(world_source_!=nullptr,"Party renderer lacks request source");
        const auto window=world_source_->renderer_window_owned();window->require_current();
        const auto client=loading_client(viewer->early_client_provider());
        // The real request preparation already captured this SAME Bank once.
        party_need(bank_&&world_source_,"Party renderer lacks the actual request-owned source");
        bank_->require_current();world_source_->require_current();first();
        const auto rules=world_source_->selected_rules_owned();first();
        scene_=CivisFrontendPartySceneSource::select(content_,request,bank_,world,window,rules);
        first();source.require_selected_owned(selected,*viewer->context);
        const auto render=[&](std::shared_ptr<BnwOwnedCameraUiSourceInput> input_source,
            const SelectedBnwReadOnlyPreview& preview,
            std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner> stock,
            std::shared_ptr<BnwWorldViewFontSourceScope> fonts){
            first();source.require_selected_owned(selected,*viewer->context);input_source->require_current();
            party_need(stock&&fonts&&!viewer->stock_source_owner&&!routing_->state_->executing,
                       "Party renderer received repeated or absent actual Source");
            viewer->stock_source_owner=std::move(stock);viewer->renderer_window_owner=window;
            viewer->party_routing_=routing_;
            struct Clear{CivisFrontendPartyRoutingOwner::State& s;std::shared_ptr<PreparedCameraViewer> v;
                ~Clear(){s.executing=false;v->retire_gui_screen();v->native_pointer_capture={};
                    v->stock_source_owner.reset();v->renderer_window_owner.reset();}}clear{*routing_->state_,viewer};
            const auto host=input_source->parent();const auto* live=&input_source->input();input_source->require_current();
            {
                window->require_current();first();Control control(viewer->context,false);
                auto color_keys=source.color_keys_owned();
                auto land_scene=source.land_inputs_owned();
                const auto* fog=source.distance_fog_owned();
                const auto lighting=source.lighting_owned();
                party_need(color_keys&&fog,"Party source lacks actual ColorKey or distance-fog owners");
                source.require_selected_owned(selected,*viewer->context);window->require_current();first();
                graphics::GlMaterialWorldRenderer renderer(viewer->context,world,source.cubes_owned(),{false,true,true},
                    graphics::LightingCubeSampling::measured_coordinates,
                    graphics::LandShadowArithmetic::measured_fma_t_x_z_y,
                    graphics::LandMipSampling::d3d9_rgb565,color_keys);
                ScheduledMaterialPreview land(viewer->context,renderer,world,lighting,false,source.profile_owned(),false);
                const auto* ui=&fonts->resources();const auto language=fonts->locale().language;
                source.require_selected_owned(selected,*viewer->context);input_source->require_current();window->require_current();first();
                const auto output=camera_output;party_need(!output.empty(),"Party actual client output occurrence absent");
                source.require_selected_owned(selected,*viewer->context);first();
                progress_->stage("playing");
                progress_->disable_polling(); // The real Camera loop owns dispatch now.
                run_camera_preview(viewer->context,world,land,control,land_scene,payload_root_,
                    output,false,{},true,fog,true,color_keys.get(),false,{},ui,
                    &scene_->team_art(),&scene_->projection(),false,language,{}, {},false,{},
                    selected.save_identity().sha256,{}, {},nullptr,live,nullptr,nullptr,false,
                    &scene_->scene_receipt(),{}, {},nullptr,viewer,false,host,nullptr,false,false
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                    ,&preview,selected.save_path(),game_root_
#endif
                    ,std::move(fonts),alpha_frame_.get(),material_storage_.get());
                routing_->state_->executing=false;
                // All source/renderer target scopes and their destructors stay
                // before the final complete selected-input/Bank POST.
                source.require_selected_owned(selected,*viewer->context);first();
            }
            source.require_selected_owned(selected,*viewer->context);selected.require_files_current();first();
            (void)preview; // Real launch keeps the same immutable preview owner.
        };
        BnwLivePreviewOutcome outcome;
        {
            const auto prepare=scene_->prepare_scene();
            outcome=scene_->handoff().execute(scene_->active_player(),scene_->scene_save(),window,client,
                render,scene_->scene_receipt(),prepare,{},routing_->state_->application);
        } // callback copies/prepare targets retire before final source POST.
        source.require_selected_owned(selected,*viewer->context);selected.require_files_current();bank_->require_current();first();
        if(outcome.blocker)return {UiCommandStatus::unavailable,outcome.blocker->reason};
        party_need(viewer->party_loop_return_!=nullptr,
                   "Party load has no physical presentation and synchronous loop Return");
        viewer->party_loop_return_->require_for_owned(*viewer);first();
        party_need(viewer->viewport_transaction&&!viewer->run_active&&viewer->last_run_paint_lifetime.expired()&&
                   !viewer->retire_metadata&&!viewer->world_plot_input&&!viewer->native_pointer_capture,
                   "Party terminal close still borrows camera paint/input targets");
        const auto close=viewer->party_loop_return_->close_;
        viewer->viewport_transaction->require_staged_native_close_owned(close);
        window->require_current();first();
        // Every outer launch/Bank/material callback and destructor POST above
        // returned with the genuine Window live. Only this sealed owner now retires.
        viewer->viewport_transaction->finalize_native_close(close);first();
        viewer->native_close_occurrence.reset();viewer->party_loop_return_.reset();
        party_need(viewer->native_window_close_retired,"Party terminal retirement did not reach its actual owner");
        // outcome.rendered is deliberately not consulted here.
        return {UiCommandStatus::performed,{}};
    }
public:
    CivisFrontendPartyRendererBackend(const FrontendCatalogContentAdapter& content,
        std::shared_ptr<CivisFrontendPartyRoutingOwner> routing,std::filesystem::path game,std::filesystem::path payload,
        std::filesystem::path output,std::function<void(std::string_view)> loading_progress)
      :content_(content),routing_(std::move(routing)),game_root_(std::move(game)),payload_root_(std::move(payload)),output_root_(std::move(output)),
       progress_(std::make_shared<FrontendLoadProgress>(std::move(loading_progress))){
        party_need(routing_&&!game_root_.empty()&&!payload_root_.empty()&&!output_root_.empty(),"Party backend needs explicit source roots/routing/output descriptor");first();
        party_need(routing_->state_->backend_facades<std::numeric_limits<std::size_t>::max(),"Party backend count overflow");
        ++routing_->state_->backend_facades;
    }
    ~CivisFrontendPartyRendererBackend(){
        if(GetCurrentThreadId()!=creator_||busy_)std::terminate();failed_=true;
        try{routing_->state_->binding->require_retirement_idle_owned(routing_->state_->input->router);}catch(...){std::terminate();}
        scene_.reset();alpha_frame_.reset();material_storage_.reset();loading_source_.reset();world_source_.reset();bank_.reset();
        if(!routing_->state_->backend_facades)std::terminate();--routing_->state_->backend_facades;
    }
    UiCommandResult availability(const FrontendRequest& request)const override{
        // Pure retained save descriptor on paint; never a frame/completion.
        if(GetCurrentThreadId()!=creator_||failed_)return {UiCommandStatus::unavailable,"Party backend retired"};
        return unavailable(request);
    }
    FrontendContentCheck check_content(const FrontendRequest& request)const override{
        first();auto result=content_.check_content(request);first();return result;
    }
    UiCommandResult execute(const FrontendRequest& request)override{
        first();if(busy_){failed_=true;throw std::runtime_error("Party backend reentry");}
        if(request.operation!="load_game")return unavailable(request);
        if(attempted_)return {UiCommandStatus::unavailable,"This retained load occurrence is closed"};
        const auto available=unavailable(request);if(available.status!=UiCommandStatus::ready)return available;
        if(busy_){failed_=true;throw std::runtime_error("Party backend reentry");}
        busy_=true;attempted_=true;struct Exit{bool& value;~Exit(){value=false;}}exit{busy_};
        try{
            load_output_=party_fresh_output(output_root_);first();
            progress_->open(load_output_);progress_->enable_polling();
            progress_->stage("checking-save");
            auto selected=content_.prepare_selected_save_input(request);selected.require_files_current();first();
            const auto viewer=routing_->state_->viewer;viewer->require_initial_ticket();
            const auto window=viewer->viewport_transaction->capture_renderer_window_owner(viewer->context);window->require_current();
            const auto client=loading_client(viewer->early_client_provider());
            progress_->stage("checking-content");
            const auto lease=BnwNativeEarlyInputLease::acquire(selected.save_path(),game_root_,payload_root_,
                reinterpret_cast<std::uintptr_t>(viewer->window),client);
            const std::array<assets::CivisRequiredResource,8> roots{{
                {"Assets/UI/InGame/InGame.xml",assets::CivisResourceParse::xml,assets::CivisResourceSelection::winner},
                {"Assets/UI/InGame/WorldView/WorldView.xml",assets::CivisResourceParse::xml,assets::CivisResourceSelection::winner},
                {"Assets/UI/InGame/WorldView/NotificationPanel.xml",assets::CivisResourceParse::xml,assets::CivisResourceSelection::winner},
                {"Assets/UI/Fonts/Tw Cent MT/TwCenMT14.ggxml",assets::CivisResourceParse::xml,assets::CivisResourceSelection::winner},
                {"Assets/UI/Fonts/Tw Cent MT/TwCenMT16.ggxml",assets::CivisResourceParse::xml,assets::CivisResourceSelection::winner},
                {"Assets/UI/Fonts/Tw Cent MT/TwCenMT18.ggxml",assets::CivisResourceParse::xml,assets::CivisResourceSelection::winner},
                {"Assets/UI/Fonts/Tw Cent MT/TwCenMT20.ggxml",assets::CivisResourceParse::xml,assets::CivisResourceSelection::winner},
                {"Assets/UI/Fonts/Tw Cent MT/TwCenMT24.ggxml",assets::CivisResourceParse::xml,assets::CivisResourceSelection::winner}}};
            party_need(!bank_&&!world_source_&&!loading_source_&&!material_storage_&&!alpha_frame_,"Retained load cannot recapture its source occurrence");
            progress_->stage("loading-resources");
            bank_=assets::CivisResourceBankOwner::capture(lease,roots);first();selected.require_files_current();
            progress_->stage("loading-language");
            const auto language=runtime::BnwSelectedLanguageInputOwner::capture_from_civis_resources(bank_);language->require_current();first();
            progress_->stage("loading-interface");
            const auto ui=CivisResourceBankUiSnapshot::capture(bank_);first();
            progress_->stage("loading-fonts");
            const auto fonts=CivisResourceBankFontColorSnapshot::capture(bank_,ui);first();
            const auto scope=BnwWorldViewFontSourceScope::create_from_civis_resources(bank_,ui,fonts,language->language());first();
            auto handoff=CivisFrontendPartyBankHandoff::capture(std::move(selected),bank_,ui,fonts,scope);first();
            progress_->stage("restoring-world");
            world_source_=CivisFrontendPartyWorldSource::capture(std::move(handoff),bank_,window,viewer->context);first();
            loading_source_=world_source_->loading_inputs_owned();world_source_->require_current();first();
            progress_->stage("preparing-terrain");
            material_storage_=CivisFrontendPartyMaterialStorage::capture_owned(*world_source_,viewer->context);
            material_storage_->require_current();world_source_->require_current();first();
            party_need(viewer->client_width>0&&viewer->client_height>0,"Party actual framebuffer extent absent");
            progress_->stage("preparing-frame");
            alpha_frame_=CivisFrontendAlphaLandFrame::capture_owned(*material_storage_,viewer->context,
                static_cast<std::uint32_t>(viewer->client_width),static_cast<std::uint32_t>(viewer->client_height));first();
            const auto actual_selected=content_.prepare_selected_save_input(request);actual_selected.require_files_current();first();
            progress_->stage("drawing-frame");
            alpha_frame_->render_first_owned(*material_storage_,actual_selected,load_output_);first();
            material_storage_->require_selected_owned(actual_selected,*viewer->context);first();world_source_->require_current();first();
            // Continue through the original typed scene/source handoff and its
            // normal Camera loop. Only real Swap + installed POST + private
            // PartyLoopReturn may produce performed; outcome.rendered is unused.
            progress_->stage("starting-world");
            const auto result=execute_actual_owned(request,*material_storage_);
            progress_->disable_polling();progress_->stage("returned");
            return result;
        }catch(...){
            const auto error=std::current_exception();failed_=true;
            progress_->failed();
            // Actual loop closures/SceneTarget aliases have unwound. Retire
            // scene/frame/material GPU targets before fresh genuine Source POST,
            // preserving the original exception and the closed attempted flag.
            scene_.reset();alpha_frame_.reset();material_storage_.reset();
            if(world_source_){try{world_source_->require_current();}catch(...){}}
            std::rethrow_exception(error);
        }
    }
};
std::shared_ptr<FrontendBackend> create_civis_frontend_party_renderer_backend(
    const FrontendCatalogContentAdapter& content,std::shared_ptr<CivisFrontendPartyRoutingOwner> routing,
    std::filesystem::path game,std::filesystem::path payload,std::filesystem::path output,
    std::function<void(std::string_view)> loading_progress){
    return std::make_shared<CivisFrontendPartyRendererBackend>(content,std::move(routing),std::move(game),std::move(payload),std::move(output),std::move(loading_progress));
}
}
