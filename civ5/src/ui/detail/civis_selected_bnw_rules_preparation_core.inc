// Include once in civ5::ui after the existing Civis rules/Host producer.
namespace detail {
namespace {
void prepared_need(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
bool prepared_same_file(const runtime::FileIdentity& a,const runtime::FileIdentity& b){
    return a.bytes==b.bytes&&a.sha256==b.sha256;
}
}
struct CivisSelectedBnwRulesPreparation::State final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    std::weak_ptr<const CivisSelectedBnwRulesPreparation> self;
    std::optional<CivisBankHostBootstrapInputs> actual;
    runtime::FileIdentity selected_save;
    saves::SaveHeader header;
    saves::SaveContainerKind container=saves::SaveContainerKind::civ5;
    std::unique_ptr<content::RulesDatabase> activated_database;
    std::optional<content::LooseResourcePlan> rules_mounts;
    SelectedBnwBootstrapResult metadata;
    BnwSourceDecodeContract source;
    std::shared_ptr<SelectedBnwPreviewHost> host;
    std::shared_ptr<content::RulesDatabase> final_database;
    saves::SavedTerrainFeatureResolver features;
    std::string final_fingerprint;
    bool restoring=false,attempted=false;
    void first()const{
        prepared_need(creator==std::this_thread::get_id(),"Prepared Civis rules belong to another creator");
        prepared_need(alive.load()&&!failed.load()&&!self.expired(),"Prepared Civis rules retired or quarantined");
    }
    void source_current()const{
        first();prepared_need(actual.has_value(),"Prepared Civis rules lack actual Bank inputs");
        actual->current();first();
        const auto canonical=actual->bank->retain_owner();first();
        prepared_need(canonical.get()==actual->bank.get()&&!canonical.owner_before(actual->bank)&&
            !actual->bank.owner_before(canonical),"Prepared Civis rules Bank control changed");
        prepared_need(prepared_same_file(selected_save,runtime::identify_file(actual->recipe.save_path)),
            "Prepared Civis rules selected file changed");first();
        actual->current();first();
    }
    void full()const{
        source_current();
        prepared_need(activated_database&&rules_mounts&&final_database&&features&&(host||attempted)&&
            !final_fingerprint.empty(),"Prepared Civis rules are partial");
        prepared_need(final_database->immutable_preview_fingerprints_current()&&
            activated_database->immutable_preview_fingerprints_current()&&
            final_database->gameplay_fingerprint_sha256()==final_fingerprint&&
            activated_database->gameplay_fingerprint_sha256()==final_fingerprint,
            "Prepared Civis final rules lineage changed");first();
        if(host)prepared_need(host->saved_database_thread_==creator&&host->database_.get()==final_database.get()&&
            !host->database_.owner_before(final_database)&&!final_database.owner_before(host->database_)&&
            host->types_.report().all_succeeded(),"Prepared Civis Host/cache control changed");
        prepared_need(source.actual_early_pregame_locale!=nullptr,"Prepared Civis rules lost actual early Locale");
        source.actual_early_pregame_locale->require_current();first();
        if(host&&host->civis_resource_bank_){host->require_same_civis_resources(*actual->bank);first();}
        source_current();
    }
    void selected(const assets::CivisResourceBankOwner& bank,const FrontendSelectedSaveInput& input)const{
        full();prepared_need(actual->bank.get()==&bank,"Prepared Civis rules use another Bank");
        input.require_files_current();first();
        prepared_need(prepared_same_file(input.save_identity(),selected_save)&&input.loaded_save().header==header&&
            input.loaded_save().container==container&&
            std::filesystem::canonical(input.save_path())==std::filesystem::canonical(actual->recipe.save_path),
            "Prepared Civis rules use another selected save/header/container");first();
        const auto& profile=input.selected_profile();
        prepared_need(profile.kind==actual->recipe.profile.kind&&profile.enabled_mods.empty()&&
            profile.enabled_dlc.size()==actual->recipe.profile.enabled_dlc.size()&&profile.can_activate&&
            profile.all_requirements_located&&!profile.native_mods_require_port,
            "Prepared Civis rules selected profile differs");
        // Catalog indices belong to different catalogs. Compare the actual
        // ordered DLC identities/versions; Handoff also joins source locations.
        for(std::size_t n=0;n<profile.enabled_dlc.size();++n){
            const auto& a=input.selected_catalog().packages.at(profile.enabled_dlc[n]);
            const auto& b=actual->recipe.catalog.packages.at(actual->recipe.profile.enabled_dlc[n]);
            prepared_need(a.kind==content::PackageKind::dlc&&b.kind==a.kind&&a.version==b.version&&
                content::canonical_guid(a.id)==content::canonical_guid(b.id),
                "Prepared Civis rules selected DLC identity/order differs");
        }
        full();input.require_files_current();first();
    }
};
CivisSelectedBnwRulesPreparation::CivisSelectedBnwRulesPreparation(std::shared_ptr<State> s):state_(std::move(s)){}
CivisSelectedBnwRulesPreparation::~CivisSelectedBnwRulesPreparation(){
    if(std::this_thread::get_id()!=creator_)std::terminate();state_->alive.store(false);
}
std::shared_ptr<const CivisSelectedBnwRulesPreparation> CivisSelectedBnwRulesPreparation::capture_owned(
    const std::shared_ptr<assets::CivisResourceBankOwner>& supplied,const FrontendSelectedSaveInput& selected){
    prepared_need(bool(supplied),"Prepared Civis rules need actual Bank");
    const auto bank=supplied->retain_owner();
    prepared_need(bank.get()==supplied.get()&&!bank.owner_before(supplied)&&!supplied.owner_before(bank),
        "Prepared Civis rules need canonical Bank control");
    selected.require_files_current();bank->require_boundary();
    auto s=std::make_shared<State>();
    auto result=std::shared_ptr<CivisSelectedBnwRulesPreparation>(new CivisSelectedBnwRulesPreparation(s));s->self=result;
    try{
        s->first();s->selected_save=selected.save_identity();s->first();
        s->header=selected.loaded_save().header;s->first();s->container=selected.loaded_save().container;
        // Preserve the existing Civis supplier's exact cold art demand/order.
        for(const auto* key:{"Assets/Units/Civ5ArtDefines_UnitMembers.xml",
                            "Assets/Units/Civ5ArtDefines_Units.xml",
                            "Assets/Units/Civ5ArtDefines_UnitMemberTemplates.xml"}){
            bank->load(assets::AssetKey(key),assets::CivisResourceParse::xml);s->first();
        }
        s->actual.emplace(bank);s->first();s->source_current();
        CivisRulesPreparationSink sink{*result};
        {
            std::string producer_fingerprint;
            auto prepared=bootstrap_selected_bnw_preview_impl(s->actual->recipe.save_path,
                s->actual->recipe.game_root,std::nullopt,true,{},&producer_fingerprint,nullptr,&*s->actual,false,&sink);
            if(prepared.preview.blocker)throw std::runtime_error(prepared.preview.blocker->reason);
            s->first();prepared_need(!producer_fingerprint.empty()&&producer_fingerprint==s->final_fingerprint,
                "Prepared Civis producer final fingerprint differs");s->full();
        } // All producer temporaries retire before the fresh complete POST.
        s->full();s->selected(*bank,selected);s->full();return result;
    }catch(...){
        const auto error=std::current_exception();s->failed.store(true);result.reset();
        s->host.reset();s->features={};s->final_database.reset();s->source.actual_early_pregame_locale.reset();
        s->rules_mounts.reset();s->activated_database.reset();
        try{bank->require_boundary();}catch(...){}std::rethrow_exception(error);
    }
}
SelectedBnwBootstrapResult CivisSelectedBnwRulesPreparation::adopt_activated_owned(
    std::unique_ptr<content::RulesDatabase>&& database,std::optional<content::LooseResourcePlan>&& mounts,
    SelectedBnwBootstrapResult&& metadata,BnwSourceDecodeContract&& source,
    const runtime::FileIdentity& selected_save,const saves::SaveHeader& header){
    prepared_need(std::this_thread::get_id()==creator_,"Prepared Civis adoption foreign creator");
    const auto s=state_;s->source_current();
    const auto fail=[&](SelectedBnwPreviewStage stage,std::string reason){
        metadata.preview=blocked(stage,std::move(reason));return std::move(metadata);
    };
    prepared_need(!s->host&&database&&mounts&&metadata.activation&&metadata.postprocess&&
        prepared_same_file(s->selected_save,selected_save)&&s->header==header,
        "Prepared Civis producer output differs from actual selected input");
    std::string reason;
    if(!activation_complete(*metadata.activation,reason))return fail(SelectedBnwPreviewStage::activation,std::move(reason));
    if(!metadata.postprocess->all_succeeded||!metadata.postprocess->transaction_released||
       !metadata.postprocess->strategic_resources_applied||!metadata.postprocess->luxury_resources_applied)
        return fail(SelectedBnwPreviewStage::postprocess,"selected BNW gameplay postprocessing is incomplete");
    std::optional<runtime::FileIdentity> actual_gamecore;
    if(!source_complete(source,reason,actual_gamecore))return fail(SelectedBnwPreviewStage::source_contract,std::move(reason));
    s->source_current();
    auto host=SelectedBnwPreviewHost::prepare_rules_owned(*database,std::move(actual_gamecore),true,
        [s]{s->source_current();});
    if(!host){metadata.preview=std::move(host);return std::move(metadata);}
    s->first();
    s->host=std::move(host.host);s->first();
    s->final_database=s->host->database_;s->first();
    s->final_fingerprint=database->gameplay_fingerprint_sha256();s->source_current();
    prepared_need(s->host->database_->gameplay_fingerprint_sha256()==s->final_fingerprint,
        "Prepared Civis actual final Host clone differs");
    s->features=content::saved_terrain_features(*s->host->database_,s->host->types_).resolve;s->source_current();
    // The old activated temporary now remains as an OWN immutable preparation
    // output. Seal its actual Impl after all producer writes and Locale cloning.
    database->seal_immutable_preview_fingerprints();s->source_current();
    s->activated_database=std::move(database);s->rules_mounts=std::move(mounts);
    s->metadata=std::move(metadata);s->source=std::move(source);s->first();s->full();
    return {}; // The private preparation output, not a published preview Host.
}
std::shared_ptr<const CivisSelectedBnwRulesPreparation> CivisSelectedBnwRulesPreparation::retain_owner()const{
    prepared_need(std::this_thread::get_id()==creator_,"Prepared Civis owner foreign creator");
    const auto s=state_;s->first();const auto actual=s->self.lock();
    prepared_need(actual&&actual.get()==this&&!actual.owner_before(s->self)&&!s->self.owner_before(actual),
        "Prepared Civis canonical factory control retired");s->first();return actual;
}
void CivisSelectedBnwRulesPreparation::require_bank_owned(const assets::CivisResourceBankOwner& bank)const{
    prepared_need(std::this_thread::get_id()==creator_,"Prepared Civis read foreign creator");
    const auto s=state_;s->full();prepared_need(s->actual->bank.get()==&bank,"Prepared Civis read has another Bank");s->full();
}
void CivisSelectedBnwRulesPreparation::require_selected_owned(const assets::CivisResourceBankOwner& bank,
    const FrontendSelectedSaveInput& selected)const{
    prepared_need(std::this_thread::get_id()==creator_,"Prepared Civis selected read foreign creator");
    const auto s=state_;s->selected(bank,selected);
}
saves::SavedTerrainFeatureResolver CivisSelectedBnwRulesPreparation::terrain_features_owned(
    const assets::CivisResourceBankOwner& bank,const FrontendSelectedSaveInput& selected)const{
    prepared_need(std::this_thread::get_id()==creator_,"Prepared Civis features foreign creator");
    const auto s=state_;s->selected(bank,selected);auto result=s->features;s->first();s->selected(bank,selected);return result;
}
SelectedBnwBootstrapResult CivisSelectedBnwRulesPreparation::restore_host_owned(
    const std::shared_ptr<assets::CivisResourceBankOwner>& supplied,std::int64_t active_player)const{
    prepared_need(std::this_thread::get_id()==creator_,"Prepared Civis restore foreign creator");
    const auto s=state_;s->first();
    prepared_need(supplied!=nullptr,"Prepared Civis restore Bank absent");
    const auto bank=supplied->retain_owner();s->first();
    prepared_need(bank.get()==supplied.get()&&!bank.owner_before(supplied)&&!supplied.owner_before(bank)&&
        bank.get()==s->actual->bank.get()&&!bank.owner_before(s->actual->bank)&&!s->actual->bank.owner_before(bank),
        "Prepared Civis restore Bank control differs");
    s->full();
    if(s->restoring||s->attempted){s->failed.store(true);throw std::runtime_error("Prepared Civis Host restore repeated or reentered");}
    s->restoring=true;s->attempted=true;
    struct End{State& s;bool returned=false;~End(){if(!returned)s.failed.store(true);s.restoring=false;}}end{*s};
    try{
        SelectedBnwBootstrapResult result;
        // Reports are copyable; preview is deliberately move-only and stays
        // in the single prepared Host control until the actual restore.
        result.profile=s->metadata.profile;s->first();
        result.activation=s->metadata.activation;s->first();
        result.postprocess=s->metadata.postprocess;s->first();
        result.indexed_resources=s->metadata.indexed_resources;
        result.gamecore_sha256=s->metadata.gamecore_sha256;s->first();s->full();
        {
            auto features=s->features;s->first();s->full();
            result.preview=SelectedBnwPreviewHost::restore_selected_owned(s->actual->recipe.save_path,
                *s->activated_database,*s->metadata.activation,*s->metadata.postprocess,s->source,
                active_player,true,s->host,std::move(features));
        } // Copy/target destructors retire before fresh complete source POST.
        if(!result.preview){
            const auto reason=result.preview.blocker?result.preview.blocker->reason:"Prepared Civis restore returned no Host";
            // Retire the failed private restore target before source POST.
            // Keep only the exact final rules Impl/feature snapshot needed by
            // the already-produced saved-world source; restore remains closed.
            s->host.reset();
            try{s->full();}catch(...){s->failed.store(true);throw std::runtime_error(reason);}
            end.returned=true;return result;
        }
        s->first();s->full();
        result.original_source_file_verified=s->container==saves::SaveContainerKind::civ5;
        auto& host=*result.preview.host;
        prepared_need(result.preview.host.get()==s->host.get()&&!result.preview.host.owner_before(s->host)&&
            !s->host.owner_before(result.preview.host),"Prepared Civis Host clone/control was replaced");
        host.civis_resource_bank_=bank;
        host.civis_art_final_fingerprint_=s->final_fingerprint;
        host.civis_resource_recipe_identity_=s->actual->recipe.recipe_identity;
        host.civis_canonical_self_=result.preview.host;s->first();
        host.require_same_civis_resources(*bank);s->first();host.stage_globals_cache_source_owner();s->first();
        s->full();host.require_same_civis_resources(*bank);s->first();end.returned=true;return result;
    }catch(...){
        const auto error=std::current_exception();s->failed.store(true);s->host.reset();
        try{s->actual->current();}catch(...){}std::rethrow_exception(error);
    }
}
} // namespace detail
