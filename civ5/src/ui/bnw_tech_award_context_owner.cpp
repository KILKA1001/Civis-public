#include "civ5/ui/bnw_tech_award_context_owner.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/bnw_tech_award_input_dispatch.hpp"
#include "civ5/runtime/bnw_major_tech_transport_consumer.hpp"
#include "civ5/runtime/bnw_barbarian_team_has_tech.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/assets/dds.hpp"
#include "civ5/assets/ui_texture_binding.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include "civ5/ui/top_panel.hpp"
#include "civ5/ui/civis_locale_reference_text.hpp"
#include "civ5/content/game_data_xml.hpp"
#include "civ5/content/saved_rules.hpp"
#include <set>
#include <charconv>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include "civ5/ui/native_extended_font_style.hpp"
#include <array>
#include <atomic>
#include <climits>
#include <map>
#include <optional>
#include <thread>
#include <utility>
namespace civ5::ui {
namespace {
void need(bool ok,const char* why){if(!ok)throw runtime::BnwMajorTechTransportUnavailable(why);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"TechAward native operand overflow");return static_cast<std::int32_t>(n);}
struct Texture {assets::AssetHandle source;std::string sha;std::uint32_t width,height;};
}
struct BnwTechAwardContextOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 mutable std::atomic<bool> retired{false},busy{false};
 std::weak_ptr<const BnwTechAwardContextOwner> self;
 std::weak_ptr<const runtime::BnwMajorTechTransportRequest> request;
 std::weak_ptr<const BnwTechAwardInputDispatch> input_dispatch;
 std::weak_ptr<const SelectedBnwPreviewHost> host;
 // Borrow ONLY from the real retained Bootstrap member; no caller lifetime or
 // Host-owning std::function is copied into persistent VM control storage.
 const assets::ResourceMountTable* resources=nullptr;
 std::optional<assets::AssetHandle> xml,styles;
 struct MeasuredFontSource {assets::AssetHandle handle;std::string sha;};
 std::map<std::string,MeasuredFontSource> measured_font_sources;
 std::map<std::string,BnwTechAwardControlValue> controls;
 std::map<std::string,content::XmlNode> authored;
 std::vector<std::pair<std::string,content::XmlNode>> authored_labels;
 std::string grid_path;
 content::XmlNode context_document;
 std::map<std::string,std::shared_ptr<const assets::UiTextureBinding>> layout_textures;
 std::map<std::string,Texture> textures;
 BnwTechAwardContextObservation observed;
 struct TextSource {assets::AssetHandle asset;std::string sha;};
 struct TextOrigin {std::string text;std::size_t source;};
 struct TextManifest {std::filesystem::path path;std::string sha;};
 std::weak_ptr<BnwWorldViewFontSourceScope> font_scope;
 std::string language;
 std::unique_ptr<content::RulesDatabase> locale_database;
 std::vector<TextSource> text_sources;
 std::vector<TextManifest> text_manifests;
 std::map<std::string,TextOrigin> text_origins;
 std::set<std::string> ambiguous_text;
 bool locale_attempted=false;
  mutable std::size_t animation_pins=0;
  struct AlphaState {
   std::vector<std::pair<std::string,std::string>> profile;
   float progress=0,remaining=0,alpha=.99f;bool reverse=false;
  };
  std::map<std::string,AlphaState> alpha_states;
  struct MouseMember {std::string wrapper,parent;bool hide=false;};
  std::map<std::string,MouseMember> mouse_members;
  std::map<std::string,bool> mouse_parents;
  bool alpha_initialized=false;
  std::weak_ptr<const BnwTechAwardAnimationSample> last_alpha_sample;
 std::set<std::string> click_bindings;
 bool input_binding=false,showhide_binding=false;
 bool showhide_required=false,shown_received=false,processed_received=false;
 std::int32_t timer_owned=0;


 struct Guard {
  const Impl& p;
  explicit Guard(const Impl& q):p(q){
   if(q.thread!=std::this_thread::get_id()){q.retired.store(true);throw runtime::BnwMajorTechTransportUnavailable("TechAward control foreign thread");}
   need(!q.retired.load(),"TechAward control retired");bool expected=false;
   if(!q.busy.compare_exchange_strong(expected,true)){q.retired.store(true);throw runtime::BnwMajorTechTransportUnavailable("TechAward control reentry");}
  }
  ~Guard(){p.busy.store(false);}
 };
 void count(){need(observed.setter_occurrences<UINT64_MAX,"TechAward setter occurrence exhausted");++observed.setter_occurrences;}
 BnwTechAwardControlValue& control(std::string_view id){const auto at=controls.find(std::string(id));need(at!=controls.end(),"TechAward unknown selected control");return at->second;}
};
BnwTechAwardContextOwner::BnwTechAwardContextOwner(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwTechAwardContextOwner::~BnwTechAwardContextOwner(){impl_->retired.store(true);}
std::shared_ptr<const BnwTechAwardContextOwner> BnwTechAwardContextOwner::retain_canonical(
 const std::shared_ptr<const BnwTechAwardContextOwner>& supplied)const{
 if(impl_->thread!=std::this_thread::get_id()){impl_->retired.store(true);throw runtime::BnwMajorTechTransportUnavailable("TechAward binding foreign thread");}
 const auto canonical=impl_->self.lock();
 need(canonical&&canonical.get()==this&&supplied.get()==this&&
  !canonical.owner_before(supplied)&&!supplied.owner_before(canonical),
  "TechAward binding no-op/foreign shared control");
 canonical->require_current();return canonical;
}
void BnwTechAwardContextOwner::require_current()const{
 const auto& p=*impl_;
 if(p.thread!=std::this_thread::get_id()){p.retired.store(true);throw runtime::BnwMajorTechTransportUnavailable("TechAward context foreign thread");}
 try{
  need(!p.retired.load(),"TechAward context retired");const auto source=p.request.lock();
  need(bool(source),"TechAward actual request expired");source->require_context_mutation_current();
  const auto actual=source->retain_mutation_host_for_context();const auto host=p.host.lock();
  need(actual&&host&&actual.get()==host.get()&&!actual.owner_before(host)&&!host.owner_before(actual),
   "TechAward canonical Host changed");
  need(p.xml&&runtime::identify_bytes(p.xml->read_bytes(2*1024*1024)).sha256==p.observed.xml_sha256,
   "TechAward selected XML changed");
  if(p.styles)need(runtime::identify_bytes(p.styles->read_bytes(8*1024*1024)).sha256==p.observed.styles_sha256,
                  "TechAward selected Styles changed");
  source->require_context_mutation_current();
 }catch(...){p.retired.store(true);throw;}
}
std::shared_ptr<const BnwTechAwardContextOwner> BnwTechAwardContextOwner::create(
 std::shared_ptr<const runtime::BnwMajorTechTransportRequest> request,const assets::ResourceMountTable& resources){
 need(bool(request),"TechAward actual opaque request unavailable");request->require_current();
 const auto event=request->event();need(event.kind==runtime::BnwMajorTechUiTransportKind::popup_add&&
  event.native_name=="BUTTONPOPUP_TECH_AWARD"&&event.data3==event.technology&&event.data1==event.player,
  "TechAward context requested by another native event");
 const auto host=request->retain_selected_host_for_context();
 auto p=std::make_unique<Impl>();p->request=request;p->host=host;p->resources=&resources;
 const auto resolved=resources.resolve(assets::AssetKey("TechAwardPopup.xml"));
 request->require_current();const auto* handle=std::get_if<assets::AssetHandle>(&resolved.asset);
 need(handle,"TechAward selected VFS XML unavailable/ambiguous");p->xml=*handle;
 const auto bytes=handle->read_bytes(2*1024*1024);p->observed.xml_sha256=runtime::identify_bytes(bytes).sha256;
 const auto document=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));
 need(document.name=="Context","TechAward selected XML is not a Context");p->context_document=document;
 const auto walk=[&](const auto& visit,const content::XmlNode& node,std::string path)->void{
  const auto id=node.attribute("ID");
  if(node.name=="Label")p->authored_labels.emplace_back(path,node);
  if(id=="OuterGrid")p->grid_path=path;
  if(!id.empty()){
   need(!p->controls.contains(id),"TechAward duplicate selected XML ID");
   const auto hidden=node.attribute("Hidden");need(hidden.empty()||hidden=="0"||hidden=="1","TechAward Hidden schema unknown");
   p->controls.emplace(id,BnwTechAwardControlValue{id,node.name,node.attribute("String"),node.attribute("ToolTip"),node.attribute("Texture"),{},hidden=="1",0,0,std::nullopt,std::nullopt,std::nullopt,std::nullopt});
   p->authored.emplace(id,node);
   const auto source_crop=node.attribute("TextureOffset");
   if(!source_crop.empty()) {
    const auto split=source_crop.find_first_of(",.");need(split!=source_crop.npos&&source_crop.find_first_of(",.",split+1)==source_crop.npos,"TechAward selected texture offset schema unknown");
    const auto parse=[&](std::string_view value){std::int32_t n=0;const auto r=std::from_chars(value.data(),value.data()+value.size(),n);need(r.ec==std::errc{}&&r.ptr==value.data()+value.size()&&n>=0,"TechAward selected texture offset invalid");return n;};
    p->control(id).texture_x=parse(std::string_view(source_crop).substr(0,split));p->control(id).texture_y=parse(std::string_view(source_crop).substr(split+1));
   }
  }
  for(std::size_t n=0;n<node.children.size();++n)visit(visit,node.children[n],path+"/"+std::to_string(n)+":"+node.children[n].name);
 };
 walk(walk,document,"Context");
 for(const auto* id:{"TechName","TechQuote","TechHelp"})need(p->control(id).kind=="Label","TechAward selected text receiver kind differs");
 for(const auto* id:{"B1","B2","B3","B4","B5"})need(p->control(id).kind=="Button"&&
  p->authored.at(id).attribute("Size")=="64,64","TechAward selected bonus button extent differs");
 for(const auto* id:{"B1","B2","B3","B4","B5"}){p->control(id).width=64;p->control(id).height=64;}
 need(p->authored.at("TechIcon").attribute("Size")=="128.128","TechAward selected main icon extent differs");
 p->control("TechIcon").width=128;p->control("TechIcon").height=128;
 need(p->control("BonusStack").kind=="Stack"&&p->authored.at("BonusStack").attribute("StackGrowth")=="Right"&&
  p->authored.at("BonusStack").attribute("Padding")=="-12","TechAward selected bonus stack differs");
 need(p->control("OuterGrid").kind=="Grid"&&p->control("TechIcon").kind=="Image"&&
  p->control("ContinueButton").kind=="GridButton"&&p->control("CloseButton").kind=="Button",
  "TechAward selected modal controls differ");
 p->observed.pending="Real selected TechAward controls created; stock Lua/context registration and actual font layout/PopupPriority required";
 request->require_current();auto result=std::shared_ptr<const BnwTechAwardContextOwner>(new BnwTechAwardContextOwner(std::move(p)));
 result->impl_->self=result;result->require_current();request->register_actual_context_owner(result);result->require_current();return result;
}
BnwTechAwardContextObservation BnwTechAwardContextOwner::observation()const{
 require_current();auto result=impl_->observed;result.controls.clear();
 for(const auto& [id,value]:impl_->controls){(void)id;result.controls.push_back(value);}require_current();return result;
}
std::int32_t BnwTechAwardContextOwner::active_player()const{
 require_current();const auto host=impl_->host.lock();need(bool(host),"TechAward Host expired");
 const auto n=runtime::detail::bnw_game_active_player_read(host->session().state());require_current();return n;
}
std::int32_t BnwTechAwardContextOwner::active_team()const{
 require_current();const auto host=impl_->host.lock();need(bool(host),"TechAward Host expired");
 const auto n=runtime::detail::bnw_game_active_team_read(host->session().state());require_current();return n;
}
std::vector<std::string> BnwTechAwardContextOwner::control_ids()const{
 require_current();std::vector<std::string> ids;for(const auto& [id,value]:impl_->controls){(void)value;ids.push_back(id);}require_current();return ids;
}
std::string BnwTechAwardContextOwner::control_kind(std::string_view id)const{require_current();auto kind=impl_->control(id).kind;require_current();return kind;}
void BnwTechAwardContextOwner::set_text(std::string_view id,std::string_view text)const{
 Impl::Guard guard(*impl_);require_current();auto& control=impl_->control(id);
 need(control.kind=="Label","TechAward SetText receiver is not a selected Label");std::string prepared(text);
 require_current();impl_->count();control.text.swap(prepared);impl_->observed.auto_size_complete=false;require_current();
}
void BnwTechAwardContextOwner::set_tooltip(std::string_view id,std::string_view text)const{
 Impl::Guard guard(*impl_);require_current();auto& control=impl_->control(id);std::string prepared(text);
 require_current();impl_->count();control.tooltip.swap(prepared);require_current();
}
void BnwTechAwardContextOwner::set_texture(std::string_view id,std::string_view key)const{
 Impl::Guard guard(*impl_);require_current();auto& control=impl_->control(id);
 need(control.kind=="Button"||control.kind=="Image","TechAward SetTexture selected receiver kind differs");
 const auto resolved=impl_->resources->resolve(assets::AssetKey(std::string(key)));require_current();
 const auto* handle=std::get_if<assets::AssetHandle>(&resolved.asset);need(handle,"TechAward selected icon texture unavailable/ambiguous");
 const auto bytes=handle->read_bytes(64ULL*1024*1024);const auto sha=runtime::identify_bytes(bytes).sha256;
 const auto dds=assets::DdsData::parse(bytes,assets::NumericDdsProfile::legacy_d3d9,assets::DdsHeaderProfile::civ5_d3dx_size24);
 Texture prepared{*handle,sha,dds.width(),dds.height()};std::string name(key),hash=sha;
 auto next=impl_->textures;next.insert_or_assign(std::string(id),std::move(prepared));require_current();
 impl_->count();impl_->textures.swap(next);control.texture.swap(name);control.texture_sha256.swap(hash);require_current();
}
void BnwTechAwardContextOwner::set_texture_offset(std::string_view id,std::int64_t x,std::int64_t y)const{
 Impl::Guard guard(*impl_);require_current();auto& control=impl_->control(id);
 const auto px=native(x),py=native(y);need(px>=0&&py>=0,"TechAward negative texture crop");
 const auto texture=impl_->textures.find(std::string(id));need(texture!=impl_->textures.end(),"TechAward offset lacks actual selected texture");
 need(control.width&&control.height&&*control.width>0&&*control.height>0&&
  static_cast<std::uint64_t>(px)+static_cast<std::uint64_t>(*control.width)<=texture->second.width&&
  static_cast<std::uint64_t>(py)+static_cast<std::uint64_t>(*control.height)<=texture->second.height,
  "TechAward texture offset outside actual DDS");
 require_current();impl_->count();control.texture_x=px;control.texture_y=py;require_current();
}
void BnwTechAwardContextOwner::set_hide(std::string_view id,bool hidden)const{
 Impl::Guard guard(*impl_);require_current();auto& control=impl_->control(id);require_current();
 impl_->count();control.hidden=hidden;impl_->observed.bonus_size_calculated=false;
 impl_->observed.bonus_anchoring_reprocessed=false;impl_->observed.auto_size_complete=false;require_current();
}
void BnwTechAwardContextOwner::calculate_bonus_stack()const{
 Impl::Guard guard(*impl_);require_current();std::int32_t count=0;
 for(const auto* id:{"B1","B2","B3","B4","B5"})if(!impl_->control(id).hidden)++count;
 const auto width=count?count*64+(count-1)*(-12):0;
 require_current();impl_->count();impl_->observed.bonus_width=width;impl_->observed.bonus_height=count?64:0;
 impl_->observed.bonus_size_calculated=true;impl_->observed.bonus_anchoring_reprocessed=false;require_current();
}
void BnwTechAwardContextOwner::reprocess_bonus_stack()const{
 Impl::Guard guard(*impl_);require_current();need(impl_->observed.bonus_size_calculated,"TechAward anchoring before genuine size calculation");
 // Concrete Civis layout relative to the stack origin, in source child order.
 // Native absolute anchoring/font geometry is not inferred from these values.
 std::array<std::optional<std::int32_t>,5> positions{};std::int32_t cursor=0;
 const std::array<const char*,5> names{"B1","B2","B3","B4","B5"};
 for(std::size_t n=0;n<names.size();++n)if(!impl_->control(names[n]).hidden){positions[n]=cursor;cursor+=52;}
 require_current();impl_->count();
 for(std::size_t n=0;n<names.size();++n){auto& control=impl_->control(names[n]);control.local_x=positions[n];control.local_y=positions[n]?std::optional<std::int32_t>{0}:std::nullopt;}
 impl_->observed.bonus_anchoring_reprocessed=true;require_current();
}
void BnwTechAwardContextOwner::auto_size()const{
 Impl::Guard guard(*impl_);require_current();
 need(impl_->observed.bonus_size_calculated&&impl_->observed.bonus_anchoring_reprocessed,"TechAward auto-size before actual bonus stack layout");
 // Execute actual selected authored Label measurement in XML order. Each
 // completed receiver writes genuine dynamic extents; a later unsupported
 // receiver leaves an observable completed prefix, never full auto-size.
 impl_->observed.measured_labels.clear();
 for(const auto& [path,node]:impl_->authored_labels) {
  const auto id=node.attribute("ID");
  auto actual_text=id.empty()?node.attribute("String"):impl_->control(id).text;
  if(actual_text.starts_with("TXT_KEY_"))actual_text=convert_text_key_owned(actual_text,{});
  const auto measured=measure_authored_plain_label(path,node,std::move(actual_text));
  auto next=impl_->observed.measured_labels;next.push_back(measured);
  require_current();impl_->count();
  if(!id.empty()) {impl_->control(id).width=measured.layout.width;impl_->control(id).height=measured.layout.height;}
  impl_->observed.measured_labels.swap(next);require_current();
 }
 const auto grid=measure_grid_nominal();require_current();impl_->count();
 impl_->control("OuterGrid").width=grid[0];impl_->control("OuterGrid").height=grid[1];
 impl_->observed.measured_grid_width=grid[0];impl_->observed.measured_grid_height=grid[1];
 // Port the real anchor/bounds/recursive child effect using the actual Civis
 // viewport. It does not assert retail GUIglobal dimension identity.
 const auto request=impl_->request.lock();need(bool(request),"TechAward actual layout request expired");
 const auto snapshot=request->retained_ui_snapshot_for_context();require_current();
 const auto* stamp=snapshot.find<BnwClientFrameStamp>();need(stamp&&stamp->viewport.drawable(),"TechAward layout lacks actual current FrameOwner viewport");
 const auto publication=*stamp;const auto host=impl_->host.lock();
 need(host&&publication.identity.ui_source_revision==host->session().state().ui_source_revision&&
      publication.identity.player==active_player()&&publication.identity.team==active_team(),
      "TechAward layout FrameOwner/source recipient differs");
 prepare_client_layout_resources();require_current();
 const auto layout=project_client_layout(publication.viewport);require_current();
 initialize_selected_alpha(*layout);require_current();
 impl_->count();impl_->observed.client_layout=layout;impl_->observed.auto_size_complete=true;
 impl_->observed.pending="Actual Civis selected context anchors/bounds and nine-slice data completed; genuine compositor/presentation required";
 require_current();
}
void BnwTechAwardContextOwner::queue_popup()const{
 Impl::Guard guard(*impl_);require_current();
 need(impl_->observed.auto_size_complete,"TechAward QueuePopup before genuine completed control layout");
 need(impl_->click_bindings.size()==2&&impl_->input_binding&&impl_->showhide_binding,
      "TechAward QueuePopup before original input/showhide registration");
 need(!impl_->observed.queued,"TechAward duplicate QueuePopup occurrence");
 const auto request=impl_->request.lock();need(bool(request),"TechAward popup source expired");
 request->apply_context_effect(runtime::BnwTechAwardContextEffect::queue);
 impl_->observed.queued=true;impl_->observed.visible=true;impl_->showhide_required=true;
 impl_->shown_received=false;impl_->processed_received=false;require_current();
 // Caller is the authentic SAME-VM QueuePopup C handler. It MUST now invoke
 // the retained original ShowHide(false,false) before returning to OnPopup.

}
void BnwTechAwardContextOwner::dequeue_popup()const{
 Impl::Guard guard(*impl_);require_current();
 if(!impl_->observed.queued)return; // genuine initialized-hidden no-entry route
 need(!impl_->showhide_required,"TechAward Dequeue before completed ShowHide");
 const auto request=impl_->request.lock();need(bool(request),"TechAward popup source expired");
 request->apply_context_effect(runtime::BnwTechAwardContextEffect::dequeue);
 impl_->observed.queued=false;impl_->observed.visible=false;impl_->showhide_required=true;
 impl_->processed_received=false;require_current();

}
void BnwTechAwardContextOwner::increment_timer_semaphore()const{
 Impl::Guard guard(*impl_);require_current();
 need(impl_->showhide_required&&impl_->observed.visible,"TechAward timer increment outside actual ShowHide show");
 const auto request=impl_->request.lock();need(bool(request),"TechAward timer source expired");
 request->apply_context_effect(runtime::BnwTechAwardContextEffect::timer_increment);require_current();

}
void BnwTechAwardContextOwner::decrement_timer_semaphore()const{
 Impl::Guard guard(*impl_);require_current();
 need(impl_->showhide_required&&!impl_->observed.visible,"TechAward timer decrement outside actual ShowHide hide");
 const auto request=impl_->request.lock();need(bool(request),"TechAward timer source expired");
 request->apply_context_effect(runtime::BnwTechAwardContextEffect::timer_decrement);require_current();

}
void BnwTechAwardContextOwner::popup_shown()const{
 Impl::Guard guard(*impl_);require_current();
 need(impl_->showhide_required&&impl_->observed.visible&&!impl_->shown_received,"TechAward unsolicited/repeated PopupShown");
 const auto request=impl_->request.lock();need(bool(request),"TechAward shown source expired");
 request->apply_context_effect(runtime::BnwTechAwardContextEffect::shown);
 impl_->shown_received=true;require_current();

}
void BnwTechAwardContextOwner::popup_processed(std::int64_t popup_type,std::int64_t result)const{
 Impl::Guard guard(*impl_);require_current();
 need(native(popup_type)==native_popup_type()&&native(result)==0&&impl_->showhide_required&&
  !impl_->observed.visible&&!impl_->processed_received,"TechAward unsolicited/wrong PopupProcessed");
 const auto request=impl_->request.lock();need(bool(request),"TechAward processed source expired");
 request->apply_context_effect(runtime::BnwTechAwardContextEffect::processed);
 impl_->processed_received=true;require_current();
}
bool BnwTechAwardContextOwner::hidden()const{require_current();return !impl_->observed.visible;}
}

