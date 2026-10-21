#include "civ5/ui/bnw_plot_yield_refresh_owned_ui_context.hpp"
#include "civ5/ui/bnw_tech_tree_lifecycle.hpp"
#include "civ5/ui/bnw_city_banner_appearance.hpp"
#include "civ5/ui/bnw_unit_panel_stat_rows_feed.hpp"
#include "civ5/ui/bnw_info_default_form.hpp"
#include "civ5/ui/system_screens.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include <stdexcept>
#include <thread>
#include <type_traits>
namespace civ5::ui {
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
}
// No strong child back-pointers: source storage can safely outlive its context
// facade while retained by a host/Scene method or frame callback.
class BnwPlotYieldRefreshOwnedUiStorage final {
 public:
 std::thread::id thread=std::this_thread::get_id();mutable std::atomic<bool> retired{false};
 bool operation=false; // Creator-thread public action entry only.
 std::shared_ptr<BnwOwnedCameraUiSourceInput> source;
 std::shared_ptr<BnwWorldViewFontSourceScope> font;
 std::optional<graphics::BnwRendererSceneReceipt> receipt;
 std::optional<UiContextRegistry> registry;
 std::unique_ptr<UiScreenController> screens;
 std::optional<TopPanelLayout> top_panel;
 std::shared_ptr<const TopPanelLocalizationCatalog> catalog;
 std::optional<WorldViewSkin> skin;
 std::vector<SystemPediaEntry> pedia;
 std::unique_ptr<EconomicOverviewScene> economic;
 std::shared_ptr<BnwTechTreeLifecycle> tech;
 std::optional<LiveBnwPreviewBinding> binding;
 std::unique_ptr<UiScreenCompositor> compositor;
 std::weak_ptr<LiveUiHost> host;
 std::weak_ptr<BnwClientSceneFrame> scene;
 std::vector<BnwTechTreeEffectObservation> tech_effects;
 std::string economic_blocker,popup_blocker;
 struct Projection {terrain::WorldLayout layout;terrain::HeightOwnerParameters height;
  terrain::CameraFrameState camera;std::int32_t width=0,height_pixels=0;};
 std::optional<Projection> projection;
 std::vector<LiveCityBannerProjectionAudit> audit;
 void current()const{
  if(thread!=std::this_thread::get_id()){retired.store(true);throw std::runtime_error("Owned camera UI storage foreign thread");}
  need(!retired.load()&&source&&font,"Owned camera UI storage retired");
  source->require_current();need(!retired.load(),"Owned camera UI storage retired during source callback");
  font->require_current();
  need(!retired.load(),"Owned camera UI storage retired during currentness callback");
 }
};
namespace {
struct ContextOperation {
 std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage> storage;
 explicit ContextOperation(std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage> s):storage(std::move(s)){
  need(storage!=nullptr,"Owned camera operation storage absent");
  if(storage->thread!=std::this_thread::get_id()||storage->operation){
   storage->retired.store(true);throw std::runtime_error("Owned camera operation foreign/reentered");}
  need(!storage->retired.load(),"Owned camera operation permanently retired");storage->operation=true;
 }
 ~ContextOperation(){storage->operation=false;}
};
// Every preserved real source callable is pinned, checked before its borrow,
// and checked again after arbitrary code returns. No callback fabricates data.
template<class R,class... A> void guard_source(
 std::function<R(A...)>& fn,std::weak_ptr<BnwPlotYieldRefreshOwnedUiStorage> weak){
 if(!fn)return;
 fn=[weak,actual=std::move(fn)](A... args)->R{
  const auto storage=weak.lock();need(storage!=nullptr,"Owned camera source storage lost");storage->current();
  if constexpr(std::is_void_v<R>){actual(std::forward<A>(args)...);storage->current();}
  else{auto value=actual(std::forward<A>(args)...);storage->current();return value;}
 };
}
}
std::shared_ptr<void> BnwPlotYieldRefreshOwnedUiContext::retain_adapter_operation(
 const std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>& storage){
 return std::make_shared<ContextOperation>(storage);
}
std::shared_ptr<BnwPlotYieldRefreshOwnedUiContext> BnwPlotYieldRefreshOwnedUiContext::create(
 std::shared_ptr<BnwOwnedCameraUiSourceInput> source,
 std::shared_ptr<BnwWorldViewFontSourceScope> font,const graphics::BnwRendererSceneReceipt& receipt){
 const auto pinned_receipt=receipt;
 need(source&&font,"Owned camera UI actual source/scope absent");
 source=source->retain_owned_ui_owner(source);font=font->retain_owned_ui_owner(font);
 source->require_current();font->require_current();
 const auto parent=source->parent();
 need(pinned_receipt.projection_reconciled&&pinned_receipt.restored_payload_sha256==parent->identity().payload_sha256&&
      pinned_receipt.active_player==static_cast<std::uint64_t>(parent->active_player_id()),
      "Owned camera UI renderer receipt differs from real parent");
 const auto context=std::shared_ptr<BnwPlotYieldRefreshOwnedUiContext>(new BnwPlotYieldRefreshOwnedUiContext);
 context->storage_=std::make_shared<BnwPlotYieldRefreshOwnedUiStorage>();
 context->storage_->source=std::move(source);context->storage_->font=std::move(font);
 context->storage_->receipt=pinned_receipt;context->build();context->require_current();return context;
}
BnwPlotYieldRefreshOwnedUiContext::~BnwPlotYieldRefreshOwnedUiContext(){retire();}
void BnwPlotYieldRefreshOwnedUiContext::retire()noexcept{
 const auto s=storage_;if(!s)return;s->retired.store(true);
 // Foreign retirement may atomically close storage, but must not inspect or
 // mutate creator-thread host/Scene/controller state.
 if(s->thread!=std::this_thread::get_id()||retirement_finalized_)return;retirement_finalized_=true;
 if(host_){host_->issued_plot_yield_snapshot_.reset();host_->issued_plot_yield_sequence_=0;
   host_->issued_plot_yield_selection_.reset();}
 if(const auto adapter=adapter_.lock())adapter->retire();
 if(s->font)s->font->retire();
 // Retirement comes BEFORE any dependent object's destruction. A retained
 // child keeps all raw source pointers allocated but can no longer publish.
 try{if(scene_)scene_->invalidate();}catch(...){}
}
void BnwPlotYieldRefreshOwnedUiContext::require_current()const{
 const auto s=storage_;need(s!=nullptr,"Owned camera UI context absent");s->current();
}
void BnwPlotYieldRefreshOwnedUiContext::require_storage_current(
 const std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>& s){const auto retained=s;if(retained)retained->current();}
void BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(
 const std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>& supplied){
 const auto s=supplied;if(!s)return;need(s&&s->thread==std::this_thread::get_id()&&!s->retired.load()&&s->source&&s->font,
     "Owned camera final source storage absent/retired/foreign thread");
 s->source->require_finality();s->font->require_owned_ui_finality();
 need(!s->retired.load(),"Owned camera final source retired");
}
// Only the real private factory installs these weak children. A raw caller
// pointer is compared with that exact owned child, never adopted as ownership.
// These pins retain children/storage, not the Context facade; facade loss still
// retires sources before a subsequent callback/borrow can be accepted.
std::shared_ptr<BnwClientSceneFrame> BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(
 const std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>& supplied,const BnwClientFrameOwner* owner){
 const auto s=supplied;if(!s)return {};require_storage_finality(s);
 const auto scene=s->scene.lock();
 need(scene&&&scene->owner_==owner&&scene->owned_plot_yield_ui_sources_==s,
      "Owned camera actual Scene/FrameOwner storage binding lost");return scene;
}
std::shared_ptr<LiveUiHost> BnwPlotYieldRefreshOwnedUiContext::retain_owned_host(
 const std::shared_ptr<BnwPlotYieldRefreshOwnedUiStorage>& supplied,const LiveUiHost* expected){
 const auto s=supplied;if(!s)return {};require_storage_finality(s);
 const auto host=s->host.lock();
 need(host&&host.get()==expected&&host->owned_plot_yield_ui_sources_==s,
      "Owned camera actual host storage binding lost");return host;
}
std::shared_ptr<BnwClientSceneFrame> BnwClientSceneFrame::pin_owned()const{
 return BnwPlotYieldRefreshOwnedUiContext::retain_owned_scene(owned_plot_yield_ui_sources_,&owner_);
}
std::shared_ptr<LiveUiHost> BnwClientSceneFrame::pin_owned_host(const LiveUiHost& host)const{
 return BnwPlotYieldRefreshOwnedUiContext::retain_owned_host(owned_plot_yield_ui_sources_,&host);
}
void BnwClientSceneFrame::verify_owned()const{
 BnwPlotYieldRefreshOwnedUiContext::require_storage_finality(owned_plot_yield_ui_sources_);
}
void BnwPlotYieldRefreshOwnedUiContext::require_source_finality()const{
 need(storage_!=nullptr,"Owned camera final storage absent");require_storage_finality(storage_);
}
void BnwPlotYieldRefreshOwnedUiContext::require_frame_finality(const LiveUiFrame& ui,
 const std::shared_ptr<const BnwClientFrame>& publication)const{
 require_source_finality();need(host_&&scene_,"Owned camera final children absent");
 host_->require_issued_frame_finality(ui);scene_->require_publication_finality(publication);
 const auto* stamp=ui.snapshot.find<BnwClientFrameStamp>();
 need(stamp&&stamp==publication->ui.find<BnwClientFrameStamp>()&&
      stamp->identity==publication->stamp.identity&&stamp->selection==publication->stamp.selection&&
      stamp->viewport==publication->stamp.viewport,"Owned camera final host/publication payload binding changed");
 const auto& input=storage_->source->final_input();const auto state=input.session.read_state_lease();
 const auto active=state->setup.players.find(input.active_player_id);
 need(active!=state->setup.players.end()&&active->second.alive&&
      active->second.team==publication->stamp.identity.team&&
      publication->stamp.identity.player==input.active_player_id&&
      publication->stamp.identity.turn==state->setup.game_turn,
      "Owned camera final active binding changed");
 if(publication->stamp.selection.city){const auto cities=state->cities.players.find(input.active_player_id);
  need(cities!=state->cities.players.end()&&cities->second.cities.contains(*publication->stamp.selection.city)&&
       host_->selected_city_==std::optional(std::pair{input.active_player_id,*publication->stamp.selection.city}),
       "Owned camera final selected city binding changed");}
 if(publication->stamp.selection.unit){const auto army=state->maintenance.players.find(input.active_player_id);
  need(army!=state->maintenance.players.end()&&army->second.units.contains(*publication->stamp.selection.unit),
       "Owned camera final selected unit binding changed");}
}
void BnwPlotYieldRefreshOwnedUiContext::build(){
 const auto s=storage_;s->current();const auto weak=std::weak_ptr<BnwPlotYieldRefreshOwnedUiStorage>(s);
 const auto parent=s->source->parent();const auto& input=s->source->input();
 const auto& resources=s->font->resources();
 s->registry=UiContextRegistry::from_resources(resources);
 need(s->registry->find("CityView")&&s->registry->find("WorldView")&&s->registry->find("TechTree"),
      "Owned camera UI actual source graph lacks required consumers");
 s->screens=std::make_unique<UiScreenController>(*s->registry);
 s->top_panel=read_top_panel(resources);
 const auto lang=s->font->locale().language;
 need(lang=="ru_RU"||lang=="en_US","Owned camera UI selected locale unsupported");
 const std::string table=lang=="ru_RU"?"Language_RU_RU":"Language_en_US";
 s->catalog=std::make_shared<TopPanelLocalizationCatalog>(
     TopPanelLocalizationCatalog::read_active_bnw_resources(resources,table));
 auto localized=input;
 const auto catalog=s->catalog;
 if(localized.frame_sources.unit_stats_database&&localized.frame_sources.unit_stats_types&&
    localized.frame_sources.unit_stats_identity)
  install_bnw_unit_panel_stat_rows_feed(localized.frame_sources,localized.session,
      localized.active_player_id,catalog,lang);
 if(localized.pregame_roster&&localized.frame_sources.database){
  struct Appearance {std::uint64_t generation;std::shared_ptr<BnwCityBannerAppearanceOwner> owner;};
  const auto appearance=std::make_shared<Appearance>(Appearance{localized.session.state_generation(),
   std::make_shared<BnwCityBannerAppearanceOwner>(localized.session,*localized.pregame_roster,
      *localized.frame_sources.database,resources)});
  localized.frame_sources.city_banner_appearance=[weak,appearance](const auto& session,auto active,
     auto player,auto city,auto frame)->std::optional<BnwCityBannerAppearance>{
   const auto owner=weak.lock();need(owner!=nullptr,"Owned camera banner storage lost");owner->current();
   const auto& base=owner->source->input();
   need(&session==&base.session&&appearance->generation==session.state_generation(),
        "Owned camera banner allocation replaced");
   const auto value=appearance->owner->capture(active,player,city,frame);owner->current();return value;
  };
  localized.frame_sources.city_banner_appearance_current=[weak,appearance](const auto& session,const auto& receipt){
   const auto owner=weak.lock();if(!owner)return false;
   try{owner->current();return &session==&owner->source->input().session&&
       appearance->generation==session.state_generation()&&appearance->owner->matches(receipt);}
   catch(...){return false;}
  };
 }
 // These owned transforms read only the immutable catalog. Scene capture
 // checks resource currentness before/after the entire capture and at its
 // publication fences; each string still checks storage/source/font finality.
 localized.frame_sources.localize_city=[weak,catalog](std::string_view key)->std::optional<std::string>{
  const auto owner=weak.lock();need(owner!=nullptr,"Owned camera localization storage lost");require_storage_finality(owner);
  const auto it=catalog->rows.find(std::string(key));auto value=it==catalog->rows.end()?std::nullopt:std::optional(it->second);
  require_storage_finality(owner);return value;
 };
 localized.frame_sources.localize_info=[weak,catalog](std::string_view key){
  const auto owner=weak.lock();need(owner!=nullptr,"Owned camera localization storage lost");require_storage_finality(owner);
  const auto it=catalog->rows.find(std::string(key));auto value=it==catalog->rows.end()?std::string(key):bnw_info_default_form(*catalog,key);
  require_storage_finality(owner);return value;
 };
 const auto scene_localize_city=localized.frame_sources.localize_city;
 const auto scene_localize_info=localized.frame_sources.localize_info;
 s->economic=std::make_unique<EconomicOverviewScene>(input.session,input.active_player_id,*s->screens,resources);
 LiveSessionUiSources economy;
 if(s->economic->install_popup_feed(economy,s->economic_blocker)){
  auto existing=localized.frame_sources.active_popups;
  auto economic=std::move(economy.active_popups);
  localized.frame_sources.active_popups=[weak,existing=std::move(existing),economic=std::move(economic)]
    (const runtime::LiveSession& session)->std::optional<LivePopupEventFeed>{
   const auto owner=weak.lock();need(owner!=nullptr,"Owned camera popup storage lost");owner->current();
   const auto other=existing?existing(session):std::nullopt;owner->current();
   const auto local=economic?economic(session):std::nullopt;owner->current();
   if(other&&local){owner->popup_blocker="Concurrent notification and EconomicOverview popup owners cannot share one PopupFrame owner sequence";return std::nullopt;}
   owner->popup_blocker.clear();return other?other:local;
  };
 }else{s->economic.reset();}
 // Preserve the real shared controller even if economics is source-blocked.
 // No native missing handler is replaced with a success service.
 s->tech=std::make_shared<BnwTechTreeLifecycle>(input.session,input.active_player_id,*s->screens,resources,
   [weak](const BnwTechTreeEffectObservation& observation){
    const auto owner=weak.lock();need(owner!=nullptr,"Owned camera TechTree storage lost");owner->current();
    owner->tech_effects.push_back(observation);owner->current();
   });
 auto previous=localized.frame_sources.augment_snapshot;
 localized.frame_sources.augment_snapshot=[weak,previous=std::move(previous)]
   (const runtime::LiveSession& session,UiFrameSnapshot frame){
  const auto owner=weak.lock();need(owner!=nullptr,"Owned camera augmentation storage lost");owner->current();
  need(&session==&owner->source->input().session,"Owned camera augmentation foreign session");
  const auto base=frame;need(ui_owners_current(base),"Owned camera inherited base fences stale");
  if(previous)frame=previous(session,std::move(frame));owner->current();
  need(ui_owners_current(base),"Owned camera inherited base fences changed");
  frame=owner->tech->augment_snapshot(session,frame);owner->current();
  frame=owner->font->augment_snapshot(std::move(frame));owner->current();
  auto fences=frame.find<UiOwnerFences>()?*frame.find<UiOwnerFences>():UiOwnerFences{};
  fences.checks.push_back([weak]{const auto scope=weak.lock();if(!scope)return false;
   try{scope->current();return true;}catch(...){return false;}});
  frame=frame.with<UiOwnerFences>(std::make_shared<const UiOwnerFences>(std::move(fences)));
  need(ui_owners_current(base)&&ui_owners_current(frame),"Owned camera inherited/source fences changed before publication");
  return frame;
 };
 localized.frame_sources.selected_city=[weak]()->std::optional<std::int64_t>{
  const auto owner=weak.lock();need(owner!=nullptr,"Owned camera selection storage lost");owner->current();
  const auto host=owner->host.lock();if(!host)return std::nullopt;
  const auto selected=host->selected_city();if(!selected)return std::nullopt;
  need(selected->first==owner->source->input().active_player_id,"Owned camera selection foreign player");
  owner->current();return selected->second;
 };
 guard_source(localized.frame_sources.localize_info,weak);
 guard_source(localized.frame_sources.localize_city,weak);
 guard_source(localized.frame_sources.selected_city,weak);
 guard_source(localized.frame_sources.selected_unit,weak);
 guard_source(localized.frame_sources.selected_unit_portrait,weak);
 guard_source(localized.frame_sources.selected_unit_stat_boxes,weak);
 guard_source(localized.frame_sources.selected_unit_actions,weak);
 guard_source(localized.frame_sources.info_corner_items,weak);
 guard_source(localized.frame_sources.game_menu_quick_save,weak);
 guard_source(localized.frame_sources.game_menu_load_game,weak);
 guard_source(localized.frame_sources.active_notifications,weak);
 guard_source(localized.frame_sources.active_popups,weak);
 guard_source(localized.frame_sources.plot_feed,weak);
 guard_source(localized.frame_sources.visible_resource_type,weak);
 guard_source(localized.frame_sources.current_plot_yield,weak);
 guard_source(localized.frame_sources.quotes,weak);
 guard_source(localized.frame_sources.active_tech_popup,weak);
 guard_source(localized.frame_sources.active_ideology_popup,weak);
 guard_source(localized.frame_sources.research_selection,weak);
 guard_source(localized.frame_sources.city_banner_appearance,weak);
 guard_source(localized.frame_sources.city_banner_appearance_current,weak);
 guard_source(localized.frame_sources.selected_unit_read_stats,weak);
 guard_source(localized.frame_sources.selected_unit_read_stats_current,weak);
 guard_source(localized.frame_sources.quotes_current,weak);
 guard_source(localized.frame_sources.selected_unit_stat_rows,weak);
 guard_source(localized.frame_sources.selected_unit_stat_rows_current,weak);
 guard_source(localized.frame_sources.augment_snapshot,weak);
 // Preserve full guards for bind_live_bnw_preview's production backend and
 // frame provider. Only the Scene's two known owned targets use finality.
 auto binding_localize_city=std::move(localized.frame_sources.localize_city);
 auto binding_localize_info=std::move(localized.frame_sources.localize_info);
 localized.frame_sources.localize_city=scene_localize_city;
 localized.frame_sources.localize_info=scene_localize_info;
 scene_=std::make_shared<BnwClientSceneFrame>(parent,*s->receipt,localized.frame_sources);
 scene_->owned_plot_yield_ui_sources_=s;scene_->owner_.retain_owned_ui_sources(s);s->scene=scene_;
 localized.frame_sources.localize_city=std::move(binding_localize_city);
 localized.frame_sources.localize_info=std::move(binding_localize_info);
 s->binding.emplace(bind_live_bnw_preview(std::move(localized)));
 s->skin=WorldViewSkin::from_resources(resources,*s->registry);
 s->pedia=read_civilopedia_entries(resources,[catalog](std::string_view key)->std::optional<std::string>{
  const auto it=catalog->rows.find(std::string(key));return it==catalog->rows.end()?std::nullopt:std::optional(it->second);
 });
 auto* backend=s->binding->backend.get();
 s->compositor=std::make_unique<UiScreenCompositor>(resources,*s->registry,backend,
  [catalog](std::string_view key)->std::optional<std::string>{const auto it=catalog->rows.find(std::string(key));
   return it==catalog->rows.end()?std::nullopt:std::optional(it->second);});
 LiveUiSources ui;ui.world_skin=&*s->skin;ui.pedia_entries=&s->pedia;ui.top_panel_layout=&*s->top_panel;
 ui.top_panel_localizer.text=[catalog](std::string_view key,const std::vector<std::string>& args){return catalog->format(key,args);};
 ui.production_backend=dynamic_cast<BnwCityProductionUiBackend*>(backend);
 ui.production_popup_source=&s->compositor->production_popup_source();
 ui.progression[static_cast<std::size_t>(ProgressionScreen::technologies)]=&s->tech->source();
 ui.active_tech_tree_event=[weak]{const auto owner=weak.lock();if(!owner)return std::optional<ProgressionTechnologyTreeEvent>{};
  owner->current();const auto value=owner->tech->active_event(owner->source->input().session);owner->current();return value;};
 ui.tech_tree_processed=[weak](const auto& event){const auto owner=weak.lock();need(owner!=nullptr,"Owned camera TechTree processing storage lost");
  owner->current();const auto value=owner->tech->process(event);owner->current();return value;};
 ui.tech_tree_local_dequeue_observed=[weak](const auto& event){const auto owner=weak.lock();if(!owner)return false;
  try{owner->current();return owner->tech->local_dequeue_observed(event);}catch(...){return false;}};
 auto provider=scene_->ui_provider();
 host_=scene_->create_selected_ui_host(*s->screens,
   [weak,provider=std::move(provider)]()mutable{
    const auto owner=weak.lock();need(owner!=nullptr,"Owned camera provider storage lost");owner->current();
    auto frame=provider();owner->current();
    if(owner->projection){const auto& p=*owner->projection;
     frame=project_live_city_banners(frame,p.layout,p.height,p.camera,p.width,p.height_pixels,&owner->audit);}
    owner->current();return frame;
   },backend,std::move(ui));
 host_->owned_plot_yield_ui_sources_=s;s->host=host_;s->current();
}
std::shared_ptr<const BnwClientFrame> BnwPlotYieldRefreshOwnedUiContext::begin_client(ClientViewport viewport,
 terrain::CameraControllerState camera){const auto self=shared_from_this();ContextOperation operation(self->storage_);self->require_current();
 const auto scene=self->scene_;const auto value=scene->begin(viewport,std::move(camera));self->require_current();return value;}
