#pragma once
#include "civ5/ui/ui_audio_assets.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
namespace civ5::ui::civis_audio_wave_detail {
inline void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
inline std::uint16_t u16(std::span<const std::uint8_t> b,std::size_t at){
 need(at<=b.size()&&b.size()-at>=2,"Civis WAVE uint16 outside payload");
 return static_cast<std::uint16_t>(b[at]|(static_cast<std::uint16_t>(b[at+1])<<8));
}
inline std::uint32_t u32(std::span<const std::uint8_t> b,std::size_t at){
 need(at<=b.size()&&b.size()-at>=4,"Civis WAVE uint32 outside payload");
 std::uint32_t v=0;for(std::size_t i=0;i<4;++i)v|=static_cast<std::uint32_t>(b[at+i])<<static_cast<unsigned>(8*i);return v;
}
inline bool tag(std::span<const std::uint8_t> b,std::size_t at,std::string_view value){
 if(at>b.size()||value.size()>b.size()-at)return false;
 for(std::size_t i=0;i<value.size();++i){
  if(b[at+i]!=static_cast<std::uint8_t>(value[i]))return false;
 }
 return true;
}
struct Decoded {UiPcmFormat pcm;std::vector<std::uint8_t> samples;};
inline std::vector<std::uint8_t> decode_ima_adpcm(std::span<const std::uint8_t> bytes,
                                            const UiImaAdpcmFormat &format) {
    static constexpr std::array<int, 89> steps = {
        7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,
        66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,
        337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,
        1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,
        4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,
        13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
    static constexpr std::array<int, 16> index_delta = {
        -1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};
    std::vector<std::uint8_t> output;
    output.reserve(static_cast<std::size_t>(format.sample_count) *
                   static_cast<std::size_t>(format.channels) * 2);
    auto emit = [&](std::span<const int> frame) {
        if (output.size() / (std::size_t{2} * format.channels) >= format.sample_count) return;
        for (const auto sample : frame) {
            output.push_back(static_cast<std::uint8_t>(sample));
            output.push_back(static_cast<std::uint8_t>(sample >> 8));
        }
    };
    auto decode = [&](int &predictor,int &index,int nibble) {
        const int step = steps[static_cast<std::size_t>(index)];
        int delta = step >> 3;
        if (nibble & 1) delta += step >> 2;
        if (nibble & 2) delta += step >> 1;
        if (nibble & 4) delta += step;
        predictor = std::clamp(predictor + ((nibble & 8) ? -delta : delta),
                               -32768, 32767);
        index = std::clamp(index + index_delta[static_cast<std::size_t>(nibble)], 0, 88);
        return predictor;
    };
    const auto end = format.data_offset + format.data_bytes;
    for (std::uint64_t at = format.data_offset; at < end;) {
        const auto length = std::min<std::uint64_t>(format.block_align, end - at);
        if (length < 4u * format.channels ||
            (format.channels == 2 && (length - 8) % 8 != 0))
            throw std::runtime_error("Invalid IMA ADPCM block header");
        std::array<int,2> predictor{},index{};
        for (std::uint16_t channel=0;channel<format.channels;++channel) {
            const auto header=at+std::uint64_t{4}*channel;
            if (bytes[header+2] > 88 || bytes[header+3] != 0)
                throw std::runtime_error("Invalid IMA ADPCM block header");
            predictor[channel]=std::bit_cast<std::int16_t>(u16(bytes,header));
            index[channel]=bytes[header+2];
        }
        emit(std::span<const int>(predictor.data(),format.channels));
        if (format.channels == 1) {
            for (std::uint64_t i=at+4;i<at+length;++i)
                for (const int nibble : {bytes[i] & 0x0f,bytes[i] >> 4}) {
                    const int sample=decode(predictor[0],index[0],nibble);
                    emit(std::span<const int>(&sample,1));
                }
        } else {
            // Microsoft IMA stereo stores four bytes for the left channel,
            // then four for the right channel, repeating within each block.
            for (std::uint64_t group=at+8;group<at+length;group+=8) {
                std::array<std::array<int,8>,2> channel_samples{};
                for (std::uint16_t channel=0;channel<2;++channel)
                    for (std::uint64_t byte=0;byte<4;++byte) {
                        const auto packed=bytes[group+std::uint64_t{4}*channel+byte];
                        channel_samples[channel][2*byte]=decode(
                            predictor[channel],index[channel],packed & 0x0f);
                        channel_samples[channel][2*byte+1]=decode(
                            predictor[channel],index[channel],packed >> 4);
                    }
                for (std::size_t sample=0;sample<8;++sample) {
                    const std::array<int,2> frame{
                        channel_samples[0][sample],channel_samples[1][sample]};
                    emit(frame);
                }
            }
        }
        at += length;
    }
    if (output.size() != static_cast<std::size_t>(format.sample_count) *
                         static_cast<std::size_t>(format.channels) * 2)
        throw std::runtime_error("IMA ADPCM decoded sample count differs from fact");
    return output;
}

//Same established Civis IMA algorithm, supplied with a fully bounded native
// SelectedRead payload. This does not claim original Miles codec parity.
inline Decoded decode(std::span<const std::uint8_t> b){
 need(b.size()>=12&&tag(b,0,"RIFF")&&tag(b,8,"WAVE")&&static_cast<std::uint64_t>(u32(b,4))+8==b.size(),"Civis RIFF/WAVE source malformed");
 std::optional<std::uint16_t> codec,channels,bits,align,perblock;std::optional<std::uint32_t> fact;
 std::uint32_t rate=0;std::optional<std::size_t> data,size;
 for(std::size_t at=12;at<b.size();){
  need(b.size()-at>=8,"Civis WAVE incomplete chunk");const auto n=static_cast<std::size_t>(u32(b,at+4));const auto p=at+8;
  need(n<=b.size()-p,"Civis WAVE chunk outside payload");
  if(tag(b,at,"fmt ")){
   need(!codec&&n>=16,"Civis WAVE duplicate/incomplete fmt");codec=u16(b,p);channels=u16(b,p+2);rate=u32(b,p+4);align=u16(b,p+12);bits=u16(b,p+14);
   if(*codec==0x11){need(n>=20&&u16(b,p+16)>=2,"Civis IMA extension incomplete");perblock=u16(b,p+18);}
  }else if(tag(b,at,"fact")){need(!fact&&n>=4,"Civis WAVE duplicate/incomplete fact");fact=u32(b,p);}
  else if(tag(b,at,"data")){need(!data&&n>0,"Civis WAVE duplicate/empty data");data=p;size=n;}
  const auto padded=n+(n&1U);need(padded<=b.size()-p,"Civis WAVE padding outside payload");at=p+padded;
 }
 need(codec&&channels&&bits&&align&&data&&size&&(*channels==1||*channels==2)&&rate>0,"Civis WAVE required format/data absent");
 if(*codec==1){
  need(*bits==8||*bits==16,"Civis PCM sample width unsupported");const auto expected=static_cast<std::uint32_t>(*channels)*(*bits/8U);
  need(*align==expected&&*size%expected==0&&rate<=std::numeric_limits<std::uint32_t>::max()/expected,"Civis PCM block alignment/rate invalid");
  const auto samples=b.subspan(*data,*size);return {{*channels,*bits,rate,0,samples.size()},{samples.begin(),samples.end()}};
 }
 need(*codec==0x11&&*bits==4&&perblock&&fact&&*fact>0,"Civis audio codec unsupported (PCM/IMA only)");
 const auto header=std::uint32_t{4}*(*channels);
 need(*align>header&&(*channels==1||(*align-header)%8==0)&&
      *perblock==1+2*(*align-header)/(*channels),"Civis IMA block geometry invalid");
 const auto blocks=*size/(*align),partial=*size%(*align);
 need(blocks>0||partial>0,"Civis IMA data empty");
 need(partial==0||(partial>=header&&(*channels==1||(partial-header)%8==0)),"Civis IMA partial block invalid");
 const auto last=partial?1+2*(partial-header)/(*channels):static_cast<std::size_t>(*perblock);
 const auto total=blocks*(*perblock)+(partial?last:0);
 need(*fact>total-last&&*fact<=total&&static_cast<std::uint64_t>(*fact)*(*channels)*2<=64ULL*1024*1024,"Civis IMA fact/budget invalid");
 const UiImaAdpcmFormat encoded{*data,*size,*align,*perblock,*fact,*channels};
 auto samples=decode_ima_adpcm(b,encoded);return {{*channels,16,rate,0,samples.size()},std::move(samples)};
}
inline void scale(Decoded& value,float gain){
 need(std::isfinite(gain)&&gain>=0&&gain<=1,"Civis audio gain unsupported");
 auto& b=value.samples;
 if(value.pcm.bits_per_sample==8){for(auto& x:b){const auto s=static_cast<int>(x)-128;
   const auto scaled=std::clamp(std::lround(static_cast<double>(s)*gain),-128L,127L);x=static_cast<std::uint8_t>(scaled+128);}}
 else{need(value.pcm.bits_per_sample==16&&b.size()%2==0,"Civis scaled PCM shape invalid");
  for(std::size_t at=0;at<b.size();at+=2){const auto s=std::bit_cast<std::int16_t>(u16(b,at));
   const auto scaled=std::clamp(std::lround(static_cast<double>(s)*gain),-32768L,32767L);
   const auto word=static_cast<std::uint16_t>(static_cast<std::int16_t>(scaled));b[at]=static_cast<std::uint8_t>(word);b[at+1]=static_cast<std::uint8_t>(word>>8);}}
}
}
