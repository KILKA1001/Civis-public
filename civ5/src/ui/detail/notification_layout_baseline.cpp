#include "civ5/ui/detail/notification_layout_baseline.hpp"
#include "civ5/content/info_types.hpp"
#include "civ5/ui/detail/notification_layout_runtime_dimensions.hpp"
#include "civ5/ui/detail/notification_layout_scroll_receiver.hpp"
#include "civ5/ui/detail/notification_layout_image_state.hpp"
#include "../bnw_native_color_load_kernel.hpp"
#include <bit>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>
namespace civ5::ui::detail {
namespace {
void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
constexpr float own_zero=std::bit_cast<float>(std::uint32_t{0});
constexpr float own_one=std::bit_cast<float>(std::uint32_t{0x3f800000});
static_assert(std::numeric_limits<float>::is_iec559&&sizeof(float)==4);
static_assert(std::bit_cast<std::uint32_t>(own_zero)==0);
constexpr std::array<float,2> own_arithmetic_origin{own_zero,own_zero};
float number(std::string_view text){
    float out;const auto result=std::from_chars(text.data(),text.data()+text.size(),out);
    need(!text.empty()&&result.ec==std::errc{}&&result.ptr==text.data()+text.size()&&std::isfinite(out),
         "Notification earned layout scalar is outside selected finite grammar");return out;
}
std::array<std::string_view,2> pair(std::string_view text){
    const auto at=text.find_first_of(" .,x*|");
    need(at!=text.npos&&at&&at+1<text.size()&&text.find_first_of(" .,x*|",at+1)==text.npos,
         "Notification earned layout pair needs exact selected two tokens");
    return {text.substr(0,at),text.substr(at+1)};
}
bool truth(std::string_view text){return !text.empty()&&(text.front()=='1'||text.front()=='t'||text.front()=='T');}
char first(std::string_view text){need(!text.empty(),"Notification earned layout keyword empty");
    const char ch=text.front();return ch>='a'&&ch<='z'?static_cast<char>(ch-'a'+'A'):ch;}
std::array<float,2> numeric_pair(std::string_view text){const auto p=pair(text);return {number(p[0]),number(p[1])};}
bool selected_full_size(std::string_view text){
    // Native76DC50 uses _strnicmp(...,"Full",1); admit only selected F/Full.
    if(text.size()!=1&&text.size()!=4)return false;
    constexpr std::string_view full="FULL";
    for(std::size_t k=0;k<text.size();++k){const char c=text[k];
        const char u=c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;
        if(u!=full[k])return false;}return true;
}
std::uint32_t anchor_bits(std::string_view text){const auto p=pair(text);std::uint32_t flags=0;
    const auto x=first(p[0]),y=first(p[1]);
    need(x=='L'||x=='C'||x=='R',"Notification earned horizontal anchor unsupported");
    need(y=='T'||y=='C'||y=='B',"Notification earned vertical anchor unsupported");
    if(x=='C')flags|=0x10;if(x=='R')flags|=0x20;
    if(y=='C')flags|=0x80;if(y=='B')flags|=0x100;return flags;
}
std::uint32_t side_bits(std::string_view text){const auto p=pair(text);
    return (first(p[0])=='O'?0x40u:0u)|(first(p[1])=='O'?0x200u:0u);
}
}
NotificationLayoutBaseline::NotificationLayoutBaseline(Kind kind,std::string type):kind_(kind),
 base_{{own_zero,own_zero,own_zero,own_zero},{own_zero,own_zero},{own_zero,own_zero},
       {own_zero,own_zero},{own_zero,own_zero},0},actual_type_(std::move(type)){
    base_.alpha_word_44=0x3f800000u; //actual4F94CF MOVSS [THIS+44],raw1
    // 4F9470 explicitly XORPS stores10..34,70/74 and zeroesflags40.
    // This implements the OWN canonical constructor, not native process capture.
    if(kind_==Kind::stack)stack_.emplace(Stack{0,2,0,own_zero,own_zero,own_zero,own_zero});
    if(kind_==Kind::scroll_panel)scroll_.emplace(Scroll{{own_zero,own_zero},
        {own_zero,own_zero,own_zero,own_zero},{own_zero,own_zero},own_zero,own_one,
        own_zero,own_zero,own_zero,std::nullopt});
    construct_grid_box_fields_owned();
    record_owned(Mutation::construct);
}
NotificationLayoutBaseline NotificationLayoutBaseline::construct_owned(const content::XmlNode& actual){
    const auto kind=actual.name=="Context"?Kind::context:actual.name=="Stack"?Kind::stack:actual.name=="ScrollPanel"?Kind::scroll_panel:
        actual.name=="ScrollBar"?Kind::slider:
        (actual.name=="UpButton"||actual.name=="DownButton"||actual.name=="Button")?Kind::button:
        actual.name=="Container"?Kind::container:actual.name=="Label"?Kind::label:actual.name=="Image"?Kind::image:
        actual.name=="Grid"?Kind::grid:actual.name=="Box"?Kind::box:Kind::base;
    // Native5297D6 Container ->70DC70 ->4F9470/A40CB8 has exactly the
    // existing literal common-base constructor/parser, with no subtype tail.
    // Other XML names remain unissued generic-base variants.
    return NotificationLayoutBaseline(kind,actual.name);
}
NotificationLayoutBaseline::Capture NotificationLayoutBaseline::capture_owned(
    std::uint64_t id,const content::XmlNode& authored,const std::vector<content::XmlNode>& actual_layers,
    const std::map<std::string,content::XmlNode,std::less<>>& actual_styles,std::uint64_t serial,
    const NotificationLayoutRuntimeDimensions* runtime_dimensions){
    need(!actual_layers.empty(),"Notification parser lacks retained style/authored layer sequence");
    if(authored.name=="Image")NotificationLayoutImageState::require_capture_profile_owned(actual_layers);
    auto state=construct_owned(authored);
    state.parse_canonical_owned(actual_layers,actual_styles,serial,runtime_dimensions);
    return Capture{id,std::move(state)};
}
#include "notification_layout_grid_box_prefix.inc"
#include "notification_layout_grid_parser_core.inc"
void NotificationLayoutBaseline::parse_canonical_owned(const std::vector<content::XmlNode>& actual_layers,
    const std::map<std::string,content::XmlNode,std::less<>>& actual_styles,std::uint64_t serial,
    const NotificationLayoutRuntimeDimensions* runtime_dimensions){
    need(!actual_layers.empty()&&!parsed_source_serial_,"Notification actual canonical parser replay/missing layers");
    if(kind_==Kind::image)NotificationLayoutImageState::require_capture_profile_owned(actual_layers);
    parser_runtime_dimensions_=runtime_dimensions;
    struct ParserRuntimeScope{NotificationLayoutBaseline& state;
        ~ParserRuntimeScope(){state.parser_runtime_dimensions_=nullptr;}} parser_scope{*this};
    parser_layers_=actual_layers;
    //Actual node-owned45EA90 owns its Style/5E/common order, never generic metadata.
    if(kind_==Kind::label){need(runtime_dimensions&&standalone_label_,"Standalone Label needs genuine allocated703120 object");
        runtime_dimensions->enter_standalone_label_slot44_owned();}
    // Existing subtype per-layer order is preserved; only receiver storage
    // now belongs to actual Core before any Size/registration/Anchor stage.
    else if(kind_==Kind::stack){
        need(runtime_dimensions&&stack_,"Stack requires genuine allocated4BDF90/actual Runtime");
        runtime_dimensions->enter_stack_slot44_owned();
    }
    else if(kind_==Kind::slider)
        slider_=NotificationLayoutSliderState::capture_owned(*this,actual_layers,actual_styles);
    else if(kind_==Kind::button)
        button_=NotificationLayoutButtonState::capture_owned(*this,actual_layers,actual_styles);
    else if(kind_==Kind::image)
        image_=NotificationLayoutImageState::capture_owned(*this,actual_layers,actual_styles);
    else if(kind_==Kind::grid||kind_==Kind::box)
        parse_grid_box_prefix_until_required_owner_owned(actual_layers);
    else for(const auto& layer:actual_layers)parse_layer_owned(layer);
    parser_runtime_dimensions_=nullptr;finish_parser_owned(serial);
}
NotificationLayoutBaseline NotificationLayoutBaseline::clone_owned(const NotificationLayoutBaseline& actual){
    need(!actual.standalone_label_,"Standalone Label cannot clone a live physical allocation as metadata");
    auto result=actual;
    if(actual.slider_)result.slider_=NotificationLayoutSliderState::clone_owned(actual.slider_);
    if(actual.button_)result.button_=NotificationLayoutButtonState::clone_owned(actual.button_);
    if(actual.image_)result.image_=NotificationLayoutImageState::clone_owned(actual.image_);
    return result;
}
const std::array<float,2>& NotificationLayoutBaseline::arithmetic_zero_owned()noexcept{return own_arithmetic_origin;}
void NotificationLayoutBaseline::record_owned(Mutation command){
    need(sequence_<UINT64_MAX,"Notification canonical layout history exhausted");
    history_.push_back(Entry{sequence_+1,command});++sequence_;
}
void NotificationLayoutBaseline::parse_stack_layer_owned(const content::XmlNode& layer){
    need(kind_==Kind::stack&&stack_&&!parsed_source_serial_,"Stack actual canonical parser occurrence absent/replayed");
    const auto direction=[](std::string_view text,std::uint32_t& destination,const char* why){
        if(text.empty())return; //Native first-byte comparisons all MISS; preserves prior genuine field.
        const auto value=first(text);
        if(value=='B'||value=='D')destination=0;
        else if(value=='T'||value=='U')destination=1;
        else if(value=='L')destination=2;
        else if(value=='R')destination=3;
        else need(false,why); //Only unsupported direction; never substitutes a default.
    };
    //4B02C0 per-layer subtype order precedes common438170 at4B0566.
    direction(layer.attribute("StackGrowth"),stack_->field_84,"Notification actual StackGrowth unknown");
    for(const auto& entry:layer.attributes)if(entry.first=="WrapWidth"){
        //4B03EA calls81EC40(base10,allow0x=true),4B03EF MOVW AX into8C.
        //Existing pure native integer kernel preserves DWORD wrap/sign/prefix,
        //then this literal low16 conversion preserves native short narrowing.
        const auto value=native_color_load_detail::integer(entry.second);
        stack_->field_8c=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(static_cast<std::uint32_t>(value)));
    }
    direction(layer.attribute("WrapGrowth"),stack_->field_88,"Notification actual WrapGrowth unsupported");
    //4B04C1..4B04FA selects Padding axis by84 and parses into98 or9C.
    if(parser_runtime_dimensions_){
        //Original4B04FA calls49A030 evenMISS and ignores its AL result.
        (void)parser_runtime_dimensions_->read_stack_padding_slot_owned(layer);
    }else if(const auto text=layer.attribute("Padding");!text.empty()){
        const auto value=number(text);if(stack_->field_84<2)stack_->field_9c=value;else stack_->field_98=value;
    }
    //4B0516..4B0555: WrapPadding only when88!=84; axis follows88.
    //90/94 are not padding: existing genuine4224D0 initializer owns anchor stores.
    for(const auto& entry:layer.attributes)if(entry.first=="WrapPadding"&&stack_->field_88!=stack_->field_84){
        const auto value=number(entry.second);
        if(stack_->field_88<2)stack_->field_9c=value;else stack_->field_98=value;
    }
}
void NotificationLayoutBaseline::parse_layer_owned(const content::XmlNode& layer){
    need(!parsed_source_serial_,"Notification canonical parser cannot replay after publication");
    if(stack_){
        if(parser_runtime_dimensions_)parser_runtime_dimensions_->require_stack_common_operand_slot_owned(layer);
        else parse_stack_layer_owned(layer); //Existing noncanonical pure layout route.
    } //ActualStack slot44 has already executed prefix ONCE before4B0566 Common.
    if(kind_==Kind::context&&truth(layer.attribute("DeferLoad")))base_.flags_40|=0x10000000U;
    // Real4381AC secondarg store precedes Offset, independently of child5E.
    // Context-root caller is deliberately unissued in this ordinary-only slice.
    if(parser_runtime_dimensions_&&kind_!=Kind::context)
        parser_runtime_dimensions_->assign_common_context_3c_owned(*this,layer);
    // The enclosing actual renderer/Tree issuer supplies retained style layers
    // in their real parse order. No runtime XML reread occurs in this state.
    if(const auto text=layer.attribute("Offset");!text.empty())base_.margin_70_74=numeric_pair(text);
    if(const auto text=layer.attribute("Size");!text.empty()){
        const auto tokens=pair(text);std::uint32_t full_flags=0;
        // Exact helper store order: X then Y, then caller ORs returned flags.
        for(std::size_t axis=0;axis<2;++axis){
            if(selected_full_size(tokens[axis])){
                need(parser_runtime_dimensions_,"Notification Full Size lacks canonical runtime dimensions");
                base_.size_30_34[axis]=parser_runtime_dimensions_->dimension_owned(axis);
                full_flags|=axis==0?0x100000u:0x200000u;
            }else base_.size_30_34[axis]=number(tokens[axis]);
        }
        base_.flags_40|=full_flags;
        // Native4381F5 tests accumulated flags, including earlier style layers.
        // Registration targets the actual canonical node, never this staged value.
        if(base_.flags_40&0x300000u){
            need(parser_runtime_dimensions_,"Notification Full Size lacks real resize registration");
            parser_runtime_dimensions_->register_resize_owned(base_.flags_40);
        }
    }
    if(const auto text=layer.attribute("Anchor");!text.empty())base_.flags_40|=anchor_bits(text);
    if(const auto text=layer.attribute("AnchorSide");!text.empty())base_.flags_40|=side_bits(text);
    //438263..4382C6 Color occurs after AnchorSide and before ID/Hidden.
    // Image virtual+58 is6CBAF0: literal stores98/9C/A0/A4, no dirty callbacks.
    if(kind_==Kind::image){
        for(const auto& entry:layer.attributes)if(entry.first=="Color"){
            need(image_&&parser_runtime_dimensions_,"Root Image Color lacks actual canonical parser borrower");
            const auto rgba=parser_runtime_dimensions_->image_color_owned(entry.second);
            // Supplier checked canonical receiver/lifetime FIRST after lookup.
            image_->color_98_a4=rgba;
        }
    }
    if(kind_==Kind::button){
        for(const auto& entry:layer.attributes)if(entry.first=="Color"){
            need(button_&&parser_runtime_dimensions_,"Button Color lacks actual canonical parser borrower");
            //4382B? virtual+58 is Button5CABC0, not Image6CBAF0.
            const auto rgba=parser_runtime_dimensions_->image_color_owned(entry.second);
            if(actual_type_=="Button")BnwNormalButtonSubobject118Backing::store_color5cabc0_owned(*button_,rgba);
            else button_->color_12c_138=rgba;
        }
    }
    if(kind_==Kind::image&&parser_runtime_dimensions_)
        parser_runtime_dimensions_->store_image_common_id_58_owned(layer);
    if(kind_==Kind::grid&&parser_runtime_dimensions_)
        parser_runtime_dimensions_->store_grid_common_id_58_owned(layer);
    if(kind_==Kind::button&&actual_type_=="Button"&&parser_runtime_dimensions_)
        parser_runtime_dimensions_->store_button_common_id_58_owned(layer);
    if(kind_==Kind::stack&&actual_type_=="Stack"&&parser_runtime_dimensions_)
        parser_runtime_dimensions_->store_stack_common_id_58_owned(layer);
    if(truth(layer.attribute("Hidden")))base_.flags_40|=2;
    //4383D2..438417 common ConsumeMouse lookup compares only first-byte t/1,
    //case-insensitively, then ORs all three real consume bits into flags40.
    //Selected ordinary Button retains its actual canonical flags/owner; this
    //does not allocate Label178, texture15C, or issue a hover/input receipt.
    if(kind_==Kind::button&&actual_type_=="Button"&&truth(layer.attribute("ConsumeMouse")))
        base_.flags_40|=0x1c00u;
    //4384AF is OR800 only. MISS/false preserves a bit already set by ConsumeMouse.
    if(kind_==Kind::button&&actual_type_=="Button"&&truth(layer.attribute("ConsumeMouseOver")))
        base_.flags_40|=0x800u;
    if(kind_==Kind::stack&&actual_type_=="Stack"&&parser_runtime_dimensions_){
        parser_runtime_dimensions_->require_stack_common_operand_slot_owned(layer);
        if(truth(layer.attribute("ConsumeMouse")))base_.flags_40|=0x1c00u;
        if(truth(layer.attribute("ConsumeMouseOver")))base_.flags_40|=0x800u;
    } //Real438417/4384AF OR-only; MISS/false retains prior flags, no clear.
    if(scroll_){
        if(truth(layer.attribute("AutoScrollBar")))base_.flags_40|=0x800000;
        if(truth(layer.attribute("HideScrollBar")))base_.flags_40|=0x1000000;
        if(truth(layer.attribute("Vertical")))base_.flags_40|=0x400000;
        // Genuine selected XML spells AutoScollbar; native parser reads ONLY
        // AutoScrollBar. Do not silently "correct" the original attribute.
    }
    //438502..438547 common Disabled compares first-byte t/1 and ORs20000.
    //Image701CFA calls this common parser last; no virtual setter, callback,
    //texture change, or constructor replacement is part of this authored store.
    if(kind_==Kind::image&&actual_type_=="Image"&&truth(layer.attribute("Disabled")))
        base_.flags_40|=0x20000u;
    //438575..4385B1 common ToolTip AFTER common flag operands. Actual
    //Button and Image share the SAME base54/flags4000/GUI524CC0 path.
    if(kind_==Kind::button||kind_==Kind::image)for(const auto& entry:layer.attributes)if(entry.first=="ToolTip"){
        need(((kind_==Kind::button&&button_)||(kind_==Kind::image&&actual_type_=="Image"&&image_))&&
            parser_runtime_dimensions_,"Button/Image ToolTip lacks actual canonical parser borrower");
        const auto localized=parser_runtime_dimensions_->tooltip_text_owned(entry.second);
        //5F3B90 releases the old54, ORs4000, then allocates/copies at most4000
        //UTF8 bytes plus terminatingNUL. Distinct actual storage, not asset metadata.
        base_.tooltip_54.reset();base_.flags_40|=0x4000u;
        base_.tooltip_54=std::make_shared<const std::string>(localized.substr(0,4000));
        parser_runtime_dimensions_->tooltip_changed_owned(); //actual GUI524CC0 branch, not unconditional no-op.
    }
    //4385C5..4385EA is a raw authored-byte CRC, not a Locale lookup or
    //ToolTip54 replacement. 838C90 folds CA7810 with seedFFFFFFFF/no xor;
    //existing FString hash kernel implements the same EDB88320 recurrence.
    if(kind_==Kind::button&&actual_type_=="Button")
        for(const auto& entry:layer.attributes)if(entry.first=="ToolTipType"){
            const auto hash=content::saved_info_type_hash(entry.second);
            base_.flags_40|=0x4000u;
            base_.tooltip_type_48=hash; //original OR4000 precedes actual48 store.
        }
    record_owned(Mutation::parser);
}
void NotificationLayoutBaseline::finish_parser_owned(std::uint64_t serial){
    need(serial&&!parsed_source_serial_,"Notification canonical parser publication repeated/unscoped");
    parsed_source_serial_=serial;
}
void NotificationLayoutBaseline::hide_owned(bool hidden){
    record_owned(Mutation::hide);if(hidden)base_.flags_40|=2;else base_.flags_40&=~2u;
}
void NotificationLayoutBaseline::offset_prefix_owned(const std::array<double,2>& value){
    std::array<float,2> narrowed;
    for(std::size_t k=0;k<2;++k){narrowed[k]=static_cast<float>(value[k]);need(std::isfinite(narrowed[k]),"Notification offset prefix nonfinite");}
    record_owned(Mutation::offset_prefix);base_.margin_70_74=narrowed;
    // No actual slot4C/94/child return is issued by recording these stores.
}
void NotificationLayoutBaseline::size_y_prefix_owned(float value){
    need(std::isfinite(value),"Notification size prefix nonfinite");record_owned(Mutation::size_prefix);base_.size_30_34[1]=value;
}
void NotificationLayoutBaseline::text_prefix_owned(const std::string& value){
    need(value.find('\0')==value.npos,"Notification text command contains NUL");
    record_owned(Mutation::text_prefix);actual_text_command_=value;
    // Real glyph/intrinsic-size initialization and virtual suffix remain separate.
}
}
