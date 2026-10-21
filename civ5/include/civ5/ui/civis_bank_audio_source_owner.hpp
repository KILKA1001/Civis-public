#pragma once
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/ui/bnw_audio_source_catalog.hpp"
#include "civ5/ui/bnw_audio_native_filename_probes.hpp"
#include "civ5/ui/bnw_audio_native_settings_owner.hpp"
#include "civ5/ui/bnw_audio_constructor_rng_owner.hpp"
#ifdef _WIN32
namespace civ5::ui {
class SelectedBnwPreviewHost;
struct CivisBankSoundData {
 std::string sound_id,filename,load_type;
 bool dont_cache=false,only_load_one_variation=false;
};
// Own Civis bank/OS profile source. No OriginalParent, SelectedRead or Boot
// capability is minted. Factory prepares actual XML and admission WAV before
// the real first activation; later explicit PCM loads stay on this creator.
class CivisBankAudioSourceOwner final {
 public:
 static std::shared_ptr<CivisBankAudioSourceOwner> prepare_from_selected_host(
  const std::shared_ptr<const SelectedBnwPreviewHost>&);
 ~CivisBankAudioSourceOwner();
 CivisBankAudioSourceOwner(const CivisBankAudioSourceOwner&)=delete;
 CivisBankAudioSourceOwner& operator=(const CivisBankAudioSourceOwner&)=delete;
 void require_current()const;
 BnwAudioNativeXmlDocument read_audio_xml(std::string_view)const;
 BnwAudioNativeWavPayload read_audio_wav(std::string_view)const;
 BnwAudioNativeBasenameResult inspect_audio_basename(std::string_view)const;
 BnwAudioNativeUserSettings own_selected_user_settings()const;
 CivisBankSoundData sound_definition(std::string_view)const;
 private:
 friend class CivisStandaloneAudioOwner;
 struct Impl;std::shared_ptr<Impl> impl_;
 std::weak_ptr<CivisBankAudioSourceOwner> factory_owner_;
 explicit CivisBankAudioSourceOwner(std::shared_ptr<Impl>);
 std::shared_ptr<CivisBankAudioSourceOwner> retain_factory_owner(
  const std::shared_ptr<CivisBankAudioSourceOwner>&)const;
 std::shared_ptr<assets::CivisResourceBankOwner> retained_bank()const;
 void bind_first_actual_activation(const BnwAudioWindowActivationLoan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
 void require_same_actual_window(const BnwAudioWindowActivationLoan::SourceHandle&)const;
 void consume_actual_activation(const BnwAudioWindowActivationLoan&);
 BnwAudioInspectedVariationCount inspect_variation_count(std::string_view)const;
 BnwAudioNativeCategoryFactor category_factor(std::string_view)const;
 std::uint32_t draw_numeric_component(std::uint32_t)const;
 BnwAudioConstructorState constructor_state()const;
};
}
#endif
