// After complete actual Button String/FactoryReturn/ordinary Context Realm.
detail::NotificationLabelColorSetKnownBytes detail::NotificationLayoutRuntimeDimensions::label_color_set_owned(std::string_view input)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    try{
        const auto layer=saved->active_button_string_layer_;
        require(bool(layer),"Label ColorSet getter outside actual String lexical occurrence");
        require_button_string_layer_owned(saved,layer,layer->index_);
        const auto argument=context_realm_argument_owned();require_context_argument_owned(saved,argument);
        require(saved->baseline->button_&&saved->baseline->button_->label_178_&&saved->baseline->button_->label_178_factory_return_,
            "Label ColorSet getter needs actual published child factory");
        const auto returned=saved->baseline->button_->label_178_factory_return_;
        require_label_factory_return_owned(saved,returned,layer->index_);
        const std::string request(input);native_color_load_detail::bounded(request);
        require_label_factory_return_owned(saved,returned,layer->index_);
        saved->operation->require_current();require_label_factory_return_owned(saved,returned,layer->index_);
        require_button_string_layer_owned(saved,layer,layer->index_);require_context_argument_owned(saved,argument);
        const auto& actual_layer=*layer->reached_layer_;
        require(std::any_of(actual_layer.attributes.begin(),actual_layer.attributes.end(),[&](const auto& a){return a.first=="ColorSet"&&a.second==request;}),
            "Label ColorSet selector differs from SAME actual reached XML attribute");
        std::weak_ptr<const BnwWorldViewControlTree::Core::SelectedContextColorSets> local_key;
        std::weak_ptr<NotificationContextGuiPort::Impl::SelectedColorSetRegistry> global_key;
        {const auto actual=saved->core.lock();const auto manager=saved->manager.lock();
            require(actual&&manager&&actual->selected_context_color_sets&&manager->impl_->selected_color_sets,
                "ColorSet lacks actual Context/local and Gui/global registry setup");
            manager->impl_->selected_color_sets->first();manager->impl_->first(*saved->operation);
            local_key=actual->selected_context_color_sets;global_key=manager->impl_->selected_color_sets;}
        const auto pure=[&]{
            require_storage_without_callbacks(saved);require_label_factory_return_owned(saved,returned,layer->index_);
            require_button_string_layer_owned(saved,layer,layer->index_);require_context_argument_owned(saved,argument);
            const auto local=local_key.lock();const auto global=global_key.lock();const auto actual=saved->core.lock();const auto manager=saved->manager.lock();
            require(local&&global&&actual&&manager,"ColorSet actual registry allocation retired");global->first();manager->impl_->first(*saved->operation);
            require(local->construction_serial&&global->construction_serial&&actual->selected_context_color_sets.get()==local.get()&&
                !actual->selected_context_color_sets.owner_before(local)&&!local.owner_before(actual->selected_context_color_sets)&&
                manager->impl_->selected_color_sets.get()==global.get()&&!manager->impl_->selected_color_sets.owner_before(global)&&
                !global.owner_before(manager->impl_->selected_color_sets)&&
                BnwWorldViewControlTree::Core::same_layout_xml(local->selected_context,actual->root_authored),
                "ColorSet SAME Context/Gui/Realm allocation changed");
        };
        pure();saved->operation->require_current();pure();
        const auto current=saved->operation->color_atlas_resources_owned();pure();
        detail::NotificationLabelColorSetKnownBytes result;bool global_needed=false;
        const auto key=native_color_load_detail::hash_query(request); //67C5C0 C-prefix CRC, no trim/case-fold.
        {const auto local=local_key.lock();const auto global=global_key.lock();require(local&&global,"ColorSet actual owner expired before lookup");global->first();
            for(std::size_t i=0;i<current.size();++i){const auto& actual=current[i];const auto& expected=global->atlas[i];
                require(actual.payload_identity.sha256==expected.payload_identity.sha256&&actual.bytes.get()==expected.bytes.get()&&
                    !actual.bytes.owner_before(expected.bytes)&&!expected.bytes.owner_before(actual.bytes)&&actual.xml.get()==expected.xml.get()&&
                    !actual.xml.owner_before(expected.xml)&&!expected.xml.owner_before(actual.xml),"ColorSet SAME selected Bank ColorAtlas quote changed");}
            const auto found_local=local->values.find(key);
            if(found_local!=local->values.end())result=found_local->second;else global_needed=true;
        }
        if(global_needed){
            const auto manager=saved->manager.lock();require(bool(manager),"ColorSet actual Gui retired before activation");
            const auto activation=manager->impl_->selected_provider_activation;
            require(bool(activation),"ColorSet required Gameplay startup has not returned");
            pure();activation->require_current_owned(*saved->operation,manager->impl_);pure();
            const bool enabled=activation->gameplay_enabled_without_callbacks(manager->impl_);
            {const auto local=local_key.lock();const auto global=global_key.lock();require(local&&global,"ColorSet actual owner expired after activation");global->first();
                // Only earned Gameplay state0 admits the active map. A genuinely
                // disabled completed state is MISS; missing required setup STOPs.
                const auto found_active=global->active_provider.sets.find(key);
                if(enabled&&found_active!=global->active_provider.sets.end())result=found_active->second;
                else {const auto found_base=global->loaded.sets.find(key);
                    if(found_base!=global->loaded.sets.end())result=found_base->second;
                    else {const auto actual=saved->core.lock();require(bool(actual),"ColorSet default53C actual Context retired");
                        pure();result=actual->default_context_colors_owned(*saved->operation);pure();}}
            }
            pure();saved->operation->require_current();pure();
            activation->require_current_owned(*saved->operation,manager->impl_);pure();
        }else {pure();saved->operation->require_current();pure();}
        // Lookup owner locks ended before full POST; no renewed paint/Label loan.
        pure();return result;
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
