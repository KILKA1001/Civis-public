#pragma once
#include "civ5/ui/civis_standalone_audio_owner.hpp"
#include "civ5/ui/live_ui_audio.hpp"
#ifdef _WIN32
namespace civ5::ui {
struct CivisClientAudioShutdown {
 std::size_t discarded_unsubmitted_events=0;
 CivisAudioShutdown backend;
};
//One actual client queue for the canonical real backend lifetime. Rendered
//frames cannot allocate/reseed/replay it. Source cue sequence is the actual
//publisher's increasing shared popup/notification/Lua event sequence.
class CivisClientAudioQueueOwner final {
 public:
 static std::shared_ptr<CivisClientAudioQueueOwner> from_first_actual_activation(
  std::shared_ptr<const BnwAudioCompletedResourceOwner>,const BnwAudioWindowActivationLoan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
 static std::shared_ptr<CivisClientAudioQueueOwner> from_first_actual_activation(
  std::shared_ptr<CivisBankAudioSourceOwner>,const BnwAudioWindowActivationLoan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
 ~CivisClientAudioQueueOwner();
 CivisClientAudioQueueOwner(const CivisClientAudioQueueOwner&)=delete;
 CivisClientAudioQueueOwner& operator=(const CivisClientAudioQueueOwner&)=delete;
 LiveUiAudioResult transition(std::uint64_t source_sequence,UiAudioKind,std::string_view native_id);
 LiveUiAudioResult script_transition(std::uint64_t source_sequence,std::string_view actual_context,std::string_view script);
 //Call outside mutation/render, after actual publication; backlog is serialized
 //so Audio11's one genuine voice does not discard simultaneous notifications.
 std::vector<CivisAudioEventResult> pump();
 void consume_actual_activation(const BnwAudioWindowActivationLoan&);
 CivisClientAudioShutdown close();
 CivisAudioStartup startup()const;
 std::uint64_t last_source_sequence()const;
 private:
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit CivisClientAudioQueueOwner(std::shared_ptr<Impl>);
};
}
#endif
