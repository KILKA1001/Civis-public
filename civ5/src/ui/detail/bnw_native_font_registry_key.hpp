#pragma once
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
namespace civ5::ui::detail {
// Pure key component ONLY: Original899BF0/838C90. A key, authored GGXML or
// matching source digest cannot issue UIManager96CC/Font pointer authority.
inline std::uint32_t bnw_native_font_registry_key(std::optional<std::string_view>input){
 if(!input)return 0;
 auto raw=*input;if(const auto nul=raw.find('\0');nul!=raw.npos)raw=raw.substr(0,nul);
 if(raw.empty())return 0;
 if(raw.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
  throw std::runtime_error("Native font C-string length outside original signed domain");
 // Original never tests character0 as a separator. Preserve that edge, not
 // conventional filesystem basename semantics. A trailing separator after0
 // produces empty suffix whose CRC is FFFFFFFF, distinct from empty input0.
 auto first=raw.size()-1;
 while(first!=0){const auto c=raw[first];if(c=='\\'||c=='/'||c==':'){++first;break;}--first;}
 std::string name(raw.substr(first));
#if defined(_WIN32)
 // Use the real current Windows CRT case conversion, not a supplied upper
 // callback/table. Same policy is used by current native FPK hash port.
 ::_strupr(name.data());
 std::uint32_t hash=0xffffffffu;
 for(unsigned char byte:name){hash^=byte;for(unsigned bit=0;bit<8;++bit)hash=(hash>>1)^((hash&1)?0xedb88320u:0u);}
 return hash;
#else
 throw std::runtime_error("Native font registry requires actual Windows CRT uppercase supplier");
#endif
}
}
