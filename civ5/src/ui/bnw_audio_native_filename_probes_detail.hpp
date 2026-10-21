#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <stdexcept>
#include "civ5/ui/bnw_audio_native_filename_probes.hpp"
namespace civ5::ui::audio_probe_detail {
inline void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
inline std::string require_stem(std::string_view borrowed){
 std::string stem(borrowed);
 need(!stem.empty()&&stem.size()<=249,"Native audio stem bound unsupported");
 for(const char byte:stem){const auto c=static_cast<unsigned char>(byte);need(c>=32&&c<127&&c!='/'&&c!='\\'&&c!=':',"Native audio stem must be ordinary ASCII basename");}
 need(stem!="."&&stem!="..","Native audio stem traversal refused");return stem;
}
// Independent test seam only: production supplies a closed lambda to real
// completed Parent/Bank inspected record. Arbitrary test functors grant no owner.
template<class Inspect>
BnwAudioInspectedVariationCount collect(std::string stem,std::int32_t cap,std::uint64_t sequence,Inspect&& inspect){
 need(cap>=0,"Native audio negative variation-cap branch unsupported");
 BnwAudioInspectedVariationCount result{std::move(stem),{},cap,0,sequence,{}};
 constexpr std::array<std::string_view,4> extensions{".wav",".mp3",".ogg",".binka"};
 const auto probe=[&](const std::string& name){
  const auto actual=inspect(name);
  need(actual==BnwAudioNativeBasenameResult::inspected_hit||actual==BnwAudioNativeBasenameResult::inspected_miss,
       "Native audio inspection returned unsupported result");
  result.inspected.push_back({name,actual});return actual==BnwAudioNativeBasenameResult::inspected_hit;
 };
 for(const auto extension:extensions){
  std::uint32_t count=0;
  while(count<16&&(cap==0||count<static_cast<std::uint32_t>(cap))){
   // Native literal %s-%03d%s, finite admitted indices0..15.
   const auto digits=std::string("-")+(count<10?"00":"0")+std::to_string(count);
   if(!probe(result.queried_stem+digits+std::string(extension)))break;
   ++count;
  }
  if(count==0&&probe(result.queried_stem+std::string(extension)))count=1;
  if(count>0){result.count=count;result.selected_extension=extension;return result;}
 }
 return result;
}
}
