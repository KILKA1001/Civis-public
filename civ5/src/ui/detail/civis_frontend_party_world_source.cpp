#include "civis_frontend_party_world_source.hpp"
#include "civis_selected_bnw_rules_preparation.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include <atomic>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::ui::detail {
namespace {
void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
template<class A,class B>bool same_control(const A& a,const B& b){return !a.owner_before(b)&&!b.owner_before(a);}
}
struct CivisFrontendPartyWorldSource::State final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    std::weak_ptr<const CivisFrontendPartyWorldSource> self;
    const std::shared_ptr<assets::CivisResourceBankOwner> bank;
    const std::shared_ptr<const BnwNativeEarlyInputLease> early;
    const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
    const std::weak_ptr<graphics::GlContext> context;
    const graphics::GlContextNativeIdentity context_identity;
    const std::unique_ptr<CivisFrontendPartyBankHandoff> handoff;
    const FrontendSelectedSaveInput* selected_input=nullptr;
    std::optional<saves::SavedSessionSnapshot> snapshot;
    std::optional<saves::SavedMapTeamArtObservation> art;
    std::shared_ptr<const CivisSavedWorldLoadingOwner> loading_inputs;
    std::vector<terrain::TerrainSourceCell> classified;
    std::vector<terrain::TerrainAuxiliaryPlot> auxiliary;
    std::vector<terrain::TerrainRiverPlot> rivers;
    std::uint64_t generation=0;
    const LoadingInputsLoan* active_loading=nullptr;
    bool validating=false,issuing=false;
    State(std::unique_ptr<CivisFrontendPartyBankHandoff> h,
        std::shared_ptr<assets::CivisResourceBankOwner> b,
        std::shared_ptr<const BnwNativeEarlyInputLease> e,
        std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> w,
        const std::shared_ptr<graphics::GlContext>& c)
      :bank(std::move(b)),early(std::move(e)),window(std::move(w)),context(c),
       context_identity(c->native_identity()),handoff(std::move(h)){}
    void first()const{
        require(std::this_thread::get_id()==creator,"Saved world source belongs to another creator");
        require(alive.load()&&!failed.load()&&!self.expired(),"Saved world source retired or quarantined");
        require(!context.expired(),"Saved world source lost its actual GL owner");
    }
    [[noreturn]]void reentry(){failed.store(true);throw std::runtime_error("Saved world source reentry");}
    void full(){
        first();if(validating)reentry();validating=true;
        struct End{State& s;~End(){s.validating=false;}}end{*this};
        try{
            bank->require_boundary();first();early->require_current();first();
            // Existing Source22 producer checks exact save/profile/package
            // kind/version/order/manifest bytes and physical source locations.
            const auto& selected=handoff->selected_world_input_owned(*bank);first();
            require(!selected_input||selected_input==&selected,"Saved world immutable Input control changed");
            selected_input=&selected;
            require(std::filesystem::canonical(selected.save_path())==std::filesystem::canonical(early->save_path()),
                    "Saved world source differs from actual Bank input");first();
            window->with_actual_window_loan([&](const auto& actual){
                first();actual.require_context_control_owned(context);first();
                require(actual.window()==early->actual_hwnd()&&actual.window()==context_identity.window&&
                    actual.dc()==context_identity.dc&&actual.wgl_context()==context_identity.context&&context_identity.owner_thread,
                    "Saved world source uses another window/context/input");first();actual.require_current();first();
            });first(); // Window callback COPY/invoke/DTOR all retired here.
            selected.require_files_current();first();bank->require_current();first();
        }catch(...){failed.store(true);throw;}
    }
    void decode_owned(){
        full();saves::SavedSessionDecodeContext decode;
        decode.decode_map_regions=true;decode.decode_teams=true;
        decode.decode_all_players=true;decode.decode_supporting_classes=true;
        decode.all_players=saves::AllPlayersContext{};
        {
            const auto& selected=handoff->selected_world_input_owned(*bank);first();
            const auto rules=handoff->selected_rules_owned(*bank);first();
            decode.terrain_features=rules->terrain_features_owned(*bank,selected);first();
        }full(); // Actual preparation/feature-copy temporaries retire before POST.
        {
            const auto& selected=handoff->selected_world_input_owned(*bank);first();
            if(selected.loaded_save().container==saves::SaveContainerKind::civis){
                const auto document=saves::read_civis_save(selected.save_path());first();
                if(graphics::bnw_treasury_presentation_digest(document)){
                    decode.permit_live_treasury_overlay=true;
                    decode.permit_live_bnw_treasury_balance_overlay=true;
                }
            }
        }full(); // Actual document and all IO temporaries retire before POST.
        {
            const auto& selected=handoff->selected_world_input_owned(*bank);first();
            // Decode the actual already-owned LoadedSave, including its exact
            // Original/Civis framing; never reread or invent a default prefix.
            auto decoded=saves::observe_loaded_session(selected.loaded_save(),decode);first();
            require(decoded.pregame&&decoded.map_plots&&decoded.map_terrain&&decoded.map_projection,
                    "Saved world source lacks actual map/setup observations");
            const auto& source=*decoded.map_terrain;const auto& map=decoded.map_header;
            const auto count=source.grid.size();
            require(count>0&&count<=1024*1024&&source.cells.size()==count&&
                map.grid.width==source.grid.width&&map.grid.height==source.grid.height&&
                map.wrap_x==source.wrap_x&&map.wrap_y==source.wrap_y&&
                decoded.map_plots->plots.size()==count&&decoded.map_plots->layout==map.layout,
                "Saved world observations differ from one source grid");
            std::vector<terrain::TerrainSourceCell> cells;
            std::vector<terrain::TerrainAuxiliaryPlot> aux;
            std::vector<terrain::TerrainRiverPlot> river;
            cells.reserve(source.cells.size());aux.reserve(source.cells.size());river.reserve(source.cells.size());
            for(std::uint64_t n=0;n<count;++n){
                const auto& plot=decoded.map_plots->plots.at(static_cast<std::size_t>(n));
                const auto [x,y]=source.grid.coordinates(n);
                require(plot.index==n&&plot.x==x&&plot.y==y,"Saved world plot coordinates differ");
                const auto& cell=source.cells.at(static_cast<std::size_t>(n));
                require(cell.native_terrain_mask_known&&cell.native_category&&cell.rivers&&cell.auxiliary,
                        "Saved world has unresolved terrain/river/auxiliary arguments");
                cells.push_back({cell.native_terrain_mask,*cell.native_category});
                const auto& a=*cell.auxiliary;aux.push_back({a.terrain,a.plot,a.continent,a.feature});
                const auto& r=*cell.rivers;river.push_back({r.is_river,r.west,r.northwest,r.northeast,r.flow_e,r.flow_se,r.flow_sw});
            }
            auto actual_art=saves::observe_saved_map_team_art(*decoded.pregame,*decoded.map_plots);first();
            // No terrain-grid construction yet: coordinate Factory/RCP, CRT
            // cursors, prior/order/fill/styles and append effects remain real
            // later stages. In particular native_markup_complete is untouched.
            snapshot.emplace(std::move(decoded));art.emplace(std::move(actual_art));
            classified.swap(cells);auxiliary.swap(aux);rivers.swap(river);first();
        }full(); // Old vectors/decoded temporaries have actually been destroyed.
    }
};
CivisFrontendPartyWorldSource::CivisFrontendPartyWorldSource(std::shared_ptr<State> s):state_(std::move(s)){}
CivisFrontendPartyWorldSource::~CivisFrontendPartyWorldSource(){
    if(std::this_thread::get_id()!=creator_)std::terminate();state_->alive.store(false);
}
std::shared_ptr<CivisFrontendPartyWorldSource> CivisFrontendPartyWorldSource::capture(
    std::unique_ptr<CivisFrontendPartyBankHandoff>&& handoff,
    const std::shared_ptr<assets::CivisResourceBankOwner>& supplied_bank,
    const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& window,
    const std::shared_ptr<graphics::GlContext>& context){
    require(handoff&&supplied_bank&&window&&context,"Saved world needs actual Input/Bank/Window/context owners");
    const auto bank=supplied_bank->retain_owner();
    require(bank.get()==supplied_bank.get()&&same_control(bank,supplied_bank),"Saved world Bank control is not canonical");
    const auto early=bank->frontend_party_input_owned();early->require_current();
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> canonical_window;
    window->with_actual_window_loan([&](const auto& actual){
        actual.require_context_control_owned(std::weak_ptr<graphics::GlContext>(context));
        canonical_window=actual.retain_owner();actual.require_current();
    });
    require(canonical_window.get()==window.get()&&same_control(canonical_window,window),"Saved world Window control differs");
    auto s=std::make_shared<State>(std::move(handoff),bank,early,std::move(canonical_window),context);
    auto facade=std::shared_ptr<CivisFrontendPartyWorldSource>(new CivisFrontendPartyWorldSource(s));s->self=facade;
    try{
        s->decode_owned();s->full();
        s->loading_inputs=facade->capture_loading_inputs_owned();s->full();
        return facade;
    }catch(...){s->failed.store(true);throw;}
}
void CivisFrontendPartyWorldSource::require_current()const{
    require(std::this_thread::get_id()==creator_,"Saved world source foreign creator");const auto s=state_;s->full();
}
std::shared_ptr<const CivisSelectedBnwRulesPreparation> CivisFrontendPartyWorldSource::selected_rules_owned()const{
    require(std::this_thread::get_id()==creator_,"Saved selected rules foreign creator");
    const auto s=state_;s->full();auto result=s->handoff->selected_rules_owned(*s->bank);s->first();s->full();return result;
}
std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> CivisFrontendPartyWorldSource::renderer_window_owned()const{
    require(std::this_thread::get_id()==creator_,"Saved renderer source foreign creator");
    const auto s=state_;s->full();auto result=s->window;s->first();return result;
}
std::shared_ptr<const CivisSavedWorldLoadingOwner> CivisFrontendPartyWorldSource::loading_inputs_owned()const{
    require(std::this_thread::get_id()==creator_,"Saved loading source foreign creator");
    const auto s=state_;s->full();require(s->loading_inputs!=nullptr,"Actual saved loading child not constructed");auto result=s->loading_inputs;s->first();return result;
}
CivisFrontendPartyWorldSource::LoadingControl::LoadingControl(const std::shared_ptr<State>& s)
    :creator_(s->creator),state_(s),facade_(s->self){}
