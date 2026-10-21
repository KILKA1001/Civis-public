#include "civis_frontend_party_material_storage.hpp"
#include "frontend_party_authored_lighting_source26.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/materials/material_setup.hpp"
#include "civ5/terrain/terrain_marsh.hpp"
#include "civis_frontend_alpha_land_frame.hpp"
#include "civ5/ui/frontend_party_alpha_land_layers.hpp"
#include "civis_basic_water_surface.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/materials/fog_of_war_frame.hpp"
#include "civ5/ui/world_plot_pick.hpp"
#include "civ5/runtime/binary_reader.hpp"
#include "../../../validation/material_preview_settings.hpp"
#include <atomic>
#include <exception>
#include <optional>
#include <fstream>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <sstream>
#include <iomanip>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace civ5::ui::detail {
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
// A closed producer for the same Bank. Loads happen only in the synchronous
// loading stage; every returned handle owns the actual winner's immutable
// bytes/provenance. No loose reconstruction, second Bank or fallback provider.
class SelectedTerrainAssets final:public assets::AssetSource {
    const CivisFrontendPartyWorldSource::LoadingInputsLoan& loan_;
public:
    explicit SelectedTerrainAssets(const CivisFrontendPartyWorldSource::LoadingInputsLoan& actual):loan_(actual){}
    assets::AssetLookup find(const assets::AssetKey& authored)const override{
        loan_.require_current();
        need(authored.safe_relative_path(),"Unsafe authored terrain request");
        const assets::AssetKey native(assets::resource_basename(authored.original_name()));
        const auto& bank=loan_.bank_owned();
        if(!bank->has_indexed(native)){loan_.require_current();return {};}
        bank->load(native);loan_.require_current();
        const auto cap=bank->capability();
        auto hit=bank->resources(cap).resolve(native);
        cap.require_current();loan_.require_current();
        return std::move(hit.asset);
    }
    assets::Provenance provenance()const override{
        // Winners can originate from different mounted providers. Reporting
        // one invented provider would discard their actual handle provenance.
        throw std::logic_error("Selected terrain Bank has per-resource provenance");
    }
};
assets::AssetHandle required(const assets::AssetSource& provider,const char* name){
    auto result=provider.find(assets::AssetKey(name));
    auto* actual=std::get_if<assets::AssetHandle>(&result);
    need(actual!=nullptr,"Selected terrain authored resource is missing or ambiguous");
    return std::move(*actual);
}
}
struct CivisFrontendPartyMaterialStorage::State final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    bool checking=false;
    std::shared_ptr<const CivisFrontendPartyWorldSource::LoadingControl> control;
    std::weak_ptr<graphics::GlContext> context;
    const graphics::GlContext* context_identity=nullptr; // Comparison only, never dereferenced.
    // Reverse destruction: GPU objects before world/loading/assets; issuer
    // and Window are still owned by the caller during the complete retirement.
    std::optional<terrain::TerrainPieceLibrary> pieces;
    std::optional<terrain::CoastLibrary> coasts;
    std::optional<assets::AssetHandle> marsh_asset;
    std::optional<terrain::TerrainMarshStamp> marsh_stamp;
    std::shared_ptr<const CivisSavedWorldLoadingOwner::RendererLoadingReturn> loading;
    std::shared_ptr<const materials::MaterialWorld> world;
    std::optional<graphics::GlMaterialWorldLighting> cache_lighting;
    std::unique_ptr<FrontendPartyAuthoredLighting> lighting;
    std::unique_ptr<ui::FrontendPartyAlphaLandLayers> layers;
    std::optional<material_preview_settings::Profile> profile;
    std::optional<materials::FogOfWarAssets> fog_assets;
    std::optional<saves::SavedMapProjection> projection;
    std::optional<saves::SavedMapTeamArtObservation> team_art;
    void first()const{
        need(std::this_thread::get_id()==creator,"Material storage belongs to another creator");
        need(alive.load()&&!failed.load()&&control&&!context.expired(),"Material storage retired or quarantined");
    }
    void full(){
        first();if(checking){failed.store(true);throw std::runtime_error("Material storage reentry");}
        checking=true;struct End{bool& value;~End(){value=false;}}end{checking};
        try{
            control->require_current_owned();first();
            need(loading&&world&&cache_lighting&&lighting&&layers&&profile&&fog_assets&&projection,
                "Material storage has no completed actual producers");
            loading->require_current();first();lighting->require_boundary();first();
            layers->require_boundary();first();
            control->require_current_owned();first();
        }catch(...){failed.store(true);throw;}
    }
};
CivisFrontendPartyMaterialStorage::CivisFrontendPartyMaterialStorage(std::shared_ptr<State> state)
    :state_(std::move(state)){}
