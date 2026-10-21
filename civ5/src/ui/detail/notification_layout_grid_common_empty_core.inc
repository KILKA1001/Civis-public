// After canonical Runtime State, actual OrdinaryReceiver/78/Return storage,
// and the distinct selected Grid Style producer. Empty children ONLY.
void detail::NotificationLayoutRuntimeDimensions::publish_grid_constructor_owned()const {
    const auto saved=state_;
    try{
        const auto first=[&]{require_storage_without_callbacks(saved);};
        const auto full=[&]{first();saved->operation->require_current();first();};
        full();auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        const auto grid=owner.grid_;const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
        require(core&&occurrence&&grid&&owner.kind_==NotificationLayoutBaseline::Kind::grid&&
            owner.actual_type_=="Grid"&&!owner.box_&&!owner.image_&&owner.parser_runtime_dimensions_==this&&
            grid->parser_attempted&&!grid->failed&&!owner.base_.receiver_owned,
            "Grid ordinary5D33C0 actual constructor already returned; canonical publication replay/subtype mismatch");
        // Capture physical values BEFORE allocation. This issues a distinct
        // Grid receiver; neither Image/Box token nor a DOM name is substituted.
        const auto id=saved->id,serial=saved->serial;
        auto fields=std::make_shared<NotificationLayoutOrdinaryReceiver::Impl>(core,occurrence,grid,id,serial);full();
        auto receiver=std::shared_ptr<const NotificationLayoutOrdinaryReceiver>(new NotificationLayoutOrdinaryReceiver(fields));full();
        require(!owner.base_.receiver_owned&&owner.grid_==grid&&fields->grid.lock()==grid,
            "Grid receiver actual constructor identity changed during allocation POST");
        fields->canonical=receiver;owner.base_.receiver_owned=std::move(receiver);first();
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::begin_grid_common_empty_layer_owned(std::size_t ordinal)const {
    const auto saved=state_;require_storage_without_callbacks(saved);const auto grid=saved->baseline->grid_;
    try{
        const auto first=[&]{require_grid_parser_owned(grid.get());};
        const auto full=[&]{first();saved->operation->require_current();first();};
        full();auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
        require(core&&occurrence&&owner.base_.receiver_owned&&owner.base_.receiver_owned->impl_&&
            !grid->common_layer_&&saved->grid_style_loan&&saved->grid_style_loan->entered.count(ordinal)&&
            ordinal<owner.parser_layers_.size(),
            "Grid common needs exact actual Style/v44-entered layer, never supplied XML/serial authority");
        const auto xml=BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence;
        require(xml&&!xml->dynamic&&xml->source.lock()==core->source&&xml->value.name=="Grid"&&
            xml->value.children.empty()&&owner.parser_layers_[ordinal].children.empty()&&
            owner.parser_layers_[0].name==xml->value.name&&owner.parser_layers_[0].attributes==xml->value.attributes,
            "Grid common empty slice needs selected full XML children0 AND reached layer children0; nested factory remains required");
        const auto fields=owner.base_.receiver_owned->impl_;
        require(fields->creator==std::this_thread::get_id()&&fields->core.lock()==core&&
            fields->occurrence.lock()==occurrence&&fields->grid.lock()==grid&&fields->image.expired()&&
            fields->id==saved->id&&fields->serial==saved->serial&&fields->canonical.lock()==owner.base_.receiver_owned,
            "Grid actual canonical receiver/control changed before common438170");
        for(const auto& returned:grid->common_layer_returns_)
            require(returned.first!=ordinal,"Grid same common layer was already returned");
        // Private lexical ordinal is admitted only by the real Style producer;
        // it is not a readiness receipt. Reserve before the actual3C/flags/ID.
        grid->common_layer_=ordinal;first();
        require(owner.history_.max_size()-owner.history_.size()>=4&&owner.sequence_<=UINT64_MAX-4,
            "Grid common journal exhausted");
        const auto sequence=owner.sequence_,size=owner.history_.size();
        owner.history_.reserve(size+4);full();
        require(owner.sequence_==sequence&&owner.history_.size()==size&&owner.history_.capacity()>=size+4,
            "Grid common journal changed across allocator POST");
    }catch(...){if(grid)grid->failed=true;fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::store_grid_common_id_58_owned(const content::XmlNode& layer)const {
    const auto saved=state_;require_storage_without_callbacks(saved);const auto grid=saved->baseline->grid_;
    try{
        const auto first=[&]{require_grid_parser_owned(grid.get());
            require(grid->common_layer_&&*grid->common_layer_<saved->baseline->parser_layers_.size()&&
                BnwWorldViewControlTree::Core::same_layout_xml(layer,saved->baseline->parser_layers_[*grid->common_layer_]),
                "Grid ID58 needs actual current common layer");};
        const auto full=[&]{first();saved->operation->require_current();first();};
        first();auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        const auto found=std::find_if(layer.attributes.begin(),layer.attributes.end(),[](const auto& p){return p.first=="ID";});
        if(found==layer.attributes.end())return; //409390 genuine MISS preserves current58.
        const auto& text=found->second;
        require(text.size()<32&&std::all_of(text.begin(),text.end(),[](unsigned char c){return c>0&&c<128;}),
            "Grid564300 reached unsupported ID allocation/conversion branch");
        if(text.empty()){owner.base_.id_58.reset();first();return;}
        require(!owner.base_.id_58,"Grid existing nonNULL58 mutable buffer update requires distinct owner");
        std::array<char,32> bytes{};std::copy(text.begin(),text.end(),bytes.begin());
        const auto length=static_cast<std::uint32_t>(text.size());
        auto id=std::shared_ptr<const NotificationLayoutControlID58>(new NotificationLayoutControlID58(bytes,length));full();
        require(!owner.base_.id_58&&id->bytes==bytes&&id->length==length,
            "Grid ID58 allocation/store source changed across FULL POST");
        owner.base_.id_58=std::move(id);first();
    }catch(...){if(grid)grid->failed=true;fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::finish_grid_common_empty_layer_owned(std::size_t ordinal)const {
    const auto saved=state_;require_storage_without_callbacks(saved);const auto grid=saved->baseline->grid_;
    try{
        const auto first=[&]{require_grid_parser_owned(grid.get());
            require(grid->common_layer_&&*grid->common_layer_==ordinal,
                "Grid43875E actual reached common layer changed");};
        const auto full=[&]{first();saved->operation->require_current();first();};
        full();auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
        require(core&&occurrence,"Grid actual native suffix Core/occurrence retired");
        const auto xml=BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence;
        const auto receiver=owner.base_.receiver_owned;
        require(core&&occurrence&&receiver&&xml&&!xml->dynamic&&xml->value.children.empty()&&
            owner.parser_layers_[ordinal].children.empty()&&receiver->impl_->grid.lock()==grid,
            "Grid empty common cannot bypass any actual full XML child factory");
        //43875E: allocate495E10 iff real78NULL, INCLUDING zero children.
        if(!owner.base_.children_78){
            auto fields=std::make_shared<NotificationLayoutOrdinaryChildren78::Impl>(receiver);full();
            auto array=std::shared_ptr<NotificationLayoutOrdinaryChildren78>(new NotificationLayoutOrdinaryChildren78(fields));full();
            require(!owner.base_.children_78&&owner.base_.receiver_owned==receiver,
                "Grid actual children78 appeared during allocator POST");
            owner.base_.children_78=std::move(array);first();
        }
        const auto array=owner.base_.children_78;const auto fields=array->impl_;
        require(fields&&fields->creator==std::this_thread::get_id()&&fields->parent.lock()==receiver&&fields->ordered.empty(),
            "Grid actual78 owner/order differs from genuine zero-child branch");
        //8355540 XML cursor,835620 direct count0 ->5E51D0 reserve(current+0),
        //835640 genuine false, OR8, then8353F0/temporary cleanup BEFORE RET.
        struct Cursor final {
            std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
            std::size_t direct_count=0;
            void retire_owned(){selected.reset();}
        } cursor{xml,xml->value.children.size()};
        fields->ordered.reserve(fields->ordered.size()+cursor.direct_count);full();
        require(cursor.direct_count==0&&fields->ordered.empty()&&owner.base_.children_78==array,
            "Grid835640 zero-child iterator changed across actual reserve POST");
        require(grid->common_layer_returns_.size()<grid->common_layer_returns_.max_size(),
            "Grid common layered return storage exhausted");
        const auto returned_count=grid->common_layer_returns_.size();
        grid->common_layer_returns_.reserve(returned_count+1);full();
        require(grid->common_layer_returns_.size()==returned_count,"Grid common return journal changed during reserve");
        owner.base_.flags_40|=8u;first();cursor.retire_owned();full();
        auto data=std::make_shared<NotificationLayoutCommonChildFactoryReturn::Impl>(core,occurrence,xml,receiver,array,saved->serial,0);full();
        auto returned=std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn>(new NotificationLayoutCommonChildFactoryReturn(data));full();
        require(owner.base_.receiver_owned==receiver&&owner.base_.children_78==array&&fields->ordered.empty()&&
            (owner.base_.flags_40&8u)&&grid->common_layer_returns_.size()==returned_count,
            "Grid native suffix current physical stores changed across typed return allocation POST");
        //Reserved pair is nonallocating. Opaque return is the actual completed
        //suffix; scalar parser serial/bool alone cannot substitute this owner.
        grid->common_layer_returns_.emplace_back(ordinal,returned);
        owner.base_.common_return_owned=returned;grid->common_return_=std::move(returned);
        grid->common_layer_.reset();require_grid_parser_owned(grid.get());
    }catch(...){if(grid)grid->failed=true;fail_storage_without_callbacks(saved);throw;}
}
