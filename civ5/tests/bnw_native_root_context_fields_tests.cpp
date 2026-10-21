#include "civ5/ui/detail/bnw_native_root_context_fields.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace civ5;
using namespace civ5::ui::detail;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Expected refusal");}
std::shared_ptr<const content::XmlNode> root(){auto p=std::make_shared<content::XmlNode>();p->name="Context";return p;}
BnwRootContextFieldBinding binding(std::shared_ptr<const content::XmlNode> xml,std::function<bool()> current){return {std::move(xml),"selected/CivilopediaScreen.xml",std::string(64,'a'),std::move(current)};}
void defaults(const BnwRootContextStoredFields& f){
 check(!f.authored_font&&f.style.empty(),"Context ctor default font/style differs");
 for(auto color:f.colors)check(color==std::array<float,4>{1,1,1,1},"Source Color ctor does not contain three RGBA1 values");
 check(f.color_words==std::array<std::uint32_t,3>{}&&f.color_flags==std::array<std::uint8_t,3>{},"Source Color metadata differs");
 check(!f.color_padding_known&&!f.full_native_constructor_complete&&!f.native_read_executed&&!f.native_reset_executed&&!f.child_read_context_available&&!f.font_registry_available&&!f.name_registration_complete&&!f.parent_append_complete&&!f.native_initialized&&!f.native_label_ready,"Ctor/read inventory invented child/factory/readiness");
}
}

