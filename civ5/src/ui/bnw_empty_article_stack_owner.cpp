#include "civ5/ui/bnw_empty_article_stack_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <set>
namespace civ5::ui {
namespace {
void need(bool v,const char *s){if(!v)throw std::runtime_error(s);}
void find(const content::XmlNode &n,std::vector<const content::XmlNode*> &path,std::vector<std::vector<const content::XmlNode*>> &matches){if(n.name=="Instance")return;path.push_back(&n);if(n.attribute("ID")=="ListOfArticles")matches.push_back(path);for(const auto &c:n.children)find(c,path,matches);path.pop_back();}
bool same(const runtime::BnwStockLuaClientExtent &a,const runtime::BnwStockLuaClientExtent &b){return a.surface_owner==b.surface_owner&&a.revision==b.revision&&a.width==b.width&&a.height==b.height;}
}
BnwEmptyArticleStackFrame inspect_bnw_empty_article_stack_init(const content::XmlNode &xml){
 std::vector<const content::XmlNode*> path;std::vector<std::vector<const content::XmlNode*>> matches;find(xml,path,matches);
 need(matches.size()==1&&matches[0].size()==5,"Empty native ListOfArticles source path ambiguous");const auto &p=matches[0];const auto &n=*p.back();
 need(p[0]->name=="Context"&&p[1]->name=="Box"&&p[2]->name=="Box"&&p[3]->name=="ScrollPanel"&&p[3]->attribute("ID")=="LeftScrollPanel"&&p[3]->attribute("Vertical")=="1"&&n.name=="Stack","Empty native Stack parent/class differs");
 std::set<std::string> seen;constexpr std::array<std::string_view,6> allowed{"Anchor","Offset","Size","StackGrowth","Padding","ID"};
 for(const auto &[a,v]:n.attributes){(void)v;need(seen.insert(a).second&&std::find(allowed.begin(),allowed.end(),a)!=allowed.end(),"Empty native Stack has unowned style/flags/dependency");}
 need(n.attribute("Anchor")=="L,T"&&n.attribute("Offset")=="7,0"&&n.attribute("Size")=="64,64"&&n.attribute("StackGrowth")=="Bottom"&&n.attribute("Padding")=="0"&&n.children.empty()&&n.text.empty(),"ListOfArticles is not the verified authored empty Stack instance");
 // Actual creation owns an empty child vector, explicit nominal extent/offset,
 // default Hidden=false. Native Stack.anchor 0x005895E0 has growth=(0,1),
 // positive size -> neither negative start adjustment nor wrap branch occurs.
 // ControlBase.initialize 0x005761E0 anchors L,T, records abs(offset)+size
 // cached edges, sets own bounds and visits ZERO actual created children.
 BnwEmptyArticleStackFrame f;f.local_rectangle={7,0,64,64};f.cached_edge_x=71;f.cached_edge_y=64;f.own_base_init_complete=true;return f;
}
std::unique_ptr<BnwEmptyArticleStackOwner> BnwEmptyArticleStackOwner::create(const std::filesystem::path &root,const BnwScrollBarLayoutFrame &shell){
 need(shell.control_revision==0&&!shell.xml_sha256.empty()&&!shell.client.surface_owner.empty()&&shell.control("LeftScrollPanel"),"Empty Stack requires original actual retained shell");
 const auto path=root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml";const auto before=runtime::identify_file(path);need(before.sha256==shell.xml_sha256,"Empty Stack source changed since shell creation");const auto xml=content::read_xml(path);need(runtime::identify_file(path).sha256==before.sha256,"Empty Stack source changed during creation");
 auto frame=inspect_bnw_empty_article_stack_init(xml);frame.client=shell.client;frame.shell_revision=shell.control_revision;frame.xml_sha256=before.sha256;
 return std::unique_ptr<BnwEmptyArticleStackOwner>(new BnwEmptyArticleStackOwner(std::move(frame)));
}
const BnwScrollBarLayoutRect *BnwEmptyArticleStackOwner::local_rectangle(const runtime::BnwStockLuaClientExtent &client,std::uint64_t revision)const noexcept{return same(client,frame_.client)&&revision==frame_.shell_revision?&frame_.local_rectangle:nullptr;}
}
