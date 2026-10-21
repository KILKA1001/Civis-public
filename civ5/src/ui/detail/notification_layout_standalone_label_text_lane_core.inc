//Disjoint standalone actual C4/CC/170 lane. No Embedded TextObject, child,
//factory, initialization or historical metadata is an issuer in this branch.
struct detail::BnwLabelCurrentTextLoan::StandaloneImpl {
    using Label=NotificationLayoutStandaloneLabelState;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<Label::InitializationLoan> initialization;
    const std::weak_ptr<Label::Impl> child;
    const std::shared_ptr<Label::Impl::TextObject> text;
    const Label::Impl::Cell*const actual_c4;
    const std::shared_ptr<const BnwCompletedFontLoan> cc;
    const std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial;
    const Phase phase;
    const std::vector<char> value;
    StandaloneImpl(std::shared_ptr<Label::InitializationLoan> init,
        std::shared_ptr<Label::Impl::TextObject> object,std::shared_ptr<const BnwCompletedFontLoan> font,
        std::shared_ptr<const BnwLabelSystemFontBorrower> cache,Phase mode,std::vector<char> bytes)
        :initialization(init),child(init->label->impl_),text(std::move(object)),
         actual_c4(&init->label->impl_->fields[0xc4/4]),cc(std::move(font)),system(std::move(cache)),
         operation(init->operation),serial(init->serial),phase(mode),value(std::move(bytes)){}
    void first()const{
        require(creator==std::this_thread::get_id(),"Standalone C4 lexical input foreign creator");
        const auto init=initialization.lock();require(bool(init),"Standalone C4 actual initializer retired");init->first();
        const auto fields=child.lock();require(fields&&fields==init->label->impl_&&init->operation==operation&&
            init->serial==serial&&&fields->fields[0xc4/4]==actual_c4,"Standalone C4 actual owner/Source/field changed");
        const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
        const auto* object=std::get_if<Label::Impl::TextBorrow>(actual_c4);
        const auto* font=std::get_if<Label::Impl::FontBorrow>(&fields->fields[0xcc/4]);
        const auto* cache=std::get_if<Label::Impl::SystemFontBorrow>(&fields->fields[0x170/4]);
        require(object&&text&&same(object->object,text)&&text->creator==creator&&!text->retired.load()&&
            same(text->allocation_child.lock(),fields)&&text->source_serial==fields->source_serial&&
            !value.empty()&&value.back()=='\0'&&value==text->bytes&&
            font&&cc&&same(font->loan,cc)&&cache&&system&&same(cache->loan,system),
            "Standalone C4 physical allocation/immutable VALUE/Font170 controls changed");
        require((phase==Phase::position_7505&&fields->init_stage==Label::Impl::InitStage::glyph_generated)||
            (phase==Phase::size_5077&&fields->init_stage==Label::Impl::InitStage::positioned_returned),
            "Standalone C4 no genuine native text phase");
        //The real reset138==0 is checked ONLY by the producer below. Actual
        //7505 may append its descriptor before later prepare/glyph callbacks.
        cc->require_current_without_callbacks();init->first();system->require_current_without_callbacks();init->first();
    }
};
detail::BnwLabelCurrentTextLoan::Impl::Impl(std::shared_ptr<StandaloneImpl> data)
    :standalone(data),initialization(),child(),text(),actual_c4_address(nullptr),
     cc(data->cc),system(data->system),operation(data->operation),operation_serial(data->serial),
     phase(data->phase),value(data->value){first();}
void detail::BnwLabelCurrentTextLoan::Impl::first_standalone_owned()const{
    require(creator==std::this_thread::get_id()&&alive.load()&&standalone,
        "Standalone C4 wrapper retired/foreign");standalone->first();
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(!initialization.lock()&&!child.lock()&&!text&&!actual_c4_address&&
        same(cc,standalone->cc)&&same(system,standalone->system)&&operation==standalone->operation&&
        operation_serial==standalone->serial&&phase==standalone->phase&&value==standalone->value,
        "Standalone C4 wrapper cannot contain Embedded aliases or substituted DATA");standalone->first();
}
std::unique_ptr<detail::BnwLabelCurrentTextLoan>
detail::NotificationLayoutStandaloneLabelState::current_text_owned(
    const std::shared_ptr<InitializationLoan>& loan,bool for_size)const{
    require(loan&&loan->label.get()==this,"Standalone C4 real issuer absent");loan->full();
    const auto phase=[&]{loan->first();require(impl_->init_stage==(for_size?Impl::InitStage::positioned_returned:Impl::InitStage::glyph_generated),
        "Standalone C4 producer not at real7505/5077 boundary");
        if(!for_size){const auto* count=std::get_if<std::uint32_t>(&impl_->fields[0x138/4]);
            require(count&&!*count,"Standalone C4 position producer no genuine138 reset");}};phase();
    const auto* object=std::get_if<Impl::TextBorrow>(&impl_->fields[0xc4/4]);
    const auto* font=std::get_if<Impl::FontBorrow>(&impl_->fields[0xcc/4]);
    const auto* cache=std::get_if<Impl::SystemFontBorrow>(&impl_->fields[0x170/4]);
    require(object&&object->object&&font&&font->loan&&cache&&cache->loan,"Standalone C4 actual operands missing");
    const auto text=object->object;const auto cc=font->loan;const auto system=cache->loan;
    const auto value=text->bytes;loan->full();phase();require(value==text->bytes,"Standalone C4 allocating VALUE copy changed");
    auto lane=std::shared_ptr<BnwLabelCurrentTextLoan::StandaloneImpl>(new BnwLabelCurrentTextLoan::StandaloneImpl(
        loan,text,cc,system,for_size?BnwLabelCurrentTextLoan::Phase::size_5077:BnwLabelCurrentTextLoan::Phase::position_7505,value));
    loan->full();phase();lane->first();
    auto data=std::shared_ptr<BnwLabelCurrentTextLoan::Impl>(new BnwLabelCurrentTextLoan::Impl(lane));loan->full();phase();data->first();
    auto result=std::unique_ptr<BnwLabelCurrentTextLoan>(new BnwLabelCurrentTextLoan(data));loan->full();phase();data->first();
    return result;
}
