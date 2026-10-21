//Inside civ5::ui AFTER complete standalone ParserLoan and selected Font/Gui providers.
void detail::NotificationLayoutRuntimeDimensions::run_standalone_label_common_prefix_owned()const{
    using Label=NotificationLayoutStandaloneLabelState;using Loan=Label::ParserLoan;
    const auto saved=state_;const auto loan=saved->active_standalone_parser.lock();
    require(bool(loan),"Standalone438170 needs genuine current45EA90 lexical callframe");loan->full();
    const auto label=loan->label;const auto fields=label->impl_;const auto core=saved->core.lock();
    const auto occurrence=saved->occurrence.lock();auto& base=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
    const auto pure=[&]{loan->first();require(core&&occurrence&&loan->stage==Loan::Stage::common_required&&
        fields->fields_returned&&fields->fields_returned->serial==loan->serial&&
        fields->fields_returned->selected.lock()==loan->selected&&base.standalone_label_.get()==label.get()&&
        !base.standalone_label_.owner_before(label)&&!label.owner_before(base.standalone_label_)&&
        !base.base_.common_return_owned&&!base.base_.children_78,"Standalone438170 genuine5E/receiver/phase changed");};
    const auto full=[&]{pure();saved->operation->require_current();pure();};
    try{
        pure();require(!fields->common_prefix_entered&&!fields->common_prefix_returned&&!base.base_.receiver_owned&&
            fields->fields_returned->sequence==fields->sequence,"Standalone438170 replay or stale fields receipt");
        fields->common_prefix_entered=true;full();
        require(fields->history.max_size()-fields->history.size()>=128&&base.history_.max_size()-base.history_.size()>=64,
            "Standalone common actual journals exhausted");
        fields->history.reserve(fields->history.size()+128);full();base.history_.reserve(base.history_.size()+64);full();
        const auto receiver_id=saved->id,serial=saved->serial;
        auto pending=std::unique_ptr<NotificationLayoutOrdinaryReceiver::Impl>(
            new NotificationLayoutOrdinaryReceiver::Impl(core,occurrence,label,receiver_id,serial));full();
        auto receiver_fields=std::shared_ptr<NotificationLayoutOrdinaryReceiver::Impl>(std::move(pending));full();
        auto receiver=std::shared_ptr<const NotificationLayoutOrdinaryReceiver>(new NotificationLayoutOrdinaryReceiver(receiver_fields));full();
        const auto actual_label=receiver_fields->standalone_label.lock();
        require(receiver_fields->core.lock()==core&&receiver_fields->occurrence.lock()==occurrence&&
            receiver_fields->id==receiver_id&&receiver_fields->serial==serial&&actual_label.get()==label.get()&&
            !actual_label.owner_before(label)&&!label.owner_before(actual_label)&&receiver_fields->image.expired()&&
            receiver_fields->box.expired()&&!base.base_.receiver_owned,"Standalone genuine receiver changed during allocation POST");
        receiver_fields->canonical=receiver;base.base_.receiver_owned=receiver;full();
        const auto current=[&]{pure();const auto actual=receiver_fields->standalone_label.lock();
            require(base.base_.receiver_owned.get()==receiver.get()&&!base.base_.receiver_owned.owner_before(receiver)&&
                !receiver.owner_before(base.base_.receiver_owned)&&receiver_fields->canonical.lock()==receiver&&
                actual.get()==label.get()&&!actual.owner_before(label)&&!label.owner_before(actual),
                "Standalone common SAME actual receiver/control replaced");};
        const auto post=[&]{current();saved->operation->require_current();current();};
        const auto attr=[&](std::string_view name)->std::optional<std::string>{current();std::optional<std::string> out;
            for(const auto& a:loan->layer().attributes)if(a.first==name){out=a.second;break;}post();return out;};
        const auto store=[&](std::uint32_t offset){loan->record(offset);loan->base_record(NotificationLayoutBaseline::Mutation::receiver_prefix);};
        const auto flag=[&](std::uint32_t bits){store(0x40);std::get<std::uint32_t>(fields->fields[0x40/4])|=bits;
            base.base_.flags_40=std::get<std::uint32_t>(fields->fields[0x40/4]);current();};
        //Real4381AC repeats the actual second argument store after5E.
        store(0x3c);fields->fields[0x3c/4]=Label::Impl::ParsedRealm{loan->argument->impl_->realm};
        base.base_.context_realm_3c=loan->argument->impl_->realm;
        base.base_.context_3c_observation=NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm;current();
        const auto tokens=[&](std::string_view text){
            //Finite selected76DC50 integral grammar only; no fabricated decimal/Full fallback.
            const auto comma=text.find(',');require(comma!=text.npos&&text.find(',',comma+1)==text.npos,
                "Standalone common pair needs proven selected two comma tokens");
            return std::array<std::string_view,2>{text.substr(0,comma),text.substr(comma+1)};
        };
        const auto integral=[&](std::string_view text){std::int32_t n=0;const auto p=std::from_chars(text.data(),text.data()+text.size(),n);
            require(!text.empty()&&p.ec==std::errc{}&&p.ptr==text.data()+text.size()&&n>=-16777216&&n<=16777216&&
                (_mm_getcsr()&0xe040u)==0,"Standalone76DC50 current SSE/integral exact-float grammar unsupported");
            return static_cast<float>(n);};
        const auto tuple=[&](std::string_view name,std::uint32_t offset,std::array<float,2>& destination){
            if(const auto text=attr(name)){const auto parts=tokens(*text);
                for(std::size_t axis=0;axis<2;++axis){
                    require(parts[axis].empty()||(parts[axis].front()!='F'&&parts[axis].front()!='f'),
                        "Standalone76DC50 Full axis requires genuine86AC20 registration supplier");
                    const auto value=integral(parts[axis]);store(offset+std::uint32_t(axis*4));
                    fields->fields[(offset+axis*4)/4]=std::bit_cast<std::uint32_t>(value);destination[axis]=value;current();
                }
            }
        };
        tuple("Offset",0x70,base.base_.margin_70_74);tuple("Size",0x30,base.base_.size_30_34);
        require((base.base_.flags_40&0x300000u)==0,"Standalone common accumulated Full registration required");
        const auto upper=[](std::string_view text){require(!text.empty(),"Standalone anchor token empty");const auto c=text.front();
            return c>='a'&&c<='z'?char(c-'a'+'A'):c;};
        if(const auto value=attr("Anchor")){const auto p=tokens(*value);const auto x=upper(p[0]),y=upper(p[1]);
            require((x=='L'||x=='C'||x=='R')&&(y=='T'||y=='C'||y=='B'),"Standalone native anchor outside selected profile");
            flag((x=='C'?0x10u:x=='R'?0x20u:0u)|(y=='C'?0x80u:y=='B'?0x100u:0u));}
        if(const auto value=attr("AnchorSide")){const auto p=tokens(*value);flag((upper(p[0])=='O'?0x40u:0u)|(upper(p[1])=='O'?0x200u:0u));}
        if(const auto value=attr("Color")){
            const auto rgba=standalone_label_color_owned(*value);post();(void)rgba;
            //A48DD0+58 physically points412770, whose entire body RET16 is
            //proven directly in PE. Actual580780 query has happened; no Label
            //color store/callback exists in this COMMON virtual slot.
        }
        if(const auto value=attr("ID")){
            require(value->size()<32&&std::all_of(value->begin(),value->end(),[](unsigned char c){return c>0&&c<128;}),
                "Standalone564300 selected terminated ASCII ID shorter32 required");
            const auto* constructor_id=std::get_if<std::uint32_t>(&fields->fields[0x58/4]);
            require(constructor_id&&*constructor_id==0u&&!base.base_.id_58,
                "Standalone actual ID58 needs genuine constructor DWORD zero; mutable replacement supplier required");
            if(!value->empty()){
                std::array<char,32> bytes{};std::copy(value->begin(),value->end(),bytes.begin());const auto size=std::uint32_t(value->size());
                auto id=std::shared_ptr<const NotificationLayoutControlID58>(new NotificationLayoutControlID58(bytes,size));post();
                require(id->bytes==bytes&&id->length==size&&!base.base_.id_58,"Standalone ID allocator POST changed operands");
                store(0x58);fields->fields[0x58/4]=Label::Impl::IDBorrow{id};base.base_.id_58=id;current();
            } //Real empty release branch preserves genuine constructor NULL.
        }
        const auto truth=[](std::string_view s){return !s.empty()&&(s.front()=='1'||s.front()=='t'||s.front()=='T');};
        //Actual4382EB..43854E lookup order, no invented default flags.
        for(const auto& [name,bits]:std::array<std::pair<std::string_view,std::uint32_t>,8>{{
            {"Hidden",2u},{"NeedsMouseOver",0x8000u},{"NoClip",0x40000u},{"ConsumeMouse",0x1c00u},
            {"ConsumeMouseButton",0x400u},{"ConsumeMouseOver",0x800u},{"ConsumeMouseWheel",0x1000u},
            {"Disabled",0x20000u}}}){
            if(const auto value=attr(name);value&&truth(*value))flag(bits);
        }
        require(!attr("BranchAlpha").has_value(),"Standalone43856B real81ED50 x87 scalar BranchAlpha supplier required");
        if(const auto value=attr("ToolTip")){
            const auto localized=standalone_label_text_owned(*value);post();
            require(localized.size()<4000&&localized.find('\0')==std::string::npos,
                "Standalone5F3B90 selected nonNULL terminated tooltip shorter4000 required");
            //5F3B90 old54 release, OR4000, allocator/copy, then GUI524CC0.
            store(0x54);fields->fields[0x54/4]=Label::Impl::Null{};base.base_.tooltip_54.reset();current();flag(0x4000u);
            const std::string text=localized;post();
            auto tooltip=std::make_shared<const std::string>(text);post();
            require(*tooltip==text&&!base.base_.tooltip_54,"Standalone54 allocation POST changed actual pointer");
            store(0x54);fields->fields[0x54/4]=Label::Impl::TooltipBorrow{tooltip};base.base_.tooltip_54=tooltip;current();
            const auto manager=saved->manager.lock();require(bool(manager),"Standalone actual GUI524CC0 receiver retired");
            manager->refresh_tooltip_if_hovered_owned(*saved->operation,*saved->tree_facade,saved->id);post();
        }
        if(const auto value=attr("ToolTipType")){const auto hash=content::saved_info_type_hash(*value);flag(0x4000u);
            store(0x48);fields->fields[0x48/4]=hash;base.base_.tooltip_type_48=hash;current();}
        post();fields->common_prefix_returned=true;post();
        run_standalone_label_common_empty_children_owned();return;
    }catch(...){fields->failed.store(true);loan->alive.store(false);fail_storage_without_callbacks(saved);throw;}
}
