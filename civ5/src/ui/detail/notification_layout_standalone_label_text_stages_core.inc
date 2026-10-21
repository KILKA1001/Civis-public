//Standalone native plain unwrapped/untruncated positive C4/CC branch; distinct owner/loan.
void detail::NotificationLayoutStandaloneLabelState::generate_text_segments_5a_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    const auto fields=impl_;loan->full();
    require(fields->init_stage==Impl::InitStage::common_returned,
        "Standalone5A needs earned own5761 and fresh genuine attached slot10 loan");
    const auto* font=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
    const auto* text=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4]);
    require(font&&font->loan&&text&&text->object,
        "Label5A selected nonNULL CC/C4 missing actual producers");
    const auto object=text->object;const auto allocation=object->allocation_child.lock();
    require(object->creator==std::this_thread::get_id()&&!object->retired.load()&&
        allocation&&allocation.get()==fields.get()&&!allocation.owner_before(fields)&&
        !fields.owner_before(allocation)&&object->source_serial==fields->source_serial&&
        !object->bytes.empty()&&object->bytes.size()<=0x4000&&object->bytes.back()=='\0'&&
        std::find(object->bytes.begin(),object->bytes.end(),'\0')==object->bytes.end()-1,
        "Label5A text is not this actual OWN UTF8+NUL object");
    //5A75CE/5A75DD happen before color conversion or markup resolution.
    loan->store(0x40);std::get<std::uint32_t>(fields->fields[0x40/4])|=0x00800000u;loan->baseline->base_.flags_40=std::get<std::uint32_t>(fields->fields[0x40/4]);loan->first();
    loan->store(0x138);fields->fields[0x138/4]=std::uint32_t(0);loan->first();
    std::array<std::uint32_t,3> colors{};
    const auto component=[&](std::uint32_t offset){
        const auto* bits=std::get_if<std::uint32_t>(&fields->fields[offset/4]);
        require(bits,"Label5A actual color component unknown");
        const auto f=std::bit_cast<float>(*bits);
        //Selected ColorAtlas authored components are finite [0,1]. This is
        //a qualifier, never Original-clamping or a guessed unknown63 byte.
        require(std::isfinite(f)&&f>=0.0f&&f<=1.0f,"Label5A color conversion outside qualified selected operands");
        return std::uint32_t(std::int64_t(std::trunc(double(f)*255.0)));
    };
    for(std::uint32_t layer=0;layer<3;++layer){
        const auto base=0x84u+layer*16;
        auto value=component(base+12)<<8;
        value=(value|(component(base+8)&255u))<<8;
        value=(value|(component(base+4)&255u))<<8;
        colors[layer]=value|(component(base)&255u);loan->first();
    }
    //Original global scratch color-stack resets count on EVERY call, pushes
    //the first packed color and performs no external callback on plain text.
    //For this plain/one-record branch the local value is observationally the
    //same scratch top; no native global capacity/address/counter is claimed.
    require(fields->style_c8.has_value(),"Label726E90 actual style byte unknown");
    const auto append_inline=[&](std::uint32_t begin,std::uint32_t count){
        const auto* inline_cell=std::get_if<Impl::SelfInline140>(&fields->fields[0x134/4]);
        const auto* size=std::get_if<std::uint32_t>(&fields->fields[0x138/4]);
        const auto* capacity=std::get_if<std::uint32_t>(&fields->fields[0x13c/4]);
        require(inline_cell&&inline_cell->actual_inline_cell==&fields->fields[0x140/4]&&
            size&&!*size&&capacity&&*capacity==1,
            "Label726E90 additional record requires genuine OWN grown segment vector");
        const auto put=[&](std::uint32_t offset,std::uint32_t value){
            loan->store(offset);fields->fields[offset/4]=value;loan->first();
        };
        loan->store(0x164);fields->segment_mutated_164=std::uint8_t(0);loan->first(); //726EE1 before capacity/copy
        put(0x140,begin);put(0x144,count);
        for(std::uint32_t layer=0;layer<3;++layer)put(0x148+layer*4,colors[layer]);
        //726E90 writes byte14 only. The other three padding bytes are still
        //unknown; do not invent a zero DWORD at154.
        loan->store(0x154);fields->inline_segment_newline_154=std::uint8_t(0);loan->first();
        put(0x158,std::uint32_t(*fields->style_c8));put(0x15c,0xffffffffu);put(0x160,0xffffffffu);
        put(0x138,1); //Actual count increment strictly after record stores.
    };
    const auto length=std::uint32_t(object->bytes.size()-1);
    const auto flags=std::get<std::uint32_t>(fields->fields[0x40/4]);
    for(std::uint32_t at=0;at<length;++at){
        if(object->bytes[at]=='['&&(flags&0x08000000u)){
            //5A776F/5A7784: doubled '[' appends the preceding bytes PLUS
            //the first literal '[' even at offset zero. Escape continuation
            //and any further segment remain mandatory; this is no admission.
            if(at+1<length&&object->bytes[at+1]=='[')append_inline(0,at+1);
            else if(at)append_inline(0,at);
            throw std::runtime_error("Label5A reached markup/escape: real ordered color/icon/newline producer remains mandatory");
        }
    }
    append_inline(0,length); //5A7D9E includes the actual empty-caption record.
    fields->init_stage=Impl::InitStage::glyph_generated;loan->full();
}

