// Inside civ5::ui after actual Core/Gui/RuntimeState and color activation owner.
// Closed actual ordinary UnitPanel Box#Base prefix; no Color result DTO/issuer.
void detail::NotificationLayoutRuntimeDimensions::run_box_color_prefix_owned(
    const content::XmlNode& reached_layer,std::size_t ordinal)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();const auto manager=saved->manager.lock();
    require(core&&manager&&saved->id&&core->context=="UnitPanel",
        "Box Color requires actual UnitPanel ordinary parser allocation");
    const auto box=saved->baseline->box_;const auto registry=manager->impl_->selected_color_sets;
    const auto activation=manager->impl_->selected_provider_activation;
    require(box&&registry&&activation,"Box Color required actual Gui/Color Gameplay setup absent");
    const auto argument=context_realm_argument_owned();require_context_argument_owned(saved,argument);
    const auto pure=[&](bool entered){
        require_storage_without_callbacks(saved);require_context_argument_owned(saved,argument);
        registry->first();manager->impl_->first(*saved->operation);
        require(saved->baseline->kind_==NotificationLayoutBaseline::Kind::box&&
            saved->baseline->actual_type_=="Box"&&saved->baseline->box_.get()==box.get()&&
            !saved->baseline->box_.owner_before(box)&&!box.owner_before(saved->baseline->box_)&&
            saved->baseline->parser_runtime_dimensions_==saved->facade&&!saved->baseline->parsed_source_serial_&&
            !saved->registering&&!box->failed&&box->parser_attempted&&box->absent_style_returned&&
            box->frontier==NotificationLayoutBoxState::Frontier::color_required&&
            manager->impl_->selected_color_sets.get()==registry.get()&&
            !manager->impl_->selected_color_sets.owner_before(registry)&&!registry.owner_before(manager->impl_->selected_color_sets)&&
            manager->impl_->selected_provider_activation.get()==activation.get()&&
            !manager->impl_->selected_provider_activation.owner_before(activation)&&!activation.owner_before(manager->impl_->selected_provider_activation)&&
            ordinal<saved->baseline->parser_layers_.size(),
            "Box Color actual subtype/Gui/control/phase or parser occurrence changed");
        const auto& actual=BnwWorldViewControlTree::Core::node(core->nodes,saved->id);
        require(actual.authored.name=="Box"&&actual.authored.attribute("ID")=="Base"&&
            box->color_querying==entered,
            "Box Color selected real receiver changed/replayed/reentered");
    };
    pure(false);require(!box->color_stored,"Box Color actual prefix store replay");
    require(BnwWorldViewControlTree::Core::same_layout_xml(
        saved->baseline->parser_layers_[ordinal],reached_layer),
        "Box Color layer is not actual retained source occurrence");
    //Original selected profile has Style MISS and literal Color=0,0,0,0.
    //Read no borrowed XML across an allocation/callback. Revalidate owned
    //canonical layer after each allocating step before later stores.
    const auto matches=[&](bool entered){pure(entered);const auto& layer=saved->baseline->parser_layers_[ordinal];
        bool seen=false;for(const auto& a:layer.attributes){
            require(a.first!="Style","Box Color Style recursion has not returned");
            if(a.first=="Color"){require(!seen&&a.second=="0,0,0,0","Box Color outside selected finite literal0 profile");seen=true;}}
        require(seen,"Box Color actual PRESENT attribute disappeared");};
    matches(false);
    //Own busy guard remains in retained Box allocation across callbacks.
    box->color_querying=true;
    struct End{std::shared_ptr<NotificationLayoutBoxState> box;bool& busy;~End(){busy=false;}}end{box,box->color_querying};
    const auto current=[&]{matches(true);};
    const auto full=[&]{current();saved->operation->require_current();current();};
    try{
        full();const std::string request("0,0,0,0");full();
        const auto atlases=saved->operation->color_atlas_resources_owned();full();
        require(atlases.size()==registry->atlas.size(),"Box Color actual atlas quote count changed");
        for(std::size_t i=0;i<atlases.size();++i){const auto& now=atlases[i];const auto& old=registry->atlas[i];
            require(now.payload_identity.sha256==old.payload_identity.sha256&&now.bytes.get()==old.bytes.get()&&
                !now.bytes.owner_before(old.bytes)&&!old.bytes.owner_before(now.bytes)&&now.xml.get()==old.xml.get()&&
                !now.xml.owner_before(old.xml)&&!old.xml.owner_before(now.xml),"Box Color SAME published Gui/Bank atlas changed");}
        current();activation->require_current_owned(*saved->operation,manager->impl_);current();
        const bool enabled=activation->gameplay_enabled_without_callbacks(manager->impl_);current();
        //55E2B0 ordinary key→5307B0 active providers then loaded map. Selected
        //active provider has no named Color rows; both real maps are still checked.
        const auto key=native_color_load_detail::hash_query("0");
        const auto active=registry->active_provider.colors.find(key);
        const auto loaded=registry->loaded.colors.find(key);
        require((!enabled||active==registry->active_provider.colors.end())&&
            loaded==registry->loaded.colors.end(),"Box Color0 named HIT outside finite numeric-MISS profile");
        //Only this genuine completed named MISS permits5808AE→4B9650.
        require((_mm_getcsr()&0xe040u)==0,"Box Color normalization unsupported current SSE mode");
        native_color_load_detail::ColorBits bits{};native_color_load_detail::numeric(request,bits);
        for(const auto word:bits)require(word==0,"Box Color selected numeric4B9650 result is not exact+0");
        full();activation->require_current_owned(*saved->operation,manager->impl_);current();
        auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        require(owner.sequence_<UINT64_MAX&&owner.history_.size()<owner.history_.max_size(),"Box Color journal exhausted");
        const auto sequence=owner.sequence_,history_size=owner.history_.size();
        owner.history_.reserve(history_size+1);full();
        require(owner.sequence_==sequence&&owner.history_.size()==history_size&&
            owner.history_.capacity()>history_size,"Box Color actual Base journal changed across allocation");
        //Nonallocating actual sameBox84/88/8C/90 stores, then own journal row.
        for(std::size_t i=0;i<4;++i)box->color_84_90[i]=std::bit_cast<float>(bits[i]);
        owner.history_.push_back({sequence+1,NotificationLayoutBaseline::Mutation::parser});owner.sequence_=sequence+1;
        box->color_stored=true;box->color_source_serial=saved->serial;
        full();require(box->color_stored&&box->color_source_serial==saved->serial&&owner.sequence_==sequence+1&&
            owner.history_.size()==history_size+1,"Box Color actual prefix store changed across full POST");
        for(const auto v:box->color_84_90)require(std::bit_cast<std::uint32_t>(v)==0,"Box Color actual stored VALUE changed across POST");
        //Caller must still execute HideBox/common. Retained stores on any
        //later failure do not issue parser/initializer completion.
    }catch(...){box->failed=true;fail_storage_without_callbacks(saved);throw;}
}
