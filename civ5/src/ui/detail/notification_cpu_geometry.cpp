#include "civ5/ui/detail/notification_cpu_geometry.hpp"
#include <charconv>
#include <cmath>
#include <stdexcept>
namespace civ5::ui::detail {
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
float scalar(std::string_view s){
    float v=0;const auto p=std::from_chars(s.data(),s.data()+s.size(),v);
    need(!s.empty()&&p.ec==std::errc{}&&p.ptr==s.data()+s.size()&&std::isfinite(v),"Notification actual authored CPU scalar invalid");return v;
}
std::array<std::string_view,2> pair(std::string_view s){
    const auto at=s.find_first_of(" .,x*|");
    need(at!=s.npos&&at&&at+1<s.size()&&s.find_first_of(" .,x*|",at+1)==s.npos,
        "Notification CPU producer supports selected nonempty two-token spelling only");
    return {s.substr(0,at),s.substr(at+1)};
}
char letter(std::string_view s){
    need(!s.empty(),"Notification authored CPU anchor empty");
    const auto c=s.front();return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;
}
float add(float a,float b){volatile float out=a+b;return out;}
float anchor(float origin,float parent,float size,char a,bool outside,float off){
    // Own layout convention. Native79D300/5895E0 parity remains a separate gate.
    float v;
    if(a=='C'){volatile float delta=parent-size;volatile float half=delta*0.5f;v=add(add(origin,half),off);}
    else if(a=='L'||a=='T')v=add(add(origin,outside?-size:0.0f),off);
    else if(a=='R'||a=='B')v=add(add(origin,parent),outside?off:add(-size,-off));
    else throw std::runtime_error("Notification actual CPU anchor unsupported");
    need(std::isfinite(v),"Notification CPU anchor overflow");return v;
}
}
std::uint32_t notification_authored_side_flags(std::string_view s){
    if(s.empty())return 0;
    const auto p=pair(s);
    // Literal native5B1B20 uses a case-insensitive first-character Out match.
    return (letter(p[0])=='O'?0x40u:0u)|(letter(p[1])=='O'?0x200u:0u);
}
NotificationCpuGeometry notification_cpu_leaf(const NotificationCpuLeafInput& in){
    NotificationCpuGeometry out=in.parent;out.id=out.parent=0;
    for(const auto v:in.parent.rect)need(std::isfinite(v),"Notification CPU actual parent nonfinite");
    out.hidden=in.parent.hidden||in.hidden;
    for(const auto& field:std::array<std::pair<const char*,std::string*>,3>{
        {{"Font",&out.font},{"FontStyle",&out.font_style},{"ColorSet",&out.color_set}}}){
        const auto value=in.effective.attribute(field.first);if(!value.empty())*field.second=value;
    }
    const auto authored_size=in.effective.attribute("Size");
    std::array<float,2> size{}; // Own zero-sized wrapper/base ctor model; no parent-size fallback.
    if(!authored_size.empty()){
        const auto p=pair(authored_size);
        for(std::size_t k=0;k<2;++k)size[k]=p[k]=="Full"?in.parent.rect[k+2]:scalar(p[k]);
    }else if(in.returned_intrinsic)size=*in.returned_intrinsic;
    if(in.actual_size_y)size[1]=*in.actual_size_y;
    for(const auto v:size)need(std::isfinite(v),"Notification actual CPU size nonfinite");
    out.intrinsic_size=size;
    std::array<float,2> offset{};
    if(in.actual_offset){
        for(std::size_t k=0;k<2;++k){offset[k]=static_cast<float>((*in.actual_offset)[k]);need(std::isfinite(offset[k]),"Notification actual CPU offset overflow");}
    }else {
        const auto authored_offset=in.effective.attribute("Offset");
        if(!authored_offset.empty()){const auto p=pair(authored_offset);offset={scalar(p[0]),scalar(p[1])};}
    }
    out.offset=offset;
    auto a=std::array<std::string_view,2>{"L","T"};
    const auto authored_anchor=in.effective.attribute("Anchor");
    if(!authored_anchor.empty())a=pair(authored_anchor);
    out.authored_side_flags=in.prior_authored_side_flags|notification_authored_side_flags(in.effective.attribute("AnchorSide"));
    out.rect={anchor(in.parent.rect[0],in.parent.rect[2],size[0],letter(a[0]),bool(out.authored_side_flags&0x40),offset[0]),
        anchor(in.parent.rect[1],in.parent.rect[3],size[1],letter(a[1]),bool(out.authored_side_flags&0x200),offset[1]),size[0],size[1]};
    return out;
}
}