LiveUiFrame BnwPlotYieldRefreshOwnedUiContext::begin_ui(std::int32_t width,std::int32_t height){
 const auto self=shared_from_this();ContextOperation operation(self->storage_);self->require_current();const auto host=self->host_;
 const auto value=host->begin_frame(width,height);self->require_current();need(host->is_current_frame(value),"Owned camera new UI frame stale");self->require_current();return value;}
UiCommandResult BnwPlotYieldRefreshOwnedUiContext::dispatch(const UiCommand& command,const LiveUiFrame& frame){
 const auto self=shared_from_this();ContextOperation operation(self->storage_);const auto owned_command=command;const auto owned_frame=frame;
 self->require_current();const auto host=self->host_;const auto scene=self->scene_;
 const auto value=scene->dispatch(*host,owned_frame,owned_command);
 self->require_current();return value;
}
std::shared_ptr<BnwPlotYieldRefreshUiAdapter> BnwPlotYieldRefreshOwnedUiContext::yield_adapter(){
 const auto self=shared_from_this();ContextOperation operation(self->storage_);self->require_current();if(const auto old=adapter_.lock())return old;
 const auto child=BnwPlotYieldRefreshUiAdapter::from_owned_context(storage_,scene_,host_);adapter_=child;
 self->require_current();return child;
}
void BnwPlotYieldRefreshOwnedUiContext::update_city_projection(terrain::WorldLayout layout,
 terrain::HeightOwnerParameters height,terrain::CameraFrameState camera,std::int32_t width,std::int32_t height_pixels){
 const auto self=shared_from_this();ContextOperation operation(self->storage_);self->require_current();need(width>0&&height_pixels>0&&camera.view_count>0,"Owned camera projection values absent");
 self->storage_->projection=BnwPlotYieldRefreshOwnedUiStorage::Projection{std::move(layout),std::move(height),std::move(camera),width,height_pixels};self->require_current();
}
const std::shared_ptr<BnwClientSceneFrame>& BnwPlotYieldRefreshOwnedUiContext::scene()const{require_current();return scene_;}
const std::shared_ptr<LiveUiHost>& BnwPlotYieldRefreshOwnedUiContext::host()const{require_current();return host_;}
UiScreenController& BnwPlotYieldRefreshOwnedUiContext::screens()const{require_current();return *storage_->screens;}
EconomicOverviewScene* BnwPlotYieldRefreshOwnedUiContext::economic()const{require_current();return storage_->economic.get();}
UiScreenCompositor& BnwPlotYieldRefreshOwnedUiContext::compositor()const{require_current();return *storage_->compositor;}
std::shared_ptr<BnwTechTreeLifecycle> BnwPlotYieldRefreshOwnedUiContext::tech_tree()const{require_current();return storage_->tech;}
const std::vector<LiveCityBannerProjectionAudit>& BnwPlotYieldRefreshOwnedUiContext::city_projection_audit()const{require_current();return storage_->audit;}
const std::shared_ptr<BnwWorldViewFontSourceScope>& BnwPlotYieldRefreshOwnedUiContext::font_source()const{require_current();return storage_->font;}
const std::shared_ptr<BnwOwnedCameraUiSourceInput>& BnwPlotYieldRefreshOwnedUiContext::source_input()const{require_current();return storage_->source;}
const std::string& BnwPlotYieldRefreshOwnedUiContext::popup_blocker()const{require_current();return storage_->popup_blocker;}
}
