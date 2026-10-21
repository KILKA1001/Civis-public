// Inside civ5::ui, after all actual Package/Completed Source types.
// Genuine BNW selected-namespace activation. The typed ModFiles filter is
// bound to this Bank's canonical input; it grants no whole6CD160/game readiness.
struct BnwNativePackageDiscoveryOwner::Impl::BankContextAppSubobject final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const BnwNativePackageDiscoveryOwner> package;
    const std::shared_ptr<const BnwNativeEarlyInputLease> input;
    struct AdditionalNamespace final {
        DefaultState state; //822A50→82A050 actual89=1 and empty native buckets.
        std::string name84; //82A050 empty FString; actual82B3E0 setter below.
    };
    struct SelectedActivation final {
        struct ModRow {std::size_t package;std::string id;std::uint64_t version;bool activated;};
        struct ModFileRow {std::size_t package;std::string requested_path;bool import;};
        const std::vector<std::size_t> selected_dlc,selected_mods;
        const std::vector<ModRow> mods;
        const std::vector<ModFileRow> files;
        const std::vector<std::filesystem::path> selected_imports;
        const std::shared_ptr<AdditionalNamespace> additional;
        SelectedActivation(std::vector<std::size_t> d,std::vector<std::size_t> m,
            std::vector<ModRow> rows,std::vector<ModFileRow> f,
            std::vector<std::filesystem::path> imports,std::shared_ptr<AdditionalNamespace> a)
            :selected_dlc(std::move(d)),selected_mods(std::move(m)),mods(std::move(rows)),
             files(std::move(f)),selected_imports(std::move(imports)),additional(std::move(a)){}
    };
    //507D92 initializes SAME App+ABC subobject +438 (App+EF4) to zero.
    std::shared_ptr<AdditionalNamespace> additional438;
    std::unique_ptr<const SelectedActivation> selected_activation;
    //Own operation quarantine, not an inferred native offset/ready boolean.
    bool activating=false,failed=false;
    BankContextAppSubobject(std::weak_ptr<const BnwNativePackageDiscoveryOwner> p,
        std::shared_ptr<const BnwNativeEarlyInputLease> i):package(std::move(p)),input(std::move(i)){}
};

std::shared_ptr<const detail::BnwNativeOwnedStartupCompletion>
BnwNativePackageDiscoveryOwner::capture_bank_context_lua_include_source_owned(
    const std::shared_ptr<const BnwNativeEarlyInputLease>& supplied,
    const std::function<void()>& raw_bank_bookend){
    BnwPrimaryDefaultStageTrace bank_include_capture_trace("BankInclude.capture"); // BANK_INCLUDE_ENTRY_TRACE
    const auto input=supplied;const auto bank_bookend=raw_bank_bookend;
    need(input&&bank_bookend,"Bank include source needs actual input/bookend");
    const auto current_input=[&]{
        bank_bookend();const auto canonical=input->retain_owner();
        need(canonical.get()==input.get()&&!canonical.owner_before(input)&&!input.owner_before(canonical),
            "Bank include source substituted EarlyInput control");
        input->require_current();bank_bookend();
    };
    current_input();const auto package=BnwNativePackageDiscoveryOwner::create(input);current_input();
    const auto current=[&]{
        current_input();const auto canonical=package->retain_factory_owner();
        need(canonical.get()==package.get()&&!canonical.owner_before(package)&&!package.owner_before(canonical),
            "Bank include source substituted Package factory control");
        const auto actual=package->early_input_lease();current_input();
        need(actual.get()==input.get()&&!actual.owner_before(input)&&!input.owner_before(actual),
            "Bank include source created another selected input");
    };
    current();
    need(!package->impl_->bank_context_app_subobject,"Bank include App constructor replay");
    auto app=std::make_shared<Impl::BankContextAppSubobject>(package,input);current();
    package->impl_->bank_context_app_subobject=std::move(app);current();
    // Reuse genuine constructor, admission, FIRST and manager actors. There is
    // no conversion from a port mount preferred/enabled flag or sorted keys.
    detail::BnwNativeDefaultCommonProduction::primary(package);current();
    auto settings=BnwNativeResourceBootSettingsOwner::create(package);current();
    const auto language=settings->selected_language_input();current();language->require_current();current();
    const auto options=settings->selected_user_options();current();options->require_current();current();
    package->register_native_dx9_texture_quality_branch(settings);current();
    package->register_native_common_high_quality_prefix();current();
    const auto result=detail::BnwNativeDefaultCommonProduction::advance_owned_initial_category_prefix(package);
    current();need(bool(result),"Bank include actual resource actor returned no completion");
    result->require_current();current();
    // The selected namespace producer runs once even for a genuine zero-row
    // typed query. Constructor NULL is never published as additional names.
    package->activate_bank_context_lua_include_mods_owned(bank_bookend);current();
    result->require_current();current();return result;
}

