#pragma once
#include <charconv>
#include <cstdint>
#include <cmath>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
namespace civ5::graphics::unit_art_scale_detail {
struct UserScaleFields {float scale,multiplier;bool scale_defaulted,multiplier_defaulted;std::int32_t seed=-1,threaded=1;bool seed_defaulted=true,threaded_defaulted=true;};
inline UserScaleFields read_user_scale_fields(std::string_view text) {
    const auto need=[](bool v,const char* s){if(!v)throw std::invalid_argument(s);};
    const auto trim=[](std::string_view v){auto p=v.find_first_not_of(" \t\r\n");return p==v.npos?std::string_view{}:v.substr(p,v.find_last_not_of(" \t\r\n")-p+1);};
    need(text.size()<=65536&&text.find('\0')==text.npos,"User unit INI extent/NUL unsupported");
    for(unsigned char c:text)need(c=='\r'||c=='\n'||c=='\t'||(c>=32&&c<127),"User unit INI encoding lexical branch unsupported");
    std::map<std::string,std::string> fields;std::string section;
    for(std::size_t at=0;at<text.size();){auto end=text.find('\n',at);auto line=trim(text.substr(at,end==text.npos?end:end-at));at=end==text.npos?text.size():end+1;if(line.empty()||line.front()==';')continue;
        if(line.front()=='['){need(line.back()==']'&&line.size()>2,"Malformed user unit INI section");section=std::string(line.substr(1,line.size()-2));continue;}auto eq=line.find('=');need(!section.empty()&&eq!=line.npos&&eq!=0,"Malformed user unit INI entry");if(section!="Debugging")continue;auto key=std::string(trim(line.substr(0,eq)));if(key!="GlobalUnitScale"&&key!="GlobalUnitMemberMultiplier"&&key!="GlobalUnitPlacementSeed"&&key!="UnitRenderMultiThreaded")continue;need(fields.emplace(key,std::string(trim(line.substr(eq+1)))).second,"Duplicate selected user unit INI key");}
    const auto number=[&](const char* key)->std::optional<float>{auto f=fields.find(key);if(f==fields.end())return {};float value=0;auto result=std::from_chars(f->second.data(),f->second.data()+f->second.size(),value);need(result.ec==std::errc{}&&result.ptr==f->second.data()+f->second.size()&&std::isfinite(value),"Unsupported selected user unit INI numeric value");return value;};
    auto scale=number("GlobalUnitScale"),mult=number("GlobalUnitMemberMultiplier");
    const auto integer=[&](const char* key)->std::optional<std::int32_t>{auto f=fields.find(key);if(f==fields.end())return {};std::int32_t value=0;auto result=std::from_chars(f->second.data(),f->second.data()+f->second.size(),value);need(result.ec==std::errc{}&&result.ptr==f->second.data()+f->second.size(),"Unsupported selected user INI GetInt lexical branch");return value;};
    auto seed=integer("GlobalUnitPlacementSeed"),threaded=integer("UnitRenderMultiThreaded");
    // Source831F60 lookup false ->831FB9 writes caller-supplied literal100,
    // followed by Float32 percent multiplication4C60F0/4C60BD.
    const float s=scale.value_or(100.f)*.01f,m=mult.value_or(100.f)*.01f;
    need(std::isfinite(s)&&s>0&&std::isfinite(m)&&m>0,"Unsupported unit caller scale/multiplier");return {s,m,!scale,!mult,seed.value_or(-1),threaded.value_or(1),!seed,!threaded};
}
} // namespace civ5::graphics::unit_art_scale_detail

