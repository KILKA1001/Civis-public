//Inside actual Core TU after closed Frame and Runtime State; Image only, no Base alias.
void detail::NotificationLayoutRuntimeDimensions::publish_image_constructor_owned(
    const std::shared_ptr<NotificationLayoutImageState>& image)const {
    const auto saved=state_;
    try{
        require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
        auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        require(owner.kind_==NotificationLayoutBaseline::Kind::image&&owner.actual_type_=="Image"&&
            owner.parser_runtime_dimensions_==this&&!owner.image_&&!owner.base_.receiver_owned&&image,
            "Image5E4D70 actual embedded constructor replay/subtype mismatch");
        const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
        require(core&&occurrence,"Image actual constructor receiver retired");
        auto fields=std::make_shared<NotificationLayoutOrdinaryReceiver::Impl>(core,occurrence,image,saved->serial);
        require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
        auto receiver=std::shared_ptr<const NotificationLayoutOrdinaryReceiver>(new NotificationLayoutOrdinaryReceiver(fields));
        require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
        require(!owner.image_&&!owner.base_.receiver_owned&&fields->image.lock()==image,
            "Image constructor actual receiver changed across allocation POST");
        owner.history_.reserve(owner.history_.size()+16);
        require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
        fields->canonical=receiver;owner.image_=image;owner.base_.receiver_owned=receiver;
        require_storage_without_callbacks(saved);
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::load_image_texture_before_common_owned(const content::XmlNode& layer)const {
    const auto saved=state_;
    try{
        require_storage_without_callbacks(saved);
        auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        require(owner.kind_==NotificationLayoutBaseline::Kind::image&&owner.image_&&owner.base_.receiver_owned&&
            owner.parser_runtime_dimensions_==this&&owner.parser_layers_.size()==1,
            "Image701B10 needs actual Image constructor and selected absent-Style parser");
        const auto core=saved->core.lock();const auto xml=BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence;
        require(xml&&!xml->dynamic&&xml->value.name=="Image"&&xml->value.attributes==layer.attributes,
            "Image701B10 actual selected Source occurrence differs");
        for(const auto& [key,value]:layer.attributes)
            require(key!="Style"&&key!="MaskTexture"&&key!="StretchMode","Image native PRESENT subtype branch not implemented");
        for(const auto& [key,value]:layer.attributes)if(key=="Size"||key=="Offset"){
            //The reached selected pair is integral76DC50; broader scalar/Full branches need their own closure.
            const auto split=value.find(',');
            require(split!=std::string::npos&&value.find(',',split+1)==std::string::npos,
                "Image selected common pair requires two comma-delimited native integers");
            for(const auto token:{std::string_view(value).substr(0,split),std::string_view(value).substr(split+1)}){
                std::int32_t number=0;const auto parsed=std::from_chars(token.data(),token.data()+token.size(),number);
                require(!token.empty()&&parsed.ec==std::errc{}&&parsed.ptr==token.data()+token.size()&&
                    number>=-16777216&&number<=16777216,"Image common tuple outside proven integral exact-float bounds");
            }
        }
        const auto found=std::find_if(layer.attributes.begin(),layer.attributes.end(),[](const auto& row){return row.first=="Texture";});
        require(found!=layer.attributes.end()&&!found->second.empty()&&found->second.find('\0')==std::string::npos,
            "Image native no-Texture path leaves E8/EC unobserved; unsupported constructor NULL suffix");
        const auto image=owner.image_;std::string key=found->second;
        require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
        const auto scope=core->layout_resource_scope.lock();const auto tree=core->layout_resource_tree.lock();
        require(scope&&tree&&tree.get()==saved->tree_facade&&core->layout_resource_bound&&
            core->layout_resource_actual_tree==saved->tree_facade&&core->layout_resource_operation==saved->operation&&
            core->layout_resource_serial==saved->serial,"Image4B7BD0 genuine entered ResourceScope absent");
        scope->require_full(*saved->operation,saved->tree_facade);require_storage_without_callbacks(saved);
        const auto keeper=scope->load_owned(*saved->operation,saved->tree_facade,key);
        require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
        scope->require_full(*saved->operation,saved->tree_facade);require_storage_without_callbacks(saved);
        scope->require_entry(*saved->operation,saved->tree_facade,keeper);require_storage_without_callbacks(saved);
        require(keeper&&keeper->key_==key&&keeper->native_114_118_[0]&&keeper->native_114_118_[1]&&
            keeper->native_114_118_[0]<=INT32_MAX&&keeper->native_114_118_[1]<=INT32_MAX&&owner.image_==image&&
            image->texture_origin==NotificationLayoutImageState::TextureOrigin::constructor_null,
            "Image real resource keeper/extents or original constructor identity changed");
        //Real701B72 C8 pointer store. E8/EC are stored separately after TextureOffset/Sampler.
        image->texture_key.swap(key);image->texture_keeper=keeper;
        image->texture_extent_c8=keeper->native_114_118_;
        image->texture_origin=NotificationLayoutImageState::TextureOrigin::retained_raw;
        require_storage_without_callbacks(saved);
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::store_image_extent_before_common_owned()const {
    const auto saved=state_;
    try{
        require_storage_without_callbacks(saved);
        auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        require(owner.image_&&owner.base_.receiver_owned&&owner.parser_runtime_dimensions_==this&&
            owner.image_->texture_origin==NotificationLayoutImageState::TextureOrigin::retained_raw&&
            owner.image_->texture_extent_c8,"Image701CAD actual nonNULL C8 keeper missing");
        const auto fields=*owner.image_->texture_extent_c8;
        //701CBD width118→E8, then701CD9 height114→EC, after actual TextureOffset/Sampler and before common.
        owner.image_->resource_e8_ec={static_cast<float>(fields[1]),static_cast<float>(fields[0])};
        require_storage_without_callbacks(saved);
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::store_image_common_id_58_owned(const content::XmlNode& layer)const {
    const auto saved=state_;
    try{
        require_storage_without_callbacks(saved);
        auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        require(owner.kind_==NotificationLayoutBaseline::Kind::image&&owner.image_&&owner.base_.receiver_owned&&
            owner.parser_runtime_dimensions_==this&&!owner.base_.common_return_owned,"Image common ID58 wrong actual receiver/phase");
        const auto found=std::find_if(layer.attributes.begin(),layer.attributes.end(),[](const auto& row){return row.first=="ID";});
        if(found==layer.attributes.end())return; //Original409390 genuine MISS, preserves constructor NULL.
        const auto& text=found->second;
        require(text.size()<32&&std::all_of(text.begin(),text.end(),[](unsigned char c){return c>0&&c<128;}),
            "Image564300 only proven terminated selected ASCII ID shorter than32 supported");
        if(text.empty()){owner.base_.id_58.reset();return;} //564351 real release+NULL branch.
        require(!owner.base_.id_58,"Image selected ID58 update needs separate mutable buffer owner");
        std::array<char,32> bytes{};std::copy(text.begin(),text.end(),bytes.begin());const auto size=static_cast<std::uint32_t>(text.size());
        auto id=std::shared_ptr<const NotificationLayoutControlID58>(new NotificationLayoutControlID58(bytes,size));
        require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
        require(!owner.base_.id_58&&id->bytes==bytes&&id->length==size,"Image ID58 changed across allocator POST");
        owner.base_.id_58=std::move(id);require_storage_without_callbacks(saved);
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::run_image_common_children_owned(const content::XmlNode& layer)const {
    run_ordinary_common_children_owned(layer);
}

void detail::NotificationLayoutRuntimeDimensions::run_ordinary_common_children_owned(const content::XmlNode& layer)const {
    const auto saved=state_;
    try{
        //Generic ordinary common suffix starts after the actual subtype prefix.
        require_storage_without_callbacks(saved);
        auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
        const auto xml=BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence;
        const auto receiver=owner.base_.receiver_owned;const auto image=owner.image_;const auto box=owner.box_;
        const auto serial=saved->serial;
        std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn> published_return{};
        const bool actual_image=owner.kind_==NotificationLayoutBaseline::Kind::image&&owner.actual_type_=="Image"&&image&&!box;
        const bool actual_box=owner.kind_==NotificationLayoutBaseline::Kind::box&&owner.actual_type_=="Box"&&box&&!image;
        const auto current=[&]{require_storage_without_callbacks(saved);
            require(core&&occurrence&&receiver&&receiver->impl_&&
                owner.base_.receiver_owned.get()==receiver.get()&&!owner.base_.receiver_owned.owner_before(receiver)&&
                !receiver.owner_before(owner.base_.receiver_owned)&&owner.parser_runtime_dimensions_==this&&
                receiver->impl_->creator==std::this_thread::get_id()&&receiver->impl_->core.lock()==core&&
                receiver->impl_->occurrence.lock()==occurrence&&receiver->impl_->canonical.lock()==receiver&&
                receiver->impl_->id==saved->id&&receiver->impl_->serial==serial&&
                ((actual_image&&owner.kind_==NotificationLayoutBaseline::Kind::image&&owner.actual_type_=="Image"&&
                    owner.image_==image&&!owner.box_&&receiver->impl_->image.lock()==image&&receiver->impl_->box.expired()&&
                    image->texture_origin==NotificationLayoutImageState::TextureOrigin::retained_raw)||
                 (actual_box&&owner.kind_==NotificationLayoutBaseline::Kind::box&&owner.actual_type_=="Box"&&
                    owner.box_==box&&!owner.image_&&receiver->impl_->box.lock()==box&&receiver->impl_->image.expired()&&
                    !box->failed&&box->parser_attempted&&box->absent_style_returned&&box->color_stored&&
                    box->color_source_serial==serial&&box->common_context_stored&&box->common_size_axes==2&&
                    box->common_full_flags_stored&&box->resize_registration_&&box->common_color_stored&&
                    box->id_58&&box->id_58==owner.base_.id_58&&!box->common_children_return_&&
                    box->frontier==NotificationLayoutBoxState::Frontier::common_child_factory_required)),
                "Ordinary43875E actual constructor/prefix/control/phase changed");
            require(xml&&!xml->dynamic&&xml->value.attributes==layer.attributes&&owner.parser_layers_.size()==1&&
                BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence==xml&&
                owner.base_.common_return_owned.get()==published_return.get()&&
                !owner.base_.common_return_owned.owner_before(published_return)&&!published_return.owner_before(owner.base_.common_return_owned),
                "Ordinary43875E actual selected XML/return changed");
        };
        const auto full=[&]{current();saved->operation->require_current();current();};
        full();require(!owner.base_.children_78,"Ordinary common children78 replay");
        //True Image/Box constructor branches only. Native wrapper effects cannot be inferred from flat nodes.
        for(const auto& child:xml->value.children)
            require(child.name!="ShowOnMouseOver"&&child.name!="HideOnMouseOver","Image mouse-over wrapper actual producer required");
        //Both genuine wrapper MISS preserve flags:43860B→4386AA and4386BF→43875E.
        //OR8000 occurs only inside entered wrapper child loops, never on this absent branch.
        auto fields=std::make_shared<NotificationLayoutOrdinaryChildren78::Impl>(receiver);full();
        auto array=std::shared_ptr<NotificationLayoutOrdinaryChildren78>(new NotificationLayoutOrdinaryChildren78(fields));full();
        require(!owner.base_.children_78,"Image actual children78 appeared during constructor POST");
        owner.base_.children_78=array; //43875E→495E10 actual zero-length array object, even children0.
        const auto count=xml->value.children.size();
        //Port the8355540/835640/8356A0 temporary cursor as lexical OWN selected XML retention.
        struct Cursor final {
            std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
            std::size_t next=0;
            void retire_owned(){selected.reset();} //8353F0 only drops temporary XML handle, not actual Source.
        } cursor{xml};
        fields->ordered.reserve(count);full(); //835620(1)→5E51D0 real direct count and reserve; no ready shortcut.
        require(fields->ordered.empty()&&fields->ordered.capacity()>=count,"Image children78 changed across reserve POST");
        for(std::size_t ordinal=0;ordinal<count;++ordinal){
            current();require(fields->ordered.size()==ordinal,"Image actual child vector ordering changed");
            const auto& authored=xml->value.children[ordinal];
            require(authored.name=="Image"||authored.name=="Box",
                "Ordinary529520 actual Grid/Stack/other constructor-parser required (ToolTipType needs795ED0)");
            auto* tree=const_cast<BnwWorldViewControlTree*>(saved->tree_facade);
            const auto* styles=core->layout_parser_styles;
            require(styles,"Image child actual selected lexical Styles absent");
            auto frame=BnwWorldViewControlTree::CommonChildParserFrame::enter_owned(*tree,*this,ordinal);
            auto returned=tree->capture_layout_node_canonical_owned(frame->child_id_,*styles,saved->serial);
            require(bool(returned),"Image actual child v44 has no genuine returning receipt");
            frame->close_after_child_return_owned(*returned); //DTOR+FULL POST, exact parent restoration FIRST.
            full();
            auto& child=*const_cast<NotificationLayoutBaseline*>(returned->child_->baseline);
            require((child.kind_==NotificationLayoutBaseline::Kind::image||child.kind_==NotificationLayoutBaseline::Kind::box)&&
                child.base_.common_return_owned&&
                child.base_.receiver_owned&&child.base_.parent_38.expired()&&fields->ordered.size()==ordinal&&
                fields->ordered.capacity()>=count,"Image virtual38 child actual typed return/array capacity changed");
            if(child.kind_==NotificationLayoutBaseline::Kind::box)
                BnwWorldViewControlTree::require_box_native_common_return_owned(core,returned->child_->id,*saved->operation);
            //Actual44BF40 called from4387B1: child38=parent, child3C=parent3C, then ordered parent78 append.
            child.base_.parent_38=owner.base_.receiver_owned;
            child.base_.context_realm_3c=owner.base_.context_realm_3c;
            child.base_.context_3c_observation=owner.base_.context_3c_observation;
            fields->ordered.push_back(child.base_.receiver_owned); //Nonalloc, actual reserved array.
            ++cursor.next; //8356A0 direct ordered advance; does not issue a factory NULL.
            full();
        }
        require(fields->ordered.size()==count,"Image4387C1 before all real child factories returned");
        owner.base_.flags_40|=8u; //Mandatory even count0.
        require(cursor.next==count&&cursor.selected==xml,"Image genuine direct XML iterator did not complete");
        cursor.retire_owned(); //4387C9 actual temporary cleanup before any return receipt.
        full();
        auto result_fields=std::make_shared<NotificationLayoutCommonChildFactoryReturn::Impl>(core,occurrence,xml,
            receiver,array,serial,count);full();
        auto returned=std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn>(new NotificationLayoutCommonChildFactoryReturn(result_fields));full();
        require(owner.base_.children_78==array&&fields->ordered.size()==count&&(owner.base_.flags_40&8u)&&
            !owner.base_.common_return_owned,"Image common final physical fields changed across return allocation POST");
        current();owner.base_.common_return_owned=returned;published_return=returned;full();
        require(owner.base_.receiver_owned==receiver&&owner.base_.children_78==array&&fields->ordered.size()==count&&
            owner.base_.common_return_owned&&(owner.base_.flags_40&8u),"Ordinary actual common return publication POST changed");
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