void BnwNativePackageDiscoveryOwner::activate_bank_context_lua_include_mods_owned(
    const std::function<void()>& raw_bank_bookend)const{
    BnwPrimaryDefaultStageTrace bank_include_activation_trace("BankInclude.activation"); // BANK_INCLUDE_ENTRY_TRACE
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Bank selected App activation foreign creator thread");
    const auto bank_bookend=raw_bank_bookend;const auto app=i.bank_context_app_subobject;
    const auto input=i.lease;const auto first=i.first_lineage;
    const auto resource=i.completed_resource_capability.lock();
    need(bank_bookend&&app&&input&&first&&resource&&app->creator==i.thread&&!app->failed&&!app->activating&&
         !app->selected_activation&&app->input.get()==input.get()&&
         !app->input.owner_before(input)&&!input.owner_before(app->input)&&
         !app->package.owner_before(i.self)&&!i.self.owner_before(app->package),
         "Bank selected App activation lost SAME fresh constructor/input/source");
    struct Scope final {
        Impl::BankContextAppSubobject& app;bool returned=false;
        explicit Scope(Impl::BankContextAppSubobject& a):app(a){app.activating=true;}
        ~Scope(){if(!returned)app.failed=true;app.activating=false;}
    } scope(*app);
    const auto current=[&]{
        need(app->creator==std::this_thread::get_id()&&!app->failed&&app->activating&&
             i.bank_context_app_subobject==app&&i.lease==input&&i.first_lineage==first,
             "Bank selected App activation source changed/reentered");
        bank_bookend();input->require_current();bank_bookend();resource->require_identity_current();bank_bookend();
        need(i.bank_context_app_subobject==app&&i.lease==input&&i.first_lineage==first&&!app->failed,
             "Bank selected App activation lost current source after callback");
        first->require_current();
    };
    current();const auto profile=input->profile();current();
    const auto catalog=input->catalog();current();
    const auto header=input->selected_header();current();
    // Authenticate the actual selected profile again through the SAME source
    // resolver. Extra installed mods do not become active. No Steam cache is read.
    const auto resolved=runtime::resolve_saved_content_profile(header,catalog);current();
    const auto same_requirements=[&]{
        if(profile.requirements.size()!=resolved.requirements.size())return false;
        for(std::size_t n=0;n<profile.requirements.size();++n){
            const auto& a=profile.requirements[n];const auto& b=resolved.requirements[n];
            if(a.kind!=b.kind||content::canonical_guid(a.id)!=content::canonical_guid(b.id)||
               a.wire_value!=b.wire_value||a.status!=b.status||a.package!=b.package||a.candidates!=b.candidates)return false;
        }
        return true;
    };
    need(profile.can_activate&&profile.all_requirements_located&&profile.files_complete&&!profile.native_mods_require_port&&
         resolved.can_activate&&resolved.all_requirements_located&&resolved.files_complete&&!resolved.native_mods_require_port&&
         profile.kind==resolved.kind&&profile.enabled_dlc==resolved.enabled_dlc&&
         profile.enabled_mods==resolved.enabled_mods&&same_requirements(),
         "Bank selected App activation does not match actual current header/catalog/profile");current();
    using Activation=Impl::BankContextAppSubobject::SelectedActivation;
    std::vector<Activation::ModRow> mods;std::vector<Activation::ModFileRow> files;
    std::set<std::size_t> active;
    for(const auto n:profile.enabled_mods){
        need(n<catalog.packages.size()&&catalog.packages[n].kind==content::PackageKind::mod&&active.insert(n).second,
             "Bank selected App activation mod selection is not unique actual catalog rows");current();
    }
    for(std::size_t n=0;n<catalog.packages.size();++n){
        const auto& package=catalog.packages[n];if(package.kind!=content::PackageKind::mod)continue;
        // This physical selection write is the OWN Mods.Activated column.
        // Each ModFiles row retains the selected manifest's authored import bit.
        mods.push_back({n,content::canonical_guid(package.id),package.version,active.contains(n)});current();
        for(const auto& file:package.files){files.push_back({n,file.path,file.import});current();}
    }
    need(std::count_if(mods.begin(),mods.end(),[](const auto& row){return row.activated;})==
         static_cast<std::ptrdiff_t>(profile.enabled_mods.size()),"Bank selected App activation omitted active catalog rows");
    std::vector<std::filesystem::path> imports;
    // Exact typed equivalent of the native query:
    // Select EvaluatedPath from ModFiles natural join Mods
    // where Mods.Activated=1 and Import=1.
    for(const auto& file:files){
        const auto mod=std::find_if(mods.begin(),mods.end(),[&](const auto& row){return row.package==file.package;});
        need(mod!=mods.end(),"Bank ModFiles typed join lost its actual Mods row");
        if(!mod->activated||!file.import)continue;
        imports.push_back(content::package_file(catalog.packages[file.package].path.parent_path(),file.requested_path));current();
    }
    // BNW bank capture itself admits only the verified zero-mod source domain.
    // Positive imported rows need the independent83AC70 original Stat/Prefix/
    // mode2 admission producer. Refuse before namespace reset or import effects.
    need(profile.enabled_mods.empty()&&imports.empty(),
         "Bank App mode2 imported files need original83AC70/Stat/Prefix admission producer");current();
    //6CD70D..725 retires old SAME namespace then zeroes slot438. The fresh BNW
    //source has no prior namespace, but it still executes the selected actor.
    app->additional438.reset();current();
    //6CD72D822A50(0,0)→82A050 ALWAYS constructs a nonNULL namespace, even when
    //the physically evaluated selected-import query has zero rows.
    auto additional=std::make_shared<Impl::BankContextAppSubobject::AdditionalNamespace>();current();
    additional->state.selected.bind_shared_first(first);current();
    app->additional438=additional;current(); //6CD742 publishes SAME returned pointer.
    additional->name84="Modding";current(); //6CD73B literalA710F4→82B3E0, NOT +89 setter.
    need(additional->state.live_enabled89==1&&additional->state.selected.records.empty()&&
         additional->state.selected.bucket_order.live_nodes()==0&&additional->state.selected.source_first_==first,
         "BNW empty selected App namespace did not execute its genuine constructor");current();
    auto receipt=std::make_unique<const Activation>(profile.enabled_dlc,profile.enabled_mods,
        std::move(mods),std::move(files),std::move(imports),additional);current();
    // All selection, constructor, publication and name effects precede this
    // typed namespace-only return. Full6CD160/Modding/GameCore readiness is unissued.
    app->selected_activation=std::move(receipt);current();scope.returned=true;
}

