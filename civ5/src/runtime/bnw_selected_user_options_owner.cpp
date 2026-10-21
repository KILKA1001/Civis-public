#include "civ5/runtime/bnw_selected_user_options_owner.hpp"
#include "civ5/runtime/bnw_worldview_source_operation_loan.hpp"
#include "civ5/runtime/bnw_selected_worldview_context_owner.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include "civ5/platform/windows_path.hpp"
#include <atomic>
#include <fstream>
#include <locale>
#include <thread>
#include <vector>
#include <iterator>
#include <string_view>
#include <climits>
#include <map>
#include <charconv>
#include <limits>
#include <stdexcept>
namespace civ5::runtime {
namespace {
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
std::vector<std::uint8_t> read_raw(const std::filesystem::path& path){
 need(std::filesystem::is_regular_file(path),"Actual UserSettings.ini changed to a missing file during exact-byte read");
 const auto size=std::filesystem::file_size(path);need(size<=1024*1024,"UserSettings input exceeds bounded native subset");
 std::ifstream input;input.imbue(std::locale::classic());input.open(platform::extended_windows_path(path),std::ios::binary);
 need(bool(input),"Actual UserSettings stream unavailable");std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
 input.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
 need(input.gcount()==static_cast<std::streamsize>(bytes.size())&&input.peek()==std::char_traits<char>::eof()&&!input.bad(),
  "UserSettings source changed during exact-byte read");return bytes;
}
std::string_view trim(std::string_view s){
 while(!s.empty()&&(s.front()==' '||s.front()=='\t'))s.remove_prefix(1);
 while(!s.empty()&&(s.back()==' '||s.back()=='\t'))s.remove_suffix(1);
 return s;
}
bool equal(std::string_view a,std::string_view b){
 if(a.size()!=b.size())return false;
 for(std::size_t i=0;i<a.size();++i){auto x=a[i],y=b[i];if(x>='A'&&x<='Z')x=static_cast<char>(x-'A'+'a');if(y>='A'&&y<='Z')y=static_cast<char>(y-'A'+'a');if(x!=y)return false;}
 return true;
}
static constexpr std::string_view minimap_game_keys[]={
 "NoTileRecommendations","GridOn","ShowTradeOn","ResourceOn","YieldOn"};
struct Parsed {std::optional<bool> saved;std::map<std::string,std::string,std::less<>> pregame;
 std::map<std::string,bool,std::less<>> minimap_game;
 std::optional<std::string> interface_small,interface_auto;};
std::int32_t native_i32_subset(std::string_view value){
 // Canonical finite ASCII decimal subset agrees with native signed-word
 // integer conversion. Empty/plus/overflow/trailing characters stay unknown.
 need(!value.empty()&&value.front()!='+',"Native Pregame integer input outside proven decimal subset");
 std::int32_t result=0;const auto converted=std::from_chars(value.data(),value.data()+value.size(),result,10);
 need(converted.ec==std::errc{}&&converted.ptr==value.data()+value.size(),"Native Pregame integer overflow/syntax outside proven subset");return result;
}
Parsed parse(const std::vector<std::uint8_t>& bytes){
 std::string_view source(reinterpret_cast<const char*>(bytes.data()),bytes.size());
 for(const auto c:source)need((static_cast<unsigned char>(c)>=32&&static_cast<unsigned char>(c)<=126)||c=='\t'||c=='\r'||c=='\n',
  "UserSettings non-ASCII/embedded-NUL input outside native proven subset");
 std::vector<std::string> sections,keys;std::string section;std::optional<bool> found;
 std::map<std::string,std::string,std::less<>> pregame;
 std::optional<std::string> interface_small,interface_auto;
 std::map<std::string,bool,std::less<>> minimap_game;
 std::size_t offset=0;
 while(offset<source.size()){
  const auto end=source.find('\n',offset);auto line=source.substr(offset,end==source.npos?source.size()-offset:end-offset);
  offset=end==source.npos?source.size():end+1;
  if(!line.empty()&&line.back()=='\r')line.remove_suffix(1);
  need(line.find('\r')==line.npos&&line.size()<1024,"UserSettings CR/line width outside native proven subset");
  line=trim(line);if(line.empty()||line.front()==';')continue;
  if(line.front()=='['){
   need(line.size()>2&&line.back()==']'&&line.find(']',1)==line.size()-1&&line.find('[',1)==line.npos,
    "UserSettings malformed section outside native proven subset");
   // Preserve inner bytes: native does not trim after removing brackets.
   section=std::string(line.substr(1,line.size()-2));
   for(const auto& prior:sections)need(!equal(prior,section),"UserSettings duplicate section first-match branch unsupported");
   sections.push_back(section);keys.clear();continue;
  }
  const auto sep=line.find('=');if(section.empty()||sep==line.npos)continue;
  const auto key=trim(line.substr(0,sep));const auto value=trim(line.substr(sep+1));
  need(!key.empty(),"UserSettings empty key unsupported");
  for(const auto& prior:keys)need(!equal(prior,key),"UserSettings duplicate key first-match branch unsupported");
  keys.emplace_back(key);
  if(equal(section,"UserSettings")&&equal(key,"LastCiv"))pregame.emplace("lastciv",value);
  if(equal(section,"InterfaceSettings")&&equal(key,"SmallUIAssets"))interface_small=std::string(value);
  if(equal(section,"InterfaceSettings")&&equal(key,"AutoUIAssets"))interface_auto=std::string(value);
  if(equal(section,"GameSettings"))for(const auto registered:minimap_game_keys)if(equal(key,registered)){
   // Exact0/1 subset of original827770 signed integer ->827A00 nonzero store.
   // Other syntax/overflow remains unported instead of guessed.
   need(value=="0"||value=="1","MiniMap GameSettings bool input outside proven0/1 subset");
   minimap_game.emplace(std::string(registered),value=="1");break;
  }
  if(!equal(section,"GameSettings")||!equal(key,"NoRewardPopups"))continue;
  need(!found,"NoRewardPopups source key duplicated");
  // Native827A00 stores (saved signed atoi value !=0). These explicit 0/1
  // values are a proven finite subset, not cached OptionsManager values.
  need(value=="0"||value=="1","NoRewardPopups atoi/default branch outside explicit boolean subset");found=value=="1";
 }
 return {found,std::move(pregame),std::move(minimap_game),std::move(interface_small),std::move(interface_auto)};
}
struct Input {
 std::optional<FileIdentity> file;
 std::optional<bool> saved;
 std::map<std::string,std::string,std::less<>> pregame;
 std::map<std::string,bool,std::less<>> minimap_game;
 std::optional<std::string> interface_small,interface_auto;
};
Input read_input(const std::filesystem::path& path){
 // Permission/I/O/type failures are never converted into a clean missing file.
 std::error_code error;const auto status=std::filesystem::symlink_status(path,error);
 need(!error||error==std::errc::no_such_file_or_directory,"UserSettings source status unavailable");
 if(status.type()==std::filesystem::file_type::not_found)return {};
 need(std::filesystem::is_regular_file(path),"UserSettings source is not a regular file");
 const auto bytes=read_raw(path);const auto parsed=parse(bytes);return {identify_bytes(bytes),parsed.saved,parsed.pregame,parsed.minimap_game,parsed.interface_small,parsed.interface_auto};
}
bool same(const Input& a,const Input& b){
 if(a.file.has_value()!=b.file.has_value()||a.saved!=b.saved||a.pregame!=b.pregame||a.minimap_game!=b.minimap_game||a.interface_small!=b.interface_small||a.interface_auto!=b.interface_auto)return false;
 return !a.file||(a.file->bytes==b.file->bytes&&a.file->sha256==b.file->sha256);
}
}
struct BnwSelectedUserOptionsOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::weak_ptr<const BnwSelectedUserOptionsOwner> self;
 bool interface_registered=false,interface_querying=false;
 bool interface_small_value=false,interface_auto_value=false;
 std::uint64_t interface_revision=0;
 std::weak_ptr<const ui::BnwNativeResourceBootSettingsOwner> boot;
 const ui::BnwNativeResourceBootSettingsOwner* boot_identity=nullptr;
 std::filesystem::path logical,selected;
 Input input;
 // Exact registered slot, not a caller supplied bool or cached OptionsManager field.
 struct RegisteredSlot {std::string section,key;std::int32_t value=0;};
 RegisteredSlot slot{"GameSettings","NoRewardPopups",0};
 bool default_inserted=false;
 struct GameSlot {bool value;bool default_inserted;};
 std::map<std::string,GameSlot,std::less<>> minimap_game_slots;
 struct PregameSlot {std::string section,key;std::int32_t value;bool default_inserted;};
 std::map<std::string,PregameSlot,std::less<>> pregame_slots;
 bool setup_getter_active=false;
 mutable std::atomic<bool> retired{false};mutable bool busy=false;
};
#include "bnw_user_options_context_small_ui_policy_core.inc"
BnwSelectedUserOptionsOwner::BnwSelectedUserOptionsOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwSelectedUserOptionsOwner::~BnwSelectedUserOptionsOwner(){impl_->retired.store(true);}
std::shared_ptr<const BnwSelectedUserOptionsOwner> BnwSelectedUserOptionsOwner::capture_from_boot(
 const std::shared_ptr<const ui::BnwNativeResourceBootSettingsOwner>& actual){
 need(bool(actual),"User options actual closed Boot factory absent");actual->validate_source();
 auto p=std::make_shared<Impl>();p->boot=actual;p->boot_identity=actual.get();
 const auto profile=actual->selected_language_profile_path_without_parent_callbacks();
 p->logical=profile.parent_path()/L"UserSettings.ini";p->selected=std::filesystem::weakly_canonical(p->logical);
 const auto before=read_input(p->selected);p->input=before;
 // Native827A00: lookup827770; on absence call827EB0(default0), then
 // store the registered default byte even if insertion returns false.
 // Port keeps the actual registered slot in its fresh owned registry. Dirty
 // persistence is addressed to a future Civis profile, never the original file.
 p->slot.value=before.saved.has_value()?(*before.saved?1:0):0;
 p->default_inserted=!before.saved.has_value();
 // Actual cold471651/47177D/4717C8/4718A9/4718F4 ->827A00.
 // Their missing-input default is proven explicit0, unlike Commit443D50.
 // Publish these genuine current fields on this SAME actual Boot/Options actor.
 for(const auto key:minimap_game_keys){
  const auto at=before.minimap_game.find(key);const bool missing=at==before.minimap_game.end();
  p->minimap_game_slots.emplace(std::string(key),Impl::GameSlot{missing?false:at->second,missing});
 }

 actual->validate_source();need(actual->selected_language_profile_path_without_parent_callbacks()==profile&&
  std::filesystem::weakly_canonical(p->logical)==p->selected&&same(read_input(p->selected),before),
  "User options source changed during closed factory");
 auto result=std::shared_ptr<const BnwSelectedUserOptionsOwner>(new BnwSelectedUserOptionsOwner(std::move(p)));
 result->impl_->self=result;result->require_current();return result;
}
void BnwSelectedUserOptionsOwner::require_current()const{
 const auto p=impl_;need(p->thread==std::this_thread::get_id(),"User options foreign creator thread");
 need(!p->retired.load(),"User options source retired");
 if(p->busy){p->retired.store(true);throw std::runtime_error("User options source reentry");}
 p->busy=true;struct End{Impl& p;~End(){p.busy=false;}}end{*p};
 try{
  const auto boot=p->boot.lock();need(boot&&boot.get()==p->boot_identity,"User options actual Boot retired/substituted");
  boot->validate_source();const auto path=boot->selected_language_profile_path_without_parent_callbacks().parent_path()/L"UserSettings.ini";
  need(path==p->logical&&std::filesystem::weakly_canonical(path)==p->selected,"User options actual profile/selected target changed");
  const auto input=read_input(p->selected);
  need(same(input,p->input)&&p->slot.section=="GameSettings"&&p->slot.key=="NoRewardPopups"&&
   p->slot.value==(input.saved.has_value()?(*input.saved?1:0):0)&&
   p->default_inserted==!input.saved.has_value(),"User options source bytes/native registered slot changed");
  need(p->minimap_game_slots.size()==std::size(minimap_game_keys),"MiniMap actual current parameter publication incomplete");
  for(const auto key:minimap_game_keys){
   const auto slot=p->minimap_game_slots.find(key);const auto stored=input.minimap_game.find(key);
   const bool missing=stored==input.minimap_game.end();
   need(slot!=p->minimap_game_slots.end()&&slot->second.default_inserted==missing&&
    slot->second.value==(missing?false:stored->second),"MiniMap actual current GameSettings parameter changed");
  }
  for(const auto& [name,slot]:p->pregame_slots){
   need(name=="lastciv"&&slot.section=="UserSettings"&&slot.key=="LastCiv","Actual Pregame registry key/section changed");
   const auto saved=input.pregame.find(name);const bool missing=saved==input.pregame.end();
   need(slot.default_inserted==missing&&slot.value==(missing?-1:native_i32_subset(saved->second)),"Actual Pregame registered source value changed");
  }
  boot->validate_source();need(!p->retired.load(),"User options retired during currentness");
 }catch(...){p->retired.store(true);throw;}
}
bool BnwSelectedUserOptionsOwner::minimap_game_setting_owned(std::string_view key)const{
 require_current();const auto held=impl_;const auto canonical=held->self.lock();
 need(canonical&&canonical.get()==this&&canonical->impl_.get()==held.get()&&
  !canonical->impl_.owner_before(held)&&!held.owner_before(canonical->impl_),
  "MiniMap actual UserOptions canonical field storage changed");
 const auto at=held->minimap_game_slots.find(key);
 need(at!=held->minimap_game_slots.end(),"MiniMap unqualified GameSettings current field");
 const bool value=at->second.value;require_current();return value;
}
BnwUserOptionsSourceReceipt BnwSelectedUserOptionsOwner::source_receipt()const{
 require_current();BnwUserOptionsSourceReceipt result;
 result.kind=impl_->input.file?BnwUserOptionsInputKind::existing_file:BnwUserOptionsInputKind::absent_file;
 result.logical_path=impl_->logical;result.selected_path=impl_->selected;result.file=impl_->input.file;
 result.default_inserted=impl_->default_inserted;
 for(const auto& [name,slot]:impl_->pregame_slots){(void)name;result.default_inserted=result.default_inserted||slot.default_inserted;}
 for(const auto&[name,slot]:impl_->minimap_game_slots){(void)name;result.default_inserted=result.default_inserted||slot.default_inserted;}
 result.pending_civis_profile_persistence=result.default_inserted;
 require_current();return result;
}

