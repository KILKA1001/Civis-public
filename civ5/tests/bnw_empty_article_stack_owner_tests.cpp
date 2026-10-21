#include "civ5/ui/bnw_empty_article_stack_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <iostream>
using namespace civ5;
void check(bool v,const char *s){if(!v)throw std::runtime_error(s);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Unknown/nonempty native Stack dependency silently accepted");}
content::XmlNode *target(content::XmlNode &n){if(n.attribute("ID")=="ListOfArticles")return &n;for(auto &c:n.children)if(auto *v=target(c))return v;return nullptr;}
int main(int argc,char **argv){
 check(argc==2,"Requires explicit shared source root; isolated creation is not a gameplay epoch");const std::filesystem::path root(argv[1]);auto source=std::make_shared<assets::LooseResourceIndex>(root);source->add_file("Assets/UI/Styles.xml");assets::ResourceMountTable resources(assets::resource_lookup(source));
 const auto path=root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml";const auto id=runtime::identify_file(path);runtime::BnwStockLuaClientExtent client{"source-native-stack",8,841,621};auto shell=ui::BnwCivilopediaScrollBarOwner::create(root,resources,client,id.sha256);
 auto owner=ui::BnwEmptyArticleStackOwner::create(root,shell->render_snapshot());const auto &f=owner->render_local_snapshot();const auto *rect=owner->local_rectangle(client,0);
 check(rect==&f.local_rectangle&&f.xml_sha256==id.sha256&&f.own_base_init_complete&&!f.parent_scroll_init_complete&&!f.hidden&&f.source_child_count==0&&f.created_child_count==0&&rect->x==7&&rect->y==0&&rect->width==64&&rect->height==64&&f.cached_edge_x==71&&f.cached_edge_y==64,"Actual native leaf/default/cache getter/render scope differs");
 auto stale=client;++stale.revision;check(!owner->local_rectangle(stale,0)&&!owner->local_rectangle(client,1),"Stale native Stack frame accepted");auto changed=shell->render_snapshot();changed.xml_sha256=std::string(64,'0');rejects([&]{ui::BnwEmptyArticleStackOwner::create(root,changed);});
 auto xml=content::read_xml(path);auto *n=target(xml);check(n,"Actual empty Stack source unavailable");n->children.push_back(content::XmlNode{});rejects([&]{ui::inspect_bnw_empty_article_stack_init(xml);});xml=content::read_xml(path);n=target(xml);n->attributes.emplace_back("Hidden","1");rejects([&]{ui::inspect_bnw_empty_article_stack_init(xml);});xml=content::read_xml(path);n=target(xml);n->attributes.emplace_back("Style","Unknown");rejects([&]{ui::inspect_bnw_empty_article_stack_init(xml);});
 std::cout<<"PASS: actual authored empty ListOfArticles leaf created and own native base/cache initialized; unknown child/style/hidden/stale refused; enclosing panel and gameplay epoch remain incomplete\n";
}
