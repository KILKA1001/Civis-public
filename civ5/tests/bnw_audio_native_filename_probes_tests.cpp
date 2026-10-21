#include "civ5/ui/bnw_audio_native_filename_probes.hpp"
#ifdef _WIN32
#include "../src/ui/bnw_audio_native_filename_probes_detail.hpp"
#include <stdexcept>
#include <string>
#include <vector>
namespace civ5::ui::test {
void check_native_audio_filename_probe_component(){
 using R=BnwAudioNativeBasenameResult;
 std::vector<std::string> calls;
 const auto all_miss=audio_probe_detail::collect("Tone",0,12,[&](const std::string& q){calls.push_back(q);return R::inspected_miss;});
 const std::vector<std::string> missing{"Tone-000.wav","Tone.wav","Tone-000.mp3","Tone.mp3","Tone-000.ogg","Tone.ogg","Tone-000.binka","Tone.binka"};
 if(calls!=missing||all_miss.count!=0||!all_miss.selected_extension.empty()||all_miss.inspected.size()!=8)
  throw std::runtime_error("Original all-MISS filename order differs");
 calls.clear();
 const auto base_hit=audio_probe_detail::collect("Tone",0,12,[&](const std::string& q){calls.push_back(q);return q=="Tone.wav"?R::inspected_hit:R::inspected_miss;});
 if(base_hit.count!=1||base_hit.selected_extension!=".wav"||calls!=std::vector<std::string>{"Tone-000.wav","Tone.wav"})
  throw std::runtime_error("Original zero-numbered fallback differs");
 calls.clear();
 const auto cap=audio_probe_detail::collect("Tone",2,12,[&](const std::string& q){calls.push_back(q);return R::inspected_hit;});
 if(cap.count!=2||calls!=std::vector<std::string>{"Tone-000.wav","Tone-001.wav"})
  throw std::runtime_error("Original count cap probes beyond admitted count");
 calls.clear();
 bool refused=false;try{
  (void)audio_probe_detail::collect("Tone",0,12,[&](const std::string& q)->R{
   calls.push_back(q);throw std::runtime_error("Actual read/provider failure, never MISS");
  });
 }catch(const std::runtime_error&){refused=true;}
 if(!refused||calls!=std::vector<std::string>{"Tone-000.wav"})
  throw std::runtime_error("Provider failure was converted to ordinary MISS/fallback");
}

// Authored unrun integration helper. Independent fixture observations are
// expectations only; they cannot issue the actual producer or its result.
void check_native_audio_filename_probes_live(
 std::shared_ptr<const BnwAudioCompletedResourceOwner> actual_resource,
 const BnwAudioWindowActivationLoan& actual_activation,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& actual_physical,
 std::string independently_selected_stem,
 const std::vector<BnwAudioInspectedFilename>& independently_observed_probes,
 std::uint32_t independently_observed_count){
 const auto owner=BnwAudioNativeFilenameProbes::from_actual_activation(actual_resource,actual_activation,actual_physical);
 const auto actual=owner->inspect_uncached_variation_count(independently_selected_stem);
 if(actual.count!=independently_observed_count||actual.inspected.size()!=independently_observed_probes.size())
  throw std::runtime_error("Actual native sound-probe count differs from independent fixture");
 for(std::size_t i=0;i<actual.inspected.size();++i)
  if(actual.inspected[i].basename!=independently_observed_probes[i].basename||
     actual.inspected[i].result!=independently_observed_probes[i].result)
   throw std::runtime_error("Actual native sound-probe order/HIT/MISS differs from independent fixture");
 owner->require_current();
}
}
#endif
