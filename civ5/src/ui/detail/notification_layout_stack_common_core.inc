//Inside civ5::ui AFTER actual Ordinary/ChildFrame/EmbeddedLabel/Button bodies.
//Stack only: real embedded4BDF90 storage, never a metadata factory projection.
void detail::NotificationLayoutRuntimeDimensions::publish_stack_constructor_owned()const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
    require(core&&occurrence&&saved->id,"Stack4BDF90 actual constructor owners absent");
    auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
    require(bool(owner.stack_),"Stack4BDF90 real embedded allocation absent");
    const auto*const physical=&*owner.stack_;
    std::shared_ptr<const NotificationLayoutOrdinaryReceiver> published;
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto current=[&]{require_storage_without_callbacks(saved);
        require(owner.kind_==NotificationLayoutBaseline::Kind::stack&&owner.actual_type_=="Stack"&&
            owner.parser_runtime_dimensions_==this&&!owner.parsed_source_serial_&&owner.stack_&&&*owner.stack_==physical&&
            same(owner.base_.receiver_owned,published)&&!owner.base_.common_return_owned&&!owner.base_.children_78&&!owner.base_.id_58&&
            !owner.history_.empty()&&owner.history_.front().command==NotificationLayoutBaseline::Mutation::construct&&
            same(saved->core.lock(),core)&&same(saved->occurrence.lock(),occurrence)&&
            occurrence->id==saved->id&&occurrence->constructor_serial==saved->serial,
            "Stack4BDF90 physical constructor/occurrence/borrower changed");
        require(physical->field_84==0u&&physical->field_88==2u&&physical->field_8c==0,
            "Stack4BDFA1/AB/B7 actual literalDWORD0/DWORD2/WORD0 changed");
        for(float value:{physical->field_90,physical->field_94,physical->field_98,physical->field_9c})
            require(std::bit_cast<std::uint32_t>(value)==0u,"Stack4BDFBE/D6 actual literalFPzero changed");
        if(published){const auto fields=published->impl_;const auto alias=fields->stack.lock();
            require(fields->creator==std::this_thread::get_id()&&same(fields->canonical.lock(),published)&&
                same(fields->core.lock(),core)&&same(fields->occurrence.lock(),occurrence)&&
                alias&&alias.get()==physical&&!alias.owner_before(core)&&!core.owner_before(alias)&&
                fields->id==saved->id&&fields->serial==saved->serial&&
                fields->stack_parser_progress==NotificationLayoutOrdinaryReceiver::Impl::StackParserProgress::constructor_returned,
                "Stack4BDF90 published receiver lost genuine physical custody");}
    };
    const auto full=[&]{current();saved->operation->require_current();current();};
    try{
        full();const auto id=saved->id,serial=saved->serial;
        const auto sequence=owner.sequence_,history_size=owner.history_.size();
        //VALUE alias uses the actual Core control and address of its real embedded Stack.
        //Only weak custody is retained by receiver, avoiding Core->receiver->Core cycle.
        const std::shared_ptr<const void> alias(core,static_cast<const void*>(physical));
        auto pending=std::unique_ptr<NotificationLayoutOrdinaryReceiver::Impl>(
            new NotificationLayoutOrdinaryReceiver::Impl(core,occurrence,alias,id,serial));full();
        auto fields=std::shared_ptr<NotificationLayoutOrdinaryReceiver::Impl>(std::move(pending));full();
        auto receiver=std::shared_ptr<const NotificationLayoutOrdinaryReceiver>(new NotificationLayoutOrdinaryReceiver(fields));full();
        const auto retained=fields->stack.lock();
        require(retained&&retained.get()==physical&&!retained.owner_before(core)&&!core.owner_before(retained)&&
            fields->image.expired()&&fields->box.expired()&&fields->grid.expired()&&fields->button.expired()&&
            fields->standalone_label.expired()&&fields->context.expired()&&!fields->button_void_f4_f8&&
            fields->stack_padding_reader_progress==NotificationLayoutOrdinaryReceiver::Impl::StackPaddingReaderProgress::unentered&&!fields->stack_padding_read_return&&
            owner.sequence_==sequence&&owner.history_.size()==history_size,
            "Stack4BDF90 alias/control/constructor journal changed across FULL POST");
        fields->canonical=receiver;owner.base_.receiver_owned=receiver;published=std::move(receiver);full();
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

//A private actualslot verifier used at the Baseline split/ID/flag callsites.
void detail::NotificationLayoutRuntimeDimensions::require_stack_common_operand_slot_owned(const content::XmlNode& layer)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
    require(core&&occurrence&&saved->id,"Stack438170 actual operand owners absent");
    const auto& owner=*saved->baseline;const auto receiver=owner.base_.receiver_owned;
    const auto& node=BnwWorldViewControlTree::Core::node(core->nodes,saved->id);
    require(owner.kind_==NotificationLayoutBaseline::Kind::stack&&owner.actual_type_=="Stack"&&owner.stack_&&
        owner.parser_runtime_dimensions_==this&&!owner.parsed_source_serial_&&receiver&&receiver->impl_&&node.full_xml_occurrence,
        "Stack438170 operand is not SAME genuine entered receiver");
    const auto fields=receiver->impl_;const auto alias=fields->stack.lock();
    const auto canonical=fields->canonical.lock();const auto actual_core=fields->core.lock();
    require(fields->creator==std::this_thread::get_id()&&alias&&alias.get()==&*owner.stack_&&
        !alias.owner_before(core)&&!core.owner_before(alias)&&actual_core.get()==core.get()&&
        !actual_core.owner_before(core)&&!core.owner_before(actual_core)&&canonical.get()==receiver.get()&&
        !canonical.owner_before(receiver)&&!receiver.owner_before(canonical)&&fields->occurrence.lock()==occurrence&&
        fields->id==saved->id&&fields->serial==saved->serial&&
        (fields->stack_parser_progress==NotificationLayoutOrdinaryReceiver::Impl::StackParserProgress::common_entered||
         fields->stack_parser_progress==NotificationLayoutOrdinaryReceiver::Impl::StackParserProgress::id_lookup_returned)&&
        ((fields->stack_padding_reader_progress==NotificationLayoutOrdinaryReceiver::Impl::StackPaddingReaderProgress::missing_returned&&!fields->stack_padding_read_return)||
         (fields->stack_padding_reader_progress==NotificationLayoutOrdinaryReceiver::Impl::StackPaddingReaderProgress::present_returned&&fields->stack_padding_read_return))&&
        !node.full_xml_occurrence->dynamic&&node.full_xml_occurrence->source.lock()==core->source&&
        owner.parser_layers_.size()==1&&BnwWorldViewControlTree::Core::same_layout_xml(owner.parser_layers_[0],layer)&&
        BnwWorldViewControlTree::Core::same_layout_xml(node.full_xml_occurrence->value,layer),
        "Stack438170 actual prefix-return/physical alias/Source operand changed");
    if(fields->stack_padding_read_return){
        const auto& done=*fields->stack_padding_read_return;done.first_without_callbacks();
        const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
        const auto selected_padding=std::find_if(layer.attributes.begin(),layer.attributes.end(),[](const auto& row){return row.first=="Padding";});
        require(selected_padding!=layer.attributes.end()&&done.payload==selected_padding->second&&
            same(done.selected.lock(),node.full_xml_occurrence)&&same(done.physical_stack.lock(),alias)&&
            done.source_serial==saved->serial&&done.axis84==owner.stack_->field_84&&
            done.destination_offset==(done.axis84<2u?0x9cu:0x98u)&&done.axis84<4u&&
            done.after_word==std::bit_cast<std::uint32_t>(done.axis84<2u?owner.stack_->field_9c:owner.stack_->field_98),
            "Stack438170 PRESENT payload/locale/physical destination association changed");
    }else require(std::none_of(layer.attributes.begin(),layer.attributes.end(),[](const auto& row){return row.first=="Padding";}),
        "Stack438170 namedMISS cannot skip selected PRESENT Padding");
    require_storage_without_callbacks(saved);
}

