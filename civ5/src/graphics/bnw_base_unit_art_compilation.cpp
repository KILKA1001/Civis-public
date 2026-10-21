#include "civ5/graphics/bnw_base_unit_art_compilation.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "../ui/bnw_native_completed_unit_art_inputs.hpp"
#include "bnw_unit_art_source_columns.hpp"
#include <charconv>
#include <cmath>
#include <map>
#include <set>
namespace civ5::graphics {
namespace {
using content::DatabaseValue;
using content::XmlNode;
void require(bool value,const char* why){if(!value)throw std::invalid_argument(why);}
const XmlNode* first(const XmlNode& row,std::string_view name){for(const auto& c:row.children)if(c.name==name)return &c;return nullptr;}
std::string text(const XmlNode& node,std::size_t extent){require(node.children.empty()&&node.text.find('\0')==std::string::npos&&node.text.size()<extent,"Art string outside native buffer/text scope");return node.text;}
std::string field(const XmlNode& row,std::string_view name,std::size_t extent){auto* p=first(row,name);return p?text(*p,extent):std::string{};}
std::string_view numeric_text(const XmlNode& n){require(n.children.empty(),"Nested art numeric text unsupported");std::string_view s=n.text;while(!s.empty()&&(s.front()==' '||s.front()=='\t'||s.front()=='\r'||s.front()=='\n'))s.remove_prefix(1);if(!s.empty()&&s.front()=='+')s.remove_prefix(1);return s;}
DatabaseValue integer(const XmlNode& row,std::string_view name){auto* p=first(row,name);if(!p)return {};auto s=numeric_text(*p);std::int32_t n=0;auto r=std::from_chars(s.data(),s.data()+s.size(),n);if(r.ec!=std::errc{})return {};return std::int64_t(n);}
DatabaseValue number(const XmlNode& row,std::string_view name){auto* p=first(row,name);if(!p)return {};auto s=numeric_text(*p);float n=0;auto r=std::from_chars(s.data(),s.data()+s.size(),n);if(r.ec==std::errc::invalid_argument)return {};
 // VC90 accepts a trailing f/other nonnumeric suffix, but a started malformed
 // exponent fails extraction. Overflow can return INF in VC90: refuse that
 // unsupported finite art input instead of silently changing an effect.
 require(r.ec!=std::errc::result_out_of_range&&std::isfinite(n),"Nonfinite/range art float unsupported");
 if(r.ptr!=s.data()+s.size()&&(*r.ptr=='e'||*r.ptr=='E'))return {};
 return double(n);
}
DatabaseValue boolean(const XmlNode& row,std::string_view name){auto* p=first(row,name);if(!p)return {};require(p->children.empty(),"Nested art bool unsupported");const auto& s=p->text;if(s.empty())return {};if(s[0]=='t'||s[0]=='T')return std::int64_t(1);if(s[0]=='f'||s[0]=='F')return std::int64_t(0);auto v=integer(row,name);if(auto* n=std::get_if<std::int64_t>(&v)){if(*n!=0&&*n!=1)return {};return *n;}return {};}
void override_value(DatabaseValue& current,DatabaseValue value){if(!std::holds_alternative<std::monostate>(value))current=std::move(value);}
struct Combat {
 std::vector<DatabaseValue> values=std::vector<DatabaseValue>(36);
 std::array<std::optional<std::vector<DatabaseValue>>,12> weapons;
 Combat(){values[1]=std::string{};values[2]=std::string{};values[34]=std::string{};}
};
std::vector<DatabaseValue> new_weapon(){std::vector<DatabaseValue> w(21);for(auto i:{0,3,9,18,19})w[i]=std::string{};return w;}
void weapon(Combat& out,const XmlNode& row,std::size_t index,std::size_t sub){auto& slot=out.weapons[index*2+sub];if(!slot)slot=new_weapon();auto& w=*slot;
 if(auto* p=first(row,"ID")){auto s=text(*p,128);if(!s.empty())w[3]=std::move(s);}
 // Usage drives native action selection after this SQL stage; source binder
 // contains no Usage column. Preserve all source-indexed weapon rows.
 const int floats[]={4,5,6,7,8,10,11,12,13,20};for(int i:floats){std::string name="f"+std::string(unit_art_source::columns4[i]);override_value(w[i],number(row,name));}
 for(int i:{14,15,16,17}){std::string name="b"+std::string(unit_art_source::columns4[i]);override_value(w[i],boolean(row,name));}
 for(int i:{9,18,19})if(auto* p=first(row,unit_art_source::columns4[i]))w[i]=text(*p,i==9?128:32);
}
Combat compile_combat(const XmlNode& row,const std::map<std::string,Combat>& templates){Combat out;auto* c=first(row,"Combat");if(!c)return out;
 if(auto* p=first(*c,"Defaults")){auto name=text(*p,256);auto found=templates.find(name);require(found!=templates.end(),"Unknown art Defaults lookup/fallback is not closed");out=found->second;}
 for(int i:{1,2}){auto name=unit_art_source::columns3[i];if(auto* p=first(*c,name)){auto s=text(*p,512);auto& dest=std::get<std::string>(out.values[i]);if(!dest.empty())dest+=' ';dest+=s;require(dest.size()<512,"Inherited action string exceeds source buffer");}}
 for(std::size_t i=3;i<=20;++i){std::string name="f"+std::string(unit_art_source::columns3[i]);override_value(out.values[i],number(*c,name));}
 for(std::size_t i=21;i<=30;++i){std::string name="b"+std::string(unit_art_source::columns3[i]);override_value(out.values[i],boolean(*c,name));}
 // Source bool predecessor and later integer overwrite share the same nullable
 // ReformBeforeCombat field (733762/7347BC), in this exact order.
 override_value(out.values[27],integer(*c,"iReformBeforeCombat"));
 for(int i:{31,32}){std::string name="f"+std::string(unit_art_source::columns3[i]);override_value(out.values[i],number(*c,name));}
 override_value(out.values[33],boolean(*c,"bOnlyTurnInMovementActions"));override_value(out.values[35],boolean(*c,"bLastToDie"));
 if(auto* p=first(*c,"RushAttackFormation"))out.values[34]=text(*p,256);
 std::size_t index=0;for(const auto& w:c->children){if(w.name!="Weapon")continue;if(index==6)break;std::size_t sub=0;for(const auto& nested:w.children){if(nested.name!="Weapon")continue;if(sub==2)break;weapon(out,nested,index,sub++);}if(sub==0)weapon(out,w,index,0);++index;}
 return out;
}
std::string profile_key(const runtime::SavedContentProfile& p){require(p.can_activate&&p.all_requirements_located&&p.files_complete&&!p.native_mods_require_port,"Unit art selected profile not activatable");std::string out=std::to_string(int(p.kind));for(auto i:p.enabled_dlc)out+="D"+std::to_string(i);for(auto i:p.enabled_mods)out+="M"+std::to_string(i);for(const auto& r:p.requirements)out+="R"+std::to_string(int(r.kind))+":"+r.id+":"+std::to_string(r.wire_value)+":"+std::to_string(int(r.status));return out;}
XmlNode read(const assets::ResourceMountTable& resources,const BnwUnitMemberInfoSourceRecord& expected){auto hit=resources.resolve(assets::AssetKey(expected.request));auto* h=std::get_if<assets::AssetHandle>(&hit.asset);require(h,"Unit art source missing/ambiguous");auto b=h->read_bytes(64*1024*1024);const auto& p=h->candidate().provenance;require(runtime::identify_bytes(b).sha256==expected.sha256&&hit.mount_id==expected.mount&&p.kind==expected.provenance.kind&&p.source_id==expected.provenance.source_id&&p.source_relative_path==expected.provenance.source_relative_path&&p.archive_relative_path==expected.provenance.archive_relative_path,"SAME unit art selected source changed");return content::read_xml_metadata(std::string(b.begin(),b.end()));}
}
void BnwBaseUnitArtCompilation::append_documents(BnwBaseUnitArtCompilation& out,
    BnwBaseUnitMemberInfoCompilation& member,const std::vector<XmlNode>& documents){
 std::set<std::string> units;
 // Native43E9D0 completes UnitInfos/member links before the 6ECBC0 template
 // map/member/combat/weapons pass. SQLite owns one transaction for all stages.
 for(const auto& doc:documents)if(doc.name=="UnitArtInfos")for(const auto& row:doc.children){if(row.name!="UnitArtInfo")continue;auto type=field(row,"Type",128);require(!type.empty(),"Art unit Type absent");if(!units.insert(type).second)continue;
  out.statements_.push_back({std::string(unit_art_source::sql0),{type,integer(row,"DamageStates"),field(row,"Formation",128),field(row,"UnitFlagAtlas",128),integer(row,"UnitFlagIconOffset"),field(row,"IconAtlas",128),integer(row,"PortraitIndex")}});++out.stats_.unit_infos;
  for(const auto& link:row.children){if(link.name!="UnitMemberArt")continue;auto target=field(link,"MemberType",128);auto value=integer(link,"MemberCount");auto* n=std::get_if<std::int64_t>(&value);if(target.empty()||!n||*n<=0)continue;out.statements_.push_back({std::string(unit_art_source::sql1),{type,std::move(target),value}});++out.stats_.info_members;}
 }
 std::map<std::string,Combat> templates;
 for(const auto& doc:documents)if(doc.name=="UnitMemberTemplates")for(const auto& row:doc.children){if(row.name!="UnitMemberTemplate")continue;auto type=field(row,"Type",128);require(!type.empty(),"Art template Type absent");auto combat=compile_combat(row,templates);templates.try_emplace(std::move(type),std::move(combat));}
 std::map<std::string,const XmlNode*> member_xml;
 for(const auto& doc:documents)if(doc.name=="UnitMemberArtInfos")for(const auto& row:doc.children)if(row.name=="UnitMemberArtInfo")member_xml.try_emplace(field(row,"Type",128),&row);
 for(auto& statement:member.statements_){auto type=std::get<std::string>(statement.parameters[0]);auto found=member_xml.find(type);require(found!=member_xml.end(),"Private member source no longer matched");auto combat=compile_combat(*found->second,templates);
  out.statements_.push_back(std::move(statement));++out.stats_.member_infos;combat.values[0]=type;out.statements_.push_back({std::string(unit_art_source::sql3),std::move(combat.values)});++out.stats_.combats;
  for(std::size_t i=0;i<12;++i)if(combat.weapons[i]){auto w=std::move(*combat.weapons[i]);w[0]=type;w[1]=std::int64_t(i/2);w[2]=std::int64_t(i%2);out.statements_.push_back({std::string(unit_art_source::sql4),std::move(w)});++out.stats_.weapons;}
 }
 require(out.stats_.unit_infos&&out.stats_.member_infos&&out.stats_.combats,"Native base art stage empty");
}
BnwBaseUnitArtCompilation BnwBaseUnitArtCompilation::compile_from_completed_native_inputs(
    const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>& supplied,
    const runtime::SavedContentProfile& profile){
 const runtime::SavedContentProfile selected(profile);
 auto member=BnwBaseUnitMemberInfoCompilation::compile_from_completed_native_inputs(supplied,selected);
 const auto inputs=member.completed_inputs_;BnwBaseUnitArtCompilation out;
 out.completed_inputs_=inputs;out.completed_sha256_=member.completed_sha256_;out.profile_key_=profile_key(selected);
 std::vector<XmlNode> documents;documents.reserve(3);
 for(unsigned ordinal=0;ordinal<3;++ordinal){
  auto doc=BnwBaseUnitMemberInfoCompilation::read_completed_document(*inputs,ordinal,&out.completed_sha256_[ordinal]);
  documents.push_back(std::move(doc.xml));
 }
 append_documents(out,member,documents);
 for(unsigned ordinal=0;ordinal<3;++ordinal)
  BnwBaseUnitMemberInfoCompilation::read_completed_document(*inputs,ordinal,&out.completed_sha256_[ordinal]);
 inputs->require_same_profile(selected);return out;
}
void BnwBaseUnitArtCompilation::require_same_completed_native_inputs(
    const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>& supplied,
    const runtime::SavedContentProfile& profile)const{
 const runtime::SavedContentProfile selected(profile);const auto stored=completed_inputs_;
 const auto incoming=supplied;const auto hashes=completed_sha256_;const auto key=profile_key_;
 require(stored&&incoming&&stored.get()==incoming.get()&&!stored.owner_before(incoming)&&
         !incoming.owner_before(stored)&&key==profile_key(selected),
         "Completed base art requires actual compiled SAME native inputs/profile");
 // The private compilation already retained only the genuine canonical leaf.
 // Reject caller aliases before any real source/provider bookend.
 stored->require_same_profile(selected);
 for(unsigned ordinal=0;ordinal<3;++ordinal)
  BnwBaseUnitMemberInfoCompilation::read_completed_document(*stored,ordinal,&hashes[ordinal]);
 stored->require_same_profile(selected);
}
void BnwBaseUnitArtCompilation::apply_to_unsealed_database_from_completed_native_inputs(
    content::RulesDatabase& db,const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>& supplied,
    const runtime::SavedContentProfile& profile)const{
 const runtime::SavedContentProfile selected(profile);const auto stored=completed_inputs_;
 const auto hashes=completed_sha256_;const auto key=profile_key_;const auto statements=statements_;
 const auto incoming=supplied;
 require(stored&&incoming&&stored.get()==incoming.get()&&!stored.owner_before(incoming)&&
         !incoming.owner_before(stored)&&key==profile_key(selected),"Completed base art foreign inputs/profile");
 const auto inputs=BnwBaseUnitMemberInfoCompilation::canonical_completed_inputs(stored);
 inputs->require_same_profile(selected);
 for(unsigned ordinal=0;ordinal<3;++ordinal)
  BnwBaseUnitMemberInfoCompilation::read_completed_document(*inputs,ordinal,&hashes[ordinal]);
 inputs->require_same_profile(selected);
 for(auto table:unit_art_source::tables){std::int64_t count=-1;
  const auto result=db.query_rows({"SELECT count(*) AS n FROM "+std::string(table),{}},
      [&](const content::RulesRowView& row){count=row.integer("n");},1);
  require(result.all_succeeded()&&count==0,"Art source requires empty pre-action engine tables");
 }
 db.apply_statements(statements);
}
BnwBaseUnitArtCompilation BnwBaseUnitArtCompilation::compile(const assets::ResourceMountTable& resources,const runtime::SavedContentProfile& profile){BnwBaseUnitArtCompilation out;auto member=BnwBaseUnitMemberInfoCompilation::compile(resources,profile);out.resources_=&resources;out.profile_key_=profile_key(profile);out.sources_=member.sources_;
 std::vector<XmlNode> documents;for(const auto& s:out.sources_)documents.push_back(read(resources,s));
 append_documents(out,member,documents);for(const auto& s:out.sources_)read(resources,s);return out;
}
void BnwBaseUnitArtCompilation::apply_to_unsealed_database(content::RulesDatabase& db,const assets::ResourceMountTable& resources,const runtime::SavedContentProfile& profile)const{require(resources_==&resources&&profile_key_==profile_key(profile),"Foreign unit art source table/profile");for(const auto& s:sources_)read(resources,s);
 for(auto table:unit_art_source::tables){std::int64_t count=-1;auto result=db.query_rows({"SELECT count(*) AS n FROM "+std::string(table),{}},[&](const content::RulesRowView& row){count=row.integer("n");},1);require(result.all_succeeded()&&count==0,"Art source requires empty pre-action engine tables");}
 db.apply_statements(statements_);
}
}