CivisFrontendPartyMaterialStorage::~CivisFrontendPartyMaterialStorage(){
    if(std::this_thread::get_id()!=creator_)std::terminate();
    state_->alive.store(false); // Physical facade retirement FIRST, before target release.
}
std::unique_ptr<CivisFrontendPartyMaterialStorage> CivisFrontendPartyMaterialStorage::capture_owned(
    const CivisFrontendPartyWorldSource& source,const std::shared_ptr<graphics::GlContext>& context){
    need(bool(context),"Material storage requires actual GL owner");
    auto loan=source.begin_authored_material_inputs_owned();
    std::shared_ptr<State> state;
    std::shared_ptr<const CivisFrontendPartyWorldSource::LoadingControl> retirement_control;
    try {
    loan->require_context_control_owned(std::weak_ptr<graphics::GlContext>(context));
    state=std::make_shared<State>();loan->require_current();
    state->control=loan->loading_control_owned_without_callbacks();loan->require_current();
    retirement_control=state->control;
    state->context=context;
    state->context_identity=context.get();
    need(loan->snapshot_owned().map_projection.has_value(),"Selected map projection is absent");
    state->projection.emplace(*loan->snapshot_owned().map_projection);loan->require_current();
    state->team_art.emplace(loan->team_art_owned());loan->require_current();
    {
        SelectedTerrainAssets authored(*loan);
        state->pieces.emplace(terrain::TerrainPieceLibrary::load(authored,
            assets::AssetKey("Resource/Common/TerrainPiecesLibrary.fxsb")));loan->require_current();
        state->coasts.emplace(terrain::CoastLibrary::load(authored,assets::AssetKey("coaststyles.xml"),
            terrain::X87Precision::double_precision));loan->require_current();
        auto material=materials::MaterialAssets::load_terrain_provider(required(authored,"TerrainDef.csv"),authored);
        loan->require_current();
        // OWN quality2 diagnostic clouds; actual authored assets, not a
        // generated/default visibility texture. Saved FOW remains the source.
        state->fog_assets.emplace(materials::FogOfWarAssets::load(2,[&](const char* key){return required(authored,key);}));
        loan->require_current();
        const auto child=source.loading_inputs_owned();loan->require_current();
        state->loading=child->capture_renderer_loading_owned(*loan,*state->pieces,material);
        loan->require_current();state->loading->require_current();loan->require_current();
        state->loading->require_same_source_owned(*loan);loan->require_current();
        const auto input=state->loading->input_owned();
        const auto storage=state->loading->storage_owned();
        const auto reciprocal=state->loading->reciprocal_owned();
        const auto order=state->loading->execution_order_owned();
        const auto marsh_sources=state->loading->marsh_sources_owned();
        std::optional<materials::MaterialWorldLoadingMarsh> marsh;
        if(!marsh_sources.empty()){
            state->marsh_asset.emplace(required(authored,"marsh_stamp.dds"));loan->require_current();
            // New OWN x64 loading policy: initialize its entire private stamp
            // buffer explicitly. This does not replace a captured prior byte.
            state->marsh_stamp.emplace(terrain::TerrainMarshStamp::prepare(
                assets::DdsData::parse(state->marsh_asset->read_bytes(65536)),{}));loan->require_current();
            marsh.emplace(materials::MaterialWorldLoadingMarsh{marsh_sources,&*state->marsh_stamp,
                state->loading->precision_owned(),terrain::Vec4Path::sse,{}});
        }
        auto world=materials::MaterialWorld::build(*state->pieces,*state->coasts,std::move(material),
            input,storage,reciprocal,order,{},marsh?&*marsh:nullptr);loan->require_current();
        auto complete=std::move(world).with_rivers(state->loading->saved_scene_owned().river_edges,
            state->loading->river_random_owned(),state->loading->reciprocal_sqrt_owned(),
            state->loading->precision_owned());loan->require_current();
        state->world=std::make_shared<const materials::MaterialWorld>(std::move(complete));loan->require_current();
        const auto own_precision=state->loading->precision_owned();loan->require_current();
        const auto own_rsqrt=state->loading->reciprocal_sqrt_owned();loan->require_current();
        state->cache_lighting.emplace(graphics::GlMaterialWorldLighting::from_terrain(
            materials::terrain_lighting_defaults(own_precision),own_rsqrt));loan->require_current();
    } // producer functions, spans, target copies and child pin retire here.
    loan->require_current();loan->finish_owned();loan.reset();state->control->require_current_owned();
    // Distinct genuine source occurrence. Never open a second loan while the
    // authored CPU loan is active. The SAME canonical Bank/context are used.
    state->lighting=capture_frontend_party_authored_lighting_with_spec_screen(source,context,
        graphics::DdsSamplingColor::linear);state->control->require_current_owned();
    state->layers=ui::FrontendPartyAlphaLandLayers::capture(source,context,
        ui::FrontendPartyAlphaLandLayers::OverlayPolicy::OverlayNone,
        ui::FrontendPartyAlphaLandLayers::ShadowPolicy::ShadowDisabled);
    state->control->require_current_owned();
    state->profile.emplace(material_preview_settings::discover());state->control->require_current_owned();
    state->full();
    auto result=std::unique_ptr<CivisFrontendPartyMaterialStorage>(new CivisFrontendPartyMaterialStorage(std::move(state)));
    result->require_current();return result;
    }catch(...){
        const auto error=std::current_exception();
        // No publication on partial build. Retire the active source loan and
        // every real CPU/GPU target while the caller's binding/context live.
        // An unfinished loan poisons its issuer; the original failure survives.
        loan.reset();state.reset();
        if(retirement_control){try{retirement_control->require_current_owned();}catch(...){}}
        std::rethrow_exception(error);
    }
}
std::shared_ptr<CivisFrontendPartyMaterialStorage::State> CivisFrontendPartyMaterialStorage::retain_current_owned()const{
    need(std::this_thread::get_id()==creator_,"Material storage foreign creator");
    auto state=state_;const auto retirement_control=state->control;
    try{state->full();return state;}catch(...){
        const auto error=std::current_exception();state.reset();
        // A callback may have destroyed the facade. Drop this counted storage
        // pin/actual GPU targets before the issuer POST; no facade pin masks it.
        if(retirement_control){try{retirement_control->require_current_owned();}catch(...){}}
        std::rethrow_exception(error);
    }
}
void CivisFrontendPartyMaterialStorage::require_current()const{(void)retain_current_owned();}
std::shared_ptr<const materials::MaterialWorld> CivisFrontendPartyMaterialStorage::world_owned()const{
    const auto state=retain_current_owned();auto result=state->world;state->first();return result;
}
const FrontendPartyAuthoredLighting& CivisFrontendPartyMaterialStorage::authored_lighting_owned()const{
    const auto state=retain_current_owned();return *state->lighting;
}
const CivisSavedWorldLoadingOwner::RendererLoadingReturn& CivisFrontendPartyMaterialStorage::loading_owned()const{
    const auto state=retain_current_owned();return *state->loading;
}
const material_preview_settings::Profile& CivisFrontendPartyMaterialStorage::profile_owned()const{
    const auto state=retain_current_owned();return *state->profile;
}
const materials::FogOfWarAssets& CivisFrontendPartyMaterialStorage::fog_assets_owned()const{
    const auto state=retain_current_owned();return *state->fog_assets;
}
const materials::LightingCubeAssets& CivisFrontendPartyMaterialStorage::cubes_owned()const{
    const auto state=retain_current_owned();return state->lighting->lighting_assets();
}
graphics::GlMaterialWorldLighting CivisFrontendPartyMaterialStorage::lighting_owned()const{
    const auto state=retain_current_owned();return *state->cache_lighting;
}
graphics::GlLandMaterialInputs CivisFrontendPartyMaterialStorage::land_inputs_owned()const{
    const auto state=retain_current_owned();graphics::GlLandMaterialInputs result{};
    result.pixel[1]=state->layers->overlay().id();result.pixel[3]=state->layers->shadow().id();
    result.pixel[2]=state->lighting->distance_fog().ramp().id();result.pixel[9]=state->lighting->spec_screen().id();
    state->first();return result; // Same held GPU owners, not synthetic handles.
}
std::shared_ptr<const graphics::GlColorKeys> CivisFrontendPartyMaterialStorage::color_keys_owned()const{
    const auto state=retain_current_owned();auto result=state->lighting->color_keys_owned();state->first();return result;
}
const graphics::GlDistanceFog* CivisFrontendPartyMaterialStorage::distance_fog_owned()const{
    const auto state=retain_current_owned();return &state->lighting->distance_fog();
}
void CivisFrontendPartyMaterialStorage::require_selected_owned(
    const FrontendSelectedSaveInput& selected,const graphics::GlContext& context)const{
    auto state=retain_current_owned();const auto retirement_control=state->control;
    std::unique_ptr<CivisFrontendPartyWorldSource::LoadingInputsLoan> actual;
    try{
        actual=CivisFrontendPartyWorldSource::begin_loading_inputs_from_control_owned(*retirement_control);state->first();
        actual->require_context_control_owned(state->context);state->first();
        const auto& expected=actual->selected_owned();
        need(!state->context.expired()&&state->context_identity==&context,"Selected render context is not canonical");
        selected.require_files_current();state->first();actual->require_current();state->first();
        need(std::filesystem::canonical(selected.save_path())==std::filesystem::canonical(expected.save_path())&&
            selected.save_identity().bytes==expected.save_identity().bytes&&
            selected.save_identity().sha256==expected.save_identity().sha256&&
            selected.required_packages()==expected.required_packages(),"Selected render input changed");
        const auto& p=selected.selected_profile();const auto& q=expected.selected_profile();
        need(p.kind==q.kind&&p.all_requirements_located==q.all_requirements_located&&p.files_complete==q.files_complete&&
            p.native_mods_require_port==q.native_mods_require_port&&p.can_activate==q.can_activate&&
            p.enabled_dlc.size()==q.enabled_dlc.size()&&p.enabled_mods.size()==q.enabled_mods.size(),
            "Selected render profile changed");
        const auto package=[&](std::size_t a,std::size_t b,content::PackageKind kind){
            const auto& x=selected.selected_catalog().packages.at(a);
            const auto& y=expected.selected_catalog().packages.at(b);
            const auto key=[&](const auto& v){return kind==content::PackageKind::dlc?
                content::canonical_guid(v.id):runtime::ascii_lower(v.id);};
            need(x.kind==kind&&y.kind==kind&&key(x)==key(y)&&x.version==y.version&&
                std::filesystem::canonical(x.path)==std::filesystem::canonical(y.path)&&
                std::filesystem::canonical(std::filesystem::absolute(x.path).parent_path())==
                std::filesystem::canonical(std::filesystem::absolute(y.path).parent_path()),
                "Selected render package/source occurrence changed");
            actual->require_current();state->first();
        };
        for(std::size_t i=0;i<p.enabled_dlc.size();++i)package(p.enabled_dlc[i],q.enabled_dlc[i],content::PackageKind::dlc);
        for(std::size_t i=0;i<p.enabled_mods.size();++i)package(p.enabled_mods[i],q.enabled_mods[i],content::PackageKind::mod);
        selected.require_files_current();state->first();actual->require_current();state->first();
        actual->finish_owned();state->first();actual.reset();state->first();
        retirement_control->require_current_owned();state->first();
    }catch(...){
        const auto error=std::current_exception();state->failed.store(true);
        actual.reset();state.reset(); // Actual targets before fresh Source POST.
        try{retirement_control->require_current_owned();}catch(...){}
        std::rethrow_exception(error);
    }
}
#include "civis_frontend_alpha_land_frame_core.inc"
}
