#include "../src/ui/detail/bnw_native_font_registry_key.hpp"
#include <stdexcept>
int main(){
#if defined(_WIN32)
 using civ5::ui::detail::bnw_native_font_registry_key;
 const auto need=[](bool b){if(!b)throw std::runtime_error("Native899BF0 key mismatch");};
 const auto key=[](std::string_view s){return bnw_native_font_registry_key(std::optional<std::string_view>{s});};
 need(bnw_native_font_registry_key(std::nullopt)==0);
 need(key("")==0);
 const auto tw=key("TwCenMT18");
 need(tw==key("twcenmt18"));
 need(tw==key("Assets/UI/Fonts/Tw Cent MT/TwCenMT18"));
 need(tw==key("C:\\Fonts\\TwCenMT18"));
 need(key("font/")==0xffffffffu);
 need(key("C:")==0xffffffffu);
 need(key("/TwCenMT18")!=tw);
 need(key("\\TwCenMT18")!=tw);
 need(key(std::string_view("TwCenMT18\0ignored",17))==tw);
#else
 // The natural acceptance platform is Windows x64. No guessed portable CRT.
 return 2;
#endif
}
