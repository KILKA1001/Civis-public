#include "civ5/ui/anchored_text_measurement.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace civ5::ui;
namespace {
void check(bool value,const char *message) {if(!value) throw std::runtime_error(message);}
template<class F> void rejected(F function,const char *message) {
    bool threw=false;try {function();}catch(const std::exception &) {threw=true;}check(threw,message);
}
void run() {
    // Mathematical painter fixture; no actual native text/hitbox is claimed.
    // Includes negative left bearing, right overhang and Shadow layer bounds.
    const std::array<WorldViewRect,3> glyphs{{{-2,-18,10,20},{8,-17,13,19},{9,-16,14,21}}};
    const auto read=measure_right_center_text_quads({482,837,0,27},20,24,24,glyphs,{0,0,1274,925});
    check(read.origin_x==462 && read.center_y_twice==1701 && read.compatibility_line_top==838 && read.baseline_y==862,
        "Right alignment or half-pixel source center lost");
    check(read.clipped_glyph_bounds && read.clipped_glyph_bounds->x==460 && read.clipped_glyph_bounds->y==844 &&
        read.clipped_glyph_bounds->width==25 && read.clipped_glyph_bounds->height==23,
        "Actual projected negative bearing/overhang/effect quad union wrong");
    const auto clipped=measure_right_center_text_quads({482,837,0,27},20,24,24,glyphs,{465,850,10,10});
    check(clipped.clipped_glyph_bounds && clipped.clipped_glyph_bounds->x==465 && clipped.clipped_glyph_bounds->y==850 &&
        clipped.clipped_glyph_bounds->width==10 && clipped.clipped_glyph_bounds->height==10,
        "Visible glyph/effect clip leaked source quad");
    const auto hidden=measure_right_center_text_quads({482,837,0,27},20,24,24,glyphs,{0,0,100,100});
    check(!hidden.clipped_glyph_bounds,"Fully clipped text gained a hover rectangle");
    const auto whitespace=measure_right_center_text_quads({482,837,0,27},20,24,24,{}, {0,0,1274,925});
    check(whitespace.origin_x==462 && !whitespace.clipped_glyph_bounds,"Advance-only whitespace invented visible ink bounds");
    const auto negative=measure_right_center_text_quads({0,-30,0,27},0,24,24,{}, {-100,-100,200,200});
    check(negative.center_y_twice==-33 && negative.compatibility_line_top==-29,"Negative odd center used truncation rounding");
    rejected([&]{measure_right_center_text_quads({1,1,10,27},20,24,24,glyphs,{0,0,100,100});},"Fixed width treated as anchored autosize");
    rejected([&]{measure_right_center_text_quads({1,1,0,27},-1,24,24,glyphs,{0,0,100,100});},"Negative advance accepted");
    rejected([&]{measure_right_center_text_quads({std::numeric_limits<std::int32_t>::min(),1,0,27},1,24,24,glyphs,{0,0,100,100});},"Origin arithmetic overflow accepted");
    rejected([&]{measure_right_center_text_quads({1,std::numeric_limits<std::int32_t>::max(),0,27},1,24,24,glyphs,{0,0,100,100});},"Baseline arithmetic overflow accepted");
    rejected([&]{measure_right_center_text_quads({1,1,0,27},1,24,24,glyphs,{std::numeric_limits<std::int32_t>::max(),0,10,10});},"Clip extent arithmetic overflow accepted");
}
}
int main() {try {run();std::cout<<"Anchored actual-metric quad measurement PASS; compatibility_integer_rounding=1; no_native_autosize_claim=1\n";return 0;}
catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}}