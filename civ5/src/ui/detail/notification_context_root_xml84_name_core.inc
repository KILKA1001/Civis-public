// PRIVATE inside civ5::ui after RootReadFileFrame and physical Context State.
// Supported own-x64 stream/XML adapter only; no Original FileAPI/CRC receipt.
// XML84 is mutable current state, separate from immutable constructor evidence.
struct BnwWorldViewControlTree::ContextRootXml84 final {
    using EmptyArena=context_constructor_prefix_detail::EmptyXmlArena;
    struct Storage final {
        // Own arena is the normalized x64 tree container. XmlNode recursively
        // owns parsed strings/nodes; no x86 arena offsets are used as pointers.
        std::unique_ptr<EmptyArena> reset_arena;
        std::vector<char> buffer_4;
        std::unique_ptr<content::XmlNode> document;
        Storage* quarantine_next=nullptr;
        explicit Storage(std::unique_ptr<EmptyArena> arena):reset_arena(std::move(arena)){}
    };
    const std::thread::id creator=std::this_thread::get_id();
    std::unique_ptr<Storage> storage;
    enum class Stage {empty,reset,buffer_allocated,read_complete,parsed,stream_retired,name_complete,defaults_complete};
    Stage stage=Stage::empty;
    std::optional<bool> parser_outcome;
    std::shared_ptr<const detail::NotificationContextRootDefaultFieldsReturn> defaults_return;
    std::uint32_t read_length=0;
    std::shared_ptr<const assets::Bytes> source_bytes;
    std::shared_ptr<const assets::detail::CivisLoadedXmlPhysicalPathLoan> physical;
    explicit ContextRootXml84(std::unique_ptr<Storage> value):storage(std::move(value)){}
    ~ContextRootXml84(){
        // Real Context retirement calls GUI676800 before XML84 retirement.
        // Until that owner is closed, retain the physical field, not a false
        // successful document cleanup. Reader-local streams still retire now.
        static thread_local Storage*head=nullptr;
        if(storage){auto*retained=storage.release();retained->quarantine_next=head;head=retained;}
    }
};

namespace context_root_xml84_supported_detail {
// No callbacks/allocations. Deliberately narrow grammar accepted by both the
// stock basic XML path and XmlLite: ASCII names, quoted plain attributes,
// whitespace-only element content, standard prolog and comments. Entities,
// CDATA, DTD, namespaces, nonASCII/attribute normalization stay required STOP.
inline bool space(char c){return c==' '||c=='\t'||c=='\r'||c=='\n';}
inline bool name_start(char c){return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||c=='_';}
inline bool name_char(char c){return name_start(c)||(c>='0'&&c<='9')||c=='.'||c=='-';}
inline void require_grammar(std::string_view xml){
    for(const unsigned char c:xml)require(c>0&&c<128&&(c>=32||space(char(c))),
        "required XML84 nonASCII/NUL/control-byte parser mapping");
    std::size_t at=0;
    if(xml.starts_with("<?xml")){
        constexpr std::string_view declaration="<?xml version=\"1.0\" encoding=\"utf-8\"?>";
        require(xml.starts_with(declaration),"required XML84 nonstandard XML declaration mapping");
        at=declaration.size();
    }
    const auto name=[&]{
        require(at<xml.size()&&name_start(xml[at]),"required XML84 qualified/nonstandard XML name");
        ++at;while(at<xml.size()&&name_char(xml[at]))++at;
    };
    const auto ws=[&]{while(at<xml.size()&&space(xml[at]))++at;};
    while(at<xml.size()){
        if(space(xml[at])){++at;continue;}
        require(xml[at]=='<',"required XML84 non-whitespace element content mapping");
        if(xml.substr(at).starts_with("<!--")){
            const auto end=xml.find("-->",at+4);
            require(end!=std::string_view::npos&&xml.substr(at+4,end-at-4).find("--")==std::string_view::npos,
                "required XML84 malformed comment/error mapping");
            at=end+3;continue;
        }
        ++at;require(at<xml.size(),"required XML84 truncated delimiter/error mapping");
        if(xml[at]=='/'){
            ++at;name();ws();require(at<xml.size()&&xml[at]=='>',"required XML84 malformed close mapping");++at;continue;
        }
        name();
        for(;;){
            const auto before_ws=at;ws();
            require(at<xml.size(),"required XML84 truncated start-tag mapping");
            if(xml[at]=='>'){++at;break;}
            if(xml[at]=='/'&&at+1<xml.size()&&xml[at+1]=='>'){at+=2;break;}
            require(at>before_ws,"required XML84 attribute separator mapping");
            name();ws();require(at<xml.size()&&xml[at]=='=',"required XML84 attribute '=' mapping");++at;ws();
            require(at<xml.size()&&(xml[at]=='\"'||xml[at]=='\''),"required XML84 unquoted attribute mapping");
            const char quote=xml[at++];
            while(at<xml.size()&&xml[at]!=quote){
                require((xml[at]!='&'&&xml[at]!='<'&&!space(xml[at]))||xml[at]==' ',
                    "required XML84 entity/attribute whitespace normalization mapping");++at;
            }
            require(at<xml.size(),"required XML84 unterminated attribute/error mapping");++at;
        }
    }
}
struct RetainedStream final {
    const std::shared_ptr<const assets::Bytes> bytes;
    std::size_t position=0;
    explicit RetainedStream(std::shared_ptr<const assets::Bytes> value):bytes(std::move(value)){}
    std::size_t read(char*destination,std::size_t requested){
        const auto count=std::min(requested,bytes->size()-position);
        std::copy_n(bytes->data()+position,count,destination);position+=count;return count;
    }
};
}

