#include "civ5/ui/bnw_tech_award_plain_label_layout.hpp"
#include <algorithm>
#include <cmath>
#include <charconv>
#include <climits>
#include <stdexcept>
namespace civ5::ui {
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
using Groups=std::array<std::optional<std::array<std::uint8_t,4>>,3>;
struct Unit {std::size_t begin,end;std::uint32_t code;bool newline;std::int32_t advance;Groups groups;std::optional<BnwTechAwardMarkupIcon> icon;};
std::array<std::uint8_t,4> numeric_color(std::string_view source){std::array<std::uint8_t,4> result{};for(std::size_t i=0;i<4;++i){const auto split=source.find(':');need((i==3)==(split==source.npos),"TechAward numeric color operand count invalid");const auto part=source.substr(0,split);int n=0;const auto p=std::from_chars(part.data(),part.data()+part.size(),n);need(p.ec==std::errc{}&&p.ptr==part.data()+part.size()&&n>=0&&n<=255,"TechAward numeric color outside exact byte profile");result[i]=static_cast<std::uint8_t>(n);if(split!=source.npos)source.remove_prefix(split+1);}return result;}
std::uint32_t decode(std::string_view text,std::size_t& cursor){
 need(cursor<text.size(),"TechAward UTF8 cursor exhausted");const auto first=static_cast<unsigned char>(text[cursor++]);
 if(first<0x80)return first;
 std::uint32_t code=0;std::size_t count=0;unsigned char second_min=0x80,second_max=0xbf;
 if(first>=0xc2&&first<=0xdf){count=1;code=first&0x1f;}
 else if(first>=0xe0&&first<=0xef){count=2;code=first&0x0f;if(first==0xe0)second_min=0xa0;if(first==0xed)second_max=0x9f;}
 else throw std::runtime_error("TechAward malformed/non-BMP UTF8 requires separate producer");
 need(cursor+count<=text.size(),"TechAward truncated UTF8");
 for(std::size_t i=0;i<count;++i){const auto byte=static_cast<unsigned char>(text[cursor++]);need(byte>=(i==0?second_min:0x80)&&byte<=(i==0?second_max:0xbf),"TechAward malformed UTF8 continuation");code=(code<<6)|(byte&0x3f);}
 return code;
}
std::string encode(std::uint32_t code){
 std::string text;if(code<0x80)text.push_back(static_cast<char>(code));
 else if(code<0x800){text.push_back(static_cast<char>(0xc0|(code>>6)));text.push_back(static_cast<char>(0x80|(code&0x3f)));}
 else {text.push_back(static_cast<char>(0xe0|(code>>12)));text.push_back(static_cast<char>(0x80|((code>>6)&0x3f)));text.push_back(static_cast<char>(0x80|(code&0x3f)));}return text;
}
}
BnwTechAwardPlainLabelLayout project_bnw_tech_award_rich_label(
 const GlyphGenGeometry& font,std::string_view source,std::int32_t wrap,std::int32_t leading,
 const BnwTechAwardMarkupSources& sources) {
 const std::string text(source);
 need(!text.empty()&&text.size()<4000,"TechAward Label requires complete bounded nonempty text");
 need(wrap==-1||(wrap>0&&wrap<32768),"TechAward native signed wrap operand unsupported");
 need(leading>-32768&&leading<32768,"TechAward native leading operand overflow");
 need(font.advance_adjustment==0&&font.ascent>0&&font.descent>=0,"TechAward authored glyph metrics require exact ordinary font source");
 std::vector<Unit> units;Groups groups{};std::vector<std::optional<std::array<std::uint8_t,4>>> stack{std::nullopt};
 for(std::size_t cursor=0;cursor<text.size();){
  const auto begin=cursor;
  bool escaped=false;
  if(text[cursor]=='['){
   if(cursor+1<text.size()&&text[cursor+1]=='['){++cursor;escaped=true;}
   else {
    const auto close=text.find(']',cursor+1);need(close!=text.npos,"TechAward unterminated actual markup");const auto tag=std::string_view(text).substr(cursor+1,close-cursor-1);cursor=close+1;
    if(tag=="NEWLINE"){units.push_back({begin,cursor,0,true,0,groups,std::nullopt});continue;}
    if(tag=="ENDCOLOR"){groups={};continue;}
    if(tag=="/COLOR"){if(stack.size()>1)stack.pop_back();groups[0]=stack.back();continue;}
    if(tag.starts_with("COLOR:")){const auto color=numeric_color(tag.substr(6));stack.push_back(color);groups[0]=color;continue;}
    if(tag.size()>7&&tag.starts_with("COLOR")&&tag[5]>='0'&&tag[5]<='2'&&tag[6]==':'){groups[static_cast<std::size_t>(tag[5]-'0')]=numeric_color(tag.substr(7));continue;}
    if(tag.starts_with("COLOR_")){const auto found=sources.colors.find(std::string(tag));need(found!=sources.colors.end(),"TechAward actual selected named color unavailable");stack.push_back(found->second);groups[0]=found->second;continue;}
    if(tag.starts_with("ICON_")){const auto found=sources.icons.find(std::string(tag));need(found!=sources.icons.end()&&found->second.font&&found->second.geometry,"TechAward actual selected icon font unavailable");const auto& icon=found->second;const auto& geometry=*icon.geometry;
     need(geometry.advance_adjustment==0,"TechAward actual icon font fudgeadv unsupported");const auto glyph=std::find_if(geometry.glyphs.begin(),geometry.glyphs.end(),[&](const auto& g){return g.code_point==icon.code_point;});need(glyph!=geometry.glyphs.end()&&glyph->advance()>=0&&glyph->advance()<32768,"TechAward actual icon glyph unavailable/overflow");
     units.push_back({begin,cursor,icon.code_point,false,static_cast<std::int32_t>(glyph->advance()),groups,icon});continue;
    }
    throw std::runtime_error("TechAward unsupported reached markup token");
   }
  }
  const auto code=decode(text,cursor);need(code>=32&&(code!='['||escaped),"TechAward unsupported actual text control");
  auto glyph=std::find_if(font.glyphs.begin(),font.glyphs.end(),[&](const auto& g){return g.code_point==code;});
  // Actual 5077F0/7505D0: HasGlyph miss selects SPACE before GetGlyph.
  // The selected ordinary font's SPACE must really exist; first-map fallback
  // is still refused when that source is unavailable.
  if(glyph==font.glyphs.end())glyph=std::find_if(font.glyphs.begin(),font.glyphs.end(),[](const auto& g){return g.code_point==32;});
  need(glyph!=font.glyphs.end(),"TechAward actual SPACE fallback glyph unavailable");const auto advance=glyph->advance();
  need(advance>=0&&advance<32768,"TechAward native glyph advance outside bounded profile");
  units.push_back({begin,cursor,glyph->code_point,false,static_cast<std::int32_t>(advance),groups,std::nullopt});
 }
 const auto base_height=std::int64_t{font.ascent}-font.ascent_adjustment;
 need(base_height>0&&base_height<32768,"TechAward native adjusted ascent unsupported");
 const auto unadjusted=std::int64_t{font.ascent}+font.descent;
 need(unadjusted>0&&unadjusted<32768,"TechAward native line-height overflow");
 const float line_float=static_cast<float>(unadjusted)*1.2000000476837158f;
 const auto step=static_cast<std::int32_t>(line_float)+leading;need(step>0&&step<32768,"TechAward native line step unsupported");
 BnwTechAwardPlainLabelLayout result;result.line_step=step;std::size_t begin=0;
 std::int32_t cursor=0,baseline=0;bool trim_after_break=false;GlyphGenLine projected;std::vector<BnwTechAwardStyledGlyph> styled;
 const auto emit=[&](std::size_t end){BnwTechAwardPlainLabelLine line;line.byte_begin=begin;line.byte_end=end;line.width=cursor;line.baseline_offset=baseline;line.glyphs=projected;line.glyphs.advance_width=cursor;line.styled_glyphs=styled;result.width=std::max(result.width,cursor);result.lines.push_back(std::move(line));};
 const auto new_line=[&](){need(baseline<=32767-step,"TechAward native multiline height overflow");baseline+=step;cursor=0;projected={};styled.clear();};
 for(std::size_t i=0;i<units.size();++i){const auto& unit=units[i];
  if(unit.newline){emit(unit.begin);begin=unit.end;new_line();trim_after_break=false;continue;}
  bool line_break=false;
  if(wrap!=-1){
   line_break=static_cast<float>(wrap)<static_cast<float>(cursor)+static_cast<float>(unit.advance);
   if(!line_break&&(!unit.icon&&unit.code!=32)&&(i==0||units[i-1].newline||(!units[i-1].icon&&units[i-1].code==32))){
    float word=0,total=static_cast<float>(cursor);
    for(std::size_t j=i;j<units.size()&&!units[j].newline&&(units[j].icon||units[j].code!=32);++j){word=word+static_cast<float>(units[j].advance);total=total+static_cast<float>(units[j].advance);need(word<32768&&total<32768,"TechAward native word lookahead overflow");}
    line_break=static_cast<std::int32_t>(word)<wrap&&static_cast<std::int32_t>(total)>=wrap;
   }
  }
  if(line_break){emit(unit.begin);begin=unit.begin;new_line();trim_after_break=true;}
  if(trim_after_break&&!unit.icon&&unit.code==32){begin=unit.end;continue;}trim_after_break=false;
  need(cursor<=32767-unit.advance,"TechAward native Label width overflow");
  if(unit.icon){const auto& geometry=*unit.icon->geometry;const auto found=std::find_if(geometry.glyphs.begin(),geometry.glyphs.end(),[&](const auto& g){return g.code_point==unit.code;});need(found!=geometry.glyphs.end(),"TechAward retained icon glyph changed");const auto& glyph=*found;
   need(std::int64_t{cursor}+glyph.origin_x>=INT32_MIN&&std::int64_t{cursor}+glyph.origin_x<=INT32_MAX,"TechAward icon projection overflow");
   if(glyph.width>0&&glyph.height>0)styled.push_back({{glyph.code_point,glyph.atlas_x,glyph.atlas_y,glyph.width,glyph.height,cursor+glyph.origin_x,glyph.origin_y},unit.groups,unit.icon});
  }else {auto glyphs=project_glyphgen_line(font,encode(unit.code));need(glyphs.advance_width==unit.advance,"TechAward selected glyph projection differs from native advance");
   for(auto& glyph:glyphs.glyphs){glyph.left_from_line_origin+=cursor;projected.glyphs.push_back(glyph);styled.push_back({glyph,unit.groups,std::nullopt});}
  }
  cursor+=unit.advance;
 }
 emit(text.size());need(std::int64_t{baseline}+base_height<32768,"TechAward native Label height overflow");result.height=static_cast<std::int32_t>(std::int64_t{baseline}+base_height);return result;
}
BnwTechAwardPlainLabelLayout project_bnw_tech_award_plain_label(const GlyphGenGeometry& font,std::string_view text,std::int32_t wrap,std::int32_t leading){return project_bnw_tech_award_rich_label(font,text,wrap,leading,{});}
}