void detail::NotificationLayoutStandaloneLabelState::position_plain_segments_7505_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    const auto fields=impl_;loan->full();
    require(fields->init_stage==Impl::InitStage::glyph_generated&&fields->truncate_16c&&
        *fields->truncate_16c<=0,"Label7505 actual generated/plain untruncated branch unavailable");
    require(fields->wrap_168&&*fields->wrap_168==-1,
        "Label7505 actual6204A0 nontrivial word-wrap producer required");
    const auto* text=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4]);
    const auto* font_cell=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
    require(text&&text->object&&!text->object->retired.load()&&font_cell&&font_cell->loan,
        "Label7505 actual C4/CC retired");
    //Snapshot ALL physical operand owners by VALUE before any allocation or
    //Source/backend callback. Holding memory never revives retired C4/CC/170.
    const auto object=text->object;const auto font=font_cell->loan;
    std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    if(const auto* cell=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4])){
        require(bool(cell->loan),"Label7505 actual system descriptor empty");system=cell->loan;
    }else require(std::holds_alternative<Impl::Null>(fields->fields[0x170/4]),
        "Label7505 unknown170 is not NULL");
    const auto validate_operands=[&]{
        loan->first();
        const auto* actual_text=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4]);
        const auto* actual_font=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
        const auto* actual_system=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4]);
        const auto allocation=object->allocation_child.lock();
        require(actual_text&&actual_text->object.get()==object.get()&&
            !actual_text->object.owner_before(object)&&!object.owner_before(actual_text->object)&&
            object->creator==std::this_thread::get_id()&&!object->retired.load()&&
            allocation&&allocation.get()==fields.get()&&!allocation.owner_before(fields)&&
            !fields.owner_before(allocation)&&object->source_serial==fields->source_serial&&
            object->source_serial&&!object->bytes.empty()&&object->bytes.size()<=0x4000&&
            object->bytes.back()=='\0'&&std::find(object->bytes.begin(),object->bytes.end(),'\0')==object->bytes.end()-1&&
            actual_font&&actual_font->loan.get()==font.get()&&!actual_font->loan.owner_before(font)&&
            !font.owner_before(actual_font->loan)&&
            ((!system&&std::holds_alternative<Impl::Null>(fields->fields[0x170/4]))||
             (system&&actual_system&&actual_system->loan.get()==system.get()&&
              !actual_system->loan.owner_before(system)&&!system.owner_before(actual_system->loan)))&&
            fields->init_stage==Impl::InitStage::glyph_generated,
            "Label7505 SAME actual C4/CC/170 object/control/phase retired or changed");
        font->require_current_without_callbacks();loan->first();
        if(system){system->require_current_without_callbacks();loan->first();}
    };
    const auto full_operands=[&]{
        validate_operands();loan->full();validate_operands();
        font->require_source_owned(*loan->operation);validate_operands();
        if(system){system->require_source_owned(*loan->operation);validate_operands();}
        loan->full();validate_operands();
    };
    validate_operands();
    const auto bytes=object->bytes; //Strong TextObject snapshot precedes allocating vector copy.
    loan->full();validate_operands();require(bytes==object->bytes,"Standalone7505 C4 VALUE changed across allocating copy");
    full_operands();require(bytes==object->bytes,"Label7505 C4 VALUE changed across Source POST");
    //Qualified plain one-record producer owns real begin/count/newline/style;
    //copy all descriptor data BEFORE actual138 reset as Original75060E does.
    const auto word=[&](std::uint32_t offset){
        const auto* value=std::get_if<std::uint32_t>(&fields->fields[offset/4]);
        require(value,"Label7505 generated record word unknown");return *value;
    };
    require(word(0x138)==1&&word(0x140)==0&&word(0x144)==bytes.size()-1&&
        fields->inline_segment_newline_154&&!*fields->inline_segment_newline_154&&
        word(0x15c)==0xffffffffu&&word(0x160)==0xffffffffu,
        "Label7505 needs genuine additional/newline/icon record producers");
    std::array<std::uint32_t,9> record{};
    for(unsigned i=0;i<9;++i)if(i!=5)record[i]=word(0x140+i*4);
    const auto newline=*fields->inline_segment_newline_154; //Other three padding bytes remain unknown.
    loan->store(0x138);fields->fields[0x138/4]=std::uint32_t(0);loan->first();
    std::unique_ptr<BnwLabelCurrentTextLoan> input;
    std::shared_ptr<const BnwLabelSystemFontPreparedRun> prepared;
    if(system){
        input=current_text_owned(loan,false);input->require_source_owned(*loan->operation);loan->first();
        prepared=system->prepare_label_text_run_owned(*loan->operation,*input);loan->first();
        input->require_source_owned(*loan->operation);loan->first();
        require(bool(prepared),"Label7506AA actual410900 did not return completed preparation");
        prepared->require_source_owned(*loan->operation);loan->first();
    }
    full_operands();require(bytes==object->bytes,"Label7505 C4 VALUE changed before positioning");
    //621320 consumes the actual record style DWORD; no guessed Label-wide style.
    (void)font->style_layer_texture_indices_without_callbacks(record[6]);loan->first();
    fields->positioned_glyphs.clear();fields->positioned_glyphs.reserve(bytes.size()-1);
    full_operands();require(bytes==object->bytes,"Label7505 C4 VALUE changed across glyph reserve");
    std::uint32_t cursor=0;
    for(std::uint32_t at=0;at<bytes.size()-1;){
        const auto begin=at;const auto lead=std::uint8_t(bytes[at++]);
        unsigned count=0;std::uint32_t cp=lead;
        if(lead>=0xc2&&lead<=0xdf){count=1;cp=lead&0x1fu;}
        else if(lead>=0xe0&&lead<=0xef){count=2;cp=lead&0x0fu;}
        else if(lead>=0xf0&&lead<=0xf4){count=3;cp=lead&7u;}
        else require(lead<0x80,"Label486330 input outside qualified valid UTF8");
        require(at+count<=bytes.size()-1,"Label486330 truncated actual UTF8");
        for(unsigned i=0;i<count;++i){const auto next=std::uint8_t(bytes[at++]);
            require((next&0xc0)==0x80,"Label486330 invalid continuation byte");cp=(cp<<6)|(next&0x3fu);}
        require((count!=1||cp>=0x80)&&(count!=2||cp>=0x800)&&(count!=3||cp>=0x10000)&&
            cp<=0x10ffff&&!(cp>=0xd800&&cp<=0xdfff),"Label486330 noncanonical actual UTF8");
        if(std::get<std::uint32_t>(fields->fields[0x40/4])&0x80000000u)cp=0x2au;
        //Actual6204A0 wrap=-1 returns before glyph access. Font7640C0 then
        //735AB0 are distinct map-presence and real record lookups.
        const auto decoded=cp;std::array<std::uint32_t,9> bits{};
        if(system){
            //Original extended753CF0 is literal TRUE; no SPACE/GDI-has
            //substitution. The closed method still validates actual operands.
            const auto present=system->has_glyph_owned(*loan->operation,*input,cp);loan->first();
            require(present,"Label extended753CF0 must return genuine literalTRUE");
            bits=system->glyph_bits_owned(*loan->operation,*input,cp);loan->first();
            prepared->require_source_owned(*loan->operation);loan->first();
        }else{
            auto glyph=font->glyph_bits_without_callbacks(cp);loan->first();
            if(!glyph){cp=0x20;glyph=font->glyph_bits_without_callbacks(cp);loan->first();}
            require(glyph.has_value(),"Label735AB0 missingSPACE requires actual firstGlyph fallback owner");bits=*glyph;
        }
        const auto advance=std::bit_cast<float>(bits[8]);
        require(std::isfinite(advance)&&double(advance)>=-2147483648.0&&double(advance)<=2147483647.0,
            "Label75096A advance outside qualified x87 int64/lowword conversion");
        fields->positioned_glyphs.push_back({begin,at-begin,decoded,cp,cursor,bits});loan->first();
        cursor+=std::uint32_t(std::int64_t(std::trunc(double(advance)))); //Actual unsigned lowword addition.
    }
    if(record[1]){
        //Actual7509B7 resets byte164 then copies the SAME full descriptor.
        //Unknown padding remains represented separately instead of forged bits.
        loan->store(0x164);fields->segment_mutated_164=std::uint8_t(0);loan->first();
        for(unsigned i=0;i<9;++i){loan->store(0x140+i*4);
            if(i==5)fields->inline_segment_newline_154=newline;
            else fields->fields[(0x140+i*4)/4]=record[i];loan->first();}
        loan->store(0x138);fields->fields[0x138/4]=std::uint32_t(1);loan->first();
    } //Actual empty record is not re-emitted by7509A9 on this plain branch.
    full_operands();require(bytes==object->bytes,"Label7505 C4 VALUE changed before actual return");
    fields->init_stage=Impl::InitStage::positioned_returned;loan->full();
}

