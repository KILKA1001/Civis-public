#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
namespace civ5::ui::detail {
// Pure DATA kernels. A computed filename/marker cannot issue another query,
// nativeFTextureData ctor, pending-row/borrower authority or Font readiness.
inline std::string bnw_native_font_optional_filename(std::string_view source){
 if(source.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
  throw std::runtime_error("Font native filename signed length unsupported");
 for(unsigned char c:source)if(!c||c&0x80)throw std::runtime_error("Font filename outside selected nonNUL ASCII profile");
 const auto slash=source.find_last_of('\\');
 std::string basename;
 if(slash!=source.npos)basename=source.substr(slash+1);
 else{const auto colon=source.find(':');basename=source.substr(colon==source.npos?0:colon+1);}
 if(const auto dot=basename.find_last_of('.');dot!=basename.npos)basename.resize(dot);
 basename+="-index"; // Actual constant0A20808 from native81D0A0 call.
 const auto directory=slash==source.npos?std::string_view{}:source.substr(0,slash+1);
 const auto dot=source.find_last_of('.');
 const auto extension=(dot!=source.npos&&(slash==source.npos||slash<dot))?source.substr(dot):std::string_view{};
 return std::string(directory)+basename+std::string(extension);
}
inline std::int32_t bnw_native_font_bc_marker(std::span<const std::uint8_t> source){
 // Proven selected DDS header domain. Do not read truncated/native unsafe
 // inputs or promote arbitrary codec parser success to FTextureData authority.
 if(source.size()<128||source[0]!='D'||source[1]!='D'||source[2]!='S'||source[3]!=' ')
  throw std::runtime_error("Font metadata projection needs genuine complete selected DDS header");
 const auto u32=[&](std::size_t i){return std::uint32_t(source[i])|(std::uint32_t(source[i+1])<<8)|
  (std::uint32_t(source[i+2])<<16)|(std::uint32_t(source[i+3])<<24);};
 if(u32(4)!=124||u32(76)!=32)throw std::runtime_error("Font metadata DDS header outside proven normalized profile");
 std::array<char,12> type{};
 if(source[32]=='F'&&source[33]=='T'&&source[34]=='X'&&source[35]=='T'){
  for(std::size_t i=0;i<12;++i)type[i]=static_cast<char>(source[116+i]);type[11]='\0';
 }else{constexpr std::string_view literal="Typeless";for(std::size_t i=0;i<literal.size();++i)type[i]=literal[i];}
#if defined(_WIN32)
 static_assert(sizeof(unsigned int)==4);
 unsigned int marker=0; // Explicit native5387BA marker initialization.
 (void)std::sscanf(type.data(),"BC%3x",&marker); // Native actualCRT, width3, return ignored.
 return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(marker));
#else
 throw std::runtime_error("Font BC parser requires actual selected Windows CRT sscanf");
#endif
}
}
