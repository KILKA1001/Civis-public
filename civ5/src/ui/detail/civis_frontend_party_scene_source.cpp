#include "civis_frontend_party_scene_source.hpp"
#include "civis_selected_bnw_rules_preparation.hpp"
#include "civ5/ui/frontend_content.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/ui/civis_resource_bank_ui_snapshot.hpp"
#include "civ5/ui/civis_resource_bank_font_color_snapshot.hpp"
#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/terrain/terrain_source_rivers.hpp"
#include <array>
#include <atomic>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>

namespace civ5::ui::detail {
namespace {
void need(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
bool same(const runtime::FileIdentity& a,const runtime::FileIdentity& b){
    return a.bytes==b.bytes && a.sha256==b.sha256;
}
// Same authored required roots as the existing saved-world Bank route.
const std::array<assets::CivisRequiredResource,8>& required_roots(){
    using P=assets::CivisResourceParse;using S=assets::CivisResourceSelection;
    static const std::array<assets::CivisRequiredResource,8> value{{
        {"Assets/UI/InGame/InGame.xml",P::xml,S::winner},
        {"Assets/UI/InGame/WorldView/WorldView.xml",P::xml,S::winner},
        {"Assets/UI/InGame/WorldView/NotificationPanel.xml",P::xml,S::winner},
        {"Assets/UI/Fonts/Tw Cent MT/TwCenMT14.ggxml",P::xml,S::winner},
        {"Assets/UI/Fonts/Tw Cent MT/TwCenMT16.ggxml",P::xml,S::winner},
        {"Assets/UI/Fonts/Tw Cent MT/TwCenMT18.ggxml",P::xml,S::winner},
        {"Assets/UI/Fonts/Tw Cent MT/TwCenMT20.ggxml",P::xml,S::winner},
        {"Assets/UI/Fonts/Tw Cent MT/TwCenMT24.ggxml",P::xml,S::winner}
    }};return value;
}
}
struct CivisFrontendPartySceneSource::State final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    bool preparing=false,prepare_attempted=false;
    const std::shared_ptr<assets::CivisResourceBankOwner> bank;
    const std::shared_ptr<const BnwNativeEarlyInputLease> input;
    const std::shared_ptr<const materials::MaterialWorld> world;
    const std::shared_ptr<const CivisSelectedBnwRulesPreparation> rules;
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
    std::filesystem::path save;
    runtime::FileIdentity identity;
    runtime::BnwStockLuaClientExtentProvider client;
    saves::SavedMapProjection projection;
    saves::SavedMapTeamArtObservation art;
    std::optional<graphics::BnwRendererSceneReceipt> receipt;
    std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner> language;
    std::shared_ptr<const CivisResourceBankUiSnapshot> ui;
    std::shared_ptr<const CivisResourceBankFontColorSnapshot> fonts;
    std::shared_ptr<BnwWorldViewFontSourceScope> scope;
    std::unique_ptr<CivisFrontendPartyBankHandoff> leaf;
    State(std::shared_ptr<assets::CivisResourceBankOwner> b,
        std::shared_ptr<const BnwNativeEarlyInputLease> i,
        std::shared_ptr<const materials::MaterialWorld> w,
        std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> r,
        std::shared_ptr<const CivisSelectedBnwRulesPreparation> p)
        :bank(std::move(b)),input(std::move(i)),world(std::move(w)),rules(std::move(p)),window(std::move(r)){}
    void first()const{
        need(creator==std::this_thread::get_id(),"Scene source belongs to another creator");
        need(alive.load()&&!failed.load(),"Scene source retired or failed");
    }
    void full()const{
        first();bank->require_boundary();first();
        input->require_current();first();window->require_current();first();
        rules->require_bank_owned(*bank);first();
        if(!save.empty())need(same(identity,runtime::identify_file(save)),"Scene save changed");
        first();bank->require_current();first();
        if(language){language->require_current();first();}
    }
    void decode_and_match(const FrontendSelectedSaveInput& selected){
        full();
        saves::SavedSessionDecodeContext context;
        context.decode_map_regions=true;context.decode_teams=true;
        context.decode_all_players=true;context.decode_supporting_classes=true;
        context.all_players=saves::AllPlayersContext{};
        context.terrain_features=rules->terrain_features_owned(*bank,selected);first();full();
        {
            const auto container=saves::read_loaded_save(save);first();
            if(container.container==saves::SaveContainerKind::civis &&
               graphics::bnw_treasury_presentation_digest(saves::read_civis_save(save))){
                // Same immutable-art decode policy as the existing renderer.
                // LiveSession must independently restore the real treasury pair.
                context.permit_live_treasury_overlay=true;
                context.permit_live_bnw_treasury_balance_overlay=true;
            }
        }
        const auto saved=saves::read_saved_session(save,context);first();
        const auto& snapshot=saved.snapshot;const auto& map=snapshot.map_header;
        const auto& source=world->terrain().source();
        need(snapshot.pregame && snapshot.map_plots && snapshot.map_terrain && snapshot.map_projection,
             "Scene decode lacks actual map/setup observations");
        need(source.map_mode==terrain::game_world_grid_mode(map.wrap_x) && source.map_mode==1 &&
             source.layout.source_grid.width>0 && source.layout.source_grid.height>0 &&
             map.grid.width==static_cast<std::uint64_t>(source.layout.source_grid.width) &&
             map.grid.height==static_cast<std::uint64_t>(source.layout.source_grid.height),
             "Scene decode differs from retained MaterialWorld layout");
        need(map.grid.height<=std::numeric_limits<std::size_t>::max()-4 &&
             map.grid.width<=std::numeric_limits<std::size_t>::max()/8/(map.grid.height+4),
             "Scene expanded grid extent overflows");
        const auto expected=static_cast<std::size_t>(map.grid.width*(map.grid.height+4)*8);
        const auto& grid=source.initial_cells;
        need(grid.size()==expected && snapshot.map_terrain->cells.size()==map.grid.size(),
             "Scene grid/decoded terrain extent differs");
        std::size_t verified=0;
        for(std::uint64_t x=0;x<map.grid.width;++x)for(std::uint64_t y=0;y<map.grid.height;++y){
            const auto& cell=snapshot.map_terrain->cells.at(static_cast<std::size_t>(x*map.grid.height+y));
            if(!cell.native_terrain_mask_known)continue;
            const auto at=static_cast<std::size_t>((x*(map.grid.height+4)+y+2)*8);
            const auto mask=std::uint32_t(grid.at(at)) | std::uint32_t(grid.at(at+1))<<8 |
                            std::uint32_t(grid.at(at+2))<<16 | std::uint32_t(grid.at(at+3))<<24;
            need(mask==cell.native_terrain_mask,"Scene terrain mask differs from decoded save");++verified;
        }
        need(verified>0,"Scene has no independently classified terrain cells");
        std::vector<terrain::TerrainRiverPlot> plots;plots.reserve(snapshot.map_terrain->cells.size());
        for(const auto& cell:snapshot.map_terrain->cells){
            need(cell.rivers.has_value(),"Scene river plot is unresolved");const auto& r=*cell.rivers;
            plots.push_back({r.is_river,r.west,r.northwest,r.northeast,r.flow_e,r.flow_se,r.flow_sw});
        }
        const auto edges=terrain::build_terrain_river_edges(source.layout.source_grid,plots);
        const auto& rivers=world->terrain().rivers();
        need(rivers && rivers->generated && rivers->generated->input_edges==edges,
             "Retained MaterialWorld did not consume this save's real river edges");
        art=saves::observe_saved_map_team_art(*snapshot.pregame,*snapshot.map_plots);
        projection=*snapshot.map_projection;
        receipt=graphics::capture_bnw_renderer_scene_receipt(save,identity.sha256,saved,art,
            BnwSceneOverlayPolicy::treasury_presentation);
        need(receipt.has_value(),"Actual saved-scene receipt creation refused");
        full(); // Caller repeats FULL after these decoded temporaries retire.
    }
};
CivisFrontendPartySceneSource::CivisFrontendPartySceneSource(std::shared_ptr<State> state)
    :state_(std::move(state)){}
CivisFrontendPartySceneSource::~CivisFrontendPartySceneSource(){
    if(state_){state_->alive.store(false);state_->leaf.reset();}
}
std::unique_ptr<CivisFrontendPartySceneSource> CivisFrontendPartySceneSource::select(
    const FrontendCatalogContentAdapter& content,const FrontendRequest& request,
    const std::shared_ptr<assets::CivisResourceBankOwner>& supplied_bank,
    const std::shared_ptr<const materials::MaterialWorld>& world,
    const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& supplied_window,
    const std::shared_ptr<const CivisSelectedBnwRulesPreparation>& supplied_rules){
    need(supplied_bank && world && supplied_window && supplied_rules,"Scene source requires actual Bank/World/Window/rules");
    const auto rules=supplied_rules->retain_owner();
    need(rules.get()==supplied_rules.get()&&!rules.owner_before(supplied_rules)&&!supplied_rules.owner_before(rules),
         "Scene prepared rules control differs");
    const auto bank=supplied_bank->retain_owner();
    need(bank.get()==supplied_bank.get()&&!bank.owner_before(supplied_bank)&&!supplied_bank.owner_before(bank),
         "Scene source Bank is not its canonical owner");
    const auto input=bank->frontend_party_input_owned();
    auto s=std::make_shared<State>(bank,input,world,supplied_window,rules);
    try{
        auto selected=content.prepare_selected_save_input(request);s->first();
        s->save=selected.save_path();s->identity=selected.save_identity();
        // Retain actual typed Window control inside its genuine lexical loan.
        supplied_window->with_actual_window_loan([&](const auto& loan){
            s->first();const auto actual=loan.retain_owner();s->first();
            need(actual.get()==supplied_window.get()&&!actual.owner_before(supplied_window)&&
                 !supplied_window.owner_before(actual),"Scene source Window control differs");
            const auto hwnd=loan.window();s->first();
            need(hwnd==input->actual_hwnd(),"Scene source Window differs from existing Bank input");s->first();
            s->window=actual;loan.require_current();s->first();
        });s->full();
        need(std::filesystem::canonical(s->save)==std::filesystem::canonical(input->save_path()),
             "Scene selected save differs from existing Bank input");s->first();
        // Copy the actual ALREADY-owned provider. No acquisition, synthesized
        // HWND/revision callback, new epoch or Original source renewal.
        s->client=input->client_provider();s->first();s->full();
        need(bool(s->client.current),"Existing Bank input client provider is absent");
        s->decode_and_match(selected);s->full();
        // Retain SAME Bank cache/storage. Demand existing required XML through
        // the actual Bank before capturing the existing data-only owners.
        for(const auto& resource:required_roots()){
            bank->load(assets::AssetKey(resource.name),resource.parse);s->first();s->full();
        }
        s->language=runtime::BnwSelectedLanguageInputOwner::capture_from_civis_resources(bank);s->first();s->full();
        s->ui=CivisResourceBankUiSnapshot::capture(bank);s->first();s->full();
        s->fonts=CivisResourceBankFontColorSnapshot::capture(bank,s->ui);s->first();s->full();
        const auto language=s->language->language();s->first();
        s->scope=BnwWorldViewFontSourceScope::create_from_civis_resources(bank,s->ui,s->fonts,language);
        s->first();s->full();
        s->leaf=CivisFrontendPartyBankHandoff::capture(std::move(selected),bank,s->ui,s->fonts,s->scope,rules);
        s->first();s->full();
        return std::unique_ptr<CivisFrontendPartySceneSource>(new CivisFrontendPartySceneSource(std::move(s)));
    }catch(...){s->failed.store(true);throw;}
}
CivisFrontendPartyBankHandoff& CivisFrontendPartySceneSource::handoff()const{
    const auto s=state_;s->first();need(bool(s->leaf),"Scene handoff retired");return *s->leaf;
}
const std::filesystem::path& CivisFrontendPartySceneSource::scene_save()const{state_->first();return state_->save;}
std::int64_t CivisFrontendPartySceneSource::active_player()const{
    state_->first();need(state_->art.active_player<=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()),
        "Scene active player overflows");return static_cast<std::int64_t>(state_->art.active_player);
}
const graphics::BnwRendererSceneReceipt& CivisFrontendPartySceneSource::scene_receipt()const{
    state_->first();need(state_->receipt.has_value(),"Scene receipt is absent");return *state_->receipt;
}
const saves::SavedMapProjection& CivisFrontendPartySceneSource::projection()const{state_->first();return state_->projection;}
const saves::SavedMapTeamArtObservation& CivisFrontendPartySceneSource::team_art()const{state_->first();return state_->art;}
const std::shared_ptr<const materials::MaterialWorld>& CivisFrontendPartySceneSource::material_world()const{
    state_->first();return state_->world;
}
const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& CivisFrontendPartySceneSource::renderer_window()const{
    state_->first();return state_->window;
}
const runtime::BnwStockLuaClientExtentProvider& CivisFrontendPartySceneSource::client_provider()const{
    state_->first();return state_->client;
}
BnwPrepareRendererScene CivisFrontendPartySceneSource::prepare_scene()const{
    const auto s=state_;s->first();const std::weak_ptr<State> weak=s;
    return [weak](const LiveBnwPreviewInput* input,graphics::BnwRendererSceneReceipt& receipt){
        const auto current=weak.lock();need(bool(current),"Scene callback source expired");current->first();
        if(current->preparing || current->prepare_attempted){current->failed.store(true);
            throw std::runtime_error("Scene reconciliation repeated or reentered");}
        current->preparing=true;current->prepare_attempted=true;
        struct End{State& state;bool returned=false;~End(){if(!returned)state.failed.store(true);state.preparing=false;}} end{*current};
        need(input && current->receipt && receipt.scene_file_sha256==current->receipt->scene_file_sha256 &&
             receipt.restored_payload_sha256==current->receipt->restored_payload_sha256 &&
             receipt.map_guid==current->receipt->map_guid,"Scene reconciliation source revision differs");
        // Actual existing body: reconcile restored cities/projection/art under
        // the launcher's genuine Preview borrower, then retain returned data.
        if(!graphics::reconcile_bnw_renderer_scene_projection(current->projection,receipt,input->session.state())){
            current->failed.store(true);return false;
        }
        current->first();current->receipt=receipt;current->first();end.returned=true;return true;
    };
}
}