void detail::NotificationLayoutStandaloneLabelState::calculate_plain_size_5077_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    const auto fields=impl_;loan->full();require(fields->init_stage==Impl::InitStage::positioned_returned,
        "Label5077 missing actual7505 return");
    const auto* text=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4]);
    const auto* font_cell=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
    require(text&&text->object&&!text->object->retired.load()&&font_cell&&font_cell->loan,
        "Label5077 actual C4/CC absent or retired");
    //Snapshot ALL physical operand owners by VALUE before any allocation or
    //Source/backend callback. Holding memory never revives retired C4/CC/170.
    const auto object=text->object;const auto font=font_cell->loan;
    std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    if(const auto* cell=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4])){
        require(bool(cell->loan),"Label5077 actual system descriptor empty");system=cell->loan;
    }else require(std::holds_alternative<Impl::Null>(fields->fields[0x170/4]),
        "Label5077 unknown170 is not NULL");
    const auto validate_operands=[&]{
        loan->first();
        const auto* actual_text=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4]);
        const auto* actual_font=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
        const auto* actual_system=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4]);
        const auto allocation=object->allocation_child.lock();
        require(actual_text&&actual_text->object.get()==object.get()&&
            !actual_text->object.owner_before(object)&&!object.owner_before(actual_text->object)&&
            object->creator==std::this_thread::get_id()&&!object->retired.load()&&
            allocation&&allocation.get()==fields.get()&&!allocation.owner_before(fields)&&
            !fields.owner_before(allocation)&&object->source_serial==fields->source_serial&&
            object->source_serial&&!object->bytes.empty()&&object->bytes.size()<=0x4000&&
            object->bytes.back()=='\0'&&std::find(object->bytes.begin(),object->bytes.end(),'\0')==object->bytes.end()-1&&
            actual_font&&actual_font->loan.get()==font.get()&&!actual_font->loan.owner_before(font)&&
            !font.owner_before(actual_font->loan)&&
            ((!system&&std::holds_alternative<Impl::Null>(fields->fields[0x170/4]))||
             (system&&actual_system&&actual_system->loan.get()==system.get()&&
              !actual_system->loan.owner_before(system)&&!system.owner_before(actual_system->loan)))&&
            fields->init_stage==Impl::InitStage::positioned_returned,
            "Label5077 SAME actual C4/CC/170 object/control/phase retired or changed");
        font->require_current_without_callbacks();loan->first();
        if(system){system->require_current_without_callbacks();loan->first();}
    };
    const auto full_operands=[&]{
        validate_operands();loan->full();validate_operands();
        font->require_source_owned(*loan->operation);validate_operands();
        if(system){system->require_source_owned(*loan->operation);validate_operands();}
        loan->full();validate_operands();
    };
    validate_operands();
    const auto flags=std::get<std::uint32_t>(fields->fields[0x40/4]);
    //Capture descriptor scalars and a physically owned positioned vector
    //before the first backend call; no variant pointer survives a callback.
    const auto* count_cell=std::get_if<std::uint32_t>(&fields->fields[0x138/4]);
    require(count_cell&&*count_cell<=1,"Label5077 needs actual multi-record size traversal");
    const auto count=*count_cell;
    std::uint32_t span_bytes=0,record_style=0;
    if(count){
        const auto* style=std::get_if<std::uint32_t>(&fields->fields[0x158/4]);
        const auto* begin=std::get_if<std::uint32_t>(&fields->fields[0x140/4]);
        const auto* length=std::get_if<std::uint32_t>(&fields->fields[0x144/4]);
        require(style&&fields->inline_segment_newline_154&&!*fields->inline_segment_newline_154&&
            begin&&!*begin&&length&&*length==object->bytes.size()-1,
            "Label5077 actual plain descriptor changed");
        span_bytes=*length;record_style=*style;
    }
    const auto captured_sequence=fields->sequence;
    const auto glyphs=fields->positioned_glyphs;
    loan->full();validate_operands();require(fields->sequence==captured_sequence,
        "Standalone5077 actual positioned state changed across allocating copy");
    full_operands();require(fields->sequence==captured_sequence,
        "Label5077 actual positioned state changed across Source POST");
    std::unique_ptr<BnwLabelCurrentTextLoan> input;
    std::shared_ptr<const BnwLabelSystemFontPreparedRun> prepared;
    if(system){
        input=current_text_owned(loan,true);input->require_source_owned(*loan->operation);loan->first();
    }
    full_operands();require(fields->sequence==captured_sequence,
        "Label5077 actual positioned state changed before ellipsis");
    const auto advance_for=[&](std::uint32_t cp){
        if(flags&0x80000000u)cp=0x2au;
        std::array<std::uint32_t,9> bits{};
        if(system){
            require(system->has_glyph_owned(*loan->operation,*input,cp),"Label5077 extended753CF0 did not return literalTRUE");loan->first();
            bits=system->glyph_bits_owned(*loan->operation,*input,cp);loan->first();
            input->require_source_owned(*loan->operation);loan->first();
        }else{
            auto glyph=font->glyph_bits_without_callbacks(cp);loan->first();
            if(!glyph){glyph=font->glyph_bits_without_callbacks(0x20);loan->first();}
            require(glyph.has_value(),"Label6C5F/5077 missing SPACE requires actual firstGlyph fallback owner");bits=*glyph;
        }
        const auto advance=std::bit_cast<float>(bits[8]);
        require(std::isfinite(advance)&&double(advance)>=-2147483648.0&&double(advance)<=2147483647.0,
            "Label6C5F/5077 advance outside qualified x87 conversion");
        return std::uint32_t(std::int64_t(std::trunc(double(advance))));
    };
    //6C5FB0 walks actual Original PE A484F8 "...\0". No wholetext
    //preparation is called here; its three real glyph lookups still occur.
    std::uint32_t ellipsis_width=0;
    for(unsigned i=0;i<3;++i)ellipsis_width+=advance_for(0x2eu);
    (void)ellipsis_width; //Untruncated branch does not use its returned width.
    if(system){ //507836 actual410900 AFTER actual6C5FB0 returned.
        prepared=system->prepare_label_text_run_owned(*loan->operation,*input);loan->first();
        input->require_source_owned(*loan->operation);loan->first();
        require(bool(prepared),"Label507837 repeated410900 did not return completion");
        prepared->require_source_owned(*loan->operation);loan->first();
    }
    full_operands();require(fields->sequence==captured_sequence,
        "Label5077 actual positioned state changed before size traversal");
    std::uint32_t width=0,cursor=0;
    if(count){
        (void)font->style_layer_texture_indices_without_callbacks(record_style);loan->first(); //Actual621320.
        //The validated UTF8/codepoint span was produced by actual7505 above;
        //repeat EACH actual glyph getter now rather than copying cached width.
        std::uint32_t covered=0;
        for(const auto& glyph:glyphs){
            require(glyph.byte_begin==covered&&glyph.byte_count&&glyph.byte_count<=span_bytes&&covered<=span_bytes-glyph.byte_count,
                "Label5077 same positioned UTF8 span changed");
            cursor+=advance_for(glyph.codepoint);validate_operands();
            require(fields->sequence==captured_sequence,"Label5077 actual positioned state changed during glyph callback");
            if(cursor>width)width=cursor; //507A15 unsigned CMP/JBE after EACH advance, including negative lowwords.
            covered+=glyph.byte_count;
        }
        require(covered==span_bytes,"Label5077 actual UTF8 traversal incomplete");
    }
    //507A74 width store precedes the mandatory7AC170 metric producer below.
    const auto put=[&](std::uint32_t offset,float value){loan->put_geometry_owned(offset,value);};
    const auto get=[&](std::uint32_t offset){const auto* bits=std::get_if<std::uint32_t>(&fields->fields[offset/4]);
        require(bits,"Label5077 margin unknown");const auto value=std::bit_cast<float>(*bits);
        require(std::isfinite(value),"Label5077 margin nonfinite");return value;};
    full_operands();require(fields->sequence==captured_sequence,
        "Label5077 actual positioned state changed before geometry stores");
    put(0x30,float(width)); //507A74 FSTP32 BEFORE actual7AC170 metric lookup.
    const auto metrics=font->metrics_without_callbacks();validate_operands(); //7AC170 actual40C08.
    const auto height=std::uint32_t(metrics[0]); //Native lowword plus zero newline height, unsigned conversion.
    put(0x34,float(height)); //507A94 FST32 retains extended integer height on x87 stack.
    put(0x20,float(std::fabs(double(get(0x70)))+double(get(0x30)))); //507AA4 reloads rounded FP32 width.
    put(0x24,float(std::fabs(double(get(0x74)))+double(height))); //507AAF uses retained unrounded integer height.
    bounds_slot94_owned(loan);loan->full();
}
