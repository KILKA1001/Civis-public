#pragma once
#include <cstdint>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string_view>
namespace civ5::ui::detail {
// Native835330 rejects high-bit bytes before calling836080: false, no store.
// MSVC90 fail/overflow writes differ from newer num_get. Admit only proven
// ASCII decimal in-range success; unsupported input stops before destination.
inline bool bnw_native_font_integer(std::string_view text,std::int32_t& destination){
 if(text.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
  throw std::runtime_error("Font attribute span exceeds native signed length domain");
 for(unsigned char c:text)if(c&0x80)return false;
 const auto white=[](char c){return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\f'||c=='\v';};
 std::size_t at=0;while(at<text.size()&&white(text[at]))++at;
 bool negative=false;if(at<text.size()&&(text[at]=='+'||text[at]=='-'))negative=text[at++]=='-';
 const auto digits=at;std::uint64_t magnitude=0;
 const std::uint64_t bound=negative?2147483648ULL:2147483647ULL;
 while(at<text.size()&&text[at]>='0'&&text[at]<='9'){
  const auto next=static_cast<unsigned>(text[at++]-'0');
  if(magnitude>(bound-next)/10)throw std::runtime_error("Font MSVC90 overflow-write semantics unsupported before destination");
  magnitude=magnitude*10+next;
 }
 if(at==digits)throw std::runtime_error("Font MSVC90 malformed/fail-write semantics unsupported before destination");
 // Locale grouping punctuation is not claimed equivalent between libraries.
 if(at<text.size()&&!white(text[at])&&!((text[at]>='A'&&text[at]<='Z')||(text[at]>='a'&&text[at]<='z')))
  throw std::runtime_error("Font locale-dependent numeric suffix unsupported before destination");
 if(std::locale()!=std::locale::classic())throw std::runtime_error("Font current stream locale is outside selected C-locale profile");
 std::stringstream input(std::ios::in|std::ios::out);
 input.write(text.data(),static_cast<std::streamsize>(text.size()));
 std::int32_t extracted;input>>extracted;
 // Native mask6 checks badbit/failbit; eofbit alone succeeds. Do not require
 // whole-token consumption. Precheck does not replace the actual extraction.
 if(input.bad()||input.fail())throw std::runtime_error("Font actual stream extraction failed in qualified success domain");
 const auto expected=negative?-static_cast<std::int64_t>(magnitude):static_cast<std::int64_t>(magnitude);
 if(extracted!=expected)throw std::runtime_error("Font actual stream differs from qualified decimal grammar");
 destination=extracted;return true;
}
}
