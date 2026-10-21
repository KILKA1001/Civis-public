// Nested in private actual Gui::Impl; only canonical Source/Gui setup builds it.
// Immutable selected zero-mod ColorAtlas registry. No caller map/current bool.
struct SelectedColorSetRegistry final {
    using Known=detail::NotificationLabelColorSetKnownBytes;
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::array<assets::CivisLoadedResource,2> atlas;
    const std::uint64_t construction_serial;
    struct Loaded final {std::map<std::uint32_t,native_color_load_detail::ColorBits> colors;std::map<std::uint32_t,Known> sets;};
    const Loaded loaded,active_provider;
    static Loaded build(const assets::CivisLoadedResource& input){
        namespace v=native_color_load_detail;
        require(input.bytes&&input.xml&&input.xml->name=="ColorAtlas","ColorSet actual Bank ColorAtlas shape differs");
        Loaded result;
        //601CA0 performs all Color rows before all ColorSet rows, first hash wins.
        for(const auto& row:input.xml->children){
            require(row.name=="Color"||row.name=="ColorSet","ColorSet selected ColorAtlas contains unsupported row");
            const auto name=std::find_if(row.attributes.begin(),row.attributes.end(),[](const auto& a){return a.first=="Name";});
            require(name!=row.attributes.end()&&!name->second.empty()&&row.children.empty(),"ColorSet selected Name/row shape unsupported");
            if(row.name!="Color")continue;
            require(row.attributes.size()==2,"ColorSet named Color has unsupported actual attributes");
            const auto color=std::find_if(row.attributes.begin(),row.attributes.end(),[](const auto& a){return a.first=="Color";});
            require(color!=row.attributes.end()&&!v::trim(color->second).empty(),"ColorSet named Color lacks literal value");
            v::ColorBits value{};v::numeric(color->second,value);
            v::remember_first_hash(result.colors,v::hash_query(name->second),value);
        }
        for(const auto& row:input.xml->children)if(row.name=="ColorSet"){
            const auto name=std::find_if(row.attributes.begin(),row.attributes.end(),[](const auto& a){return a.first=="Name";});
            require(name!=row.attributes.end(),"ColorSet selected definition Name absent");
            const auto key=v::hash_length(name->second);if(result.sets.contains(key))continue;
            for(const auto& a:row.attributes)require(a.first=="Name"||a.first=="Color0"||a.first=="Color1"||a.first=="Color2"||a.first=="Color3",
                "ColorSet selected definition attribute unsupported");
            v::ColorSetBytes scratch{};v::default_color_set(scratch);
            Known known{};for(std::size_t i=0;i<63;++i)known[i]=scratch[i];
            const auto publish=[&](std::size_t start,std::size_t length){for(std::size_t i=start;i<start+length;++i)known[i]=scratch[i];};
            for(std::size_t slot=0;slot<4;++slot){
                const auto attr="Color"+std::to_string(slot);
                const auto value=std::find_if(row.attributes.begin(),row.attributes.end(),[&](const auto& a){return a.first==attr;});
                if(value==row.attributes.end())continue;
                const auto text=v::trim(value->second);if(text.empty())continue; //76D3C0 no mutation.
                v::Tokens tokens{text};const auto first=tokens.take();
                const auto color_key=v::hash_query(first);const auto color=result.colors.find(color_key);
                if(!first.empty()&&color!=result.colors.end()){
                    v::component_named_value(scratch,slot,color->second,color_key);
                    if(tokens.next!=std::string_view::npos)v::component_named_alpha(scratch,slot,tokens.take());
                }else v::component_literal(scratch,slot,text); // Only actual completed named palette MISS.
                // Publish only bytes actually written. Slot3 can legitimately
                // write3F; default/selected slots0..2 keep that byte unknown.
                publish(slot*16,16);publish(0x30+slot*4,4);publish(0x3c+slot,1);
            }
            result.sets.emplace(key,std::move(known));
        }
        return result;
    }
    static Loaded build_selected_provider(const assets::CivisLoadedResource& input){
        require(input.xml&&input.xml->name=="ColorAtlas","Active ColorSet provider lacks actual staged atlas");
        for(const auto& row:input.xml->children){
            require(row.name=="ColorSet","Active ColorSet provider named Color registration unsupported");
            for(const auto& a:row.attributes)if(a.first!="Name"){
                const auto text=native_color_load_detail::trim(a.second);native_color_load_detail::Tokens tokens{text};
                const auto first=tokens.take();require(!first.empty()&&std::isdigit(static_cast<unsigned char>(first.front())),
                    "Active ColorSet provider nonliteral color lookup needs original palette-provider join");
            }
        }
        return build(input);
    }
    SelectedColorSetRegistry(std::array<assets::CivisLoadedResource,2> source,std::uint64_t serial)
        :atlas(std::move(source)),construction_serial(serial),loaded(build(atlas[0])),active_provider(build_selected_provider(atlas[1])){
        require(serial,"ColorSet setup lacks actual Source occurrence");}
    void first()const{require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),"ColorSet registry retired/foreign/failed FIRST");}
    ~SelectedColorSetRegistry(){alive.store(false);}
};
std::shared_ptr<SelectedColorSetRegistry> selected_color_sets;
