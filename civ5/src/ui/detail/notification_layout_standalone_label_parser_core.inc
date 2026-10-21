//Inside civ5::ui AFTER genuine standalone factory, selected Style/Color/Gui and tooltip providers.
struct detail::NotificationLayoutStandaloneLabelState::ParserLoan {
    const std::thread::id creator=std::this_thread::get_id();std::atomic<bool> alive{true};
    const std::shared_ptr<NotificationLayoutRuntimeDimensions::State> parser;
    const std::shared_ptr<NotificationLayoutStandaloneLabelState> label;
    const std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
    const std::shared_ptr<const NotificationLayoutContextRealmArgumentLoan> argument;
    const std::uint64_t serial;
    mutable std::uint64_t child_sequence,baseline_sequence;
    enum class Stage{style_entered,style_returned,fields_entered,fields_returned,common_required,common_returned};
    Stage stage=Stage::style_entered;
    ParserLoan(const std::shared_ptr<NotificationLayoutRuntimeDimensions::State>& p,
        const std::shared_ptr<NotificationLayoutStandaloneLabelState>& child,
        const std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence>& x,
        const std::shared_ptr<const NotificationLayoutContextRealmArgumentLoan>& a,std::uint64_t s)
        :parser(p),label(child),selected(x),argument(a),serial(s),
         child_sequence(child->impl_->sequence),baseline_sequence(p->baseline->sequence_){}
    ~ParserLoan(){alive.store(false);}
    void first()const{
        require(creator==std::this_thread::get_id()&&alive.load(),"Standalone parser lexical issuer retired/foreign");
        const auto core=parser->core.lock();require(bool(core),"Standalone parser Core expired");core->layout_origin(true);
        const auto& current=core->source->current_operation_loan();core->layout_origin(true);
        require(&current==parser->operation&&current.operation_serial()==serial,"Standalone parser fresh Source replaced before borrowed access");
        NotificationLayoutRuntimeDimensions::require_storage_without_callbacks(parser);
        NotificationLayoutRuntimeDimensions::require_context_argument_owned(parser,argument);
        const auto active=parser->active_standalone_parser.lock();const auto fields=label->impl_;fields->first();
        const auto& node=BnwWorldViewControlTree::Core::node(core->nodes,parser->id);
        const auto actual_xml=selected?selected->source.lock():nullptr;const auto receipt=fields->returned;
        require(active.get()==this&&selected&&actual_xml&&actual_xml.get()==core->source.get()&&
            !actual_xml.owner_before(core->source)&&!core->source.owner_before(actual_xml)&&node.full_xml_occurrence==selected&&
            selected->value.name=="Label"&&node.layout_receiver_occurrence==parser->occurrence.lock()&&
            node.layout_baseline&&&*node.layout_baseline==parser->baseline&&parser->baseline->standalone_label_.get()==label.get()&&
            !parser->baseline->standalone_label_.owner_before(label)&&!label.owner_before(parser->baseline->standalone_label_)&&
            parser->baseline->kind_==NotificationLayoutBaseline::Kind::label&&
            parser->baseline->parser_runtime_dimensions_==parser->facade&&!parser->baseline->parsed_source_serial_&&
            parser->baseline->sequence_==baseline_sequence&&fields->sequence==child_sequence&&receipt&&
            receipt->source_serial==fields->source_serial&&receipt->core.lock()==core&&
            receipt->occurrence.lock()==parser->occurrence.lock()&&receipt->selected.lock()==selected&&
            !fields->busy&&fields->entered,"Standalone parser actual constructor/receiver/selected occurrence/journal changed");
        fields->module->require_label_allocation_owned(current,receipt->ticket,label,true);
        if(const auto* cc=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4])){
            require(bool(cc->loan),"Standalone parser actual CC typed descriptor empty");cc->loan->require_current_without_callbacks();
        }else require(std::holds_alternative<Impl::Null>(fields->fields[0xcc/4]),"Standalone parser actual CC unknown");
        if(const auto* system=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4])){
            require(bool(system->loan),"Standalone parser actual170 typed descriptor empty");system->loan->require_current_without_callbacks();
        }else require(std::holds_alternative<Impl::Null>(fields->fields[0x170/4]),"Standalone parser actual170 unknown");
        const auto* flags=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
        require(flags&&parser->baseline->base_.flags_40==*flags,"Standalone physical Base/Label flag alias diverged");
    }
    void full()const{first();parser->operation->require_current();first();}
    const content::XmlNode& layer()const{first();return selected->value;}
    void record(std::uint32_t offset)const{
        first();auto& fields=*label->impl_;require(fields.sequence<UINT64_MAX&&fields.history.size()<fields.history.capacity(),
            "Standalone5E actual journal unprepared");fields.history.push_back({++fields.sequence,offset});child_sequence=fields.sequence;
    }
    void base_record(NotificationLayoutBaseline::Mutation reason)const{
        auto& base=*const_cast<NotificationLayoutBaseline*>(parser->baseline);
        require(base.sequence_<UINT64_MAX&&base.history_.size()<base.history_.capacity(),"Standalone Base journal unprepared");
        base.record_owned(reason);baseline_sequence=base.sequence_;
    }
};
struct detail::NotificationLayoutStandaloneLabelState::FieldsReturn {
    const std::uint64_t serial,sequence;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
    FieldsReturn(const ParserLoan& loan):serial(loan.serial),sequence(loan.child_sequence),selected(loan.selected){}
};
std::uint8_t detail::NotificationLayoutRuntimeDimensions::standalone_label_language_owned()const{
    const auto saved=state_;const auto loan=saved->active_standalone_parser.lock();require(bool(loan),"Standalone language needs actual parser issuer");loan->full();
    const auto manager=saved->manager.lock();require(bool(manager),"Standalone language actual Gui retired");
    const auto value=manager->label_language_980a_owned(*saved->operation);loan->full();return value;
}
std::shared_ptr<const detail::BnwLabelSystemFontBorrower>
detail::NotificationLayoutRuntimeDimensions::standalone_label_system_font_owned(std::shared_ptr<const BnwCompletedFontLoan> font)const{
    const auto saved=state_;const auto loan=saved->active_standalone_parser.lock();require(bool(loan),"Standalone170 issuer absent");loan->full();
    const auto* cc=std::get_if<NotificationLayoutStandaloneLabelState::Impl::FontBorrow>(&loan->label->impl_->fields[0xcc/4]);
    require(cc&&font&&cc->loan.get()==font.get()&&!cc->loan.owner_before(font)&&!font.owner_before(cc->loan),"Standalone170 needs genuine actual CC");
    const auto manager=saved->manager.lock();require(bool(manager),"Standalone170 actual Gui retired");
    auto result=manager->begin_system_font_lookup_owned(*saved->operation,*font);loan->full();return result;
}
std::array<float,4> detail::NotificationLayoutRuntimeDimensions::standalone_label_color_owned(std::string_view input)const{
    const auto saved=state_;const auto loan=saved->active_standalone_parser.lock();require(bool(loan),"Standalone Color actual parser missing");loan->full();
    const std::string request(input);loan->full();const auto core=saved->core.lock();require(bool(core),"Standalone Color Core retired");
    const auto source=core->source;const auto value=source->image_color_value_owned(*saved->operation,request);loan->full();return value;
}
std::string detail::NotificationLayoutRuntimeDimensions::standalone_label_text_owned(std::string_view input)const{
    const auto saved=state_;const auto loan=saved->active_standalone_parser.lock();require(bool(loan),"Standalone text actual parser missing");loan->full();
    const std::string request(input);loan->full();const auto core=saved->core.lock();require(bool(core),"Standalone text Core retired");
    const auto source=core->source;auto value=source->tooltip_localized_text_owned(*saved->operation,request);loan->full();return value;
}
detail::NotificationLabelColorSetKnownBytes detail::NotificationLayoutRuntimeDimensions::standalone_label_color_set_owned(std::string_view input)const{
    const auto saved=state_;const auto loan=saved->active_standalone_parser.lock();require(bool(loan),"Standalone ColorSet missing actual parser loan");loan->first();
    try{require(loan->stage==NotificationLayoutStandaloneLabelState::ParserLoan::Stage::fields_entered,"Standalone ColorSet outside real5E call");
        const auto argument=loan->argument;const std::string request(input);loan->full();native_color_load_detail::bounded(request);
        require(std::any_of(loan->layer().attributes.begin(),loan->layer().attributes.end(),[&](const auto& a){return a.first=="ColorSet"&&a.second==request;}),
            "Standalone ColorSet differs from SAME actual selected XML attribute");
        std::weak_ptr<const BnwWorldViewControlTree::Core::SelectedContextColorSets> local_key;
        std::weak_ptr<NotificationContextGuiPort::Impl::SelectedColorSetRegistry> global_key;
        {const auto actual=saved->core.lock();const auto manager=saved->manager.lock();
            require(actual&&manager&&actual->selected_context_color_sets&&manager->impl_->selected_color_sets,
                "ColorSet lacks actual Context/local and Gui/global registry setup");
            manager->impl_->selected_color_sets->first();manager->impl_->first(*saved->operation);
            local_key=actual->selected_context_color_sets;global_key=manager->impl_->selected_color_sets;}
        const auto pure=[&]{
            loan->first();require_context_argument_owned(saved,argument);
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
        {const auto local=local_key.lock();const auto global=global_key.lock();require(local&&global,"ColorSet actual owner expired before lookup");global->first();require(current.size()==global->atlas.size(),"Standalone ColorAtlas selected layer count changed");
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
void detail::NotificationLayoutStandaloneLabelState::parse_fields_5e_owned(
    const NotificationLayoutRuntimeDimensions& parser,const std::shared_ptr<ParserLoan>& loan){
    const auto fields=impl_;fields->first();require(loan&&loan->label.get()==this,"Standalone5E actual lexical caller mismatch");
    const auto saved=loan->parser;const auto pure=[&]{loan->first();fields->first();};
    try{require(loan->stage==ParserLoan::Stage::style_returned&&!fields->fields_returned,"Standalone5E actual order/replay mismatch");
        loan->stage=ParserLoan::Stage::fields_entered;loan->full();
        require(fields->sequence<=UINT64_MAX-256&&fields->history.max_size()-fields->history.size()>=256&&
            saved->baseline->history_.max_size()-saved->baseline->history_.size()>=32,"Standalone5E journals exhausted");
        fields->history.reserve(fields->history.size()+256);loan->full();
        auto& base=*const_cast<NotificationLayoutBaseline*>(saved->baseline);base.history_.reserve(base.history_.size()+32);loan->full();
        const auto store=[&](std::uint32_t offset){loan->record(offset);};
        const auto flag=[&](std::uint32_t bits){store(0x40);loan->base_record(NotificationLayoutBaseline::Mutation::receiver_prefix);
            std::get<std::uint32_t>(fields->fields[0x40/4])|=bits;base.base_.flags_40=std::get<std::uint32_t>(fields->fields[0x40/4]);pure();};
        const auto attr=[&](std::string_view name)->std::optional<std::string>{pure();std::optional<std::string> out;
            for(const auto& a:loan->layer().attributes)if(a.first==name){out=a.second;break;}loan->full();return out;};
        const auto rebuild=[&]{pure();require((std::get<std::uint32_t>(fields->fields[0x40/4])&1u)==0,
            "Standalone6A2EA0 needs real initialized layout producer");};
        require(std::get<std::uint32_t>(fields->fields[0x40/4])==0x09000000u&&
            std::holds_alternative<Impl::Null>(fields->fields[0x3c/4]),"Standalone first5E needs literal constructor flags/NULL3C");
        flag(0x02000000u);
        store(0x3c);loan->base_record(NotificationLayoutBaseline::Mutation::context_realm_3c);
        fields->fields[0x3c/4]=Impl::ParsedRealm{loan->argument->impl_->realm};
        base.base_.context_realm_3c=loan->argument->impl_->realm;
        base.base_.context_3c_observation=NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm;pure();
        const auto name=attr("Font");require(name.has_value(),"Standalone5E absent Font requires proven remaining NULL-CC parser path");
        const auto manager=saved->manager.lock();require(bool(manager),"Standalone Font genuine Gui expired");loan->full();
        auto returned=manager->begin_font_lookup_owned(*saved->operation,std::optional<std::string_view>{*name});loan->full();
        require(bool(returned),"Standalone genuine Font lookup returned NULL; no guessed fallback");
        std::shared_ptr<const BnwCompletedFontLoan> font(std::move(returned));loan->full();font->require_source_owned(*saved->operation);pure();
        require(std::holds_alternative<Impl::Null>(fields->fields[0xcc/4]),"Standalone subsequent Font setter needs physical same/replacement supplier");
        store(0xcc);fields->fields[0xcc/4]=Impl::FontBorrow{font};pure();
        const auto extended=parser.standalone_label_language_owned();pure();
        std::shared_ptr<const BnwLabelSystemFontBorrower> cache;
        if(extended){cache=parser.standalone_label_system_font_owned(font);pure();if(cache){cache->require_source_owned(*saved->operation);pure();}}
        store(0x170);fields->fields[0x170/4]=cache?Impl::Cell{Impl::SystemFontBorrow{cache}}:Impl::Cell{Impl::Null{}};pure();rebuild();
        if(const auto colors=attr("ColorSet")){
            flag(0x04000000u); //5E3B1D BEFORE actual67C5C0.
            const auto source=parser.standalone_label_color_set_owned(*colors);pure();
            //67C61F or67C644 REP MOVSD: actual16 DWORD order. Unknown
            //native byte63 stays unknown, not a fabricated initialized word.
            for(std::size_t word=0;word<16;++word){
                std::optional<std::uint32_t> bits=std::uint32_t{0};
                for(std::size_t byte=0;byte<4;++byte){
                    const auto& value=source[word*4+byte];
                    if(!value){bits.reset();break;}
                    *bits|=std::uint32_t(*value)<<(byte*8);
                }
                store(0x84+std::uint32_t(word*4));
                if(bits)fields->fields[0x84/4+word]=*bits;
                else fields->fields[0x84/4+word]=std::monostate{};
                if(word==15)for(std::size_t byte=0;byte<4;++byte)
                    fields->color_metadata_c0_c3[byte]=source[60+byte];
            }
            pure();
        }
        const auto color=[&](std::string_view name,std::uint32_t offset){
            if(const auto value=attr(name)){
                flag(0x04000000u);
                //Existing actual580780 Source decoder owns the same palette.
                //Its unsupported empty route is refused, never defaulted.
                const auto rgba=parser.standalone_label_color_owned(*value);pure();
                for(std::size_t i=0;i<4;++i){
                    store(offset+std::uint32_t(i*4));
                    fields->fields[(offset+i*4)/4]=std::bit_cast<std::uint32_t>(rgba[i]);
                }
                pure();
            }
        };
        color("Color",0x84);color("Color0",0x84);color("Color1",0x94);color("Color2",0xa4);
        (void)attr("Color3"); //Actual5E3C01 queries it then overwrites local; no color3 write.
        if(const auto style=attr("FontStyle")){
            const auto font=std::get<Impl::FontBorrow>(fields->fields[0xcc/4]).loan;
            pure();font->require_source_owned(*saved->operation);pure();
            const auto ordinal=font->style_ordinal_without_callbacks(*style);pure();
            store(0xc8);fields->style_c8=std::uint8_t(ordinal); //5E3C33 AL, including real lookup-miss0.
            pure();rebuild();
        }
        const auto integer=[](std::string_view text)->std::int16_t{
            //Qualified actual81EC40 base10/allow0x branch. Literal selected
            //decimal XML is handled; unsupported grammar STOP before writes.
            text=text.substr(0,text.find('\0'));
            const auto white=[](char ch){return ch==' '||ch=='\t'||ch=='\n'||ch=='\r'||ch=='\f'||ch=='\v';};
            while(!text.empty()&&white(text.front()))text.remove_prefix(1);
            bool neg=false;if(!text.empty()&&(text.front()=='-'||text.front()=='+')){neg=text.front()=='-';text.remove_prefix(1);}
            require(!text.starts_with("0x"),"Label81EC40 hexadecimal grammar requires proven successor");
            std::uint32_t bits=0;std::size_t at=0;
            while(at<text.size()&&text[at]>='0'&&text[at]<='9')bits=bits*10u+std::uint32_t(text[at++]-'0');
            for(;at<text.size();++at)require(white(text[at]),"Label81EC40 nondecimal suffix outside qualified selected XML");
            if(neg)bits=0u-bits;return std::bit_cast<std::int16_t>(std::uint16_t(bits));
        };
        if(const auto wrap=attr("WrapWidth")){
            const auto n=integer(*wrap);pure();store(0x16c);fields->truncate_16c=std::int16_t{-1};
            store(0x168);fields->wrap_168=n;pure();rebuild();
        }
        if(const auto trunc=attr("TruncateWidth")){
            const auto n=integer(*trunc);pure();store(0x16c);fields->truncate_16c=n;
            store(0x168);fields->wrap_168=std::int16_t{-1};pure();rebuild();
        }
        const auto truth=[&](std::string_view key,std::uint32_t bits){
            if(const auto value=attr(key))if(!value->empty()&&
                (value->front()=='t'||value->front()=='T'||value->front()=='1'))flag(bits);
        };
        truth("ForceIME",0x40000000u);truth("ForceNonIME",0x20000000u);truth("ReduceOnTruncate",0x10000000u);
        if(const auto leading=attr("LeadingOffset")){
            const auto n=integer(*leading);pure();store(0x16a);fields->leading_16a=n;pure();
        }
        if(const auto key=attr("String")){
            //Actual5E3DE4 positive FString length localizes and obtains UTF8;
            //empty string directly enters611930. Same selected Source locale.
            const auto ckey=key->substr(0,key->find('\0'));pure();
            std::string text=key->empty()?std::string{}:parser.standalone_label_text_owned(ckey);pure();
            require(text.find('\0')==std::string::npos&&text.size()<0x4000,
                "Label611930 overlong/NUL text requires bounded raw-copy successor");
            //First actual text setter: C4 constructorNULL. No existing pooled
            //text object is falsely released; this fresh OWN x64 allocation
            //preserves exact UTF8+NUL bytes and has independent retirement.
            require(std::holds_alternative<Impl::Null>(fields->fields[0xc4/4]),
                "Label611930 subsequent text replacement requires genuine text retirement");
            auto object=std::make_shared<Impl::TextObject>();loan->full();
            object->allocation_child=fields;object->source_serial=fields->source_serial;
            object->bytes.resize(text.size()+1);loan->full();
            store(0xc4);fields->fields[0xc4/4]=Impl::TextBorrow{object};pure();
            std::copy(text.begin(),text.end(),object->bytes.begin());object->bytes.back()='\0';pure();
            require(object->bytes.size()==text.size()+1&&std::equal(text.begin(),text.end(),object->bytes.begin())&&
                object->bytes.back()=='\0',"Standalone61198F actual store-before-copy changed");loan->full();rebuild();
        }
        loan->full();require(!fields->fields_returned,"Standalone5E return repeated");
        auto completed=std::shared_ptr<const FieldsReturn>(new FieldsReturn(*loan));loan->full();
        fields->fields_returned=completed;loan->stage=ParserLoan::Stage::fields_returned;pure();
    }catch(...){fields->failed.store(true);NotificationLayoutRuntimeDimensions::fail_storage_without_callbacks(saved);throw;}
}
void detail::NotificationLayoutRuntimeDimensions::enter_standalone_label_slot44_owned()const{
    using Label=NotificationLayoutStandaloneLabelState;using Loan=Label::ParserLoan;
    const auto saved=state_;require_storage_without_callbacks(saved);require(saved->baseline->standalone_label_&&
        saved->active_standalone_parser.expired(),"Standalone45EA90 genuine allocated receiver missing/reentry");
    const auto label=saved->baseline->standalone_label_;const auto core=saved->core.lock();
    const auto selected=BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence;
    const auto argument=context_realm_argument_owned();const auto serial=saved->serial;
    auto loan=std::shared_ptr<Loan>(new Loan(saved,label,selected,argument,serial));
    //Re-resolve current Source independently BEFORE any old borrowed operation access after allocation.
    core->layout_origin(true);const auto& current=core->source->current_operation_loan();core->layout_origin(true);
    require(&current==saved->operation&&current.operation_serial()==serial,"Standalone wrapper Source replaced after loan allocation");
    saved->active_standalone_parser=loan;loan->full();
    try{
        //5C2402 actual first-case PRESENT test, including empty values.
        const auto style=std::find_if(loan->layer().attributes.begin(),loan->layer().attributes.end(),[](const auto& a){return a.first=="Style";});
        if(style!=loan->layer().attributes.end()){
            const std::string request=style->second;loan->full();
            std::weak_ptr<const BnwWorldViewControlTree::Core::SelectedContextStyles> local_key;
            std::weak_ptr<NotificationContextGuiPort::Impl::SelectedStyleRegistry> global_key;
            {const auto manager=saved->manager.lock();require(manager&&core->selected_context_styles&&manager->impl_->selected_styles,
                "Standalone5C actual Context/local and Gui/global Styles absent");
             local_key=core->selected_context_styles;global_key=manager->impl_->selected_styles;}
            loan->full();const auto current_styles=saved->operation->selected_style_resources_owned();loan->full();
            std::optional<content::XmlNode> resolved;
            {const auto local=local_key.lock();const auto global=global_key.lock();require(local&&global,"Standalone Style owners expired");global->first();
             const auto manager=saved->manager.lock();require(manager&&manager->impl_->selected_styles.get()==global.get()&&
                !manager->impl_->selected_styles.owner_before(global)&&!global.owner_before(manager->impl_->selected_styles)&&
                core->selected_context_styles.get()==local.get()&&!core->selected_context_styles.owner_before(local)&&
                !local.owner_before(core->selected_context_styles)&&current_styles.size()==global->sheets.size()&&
                BnwWorldViewControlTree::Core::same_layout_xml(local->selected_context,core->root_authored),"Standalone Style selected profile changed");
             for(std::size_t i=0;i<current_styles.size();++i){const auto& a=current_styles[i];const auto& e=global->sheets[i];
                require(a.payload_identity.sha256==e.payload_identity.sha256&&a.bytes.get()==e.bytes.get()&&
                    !a.bytes.owner_before(e.bytes)&&!e.bytes.owner_before(a.bytes)&&a.xml.get()==e.xml.get()&&
                    !a.xml.owner_before(e.xml)&&!e.xml.owner_before(a.xml),"Standalone Style resource Bank/VFS allocation changed");}
             const auto found=local->values.find(request);resolved=found==local->values.end()?global->lookup_global(request):std::optional<content::XmlNode>{found->second};}
            loan->full();require(bool(resolved),"Standalone5C actual Style MISS diagnostic manager required");
            const auto own=std::find_if(resolved->attributes.begin(),resolved->attributes.end(),[](const auto& a){return a.first=="Style";});
            require(own!=resolved->attributes.end()&&std::all_of(own->second.begin(),own->second.end(),[](unsigned char c){return c<0x80;}),
                "Standalone5C2521 recursive45EA90 Style/common callframe supplier required; no flattened Style grant");
            //835330 successful ASCII value-read (including empty) ->5C251F skip.
        }
        loan->stage=Loan::Stage::style_returned;loan->full();label->parse_fields_5e_owned(*this,loan);loan->full();
        require(loan->stage==Loan::Stage::fields_returned&&label->impl_->fields_returned&&
            label->impl_->fields_returned->serial==serial&&label->impl_->fields_returned->sequence==label->impl_->sequence,
            "Standalone45EAA8 real5E return did not close");
        loan->stage=Loan::Stage::common_required;loan->first();
        run_standalone_label_common_prefix_owned();loan->full();
        require(loan->stage==Loan::Stage::common_returned&&label->impl_->wrapper_returned,
            "Standalone45EA90 actual438170 return did not close");
        require_standalone_label_common_return_from_parser_owned();loan->full();
        loan->alive.store(false);saved->active_standalone_parser.reset(); //Lexical caller ends; never renewable.
        return;
    }catch(...){loan->alive.store(false);label->impl_->failed.store(true);fail_storage_without_callbacks(saved);throw;}
}
