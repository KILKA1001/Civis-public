std::shared_ptr<const assets::CivisResourceBankOwner>
BnwSelectedWorldViewContextOwner::selected_provider_startup_bank_owned(const BnwWorldViewSourceOperationLoan& operation)const{
    operation.require_origin_without_callbacks();operation.require_render_operation_owned();
    const auto saved=state_;const auto scope=saved->scope_serial;
    const auto pure=[&]{require_operation_origin_without_callbacks(operation,scope);};
    try{
        pure();require(bool(saved->civis_sources),"Selected startup lacks actual own Bank Source");
        const auto bundle=saved->civis_sources;bundle->require_current();pure();
        const auto bank=bundle->bank_->retain_owner();pure();bundle->require_current();pure();
        require(bank&&bank.get()==bundle->bank_.get()&&!bank.owner_before(bundle->bank_)&&!bundle->bank_.owner_before(bank),
            "Selected startup Bank control differs from actual Source bundle");
        bank->require_current();pure();operation.require_operation(scope);pure();bundle->require_current();pure();
        require(saved->civis_sources.get()==bundle.get()&&!saved->civis_sources.owner_before(bundle)&&!bundle.owner_before(saved->civis_sources),
            "Selected startup canonical bundle changed");
        return bank;
    }catch(...){saved->failed.store(true);throw;}
}
// SAME selected Source's actual own Bank palette. No native global-bank receipt.
std::array<assets::CivisLoadedResource,2> BnwSelectedWorldViewContextOwner::color_atlas_resources_owned(
    const BnwWorldViewSourceOperationLoan& operation)const{
    operation.require_origin_without_callbacks();operation.require_render_operation_owned();
    const auto saved=state_;const auto scope=saved->scope_serial;
    const auto pure=[&]{require_operation_origin_without_callbacks(operation,scope);};
    try{
        pure();require(bool(saved->civis_sources),"ColorSet needs canonical actual staged Bank");
        const auto bundle=saved->civis_sources;const auto& recipe=bundle->recipe();pure();
        require(recipe.profile.enabled_mods.empty(),"ColorSet dynamic/mod provider setup unsupported");
        std::size_t declared_states=0;
        for(const auto index:recipe.profile.enabled_dlc){
            require(index<recipe.catalog.packages.size(),"ColorSet selected DLC index invalid");
            const auto& package=recipe.catalog.packages[index];
            for(const auto& skin:package.document.children)if(skin.name=="UISkin"&&skin.attribute("platform")=="Common"){
                for(const auto& state:skin.children)if(state.name=="Skin"||state.name=="GameplaySkin")
                    for(const auto& color:state.children)if(color.name=="Color"){
                        require(package.path.parent_path().filename()=="Expansion2"&&color.text=="UI/ColorAtlas_Expansion2.xml",
                            "ColorSet unsupported selected active Skin color declaration");++declared_states;
                    }
            }
        }
        require(declared_states==2,"ColorSet selected active Skin/GameplaySkin origin differs");
        std::array<assets::CivisLoadedResource,2> result;
        constexpr std::array<std::string_view,2> keys{"Assets/UI/ColorAtlas.xml","Assets/DLC/Expansion2/UI/ColorAtlas_Expansion2.xml"};
        for(std::size_t i=0;i<keys.size();++i){result[i]=bundle->loaded(keys[i]);pure();
            require(result[i].bytes&&result[i].xml&&result[i].xml->name=="ColorAtlas","ColorSet actual staged ColorAtlas unavailable");}
        operation.require_current();pure();return result;
    }catch(...){saved->failed.store(true);throw;}
}
std::array<float,4> BnwSelectedWorldViewContextOwner::image_color_value_owned(
    const BnwWorldViewSourceOperationLoan& operation,std::string_view input)const{
    const auto saved=state_;operation.require_origin_without_callbacks();
    const auto scope=saved->scope_serial;const std::string request(input);
    const auto pure=[&]{require_operation_origin_without_callbacks(operation,scope);};
    try{
        pure();require(!saved->image_color_querying,"Root Image actual named Color provider reentered");
        saved->image_color_querying=true;
        struct End{std::shared_ptr<State> state;~End(){state->image_color_querying=false;}}end{saved};
        require(saved->civis_sources&&!request.empty()&&request.size()<=1024&&request.find('\0')==std::string::npos,
            "Root Image Color requires actual bounded SAME own Bank source");
        const auto bundle=saved->civis_sources;bundle->require_current();pure();
        const auto font=bundle->font_color_snapshot().retain_owner();pure();
        require(&bundle->font_color_snapshot()==font.get(),"Root Image Color canonical palette owner replaced");
        namespace values=ui::native_color_load_detail;
        const auto text=values::trim(request);require(!text.empty(),"Root Image PRESENT Color is empty");
        values::Tokens tokens{text};const std::string name(tokens.take());
        require(!name.empty(),"Root Image Color first palette/numeric token is empty");
        font->require_boundary();pure();bundle->require_current();pure();
        const auto packed=font->strict_named_color(name); // Null ONLY validated palette MISS; errors propagate.
        pure();bundle->require_current();pure();font->require_current();pure();
        require((_mm_getcsr()&0xe040u)==0,"Root Image Color normalization unsupported current SSE mode");
        values::ColorBits bits{};
        if(packed){
            for(std::size_t i=0;i<4;++i)
                bits[i]=values::scaled(static_cast<std::int32_t>((*packed>>(24-8*i))&255u));
            //580853..5808A7: named HIT takes optional alpha token and uses
            //81EC40 -> CVTSI2SS/MULSS3b808081. No default-black substitute.
            if(tokens.next!=std::string_view::npos)bits[3]=values::scaled(values::integer(tokens.take()));
        }else values::numeric(request,bits); // Only genuine completed palette MISS, original4B9650 value route.
        font->require_boundary();pure();bundle->require_current();pure();
        require(saved->civis_sources.get()==bundle.get()&&!saved->civis_sources.owner_before(bundle)&&
            !bundle.owner_before(saved->civis_sources)&&&bundle->font_color_snapshot()==font.get(),
            "Root Image Color SAME retained source/palette changed across lookup");
        std::array<float,4> result;
        for(std::size_t i=0;i<4;++i)result[i]=std::bit_cast<float>(bits[i]);
        pure();return result;
    }catch(...){saved->failed.store(true);throw;}
}
