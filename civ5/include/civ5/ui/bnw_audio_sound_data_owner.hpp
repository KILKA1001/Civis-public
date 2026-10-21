#pragma once
#include "civ5/ui/bnw_audio_native_filename_probes.hpp"
#include "civ5/ui/bnw_new_era_native_xml_owner.hpp"
#include <memory>
#ifdef _WIN32
namespace civ5::ui {
// Retained source operands. Empty, absent and explicit fields stay distinct;
// no native Boolean default is inferred from XML presence or a caller value.
enum class BnwSoundDataFieldForm { absent, empty, authored_text };
struct BnwSoundDataFieldOperand {
 BnwSoundDataFieldForm form=BnwSoundDataFieldForm::absent;
 std::string text;
};
struct BnwSoundDataSourceOperands {
 std::string sound_id,filename,load_type;
 BnwSoundDataFieldOperand dont_cache,only_load_one_variation;
 std::vector<content::XmlNode> authored_defaults;
 BnwNewEraNativeXmlFile defining_file;
 std::size_t defining_row=0;
 std::uint64_t actual_activation_sequence=0;
};
// Native85a2d0 initializes the load slot to -1 and both Boolean bytes
// to zero. The authored loader then resolves exact load labels via the
// registry initialized in859659..8596dd. This value is parsed data only;
// it does not issue a native manager/voice/resource capability.
struct BnwNativeSoundDataDefinition {
 std::int32_t constructor_load_type=-1;
 std::int32_t resolved_load_type=-1;
 bool dont_cache=false;
 bool only_load_one_variation=false;
};
struct BnwSoundDataCountObservation {
 BnwAudioInspectedVariationCount inspected;
 bool reused_source_count_cache=false;
};
// Source-derived NewEra SoundData/count consumer. Its private cache belongs
// to this actual source, not the original global manager or native voice bank.
// Native defaults, prefix, RNG history and resource/device admission remain
// mandatory before this can become a complete native SoundData/voice owner.
class BnwAudioSoundDataOwner final {
 public:
 static std::shared_ptr<const BnwAudioSoundDataOwner> from_actual_activation(
  std::shared_ptr<const BnwAudioCompletedResourceOwner>,
  const BnwAudioWindowActivationLoan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
 ~BnwAudioSoundDataOwner();
 BnwAudioSoundDataOwner(const BnwAudioSoundDataOwner&)=delete;
 BnwAudioSoundDataOwner& operator=(const BnwAudioSoundDataOwner&)=delete;
 void require_current()const;
 BnwSoundDataSourceOperands source_operands()const;
 BnwNativeSoundDataDefinition native_definition()const;
 // Uses only the genuine definition's filename; no caller-selected stem,
 // count, extension or inspection callback can populate this private cache.
 BnwSoundDataCountObservation inspect_unprefixed_variation_count()const;
 // Explicit incomplete boundary; it can never return a successful voice or
 // RNG grant until original parser/startup/load/admission effects are owned.
 void require_native_voice_ready()const;
 private:
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwAudioSoundDataOwner(std::shared_ptr<Impl>);
};
}
#endif
