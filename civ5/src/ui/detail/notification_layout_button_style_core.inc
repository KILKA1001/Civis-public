// After complete Core/Gui/State and ordinary Context realm implementation.
std::optional<std::size_t> detail::NotificationLayoutRuntimeDimensions::run_button_style_prefix_owned(
    const content::XmlNode& layer,std::size_t ordinal)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    require(saved->id&&saved->baseline->actual_type_=="Button"&&saved->baseline->parser_runtime_dimensions_==this&&
        !saved->baseline->parsed_source_serial_,"Style prefix is outside SAME ordinary Button parser");
    const auto argument=context_realm_argument_owned();require_context_argument_owned(saved,argument);
    try{
        {const auto actual=saved->core.lock();const auto manager=saved->manager.lock();
            require(actual&&manager&&actual->selected_context_styles&&manager->impl_->selected_styles,
                "Style prefix has no constructed actual Context/Gui owners");
            manager->impl_->selected_styles->first();manager->impl_->first(*saved->operation);
            if(!saved->button_style_loan)saved->button_style_loan=std::make_unique<State::StylePrefixLoan>(
                actual->selected_context_styles,manager->impl_->selected_styles,argument,saved->serial);}
        auto& loan=*saved->button_style_loan;
        const auto pure=[&]{
            require_storage_without_callbacks(saved);require_context_argument_owned(saved,argument);
            const auto local=loan.local.lock();const auto global=loan.global.lock();
            const auto actual=saved->core.lock();const auto manager=saved->manager.lock();
            require(local&&global&&actual&&manager,"Style loan actual owner retired");global->first();manager->impl_->first(*saved->operation);
            require(loan.parser_serial==saved->serial&&loan.argument.get()==argument.get()&&
                !loan.argument.owner_before(argument)&&!argument.owner_before(loan.argument)&&
                actual->selected_context_styles.get()==local.get()&&
                !actual->selected_context_styles.owner_before(local)&&!local.owner_before(actual->selected_context_styles)&&
                manager->impl_->selected_styles.get()==global.get()&&
                !manager->impl_->selected_styles.owner_before(global)&&!global.owner_before(manager->impl_->selected_styles)&&
                local->construction_serial&&BnwWorldViewControlTree::Core::same_layout_xml(local->selected_context,actual->root_authored)&&
                ordinal<saved->baseline->parser_layers_.size()&&
                BnwWorldViewControlTree::Core::same_layout_xml(saved->baseline->parser_layers_[ordinal],layer),
                "Style loan differs from actual Context/Gui/Realm/layer allocation");
        };
        pure();require(!loan.busy&&loan.entered.insert(ordinal).second,"Style prefix replay/reentry");
        loan.busy=true;struct End{bool& value;~End(){value=false;}}end{loan.busy};
        // Lookup PRESENT is separate from a PRESENT-empty value.
        const auto style=std::find_if(layer.attributes.begin(),layer.attributes.end(),[](const auto& a){return a.first=="Style";});
        if(style==layer.attributes.end()){pure();return std::nullopt;}
        // Strong checker owner locks never cross the full Source callbacks.
        saved->operation->require_current();pure();
        const auto current=saved->operation->selected_style_resources_owned();pure();
        std::optional<content::XmlNode> resolved;
        {const auto local=loan.local.lock();const auto global=loan.global.lock();
            require(local&&global,"Style owner expired at actual lookup");global->first();
            for(std::size_t i=0;i<current.size();++i){const auto& expected=global->sheets[i];const auto& now=current[i];
                require(now.payload_identity.sha256==expected.payload_identity.sha256&&now.bytes.get()==expected.bytes.get()&&
                    !now.bytes.owner_before(expected.bytes)&&!expected.bytes.owner_before(now.bytes)&&now.xml.get()==expected.xml.get()&&
                    !now.xml.owner_before(expected.xml)&&!expected.xml.owner_before(now.xml),"Style selected Bank/provider allocation changed");}
            const auto found=local->values.find(style->second);
            resolved=found==local->values.end()?global->lookup_global(style->second):std::optional<content::XmlNode>{found->second};}
        pure();require(bool(resolved),"Style exact lookup unresolved: genuine diagnostic path required");
        //5C24FB PRESENT/value-get-true skips vslot44. General inherited style
        // startup is not proved: keep strict STOP, never recursively flatten it.
        require(std::none_of(resolved->attributes.begin(),resolved->attributes.end(),[](const auto& a){return a.first=="Style";}),
            "Resolved own Style PRESENT needs separately proved registry semantics");
        auto& actual=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        const auto result=actual.parser_layers_.size();actual.parser_layers_.push_back(*resolved);
        pure();saved->operation->require_current();pure();
        require(result<actual.parser_layers_.size()&&BnwWorldViewControlTree::Core::same_layout_xml(actual.parser_layers_[result],*resolved),
            "Style SAME Button retained definition changed across POST");return result;
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
