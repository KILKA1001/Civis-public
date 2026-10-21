// Included in Baseline TU inside detail; finite selected numeric XML branch.
namespace {
std::array<float,2> grid_native_pair(std::string_view text){
    const auto delimiter=[](char c){return std::string_view(" .,x*|").find(c)!=std::string_view::npos;};
    std::array<float,2> result;std::size_t at=0;
    for(auto& value:result){
        while(at<text.size()&&delimiter(text[at]))++at;
        const auto begin=at;while(at<text.size()&&!delimiter(text[at]))++at;
        const auto token=text.substr(begin,at-begin);std::int32_t integer=0;
        const auto parsed=std::from_chars(token.data(),token.data()+token.size(),integer);
        need(!token.empty()&&parsed.ec==std::errc{}&&parsed.ptr==token.data()+token.size()&&
            integer>=-16777216&&integer<=16777216,
            "Grid76DC50 pair is outside exact selected integral FP32 branch; Full/float/error suffix required");
        //81ED71 calls the integer converter, then81ED76 CVTSI2SS.
        //Thus integral -0/-000 also becomes positive zero in this branch.
        value=static_cast<float>(integer);
    }
    while(at<text.size()&&delimiter(text[at]))++at;
    need(at==text.size(),"Grid76DC50 extra tokens require original continuation");
    return result;
}
}
void NotificationLayoutBaseline::parse_grid_421d70_owned(const std::vector<content::XmlNode>& layers){
    const auto grid=grid_;const auto runtime=parser_runtime_dimensions_;
    need(kind_==Kind::grid&&actual_type_=="Grid"&&grid&&!box_&&runtime&&
        !grid->parser_attempted&&!grid->failed&&!parsed_source_serial_&&!layers.empty(),
        "Grid421D70 requires its fresh ordinary constructor/canonical runtime");
    grid->parser_attempted=true;
    runtime->publish_grid_constructor_owned();
    const auto pure=[&]{runtime->require_grid_parser_owned(grid.get());};
    const auto parse=[&](auto&& self,std::size_t index)->void{
        pure();const auto layer=parser_layers_.at(index);pure(); //retained OWN immutable XML copy
        grid->frontier=NotificationLayoutGridState::Frontier::style_required;
        if(const auto style=runtime->run_grid_style_prefix_owned(layer,index)){pure();self(self,*style);pure();}
        else grid->absent_style_returned=true;
        grid->frontier=NotificationLayoutGridState::Frontier::nine_slice_parser_required;
        const auto query=[&](std::string_view key)->std::optional<std::string_view>{
            pure();for(const auto& item:layer.attributes)if(item.first==key)return item.second;return std::nullopt;
        };
        const auto pair_store=[&](std::string_view key,auto&& store){
            if(const auto text=query(key)){const auto value=grid_native_pair(*text);pure();store(value);pure();}
        };
        //Literal72A7A0 order: B8 first, then the exact native switch order.
        pair_store("TextureOffset",[&](const auto& value){grid->authored_b8_bc=value;});
        constexpr std::array<std::string_view,9> size_keys{
            "CSize","UCSize","LCSize","LSize","RSize","ULSize","URSize","LLSize","LRSize"};
        constexpr std::array<std::string_view,9> start_keys{
            "CTexStart","UCTexStart","LCTexStart","LTexStart","RTexStart","ULTexStart","URTexStart","LLTexStart","LRTexStart"};
        for(std::size_t n=0;n<9;++n){
            pair_store(size_keys[n],[&](const auto& value){for(std::size_t axis=0;axis<2;++axis)
                grid->words_c8_274[1][2*n+axis]=std::bit_cast<std::uint32_t>(value[axis]);});
            pair_store(start_keys[n],[&](const auto& value){for(std::size_t axis=0;axis<2;++axis)
                grid->words_c8_274[2][2*n+axis]=std::bit_cast<std::uint32_t>(value[axis]);});
        }
        pair_store("HKerning",[&](const auto& value){grid->kerning_a0_ac[0]=value[0];grid->kerning_a0_ac[1]=value[1];});
        pair_store("VKerning",[&](const auto& value){grid->kerning_a0_ac[2]=value[0];grid->kerning_a0_ac[3]=value[1];});
        if(query("Texture")){runtime->bind_grid_texture_owned(index);pure();} //real SAME load/278 store, never pending-success
        pair_store("StateOffsetIncrement",[&](const auto& value){grid->increment_b0_b4=value;});
        if(const auto text=query("NoStateChange");text&&truth(*text))base_.flags_40|=0x800000u;
        if(const auto text=query("AutoSize");text&&truth(*text))base_.flags_40|=0x400000u;
        pair_store("Padding",[&](const auto& value){grid->field_88_8c=value;});
        pure();
        //438170 is a separate required common parser. Only the independently
        //proved selected common operands below are admitted; no ignored Color,
        //ToolTip, BranchAlpha or mouse-list callbacks become successful no-ops.
        constexpr std::array<std::string_view,39> allowed{
            "Style","ID","TextureOffset","Texture","StateOffsetIncrement","NoStateChange","AutoSize","Padding",
            "HKerning","VKerning","CSize","UCSize","LCSize","LSize","RSize","ULSize","URSize","LLSize","LRSize",
            "CTexStart","UCTexStart","LCTexStart","LTexStart","RTexStart","ULTexStart","URTexStart","LLTexStart","LRTexStart",
            "Offset","Size","Anchor","AnchorSide","Hidden","NeedsMouseOver","NoClip","ConsumeMouse","ConsumeMouseButton",
            "ConsumeMouseOver","ConsumeMouseWheel"};
        for(const auto& item:layer.attributes)need(std::find(allowed.begin(),allowed.end(),item.first)!=allowed.end(),
            "Grid reached common attribute requires separate original supplier");
        //This first genuine return slice admits only native integral common
        //pairs. Full registration/unknown grammar remains an explicit frontier.
        for(const auto key:{std::string_view("Offset"),std::string_view("Size")})
            if(const auto value=query(key))(void)grid_native_pair(*value);
        runtime->begin_grid_common_empty_layer_owned(index);pure();
        grid->frontier=NotificationLayoutGridState::Frontier::common_initializer_required;
        pure();parse_layer_owned(layer);pure();
        //Known common literal flags follow Hidden. No values/semantics are
        //borrowed from Button dispatch: these are raw43837F..4384FB stores.
        for(const auto& spec:std::array<std::pair<std::string_view,std::uint32_t>,6>{{
            {"NeedsMouseOver",0x8000u},{"NoClip",0x40000u},{"ConsumeMouse",0x1c00u},
            {"ConsumeMouseButton",0x400u},{"ConsumeMouseOver",0x800u},{"ConsumeMouseWheel",0x1000u}}})
            if(const auto text=query(spec.first);text&&truth(*text))base_.flags_40|=spec.second;
        pure();runtime->finish_grid_common_empty_layer_owned(index);pure();
    };
    try{const auto count=layers.size();for(std::size_t i=0;i<count;++i){parse(parse,i);pure();}
        need(grid->common_return_&&base_.common_return_owned==grid->common_return_&&
            !grid->common_layer_&&grid->common_layer_returns_.size()==parser_layers_.size()&&
            !grid->common_layer_returns_.empty()&&grid->common_layer_returns_.back().first==0,
            "Grid421D70 cannot return without every actual layered common suffix and final Root return");
        grid->parser_returned=true;pure();
    }catch(...){grid->failed=true;throw;}
}
