#include "civ5/graphics/gl_context.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/civis_bank_audio_source_owner.hpp"
#include "civ5/ui/civis_client_audio_queue_owner.hpp"
#include "civ5/ui/win32_viewport_transaction.hpp"
#include <array>
#include <chrono>
#include <exception>
#include <iostream>
#include <thread>
#include <memory>
#include <variant>
#ifdef _WIN32
#include "../src/ui/bnw_native_resource_stamp_diagnostic.hpp"
namespace {
using namespace civ5;
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void stage(const char* value){std::cerr<<"ACTUAL_OWN_BANK_AUDIO "<<value<<'\n'<<std::flush;}
// Resource prerequisite only. This fixture owns a real Bank/VFS, not a
// selected Context Frame/Source operation or permission to invoke private Lua.
void loader_resource_prerequisite(const std::shared_ptr<assets::CivisResourceBankOwner>& bank){
 const std::array<const char*,2> names{{"Assets/UI/InGame/WorldView/WorldView.lua",
                                      "Assets/UI/InGame/WorldView/WorldView.xml"}};
 for(const auto* name:names){
  const assets::AssetKey key(name);bank->load(key,std::string_view(name).ends_with(".xml")?
      assets::CivisResourceParse::xml:assets::CivisResourceParse::bytes);
 }
 const auto capability=bank->capability();
 for(const auto* name:names){
  const assets::AssetKey key(name);const auto canonical=capability.loaded(key);
  need(canonical.bytes&&!canonical.bytes->empty(),"Real selected loader resource was not staged");
  const auto original=runtime::identify_bytes(*canonical.bytes);
  need(original.bytes==canonical.payload_identity.bytes&&original.sha256==canonical.payload_identity.sha256,
       "Real Bank loader resource has a wrong captured fingerprint");
  {
   auto first=bank->resources(capability).resolve(key);
   auto second=bank->resources(capability).resolve(key);
   auto* first_handle=std::get_if<assets::AssetHandle>(&first.asset);
   auto* second_handle=std::get_if<assets::AssetHandle>(&second.asset);
   need(first_handle&&second_handle,"Real memory-only VFS failed to resolve selected loader paths");
   auto first_owner=std::make_unique<assets::AssetHandle>(std::move(*first_handle));
   auto second_owner=std::make_unique<assets::AssetHandle>(std::move(*second_handle));
   auto first_buffer=std::make_unique<assets::Bytes>(first_owner->read_bytes(8*1024*1024));
   auto second_buffer=std::make_unique<assets::Bytes>(second_owner->read_bytes(8*1024*1024));
   need(first_owner.get()!=second_owner.get()&&first_buffer->data()!=second_buffer->data()&&
        first_buffer->data()!=canonical.bytes->data()&&second_buffer->data()!=canonical.bytes->data(),
        "Fresh loader resource buffer aliases another buffer or canonical Bank storage");
   need(*first_buffer==*second_buffer&&runtime::identify_bytes(*first_buffer).sha256==original.sha256,
        "Fresh resource buffers did not come from the actual selected VFS winner");
   // Observable independence: changing/discarding scratch must neither alter
   // another live buffer nor the canonical Bank used by subsequent contexts.
   first_buffer->front()=static_cast<std::uint8_t>(first_buffer->front()^0xFFu);
   need(runtime::identify_bytes(*second_buffer).sha256==original.sha256&&
        runtime::identify_bytes(*canonical.bytes).sha256==original.sha256,
        "Scratch mutation changed another loader or immutable canonical source");
   first_buffer.reset();first_owner.reset();
   need(runtime::identify_bytes(*second_buffer).sha256==original.sha256,
        "Destroying one actual loader owner invalidated a separate live resource");
  } // actual scratch buffer/handle destructors, no invented cleanup receipt
  bank->require_current();const auto retained=capability.loaded(key);
  need(retained.bytes.get()==canonical.bytes.get()&&!retained.bytes.owner_before(canonical.bytes)&&
       !canonical.bytes.owner_before(retained.bytes)&&runtime::identify_bytes(*retained.bytes).sha256==original.sha256,
       "Actual scratch destruction replaced or damaged retained canonical Bank storage");
  auto next=bank->resources(capability).resolve(key);const auto* handle=std::get_if<assets::AssetHandle>(&next.asset);
  need(handle&&runtime::identify_bytes(handle->read_bytes(8*1024*1024)).sha256==original.sha256,
       "A later real loader resolve cannot read the retained canonical resource");
 }
 const assets::AssetKey absent("Assets/UI/InGame/WorldView/CivisMissingLoaderResourceSource10_773f102.lua");
 need(!bank->has_indexed(absent)&&std::holds_alternative<std::monostate>(bank->resources(capability).resolve(absent).asset),
      "Real absent loader name fabricated a VFS resource handle");
 bool refused=false;try{(void)capability.loaded(absent);}catch(const std::exception&){refused=true;}
 need(refused,"Required absent loader resource fell back to an empty or different captured source");
 bank->require_current();
 std::cout<<"PASS selected loader RESOURCE prerequisite: actual Bank/VFS, independent scratch buffers, retained canonical bytes after real destruction, absent resource refusal; selected VM/temp-ref cleanup/replay/partial coverage NOT RUN\n"<<std::flush;
}
}
int main(int argc,char **argv){try {
 using namespace civ5;
 ui::native_stamp_diagnostic::Collector stamp_collector;
 ui::native_stamp_diagnostic::ScopedCollector stamp_diagnostic(stamp_collector);
 need(argc>=4,"Expected SAVE ORIGINAL_GAME CANONICAL_ASSETS --ui-language TYPE [--cancel|--loader-resources-only]");
 bool cancel=false,loader_resources_only=false;std::string_view selected_language;
 for(int i=4;i<argc;++i){
  const std::string_view option(argv[i]);
  if(option=="--ui-language"){
   need(selected_language.empty()&&i+1<argc,"Expected one explicit --ui-language TYPE");
   selected_language=argv[++i];need(!selected_language.empty(),"Explicit language Type is empty");
  }else if(option=="--cancel"||option=="--loader-resources-only"){
   need(!cancel&&!loader_resources_only,"Duplicate or conflicting actual audio test mode");
   cancel=option=="--cancel";loader_resources_only=option=="--loader-resources-only";
  }else need(false,"Unknown actual audio test option");
 }
 need(!selected_language.empty(),"Required fixture needs an explicit installed --ui-language TYPE");
 const std::filesystem::path save=argv[1],game=argv[2],assets_root=argv[3];
 const auto input_before=runtime::identify_file(save);
 stage("create-hidden-own-WGL-HWND");
 const auto context=std::make_shared<graphics::GlContext>(800,600,false);
 const auto hwnd=static_cast<HWND>(context->native_window_handle());need(hwnd&&IsWindow(hwnd),"Own bank audio actual window absent");
 const auto extent=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(hwnd),
     "civis-bank-audio-actual",[]{return std::uint64_t{1};});
 const auto early=ui::BnwNativeEarlyInputLease::acquire(save,game,assets_root,
     reinterpret_cast<std::uintptr_t>(hwnd),extent);
 stage("capture-real-own-bank-and-selected-host");
 const std::array<assets::CivisRequiredResource,1> required{{{"AudioParams.xml",assets::CivisResourceParse::xml,assets::CivisResourceSelection::winner}}};
 const auto bank=assets::CivisResourceBankOwner::capture(early,required);
 // Production reads the actual process command and local Languages metadata.
 // An argv string alone never issues Locale authority or fabricates a default.
 const auto language_input=runtime::BnwSelectedLanguageInputOwner::capture_from_civis_resources(bank);
 language_input->require_current();
 need(language_input->language()==selected_language,"Actual process language differs from explicit fixture selection");
 language_input->require_current();
 const auto bootstrap=ui::bootstrap_selected_bnw_preview_from_civis_resources(bank,0);
 if(bootstrap.preview.blocker)
  std::cerr<<"ACTUAL_OWN_BANK_AUDIO selected-host-blocker stage="
           <<static_cast<int>(bootstrap.preview.blocker->stage)
           <<" reason="<<bootstrap.preview.blocker->reason<<'\n'<<std::flush;
 need(bootstrap.preview.host&&!bootstrap.preview.blocker&&bootstrap.preview.host->uses_civis_resource_bank(),
      "Own bank audio authentic selected host unavailable");
 need(!bootstrap.preview.host->session().readiness().can_continue(),"Own bank audio fixture falsely enabled whole game");
 if(loader_resources_only){
  stage("selected-loader-resource-prerequisite-only");loader_resource_prerequisite(bank);
  const auto input_after=runtime::identify_file(save);
  need(input_before.bytes==input_after.bytes&&input_before.sha256==input_after.sha256,
       "Loader resource prerequisite modified original selected save");
  need(!bootstrap.preview.host->session().readiness().can_continue(),"Loader resource prerequisite enabled full-game continuation");
  return 0;
 }
 stage("prepare-real-XML-SoundData-profile-and-admission-WAV-before-activation");
 const auto resource=ui::CivisBankAudioSourceOwner::prepare_from_selected_host(bootstrap.preview.host);
 const auto alexander=resource->sound_definition("SND_LEADER_SPEECH_ALEXANDER_PEACEFUL");
 need(alexander.sound_id=="SND_LEADER_SPEECH_ALEXANDER_PEACEFUL"&&
      alexander.filename=="AlexanderPeaceful"&&alexander.load_type=="DYNAMIC_RES"&&
      !alexander.dont_cache&&!alexander.only_load_one_variation,
      "Own bank actual retained AlexanderPeaceful SoundData tuple differs");
 std::cout<<"EARNED_OWN_BANK_AUDIO_SOUND_ROW SND_LEADER_SPEECH_ALEXANDER_PEACEFUL AlexanderPeaceful DYNAMIC_RES dont_cache=false only_load_one_variation=false\n"<<std::flush;
 stage("earned-retained-SND_LEADER_SPEECH_ALEXANDER_PEACEFUL-AlexanderPeaceful-DYNAMIC_RES-dont_cache=false-only_load_one_variation=false");
 const auto repeated=ui::CivisBankAudioSourceOwner::prepare_from_selected_host(bootstrap.preview.host);
 need(resource.get()==repeated.get()&&!resource.owner_before(repeated)&&!repeated.owner_before(resource),
      "Own bank audio preparation created a second context");
 need(resource->inspect_audio_basename("AudioParams.xml")==ui::BnwAudioNativeBasenameResult::inspected_hit,
      "Own bank audio known installed HIT missing");
 need(resource->inspect_audio_basename("CivisAudioAbsent773f102Main441.wav")==ui::BnwAudioNativeBasenameResult::inspected_miss,
      "Own bank audio genuine missing name was not MISS");
 const auto definition=resource->sound_definition("SND_INTERFACE_NEW_ERA");
 need(definition.filename=="NotificationNewEra"&&definition.load_type=="DYNAMIC_RES"&&!definition.only_load_one_variation,
      "Own bank actual cached SoundData differs from installed NewEra definition");
 const auto settings=resource->own_selected_user_settings();const auto measured=runtime::identify_file(settings.selected_path);
 need(measured.bytes==settings.bytes&&measured.sha256==settings.sha256&&!settings.raw.empty(),
      "Own bank real profile snapshot differs from actual selected file");
 ShowWindow(hwnd,SW_SHOWNOACTIVATE);
 ui::ClientWindowInput input;std::unique_ptr<ui::Win32ViewportTransaction> viewport;
 std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> physical;
 std::shared_ptr<ui::CivisClientAudioQueueOwner> queue;
 std::shared_ptr<const ui::BnwAudioWindowActivationLoan> completed_activation;
 bool observed=false;std::exception_ptr failure;
 viewport=std::make_unique<ui::Win32ViewportTransaction>(hwnd,input,
   [](const ui::Win32ViewportTransaction::Retirement&){},[&](const ui::ClientWindowEvent& event){
    if(event.kind!=ui::ClientWindowEventKind::activation)return;
    try{
     need(event.activation&&event.activation->sequence()==1&&!observed&&physical,
          "Own bank audio requires real first WM_ACTIVATE after physical setup");
     viewport->require_activation_window_owner(*event.activation);
     physical->with_actual_window_loan([&](const auto &loan){
      stage("real-first-WM_ACTIVATE-own-bank-device-queue-factory");
      queue=ui::CivisClientAudioQueueOwner::from_first_actual_activation(resource,*event.activation,loan);
      const auto same=ui::CivisClientAudioQueueOwner::from_first_actual_activation(resource,*event.activation,loan);
      need(queue.get()==same.get()&&!queue.owner_before(same)&&!same.owner_before(queue),
           "Own bank activation factory reseeded/recreated the live queue");
      const auto ready=queue->startup();need(ready.own_resource_bank&&ready.own_registry_loaded&&
       ready.own_device_format_admitted&&ready.scripts_2d>0&&ready.sound_definitions>0&&
       !ready.original_startup_rng_verified&&!ready.original_audio_parity_verified,
       "Own bank actual device/registry startup missing or fake Original grant");
     });
     completed_activation=event.activation;observed=true;
    }catch(...){failure=std::current_exception();throw;}
   });
 physical=viewport->capture_renderer_window_owner(context);
 stage("OS-activate-own-window-no-synthetic-message");SetActiveWindow(hwnd);
 if(failure)std::rethrow_exception(failure);
 need(observed&&queue&&!viewport->failed(),"Own bank first real activation did not finish");
 bool expired=false;try{completed_activation->require_current();}catch(const std::exception&){expired=true;}
 need(expired,"Borrowed actual WM_ACTIVATE remained usable after return");
 // This is an explicit OWN fixture UI publication. It exercises the installed
 // mapping and real PCM queue, never acknowledges OriginalPopupShown/GameCore.
 const auto emitted=queue->transition(1,ui::UiAudioKind::popup,"BUTTONPOPUP_NEW_ERA");
 need(emitted.disposition==ui::LiveUiAudioDisposition::processed&&emitted.emission==ui::UiAudioEmitStatus::queued,
      "Own bank concrete fixture UI event did not enqueue installed cue");
 need(queue->transition(1,ui::UiAudioKind::popup,"BUTTONPOPUP_NEW_ERA").disposition==ui::LiveUiAudioDisposition::duplicate_or_stale,
      "Own bank repeated frame/source sequence replayed sound");
 stage("creator-capture-actual-PCM-then-real-waveOut-worker");
 const auto submitted=queue->pump();need(submitted.size()==1&&submitted.front().status==ui::CivisAudioEventStatus::queued&&
   !submitted.front().native_basename.empty()&&!submitted.front().native_sha256.empty()&&submitted.front().numeric_draws==2,
   "Own bank actual selected PCM/RNG did not submit one voice");
 if(!cancel){
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);bool done=false;
  while(std::chrono::steady_clock::now()<deadline&&!done){
   const auto progress=queue->pump();for(const auto &row:progress){
    if(row.status==ui::CivisAudioEventStatus::completed){
     need(row.event.sequence==submitted.front().event.sequence&&row.native_sha256==submitted.front().native_sha256&&
       row.numeric_state==submitted.front().numeric_state,"Own bank completed voice correlation changed");done=true;
    }else need(row.status==ui::CivisAudioEventStatus::pending,"Own bank OS voice failed before released completion");
   }
   if(!done)std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  need(done&&queue->pump().empty(),"Own bank PCM completion absent/repeated");
 }
 need(queue->transition(2,ui::UiAudioKind::popup,"BUTTONPOPUP_NEW_ERA").emission==ui::UiAudioEmitStatus::queued&&
      queue->transition(3,ui::UiAudioKind::popup,"BUTTONPOPUP_NEW_ERA").emission==ui::UiAudioEmitStatus::queued,
      "Own bank actual backlog events absent");
 const auto closed=queue->close();need(closed.discarded_unsubmitted_events==2&&closed.backend.closed&&
  closed.backend.device_released&&closed.backend.device.accepted==1&&
  closed.backend.device.completed+closed.backend.device.canceled==1,
  "Own bank actual worker join/cancel/device release or backlog retirement differs");
 const auto again=queue->close();need(again.backend.device.accepted==1&&again.discarded_unsubmitted_events==2,
      "Own bank close is not idempotent");
 bool refused=false;try{(void)queue->transition(4,ui::UiAudioKind::popup,"BUTTONPOPUP_NEW_ERA");}catch(const std::exception&){refused=true;}
 need(refused,"Own bank closed queue accepted another voice");
 const auto input_after=runtime::identify_file(save);const auto settings_after=runtime::identify_file(settings.selected_path);
 need(input_before.bytes==input_after.bytes&&input_before.sha256==input_after.sha256&&
      measured.bytes==settings_after.bytes&&measured.sha256==settings_after.sha256,
      "Own bank source fixture modified selected save/profile");
 viewport.reset();physical.reset();
 std::cout<<"PASS own Civis bank XML/SoundData/settings/actual HWND/real released PCM queue; OriginalPopup/startupRNG/audio parity unverified\n";
 return 0;
}catch(const std::exception& error){std::cerr<<"FAIL actual own bank audio: "<<error.what()<<'\n';return 1;}}
#else
int main(){return 77;}
#endif
