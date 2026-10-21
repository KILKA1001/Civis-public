#include "civ5/graphics/bnw_base_unit_member_info_compilation.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "../ui/bnw_native_completed_unit_art_inputs.hpp"
#include <charconv>
#include <cmath>
#include <set>
namespace civ5::graphics {
namespace {
void require(bool value,const char* why){if(!value)throw std::invalid_argument(why);}
constexpr const char* paths[]={"Assets/Units/Civ5ArtDefines_UnitMembers.xml","Assets/Units/Civ5ArtDefines_Units.xml","Assets/Units/Civ5ArtDefines_UnitMemberTemplates.xml"};
constexpr const char* sql="INSERT INTO ArtDefine_UnitMemberInfos(Type, Scale, ZOffset, Domain, Model, MaterialTypeTag, MaterialTypeSoundOverrideTag) VALUES(?,?,?,?,?,?,?);";
std::string profile_key(const runtime::SavedContentProfile& p){require(p.can_activate&&p.all_requirements_located&&p.files_complete&&!p.native_mods_require_port,"Unit art source selected profile not activatable");std::string out=std::to_string(int(p.kind));for(auto i:p.enabled_dlc)out+="D"+std::to_string(i);for(auto i:p.enabled_mods)out+="M"+std::to_string(i);for(const auto& r:p.requirements)out+="R"+std::to_string(int(r.kind))+":"+r.id+":"+std::to_string(r.wire_value)+":"+std::to_string(int(r.status));return out;}
struct Document{BnwUnitMemberInfoSourceRecord source;content::XmlNode xml;};
Document read(const assets::ResourceMountTable& resources,const char* path){auto hit=resources.resolve(assets::AssetKey(path));const auto* h=std::get_if<assets::AssetHandle>(&hit.asset);require(h,"Source unit-art XML missing/ambiguous");auto bytes=h->read_bytes(64*1024*1024);auto xml=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));return {{path,runtime::identify_bytes(bytes).sha256,hit.mount_id,h->candidate().provenance},std::move(xml)};}
const content::XmlNode* one(const content::XmlNode& row,const char* name){for(const auto& c:row.children)if(c.name==name)return &c;return nullptr;}
std::string field(const content::XmlNode& row,const char* name,std::size_t source_extent){const auto* n=one(row,name);if(!n)return {};require(n->children.empty()&&n->text.find('\0')==std::string::npos,"Member info string nesting/NUL unsupported");require(n->text.size()<source_extent,"Member info string exceeds original buffer");return n->text;}
content::DatabaseValue number(const content::XmlNode& row,const char* name){const auto* n=one(row,name);if(!n)return std::monostate{};require(n->children.empty(),"Member info numeric nesting unsupported");float value=0;auto r=std::from_chars(n->text.data(),n->text.data()+n->text.size(),value);require(r.ec==std::errc{}&&r.ptr==n->text.data()+n->text.size()&&std::isfinite(value),"Member info numeric lexical branch unsupported");return double(value);}
void validate_sources(const assets::ResourceMountTable& resources,const std::vector<BnwUnitMemberInfoSourceRecord>& source){for(const auto& s:source){auto now=read(resources,s.request.c_str());require(now.source.sha256==s.sha256&&now.source.mount==s.mount&&now.source.provenance.kind==s.provenance.kind&&now.source.provenance.source_id==s.provenance.source_id&&now.source.provenance.source_relative_path==s.provenance.source_relative_path&&now.source.provenance.archive_relative_path==s.provenance.archive_relative_path,"Unit art selected resource source changed");}}
}
void BnwBaseUnitMemberInfoCompilation::append_document(BnwBaseUnitMemberInfoCompilation& out,
    const content::XmlNode& doc,std::set<std::string>& inserted){
 if(doc.name!="UnitMemberArtInfos")return;
  for(const auto& row:doc.children){if(row.name!="UnitMemberArtInfo")continue;auto type=field(row,"Type",128),model=field(row,"Granny",128);require(!type.empty()&&!model.empty(),"Member info required source Type/Granny absent");if(!inserted.insert(type).second)continue;
   out.statements_.push_back({sql,{std::move(type),number(row,"fScale"),number(row,"fZOffset"),field(row,"Domain",32),std::move(model),field(row,"MaterialTypeTag",32),field(row,"MaterialTypeSoundOverrideTag",32)}});
  }
}
std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>
BnwBaseUnitMemberInfoCompilation::canonical_completed_inputs(
    const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>& supplied){
 const auto incoming=supplied;require(bool(incoming),"Completed unit art genuine inputs absent");
 const auto canonical=incoming->retain_owner();
 require(canonical.get()==incoming.get()&&!canonical.owner_before(incoming)&&!incoming.owner_before(canonical),
         "Completed unit art requires SAME canonical inputs control");
 return canonical;
}
BnwBaseUnitMemberInfoCompilation::CompletedDocument
BnwBaseUnitMemberInfoCompilation::read_completed_document(
    const ui::detail::BnwNativeCompletedUnitArtInputs& inputs,unsigned ordinal,const std::string* expected){
 const std::optional<std::string> expected_value=expected?std::optional<std::string>(*expected):std::nullopt;
 std::optional<CompletedDocument> document;
 inputs.with_document_bytes(ordinal,[&](std::span<const std::uint8_t> bytes){
  const auto sha=runtime::identify_bytes(bytes).sha256;
  require(!expected_value||sha==*expected_value,"Completed unit art SAME source bytes changed");
  // This parser owns the real read bytes; no caller XML/DOM input is adopted.
  document.emplace(CompletedDocument{sha,content::read_xml_metadata(std::string(bytes.begin(),bytes.end()))});
 });
 require(document.has_value(),"Completed unit art omitted genuine document");return std::move(*document);
}
BnwBaseUnitMemberInfoCompilation BnwBaseUnitMemberInfoCompilation::compile_from_completed_native_inputs(
    const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>& supplied,
    const runtime::SavedContentProfile& profile){
 const runtime::SavedContentProfile selected(profile);const auto inputs=canonical_completed_inputs(supplied);
 inputs->require_same_profile(selected);BnwBaseUnitMemberInfoCompilation out;
 out.completed_inputs_=inputs;out.profile_key_=profile_key(selected);std::set<std::string> inserted;
 for(unsigned ordinal=0;ordinal<3;++ordinal){
  const auto doc=read_completed_document(*inputs,ordinal);out.completed_sha256_[ordinal]=doc.sha256;
  append_document(out,doc.xml,inserted);
 }
 require(!out.statements_.empty(),"Native base unit-member source table empty");
 for(unsigned ordinal=0;ordinal<3;++ordinal)read_completed_document(*inputs,ordinal,&out.completed_sha256_[ordinal]);
 inputs->require_same_profile(selected);return out;
}
void BnwBaseUnitMemberInfoCompilation::apply_to_unsealed_database_from_completed_native_inputs(
    content::RulesDatabase& db,const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>& supplied,
    const runtime::SavedContentProfile& profile)const{
 // Own every consumer value before providers; no field of this is read later.
 const runtime::SavedContentProfile selected(profile);const auto stored=completed_inputs_;
 const auto hashes=completed_sha256_;const auto key=profile_key_;const auto statements=statements_;
 const auto incoming=supplied;
 require(stored&&incoming&&stored.get()==incoming.get()&&!stored.owner_before(incoming)&&
         !incoming.owner_before(stored)&&key==profile_key(selected),"Completed member art foreign inputs/profile");
 const auto inputs=canonical_completed_inputs(stored);
 inputs->require_same_profile(selected);
 for(unsigned ordinal=0;ordinal<3;++ordinal)read_completed_document(*inputs,ordinal,&hashes[ordinal]);
 inputs->require_same_profile(selected);std::int64_t count=-1;
 const auto query=db.query_rows({"SELECT count(*) AS n FROM ArtDefine_UnitMemberInfos",{}},
     [&](const content::RulesRowView& row){count=row.integer("n");},1);
 require(query.all_succeeded()&&count==0,"Unit art source application requires empty pre-action engine table");
 db.apply_statements(statements);
}
BnwBaseUnitMemberInfoCompilation BnwBaseUnitMemberInfoCompilation::compile(const assets::ResourceMountTable& resources,const runtime::SavedContentProfile& profile){BnwBaseUnitMemberInfoCompilation out;out.resources_=&resources;out.profile_key_=profile_key(profile);std::set<std::string> inserted;
 for(const auto* path:paths){auto doc=read(resources,path);out.sources_.push_back(doc.source);
  append_document(out,doc.xml,inserted);
 }
 require(!out.statements_.empty(),"Native base unit-member source table empty");validate_sources(resources,out.sources_);return out;
}
void BnwBaseUnitMemberInfoCompilation::apply_to_unsealed_database(content::RulesDatabase& db,const assets::ResourceMountTable& resources,const runtime::SavedContentProfile& profile)const {
 require(resources_==&resources&&profile_key_==profile_key(profile),"Unit art compiler foreign source table/profile");validate_sources(resources,sources_);std::int64_t count=-1;auto query=db.query_rows({"SELECT count(*) AS n FROM ArtDefine_UnitMemberInfos",{}},[&](const content::RulesRowView& row){count=row.integer("n");},1);require(query.all_succeeded()&&count==0,"Unit art source application requires empty pre-action engine table");
 // Existing apply_statements owns BEGIN/COMMIT and rollback of all rows on
 // failure. Source files are checked before the transaction, never after a
 // successful write where throwing could falsely advertise rollback.
 db.apply_statements(statements_);
}
} // namespace civ5::graphics


