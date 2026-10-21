#include "civ5/runtime/bnw_fresh_game_content.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include <algorithm>
#include <exception>
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/lua_script_host.hpp"

namespace civ5::runtime {
namespace {
constexpr std::string_view expansion2_id = "6da07636-4123-4018-b643-6575b4ec336b";

void add(BnwFreshGameContentResult &result, FreshGameContentBlockerCode code,
         std::string message) {
    result.blockers.push_back({code, std::move(message)});
}

bool selects_expansion2(const BnwFreshGameContentResult &result,
                        const content::Catalog &catalog) {
    return std::any_of(result.profile.enabled_dlc.begin(), result.profile.enabled_dlc.end(),
                       [&](std::size_t index) {
                           const auto &package = catalog.packages.at(index);
                           return package.kind == content::PackageKind::dlc &&
                                  content::canonical_guid(package.id) == expansion2_id;
                       });
}
} // namespace

bool BnwFreshGameContentResult::has_blocker(FreshGameContentBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshGameContentResult::initialize_actual_rules_identity(const content::RulesDatabase& actual) {
    activated_rules_identity_=actual.capture_actual_impl_identity();
}

bool BnwFreshGameContentResult::content_ready() const {
    return rules && activation && activation->rules_database_ready &&
           !has_blocker(FreshGameContentBlockerCode::expansion2_not_selected) &&
           !has_blocker(FreshGameContentBlockerCode::content_profile_incomplete) &&
           !has_blocker(FreshGameContentBlockerCode::native_gamecore_unavailable) &&
           !has_blocker(FreshGameContentBlockerCode::rules_activation_failed) &&
           !has_blocker(FreshGameContentBlockerCode::rules_database_incomplete);
}

BnwFreshGameContentResult bootstrap_bnw_fresh_game_content(
    const std::filesystem::path &game_root, const content::Catalog &catalog,
    std::span<const std::size_t> selected_dlc,
    std::span<const std::size_t> selected_mods) {
    BnwFreshGameContentResult result;
    result.profile = resolve_selected_content_profile(selected_dlc, selected_mods, catalog);

    if (!selects_expansion2(result, catalog))
        add(result, FreshGameContentBlockerCode::expansion2_not_selected,
            "Brave New World requires the selected Expansion2 DLC package");
    if (!result.profile.all_requirements_located || !result.profile.files_complete)
        add(result, FreshGameContentBlockerCode::content_profile_incomplete,
            "Selected DLC or mod dependencies and declared files are incomplete");
    if (result.profile.native_mods_require_port)
        add(result, FreshGameContentBlockerCode::native_gamecore_unavailable,
            "A selected mod declares a native GameCore DLL without a compatible x64 profile");

    // A content-only boundary cannot attest to later session stages.
    add(result, FreshGameContentBlockerCode::lua_loading_pending,
        "Lua entrypoints must run in selected contexts and complete_loading must succeed");
    add(result, FreshGameContentBlockerCode::map_script_pending,
        "The selected map script has not generated and validated a map");
    add(result, FreshGameContentBlockerCode::world_setup_pending,
        "Players, teams, cities, units, and turn state have not been created");

    if (!result.profile.can_activate ||
        result.has_blocker(FreshGameContentBlockerCode::expansion2_not_selected))
        return result;

    try {
        const auto actual_root=std::filesystem::canonical(game_root);
        auto mounts = content::build_selected_loose_resource_plan(
            game_root, result.profile.enabled_dlc, catalog);
        auto rules = std::make_shared<content::RulesDatabase>();
        result.activation = content::activate_saved_rules(
            *rules, game_root, result.profile, catalog, mounts.resources);
        if (!result.activation->rules_database_ready) {
            add(result, FreshGameContentBlockerCode::rules_database_incomplete,
                "Base, selected DLC, localization, or mod rule actions did not all succeed");
            return result;
        }
        if(std::filesystem::canonical(game_root)!=actual_root)
            throw std::runtime_error("BNW activation game root changed during real rules activation");
        result.initialize_actual_rules_identity(*rules);
        result.activated_game_root_=actual_root;result.activated_rules_=rules.get();
        result.activated_rules_owner_=rules;
        result.rules = std::move(rules);
    } catch (const std::exception &error) {
        add(result, FreshGameContentBlockerCode::rules_activation_failed,
            std::string("Cannot activate selected BNW rules: ") + error.what());
    }
    return result;
}


namespace {
bool same_xml(const content::XmlNode& a,const content::XmlNode& b){
    if(a.name!=b.name||a.text!=b.text||a.attributes!=b.attributes||a.children.size()!=b.children.size())return false;
    for(std::size_t i=0;i<a.children.size();++i)if(!same_xml(a.children[i],b.children[i]))return false;return true;
}
bool same_package(const content::PackageManifest& a,const content::PackageManifest& b){
    if(a.kind!=b.kind||a.path.lexically_normal()!=b.path.lexically_normal()||a.id!=b.id||a.name!=b.name||a.version!=b.version||
       a.files.size()!=b.files.size()||!same_xml(a.document,b.document))return false;
    for(std::size_t i=0;i<a.files.size();++i)if(a.files[i].path!=b.files[i].path||a.files[i].md5!=b.files[i].md5||a.files[i].import!=b.files[i].import)return false;
    return true;
}
bool same_catalog(const content::Catalog& a,const content::Catalog& b){
    if(a.packages.size()!=b.packages.size()||a.diagnostics!=b.diagnostics)return false;
    for(std::size_t i=0;i<a.packages.size();++i)if(!same_package(a.packages[i],b.packages[i]))return false;return true;
}
bool same_profile(const SavedContentProfile& a,const SavedContentProfile& b){
    if(a.kind!=b.kind||a.enabled_dlc!=b.enabled_dlc||a.enabled_mods!=b.enabled_mods||
       a.all_requirements_located!=b.all_requirements_located||a.files_complete!=b.files_complete||
       a.native_mods_require_port!=b.native_mods_require_port||a.can_activate!=b.can_activate||a.requirements.size()!=b.requirements.size())return false;
    for(std::size_t i=0;i<a.requirements.size();++i){const auto& x=a.requirements[i];const auto& y=b.requirements[i];
        if(x.kind!=y.kind||x.id!=y.id||x.name!=y.name||x.wire_value!=y.wire_value||x.status!=y.status||x.package!=y.package||x.candidates!=y.candidates)return false;}return true;
}
}
BnwFreshGameContentResult::~BnwFreshGameContentResult(){if(epoch_)epoch_->retired.store(true);}
BnwFreshGameContentResult::BnwFreshGameContentResult(BnwFreshGameContentResult&& other)noexcept {
    if(other.epoch_)other.epoch_->retired.store(true);
    profile=std::move(other.profile);rules=std::move(other.rules);activation=std::move(other.activation);
    blockers=std::move(other.blockers);epoch_=std::move(other.epoch_);owner_thread_=other.owner_thread_;
    activated_game_root_=std::move(other.activated_game_root_);
    activated_rules_=other.activated_rules_;
    activated_rules_identity_=std::move(other.activated_rules_identity_);
    activated_rules_owner_=std::move(other.activated_rules_owner_);
    other.activated_game_root_.reset();other.activated_rules_=nullptr;
    other.activated_rules_identity_={};other.activated_rules_owner_.reset();
}
BnwFreshGameContentResult& BnwFreshGameContentResult::operator=(BnwFreshGameContentResult&& other)noexcept {
    if(this==&other)return *this;
    if(epoch_)epoch_->retired.store(true);if(other.epoch_)other.epoch_->retired.store(true);
    profile=std::move(other.profile);rules=std::move(other.rules);activation=std::move(other.activation);
    blockers=std::move(other.blockers);epoch_=std::move(other.epoch_);owner_thread_=other.owner_thread_;
    activated_game_root_=std::move(other.activated_game_root_);
    activated_rules_=other.activated_rules_;
    activated_rules_identity_=std::move(other.activated_rules_identity_);
    activated_rules_owner_=std::move(other.activated_rules_owner_);
    other.activated_game_root_.reset();other.activated_rules_=nullptr;
    other.activated_rules_identity_={};other.activated_rules_owner_.reset();return *this;
}
BnwFreshGameContentResult::ContentEpoch::ContentEpoch(
    const std::shared_ptr<BnwFreshGameContentResult>& actual,std::filesystem::path root,content::Catalog recipe,
    std::vector<std::pair<std::filesystem::path,std::pair<std::uint64_t,std::string>>> pins)
    :thread(actual->owner_thread_),installation(std::move(root)),catalog(std::move(recipe)),canonical(actual),manifest_pins(std::move(pins)){
    view.profile=actual->profile;view.rules=actual->rules;view.activation=actual->activation;view.blockers=actual->blockers;
    view.activated_game_root_=actual->activated_game_root_;
    view.activated_rules_=actual->activated_rules_;
    view.activated_rules_identity_=actual->activated_rules_identity_;
    view.activated_rules_owner_=actual->activated_rules_owner_;
    // view.epoch_ stays empty: an internal stable view is not another issuer.
}
void BnwFreshGameContentResult::ContentEpoch::require_current()const {
    if(thread!=std::this_thread::get_id())
        throw ScriptError("Immutable preview rules scope already sealed on another thread; content epoch refuses before callback");
    if(retired.load())throw ScriptError("Selected content factory epoch retired");
    try{
        // This is the genuine factory control, locked before any facade borrow.
        const auto actual=canonical.lock();
        if(!actual||actual->epoch_.get()!=this||actual->rules!=view.rules||!actual->content_ready()||
           !same_profile(actual->profile,view.profile)||!view.rules||!view.content_ready())
            throw ScriptError("Selected content factory owner/rules/profile changed");
        const auto receipt_matches=[](const BnwFreshGameContentResult& value){
            return value.rules&&value.activated_rules_owner_&&
                value.rules.get()==value.activated_rules_&&
                value.rules.get()==value.activated_rules_owner_.get()&&
                !value.rules.owner_before(value.activated_rules_owner_)&&
                !value.activated_rules_owner_.owner_before(value.rules)&&
                value.activated_rules_identity_&&value.activated_rules_identity_(*value.rules);
        };
        if(actual->rules.owner_before(view.rules)||view.rules.owner_before(actual->rules)||
           !receipt_matches(*actual)||!receipt_matches(view)||
           actual->activated_game_root_!=view.activated_game_root_||
           !view.activated_game_root_||*view.activated_game_root_!=installation)
            throw ScriptError("Selected content actual rules facade/control/activation receipt changed");
        if(std::filesystem::canonical(installation)!=installation)
            throw ScriptError("Selected content factory installation changed");
        for(const auto& [path,before]:manifest_pins){const auto now=identify_file(path);
            if(now.bytes!=before.first||now.sha256!=before.second)throw ScriptError("Selected content factory manifest bytes changed");}
        if(!view.rules->immutable_preview_fingerprints_current())
            throw ScriptError("Selected content factory immutable actual rules changed");
    }catch(...){retired.store(true);throw;}
}
void BnwFreshGameContentResult::ContentEpoch::require_recipe(
    const std::filesystem::path& root,const content::Catalog& supplied)const {
    require_current();
    if(std::filesystem::canonical(root)!=installation||!same_catalog(supplied,catalog))
        throw ScriptError("Selected content factory installation/Catalog recipe differs");
}
std::shared_ptr<const BnwFreshGameContentResult> BnwFreshGameContentResult::retain_factory_owner(
    const std::shared_ptr<const BnwFreshGameContentResult>& supplied){
    if(!supplied)throw ScriptError("Selected content factory owner absent");
    if(supplied->owner_thread_!=std::this_thread::get_id())
        throw ScriptError("Immutable preview rules scope already sealed on another thread; content admission before callback");
    const auto epoch=supplied->epoch_;
    if(!epoch)throw ScriptError("Selected content requires genuine factory-issued epoch");
    // Authenticate the actual factory block BEFORE currentness/provider work.
    const auto actual=epoch->canonical.lock();
    if(!actual||actual.get()!=supplied.get()||actual.owner_before(supplied)||supplied.owner_before(actual))
        throw ScriptError("Selected content requires exact factory owning controlblock");
    epoch->require_current();
    if(actual->epoch_.get()!=epoch.get()||epoch->retired.load())
        throw ScriptError("Selected content factory epoch retired during admission");
    return actual;
}
std::shared_ptr<BnwFreshGameContentResult::ContentEpoch> BnwFreshGameContentResult::retain_epoch(
    const std::shared_ptr<const BnwFreshGameContentResult>& supplied){
    const auto actual=retain_factory_owner(supplied);
    return actual->epoch_;
}
std::shared_ptr<BnwFreshGameContentResult> bootstrap_bnw_fresh_game_content_owner(
    const std::filesystem::path& game_root,const content::Catalog& catalog,
    std::span<const std::size_t> dlc,std::span<const std::size_t> mods){
    const auto root=std::filesystem::canonical(game_root);
    // Validate the actual selected manifest recipe BEFORE rule activation.
    std::vector<std::pair<std::filesystem::path,FileIdentity>> manifests;
    const auto validate=[&](std::span<const std::size_t> selected){for(const auto index:selected){
        const auto& supplied=catalog.packages.at(index);const auto path=std::filesystem::canonical(supplied.path);
        if(supplied.kind==content::PackageKind::dlc){
            const auto relative=path.lexically_relative(root);
            if(relative.empty()||relative.is_absolute()||*relative.begin()=="..")
                throw ScriptError("Selected DLC manifest is outside actual activation installation");
        }
        const auto before_parse=identify_file(path);
        const auto actual=content::read_manifest(supplied.path);
        const auto after_parse=identify_file(path);
        if(before_parse.bytes!=after_parse.bytes||before_parse.sha256!=after_parse.sha256)
            throw ScriptError("Selected manifest changed during actual parse");
        if(!same_package(actual,supplied))throw ScriptError("Actual selected Catalog differs from manifest recipe");
        // Carry this SAME pre-parse/pre-activation identity into the epoch.
        manifests.emplace_back(path,before_parse);}};
    validate(dlc);validate(mods);
    auto result=std::make_shared<BnwFreshGameContentResult>(bootstrap_bnw_fresh_game_content(root,catalog,dlc,mods));
    if(!result->content_ready())return result; // Failed result has no epoch authority.
    for(const auto& [path,before]:manifests){const auto now=identify_file(path);
        if(now.bytes!=before.bytes||now.sha256!=before.sha256)throw ScriptError("Selected manifest changed during activation");}
    result->rules->seal_immutable_preview_fingerprints();
    std::vector<std::pair<std::filesystem::path,std::pair<std::uint64_t,std::string>>> pins;
    for(const auto& [path,identity]:manifests)pins.push_back({path,{identity.bytes,identity.sha256}});
    auto epoch=std::make_shared<BnwFreshGameContentResult::ContentEpoch>(result,root,catalog,std::move(pins));
    result->epoch_=epoch;epoch->require_current();return result;
}

} // namespace civ5::runtime