namespace civ5::ui {
namespace {
std::optional<std::string> text_field(const content::XmlNode& row,const std::string& name) {
 if(const auto* field=row.child(name)) {need(field->children.empty(),"TechAward nested localization grammar unsupported");return field->text;}
 for(const auto& [key,value]:row.attributes)if(key==name)return value;
 return std::nullopt;
}
}
void BnwTechAwardContextOwner::bind_selected_localization(const std::filesystem::path& root,
 const runtime::SavedContentProfile& input_profile,const content::Catalog& input_catalog)const {
 Impl::Guard guard(*impl_);require_current();
 need(!impl_->locale_attempted,"TechAward localization reinitialization refused");
 impl_->locale_attempted=true; // before any source/DB/registry work
 try {
  const auto selected_root=root;const auto profile=input_profile;const auto catalog=input_catalog;
  const auto request=impl_->request.lock();need(bool(request),"TechAward localization request expired");
  const auto host=request->retain_selected_host_for_context();
  auto database=host->database().clone_gameplay();
  const auto actual_ui=request->retained_ui_snapshot_for_context();
  const auto scope=BnwWorldViewFontSourceScope::from_actual_tech_context_snapshot(actual_ui);
  const std::string language=scope->locale().language;
  need(language=="en_US"||language=="ru_RU","TechAward selected language unsupported");
  need(profile.can_activate&&profile.all_requirements_located&&profile.files_complete&&profile.enabled_mods.empty(),
       "TechAward actual stock TextData selection incomplete");
  std::vector<Impl::TextManifest> manifests;
  std::set<std::size_t> seen;
  for(const auto index:profile.enabled_dlc) {
   need(index<catalog.packages.size()&&seen.insert(index).second,"TechAward duplicated selected DLC");
   const auto& p=catalog.packages[index];const auto identity=runtime::identify_file(p.path);
   const auto actual=content::read_manifest(p.path);
   content::PackageResourceQuery query;query.selector="TextData";query.enabled_categories={true,true,true};
   const auto expected=content::package_resources(p,query),found=content::package_resources(actual,query);
   need(actual.kind==p.kind&&content::canonical_guid(actual.id)==content::canonical_guid(p.id)&&actual.version==p.version&&expected.size()==found.size(),"TechAward activated TextData manifest differs");
   for(std::size_t n=0;n<found.size();++n)need(expected[n].value==found[n].value&&expected[n].category==found[n].category&&expected[n].category_occurrence==found[n].category_occurrence&&expected[n].file_list==found[n].file_list,"TechAward TextData declarations changed");
   need(runtime::identify_file(p.path).sha256==identity.sha256,"TechAward TextData manifest changed during read");
   manifests.push_back({p.path,identity.sha256});
  }
  const auto records=content::selected_localization_sources(selected_root,profile.enabled_dlc,catalog,*impl_->resources);
  const std::string language_node=language=="en_US"?"Language_en_US":"Language_RU_RU";
  std::vector<Impl::TextSource> sources;
  std::map<std::string,Impl::TextOrigin> origins;
  std::set<std::string> ambiguous;
  std::map<std::string,std::optional<Impl::TextOrigin>> layer;
  std::optional<std::size_t> group;
  const auto flush=[&] {
   for(const auto& [tag,value]:layer) {
    if(value){origins.insert_or_assign(tag,*value);ambiguous.erase(tag);}
    else {origins.erase(tag);ambiguous.insert(tag);}
   }
   layer.clear();
  };
  for(const auto& record:records) {
   if(record.package!=group){flush();group=record.package;}
   const auto bytes=record.asset.read_bytes(64ULL*1024*1024);
   const auto doc=content::read_game_data_xml_text(std::string(bytes.begin(),bytes.end()));
   need(doc.name=="GameData","TechAward TextData root differs");
   const content::XmlNode* table=nullptr;
   for(const auto& child:doc.children)if(child.name==language_node){need(!table,"TechAward duplicated language table");table=&child;}
   if(!table)continue; // other languages are not fallback
   const auto n=sources.size();sources.push_back({record.asset,runtime::identify_bytes(bytes).sha256});
   for(const auto& row:table->children) {
    const auto tag=text_field(row,"Tag"),text=text_field(row,"Text");
    need(row.name=="Row"&&tag&&!tag->empty()&&text,"TechAward unsupported TextData row operation");
    if(!layer.emplace(*tag,Impl::TextOrigin{*text,n}).second)layer.at(*tag).reset();
   }
  }
  flush();need(!origins.empty(),"TechAward selected TextData has no rows");
  std::size_t count=0;
  database->query({"SELECT Type FROM Localization.Languages WHERE Type=?",{language}},[&](auto row){need(row.size()==1&&std::get_if<std::string>(&row[0]),"TechAward language metadata malformed");++count;});
  need(count==1,"TechAward activated language absent or duplicated");
  scope->require_current();require_current();
  impl_->font_scope=scope;impl_->language=language;impl_->locale_database=std::move(database);
  impl_->text_sources=std::move(sources);impl_->text_origins=std::move(origins);
  impl_->ambiguous_text=std::move(ambiguous);impl_->text_manifests=std::move(manifests);
  require_current();
 }catch(...){impl_->retired.store(true);throw;}
}
std::string BnwTechAwardContextOwner::convert_text_key(std::string_view input,std::span<const runtime::ScriptValue> input_args)const {
 Impl::Guard guard(*impl_);return convert_text_key_owned(input,input_args);
}
std::string BnwTechAwardContextOwner::convert_text_key_owned(std::string_view input,std::span<const runtime::ScriptValue> input_args)const {
 require_current();
 const std::string tag(input);const std::vector<runtime::ScriptValue> args(input_args.begin(),input_args.end());
 const auto scope=impl_->font_scope.lock();need(scope&&impl_->locale_database,"TechAward actual selected locale not bound");
 scope->require_current();need(scope->locale().language==impl_->language,"TechAward selected locale changed");
 const auto raw=[&](std::string_view key) {
  need(!key.empty()&&key.find('\0')==key.npos&&!impl_->ambiguous_text.contains(std::string(key)),"TechAward localized key invalid or ambiguous");
  const auto found=impl_->text_origins.find(std::string(key));need(found!=impl_->text_origins.end(),"TechAward localized key lacks selected source");
  std::optional<std::string> text;std::size_t count=0;
  impl_->locale_database->query({"SELECT Text FROM Localization.LocalizedText WHERE Language=? AND Tag=?",{impl_->language,std::string(key)}},[&](auto row){need(row.size()==1&&std::get_if<std::string>(&row[0]),"TechAward localized DB row malformed");text=std::get<std::string>(row[0]);++count;});
  need(count==1&&text&&*text==found->second.text,"TechAward selected TextData/activated DB mismatch");
  const auto& source=impl_->text_sources.at(found->second.source);
  need(runtime::identify_bytes(source.asset.read_bytes(64ULL*1024*1024)).sha256==source.sha,"TechAward localized source changed");
  return *text;
 };
 for(const auto& manifest:impl_->text_manifests)need(runtime::identify_file(manifest.path).sha256==manifest.sha,"TechAward localized manifest changed");
 auto expanded=expand_civis_locale_references(tag,raw);
 std::vector<std::string> values;
 for(const auto& arg:args) {
  if(const auto* value=std::get_if<std::string>(&arg))values.push_back(value->starts_with("TXT_KEY_")?raw(*value):*value);
  else if(const auto* value=std::get_if<std::int64_t>(&arg))values.push_back(std::to_string(*value));
  else throw runtime::BnwMajorTechTransportUnavailable("TechAward locale parameter needs source-proved string or integer");
 }
 TopPanelLocalizationCatalog resolved;resolved.rows.emplace(tag,std::move(expanded));
 // Bounded existing plain formatter only: never erase native gender/plural
 // selectors and pretend they were evaluated. Ordinary text punctuation is
 // unaffected; only directives inside braces are constrained here.
 const auto& raw_format=resolved.rows.at(tag);
 for(std::size_t p=0;(p=raw_format.find('{',p))!=raw_format.npos;) {
  const auto end=raw_format.find('}',p+1);need(end!=raw_format.npos,"TechAward unterminated locale grammar");
  const auto token=std::string_view(raw_format).substr(p+1,end-p-1);
  need(token.find(':')==token.npos&&token.find('|')==token.npos,"TechAward locale requires native grammar selector owner");p=end+1;
 }
 auto text=resolved.format(tag,values);need(!text.empty()&&text!=tag,"TechAward unresolved localized text");
 auto observations=impl_->observed.localized_queries;observations.emplace_back(tag,text);
 require_current();impl_->observed.language=impl_->language;impl_->observed.localized_queries.swap(observations);
 scope->require_current();require_current();return text;
}
std::int32_t BnwTechAwardContextOwner::popup_priority()const {
 require_current();
 // Exact retail 0x41D2B6 PUSH 41 -> lua_pushinteger(L,41), then
 // PUSH "TechAward"/-2/L -> lua_setfield; table is PopupPriority.
 // This value grants no queue/layout/presentation authority.
 return 41;
}
std::int32_t BnwTechAwardContextOwner::native_popup_type()const {
 require_current();
 // SDK CvEnums.h zero-based ButtonPopupTypes, actual CvLuaEnums.cpp153
 // RegisterEnum(BUTTONPOPUP_TECH_AWARD). Complete preceding enum is pinned.
 return 68;
}
}