void CivisFrontendPartyWorldSource::LoadingControl::require_current_owned()const{
    require(std::this_thread::get_id()==creator_,"Saved loading control foreign creator");
    const auto s=state_.lock();require(s&&!facade_.expired(),"Saved loading control retired");
    require(same_control(facade_,s->self),"Saved loading control replaced");s->full();
}
CivisFrontendPartyWorldSource::LoadingInputsLoan::LoadingInputsLoan(std::shared_ptr<State> s)
    :creator_(s->creator),state_(std::move(s)),generation_(++state_->generation){state_->active_loading=this;}
void CivisFrontendPartyWorldSource::LoadingInputsLoan::first()const{
    require(std::this_thread::get_id()==creator_,"Saved loading loan foreign creator");state_->first();
    if(state_->validating){state_->failed.store(true);throw std::runtime_error("Saved loading pure read inside Source callback");}
    require(!returned_&&state_->active_loading==this&&state_->generation==generation_,"Saved loading loan retired or replaced");
}
CivisFrontendPartyWorldSource::LoadingInputsLoan::~LoadingInputsLoan(){
    if(std::this_thread::get_id()!=creator_)std::terminate();
    if(!returned_)state_->failed.store(true);
    if(state_->active_loading==this)state_->active_loading=nullptr;
}
void CivisFrontendPartyWorldSource::LoadingInputsLoan::require_current()const{first();state_->full();first();}
void CivisFrontendPartyWorldSource::LoadingInputsLoan::require_context_control_owned(const std::weak_ptr<graphics::GlContext>& actual)const{
    first();require(!actual.expired()&&same_control(actual,state_->context),"Saved material source has another GL control");require_current();
}
const FrontendSelectedSaveInput& CivisFrontendPartyWorldSource::LoadingInputsLoan::selected_owned()const{first();require(state_->selected_input!=nullptr,"Saved immutable Input was not captured");return *state_->selected_input;}
const std::shared_ptr<assets::CivisResourceBankOwner>& CivisFrontendPartyWorldSource::LoadingInputsLoan::bank_owned()const{first();return state_->bank;}
const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& CivisFrontendPartyWorldSource::LoadingInputsLoan::renderer_window_owned()const{first();return state_->window;}
const saves::SavedSessionSnapshot& CivisFrontendPartyWorldSource::LoadingInputsLoan::snapshot_owned()const{first();return *state_->snapshot;}
const saves::SavedMapTerrainSource& CivisFrontendPartyWorldSource::LoadingInputsLoan::terrain_source_owned()const{first();return *state_->snapshot->map_terrain;}
const saves::SavedMapTeamArtObservation& CivisFrontendPartyWorldSource::LoadingInputsLoan::team_art_owned()const{first();return *state_->art;}
std::span<const terrain::TerrainSourceCell> CivisFrontendPartyWorldSource::LoadingInputsLoan::classified_owned()const{first();return state_->classified;}
std::span<const terrain::TerrainAuxiliaryPlot> CivisFrontendPartyWorldSource::LoadingInputsLoan::auxiliary_plots_owned()const{first();return state_->auxiliary;}
std::span<const terrain::TerrainRiverPlot> CivisFrontendPartyWorldSource::LoadingInputsLoan::river_plots_owned()const{first();return state_->rivers;}
std::shared_ptr<const CivisFrontendPartyWorldSource::LoadingControl> CivisFrontendPartyWorldSource::LoadingInputsLoan::loading_control_owned_without_callbacks()const{
    first();auto result=std::shared_ptr<const LoadingControl>(new LoadingControl(state_));first();return result;
}
std::weak_ptr<const CivisFrontendPartyWorldSource> CivisFrontendPartyWorldSource::LoadingInputsLoan::source_control_owned_without_callbacks()const{first();return state_->self;}
void CivisFrontendPartyWorldSource::LoadingInputsLoan::finish_owned(){require_current();returned_=true;state_->active_loading=nullptr;}
std::unique_ptr<CivisFrontendPartyWorldSource::LoadingInputsLoan> CivisFrontendPartyWorldSource::issue_loading_owned(const std::shared_ptr<State>& s){
    s->first();if(s->active_loading||s->issuing||s->validating)s->reentry();s->issuing=true;
    struct End{State& s;~End(){s.issuing=false;}}end{*s};
    try{
        s->full();require(s->snapshot&&s->art,"Saved loading inputs not produced");
        require(s->generation<std::numeric_limits<std::uint64_t>::max(),"Saved loading occurrence overflow");
        auto result=std::unique_ptr<LoadingInputsLoan>(new LoadingInputsLoan(s));result->require_current();return result;
    }catch(...){s->failed.store(true);throw;}
}
std::unique_ptr<CivisFrontendPartyWorldSource::LoadingInputsLoan> CivisFrontendPartyWorldSource::begin_loading_inputs_owned()const{
    require(std::this_thread::get_id()==creator_,"Saved loading issuer foreign creator");return issue_loading_owned(state_);
}
std::unique_ptr<CivisFrontendPartyWorldSource::LoadingInputsLoan> CivisFrontendPartyWorldSource::begin_authored_material_inputs_owned()const{
    require(std::this_thread::get_id()==creator_,"Saved material issuer foreign creator");return issue_loading_owned(state_);
}
std::unique_ptr<CivisFrontendPartyWorldSource::LoadingInputsLoan> CivisFrontendPartyWorldSource::begin_loading_inputs_from_control_owned(const LoadingControl& actual){
    require(std::this_thread::get_id()==actual.creator_,"Saved loading issuer foreign creator");
    const auto s=actual.state_.lock();require(s&&!actual.facade_.expired()&&same_control(actual.facade_,s->self),"Saved loading issuer source retired or replaced");
    return issue_loading_owned(s); // No strong facade pin can mask owner loss.
}
}

