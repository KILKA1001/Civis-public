#include "civ5/ui/frontend_party_alpha_land_layers.hpp"
#include "detail/civis_frontend_party_world_source.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <stdexcept>
#include <thread>
#include <utility>
#include <windows.h>

namespace civ5::ui {
namespace {
void need(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
// Read physical WGL without selecting or repairing a context. Held handles
// alone do not show which context is currently receiving raw GL operations.
const graphics::GlContext& already_current(const graphics::GlContext& context){
    const auto identity=context.native_identity();
    need(identity.owner_thread && identity.window && identity.dc && identity.context &&
         reinterpret_cast<std::uintptr_t>(wglGetCurrentContext())==identity.context &&
         reinterpret_cast<std::uintptr_t>(wglGetCurrentDC())==identity.dc,
         "Party textures require the existing GL context already current");
    return context;
}

template<class A,class B>bool same_control(const A& a,const B& b){
    return !a.owner_before(b)&&!b.owner_before(a);
}
bool same(const runtime::FileIdentity& a,const runtime::FileIdentity& b){
    return a.bytes==b.bytes && a.sha256==b.sha256;
}
void selected_boundary(const FrontendSelectedSaveInput& selected,const assets::CivisResourceBankOwner& bank){
    bank.require_boundary();selected.require_files_current();
    const auto recipe=bank.source_recipe();
    need(std::filesystem::canonical(selected.save_path())==std::filesystem::canonical(recipe.save_path),
         "Alpha LAND selected save differs from actual Bank");
    need(same(selected.save_identity(),runtime::identify_file(recipe.save_path)),
         "Alpha LAND selected save bytes differ from actual Bank");
    selected.require_files_current();bank.require_boundary();
}
struct TemporaryFramebuffer final {
    const graphics::GlRenderApi& gl;
    GLuint id=0;
    explicit TemporaryFramebuffer(const graphics::GlRenderApi& dispatch):gl(dispatch){gl.CreateFramebuffers(1,&id);}
    ~TemporaryFramebuffer(){if(id)gl.DeleteFramebuffers(1,&id);}
    TemporaryFramebuffer(const TemporaryFramebuffer&)=delete;
};
class ClearState final {
    const graphics::GlRenderApi& gl_;
    std::array<GLboolean,4> mask_{};
    GLboolean scissor_=GL_FALSE;
    static constexpr std::array<GLenum,3> flags_{GL_DITHER,GL_FRAMEBUFFER_SRGB,GL_RASTERIZER_DISCARD};
    std::array<GLboolean,3> enabled_{};
    bool changed_=false;
public:
    explicit ClearState(const graphics::GlRenderApi& gl):gl_(gl){
        gl_.GetBooleani_v(GL_COLOR_WRITEMASK,0,mask_.data());
        scissor_=gl_.IsEnabledi(GL_SCISSOR_TEST,0);
        for(std::size_t i=0;i<flags_.size();++i)enabled_[i]=gl_.IsEnabled(flags_[i]);
    }
    ClearState(const ClearState&)=delete;
    ~ClearState(){restore();}
    void enable_clear(){
        changed_=true; // All later failures still restore exactly the saved state.
        gl_.ColorMaski(0,GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
        gl_.Disablei(GL_SCISSOR_TEST,0);
        for(const auto flag:flags_)gl_.Disable(flag);
    }
    void restore()noexcept{
        if(!changed_)return;
        gl_.ColorMaski(0,mask_[0],mask_[1],mask_[2],mask_[3]);
        if(scissor_)gl_.Enablei(GL_SCISSOR_TEST,0);else gl_.Disablei(GL_SCISSOR_TEST,0);
        for(std::size_t i=0;i<flags_.size();++i)
            if(enabled_[i])gl_.Enable(flags_[i]);else gl_.Disable(flags_[i]);
        changed_=false;
    }
    void verify_restored()const{
        std::array<GLboolean,4> actual{};
        gl_.GetBooleani_v(GL_COLOR_WRITEMASK,0,actual.data());
        need(actual==mask_ && gl_.IsEnabledi(GL_SCISSOR_TEST,0)==scissor_,
             "Alpha LAND indexed clear state was not restored");
        for(std::size_t i=0;i<flags_.size();++i)
            need(gl_.IsEnabled(flags_[i])==enabled_[i],"Alpha LAND global clear state was not restored");
    }
};
}

struct FrontendPartyAlphaLandLayers::State final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true}; // Actual facade destructor witness, not a readiness flag.
    // Texture owners are destroyed first; genuine window/context/Bank last.
    const std::shared_ptr<assets::CivisResourceBankOwner> bank;
    const assets::CivisResourceBankCapability capability;
    const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
    const std::shared_ptr<graphics::GlContext> context;
    const graphics::GlContextNativeIdentity identity;
    const std::filesystem::path save;
    const runtime::FileIdentity save_identity;
    const std::string recipe_identity;
    const graphics::GlTexture overlay;
    const graphics::GlTexture shadow;
    State(std::shared_ptr<assets::CivisResourceBankOwner> b,
          std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> w,
          std::shared_ptr<graphics::GlContext> c,const FrontendSelectedSaveInput& selected)
      :bank(std::move(b)),capability(bank->capability()),window(std::move(w)),context(std::move(c)),
       identity(context->native_identity()),save(std::filesystem::canonical(selected.save_path())),
       save_identity(selected.save_identity()),recipe_identity(bank->source_recipe().recipe_identity),
       overlay(graphics::GlTexture::render_target(already_current(*context),1,1,1,graphics::RenderTextureFormat::rgba8)),
       shadow(graphics::GlTexture::render_target(already_current(*context),4,4,1,graphics::RenderTextureFormat::rgba32_float)){}
    void current()const{
        need(std::this_thread::get_id()==creator,"Alpha LAND belongs to another creator GL thread");
        need(alive.load(),"Alpha LAND owner retired during operation");
        capability.require_current();bank->require_current();
        const auto now=context->native_identity();
        need(now.owner_thread && now.window==identity.window && now.dc==identity.dc && now.context==identity.context,
             "Alpha LAND actual GL identity changed");
        (void)already_current(*context);
        need(overlay.belongs_to(*context) && shadow.belongs_to(*context),"Alpha LAND texture GL owner changed");
    }
    void boundary()const{
        current();bank->require_boundary();current();
        const auto recipe=bank->source_recipe();current();
        need(recipe.recipe_identity==recipe_identity && std::filesystem::canonical(recipe.save_path)==save,
             "Alpha LAND Bank recipe/save changed");
        need(same(save_identity,runtime::identify_file(save)),"Alpha LAND selected save changed");current();
        window->with_actual_window_loan([&](const auto& actual){
            current();
            actual.require_context_control_owned(std::weak_ptr<graphics::GlContext>(context));
            current();
            const auto actual_window=actual.window();current();
            const auto actual_dc=actual.dc();current();
            const auto actual_context=actual.wgl_context();current();
            need(actual_window==identity.window && actual_dc==identity.dc && actual_context==identity.context,
                 "Alpha LAND actual renderer window changed");
            actual.require_current();current();
        });
        current();const auto bank_window=bank->actual_hwnd();current();
        need(bank_window==identity.window,"Alpha LAND Bank HWND changed");
        bank->require_boundary();current();
        need(same(save_identity,runtime::identify_file(save)),"Alpha LAND save changed across boundary");current();
    }
    void clear_and_verify()const{
        current();context->check("Before Alpha LAND physical clear");
        const graphics::GlRenderApi gl(*context);current();
        {
            TemporaryFramebuffer fbo(gl);
            need(fbo.id!=0,"Alpha LAND framebuffer allocation failed");
            context->check("Alpha LAND framebuffer allocation");
            ClearState state(gl);
            context->check("Alpha LAND saved clear state");
            state.enable_clear();context->check("Alpha LAND enabled complete clear");
            const GLenum drawbuffer=GL_COLOR_ATTACHMENT0;
            const std::array<float,4> zero{0.f,0.f,0.f,0.f};
            for(const auto* texture:{&overlay,&shadow}){
                gl.NamedFramebufferTexture(fbo.id,GL_COLOR_ATTACHMENT0,texture->id(),0);
                gl.NamedFramebufferDrawBuffers(fbo.id,1,&drawbuffer);
                need(gl.CheckNamedFramebufferStatus(fbo.id,GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,
                     "Alpha LAND clear framebuffer incomplete");
                context->check("Alpha LAND actual attachment/drawbuffer");
                gl.ClearNamedFramebufferfv(fbo.id,GL_COLOR,0,zero.data());
                context->check("Alpha LAND actual complete zero clear");
            }
            state.restore();state.verify_restored();
            context->check("Alpha LAND exact clear state restoration");
        } // Actual state and temporary FBO destructors before the source POST.
        context->check("Alpha LAND temporary framebuffer retirement");
        const auto verify=[](const graphics::GlTexture& texture,std::size_t size){
            const auto actual=texture.read_level_raw(0,0);
            need(actual.size()==size && std::all_of(actual.begin(),actual.end(),[](auto byte){return byte==0;}),
                 "Alpha LAND clear readback is not complete exact zero");
        };
        // Existing readback binds CPU PACK_BUFFER0 and restores all pack state.
        // This completes/verifies these two storage writes, never a game frame.
        verify(overlay,4);verify(shadow,256);
        context->check("Alpha LAND synchronous zero readback complete");current();
    }
};
FrontendPartyAlphaLandLayers::FrontendPartyAlphaLandLayers(std::shared_ptr<State> state):state_(std::move(state)){}
FrontendPartyAlphaLandLayers::~FrontendPartyAlphaLandLayers(){
    state_->alive.store(false); // FIRST: no retained State can mask facade retirement.
    if(creator_!=std::this_thread::get_id())std::terminate();
}
std::unique_ptr<FrontendPartyAlphaLandLayers> FrontendPartyAlphaLandLayers::capture(
    const detail::CivisFrontendPartyWorldSource& source,
    const std::shared_ptr<graphics::GlContext>& supplied_context,OverlayPolicy overlay,ShadowPolicy shadow){
    need(overlay==OverlayPolicy::OverlayNone && shadow==ShadowPolicy::ShadowDisabled,
         "Alpha LAND needs explicit supported own overlay/shadow policy");
    need(bool(supplied_context),"Alpha LAND needs the actual shared GL owner");
    auto loan=source.begin_authored_material_inputs_owned();
    loan->require_current();
    const std::weak_ptr<graphics::GlContext> expected_context(supplied_context);
    loan->require_context_control_owned(expected_context);
    std::unique_ptr<FrontendPartyAlphaLandLayers> result;
    {
        const auto context=supplied_context;
        loan->require_current();loan->require_context_control_owned(expected_context);
        const auto supplied_bank=loan->bank_owned();loan->require_current();
        auto bank=supplied_bank->retain_owner();
        need(bank.get()==supplied_bank.get() && same_control(bank,supplied_bank),"Alpha LAND Bank control is not canonical");
        const auto supplied_window=loan->renderer_window_owned();loan->require_current();
        const auto& selected=loan->selected_owned();loan->require_current();
        selected_boundary(selected,*bank);
        const auto identity=context->native_identity();
        need(identity.owner_thread && identity.window && identity.dc && identity.context,
             "Alpha LAND needs the real creator GL thread");
        need(bank->actual_hwnd()==identity.window,"Alpha LAND selected Bank uses another HWND");
        std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
        supplied_window->with_actual_window_loan([&](const auto& actual){
            actual.require_context_control_owned(expected_context);
            need(actual.window()==identity.window && actual.dc()==identity.dc && actual.wgl_context()==identity.context,
                 "Alpha LAND uses another renderer context");
            window=actual.retain_owner();actual.require_current();
        });
        need(window.get()==supplied_window.get() && same_control(window,supplied_window),
             "Alpha LAND renderer window control is not canonical");
        loan->require_current();loan->require_context_control_owned(expected_context);
        auto state=std::make_shared<State>(bank,window,context,selected);
        loan->require_current();state->boundary();
        state->clear_and_verify();
        selected_boundary(selected,*bank);state->boundary();
        loan->require_current();loan->require_context_control_owned(expected_context);
        result=std::unique_ptr<FrontendPartyAlphaLandLayers>(new FrontendPartyAlphaLandLayers(std::move(state)));
        result->require_boundary();loan->require_current();
    } // All supplied/canonical local control copies retire inside actual POST.
    loan->require_current();loan->require_context_control_owned(expected_context);
    loan->finish_owned();
    return result;
}
void FrontendPartyAlphaLandLayers::require_current()const{
    need(creator_==std::this_thread::get_id(),"Alpha LAND facade belongs to another creator");
    const auto state=state_;state->current();
}
void FrontendPartyAlphaLandLayers::require_boundary()const{
    need(creator_==std::this_thread::get_id(),"Alpha LAND boundary belongs to another creator");
    // Pin actual storage before the first native/source callback. A callback
    // may destroy this facade; only State is touched afterward, and its alive
    // POST rejects that retirement. No strong facade pin prolongs authority.
    const auto state=state_;state->boundary();
}
const graphics::GlTexture& FrontendPartyAlphaLandLayers::overlay()const{require_current();return state_->overlay;}
const graphics::GlTexture& FrontendPartyAlphaLandLayers::shadow()const{require_current();return state_->shadow;}
void FrontendPartyAlphaLandLayers::apply_to(materials::LandMaterialConstants& constants,
                                         graphics::GlLandMaterialInputs& inputs)const{
    require_current();
    auto updated=constants;
    updated.shadow_fade=0.f;
    updated.shadow_projection={{{1.f,0.f,0.f,0.f},{0.f,1.f,0.f,0.f},
                                {0.f,0.f,1.f,0.f},{0.f,0.f,0.f,1.f}}};
    updated.validate(); // Reject before modifying either caller object.
    constants.shadow_fade=updated.shadow_fade;
    constants.shadow_projection=updated.shadow_projection;
    inputs.pixel[1]=state_->overlay.id();inputs.pixel[3]=state_->shadow.id();
}
}
