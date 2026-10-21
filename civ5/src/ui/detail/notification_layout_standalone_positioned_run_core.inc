//Actual standalone full initializer DATA and publication. No embedded Run,
//Factory/Init authority, prepared SystemFont token or paint frame is reused.
struct detail::BnwStandalonePositionedLabelRunLoan::Impl {
    using Label=NotificationLayoutStandaloneLabelState;using F=Label::Impl;
    const std::thread::id creator=std::this_thread::get_id();std::atomic<bool> alive{true};
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<Label> child;
    const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> receiver;
    const std::shared_ptr<const Label::ConstructorReturn> allocation;
    const std::shared_ptr<F::TextObject> text;
    const std::shared_ptr<const BnwCompletedFontLoan> font;
    const std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    const std::uint64_t id,sequence,base_sequence,topology,constructor_serial;
    struct Value{
        std::vector<char> c4;std::vector<F::PositionedGlyph> glyphs;
        std::array<std::uint32_t,10> geometry;
        std::array<std::uint32_t,8> descriptor;
        std::array<std::uint32_t,12> raw_colors;
        std::uint32_t count,flags,field_44;std::uint8_t style,newline;
        std::int16_t wrap,leading,truncate;
    };
    const Value value;
    struct PublishedReturn {std::uint64_t source_serial,receiver_return_sequence,topology,effects;};
    std::optional<PublishedReturn> published;
    static Label::InitializationLoan& checked(const std::shared_ptr<Label::InitializationLoan>& loan){
        require(bool(loan),"Standalone Run constructor no genuine lexical owner");loan->first();return *loan;
    }
    static Value capture_value(const std::shared_ptr<Label::InitializationLoan>& loan){
        loan->full();const auto fields=loan->label->impl_;require(fields->init_stage==F::InitStage::bounds_returned,
            "Standalone Run producer no genuine complete slot10 body");
        const auto* cell=std::get_if<F::TextBorrow>(&fields->fields[0xc4/4]);require(cell&&cell->object,
            "Standalone Run no real C4 producer");const auto text=cell->object;const auto sequence=fields->sequence;
        const auto current=[&]{loan->first();require(fields->init_stage==F::InitStage::bounds_returned&&
            fields->sequence==sequence&&!text->retired.load(),"Standalone Run capture actual completed state changed");};
        Value out;out.c4=text->bytes;loan->full();current();require(out.c4==text->bytes,"Standalone Run C4 allocating POST changed");
        out.glyphs=fields->positioned_glyphs;loan->full();current();
        const auto word=[&](std::uint32_t offset){current();const auto* cell=std::get_if<std::uint32_t>(&fields->fields[offset/4]);
            require(cell,"Standalone Run actual known word unavailable");return *cell;};
        for(unsigned i=0;i<10;++i)out.geometry[i]=word(0x10+4*i);
        constexpr std::array<std::uint32_t,8> offsets{0x140,0x144,0x148,0x14c,0x150,0x158,0x15c,0x160};
        for(unsigned i=0;i<offsets.size();++i)out.descriptor[i]=word(offsets[i]);
        for(unsigned i=0;i<12;++i)out.raw_colors[i]=word(0x84+4*i);
        out.count=word(0x138);out.flags=word(0x40);out.field_44=word(0x44);
        require(fields->style_c8&&fields->inline_segment_newline_154&&fields->wrap_168&&fields->leading_16a&&fields->truncate_16c,
            "Standalone Run genuine scalar fields unavailable");
        out.style=*fields->style_c8;out.newline=*fields->inline_segment_newline_154;
        out.wrap=*fields->wrap_168;out.leading=*fields->leading_16a;out.truncate=*fields->truncate_16c;
        loan->full();current();require(out.c4==text->bytes,"Standalone Run final capture Source POST changed");return out;
    }
    Impl(const std::shared_ptr<Label::InitializationLoan>& loan,
        std::shared_ptr<F::TextObject> object,std::shared_ptr<const BnwCompletedFontLoan> cc,
        std::shared_ptr<const BnwLabelSystemFontBorrower> cache)
        :core(checked(loan).binding.lock()->domains.at(loan->key.domain).core),child(loan->label),receiver(loan->receiver),
         allocation(loan->label->impl_->returned),text(std::move(object)),font(std::move(cc)),system(std::move(cache)),
         id(loan->key.id),sequence(loan->child_sequence),base_sequence(loan->base_sequence),
         topology(loan->binding.lock()->domains.at(loan->key.domain).topology),
         constructor_serial(loan->label->impl_->source_serial),value(capture_value(loan)){}
    void first_for_paint(const runtime::BnwWorldViewSourceOperationLoan&,
        const std::shared_ptr<const BnwWorldViewControlTree>&,const BnwLabelActualPaintCallFrame::Guard&)const;
    void first_fields(const runtime::BnwWorldViewSourceOperationLoan& operation,bool entered,
        const BnwLabelActualPaintCallFrame::Guard* actual_paint=nullptr)const{
        require(creator==std::this_thread::get_id()&&alive.load(),"Standalone Run actual DATA retired/foreign");
        const auto owner=core.lock();require(bool(owner),"Standalone Run actual Core retired");owner->layout_origin(entered);
        const auto& current=owner->source->current_operation_loan();owner->layout_origin(entered);
        require(&current==&operation,"Standalone Run fresh Source replaced before borrowed operand access");
        current.require_origin_without_callbacks();
        const auto actual=child.lock();require(bool(actual),"Standalone Run actual child retired");const auto fields=actual->impl_;fields->first();
        const auto& node=BnwWorldViewControlTree::Core::node(owner->nodes,id);
        const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
        require(node.layout_baseline&&same(node.layout_baseline->standalone_label_,actual)&&
            same(node.layout_baseline->base_.receiver_owned,receiver)&&fields->sequence==sequence&&
            node.layout_baseline->sequence_==base_sequence&&owner->topology_revision==topology&&
            fields->source_serial==constructor_serial&&same(fields->returned,allocation)&&
            fields->init_stage==F::InitStage::bounds_returned,"Standalone Run canonical allocation/control/revision changed");
        const auto* realm=std::get_if<F::ParsedRealm>(&fields->fields[0x3c/4]);
        const auto* children=std::get_if<F::ChildrenBorrow>(&fields->fields[0x78/4]);
        require(realm&&realm->value&&same(realm->value,node.layout_baseline->base_.context_realm_3c)&&
            same(realm->value,owner->context_realm_identity)&&children&&
            same(children->object,node.layout_baseline->base_.children_78),
            "Standalone Run actual attached3C/78 aliases changed");
        realm->value->impl_->first(current);owner->layout_origin(entered);
        if(actual_paint){require(entered,"Standalone Paint fields require actual entered Frame");
            actual_paint->require_current_without_callbacks();
            fields->module->require_standalone_label_live_allocation_without_callbacks(
                current,allocation->ticket,actual,constructor_serial);actual_paint->require_current_without_callbacks();}
        else fields->module->require_label_allocation_owned(current,allocation->ticket,actual,true);
        const auto* c4=std::get_if<F::TextBorrow>(&fields->fields[0xc4/4]);
        const auto* cc=std::get_if<F::FontBorrow>(&fields->fields[0xcc/4]);
        const auto* cache=std::get_if<F::SystemFontBorrow>(&fields->fields[0x170/4]);
        require(c4&&same(c4->object,text)&&text&&!text->retired.load()&&text->bytes==value.c4&&
            same(text->allocation_child.lock(),fields)&&cc&&same(cc->loan,font)&&
            ((system&&cache&&same(cache->loan,system))||(!system&&std::holds_alternative<F::Null>(fields->fields[0x170/4]))),
            "Standalone Run physical C4/CC/170 owners or VALUE changed");
        const auto word=[&](std::uint32_t offset){const auto* cell=std::get_if<std::uint32_t>(&fields->fields[offset/4]);require(cell,"Standalone Run known word retired");return *cell;};
        for(unsigned i=0;i<10;++i)require(word(0x10+4*i)==value.geometry[i],"Standalone Run geometry changed");
        constexpr std::array<std::uint32_t,8> offsets{0x140,0x144,0x148,0x14c,0x150,0x158,0x15c,0x160};
        for(unsigned i=0;i<offsets.size();++i)require(word(offsets[i])==value.descriptor[i],"Standalone Run descriptor changed");
        for(unsigned i=0;i<12;++i)require(word(0x84+4*i)==value.raw_colors[i],"Standalone Run colors changed");
        require(word(0x44)==value.field_44,"Standalone Run physical word44 changed");
        require(word(0x138)==value.count&&word(0x40)==value.flags&&fields->style_c8==value.style&&
            fields->inline_segment_newline_154==value.newline&&fields->wrap_168==value.wrap&&
            fields->leading_16a==value.leading&&fields->truncate_16c==value.truncate&&
            fields->positioned_glyphs.size()==value.glyphs.size(),"Standalone Run actual record/text fields changed");
        for(std::size_t i=0;i<value.glyphs.size();++i){const auto& a=fields->positioned_glyphs[i];const auto& b=value.glyphs[i];
            require(a.byte_begin==b.byte_begin&&a.byte_count==b.byte_count&&a.codepoint==b.codepoint&&
                a.lookup_codepoint==b.lookup_codepoint&&a.cursor==b.cursor&&a.bits==b.bits,"Standalone Run positioned glyph changed");}
        font->require_current_without_callbacks();if(system)system->require_current_without_callbacks();owner->layout_origin(entered);
    }
};
detail::BnwStandalonePositionedLabelRunLoan::BnwStandalonePositionedLabelRunLoan(std::shared_ptr<Impl> data):impl_(std::move(data)){}
detail::BnwStandalonePositionedLabelRunLoan::~BnwStandalonePositionedLabelRunLoan(){impl_->alive.store(false);}
void detail::BnwStandalonePositionedLabelRunLoan::require_current_data_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto saved=impl_;saved->first_fields(operation,false);
    const auto fields=saved->child.lock()->impl_;
    require(saved->published&&saved->published->source_serial&&saved->published->receiver_return_sequence&&
        saved->published->topology==saved->topology&&fields->completed_standalone_run.get()==this,
        "Standalone Run no actual post-DTOR typed published return");saved->first_fields(operation,false);
}
std::shared_ptr<detail::BnwStandalonePositionedLabelRunLoan>
detail::NotificationLayoutStandaloneLabelState::pending_positioned_run_owned(const std::shared_ptr<InitializationLoan>& loan){
    loan->full();require(loan->label.get()==this&&impl_->init_stage==Impl::InitStage::bounds_returned,
        "Standalone Run actual initializer producer absent");
    const auto* c4=std::get_if<Impl::TextBorrow>(&impl_->fields[0xc4/4]);const auto* cc=std::get_if<Impl::FontBorrow>(&impl_->fields[0xcc/4]);
    require(c4&&c4->object&&cc&&cc->loan,"Standalone Run actual operands absent");const auto text=c4->object;const auto font=cc->loan;
    std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    if(const auto* cache=std::get_if<Impl::SystemFontBorrow>(&impl_->fields[0x170/4]))system=cache->loan;
    auto data=std::shared_ptr<BnwStandalonePositionedLabelRunLoan::Impl>(new BnwStandalonePositionedLabelRunLoan::Impl(loan,text,font,system));
    loan->full();data->first_fields(*loan->operation,true);
    auto result=std::shared_ptr<BnwStandalonePositionedLabelRunLoan>(new BnwStandalonePositionedLabelRunLoan(data));
    loan->full();data->first_fields(*loan->operation,true);return result; //Pending only, no published receipt.
}
#include "notification_layout_standalone_entered_paint_run_core.inc"
#include "bnw_normal_label_paint_invocation_core.inc"
#include "bnw_standalone_run_paint_data_core.inc"
