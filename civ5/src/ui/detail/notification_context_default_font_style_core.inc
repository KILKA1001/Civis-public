// Inside civ5::ui after complete Core, constructor, canonical Gui and Realm.
void BnwWorldViewControlTree::Core::read_context_default_font_style_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    using Effect=ContextDefaultFontStyle::Effect;
    layout_origin(true);operation.require_origin_without_callbacks();
    const auto occurrence=context_receiver_occurrence;
    require(bool(occurrence),"Context5D2920 genuine root receiver occurrence absent");
    const auto first=[&]{
        layout_origin(true);operation.require_origin_without_callbacks();
        require(context_default_font_style.creator==std::this_thread::get_id()&&
            context_default_font_style.allocation==this&&context_construction_return&&
            context_layout_baseline&&!context_layout_baseline->parsed_source_serial_&&
            context_receiver_occurrence.get()==occurrence.get()&&!context_receiver_occurrence.owner_before(occurrence)&&
            !occurrence.owner_before(context_receiver_occurrence)&&occurrence->id==0&&occurrence->instance==0&&
            occurrence->constructor_serial==operation.operation_serial()&&
            &source->current_operation_loan()==&operation,
            "Context5D2920 real constructor/root parser/Source changed");
    };
    first();require(!context_default_font_style.read_source_serial&&!context_default_font_style.reading&&
        context_default_font_style.value.empty()&&!context_default_font_style.font_538&&
        context_default_font_style.effect_count==3,"Context5D2920 root defaults replay");
    context_default_font_style.read_occurrence=occurrence;
    context_default_font_style.reading=true;
    struct Exit{bool& v;~Exit(){v=false;}}exit{context_default_font_style.reading};
    try{
        operation.require_current();first();
        // The unfiltered actual selected XML establishes local declarations;
        // root_authored intentionally retains only LuaContext children.
        const auto& selected=source->authored_layout(context);first();
        require(selected.name=="Context"&&std::none_of(selected.children.begin(),selected.children.end(),
            [](const auto& child){return child.name=="ColorSet";}),
            "Context5D2920 reached local ColorSet declaration requires genuine local registration");
        require(selected_context_color_sets&&selected_context_color_sets->values.empty()&&
            same_layout_xml(selected_context_color_sets->selected_context,root_authored),
            "Context5D2920 SAME selected Context local registry changed");
        context_default_font_style.record(Effect::local_declarations_absent);
        const auto attribute=[&](std::string_view name)->const std::string*{
            const auto found=std::find_if(root_authored.attributes.begin(),root_authored.attributes.end(),
                [&](const auto& item){return item.first==name;});
            return found==root_authored.attributes.end()?nullptr:&found->second;
        };
        const auto conversion=[&](const std::string& text){
            require(std::all_of(text.begin(),text.end(),[](unsigned char c){return c>0&&c<0x80;}),
                "Context5D2920 conversion outside proven ASCII branch");
            std::string result=text;first();operation.require_current();first();return result;
        };
        // Native local580 lookup then4A1A90/6354C0. Genuine all-global MISS
        // leaves destination53C unchanged; it never substitutes a guessedset.
        if(const auto value=attribute("ColorSet")){
            const auto request=conversion(*value);first();
            const auto gui=context_gui_port;require(bool(gui),"Context ColorSet canonical Gui absent");
            const auto storage=gui->impl_;storage->first(operation);first();
            const auto registry=storage->selected_color_sets;
            const auto activation=storage->selected_provider_activation;
            require(registry&&activation,"Context ColorSet genuine registry/activation setup absent");
            const auto pure=[&]{
                first();storage->first(operation);first();registry->first();first();activation->first(storage);first();
                require(context_gui_port.get()==gui.get()&&!context_gui_port.owner_before(gui)&&!gui.owner_before(context_gui_port)&&
                    gui->impl_.get()==storage.get()&&!gui->impl_.owner_before(storage)&&!storage.owner_before(gui->impl_)&&
                    storage->selected_color_sets.get()==registry.get()&&!storage->selected_color_sets.owner_before(registry)&&
                    !registry.owner_before(storage->selected_color_sets)&&storage->selected_provider_activation.get()==activation.get()&&
                    !storage->selected_provider_activation.owner_before(activation)&&!activation.owner_before(storage->selected_provider_activation),
                    "Context ColorSet canonical owners changed");
            };
            pure();activation->require_current_owned(operation,storage);pure();
            const bool enabled=activation->gameplay_enabled_without_callbacks(storage);pure();
            const auto current=operation.color_atlas_resources_owned();pure();
            for(std::size_t i=0;i<current.size();++i){const auto& quote=current[i];const auto& expected=registry->atlas[i];
                require(quote.payload_identity.sha256==expected.payload_identity.sha256&&quote.bytes.get()==expected.bytes.get()&&
                    !quote.bytes.owner_before(expected.bytes)&&!expected.bytes.owner_before(quote.bytes)&&quote.xml.get()==expected.xml.get()&&
                    !quote.xml.owner_before(expected.xml)&&!expected.xml.owner_before(quote.xml),"Context ColorSet SAME selected Bank quote changed");}
            pure();const auto key=native_color_load_detail::hash_query(request);pure();
            const detail::NotificationLabelColorSetKnownBytes* found=nullptr;
            const auto local=selected_context_color_sets->values.find(key);
            if(local!=selected_context_color_sets->values.end())found=&local->second;
            else{
                if(enabled){const auto active=registry->active_provider.sets.find(key);
                    if(active!=registry->active_provider.sets.end())found=&active->second;}
                if(!found){const auto base=registry->loaded.sets.find(key);if(base!=registry->loaded.sets.end())found=&base->second;}
            }
            pure();if(found){
                // Native16 DWORD copy without allocation; unknown3F propagates.
                for(std::size_t word=0;word<16;++word)for(std::size_t byte=0;byte<4;++byte)
                    context_default_font_style.colors_53c[word*4+byte]=(*found)[word*4+byte];
                context_default_font_style.record(Effect::root_colorset_copy);
            }else context_default_font_style.record(Effect::root_colorset_miss);
            pure();operation.require_current();pure();
        }else context_default_font_style.record(Effect::root_colorset_absent);
        // The selected WorldView/UnitList roots contain no direct Color0..3.
        // Stop on reached unported stores BEFORE Font538/FontStyle534.
        for(const auto name:std::array<std::string_view,4>{"Color0","Color1","Color2","Color3"})
            require(!attribute(name),"Context5D2920 direct default Color store requires separate real palette setter");
        context_default_font_style.record(Effect::direct_colors_absent);first();
        if(const auto value=attribute("Font")){
            const auto request=conversion(*value);first();
            const auto gui=context_gui_port;require(bool(gui),"Context Font538 actual canonical Gui absent");
            gui->require_current_owned(operation);first();
            // Root id0 is the sole genuine caller; no Runtime/String DTO issuer.
            auto font=gui->begin_font_lookup_owned(operation,std::string_view(request));first();
            require(bool(font),"Context Font538 loader did not return actual typed completed Font");
            font->require_current_without_callbacks();first();font->require_source_owned(operation);first();
            require(context_gui_port.get()==gui.get()&&!context_gui_port.owner_before(gui)&&!gui.owner_before(context_gui_port)&&
                !context_default_font_style.font_538,"Context Font538 canonical owner changed during lookup");
            context_default_font_style.record(Effect::font_lookup_return);
            context_default_font_style.font_538=std::move(font); // physical owned nonalloc store
            context_default_font_style.record(Effect::font_store);first();
            context_default_font_style.font_538->require_current_without_callbacks();first();
            operation.require_current();first();context_default_font_style.font_538->require_current_without_callbacks();first();
        }else context_default_font_style.record(Effect::font_absent);
        // Only after actual reached prefix completes:5D2E6F 409390 to534.
        if(const auto value=attribute("FontStyle")){
            auto staged=conversion(*value);first();
            context_default_font_style.value.swap(staged); // nonalloc, includes genuineempty
            context_default_font_style.record(Effect::style_store);
        }else context_default_font_style.record(Effect::style_absent);
        first();operation.require_current();first();
        if(context_default_font_style.font_538){context_default_font_style.font_538->require_current_without_callbacks();first();}
        context_default_font_style.record(Effect::read_return);
        context_default_font_style.read_source_serial=operation.operation_serial();first();
    }catch(...){failed.store(true);throw;}
}
void BnwWorldViewControlTree::Core::require_context_default_font_owned(){
    layout_origin(true);
    if(context_default_font_style.font_538){context_default_font_style.font_538->require_current_without_callbacks();layout_origin(true);}
}
BnwWorldViewControlTree::Core::ContextDefaultFontStyle::Known
BnwWorldViewControlTree::Core::default_context_colors_owned(const runtime::BnwWorldViewSourceOperationLoan& operation){
    const auto first=[&]{layout_origin(true);operation.require_origin_without_callbacks();
        const auto occurrence=context_default_font_style.read_occurrence.lock();
        require(occurrence&&context_receiver_occurrence.get()==occurrence.get()&&
            !context_receiver_occurrence.owner_before(occurrence)&&!occurrence.owner_before(context_receiver_occurrence)&&
            occurrence->id==0&&occurrence->instance==0&&occurrence->constructor_serial==context_default_font_style.read_source_serial&&
            context_construction_return&&context_default_font_style.allocation==this&&
            context_default_font_style.creator==std::this_thread::get_id()&&!context_default_font_style.reading&&
            context_layout_baseline&&context_layout_baseline->parsed_source_serial_.has_value()&&
            context_default_font_style.read_source_serial==*context_layout_baseline->parsed_source_serial_&&
            context_default_font_style.read_source_serial&&context_default_font_style.effect_count&&
            context_default_font_style.effect_count<=context_default_font_style.effects.size()&&
            context_default_font_style.effects[context_default_font_style.effect_count-1]==ContextDefaultFontStyle::Effect::read_return&&
            &source->current_operation_loan()==&operation,
            "Context default53C actual constructor/root prefix/Source changed");
        require_context_default_font_owned();layout_origin(true);};
    first();const auto result=context_default_font_style.colors_53c;first();return result;
}
std::string detail::NotificationLayoutContextRealmRef::default_font_style_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto saved=impl_;saved->first(operation);
    auto actual=saved->context.lock();require(bool(actual),"Context534 actual owner retired");
    const auto first=[&]{
        saved->first(operation);
        const auto occurrence=actual->context_default_font_style.read_occurrence.lock();
        require(occurrence&&actual->context_receiver_occurrence.get()==occurrence.get()&&
            !actual->context_receiver_occurrence.owner_before(occurrence)&&!occurrence.owner_before(actual->context_receiver_occurrence)&&
            occurrence->id==0&&occurrence->instance==0&&occurrence->constructor_serial==actual->context_default_font_style.read_source_serial&&
            actual->context_realm_identity.get()==this&&actual->context_realm_identity->impl_.get()==saved.get()&&
            actual->context_default_font_style.creator==std::this_thread::get_id()&&
            actual->context_default_font_style.allocation==actual.get()&&!actual->context_default_font_style.reading&&
            actual->context_layout_baseline&&actual->context_layout_baseline->parsed_source_serial_.has_value()&&
            *actual->context_layout_baseline->parsed_source_serial_!=0&&
            actual->context_default_font_style.read_source_serial==*actual->context_layout_baseline->parsed_source_serial_&&
            actual->context_default_font_style.effect_count&&
            actual->context_default_font_style.effect_count<=actual->context_default_font_style.effects.size()&&actual->context_default_font_style.effects[
                actual->context_default_font_style.effect_count-1]==BnwWorldViewControlTree::Core::ContextDefaultFontStyle::Effect::read_return,
            "Context534 actual full reached root read/realm publication missing");
        actual->require_context_default_font_owned();saved->first(operation);
    };
    try{
        first();operation.require_current();first();
        std::string result=actual->context_default_font_style.value;
        first();operation.require_current();first();return result;
        // DATA only: no retired Runtime/ArgumentLoan or wholeLabel/frame grant.
    }catch(...){saved->failed.store(true);actual->failed.store(true);throw;}
}
