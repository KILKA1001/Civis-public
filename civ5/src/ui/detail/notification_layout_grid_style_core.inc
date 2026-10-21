// After complete canonical Core/Gui/Runtime State. DISTINCT Grid v44 adapter;
// no Button method can admit this receiver.
void detail::NotificationLayoutRuntimeDimensions::require_grid_parser_owned(
    const NotificationLayoutGridState* grid)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    require(saved->id&&saved->baseline->actual_type_=="Grid"&&saved->baseline->grid_&&
        saved->baseline->grid_.get()==grid&&!saved->baseline->box_&&
        saved->baseline->parser_runtime_dimensions_==this&&!saved->baseline->parsed_source_serial_&&
        grid->parser_attempted&&!grid->failed,"Grid parser requires SAME actual constructor/occurrence");
    const auto argument=context_realm_argument_owned();require_context_argument_owned(saved,argument);
}
std::optional<std::size_t> detail::NotificationLayoutRuntimeDimensions::run_grid_style_prefix_owned(
    const content::XmlNode& layer,std::size_t ordinal)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto grid=saved->baseline->grid_;require_grid_parser_owned(grid.get());
    const auto argument=context_realm_argument_owned();require_context_argument_owned(saved,argument);
    const auto pure=[&]{
        require_grid_parser_owned(grid.get());require_context_argument_owned(saved,argument);
        require(ordinal<saved->baseline->parser_layers_.size()&&
            BnwWorldViewControlTree::Core::same_layout_xml(saved->baseline->parser_layers_[ordinal],layer),
            "Grid Style source is not its retained SAME actual layer");
    };
    const auto full=[&]{pure();saved->operation->require_operation(saved->serial);pure();};
    try{
        full();
        if(!saved->grid_style_loan){
            std::shared_ptr<const BnwWorldViewControlTree::Core::SelectedContextStyles> local;
            std::shared_ptr<NotificationContextGuiPort::Impl::SelectedStyleRegistry> global;
            {const auto core=saved->core.lock();const auto gui=saved->manager.lock();
                require(core&&gui&&core->selected_context_styles&&gui->impl_->selected_styles,
                    "Grid Style actual Context/Gui registries missing");
                local=core->selected_context_styles;global=gui->impl_->selected_styles;}
            auto staged=std::make_unique<State::GridStylePrefixLoan>(local,global,argument,saved->serial);
            full();saved->grid_style_loan=std::move(staged);pure();
        }
        auto& loan=*saved->grid_style_loan;
        const auto owners=[&]{
            pure();const auto local=loan.local.lock();const auto global=loan.global.lock();
            const auto core=saved->core.lock();const auto gui=saved->manager.lock();
            require(local&&global&&core&&gui,"Grid Style actual owners retired");
            global->first();gui->impl_->first(*saved->operation);
            require(loan.parser_serial==saved->serial&&loan.argument.get()==argument.get()&&
                !loan.argument.owner_before(argument)&&!argument.owner_before(loan.argument)&&
                core->selected_context_styles.get()==local.get()&&
                !core->selected_context_styles.owner_before(local)&&!local.owner_before(core->selected_context_styles)&&
                gui->impl_->selected_styles.get()==global.get()&&
                !gui->impl_->selected_styles.owner_before(global)&&!global.owner_before(gui->impl_->selected_styles)&&
                local->construction_serial&&BnwWorldViewControlTree::Core::same_layout_xml(local->selected_context,core->root_authored)&&
                gui->impl_->selected_provider_activation,
                "Grid Style actual local/global/control/source receipt changed");
            gui->impl_->selected_provider_activation->first(gui->impl_);
        };
        owners();require(!loan.busy,"Grid Style callback reentry");
        const auto inserted=loan.entered.insert(ordinal).second;owners();full();owners();
        require(inserted,"Grid Style per-layer v44 prefix replay");
        loan.busy=true;struct End{bool& value;~End(){value=false;}}end{loan.busy};
        const auto style=std::find_if(layer.attributes.begin(),layer.attributes.end(),
            [](const auto& item){return item.first=="Style";});
        if(style==layer.attributes.end()){owners();return std::nullopt;}
        const auto name=style->second;owners();full();owners(); //owned XML DATA, not renewed authority
        {const auto gui=saved->manager.lock();require(bool(gui),"Grid selected Gui retired");
            gui->impl_->selected_provider_activation->require_current_owned(*saved->operation,gui->impl_);}
        owners();const auto current=saved->operation->selected_style_resources_owned();owners();full();owners();
        std::optional<content::XmlNode> resolved;
        {const auto local=loan.local.lock();const auto global=loan.global.lock();const auto gui=saved->manager.lock();
            require(local&&global&&gui,"Grid Style owners expired at lookup");
            for(std::size_t i=0;i<current.size();++i){const auto& expected=global->sheets[i];const auto& now=current[i];
                require(now.payload_identity.sha256==expected.payload_identity.sha256&&now.bytes.get()==expected.bytes.get()&&
                    !now.bytes.owner_before(expected.bytes)&&!expected.bytes.owner_before(now.bytes)&&now.xml.get()==expected.xml.get()&&
                    !now.xml.owner_before(expected.xml)&&!expected.xml.owner_before(now.xml),
                    "Grid Style SAME selected Bank/VFS/DLC payload/control changed");}
            const auto found=local->values.find(name);
            if(found!=local->values.end())resolved=found->second;
            else if(gui->impl_->selected_provider_activation->gameplay_enabled_without_callbacks(gui->impl_))
                resolved=global->lookup_global(name);
            else{const auto base=global->providers[0].find(name);if(base!=global->providers[0].end())resolved=base->second;}}
        owners();full();owners();require(bool(resolved),"Grid exact Style miss requires original diagnostic owner");
        //5C24FB PRESENT/value-get-true skips virtual44. Selected Grid definitions
        //have no own Style; do not guess typed XML value extraction/inheritance.
        require(std::none_of(resolved->attributes.begin(),resolved->attributes.end(),
            [](const auto& item){return item.first=="Style";}),
            "Grid resolved own Style branch needs genuine registry value-get semantics");
        //Copy/append entirely in retained immutable OWN DATA before FULL.
        //Atomic no-allocation swap AFTER Source/owner/layer POST avoids writes
        //through a parser_layers vector invalidated by allocation new_handler.
        auto staged=saved->baseline->parser_layers_;owners();full();owners();
        const auto result=staged.size();staged.push_back(*resolved);owners();full();owners();
        require(result==saved->baseline->parser_layers_.size(),"Grid Style layer count changed across staging");
        for(std::size_t i=0;i<result;++i)require(BnwWorldViewControlTree::Core::same_layout_xml(
            staged[i],saved->baseline->parser_layers_[i]),"Grid retained layer changed across staging");
        const_cast<NotificationLayoutBaseline*>(saved->baseline)->parser_layers_.swap(staged);
        owners();full();owners();return result;
    }catch(...){grid->failed=true;fail_storage_without_callbacks(saved);throw;}
}
void detail::NotificationLayoutRuntimeDimensions::bind_grid_texture_owned(std::size_t ordinal)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto grid=saved->baseline->grid_;require_grid_parser_owned(grid.get());
    const auto pure=[&]{require_grid_parser_owned(grid.get());require(ordinal<saved->baseline->parser_layers_.size(),
        "Grid Texture layer ordinal retired");};
    const auto full=[&]{pure();saved->operation->require_operation(saved->serial);pure();};
    try{
        full();const auto layer=saved->baseline->parser_layers_[ordinal];pure();full();
        const auto found=std::find_if(layer.attributes.begin(),layer.attributes.end(),
            [](const auto& item){return item.first=="Texture";});
        require(found!=layer.attributes.end()&&!found->second.empty()&&found->second.find('\0')==std::string::npos,
            "Grid PRESENT Texture missing/unknown cannot substitute constructorNULL");
        const auto key=found->second;pure();full();
        std::shared_ptr<NotificationLayoutResourceScope> scope;
        {const auto core=saved->core.lock();require(core&&core->layout_resource_bound&&
            core->layout_resource_operation==saved->operation&&core->layout_resource_actual_tree==saved->tree_facade,
            "Grid4B7BD0 needs real SAME entered renderer resource scope");
            scope=core->layout_resource_scope.lock();require(bool(scope),"Grid texture actual resource lender retired");}
        scope->require_pure(*saved->operation,saved->tree_facade);pure();full();
        auto keeper=scope->load_owned(*saved->operation,saved->tree_facade,key);pure();full();
        scope->require_entry(*saved->operation,saved->tree_facade,keeper);pure();full();
        require(BnwWorldViewControlTree::Core::same_layout_xml(layer,saved->baseline->parser_layers_[ordinal]),
            "Grid Texture source layer changed across real load");
        const auto fields=keeper->native_114_118_;require(fields[0]&&fields[1],"Grid texture selected physical extent zero");
        //OWN pending key copy precedes actual+278 publication and last FULL.
        auto owned_key=key;pure();full();
        grid->texture_key.swap(owned_key);grid->texture_keeper=std::move(keeper);
        grid->texture_extent_278=std::array<std::uint32_t,2>{fields[1],fields[0]};
        grid->field_278=NotificationLayoutGridState::Field278::retained_raw;
        grid->texture_binding_earned=true;pure();full();
    }catch(...){grid->failed=true;fail_storage_without_callbacks(saved);throw;}
}
