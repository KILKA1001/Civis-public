#include "civ5/ui/detail/notification_layout_image_state.hpp"
#include "civ5/ui/detail/notification_layout_runtime_dimensions.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string_view>
namespace civ5::ui::detail {
namespace {
void need_image(bool v,const char* why){if(!v)throw std::runtime_error(why);}
std::array<float,2> image_integer_pair(std::string_view text){
    // Selected integral two-token branch of76DC50. F/Full GUI operands and
    // native fallback/error grammars are deliberately not accepted here.
    const auto delimiter=[](char c){return std::string_view(" .,x*|").find(c)!=std::string_view::npos;};
    std::array<float,2> out;std::size_t at=0;
    for(auto& value:out){while(at<text.size()&&delimiter(text[at]))++at;
        const auto start=at;while(at<text.size()&&!delimiter(text[at]))++at;
        std::int32_t n;const auto token=text.substr(start,at-start);
        const auto parsed=std::from_chars(token.data(),token.data()+token.size(),n);
        need_image(!token.empty()&&parsed.ec==std::errc{}&&parsed.ptr==token.data()+token.size(),
            "Root Image TextureOffset outside selected integral76DC50 profile");value=static_cast<float>(n);
    }
    while(at<text.size()&&delimiter(text[at]))++at;
    need_image(at==text.size(),"Root Image TextureOffset has extra operands");return out;
}
}
void NotificationLayoutImageState::require_capture_profile_owned(const std::vector<content::XmlNode>& layers){
    need_image(!layers.empty(),"Root Image actual parser layers absent");
    for(const auto& layer:layers){
        for(const auto& entry:layer.attributes){
            const auto& key=entry.first;
            // Actual ActionInfo side images need these common/image attributes.
            // Reject PRESENT empty Style/MaskTexture/StretchMode/Color rather
            // than inferring their native branch from attribute().empty().
            if(!(key=="ID"||key=="Anchor"||key=="AnchorSide"||key=="Size"||
                key=="Offset"||key=="Hidden"||key=="Texture"||key=="TextureOffset"||key=="Sampler"||key=="Color"||key=="Disabled"||key=="ToolTip")){
                // Name only the already-refused real operand. Layers are the
                // actual retained source/style chain, not a new parser issuer.
                const auto& authored=layers.back();
                const auto why=std::string("Root Image reached unsupported authored/style parser operand")+
                    " [authored_tag="+authored.name+" authored_id="+authored.attribute("ID")+
                    " authored_style="+authored.attribute("Style")+" authored_texture="+authored.attribute("Texture")+
                    " layer="+layer.name+" layer_id="+layer.attribute("ID")+
                    " layer_style="+layer.attribute("Style")+" key="+key+" value="+entry.second+
                    "]";
                need_image(false,why.c_str()); // unchanged refusal and outer quarantine.
            }
            if(key=="Texture")need_image(!entry.second.empty()&&entry.second.find('\0')==std::string::npos,
                "Root Image PRESENT Texture is not native constructor NULL");
            if(key=="TextureOffset")(void)image_integer_pair(entry.second);
            if(key=="Sampler")need_image(entry.second=="Linear","Root Image selected Sampler must be exact Linear");
        }
    }
}
std::shared_ptr<NotificationLayoutImageState> NotificationLayoutImageState::capture_owned(
    NotificationLayoutBaseline& owner,const std::vector<content::XmlNode>& layers,
    const std::map<std::string,content::XmlNode,std::less<>>&){
    require_capture_profile_owned(layers);
    need_image(owner.kind_==NotificationLayoutBaseline::Kind::image&&owner.actual_type_=="Image",
        "Root Image capture requires actual5E4D70 constructor variant");
    need_image(!owner.image_,"Root Image actual embedded constructor replay");
    auto result=std::shared_ptr<NotificationLayoutImageState>(new NotificationLayoutImageState);
    // Actual canonical receiver retains earned constructor/stores on failure.
    // Common438170 Color+58 dispatch below addresses THIS embedded Image.
    if(owner.parser_runtime_dimensions_)owner.parser_runtime_dimensions_->publish_image_constructor_owned(result);
    else owner.image_=result; //Snapshot-only capture never receives a native parser-return token.
    for(const auto& layer:layers){
        //5C23C0 Style branch is absent in this finite actual-authored profile.
        if(owner.parser_runtime_dimensions_)owner.parser_runtime_dimensions_->load_image_texture_before_common_owned(layer);
        else for(const auto& entry:layer.attributes)if(entry.first=="Texture"){
            result->texture_key=entry.second;result->texture_origin=TextureOrigin::parsed_pending;
            result->texture_extent_c8.reset();result->texture_keeper.reset();
        }
        for(const auto& entry:layer.attributes)if(entry.first=="TextureOffset")
            result->offset_b8_bc=image_integer_pair(entry.second);
        for(const auto& entry:layer.attributes)if(entry.first=="Sampler")owner.base_.flags_40|=0x400000;
        // Native701CFA common438170 occurs LAST peractual layer. Raw texture
        // custody is completed in entered SAME resource scope before+10.
        if(owner.parser_runtime_dimensions_)owner.parser_runtime_dimensions_->store_image_extent_before_common_owned();
        owner.parse_layer_owned(layer);
        if(owner.parser_runtime_dimensions_)owner.parser_runtime_dimensions_->run_image_common_children_owned(layer);
    }
    return result;
}
std::shared_ptr<NotificationLayoutImageState> NotificationLayoutImageState::clone_owned(
    const std::shared_ptr<NotificationLayoutImageState>& value){
    return value?std::shared_ptr<NotificationLayoutImageState>(new NotificationLayoutImageState(*value)):nullptr;
}
void NotificationLayoutImageState::coordinates_owned(NotificationLayoutImageState& state){
    need_image(state.texture_origin!=TextureOrigin::parsed_pending,"Root Image Texture has no actual raw custody");
    if(state.texture_origin==TextureOrigin::constructor_null){
        need_image(state.texture_key.empty()&&!state.texture_extent_c8&&!state.texture_keeper,
            "Root Image constructor NULL custody substituted");return;
    }
    need_image(state.texture_extent_c8.has_value(),"Root Image actual retained resource extent absent");
    const auto d=*state.texture_extent_c8;need_image(d[0]&&d[1],"Root Image actual retained resource extent zero");
    // Literal embedded5C7B50 field order, independently stored rootImage UVs.
    // Same accepted OWN x87-result port qualification as embeddedButton math.
    state.uv_dc_e0={static_cast<float>(static_cast<long double>(state.offset_b8_bc[0])/d[1]),
                   static_cast<float>(static_cast<long double>(state.offset_b8_bc[1])/d[0])};
    state.uv_d4_d8={static_cast<float>(static_cast<long double>(state.image_b0_b4[0])/d[1]),
                   static_cast<float>(static_cast<long double>(state.image_b0_b4[1])/d[0])};
}
}