namespace civ5::ui {
void BnwTechAwardContextOwner::register_click(std::string_view id,std::int64_t mouse_event)const {
 Impl::Guard guard(*impl_);require_current();
 need(mouse_event==native_mouse_left_click(),"TechAward callback event differs from native Mouse.eLClick");
 need(id=="ContinueButton"||id=="CloseButton","TechAward click source receiver differs");
 const auto kind=impl_->control(id).kind;
 need(kind=="GridButton"||kind=="Button","TechAward callback receiver is not authored button");
 // Allocate real owned native control routing storage before the setter.
 auto next=impl_->click_bindings;need(next.insert(std::string(id)).second,"TechAward original click registered twice");
 const auto request=impl_->request.lock();need(bool(request),"TechAward callback source expired");
 request->register_context_input_slot(id);require_current();
 impl_->click_bindings.swap(next);impl_->count();require_current();
}
void BnwTechAwardContextOwner::register_input_handler()const {
 Impl::Guard guard(*impl_);require_current();need(!impl_->input_binding,"TechAward input source registered twice");
 const auto request=impl_->request.lock();need(bool(request),"TechAward input source expired");
 request->register_context_input_slot("ContextPtr.InputHandler");require_current();
 impl_->input_binding=true;impl_->count();require_current();
}
void BnwTechAwardContextOwner::register_showhide_handler()const {
 Impl::Guard guard(*impl_);require_current();need(!impl_->showhide_binding,"TechAward showhide source registered twice");
 const auto request=impl_->request.lock();need(bool(request),"TechAward showhide source expired");
 request->register_context_input_slot("ContextPtr.ShowHideHandler");require_current();
 impl_->showhide_binding=true;impl_->count();require_current();
}
bool BnwTechAwardContextOwner::requires_showhide()const {require_current();return impl_->showhide_required;}
void BnwTechAwardContextOwner::finish_actual_showhide()const {
 Impl::Guard guard(*impl_);require_current();
 need(impl_->showhide_required&&impl_->showhide_binding,"TechAward unsolicited ShowHide completion");
 need(impl_->observed.visible?impl_->shown_received:impl_->processed_received,
      "TechAward original ShowHide has not applied required timer/native event");
 impl_->showhide_required=false;require_current();
}
}

