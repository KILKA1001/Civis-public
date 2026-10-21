#include "civ5/ui/bnw_audio_native_settings_owner.hpp"
#ifdef _WIN32
#include <stdexcept>
namespace civ5::ui::test {
// Authored fixture helper: caller must supply genuine live producer leases.
// Not an executed acceptance test, not a factory for counterfeit window/receipt.
void check_native_audio_settings_live(
 std::shared_ptr<const BnwAudioCompletedResourceOwner> resource,
 const BnwAudioWindowActivationLoan& actual_activation,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& actual_physical,
 const std::array<std::int32_t,4>& independently_read_percentages,
 std::uint32_t independently_computed_game_sfx_bits){
 const auto owner=BnwAudioNativeSettingsOwner::from_first_actual_activation(resource,actual_activation,actual_physical);
 const auto result=owner->category_factor("GAME_SFX");
 if(result.saved_percentages!=independently_read_percentages||result.factor_bits!=independently_computed_game_sfx_bits||
    result.activation_sequence!=actual_activation.sequence()||result.master_current_bits!=0x3f800000u)
  throw std::runtime_error("Actual native settings/category differs from independent source expectations");
 owner->require_current();
 // Positive sequence1 cannot be consumed twice; terminal refusal is mandatory.
 bool refused=false;try{owner->consume_actual_activation(actual_activation);}catch(const std::runtime_error&){refused=true;}
 if(!refused)throw std::runtime_error("Actual repeated activation accepted");
 refused=false;try{owner->require_current();}catch(const std::runtime_error&){refused=true;}
 if(!refused)throw std::runtime_error("Failed settings owner resurrected");
}
}
#endif
