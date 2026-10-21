#include "civ5/runtime/bnw_civis_owned_stock_source_bundle.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/assets/ui_texture_binding.hpp"
#include <stdexcept>
#include <algorithm>
#include <regex>
#include <set>
#include <chrono>
#include <cstdio>
namespace civ5::runtime {
namespace {
// Diagnostic counters only: no source authority, callbacks or loading policy.
// All labels are static; no script names or payload bytes are printed.
struct ColdStageObservation {
 using Clock=std::chrono::steady_clock;
 const Clock::time_point began=Clock::now();
 Clock::time_point phase_began=began,last_progress=began;
 const char* phase="entry";
 std::size_t lua_names=0,scripts_queued=0,scripts_completed=0;
 std::uint64_t script_payloads=0,script_bytes=0,include_requests=0;
 std::uint64_t regex_compilations=0,regex_attempts=0,regex_matches=0;
 std::uint64_t xml_requests=0,texture_requests=0;
 bool completed=false;
 void emit(const char* event) noexcept {
  const auto now=Clock::now();
  const auto stamp=std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(now-began).count();
  const auto phase_elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(now-phase_began).count();
  std::fprintf(stderr,"[CIVIS_COLD_STAGE] event=%s phase=%s steady_ms=%lld elapsed_ms=%lld phase_elapsed_ms=%lld lua_names=%zu scripts_queued=%zu scripts_completed=%zu script_payloads=%llu script_bytes=%llu include_requests=%llu regex_compilations=%llu regex_attempts=%llu regex_matches=%llu xml_requests=%llu texture_requests=%llu\n",
   event,phase,static_cast<long long>(stamp),static_cast<long long>(elapsed),static_cast<long long>(phase_elapsed),
   lua_names,scripts_queued,scripts_completed,
   static_cast<unsigned long long>(script_payloads),static_cast<unsigned long long>(script_bytes),
   static_cast<unsigned long long>(include_requests),static_cast<unsigned long long>(regex_compilations),
   static_cast<unsigned long long>(regex_attempts),static_cast<unsigned long long>(regex_matches),
   static_cast<unsigned long long>(xml_requests),static_cast<unsigned long long>(texture_requests));
  std::fflush(stderr);last_progress=now;
 }
 void begin(const char* label) noexcept {phase=label;phase_began=Clock::now();emit("begin");}
 void end() noexcept {emit("end");}
 void progress() noexcept {
  if(Clock::now()-last_progress>=std::chrono::seconds(1))emit("progress");
 }
 ~ColdStageObservation() noexcept {emit(completed?"complete":"unwind");}
};
// Static cold-discovery aid only. Runtime include still owns Lua pattern
// matching, first/all execution and the actual IncludeFileList order.
bool cold_word(char c) {
 return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';
}
bool cold_space(char c) {return c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\f'||c=='\v';}
bool cold_long(std::string_view s,std::size_t at,std::size_t& body,std::size_t& end) {
 if(at>=s.size()||s[at]!='[')return false;
 auto j=at+1;while(j<s.size()&&s[j]=='=')++j;
 if(j==s.size()||s[j]!='[')return false;
 const std::string close="]"+std::string(j-at-1,'=')+"]";
 body=j+1;const auto found=s.find(close,body);
 if(found==std::string_view::npos)throw std::runtime_error("Unclosed cold Lua long token");
 end=found+close.size();return true;
}
std::size_t cold_trivia(std::string_view s,std::size_t at) {
 while(at<s.size()){
  if(cold_space(s[at])){++at;continue;}
  if(s.substr(at,2)!="--")break;
  std::size_t body,end;
  if(cold_long(s,at+2,body,end)){at=end;continue;}
  const auto line=s.find('\n',at+2);at=line==std::string_view::npos?s.size():line+1;
 }
 return at;
}
std::string cold_string(std::string_view s,std::size_t& at) {
 std::size_t body,end;
 if(cold_long(s,at,body,end)){
  if(body<s.size()&&s[body]=='\r')++body;
  if(body<s.size()&&s[body]=='\n')++body;
  auto close=end-1;while(close>body&&s[close-1]=='=')--close;
  const auto result=std::string(s.substr(body,close-body-1));at=end;return result;
 }
 if(at==s.size()||(s[at]!='\''&&s[at]!='"'))
  throw std::runtime_error("Cold Lua include is not a literal string");
 const char quote=s[at++];std::string result;
 while(at<s.size()){
  char c=s[at++];if(c==quote)return result;
  if(c=='\\'){
   if(at==s.size())break;c=s[at++];
   if(c>='0'&&c<='9'){
    unsigned value=unsigned(c-'0');
    for(unsigned n=1;n<3&&at<s.size()&&s[at]>='0'&&s[at]<='9';++n)
     value=value*10+unsigned(s[at++]-'0');
    if(value>255)throw std::runtime_error("Cold Lua string escape exceeds byte range");
    c=char(value);
   }else switch(c){
    case 'a':c='\a';break;case 'b':c='\b';break;case 'f':c='\f';break;
    case 'n':c='\n';break;case 'r':c='\r';break;case 't':c='\t';break;case 'v':c='\v';break;
    case '\r':if(at<s.size()&&s[at]=='\n')++at;c='\n';break;
    default:break;
   }
  }
  result.push_back(c);
 }
 throw std::runtime_error("Unclosed cold Lua short string");
}
std::vector<std::string> cold_literal_includes(std::string_view s) {
 std::vector<std::string> result;std::size_t at=0;
 while((at=cold_trivia(s,at))<s.size()){
  std::size_t body,end;
  if(s[at]=='\''||s[at]=='"'||cold_long(s,at,body,end)){(void)cold_string(s,at);continue;}
  if(!cold_word(s[at])||(s[at]>='0'&&s[at]<='9')){++at;continue;}
  const auto begin=at;while(at<s.size()&&cold_word(s[at]))++at;
  if(s.substr(begin,at-begin)!="include")continue;
  auto prev=begin;while(prev&&cold_space(s[prev-1]))--prev;
  if(prev&&(s[prev-1]=='.'||s[prev-1]==':'))continue;
  auto arg=cold_trivia(s,at);const bool paren=arg<s.size()&&s[arg]=='(';
  if(paren)arg=cold_trivia(s,arg+1);
  else if(arg==s.size()||(s[arg]!='\''&&s[arg]!='"'&&s[arg]!='['))continue;
  auto request=cold_string(s,arg);
  const auto tail=cold_trivia(s,arg);
  if(paren&&(tail==s.size()||(s[tail]!=','&&s[tail]!=')')))
   throw std::runtime_error("Cold Lua include has a nonliteral first argument expression");
  if(request.empty())throw std::runtime_error("Cold Lua include has an empty pattern");
  result.push_back(std::move(request));at=arg;
 }
 return result;
}
std::string cold_name_view(std::string name) {
 for(auto& c:name){if(c=='\\')c='/';else if(c>='A'&&c<='Z')c=char(c-'A'+'a');}
 return name;
}
std::regex cold_include_pattern(const std::string& request) {
 // Selected literals use names, dot and %w+. Case/separator name views form
 // a conservative cold byte superset, not runtime Lua matcher equivalence.
 const auto name=cold_name_view(request);std::string regex;
 for(std::size_t i=0;i<name.size();++i){
  const char c=name[i];
  if(c=='%'&&name.substr(i,3)=="%w+"){regex+="[a-z0-9]+";i+=2;}
  else if(cold_word(c)||c=='/'||c=='.')regex.push_back(c);
  else throw std::runtime_error("Unsupported static cold Lua include pattern: "+request);
 }
 return std::regex(regex,std::regex::ECMAScript);
}
bool cold_context_excluded(const std::string& id,const std::string& source) {
 return id=="StagingRoom"||id=="MPTurnPanel"||id=="MPList"||id=="DebugMenu"||
  id=="NetworkDebug"||source.find("/Multiplayer/")!=std::string::npos;
}
}
BnwCivisOwnedStockSourceBundle::BnwCivisOwnedStockSourceBundle(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,
 std::shared_ptr<assets::CivisResourceBankOwner> bank,
 std::shared_ptr<const ui::CivisResourceBankUiSnapshot> ui,
 std::shared_ptr<const ui::CivisResourceBankFontColorSnapshot> font)
 :host_(host),bank_(std::move(bank)),capability_(bank_->capability()),
  ui_(std::move(ui)),font_(std::move(font)),recipe_(bank_->source_recipe()){
 require_current();require_same_host(*host);
 ColdStageObservation observation;observation.begin("explicit_sources");
 // Explicit cold staging, before a retained VM/native callback is entered.
 // Physical paths and normalized VFS aliases are staged separately and each
 // actual reader checks the selected payload hash, preserving precedence.
 for(const auto* name:{
  "Assets/DLC/Expansion2/UI/InGame/InGame.xml","Assets/UI/InGame/InGame.xml",
  "Assets/DLC/Expansion2/UI/InGame/InGame.lua","Assets/UI/InGame/InGame.lua",
  "Assets/DLC/Expansion2/UI/InGame/YieldIconManager.lua","Assets/UI/InGame/YieldIconManager.lua",
  "Assets/UI/InGame/YieldIconManager.xml",
  "Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua","Assets/UI/Civilopedia/CivilopediaScreen.lua",
  "Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml","Assets/UI/Civilopedia/CivilopediaScreen.xml",
  "Assets/UI/InstanceManager.lua","Assets/UI/IconSupport.lua","Assets/Gameplay/Lua/FLuaVector.lua",
  "Assets/UI/Styles.xml","Assets/UI/ColorAtlas.xml",
  "Assets/DLC/Expansion2/UI/ColorAtlas_Expansion2.xml",
  "Assets/DLC/Expansion/UI/Styles_Expansion1.xml","Assets/DLC/Expansion2/UI/Styles_Expansion2.xml",
  "Assets/DLC/Expansion2/UI/InGame/WorldView/WorldView.lua","Assets/UI/InGame/WorldView/WorldView.lua",
  "Assets/UI/InGame/WorldView/WorldView.xml",
  "Assets/DLC/Expansion2/UI/InGame/UnitFlagManager.lua","Assets/UI/InGame/UnitFlagManager.lua",
  "Assets/UI/InGame/UnitFlagManager.xml",
  "Assets/DLC/Expansion2/UI/InGame/WorldView/ActionInfoPanel.lua","Assets/UI/InGame/WorldView/ActionInfoPanel.lua",
  "Assets/DLC/Expansion2/UI/InGame/WorldView/ActionInfoPanel.xml","Assets/UI/InGame/WorldView/ActionInfoPanel.xml",
  "Assets/UI/InGame/Popups/TechAwardPopup.lua","Assets/UI/InGame/Popups/TechAwardPopup.xml",
  "Assets/UI/InGame/TechTree/TechButtonInclude.lua","Assets/UI/InGame/InfoTooltipInclude.lua",
  "Assets/DLC/Expansion2/UI/InGame/Popups/TechAwardPopup.xml",
  "Assets/DLC/Expansion2/UI/InGame/TechTree/TechButtonInclude.lua",
  "Assets/DLC/Expansion2/UI/InGame/InfoTooltipInclude.lua"}){
  require_current();const std::string name_string(name);
  bank_->load(assets::AssetKey(name_string),name_string.ends_with(".xml")?
    assets::CivisResourceParse::xml:assets::CivisResourceParse::bytes);
  require_current();
 }
 observation.end();observation.begin("selected_context_bank");
#include "bnw_selected_client_context_bank_stage.inc"
 observation.end();observation.begin("lua_namespace");
 // Prepare real context scripts and their literal include dependency bytes
 // before any memory-only GUI inspector or retained native VM callback.
 // Namespace names are discovery inventory; they do not issue IncludeFileList.
 const auto namespace_names=resources().keys();require_current();
 std::vector<std::pair<std::string,std::string>> lua_names;
 for(const auto& name:namespace_names){
  const auto view=cold_name_view(name);
  if(view.ends_with(".lua"))lua_names.emplace_back(name,view);
 }
 observation.lua_names=lua_names.size();observation.end();
 std::vector<std::string> scripts;std::set<std::string> script_keys;
 const auto add_script=[&](const std::string& key){
  if(script_keys.insert(assets::normalize_asset_name(key)).second){
   if(scripts.size()>=4096)throw std::runtime_error("Civis cold script closure exceeds its bound");
   scripts.push_back(key);observation.scripts_queued=scripts.size();
  }
 };
 for(const auto& context:ui_->registry().contexts())add_script(context.source_key+".lua");
 for(const auto& source:ui_->document_sources()){
  const auto& key=source.requested_key;
  if(!cold_name_view(key).ends_with(".xml"))continue;
  const auto sibling=key.substr(0,key.size()-4)+".lua";
  require_current();const auto present=bank_->has_indexed(assets::AssetKey(sibling));require_current();
  if(present)add_script(sibling);
 }
 std::set<std::string> documents;std::vector<std::string> document_stack;
 const auto stage_document=[&](auto&& self,const std::string& key)->void{
  const auto normalized=assets::normalize_asset_name(key);
  if(std::find(document_stack.begin(),document_stack.end(),normalized)!=document_stack.end())
   throw std::runtime_error("Cycle in Civis cold context XML: "+key);
  if(!documents.insert(normalized).second)return;
  if(documents.size()>4096||document_stack.size()>=128)
   throw std::runtime_error("Civis cold context XML closure exceeds its bound");
  document_stack.push_back(normalized);++observation.xml_requests;
  require_current();bank_->load(assets::AssetKey(key),assets::CivisResourceParse::xml);require_current();
  bank_->load_all(assets::AssetKey(key),assets::CivisResourceParse::xml);require_current();
  const auto hits=resources().resolve_all(assets::AssetKey(key));require_current();
  for(const auto& hit:hits){
   const auto* actual=std::get_if<assets::AssetHandle>(&hit.asset);
   if(!actual)throw std::runtime_error("Civis cold context XML all-hit is not an actual payload: "+key);
   require_current();const auto bytes=actual->read_bytes(64ULL*1024*1024);require_current();
   const auto xml=content::read_xml_metadata(std::string(reinterpret_cast<const char*>(bytes.data()),bytes.size()));
   if(xml.name!="Context")throw std::runtime_error("Civis cold context XML has no Context root: "+key);
   const auto walk=[&](auto&& visit,const content::XmlNode& node)->void{
    if(node.name=="LuaContext"){
     const auto source=node.attribute("FileName");auto id=node.attribute("ID");
     if(id.empty()){const auto slash=source.find_last_of("/\\");id=source.substr(slash==std::string::npos?0:slash+1);}
     if(!source.empty()&&!cold_context_excluded(id,source)){
      add_script(source+".lua");
      const auto nested=source+".xml";
      require_current();const auto present=bank_->has_indexed(assets::AssetKey(nested));require_current();
      if(present)self(self,nested);
     }
    }
    for(const auto& child:node.children)visit(visit,child);
   };
   walk(walk,xml);
  }
  document_stack.pop_back();
 };
 observation.begin("context_xml");
 for(const auto* key:{"Assets/DLC/Expansion2/UI/InGame/InGame.xml",
                     "Assets/UI/InGame/InGame.xml","Assets/UI/InGame/WorldView/WorldView.xml"})
  stage_document(stage_document,key);
 observation.end();observation.begin("script_closure");
 std::uint64_t inspected_script_bytes=0;
 for(std::size_t cursor=0;cursor<scripts.size();++cursor){
  const auto key=scripts[cursor];
  try{
   // Winner first: ambiguity is never bypassed by the all-hit preparation.
   require_current();bank_->load(assets::AssetKey(key));require_current();
   bank_->load_all(assets::AssetKey(key));require_current();
   const auto hits=resources().resolve_all(assets::AssetKey(key));require_current();
   for(const auto& hit:hits){
    const auto* actual=std::get_if<assets::AssetHandle>(&hit.asset);
    if(!actual)throw std::runtime_error("Civis cold script all-hit is not an actual payload");
    require_current();const auto bytes=actual->read_bytes(64ULL*1024*1024);require_current();
    if(bytes.size()>64ULL*1024*1024-inspected_script_bytes)
     throw std::runtime_error("Civis cold literal script inspection exceeds its byte bound");
    inspected_script_bytes+=bytes.size();
    ++observation.script_payloads;observation.script_bytes=inspected_script_bytes;
    for(const auto& request:cold_literal_includes(std::string_view(reinterpret_cast<const char*>(bytes.data()),bytes.size()))){
     ++observation.include_requests;++observation.regex_compilations;
     const auto pattern=cold_include_pattern(request);
     for(const auto& [name,view]:lua_names){
      ++observation.regex_attempts;
      if(std::regex_search(view,pattern)){++observation.regex_matches;add_script(name);}
      if((observation.regex_attempts&16383ULL)==0)observation.progress();
     }
     observation.progress();
     // The actual generic include returns an empty list for no match. This
     // does not make a missing mandatory context script optional.
    }
   }
  }catch(const std::exception& error){
   throw std::runtime_error("Civis context script cold stage: "+key+": "+error.what());
  }
  ++observation.scripts_completed;observation.progress();
 }
 require_current();observation.end();observation.begin("authored_textures");
 // Existing real authored texture declarations, never manufactured atlas
 // names. Dynamic gameplay texture selections need their own later cold stage.
 // Match bind_ui_texture's mounted-key policy at the cold IO boundary.
 // has_indexed is false only for an absent/monostate winner; ambiguity is
 // loaded under the authored key and must fail rather than select a basename.
 const auto stage_texture_source=[&](const std::string& requested){
  require_current();
  const bool present=bank_->has_indexed(assets::AssetKey(requested));
  require_current();
  const auto basename=assets::resource_basename(requested);
  const std::string selected=!present&&basename!=requested?basename:requested;
  bank_->load(assets::AssetKey(selected));require_current();
  return loaded(selected);
 };
 const auto stage_texture=[&](const std::string& requested){
  ++observation.texture_requests;
  try{
  const auto source=stage_texture_source(requested);
  const auto dds=assets::DdsData::parse(*source.bytes);
  require_current();
  if(assets::ui_texture_storage_profile(dds)==assets::UiTextureStorageProfile::indexed_interface){
   const auto& original=source.candidate.original_name;
   if(original.size()<4||assets::normalize_asset_name(original.substr(original.size()-4))!=".dds")
    throw std::runtime_error("Indexed UI source is not a DDS: "+requested);
   stage_texture_source(original.substr(0,original.size()-4)+"-index.dds");
  }
  // Reuse the real reader's paired-container, identity and byte-stability
  // checks after both actual selected payloads are in the memory-only bank.
  require_current();
  assets::bind_ui_texture(bank_->resources(capability_),requested);
  require_current();
  }catch(const std::exception& error){
   throw std::runtime_error("Civis authored texture cold stage: "+requested+": "+error.what());
  }
 };
 const auto stage_textures=[&](auto&& self,const content::XmlNode& node)->void{
  for(const auto& [name,value]:node.attributes)
   if(!value.empty()&&(name=="Texture"||name=="FGTexture"||name=="BGTexture"||name=="Texture2")){
    stage_texture(value);
   }
  for(const auto& child:node.children)self(self,child);
 };
 for(const auto& source:ui_->document_sources()){
  const auto value=loaded(source.requested_key);
  if(value.xml)stage_textures(stage_textures,*value.xml);
 }
 for(const auto* key:{"Assets/UI/Styles.xml","Assets/DLC/Expansion/UI/Styles_Expansion1.xml",
                      "Assets/DLC/Expansion2/UI/Styles_Expansion2.xml"}){
  const auto styles=loaded(key);
  if(styles.xml)stage_textures(stage_textures,*styles.xml);
 }
 observation.end();observation.begin("final_boundary");
 require_boundary();
 observation.end();observation.completed=true;
}
void BnwCivisOwnedStockSourceBundle::require_current()const{
 if(creator_!=std::this_thread::get_id())throw std::runtime_error("Own stock source foreign creator");
 capability_.require_current();bank_->require_current();
 ui_->require_current();ui_->require_same_bank(*bank_);
 font_->require_current();font_->require_same_bank(*bank_);font_->require_same_ui(*ui_);
 if(host_.expired())throw std::runtime_error("Own stock selected host retired");
}
void BnwCivisOwnedStockSourceBundle::require_boundary()const{
 require_current();bank_->require_boundary();ui_->require_boundary();font_->require_boundary();require_current();
}
void BnwCivisOwnedStockSourceBundle::require_same_host(const ui::SelectedBnwPreviewHost& supplied)const{
 require_current();{
  const auto actual=host_.lock();
  if(!actual||actual.get()!=&supplied)throw std::runtime_error("Own stock selected host allocation differs");
  actual->require_same_civis_resources(*bank_);
 }
 require_current();
}
std::shared_ptr<const ui::SelectedBnwPreviewHost> BnwCivisOwnedStockSourceBundle::retain_host()const{
 require_current();auto actual=host_.lock();
 if(!actual)throw std::runtime_error("Own stock selected host retired");
 require_same_host(*actual);return actual;
}
const assets::CivisResourceBankSourceRecipe& BnwCivisOwnedStockSourceBundle::recipe()const{require_current();return recipe_;}
const assets::ResourceMountTable& BnwCivisOwnedStockSourceBundle::resources()const{
 require_current();return bank_->resources(capability_);
}
assets::CivisLoadedResource BnwCivisOwnedStockSourceBundle::loaded(std::string_view name)const{
 require_current();auto result=capability_.loaded(assets::AssetKey(std::string(name)));
 if(!result.bytes||identify_bytes(*result.bytes).sha256!=result.payload_identity.sha256)
  throw std::runtime_error("Own stock captured source payload differs");
 require_current();return result;
}
std::string BnwCivisOwnedStockSourceBundle::text(std::string_view name,std::string* sha)const{
 const auto value=loaded(name);if(sha)*sha=value.payload_identity.sha256;
 return {reinterpret_cast<const char*>(value.bytes->data()),value.bytes->size()};
}
std::string BnwCivisOwnedStockSourceBundle::key(const std::filesystem::path& path)const{
 require_current();const auto relative=path.lexically_normal().lexically_relative(recipe_.canonical_payload_root.lexically_normal());
 if(relative.empty()||relative.is_absolute())throw std::runtime_error("Own stock source outside captured payload root");
 for(const auto& part:relative)if(part=="..")throw std::runtime_error("Own stock source escapes captured payload root");
 return assets::path_utf8(relative);
}
FileIdentity BnwCivisOwnedStockSourceBundle::source_identity(const std::filesystem::path& path)const{return loaded(key(path)).payload_identity;}
content::XmlNode BnwCivisOwnedStockSourceBundle::source_xml(const std::filesystem::path& path)const{
 const auto value=loaded(key(path));if(!value.xml)throw std::runtime_error("Own stock source XML was not staged");return *value.xml;
}
const ui::CivisResourceBankUiSnapshot& BnwCivisOwnedStockSourceBundle::ui_snapshot()const{require_current();return *ui_;}
const ui::CivisResourceBankFontColorSnapshot& BnwCivisOwnedStockSourceBundle::font_color_snapshot()const{require_current();return *font_;}
}