namespace civ5::ui {
std::int32_t BnwTechAwardContextOwner::native_mouse_left_click()const {
 require_current();
 // Retail 0x41D98C PUSH 0 -> lua_pushinteger, then eLClick -> lua_setfield.
 // The subsequent PUSH 1 belongs to eLDblClick, not eLClick.
 return 0;
}
}

namespace civ5::ui {
BnwTechAwardMeasuredLabel BnwTechAwardContextOwner::measure_authored_plain_label(
 std::string path,const content::XmlNode& input,std::string text)const {
 require_current();const auto declaration=input; // no borrowed node through VFS callbacks
 const auto scope=impl_->font_scope.lock();const auto host=impl_->host.lock();
 need(scope&&host&&impl_->locale_database,"TechAward metric requires genuine bound source locale");
 scope->require_current();
 need(scope->locale().language==impl_->language,"TechAward metric selected language changed");
 const auto language=read_bnw_language_font_row(host->database(),impl_->language);
 const auto flag=[&](std::string_view key){const auto value=declaration.attribute(std::string(key));
  need(value.empty()||value=="0"||value=="1","TechAward authored Label Force flag schema unknown");return value=="1";};
 need(native_extended_font_route({language.use_extended_font,flag("ForceIME"),flag("ForceNonIME")})==NativeExtendedFontRoute::authored_label,
      "TechAward actual extended Label requires SAME-window system-font owner");
 const auto font=declaration.attribute("Font"),style=declaration.attribute("FontStyle");
 need(!font.empty()&&!style.empty(),"TechAward authored font/style inheritance requires actual parent producer");
 const auto metadata=read_glyphgen_style_metadata(*impl_->resources,font,style);require_current();
 const auto font_key="Assets/UI/Fonts/Tw Cent MT/"+font+".ggxml";
 auto font_resolved=impl_->resources->resolve(assets::AssetKey(font_key));require_current();
 if(std::holds_alternative<std::monostate>(font_resolved.asset)) {
  font_resolved=impl_->resources->resolve(assets::AssetKey(assets::resource_basename(font_key)));require_current();
 }
 const auto* font_handle=std::get_if<assets::AssetHandle>(&font_resolved.asset);
 need(font_handle&&runtime::identify_bytes(font_handle->read_bytes(64ULL*1024*1024)).sha256==metadata.font_source.sha256,
      "TechAward selected retained GG source differs from measured metadata");
 auto font_sources=impl_->measured_font_sources;
 font_sources.insert_or_assign(font,Impl::MeasuredFontSource{*font_handle,metadata.font_source.sha256});
 require_current();impl_->measured_font_sources.swap(font_sources);require_current();
 const auto geometry=glyphgen_geometry(metadata.font);
 const auto scalar=[&](std::string_view key,std::int32_t fallback){const auto value=declaration.attribute(std::string(key));
  if(value.empty())return fallback;
  std::int32_t n=0;const auto parsed=std::from_chars(value.data(),value.data()+value.size(),n);
  need(parsed.ec==std::errc{}&&parsed.ptr==value.data()+value.size(),"TechAward authored Label scalar invalid");return n;};
 need(scalar("TruncateWidth",-1)<=0,"TechAward positive truncation requires actual truncation producer");
 // Resolve only actual reached markup against this SAME activated Host.
 // Immutable selected payloads survive queued paint; no Bootstrap pointer is
 // borrowed after the real operation returns.
 BnwTechAwardMarkupSources markup;
 auto retained_sources=impl_->measured_font_sources;
 const auto selected=[&](const std::string& key){
  require_current();auto found=impl_->resources->resolve(assets::AssetKey(key));require_current();
  if(std::holds_alternative<std::monostate>(found.asset)){
   found=impl_->resources->resolve(assets::AssetKey(assets::resource_basename(key)));require_current();
  }
  need(std::holds_alternative<assets::AssetHandle>(found.asset),"TechAward reached markup asset unavailable/ambiguous");return found;
 };
 const auto capture=[&](const assets::AssetHandle& handle,const std::string& key){
  require_current();const auto bytes=handle.read_bytes(64ULL*1024*1024);require_current();
  const auto sha=runtime::identify_bytes(bytes).sha256;
  retained_sources.insert_or_assign(key,Impl::MeasuredFontSource{handle,sha});return std::pair{bytes,sha};
 };
 for(std::size_t at=0;at<text.size();++at){
  if(text[at]!='[')continue;
  if(at+1<text.size()&&text[at+1]=='['){++at;continue;}
  const auto end=text.find(']',at+1);need(end!=text.npos,"TechAward unterminated actual markup");
  const auto tag=text.substr(at+1,end-at-1);at=end;
  if(tag.starts_with("COLOR_")&&!markup.colors.contains(tag)){
   std::size_t rows=0;std::array<std::uint8_t,4> color{};
   host->database().query({"SELECT Red,Green,Blue,Alpha FROM Colors WHERE Type=?",{tag}},[&](auto row){
    need(row.size()==4,"TechAward selected Colors schema changed");
    for(std::size_t i=0;i<4;++i){double value=0;
     if(const auto* d=std::get_if<double>(&row[i]))value=*d;
     else if(const auto* n=std::get_if<std::int64_t>(&row[i]))value=static_cast<double>(*n);
     else need(false,"TechAward selected Color channel not numeric");
     need(std::isfinite(value)&&value>=0&&value<=1,"TechAward selected Color channel outside bounded Civis profile");
     color[i]=static_cast<std::uint8_t>(std::floor(value*255.0+0.5));
    }++rows;
   });require_current();need(rows==1,"TechAward reached selected Color unavailable/ambiguous");markup.colors.emplace(tag,color);
  }
  if(tag.starts_with("ICON_")&&!markup.icons.contains(tag)){
   std::size_t rows=0;std::int64_t code=0;std::string texture_file;
   host->database().query({"SELECT m.IconMapping,t.IconFontTextureFile FROM IconFontMapping m JOIN IconFontTextures t ON m.IconFontTexture=t.IconFontTexture WHERE m.IconName=?",{tag}},[&](auto row){
    need(row.size()==2&&std::get_if<std::int64_t>(&row[0])&&std::get_if<std::string>(&row[1]),"TechAward selected icon mapping schema changed");
    code=std::get<std::int64_t>(row[0]);texture_file=std::get<std::string>(row[1]);++rows;
   });require_current();need(rows==1&&code>0&&code<=65535&&!texture_file.empty(),"TechAward reached selected icon unavailable/ambiguous");
   const auto fm=selected(texture_file+".ggxml");const auto fh=std::get<assets::AssetHandle>(fm.asset);
   const auto fb=capture(fh,"markup/font/"+texture_file);
   auto icon_font=std::make_shared<const GlyphGenFont>(GlyphGenFont::from_xml(content::read_xml_metadata(std::string(fb.first.begin(),fb.first.end()))));
   need(icon_font->textures.size()==1&&icon_font->glyph(static_cast<std::uint32_t>(code)),"TechAward icon font texture/glyph unavailable");
   const auto image=selected(icon_font->textures.front().source());const auto ph=std::get<assets::AssetHandle>(image.asset);
   const auto original=ph.candidate().original_name;
   need(original.size()>=4&&assets::normalize_asset_name(original).substr(original.size()-4)==".dds","TechAward icon packed texture is not DDS");
   const auto index_name=original.substr(0,original.size()-4)+"-index.dds";
   const auto index=selected(index_name);const auto ih=std::get<assets::AssetHandle>(index.asset);
   const auto& a=ph.candidate().provenance;const auto& c=ih.candidate().provenance;
   need(image.mount_id==index.mount_id&&a.kind==c.kind&&a.source_id==c.source_id&&a.archive_relative_path==c.archive_relative_path&&
        assets::normalize_asset_name(ih.candidate().original_name)==assets::normalize_asset_name(index_name),"TechAward icon atlas pair source mismatch");
   auto pb=capture(ph,"markup/packed/"+texture_file),ib=capture(ih,"markup/index/"+texture_file);
   auto atlas=std::make_shared<const GlyphGenRgbaAtlas>(reconstruct_glyphgen_rgba(*icon_font,assets::DdsData::parse(std::move(ib.first)),assets::DdsData::parse(std::move(pb.first))));
   auto icon_geometry=std::make_shared<const GlyphGenGeometry>(glyphgen_geometry(*icon_font));
   need(atlas->width==icon_geometry->atlas_width&&atlas->height==icon_geometry->atlas_height,"TechAward actual icon atlas geometry mismatch");
   markup.icons.emplace(tag,BnwTechAwardMarkupIcon{texture_file,fb.second,std::move(icon_font),std::move(icon_geometry),std::move(atlas),pb.second,ib.second,static_cast<std::uint32_t>(code)});
  }
 }
 auto layout=project_bnw_tech_award_rich_label(geometry,text,scalar("WrapWidth",-1),scalar("LeadingOffset",0),markup);
 for(const auto& [key,source]:retained_sources){(void)key;require_current();
  need(runtime::identify_bytes(source.handle.read_bytes(64ULL*1024*1024)).sha256==source.sha,"TechAward markup source changed during measurement");require_current();
 }
 impl_->measured_font_sources.swap(retained_sources);require_current();
 need(read_bnw_language_font_row(host->database(),impl_->language)==language,"TechAward actual language font row changed");
 scope->require_current();require_current();
 return {std::move(path),declaration.attribute("ID"),std::move(text),font,metadata.font_source.sha256,style,std::move(layout)};
}
}

