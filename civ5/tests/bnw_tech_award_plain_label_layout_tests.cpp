#include "civ5/ui/bnw_tech_award_plain_label_layout.hpp"
#include <iostream>
#include <charconv>
#include <algorithm>
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
}
int main(int argc,char**argv) {
 try {
  if(argc!=3){std::cerr<<"UNAVAILABLE: actual selected TwCenMT20.ggxml and asset root required\n";return 2;}
  const auto actual=civ5::ui::read_glyphgen(std::filesystem::path(argv[1]));
  const auto geometry=civ5::ui::glyphgen_geometry(actual);
  const auto one=civ5::ui::project_bnw_tech_award_plain_label(geometry,"A",-1,-4);
  const auto space=civ5::ui::project_bnw_tech_award_plain_label(geometry," ",-1,-4);
  need(one.width>0&&space.width>0,"Genuine selected alphabet/space unavailable");
  const auto exact=2*one.width+space.width;
  // Important native distinction: equal whole-word boundary wraps, although
  // the current glyph's direct overflow comparison itself is strictly >.
  const auto wrapped=civ5::ui::project_bnw_tech_award_plain_label(geometry,"A A",exact,-4);
  need(wrapped.lines.size()==2&&wrapped.lines[0].byte_end==2&&wrapped.lines[1].byte_begin==2,
       "Actual native equality word boundary was replaced by generic wrap");
  need(wrapped.width==one.width+space.width&&wrapped.height==one.height+wrapped.line_step,
       "Actual wrap has wrong retained spaces or native line-step geometry");
  const auto unwrapped=civ5::ui::project_bnw_tech_award_plain_label(geometry,"A A",exact+1,-4);
  need(unwrapped.lines.size()==1&&unwrapped.width==exact&&unwrapped.height==one.height,
       "Native no-break path differs just above exact word boundary");
  const auto explicit_line=civ5::ui::project_bnw_tech_award_plain_label(geometry,"A[NEWLINE]A",500,-4);
  need(explicit_line.lines.size()==2&&explicit_line.lines[0].byte_end==1&&explicit_line.lines[1].byte_begin==10&&
       explicit_line.width==one.width&&explicit_line.height==one.height+explicit_line.line_step,
       "Actual source NEWLINE marker lost byte ranges or real line-step effect");
  const auto utf8=civ5::ui::project_bnw_tech_award_plain_label(geometry,"A\xc3\xa9 A",500,-4);
  const auto independent=civ5::ui::project_glyphgen_line(geometry,actual.glyph(233)?"A\xc3\xa9 A":"A  A");
  need(utf8.lines.size()==1&&utf8.lines[0].byte_end==5&&
       utf8.width==independent.advance_width&&utf8.lines[0].glyphs.glyphs.size()==independent.glyphs.size(),
       "UTF8 bytes were counted as separate glyphs or missed actual SPACE fallback");
  bool bad_utf8=false;
  try{(void)civ5::ui::project_bnw_tech_award_plain_label(geometry,std::string("A\xc3",2),500,-4);}
  catch(const std::runtime_error&){bad_utf8=true;}
  need(bad_utf8,"Malformed UTF8 input was silently repaired");
  bool refused=false;
  try{(void)civ5::ui::project_bnw_tech_award_plain_label(geometry,"[ICON_FOOD]",500,-4);}
  catch(const std::runtime_error&){refused=true;}
  need(refused,"Plain Label geometry silently replaced actual markup/icon owner");
  // Independent actual XML mapping, rather than a fabricated glyph or code5.
  const auto root=std::filesystem::path(argv[2]);
  const auto mapping=civ5::content::read_xml(root/"Assets/Gameplay/XML/GameInfo/CIV5IconFontMapping.xml");
  const auto* mappings=mapping.child("IconFontMapping");need(mappings,"Actual IconFontMapping source absent");
  std::uint32_t code=0;std::size_t rows=0;
  for(const auto& row:mappings->children){const auto* name=row.child("IconName");const auto* value=row.child("IconMapping");
   if(name&&name->text=="ICON_FOOD"){need(value,"Actual icon mapping integer absent");const auto parsed=std::from_chars(value->text.data(),value->text.data()+value->text.size(),code);need(parsed.ec==std::errc{}&&parsed.ptr==value->text.data()+value->text.size(),"Actual icon code invalid");++rows;}
  }
  need(rows==1&&code>0,"Actual icon mapping absent/duplicate");
  const auto icon_path=root/"Assets/UI/Fonts/Font Icons/FontIcons.ggxml";
  const auto icon_font=std::make_shared<const civ5::ui::GlyphGenFont>(civ5::ui::read_glyphgen(icon_path));
  const auto icon_geometry=std::make_shared<const civ5::ui::GlyphGenGeometry>(civ5::ui::glyphgen_geometry(*icon_font));
  const auto* actual_icon=icon_font->glyph(code);need(actual_icon,"Actual icon font lacks source-mapped glyph");
  const auto icon_metric=std::find_if(icon_geometry->glyphs.begin(),icon_geometry->glyphs.end(),[&](const auto& g){return g.code_point==code;});
  need(icon_metric!=icon_geometry->glyphs.end(),"Actual source glyph geometry absent");
  civ5::ui::BnwTechAwardMarkupSources sources;
  sources.icons.emplace("ICON_FOOD",civ5::ui::BnwTechAwardMarkupIcon{"FontIcons",civ5::runtime::identify_file(icon_path).sha256,icon_font,icon_geometry,nullptr,{},{},code});
  const auto mixed=civ5::ui::project_bnw_tech_award_rich_label(geometry,"A[ICON_FOOD]A",-1,-4,sources);
  need(mixed.width==2*one.width+icon_metric->advance()&&mixed.lines.size()==1&&mixed.lines[0].styled_glyphs.size()==3,
       "Mixed real font glyphs lost width or genuine selected icon run");
  const auto& icon=mixed.lines[0].styled_glyphs[1];
  need(icon.icon&&icon.glyph.code_point==code&&icon.glyph.atlas_x==icon_metric->atlas_x&&
       icon.glyph.left_from_line_origin==one.width+icon_metric->origin_x,
       "Icon was substituted with text glyph or guessed atlas position");
  const auto colors=civ5::ui::project_bnw_tech_award_rich_label(geometry,"[COLOR:20:40:60:80]A[COLOR1:1:2:3:4]A[/COLOR]A[ENDCOLOR]A",-1,-4,{});
  const auto& runs=colors.lines[0].styled_glyphs;need(runs.size()==4,"Actual color tags leaked into text geometry");
  need(runs[0].color_groups[0]==std::optional{std::array<std::uint8_t,4>{20,40,60,80}}&&
       runs[1].color_groups[1]==std::optional{std::array<std::uint8_t,4>{1,2,3,4}}&&
       !runs[2].color_groups[0]&&runs[2].color_groups[1]==runs[1].color_groups[1]&&
       !runs[3].color_groups[0]&&!runs[3].color_groups[1],"Foreground stack reset incorrectly changed direct group1 or ENDCOLOR");
  const auto escaped=civ5::ui::project_bnw_tech_award_rich_label(geometry,"[[A",-1,-4,{});
  need(escaped.width==civ5::ui::project_glyphgen_line(geometry,"[A").advance_width,"Actual literal bracket escape is not a real glyph");
  std::cout<<"PASS: selected authored-font wordbreak/BMP/newline arithmetic and real selected icon/color runs; GUI/parity not measured\n";
  return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
