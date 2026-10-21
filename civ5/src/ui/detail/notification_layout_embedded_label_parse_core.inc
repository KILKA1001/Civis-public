// Inside civ5::ui AFTER actual Child/Factory/Realm definitions, BEFORE child5E.
// First genuine nonNULL Font only. Qualified6F3CD0 returns after the earned
// cache/NULL path and real early6A2EA0 RET. No parsed/init/paint grant follows.
std::shared_ptr<const detail::BnwLabelSystemFontBorrower>
detail::NotificationLayoutRuntimeDimensions::label_system_font_cache_owned(
    std::shared_ptr<const BnwCompletedFontLoan> font)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    try{
        const auto active=saved->active_button_string_layer_;
        require(bool(active),"Label170 cache outside genuine String lexical occurrence");
        require_button_string_layer_owned(saved,active,active->index_);
        const auto parent=saved->baseline->button_;
        require(parent&&parent->label_178_&&parent->label_178_factory_return_,
            "Label170 requires canonical published child");
        const auto returned=parent->label_178_factory_return_;const auto child=parent->label_178_;
        const auto fields=child->impl_;
        const auto pure=[&]{
            fields->first();require_label_factory_return_owned(saved,returned,active->index_);
            require_context_argument_owned(saved,saved->context_realm_argument);
            const auto* actual=std::get_if<NotificationLayoutEmbeddedLabelState::Impl::FontBorrow>(
                &fields->fields[0xcc/4]);
            require(actual&&font&&actual->loan.get()==font.get()&&
                !actual->loan.owner_before(font)&&!font.owner_before(actual->loan),
                "Label170 caller Font does not name genuine actual CC");
            require_storage_without_callbacks(saved);fields->first();
        };
        pure();saved->operation->require_current();pure();
        const auto manager=saved->manager.lock();require(bool(manager),"Label170 canonical Gui retired");
        pure();
        auto result=manager->begin_system_font_lookup_owned(*saved->operation,*font);
        //The Gui issuer owns genuine Font FIRST/POST, selectedSQL, registry,
        //actual50E500 cache hit/miss and full initialization/publication.
        pure();saved->operation->require_current();pure();
        return result; //NULL only the issuer's real SQL MISS; no unavailable-as-NULL.
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutEmbeddedLabelState::enter_first_font_setter_owned(
    const NotificationLayoutRuntimeDimensions& parser,
    const std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn>& returned,
    const std::shared_ptr<const BnwCompletedFontLoan>& font,std::size_t index)const{
    const auto fields=impl_;fields->first();
    require(returned&&returned->impl_&&
        returned->impl_->creator==std::this_thread::get_id()&&returned->impl_->alive.load(),
        "Label Font setter requires live actual constructor return FIRST");
    const auto stamp=returned->impl_;const auto saved=parser.state_;
    const auto pure=[&]{
        fields->first();
        NotificationLayoutRuntimeDimensions::require_label_factory_return_owned(saved,returned,index);
        require(stamp->published_sequence&&stamp->child.get()==this&&
            stamp->child->impl_.get()==fields.get()&&
            !stamp->child->impl_.owner_before(fields)&&!fields.owner_before(stamp->child->impl_),
            "6F3CD0 actual published child changed");
        NotificationLayoutRuntimeDimensions::require_context_argument_owned(saved,saved->context_realm_argument);
        require(bool(font),"6F3CD0 cannot synthesize a successful NULL Font return");
        font->require_current_without_callbacks();fields->first();
        if(const auto* system=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4])){
            require(bool(system->loan),"Label170 empty typed cache descriptor");
            system->loan->require_current_without_callbacks();fields->first();
        }else require(std::holds_alternative<Impl::Null>(fields->fields[0x170/4]),
            "Label170 unknown cache state");
        const auto* flags=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
        const auto* realm=std::get_if<Impl::ParsedRealm>(&fields->fields[0x3c/4]);
        require(fields->entered&&!fields->busy&&flags&&*flags==0x0b000000u&&
            realm&&realm->value&&realm->value.get()==saved->context_realm_argument->impl_->realm.get()&&
            !realm->value.owner_before(saved->context_realm_argument->impl_->realm)&&
            !saved->context_realm_argument->impl_->realm.owner_before(realm->value),
            "6F3CD0 requires earned child5E flags and ordinary parsed Realm");
    };
    try{
        pure();font->require_source_owned(*saved->operation);pure();
        // The first call's old CC is genuinely constructorNULL; the real
        // closed registry return is nonNULL. Thus6F3CD7 comparison differs,
        // 6F3CDF is nonzero, and the literal6F3CE3 store must happen.
        require(std::holds_alternative<Impl::Null>(fields->fields[0xcc/4]),
            "First6F3CD0 requires actual NULL CC; subsequent same-Font/replacement is unsupported");
        const auto sequence=fields->sequence;
        require(sequence<=UINT64_MAX-2&&fields->history.max_size()-fields->history.size()>=2,
            "6F3CD0 journal exhausted before CC store");
        fields->history.reserve(fields->history.size()+2);pure();
        font->require_source_owned(*saved->operation);pure();
        require(fields->sequence==sequence&&
            std::holds_alternative<Impl::Null>(fields->fields[0xcc/4]),
            "6F3CD0 child changed during allocation/Source POST");
        fields->history.push_back({++fields->sequence,0xcc});
        fields->fields[0xcc/4]=Impl::FontBorrow{font}; //6F3CE3; no Font refcount mutation.
        pure();
        const auto* stored=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
        require(stored&&stored->loan.get()==font.get()&&!stored->loan.owner_before(font)&&
            !font.owner_before(stored->loan),"6F3CD0 actual CC store changed");
        //6F3CFB does not branch with current0B000000. This private producer
        // must earn final4630A0 SQL refresh; constructor-zero is insufficient.
        const auto extended=parser.label_language_980a_owned();pure();
        require(fields->sequence==sequence+1&&
            std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4])&&
            std::get<Impl::FontBorrow>(fields->fields[0xcc/4]).loan.get()==font.get(),
            "6F3CD0 earned CC/history changed across language refresh");
        if(extended){
            //Actual40FF60 reads signed Font40C10, which the canonical Gui
            //issuer obtains from THIS closed Font, never request-name metadata.
            auto cache=parser.label_system_font_cache_owned(font);pure();
            if(cache){cache->require_source_owned(*saved->operation);pure();}
            require(fields->sequence==sequence+1&&
                std::get<Impl::FontBorrow>(fields->fields[0xcc/4]).loan.get()==font.get(),
                "6F3CD0 CC/history changed during genuine50E500 call");
            fields->history.push_back({++fields->sequence,0x170});
            if(cache)fields->fields[0x170/4]=Impl::SystemFontBorrow{std::move(cache)};
            else fields->fields[0x170/4]=Impl::Null{}; //Only real missingSQL row.
        }else{
            fields->history.push_back({++fields->sequence,0x170});
            fields->fields[0x170/4]=Impl::Null{}; //6F3D38 actual byte0 path.
        }
        pure();
        require((std::get<std::uint32_t>(fields->fields[0x40/4])&1u)==0,
            "6A2EA0 initialized layout rebuild requires actual mandatory producer");
        //Actual6A2EA3→6A2F0E RET: no texture/layout/metrics side effects.
        //This really returns the supported Font setter; no full parser receipt.
    }catch(...){
        fields->failed.store(true);
        NotificationLayoutRuntimeDimensions::fail_storage_without_callbacks(saved);
        throw; //Lexical catcher retires first; cleanup preserves/quarantines actual earned state.
    }
}