void metadata_tests(){
 // Immutable source-derived selected33 names/hash fixtures. Whole actual XML
 // and graph admission are exercised separately by the existing graph target.
 const std::pair<const char*,std::uint32_t> names[]={{"ListItemInstance",0xe5a04aaeu},
{"ListHeadingInstance",0x5c6b0eebu},
{"GreatWorksInstance",0x8eedc3d2u},
{"PrereqTechInstance",0x236df6b0u},
{"LeadsToTechInstance",0x8fc00e3au},
{"ObsoleteTechInstance",0x22c83e6au},
{"UpgradeInstance",0x456af371u},
{"RevealTechInstance",0xeb712b83u},
{"UnlockedUnitInstance",0xdee4c31u},
{"UnlockedBuildingInstance",0xceb1669eu},
{"RequiredBuildingInstance",0xc20eac7du},
{"RevealedResourceInstance",0x9cbf113du},
{"RequiredResourceInstance",0xa866c6bfu},
{"LocalResourceInstance",0x611e423au},
{"WorkerActionInstance",0x221f958u},
{"UnlockedProjectInstance",0x22e9f91cu},
{"PromotionInstance",0x462f09cau},
{"RequiredPromotionInstance",0x7d932195u},
{"SpecialistInstance",0xbd92af5u},
{"RequiredPolicyInstance",0x8c333821u},
{"LeaderInstance",0x32d164f2u},
{"CivilizationInstance",0x2d1d088bu},
{"TraitInstance",0x3e4a038bu},
{"UniqueUnitInstance",0x4dfc56du},
{"UniqueBuildingInstance",0x1e0f499du},
{"UniqueImprovementInstance",0x5768c7eeu},
{"ReplaceInstance",0xf2fb2b73u},
{"ImprovementInstance",0xa212f270u},
{"FeatureInstance",0x8923689du},
{"TerrainInstance",0xc0037a3cu},
{"ResourceFoundInstance",0x6067dedeu},
{"FreeFormTextInstance",0x683d38b6u},
{"BBTextInstance",0xdb0991d9u}};
 auto xml=std::make_shared<content::XmlNode>();xml->name="Context";
 for(const auto& [name,hash]:names){(void)hash;content::XmlNode n;n.name="Instance";n.text="\n\t";n.attributes={{"Name",name}};content::XmlNode child;child.name="Grid";child.attributes={{"ID",name}};n.children.push_back(child);xml->children.push_back(n);}
 content::XmlNode ordinary;ordinary.name="Box";ordinary.attributes={{"ID","BackDrop"}};xml->children.push_back(ordinary);xml->children.push_back(xml->children.front());
 int callbacks=0;bool fresh=true;
 auto owner=std::make_unique<BnwNativeRootContextFields>(binding(xml,[&]{++callbacks;return fresh;}));auto ctor=owner->snapshot();rejects([&]{owner->register_instance_metadata_prefix();});check(ctor.current(),"Metadata before declaration changed constructor");owner->read_empty_declaration();auto before=owner->snapshot();owner->register_instance_metadata_prefix();auto after=owner->snapshot();
 check(after.revision()==3&&!before.current()&&after.current()&&after.fields().instance_metadata_prefix_modeled&&after.fields().next_source_cursor==33&&after.fields().instance_template_count==33&&after.fields().instance_metadata_journal.size()==33,"Selected metadata did not stop at first ordinary source child");
 for(std::size_t i=0;i<33;++i){const auto q=after.instance_template(names[i].first);check(q&&q->current()&&q->entry().hash==names[i].second&&q->entry().source_ordinal==i&&q->entry().first_child.get()==&xml->children[i].children[0]&&q->authority().identity().document==xml&&!q->native_build_instance_executed&&!q->child_read_context_available&&!q->native_label_ready,"Template quote copied tree/minted native producer or hash differs");}
 defaults(after.fields());check(!after.fields().native_build_instance_executed,"Metadata minted BuildInstance");check(!after.instance_template("missing")&&!after.instance_template("listiteminstance"),"Template lookup guessed alternate name/case");rejects([&]{after.instance_template(std::string("ListItemInstance\0foreign",24));});rejects([&]{owner->register_instance_metadata_prefix();});check(after.current(),"Metadata replay changed accepted revision");
 const auto q=*after.instance_template("ListItemInstance");const auto count=callbacks;bool rejected=false;std::thread thread([&]{check(!q.current(),"Foreign template quote current");try{owner->register_instance_metadata_prefix();}catch(...){rejected=true;}});thread.join();check(rejected&&callbacks==count&&q.current(),"Foreign metadata call reached source callbacks/poisoned owner");fresh=false;check(!q.current(),"Changed source still authorized template");fresh=true;check(!q.current(),"Source failure did not latch");
 // Independent actual CRC collision, not a duplicate-name approximation.
 auto collided=std::make_shared<content::XmlNode>();collided->name="Context";
 for(const auto name:{"T5a9494614533f10d","T938a74445548fcbf","Empty"}){content::XmlNode n;n.name="Instance";n.attributes={{"Name",name}};if(std::string(name)!="Empty"){content::XmlNode child;child.name=std::string(name);n.children.push_back(child);}collided->children.push_back(n);}collided->children.push_back(ordinary);
 BnwNativeRootContextFields collision(binding(collided,[]{return true;}));collision.read_empty_declaration();collision.register_instance_metadata_prefix();auto cp=collision.snapshot();const auto first=cp.instance_template("T5a9494614533f10d"),second=cp.instance_template("T938a74445548fcbf");check(first&&second&&first->entry().hash==0xc19129bfu&&first->entry().name=="T938a74445548fcbf"&&first->entry().first_child.get()==&collided->children[1].children[0]&&second->entry().first_child==first->entry().first_child&&cp.fields().instance_template_count==1&&cp.fields().next_source_cursor==3&&cp.fields().instance_metadata_journal[2].skipped_no_first_child&&!cp.instance_template("Empty"),"Source last-same-hash/empty-child skip semantics differ");
 for(int malformed=0;malformed<7;++malformed){auto invalid=std::make_shared<content::XmlNode>(*xml);auto& node=invalid->children[0];if(malformed==0)node.attributes.clear();if(malformed==1)node.attributes.push_back({"Name","other"});if(malformed==2)node.attributes.push_back({"Flag","1"});if(malformed==3)node.attributes[0].second=std::string("bad\0name",8);if(malformed==4)node.text="non-scalar";if(malformed==5)node.children[0].name.clear();if(malformed==6)node.attributes[0].second="";BnwNativeRootContextFields refused(binding(invalid,[]{return true;}));refused.read_empty_declaration();auto prior=refused.snapshot();rejects([&]{refused.register_instance_metadata_prefix();});check(prior.current()&&prior.revision()==2&&!prior.fields().instance_metadata_prefix_modeled,"Malformed metadata executed unowned factory fallback or changed receiver");}
 for(bool query:{false,true}){std::function<void()> callback;BnwNativeRootContextFields nested(binding(xml,[&]{const auto copy=callback;if(copy)copy();return true;}));nested.read_empty_declaration();if(query)nested.register_instance_metadata_prefix();auto prior=nested.snapshot();bool caught=false;callback=[&]{try{nested.snapshot();}catch(...){caught=true;}};if(query)rejects([&]{prior.instance_template("ListItemInstance");});else rejects([&]{nested.register_instance_metadata_prefix();});callback={};check(caught&&!prior.current(),"Caught metadata/query recursion kept outer authorization");}
 int n=0;bool poison=false;BnwNativeRootContextFields uncertain(binding(xml,[&]{++n;if(poison&&n==2)throw std::bad_alloc();return true;}));uncertain.read_empty_declaration();auto preceding=uncertain.snapshot();n=0;poison=true;rejects([&]{uncertain.register_instance_metadata_prefix();});poison=false;check(!preceding.current(),"Post-write failure retained preceding template/receiver");rejects([&]{uncertain.snapshot();});
 auto teardown=std::make_unique<BnwNativeRootContextFields>(binding(xml,[]{return true;}));teardown->read_empty_declaration();teardown->register_instance_metadata_prefix();auto stored=*teardown->snapshot().instance_template("ListItemInstance");teardown.reset();check(!stored.current()&&stored.entry().first_child.get()==&xml->children[0].children[0],"Template alias immortalized mutable component");
 std::function<void()> release;auto disappearing=std::make_unique<BnwNativeRootContextFields>(binding(xml,[&]{const auto copy=release;if(copy)copy();return true;}));disappearing->read_empty_declaration();auto old=disappearing->snapshot();auto borrowed=disappearing.get();release=[&]{release={};disappearing.reset();};rejects([&]{borrowed->register_instance_metadata_prefix();});check(!old.current()&&!disappearing,"Metadata callback facade release retained authority");
}