//Called exactly at4B04FA, afterWrapGrowth and beforeWrapPadding. Actual49A030
//namedMISS returns false before reading its destination argument. PRESENT
//executes835330 ASCII payload then836010 ctor/write/operator>>(float&)/failbad
//and the real stream DTOR. No81ED50 or integer_float destination substitute.
bool detail::NotificationLayoutRuntimeDimensions::read_stack_padding_slot_owned(const content::XmlNode& layer)const{
    static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559&&
        std::numeric_limits<float>::radix==2&&std::numeric_limits<float>::digits==24);
    const auto saved=state_;
    try{
        require_storage_without_callbacks(saved);
        const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
        require(core&&occurrence&&saved->id,"Stack49A030 actual Core/occurrence absent");
        auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        const auto receiver=owner.base_.receiver_owned;
        require(owner.stack_&&receiver&&receiver->impl_,"Stack49A030 actual embedded receiver absent");
        const auto fields=receiver->impl_;
        using Read=NotificationLayoutOrdinaryReceiver::Impl::StackPaddingReaderProgress;
        const auto xml=BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence;
        const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
        const auto raw=[&]{require(bool(owner.stack_),"Stack49A030 physical Stack retired");const auto& value=*owner.stack_;
            return std::array<std::uint32_t,7>{value.field_84,value.field_88,std::bit_cast<std::uint16_t>(value.field_8c),
                std::bit_cast<std::uint32_t>(value.field_90),std::bit_cast<std::uint32_t>(value.field_94),
                std::bit_cast<std::uint32_t>(value.field_98),std::bit_cast<std::uint32_t>(value.field_9c)};};
        const auto before=raw();auto expected_words=before;auto expected=Read::unentered;
        //The pure custody snapshot above is not49A030's destination argument.
        //Only the genuine PRESENT branch obtains that argument below835330.
        bool extraction_in_progress=false;
        const std::size_t word_index=before[0]<2u?6u:5u;
        const auto current=[&]{require_storage_without_callbacks(saved);const auto alias=fields->stack.lock();
            require(owner.kind_==NotificationLayoutBaseline::Kind::stack&&owner.actual_type_=="Stack"&&
                owner.parser_runtime_dimensions_==this&&!owner.parsed_source_serial_&&same(owner.base_.receiver_owned,receiver)&&
                fields->creator==std::this_thread::get_id()&&same(fields->canonical.lock(),receiver)&&
                same(fields->core.lock(),core)&&same(fields->occurrence.lock(),occurrence)&&alias&&
                alias.get()==&*owner.stack_&&!alias.owner_before(core)&&!core.owner_before(alias)&&
                fields->id==saved->id&&fields->serial==saved->serial&&
                fields->stack_parser_progress==NotificationLayoutOrdinaryReceiver::Impl::StackParserProgress::style_missing_returned&&
                fields->stack_padding_reader_progress==expected&&before[0]<4u&&
                xml&&!xml->dynamic&&same(xml->source.lock(),core->source)&&
                same(BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence,xml)&&
                owner.parser_layers_.size()==1&&BnwWorldViewControlTree::Core::same_layout_xml(owner.parser_layers_[0],layer)&&
                BnwWorldViewControlTree::Core::same_layout_xml(xml->value,layer),
                "Stack49A030 SAME physical receiver/selectedXML/phase changed");
            const auto actual=raw();for(std::size_t i=0;i<actual.size();++i)
                if(!extraction_in_progress||i!=word_index)require(actual[i]==expected_words[i],
                    "Stack49A030 actual non-destination or completed word changed");
            if(fields->stack_padding_read_return){
                require(expected==Read::present_returned,"Stack49A030 premature PRESENT return publication");
                const auto& done=*fields->stack_padding_read_return;done.first_without_callbacks();
                require(same(done.selected.lock(),xml)&&same(done.physical_stack.lock(),alias)&&
                    done.source_serial==saved->serial&&done.axis84==before[0]&&
                    done.destination_offset==(word_index==5u?0x98u:0x9cu)&&
                    done.before_word==before[word_index]&&done.after_word==expected_words[word_index],
                    "Stack49A030 actual PRESENT return lost SAME source/physical destination");
            }else require(expected!=Read::present_returned,"Stack49A030 PRESENT return not genuinely supplied");
        };
        const auto full=[&]{current();saved->operation->require_current();current();};
        full();
        const auto found=std::find_if(layer.attributes.begin(),layer.attributes.end(),[](const auto& row){return row.first=="Padding";});
        full();
        if(found==layer.attributes.end()){
            require(!fields->stack_padding_read_return,"Stack49A030 namedMISS has no prior PRESENT owner");
            fields->stack_padding_reader_progress=expected=Read::missing_returned;full();
            return false; //RealMISS retains its genuine current destination word.
        }
        //Actual835330 typed supplier owns the selected attribute text. The
        //retained XmlOccurrence keeps its source bytes physically alive even
        //if an allocator callback revokes the ACTIVE Source; FULL POST rejects
        //such revocation before another borrowed operand can be consumed.
        const std::string payload(found->second);full();
        require(payload==found->second&&payload.size()<=static_cast<std::size_t>(INT32_MAX),
            "Stack835330 actual selected payload changed/length exceeds real signed write argument");
        const bool ascii=std::all_of(payload.begin(),payload.end(),[](unsigned char c){return !(c&0x80u);});
        full();require(ascii,"Stack835330 high-bit payload requires real non-ASCII failure path");
        //Finite, exact binary32 domain: nonempty decimal ASCII digits and
        //magnitude<=2^24. This is a grammar, not a selected file/value whitelist.
        //Signs/decimal points/whitespace/empty/overflow remain explicitSTOP.
        require(!payload.empty(),"Stack836010 PRESENT-empty extraction domain unavailable");
        std::uint32_t integer_value=0;constexpr std::uint32_t exact_limit=1u<<24;
        for(unsigned char c:payload){
            require(c>='0'&&c<='9',"Stack836010 PRESENT token outside earned unsigned ASCII integer domain");
            const auto digit=static_cast<std::uint32_t>(c-'0');
            require(integer_value<=(exact_limit-digit)/10u,"Stack836010 PRESENT magnitude outside exact binary32 integer domain");
            integer_value=integer_value*10u+digit;
        }
        const auto exact_word=std::bit_cast<std::uint32_t>(static_cast<float>(integer_value));
        full();
        //Same operation as original literalmode3, expressed using the typed
        //library bits. Do not imbue classic(), alter the current global locale
        //or infer locale from selected ru_RU game-language metadata.
        auto stream=std::make_unique<std::stringstream>(std::ios_base::in|std::ios_base::out);full();
        const std::locale actual_locale=stream->getloc();full();
        const auto*const actual_get=&std::use_facet<std::num_get<char>>(actual_locale);full();
        const auto*const actual_ctype=&std::use_facet<std::ctype<char>>(actual_locale);full();
        const auto*const actual_punct=&std::use_facet<std::numpunct<char>>(actual_locale);full();
        //Custom extraction/character/punctuation facets have no original
        //contract here. Guard genuine retained standard facet objects, not a
        //locale name/Boolean receipt. Plain integral tokens need no decimal or
        //thousands punctuation; exact results are independent of rounding.
        require(typeid(*actual_get)==typeid(std::num_get<char>)&&
            typeid(*actual_ctype)==typeid(std::ctype<char>)&&
            typeid(*actual_punct)==typeid(std::numpunct<char>),
            "Stack836010 custom stream locale facet domain unavailable");
        const auto stream_current=[&]{current();
            require(stream&&stream->getloc()==actual_locale&&
                &std::use_facet<std::num_get<char>>(stream->getloc())==actual_get&&
                &std::use_facet<std::ctype<char>>(stream->getloc())==actual_ctype&&
                &std::use_facet<std::numpunct<char>>(stream->getloc())==actual_punct,
                "Stack836010 actual stream/retained facet association changed");current();};
        const auto stream_full=[&]{stream_current();saved->operation->require_current();stream_current();};
        stream_full();
        const auto decimal=actual_punct->decimal_point();stream_full();
        const auto thousands=actual_punct->thousands_sep();stream_full();
        const auto digit_punctuation=[](unsigned char c){return c>='0'&&c<='9';};
        require(!digit_punctuation(static_cast<unsigned char>(decimal))&&
            !digit_punctuation(static_cast<unsigned char>(thousands)),
            "Stack836010 selected digits intersect actual locale punctuation");
        for(char c='0';c<='9';++c){
            const char widened=actual_ctype->widen(c);stream_full();
            const bool whitespace=actual_ctype->is(std::ctype_base::space,c);stream_full();
            require(widened==c&&!whitespace,"Stack836010 actual ctype lacks ASCII decimal digit semantics");
        }
        const auto flags=stream->flags();stream_full();
        require((flags&std::ios_base::basefield)==std::ios_base::dec&&
            !(flags&std::ios_base::floatfield)&&(flags&std::ios_base::skipws),
            "Stack836010 actual default extraction flags unavailable");
        stream_full();stream->write(payload.data(),static_cast<std::streamsize>(payload.size()));stream_full();
        require(stream->good(),"Stack836010 actual write did not complete; no successful skip allowed");
        fields->stack_padding_reader_progress=expected=Read::present_extracting;
        extraction_in_progress=true;stream_full();
        //The FIRST destination argument read is genuinely below835330 and
        //all present supplier guards. The operator writes THIS actual field.
        float& actual_destination=word_index==5u?owner.stack_->field_98:owner.stack_->field_9c;
        (*stream)>>actual_destination;
        //FIRST after library callbacks is supplier/control/Source health.
        //During this POST only the proven destination word may have changed.
        stream_full();
        const auto state=stream->rdstate();stream_full();
        const bool returned_al=(state&(std::ios_base::failbit|std::ios_base::badbit))==std::ios_base::goodbit;
        //EOF alone is not failure, matching original state-mask0x6.
        require(returned_al,"Stack836010 real extraction returned false; actual failure write retained and session poisoned");
        require(std::bit_cast<std::uint32_t>(actual_destination)==exact_word,
            "Stack836010 actual float extraction differs from exact admitted integer domain");
        expected_words[word_index]=std::bit_cast<std::uint32_t>(actual_destination);
        extraction_in_progress=false;stream_full();
        //Actual836010 destroys its stream on both branches before returningAL.
        //No retained oldstream/readiness flag substitutes for this DTOR.
        stream_full();stream.reset();full();
        const auto actual_stack=fields->stack.lock();full();
        const auto serial=saved->serial;const auto axis=before[0];
        const auto offset=word_index==5u?0x98u:0x9cu;
        const auto before_word=before[word_index],after_word=expected_words[word_index];full();
        auto returned=std::unique_ptr<const NotificationLayoutOrdinaryReceiver::Impl::StackPaddingReadReturn>(
            new NotificationLayoutOrdinaryReceiver::Impl::StackPaddingReadReturn(
                xml,actual_stack,serial,axis,offset,
                before_word,after_word,payload,actual_locale,
                actual_get,actual_ctype,actual_punct));full();
        returned->first_without_callbacks();full();
        require(returned->payload==payload,"Stack49A030 actual payload return changed across FULL POST");
        fields->stack_padding_read_return=std::move(returned);
        fields->stack_padding_reader_progress=expected=Read::present_returned;full();
        return returned_al; //Actual operator result, not an unconditional success.
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::store_stack_common_id_58_owned(const content::XmlNode& layer)const{
    const auto saved=state_;
    try{
        require_stack_common_operand_slot_owned(layer);
        auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);const auto receiver=owner.base_.receiver_owned;
        require(receiver->impl_->stack_parser_progress==NotificationLayoutOrdinaryReceiver::Impl::StackParserProgress::common_entered&&
            !owner.base_.id_58,"Stack564300 repeated58 lookup/allocation");
        const auto full=[&]{require_stack_common_operand_slot_owned(layer);saved->operation->require_current();require_stack_common_operand_slot_owned(layer);};
        full();const auto found=std::find_if(layer.attributes.begin(),layer.attributes.end(),[](const auto& row){return row.first=="ID";});
        std::shared_ptr<const NotificationLayoutControlID58> result;
        if(found!=layer.attributes.end()){
            const auto& text=found->second;
            require(text.size()<32&&std::all_of(text.begin(),text.end(),[](unsigned char c){return c>0&&c<128;}),
                "Stack564300 reached unported ID encoding/long-buffer branch");
            if(!text.empty()){
                std::array<char,32> bytes{};std::copy(text.begin(),text.end(),bytes.begin());
                const auto length=static_cast<std::uint32_t>(text.size());
                auto pending=std::unique_ptr<const NotificationLayoutControlID58>(new NotificationLayoutControlID58(bytes,length));full();
                result=std::shared_ptr<const NotificationLayoutControlID58>(std::move(pending));full();
                require(result->bytes==bytes&&result->length==length,"Stack564300 actual58 allocation bytes changed across FULL POST");
            }
            owner.base_.id_58=result; //PRESENT-empty actual releaseNULL, previous58 was genuineNULL.
        } //Actual namedMISS retains existing physically constructed NULL58.
        receiver->impl_->stack_id58_return=owner.base_.id_58;
        receiver->impl_->stack_parser_progress=NotificationLayoutOrdinaryReceiver::Impl::StackParserProgress::id_lookup_returned;
        full();require(owner.base_.id_58==result&&receiver->impl_->stack_id58_return.lock()==owner.base_.id_58,
            "Stack564300 actuallookup/store association changed across FULL POST");
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::enter_stack_slot44_owned()const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    publish_stack_constructor_owned();require_storage_without_callbacks(saved);
    auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
    const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
    require(core&&occurrence,"Stack4B02C0 actual owners retired");
    const auto xml=BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence;
    const auto receiver=owner.base_.receiver_owned;require(receiver&&receiver->impl_,"Stack4B02C0 real4BDF90 receiver absent");
    const auto fields=receiver->impl_;using Progress=NotificationLayoutOrdinaryReceiver::Impl::StackParserProgress;
    auto expected_progress=Progress::constructor_returned;
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto raw_stack=[&]{require(bool(owner.stack_),"Stack actual embeddedfields retired");const auto& value=*owner.stack_;
        return std::array<std::uint32_t,7>{value.field_84,value.field_88,std::bit_cast<std::uint16_t>(value.field_8c),
            std::bit_cast<std::uint32_t>(value.field_90),std::bit_cast<std::uint32_t>(value.field_94),
            std::bit_cast<std::uint32_t>(value.field_98),std::bit_cast<std::uint32_t>(value.field_9c)};};
    auto expected_words=raw_stack();
    std::shared_ptr<NotificationLayoutOrdinaryChildren78> published_array;
    std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn> published_return;
    const auto current=[&]{require_storage_without_callbacks(saved);
        const auto alias=fields->stack.lock();
        require(owner.kind_==NotificationLayoutBaseline::Kind::stack&&owner.actual_type_=="Stack"&&owner.stack_&&
            owner.parser_runtime_dimensions_==this&&!owner.parsed_source_serial_&&same(owner.base_.receiver_owned,receiver)&&
            fields->creator==std::this_thread::get_id()&&same(fields->core.lock(),core)&&same(fields->occurrence.lock(),occurrence)&&
            same(fields->canonical.lock(),receiver)&&alias&&alias.get()==&*owner.stack_&&!alias.owner_before(core)&&!core.owner_before(alias)&&
            fields->id==saved->id&&fields->serial==saved->serial&&fields->stack_parser_progress==expected_progress&&
            raw_stack()==expected_words&&same(owner.base_.children_78,published_array)&&same(owner.base_.common_return_owned,published_return),
            "Stack4B02C0/Common physicalalias/control/phase/fields changed");
        if(published_array){
            require(published_array->impl_&&published_array->impl_->creator==std::this_thread::get_id()&&
                same(published_array->impl_->parent.lock(),receiver)&&published_array->impl_->stack_list,
                "Stack SAME real physical list/parent issuer absent");
            const auto*const actual_list=published_array->impl_->stack_list.get();
            actual_list->first_without_callbacks();
            require(actual_list->count==published_array->impl_->ordered.size(),
                "Stack real physical count and paired ordered view diverged");
            if(actual_list->phase!=NotificationLayoutOrdinaryChildren78::Impl::StackList::Phase::old_released)
                for(std::size_t j=0;j<actual_list->count;++j)require(
                    same(actual_list->data[j],published_array->impl_->ordered[j]),
                    "Stack SAME physical data slot/strong ordered control changed");
        }
        require(xml&&!xml->dynamic&&xml->value.name=="Stack"&&same(xml->source.lock(),core->source)&&
            same(BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence,xml)&&owner.parser_layers_.size()==1&&
            BnwWorldViewControlTree::Core::same_layout_xml(owner.parser_layers_[0],xml->value),
            "Stack4B02C0 needs SAME genuine selected authoredsinglelayer occurrence");
    };
    const auto full=[&]{current();saved->operation->require_current();current();};
    try{
        full();const auto& layer=xml->value;
        //5C23C0 is an actual named Style lookup, even onMISS. Original Bank/Gui controls remain real.
        const auto argument=context_realm_argument_owned();require_context_argument_owned(saved,argument);
        const auto local=core->selected_context_styles;const auto manager=saved->manager.lock();
        require(local&&manager&&manager->impl_->selected_styles,"Stack5C23C0 actual Context/Gui Style controls absent");
        const auto global=manager->impl_->selected_styles;global->first();manager->impl_->first(*saved->operation);full();
        const bool has_style=std::any_of(layer.attributes.begin(),layer.attributes.end(),[](const auto& row){return row.first=="Style";});
        full();require(same(core->selected_context_styles,local)&&same(manager->impl_->selected_styles,global),
            "Stack5C23C0 actualContext/Gui Style controls changed across lookup POST");
        require(!has_style,"Stack5C23C0 PRESENT Style needs real retained replay occurrence/vslot44 actor");
        fields->stack_parser_progress=expected_progress=Progress::style_missing_returned;full();
        for(const auto& row:layer.attributes){
            const auto& key=row.first;
            require(key=="ID"||key=="StackGrowth"||key=="WrapWidth"||key=="WrapGrowth"||key=="Padding"||key=="WrapPadding"||
                key=="Size"||key=="Offset"||key=="Anchor"||key=="AnchorSide"||key=="Hidden"||key=="ConsumeMouse"||
                key=="ConsumeMouseOver"||key=="ToolTip"||key=="ToolTipType",
                "Stack4B02C0 selected operand needs its own proven consumer");
            require(key!="WrapPadding","Stack4B053D PRESENT WrapPadding needs genuine81ED50 conversion actor");
            if(key=="Size"||key=="Offset"||key=="Anchor"||key=="AnchorSide")
                require(!row.second.empty(),"Stack438170 PRESENT-empty geometry needs actualnative branch");
        }
        for(const auto& child:layer.children)require(child.name!="ShowOnMouseOver"&&child.name!="HideOnMouseOver",
            "Stack438170 mouseover wrapper needs genuine7C/80 producer");
        //Actual4B02C0 typed prefix. The Root Padding seam calls49A030 ONCE at
        //its original slot, includingMISS, before any WrapPadding actor.
        owner.parse_stack_layer_owned(layer);expected_words=raw_stack();full();
        require((fields->stack_padding_reader_progress==NotificationLayoutOrdinaryReceiver::Impl::StackPaddingReaderProgress::missing_returned&&!fields->stack_padding_read_return)||
            (fields->stack_padding_reader_progress==NotificationLayoutOrdinaryReceiver::Impl::StackPaddingReaderProgress::present_returned&&fields->stack_padding_read_return),
            "Stack4B04FA mandatory49A030 reader did not genuinelyreturn");
        fields->stack_parser_progress=expected_progress=Progress::prefix_returned;full();
        const auto sequence=owner.sequence_,history_size=owner.history_.size();
        require(history_size<=owner.history_.max_size()-4&&sequence<=UINT64_MAX-4,"Stack438170 actual journal exhausted");
        owner.history_.reserve(history_size+4);full();require(owner.sequence_==sequence&&owner.history_.size()==history_size,
            "Stack438170 journal changed across realreserve POST");
        fields->stack_parser_progress=expected_progress=Progress::common_entered;
        full();owner.parse_layer_owned(layer); //4B0566->438170; proposed split verifies prefix, NEVER reruns it.
        expected_progress=Progress::id_lookup_returned;full();
        require(owner.sequence_==sequence+2&&owner.history_.size()==history_size+2&&
            owner.history_[history_size].command==NotificationLayoutBaseline::Mutation::context_realm_3c&&
            owner.history_[history_size+1].command==NotificationLayoutBaseline::Mutation::parser,
            "Stack438170 actual3C/parser effects did not return in originalorder");
        require_context_argument_owned(saved,argument);
        require(owner.base_.context_3c_observation==NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm&&
            same(owner.base_.context_realm_3c,argument->impl_->realm),"Stack4381AC actualsecondarg3C store missing");
        using StackList=NotificationLayoutOrdinaryChildren78::Impl::StackList;
        //The port allocates a genuine standalone list. Its typed VALUE ctor
        //really initializes count0/capacity4/data=THIS inline4. It does not
        //invent an Original global pool/counter or copy a different list.
        auto pending_list=std::unique_ptr<StackList>(new StackList);full();
        auto*const physical_list=pending_list.get();physical_list->first_without_callbacks();
        require(physical_list->count==0&&physical_list->capacity==4&&
            physical_list->data==physical_list->inline_slots.data()&&!physical_list->heap&&
            !physical_list->change&&physical_list->reserve_progress==StackList::ReserveProgress::unentered,
            "Stack495E10 real VALUE list constructor did not return");
        auto pending_array=std::unique_ptr<NotificationLayoutOrdinaryChildren78::Impl>(
            new NotificationLayoutOrdinaryChildren78::Impl(receiver,std::move(pending_list)));full();
        auto array_fields=std::shared_ptr<NotificationLayoutOrdinaryChildren78::Impl>(std::move(pending_array));full();
        auto array=std::shared_ptr<NotificationLayoutOrdinaryChildren78>(new NotificationLayoutOrdinaryChildren78(array_fields));full();
        require(!owner.base_.children_78&&array_fields->ordered.empty()&&
            array_fields->stack_list.get()==physical_list,"Stack495E10 genuine NULL78/list changed across allocation POST");
        owner.base_.children_78=array;published_array=array;full();
        const auto count=layer.children.size();
        require(count<=array_fields->ordered.max_size()&&count<=UINT32_MAX/4u,
            "Stack actualchildcount exceeds real 32bit allocation domain");
        struct Cursor final{std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;std::size_t next=0;
            void retire_owned(){selected.reset();}}cursor{xml};
        //The common ordered view is a paired strong observer of real data
        //slots. Reserve its OWN backing before any physical child stores.
        array_fields->ordered.reserve(count);full();
        require(array_fields->ordered.empty()&&array_fields->ordered.capacity()>=count,
            "Stack paired ordered78 observer reserve POST changed");
        enum class ResizeCall {reserve5e51d0,grow5b45d0};
        //The source contracts have DIFFERENT capacity writers. Only their
        //identical allocation/copy/free/install suffix is shared here.
        const auto resize_owned=[&](ResizeCall call,std::uint32_t requested){
            auto& list=*physical_list;
            try{
                full();list.first_without_callbacks();
                require(list.phase==StackList::Phase::stable&&list.count==array_fields->ordered.size(),
                    "Stack5B45D0 actual stable list/count changed before grow");
                const auto*const old_data=list.data;
                const auto old_count=list.count,old_heap_capacity=list.heap_capacity;
                const bool old_noninline=bool(list.heap);
                const auto old_change=list.change;
                auto expected_capacity=list.capacity;
                auto expected_phase=list.phase;
                const auto unchanged_data=[&]{full();list.first_without_callbacks();
                    require(list.data==old_data&&list.count==old_count&&list.heap_capacity==old_heap_capacity&&
                        bool(list.heap)==old_noninline&&array_fields->ordered.size()==old_count&&
                        list.capacity==expected_capacity&&list.phase==expected_phase&&list.change==old_change,
                        "Stack5B45D0 real old data/count/custody changed across FULL POST");
                    for(std::size_t j=0;j<old_count;++j)require(same(list.data[j],array_fields->ordered[j]),
                        "Stack5B45D0 real retained old slot changed");};
                list.phase=expected_phase=StackList::Phase::capacity_writes;
                if(call==ResizeCall::reserve5e51d0){
                    require(requested>list.capacity,"Stack5E51DE entered reserve with no actual capacity growth");
                    list.capacity=requested; //Original5E51DE, BEFORE allocator.
                }else{
                    if(!list.capacity)list.capacity=1u;
                    while(requested>=list.capacity){const auto old=list.capacity;
                        const auto doubled=static_cast<std::uint32_t>(old+old);
                        if(doubled<old){list.capacity=requested;break;}
                        list.capacity=doubled;
                    }
                }
                expected_capacity=list.capacity;
                unchanged_data();
                const auto target=list.capacity;
                require(target<=UINT32_MAX/4u,"Stack5B4627 original capacity*4 overflow needs its actual allocator branch");
                std::unique_ptr<StackList::Slot[]> new_heap;
                StackList::Slot* next=nullptr;
                if(target>4u){
                    //A real typed port allocation, with failure propagated.
                    //No borrowed child/Node is read inside the allocator.
                    new_heap=std::unique_ptr<StackList::Slot[]>(new StackList::Slot[target]);
                    unchanged_data();next=new_heap.get();
                }else{next=list.inline_slots.data();list.capacity=expected_capacity=4u;unchanged_data();}
                require(next,"Stack5B4627 actual allocator returned no writable storage");
                list.phase=expected_phase=StackList::Phase::allocated;unchanged_data();
                //Typed copy retains the SAME receiver controls; this is the
                //x64 owned equivalent of copying original count*4 pointers.
                for(std::size_t j=0;j<old_count;++j)next[j]=list.data[j];
                list.phase=expected_phase=StackList::Phase::copied;unchanged_data();
                for(std::size_t j=0;j<old_count;++j)require(same(next[j],array_fields->ordered[j]),
                    "Stack5B464C actual copied child control changed");
                if(old_noninline){
                    //ordered holds every actual child strongly, so releasing
                    //the old typed slots cannot retire a receiver mid-grow.
                    list.heap.reset();list.phase=StackList::Phase::old_released;
                    full();list.first_without_callbacks();
                    require(list.data==old_data&&list.count==old_count&&!list.heap&&
                        list.heap_capacity==old_heap_capacity,"Stack5B465F actual old release changed");
                }
                list.heap=std::move(new_heap);list.heap_capacity=target>4u?target:0u;
                list.data=next;list.change=1u;list.phase=StackList::Phase::stable;
                full();list.first_without_callbacks();
                require(list.count==old_count&&list.capacity==(target>4u?target:4u)&&list.change==1u,
                    "Stack5B4667/6A actual data/change1 publication changed");
            }catch(...){physical_list->failed=true;fail_storage_without_callbacks(saved);throw;}
        };
        const auto reserve_owned=[&](std::uint32_t requested){
            full();physical_list->first_without_callbacks();
            require(physical_list->phase==StackList::Phase::stable&&
                physical_list->reserve_progress==StackList::ReserveProgress::unentered,
                "Stack5E51D0 real reserve issuer reentered");
            const auto old_count=physical_list->count;
            const auto old_capacity=physical_list->capacity;
            const auto*const old_data=physical_list->data;
            const auto old_change=physical_list->change;
            if(requested>old_capacity)resize_owned(ResizeCall::reserve5e51d0,requested);
            //Real requested<=capacity branch goes directly toRET4. In
            //particular UNKNOWN change byte remains UNKNOWN on zero reserve.
            full();physical_list->first_without_callbacks();
            require(physical_list->count==old_count&&physical_list->capacity>=requested,
                "Stack5E5246 actual reserve count/capacity return changed");
            if(requested<=old_capacity)require(physical_list->capacity==old_capacity&&
                physical_list->data==old_data&&physical_list->change==old_change,
                "Stack5E51DB genuine RET4 no-store branch mutated physical fields");
            physical_list->reserve_progress=StackList::ReserveProgress::returned;full();
        };
        const auto grow_owned=[&](std::uint32_t requested){resize_owned(ResizeCall::grow5b45d0,requested);};
        //The operand comes from SAME captured source XML control, not a
        //sorted map, cached manifest count or current ordered array length.
        full();require(cursor.selected==xml&&same(xml->source.lock(),core->source)&&
            count==xml->value.children.size(),"Stack438770 actual selected XML request owner changed");
        reserve_owned(static_cast<std::uint32_t>(count));
        for(std::size_t i=0;i<count;++i){
            full();require(cursor.selected==xml&&cursor.next==i&&array_fields->ordered.size()==i&&
                physical_list->count==i,"Stack835640 actual selected XML/physical78 cursor changed");
            auto*const tree=const_cast<BnwWorldViewControlTree*>(saved->tree_facade);
            const auto*const styles=core->layout_parser_styles;
            require(styles,"Stack child genuine lexical Styles owner absent");
            auto frame=BnwWorldViewControlTree::CommonChildParserFrame::enter_owned(*tree,*this,i);
            auto returned=tree->capture_layout_node_canonical_owned(frame->child_id_,*styles,saved->serial);
            require(bool(returned),"Stack529520 child factory did not actually return");
            frame->close_after_child_return_owned(*returned);frame.reset();full();
            require(returned->creator_==std::this_thread::get_id()&&returned->consumed_&&returned->child_&&
                !returned->child_->alive.load()&&!returned->child_->failed.load(),
                "Stack685AA0 requires actual child Runtime DTOR and closed factory RETURN");
            const auto child_id=returned->child_->id;
            const auto& fresh=BnwWorldViewControlTree::Core::node(core->nodes,child_id);
            require(fresh.alive&&!fresh.dynamic&&fresh.layout_baseline&&
                &*fresh.layout_baseline==returned->child_->baseline,"Stack child physical Base changed after closed RETURN");
            auto& child=*const_cast<NotificationLayoutBaseline*>(returned->child_->baseline);
            auto*const child_address=&child;
            const auto child_receiver=child.base_.receiver_owned;
            const auto child_done=child.base_.common_return_owned;
            require(child_receiver&&child_receiver->impl_&&child_done&&child_done->impl_&&
                same(child_done,returned->native_)&&child.base_.children_78&&child.base_.children_78->impl_&&
                child.base_.parent_38.expired(),"Stack685AA0 actual child receiver/NULL38/CommonReturn absent");
            const auto child_fields=child_receiver->impl_;
            const auto image=child.image_;const auto box=child.box_;const auto grid=child.grid_;
            const auto button=child.button_;const auto label=child.standalone_label_;
            const bool actual_image=image&&same(child_fields->image.lock(),image)&&child_fields->box.expired()&&
                child_fields->grid.expired()&&child_fields->button.expired()&&child_fields->standalone_label.expired()&&child_fields->stack.expired();
            const bool actual_box=box&&same(child_fields->box.lock(),box)&&child_fields->image.expired()&&
                child_fields->grid.expired()&&child_fields->button.expired()&&child_fields->standalone_label.expired()&&child_fields->stack.expired();
            const bool actual_grid=grid&&same(child_fields->grid.lock(),grid)&&child_fields->image.expired()&&
                child_fields->box.expired()&&child_fields->button.expired()&&child_fields->standalone_label.expired()&&child_fields->stack.expired();
            const bool actual_button=button&&same(child_fields->button.lock(),button)&&child_fields->image.expired()&&
                child_fields->box.expired()&&child_fields->grid.expired()&&child_fields->standalone_label.expired()&&child_fields->stack.expired();
            const bool actual_label=label&&same(child_fields->standalone_label.lock(),label)&&child_fields->image.expired()&&
                child_fields->box.expired()&&child_fields->grid.expired()&&child_fields->button.expired()&&child_fields->stack.expired();
            const auto stack_alias=child_fields->stack.lock();
            const bool actual_stack=child.stack_&&stack_alias&&stack_alias.get()==&*child.stack_&&
                !stack_alias.owner_before(core)&&!core.owner_before(stack_alias)&&child_fields->image.expired()&&
                child_fields->box.expired()&&child_fields->grid.expired()&&child_fields->button.expired()&&child_fields->standalone_label.expired();
            require(actual_image||actual_box||actual_grid||actual_button||actual_label||actual_stack,
                "Stack685AA0 child needs its actual physical subtype/paired writer");
            require(child_fields->context.expired()&&!child_fields->stack_parent38_operands,
                "Stack685AA0 child has a different Context/parent issuer");
            if(actual_box)BnwWorldViewControlTree::require_box_native_common_return_owned(core,child_id,*saved->operation);
            if(actual_button)BnwWorldViewControlTree::require_button_native_common_return_owned(core,child_id,*saved->operation);
            if(actual_stack)BnwWorldViewControlTree::require_stack_native_common_return_owned(core,child_id,*saved->operation);
            using Label=NotificationLayoutStandaloneLabelState;
            using Base=NotificationLayoutBaseline;
            //This parent is the SAME actual Stack receiver ECX. Do not use
            //child vptr, CData, node kind or XML StackGrowth as this operand.
            const auto parent84=owner.stack_->field_84;
            const auto before40=child.base_.flags_40;
            const bool first_axis=parent84==0u||parent84==1u;
            const auto retained_bit=first_axis?0x200u:0x40u;
            const auto mask=first_axis?0xfffffe7fu:0xffffffcfu;
            const bool write40=(before40&retained_bit)==0u;
            const auto after40=write40?(before40&mask):before40;
            const auto base_sequence=child.sequence_,base_history=child.history_.size();
            const auto old_realm=child.base_.context_realm_3c;
            const auto old_observation=child.base_.context_3c_observation;
            const auto realm=owner.base_.context_realm_3c;
            const auto observation=owner.base_.context_3c_observation;
            const std::uint64_t mutations=(write40?1u:0u)+2u;
            require(base_sequence<=UINT64_MAX-mutations&&old_realm&&realm&&
                old_observation==Base::Base::Context3CObservation::parsed_realm&&
                observation==Base::Base::Context3CObservation::parsed_realm,
                "Stack685AA0/44BF40 actual journals or real parsed realms unavailable");
            const auto label_fields=actual_label?label->impl_:std::shared_ptr<Label::Impl>{};
            if(label_fields)label_fields->first();
            const auto wrapper=label_fields?label_fields->wrapper_returned:std::shared_ptr<const Label::WrapperReturn>{};
            const auto label_sequence=label_fields?label_fields->sequence:0;
            const auto label_history=label_fields?label_fields->history.size():0;
            unsigned stage=0; //actual execution: wrapper, parent38, realm3C, realappend
            const auto stored_count=[&]()->std::uint64_t{
                return (stage>=1&&write40?1u:0u)+(stage>=2?1u:0u)+(stage>=3?1u:0u);};
            const auto child_current=[&]{
                //Owned allocation/parent FIRST, then a fresh canonical address
                //check, before reading the old borrowed child Base.
                if(label_fields)label_fields->first();
                require_storage_without_callbacks(saved);
                const auto& current_child=BnwWorldViewControlTree::Core::node(core->nodes,child_id);
                require(current_child.alive&&!current_child.dynamic&&current_child.layout_baseline&&
                    &*current_child.layout_baseline==child_address&&
                    same(current_child.layout_receiver_occurrence,returned->occurrence_)&&
                    same(current_child.full_xml_occurrence,returned->xml_)&&
                    same(current_child.layout_baseline->base_.receiver_owned,child_receiver)&&
                    same(current_child.layout_baseline->image_,image)&&same(current_child.layout_baseline->box_,box)&&
                    same(current_child.layout_baseline->grid_,grid)&&same(current_child.layout_baseline->button_,button)&&
                    same(current_child.layout_baseline->standalone_label_,label),
                    "Stack child SAME physical allocation/selected occurrence changed");
                const auto actual_stack_alias=child_fields->stack.lock();
                require(!actual_stack||(child.stack_&&actual_stack_alias&&actual_stack_alias.get()==&*child.stack_&&
                    !actual_stack_alias.owner_before(core)&&!core.owner_before(actual_stack_alias)),
                    "Stack child SAME real embedded Stack address/control changed");
                require((!box||!box->failed)&&(!grid||!grid->failed)&&
                    returned->creator_==std::this_thread::get_id()&&returned->consumed_&&returned->child_&&
                    !returned->child_->alive.load()&&!returned->child_->failed.load()&&returned->child_->baseline==child_address&&
                    same(returned->native_,child_done)&&same(child.base_.common_return_owned,child_done)&&
                    child_fields->creator==std::this_thread::get_id()&&same(child_fields->core.lock(),core)&&
                    same(child_fields->canonical.lock(),child_receiver)&&same(child_fields->occurrence.lock(),returned->occurrence_)&&
                    child_fields->id==child_id&&child_fields->serial==saved->serial&&
                    same(returned->xml_->parent.lock(),xml)&&returned->xml_->ordinal==i&&
                    same(returned->xml_->source.lock(),core->source)&&
                    child_done->impl_->creator==std::this_thread::get_id()&&same(child_done->impl_->core.lock(),core)&&
                    same(child_done->impl_->occurrence.lock(),returned->occurrence_)&&same(child_done->impl_->selected.lock(),returned->xml_)&&
                    same(child_done->impl_->receiver.lock(),child_receiver)&&same(child_done->impl_->children.lock(),child.base_.children_78)&&
                    child_done->impl_->serial==saved->serial&&child_done->impl_->count==returned->xml_->value.children.size()&&
                    child.base_.children_78->impl_->creator==std::this_thread::get_id()&&
                    same(child.base_.children_78->impl_->parent.lock(),child_receiver)&&
                    child.base_.children_78->impl_->ordered.size()==child_done->impl_->count&&
                    child.sequence_==base_sequence+stored_count()&&child.history_.size()==base_history+stored_count()&&
                    child.base_.flags_40==(stage>=1?after40:before40)&&
                    (stage>=2?same(child.base_.parent_38.lock(),receiver):child.base_.parent_38.expired())&&
                    same(child.base_.context_realm_3c,stage>=3?realm:old_realm)&&
                    child.base_.context_3c_observation==(stage>=3?observation:old_observation)&&
                    owner.stack_->field_84==parent84&&same(owner.base_.receiver_owned,receiver)&&
                    same(owner.base_.context_realm_3c,realm)&&owner.base_.context_3c_observation==observation&&
                    cursor.selected==xml&&cursor.next==i+(stage>=4?1u:0u)&&
                    physical_list->count==i+(stage>=4?1u:0u)&&array_fields->ordered.size()==physical_list->count&&
                    array_fields->ordered.capacity()>=count,
                    "Stack685AA0/44BF40 closed RETURN, physical words, order or journals changed");
                if(stage<4)require(!child_fields->stack_parent38_operands,"Stack child parent association was issued before append returned");
                if(label_fields){
                    const auto* native_flags=std::get_if<std::uint32_t>(&label_fields->fields[0x40/4]);
                    const auto* native_parent=std::get_if<Label::Impl::ParentReceiverBorrow>(&label_fields->fields[0x38/4]);
                    const auto* native_realm=std::get_if<Label::Impl::ParsedRealm>(&label_fields->fields[0x3c/4]);
                    const auto* native_children=std::get_if<Label::Impl::ChildrenBorrow>(&label_fields->fields[0x78/4]);
                    require(same(label_fields->core.lock(),core)&&same(label_fields->occurrence.lock(),returned->occurrence_)&&
                        same(label_fields->selected.lock(),returned->xml_)&&label_fields->source_serial==saved->serial&&
                        wrapper&&wrapper->creator==std::this_thread::get_id()&&same(label_fields->wrapper_returned,wrapper)&&
                        same(wrapper->core.lock(),core)&&same(wrapper->occurrence.lock(),returned->occurrence_)&&
                        same(wrapper->selected.lock(),returned->xml_)&&same(wrapper->receiver.lock(),child_receiver)&&
                        same(wrapper->children.lock(),child.base_.children_78)&&same(wrapper->common.lock(),child_done)&&
                        wrapper->serial==saved->serial&&wrapper->sequence==label_sequence&&wrapper->base_sequence==base_sequence&&
                        label_sequence<=UINT64_MAX-mutations&&label_fields->sequence==label_sequence+stored_count()&&
                        label_fields->history.size()==label_history+stored_count()&&
                        label_fields->common_prefix_returned&&!label_fields->initialization_entered&&!label_fields->completed_standalone_run&&
                        native_flags&&*native_flags==child.base_.flags_40&&
                        (stage>=2?(native_parent&&same(native_parent->value.lock(),receiver)):
                         std::holds_alternative<Label::Impl::Null>(label_fields->fields[0x38/4]))&&
                        native_realm&&same(native_realm->value,stage>=3?realm:old_realm)&&
                        native_children&&same(native_children->object,child.base_.children_78),
                        "Stack Label actual wrapper/paired40/38/3C/78 physical words changed");
                    label_fields->first();
                }
                require_storage_without_callbacks(saved);
            };
            const auto child_full=[&]{child_current();full();child_current();};
            try{
                child_full();
                //Reserve owned journals under FULL PRE/POST, then publish
                //them with no borrowed Node-vector allocator mutation.
                std::vector<Base::Entry> next_base_history;
                require(base_history<=next_base_history.max_size()-mutations,
                    "Stack child actual Base journal capacity exhausted");
                next_base_history.reserve(base_history+mutations);child_full();
                std::vector<Label::Impl::Entry> next_label_history;
                if(label_fields){require(label_history<=next_label_history.max_size()-mutations,
                        "Stack Label actual journal capacity exhausted");
                    next_label_history.reserve(label_history+mutations);child_full();
                    for(const auto& entry:label_fields->history)next_label_history.push_back(entry);}
                for(const auto& entry:child.history_)next_base_history.push_back(entry);
                child.history_.swap(next_base_history);
                if(label_fields)label_fields->history.swap(next_label_history);
                //Real685ABE/ADF. Even an unchanged masked value is written
                //when its predicate takes the MOV; do not suppress that store.
                if(write40){
                    if(label_fields){label_fields->history.push_back({++label_fields->sequence,0x40});
                        label_fields->fields[0x40/4]=after40;}
                    child.history_.push_back({++child.sequence_,Base::Mutation::receiver_prefix});
                    child.base_.flags_40=after40;
                }
                stage=1;child_full(); //Only THEN actual tail44BF40.
                if(label_fields){label_fields->history.push_back({++label_fields->sequence,0x38});
                    label_fields->fields[0x38/4]=Label::Impl::ParentReceiverBorrow{receiver};}
                child.history_.push_back({++child.sequence_,Base::Mutation::receiver_prefix});
                child.base_.parent_38=receiver;stage=2;child_full();
                if(label_fields){label_fields->history.push_back({++label_fields->sequence,0x3c});
                    label_fields->fields[0x3c/4]=Label::Impl::ParsedRealm{realm};}
                child.history_.push_back({++child.sequence_,Base::Mutation::context_realm_3c});
                child.base_.context_realm_3c=realm;child.base_.context_3c_observation=observation;
                stage=3;child_full();
                //Common already published SAME real78; actual44BF59 branch
                //therefore does not invoke495E10 again. Its change-byte store
                //is mandatory before the real count==capacity grow branch.
                require(array_fields->stack_list.get()==physical_list&&same(owner.base_.children_78,array),
                    "Stack44BF59 SAME existing real78 changed");
                physical_list->change=0u;child_full();
                if(physical_list->count==physical_list->capacity){
                    grow_owned(physical_list->capacity);child_full();
                }
                require(physical_list->phase==StackList::Phase::stable&&physical_list->data&&
                    physical_list->count<physical_list->capacity&&physical_list->count==i,
                    "Stack44BF89 actual append storage unavailable");
                physical_list->data[physical_list->count]=child_receiver;
                ++physical_list->count;
                array_fields->ordered.push_back(child_receiver); //paired nonallocating strong view
                child_fields->stack_parent38_operands=NotificationLayoutOrdinaryReceiver::Impl::StackParent38Operands{
                    receiver,array,parent84,before40,saved->serial,base_sequence,base_history};
                ++cursor.next;stage=4;child_full();
                require(same(physical_list->data[i],child_receiver)&&same(array_fields->ordered[i],child_receiver)&&
                    child_fields->stack_parent38_operands,"Stack44BF89/8B actual slot/count return changed");
            }catch(...){if(label_fields)label_fields->failed.store(true);
                if(box)box->failed=true;if(grid)grid->failed=true;
                fail_storage_without_callbacks(saved);throw;}
        }
        require(array_fields->ordered.size()==count&&cursor.next==count&&cursor.selected==xml,
            "Stack4387C1 before all actualfactories/44BF40 attachments returned");
        owner.base_.flags_40|=8u;cursor.retire_owned();full(); //OR8 INCLUDING0, then actual8353F0 temporarycleanup.
        const auto serial=saved->serial;
        auto pending_return=std::unique_ptr<NotificationLayoutCommonChildFactoryReturn::Impl>(
            new NotificationLayoutCommonChildFactoryReturn::Impl(core,occurrence,xml,receiver,array,serial,count));full();
        auto done=std::shared_ptr<NotificationLayoutCommonChildFactoryReturn::Impl>(std::move(pending_return));full();
        auto returned=std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn>(new NotificationLayoutCommonChildFactoryReturn(done));full();
        require(!cursor.selected&&(owner.base_.flags_40&8u)&&array_fields->ordered.size()==count&&same(done->receiver.lock(),receiver)&&
            same(done->children.lock(),array)&&same(done->selected.lock(),xml),"Stack completedsuffix actualreturn operands changed across FULL POST");
        owner.base_.common_return_owned=returned;published_return=returned;
        fields->stack_parser_progress=expected_progress=Progress::common_returned;full();
        require_stack_common_return_from_parser_owned();
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void BnwWorldViewControlTree::require_stack_native_common_return_owned(
    const std::shared_ptr<Core>& core,std::uint64_t id,const runtime::BnwWorldViewSourceOperationLoan& operation,bool expected_busy){
    require(core&&id,"Stack nativecommon actual Core/id absent");core->layout_origin(expected_busy);operation.require_origin_without_callbacks();
    require(&core->source->current_operation_loan()==&operation,"Stack nativecommon SAMEcurrent Source changed");
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto& node=Core::node(core->nodes,id);const auto xml=node.full_xml_occurrence;
    require(node.alive&&!node.dynamic&&node.layout_baseline&&node.layout_receiver_occurrence&&xml&&!xml->dynamic,
        "Stack nativecommon actualallocation/constructor/XML absent");
    const auto& owner=*node.layout_baseline;const auto receiver=owner.base_.receiver_owned;
    const auto array=owner.base_.children_78;const auto returned=owner.base_.common_return_owned;
    require(owner.kind_==detail::NotificationLayoutBaseline::Kind::stack&&owner.actual_type_=="Stack"&&owner.stack_&&
        receiver&&receiver->impl_&&returned&&returned->impl_&&(owner.base_.flags_40&8u),
        "Stack nativecommon physicalctor/suffix/return owner unavailable");
    const auto fields=receiver->impl_;const auto done=returned->impl_;const auto physical=fields->stack.lock();
    require(physical&&physical.get()==&*owner.stack_&&!physical.owner_before(core)&&!core.owner_before(physical)&&
        fields->creator==std::this_thread::get_id()&&done->creator==std::this_thread::get_id()&&
        fields->image.expired()&&fields->box.expired()&&fields->grid.expired()&&fields->button.expired()&&fields->standalone_label.expired()&&
        fields->context.expired()&&!fields->button_void_f4_f8&&same(fields->core.lock(),core)&&same(done->core.lock(),core)&&
        same(fields->canonical.lock(),receiver)&&same(fields->occurrence.lock(),node.layout_receiver_occurrence)&&
        same(done->occurrence.lock(),node.layout_receiver_occurrence)&&same(done->selected.lock(),xml)&&same(xml->source.lock(),core->source)&&
        same(done->receiver.lock(),receiver)&&
        fields->id==id&&fields->serial==node.layout_receiver_occurrence->constructor_serial&&fields->serial==operation.operation_serial()&&
        done->serial==fields->serial&&done->count==xml->value.children.size()&&
        fields->stack_parser_progress==detail::NotificationLayoutOrdinaryReceiver::Impl::StackParserProgress::common_returned&&
        ((fields->stack_padding_reader_progress==detail::NotificationLayoutOrdinaryReceiver::Impl::StackPaddingReaderProgress::missing_returned&&!fields->stack_padding_read_return)||
         (fields->stack_padding_reader_progress==detail::NotificationLayoutOrdinaryReceiver::Impl::StackPaddingReaderProgress::present_returned&&fields->stack_padding_read_return))&&
        same(fields->stack_id58_return.lock(),owner.base_.id_58),"Stack sealed physical/control/Source/ordered78 association changed");
    require_native_ordinary_current_children_without_callbacks(receiver,returned,array);
    require(std::none_of(xml->value.attributes.begin(),xml->value.attributes.end(),[](const auto& row){return row.first=="Style";}),
        "Stack returning Style cannot replace realselected MISS lookup");
    const auto selected_padding=std::find_if(xml->value.attributes.begin(),xml->value.attributes.end(),[](const auto& row){return row.first=="Padding";});
    if(selected_padding==xml->value.attributes.end())require(!fields->stack_padding_read_return&&
        fields->stack_padding_reader_progress==detail::NotificationLayoutOrdinaryReceiver::Impl::StackPaddingReaderProgress::missing_returned,
        "Stack returning namedMISS cannot manufacture PRESENT owner");
    else{
        require(bool(fields->stack_padding_read_return),"Stack returning selected Padding lacks actual reader return");
        const auto& read=*fields->stack_padding_read_return;read.first_without_callbacks();
        require(same(read.selected.lock(),xml)&&same(read.physical_stack.lock(),physical)&&
            read.source_serial==fields->serial&&read.payload==selected_padding->second&&
            read.axis84==owner.stack_->field_84&&read.axis84<4u&&
            read.destination_offset==(read.axis84<2u?0x9cu:0x98u)&&
            read.after_word==std::bit_cast<std::uint32_t>(read.axis84<2u?owner.stack_->field_9c:owner.stack_->field_98),
            "Stack returning PRESENT SAME selected payload/locale/physical float destination changed");
    }
    require(std::none_of(xml->value.attributes.begin(),xml->value.attributes.end(),[](const auto& row){return row.first=="WrapPadding";}),
        "Stack returning WrapPadding requires its real81ED50 actor");
    const auto authored_id=std::find_if(xml->value.attributes.begin(),xml->value.attributes.end(),[](const auto& row){return row.first=="ID";});
    if(authored_id==xml->value.attributes.end()||authored_id->second.empty())require(!owner.base_.id_58,"Stack genuineNULL58 selectedbranch changed");
    else{require(owner.base_.id_58&&owner.base_.id_58->length==authored_id->second.size()&&authored_id->second.size()<32&&
        std::equal(authored_id->second.begin(),authored_id->second.end(),owner.base_.id_58->bytes.begin())&&
        owner.base_.id_58->bytes[authored_id->second.size()]=='\0',"Stack actualretained58 selectedbytes changed");}
    if(!array){core->layout_origin(expected_busy);operation.require_origin_without_callbacks();return;}
    const auto*const list=array->impl_->stack_list.get();
    require(list,"Stack returning real495E10 list allocation absent");list->first_without_callbacks();
    using StackList=detail::NotificationLayoutOrdinaryChildren78::Impl::StackList;
    require(list->phase==StackList::Phase::stable&&list->reserve_progress==StackList::ReserveProgress::returned&&
        list->count==array->impl_->ordered.size()&&list->capacity>=list->count&&
        (!list->count||list->change==0u),"Stack returning physical data/count/capacity/reserve/change-byte changed");
    for(std::size_t i=0;i<array->impl_->ordered.size();++i){
        const auto child_receiver=list->data[i];
        require(child_receiver&&child_receiver->impl_&&same(array->impl_->ordered[i],child_receiver),
            "Stack returning actual physical78/ordered child controls disagree");
        const auto cf=child_receiver->impl_;
        require(cf->creator==std::this_thread::get_id()&&same(cf->core.lock(),core)&&cf->stack_parent38_operands,
            "Stack returning child SAME Core/actual685AA0 association absent");
        const auto& before=*cf->stack_parent38_operands;
        const auto& child=Core::node(core->nodes,cf->id);
        require(child.alive&&!child.dynamic&&child.layout_baseline&&child.layout_receiver_occurrence&&child.full_xml_occurrence&&
            same(cf->canonical.lock(),child_receiver)&&same(cf->occurrence.lock(),child.layout_receiver_occurrence)&&
            cf->serial==fields->serial&&same(child.full_xml_occurrence->source.lock(),core->source)&&
            same(child.full_xml_occurrence->parent.lock(),xml)&&child.full_xml_occurrence->ordinal<done->count&&
            same(child.layout_baseline->base_.receiver_owned,child_receiver)&&same(before.parent.lock(),receiver)&&
            same(before.children.lock(),array)&&before.source_serial==fields->serial&&before.parent84==owner.stack_->field_84,
            "Stack returning actual parent receiver/argument/Source/ordinal association changed");
        const auto& state=*child.layout_baseline;
        const bool first_axis=before.parent84==0u||before.parent84==1u;
        const bool write40=(before.child40_before&(first_axis?0x200u:0x40u))==0u;
        const auto expected40=write40?(before.child40_before&(first_axis?0xfffffe7fu:0xffffffcfu)):before.child40_before;
        const std::uint64_t mutations=(write40?1u:0u)+2u;
        require(state.base_.flags_40==expected40&&(expected40&8u)&&
            same(state.base_.parent_38.lock(),receiver)&&same(state.base_.context_realm_3c,owner.base_.context_realm_3c)&&
            state.base_.context_3c_observation==owner.base_.context_3c_observation&&
            before.base_sequence<=UINT64_MAX-mutations&&
            (state.base_.children_78?state.sequence_==before.base_sequence+mutations:state.sequence_>=before.base_sequence+mutations)&&
            before.base_history_size<=state.history_.size()&&
            (state.base_.children_78?state.history_.size()-before.base_history_size==mutations:state.history_.size()-before.base_history_size>=mutations)&&
            state.history_[before.base_history_size+mutations-2].command==detail::NotificationLayoutBaseline::Mutation::receiver_prefix&&
            state.history_[before.base_history_size+mutations-1].command==detail::NotificationLayoutBaseline::Mutation::context_realm_3c&&
            (!write40||state.history_[before.base_history_size].command==detail::NotificationLayoutBaseline::Mutation::receiver_prefix),
            "Stack returning original685AA0 filter then44BF40 actual words/journal order changed");
        const auto child_done=state.base_.common_return_owned;
        require(child_done&&child_done->impl_,
            "Stack returning child's actual sealed CommonReturn/78 missing");
        const auto cd=child_done->impl_;
        require_native_ordinary_current_children_without_callbacks(child_receiver,child_done,state.base_.children_78);
        require(cd->creator==std::this_thread::get_id()&&same(cd->core.lock(),core)&&
            same(cd->occurrence.lock(),child.layout_receiver_occurrence)&&same(cd->selected.lock(),child.full_xml_occurrence)&&
            same(cd->receiver.lock(),child_receiver)&&
            cd->serial==fields->serial&&cd->count==child.full_xml_occurrence->value.children.size(),
            "Stack returning child Common suffix/physical owners/Source changed");
        const auto child_stack=cf->stack.lock();
        const bool actual_image=state.image_&&same(cf->image.lock(),state.image_)&&cf->box.expired()&&cf->grid.expired()&&
            cf->button.expired()&&cf->standalone_label.expired()&&cf->stack.expired();
        const bool actual_box=state.box_&&same(cf->box.lock(),state.box_)&&cf->image.expired()&&cf->grid.expired()&&
            cf->button.expired()&&cf->standalone_label.expired()&&cf->stack.expired();
        const bool actual_grid=state.grid_&&same(cf->grid.lock(),state.grid_)&&cf->image.expired()&&cf->box.expired()&&
            cf->button.expired()&&cf->standalone_label.expired()&&cf->stack.expired();
        const bool actual_button=state.button_&&same(cf->button.lock(),state.button_)&&cf->image.expired()&&cf->box.expired()&&
            cf->grid.expired()&&cf->standalone_label.expired()&&cf->stack.expired();
        const bool actual_label=state.standalone_label_&&same(cf->standalone_label.lock(),state.standalone_label_)&&
            cf->image.expired()&&cf->box.expired()&&cf->grid.expired()&&cf->button.expired()&&cf->stack.expired();
        const bool actual_stack=state.stack_&&child_stack&&child_stack.get()==&*state.stack_&&
            !child_stack.owner_before(core)&&!core.owner_before(child_stack)&&cf->image.expired()&&cf->box.expired()&&
            cf->grid.expired()&&cf->button.expired()&&cf->standalone_label.expired();
        require(cf->context.expired()&&(actual_image||actual_box||actual_grid||actual_button||actual_label||actual_stack),
            "Stack returning child's genuine physical subtype custody changed");
        if(actual_box)BnwWorldViewControlTree::require_box_native_common_return_owned(core,cf->id,operation,expected_busy);
        if(actual_button)BnwWorldViewControlTree::require_button_native_common_return_owned(core,cf->id,operation,expected_busy);
        if(actual_grid)require(!state.grid_->failed&&same(state.grid_->common_return_,child_done),
            "Stack returning genuine Grid Common control changed");
        if(actual_stack)BnwWorldViewControlTree::require_stack_native_common_return_owned(core,cf->id,operation,expected_busy);
        if(actual_label){
            using Label=detail::NotificationLayoutStandaloneLabelState;
            const auto label=state.standalone_label_;const auto lf=label->impl_;lf->first();
            const auto wrapper=lf->wrapper_returned;
            const auto* native_flags=std::get_if<std::uint32_t>(&lf->fields[0x40/4]);
            const auto* native_parent=std::get_if<Label::Impl::ParentReceiverBorrow>(&lf->fields[0x38/4]);
            const auto* native_realm=std::get_if<Label::Impl::ParsedRealm>(&lf->fields[0x3c/4]);
            const auto* native_children=std::get_if<Label::Impl::ChildrenBorrow>(&lf->fields[0x78/4]);
            require(wrapper&&wrapper->creator==std::this_thread::get_id()&&
                same(lf->core.lock(),core)&&same(lf->occurrence.lock(),child.layout_receiver_occurrence)&&
                same(lf->selected.lock(),child.full_xml_occurrence)&&lf->source_serial==fields->serial&&
                same(wrapper->core.lock(),core)&&same(wrapper->occurrence.lock(),child.layout_receiver_occurrence)&&
                same(wrapper->selected.lock(),child.full_xml_occurrence)&&same(wrapper->receiver.lock(),child_receiver)&&
                same(wrapper->children.lock(),cd->children.lock())&&same(wrapper->common.lock(),child_done)&&
                wrapper->serial==fields->serial&&wrapper->base_sequence==before.base_sequence&&
                wrapper->sequence<=UINT64_MAX-mutations&&
                (state.base_.children_78?lf->sequence==wrapper->sequence+mutations:lf->sequence>=wrapper->sequence+mutations)&&
                wrapper->sequence<=lf->history.size()&&lf->history.size()-wrapper->sequence>=mutations&&
                lf->history[wrapper->sequence+mutations-2].offset==0x38&&lf->history[wrapper->sequence+mutations-1].offset==0x3c&&
                (!write40||lf->history[wrapper->sequence].offset==0x40)&&
                native_flags&&*native_flags==expected40&&native_parent&&same(native_parent->value.lock(),receiver)&&
                native_realm&&same(native_realm->value,owner.base_.context_realm_3c)&&
                ((state.base_.children_78&&native_children&&same(native_children->object,state.base_.children_78))||
                 (!state.base_.children_78&&std::holds_alternative<Label::Impl::Null>(lf->fields[0x78/4])))&&
                lf->common_prefix_returned&&!lf->initialization_entered&&!lf->completed_standalone_run,
                "Stack returning Label genuine wrapper/paired40-before38/3C physical stores changed");
            lf->first();
        }
    }
    core->layout_origin(expected_busy);operation.require_origin_without_callbacks();
}

void detail::NotificationLayoutRuntimeDimensions::require_stack_common_return_from_parser_owned()const{
    const auto saved=state_;
    try{require_storage_without_callbacks(saved);const auto core=saved->core.lock();
        BnwWorldViewControlTree::require_stack_native_common_return_owned(core,saved->id,*saved->operation);
        require_storage_without_callbacks(saved);saved->operation->require_current();require_storage_without_callbacks(saved);
        BnwWorldViewControlTree::require_stack_native_common_return_owned(core,saved->id,*saved->operation);
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