namespace civ5::ui {
std::array<std::int32_t,2> BnwTechAwardContextOwner::measure_grid_nominal()const {
 require_current();const auto grid=impl_->authored.at("OuterGrid");
 const auto selected=impl_->resources->resolve(assets::AssetKey("Styles.xml"));require_current();
 const auto* handle=std::get_if<assets::AssetHandle>(&selected.asset);
 need(handle,"TechAward actual stylesheet unavailable or ambiguous");
 const auto bytes=handle->read_bytes(8*1024*1024);const auto hash=runtime::identify_bytes(bytes).sha256;
 const auto styles=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));
 need(styles.name=="StyleSheet","TechAward selected stylesheet root differs");
 const auto style=[&](std::string_view name)->const content::XmlNode& {
  const content::XmlNode* value=nullptr;
  for(const auto& n:styles.children)if(n.name==name){need(!value,"TechAward ambiguous stylesheet declaration");value=&n;}
  need(value,"TechAward authored style not selected");return *value;
 };
 const auto scalar=[](std::string_view input)->std::int32_t {
  std::int32_t value=0;const auto parsed=std::from_chars(input.data(),input.data()+input.size(),value);
  need(parsed.ec==std::errc{}&&parsed.ptr==input.data()+input.size()&&value>-32768&&value<32768,
       "TechAward integer source geometry unavailable");return value;
 };
 const auto pair=[&](std::string_view input)->std::array<std::int32_t,2> {
  const auto separator=input.find_first_of(",.");
  need(separator!=input.npos&&input.find_first_of(",.",separator+1)==input.npos,
       "TechAward source pair geometry unavailable");
  return {scalar(input.substr(0,separator)),scalar(input.substr(separator+1))};
 };
 const auto attribute=[&](const content::XmlNode& n,const std::string& key)->std::string {
  const auto authored=n.attribute(key);if(!authored.empty())return authored;
  const auto name=n.attribute("Style");return name.empty()?std::string{}:style(name).attribute(key);
 };
 const auto outside=[&](const content::XmlNode& n){const auto value=attribute(n,"AnchorSide");
  need(value.empty()||value=="I.I"||value=="I.O"||value=="O.I"||value=="O.O",
       "TechAward native outside flag source unknown");
  return value=="I.O"||value=="O.I"||value=="O.O";
 };
 const auto extent=[&](const auto& visit,const content::XmlNode& n,std::string path)->std::array<std::int32_t,2> {
  const auto id=n.attribute("ID");
  if(id=="BonusStack")return {impl_->observed.bonus_width,impl_->observed.bonus_height};
  if(n.name=="Label") {
   const auto found=std::find_if(impl_->observed.measured_labels.begin(),impl_->observed.measured_labels.end(),
    [&](const auto& value){return value.path==path&&value.id==id;});
   if(found!=impl_->observed.measured_labels.end())return {found->layout.width,found->layout.height};
   throw runtime::BnwMajorTechTransportUnavailable("TechAward actual Label metric missing");
  }
  if(n.name=="Stack") {
   need(n.attribute("StackGrowth")=="Bottom"&&n.attribute("Padding")=="0",
        "TechAward actual native Stack direction/padding unsupported");
   std::int64_t width=0,height=0;
   for(std::size_t child_index=0;child_index<n.children.size();++child_index) {
    const auto& child=n.children[child_index];
    const auto cid=child.attribute("ID");const auto hidden=cid.empty()?child.attribute("Hidden")=="1":impl_->control(cid).hidden;
    const auto source_offset=child.attribute("Offset");
    need(source_offset.empty()||pair(source_offset)==std::array<std::int32_t,2>{0,0},
         "TechAward bottom Stack child offset requires native translation receiver");
    const auto size=visit(visit,child,path+"/"+std::to_string(child_index)+":"+child.name);
    if(hidden||size[0]==0||size[1]==0)continue;
    need(!outside(child),"TechAward bottom Stack outside-child bounds receiver required");
    width=std::max(width,std::int64_t{size[0]});height+=size[1];
    need(width<32768&&height<32768,"TechAward native Stack extent overflow");
   }
   return {static_cast<std::int32_t>(width),static_cast<std::int32_t>(height)};
  }
  const auto source_size=attribute(n,"Size");need(!source_size.empty(),"TechAward eligible child lacks actual nominal-size source");
  const auto value=pair(source_size);need(value[0]>=0&&value[1]>=0,"TechAward negative nominal-size source");return value;
 };
 need(grid.attribute("Style")=="Grid9DetailFive140","TechAward selected Grid style profile changed");
 const auto& skin=style(grid.attribute("Style"));need(skin.attribute("Padding")=="0,0","TechAward Grid style padding schema changed");
 std::int64_t width=0,height=0;
 need(!impl_->grid_path.empty(),"TechAward actual Grid path missing");
 for(std::size_t child_index=0;child_index<grid.children.size();++child_index) {
  const auto& child=grid.children[child_index];
  // 563190 deliberately tests only outsideX40/outsideY200, not Hidden2.
  if(outside(child))continue;
  const auto size=extent(extent,child,impl_->grid_path+"/"+std::to_string(child_index)+":"+child.name);const auto source_offset=attribute(child,"Offset");
  const auto offset=source_offset.empty()?std::array<std::int32_t,2>{0,0}:pair(source_offset);
  width=std::max(width,std::int64_t{std::abs(offset[0])}+size[0]);
  height=std::max(height,std::int64_t{std::abs(offset[1])}+size[1]);
 }
 const auto padding=pair(attribute(grid,"Padding"));width+=std::int64_t{padding[0]}*2;height+=std::int64_t{padding[1]}*2;
 need(width>0&&height>0&&width<32768&&height<32768,"TechAward actual Grid auto-size overflow");
 require_current();
 need(runtime::identify_bytes(handle->read_bytes(8*1024*1024)).sha256==hash,"TechAward selected stylesheet changed during metrics");
 impl_->styles=*handle;impl_->observed.styles_sha256=hash;require_current();
 return {static_cast<std::int32_t>(width),static_cast<std::int32_t>(height)};
}
}

