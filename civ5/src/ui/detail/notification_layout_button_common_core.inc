// Inside civ5::ui after actual Runtime/Ordinary/ChildFrame/Label bodies.
// Button only. A tag, parser serial or completed prefix cannot issue this return.
void detail::NotificationLayoutRuntimeDimensions::construct_button_receiver_owned()const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
    require(core&&occurrence&&saved->id,"Button5AAA20 canonical constructor owners absent");
    auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
    std::shared_ptr<NotificationLayoutButtonState> published_button;
    std::shared_ptr<const NotificationLayoutOrdinaryReceiver> published_receiver;
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto current=[&]{require_storage_without_callbacks(saved);
        require(owner.kind_==NotificationLayoutBaseline::Kind::button&&owner.actual_type_=="Button"&&
            owner.parser_runtime_dimensions_==this&&!owner.parsed_source_serial_&&
            same(owner.button_,published_button)&&same(owner.base_.receiver_owned,published_receiver)&&
            !owner.base_.children_78&&!owner.base_.common_return_owned&&!owner.base_.id_58&&
            !owner.history_.empty()&&owner.history_.front().command==NotificationLayoutBaseline::Mutation::construct,
            "Button5AAA20 constructor replay/canonical allocation changed");
        require(same(saved->core.lock(),core)&&same(saved->occurrence.lock(),occurrence)&&
            occurrence->id==saved->id&&occurrence->constructor_serial==saved->serial,
            "Button constructor SAME actual Core/occurrence changed");
        if(published_receiver){const auto fields=published_receiver->impl_;
            require(fields&&fields->creator==std::this_thread::get_id()&&same(fields->button.lock(),published_button)&&
                same(fields->canonical.lock(),published_receiver)&&same(fields->core.lock(),core)&&
                same(fields->occurrence.lock(),occurrence)&&fields->id==saved->id&&fields->serial==saved->serial&&
                fields->button_parser_progress==NotificationLayoutOrdinaryReceiver::Impl::ButtonParserProgress::constructor_returned&&
                fields->button_void_f4_f8&&(*fields->button_void_f4_f8)[0]==0u&&(*fields->button_void_f4_f8)[1]==0u&&
                fields->button_void_missing_read_mask==0u,
                "Button constructor physical receiver association changed");}
    };
    const auto full=[&]{current();saved->operation->require_current();current();};
    try{
        full();require(!published_button&&!published_receiver,"Button constructor supplier replay");
        const auto id=saved->id,serial=saved->serial;
        const auto sequence=owner.sequence_,history_size=owner.history_.size();
        require(history_size<=owner.history_.max_size()-8&&sequence<=UINT64_MAX-8,
            "Button constructor actual journal exhausted");
        owner.history_.reserve(history_size+8);full();
        require(owner.sequence_==sequence&&owner.history_.size()==history_size,
            "Button constructor journal changed across reserve POST");
        auto pending_button=std::unique_ptr<NotificationLayoutButtonState>(new NotificationLayoutButtonState);full();
        auto button=std::shared_ptr<NotificationLayoutButtonState>(std::move(pending_button));full();
        require(button->state_104==0&&button->texture_binding_earned&&button->texture_key.empty()&&
            !button->texture_keeper&&!button->texture_extent_15c&&!button->pending_label_178_&&
            !button->label_178_&&!button->label_178_factory_return_&&!button->label_retirement_,
            "Button5AAA20 actual constructed fields differ from literal constructor");
        for(const auto& pair:{button->authored_108_10c,button->increment_110_114,button->image_13c_140,
            button->image_144_148,button->image_14c_150,button->uv_168_16c,button->uv_170_174})
            for(float value:pair)require(std::bit_cast<std::uint32_t>(value)==0,
                "Button40C8F0 actual literal zero field changed");
        for(float value:button->color_12c_138)require(std::bit_cast<std::uint32_t>(value)==0x3f800000u,
            "Button40C8F0 actual literal color1 field changed");
        //All constructor operands are owned VALUES before allocator callbacks.
        auto pending_fields=std::unique_ptr<NotificationLayoutOrdinaryReceiver::Impl>(
            new NotificationLayoutOrdinaryReceiver::Impl(core,occurrence,button,id,serial));full();
        auto fields=std::shared_ptr<NotificationLayoutOrdinaryReceiver::Impl>(std::move(pending_fields));full();
        require(fields->button_void_f4_f8&&(*fields->button_void_f4_f8)[0]==0u&&(*fields->button_void_f4_f8)[1]==0u,
            "Button5484F5/5484FB genuine VALUE constructor did not store physical DWORD0 F4/F8");
        auto receiver=std::shared_ptr<const NotificationLayoutOrdinaryReceiver>(new NotificationLayoutOrdinaryReceiver(fields));full();
        BnwNormalButtonSubobject118Backing::execute_constructor548513_state_owned(*this,button,receiver);full();
        owner.base_.flags_40|=0x8000u;full(); //54851E AFTER actual F4/F8 and548513 stores.
        //Actual5AAA31/39/47/4F literal stores, BEFORE embedded11840C8F0.
        button->authored_108_10c[0]=std::bit_cast<float>(0u);
        button->authored_108_10c[1]=std::bit_cast<float>(0u);
        button->increment_110_114[0]=std::bit_cast<float>(0u);
        button->increment_110_114[1]=std::bit_cast<float>(0u);full();
        BnwNormalButtonSubobject118Backing::execute_constructor40c8f0_owned(*this,button,receiver);full();
        //5AAA5C genuine178=NULL after the real embedded literal constructor.
        button->child_178_observation=NotificationLayoutButtonState::Child178Observation::constructor_null;full();
        require(same(fields->button.lock(),button)&&fields->image.expired()&&fields->box.expired()&&
            fields->grid.expired()&&fields->standalone_label.expired()&&fields->context.expired()&&
            fields->button_void_f4_f8&&(*fields->button_void_f4_f8)[0]==0u&&(*fields->button_void_f4_f8)[1]==0u&&
            fields->button_void_missing_read_mask==0u&&fields->button_state104&&
            *fields->button_state104==0u&&button->state_104==0u&&fields->button_state104_store_count==1&&
            fields->button_state104_writer==NotificationLayoutOrdinaryReceiver::Impl::ButtonState104Writer::literal_ctor548513&&
            owner.sequence_==sequence&&owner.history_.size()==history_size,
            "Button constructor allocation operands/history changed across FULL POST");
        fields->canonical=receiver;owner.button_=button;owner.base_.receiver_owned=receiver;
        published_button=std::move(button);published_receiver=std::move(receiver);full();
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::bind_button_texture_before_common_owned(
    const content::XmlNode& layer,std::size_t ordinal)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();require(bool(core),"Button543A32 actual Core retired");
    const auto xml=BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence;
    auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);const auto button=owner.button_;
    const auto receiver=owner.base_.receiver_owned;
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    std::shared_ptr<const NotificationLayoutTextureKeeper> published;
    const auto current=[&]{require_storage_without_callbacks(saved);
        require(xml&&!xml->dynamic&&ordinal==0&&owner.parser_layers_.size()==1&&
            BnwWorldViewControlTree::Core::same_layout_xml(xml->value,layer)&&
            BnwWorldViewControlTree::Core::same_layout_xml(owner.parser_layers_[0],layer)&&
            owner.kind_==NotificationLayoutBaseline::Kind::button&&owner.actual_type_=="Button"&&button&&
            same(owner.button_,button)&&receiver&&receiver->impl_&&same(owner.base_.receiver_owned,receiver)&&
            same(receiver->impl_->button.lock(),button)&&
            receiver->impl_->button_parser_progress==NotificationLayoutOrdinaryReceiver::Impl::ButtonParserProgress::constructor_returned&&
            owner.parser_runtime_dimensions_==this&&!owner.parsed_source_serial_&&!owner.base_.common_return_owned&&
            same(button->texture_keeper,published),"Button543A32 needs SAME genuine constructor/current v44 layer");
        require(std::none_of(layer.attributes.begin(),layer.attributes.end(),[](const auto& row){return row.first=="Style";}),
            "Button543A32 selected Style-layer resource replay needs its actual producer");
    };
    const auto full=[&]{current();saved->operation->require_current();current();};
    try{
        full();const auto found=std::find_if(layer.attributes.begin(),layer.attributes.end(),[](const auto& row){return row.first=="Texture";});
        require(found!=layer.attributes.end()&&!found->second.empty()&&found->second.find('\0')==std::string::npos&&
            !published&&button->texture_key.empty()&&!button->texture_extent_15c&&button->texture_binding_earned,
            "Button543A32 PRESENT Texture cannot substitute constructor NULL or replay a load");
        std::string key=found->second;full();
        const auto scope=core->layout_resource_scope.lock();const auto tree=core->layout_resource_tree.lock();
        require(scope&&tree&&tree.get()==saved->tree_facade&&core->layout_resource_bound&&
            core->layout_resource_actual_tree==saved->tree_facade&&core->layout_resource_operation==saved->operation&&
            core->layout_resource_serial==saved->serial,"Button4B7BD0 genuine SAME entered ResourceScope absent");
        scope->require_full(*saved->operation,saved->tree_facade);current();
        const auto keeper=scope->load_owned(*saved->operation,saved->tree_facade,key);full();
        scope->require_full(*saved->operation,saved->tree_facade);current();
        scope->require_entry(*saved->operation,saved->tree_facade,keeper);current();
        require(keeper&&keeper->key_==key&&keeper->native_114_118_[0]&&keeper->native_114_118_[1]&&
            !button->texture_keeper&&!button->texture_extent_15c&&button->texture_key.empty(),
            "Button4B7BD0 nonNULL selected keeper/current fields changed");
        //543A37 stores real15C before TextureOffset/Color/common438170.
        button->texture_key.swap(key);
        BnwNormalButtonSubobject118Backing::bind_source44_selected_keeper_owned(*button,keeper);
        published=keeper;full();scope->require_entry(*saved->operation,saved->tree_facade,keeper);current();
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::store_button_common_id_58_owned(const content::XmlNode& layer)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);const auto receiver=owner.base_.receiver_owned;
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto current=[&]{require_storage_without_callbacks(saved);
        require(receiver&&receiver->impl_&&owner.button_&&owner.kind_==NotificationLayoutBaseline::Kind::button&&
            owner.actual_type_=="Button"&&owner.parser_runtime_dimensions_==this&&!owner.parsed_source_serial_&&
            same(owner.base_.receiver_owned,receiver)&&same(receiver->impl_->button.lock(),owner.button_)&&
            receiver->impl_->button_parser_progress==NotificationLayoutOrdinaryReceiver::Impl::ButtonParserProgress::common_entered&&
            owner.parser_layers_.size()==1&&BnwWorldViewControlTree::Core::same_layout_xml(owner.parser_layers_[0],layer)&&
            !owner.base_.id_58&&!owner.base_.common_return_owned,
            "Button4382xx ID58 requires genuine entered Common and unchanged constructorNULL58");};
    const auto full=[&]{current();saved->operation->require_current();current();};
    try{
        full();const auto found=std::find_if(layer.attributes.begin(),layer.attributes.end(),[](const auto& row){return row.first=="ID";});
        std::shared_ptr<const NotificationLayoutControlID58> result;
        if(found!=layer.attributes.end()){
            const auto& text=found->second;
            require(text.size()<32&&std::all_of(text.begin(),text.end(),[](unsigned char c){return c>0&&c<128;}),
                "Button564300 reached unsupported nonterminated/nonASCII/long ID branch");
            if(!text.empty()){
                std::array<char,32> bytes{};std::copy(text.begin(),text.end(),bytes.begin());
                const auto length=static_cast<std::uint32_t>(text.size());
                auto pending=std::unique_ptr<const NotificationLayoutControlID58>(new NotificationLayoutControlID58(bytes,length));full();
                result=std::shared_ptr<const NotificationLayoutControlID58>(std::move(pending));full();
                require(result->bytes==bytes&&result->length==length,
                    "Button564300 actual ID58 allocation bytes changed across FULL POST");
            }
            //PRESENT-empty executes the real release/NULL branch; old58 is actualNULL.
            owner.base_.id_58=result;
        } //409390 genuine MISS preserves the physically constructed NULL58.
        receiver->impl_->button_id58_return=owner.base_.id_58;
        receiver->impl_->button_parser_progress=NotificationLayoutOrdinaryReceiver::Impl::ButtonParserProgress::id_lookup_returned;
        require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
        require(same(owner.base_.receiver_owned,receiver)&&same(owner.base_.id_58,result)&&
            same(receiver->impl_->button_id58_return.lock(),owner.base_.id_58)&&
            receiver->impl_->button_parser_progress==NotificationLayoutOrdinaryReceiver::Impl::ButtonParserProgress::id_lookup_returned,
            "Button actual ID58 lookup/store FULL POST changed");
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::run_button_common_returning_layer_owned(
    const content::XmlNode& layer,std::size_t ordinal)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
    require(core&&occurrence&&saved->id,"Button Common actual owners retired");
    auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
    const auto xml=BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence;
    const auto button=owner.button_;const auto receiver=owner.base_.receiver_owned;
    require(button&&receiver&&receiver->impl_,"Button Common actual5AAA20 receiver missing");
    const auto fields=receiver->impl_;using Progress=NotificationLayoutOrdinaryReceiver::Impl::ButtonParserProgress;
    auto expected_progress=Progress::constructor_returned;
    std::uint8_t expected_void_read_mask=0u;
    std::shared_ptr<NotificationLayoutOrdinaryChildren78> published_array;
    std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn> published_return;
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto current=[&]{require_storage_without_callbacks(saved);
        require(owner.kind_==NotificationLayoutBaseline::Kind::button&&owner.actual_type_=="Button"&&
            owner.parser_runtime_dimensions_==this&&!owner.parsed_source_serial_&&same(owner.button_,button)&&
            same(owner.base_.receiver_owned,receiver)&&fields->creator==std::this_thread::get_id()&&
            same(fields->canonical.lock(),receiver)&&same(fields->core.lock(),core)&&same(fields->occurrence.lock(),occurrence)&&
            same(fields->button.lock(),button)&&fields->image.expired()&&fields->box.expired()&&fields->grid.expired()&&
            fields->standalone_label.expired()&&fields->context.expired()&&fields->id==saved->id&&fields->serial==saved->serial&&
            fields->button_parser_progress==expected_progress&&fields->button_void_f4_f8&&
            (*fields->button_void_f4_f8)[0]==0u&&(*fields->button_void_f4_f8)[1]==0u&&
            fields->button_void_missing_read_mask==expected_void_read_mask&&same(owner.base_.children_78,published_array)&&
            same(owner.base_.common_return_owned,published_return),"Button438170 physical receiver/control/progress changed");
        require(xml&&!xml->dynamic&&xml->value.name=="Button"&&ordinal==0&&owner.parser_layers_.size()==1&&
            same(BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence,xml)&&
            same(xml->source.lock(),core->source)&&BnwWorldViewControlTree::Core::same_layout_xml(xml->value,layer)&&
            BnwWorldViewControlTree::Core::same_layout_xml(owner.parser_layers_[0],layer)&&saved->button_style_loan&&
            saved->button_style_loan->entered.count(ordinal)&&!saved->button_style_loan->busy,
            "Button Common selected authored occurrence/actually returned Style lookup changed");
        require(std::none_of(layer.attributes.begin(),layer.attributes.end(),[](const auto& row){return row.first=="Style";}),
            "Button Common Style replay needs its genuine retained layer/child occurrence owner");
        require(button->texture_binding_earned&&!button->pending_label_178_&&!button->label_retirement_&&button->child_178_observation,
            "Button Common cannot skip actual Texture/Label prefix or retirement");
        if(button->texture_key.empty())require(!button->texture_keeper&&!button->texture_extent_15c,
            "Button Common constructorNULL15C contradicted by retained texture");
        else{const auto scope=core->layout_resource_scope.lock();require(scope&&core->layout_resource_bound&&
                core->layout_resource_operation==saved->operation&&core->layout_resource_actual_tree==saved->tree_facade&&
                button->texture_keeper&&button->texture_extent_15c&&button->texture_keeper->key_==button->texture_key&&
                *button->texture_extent_15c==button->texture_keeper->native_114_118_,
                "Button Common parsed15C lacks genuine SAME selected keeper/extent");
            scope->require_entry(*saved->operation,saved->tree_facade,button->texture_keeper);require_storage_without_callbacks(saved);}
        const bool has_string=std::any_of(layer.attributes.begin(),layer.attributes.end(),[](const auto& row){return row.first=="String";});
        if(has_string){const auto returned=button->label_178_factory_return_;
            require(*button->child_178_observation==NotificationLayoutButtonState::Child178Observation::factory_child&&
                button->label_178_&&returned&&returned->impl_&&returned->impl_->creator==std::this_thread::get_id()&&
                returned->impl_->alive.load()&&same(returned->impl_->parser,saved)&&same(returned->impl_->parent.lock(),button)&&
                same(returned->impl_->child,button->label_178_)&&returned->impl_->source_serial==saved->serial&&
                returned->impl_->layer==ordinal&&returned->impl_->published_sequence&&
                returned->impl_->published_sequence<=owner.sequence_,"Button Common actual703120/5E return association lost");
            button->label_178_->require_ownership_origin_owned(button,*saved->operation,ordinal);
            returned->impl_->module->require_label_allocation_owned(*saved->operation,returned->impl_->allocation,returned->impl_->child,true);
            require_storage_without_callbacks(saved);
        }else require(*button->child_178_observation==NotificationLayoutButtonState::Child178Observation::constructor_null&&
            !button->label_178_&&!button->label_178_factory_return_,"Button Common String MISS cannot invent a NULL178");
    };
    const auto full=[&]{current();saved->operation->require_current();current();};
    try{
        full();
        //6CAEB0 calls BOTH5D2860 readers unconditionally in this order and ignoresAL.
        //This genuine named lookup MISS reaches5D28E3 falseRET before any destination read.
        //The existing physical words are retained; there is no absent/default clear.
        const auto read_void_named_missing=[&](std::string_view key,std::uint8_t next_mask)->bool{
            full();const auto actual_words=*fields->button_void_f4_f8;
            const bool found=std::any_of(xml->value.attributes.begin(),xml->value.attributes.end(),
                [key](const auto& row){return row.first==key;});
            full();require(*fields->button_void_f4_f8==actual_words,
                "Button5D2860 named lookup changed physical F4/F8 across FULL POST");
            require(!found,"Button5D2860 PRESENT Void requires actual835330 payload/836080 destination actor");
            fields->button_void_missing_read_mask=expected_void_read_mask=next_mask;
            full();require(*fields->button_void_f4_f8==actual_words,
                "Button5D2860 actual MISS must retain existing physical F4/F8");
            return false; //Earned5D2899->5D28E3 ALfalse, not an unavailable-read substitute.
        };
        const bool first_read=read_void_named_missing("Void1",1u);
        require(!first_read,"Button Void1 genuine MISS unexpectedly succeeded");
        const bool second_read=read_void_named_missing("Void2",3u);
        require(!second_read,"Button Void2 genuine MISS unexpectedly succeeded");
        //Only AFTER both readers: actual first-byte t/T/1 tests, then OR-only stores.
        const auto apply_prefix_flag=[&](std::string_view key,std::uint32_t mask){
            full();const auto before_flags=owner.base_.flags_40;
            const auto found=std::find_if(xml->value.attributes.begin(),xml->value.attributes.end(),
                [key](const auto& row){return row.first==key;});
            const bool affirmative=found!=xml->value.attributes.end()&&!found->second.empty()&&
                (found->second.front()=='t'||found->second.front()=='T'||found->second.front()=='1');
            full();require(owner.base_.flags_40==before_flags,"Button6CAEB0 flag lookup changed actual flags40");
            if(affirmative)owner.base_.flags_40|=mask;
            full();require(owner.base_.flags_40==(affirmative?(before_flags|mask):before_flags),
                "Button6CAEB0 OR-only prefix store changed across FULL POST");
        };
        apply_prefix_flag("NoDefaultSound",0x02000000u);
        apply_prefix_flag("Disabled",0x00020000u);
        for(const auto& row:layer.attributes){
            if(row.first=="Size"||row.first=="Offset"||row.first=="Anchor"||row.first=="AnchorSide")
                require(!row.second.empty(),"Button Common PRESENT-empty operand requires its actual native parser branch");
        }
        for(const auto& child:xml->value.children)require(child.name!="ShowOnMouseOver"&&child.name!="HideOnMouseOver",
            "Button Common mouse-over wrapper requires real7C/80 child producer");
        const auto sequence=owner.sequence_,history_size=owner.history_.size();
        require(history_size<=owner.history_.max_size()-4&&sequence<=UINT64_MAX-4,"Button Common actual journal exhausted");
        owner.history_.reserve(history_size+4);full();
        require(owner.sequence_==sequence&&owner.history_.size()==history_size,"Button Common journal changed across reserve POST");
        fields->button_label_return=button->label_178_;fields->button_label_factory_return=button->label_178_factory_return_;
        require(fields->button_void_missing_read_mask==3u,"Button Common entered before both actual Void readers returned");
        fields->button_parser_progress=expected_progress=Progress::common_entered;
        full();owner.parse_layer_owned(layer); //543B40->6CAEB0->438170, actual3C/flags/+58/ID/tooltip order.
        expected_progress=Progress::id_lookup_returned;full();
        require(owner.sequence_==sequence+2&&owner.history_.size()==history_size+2&&
            owner.history_[history_size].command==NotificationLayoutBaseline::Mutation::context_realm_3c&&
            owner.history_[history_size+1].command==NotificationLayoutBaseline::Mutation::parser,
            "Button Common mandatory3C and parser effects did not return in actual order");
        const auto argument=saved->context_realm_argument;require_context_argument_owned(saved,argument);
        require(owner.base_.context_3c_observation==NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm&&
            same(owner.base_.context_realm_3c,argument->impl_->realm),"Button4381AC actual secondarg3C store missing");
        //43875E only actualNULL78 enters495E10; first selected authored layer only.
        auto pending_array_fields=std::unique_ptr<NotificationLayoutOrdinaryChildren78::Impl>(
            new NotificationLayoutOrdinaryChildren78::Impl(receiver));full();
        auto array_fields=std::shared_ptr<NotificationLayoutOrdinaryChildren78::Impl>(std::move(pending_array_fields));full();
        auto array=std::shared_ptr<NotificationLayoutOrdinaryChildren78>(new NotificationLayoutOrdinaryChildren78(array_fields));full();
        require(!owner.base_.children_78&&array_fields->ordered.empty(),"Button495E10 actual NULL78 changed across allocator POST");
        owner.base_.children_78=array;published_array=array;full();
        const auto count=xml->value.children.size();require(count<=array_fields->ordered.max_size(),"Button children78 count exceeds actual array domain");
        struct Cursor final{
            std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
            std::size_t next=0;
            void retire_owned(){selected.reset();} //8353F0 temporary XML custody, not Source retirement.
        }cursor{xml};
        array_fields->ordered.reserve(count);full();
        require(array_fields->ordered.empty()&&array_fields->ordered.capacity()>=count,"Button5E51D0 actual reserve POST changed");
        for(std::size_t i=0;i<count;++i){
            full();require(cursor.selected==xml&&cursor.next==i&&array_fields->ordered.size()==i,
                "Button835640 actual direct XML/78 cursor ordering changed");
            auto*const tree=const_cast<BnwWorldViewControlTree*>(saved->tree_facade);
            const auto*const styles=core->layout_parser_styles;require(styles,"Button child genuine lexical Styles owner absent");
            auto frame=BnwWorldViewControlTree::CommonChildParserFrame::enter_owned(*tree,*this,i);
            auto returned=tree->capture_layout_node_canonical_owned(frame->child_id_,*styles,saved->serial);
            require(bool(returned),"Button529520 actual child factory has no genuine returning receipt");
            frame->close_after_child_return_owned(*returned);frame.reset();full();
            auto& child=*const_cast<NotificationLayoutBaseline*>(returned->child_->baseline);
            const auto child_receiver=child.base_.receiver_owned;
            const auto child_done=child.base_.common_return_owned;
            require(child_receiver&&child_receiver->impl_&&child_done&&child_done->impl_&&
                same(child_done,returned->native_)&&child.base_.children_78&&child.base_.children_78->impl_&&
                child.base_.parent_38.expired()&&array_fields->ordered.size()==i&&array_fields->ordered.capacity()>=count,
                "Button virtual38 actual child allocation/common RETURN/capacity changed");
            const auto child_fields=child_receiver->impl_;const auto& actual_child=BnwWorldViewControlTree::Core::node(core->nodes,returned->child_->id);
            require(child_fields->creator==std::this_thread::get_id()&&same(child_fields->core.lock(),core)&&
                same(child_fields->canonical.lock(),child_receiver)&&same(child_fields->occurrence.lock(),returned->occurrence_)&&
                same(actual_child.layout_receiver_occurrence,returned->occurrence_)&&same(actual_child.full_xml_occurrence,returned->xml_)&&
                same(returned->xml_->parent.lock(),xml)&&returned->xml_->ordinal==i&&child_fields->id==returned->child_->id&&
                child_fields->serial==saved->serial&&child_done->impl_->creator==std::this_thread::get_id()&&
                same(child_done->impl_->core.lock(),core)&&same(child_done->impl_->occurrence.lock(),returned->occurrence_)&&
                same(child_done->impl_->selected.lock(),returned->xml_)&&same(child_done->impl_->receiver.lock(),child_receiver)&&
                same(child_done->impl_->children.lock(),child.base_.children_78)&&child_done->impl_->serial==saved->serial&&
                child_done->impl_->count==returned->xml_->value.children.size()&&
                child.base_.children_78->impl_->ordered.size()==child_done->impl_->count&&(child.base_.flags_40&8u),
                "Button child genuine returned receiver/ordered Source/common identity changed");
            const bool actual_image=child.image_&&same(child_fields->image.lock(),child.image_)&&child_fields->box.expired()&&
                child_fields->button.expired()&&child_fields->grid.expired()&&child_fields->standalone_label.expired();
            const bool actual_box=child.box_&&same(child_fields->box.lock(),child.box_)&&child_fields->image.expired()&&
                child_fields->button.expired()&&child_fields->grid.expired()&&child_fields->standalone_label.expired();
            const bool actual_grid=child.grid_&&same(child_fields->grid.lock(),child.grid_)&&child_fields->image.expired()&&
                child_fields->button.expired()&&child_fields->box.expired()&&child_fields->standalone_label.expired();
            const bool actual_button=child.button_&&same(child_fields->button.lock(),child.button_)&&child_fields->image.expired()&&
                child_fields->box.expired()&&child_fields->grid.expired()&&child_fields->standalone_label.expired();
            const bool actual_label=child.standalone_label_&&same(child_fields->standalone_label.lock(),child.standalone_label_)&&
                child_fields->image.expired()&&child_fields->box.expired()&&child_fields->grid.expired()&&child_fields->button.expired();
            require(actual_image||actual_box||actual_grid||actual_button||actual_label,
                "Button44BF40 child requires its genuine physical subtype/paired alias writer");
            if(actual_box)BnwWorldViewControlTree::require_box_native_common_return_owned(core,returned->child_->id,*saved->operation);
            if(actual_button)BnwWorldViewControlTree::require_button_native_common_return_owned(core,returned->child_->id,*saved->operation);
            //Actual44BF40 writes the child's native38 and3C before the real78 append.
            //A distinct Label has two physical observations of each word; both
            //must be stored by the actual returned allocation's owner.
            if(actual_label){
                using Label=NotificationLayoutStandaloneLabelState;
                using Base=NotificationLayoutBaseline;
                const auto label=child.standalone_label_;const auto label_fields=label->impl_;
                label_fields->first();const auto wrapper=label_fields->wrapper_returned;
                const auto child_id=returned->child_->id;const auto*const child_address=&child;
                const auto child_sequence=label_fields->sequence,base_sequence=child.sequence_;
                const auto child_history=label_fields->history.size(),base_history=child.history_.size();
                const auto old_realm=child.base_.context_realm_3c;
                const auto old_observation=child.base_.context_3c_observation;
                const auto realm=owner.base_.context_realm_3c;
                const auto observation=owner.base_.context_3c_observation;
                require(child_sequence<=UINT64_MAX-2&&base_sequence<=UINT64_MAX-2,
                    "Button Label44BF40 real journals exhausted");
                unsigned stored=0;
                const auto label_current=[&]{
                    //Physical allocation health precedes every borrowed Base access.
                    label_fields->first();require_storage_without_callbacks(saved);
                    const auto& fresh=BnwWorldViewControlTree::Core::node(core->nodes,child_id);
                    require(fresh.alive&&!fresh.dynamic&&fresh.layout_baseline&&&*fresh.layout_baseline==child_address&&
                        same(fresh.layout_baseline->standalone_label_,label)&&same(fresh.layout_baseline->base_.receiver_owned,child_receiver)&&
                        same(fresh.layout_receiver_occurrence,returned->occurrence_)&&same(fresh.full_xml_occurrence,returned->xml_)&&
                        returned->creator_==std::this_thread::get_id()&&returned->consumed_&&returned->child_&&
                        !returned->child_->alive.load()&&!returned->child_->failed.load()&&returned->child_->baseline==child_address&&
                        same(returned->native_,child_done)&&same(label_fields->core.lock(),core)&&
                        same(label_fields->occurrence.lock(),returned->occurrence_)&&same(label_fields->selected.lock(),returned->xml_)&&
                        same(child_fields->standalone_label.lock(),label)&&same(child_fields->canonical.lock(),child_receiver)&&
                        wrapper&&wrapper->creator==std::this_thread::get_id()&&same(label_fields->wrapper_returned,wrapper)&&
                        same(wrapper->core.lock(),core)&&same(wrapper->occurrence.lock(),returned->occurrence_)&&
                        same(wrapper->selected.lock(),returned->xml_)&&same(wrapper->receiver.lock(),child_receiver)&&
                        same(wrapper->children.lock(),child.base_.children_78)&&same(wrapper->common.lock(),child_done)&&
                        wrapper->serial==saved->serial&&wrapper->sequence==child_sequence&&wrapper->base_sequence==base_sequence&&
                        label_fields->source_serial==saved->serial&&label_fields->common_prefix_returned&&
                        !label_fields->initialization_entered&&!label_fields->completed_standalone_run&&
                        label_fields->sequence==child_sequence+stored&&child.sequence_==base_sequence+stored&&
                        label_fields->history.size()==child_history+stored&&child.history_.size()==base_history+stored&&
                        same(owner.base_.receiver_owned,receiver)&&same(owner.base_.context_realm_3c,realm)&&
                        owner.base_.context_3c_observation==observation&&realm&&
                        observation==Base::Base::Context3CObservation::parsed_realm&&
                        array_fields->ordered.size()==i&&array_fields->ordered.capacity()>=count,
                        "Button Label44BF40 actual closed RETURN/physical owners/journals changed");
                    const auto* parent=std::get_if<Label::Impl::ParentReceiverBorrow>(&label_fields->fields[0x38/4]);
                    require(stored?(parent&&same(parent->value.lock(),receiver)&&same(child.base_.parent_38.lock(),receiver)):
                        (std::holds_alternative<Label::Impl::Null>(label_fields->fields[0x38/4])&&child.base_.parent_38.expired()),
                        "Button Label44BF4C actual constructorNULL38/paired parent store changed");
                    const auto* native_realm=std::get_if<Label::Impl::ParsedRealm>(&label_fields->fields[0x3c/4]);
                    require(stored==2?(native_realm&&same(native_realm->value,realm)&&same(child.base_.context_realm_3c,realm)&&
                        child.base_.context_3c_observation==observation):
                        (native_realm&&same(native_realm->value,old_realm)&&same(child.base_.context_realm_3c,old_realm)&&
                        child.base_.context_3c_observation==old_observation&&old_realm&&
                        old_observation==Base::Base::Context3CObservation::parsed_realm),
                        "Button Label44BF52 actual previous3C/paired realm store changed");
                    require_storage_without_callbacks(saved);label_fields->first();
                };
                const auto label_full=[&]{label_current();saved->operation->require_current();label_current();};
                try{
                    label_full();
                    //Allocate owned buffers rather than borrow a mutable Node vector
                    //through new_handler. No callback occurs between each paired store.
                    std::vector<Label::Impl::Entry> next_child_history;
                    std::vector<Base::Entry> next_base_history;
                    require(child_history<=next_child_history.max_size()-2&&base_history<=next_base_history.max_size()-2,
                        "Button Label44BF40 history capacity exhausted");
                    next_child_history.reserve(child_history+2);label_full();
                    next_base_history.reserve(base_history+2);label_full();
                    for(const auto& entry:label_fields->history)next_child_history.push_back(entry);
                    for(const auto& entry:child.history_)next_base_history.push_back(entry);
                    label_fields->history.swap(next_child_history);child.history_.swap(next_base_history);
                    label_fields->history.push_back({++label_fields->sequence,0x38});
                    child.history_.push_back({++child.sequence_,Base::Mutation::receiver_prefix});
                    label_fields->fields[0x38/4]=Label::Impl::ParentReceiverBorrow{receiver};
                    child.base_.parent_38=receiver;stored=1;label_full();
                    label_fields->history.push_back({++label_fields->sequence,0x3c});
                    child.history_.push_back({++child.sequence_,Base::Mutation::context_realm_3c});
                    label_fields->fields[0x3c/4]=Label::Impl::ParsedRealm{realm};
                    child.base_.context_realm_3c=realm;child.base_.context_3c_observation=observation;
                    stored=2;label_full();
                }catch(...){label_fields->failed.store(true);fail_storage_without_callbacks(saved);throw;}
            }else{
                child.base_.parent_38=receiver;child.base_.context_realm_3c=owner.base_.context_realm_3c;
                child.base_.context_3c_observation=owner.base_.context_3c_observation;
            }
            array_fields->ordered.push_back(child_receiver);++cursor.next;full();
        }
        require(array_fields->ordered.size()==count&&cursor.next==count&&cursor.selected==xml,
            "Button4387C1 before all actual child factories/attachments returned");
        owner.base_.flags_40|=8u; //Mandatory OR8 INCLUDING zero children.
        cursor.retire_owned();full(); //4387C9 true temporary XML cleanup before RET.
        const auto serial=saved->serial;
        auto pending_done=std::unique_ptr<NotificationLayoutCommonChildFactoryReturn::Impl>(
            new NotificationLayoutCommonChildFactoryReturn::Impl(core,occurrence,xml,receiver,array,serial,count));full();
        auto done=std::shared_ptr<NotificationLayoutCommonChildFactoryReturn::Impl>(std::move(pending_done));full();
        auto returned=std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn>(new NotificationLayoutCommonChildFactoryReturn(done));full();
        require(!cursor.selected&&array_fields->ordered.size()==count&&(owner.base_.flags_40&8u)&&
            same(done->receiver.lock(),receiver)&&same(done->children.lock(),array)&&same(done->selected.lock(),xml),
            "Button actual completed suffix/return operands changed across allocator POST");
        owner.base_.common_return_owned=returned;published_return=returned;
        fields->button_parser_progress=expected_progress=Progress::common_returned;full();
        require_button_common_return_from_parser_owned();
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void BnwWorldViewControlTree::require_button_native_common_return_owned(
    const std::shared_ptr<Core>& core,std::uint64_t id,
    const runtime::BnwWorldViewSourceOperationLoan& operation,bool expected_busy){
    using Base=detail::NotificationLayoutBaseline;using Button=detail::NotificationLayoutButtonState;
    require(core&&id,"Button native common return actual Core/id absent");
    core->layout_origin(expected_busy);operation.require_origin_without_callbacks();
    require(&core->source->current_operation_loan()==&operation,"Button native common return SAME current Source changed");
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto& node=Core::node(core->nodes,id);const auto xml=node.full_xml_occurrence;
    require(node.alive&&!node.dynamic&&node.layout_baseline&&node.layout_receiver_occurrence&&xml&&!xml->dynamic,
        "Button native common return canonical physical constructor/selected XML absent");
    const auto& owner=*node.layout_baseline;const auto button=owner.button_;const auto receiver=owner.base_.receiver_owned;
    const auto array=owner.base_.children_78;const auto returned=owner.base_.common_return_owned;
    require(owner.kind_==Base::Kind::button&&owner.actual_type_=="Button"&&button&&receiver&&receiver->impl_&&
        returned&&returned->impl_&&(owner.base_.flags_40&8u)&&button->texture_binding_earned&&
        !button->pending_label_178_&&!button->label_retirement_&&button->child_178_observation,
        "Button native common returning real constructor/suffix fields unavailable");
    const auto fields=receiver->impl_;const auto done=returned->impl_;
    require(fields->creator==std::this_thread::get_id()&&done->creator==std::this_thread::get_id()&&
        same(fields->button.lock(),button)&&
        fields->image.expired()&&fields->box.expired()&&fields->grid.expired()&&fields->context.expired()&&fields->standalone_label.expired()&&
        same(fields->canonical.lock(),receiver)&&same(fields->core.lock(),core)&&same(done->core.lock(),core)&&
        same(fields->occurrence.lock(),node.layout_receiver_occurrence)&&same(done->occurrence.lock(),node.layout_receiver_occurrence)&&
        same(done->selected.lock(),xml)&&same(done->receiver.lock(),receiver)&&
        same(xml->source.lock(),core->source)&&fields->id==id&&
        fields->serial==node.layout_receiver_occurrence->constructor_serial&&done->serial==fields->serial&&
        fields->serial==operation.operation_serial()&&done->count==xml->value.children.size()&&
        fields->button_parser_progress==detail::NotificationLayoutOrdinaryReceiver::Impl::ButtonParserProgress::common_returned&&
        fields->button_void_current_matches_owned()&&
        fields->button_void_missing_read_mask==3u&&
        same(fields->button_id58_return.lock(),owner.base_.id_58)&&same(fields->button_label_return.lock(),button->label_178_)&&
        same(fields->button_label_factory_return.lock(),button->label_178_factory_return_),
        "Button opaque common constructor/return/children/Source control changed");
    require_native_ordinary_current_children_without_callbacks(receiver,returned,array);
    require(std::none_of(xml->value.attributes.begin(),xml->value.attributes.end(),
        [](const auto& row){return row.first=="Void1"||row.first=="Void2";}),
        "Button returning physical F4/F8 lacks genuine selected named lookup MISS association");
    const auto authored_id=std::find_if(xml->value.attributes.begin(),xml->value.attributes.end(),[](const auto& row){return row.first=="ID";});
    if(authored_id==xml->value.attributes.end()||authored_id->second.empty())require(!owner.base_.id_58,
        "Button actual MISS/PRESENT-empty58 changed from genuine NULL");
    else{require(owner.base_.id_58&&owner.base_.id_58->length==authored_id->second.size()&&authored_id->second.size()<32,
            "Button actual retained ID58 length/branch changed");
        const auto& bytes=owner.base_.id_58->bytes;
        require(std::equal(authored_id->second.begin(),authored_id->second.end(),bytes.begin())&&bytes[authored_id->second.size()]=='\0',
            "Button actual retained ID58 selected bytes changed");}
    const auto texture=std::find_if(xml->value.attributes.begin(),xml->value.attributes.end(),[](const auto& row){return row.first=="Texture";});
    if(texture==xml->value.attributes.end())require(button->texture_key.empty()&&!button->texture_keeper&&!button->texture_extent_15c,
        "Button returning15C constructorNULL no longer matches selected lookup MISS");
    else{const auto scope=core->layout_resource_scope.lock();require(scope&&core->layout_resource_bound&&
            core->layout_resource_operation==&operation&&button->texture_keeper&&button->texture_extent_15c&&
            button->texture_key==texture->second&&button->texture_keeper->key_==texture->second&&
            *button->texture_extent_15c==button->texture_keeper->native_114_118_,
            "Button returning real selected15C keeper/extent/current scope changed");
        scope->require_entry(operation,core->layout_resource_actual_tree,button->texture_keeper);core->layout_origin(expected_busy);}
    const bool has_string=std::any_of(xml->value.attributes.begin(),xml->value.attributes.end(),[](const auto& row){return row.first=="String";});
    if(has_string){require(*button->child_178_observation==Button::Child178Observation::factory_child&&
            button->label_178_&&button->label_178_factory_return_,"Button genuine703120/5E published178 association absent");
        button->label_178_->require_ownership_origin_owned(button,operation,0);
    }else require(*button->child_178_observation==Button::Child178Observation::constructor_null&&
        !button->label_178_&&!button->label_178_factory_return_,"Button String MISS actual constructorNULL178 changed");
    for(std::size_t i=0;array&&i<array->impl_->ordered.size();++i){const auto child_receiver=array->impl_->ordered[i];
        require(child_receiver&&child_receiver->impl_&&child_receiver->impl_->creator==std::this_thread::get_id(),
            "Button ordered78 actual child receiver retired/foreign");const auto child_fields=child_receiver->impl_;
        require(same(child_fields->core.lock(),core),"Button ordered child SAME actual Core changed");
        const auto& child=Core::node(core->nodes,child_fields->id);
        require(child.alive&&!child.dynamic&&child.layout_baseline&&child.layout_receiver_occurrence&&child.full_xml_occurrence&&
            same(child_fields->canonical.lock(),child_receiver)&&same(child_fields->occurrence.lock(),child.layout_receiver_occurrence)&&
            child_fields->serial==fields->serial&&same(child.full_xml_occurrence->parent.lock(),xml)&&child.full_xml_occurrence->ordinal<done->count&&
            same(child.layout_baseline->base_.receiver_owned,child_receiver)&&same(child.layout_baseline->base_.parent_38.lock(),receiver)&&
            child.layout_baseline->base_.common_return_owned&&child.layout_baseline->base_.common_return_owned->impl_&&
            same(child.layout_baseline->base_.context_realm_3c,owner.base_.context_realm_3c)&&
            child.layout_baseline->base_.context_3c_observation==owner.base_.context_3c_observation,
            "Button real child RETURN/ordered occurrence/parent38/Realm3C changed");
        const auto& state=*child.layout_baseline;const auto cd=state.base_.common_return_owned->impl_;
        require_native_ordinary_current_children_without_callbacks(child_receiver,state.base_.common_return_owned,state.base_.children_78);
        require(cd->creator==std::this_thread::get_id()&&same(cd->core.lock(),core)&&same(cd->occurrence.lock(),child.layout_receiver_occurrence)&&
            same(cd->selected.lock(),child.full_xml_occurrence)&&same(cd->receiver.lock(),child_receiver)&&cd->serial==fields->serial&&
            cd->count==child.full_xml_occurrence->value.children.size()&&
            (state.base_.flags_40&8u),"Button ordered child real common return owner/suffix changed");
        require((state.image_&&same(child_fields->image.lock(),state.image_)&&child_fields->button.expired()&&child_fields->box.expired()&&child_fields->grid.expired())||
            (state.box_&&same(child_fields->box.lock(),state.box_)&&child_fields->image.expired()&&child_fields->button.expired()&&child_fields->grid.expired())||
            (state.grid_&&same(child_fields->grid.lock(),state.grid_)&&child_fields->image.expired()&&child_fields->box.expired()&&child_fields->button.expired())||
            (state.button_&&same(child_fields->button.lock(),state.button_)&&child_fields->image.expired()&&child_fields->box.expired()&&child_fields->grid.expired())||
            (state.standalone_label_&&same(child_fields->standalone_label.lock(),state.standalone_label_)&&child_fields->image.expired()&&
                child_fields->box.expired()&&child_fields->grid.expired()&&child_fields->button.expired()),
            "Button ordered child actual subtype allocation association changed");
        if(state.standalone_label_){
            using Label=detail::NotificationLayoutStandaloneLabelState;
            const auto label=state.standalone_label_;const auto lf=label->impl_;lf->first();
            const auto wrapper=lf->wrapper_returned;
            const auto* parent=std::get_if<Label::Impl::ParentReceiverBorrow>(&lf->fields[0x38/4]);
            const auto* realm=std::get_if<Label::Impl::ParsedRealm>(&lf->fields[0x3c/4]);
            const auto* children=std::get_if<Label::Impl::ChildrenBorrow>(&lf->fields[0x78/4]);
            const auto* flags=std::get_if<std::uint32_t>(&lf->fields[0x40/4]);
            require(wrapper&&wrapper->creator==std::this_thread::get_id()&&
                same(lf->core.lock(),core)&&same(lf->occurrence.lock(),child.layout_receiver_occurrence)&&
                same(lf->selected.lock(),child.full_xml_occurrence)&&lf->source_serial==fields->serial&&
                same(wrapper->core.lock(),core)&&same(wrapper->occurrence.lock(),child.layout_receiver_occurrence)&&
                same(wrapper->selected.lock(),child.full_xml_occurrence)&&same(wrapper->receiver.lock(),child_receiver)&&
                same(wrapper->children.lock(),cd->children.lock())&&same(wrapper->common.lock(),state.base_.common_return_owned)&&
                wrapper->serial==fields->serial&&wrapper->sequence<=UINT64_MAX-2&&wrapper->base_sequence<=UINT64_MAX-2&&
                (state.base_.children_78?(lf->sequence==wrapper->sequence+2&&state.sequence_==wrapper->base_sequence+2):
                 (lf->sequence>=wrapper->sequence+2&&state.sequence_>=wrapper->base_sequence+2))&&
                wrapper->sequence<=lf->history.size()&&lf->history.size()-wrapper->sequence>=2&&
                wrapper->base_sequence<=state.history_.size()&&state.history_.size()-wrapper->base_sequence>=2&&
                lf->history[wrapper->sequence].offset==0x38&&lf->history[wrapper->sequence+1].offset==0x3c&&
                state.history_[wrapper->base_sequence].command==Base::Mutation::receiver_prefix&&
                state.history_[wrapper->base_sequence+1].command==Base::Mutation::context_realm_3c&&
                parent&&same(parent->value.lock(),receiver)&&realm&&same(realm->value,owner.base_.context_realm_3c)&&
                ((state.base_.children_78&&children&&same(children->object,state.base_.children_78))||
                 (!state.base_.children_78&&std::holds_alternative<Label::Impl::Null>(lf->fields[0x78/4])))&&flags&&*flags==state.base_.flags_40&&
                lf->common_prefix_returned&&!lf->initialization_entered&&!lf->completed_standalone_run,
                "Button ordered standalone Label genuine RETURN/paired38/3C physical aliases changed");
            lf->first();
        }
    }
    core->layout_origin(expected_busy);operation.require_origin_without_callbacks();
}

void detail::NotificationLayoutRuntimeDimensions::require_button_common_return_from_parser_owned()const{
    const auto saved=state_;
    try{require_storage_without_callbacks(saved);const auto core=saved->core.lock();
        BnwWorldViewControlTree::require_button_native_common_return_owned(core,saved->id,*saved->operation);
        require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
        BnwWorldViewControlTree::require_button_native_common_return_owned(core,saved->id,*saved->operation);
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
