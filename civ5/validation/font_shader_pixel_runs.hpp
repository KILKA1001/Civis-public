#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>
namespace civ5::validation {
// C5PXRL1: top-down RGBA pixels, exact bytes (no float conversions).
// LE header: magic[8], width:u32, height:u32, stride:u32 (4 or 16).
// Remaining records: positive run:u32 followed by one exact pixel[stride].
inline std::vector<std::uint8_t> encode_font_shader_pixel_runs(
 std::span<const std::uint8_t> raw,std::uint32_t width,std::uint32_t height,std::uint32_t stride){
 if(!width||!height||(stride!=4&&stride!=16)||
    std::uint64_t(width)*height>std::numeric_limits<std::size_t>::max()/stride||
    std::uint64_t(width)*height*stride!=raw.size())
  throw std::invalid_argument("Invalid pixel run dimensions/format/length");
 std::vector<std::uint8_t> out{'C','5','P','X','R','L','1',0};
 const auto u32=[&](std::uint32_t v){for(unsigned b=0;b<4;++b)out.push_back(static_cast<std::uint8_t>(v>>(b*8)));};
 u32(width);u32(height);u32(stride);
 const auto count=raw.size()/stride;
 for(std::size_t first=0;first<count;){
  std::size_t end=first+1;
  while(end<count&&end-first<std::numeric_limits<std::uint32_t>::max()&&
        std::memcmp(raw.data()+first*stride,raw.data()+end*stride,stride)==0)++end;
  u32(static_cast<std::uint32_t>(end-first));
  out.insert(out.end(),raw.begin()+first*stride,raw.begin()+first*stride+stride);first=end;
 }
 return out;
}
}