namespace civ5::ui {
void BnwTechAwardContextOwner::require_queued_current()const {
 Impl::Guard guard(*impl_);const auto& p=*impl_;
 const auto setter_occurrences=p.observed.setter_occurrences;
 if(p.thread!=std::this_thread::get_id()){p.retired.store(true);throw runtime::BnwMajorTechTransportUnavailable("TechAward queued context foreign thread");}
 try {
  need(!p.retired.load()&&p.observed.queued&&p.observed.visible&&p.observed.auto_size_complete&&!p.showhide_required,
       "TechAward queued context has not completed actual modal ShowHide");
  const auto request=p.request.lock();const auto host=p.host.lock();
  need(request&&host,"TechAward queued actual source expired");
  request->require_context_stored_current();
  const auto actual=request->retain_stored_host_for_context();
  need(actual.get()==host.get()&&!actual.owner_before(host)&&!host.owner_before(actual),
       "TechAward queued canonical Host changed");
  need(p.xml&&runtime::identify_bytes(p.xml->read_bytes(2*1024*1024)).sha256==p.observed.xml_sha256,
       "TechAward queued XML changed");
  need(p.styles&&runtime::identify_bytes(p.styles->read_bytes(8*1024*1024)).sha256==p.observed.styles_sha256,
       "TechAward queued Styles changed");
  const auto scope=p.font_scope.lock();need(bool(scope),"TechAward queued real font source expired");
  scope->require_current();need(scope->locale().language==p.language,"TechAward queued language changed");
  for(const auto& label:p.observed.measured_labels) {
   const auto stored=p.measured_font_sources.find(label.font_key);
   need(stored!=p.measured_font_sources.end()&&stored->second.sha==label.font_sha256&&
        runtime::identify_bytes(stored->second.handle.read_bytes(64ULL*1024*1024)).sha256==label.font_sha256,
        "TechAward queued selected retained font changed");
  }
  for(const auto& [key,source]:p.measured_font_sources){
   (void)key;need(runtime::identify_bytes(source.handle.read_bytes(64ULL*1024*1024)).sha256==source.sha,
                "TechAward queued selected markup/font payload changed");
  }
  for(const auto& origin:p.text_sources)
   need(runtime::identify_bytes(origin.asset.read_bytes(32ULL*1024*1024)).sha256==origin.sha,"TechAward queued localization source changed");
  for(const auto& manifest:p.text_manifests)
   need(runtime::identify_file(manifest.path).sha256==manifest.sha,"TechAward queued localization manifest changed");
  for(const auto& [id,texture]:p.textures) {
   (void)id;need(runtime::identify_bytes(texture.source.read_bytes(64ULL*1024*1024)).sha256==texture.sha,
                "TechAward queued texture changed");
  }
  scope->require_current();request->require_context_stored_current();
  need(!p.retired.load()&&p.observed.setter_occurrences==setter_occurrences,
       "TechAward queued context changed during source checks");
 }catch(...){p.retired.store(true);throw;}
}
std::shared_ptr<const BnwTechAwardContextOwner> BnwTechAwardContextOwner::retain_queued_canonical(
 const std::shared_ptr<const BnwTechAwardContextOwner>& supplied)const {
 if(impl_->thread!=std::this_thread::get_id()){impl_->retired.store(true);throw runtime::BnwMajorTechTransportUnavailable("TechAward queued owner foreign thread");}
 const auto actual=impl_->self.lock();
 need(actual&&actual.get()==this&&supplied.get()==this&&!actual.owner_before(supplied)&&!supplied.owner_before(actual),
      "TechAward queued facade no-op/foreign control alias");
 actual->require_queued_current();return actual;
}
void BnwTechAwardContextOwner::require_same_actual_request(const runtime::BnwMajorTechTransportRequest& request)const {
 require_current();const auto actual=impl_->request.lock();
 need(actual&&actual.get()==&request,"TechAward context belongs to another actual request");require_current();
}
BnwTechAwardContextObservation BnwTechAwardContextOwner::queued_observation(ClientViewport viewport)const {
 require_queued_current();auto result=impl_->observed;result.controls.clear();
 for(const auto& [id,value]:impl_->controls){(void)id;result.controls.push_back(value);}
 result.client_layout=project_client_layout(viewport);require_queued_current();return result;
}
}

namespace civ5::ui {
void BnwTechAwardContextOwner::prepare_client_layout_resources()const {
 require_current();need(bool(impl_->styles),"TechAward actual Styles missing");
 const auto bytes=impl_->styles->read_bytes(8*1024*1024);require_current();
 need(runtime::identify_bytes(bytes).sha256==impl_->observed.styles_sha256,"TechAward actual selected Styles changed");
 const auto styles=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));
 auto prepared=impl_->layout_textures;
 const auto load=[&](const std::string& key){if(key.empty()||prepared.contains(key))return;
  const auto binding=std::make_shared<const assets::UiTextureBinding>(assets::bind_ui_texture(*impl_->resources,key));require_current();prepared.emplace(key,binding);require_current();};
 const auto visit=[&](const auto& self,const content::XmlNode& node)->void {
  load(node.attribute("Texture"));const auto name=node.attribute("Style");
  if(!name.empty()){const content::XmlNode* style=nullptr;for(const auto& candidate:styles.children)if(candidate.name==name){style=&candidate;break;}
   need(style,"TechAward actual layout skin absent");load(style->attribute("Texture"));
   for(const auto& child:style->children)self(self,child);
  }
  for(const auto& child:node.children)self(self,child);
 };
 visit(visit,impl_->context_document);for(const auto& [id,control]:impl_->controls){(void)id;load(control.texture);}
 require_current();impl_->layout_textures.swap(prepared);require_current();
}
std::shared_ptr<const BnwTechAwardContextLayout> BnwTechAwardContextOwner::project_client_layout(ClientViewport viewport)const {
 // Only private actual active-layout / applied-UiHost receivers call this.
 // No ResourceMountTable borrow or gameplay mutation occurs in this method.
 need(bool(impl_->styles),"TechAward retained Styles absent");const auto bytes=impl_->styles->read_bytes(8*1024*1024);
 need(runtime::identify_bytes(bytes).sha256==impl_->observed.styles_sha256,"TechAward retained layout Styles changed");
 const auto styles=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));
 auto value=std::make_shared<BnwTechAwardContextLayout>(project_bnw_tech_award_context_layout(impl_->context_document,styles,impl_->observed,viewport));
 for(auto& node:value->nodes) {
  if(!node.texture.empty()) {const auto selected=impl_->layout_textures.find(node.texture);
   need(selected!=impl_->layout_textures.end(),"TechAward actual selected node texture absent");
   node.selected_texture=selected->second;
  }
 }
 for(const auto& node:value->nodes)if(node.kind=="Grid"||node.kind=="GridData") {
  const content::XmlNode* skin=nullptr;for(const auto& candidate:styles.children)if(candidate.name==node.style){skin=&candidate;break;}
  need(skin,"TechAward selected Grid skin absent");
  const auto texture=impl_->layout_textures.find(skin->attribute("Texture"));need(texture!=impl_->layout_textures.end(),"TechAward selected Grid texture not retained");
  auto slices=project_bnw_tech_award_grid_slices(*skin,node,texture->second->width(),texture->second->height());
  slices.selected_texture=texture->second;value->grids.push_back(std::move(slices));
 }
 return value;
}
}

