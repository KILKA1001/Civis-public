// Initial prefix adapter. Included only in the actual Parent implementation.
// Read-only test observation. Never consulted to authorize a source or operation.
namespace detail {
namespace {
thread_local unsigned initial_descriptor_metadata_phase_for_test=0;
struct InitialDescriptorMetadataPhaseObservation {
    const unsigned previous=initial_descriptor_metadata_phase_for_test;
    explicit InitialDescriptorMetadataPhaseObservation(unsigned phase)noexcept {
        initial_descriptor_metadata_phase_for_test=phase;
    }
    ~InitialDescriptorMetadataPhaseObservation(){initial_descriptor_metadata_phase_for_test=previous;}
    InitialDescriptorMetadataPhaseObservation(const InitialDescriptorMetadataPhaseObservation&)=delete;
};
}
unsigned bnw_native_initial_descriptor_retained_metadata_phase_for_test()noexcept {
    return initial_descriptor_metadata_phase_for_test;
}
}
std::filesystem::path detail::BnwNativePreDx9DiscoveryOwner::actual_initial_descriptor_path(
    const BnwNativePackageLoadActor& actor)const {
    require_current();ParentPin retained(*this);
    need(actor.state_->receiver&&actor.state_->discovery.lock()&&
         (impl_->phase==Impl::Phase::constructing||impl_->phase==Impl::Phase::returned||impl_->phase==Impl::Phase::adopted),
         "Initial descriptor filename outside actual constructor/returned source");
    const auto raw=actor.state_->receiver->copied_input_for_498940_global_lookup();
    const std::filesystem::path relative(native_loose_detail::acp_wide(raw));
    need(!relative.empty()&&!relative.is_absolute()&&!relative.has_root_name()&&
         *relative.begin()==std::filesystem::path("Assets"),"Initial descriptor is not actual Assets relative field0");
    for(const auto& part:relative)need(part!=L".."&&part!=L".","Initial descriptor escaped actual install root");
    auto path=(retained.parent->impl_->original.parent_path()/relative).lexically_normal();
    require_current();return path;
}
void detail::BnwNativePreDx9DiscoveryOwner::initialize_actual_category2_pair(
    const std::shared_ptr<BnwNativePackageLoadActor>& actor,
    const BnwNativeResourceDiscoveryFilename::Source::ConstructionOperation& construction) {
    require_current();ParentPin retained(*this);
    need(impl_->phase==Impl::Phase::constructing&&impl_->busy.load()&&actor&&
         impl_->actor&&actor->state_->receiver&&actor->state_->busy.load()&&
         !actor->state_->discovery.owner_before(impl_->actor)&&!impl_->actor.owner_before(actor->state_->discovery)&&
         !actor->state_->initial_category2_pair&&!actor->state_->descriptor_bytes&&!actor->state_->pre_dx9_lookup_source,
         "Initial Category2 constructor escaped SAME actual new receiver before name-copy/lookup/read");
    const auto source=actor->state_->filename.lock();construction.require_same(source);
    need(!actor->state_->receiver->state_->copy_attempted&&actor->state_->receiver->state_->field0.empty(),
         "Initial Category2 constructor followed actual498940 name copy");
    const std::filesystem::path relative(native_loose_detail::acp_wide(source->native_acp_filename));
    need(!relative.empty()&&!relative.is_absolute()&&!relative.has_root_name()&&
         *relative.begin()==std::filesystem::path("Assets"),"Initial6F7D30 filename escaped actual installation");
    for(const auto& part:relative)need(part!=L".."&&part!=L".","Initial6F7D30 filename contains another root");
    auto path=(retained.parent->impl_->original.parent_path()/relative).lexically_normal();
    auto pair=std::shared_ptr<BnwNativePackageCategory2Pair>(new BnwNativePackageCategory2Pair(
        std::move(path),impl_->input,retained.parent->impl_->identity,actor,impl_->self.lock()));
    pair->parent_=retained.parent->impl_->self;construction.require_same(source);require_current();
    need(!actor->state_->initial_category2_pair,"Initial Category2 constructor replay");
    actor->state_->initial_category2_pair=std::move(pair);construction.require_same(source);require_current();
}
void detail::BnwNativePreDx9DiscoveryOwner::require_initial_descriptor_identity_without_callbacks(
    const BnwNativePackageLoadActor& actor,const BnwNativeStandalonePackageLoaded& token)const {
    need(impl_->thread==std::this_thread::get_id()&&!impl_->closed.load()&&!impl_->failed.load()&&
         (impl_->phase==Impl::Phase::returned||impl_->phase==Impl::Phase::adopted),
         "Initial prefix adapter has no actual returned pipeline");
    actor.state_->live();
    const auto& state=*actor.state_;
    need(state.receiver&&state.initial_category2_pair&&state.descriptor_bytes&&state.parsed_xml&&state.descriptor_core&&
         state.metadata_input&&impl_->language&&state.metadata_input.get()==impl_->language.get()&&
         !state.metadata_input.owner_before(impl_->language)&&!impl_->language.owner_before(state.metadata_input)&&
         state.pre_dx9_lookup_source&&!state.actual_lookup_source&&
         !state.pre_dx9_lookup_source->owner_before(impl_->self)&&!impl_->self.owner_before(*state.pre_dx9_lookup_source)&&
         token.actual_receiver_==state.receiver.get()&&token.actual_metadata_==state.completed_metadata_identity&&
         !token.actor_.owner_before(state.actor)&&!state.actor.owner_before(token.actor_)&&
         !token.self_.owner_before(state.returned_loaded)&&!state.returned_loaded.owner_before(token.self_),
         "Initial adapter lost SAME actual receiver/Parser/Core/metadata return controls");
    const auto same=[](const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);};
    bool found=false;
    if(impl_->phase==Impl::Phase::returned){
        need(impl_->receivers.size()==impl_->returned.size(),"Initial returned roster differs");
        for(std::size_t n=0;n<impl_->receivers.size();++n)
            if(impl_->receivers[n].get()==&actor&&same(impl_->receivers[n],state.actor)&&
               impl_->returned[n].get()==&token&&same(impl_->returned[n],token.self_))found=true;
    }else{
        const auto warm=impl_->consumer.lock();need(bool(warm),"Initial prefix adapter Warm retired");
        require_adopted_warm_identity_without_callbacks(*warm,state.actor);
        need(warm->preload_receivers_.size()==warm->returned_standalone_packages_.size(),"Adopted initial roster differs");
        for(std::size_t n=0;n<warm->preload_receivers_.size();++n)
            if(warm->preload_receivers_[n].get()==&actor&&same(warm->preload_receivers_[n],state.actor)&&
               warm->returned_standalone_packages_[n].get()==&token&&same(warm->returned_standalone_packages_[n],token.self_))found=true;
    }
    need(found,"Initial compatibility view is not an actually retained successful receiver/token pair");
}
struct detail::BnwNativeInitialDescriptorSource::Impl {
    using Parser=BnwNativePackageXmlParserOwner;using Core=BnwNativePackageDescriptorCoreWriter;
    const std::thread::id thread=std::this_thread::get_id();
    std::weak_ptr<BnwNativePreDx9DiscoveryOwner> issuer;
    std::weak_ptr<const BnwNativePackageLoadActor> actor;
    std::weak_ptr<const BnwNativeStandalonePackageLoaded> returned;
    std::weak_ptr<const BnwNativePackageCategory2Pair> pair;
    std::weak_ptr<const BnwNativeInitialDescriptorSource> self;
    // Aliasing shared_ptrs retain the SAME actual Parser/Core facade controls.
    // No actor, Parent or Warm facade keeper is added by this adapter.
    std::shared_ptr<const Parser::Document> document;
    std::shared_ptr<const Core::Fields> fields;
    std::shared_ptr<const Core::Metadata> metadata;
    content::PackageManifest manifest;
    runtime::FileIdentity file;
    std::string raw;
    std::vector<std::pair<const content::XmlNode*,const Parser::Node*>> nodes;
    mutable std::atomic<bool> checking{false},failed{false};
    static std::string text(const Parser::Document& doc,Parser::Range value) {
        need(value.begin<=doc.decoded.size()&&value.size<=doc.decoded.size()-value.begin,"Initial adapter range escaped SAME native Document");
        std::string result(reinterpret_cast<const char*>(doc.decoded.data()+value.begin),value.size);
        need(result.find('\0')==result.npos,"Initial legacy metadata adapter does not support embedded NUL ranges");return result;
    }
    static content::XmlNode project(const Parser::Document& doc,const Parser::Node& node,std::size_t depth=0) {
        need(depth<=512,"Initial adapter native depth exceeds bounded parser contract");content::XmlNode out;
        out.name=text(doc,node.name);if(node.value)out.text=text(doc,*node.value);
        out.attributes.reserve(node.attributes.size());
        for(const auto& attribute:node.attributes)out.attributes.emplace_back(text(doc,attribute.name),text(doc,attribute.value));
        out.children.reserve(node.children.size());
        for(const auto& child:node.children)out.children.push_back(project(doc,*child,depth+1));return out;
    }
    void map_nodes(const content::XmlNode& view,const Parser::Node& actual) {
        need(view.children.size()==actual.children.size(),"Initial adapter node arity changed");nodes.emplace_back(&view,&actual);
        for(std::size_t n=0;n<view.children.size();++n)map_nodes(view.children[n],*actual.children[n]);
    }
    static std::string guid_name(const std::array<std::uint8_t,16>& bytes) {
        constexpr unsigned order[]={3,2,1,0,5,4,7,6,8,9,10,11,12,13,14,15};
        constexpr char hex[]="0123456789abcdef";std::string out;out.reserve(36);
        for(unsigned n=0;n<16;++n){if(n==4||n==6||n==8||n==10)out+='-';const auto value=bytes[order[n]];out+=hex[value>>4];out+=hex[value&15];}
        return out;
    }
};
detail::BnwNativeInitialDescriptorSource::BnwNativeInitialDescriptorSource(std::unique_ptr<Impl> value):impl_(std::move(value)){}
detail::BnwNativeInitialDescriptorSource::~BnwNativeInitialDescriptorSource()=default;
std::shared_ptr<const detail::BnwNativeInitialDescriptorSource> detail::BnwNativeInitialDescriptorSource::capture(
    const std::shared_ptr<BnwNativePreDx9DiscoveryOwner>& issuer,const std::shared_ptr<BnwNativePackageLoadActor>& actor,
    const std::shared_ptr<const BnwNativeStandalonePackageLoaded>& token) {
    need(issuer&&actor&&token,"Initial prefix adapter requires actual issuer/receiver/metadata return");
    issuer->require_current();issuer->require_initial_descriptor_identity_without_callbacks(*actor,*token);
    auto staged=std::make_unique<Impl>();staged->issuer=issuer;staged->actor=actor;staged->returned=token;
    staged->pair=actor->state_->initial_category2_pair;
    staged->manifest.path=issuer->actual_initial_descriptor_path(*actor);
    {
        const auto owned=actor->state_;BnwNativePackageLoadActor::State::Scope operation(owned);
        const auto parser=owned->parsed_xml;const auto core=owned->descriptor_core;
        core->with_current_standalone_metadata([&](const auto& fields,const auto& metadata,const auto& doc,const auto& guard){
            guard.require_current();issuer->require_initial_descriptor_identity_without_callbacks(*actor,*token);
            need(&metadata==token->actual_metadata_&&parser->state_->document.get()==&doc&&fields.written_guid,
                 "Initial legacy metadata adapter needs actual written GUID and SAME native Document/metadata");
            staged->document=std::shared_ptr<const Impl::Parser::Document>(parser,&doc);
            staged->fields=std::shared_ptr<const Impl::Core::Fields>(core,&fields);
            staged->metadata=std::shared_ptr<const Impl::Core::Metadata>(core,&metadata);
            need(doc.roots.size()==1,"Initial legacy adapter requires one actual native package root");
            staged->manifest.kind=content::PackageKind::dlc;staged->manifest.id=Impl::guid_name(*fields.written_guid);
            staged->manifest.version=fields.version;staged->manifest.name=metadata.name;
            staged->manifest.document=Impl::project(doc,*doc.roots.front());
            staged->map_nodes(staged->manifest.document,*doc.roots.front());
            const auto bytes=std::span<const std::uint8_t>(doc.input->nul_terminated.data(),doc.input->native_size);
            staged->file=runtime::identify_bytes(bytes);staged->raw.assign(reinterpret_cast<const char*>(bytes.data()),bytes.size());
            guard.require_current();issuer->require_initial_descriptor_identity_without_callbacks(*actor,*token);
        });
    }
    issuer->require_current();
    const auto pair=actor->state_->initial_category2_pair;
    need(pair&&pair->initial_constructor_source_&&!pair->actual_read_identity_&&pair->creation_filename_==staged->manifest.path&&
         !pair->initial_receiver_.owner_before(actor)&&!actor.owner_before(pair->initial_receiver_)&&
         !pair->initial_source_.owner_before(issuer)&&!issuer.owner_before(pair->initial_source_),
         "Initial adapter is not SAME earlier actual Category2 constructor");
    pair->actual_read_identity_=staged->file; // Actual Reader witness, never a readiness receipt.
    auto out=std::shared_ptr<const BnwNativeInitialDescriptorSource>(new BnwNativeInitialDescriptorSource(std::move(staged)));
    out->impl_->self=out;out->require_current();issuer->require_current();return out;
}
void detail::BnwNativeInitialDescriptorSource::require_current()const {
    need(impl_->thread==std::this_thread::get_id()&&!impl_->failed.load()&&!impl_->self.expired(),"Initial descriptor adapter retired/foreign creator");
    if(impl_->checking.exchange(true)){impl_->failed.store(true);throw std::runtime_error("Initial descriptor adapter reentry");}
    struct End{Impl& i;~End(){i.checking.store(false);}} end{*impl_};
    try {
        const auto issuer=impl_->issuer.lock();const auto actor=impl_->actor.lock();const auto token=impl_->returned.lock();
        need(issuer&&actor&&token,"Initial descriptor lost actual source/receiver/return");issuer->require_current();
        issuer->require_initial_descriptor_identity_without_callbacks(*actor,*token);
        const auto owned=actor->state_;BnwNativePackageLoadActor::State::Scope operation(owned);
        const auto parser=owned->parsed_xml;const auto core=owned->descriptor_core;
        const auto pair=impl_->pair.lock();
        const auto pair_current=[&]{need(pair&&owned->initial_category2_pair.get()==pair.get()&&
            !owned->initial_category2_pair.owner_before(pair)&&!pair.owner_before(owned->initial_category2_pair)&&
            pair->creation_filename_==impl_->manifest.path&&pair->actual_read_identity_&&
            pair->actual_read_identity_->bytes==impl_->file.bytes&&pair->actual_read_identity_->sha256==impl_->file.sha256,
            "Initial descriptor lost SAME actual constructor/Reader witness");};
        pair_current();
        need(impl_->document&&impl_->fields&&impl_->metadata&&
             !impl_->document.owner_before(parser)&&!parser.owner_before(impl_->document)&&
             !impl_->fields.owner_before(core)&&!core.owner_before(impl_->fields)&&
             !impl_->metadata.owner_before(core)&&!core.owner_before(impl_->metadata),"Initial aliases lost actual Parser/Core controls");
        core->with_current_standalone_metadata([&](const auto& fields,const auto& metadata,const auto& doc,const auto& guard){
            guard.require_current();need(impl_->document.get()==&doc&&impl_->fields.get()==&fields&&impl_->metadata.get()==&metadata&&
                parser->state_->document.get()==&doc&&token->actual_metadata_==&metadata&&
                !impl_->nodes.empty()&&impl_->nodes.front().first==&impl_->manifest.document&&
                impl_->nodes.front().second==doc.roots.front().get(),"Initial adapter changed SAME immutable DOM/Core/metadata aliases");
            issuer->require_initial_descriptor_identity_without_callbacks(*actor,*token);guard.require_current();
        });pair_current();issuer->require_current();issuer->require_initial_descriptor_identity_without_callbacks(*actor,*token);
    }catch(...){impl_->failed.store(true);throw;}
}
void detail::BnwNativeInitialDescriptorSource::require_retained_metadata_current()const {
    need(impl_->thread==std::this_thread::get_id()&&!impl_->failed.load()&&!impl_->self.expired(),"Initial descriptor adapter retired/foreign creator");
    if(impl_->checking.exchange(true)){impl_->failed.store(true);throw std::runtime_error("Initial descriptor adapter reentry");}
    struct End{Impl& i;~End(){i.checking.store(false);}} end{*impl_};
    try {
        const auto issuer=impl_->issuer.lock();const auto actor=impl_->actor.lock();const auto token=impl_->returned.lock();
        need(issuer&&actor&&token,"Initial descriptor lost actual source/receiver/return");issuer->require_current();
        issuer->require_initial_descriptor_identity_without_callbacks(*actor,*token);
        const auto owned=actor->state_;BnwNativePackageLoadActor::State::Scope operation(owned);
        const auto parser=owned->parsed_xml;const auto core=owned->descriptor_core;
        const auto pair=impl_->pair.lock();
        const auto pair_current=[&]{need(pair&&owned->initial_category2_pair.get()==pair.get()&&
            !owned->initial_category2_pair.owner_before(pair)&&!pair.owner_before(owned->initial_category2_pair)&&
            pair->creation_filename_==impl_->manifest.path&&pair->actual_read_identity_&&
            pair->actual_read_identity_->bytes==impl_->file.bytes&&pair->actual_read_identity_->sha256==impl_->file.sha256,
            "Initial descriptor lost SAME actual constructor/Reader witness");};
        pair_current();
        need(impl_->document&&impl_->fields&&impl_->metadata&&
             !impl_->document.owner_before(parser)&&!parser.owner_before(impl_->document)&&
             !impl_->fields.owner_before(core)&&!core.owner_before(impl_->fields)&&
             !impl_->metadata.owner_before(core)&&!core.owner_before(impl_->metadata),"Initial aliases lost actual Parser/Core controls");
        // Only the Parent roster uses this retained observation. The actual
        // Actor/Core/Parser/Reader scopes and source/selected PREPOST remain;
        // no callback target borrows native values between these bookends.
        {
        const auto core_state=core->state_;
        Impl::Core::State::Scope core_scope(core_state);
        try {
            core_state->check(parser); // Actual selected process/product/CRT PRE.
            const auto parser_state=parser->state_;const auto reader=parser_state->reader.lock();
            const auto same=[](const auto& first,const auto& second){
                return !first.owner_before(second)&&!second.owner_before(first);};
            const auto aliases_without_callbacks=[&]{
                need(impl_->checking.load()&&!impl_->failed.load()&&!impl_->self.expired(),
                     "Initial retained metadata observation retired/reentered");
                owned->live();core_state->live();parser_state->check(reader);reader->state_->live();
                issuer->require_initial_descriptor_identity_without_callbacks(*actor,*token);
                need(owned->descriptor_core.get()==core.get()&&owned->parsed_xml.get()==parser.get()&&
                     owned->descriptor_bytes.get()==reader.get()&&same(core_state->parser,parser)&&
                     same(parser_state->reader,reader)&&same(reader->state_->reader,reader)&&
                     impl_->document&&impl_->fields&&impl_->metadata&&same(impl_->document,parser)&&
                     same(impl_->fields,core)&&same(impl_->metadata,core)&&parser_state->document&&
                     reader->state_->buffer&&impl_->document.get()==parser_state->document.get()&&
                     impl_->fields.get()==core_state->fields.get()&&impl_->metadata.get()==core_state->metadata.get()&&
                     token->actual_metadata_==impl_->metadata.get()&&core_state->document_identity==impl_->document.get(),
                     "Initial retained metadata lost SAME actual facade/control/aliases");
                const auto& doc=*impl_->document;const auto& buffer=*reader->state_->buffer;
                need(doc.input.get()==reader->state_->buffer.get()&&same(doc.input,reader->state_->buffer)&&
                     buffer.nul_terminated.size()==std::size_t(buffer.native_size)+1U&&
                     buffer.nul_terminated[buffer.native_size]==0&&!doc.roots.empty()&&!impl_->nodes.empty()&&
                     impl_->nodes.front().first==&impl_->manifest.document&&impl_->nodes.front().second==doc.roots.front().get(),
                     "Initial retained metadata lost SAME immutable Buffer/DOM view");
                pair_current();
            };
            {
                Impl::Parser::State::Scope parser_scope(parser_state);
                try {
                    parser_state->check(reader);
                    {
                        BnwNativePackageDescriptorByteReader::State::Scope reader_scope(reader->state_);
                        const auto source=reader->state_->source.lock(); // ONE actual Reader source pin.
                        try {
                            need(owned->busy.load()&&core_state->busy.load()&&parser_state->busy.load()&&reader->state_->busy.load(),
                                 "Initial retained metadata escaped actual guarded scopes");
                            {InitialDescriptorMetadataPhaseObservation phase(1);
                             reader->state_->check(source);aliases_without_callbacks();} // Genuine physical source PRE.
                            // Published aliases have no supported intervening writer or borrower callback.
                            {InitialDescriptorMetadataPhaseObservation phase(2);
                             reader->state_->check(source);aliases_without_callbacks();} // Genuine physical source POST.
                        }catch(...){
                            reader->state_->failed.store(true);
                            if(source){reader->state_->release_stream(source);source->invalidate();}
                            throw;
                        }
                    }
                    parser_state->check(reader);
                }catch(...){parser_state->failed.store(true);if(reader)reader->retire_after_parser_failure();throw;}
            }
            core_state->check(parser);aliases_without_callbacks(); // Actual selected process/product/CRT POST.
        }catch(...){core_state->failed.store(true);throw;}
        }
        pair_current();issuer->require_current();issuer->require_initial_descriptor_identity_without_callbacks(*actor,*token);
    }catch(...){impl_->failed.store(true);throw;}
}
const content::PackageManifest& detail::BnwNativeInitialDescriptorSource::manifest_without_callbacks()const {return impl_->manifest;}
const runtime::FileIdentity& detail::BnwNativeInitialDescriptorSource::file_without_callbacks()const {return impl_->file;}
const std::string& detail::BnwNativeInitialDescriptorSource::original_bytes_without_callbacks()const {return impl_->raw;}
std::shared_ptr<detail::BnwNativePackageCategory2Pair> detail::BnwNativeInitialDescriptorSource::actual_pair_without_callbacks()const {
    const auto actual=impl_->actor.lock();need(bool(actual),"Initial constructor Pair actual receiver retired");return actual->state_->initial_category2_pair;
}
native_ui_skin_text_detail::LexicalDocument detail::BnwNativeInitialDescriptorSource::borrow_actual_native_values(
    const content::XmlNode& root)const {
    require_current();need(&root==&impl_->manifest.document,"Initial UISkin getter supplied another adapter DOM");
    native_ui_skin_text_detail::LexicalDocument result;
    // Read actual native value ranges. No retain_runs/tokenizer or XML parser
    // runs again. Literal raw Run records are not minted by this route.
    for(const auto& [view,node]:impl_->nodes){
        native_ui_skin_text_detail::Value value;
        if(node->value){const auto range=*node->value;
            value.bytes=Impl::text(*impl_->document,range);
            need(range.begin<impl_->document->decoded.size(),"Initial native value starts outside SAME Document");
            value.first_buffer_byte_nonzero=impl_->document->decoded[range.begin]!=0;
        }
        result.values.emplace(view,std::move(value));
    }
    require_current();return result;
}
void detail::BnwNativePreDx9DiscoveryOwner::join_actual_initial_prefix() {
    need(impl_->thread==std::this_thread::get_id(),"Initial prefix join foreign creator");
    if(impl_->busy.exchange(true)){fail();throw std::runtime_error("Initial prefix join reentry");}
    struct End{Impl& i;~End(){i.busy.store(false);}} end{*impl_};
    require_current();ParentPin retained(*this);retained.parent->join_pre_dx9_initial_prefix(*this);
}
void BnwNativePackageDiscoveryOwner::join_pre_dx9_initial_prefix(detail::BnwNativePreDx9DiscoveryOwner& issuer)const {
    auto& i=*impl_;auto& b=*issuer.impl_;require_pre_dx9_discovery_current(issuer);
    need(b.busy.load()&&b.phase==detail::BnwNativePreDx9DiscoveryOwner::Impl::Phase::returned&&
         !i.installation_constructor_attempted&&!i.installation_constructor_returned&&!i.installation_pending_descriptor&&
         i.retained_descriptors.empty()&&!i.first_lineage&&!i.global_color_base_document&&
         i.pins.empty()&&!i.lease&&!i.rules&&!i.header_attempted&&!i.header_active&&!i.cold&&!i.settings&&
         !i.live_resource_query&&!i.archive_cache&&!i.entry_arena&&!i.current_default_source&&!i.production_default_published&&
         i.default_state->live_enabled89==1&&i.default_state->selected.records.empty()&&
         i.default_state->loose_native.empty()&&i.resources.mounts().empty(),
         "Initial prefix join requires actual returned pipeline and untouched initial operations");
    Impl::InstallationParentLoan self_loan(i);const auto& self=self_loan.parent;
    need(self&&self.get()==this&&i.installation_has_outside_parent(),"Initial prefix join lost actual outside Parent");
    i.installation_constructor_attempted=true;
    struct Failure{Impl& i;detail::BnwNativePreDx9DiscoveryOwner& source;bool complete=false;
        ~Failure(){if(!complete){i.header_poisoned=true;source.fail();
            for(const auto& row:i.retained_descriptors)if(row.category2_pair)row.category2_pair->revoke_unknown_writer();}}} failure{i,issuer};
    const auto check=[&]{issuer.require_current();need(!i.header_poisoned&&!i.header_active&&!i.header_attempted&&
        i.installation_has_outside_parent(),"Initial prefix join source retired/reentered");};
    check();need(i.installation_input->catalog().diagnostics.empty(),"Actual installation catalog unavailable for initial prefix");
    // Separate genuine ColorAtlas operation. Package bytes/DOM are not read or parsed here.
    i.global_color_base_document=std::make_shared<const content::XmlNode>(i.direct_source(i.shared/"Assets/UI/ColorAtlas.xml",false));
    i.registry.global_map=parse_bnw_native_color_atlas(*i.global_color_base_document).colors;check();
    need(!b.receivers.empty()&&b.receivers.size()==b.returned.size(),"Initial prefix needs actual successful package roster");
    i.retained_descriptors.reserve(b.receivers.size());
    for(std::size_t n=0;n<b.receivers.size();++n){
        check();auto source=detail::BnwNativeInitialDescriptorSource::capture(b.self.lock(),b.receivers[n],b.returned[n]);
        const auto& manifest=source->manifest_without_callbacks();const auto& file=source->file_without_callbacks();
        const auto relative=manifest.path.lexically_relative(i.original);
        need(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!=std::filesystem::path(".."),"Actual initial descriptor escaped Assets");
        for(const auto& known:i.installation_input->catalog().packages)if(std::filesystem::equivalent(known.path,manifest.path))
            need(content::canonical_guid(known.id)==manifest.id&&known.version==manifest.version,
                 "Actual Core GUID/version differs from installation catalog");
        i.pins.push_back({manifest.path,file});source->require_current();
        i.retained_descriptors.emplace_back(assets::path_utf8(relative),source);
        i.frame.original_descriptor_order.push_back(assets::path_utf8(relative));++i.frame.packages;
        // Existing potential Color-map operations consume only the actual native-DOM adapter.
        std::size_t occurrence=0;
        for(const auto& node:manifest.document.children){
            if(node.name!="UISkin")continue;
            need(!node.attribute("name").empty()&&!node.attribute("set").empty()&&!node.attribute("platform").empty(),
                 "Actual potential Skin schema unknown");
            BnwNativeColorContextRead potential;potential.identity=std::make_shared<const std::uint8_t>(0);
            for(std::size_t slot=0;slot<2;++slot){
                content::PackageResourceQuery query;query.selector=std::string(slot==0?"GameplaySkin":"Skin")+"/Color";
                query.enabled_categories={false,false,true};query.category=content::PackageCategory::ui_skin;query.category_occurrence=occurrence;
                std::map<std::uint32_t,BnwNativeColor> colors;
                for(const auto& declaration:content::package_resources(manifest,query)){
                    const auto payload=content::package_file(i.shared/"Assets"/relative.parent_path(),declaration.value);
                    const auto atlas=parse_bnw_native_color_atlas(i.direct_source(payload,false));check();
                    need(atlas.colors.empty(),"Potential nonempty installed Skin Color needs genuine enable/load order");
                }
                potential.maps[slot]=std::move(colors);++i.frame.slot_maps;
            }
            i.registry.contexts.push_back(std::move(potential));++occurrence;++i.frame.skin_contexts;
        }source->require_current();check();
    }
    for(std::size_t n=0;n<i.retained_descriptors.size();++n){auto& row=i.retained_descriptors[n];auto& pair=*row.category2_pair;
        need(pair.initial_constructor_source_&&pair.constructor_tracked_&&!pair.retired_&&
             detail::native_package_category2_pair_detail::constructor_zero(pair.native_words_)&&!pair.document_,
             "Initial join did not retain SAME actual earlier Category2 constructor");
        pair.document_=&row.manifest.document;pair.descriptor_=n;
    }
    check();i.frame.potential_package_color_maps_complete=true;
    i.first_lineage=std::shared_ptr<detail::BnwNativeSharedFirstLineage>(new detail::BnwNativeSharedFirstLineage(i.self,i.installation_input));
    i.default_state->selected.bind_shared_first(i.first_lineage);check();i.installation_constructor_returned=true;
    i.frame.next_resource_dependency="Mandatory installation-aware retained UISkin occurrences/Cold/Stat before Boot/Skin";
    i.registry.unresolved_provider=BnwNativeColorUnresolvedProvider{std::make_shared<const std::uint8_t>(0),i.frame.next_resource_dependency};
    i.validate_installation_prefix_current();check();failure.complete=true;
    throw std::runtime_error(i.frame.next_resource_dependency); // Existing explicit next operation boundary, not game readiness.
}