void BnwSelectedUserOptionsOwner::require_same_game_input(const BnwSelectedLanguageInputOwner& input)const {
 const auto p=impl_;require_current();const auto boot=p->boot.lock();
 need(boot&&boot.get()==p->boot_identity,"Pregame settings lost SAME original Boot");
 input.require_same_boot(*boot);require_current();
}
std::int32_t BnwSelectedUserOptionsOwner::last_civilization()const {
 const auto p=impl_;require_current();
 if(p->setup_getter_active){p->retired.store(true);throw std::runtime_error("Native Pregame settings registry getter reentry");}
 p->setup_getter_active=true;struct End{Impl& p;~End(){p.setup_getter_active=false;}}end{*p};
 try {
  // Actual LoadPreGameSettings first setting: [UserSettings] LastCiv,
  // default signed word -1. Register only when THIS native read is reached.
  auto it=p->pregame_slots.find("lastciv");
  if(it==p->pregame_slots.end()) {
   const auto saved=p->input.pregame.find("lastciv");
   const bool missing=saved==p->input.pregame.end();
   const auto value=missing?-1:native_i32_subset(saved->second);
   require_current();
   it=p->pregame_slots.emplace("lastciv",Impl::PregameSlot{"UserSettings","LastCiv",value,missing}).first;
  }
  const auto value=it->second.value;require_current();return value;
 }catch(...){p->retired.store(true);throw;}
}
std::vector<BnwUserOptionsPregameSlotObservation> BnwSelectedUserOptionsOwner::pregame_input_observation()const {
 require_current();std::vector<BnwUserOptionsPregameSlotObservation> result;
 for(const auto& [name,slot]:impl_->pregame_slots){(void)name;result.push_back({slot.section,slot.key,slot.value,slot.default_inserted});}
 require_current();return result;
}

bool BnwSelectedUserOptionsOwner::no_reward_popups()const{require_current();const bool value=impl_->slot.value!=0;require_current();return value;}
void BnwSelectedUserOptionsOwner::require_same_boot(const ui::BnwNativeResourceBootSettingsOwner& boot)const{
 require_current();need(&boot==impl_->boot_identity,"User options another genuine Boot instance");require_current();
}
}
