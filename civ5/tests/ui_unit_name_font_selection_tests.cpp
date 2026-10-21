#include "civ5/ui/unit_name_font_selection.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace civ5::ui;
namespace {
void need(bool v,const char*m){if(!v)throw std::runtime_error(m);}
template<class F>void refused(F&&f,const char*m){bool r=false;try{f();}catch(const std::exception&){r=true;}need(r,m);}
}
int main(int argc,char**argv){try{
    need(argc==2,"One shared assets-root argument required");
    const std::filesystem::path root(argv[1]);
    std::array<GlyphGenGeometry,3> g;
    for(std::size_t n=0;n<g.size();++n)g[n]=glyphgen_geometry(read_glyphgen(
        root/"Assets/UI/Fonts/Tw Cent MT"/(std::string(unit_name_source_fonts[n])+".ggxml")));
    std::array<const GlyphGenGeometry*,3> pointers{&g[0],&g[1],&g[2]};
    const std::string text="WWWWWWWWWWWW";
    const auto w24=project_glyphgen_line(g[0],text).advance_width;
    const auto w20=project_glyphgen_line(g[1],text).advance_width;
    need(w24>w20&&w20>0,"Actual authored W fixture must distinguish 24/20 widths");
    UnitNameOwnFontSelectionRequest r{"selected/UnitPanel.xml","UnitPanel","UnitName",text,"SoftShadow",1,2,3,4,w24+50};
    auto s=select_unit_name_own_glyphgen_font(r,text,r.style,pointers);
    need(s.selected_index==0&&s.measured_indices==std::array<std::size_t,2>{0,0}&&
        s.measured_widths==std::array<std::int32_t,2>{w24,w24},"exact threshold must preserve24 and remeasure24");
    r.button_width=w24+49;s=select_unit_name_own_glyphgen_font(r,text,r.style,pointers);
    need(s.selected_index==1&&s.measured_indices==std::array<std::size_t,2>{0,1}&&
        s.draw_line.advance_width==w20,"one over threshold must select20 then use its actual draw width");
    r.button_width=w20+50;s=select_unit_name_own_glyphgen_font(r,text,r.style,pointers);
    need(s.selected_index==1,"equal second width must keep20");
    r.button_width=w20+49;s=select_unit_name_own_glyphgen_font(r,text,r.style,pointers);
    need(s.selected_index==2&&s.measured_indices==std::array<std::size_t,2>{0,1}&&
        s.draw_line.advance_width==project_glyphgen_line(g[2],text).advance_width,
        "second overflow must select14 with SAME actual glyph geometry");
    r.button_width=1;s=select_unit_name_own_glyphgen_font(r,text,r.style,pointers);
    need(s.selected_index==2&&s.draw_line.advance_width>r.button_width-50,"source has no further fallback after14");
    auto missing=pointers;missing[2]=nullptr;
    bool typed_missing=false;
    try {select_unit_name_own_glyphgen_font(r,text,r.style,missing);}
    catch(const UnitNameOwnFontSelectionError&e){typed_missing=e.refusal.issue==UnitNameOwnFontSelectionIssue::missing_candidate&&e.refusal.candidate_index==2;}
    need(typed_missing,"missing required14 must report its typed candidate refusal");
    r.button_width=w24+50;
    need(select_unit_name_own_glyphgen_font(r,text,r.style,missing).selected_index==0,"unused missing14 must not alter source24 branch");
    missing[0]=nullptr;refused([&]{select_unit_name_own_glyphgen_font(r,text,r.style,missing);},"missing initial24 must refuse");
    refused([&]{select_unit_name_own_glyphgen_font(r,"OTHER",r.style,pointers);},"changed text must refuse stale command");
    refused([&]{select_unit_name_own_glyphgen_font(r,text,"Shadow",pointers);},"changed style must refuse");
    r.text="\xd0\x96";refused([&]{select_unit_name_own_glyphgen_font(r,r.text,r.style,pointers);},"authored missing Cyrillic cannot silently use N/A or smaller font");
    r.text="\xc0\xaf";refused([&]{select_unit_name_own_glyphgen_font(r,r.text,r.style,pointers);},"invalid UTF8 cannot measure");
    r.text="W\nW";refused([&]{select_unit_name_own_glyphgen_font(r,r.text,r.style,pointers);},"multiline must refuse before draws");
    r.text="[ICON_W]";refused([&]{select_unit_name_own_glyphgen_font(r,r.text,r.style,pointers);},"markup cannot become plain title widths");
    static_assert(!UnitNameOwnFontSelection::native_get_size_val&&!UnitNameOwnFontSelection::native_label_ready);
    std::cout<<"PASS actual GlyphGen source thresholds/ordered remeasurement/draw geometry/refusals; own C++ only\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
