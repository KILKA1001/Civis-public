#include "civ5/graphics/bnw_worldview_system_title_run.hpp"
#include <iostream>
#include <limits>
using namespace civ5;
namespace {
void check(bool v,const char*s){if(!v)throw std::runtime_error(s);}
template<class F>void refused(F&&f){bool no=false;try{f();}catch(const std::exception&){no=true;}check(no,"Controlled negative unexpectedly accepted");}
}
int main(){try{
 // Pure original width predicate: these numbers are controlled own fixtures,
 // not actual font/Label widths or a source-current producer.
 check(!ui::unit_name_own_width_exceeds_button(230,280)&&ui::unit_name_own_width_exceeds_button(231,280),
  "UnitName strict threshold/equality changed");
 check(!ui::unit_name_own_width_exceeds_button(0,50)&&ui::unit_name_own_width_exceeds_button(0,49),
  "Negative threshold was clamped");
 check(ui::unit_name_own_width_exceeds_button(std::numeric_limits<std::int32_t>::max(),1),"Width subtraction overflowed");
 refused([]{ui::unit_name_own_width_exceeds_button(-1,280);});refused([]{ui::unit_name_own_width_exceeds_button(0,0);});
 // Existing numeric cursor leaf only. No synthetic quote enters the private
 // title issuer or an attached actual cache/atlas.
 ui::BnwSystemFontSlotQuote numeric;numeric.requested_glyph_index=7;numeric.slot.glyph_index=7;numeric.slot.advance_float=3.75f;
 const auto leaf=graphics::continue_bnw_extended_glyph_cursor(numeric,{9,9,0x037f,true});
 check(leaf.truncated_advance==3&&leaf.cursor_x==12&&leaf.maximum_width==12&&!leaf.native_label_ready&&
  !leaf.production_cursor_observed,"Controlled cursor truncation/profile scope differs");
 refused([&]{graphics::continue_bnw_extended_glyph_cursor(numeric,{9,9,0x017f,true});});
 refused([]{graphics::BnwWorldViewSystemTitleRun::create({},0x037f);});
 graphics::BnwWorldViewSystemTitleRunQuote empty;refused([&]{empty.snapshot_no_current();});
 std::cout<<"PASS controlled own width/cursor rules; actual camera issuer/GDI/cache/atlas/render gates remain separate\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
