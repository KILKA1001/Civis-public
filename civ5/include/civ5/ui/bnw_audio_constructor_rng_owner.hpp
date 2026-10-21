#pragma once
#include "civ5/ui/bnw_audio_source_catalog.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include <memory>
#ifdef _WIN32
namespace civ5::ui {
struct BnwAudioConstructorState {
 std::string constructor_prefix;
 std::uint32_t numeric_state=0,numeric_draws=0;
 //Source-owned constructor fields never prove the original backend/XML
 // startup callbacks or the original earlier audio-event draw history.
 bool original_startup_complete=false;
};
// One owned numeric context per authentic registered Parent lifetime. No
// caller seed/state/import; tombstones prevent replacing lost/retired state.
// This is a constructor/numeric service, not permission for a native event,
// voice, current post-startup prefix or original prior-history equivalence.
class BnwAudioConstructorRngOwner final {
 public:
 static std::shared_ptr<const BnwAudioConstructorRngOwner> from_actual_activation(
  std::shared_ptr<const BnwAudioCompletedResourceOwner>,const BnwAudioWindowActivationLoan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
 ~BnwAudioConstructorRngOwner();
 BnwAudioConstructorRngOwner(const BnwAudioConstructorRngOwner&)=delete;
 BnwAudioConstructorRngOwner& operator=(const BnwAudioConstructorRngOwner&)=delete;
 void require_current()const;
 BnwAudioConstructorState constructor_state()const;
 //Numeric component query only; there is no native event/voice-success
 // result. All calls consume the SAME private state even for bound0/1.
 std::uint32_t draw_numeric_component(std::uint32_t bound)const;
 private:
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwAudioConstructorRngOwner(std::shared_ptr<Impl>);
};
}
#endif
