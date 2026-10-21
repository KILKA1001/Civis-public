#include <algorithm>
#include "civ5/ui/bnw_civilopedia_initial_content.hpp"
#include "civ5/ui/bnw_civilopedia_scrollbar_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <iostream>
#include <set>
using namespace civ5;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F fn,const char *why){bool rejected=false;try{fn();}catch(const std::exception&){rejected=true;}check(rejected,why);}
int main(int argc,char **argv){
 check(argc==2,"Requires explicit shared assets root; tests use source bytes, not an active gameplay epoch");
 const auto root=std::filesystem::path(argv[1]);
 auto loose=std::make_shared<assets::LooseResourceIndex>(root);loose->add_file("Assets/UI/Fonts/Tw Cent MT/TwCenMT20.ggxml");
 assets::ResourceMountTable resources(assets::resource_lookup(loose),assets::resource_lookup_all(loose),assets::resource_keys(loose));
 const auto xml_path=root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml";
 const auto identity=runtime::identify_file(xml_path);
 const runtime::BnwStockLuaClientExtent client{"source-test-client",7,840,620};
 const auto owner=ui::BnwCivilopediaInitialContentOwner::create(root,resources,client,9,identity.sha256);
 const auto &frame=owner->render_snapshot();
 check(frame.xml_sha256==identity.sha256&&frame.font_sha256.size()==64&&frame.font_key=="Assets/UI/Fonts/Tw Cent MT/TwCenMT20.ggxml"&&frame.client.revision==7&&frame.shell_revision==9&&!frame.native_internal_size_ready&&frame.blocker.find("native child traversal")!=std::string::npos,"Source font/content receipt incorrectly authorizes native internal size");
 check(frame.declared_descendants.size()==303&&frame.explicit_hidden_count==1&&frame.unknown_hidden_count>100&&frame.literal_lines.size()==2,"Declared inventory/default visibility was replaced with instantiated/visible controls");
 std::set<std::string> paths;bool hidden=false,unknown=false,padding=false,mixed_side=false;
 for(const auto &item:frame.declared_descendants){check(paths.insert(item.path).second,"Anonymous sibling source path is ambiguous");hidden|=item.explicit_hidden&&*item.explicit_hidden;unknown|=!item.explicit_hidden;padding|=item.id=="WideStack"&&item.padding=="12"&&item.stack_growth=="Bottom";mixed_side|=item.anchor_side=="I.O";}
 check(hidden&&unknown&&padding&&mixed_side,"Declared Hidden/Stack/anchor-side dependencies lost");
 const auto *title=owner->line(client,9,"ArticleID");const auto *subtitle=owner->line(client,9,"SubtitleLabel");
 check(title&&subtitle&&title==frame.line("ArticleID")&&subtitle==frame.line("SubtitleLabel")&&title->text=="title goes here"&&subtitle->text=="Subtitle goes here"&&title->image_width==500&&title->image_height==64&&title->glyph_line.glyphs.size()==static_cast<std::size_t>(std::count_if(title->text.begin(),title->text.end(),[](char c){return c!=' ';}))&&title->line_x==(500-title->glyph_line.advance_width)/2.0&&title->line_y==20&&title->baseline==38&&frame.font_style_layers==std::vector<std::string>{"TwCenMT20Base","TwCenMT20Shadow","TwCenMT20Soft"},"Getter and owned renderer glyph line/image-local geometry differ");
 auto stale=client;++stale.revision;check(!owner->line(stale,9,"ArticleID")&&!owner->line(client,10,"ArticleID")&&!owner->line(client,9,"")&&!owner->line(client,9,"Other"),"Unknown/stale glyph owner binding accepted");
 assets::ResourceMountTable missing(assets::resource_lookup(std::make_shared<assets::LooseResourceIndex>(root)));
 rejects([&]{(void)ui::BnwCivilopediaInitialContentOwner::create(root,missing,client,9,identity.sha256);},"Missing font VFS source silently used filesystem fallback");
 rejects([&]{(void)ui::BnwCivilopediaInitialContentOwner::create(root,resources,client,8,identity.sha256);},"Different initial shell revision accepted");
 rejects([&]{(void)ui::BnwCivilopediaInitialContentOwner::create(root,resources,client,9,std::string(64,'0'));},"Changed source identity accepted");
 const auto xml=content::read_xml(xml_path);auto font=ui::read_glyphgen(resources,"TwCenMT20.ggxml");auto geometry=ui::glyphgen_geometry(font);
 font.styles.erase(std::remove_if(font.styles.begin(),font.styles.end(),[](const auto &s){return s.name=="SoftShadow";}),font.styles.end());
 rejects([&]{(void)ui::project_bnw_civilopedia_initial_literal_lines(xml,font,geometry);},"Missing authored font style guessed");
 font=ui::read_glyphgen(resources,"TwCenMT20.ggxml");geometry.glyphs.erase(std::remove_if(geometry.glyphs.begin(),geometry.glyphs.end(),[](const auto &g){return g.code_point=='t';}),geometry.glyphs.end());
 rejects([&]{(void)ui::project_bnw_civilopedia_initial_literal_lines(xml,font,geometry);},"Missing literal glyph silently substituted");
 // Source-only shell test, deliberately not an active Lua epoch. Content is
 // sealed at revision 9; any subsequent shell mutation invalidates the SAME
 // renderer/getter pointer rather than retaining a stale nested render frame.
 loose->add_file("Assets/UI/Styles.xml");auto shell=ui::BnwCivilopediaScrollBarOwner::create(root,resources,client,identity.sha256);
 for(unsigned i=0;i<9;++i)shell->set_size(client,"ScrollPanel",1002,494);
 shell->create_initial_content(root,resources,client);const auto *owned_line=shell->initial_literal_line(client,"ArticleID");check(owned_line&&owned_line==shell->render_snapshot().initial_content->render_snapshot().line("ArticleID"),"Retained shell getter/render pointers differ");
 auto fork=shell->fork_for_frame(client);fork->set_offset(client,"ScrollBar",0,18);check(!fork->initial_literal_line(client,"ArticleID")&&!fork->render_snapshot().initial_content&&shell->initial_literal_line(client,"ArticleID")==owned_line&&shell->render_snapshot().control_revision==9,"Detached geometry change retained stale renderer content or mutated original owner");
 std::cout<<"PASS: actual source font "<<frame.font_sha256<<"; declared descendants="<<frame.declared_descendants.size()<<" unknown Hidden="<<frame.unknown_hidden_count<<"; literal advances="<<title->glyph_line.advance_width<<","<<subtitle->glyph_line.advance_width<<"; native internal-size/gameplay readiness false\n";
}