#include "frontend_party_authored_lighting.hpp"
#include <stdexcept>
#include <atomic>
#include <thread>
#include <utility>
#include <windows.h>

namespace civ5::ui::detail {
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
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
bool same(const runtime::FileIdentity& a,const runtime::FileIdentity& b){
    return a.bytes==b.bytes && a.sha256==b.sha256;
}
template<class A,class B>bool same_control(const A& a,const B& b){
    return !a.owner_before(b)&&!b.owner_before(a);
}
void selected_boundary(const FrontendSelectedSaveInput& selected,
                       const assets::CivisResourceBankOwner& bank){
    bank.require_boundary();selected.require_files_current();
    const auto recipe=bank.source_recipe();
    need(std::filesystem::canonical(selected.save_path())==std::filesystem::canonical(recipe.save_path),
         "Authored lighting selected save differs from Bank");
    need(same(selected.save_identity(),runtime::identify_file(recipe.save_path)),
         "Authored lighting selected save bytes differ from Bank");
    bank.require_boundary();selected.require_files_current();
}
assets::AssetKey bank_key(const std::string& native_path){
    // Published Bank names are native basenames, including archived assets.
    // Keep authored paths in request metadata; never try another namespace.
    return assets::AssetKey(assets::resource_basename(native_path));
}
// Resolves only the verified Bank memory table. AssetHandle retains its actual
// immutable bytes; no loose-source reconstruction or filesystem load here.
std::optional<assets::AssetHandle> memory_asset(
    const assets::CivisResourceBankOwner& bank,
    const assets::CivisResourceBankCapability& cap,const std::string& name){
    cap.require_current();
    auto hit=bank.resources(cap).resolve(bank_key(name));
    cap.require_current();
    if(const auto* asset=std::get_if<assets::AssetHandle>(&hit.asset))return *asset;
    need(std::holds_alternative<std::monostate>(hit.asset),
         "Authored lighting resource selection is ambiguous");
    return std::nullopt;
}
assets::AssetHandle required_asset(const assets::CivisResourceBankOwner& bank,
    const assets::CivisResourceBankCapability& cap,const std::string& name){
    auto asset=memory_asset(bank,cap,name);
    need(bool(asset),"Authored lighting required resource is missing");
    return std::move(*asset);
}
}

struct FrontendPartyAuthoredLighting::State final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    // Order is deliberate: GL textures retire before context/window/Bank.
    const std::shared_ptr<assets::CivisResourceBankOwner> bank;
    const assets::CivisResourceBankCapability capability;
    const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
    const std::shared_ptr<graphics::GlContext> context;
    const graphics::GlContextNativeIdentity identity;
    const std::filesystem::path save;
    const runtime::FileIdentity save_identity;
    const std::string recipe_identity;
    const materials::LightingCubeAssets lights;
    const materials::DistanceFogAssets fog;
    const materials::ColorKeyAssets keys;
    const graphics::GlLightingCubes gl_lights;
    const graphics::GlDistanceFog gl_fog;
    const std::shared_ptr<const graphics::GlColorKeys> gl_keys;
    // Only capture_with_spec_screen populates these genuine returned owners.
    // Destruction order: texture, decoded data, source bytes, then other GLs.
    std::optional<assets::AssetHandle> spec_asset;
    std::unique_ptr<const assets::DdsData> spec_data;
    std::optional<graphics::DdsSamplingColor> spec_color;
    std::unique_ptr<const graphics::GlTexture> gl_spec;
    State(std::shared_ptr<assets::CivisResourceBankOwner> b,
          std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> w,
          std::shared_ptr<graphics::GlContext> c,const FrontendSelectedSaveInput& selected,
          materials::LightingCubeAssets l,materials::DistanceFogAssets f,
          materials::ColorKeyAssets k)
      :bank(std::move(b)),capability(bank->capability()),window(std::move(w)),context(std::move(c)),
       identity(context->native_identity()),save(std::filesystem::canonical(selected.save_path())),
       save_identity(selected.save_identity()),recipe_identity(bank->source_recipe().recipe_identity),
       lights(std::move(l)),fog(std::move(f)),keys(std::move(k)),
       gl_lights(already_current(*context),lights),gl_fog(already_current(*context),fog),
       gl_keys(std::make_shared<graphics::GlColorKeys>(already_current(*context),keys)){}
    void current()const{
        need(std::this_thread::get_id()==creator,"Authored lighting belongs to another GL thread");
        need(alive.load(),"Authored lighting component retired");
        capability.require_current();bank->require_current();
        const auto now=context->native_identity();
        need(now.owner_thread && now.window==identity.window && now.dc==identity.dc && now.context==identity.context,
             "Authored lighting GL owner changed");
        (void)already_current(*context);
    }
    void boundary()const{
        current();bank->require_boundary();current();
        const auto recipe=bank->source_recipe();current();
        need(recipe.recipe_identity==recipe_identity && std::filesystem::canonical(recipe.save_path)==save,
             "Authored lighting Bank recipe/save changed");
        need(same(save_identity,runtime::identify_file(save)),"Authored lighting selected save changed");
        current();
        window->with_actual_window_loan([this](const auto& actual){
            current();
            // Same producer control; every callback-return rechecks retirement.
            actual.require_context_control_owned(std::weak_ptr<graphics::GlContext>(context));current();
            const auto actual_window=actual.window();current();
            const auto actual_dc=actual.dc();current();
            const auto actual_context=actual.wgl_context();current();
            need(actual_window==identity.window && actual_dc==identity.dc &&
                 actual_context==identity.context,"Authored lighting renderer window changed");
            actual.require_current();current();
        });current(); // Includes callback copy/invoke/target destruction.
        const auto bank_window=bank->actual_hwnd();current();
        need(bank_window==identity.window,"Authored lighting Bank HWND changed");
        bank->require_boundary();current();
        need(same(save_identity,runtime::identify_file(save)),"Authored lighting save changed during boundary");
        current();
    }
};
FrontendPartyAuthoredLighting::FrontendPartyAuthoredLighting(std::shared_ptr<State> state)
    :state_(std::move(state)){}