void BnwWorldViewControlTree::read_context_root_xml84_name_prefix_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::shared_ptr<ContextRootReadFileFrame> frame,
    std::shared_ptr<const assets::CivisResourceBankOwner> bank,
    std::shared_ptr<const assets::detail::CivisLoadedXmlPhysicalPathLoan> physical,assets::AssetKey key){
    using State=detail::NotificationLayoutContextOrdinaryState;using Document=ContextRootXml84;
    using ID=detail::NotificationLayoutControlID58;
    require(frame&&bank&&physical,"XML84 requires real entered RootRead/SAME bank/physicalwinner");frame->full();
    const auto actual=core_;const auto state=frame->state.lock();
    require(actual&&state&&state->impl_&&frame->operation==&operation&&
        actual->context_root_read_file_frame.lock()==frame,"XML84 actual RootRead receiver retired");
    physical->require_same_bank_without_callbacks(*bank);
    const auto bank_xml=physical->actual_xml_owned();frame->full();
    const auto loaded=bank->capability().loaded(key);frame->full();physical->require_same_bank_without_callbacks(*bank);
    const auto bytes=loaded.bytes; // VALUE retention before every allocation.
    require(bytes&&bank_xml&&State::Impl::same(loaded.xml,bank_xml),
        "XML84 bytes are not from same actual loaded physicalwinner document");
    require(bytes->size()>0&&bytes->size()<=64ULL*1024*1024&&bytes->size()<0xFFFFFFFFULL,
        "required XML84 empty/overflow/overbudget Original stream outcome mapping");
    const auto length=static_cast<std::uint32_t>(bytes->size());
    // The immutable constructor receipt proves the first empty current XML84.
    // First-use own-x64 materialization preserves it; never mutate ctorPort.
    require(!state->impl_->xml84_current&&!frame->expected_xml84,
        "required XML84 reload/reset of prior live parsed document supplier");
    state->impl_->native_constructor_return->impl_->port->require_completed();
    const auto prefix=state->impl_->native_constructor_prefix;
    const auto initial_xml_current=[&]{
        frame->full();
        require(prefix&&prefix->impl_&&prefix->impl_->port&&
            prefix->impl_->creator==std::this_thread::get_id()&&prefix->impl_->alive.load()&&!prefix->impl_->failed.load()&&
            State::Impl::same(prefix->impl_->core.lock(),actual)&&
            State::Impl::same(prefix->impl_->projection.lock(),state)&&
            State::Impl::same(state->impl_->native_constructor_prefix,prefix)&&
            State::Impl::same(state->impl_->native_constructor_return->impl_->prefix,prefix),
            "XML84 genuine initial Context XML arena constructor retired/changed");
        // No cold constructor first() under RootRead busy. This is retained
        // field evidence only; no constructor operation/scope is renewed.
        prefix->impl_->port->require_reached();
    };
    initial_xml_current();
    const auto current=[&]{
        initial_xml_current();physical->require_same_bank_without_callbacks(*bank);
        const auto fresh=bank->capability().loaded(key);frame->full();
        require(State::Impl::same(fresh.bytes,bytes)&&State::Impl::same(fresh.xml,bank_xml),
            "XML84 retained stream source bytes/XML changed");
    };
    auto arena=std::make_unique<Document::EmptyArena>();current();
    auto storage=std::make_unique<Document::Storage>(std::move(arena));current();
    auto document=std::make_shared<Document>(std::move(storage));current();
    require(!state->impl_->xml84_current&&!frame->expected_xml84,"XML84 current field changed during allocation");
    document->source_bytes=bytes;document->physical=physical;
    state->impl_->xml84_current=document;frame->expected_xml84=document;current();
    //834870 first-load reset: unlink empty arena, release old NULL buffer and
    // retain reset arena relations. Existing nonempty/reload arena unsupported.
    require(document->stage==Document::Stage::empty&&document->storage&&
        !document->storage->document&&document->storage->buffer_4.empty(),"XML84 actual first-load reset preimage changed");
    const auto&reset=*document->storage->reset_arena;
    require(!reset.first_0&&!reset.last_4&&!reset.nodes_10&&!reset.nodes_14&&!reset.nodes_18&&!reset.nodes_20&&
        reset.current_30==&reset&&reset.remaining_34==0x400&&!reset.next_arena&&
        reset.inline_cursor==reset.inline_storage.data()+8,"XML84 initial arena reset relations changed");
    document->stage=Document::Stage::reset;current();
    {
        context_root_xml84_supported_detail::RetainedStream stream(bytes);
        document->storage->buffer_4.resize(std::size_t(length)+1);current();
        document->stage=Document::Stage::buffer_allocated;document->read_length=length;
        std::size_t read=0;
        while(read<length){
            const auto count=stream.read(document->storage->buffer_4.data()+read,std::min<std::size_t>(length-read,65536));
            require(count>0&&count<=length-read,"required XML84 short/zero stream read outcome mapping");
            read+=count;current();
        }
        document->storage->buffer_4[length]='\0';document->stage=Document::Stage::read_complete;current();
        const auto view=std::string_view(document->storage->buffer_4.data(),length);
        context_root_xml84_supported_detail::require_grammar(view);current();
        // Parse the OWN current read buffer, never Source.root/Bank.xml metadata.
        // read_xml_metadata performs SHCreateMemStream/XmlLite with RAII reader
        // and IStream cleanup before returning. There is no Source callback
        // inside this private parse; all raw inputs remain physically retained.
        const std::string input(view);current();
        content::XmlNode parsed;
        try{parsed=content::read_xml_metadata(input);}
        catch(...){current();throw std::runtime_error("required XML84 malformed/XmlLite failure Original outcome mapping");}
        current();
        require(parsed.name=="Context","required XML84 non-Context root/cursor mapping");
        auto parsed_owner=std::make_unique<content::XmlNode>(std::move(parsed));current();
        document->storage->document=std::move(parsed_owner);document->parser_outcome=true;
        document->stage=Document::Stage::parsed;current();
        require(stream.position==length,"XML84 actual stream not fully consumed");
    } // normalized actual stream retirement precedes the successful8349D0 outcome.
    document->stage=Document::Stage::stream_retired;current();
    require(document->parser_outcome==true&&document->storage->document,
        "XML84 successful parse/stream retirement not completed");
    //6978F0 tests the actual physical Base58, not a Context metadata name.
    const auto old_id=actual->context_layout_baseline->base_.id_58;
    if(old_id&&old_id->bytes[0]!=0){document->stage=Document::Stage::name_complete;current();}
    else{
        //8355C0 type4 Name→8352A0 string, no attribute means real empty value.
        // Attribute value has already been parsed from the OWN mutable buffer.
        const auto&attrs=document->storage->document->attributes;
        std::string_view name_value;
        for(const auto&attr:attrs)if(attr.first=="Name"){name_value=attr.second;break;}
        require(name_value.size()<32,"required XML84 Name strncpy32 nontermination/strlen mapping");
        for(const unsigned char c:name_value)require(c>0&&c<128,"required XML84 Name nonASCII/NUL setter mapping");
        std::array<char,32> copied{};
        if(!name_value.empty())std::copy(name_value.begin(),name_value.end(),copied.begin());
        std::shared_ptr<const ID> replacement;
        if(!name_value.empty()){
            // LVALUES from parsed arena are not forwarded through allocation.
            const auto retained_length=static_cast<std::uint32_t>(name_value.size());
            replacement=std::shared_ptr<const ID>(new ID(copied,retained_length));current();
        }
        require(State::Impl::same(actual->context_layout_baseline->base_.id_58,old_id),
            "XML84 Base58 changed across Name allocation");
        //564300 typed own storage normalizes native32 buffer/string lifetime;
        // the supported <32 branch has the same NUL-padded bytes and strlen.
        // Cursor/temp-string locals are DATA, so retire before/after store as
        // proved; their lifetime provides no parser or wholeRead authority.
        actual->context_layout_baseline->base_.id_58=std::move(replacement);current();
        document->stage=Document::Stage::name_complete;current();
    }
    const auto defaults=read_context_root_default_fields_owned(operation,frame);
    const auto defaults_guard=defaults->capture_guard_without_callbacks(operation,frame->prepared);
    defaults_guard.require_current_owned(operation,frame->prepared);
    read_context_root_declarations_prefix_owned(operation,frame);
    require_context_root_read_staged_before_frame_exit_owned(frame);
    return; // genuine E lexical DTOR occurred; outer RootRead frame still live
}