int main(){
 auto xml=root();int queries=0;bool source=true;
 auto owner=std::make_unique<BnwNativeRootContextFields>(binding(xml,[&]{++queries;return source;}));
 auto ctor=owner->snapshot();defaults(ctor.fields());check(ctor.current()&&ctor.revision()==1&&!ctor.fields().empty_declaration_read_modeled&&ctor.identity().document==xml&&ctor.identity().instance.get()!=xml.get(),"Constructor identity/revision aliases source document");
 owner->read_empty_declaration();auto read=owner->snapshot();defaults(read.fields());check(read.current()&&!ctor.current()&&read.revision()==2&&read.fields().empty_declaration_read_modeled&&read.identity().instance==ctor.identity().instance,"Empty declaration read cloned receiver or retained old revision");
 rejects([&]{owner->read_empty_declaration();});check(read.current(),"Replay refusal changed current occurrence");
 auto foreign_queries=queries;bool refused=false;std::thread foreign([&]{check(!read.current(),"Foreign snapshot current");try{owner->read_empty_declaration();}catch(...){refused=true;}});foreign.join();check(refused&&queries==foreign_queries&&read.current(),"Foreign call touched callbacks/poisoned creating thread");
 owner->retire_for_graph_reset();check(!read.current()&&read.fields().empty_declaration_read_modeled&&!read.fields().native_reset_executed,"Civis reset retirement replayed constructor/native reset");rejects([&]{owner->snapshot();});
 auto replacement=std::make_unique<BnwNativeRootContextFields>(binding(xml,[]{return true;}));auto new_ctor=replacement->snapshot();check(new_ctor.identity().instance!=read.identity().instance&&!new_ctor.fields().empty_declaration_read_modeled,"New component reused retired occurrence");
 for(bool colors:{false,true}){
  auto declared=std::make_shared<content::XmlNode>();declared->name="Context";if(colors){content::XmlNode child;child.name="ColorSet";declared->children.push_back(child);}else declared->attributes.push_back({"Font","TwCenMT20"});
  BnwNativeRootContextFields unsupported(binding(declared,[]{return true;}));auto before=unsupported.snapshot();rejects([&]{unsupported.read_empty_declaration();});check(before.current()&&!before.fields().empty_declaration_read_modeled,"Unowned declaration mutated receiver");
 }
 // Root attribute presence includes an empty value; no registry is guessed.
 auto empty_attr=std::make_shared<content::XmlNode>();empty_attr->name="Context";empty_attr->attributes.push_back({"FontStyle",""});BnwNativeRootContextFields empty_style(binding(empty_attr,[]{return true;}));rejects([&]{empty_style.read_empty_declaration();});
 bool changed=true;BnwNativeRootContextFields changed_owner(binding(xml,[&]{return changed;}));auto changed_before=changed_owner.snapshot();changed=false;rejects([&]{changed_owner.read_empty_declaration();});changed=true;check(!changed_before.current(),"Source rejection was not sticky");
 // Caught nested queries retire the outer call; the callback owns its copy.
 for(bool mutation:{false,true}){
  std::function<void()> callback;auto recursive=std::make_unique<BnwNativeRootContextFields>(binding(xml,[&]{const auto owned=callback;if(owned)owned();return true;}));auto before=recursive->snapshot();bool nested=false;
  callback=[&]{try{recursive->snapshot();}catch(...){nested=true;}};if(mutation)rejects([&]{recursive->read_empty_declaration();});else rejects([&]{recursive->snapshot();});callback={};check(nested&&!before.current(),"Caught reentry retained outer authorization");
 }
 // Fail final post-write callback: no unpublished current result survives.
 int reads=0;bool poison=false;BnwNativeRootContextFields throwing(binding(xml,[&]{++reads;if(poison&&reads==2)throw std::bad_alloc();return true;}));auto preceding=throwing.snapshot();reads=0;poison=true;rejects([&]{throwing.read_empty_declaration();});poison=false;check(!preceding.current(),"Post-mutation throw retained preceding occurrence");rejects([&]{throwing.snapshot();});
 // Source records/identity tokens retained by quotes do not retain authority.
 auto teardown=std::make_unique<BnwNativeRootContextFields>(binding(xml,[]{return true;}));auto retained=teardown->snapshot();teardown.reset();check(!retained.current()&&retained.identity().document==xml&&retained.identity().instance,"Snapshot immortalized mutable owner");
 std::function<void()> release;auto disappearing=std::make_unique<BnwNativeRootContextFields>(binding(xml,[&]{const auto owned=release;if(owned)owned();return true;}));auto before_release=disappearing->snapshot();auto borrowed=disappearing.get();release=[&]{release={};disappearing.reset();};rejects([&]{borrowed->snapshot();});check(!disappearing&&!before_release.current(),"Final owner callback release retained authority");
 // A current quote can also trigger final-owner release safely.
 auto on_current=std::make_unique<BnwNativeRootContextFields>(binding(xml,[&]{const auto owned=release;if(owned)owned();return true;}));auto quote=on_current->snapshot();release=[&]{release={};on_current.reset();};check(!quote.current()&&!on_current,"Current callback dereferenced destroyed component");
 metadata_tests();
 std::cout<<"PASS: ROOT Context ctor fields/empty declaration, SAME revision, unsupported declarations, thread/reentry/source/post-write failure/retirement/final-owner lifetime; child context/native Read/reset/init unavailable\n";
}