// Independent actual5E suffix, called only AFTER an earned Font setter return.
// Existing child fields are read and written; XML values cannot issue a run.
void detail::NotificationLayoutEmbeddedLabelState::parse_post_font_attributes_owned(
    const NotificationLayoutRuntimeDimensions& parser,
    const std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn>& returned,
    std::size_t index)const{
    const auto fields=impl_;fields->first();
    const auto saved=parser.state_;
    const auto pure=[&]{
        fields->first();
        NotificationLayoutRuntimeDimensions::require_label_factory_return_owned(saved,returned,index);
        require(returned->impl_->published_sequence&&returned->impl_->child.get()==this&&
            returned->impl_->child->impl_.get()==fields.get(),"Label5E suffix actual child changed");
        NotificationLayoutRuntimeDimensions::require_context_argument_owned(saved,saved->context_realm_argument);
        const auto* font=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
        require(font&&font->loan,"Label5E suffix genuine Font missing");
        font->loan->require_current_without_callbacks();fields->first();
        if(const auto* system=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4])){
            require(bool(system->loan),"Label5E empty typed170 descriptor");
            system->loan->require_current_without_callbacks();fields->first();
        }else require(std::holds_alternative<Impl::Null>(fields->fields[0x170/4]),
            "Label5E unknown170 state");
        const auto* flags=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
        require(flags&&(*flags&0x0b000000u)==0x0b000000u&&(*flags&1u)==0,
            "Label5E suffix initialized glyph rebuild is unsupported");
    };
    try{
        pure();saved->operation->require_current();pure();
        const auto active=saved->active_button_string_layer_;
        const auto attr=[&](std::string_view name)->std::optional<std::string>{
            pure();NotificationLayoutRuntimeDimensions::require_button_string_layer_owned(saved,active,index);
            std::optional<std::string> out;
            for(const auto& entry:active->reached_layer_->attributes)if(entry.first==name){out=entry.second;break;}
            pure();return out; //First exact-case PRESENT, including empty.
        };
        require(fields->sequence<=UINT64_MAX-64&&fields->history.max_size()-fields->history.size()>=64,
            "Label5E suffix journal exhausted before native stores");
        fields->history.reserve(fields->history.size()+64);pure();
        const auto store=[&](std::uint32_t offset){
            pure();fields->history.push_back({++fields->sequence,offset});
        };
        const auto flag=[&](std::uint32_t bits){
            store(0x40);std::get<std::uint32_t>(fields->fields[0x40/4])|=bits;pure();
        };
        const auto rebuild=[&]{
            pure();require((std::get<std::uint32_t>(fields->fields[0x40/4])&1u)==0,
                "6A2EA0 actual initialized glyph/layout producer unavailable");
            //6A2EA3→6A2F0E RET genuinely has no mutation in this state.
        };
        if(const auto colors=attr("ColorSet")){
            flag(0x04000000u); //5E3B1D BEFORE actual67C5C0.
            const auto source=parser.label_color_set_owned(*colors);pure();
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
                const auto rgba=parser.image_color_owned(*value);pure();
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
            std::string text=key->empty()?std::string{}:parser.tooltip_text_owned(ckey);pure();
            require(text.find('\0')==std::string::npos&&text.size()<0x4000,
                "Label611930 overlong/NUL text requires bounded raw-copy successor");
            //First actual text setter: C4 constructorNULL. No existing pooled
            //text object is falsely released; this fresh OWN x64 allocation
            //preserves exact UTF8+NUL bytes and has independent retirement.
            require(std::holds_alternative<Impl::Null>(fields->fields[0xc4/4]),
                "Label611930 subsequent text replacement requires genuine text retirement");
            auto object=std::make_shared<Impl::TextObject>();pure();
            object->allocation_child=fields;object->source_serial=fields->source_serial;
            object->bytes.assign(text.begin(),text.end());object->bytes.push_back('\0');pure();
            require(object->bytes.size()==text.size()+1&&
                std::equal(text.begin(),text.end(),object->bytes.begin())&&object->bytes.back()=='\0',
                "Label611930 actual owned text buffer changed");
            store(0xc4);fields->fields[0xc4/4]=Impl::TextBorrow{std::move(object)};pure();rebuild();
        }
        //Actual5E local FString cleanup is portable stack-string destruction.
        //No token is issued here: enclosing Button common/init/child retirement
        //and the selected extendedFont/ColorSet branches remain mandatory.
    }catch(...){fields->failed.store(true);NotificationLayoutRuntimeDimensions::fail_storage_without_callbacks(saved);throw;}
}
