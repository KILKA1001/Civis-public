#include "civ5/runtime/bnw_imported_pregame_locale_owner.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/content/saved_rules.hpp"
#include "civ5/content/game_data_xml.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/saves/loaded_save.hpp"
#include "civ5/runtime/bnw_sp_client_update_loan.hpp"
#include <vector>
#include <atomic>
#include <map>
#include <optional>
#include <thread>
#include <variant>
#include <array>
#include <mutex>
#include "civ5/runtime/bnw_sp_client_pregame_startup.hpp"
namespace civ5::runtime {
namespace {
void locale_need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::optional<std::string> locale_field(const content::XmlNode& row,const char* name){
 std::optional<std::string> result;
 for(const auto& [key,value]:row.attributes)if(key==name){locale_need(!result,"Pregame Locale duplicate XML attribute");result=value;}
 if(result)return result;
 if(const auto* node=row.child(name)){locale_need(node->children.empty(),"Pregame Locale unsupported text grammar node");return node->text;}
 return std::nullopt;
}
constexpr const char* player_key="TXT_KEY_MULTIPLAYER_DEFAULT_PLAYER_NAME";
constexpr const char* server_key="TXT_KEY_MULTIPLAYER_DEFAULT_SERVER_PLAYER_NAME";
}
#include "bnw_pregame_native_boot_event_queue.inc"
struct BnwImportedPregameLocaleOwner::Impl final {
 const std::thread::id thread=std::this_thread::get_id();
 mutable std::atomic<bool> retired{false};
 mutable bool entered=false;
 mutable bool checking=false;
 std::shared_ptr<assets::CivisResourceBankOwner> bank;
 std::shared_ptr<BnwPregameNativeBootEventQueue> boot_events;
 std::shared_ptr<const BnwSelectedLanguageInputOwner> selected;
 enum class ClientCompatibility {first_release_legacy_native_debug_logging_disabled};
 std::optional<ClientCompatibility> selected_client_configuration;
 std::unique_ptr<content::RulesDatabase> database;
 assets::CivisResourceBankSourceRecipe recipe;
 assets::ResourceMountTable resources;
 std::string language,rules_sha;
 std::string actual_payload_sha,actual_prefix_sha;
 std::optional<std::string> original_source_sha;
 saves::SaveContainerKind actual_container=saves::SaveContainerKind::civ5;
 std::weak_ptr<const BnwImportedPregameLocaleOwner> self;
 struct Origin {std::string text,sha;assets::AssetHandle asset;};
 std::map<std::string,Origin> rows;
 struct Manifest {std::filesystem::path path;FileIdentity pin;};
 std::vector<Manifest> manifests;
 explicit Impl(const assets::ResourceMountTable& source):resources(source){}
 void current()const{
  if(thread!=std::this_thread::get_id()||retired.load()||checking){retired.store(true);throw std::runtime_error("Pregame Locale foreign thread/retired/reentry before source reads");}
  checking=true;
  try{
   const auto actual=self.lock();locale_need(actual&&actual->impl_.get()==this,"Pregame Locale canonical source expired");
   bank->require_current();selected->require_current();
   const auto canonical=bank->retain_owner();locale_need(canonical.get()==bank.get()&&!canonical.owner_before(bank)&&!bank.owner_before(canonical),"Pregame Locale Bank control replaced");
   bank->require_boundary();selected->require_current();
   const auto now=bank->source_recipe();locale_need(now.recipe_identity==recipe.recipe_identity&&now.save_path==recipe.save_path&&now.game_root==recipe.game_root,"Pregame Locale Bank source recipe changed");
   locale_need(selected->language()==language&&database->immutable_preview_fingerprints_current()&&
    database->gameplay_fingerprint_sha256()==rules_sha,"Pregame Locale selected language/rules changed");
   for(const auto& manifest:manifests){const auto now_pin=identify_file(manifest.path);locale_need(now_pin.bytes==manifest.pin.bytes&&now_pin.sha256==manifest.pin.sha256,"Pregame Locale selected TextData manifest changed");}
   bank->require_current();selected->require_current();locale_need(!retired.load(),"Pregame Locale retired during source reads");
   locale_need(boot_events!=nullptr,"Pregame Locale actual early native event namespace absent");boot_events->require_current();checking=false;
  }catch(...){retired.store(true);throw;}
 }
 std::string lookup(const char* key)const{
  current();const auto found=rows.find(key);locale_need(found!=rows.end(),"Pregame Locale selected default-name source absent");
  const auto& origin=found->second;
  locale_need(identify_bytes(origin.asset.read_bytes(64ULL*1024*1024)).sha256==origin.sha,"Pregame Locale default-name TextData changed");
  std::string text;std::size_t count=0;
  const auto actual=database->query({"SELECT Text FROM Localization.LocalizedText WHERE Language=? AND Tag=?",{language,std::string(key)}},[&](const auto row){
   locale_need(row.size()==1&&std::get_if<std::string>(&row[0]),"Pregame Locale activated default-name row malformed");text=std::get<std::string>(row[0]);++count;
  });
  locale_need(actual==count&&count==1&&text==origin.text,"Pregame Locale actual DB/default-name source differs");current();return text;
 }
};
BnwImportedPregameLocaleOwner::BnwImportedPregameLocaleOwner(std::shared_ptr<Impl> impl):impl_(std::move(impl)){}
BnwImportedPregameLocaleOwner::~BnwImportedPregameLocaleOwner(){impl_->retired.store(true);}
std::shared_ptr<const BnwImportedPregameLocaleOwner> BnwImportedPregameLocaleOwner::capture(
 std::shared_ptr<const assets::CivisResourceBankOwner> input,const content::RulesDatabase& actual,
 const assets::ResourceMountTable& same_rules_resources){
 locale_need(bool(input),"Pregame early Locale actual Bank absent");
 const auto bank=input->retain_owner();locale_need(bank.get()==input.get()&&!bank.owner_before(input)&&!input.owner_before(bank),"Pregame early Locale Bank alias control differs");
 bank->require_current();auto state=std::make_shared<Impl>(same_rules_resources);state->bank=bank;state->recipe=bank->source_recipe();
 locale_need(state->recipe.profile.can_activate&&state->recipe.profile.all_requirements_located&&state->recipe.profile.files_complete&&state->recipe.profile.enabled_mods.empty(),"Pregame early Locale actual stock profile incomplete");
 // Reuse the canonical Bank; actual process/INI source is captured with its
 // existing selected-language owner. No second Bank, UI snapshot or default.
 state->selected=BnwSelectedLanguageInputOwner::capture_from_civis_resources(bank);
 state->selected->require_current();state->language=state->selected->language();
 // Explicit own first-release early Civis Bank configuration. It controls
 // legacy native diagnostic compatibility only; own logging and gameplay
 // commands retain their independent owners. No original manager receipt.
 state->selected_client_configuration=Impl::ClientCompatibility::first_release_legacy_native_debug_logging_disabled;
 locale_need(state->language=="en_US"||state->language=="ru_RU","Pregame early Locale default-name grammar supports selected EN/RU only");
 state->database=actual.clone_gameplay();state->rules_sha=identify_bytes(state->database->gameplay_fingerprint_data()).sha256;
 locale_need(state->rules_sha==identify_bytes(actual.gameplay_fingerprint_data()).sha256,"Pregame early Locale activated clone differs");
 const auto before_save=identify_file(state->recipe.save_path);const auto loaded=saves::read_loaded_save(state->recipe.save_path);const auto after_save=identify_file(state->recipe.save_path);
 locale_need(before_save.bytes==after_save.bytes&&before_save.sha256==after_save.sha256&&loaded.source_prefix,"Pregame early Locale actual selected file changed or lacks original prefix");
 state->actual_payload_sha=identify_bytes(loaded.bytes).sha256;state->actual_prefix_sha=identify_bytes(*loaded.source_prefix).sha256;state->actual_container=loaded.container;state->original_source_sha=loaded.source_sha256;
 const auto table=state->language=="en_US"?"Language_en_US":"Language_RU_RU";
 std::map<std::string,std::optional<Impl::Origin>> layer;std::optional<std::size_t> group;
 const auto flush=[&]{for(auto& [tag,value]:layer){locale_need(value.has_value(),"Pregame Locale ambiguous default-name source layer");state->rows.insert_or_assign(tag,std::move(*value));}layer.clear();};
 for(const auto index:state->recipe.profile.enabled_dlc){
  locale_need(index<state->recipe.catalog.packages.size(),"Pregame Locale actual DLC index out of range");
  const auto& package=state->recipe.catalog.packages[index];const auto before=identify_file(package.path);const auto parsed=content::read_manifest(package.path);const auto after=identify_file(package.path);
  locale_need(before.bytes==after.bytes&&before.sha256==after.sha256&&content::canonical_guid(parsed.id)==content::canonical_guid(package.id)&&parsed.kind==package.kind&&parsed.version==package.version,"Pregame Locale actual TextData manifest changed");
  content::PackageResourceQuery query;query.selector="TextData";query.enabled_categories={true,true,true};const auto expected=content::package_resources(package,query),found=content::package_resources(parsed,query);
  locale_need(expected.size()==found.size(),"Pregame Locale actual TextData declaration count changed");
  for(std::size_t n=0;n<found.size();++n)locale_need(expected[n].value==found[n].value&&expected[n].category==found[n].category&&expected[n].category_occurrence==found[n].category_occurrence&&expected[n].file_list==found[n].file_list,"Pregame Locale actual TextData declarations changed");
  state->manifests.push_back({package.path,before});
 }
 const auto records=content::selected_localization_sources(state->recipe.game_root,state->recipe.profile.enabled_dlc,state->recipe.catalog,state->resources);
 for(const auto& record:records){
  if(record.package!=group){flush();group=record.package;}
  const auto bytes=record.asset.read_bytes(64ULL*1024*1024);const auto document=content::read_game_data_xml_text(std::string(bytes.begin(),bytes.end()));
  locale_need(document.name=="GameData","Pregame Locale selected TextData root differs");
  const content::XmlNode* selected=nullptr;for(const auto& child:document.children)if(child.name==table){locale_need(!selected,"Pregame Locale duplicate language table");selected=&child;}
  if(!selected)continue;
  for(const auto& row:selected->children){const auto tag=locale_field(row,"Tag");if(!tag||(*tag!=player_key&&*tag!=server_key))continue;
   const auto text=locale_field(row,"Text");locale_need(row.name=="Row"&&text,"Pregame Locale selected default-name operation unsupported");
   Impl::Origin origin{*text,identify_bytes(bytes).sha256,record.asset};if(!layer.emplace(*tag,std::move(origin)).second)layer.at(*tag).reset();
  }
 }
 flush();locale_need(state->rows.contains(player_key)&&state->rows.contains(server_key),"Pregame Locale actual selected default-name keys missing");
 {
  std::size_t language_count=0;const auto language_rows=state->database->query({"SELECT Type FROM Localization.Languages WHERE Type=?",{state->language}},[&](const auto row){locale_need(row.size()==1&&std::get_if<std::string>(&row[0]),"Pregame Locale actual language metadata malformed");++language_count;});
  locale_need(language_rows==language_count&&language_count==1,"Pregame Locale selected language not uniquely activated");
 } // Actual query statement/visitor temporaries have retired.
 // This SAME value-owned clone has completed capture and is only read later.
 // Actual import-database validation below remains a distinct real check.
 state->database->seal_immutable_preview_fingerprints();
 state->boot_events=BnwPregameNativeBootEventQueue::capture_once(bank);
 auto result=std::shared_ptr<const BnwImportedPregameLocaleOwner>(new BnwImportedPregameLocaleOwner(state));state->self=result;
 state->current();return result;
}
void BnwImportedPregameLocaleOwner::require_current()const{impl_->current();}
std::shared_ptr<const BnwImportedPregameLocaleOwner> BnwImportedPregameLocaleOwner::retain_canonical()const{impl_->current();const auto actual=impl_->self.lock();locale_need(actual&&actual.get()==this,"Pregame Locale actual canonical issuer expired");return actual;}
void BnwImportedPregameLocaleOwner::require_import(const saves::SavedSession& source,const content::RulesDatabase& database,const SessionCheckpointIdentity& identity)const{
 const auto p=impl_;p->current();
 locale_need(identify_bytes(source.source.bytes).sha256==identity.payload_sha256&&identity.rules_sha256==p->rules_sha&&identify_bytes(database.gameplay_fingerprint_data()).sha256==p->rules_sha,"Pregame early Locale wrong actual import payload/rules");
 locale_need(source.source.source_prefix&&identify_bytes(*source.source.source_prefix).sha256==p->actual_prefix_sha&&identity.payload_sha256==p->actual_payload_sha&&source.source.container==p->actual_container&&source.source.source_sha256==p->original_source_sha,"Pregame early Locale wrong actual selected container/payload/original provenance");p->current();
}
void BnwImportedPregameLocaleOwner::require_first_release_client_debug_configuration(const BnwSpClientPregameStartup& caller)const{
 const auto p=impl_;p->current();
 try{
  caller.current();const auto actual=p->self.lock();
  locale_need(actual&&caller.early_locale_.get()==actual.get()&&!caller.early_locale_.owner_before(actual)&&!actual.owner_before(caller.early_locale_)&&
   caller.phase_==BnwSpClientPregameStartup::Phase::namespace_constructed&&!caller.registration_returned_,
   "SP selected early client configuration requires SAME actual imported Bank/Pregame before Read");
  locale_need(p->selected_client_configuration&&*p->selected_client_configuration==Impl::ClientCompatibility::first_release_legacy_native_debug_logging_disabled,
   "SP explicit first-release legacy native debug configuration absent");
  p->selected->require_current();p->bank->require_current();caller.current();p->current();
 }catch(...){p->retired.store(true);throw;}
}
void BnwImportedPregameLocaleOwner::publish_registration_player_event(const BnwSpClientPregameStartup& caller,std::int32_t id)const{
 const auto p=impl_;p->current();
 try{
  caller.current();const auto actual=p->self.lock();
  locale_need(actual&&caller.early_locale_.get()==actual.get()&&!caller.early_locale_.owner_before(actual)&&!actual.owner_before(caller.early_locale_)&&
   caller.phase_==BnwSpClientPregameStartup::Phase::registration_prefix&&caller.selected_registration_id_==id&&caller.local_id_==id&&id>=0&&id<64&&
   !caller.native_callback_returned_&&!caller.registration_returned_,"Pregame native149 requires original SAME early caller and registration prefix");
  locale_need(caller.pregame_context_!=nullptr&&p->boot_events!=nullptr,"Pregame native149 actual Context/queue owner absent");
  p->boot_events->publish_native149(id);caller.current();p->current();
 }catch(...){p->retired.store(true);throw;}
}
std::string BnwImportedPregameLocaleOwner::default_nickname(bool host,std::int32_t id)const{
 const auto p=impl_;p->current();if(p->entered||id<0||id>=64){p->retired.store(true);throw std::runtime_error("Pregame early Locale default-name replay/reentry/range");}p->entered=true;
 try{
  // Retail71817D first constructs its Localization::String, then Lookup and
  // copy-assign. Integer PushArgument(id+1) occurs ONLY for actual host=false.
  std::string native_string;
  native_string=p->lookup(host?server_key:player_key);p->current();
  std::vector<std::int32_t> native_arguments;
  if(!host){native_arguments.push_back(id+1);p->current();}
  // Bounded actual selected EN/RU decimal grammar: IDs1..64 never need a
  // grouping separator. Other modifiers, braces or extra args are errors.
  std::string utf8;std::size_t cursor=0,used=0;
  while(cursor<native_string.size()){
   const auto open=native_string.find('{',cursor);if(open==std::string::npos){utf8.append(native_string,cursor,std::string::npos);break;}
   utf8.append(native_string,cursor,open-cursor);const auto end=native_string.find('}',open+1);
   locale_need(!host&&end!=std::string::npos&&native_string.substr(open+1,end-open-1)=="1_Num"&&native_arguments.size()==1,"Pregame Locale actual default-name grammar requires unsupported formatter");
   utf8+=std::to_string(native_arguments.front());++used;cursor=end+1;
  }
  locale_need(!utf8.empty()&&utf8.find('}')==utf8.npos&&(host?used==0:used==1),"Pregame Locale actual default-name integer argument usage differs");
  p->current();p->entered=false;return utf8;
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
void BnwImportedPregameLocaleOwner::publish_actual_control_client_event(
 const BnwSpClientPregameStartup& caller,const BnwSpClientUpdateLoan& loan,std::uint32_t type)const{
 loan.require_current_without_callbacks();const auto p=impl_;p->current();loan.require_current_without_callbacks();
 try{
  caller.require_actual_control_callback(loan);const auto actual=p->self.lock();
  locale_need(actual&&actual.get()==this&&caller.early_locale_.get()==actual.get()&&
   !caller.early_locale_.owner_before(actual)&&!actual.owner_before(caller.early_locale_)&&p->boot_events&&
   ((type==146&&caller.control_stage_==BnwSpClientPregameStartup::ControlStage::event146)||
    (type==145&&caller.control_stage_==BnwSpClientPregameStartup::ControlStage::event145)),
   "SP client events require SAME initialized149 namespace and genuine current683250 operand");
  loan.require_current_without_callbacks();
  p->boot_events->publish_native_client146_or_145(loan,type);
  loan.require_current_without_callbacks();caller.require_actual_control_callback(loan);
  p->current();loan.require_current_without_callbacks();
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
void BnwImportedPregameLocaleOwner::publish_actual_client155_event(
 const BnwSpClientPregameStartup& caller,const BnwSpClientUpdateLoan& loan,std::uint32_t type)const{
 loan.require_current_without_callbacks();const auto p=impl_;p->current();loan.require_current_without_callbacks();
 try{
  caller.require_actual_client155_callback(loan);const auto actual=p->self.lock();
  locale_need(actual&&actual.get()==this&&caller.early_locale_.get()==actual.get()&&
   !caller.early_locale_.owner_before(actual)&&!actual.owner_before(caller.early_locale_)&&p->boot_events&&
   ((type==146&&caller.client155_stage_==BnwSpClientPregameStartup::Client155Stage::event146)||
    (type==132&&caller.client155_stage_==BnwSpClientPregameStartup::Client155Stage::event132)),
   "SP155 native146/132 requires SAME initialized149 arena and actual current decoded155");
  loan.require_current_without_callbacks();p->boot_events->publish_native_client155_event(loan,type);
  loan.require_current_without_callbacks();caller.require_actual_client155_callback(loan);
  p->current();loan.require_current_without_callbacks();
 }catch(...){p->retired.store(true);throw;}
}
}
