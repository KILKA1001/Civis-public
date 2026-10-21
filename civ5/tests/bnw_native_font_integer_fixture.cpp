#include "../src/ui/detail/bnw_native_font_integer.hpp"
int main(){
 using civ5::ui::detail::bnw_native_font_integer;
 const auto need=[](bool b){if(!b)throw std::runtime_error("Native Font integer profile mismatch");};
 std::int32_t value=77;
 need(bnw_native_font_integer("  -13tail",value)&&value==-13);
 need(bnw_native_font_integer("2147483647",value)&&value==2147483647);
 need(bnw_native_font_integer("-2147483648",value)&&value==(-2147483647-1));
 value=77;need(!bnw_native_font_integer(std::string_view("\x80",1),value)&&value==77);
 for(const auto bad:{"2147483648","-2147483649","","abc","1,234"}){
  value=77;bool stopped=false;try{(void)bnw_native_font_integer(bad,value);}catch(const std::runtime_error&){stopped=true;}
  need(stopped&&value==77);
 }
}