FrontendPartyAuthoredLighting::~FrontendPartyAuthoredLighting(){
    if(creator_!=std::this_thread::get_id())std::terminate();
    if(state_)state_->alive.store(false); // FIRST: callback pins never hide facade retirement.
}
std::unique_ptr<FrontendPartyAuthoredLighting> FrontendPartyAuthoredLighting::capture(
    const FrontendSelectedSaveInput& selected,
    const std::shared_ptr<assets::CivisResourceBankOwner>& supplied_bank,
    const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& supplied_window,
    const std::shared_ptr<graphics::GlContext>& context){
    need(supplied_bank && supplied_window && context,"Authored lighting needs actual Bank/window/context");
    auto bank=supplied_bank->retain_owner();
    need(bank.get()==supplied_bank.get() && same_control(bank,supplied_bank),
         "Authored lighting Bank control is not canonical");
    selected_boundary(selected,*bank);
    const auto identity=context->native_identity();
    need(identity.owner_thread && identity.window && identity.dc && identity.context,
         "Authored lighting needs the actual creating GL thread");
    need(bank->actual_hwnd()==identity.window,"Authored lighting uses another Bank HWND");
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
    supplied_window->with_actual_window_loan([&](const auto& actual){
        actual.require_context_control_owned(std::weak_ptr<graphics::GlContext>(context));
        need(actual.window()==identity.window && actual.dc()==identity.dc && actual.wgl_context()==identity.context,
             "Authored lighting uses another renderer context");
        window=actual.retain_owner();actual.require_current();
    });
    need(window.get()==supplied_window.get() && same_control(window,supplied_window),
         "Authored lighting window control is not canonical");
    // Explicit IO/staging, outside every real client/render callback. Loading
    // required resources preserves authoritative mount selection and errors.
    for(const auto& request:materials::lighting_requests())
        if(request.loader==materials::LightingLoader::cube)bank->load(bank_key(request.native_path));
    bank->load(bank_key(materials::DistanceFogAssets::color_path));
    bank->load(bank_key(materials::DistanceFogAssets::ramp_path));
    for(const auto& request:materials::color_key_requests()){
        const auto key=bank_key(request.native_path);
        if(bank->has_indexed(key))bank->load(key);
    }
    selected_boundary(selected,*bank);
    const auto cap=bank->capability();
    auto lights=materials::LightingCubeAssets::load([&](const auto& request){
        return required_asset(*bank,cap,request.native_path);
    });
    auto fog=materials::DistanceFogAssets::load([&](const char* name){return required_asset(*bank,cap,name);});
    // Only the existing original missing/empty ColorKey policy is retained;
    // asset/authored flags expose each fallback. Malformed/ambiguous data fails.
    auto keys=materials::ColorKeyAssets::load([&](const auto& request){return memory_asset(*bank,cap,request.native_path);});
    selected_boundary(selected,*bank);
    (void)already_current(*context);
    context->check("Before selected authored lighting uploads");
    auto state=std::make_shared<State>(bank,window,context,selected,std::move(lights),std::move(fog),std::move(keys));
    need(state->gl_fog.ramp().belongs_to(*context),"Authored fog texture belongs to another GL owner");
    for(unsigned style=0;style<5;++style){
        need(state->gl_lights.sunlight(style).belongs_to(*context) &&
             state->gl_lights.shadowlight(style).belongs_to(*context),
             "Authored lighting cubes belong to another GL owner");
        for(unsigned system=0;system<5;++system)
            need(state->gl_keys->at(static_cast<materials::ColorKeySystem>(system),style).belongs_to(*context),
                 "Authored ColorKey texture belongs to another GL owner");
    }
    context->check("After selected authored lighting uploads");
    selected_boundary(selected,*bank);state->boundary();
    return std::unique_ptr<FrontendPartyAuthoredLighting>(new FrontendPartyAuthoredLighting(std::move(state)));
}
std::unique_ptr<FrontendPartyAuthoredLighting> FrontendPartyAuthoredLighting::capture_with_spec_screen(
    const FrontendSelectedSaveInput& selected,
    const std::shared_ptr<assets::CivisResourceBankOwner>& bank,
    const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& window,
    const std::shared_ptr<graphics::GlContext>& context,
    graphics::DdsSamplingColor explicit_color){
    need(explicit_color==graphics::DdsSamplingColor::linear || explicit_color==graphics::DdsSamplingColor::srgb,
         "SpecScreen requires an explicit supported x64 sampling policy");
    // Existing real 36-texture factory is unchanged. Its result is retained
    // privately until the required 37th source has actually loaded/uploaded.
    auto result=capture(selected,bank,window,context);
    auto& s=*result->state_;
    selected_boundary(selected,*s.bank);s.boundary();
    const auto& request=materials::lighting_requests()[0];
    need(request.loader==materials::LightingLoader::texture_data,"SpecScreen authored request route changed");
    s.bank->load(bank_key(request.native_path));
    selected_boundary(selected,*s.bank);s.boundary();
    auto asset=required_asset(*s.bank,s.capability,request.native_path);
    auto data=std::make_unique<const assets::DdsData>(assets::DdsData::parse(asset.read_bytes()));
    selected_boundary(selected,*s.bank);s.boundary();
    s.context->check("Before selected authored SpecScreen upload");
    auto texture=std::make_unique<const graphics::GlTexture>(already_current(*s.context),*data,explicit_color);
    need(texture->belongs_to(*s.context),"SpecScreen texture belongs to another GL owner");
    s.context->check("After selected authored SpecScreen upload");
    // Publish the actual source/data/texture together; no DDS fixture, pixel
    // replacement, dimensions/mips assumption or specscreen binding alias.
    s.spec_asset.emplace(std::move(asset));s.spec_data=std::move(data);
    s.spec_color=explicit_color;s.gl_spec=std::move(texture);
    selected_boundary(selected,*s.bank);s.boundary();
    return result;
}
void FrontendPartyAuthoredLighting::require_current()const{
    need(std::this_thread::get_id()==creator_,"Authored lighting belongs to another GL thread");
    state_->current(); // Read-only owner/WGL checks; no callback can retire this facade here.
}
void FrontendPartyAuthoredLighting::require_boundary()const{
    need(std::this_thread::get_id()==creator_,"Authored lighting belongs to another GL thread");
    const auto state=state_; // Pin actual storage before any source/native callback.
    state->boundary(); // State only: no facade access after callback or its destructor.
}
const materials::LightingCubeAssets& FrontendPartyAuthoredLighting::lighting_assets()const{require_current();return state_->lights;}
const materials::DistanceFogAssets& FrontendPartyAuthoredLighting::fog_assets()const{require_current();return state_->fog;}
const materials::ColorKeyAssets& FrontendPartyAuthoredLighting::color_key_assets()const{require_current();return state_->keys;}
const graphics::GlLightingCubes& FrontendPartyAuthoredLighting::lighting_cubes()const{require_current();return state_->gl_lights;}
const graphics::GlDistanceFog& FrontendPartyAuthoredLighting::distance_fog()const{require_current();return state_->gl_fog;}
const graphics::GlColorKeys& FrontendPartyAuthoredLighting::color_keys()const{require_current();return *state_->gl_keys;}
std::shared_ptr<const graphics::GlColorKeys> FrontendPartyAuthoredLighting::color_keys_owned()const{
    require_current();
    auto owner=state_->gl_keys;
    require_current();
    return owner;
}
std::array<GLuint,5> FrontendPartyAuthoredLighting::color_correct()const{require_current();return state_->gl_keys->color_correct();}
const graphics::GlTexture& FrontendPartyAuthoredLighting::spec_screen()const{
    require_current();need(bool(state_->gl_spec),"SpecScreen was not requested by the old authored lighting factory");
    return *state_->gl_spec;
}
const assets::AssetHandle& FrontendPartyAuthoredLighting::spec_screen_asset()const{
    (void)spec_screen();return *state_->spec_asset;
}
const assets::DdsData& FrontendPartyAuthoredLighting::spec_screen_data()const{
    (void)spec_screen();return *state_->spec_data;
}
graphics::DdsSamplingColor FrontendPartyAuthoredLighting::spec_screen_sampling_color()const{
    (void)spec_screen();return *state_->spec_color;
}
}
