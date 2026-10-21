// Synthetic plan/coverage fixtures, not original raster or native pixel parity.
#include "civ5/ui/native_extended_font_style.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace civ5::ui;
void check(bool value, const char *reason) { if (!value) throw std::runtime_error(reason); }
template<class F> void rejected(F action, const char *reason) {
    try { action(); } catch (const std::invalid_argument &) { return; }
    throw std::runtime_error(reason);
}
struct Fixture {
    std::vector<std::uint32_t> text{'A', 0x414, 'A'};
    std::vector<NativeExtendedBaseGlyph> glyphs{
        {'A', {2,2,3,3}, NativeExtendedGlyphOrigin::generated_base},
        {0x414, {6,2,3,3}, NativeExtendedGlyphOrigin::generated_base},
        {'A', {10,2,3,3}, NativeExtendedGlyphOrigin::generated_base}};
    NativeExtendedShadowInput input;
    Fixture() {
        input.flags = {true, false, false};
        input.locale_source_sha256 = std::string(64, 'a');
        input.label_source_sha256 = std::string(64, 'b');
        input.active_font = NativeExtendedActiveFont{"TwCenMT18", "Shadow", std::string(64,'c'), std::string(64,'d'), std::string(64,'e')};
        input.foreground = NativeExtendedSourceColor{{.7f,.2f,.4f,1.f}, std::string(64,'f')};
        input.shadow = NativeExtendedSourceColor{{0,0,0,.5f}, std::string(64,'a')};
        input.label_code_points = text; input.base_glyphs = glyphs; input.advance_width = 14;
    }
};
void route_and_coverage() {
    for (unsigned flags = 0; flags < 8; ++flags) {
        const bool extended = flags == 1 || flags == 2 || flags == 3 || flags == 6 || flags == 7;
        check(native_extended_font_route({bool(flags&1),bool(flags&2),bool(flags&4)}) ==
            (extended?NativeExtendedFontRoute::extended_label:NativeExtendedFontRoute::authored_label),
            "Native per-label ForceIME/ForceNonIME priority changed");
    }
    Fixture f;
    const auto plan = make_native_extended_shadow_plan(f.input);
    check(plan.glyphs().size() == 3 && plan.advance_width() == 14, "Entire mixed label/advances were not retained");
    check(plan.active_font().base_plane_sha256 == f.input.active_font->base_plane_sha256,
          "Plan invented a second effect plane");
    check(plan.layers()[0].native_slot == 1 && plan.layers()[1].native_slot == 0 &&
          plan.layers()[0].color.rgba[3] == .5f, "Native slot/color/float alpha ordering changed");
    f.glyphs[0].origin = NativeExtendedGlyphOrigin::authored;
    rejected([&]{make_native_extended_shadow_plan(f.input);}, "ASCII authored fallback was accepted in extended label");
    f.glyphs[0].origin = NativeExtendedGlyphOrigin::generated_base;
    f.input.base_glyphs = std::span(f.glyphs).subspan(1);
    rejected([&]{make_native_extended_shadow_plan(f.input);}, "Partial Cyrillic-only coverage was accepted");
    f.input.base_glyphs = f.glyphs; f.glyphs[1].code_point = 'A';
    rejected([&]{make_native_extended_shadow_plan(f.input);}, "Wrong glyph order was accepted");
    // No ASCII text-content heuristic may switch a runtime-selected label.
    Fixture ascii; ascii.text[1]='D'; ascii.glyphs[1].code_point='D';
    check(make_native_extended_shadow_plan(ascii.input).glyphs().size()==3, "ASCII-only eligible label lost its runtime route");
}
void visible_plan_fixture() {
    Fixture f; const auto plan = make_native_extended_shadow_plan(f.input);
    // Asymmetric real-sized fixture ink. Compare planned layer quads under a
    // partial clip with direct shifted-Base composition; visible overlap must
    // retain foreground while only the exposed bottom/right shadow remains.
    const std::array<float,9> ink{0,1,0, 1,1,.25f, 0,.75f,1};
    const auto sample = [&](NativeExtendedGlyphRect r,int x,int y) {
        if(x<r.x||y<r.y||x>=r.x+r.width||y>=r.y+r.height)return 0.f;
        return ink[(y-r.y)*3+x-r.x];
    };
    bool saw_shadow=false,saw_foreground=false;
    for(int y=3;y<6;++y) for(int x=3;x<6;++x) {
        float actual=.8f;
        for(std::size_t layer=0;layer<plan.layers().size();++layer) {
            const auto &paint=plan.layers()[layer];
            const float coverage=sample(plan.glyph_rectangle(0,layer),x,y)*paint.color.rgba[3];
            actual=paint.color.rgba[0]*coverage+actual*(1-coverage);
        }
        const float foreground=sample({2,2,3,3},x,y);
        const float shadow=sample({3,3,3,3},x,y)*.5f;
        const float expected=.7f*foreground+.8f*(1-shadow)*(1-foreground);
        check(std::abs(actual-expected)<1e-6f,"Translated Base/color order changed partial visible ink");
        saw_shadow|=shadow>0&&foreground==0; saw_foreground|=foreground>0;
    }
    check(saw_shadow&&saw_foreground,"Clipped fixture did not exercise separate background/foreground coverage");
    check(plan.glyph_rectangle(2,0)==NativeExtendedGlyphRect{11,3,3,3} &&
          plan.glyph_rectangle(2,1)==f.glyphs[2].rectangle,"Shadow rebased layout or used wrong coordinate space");
}
void missing_sources_and_overflow() {
    Fixture f;
    auto bad=f.input; bad.active_font.reset();
    rejected([&]{make_native_extended_shadow_plan(bad);},"Missing active font accepted");
    bad=f.input; bad.active_font->style="Stroke";
    rejected([&]{make_native_extended_shadow_plan(bad);},"Stroke was silently approximated as Shadow");
    bad=f.input; bad.shadow.reset();
    rejected([&]{make_native_extended_shadow_plan(bad);},"Effect source color was invented");
    bad=f.input; bad.foreground->rgba[0]=std::numeric_limits<float>::quiet_NaN();
    rejected([&]{make_native_extended_shadow_plan(bad);},"Nonfinite source color accepted");
    bad=f.input; bad.shadow->rgba[3]=1.1f;
    rejected([&]{make_native_extended_shadow_plan(bad);},"Invalid source alpha accepted");
    bad=f.input; bad.active_font->base_plane_sha256.clear();
    rejected([&]{make_native_extended_shadow_plan(bad);},"Missing Base plane identity accepted");
    bad=f.input; bad.label_source_sha256="unbound";
    rejected([&]{make_native_extended_shadow_plan(bad);},"Missing selected XML identity accepted");
    bad=f.input; bad.flags.force_non_ime=true;
    rejected([&]{make_native_extended_shadow_plan(bad);},"Authored route was replaced by runtime Shadow");
    f.glyphs[0].rectangle={std::numeric_limits<std::int32_t>::max()-3,2,3,3};
    rejected([&]{make_native_extended_shadow_plan(f.input);},"Shadow right-edge overflow accepted");
    f.glyphs[0].rectangle={2,std::numeric_limits<std::int32_t>::max(),0,0};
    rejected([&]{make_native_extended_shadow_plan(f.input);},"Shadow origin overflow accepted");
    f.glyphs[0].rectangle={-3,-3,-1,3};
    rejected([&]{make_native_extended_shadow_plan(f.input);},"Negative glyph dimensions accepted");
}
}
int main() {try {
    route_and_coverage(); visible_plan_fixture(); missing_sources_and_overflow();
    check(!NativeExtendedFontProof::native_pixel_parity_proven,"Portable fixture claimed native pixels");
    std::cout<<"PASS portable native label route, complete Base coverage and source-backed Shadow plan (fixture, not native pixels)\n";
    return 0;
} catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}}
