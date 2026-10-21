#include "civ5/graphics/gl_context.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/bnw_audio_native_filename_probes.hpp"
#include "civ5/ui/bnw_audio_native_settings_owner.hpp"
#include "civ5/ui/bnw_new_era_native_xml_owner.hpp"
#include "civ5/ui/bnw_audio_sound_data_owner.hpp"
#include "civ5/ui/bnw_audio_native_pcm_device_owner.hpp"
#include "civ5/ui/bnw_audio_constructor_rng_owner.hpp"
#include "civ5/ui/civis_standalone_audio_owner.hpp"
#include "civ5/ui/civis_client_audio_queue_owner.hpp"
#include <chrono>
#include <thread>
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/win32_viewport_transaction.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <locale>
#include <optional>
#include <stdexcept>
#include <string_view>

#ifdef _WIN32
namespace civ5::ui::test {
void check_native_audio_rng_numeric_independent_vectors();
}
namespace {
using namespace civ5;
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void stage(const char* name){std::cerr<<"ACTUAL_AUDIO_SOURCE "<<name<<'\n'<<std::flush;}
std::string_view trimmed(std::string_view text){
 const auto first=text.find_first_not_of(" \t\r");if(first==std::string_view::npos)return {};
 return text.substr(first,text.find_last_not_of(" \t\r")-first+1);
}
// Deliberately independent conventional INI fixture reader. This accepts only
// the exact authored four names and representable decimal input. It neither
// supplies production settings nor uses the production parser/default writer.
std::array<std::int32_t,4> expected_percentages(const ui::BnwAudioNativeUserSettings& selected){
 const auto pin_before=runtime::identify_file(selected.selected_path);
 need(pin_before.bytes==selected.bytes&&pin_before.sha256==selected.sha256,"Independent settings source differs");
 std::ifstream file;file.imbue(std::locale::classic());file.open(selected.selected_path,std::ios::binary);
 need(bool(file),"Independent existing settings open failed");
 const std::string raw((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
 need(raw==selected.raw,"Independent settings bytes differ from actual Boot-selected read");
 constexpr std::array<std::string_view,4> keys{"Music.Volume","Effects.Volume","Ambience.Volume","Speech.Volume"};
 std::array<std::optional<std::int32_t>,4> values{};
 bool user=false;std::string_view rest=raw;
 while(!rest.empty()){
  const auto newline=rest.find('\n');const auto line=trimmed(rest.substr(0,newline));
  if(newline==std::string_view::npos)rest={};else rest.remove_prefix(newline+1);
  if(line.empty()||line.front()==';')continue;
  if(line.front()=='['){user=line=="[User Settings]";continue;}
  if(!user)continue;
  const auto equal=line.find('=');if(equal==std::string_view::npos)continue;
  const auto key=trimmed(line.substr(0,equal));const auto text=trimmed(line.substr(equal+1));
  for(std::size_t i=0;i<keys.size();++i)if(key==keys[i]){
   need(!values[i],"Independent settings duplicate fixture key");
   std::int32_t value=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
   need(parsed.ec==std::errc{}&&parsed.ptr==text.data()+text.size(),"Independent decimal settings fixture unsupported");
   values[i]=value;
  }
 }
 std::array<std::int32_t,4> result{};
 for(std::size_t i=0;i<values.size();++i){need(values[i].has_value(),"Independent settings fixture missing required exact key");result[i]=*values[i];}
 const auto pin_after=runtime::identify_file(selected.selected_path);
 need(pin_after.bytes==pin_before.bytes&&pin_after.sha256==pin_before.sha256,"Independent settings changed during read");
 return result;
}
const content::XmlNode& child(const content::XmlNode& row,std::string_view name){
 const content::XmlNode* found=nullptr;
 for(const auto& item:row.children)if(item.name==name){need(!found,"Actual NewEra duplicate fixture field");found=&item;}
 need(found!=nullptr,"Actual NewEra required fixture field absent");return *found;
}
}
int main(int argc,char** argv){try{
 using namespace civ5;
 need(argc==4||(argc==5&&(std::string_view(argv[4])=="--sound-data"||std::string_view(argv[4])=="--pcm-device"||std::string_view(argv[4])=="--rng-constructor"||std::string_view(argv[4])=="--civis-backend"||std::string_view(argv[4])=="--civis-client-queue"||std::string_view(argv[4])=="--civis-client-queue-cancel")),
      "Expected original SAVE ORIGINAL_GAME CANONICAL_ASSETS [--sound-data|--pcm-device|--rng-constructor|--civis-backend|--civis-client-queue|--civis-client-queue-cancel]");
 const bool inspect_sound_data=argc==5;
 const bool inspect_pcm_device=argc==5&&std::string_view(argv[4])=="--pcm-device";
 const bool inspect_rng_constructor=argc==5&&std::string_view(argv[4])=="--rng-constructor";
 const bool inspect_civis_backend=argc==5&&std::string_view(argv[4])=="--civis-backend";
 const bool inspect_client_queue=argc==5&&(std::string_view(argv[4])=="--civis-client-queue"||std::string_view(argv[4])=="--civis-client-queue-cancel");
 const bool inspect_client_cancel=argc==5&&std::string_view(argv[4])=="--civis-client-queue-cancel";
 const std::filesystem::path save=argv[1],game=argv[2],canonical=argv[3];
 const auto original=runtime::identify_file(save);
 stage("create-own-hidden-WGL-HWND");
 const auto context=std::make_shared<graphics::GlContext>(800,600,false);
 const auto hwnd=static_cast<HWND>(context->native_window_handle());
 need(hwnd&&IsWindow(hwnd),"Actual own GL window unavailable");
 const auto extent=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(hwnd),
     "bnw-audio-actual-source",[]{return std::uint64_t{1};});
 stage("acquire-EarlyInput");
 const auto early=ui::BnwNativeEarlyInputLease::acquire(save,game,canonical,
     reinterpret_cast<std::uintptr_t>(hwnd),extent);
 const auto parent=ui::BnwNativePackageDiscoveryOwner::create(early);
 stage("register-primary32");parent->register_native_dx9_primary_prefix();
 const auto boot_settings=ui::BnwNativeResourceBootSettingsOwner::create(parent);
 stage("register-texture-quality");parent->register_native_dx9_texture_quality_branch(boot_settings);
 stage("register-Common13");parent->register_native_common_high_quality_prefix();
 stage("complete-actual-Skin-Bank-SelectedRead-and-retain-host");
 const auto bootstrap=ui::bootstrap_selected_bnw_preview_from_early_package(parent,0);
 need(!bootstrap.preview.blocker&&bootstrap.preview.host,"Actual completed bootstrap supplier blocked");
 need(!bootstrap.preview.host->session().readiness().can_continue(),"Source fixture falsely enabled full game");
 stage("consume-already-issued-canonical-Parent-Bank");
 const auto resource=ui::BnwAudioCompletedResourceOwner::from_registered_parent(parent);
 resource->require_current();
 need(resource->inspect_native_audio_basename("AudioParams.xml")==ui::BnwAudioNativeBasenameResult::inspected_hit,
      "Actual Bank did not inspect known AudioParams HIT");
 constexpr std::string_view absent_stem="CivisAudioAbsent773f102Main441";
 need(resource->inspect_native_audio_basename("CivisAudioAbsent773f102Main441.wav")==ui::BnwAudioNativeBasenameResult::inspected_miss,
      "Actual Bank did not inspect absent filename MISS");
 const auto selected_settings=resource->read_native_audio_user_settings();
 const auto percentages=expected_percentages(selected_settings);
 // Showing with NOACTIVATE occurs before the input transaction attaches, so
 // any initial resize cannot retire the subsequently captured physical loan.
 ShowWindow(hwnd,SW_SHOWNOACTIVATE);
 ui::ClientWindowInput input;
 std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> physical;
 bool observed=false;std::exception_ptr callback_failure;
 std::shared_ptr<ui::BnwAudioNativeSettingsOwner> live_settings;
 std::shared_ptr<ui::CivisStandaloneAudioOwner> audio_backend;
 std::shared_ptr<ui::CivisClientAudioQueueOwner> client_audio;
 std::shared_ptr<const ui::BnwAudioWindowActivationLoan> retired_activation;
 std::unique_ptr<ui::Win32ViewportTransaction> viewport;
 viewport=std::make_unique<ui::Win32ViewportTransaction>(hwnd,input,
    [](const ui::Win32ViewportTransaction::Retirement&){},
    [&](const ui::ClientWindowEvent& event){
     if(event.kind!=ui::ClientWindowEventKind::activation)return;
     try{
      need(event.activation&&event.activation->sequence()==1&&!observed,"Actual first activation sequence required");
      need(viewport&&physical,"Actual activation arrived before physical source setup returned");
      viewport->require_activation_window_owner(*event.activation);
      physical->with_actual_window_loan([&](const auto& actual_window){
       stage("actual-WM_ACTIVATE-XML-settings-HIT-MISS");
       const auto xml=ui::BnwNewEraNativeXmlOwner::from_actual_activation(resource,*event.activation,actual_window);
       const auto operands=xml->operands();
       need(child(operands.script.row,"ScriptID").text=="AS2D_INTERFACE_NEW_ERA"&&
            child(operands.script.row,"SoundID").text=="SND_INTERFACE_NEW_ERA"&&
            child(operands.definition.row,"Filename").text=="NotificationNewEra",
            "Actual NewEra XML operands differ");
       live_settings=ui::BnwAudioNativeSettingsOwner::from_first_actual_activation(resource,*event.activation,actual_window);
       const auto factor=live_settings->category_factor("GAME_SFX");
       const float expected=static_cast<float>(std::clamp(percentages[1],std::int32_t{0},std::int32_t{100}))*0.01f;
       need(factor.saved_percentages==percentages&&factor.factor_bits==std::bit_cast<std::uint32_t>(expected)&&
            factor.master_current_bits==0x3f800000u&&factor.activation_sequence==1,
            "Actual native settings differ from independent selected source");
       const auto probes=ui::BnwAudioNativeFilenameProbes::from_actual_activation(resource,*event.activation,actual_window);
       const auto missing=probes->inspect_uncached_variation_count(absent_stem);
       constexpr std::array<std::string_view,8> expected_missing{
        "CivisAudioAbsent773f102Main441-000.wav","CivisAudioAbsent773f102Main441.wav",
        "CivisAudioAbsent773f102Main441-000.mp3","CivisAudioAbsent773f102Main441.mp3",
        "CivisAudioAbsent773f102Main441-000.ogg","CivisAudioAbsent773f102Main441.ogg",
        "CivisAudioAbsent773f102Main441-000.binka","CivisAudioAbsent773f102Main441.binka"};
       need(missing.count==0&&missing.selected_extension.empty()&&missing.inspected.size()==expected_missing.size(),
            "Actual Bank all-MISS variation closure differs");
       for(std::size_t i=0;i<expected_missing.size();++i)
        need(missing.inspected[i].basename==expected_missing[i]&&missing.inspected[i].result==ui::BnwAudioNativeBasenameResult::inspected_miss,
             "Actual Bank probe order/result differs");
       if(inspect_sound_data){
        stage("actual-SoundData-source-count-cache");
        const auto sound=ui::BnwAudioSoundDataOwner::from_actual_activation(resource,*event.activation,actual_window);
        const auto raw=sound->source_operands();
        need(raw.sound_id=="SND_INTERFACE_NEW_ERA"&&raw.filename=="NotificationNewEra"&&raw.load_type=="DYNAMIC_RES"&&
             raw.dont_cache.form==ui::BnwSoundDataFieldForm::empty&&
             raw.only_load_one_variation.form==ui::BnwSoundDataFieldForm::empty,
             "Actual SoundData empty/native-default operands were changed");
        const auto parsed=sound->native_definition();
        need(parsed.constructor_load_type==-1&&parsed.resolved_load_type==1&&
             !parsed.dont_cache&&!parsed.only_load_one_variation,
             "Actual empty SoundData Boolean/constructor/load-type result differs from original parser");
        const auto cold=sound->inspect_unprefixed_variation_count();
        const auto cached=sound->inspect_unprefixed_variation_count();
        need(!cold.reused_source_count_cache&&cached.reused_source_count_cache&&
             cold.inspected.queried_stem==raw.filename&&cached.inspected.queried_stem==raw.filename&&
             cold.inspected.count==cached.inspected.count&&
             cold.inspected.selected_extension==cached.inspected.selected_extension&&
             cold.inspected.inspected.size()==cached.inspected.inspected.size(),
             "Actual source-derived count cache did not retain returned count/extension");
        bool voice_blocked=false;try{sound->require_native_voice_ready();}
        catch(const std::runtime_error&){voice_blocked=true;}
        need(voice_blocked,"Incomplete native SoundData/RNG/resource/voice path returned successful readiness");
        sound->require_current();
        if(inspect_pcm_device){
         stage("actual-source-PCM-waveOutOpen-format-admission");
         const auto device=ui::BnwAudioNativePcmDeviceOwner::from_actual_activation(resource,*event.activation,actual_window);
         const auto actual_pcm=device->source_format();
         need(!actual_pcm.basename.empty()&&!actual_pcm.sha256.empty()&&actual_pcm.pcm.data_bytes>0,
              "Actual source PCM/device admission lacks returned payload");
         device->require_current();
         device->close(); //Actual checked OS release, not destructor inference.
        }
       }
       if(inspect_rng_constructor){
        stage("actual-single-Parent-constructor-RNG-component");
        const auto rng=ui::BnwAudioConstructorRngOwner::from_actual_activation(resource,*event.activation,actual_window);
        const auto again=ui::BnwAudioConstructorRngOwner::from_actual_activation(resource,*event.activation,actual_window);
        need(rng.get()==again.get()&&!rng.owner_before(again)&&!again.owner_before(rng),"Actual Parent RNG supplier created a second context");
        const auto initial=rng->constructor_state();
        need(initial.constructor_prefix.empty()&&initial.numeric_state==7&&initial.numeric_draws==0&&
             !initial.original_startup_complete,"Constructor fields falsely acknowledged original startup history");
        need(rng->draw_numeric_component(0)==0,"Native bound0 numeric component differs");
        const auto first=again->constructor_state();
        const auto expected_first=std::uint32_t{7}*std::uint32_t{214013}+std::uint32_t{2531011};
        need(first.numeric_state==expected_first&&first.numeric_draws==1&&!first.original_startup_complete,
             "Actual single owned constructor numeric state did not consume bound0");
        rng->require_current();
       }
       if(inspect_client_queue){
        stage("actual-client-queue-closed-factory-first-activation");
        client_audio=ui::CivisClientAudioQueueOwner::from_first_actual_activation(resource,*event.activation,actual_window);
        const auto repeated=ui::CivisClientAudioQueueOwner::from_first_actual_activation(resource,*event.activation,actual_window);
        need(client_audio.get()==repeated.get()&&!client_audio.owner_before(repeated)&&!repeated.owner_before(client_audio),"Actual client queue replaced canonical context");
       }
       if(inspect_civis_backend){
        stage("actual-Civis-full-master-XML-registry-device-startup");
        audio_backend=ui::CivisStandaloneAudioOwner::from_first_actual_activation(resource,*event.activation,actual_window);
        const auto startup=audio_backend->startup();
        need(startup.own_registry_loaded&&startup.own_device_format_admitted&&startup.selected_xml_files>4&&
             startup.scripts_2d>0&&startup.scripts_3d>0&&startup.sound_definitions>0&&
             !startup.original_startup_rng_verified&&!startup.original_audio_parity_verified,
             "Civis actual startup incomplete or falsely acknowledged original parity");
       }
       xml->require_current();probes->require_current();live_settings->require_current();
       event.activation->require_current();actual_window.require_current();
       retired_activation=event.activation;observed=true;
      });
     }catch(...){callback_failure=std::current_exception();throw;}
    });
 physical=viewport->capture_renderer_window_owner(context);
 stage("activate-only-own-window");
 SetActiveWindow(hwnd); // OS activation of this own HWND; no fabricated WM_ACTIVATE.
 if(callback_failure)std::rethrow_exception(callback_failure);
 need(observed&&!viewport->failed(),"OS did not deliver successful first actual activation");
 need(retired_activation!=nullptr,"Actual activation loan was not observed");
 bool rejected=false;try{retired_activation->require_current();}catch(const std::exception&){rejected=true;}
 need(rejected,"Completed actual WndProc loan remained active");
 live_settings->require_current();resource->require_current(); if(inspect_client_queue){
  stage("actual-client-publish-dedup-pump-shutdown");
  need(bool(client_audio),"Actual long-lived client queue not retained");
  const auto emitted=client_audio->script_transition(10,"CivisActualClientAcceptance","AS2D_INTERFACE_NEW_ERA");
  need(emitted.disposition==ui::LiveUiAudioDisposition::processed&&emitted.emission==ui::UiAudioEmitStatus::queued&&!emitted.playback,
       "Actual own client event was confused with immediate device playback");
  need(client_audio->script_transition(10,"CivisActualClientAcceptance","AS2D_INTERFACE_NEW_ERA").disposition==ui::LiveUiAudioDisposition::duplicate_or_stale,
       "Actual repeated client cue replayed");
  const auto submit=client_audio->pump();need(submit.size()==1&&submit.front().status==ui::CivisAudioEventStatus::queued,
       "Actual client pump did not submit one genuine Windows voice");
  if(!inspect_client_cancel){
   const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);bool complete=false;
   while(std::chrono::steady_clock::now()<deadline){
    const auto results=client_audio->pump();for(const auto& result:results){
     if(result.status==ui::CivisAudioEventStatus::completed)complete=true;
     else need(result.status==ui::CivisAudioEventStatus::pending,"Actual client voice failed before released completion");
    }
    if(complete)break;
   std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }
   need(complete&&client_audio->pump().empty(),"Actual released client voice missing/repeated");
  }
  need(client_audio->script_transition(11,"CivisActualClientAcceptance","AS2D_INTERFACE_NEW_ERA").emission==ui::UiAudioEmitStatus::queued,
       "Actual backlog cue not queued");
  need(client_audio->script_transition(12,"CivisActualClientAcceptance","AS2D_INTERFACE_NEW_ERA").emission==ui::UiAudioEmitStatus::queued,
       "Actual second backlog cue not queued");
  const auto stopped=client_audio->close();
  need(stopped.discarded_unsubmitted_events==2&&stopped.backend.closed&&stopped.backend.device_released&&
       stopped.backend.device.accepted==1&&stopped.backend.device.completed+stopped.backend.device.canceled==1,
       "Actual client shutdown did not join/cancel/release exact submitted voice and discard unsubmitted backlog");
  const auto again=client_audio->close();need(again.backend.device.accepted==1&&again.discarded_unsubmitted_events==2,
       "Actual idempotent close changed receipts");
  bool queue_closed=false;try{(void)client_audio->script_transition(13,"CivisActualClientAcceptance","AS2D_INTERFACE_NEW_ERA");}
  catch(const std::runtime_error&){queue_closed=true;}need(queue_closed,"Closed actual client queue accepted new audio");
  std::cout<<"PASS actual own client queue/event dedup/Windows voice/real close and backlog discard; OriginalPopup/audio parity unverified\n";
 }

 if(inspect_civis_backend){
  stage("actual-Civis-manual-event-real-Windows-voice");
  need(bool(audio_backend),"Actual Civis startup owner absent after WndProc returned");
  const auto catalog=audio_backend->event_catalog();ui::UiAudioQueue queue(catalog);
  need(queue.emit_script("CivisDeviceAcceptance","AS2D_INTERFACE_NEW_ERA")==ui::UiAudioEmitStatus::queued,
       "Actual manual Civis sound event was not queued");
  const auto events=queue.drain();need(events.size()==1,"Actual manual sound event count differs");
  const auto submitted=audio_backend->play(events.front());
  need(submitted.status==ui::CivisAudioEventStatus::queued&&!submitted.native_sha256.empty()&&submitted.numeric_draws==2,
       "Actual native source/own RNG/Windows voice submission failed");
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);bool complete=false;
  while(std::chrono::steady_clock::now()<deadline){
   const auto result=audio_backend->poll();need(result.has_value(),"Actual voice completion disappeared");
   if(result->status==ui::CivisAudioEventStatus::completed){
    need(result->event.sequence==events.front().sequence&&result->native_sha256==submitted.native_sha256,
         "Actual released PCM completion lost event/source identity");complete=true;break;
   }
   need(result->status==ui::CivisAudioEventStatus::pending,"Actual Windows voice failed");
   std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  need(complete,"Actual Windows voice did not complete/release within finite deadline");
  need(!audio_backend->poll(),"Actual released voice produced duplicate completion");
  const auto replay=audio_backend->play(events.front());
  need(replay.status==ui::CivisAudioEventStatus::failed,"Consumed manual event replayed");
  audio_backend->require_current();
  std::cout<<"PASS actual Civis manual event/native SelectedRead/own RNG/Windows PCM released completion; "
              "original popup delivery/startup RNG/audio parity remain unverified\n";
 }
 stage("numeric-only-independent-RNG-vectors");
 ui::test::check_native_audio_rng_numeric_independent_vectors();
 need(runtime::identify_file(save).sha256==original.sha256,"Actual audio fixture changed original save");
 std::cout<<"PASS actual own HWND/Parent/Bank/SelectedRead XML/settings/HIT/MISS source acceptance; "
             "own backend acceptance only if explicitly selected above; no original audio parity or full game\n";
 return 0;
}catch(const std::exception& error){std::cerr<<"FAIL actual audio source: "<<error.what()<<'\n';return 1;}}
#else
int main(){std::cerr<<"Actual audio source fixture requires Windows x64\n";return 1;}
#endif