namespace civ5::ui {
std::shared_ptr<const runtime::BnwMajorTechTransportRequest> BnwTechAwardContextOwner::retained_request_for_input()const {
 require_queued_current();const auto request=impl_->request.lock();need(bool(request),"TechAward queued input request expired");
 request->require_context_stored_current();require_queued_current();return request;
}
void BnwTechAwardContextOwner::begin_queued_input(const std::shared_ptr<const BnwTechAwardInputDispatch>& token)const {
 require_queued_current();need(token&&impl_->input_dispatch.expired(),"TechAward queued input already active");
 token->require_same_actual_context(*this);token->require_active_dispatch();
 impl_->input_dispatch=token;require_current();
}
void BnwTechAwardContextOwner::finish_queued_input(const BnwTechAwardInputDispatch& token)const {
 require_current();const auto input=impl_->input_dispatch.lock();
 need(input.get()==&token,"TechAward queued input completion foreign");token.require_active_dispatch();
 need(!impl_->observed.queued&&!impl_->observed.visible&&!impl_->showhide_required&&impl_->processed_received,
      "TechAward input did not finish original Dequeue/ShowHide/Processed");
 impl_->input_dispatch.reset();
}
void BnwTechAwardContextOwner::fail_queued_input()const noexcept {impl_->retired.store(true);}
}

