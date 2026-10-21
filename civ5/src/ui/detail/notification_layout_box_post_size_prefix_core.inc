// After complete Runtime/Gui/Realm/Registration types; before Box common leaf.
// SAME actual ordinary Box86AC20 return ->43828A/virtual58 ->564300 ID58.
//Then enter real ordinary child factories/attach; unsupported actual subtypes STOP.
void detail::NotificationLayoutRuntimeDimensions::run_box_post_size_prefix_owned(std::size_t ordinal)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();const auto manager=saved->manager.lock();
    const auto occurrence=saved->occurrence.lock();
    require(core&&manager&&occurrence&&saved->id&&core->context=="UnitPanel",
        "Box postSize actual canonical owners absent");
    const auto box=saved->baseline->box_;const auto gui=manager->impl_;
    require(box&&gui,"Box postSize typed constructor/Gui absent");
    const auto registry=gui->selected_color_sets;const auto activation=gui->selected_provider_activation;
    require(registry&&activation,"Box postSize actual Color/Gameplay producer absent");
    const auto argument=context_realm_argument_owned();require_context_argument_owned(saved,argument);
    bool registering=false,gui_started=false,base_started=false;
    std::size_t expected_registrations=0,expected_gui_history=0,expected_base_history=0;
    std::uint64_t expected_gui_revision=0,expected_base_sequence=0;
    const auto pure=[&](bool entered){
        require_storage_without_callbacks(saved);require_context_argument_owned(saved,argument);gui->first(*saved->operation);
        registry->first();require((_mm_getcsr()&0xe040u)==0,"Box postSize unsupported current SSE mode");
        require(saved->baseline->kind_==NotificationLayoutBaseline::Kind::box&&saved->baseline->actual_type_=="Box"&&
            saved->baseline->box_.get()==box.get()&&!saved->baseline->box_.owner_before(box)&&!box.owner_before(saved->baseline->box_)&&
            saved->baseline->parser_runtime_dimensions_==saved->facade&&!saved->baseline->parsed_source_serial_&&
            box->common_querying&&box->post_size_querying==entered&&!box->failed&&!box->common_children_return_&&box->parser_attempted&&
            box->absent_style_returned&&box->color_stored&&box->color_source_serial==saved->serial&&
            box->common_context_stored&&box->common_size_axes==2&&box->common_full_flags_stored&&
            box->frontier==NotificationLayoutBoxState::Frontier::resize_registration_required&&
            saved->registering==registering&&!saved->assigning_context_3c&&!box->color_querying&&
            ordinal==0&&saved->baseline->parser_layers_.size()==1&&gui->selected_color_sets.get()==registry.get()&&
            !gui->selected_color_sets.owner_before(registry)&&!registry.owner_before(gui->selected_color_sets)&&
            gui->selected_provider_activation.get()==activation.get()&&!gui->selected_provider_activation.owner_before(activation)&&
            !activation.owner_before(gui->selected_provider_activation),"Box postSize current subtype/control/phase changed");
        const auto& actual=BnwWorldViewControlTree::Core::node(core->nodes,saved->id);
        require(actual.authored.name=="Box"&&actual.authored.attribute("ID")=="Base","Box postSize selected receiver changed");
        const auto& b=saved->baseline->base_;
        require((b.flags_40&0x300000u)==0x300000u&&b.context_realm_3c.get()==argument->impl_->realm.get()&&
            !b.context_realm_3c.owner_before(argument->impl_->realm)&&!argument->impl_->realm.owner_before(b.context_realm_3c),
            "Box postSize actual Full/Realm VALUE changed");
        const auto dims=manager->dimensions_without_callbacks(*saved->operation);
        for(std::size_t axis=0;axis<2;++axis)require(std::bit_cast<std::uint32_t>(b.size_30_34[axis])==
            std::bit_cast<std::uint32_t>(static_cast<float>(dims[axis])),"Box postSize actual size VALUE changed");
        if(gui_started)require(gui->revision==expected_gui_revision&&gui->history.size()==expected_gui_history&&
            gui->resize_registrations.size()==expected_registrations,"Box postSize actual Gui history changed across allocation/callback");
        if(base_started)require(saved->baseline->sequence_==expected_base_sequence&&
            saved->baseline->history_.size()==expected_base_history,"Box postSize actual Base journal changed");
        if(box->resize_registration_){const auto& entry=box->resize_registration_;
            require(entry->creator==std::this_thread::get_id()&&!entry->failed.load()&&
                entry->id==saved->id&&entry->constructor_serial==occurrence->constructor_serial&&
                entry->registration_source_serial==saved->serial&&entry->registered_flags==b.flags_40&&
                !gui->resize_registrations.empty()&&gui->resize_registrations.back().get()==entry.get()&&
                !gui->resize_registrations.back().owner_before(entry)&&!entry.owner_before(gui->resize_registrations.back()),
                "Box86AC20 actual retained callback entry/control changed");
            const auto ec=entry->core.lock();const auto em=entry->manager.lock();const auto eo=entry->occurrence.lock();
            require(ec.get()==core.get()&&!ec.owner_before(core)&&!core.owner_before(ec)&&
                em.get()==manager.get()&&!em.owner_before(manager)&&!manager.owner_before(em)&&
                eo.get()==occurrence.get()&&!eo.owner_before(occurrence)&&!occurrence.owner_before(eo),
                "Box86AC20 weak canonical receiver identity changed");
        }
        if(box->common_color_stored){for(float v:box->color_84_90)
            require(std::bit_cast<std::uint32_t>(v)==0,"Box common virtual58 stored VALUE changed");}
        if(box->id_58){require(box->id_58->length==4,"Box common ID58 length changed");
            constexpr std::array<char,32> expected{'B','a','s','e'};
            require(box->id_58->bytes==expected&&box->id_58.get()==saved->baseline->base_.id_58.get()&&
                !box->id_58.owner_before(saved->baseline->base_.id_58)&&!saved->baseline->base_.id_58.owner_before(box->id_58),
                "Box common ID58 actual padded bytes/control changed");}
    };
    const auto matches=[&](bool entered){pure(entered);bool id=false,color=false,size=false;
        for(const auto& a:saved->baseline->parser_layers_[ordinal].attributes){
            if(a.first=="ID"){require(!id&&a.second=="Base","Box postSize selected ID changed");id=true;}
            else if(a.first=="Color"){require(!color&&a.second=="0,0,0,0","Box postSize selected Color changed");color=true;}
            else if(a.first=="Size"){require(!size&&a.second=="Full,Full","Box postSize selected Size changed");size=true;}
            else require(false,"Box postSize outside finite selected attribute profile");}
        require(id&&color&&size,"Box postSize actual attribute missing");};
    matches(false);require(!box->resize_registration_&&!box->common_color_stored&&!box->id_58,
        "Box postSize actual registration/commonColor/ID replay");
    box->post_size_querying=true;
    struct End{std::shared_ptr<State> state;std::shared_ptr<NotificationLayoutBoxState> box;
        ~End(){state->registering=false;box->post_size_querying=false;}}end{saved,box};
    const auto current=[&]{matches(true);};
    const auto full=[&]{current();saved->operation->require_current();current();};
    try{
        full();expected_base_sequence=saved->baseline->sequence_;expected_base_history=saved->baseline->history_.size();
        base_started=true;expected_gui_revision=gui->revision;expected_gui_history=gui->history.size();
        expected_registrations=gui->resize_registrations.size();gui_started=true;
        require(expected_gui_revision<UINT64_MAX&&expected_gui_history<gui->history.max_size()&&
            expected_registrations<gui->resize_registrations.max_size(),"Box86AC20 actual Gui journal exhausted");
        registering=true;saved->registering=true;
        gui->resize_registrations.reserve(expected_registrations+1);full();
        gui->history.reserve(expected_gui_history+1);full();
        // Snapshot EVERY shared operand/scalar before object allocation. The
        // overload performs only owned weak/value initialization, no Source read.
        const auto receiver_id=saved->id,ctor_serial=occurrence->constructor_serial,serial=saved->serial;
        const auto flags=saved->baseline->base_.flags_40;
        std::unique_ptr<NotificationLayoutResizeRegistration> pending(new NotificationLayoutResizeRegistration(
            core,manager,occurrence,receiver_id,ctor_serial,serial,flags));full();
        require(pending->creator==std::this_thread::get_id()&&!pending->failed.load(),"Box86AC20 allocated entry failed");
        auto entry=std::shared_ptr<NotificationLayoutResizeRegistration>(std::move(pending));full();
        require(entry&&entry->id==receiver_id&&entry->constructor_serial==ctor_serial&&
            entry->registration_source_serial==serial&&entry->registered_flags==flags&&
            gui->resize_registrations.capacity()>expected_registrations&&gui->history.capacity()>expected_gui_history,
            "Box86AC20 entry VALUE/capacity changed across allocation POST");
        // Nonalloc publication into actual SAME Gui callback list, then OWN
        // history and SAME Box retention. No synthetic callback-return token.
        gui->resize_registrations.push_back(entry);gui->history.push_back({expected_gui_revision+1,serial,
            NotificationContextGuiPort::Impl::Store::resize_registration});++gui->revision;
        box->resize_registration_=entry;++expected_registrations;++expected_gui_history;++expected_gui_revision;full();
        saved->registering=false;registering=false;full();
        // Native Anchor and AnchorSide lookup are both real MISS in this exact
        // layer. Then common Color invokes580780 again, followed by Box+58.
        const std::string request("0,0,0,0");full();
        const auto atlases=saved->operation->color_atlas_resources_owned();full();
        require(atlases.size()==registry->atlas.size(),"Box common Color actual atlas count changed");
        for(std::size_t i=0;i<atlases.size();++i){const auto& now=atlases[i];const auto& old=registry->atlas[i];
            require(now.payload_identity.sha256==old.payload_identity.sha256&&now.bytes.get()==old.bytes.get()&&
                !now.bytes.owner_before(old.bytes)&&!old.bytes.owner_before(now.bytes)&&now.xml.get()==old.xml.get()&&
                !now.xml.owner_before(old.xml)&&!old.xml.owner_before(now.xml),"Box common Color actual Bank atlas changed");}
        activation->require_current_owned(*saved->operation,manager->impl_);current();
        const bool enabled=activation->gameplay_enabled_without_callbacks(manager->impl_);current();
        const auto key=native_color_load_detail::hash_query("0");
        require((!enabled||registry->active_provider.colors.find(key)==registry->active_provider.colors.end())&&
            registry->loaded.colors.find(key)==registry->loaded.colors.end(),"Box common Color named HIT outside selected MISS");
        native_color_load_detail::ColorBits bits{};native_color_load_detail::numeric(request,bits);
        for(auto word:bits)require(word==0,"Box common Color selected numeric result not+0");
        full();activation->require_current_owned(*saved->operation,manager->impl_);current();
        auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        require(owner.sequence_<=UINT64_MAX-2&&owner.history_.max_size()-owner.history_.size()>=2,
            "Box common Color/ID journal exhausted");
        expected_base_sequence=owner.sequence_;expected_base_history=owner.history_.size();base_started=true;
        owner.history_.reserve(expected_base_history+2);full();
        require(owner.history_.capacity()-expected_base_history>=2,"Box common Color/ID journal capacity lost");
        for(std::size_t i=0;i<4;++i)box->color_84_90[i]=std::bit_cast<float>(bits[i]);
        box->common_color_stored=true;owner.history_.push_back({++owner.sequence_,NotificationLayoutBaseline::Mutation::parser});
        ++expected_base_sequence;++expected_base_history;full();
        //564300 selected nonemptyID: actual ctor58NULL ->78B470 object with
        // 32-byte strncpy-padded buffer, then scan length4. Own x64 allocation.
        constexpr std::array<char,32> id_bytes{'B','a','s','e'};
        std::unique_ptr<NotificationLayoutBoxState::ID58> name(new NotificationLayoutBoxState::ID58{id_bytes,4});full();
        auto id=std::shared_ptr<const NotificationLayoutBoxState::ID58>(std::move(name));full();
        require(id&&id->bytes==id_bytes&&id->length==4&&!box->id_58&&
            owner.history_.capacity()>expected_base_history,"Box ID58 operand/current capacity changed across allocation");
        owner.base_.id_58=id;box->id_58=id;owner.history_.push_back({++owner.sequence_,NotificationLayoutBaseline::Mutation::parser});
        ++expected_base_sequence;++expected_base_history;full();
        //All scalar/tooltip attributes afterID are absent in this exact layer.
        //Close the prefix before recursive factories can legitimately change
        //Gui history or child state. The pre-children snapshots do not renew.
        full();box->frontier=NotificationLayoutBoxState::Frontier::common_child_factory_required;
        run_ordinary_common_children_owned(owner.parser_layers_[ordinal]);
        require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
        require(owner.base_.common_return_owned&&!box->common_children_return_,"Box genuine common child return absent/replayed");
        box->common_children_return_=owner.base_.common_return_owned;
        box->frontier=NotificationLayoutBoxState::Frontier::common_initializer_required;
        require_box_common_return_from_parser_owned();
    }catch(...){box->failed=true;fail_storage_without_callbacks(saved);throw;}
}
