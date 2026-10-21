//Inside civ5::ui AFTER actual ContextChildSlot10Frame/Binding, standalone
//constructor/parser/common, and real Font/Gui/Realm providers. Root-owned
//after-attach origin definitions are a REQUIRED dependency, not implemented
//or inferred by this leaf. No Embedded/Button178 initializer or Run is reused.
struct detail::NotificationLayoutStandaloneLabelState::InitializationLoan {
    using Parent=NotificationLayoutParentReceiver;
    using Root=NotificationContextRootChildAttachReturn;
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    Parent::TreeView*const view;
    const std::weak_ptr<Parent::TreeView::Binding> binding;
    const Parent::NodeKey key,parent_key;
    NotificationLayoutBaseline*const baseline;
    const std::shared_ptr<NotificationLayoutStandaloneLabelState> label;
    const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> receiver;
    const std::shared_ptr<const Root> parent_return;
    const Root::Guard parent_guard;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial;
    mutable std::uint64_t child_sequence,base_sequence;
    InitializationLoan(Parent::TreeView& actual,Parent::NodeKey target,
        NotificationLayoutBaseline* base,std::shared_ptr<NotificationLayoutStandaloneLabelState> child,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> concrete,
        std::shared_ptr<const Root> returned,Root::Guard guard,Parent::NodeKey actual_parent,std::uint64_t child_before,std::uint64_t base_before)
        :view(&actual),binding(actual.binding_),key(target),parent_key(actual_parent),baseline(base),label(std::move(child)),
         receiver(std::move(concrete)),parent_return(std::move(returned)),parent_guard(std::move(guard)),
         operation(actual.binding_->operation),serial(actual.binding_->serial),
         child_sequence(child_before),base_sequence(base_before){}
    ~InitializationLoan(){alive.store(false);}
    void first()const{
        const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
        require(creator==std::this_thread::get_id()&&alive.load(),"Standalone init lexical slot10 retired/foreign");
        const auto owner=binding.lock();require(owner&&owner->graph_active&&!owner->finished&&view,
            "Standalone init actual View/Binding retired before borrowed access");
        owner->pure(true);require(same(view->binding_,owner),"Standalone init SAME active View/control block changed");
        require(key.domain<owner->domains.size()&&key.id&&serial&&owner->serial==serial&&owner->operation==operation,
            "Standalone init actual Binding domain/serial changed");
        const auto core=owner->domains.at(key.domain).core;core->layout_origin(true);
        const auto& current=core->source->current_operation_loan();core->layout_origin(true);
        require(&current==operation&&current.operation_serial()==serial,
            "Standalone init fresh Source changed before historical pointer reuse");
        current.require_origin_without_callbacks();view->require_pure();
        parent_guard.require_current_without_callbacks();parent_guard.require_source_owned(current);
        require(receiver&&parent_return,"Standalone init after-attach origin absent");
        parent_guard.require_child_receiver_owned(*receiver);view->require_pure();
        require(Parent::node(*view,key).parent&&*Parent::node(*view,key).parent==parent_key,
            "Standalone init SAME actual attached38 parent changed");
        require(Parent::node(*view,key).state==baseline&&baseline&&label&&
            baseline->kind_==NotificationLayoutBaseline::Kind::label&&baseline->actual_type_=="Label"&&
            same(baseline->standalone_label_,label)&&same(baseline->base_.receiver_owned,receiver)&&
            baseline->parsed_source_serial_&&*baseline->parsed_source_serial_&&baseline->sequence_==base_sequence,
            "Standalone init physical Label/ordinary receiver/current Base journal changed");
        const auto fields=label->impl_;fields->first();
        const auto ctor=fields->returned;const auto common=fields->wrapper_returned;
        const auto& node=BnwWorldViewControlTree::Core::node(core->nodes,key.id);
        require(ctor&&common&&fields->sequence==child_sequence&&ctor->source_serial==fields->source_serial&&
            same(ctor->core.lock(),core)&&same(ctor->occurrence.lock(),node.layout_receiver_occurrence)&&
            same(ctor->selected.lock(),node.full_xml_occurrence)&&same(common->core.lock(),core)&&
            same(common->occurrence.lock(),node.layout_receiver_occurrence)&&same(common->selected.lock(),node.full_xml_occurrence)&&
            same(common->receiver.lock(),receiver)&&same(common->children.lock(),baseline->base_.children_78)&&
            same(common->common.lock(),baseline->base_.common_return_owned)&&fields->common_prefix_returned,
            "Standalone init immutable genuine constructor/common ancestry changed");
        //Old common/parser serial/journal authenticates allocation only. It is
        //NOT renewed after genuine Root attach changed child38/3C and journals.
        fields->module->require_label_allocation_owned(current,ctor->ticket,label,true);
        const auto* realm=std::get_if<Impl::ParsedRealm>(&fields->fields[0x3c/4]);
        const auto* array=std::get_if<Impl::ChildrenBorrow>(&fields->fields[0x78/4]);
        const auto* flags=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
        require(realm&&realm->value&&same(realm->value,baseline->base_.context_realm_3c)&&
            same(core->context_realm_identity,realm->value)&&array&&same(array->object,baseline->base_.children_78)&&
            flags&&*flags==baseline->base_.flags_40,
            "Standalone init actual attached3C/78/40 Base aliases changed");
        realm->value->impl_->first(current);fields->first();view->require_pure();
        if(const auto* font=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4])){
            require(bool(font->loan),"Standalone init actual CC descriptor empty");font->loan->require_current_without_callbacks();
        }else require(std::holds_alternative<Impl::Null>(fields->fields[0xcc/4]),"Standalone init unknown CC origin");
        if(const auto* cache=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4])){
            require(bool(cache->loan),"Standalone init actual170 descriptor empty");cache->loan->require_current_without_callbacks();
        }else require(std::holds_alternative<Impl::Null>(fields->fields[0x170/4]),"Standalone init unknown170 origin");
        if(const auto* text=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4])){
            require(text->object&&text->object->creator==creator&&!text->object->retired.load()&&
                same(text->object->allocation_child.lock(),fields)&&text->object->source_serial==fields->source_serial&&
                !text->object->bytes.empty()&&text->object->bytes.size()<=0x4000&&text->object->bytes.back()=='\0'&&
                std::find(text->object->bytes.begin(),text->object->bytes.end(),'\0')==text->object->bytes.end()-1,
                "Standalone init genuine C4 allocation/VALUE changed");
        }else require(std::holds_alternative<Impl::Null>(fields->fields[0xc4/4]),"Standalone init unknown C4 origin");
        view->require_pure();
    }
    void full()const{first();view->require_full();first();}
    float geometry_owned(std::uint32_t offset)const{
        first();const auto* bits=std::get_if<std::uint32_t>(&label->impl_->fields[offset/4]);
        require(bits,"Standalone actual geometry unknown");const auto out=std::bit_cast<float>(*bits);
        require(std::isfinite(out),"Standalone actual geometry nonfinite");return out;
    }
    void put_geometry_owned(std::uint32_t offset,float value)const{
        require(std::isfinite(value),"Standalone native geometry overflow");store(offset);
        auto& base=baseline->base_;
        if(offset>=0x10&&offset<=0x1c)base.bounds_10_1c[(offset-0x10)/4]=value;
        else if(offset==0x20||offset==0x24)base.extent_20_24[(offset-0x20)/4]=value;
        else if(offset==0x28||offset==0x2c)base.offset_28_2c[(offset-0x28)/4]=value;
        else if(offset==0x30||offset==0x34)base.size_30_34[(offset-0x30)/4]=value;
        else require(false,"Standalone geometry store is not a proved native cell");
        label->impl_->fields[offset/4]=std::bit_cast<std::uint32_t>(value);first();
    }
    void require_actual_empty_children_owned()const{
        first();const auto fields=label->impl_;
        const auto* actual=std::get_if<Impl::ChildrenBorrow>(&fields->fields[0x78/4]);
        require(actual&&actual->object&&actual->object==baseline->base_.children_78&&actual->object->impl_&&
            actual->object->impl_->parent.lock()==receiver&&actual->object->impl_->ordered.empty(),
            "Standalone actual common78 positive empty array custody changed");
        const auto children=Parent::children(*view,key);full();require(children.empty(),
            "Standalone genuine78 childv10/4A7F10 recursive supplier required");
    }

    void store(std::uint32_t offset)const{
        full();view->preflight_store_owned(key);full();
        auto& fields=*label->impl_;require(fields.sequence<UINT64_MAX&&fields.history.size()<fields.history.capacity()&&
            baseline->sequence_<UINT64_MAX&&baseline->history_.size()<baseline->history_.capacity(),
            "Standalone init paired journals not preflighted");
        fields.history.push_back({++fields.sequence,offset});child_sequence=fields.sequence;
        baseline->record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);base_sequence=baseline->sequence_;
        view->record_store_owned(key,(offset==0x28u||offset==0x2cu)?Parent::Store::Kind::base_anchor:Parent::Store::Kind::standalone_label);first();
    }
};
struct detail::NotificationLayoutStandaloneLabelState::InitializerFieldsReturn {
    const std::uint64_t operation_serial,child_sequence,base_sequence;
    const std::weak_ptr<Impl> owner;
    const Impl::Cell*const actual_c4;
    const std::shared_ptr<Impl::TextObject> text; //Genuine constructorNULL allowed.
    const std::vector<char> value; //Owned exact real C4 bytes; NULL is empty.
    const std::shared_ptr<const BnwCompletedFontLoan> font;
    const std::shared_ptr<const BnwLabelSystemFontBorrower> system; //Real lookupNULL only.
    const std::weak_ptr<const NotificationContextRootChildAttachReturn> attached;
    InitializerFieldsReturn(const InitializationLoan& loan,std::shared_ptr<Impl::TextObject> t,std::vector<char> v,
        std::shared_ptr<const BnwCompletedFontLoan> f,std::shared_ptr<const BnwLabelSystemFontBorrower> s)
        :operation_serial(loan.serial),child_sequence(loan.child_sequence),base_sequence(loan.base_sequence),
         owner(loan.label->impl_),actual_c4(&loan.label->impl_->fields[0xc4/4]),text(std::move(t)),
         value(std::move(v)),font(std::move(f)),system(std::move(s)),attached(loan.parent_return){}
};
void detail::NotificationLayoutStandaloneLabelState::initialize_slot10_prefix_owned(
    const std::shared_ptr<InitializationLoan>& loan){
    const auto fields=impl_;require(loan&&loan->label.get()==this,"Standalone710270 actual issuer absent");
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto current=[&]{loan->first();fields->first();};
    try{
        loan->full();require(!fields->initialization_entered&&!fields->initializer_fields_returned,
            "Standalone710270 initializer occurrence replay");
        require(fields->sequence<=UINT64_MAX-32&&fields->history.max_size()-fields->history.size()>=32&&
            loan->baseline->sequence_<=UINT64_MAX-32&&
            loan->baseline->history_.max_size()-loan->baseline->history_.size()>=32,
            "Standalone710270 journals exhausted before actual effect");
        fields->history.reserve(fields->history.size()+32);loan->full();
        loan->baseline->history_.reserve(loan->baseline->history_.size()+32);loan->full();
        fields->initialization_entered=true;
        //710273 actualCC positive branch, no guessed Context538/default Font.
        const auto* cc=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
        if(!cc){require(std::holds_alternative<Impl::Null>(fields->fields[0xcc/4]),"Standalone710273 unknown actual CC");
            throw std::runtime_error("Standalone71027F genuine Context538/Gui font_default→6F3CD0 required");}
        const auto font=cc->loan;require(bool(font),"Standalone710273 empty actual CC descriptor");
        font->require_source_owned(*loan->operation);loan->full();
        const auto binding=loan->binding.lock();require(bool(binding),"Standalone7102B0 active Binding retired");
        const auto core=binding->domains.at(loan->key.domain).core;
        const auto gui=core->context_gui_port;require(bool(gui),"Standalone7102B0 genuine Gui retired");
        auto flags=std::get<std::uint32_t>(fields->fields[0x40/4]);bool language=false;
        if(!(flags&0x40000000u)){
            language=gui->label_language_980a_owned(*loan->operation)!=0;loan->full();
            flags=std::get<std::uint32_t>(fields->fields[0x40/4]);
        }
        std::shared_ptr<const BnwLabelSystemFontBorrower> system;
        if((flags&0x40000000u)||(language&&!(flags&0x20000000u))){
            system=gui->begin_system_font_lookup_owned(*loan->operation,*font);loan->full();
            font->require_source_owned(*loan->operation);loan->full();
            if(system){system->require_source_owned(*loan->operation);loan->full();}
        }
        loan->store(0x170);fields->fields[0x170/4]=system?Impl::Cell{Impl::SystemFontBorrow{system}}:Impl::Cell{Impl::Null{}};current();
        const auto* realm_cell=std::get_if<Impl::ParsedRealm>(&fields->fields[0x3c/4]);
        require(realm_cell&&realm_cell->value&&fields->style_c8.has_value(),"Standalone7102FC true Context3C/C8 absent");
        const auto realm=realm_cell->value;
        if(!*fields->style_c8){
            auto style=realm->default_font_style_owned(*loan->operation);loan->full();
            font->require_source_owned(*loan->operation);loan->full();
            const auto ordinal=font->style_ordinal_without_callbacks(style);loan->full();
            loan->store(0xc8);fields->style_c8=std::uint8_t(ordinal);current();
            //710329 calls6A2EA0 before5761. Only actual bit1false earlyRET is
            //implemented here; no initialized/rebuild/Run authority is minted.
            require(!(std::get<std::uint32_t>(fields->fields[0x40/4])&1u),
                "Standalone710329 initialized6A2EA0 producer required");
        }
        if(!(std::get<std::uint32_t>(fields->fields[0x40/4])&0x04000000u)){
            const auto colors=core->default_context_colors_owned(*loan->operation);loan->full();
            for(std::size_t word=0;word<16;++word){
                std::optional<std::uint32_t> bits=std::uint32_t{0};
                for(std::size_t byte=0;byte<4;++byte){const auto& value=colors[word*4+byte];
                    if(!value){bits.reset();break;}*bits|=std::uint32_t(*value)<<(8*byte);}
                loan->store(0x84+std::uint32_t(word*4));
                if(bits)fields->fields[0x84/4+word]=*bits;else fields->fields[0x84/4+word]=std::monostate{};
                if(word==15)for(std::size_t byte=0;byte<4;++byte)fields->color_metadata_c0_c3[byte]=colors[60+byte];
                current();
            }
        }
        const auto c4=&fields->fields[0xc4/4];std::shared_ptr<Impl::TextObject> text;
        if(const auto* actual=std::get_if<Impl::TextBorrow>(c4)){text=actual->object;
            require(text&&text->creator==loan->creator&&!text->retired.load()&&
                same(text->allocation_child.lock(),fields)&&text->source_serial==fields->source_serial&&
                !text->bytes.empty()&&text->bytes.back()=='\0'&&
                std::find(text->bytes.begin(),text->bytes.end(),'\0')==text->bytes.end()-1,
                "Standalone710357 actual C4 owner/VALUE retired");
        }else require(std::holds_alternative<Impl::Null>(*c4),"Standalone710357 C4 has no genuine NULL/parser issuer");
        loan->full();std::vector<char> value;if(text){value=text->bytes;loan->full();
            require(value==text->bytes,"Standalone prefix C4 VALUE copy allocating POST changed");}
        auto result=std::shared_ptr<const InitializerFieldsReturn>(
            new InitializerFieldsReturn(*loan,text,std::move(value),font,system));loan->full();
        const auto* current_cc=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
        const auto* current_system=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4]);
        const auto* current_text=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4]);
        require(same(result->owner.lock(),fields)&&result->actual_c4==c4&&result->child_sequence==fields->sequence&&
            result->base_sequence==loan->baseline->sequence_&&current_cc&&same(current_cc->loan,font)&&
            ((system&&current_system&&same(current_system->loan,system))||(!system&&std::holds_alternative<Impl::Null>(fields->fields[0x170/4])))&&
            ((text&&current_text&&same(current_text->object,text))||(!text&&std::holds_alternative<Impl::Null>(fields->fields[0xc4/4])))&&
            ((!text&&result->value.empty())||(text&&result->value==text->bytes))&&
            !fields->initializer_fields_returned,"Standalone prefix actual C4/CC/170 allocation POST changed");
        fields->initializer_fields_returned=std::move(result);loan->full();
        //A typed DATA prefix is not a successful native slot10 return.
    }catch(...){fields->failed.store(true);if(const auto b=loan->binding.lock())b->poison();throw;}
}
#include "notification_layout_standalone_label_text_lane_core.inc"
#include "notification_layout_standalone_label_geometry_core.inc"
#include "notification_layout_standalone_label_text_stages_core.inc"
#include "notification_layout_standalone_label_complete_core.inc"
#include "notification_layout_standalone_positioned_run_core.inc"
#include "notification_layout_standalone_label_slot10_adapter_core.inc"