std::vector<std::string> BnwNativePackageDiscoveryOwner::collect_bank_context_lua_include_names_owned(
    const std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability>& supplied,
    const std::function<void()>& raw_bank_bookend)const{
    BnwPrimaryDefaultStageTrace bank_include_collect_trace("BankInclude.collect"); // BANK_INCLUDE_ENTRY_TRACE
    const auto storage=impl_;auto& i=*storage;
    need(i.thread==std::this_thread::get_id(),"Bank Lua collector foreign creator thread");
    const auto capability=supplied;const auto bank_bookend=raw_bank_bookend;
    const auto app=i.bank_context_app_subobject;const auto warm=i.live_resource_query;
    const auto activation=app?app->selected_activation.get():nullptr;
    const auto additional=app?app->additional438:nullptr;
    need(capability&&bank_bookend&&warm&&app&&activation&&additional&&!app->failed&&!app->activating&&
         activation->additional==additional&&activation->selected_mods.empty()&&activation->selected_imports.empty()&&
         app->creator==i.thread&&app->input.get()==i.lease.get()&&
         !app->input.owner_before(i.lease)&&!i.lease.owner_before(app->input)&&
         !app->package.owner_before(i.self)&&!i.self.owner_before(app->package),
         "Bank Lua collector lost SAME native factory/App constructor source");
    bank_bookend();capability->require_identity_current();bank_bookend();
    std::vector<std::string> result;
    with_native_owned_completed_resource_operation(capability,[&]{
        const auto* operation=i.active_completed_resource_operation;
        need(operation&&operation->completed_source&&operation->completed_cold,
             "Lua enumeration requires genuine current Completed Source/Cold scope");
        const auto& loan=*operation->completed_source;const auto& cold=*operation->completed_cold;
        using Catalog=detail::BnwNativeSkinDocumentRegistration;
        const auto catalog=i.resource_catalog->state_;Catalog::State::Scope catalog_scope(catalog);
        const auto first=i.first_lineage;const auto state=i.default_state;
        const auto revision=i.registry.revision;const auto default89=state->live_enabled89;
        const auto early_check=[&]{
            require_native_owned_completed_resource_binding_current(*capability,loan,cold);
            bank_bookend();
            need(i.active_completed_resource_operation==operation&&i.live_resource_query==warm&&
                 i.resource_catalog->state_==catalog&&i.first_lineage==first&&i.default_state==state&&
                 i.registry.revision==revision&&state->live_enabled89==default89&&
                 i.bank_context_app_subobject==app&&app->selected_activation.get()==activation&&
                 app->additional438==additional&&activation->additional==additional&&!app->failed&&!app->activating&&
                 additional->name84=="Modding"&&additional->state.live_enabled89==1&&
                 !catalog->closed.load()&&!catalog->failed.load(),"Lua enumeration actual namespace epoch changed");
            require_native_owned_completed_resource_binding_current(*capability,loan,cold);
        };
        early_check();const auto ordered=catalog->cache;early_check();
        const auto documents=catalog->documents;early_check();
        const auto check=[&]{
            early_check();need(catalog->cache==ordered&&catalog->documents==documents,
                "Lua enumeration actual registered vector changed");
            require_native_owned_completed_common_sources_current(loan,cold);
            for(const auto& manager:ordered){
                const auto found=std::find_if(warm->owned_category_stages_.begin(),warm->owned_category_stages_.end(),
                    [&](const auto& stage){return stage&&stage->manager.lock().get()==manager.get();});
                need(found!=warm->owned_category_stages_.end(),"Lua enumeration manager lacks authentic terminal Stage");
                warm->require_owned_completed_category_stage_current(**found,loan,cold);
                need(manager&&manager->owner.lock()==catalog&&manager->current89.has_value()&&
                     manager->source_kind==Catalog::SourceKind::owned_package,
                     "Lua enumeration manager current89/source owner unknown");
            }
            first->require_current();early_check();
        };
        // Only inside the genuine Source/Cold operation and Catalog Scope.
        // This observer has no Bank callback, Common replay or filesystem IO.
        const auto pure_check=[&]{
            require_native_owned_completed_resource_binding_current(*capability,loan,cold);
            need(i.active_completed_resource_operation==operation&&i.live_resource_query==warm&&
                 i.resource_catalog->state_==catalog&&i.first_lineage==first&&i.default_state==state&&
                 i.registry.revision==revision&&state->live_enabled89==default89&&
                 i.bank_context_app_subobject==app&&app->selected_activation.get()==activation&&
                 app->additional438==additional&&activation->additional==additional&&!app->failed&&!app->activating&&
                 additional->name84=="Modding"&&additional->state.live_enabled89==1&&
                 !catalog->closed.load()&&!catalog->failed.load()&&catalog->busy.load(),
                 "Lua enumeration actual namespace epoch changed");
            need(catalog->cache==ordered&&catalog->documents==documents,
                 "Lua enumeration actual registered vector changed");
            first->require_current();
            require_native_owned_completed_resource_binding_current(*capability,loan,cold);
        };
        const auto emit=[&](const detail::NativeResourceRecordIndex& index){
            check();need(index.source_first_==first&&index.bucket_order.live_nodes()==index.records.size(),
                "Lua enumeration namespace lost SAME shared FIRST/native bucket nodes");
            // Own the finite node sequence; this allocation retains its full POST.
            auto nodes=index.bucket_order.ordered_nodes();check();
            for(const auto node:nodes){
                pure_check();need(node.key==node.value&&index.records.contains(node.key),
                    "Lua enumeration physical selected record CRC/order differs");
                char raw260[260]{};std::size_t raw_size=0;
                {
                    const auto& names=first->names();const auto found=names.find(node.key);
                    // FIRST MISS stays empty; preserve the exact native matcher.
                    // Successful map/view/matcher/copy is callback-free and does
                    // not allocate. No borrowed name crosses the scope below.
                    const std::string_view raw=found==names.end()?std::string_view{}:std::string_view(found->second);
                    pure_check();
                    if(!native_category_patterns_detail::match_list_ascii(raw,"*.lua",false)){pure_check();continue;}
                    need(raw.size()<=259,"Lua FIRST path exceeds actual260 UTF16 FString");
                    raw_size=raw.size();std::copy(raw.begin(),raw.end(),raw260);raw260[raw_size]='\0';
                }
                check(); // CPU-only filter POST, and allocation PRE; view is gone.
                std::wstring wide;wide.reserve(raw_size);check();
                // reserve has already provided the whole bounded destination.
                for(std::size_t n=0;n<raw_size;++n)
                    wide.push_back(static_cast<wchar_t>(static_cast<unsigned char>(raw260[n])));
                pure_check();wchar_t physical260[260];
                need(::wcscpy_s(physical260,260,wide.c_str())==0,"Lua FIRST260 native path copy failed");check();
                const int count=WideCharToMultiByte(CP_UTF8,0,physical260,-1,nullptr,0,nullptr,nullptr);check();
                need(count>0,"Lua FIRST UTF16 to UTF8 size conversion failed");
                std::string utf8_name(static_cast<std::size_t>(count),'\0');check();
                const int written=WideCharToMultiByte(CP_UTF8,0,physical260,-1,utf8_name.data(),count,nullptr,nullptr);check();
                need(written==count&&utf8_name.back()=='\0',"Lua FIRST UTF8 conversion changed/failed");
                utf8_name.pop_back();result.push_back(std::move(utf8_name));check();
            }
            check();
        };
        check();
        //821CC0: Default+89 gates the entire first call, followed by current
        //registered namespaces in reverse cache order, THEN actual Default.
        if(default89){
            for(auto at=ordered.rbegin();at!=ordered.rend();++at){
                const auto manager=*at;check();if(!*manager->current89)continue;
                const auto found=std::find_if(warm->owned_category_stages_.begin(),warm->owned_category_stages_.end(),
                    [&](const auto& stage){return stage&&stage->manager.lock().get()==manager.get();});
                need(found!=warm->owned_category_stages_.end(),"Lua enabled manager lost SAME Stage");
                const auto stage=*found;check();emit(stage->source->records_);check();
            }
            emit(state->selected);check();
        }
        //582B54..62 passes the authentic App+EF4 container as a SECOND8B4AF0
        //call. It is not part of the first call's Default89 gate. Preserve all
        //prior names and duplicates; an actual empty container emits zero rows.
        emit(additional->state.selected);check();
        check();
    });
    // Genuine outer Bank/Header/Loaded/Cold/Observer destructors have returned.
    bank_bookend();capability->require_identity_current();bank_bookend();
    need(i.bank_context_app_subobject==app&&app->selected_activation.get()==activation&&
         app->additional438==additional&&activation->additional==additional&&!app->failed&&!app->activating,
        "Lua actual selected App namespace changed before normal collector return");
    return result;
}