namespace civ5::ui {
class BnwTechAwardAnimationWitness::Pin {
 public:
  std::shared_ptr<const BnwTechAwardContextOwner> owner;
  explicit Pin(const BnwTechAwardAnimationWitness& witness):owner(witness.owner_.lock()) {
   need(witness.creator_==std::this_thread::get_id()&&owner,"TechAward animation owner expired/foreign thread");
   auto& p=*owner->impl_;need(p.thread==std::this_thread::get_id(),"TechAward animation owner thread changed");
   need(p.animation_pins<SIZE_MAX,"TechAward animation pin overflow");++p.animation_pins;
   try{pure();}catch(...){--p.animation_pins;throw;}
  }
  ~Pin(){--owner->impl_->animation_pins;}
  void pure()const {
   const auto& p=*owner->impl_;const auto self=p.self.lock();
   need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&self&&self.get()==owner.get()&&
    !self.owner_before(owner)&&!owner.owner_before(self)&&owner.use_count()>static_cast<long>(p.animation_pins+1),
    "TechAward animation canonical control/lifetime lost");
  }
};
BnwTechAwardAnimationWitness::BnwTechAwardAnimationWitness(
 const std::shared_ptr<const BnwTechAwardContextOwner>& owner,const std::shared_ptr<LiveUiHost>& ui,
 const BnwClientFrameStamp& publication,std::uint64_t occurrence,std::uint64_t generation,
 const std::shared_ptr<const BnwTechAwardContextLayout>& layout)
 :owner_(owner),ui_(ui),layout_(layout),publication_(publication),occurrence_(occurrence),generation_(generation) {
 Pin pin(*this);pin.pure();const auto& p=*pin.owner->impl_;
 need(ui&&layout&&occurrence&&generation&&p.alpha_initialized&&p.observed.visible&&p.observed.queued,
      "TechAward animation publication has no actual initialized queued owner");
 setters_=p.observed.setter_occurrences;xml_=p.observed.xml_sha256;styles_=p.observed.styles_sha256;pin.pure();
}
void BnwTechAwardAnimationWitness::require_current_without_callbacks()const {
 Pin pin(*this);pin.pure();
 const auto& p=*pin.owner->impl_;
 const auto ui=ui_.lock();const auto actual_ui=ui?ui->selected_owner_lifetime_.lock():nullptr;
 const auto bound=ui?ui->tech_award_context_.lock():nullptr;
 need(ui&&actual_ui&&actual_ui.get()==ui.get()&&!actual_ui.owner_before(ui)&&!ui.owner_before(actual_ui)&&
  bound&&bound.get()==pin.owner.get()&&!bound.owner_before(pin.owner)&&!pin.owner.owner_before(bound)&&
  ui->tech_award_context_occurrence_==occurrence_&&ui->tech_award_context_generation_==generation_,
  "TechAward animation canonical UI/context binding changed");
 // The short binding lock is not carried across a callback or an outside-count
 // test. It is not an additional client owner.
 const auto canonical=layout_.lock();
 need(p.observed.setter_occurrences==setters_&&p.observed.xml_sha256==xml_&&p.observed.styles_sha256==styles_&&
  canonical&&p.alpha_initialized&&!p.busy.load(),"TechAward animation sealed context/source changed");
}
void BnwTechAwardAnimationWitness::require_current()const {
 require_current_without_callbacks();Pin pin(*this);pin.pure();pin.owner->require_queued_current();pin.pure();
 // Concrete bounded AssetHandle IO after the final source callback; the
 // retained original XML/Styles handles, not mutable layout DTOs, are read.
 const auto& p=*pin.owner->impl_;
 need(p.xml&&p.styles&&runtime::identify_bytes(p.xml->read_bytes(2*1024*1024)).sha256==xml_&&
      runtime::identify_bytes(p.styles->read_bytes(8*1024*1024)).sha256==styles_,
      "TechAward animation selected XML/Styles changed after last source callback");
 pin.pure();require_current_without_callbacks();
}
void BnwTechAwardAnimationWitness::quarantine()const noexcept {
 if(creator_!=std::this_thread::get_id())return;
 if(const auto owner=owner_.lock())owner->impl_->retired.store(true);
}
void BnwTechAwardAnimationWitness::require_publication(const BnwClientFrameStamp& actual,
 std::uint64_t occurrence,std::uint64_t generation,const std::shared_ptr<const BnwTechAwardContextLayout>& layout)const {
 require_current();const auto canonical=layout_.lock();
 need(actual.identity==publication_.identity&&actual.selection==publication_.selection&&actual.viewport==publication_.viewport&&
  occurrence==occurrence_&&generation==generation_&&canonical&&layout&&canonical.get()==layout.get()&&
  !canonical.owner_before(layout)&&!layout.owner_before(canonical),"TechAward animation foreign/copied publication");
}
std::vector<std::pair<std::string,float>> BnwTechAwardAnimationWitness::advance(const BnwTechAwardAnimationSample& sample)const {
 Pin pin(*this);pin.pure();require_current();pin.pure();sample.require_current();
 const auto layout=layout_.lock();const auto clock=sample.clock_.lock();
 {const auto ui=ui_.lock();const auto actual_ui=clock?clock->ui.lock():nullptr;
  need(layout&&ui&&actual_ui&&actual_ui.get()==ui.get()&&!actual_ui.owner_before(ui)&&!ui.owner_before(actual_ui),
       "TechAward animation sample belongs to another canonical UI");}
 // Temporary UI locks are gone before source callbacks and outside-count checks.
 auto result=pin.owner->advance_selected_alpha(sample,*layout,*this);sample.require_current();pin.pure();require_current();pin.pure();return result;
}
void BnwTechAwardAnimationWitness::pointer_transition(ClientPoint point,const BnwTechAwardContextLayout& supplied)const {
 Pin pin(*this);pin.pure();require_current();pin.pure();
 const auto canonical=layout_.lock();need(canonical&&canonical.get()==&supplied,"TechAward pointer layout is not canonical publication");
 pin.owner->transition_selected_mouse(point,supplied,*this);pin.pure();require_current();pin.pure();
}
bool BnwTechAwardAnimationWitness::mouse_visible(std::string_view path,const BnwTechAwardContextLayout& supplied)const {
 require_current_without_callbacks();Pin pin(*this);pin.pure();
 const auto canonical=layout_.lock();need(canonical&&canonical.get()==&supplied,"TechAward paint hover layout foreign");
 return pin.owner->selected_mouse_visible(path,supplied);
}
bool BnwTechAwardContextOwner::selected_mouse_visible(std::string_view path,const BnwTechAwardContextLayout& layout)const {
 const auto member=impl_->mouse_members.find(std::string(path));
 const auto node=std::find_if(layout.nodes.begin(),layout.nodes.end(),[&](const auto& n){return n.path==path;});
 need(node!=layout.nodes.end(),"TechAward mouse member path absent");
 if(member==impl_->mouse_members.end()){need(node->mouse_wrapper_path.empty(),"TechAward foreign hover wrapper member");return true;}
 need(node->mouse_wrapper_path==member->second.wrapper&&node->mouse_parent_path==member->second.parent&&node->mouse_hide_only==member->second.hide,
      "TechAward sealed hover membership changed");
 const auto parent=impl_->mouse_parents.find(member->second.parent);need(parent!=impl_->mouse_parents.end(),"TechAward hover parent state absent");
 return member->second.hide?!parent->second:parent->second;
}
void BnwTechAwardContextOwner::transition_selected_mouse(ClientPoint point,const BnwTechAwardContextLayout& layout,
 const BnwTechAwardAnimationWitness& witness)const {
 require_queued_current();witness.require_current_without_callbacks();
 auto parents=impl_->mouse_parents;auto phases=impl_->alpha_states;
 {
  Impl::Guard guard(*impl_);
  std::set<std::string> fresh;
  // Selected base virtual30 uses inclusive rect+(width/height)-1. Native
  // client coordinates arrive directly from the sealed LPARAM source.
  const auto inside=[&](const BnwTechAwardLayoutNode& n){
   volatile float right=n.rect[0]+n.rect[2];right=right-1.0f;
   volatile float bottom=n.rect[1]+n.rect[3];bottom=bottom-1.0f;
   return n.rect[2]>0&&n.rect[3]>0&&float(point.x)>=n.rect[0]&&float(point.y)>=n.rect[1]&&float(point.x)<=right&&float(point.y)<=bottom;
  };
  for(auto node=layout.nodes.rbegin();node!=layout.nodes.rend();++node)if(parents.contains(node->path)) {
   need(!node->mouse_input_unsupported,"TechAward hover input producer changed");
   bool admitted=!node->hidden&&inside(*node);
   for(const auto& ancestor:layout.nodes)if(node->path.starts_with(ancestor.path+"/")) {
    need(!ancestor.mouse_input_unsupported,"TechAward hover ancestor input unsupported");
    if(ancestor.hidden)admitted=false;
   }
   if(admitted)fresh.insert(node->path);
  }
  for(auto& [parent,was]:parents) {
   const bool now=fresh.contains(parent);
   if(was&&!now)for(auto& [path,phase]:phases) {
    const auto member=impl_->mouse_members.find(path);
    if(member!=impl_->mouse_members.end()&&member->second.parent==parent) {
     // Actual Alpha virtual54: pause/progress/reverse reset, alphaBC(0),
     // stopped cleared. Selected Pause=0; stable inside never enters here.
     phase.progress=0;phase.remaining=0;phase.reverse=false;phase.alpha=.99f;
    }
   }
   was=now;
  }
 }
 require_queued_current();witness.require_current_without_callbacks();
 need(!impl_->retired.load()&&!impl_->busy.load(),"TechAward mouse transition retired before publication");
 impl_->mouse_parents.swap(parents);impl_->alpha_states.swap(phases);
}
void BnwTechAwardContextOwner::initialize_selected_alpha(const BnwTechAwardContextLayout& layout)const {
 // Called only by this actual Context's auto_size, under its existing Guard,
 // after selected XML/Styles/layout preparation and before genuine QueuePopup.
 auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.busy.load(),
                    "TechAward alpha initialization outside actual control initializer");
 if(p.alpha_initialized)return;
 need(bool(p.styles),"TechAward alpha initialization lacks selected Styles");
 const auto raw=p.styles->read_bytes(8*1024*1024);require_current();
 need(runtime::identify_bytes(raw).sha256==p.observed.styles_sha256,"TechAward alpha Styles source changed");
 const auto styles=content::read_xml_metadata(std::string(raw.begin(),raw.end()));
 std::vector<std::vector<std::pair<std::string,std::string>>> allowed;
 const auto collect=[&](const auto& visit,const content::XmlNode& n)->void {
  if(n.name=="AlphaAnim") {
   const auto supported=n.attribute("Pause")=="0"&&n.attribute("Cycle")=="Bounce"&&n.attribute("Speed")=="1"&&
    n.attribute("AlphaStart")==".99"&&n.attribute("AlphaEnd")==".25"&&n.attribute("Hidden")=="0";
   need(supported,"TechAward selected AlphaAnim profile differs from recovered four styles");
   const std::set<std::string> keys{"Anchor","AnchorSide","Offset","Size","TextureOffset","Texture","Pause","Cycle","Speed","AlphaStart","AlphaEnd","Hidden"};
   std::map<std::string,std::string> attrs;for(const auto& [key,value]:n.attributes){need(keys.contains(key),"TechAward AlphaAnim function/callback/deferred profile unsupported");need(attrs.emplace(key,value).second,"TechAward duplicate AlphaAnim attribute");}
   allowed.emplace_back(attrs.begin(),attrs.end());
  }
  for(const auto& child:n.children)visit(visit,child);
 };
 for(const auto& style:styles.children)if(style.name=="SmallButton"||style.name=="MenuLeftSideTreatment"||style.name=="MenuRightSideTreatment")collect(collect,style);
 need(allowed.size()==4,"TechAward actual four AlphaAnim declarations changed");
 std::map<std::string,Impl::AlphaState> initial;
 for(const auto& node:layout.nodes)if(node.kind=="AlphaAnim") {
  need(std::find(allowed.begin(),allowed.end(),node.animation_profile)!=allowed.end(),"TechAward authored AlphaAnim is not a canonical selected profile");
  need(initial.emplace(node.path,Impl::AlphaState{node.animation_profile,0,0,.99f,false}).second,"TechAward duplicate canonical animation path");
 }
 require_current();need(!p.retired.load()&&p.busy.load(),"TechAward alpha initializer retired during source checks");
 std::map<std::string,Impl::MouseMember> members;std::map<std::string,bool> parents;
 for(const auto& node:layout.nodes)if(!node.mouse_wrapper_path.empty()) {
  const auto parent=std::find_if(layout.nodes.begin(),layout.nodes.end(),[&](const auto& value){return value.path==node.mouse_parent_path;});
  need(parent!=layout.nodes.end()&&(parent->kind=="Button"||parent->kind=="GridButton")&&!parent->mouse_input_unsupported,
       "TechAward hover parent has no supported selected Button input producer");
  for(const auto& ancestor:layout.nodes)if(parent->path==ancestor.path||parent->path.starts_with(ancestor.path+"/"))
   need(!ancestor.mouse_input_unsupported,"TechAward hover ancestor disabled/clip/callback semantics unsupported");
  need(members.emplace(node.path,Impl::MouseMember{node.mouse_wrapper_path,node.mouse_parent_path,node.mouse_hide_only}).second,"TechAward duplicate hover member");
  parents.try_emplace(node.mouse_parent_path,false);
 }
 p.mouse_members.swap(members);p.mouse_parents.swap(parents);
 p.alpha_states.swap(initial);p.alpha_initialized=true;
}
std::vector<std::pair<std::string,float>> BnwTechAwardContextOwner::advance_selected_alpha(
 const BnwTechAwardAnimationSample& sample,const BnwTechAwardContextLayout& layout,
 const BnwTechAwardAnimationWitness& witness)const {
 sample.require_current();require_queued_current();sample.require_current();
 auto next=impl_->alpha_states;std::vector<std::pair<std::string,float>> result;
 {
  Impl::Guard guard(*impl_);need(impl_->alpha_initialized,"TechAward animation deferred initialization unsupported");
  const auto previous=impl_->last_alpha_sample.lock();const auto actual=sample.self_.lock();
  need(actual&&(!previous||previous.get()!=actual.get()||previous.owner_before(actual)||actual.owner_before(previous)),
       "TechAward animation repeated sample");
  std::size_t count=0;
  for(const auto& node:layout.nodes)if(node.kind=="AlphaAnim") {
   ++count;const auto found=next.find(node.path);
   need(found!=next.end()&&found->second.profile==node.animation_profile,"TechAward animation authored path/profile changed");
   auto& state=found->second;
   // Actual Hidden2 freezes. Owned selected wrapper membership supplies the
   // recovered no-update2000 eligibility; unrelated2000 producers remain
   // unsupported. No cursor DTO manufactures native update authority.
   if(!node.hidden&&selected_mouse_visible(node.path,layout)) {
    if(state.remaining>0){volatile float value=state.remaining-sample.seconds_;state.remaining=value;}
    else {
     volatile float increment=1.0f*sample.seconds_;volatile float progress=state.progress+increment;state.progress=progress;
     float parameter=state.progress>=1.0f?1.0f:state.progress;
     if(state.reverse){volatile float reversed=1.0f-parameter;parameter=reversed;}
     volatile float remaining=1.0f-parameter;
     volatile float start=remaining*.99f;volatile float end=parameter*.25f;volatile float alpha=start+end;state.alpha=alpha;
     if(state.progress>=1.0f){state.progress=0;state.reverse=!state.reverse;state.remaining=0;}
    }
   }
   result.emplace_back(node.path,state.alpha);
  }
  need(count==next.size(),"TechAward animation canonical roster changed");sample.require_current();
 }
 require_queued_current();sample.require_current();
 // Authenticate canonical UI binding and this issued source witness AFTER
 // the last full callback, before any phase is made observable.
 witness.require_current_without_callbacks();sample.require_current();
 // No callback/provider from this final intrinsic check through publication.
 need(!impl_->retired.load()&&!impl_->busy.load(),"TechAward animation source retired before phase publication");
 impl_->alpha_states.swap(next);impl_->last_alpha_sample=sample.self_;
 return result;
}
}

namespace civ5::ui {
namespace {
void animation_sample_need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class A,class B>bool animation_sample_control(const A& a,const B& b){
 return a&&b&&a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
}
}
BnwTechAwardAnimationSample::BnwTechAwardAnimationSample(const std::shared_ptr<Clock>& clock,
 std::uint64_t sequence,std::uint32_t tick,float seconds)
 :clock_(clock),sequence_(sequence),tick_(tick),seconds_(seconds){}
void BnwTechAwardAnimationSample::require_current()const {
 animation_sample_need(creator_==std::this_thread::get_id(),"TechAward update sample foreign thread");
 const auto clock=clock_.lock();const auto self=self_.lock();const auto current=clock?clock->sample.lock():nullptr;
 animation_sample_need(clock&&clock->creator==creator_&&clock->alive&&!clock->failed&&clock->active&&
  clock->sequence==sequence_&&animation_sample_control(self,current)&&!clock->scene.expired()&&!clock->ui.expired()&&
  !clock->window.expired(),"TechAward actual renderer update sample retired/replaced");
}
}
