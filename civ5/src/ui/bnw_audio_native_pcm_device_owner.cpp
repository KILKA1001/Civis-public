#include "civ5/ui/bnw_audio_native_pcm_device_owner.hpp"
#ifdef _WIN32
#include <mmsystem.h>
#include <atomic>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>
namespace civ5::ui {
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::uint16_t u16(std::span<const std::uint8_t> bytes,std::size_t at){
 need(at<=bytes.size()&&bytes.size()-at>=2,"Actual PCM truncated uint16");
 return static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[at])|
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[at+1])<<8));
}
std::uint32_t u32(std::span<const std::uint8_t> bytes,std::size_t at){
 need(at<=bytes.size()&&bytes.size()-at>=4,"Actual PCM truncated uint32");
 std::uint32_t value=0;for(std::size_t i=0;i<4;++i)value|=static_cast<std::uint32_t>(bytes[at+i])<<static_cast<unsigned>(8*i);
 return value;
}
bool label(std::span<const std::uint8_t> b,std::size_t at,std::string_view text){
 if(at>b.size()||text.size()>b.size()-at)return false;
 for(std::size_t i=0;i<text.size();++i)if(b[at+i]!=static_cast<std::uint8_t>(text[i]))return false;
 return true;
}
UiPcmFormat inspect(std::span<const std::uint8_t> bytes){
 need(bytes.size()>=12&&label(bytes,0,"RIFF")&&label(bytes,8,"WAVE"),"Actual native source is not RIFF/WAVE");
 need(static_cast<std::uint64_t>(u32(bytes,4))+8==bytes.size(),"Actual WAVE RIFF size differs");
 UiPcmFormat result;bool format=false,data=false;std::uint16_t alignment=0;std::uint32_t rate=0;
 std::size_t at=12;
 while(at<bytes.size()){
  need(bytes.size()-at>=8,"Actual WAVE trailing incomplete chunk");
  const auto size=static_cast<std::size_t>(u32(bytes,at+4));const auto payload=at+8;
  need(size<=bytes.size()-payload,"Actual WAVE chunk exceeds source");
  if(label(bytes,at,"fmt ")){
   need(!format&&size>=16,"Actual WAVE duplicate/incomplete format");format=true;
   need(u16(bytes,payload)==1,"Actual WAVE codec requires a qualified decoder (PCM only)");
   result.channels=u16(bytes,payload+2);result.sample_rate=u32(bytes,payload+4);rate=u32(bytes,payload+8);
   alignment=u16(bytes,payload+12);result.bits_per_sample=u16(bytes,payload+14);
  }else if(label(bytes,at,"data")){
   need(!data&&size>0,"Actual WAVE duplicate/empty data");data=true;result.data_offset=payload;result.data_bytes=size;
  }
  const auto padded=size+(size&1U);need(padded<=bytes.size()-payload,"Actual WAVE padding exceeds source");at=payload+padded;
 }
 need(format&&data&&(result.channels==1||result.channels==2)&&
      (result.bits_per_sample==8||result.bits_per_sample==16)&&result.sample_rate>0,"Actual PCM format unsupported");
 const auto expected=static_cast<std::uint32_t>(result.channels)*(result.bits_per_sample/8U);
 need(alignment==expected&&result.sample_rate<=std::numeric_limits<std::uint32_t>::max()/expected&&
      rate==result.sample_rate*expected&&result.data_bytes%expected==0,"Actual PCM alignment/rate differs");
 return result;
}
std::string selected_singleton(const BnwSoundDataCountObservation& count){
 need(count.inspected.count==1&&count.inspected.selected_extension==".wav","Actual PCM device requires genuine singleton WAV source");
 std::string name;
 for(const auto& probe:count.inspected.inspected)if(probe.result==BnwAudioNativeBasenameResult::inspected_hit){
  need(name.empty(),"Actual singleton WAV has multiple inspected HITs");name=probe.basename;
 }
 need(!name.empty()&&name.ends_with(".wav"),"Actual singleton WAV did not inspect a source basename");return name;
}
}
struct BnwAudioNativePcmDeviceOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::atomic<bool> retired{false};bool busy=false;
 const std::shared_ptr<const BnwAudioCompletedResourceOwner> resource;
 const std::shared_ptr<const BnwAudioSoundDataOwner> sound;
 BnwAudioNativeWavPayload payload;BnwAudioNativePcmDeviceSource source;HWAVEOUT device=nullptr;
 Impl(std::shared_ptr<const BnwAudioCompletedResourceOwner> r,std::shared_ptr<const BnwAudioSoundDataOwner> s)
  :resource(std::move(r)),sound(std::move(s)){}
 ~Impl(){if(device)waveOutClose(device);} //No headers/DMA were ever submitted.
 struct Call {
  std::shared_ptr<Impl> p;explicit Call(std::shared_ptr<Impl> v):p(std::move(v)){
   need(p->thread==std::this_thread::get_id(),"Actual PCM device source foreign thread");
   need(!p->retired.load(),"Actual PCM device source retired");
   if(p->busy){p->retired.store(true);throw std::runtime_error("Actual PCM device source reentry");}p->busy=true;
  }
  ~Call(){p->busy=false;}
 };
 void validate()const{
  need(!retired.load(),"Actual PCM device source retired");sound->require_current();resource->require_current();
  const auto now=resource->read_native_audio_wav(payload.basename);
  sound->require_current();need(now.basename==payload.basename&&now.sha256==payload.sha256&&now.bytes==payload.bytes,
                               "Actual PCM selected payload changed");
  need(!retired.load(),"Actual PCM device source retired during source callback");
 }
};
BnwAudioNativePcmDeviceOwner::BnwAudioNativePcmDeviceOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwAudioNativePcmDeviceOwner::~BnwAudioNativePcmDeviceOwner(){impl_->retired.store(true);}
std::shared_ptr<const BnwAudioNativePcmDeviceOwner> BnwAudioNativePcmDeviceOwner::from_actual_activation(
 std::shared_ptr<const BnwAudioCompletedResourceOwner> resource,const BnwAudioWindowActivationLoan& borrowed,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& physical){
 const auto loan=borrowed;loan.require_current();
 //SoundData's child factories canonically authenticate resource + physical
 // owners before any source callback; incoming aliases cannot bypass them.
 const auto sound=BnwAudioSoundDataOwner::from_actual_activation(resource,loan,physical);
 loan.require_current();physical.require_current();
 auto p=std::make_shared<Impl>(resource,sound);Impl::Call call(p);
 try{
  const auto basename=selected_singleton(sound->inspect_unprefixed_variation_count());
  p->payload=resource->read_native_audio_wav(basename);loan.require_current();physical.require_current();
  const auto pcm=inspect(p->payload.bytes);p->source={basename,p->payload.sha256,p->payload.bytes.size(),pcm};
  p->validate();loan.require_current();physical.require_current();
  WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=pcm.channels;
  format.nSamplesPerSec=pcm.sample_rate;format.wBitsPerSample=pcm.bits_per_sample;
  format.nBlockAlign=static_cast<WORD>(pcm.channels*(pcm.bits_per_sample/8));format.nAvgBytesPerSec=pcm.sample_rate*format.nBlockAlign;
  const auto opened=waveOutOpen(&p->device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL);
  if(opened!=MMSYSERR_NOERROR){p->device=nullptr;throw std::runtime_error("Actual waveOutOpen format/device admission failed ("+std::to_string(opened)+")");}
  p->validate();loan.require_current();physical.require_current();
  return std::shared_ptr<const BnwAudioNativePcmDeviceOwner>(new BnwAudioNativePcmDeviceOwner(p));
 }catch(...){p->retired.store(true);throw;}
}
void BnwAudioNativePcmDeviceOwner::require_current()const{
 const auto p=impl_;Impl::Call call(p);try{p->validate();need(p->device!=nullptr,"Actual PCM OS device absent");}
 catch(...){p->retired.store(true);throw;}
}
void BnwAudioNativePcmDeviceOwner::close()const{
 const auto p=impl_;Impl::Call call(p);
 try{
  p->validate();need(p->device!=nullptr,"Actual PCM OS device already absent");
  const auto released=waveOutClose(p->device);
  if(released!=MMSYSERR_NOERROR)throw std::runtime_error("Actual waveOutClose admission release failed ("+std::to_string(released)+")");
  p->device=nullptr;p->retired.store(true);
 }catch(...){p->retired.store(true);throw;}
}
BnwAudioNativePcmDeviceSource BnwAudioNativePcmDeviceOwner::source_format()const{
 const auto p=impl_;Impl::Call call(p);try{p->validate();need(p->device!=nullptr,"Actual PCM OS device absent");auto result=p->source;p->validate();return result;}
 catch(...){p->retired.store(true);throw;}
}
}
#endif
