#pragma once
#include "civ5/ui/bnw_audio_source_catalog.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include <memory>
#include "civ5/ui/ui_winmm_audio.hpp"
#ifdef _WIN32
namespace civ5::ui {
class CivisBankAudioSourceOwner;
struct CivisAudioStartup {
 std::size_t selected_xml_files=0,scripts_2d=0,scripts_3d=0,sound_definitions=0;
 bool own_registry_loaded=false,own_device_format_admitted=false;
 bool own_resource_bank=false;
 //Own fresh constructor history is real. Original startup and whole-event
 // audio parity are separate, unverified requirements, never a fake grant.
 bool original_startup_rng_verified=false,original_audio_parity_verified=false;
};
struct CivisAudioShutdown {
 bool closed=false,device_released=false;
 WinMmAudioSnapshot device;
};
enum class CivisAudioEventStatus {queued,pending,completed,busy,rejected,failed};
struct CivisAudioEventResult {
 UiAudioEvent event;
 CivisAudioEventStatus status=CivisAudioEventStatus::rejected;
 std::string native_basename,native_sha256,error;
 std::uint32_t numeric_state=0,numeric_draws=0;
};
//A genuine Civis startup/voice owner, not an impersonation of the original
// Miles/DX interface. Closed resource/physical factories and real WinMM only.
//One real outstanding voice. Event sequences are consumed even on failure;
//caller must feed exactly one live UI queue, never retry partial effects.
class CivisStandaloneAudioOwner final {
 public:
 static std::shared_ptr<CivisStandaloneAudioOwner> from_first_actual_activation(
  std::shared_ptr<const BnwAudioCompletedResourceOwner>,const BnwAudioWindowActivationLoan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
 // Disjoint own-Bank lane, using no Original Parent/read capability.
 static std::shared_ptr<CivisStandaloneAudioOwner> from_first_actual_activation(
  std::shared_ptr<CivisBankAudioSourceOwner>,const BnwAudioWindowActivationLoan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
 ~CivisStandaloneAudioOwner();
 CivisStandaloneAudioOwner(const CivisStandaloneAudioOwner&)=delete;
 CivisStandaloneAudioOwner& operator=(const CivisStandaloneAudioOwner&)=delete;
 void require_current()const;
 CivisAudioStartup startup()const;
 UiAudioCatalog event_catalog()const;
 void consume_actual_activation(const BnwAudioWindowActivationLoan&);
 CivisAudioEventResult play(const UiAudioEvent&);
 //Pending/completion comes from real WHDR_DONE plus successful OS release.
 //No timer or successful enqueue is accepted as playback completion.
 std::optional<CivisAudioEventResult> poll();
 //Also valid after source/window retirement; the owning thread must release
 //actual OS PCM even when source authorization is gone. Repeat is idempotent.
 CivisAudioShutdown close();
 private:
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit CivisStandaloneAudioOwner(std::shared_ptr<Impl>);
};
}
#endif
