#include "civis_frontend_party_bank_handoff.hpp"
#include "civis_selected_bnw_rules_preparation.hpp"
#include "civ5/ui/frontend_content.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/ui/civis_resource_bank_ui_snapshot.hpp"
#include "civ5/ui/civis_resource_bank_font_color_snapshot.hpp"
#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include <atomic>
#include <stdexcept>
#include <thread>
#include <utility>

namespace civ5::ui::detail {
namespace {
void need(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
bool same(const runtime::FileIdentity& a,const runtime::FileIdentity& b) {
    return a.bytes==b.bytes && a.sha256==b.sha256;
}
template<class T> std::shared_ptr<T> canonical(const std::shared_ptr<T>& supplied) {
    need(bool(supplied),"Handoff owner is absent");
    auto actual=supplied->retain_owner();
    need(actual.get()==supplied.get() && !actual.owner_before(supplied) &&
         !supplied.owner_before(actual),"Handoff owner control differs from its factory");
    return actual;
}
}

struct CivisFrontendPartyBankHandoff::State final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    bool active=false,attempted=false;
    const FrontendSelectedSaveInput selected;
    const std::shared_ptr<assets::CivisResourceBankOwner> bank;
    const std::shared_ptr<const CivisResourceBankUiSnapshot> ui;
    const std::shared_ptr<const CivisResourceBankFontColorSnapshot> fonts;
    const std::shared_ptr<BnwWorldViewFontSourceScope> scope;
    std::shared_ptr<const CivisSelectedBnwRulesPreparation> rules;
    State(FrontendSelectedSaveInput&& input,
        std::shared_ptr<assets::CivisResourceBankOwner> b,
        std::shared_ptr<const CivisResourceBankUiSnapshot> u,
        std::shared_ptr<const CivisResourceBankFontColorSnapshot> f,
        std::shared_ptr<BnwWorldViewFontSourceScope> s)
        :selected(std::move(input)),bank(std::move(b)),ui(std::move(u)),
         fonts(std::move(f)),scope(std::move(s)){}
    void first()const {
        need(creator==std::this_thread::get_id(),"Handoff belongs to another creator");
        need(alive.load() && !failed.load(),"Handoff retired or failed");
    }
    void tuple_current()const {
        first();bank->require_current();first();ui->require_same_bank(*bank);first();
        fonts->require_same_bank(*bank);first();fonts->require_same_ui(*ui);first();
        if(rules){rules->require_selected_owned(*bank,selected);first();}
    }
    void full()const {
        first();bank->require_boundary();first();tuple_current();
        selected.require_files_current();first();tuple_current();
    }
    void selected_package(const content::Catalog& expected,
        const assets::CivisResourceBankSourceRecipe& recipe,
        std::size_t selected_index,std::size_t bank_index,content::PackageKind kind)const {
        first();const auto& a=expected.packages.at(selected_index);
        const auto& b=recipe.catalog.packages.at(bank_index);
        // DLC IDs use the GUID wire contract. Real mod IDs may be non-RFC;
        // use the same ASCII normalization as frontend package_key/resolver.
        need(a.kind==kind && b.kind==kind &&
             (kind==content::PackageKind::dlc
                ? content::canonical_guid(a.id)==content::canonical_guid(b.id)
                : runtime::ascii_lower(a.id)==runtime::ascii_lower(b.id)) && a.version==b.version,
             "Handoff selected package identity/order differs");
        // A manifest's declared package root is its Catalog row's parent,
        // which may differ from the canonical file's parent (e.g. symlinks).
        // Both actual source locations must match, not only manifest bytes.
        const auto selected_file=std::filesystem::canonical(a.path);
        const auto bank_file=std::filesystem::canonical(b.path);
        const auto selected_root=std::filesystem::canonical(std::filesystem::absolute(a.path).parent_path());
        const auto bank_root=std::filesystem::canonical(std::filesystem::absolute(b.path).parent_path());
        first();
        if(selected_file!=bank_file || selected_root!=bank_root) {
            // Bank capture explicitly mirrors original/relative to
            // canonical_payload_root/relative, then stores that mirror row.
            // Only this SAME recipe mapping admits an original Catalog row.
            const auto game_root=std::filesystem::canonical(recipe.game_root);
            const auto payload_root=std::filesystem::canonical(recipe.canonical_payload_root);
            const auto relative_file=selected_file.lexically_relative(game_root);
            const auto relative_root=selected_root.lexically_relative(game_root);
            const auto contained=[](const std::filesystem::path& value){
                if(value.empty() || value.is_absolute() || value.has_root_name())return false;
                for(const auto& part:value)if(part=="..")return false;
                return true;
            };
            need(contained(relative_file) && contained(relative_root),
                 "Handoff selected package source is outside the actual Bank recipe");
            need(std::filesystem::canonical(payload_root/relative_file)==bank_file &&
                 std::filesystem::canonical(payload_root/relative_root)==bank_root,
                 "Handoff selected package source location differs from Bank");
            first();
        }
        const auto left=runtime::identify_file(a.path);first();
        const auto right=runtime::identify_file(b.path);first();
        need(same(left,right),"Handoff selected manifest bytes differ from Bank");
    }
    assets::CivisResourceBankSourceRecipe matching_recipe()const {
        full();auto recipe=bank->source_recipe();first();tuple_current();
        need(std::filesystem::canonical(selected.save_path())==
             std::filesystem::canonical(recipe.save_path),"Handoff save path differs from Bank");
        need(same(selected.save_identity(),runtime::identify_file(recipe.save_path)),
             "Handoff save bytes differ from selected input");first();
        const auto& p=selected.selected_profile();const auto& q=recipe.profile;
        need(p.kind==q.kind && p.all_requirements_located==q.all_requirements_located &&
             p.files_complete==q.files_complete && p.native_mods_require_port==q.native_mods_require_port &&
             p.can_activate==q.can_activate && p.enabled_dlc.size()==q.enabled_dlc.size() &&
             p.enabled_mods.size()==q.enabled_mods.size(),"Handoff selected profile differs from Bank");
        // Indices belong to their respective catalogs. Join actual selected
        // package keys, ordered occurrences and manifest bytes, not ordinals
        // accidentally equal across independently discovered catalogs.
        for(std::size_t i=0;i<p.enabled_dlc.size();++i)
            selected_package(selected.selected_catalog(),recipe,
                p.enabled_dlc[i],q.enabled_dlc[i],content::PackageKind::dlc);
        for(std::size_t i=0;i<p.enabled_mods.size();++i)
            selected_package(selected.selected_catalog(),recipe,
                p.enabled_mods[i],q.enabled_mods[i],content::PackageKind::mod);
        // Full POST observes actual held/named Bank sources and menu input
        // again after filesystem reads, before any launch supplier is used.
        full();return recipe;
    }
};

CivisFrontendPartyBankHandoff::CivisFrontendPartyBankHandoff(std::shared_ptr<State> s)
    :state_(std::move(s)){}
CivisFrontendPartyBankHandoff::~CivisFrontendPartyBankHandoff() {
    if(state_)state_->alive.store(false);
}
std::unique_ptr<CivisFrontendPartyBankHandoff> CivisFrontendPartyBankHandoff::capture(
    FrontendSelectedSaveInput&& selected,
    const std::shared_ptr<assets::CivisResourceBankOwner>& supplied_bank,
    const std::shared_ptr<const CivisResourceBankUiSnapshot>& supplied_ui,
    const std::shared_ptr<const CivisResourceBankFontColorSnapshot>& supplied_fonts,
    const std::shared_ptr<BnwWorldViewFontSourceScope>& supplied_scope,
    std::shared_ptr<const CivisSelectedBnwRulesPreparation> supplied_rules) {
    // Authenticate/own every producer-created control before the first
    // provider callback. Never promote a local ResourceMountTable or alias.
    auto bank=canonical(supplied_bank);auto ui=canonical(supplied_ui);
    auto fonts=canonical(supplied_fonts);auto scope=canonical(supplied_scope);
    auto s=std::make_shared<State>(std::move(selected),std::move(bank),std::move(ui),
        std::move(fonts),std::move(scope));
    try {
        (void)s->matching_recipe();
        s->scope->require_current();s->first();
        need(&s->scope->resources()==&s->bank->resources(),"Handoff font scope uses another Bank table");s->first();
        if(supplied_rules){
            const auto actual=supplied_rules->retain_owner();s->first();
            need(actual.get()==supplied_rules.get()&&!actual.owner_before(supplied_rules)&&
                 !supplied_rules.owner_before(actual),"Handoff prepared rules canonical control differs");
            s->rules=actual;
        }else{s->rules=CivisSelectedBnwRulesPreparation::capture_owned(s->bank,s->selected);s->first();}
        s->rules->require_selected_owned(*s->bank,s->selected);s->first();
        s->scope->require_current();s->first();
        need(&s->scope->resources()==&s->bank->resources(),"Handoff font scope uses another Bank table");
        s->full();
        return std::unique_ptr<CivisFrontendPartyBankHandoff>(new CivisFrontendPartyBankHandoff(std::move(s)));
    }catch(...){s->failed.store(true);throw;}
}

const FrontendSelectedSaveInput& CivisFrontendPartyBankHandoff::selected_world_input_owned(const assets::CivisResourceBankOwner& expected)const {
    const auto s=state_;s->first();
    need(s->bank.get()==&expected,"Saved world handoff belongs to another Bank");
    (void)s->matching_recipe();s->first();return s->selected;
}

std::shared_ptr<const CivisSelectedBnwRulesPreparation> CivisFrontendPartyBankHandoff::selected_rules_owned(
    const assets::CivisResourceBankOwner& expected)const{
    const auto s=state_;s->first();need(s->bank.get()==&expected&&s->rules,"Saved rules handoff has another Bank or no preparation");
    (void)s->matching_recipe();s->first();auto result=s->rules->retain_owner();s->first();s->full();return result;
}

BnwLivePreviewOutcome CivisFrontendPartyBankHandoff::execute(
    std::int64_t active_player,const std::filesystem::path& supplied_scene,
    const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& supplied_window,
    const runtime::BnwStockLuaClientExtentProvider& supplied_client,
    const BnwObservedCivisStockUiSourceCameraRender& supplied_render,
    const graphics::BnwRendererSceneReceipt& supplied_receipt,
    const BnwPrepareRendererScene& supplied_prepare,
    const std::function<void(const SelectedBnwBootstrapResult&)>& supplied_observe,
    const std::shared_ptr<runtime::BnwCivisApplicationStateOwner>& supplied_application) {
    // A callback may destroy this facade. Local State keeps storage safe;
    // alive=false remains a refusal and is checked before continuing.
    const auto s=state_;s->first();
    if(s->active || s->attempted){s->failed.store(true);throw std::runtime_error("Handoff launch repeated or reentered");}
    s->active=true;s->attempted=true;
    struct Operation final {
        std::shared_ptr<State> state;bool returned=false;
        ~Operation(){if(!returned)state->failed.store(true);state->active=false;}
    } operation{s};
    BnwLivePreviewOutcome outcome;
    {
        // Copies are owned before callbackful boundaries, and retire BEFORE
        // final full POST. Their destructors cannot silently escape bookends.
        const auto scene=supplied_scene;const auto receipt=supplied_receipt;
        const auto window=supplied_window;auto client=supplied_client;
        const auto render=supplied_render;const auto prepare=supplied_prepare;
        const auto observe=supplied_observe;
        need(window && client.current && render && prepare,"Handoff renderer sources are absent");
        const auto recipe=s->matching_recipe();s->scope->require_current();s->first();
        need(&s->scope->resources()==&s->bank->resources(),"Handoff font scope uses another Bank table");
        window->with_actual_window_loan([&](const auto& loan){
            s->first();const auto actual=loan.retain_owner();s->first();
            need(actual.get()==window.get() && !actual.owner_before(window) && !window.owner_before(actual),
                 "Handoff renderer Window control differs");
            const auto hwnd=loan.window();s->first();
            need(hwnd==s->bank->actual_hwnd(),"Handoff renderer Window differs from Bank input");s->first();
            loan.require_current();s->first();
        });s->full();
        const auto original_client=client.current;
        client.current=[s,original_client]{s->first();auto value=original_client();s->first();return value;};
        BnwObservedCivisStockUiSourceCameraRender guarded_render=
            [s,render](auto input,const auto& preview,auto stock,auto scope){
                s->first();render(std::move(input),preview,std::move(stock),std::move(scope));s->first();
            };
        BnwPrepareRendererScene guarded_prepare=[s,prepare](const auto* input,auto& value){
            s->first();const auto result=prepare(input,value);s->first();return result;
        };
        BnwLivePreviewRequest request{s->selected.save_path(),recipe.game_root,active_player,{}, {},supplied_application};
        if(observe)request.observe_selected_profile=[s,observe](const auto& value){s->first();observe(value);s->first();};
        // This is the real existing launch, with all its guards, scene
        // reconciliation, source creation, refusal and callback effects intact.
        outcome=launch_selected_bnw_camera_preview_from_civis_resources_with_stock_renderer_with_observed_ui_source(
            request,scene,s->bank,s->ui,s->fonts,s->scope,window,client,
            guarded_render,&receipt,guarded_prepare,s->rules);
        s->first();
    }
    // Camera may have retired scope. Check retained selection/Bank/UI/fonts,
    // never renew that scope or synthesize a performed result from metadata.
    s->full();
    if(outcome.blocker || !outcome.rendered)s->failed.store(true);
    operation.returned=true;return outcome;
}
std::unique_ptr<CivisFrontendPartyBankHandoff> CivisFrontendPartyBankHandoff::select(
    const FrontendCatalogContentAdapter& content,const FrontendRequest& request,
    const std::shared_ptr<assets::CivisResourceBankOwner>& bank,
    const std::shared_ptr<const CivisResourceBankUiSnapshot>& ui,
    const std::shared_ptr<const CivisResourceBankFontColorSnapshot>& fonts,
    const std::shared_ptr<BnwWorldViewFontSourceScope>& scope,
    std::shared_ptr<const CivisSelectedBnwRulesPreparation> rules) {
    auto selected=content.prepare_selected_save_input(request);
    return capture(std::move(selected),bank,ui,fonts,scope,std::move(rules));
}
}
