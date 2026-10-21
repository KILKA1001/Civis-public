#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include <algorithm>
#include <atomic>
#include <limits>
#include <exception>
#include <thread>
#include <tuple>
namespace civ5::ui {
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
struct GuardState {
 const std::thread::id thread=std::this_thread::get_id();
 mutable std::atomic<bool>poison{false}; std::atomic<bool>retired{false};
 mutable bool busy=false;
 void live()const {
  if(std::this_thread::get_id()!=thread){poison.store(true);throw std::runtime_error("WorldView font scope foreign thread");}
  need(!poison.load()&&!retired.load(),"WorldView font scope permanently retired/poisoned");
 }
};
struct Operation {
 const GuardState&g;int exceptions=std::uncaught_exceptions();
 explicit Operation(const GuardState&s):g(s){g.live();if(g.busy){g.poison.store(true);throw std::runtime_error("WorldView font scope callback reentry");}g.busy=true;}
 ~Operation(){if(std::uncaught_exceptions()>exceptions)g.poison.store(true);g.busy=false;}
};
bool same_client(const runtime::BnwStockLuaClientExtent&a,const runtime::BnwStockLuaClientExtent&b){return a.surface_owner==b.surface_owner&&a.revision==b.revision&&a.width==b.width&&a.height==b.height;}
bool same_candidate(const assets::AssetCandidate&a,const assets::AssetCandidate&b){
 return a.original_name==b.original_name&&a.normalized_name==b.normalized_name&&a.payload_size==b.payload_size&&
 a.provenance.kind==b.provenance.kind&&a.provenance.source_id==b.provenance.source_id&&
 a.provenance.source_relative_path==b.provenance.source_relative_path&&a.provenance.archive_relative_path==b.provenance.archive_relative_path;
}
struct Pin {std::string key;UiTextureAssetSource source;};
struct Read {Pin pin;assets::Bytes bytes;};
Read read(const assets::ResourceMountTable&t,const std::string&key){
 auto result=t.resolve(assets::AssetKey(key));
 if(std::holds_alternative<std::monostate>(result.asset))result=t.resolve(assets::AssetKey(assets::resource_basename(key)));
 const auto*h=std::get_if<assets::AssetHandle>(&result.asset);if(!h)throw std::runtime_error("WorldView font source missing/ambiguous: "+key);
 const auto handle=*h;auto bytes=handle.read_bytes(16ULL*1024*1024);
 return {{key,{handle.candidate(),result.mount_id,runtime::identify_bytes(bytes).sha256}},std::move(bytes)};
}
void check(const assets::ResourceMountTable&t,const Pin&p){const auto now=read(t,p.key).pin;need(same_candidate(now.source.candidate,p.source.candidate)&&now.source.mount_id==p.source.mount_id&&now.source.sha256==p.source.sha256,"WorldView font selected source changed");}
std::atomic<std::uint64_t>next_scope{1};
std::uint64_t token(){auto value=next_scope.load();for(;;){need(value!=0&&value!=std::numeric_limits<std::uint64_t>::max(),"WorldView font scope token exhausted");if(next_scope.compare_exchange_weak(value,value+1))return value;}}
bool same_title(const WorldViewPaint&a,const WorldViewPaint&b){
 const auto rect=[](const auto&r){return std::tuple{r.x,r.y,r.width,r.height};};
 const auto request=[](const auto&r){return std::tuple{r.source_xml_key,r.context_id,r.control_id,r.text,r.style,r.player_id,r.selected_unit_id,r.snapshot_frame,r.turn,r.button_width};};
 return a.kind==b.kind&&rect(a.destination)==rect(b.destination)&&a.text==b.text&&a.style==b.style&&a.rgba==b.rgba&&a.tint==b.tint&&
 a.unit_name_font_selection&&b.unit_name_font_selection&&request(*a.unit_name_font_selection)==request(*b.unit_name_font_selection)&&
 a.font_style&&b.font_style&&a.font_style->font==b.font_style->font&&a.font_style->style==b.font_style->style&&a.font_style->colors==b.font_style->colors&&a.font_style->float_colors==b.font_style->float_colors&&
 a.clip.has_value()==b.clip.has_value()&&(!a.clip||rect(*a.clip)==rect(*b.clip));
}
struct TitleRead {WorldViewPaint paint;std::vector<Pin>pins;};
TitleRead title(const assets::ResourceMountTable&t,const BnwClientFrame&f){
 const auto*state=f.ui.find<WorldViewState>();need(state&&state->selected_unit&&f.stamp.selection.unit&&state->selected_unit->id==*f.stamp.selection.unit&&state->player_id==f.stamp.identity.player,"Current selected WorldView unit/title unavailable");
 const auto registry=UiContextRegistry::from_resources(t);const auto skin=WorldViewSkin::from_resources(t,registry);
 // Pin the SAME logical root/style/color requests used by registry/skin consumers.
 // The selected mount may redirect them to Expansion2; do not pick physical paths.
 std::vector<std::string>keys{"Assets/UI/InGame/InGame.xml",
     "Assets/UI/InGame/WorldView/WorldView.xml","Assets/UI/Styles.xml","Assets/UI/ColorAtlas.xml"};
 for(const auto&key:skin.source_keys())if(std::find(keys.begin(),keys.end(),key)==keys.end())keys.push_back(key);
 std::vector<Pin>pins;for(const auto&key:keys)pins.push_back(read(t,key).pin);
 const auto plan=plan_world_view(skin,registry,f.ui,f.stamp.viewport.framebuffer_width,f.stamp.viewport.framebuffer_height);
 const WorldViewPaint*found=nullptr;
 for(const auto&p:plan.paints)if(p.unit_name_font_selection){need(!found,"Ambiguous current WorldView title command");found=&p;}
 need(found&&found->font_style&&found->font_style->style=="SoftShadow"&&found->font_style->font=="TwCenMT24","Source selected authored UnitName title unavailable");
 need(found->unit_name_font_selection->selected_unit_id==*f.stamp.selection.unit&&found->unit_name_font_selection->snapshot_frame==f.ui.frame()&&found->unit_name_font_selection->turn==f.ui.turn(),"Title command publication differs");return {*found,std::move(pins)};
}
}
struct BnwWorldViewFontSourceQuote::Data {
 std::weak_ptr<const void>scope;BnwLabelLocaleSelection locale;
 std::uint64_t frame=0;std::optional<std::int64_t>player;std::optional<std::int32_t>turn;
};
struct BnwWorldViewFontSourceScope::Impl {
 GuardState guard;mutable bool issuing=false;
 std::unique_ptr<assets::ResourceMountTable> table;
 std::shared_ptr<const assets::CivisResourceBankOwner> bank;
 std::shared_ptr<const CivisResourceBankUiSnapshot> ui;
 std::shared_ptr<const CivisResourceBankFontColorSnapshot> font_colors;
 struct Issue {
  std::shared_ptr<Impl>i;
  explicit Issue(std::shared_ptr<Impl>p):i(std::move(p)){
   i->guard.live();if(i->issuing||i->guard.busy){i->guard.poison.store(true);throw std::runtime_error("WorldView title issuer recursive admission");}i->issuing=true;
  }
  Issue(const Issue&)=delete;
  Issue&operator=(const Issue&)=delete;
  ~Issue(){i->issuing=false;}
 };
 // Augmentation shares issuance exclusion, while inherited owner checks may
 // query the SAME font scope outside a short resource-callback Operation.
 struct Augmentation {
  Issue operation;int exceptions=std::uncaught_exceptions();
  explicit Augmentation(std::shared_ptr<Impl> p):operation(std::move(p)){}
  ~Augmentation(){if(std::uncaught_exceptions()>exceptions)operation.i->guard.poison.store(true);}
 };BnwLabelLocaleSelection selected;Pin metadata;
 std::weak_ptr<BnwWorldViewFontSourceScope>facade;
 Impl(assets::ResourceMountTable&&t,std::string language):table(std::make_unique<assets::ResourceMountTable>(std::move(t))){
  selected={std::move(language),"worldview-font-source:"+std::to_string(token()),1};
 }
 Impl(std::shared_ptr<const assets::CivisResourceBankOwner>b,
      std::shared_ptr<const CivisResourceBankUiSnapshot>u,
      std::shared_ptr<const CivisResourceBankFontColorSnapshot>f,std::string language)
  :bank(std::move(b)),ui(std::move(u)),font_colors(std::move(f)){
  selected={std::move(language),"worldview-font-source:"+std::to_string(token()),1};
 }
 const assets::ResourceMountTable&resources()const{
  guard.live();
  if(bank){font_colors->require_current();font_colors->require_same_bank(*bank);
   font_colors->require_same_ui(*ui);ui->require_same_bank(*bank);
   guard.live();return bank->resources();}
  need(static_cast<bool>(table),"WorldView actual source table absent");return *table;
 }
 void boundary()const{
  guard.live();if(bank){font_colors->require_boundary();guard.live();
   font_colors->require_same_bank(*bank);font_colors->require_same_ui(*ui);guard.live();}
 }
 void validate()const{Operation op(guard);boundary();check(resources(),metadata);
  guard.live();boundary();guard.live();need(!facade.expired(),"WorldView font source facade retired");}
};
BnwWorldViewFontSourceScope::BnwWorldViewFontSourceScope(std::shared_ptr<Impl>i):impl_(std::move(i)){}
BnwWorldViewFontSourceScope::~BnwWorldViewFontSourceScope()=default;
std::shared_ptr<BnwWorldViewFontSourceScope>BnwWorldViewFontSourceScope::create(assets::ResourceMountTable&&table,std::string language){
 need(language=="en_US"||language=="ru_RU","WorldView selected UI language unsupported");
 auto i=std::make_shared<Impl>(std::move(table),std::move(language));Operation op(i->guard);
 const auto r=read(i->resources(),"Assets/Gameplay/XML/NewText/"+std::string(i->selected.language=="ru_RU"?"Russian.xml":"English.xml"));
 const auto xml=content::read_xml_metadata(std::string(r.bytes.begin(),r.bytes.end()));const auto*languages=xml.child("Languages");
 need(xml.name=="GameData"&&languages&&languages->children.size()==1&&languages->children.front().name=="Row","WorldView language metadata shape differs");
 const auto&row=languages->children.front();const auto*type=row.child("Type"),*name=row.child("TableName");
 need(type&&name&&type->text==i->selected.language&&name->text==(i->selected.language=="ru_RU"?"Language_RU_RU":"Language_en_US"),"WorldView selected metadata language differs");
 // FontName/scales/UseExtendedFont here are source evidence only, never the
 // effective row. Actual attached Localization.Languages remains authority.
 i->metadata=r.pin;check(i->resources(),i->metadata);i->guard.live();
 auto result=std::shared_ptr<BnwWorldViewFontSourceScope>(new BnwWorldViewFontSourceScope(i));i->facade=result;return result;
}
std::shared_ptr<BnwWorldViewFontSourceScope>BnwWorldViewFontSourceScope::create_from_civis_resources(
 const std::shared_ptr<assets::CivisResourceBankOwner>&supplied_bank,
 const std::shared_ptr<const CivisResourceBankUiSnapshot>&supplied_ui,
 const std::shared_ptr<const CivisResourceBankFontColorSnapshot>&supplied_fonts,std::string language){
 need(supplied_bank&&supplied_ui&&supplied_fonts,"WorldView own-bank font tuple absent");
 // All canonical get/control checks precede any cold source/client callback.
 const auto bank=supplied_bank->retain_owner();
 const auto ui=supplied_ui->retain_owner();
 const auto fonts=supplied_fonts->retain_owner();
 need(bank&&bank.get()==supplied_bank.get()&&!bank.owner_before(supplied_bank)&&!supplied_bank.owner_before(bank),
      "WorldView font bank canonical control differs");
 need(ui&&ui.get()==supplied_ui.get()&&!ui.owner_before(supplied_ui)&&!supplied_ui.owner_before(ui),
      "WorldView font UI canonical control differs");
 need(fonts&&fonts.get()==supplied_fonts.get()&&!fonts.owner_before(supplied_fonts)&&!supplied_fonts.owner_before(fonts),
      "WorldView font snapshot canonical control differs");
 fonts->require_same_bank(*bank);fonts->require_same_ui(*ui);ui->require_same_bank(*bank);
 need(language=="en_US"||language=="ru_RU","WorldView selected UI language unsupported");
 auto i=std::make_shared<Impl>(bank,ui,fonts,std::move(language));Operation op(i->guard);
 i->boundary();
 const std::string key="Assets/Gameplay/XML/NewText/"+std::string(i->selected.language=="ru_RU"?"Russian.xml":"English.xml");
 bank->load(assets::AssetKey(key),assets::CivisResourceParse::bytes);i->guard.live();i->boundary();
 // Title selection independently checks this actual UnitPanel Lua contract;
 // stage its genuine selected bytes before a memory-only resolver can use it.
 bool unit_panel=false;
 for(const auto&context:ui->registry().contexts())if(context.id=="UnitPanel"){
  need(!unit_panel,"WorldView font UnitPanel source ambiguous");unit_panel=true;
  const auto selected_key=context.source_key+".lua";
  bank->load(assets::AssetKey(selected_key),assets::CivisResourceParse::bytes);
  i->guard.live();i->boundary();
 }
 need(unit_panel,"WorldView font selected UnitPanel context absent");
 const auto r=read(i->resources(),key);i->guard.live();
 const auto xml=content::read_xml_metadata(std::string(r.bytes.begin(),r.bytes.end()));const auto*languages=xml.child("Languages");
 need(xml.name=="GameData"&&languages&&languages->children.size()==1&&languages->children.front().name=="Row",
      "WorldView language metadata shape differs");
 const auto&row=languages->children.front();const auto*type=row.child("Type"),*name=row.child("TableName");
 need(type&&name&&type->text==i->selected.language&&name->text==(i->selected.language=="ru_RU"?"Language_RU_RU":"Language_en_US"),
      "WorldView selected metadata language differs");
 // Actual attached Localization.Languages, not these source metadata fields,
 // remains the effective FontName/scales/UseExtendedFont issuer.
 i->metadata=r.pin;check(i->resources(),i->metadata);i->guard.live();i->boundary();
 auto result=std::shared_ptr<BnwWorldViewFontSourceScope>(new BnwWorldViewFontSourceScope(i));
 i->facade=result;i->boundary();i->guard.live();return result;
}
std::shared_ptr<BnwWorldViewFontSourceScope>BnwWorldViewFontSourceScope::retain_owner()const{
 const auto i=impl_;i->guard.live();const auto actual=i->facade.lock();
 need(actual&&actual.get()==this,"WorldView font source canonical facade absent");
 if(i->bank)i->resources();i->guard.live();return actual;
}
void BnwWorldViewFontSourceScope::require_same_civis_resource_host(
 const SelectedBnwPreviewHost& supplied_host)const{
 // Creator gate precedes copied creator-owned state and all callbacks.
 impl_->guard.live();
 const auto i=impl_;
 Operation operation(i->guard);
 try{
  const auto actual_scope=i->facade.lock();
  need(actual_scope&&actual_scope.get()==this&&actual_scope->impl_.get()==i.get(),
       "WorldView own-bank Host join canonical Scope absent");
  const auto supplied_host_get=&supplied_host;
  const auto host=supplied_host.retain_civis_resource_host_owned();
  need(host&&host.get()==supplied_host_get,
       "WorldView own-bank Host join canonical Host differs");
  need(i->bank&&i->ui&&i->font_colors&&!i->table,
       "WorldView own-bank Host join refuses legacy source Scope");
  const auto bank=i->bank->retain_owner();
  const auto ui=i->ui->retain_owner();
  const auto fonts=i->font_colors->retain_owner();
  need(bank&&bank.get()==i->bank.get()&&!bank.owner_before(i->bank)&&!i->bank.owner_before(bank),
       "WorldView own-bank Host join Bank control differs");
  need(ui&&ui.get()==i->ui.get()&&!ui.owner_before(i->ui)&&!i->ui.owner_before(ui),
       "WorldView own-bank Host join UI control differs");
  need(fonts&&fonts.get()==i->font_colors.get()&&
       !fonts.owner_before(i->font_colors)&&!i->font_colors.owner_before(fonts),
       "WorldView own-bank Host join font control differs");
  const auto pure=[&]{
   // FIRST POST uses independent control, then retained canonical owners.
   // No borrowed this/supplied_host access is allowed after source callbacks.
   i->guard.live();
   // Exactly one extra Scope and one Host pin are introduced by this join.
   need(actual_scope.use_count()>1&&host.use_count()>1,
        "WorldView own-bank Host join outside Scope/Host owner retired");
   need(actual_scope.get()==i->facade.lock().get()&&
        !actual_scope.owner_before(i->facade)&&!i->facade.owner_before(actual_scope)&&
        actual_scope->impl_.get()==i.get(),
        "WorldView own-bank Host join Scope factory control changed");
   need(host->saved_database_thread_==std::this_thread::get_id()&&
        host.get()==host->civis_canonical_self_.lock().get()&&
        !host.owner_before(host->civis_canonical_self_)&&!host->civis_canonical_self_.owner_before(host)&&
        host->civis_resource_bank_&&host->civis_resource_bank_.get()==bank.get()&&
        !host->civis_resource_bank_.owner_before(bank)&&!bank.owner_before(host->civis_resource_bank_),
        "WorldView own-bank Host join actual Host/Bank controls differ");
   bank->require_current();
   host->require_same_civis_resources(*bank);
   ui->require_same_bank(*bank);
   fonts->require_same_bank(*bank);fonts->require_same_ui(*ui);
   i->guard.live();
  };
  pure();
  // Genuine source/client callback with the same sealed tuple on both sides.
  // No Original completed-bank conversion or numeric readiness is granted.
  fonts->require_boundary();
  pure();
 }catch(...){i->guard.poison.store(true);throw;}
}
void BnwWorldViewFontSourceScope::retire()noexcept{const auto i=impl_;i->guard.retired.store(true);}
void BnwWorldViewFontSourceScope::require_current()const{const auto i=impl_;i->validate();}
void BnwWorldViewFontSourceScope::require_selected_locale_without_callbacks(const BnwLabelLocaleSelection& expected)const{
 const auto saved=impl_;saved->guard.live();
 need(!saved->facade.expired(),"System font selected locale source facade retired");
 need(saved->selected==expected,"System font selected locale identity changed");
 saved->guard.live();
}
void BnwWorldViewFontSourceScope::require_owned_ui_finality()const{const auto i=impl_;i->guard.live();need(!i->facade.expired(),"Owned UI font source facade expired");if(i->bank)i->resources();}
std::shared_ptr<BnwWorldViewFontSourceScope> BnwWorldViewFontSourceScope::retain_owned_ui_owner(const std::shared_ptr<BnwWorldViewFontSourceScope>& supplied)const{const auto i=impl_;i->guard.live();const auto actual=i->facade.lock();need(actual&&actual.get()==this&&supplied.get()==this&&!actual.owner_before(supplied)&&!supplied.owner_before(actual),"Owned UI font source canonical owner differs");if(i->bank)i->resources();return actual;}
const assets::ResourceMountTable&BnwWorldViewFontSourceScope::resources()const{const auto i=impl_;i->validate();return i->resources();}
UiFrameSnapshot BnwWorldViewFontSourceScope::augment_snapshot(UiFrameSnapshot before)const{
 const auto i=impl_;Impl::Augmentation operation(i);
 need(!before.find<BnwWorldViewFontSourceQuote>(),"WorldView font source quote already published");
 need(before.frame()!=0&&before.active_player_id()&&before.turn(),"WorldView font locale requires owned numbered player/turn frame");
 need(ui_owners_current(before),"WorldView font augmentation original base owners stale");i->validate();
 auto data=std::make_shared<BnwWorldViewFontSourceQuote::Data>();data->scope=i;data->locale=i->selected;data->frame=before.frame();data->player=before.active_player_id();data->turn=before.turn();
 auto fences=std::make_shared<UiOwnerFences>();if(const auto*base=before.find<UiOwnerFences>())*fences=*base;
 fences->checks.push_back([weak=i->facade]{{auto scope=weak.lock();if(!scope)return false;scope->require_current();}return !weak.expired();});
 auto after=before.with(std::shared_ptr<const BnwWorldViewFontSourceQuote>(new BnwWorldViewFontSourceQuote(std::move(data)))).with(std::shared_ptr<const UiOwnerFences>(std::move(fences)));
 need(ui_owners_current(before),"WorldView font augmentation original owners changed during source callbacks");i->validate();i->guard.live();need(!i->facade.expired(),"WorldView font augmentation source facade retired");return after;
}
const BnwLabelLocaleSelection&BnwWorldViewFontSourceScope::locale()const{const auto i=impl_;i->validate();return i->selected;}
struct BnwWorldViewTitleFontIssuer::Impl {
 GuardState guard;std::weak_ptr<const BnwWorldViewTitleFontIssuer>facade;
 std::shared_ptr<const SelectedBnwPreviewHost>host;
 std::shared_ptr<const BnwNativePackageDiscoveryOwner>package;
 std::shared_ptr<const BnwNativeEarlyInputLease>lease;
 std::shared_ptr<const assets::CivisResourceBankOwner>bank;
 std::shared_ptr<BnwWorldViewFontSourceScope>source;
 std::shared_ptr<const BnwClientFrame>accepted;
 LiveUiHost::FrameProvider publication;
 std::uint64_t generation=0;BnwWorldViewTitleFontFrame frame;
 std::uintptr_t hwnd=0;
 std::array<BnwWorldViewTitleFontDescriptor,3>descriptors;
 std::vector<Pin>pins;
 explicit Impl(BnwWorldViewTitleFontFrame f):frame(std::move(f)){}
 void selected_binding()const{
  need(frame.publication.identity.player==host->active_player_id()&&
       accepted->stamp.identity.player==host->active_player_id()&&
       frame.publication.identity.session==host->identity().payload_sha256&&
       accepted->stamp.identity.session==host->identity().payload_sha256,
       "WorldView font selected host player/payload binding changed");
 }
 void base()const{
  guard.live();selected_binding();need(host->session().state_generation()==generation,"WorldView font session replaced");
  if(bank){
   host->require_same_civis_resources(*bank);guard.live();bank->require_boundary();guard.live();
   const auto client=bank->actual_client_extent();guard.live();
   const auto window=bank->actual_hwnd();guard.live();
   need(same_client(client,frame.client)&&window==hwnd,"WorldView own-bank font client/window epoch changed");
   host->require_same_civis_resources(*bank);guard.live();
  }else{
   host->require_same_early_package(*package);lease->require_current();
   need(same_client(lease->client_extent(),frame.client),"WorldView font client epoch changed");
  }
  const auto&state=host->session().state();const auto player=state.setup.players.find(frame.publication.identity.player);
  need(player!=state.setup.players.end()&&player->second.alive&&player->second.team==frame.publication.identity.team&&state.setup.game_turn==frame.publication.identity.turn&&state.world.map_header.guid==frame.publication.identity.map_guid,"WorldView font active player/turn/world identity changed");
  const auto army=state.maintenance.players.find(frame.publication.identity.player);
  need(frame.publication.selection.unit&&army!=state.maintenance.players.end()&&army->second.units.contains(*frame.publication.selection.unit),"WorldView font selected unit no longer owned");
  const auto ui=publication();const auto*stamp=ui.find<BnwClientFrameStamp>();
  // Exact payload identity from the internally created weak Publication
  // provider, not matching caller-fabricated stamps alone.
  need(stamp&&stamp==accepted->ui.find<BnwClientFrameStamp>()&&stamp->identity==frame.publication.identity&&stamp->selection==frame.publication.selection&&stamp->viewport==frame.publication.viewport,"WorldView font publication retired/replaced");
  const auto*q=ui.find<BnwWorldViewFontSourceQuote>();
  need(q&&q->data_&&q->data_->scope.lock().get()==source->impl_.get()&&q->data_->locale==frame.locale&&q->data_->frame==ui.frame()&&q->data_->player==ui.active_player_id()&&q->data_->turn==ui.turn(),"WorldView selected locale/source was not bound to this actual frame");
  need(ui_owners_current(accepted->ui),"WorldView font base UI owners stale");
  const auto after=publication();need(after.find<BnwClientFrameStamp>()==stamp&&host->session().state_generation()==generation,"WorldView font publication changed during base guards");selected_binding();guard.live();
 }
 void validate(bool borrowed=true)const{
  Operation op(guard);std::optional<BnwWorldViewFontSourceScope::Impl::Issue>shared_operation;
  if(borrowed)shared_operation.emplace(source->impl_);
  base();source->require_current();need(source->locale()==frame.locale,"WorldView font selected locale replaced");
  need(read_bnw_language_font_row(host->database(),frame.locale.language)==frame.language_row,"WorldView effective Languages font row changed");
  const auto&t=source->resources();for(const auto&p:pins){check(t,p);guard.live();}
  // Actual publication/payload identity and all selected metadata pins retain
  // the once-produced title. No duplicate parser/layout/measurement owner.

  base();source->require_current();need(read_bnw_language_font_row(host->database(),frame.locale.language)==frame.language_row,"WorldView effective Languages changed during source read");guard.live();
  if(borrowed)need(!facade.expired(),"WorldView title issuer facade retired");
 }
};
BnwWorldViewTitleFontIssuer::BnwWorldViewTitleFontIssuer(std::shared_ptr<Impl>i):impl_(std::move(i)){}
BnwWorldViewTitleFontIssuer::~BnwWorldViewTitleFontIssuer()=default;
std::shared_ptr<const BnwWorldViewTitleFontIssuer>BnwWorldViewTitleFontIssuer::create_from_adopted_host(
 std::shared_ptr<const SelectedBnwPreviewHost>host,const BnwClientFrameOwner&owner,
 std::shared_ptr<const BnwClientFrame>accepted,std::shared_ptr<BnwWorldViewFontSourceScope>source){
 need(static_cast<bool>(source),"WorldView legacy font source absent");source->impl_->guard.live();
 need(!source->impl_->bank,"Own-bank source cannot enter EARLY title route");
 return create_from_source_host(std::move(host),owner,std::move(accepted),std::move(source),{});
}
std::shared_ptr<const BnwWorldViewTitleFontIssuer>BnwWorldViewTitleFontIssuer::create_from_civis_resources(
 std::shared_ptr<const SelectedBnwPreviewHost>supplied_host,const BnwClientFrameOwner&owner,
 std::shared_ptr<const BnwClientFrame>accepted,std::shared_ptr<BnwWorldViewFontSourceScope>source){
 need(supplied_host&&source&&accepted,"WorldView own-bank title inputs absent");
 // Authenticate the supplied Source control before the real Host join can
 // invoke client/source callbacks. Move the genuine canonical owner once.
 auto actual_source=source->retain_owner();
 need(actual_source&&actual_source.get()==source.get()&&
      !actual_source.owner_before(source)&&!source.owner_before(actual_source),
      "WorldView own-bank title Source canonical control differs");
 source=std::move(actual_source);
 const auto scope=source->impl_;scope->guard.live();
 const auto bank=scope->bank;need(static_cast<bool>(bank),"WorldView own-bank title needs typed own source tuple");
 const auto host=supplied_host->retain_civis_resource_host_owned();
 need(host&&host.get()==supplied_host.get()&&!host.owner_before(supplied_host)&&!supplied_host.owner_before(host),
      "WorldView own-bank title Host canonical control differs");
 source->require_same_civis_resource_host(*host);scope->guard.live();
 host->require_same_civis_resources(*bank);scope->guard.live();
 return create_from_source_host(host,owner,std::move(accepted),std::move(source),bank);
}
std::shared_ptr<const BnwWorldViewTitleFontIssuer>BnwWorldViewTitleFontIssuer::create_from_source_host(
 std::shared_ptr<const SelectedBnwPreviewHost>host,const BnwClientFrameOwner&owner,
 std::shared_ptr<const BnwClientFrame>accepted,std::shared_ptr<BnwWorldViewFontSourceScope>source,
 std::shared_ptr<const assets::CivisResourceBankOwner>bank){
 need(host&&source&&accepted,"Actual WorldView font host/source/publication absent");
 const auto canonical_source=source->retain_owner();
 need(canonical_source.get()==source.get()&&!canonical_source.owner_before(source)&&!source.owner_before(canonical_source),
      "WorldView title font source canonical control differs");
 // These non-callback pins precede lease/resource/UI owner checks. No borrowed
 // FrameOwner access after callbacks: its private weak Publication is retained.
 const auto scope=source->impl_;BnwWorldViewFontSourceScope::Impl::Issue issue(scope);
 // Thread-first shared scope guard precedes every borrowed FrameOwner read.
 need(&owner.session_==&host->session(),"WorldView font host and current frame session differ");
 // Authenticate the actual selected host before ANY lease/resource callbacks,
 // even an otherwise alive player and arbitrary nonempty identity are refused.
 need(owner.player_==host->active_player_id()&&accepted->stamp.identity.player==host->active_player_id(),
      "WorldView font frame owner selected host player differs");
 need(accepted->stamp.identity.session==host->identity().payload_sha256,
      "WorldView font frame owner selected host payload identity differs");
 auto publication=owner.ui_provider();
 // Last borrowed FrameOwner access: current_frame may invoke real providers.
 // Retain its genuine immutable frame and reject a matching raw-pointer alias.
 const auto canonical_frame=owner.current_frame();
 need(canonical_frame&&canonical_frame.get()==accepted.get()&&
      !canonical_frame.owner_before(accepted)&&!accepted.owner_before(canonical_frame),
      "WorldView font host and current canonical frame owner differ");
 accepted=canonical_frame;
 auto package=host->early_package_;
 const auto*bound=accepted->ui.find<BnwWorldViewFontSourceQuote>();
 need(bound&&bound->data_&&bound->data_->scope.lock().get()==scope.get()&&bound->data_->locale==scope->selected&&bound->data_->frame==accepted->ui.frame()&&bound->data_->player==accepted->ui.active_player_id()&&bound->data_->turn==accepted->ui.turn(),"Actual WorldView frame has no SAME source/locale binding");
 if(bank){need(!package&&scope->bank.get()==bank.get()&&!scope->bank.owner_before(bank)&&!bank.owner_before(scope->bank),
     "WorldView own-bank title/source lineage differs");host->require_same_civis_resources(*bank);}
 else need(static_cast<bool>(package),"WorldView font host was not privately EARLY-adopted");
 const auto generation=host->session().state_generation();
 const auto*stamp=accepted->ui.find<BnwClientFrameStamp>();need(stamp&&stamp->identity==accepted->stamp.identity&&stamp->selection==accepted->stamp.selection&&stamp->viewport==accepted->stamp.viewport,"WorldView frame immutable stamp missing/different");
 std::shared_ptr<const BnwNativeEarlyInputLease>lease;
 runtime::BnwStockLuaClientExtent client;
 if(bank){bank->require_boundary();scope->guard.live();client=bank->actual_client_extent();scope->guard.live();
  host->require_same_civis_resources(*bank);scope->guard.live();}
 else{lease=package->early_input_lease();need(static_cast<bool>(lease),"WorldView genuine early client absent");
  host->require_same_early_package(*package);lease->require_current();client=lease->client_extent();}
 source->require_current();
 need(accepted->stamp.viewport.client_width==client.width&&accepted->stamp.viewport.client_height==client.height&&accepted->stamp.viewport.framebuffer_width==client.width&&accepted->stamp.viewport.framebuffer_height==client.height,"WorldView title framebuffer/client mismatch unsupported");
 const auto locale=source->locale();const auto language=read_bnw_language_font_row(host->database(),locale.language);
 const auto&t=source->resources();auto current_title=title(t,*accepted);
 auto i=std::make_shared<Impl>(BnwWorldViewTitleFontFrame{accepted->stamp,client,locale,language,std::move(current_title.paint)});i->pins=std::move(current_title.pins);
 i->hwnd=bank?bank->actual_hwnd():lease->actual_hwnd();scope->guard.live();i->bank=std::move(bank);i->host=std::move(host);i->package=std::move(package);i->lease=std::move(lease);i->source=std::move(source);i->accepted=std::move(accepted);i->publication=std::move(publication);i->generation=generation;
 const auto selected_registry=UiContextRegistry::from_resources(t);
 for(const auto&key:selected_registry.contexts())if(key.id=="UnitPanel"){
  auto xml=read(t,key.source_key+".xml").pin,lua=read(t,key.source_key+".lua").pin;
  // Independent immutable wave37 original UnitPanel declarations/branches.
  // Different/modded script cannot inherit the fixed 24->20->14 contract.
  need(xml.source.sha256=="ca357d8755c2362a48f9d34371697bc83150dae2f5f2c73be11d2c211cb428fe"&&
       lua.source.sha256=="5b4be7346a6570196e94e638094aa3d14227e8637d7e335a73983d7b35764524","Selected UnitPanel source differs from retained font-selection contract");
  i->pins.push_back(std::move(xml));i->pins.push_back(std::move(lua));
 }
 need(std::any_of(i->pins.begin(),i->pins.end(),[](const auto&p){return assets::resource_basename(p.key)=="unitpanel.lua";}),"Selected UnitPanel source pair absent");
 for(std::size_t n=0;n<unit_name_source_fonts.size();++n){
  const auto m=read_glyphgen_style_metadata(t,unit_name_source_fonts[n],"SoftShadow");const auto g=glyphgen_geometry(m.font);
  need(m.layers.size()==3&&m.layers[0].name==std::string(unit_name_source_fonts[n])+"Base"&&m.layers[1].name==std::string(unit_name_source_fonts[n])+"Shadow"&&m.layers[2].name==std::string(unit_name_source_fonts[n])+"Soft","WorldView selected SoftShadow layer mapping differs");
  const auto sum=std::int64_t{g.ascent}+g.descent,adjusted=std::int64_t{g.ascent}-g.ascent_adjustment;
  need(g.ascent>0&&g.descent>=0&&adjusted>0&&adjusted<32768&&sum>0&&sum<32768,"WorldView selected GG metrics unsupported");
  const std::string key="Assets/UI/Fonts/Tw Cent MT/"+std::string(unit_name_source_fonts[n])+".ggxml";
  auto pin=read(t,key).pin;need(pin.source.sha256==m.font_source.sha256&&pin.source.mount_id==m.font_source.mount_id&&same_candidate(pin.source.candidate,m.font_source.candidate),"WorldView GG changed during metadata admission");i->pins.push_back(std::move(pin));
  auto&d=i->descriptors[n];d.font_key_=key;d.font_sha256_=m.font_source.sha256;d.candidate_index_=n;d.adjusted_ascent_=static_cast<std::int32_t>(adjusted);d.source_line_height_=static_cast<std::int32_t>(sum);
 }
 const auto repeated=title(t,*i->accepted);need(same_title(repeated.paint,i->frame.title),"WorldView title changed between source parse/pin passes");
 for(const auto&p:repeated.pins){const auto old=std::find_if(i->pins.begin(),i->pins.end(),[&](const auto&pin){return pin.key==p.key;});need(old!=i->pins.end()&&same_candidate(old->source.candidate,p.source.candidate)&&old->source.mount_id==p.source.mount_id&&old->source.sha256==p.source.sha256,"WorldView source changed between title parse/pin passes");}
 i->validate(false);auto result=std::shared_ptr<const BnwWorldViewTitleFontIssuer>(new BnwWorldViewTitleFontIssuer(i));i->facade=result;return result;
}
const BnwWorldViewTitleFontFrame&BnwWorldViewTitleFontIssuer::current()const{const auto i=impl_;i->validate();return i->frame;}
const BnwWorldViewTitleFontDescriptor&BnwWorldViewTitleFontIssuer::descriptor(std::size_t n)const{const auto i=impl_;need(n<i->descriptors.size(),"WorldView title candidate index invalid");i->validate();return i->descriptors[n];}
void BnwWorldViewTitleFontIssuer::require_descriptor(const BnwWorldViewTitleFontDescriptor&d)const{const auto i=impl_;bool found=false;for(const auto&own:i->descriptors)found|=&own==&d;need(found,"WorldView font descriptor is copied/foreign/unsealed");i->validate();}
const assets::ResourceMountTable&BnwWorldViewTitleFontIssuer::resources()const{const auto i=impl_;i->validate();return i->source->impl_->resources();}
const content::RulesDatabase&BnwWorldViewTitleFontIssuer::database()const{const auto i=impl_;i->validate();return i->host->database();}
std::uintptr_t BnwWorldViewTitleFontIssuer::actual_hwnd()const{const auto i=impl_;i->validate();return i->hwnd;}
}

namespace civ5::ui {
std::shared_ptr<BnwWorldViewFontSourceScope> BnwWorldViewFontSourceScope::from_actual_tech_context_snapshot(const UiFrameSnapshot& snapshot) {
 const auto* quote=snapshot.find<BnwWorldViewFontSourceQuote>();
 need(quote&&quote->data_,"TechAward actual UI language quote absent");
 const auto data=quote->data_;const auto storage=data->scope.lock();
 need(bool(storage),"TechAward selected language source expired");
 const auto actual=std::static_pointer_cast<const Impl>(storage);
 actual->guard.live(); // creator first before lazy facade/resource operations
 const auto facade=actual->facade.lock();
 need(facade&&facade->impl_.get()==actual.get(),"TechAward language canonical source absent");
 need(data->frame==snapshot.frame()&&data->player==snapshot.active_player_id()&&data->turn==snapshot.turn()&&data->locale==actual->selected,
      "TechAward language quote differs from actual UI provider snapshot");
 facade->require_current();
 need(data->locale==facade->locale(),"TechAward selected language changed during source check");
 return facade;
}
}
