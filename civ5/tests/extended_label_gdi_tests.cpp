// Real Windows GDI API exercises with synthetic caller/source identities.
// These are not original Civ V settings, screenshots or native pixel evidence.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../validation/extended_label_gdi.hpp"
#include <windows.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>

namespace {
using namespace civ5::validation;
void check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
template<class F> void rejected(F action,ExtendedLabelGdiFailure failure,const char *message) {
    try { action(); } catch (const ExtendedLabelGdiError &error) {
        if (error.failure==failure) return;
        throw std::runtime_error(std::string(message)+": unexpected failure: "+error.what());
    }
    throw std::runtime_error(message);
}
SelectedExtendedFontRequest request() {
    // The explicit compatibility height is a fixture. No conversion from the
    // authored font name or locale scale is claimed by this test.
    return {"TwCenMT18","Shadow",std::string(64,'a'),std::string(64,'b'),
        std::string(64,'c'),"Arial",1,1,{"Arial",18,128,128}};
}
DecodedPlainLabel label(std::string_view text) {
    return decode_extended_plain_label(text,std::string(64,'d'),{true,false,false});
}
struct ActualFont {
    HDC dc=CreateCompatibleDC(nullptr);
    HFONT font=nullptr;
    HGDIOBJ old=nullptr;
    explicit ActualFont(const ExtendedLabelLogFont &observed) {
        if (!dc) throw std::runtime_error("Independent test DC creation failed");
        LOGFONTW f{}; f.lfHeight=observed.height;f.lfWidth=observed.width;
        f.lfEscapement=observed.escapement;f.lfOrientation=observed.orientation;f.lfWeight=observed.weight;
        f.lfItalic=observed.italic;f.lfUnderline=observed.underline;f.lfStrikeOut=observed.strikeout;
        f.lfCharSet=observed.charset;f.lfOutPrecision=observed.out_precision;f.lfClipPrecision=observed.clip_precision;
        f.lfQuality=observed.quality;f.lfPitchAndFamily=observed.pitch_and_family;
        std::copy(observed.face.begin(),observed.face.end(),f.lfFaceName);
        font=CreateFontIndirectW(&f);
        if (!font) throw std::runtime_error("Independent test font creation failed");
        old=SelectObject(dc,font);
        if (!old || old==HGDI_ERROR) throw std::runtime_error("Independent test font selection failed");
    }
    ~ActualFont() { if (dc && old && old!=HGDI_ERROR) SelectObject(dc,old);if (font) DeleteObject(font);if (dc) DeleteDC(dc); }
};
struct IndependentBitmap {
    HDC dc; HBITMAP bitmap=nullptr; HGDIOBJ old=nullptr; void *pixels=nullptr;
    static constexpr int side=96,origin_x=40,baseline=60;
    explicit IndependentBitmap(HDC context):dc(context) {
        BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth=side;info.bmiHeader.biHeight=-side;
        info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
        bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
        check(bitmap && pixels,"Independent large raster oracle allocation failed");
        old=SelectObject(dc,bitmap);check(old && old!=HGDI_ERROR,"Independent bitmap selection failed");
        SetTextColor(dc,RGB(255,255,255));SetBkMode(dc,TRANSPARENT);SetTextAlign(dc,TA_LEFT|TA_BASELINE);
    }
    ~IndependentBitmap() {if(old && old!=HGDI_ERROR) SelectObject(dc,old);if(bitmap) DeleteObject(bitmap);}
    void matches_plane(const PreparedExtendedLabelBase &result,const ExtendedLabelGlyphReceipt &record) {
        std::fill_n(static_cast<std::uint32_t *>(pixels),side*side,0u);
        const auto glyph=static_cast<wchar_t>(record.native_glyph_id);
        check(ExtTextOutW(dc,origin_x,baseline,ETO_GLYPH_INDEX,nullptr,&glyph,1,nullptr) && GdiFlush(),
            "Independent actual glyph-index rasterization failed");
        const auto *raw=static_cast<const std::uint8_t *>(pixels);const auto &g=record.geometry;
        std::size_t direct_nonzero=0,retained_nonzero=0;
        for (int y=0;y<side;++y) for(int x=0;x<side;++x) direct_nonzero+=raw[(y*side+x)*4+1]!=0;
        for (int y=0;y<g.height;++y) for(int x=0;x<g.width;++x) {
            const auto from=(baseline-g.origin_y+y)*side+origin_x+g.origin_x+x;
            const auto actual=raw[from*4+1];
            const auto retained=result.coverage.alpha[static_cast<std::size_t>(g.atlas_y+y)*result.coverage.width+g.atlas_x+x];
            check(actual==retained,"Retained glyph plane disagrees with independent larger actual GDI draw");
            retained_nonzero+=retained!=0;
        }
        check(direct_nonzero==retained_nonzero,"Retained glyph rectangle dropped ink from the larger actual GDI draw");
    }
};
PreparedExtendedLabelBase actual_complete_coverage() {
    auto q=request();
    std::vector labels{label("A \xD0\x94" "A"),
        decode_extended_plain_label("ASCII",std::string(64,'e'),{false,true,true})};
    auto result=prepare_extended_label_base(q,labels);
    check(extended_label_base_payload_matches(result),"Fresh actual GDI result failed its payload recheck");
    check(result.labels.size()==2 && result.labels[0].glyphs.size()==4 && result.labels[1].glyphs.size()==5,
        "ASCII/space/Cyrillic whole-label coverage was shortened");
    for (const auto &l:result.labels) for (const auto &g:l.glyphs)
        check(g.origin==civ5::ui::NativeExtendedGlyphOrigin::generated_base,"Authored fallback entered entire-label preparation");
    const auto &r=result.receipt;
    check(r.requested_logfont.height==18 && r.applied_logfont.height==18 && r.resolved_face=="Arial" &&
        r.applied_logfont.quality==PROOF_QUALITY && r.applied_logfont.charset==DEFAULT_CHARSET &&
        r.ext_text_out_options==ETO_GLYPH_INDEX && r.map_mode==MM_TEXT && r.background_mode==TRANSPARENT,
        "Actual LOGFONT/DC/draw state was not observed");
    check(!r.native_metrics_or_pixels_proven && !r.source_derived_logfont_proven,
        "Compatibility request was promoted to native proof");
    check(result.geometry.ascent==r.metrics.ascent && result.geometry.descent==r.metrics.descent &&
        result.geometry.line_height==r.metrics.height,"Actual GDI text metrics were replaced with GG-name guesses");
    ActualFont independent(r.applied_logfont);
    IndependentBitmap bitmap(independent.dc);
    std::size_t spaces=0;
    for (const auto &g:r.generated_glyphs) {
        const auto character=static_cast<wchar_t>(g.code_point); WORD id=0xffff;SIZE extent{};
        check(GetGlyphIndicesW(independent.dc,&character,1,&id,GGI_MARK_NONEXISTING_GLYPHS)!=GDI_ERROR &&
            GetTextExtentPoint32W(independent.dc,&character,1,&extent),"Independent GDI observation failed");
        check(id==g.native_glyph_id && extent.cx==g.extent_width && extent.cy==g.extent_height,
            "Receipt glyph IDs/advances disagree with independent actual GDI observation");
        bitmap.matches_plane(result,g);
        if (g.code_point==' ') {
            ++spaces;check(g.zero_ink && g.geometry.width==0 && g.geometry.height==0 && g.extent_width>0,
                "Valid zero-ink space lost its actual advance or gained fabricated ink");
        } else check(!g.zero_ink,"Ordinary ASCII/Cyrillic glyph unexpectedly has no actual ink");
    }
    check(spaces==1,"Requested space metric record was absent or duplicated");
    check(std::any_of(r.generated_glyphs.begin(),r.generated_glyphs.end(),[](const auto &g){return g.code_point==0x414;}),
        "Actual Cyrillic nominal glyph was not generated");
    check(std::any_of(result.coverage.alpha.begin(),result.coverage.alpha.end(),[](auto c){return c!=0;}),
        "Entire-label atlas has no actual glyph coverage");
    return result;
}
void zero_ink_and_payload_mutations(const PreparedExtendedLabelBase &baseline) {
    auto q=request();q.gdi.atlas_width=1;q.gdi.atlas_height=1;
    const std::vector labels{label("  ")};
    const auto space=prepare_extended_label_base(q,labels);
    check(space.receipt.generated_glyphs.size()==1 && space.labels[0].glyphs.size()==2 &&
        space.labels[0].advance_width>0 && space.coverage.alpha==std::vector<std::uint8_t>{0} &&
        extended_label_base_payload_matches(space),"Space-only label requires imaginary atlas ink");
    auto mutated=baseline;mutated.coverage.alpha[0]^=1;
    check(!extended_label_base_payload_matches(mutated),"Modified coverage payload was accepted");
    mutated=baseline;++mutated.geometry.glyphs[0].b;
    check(!extended_label_base_payload_matches(mutated),"Modified glyph geometry was accepted");
    mutated=baseline;++mutated.receipt.generated_glyphs[0].native_glyph_id;
    check(!extended_label_base_payload_matches(mutated),"Modified observed actual GDI glyph ID was accepted");
    mutated=baseline;++mutated.receipt.applied_logfont.height;
    check(!extended_label_base_payload_matches(mutated),"Modified applied LOGFONT was accepted");
    mutated=baseline;mutated.receipt.request.language_height_scale=2;
    check(!extended_label_base_payload_matches(mutated),"Modified declared language evidence was accepted");
    mutated=baseline;std::reverse(mutated.labels[0].glyphs.begin(),mutated.labels[0].glyphs.end());
    check(!extended_label_base_payload_matches(mutated),"Reordered entire-label placement was accepted");
    mutated=baseline;mutated.labels[0].glyphs[0].origin=civ5::ui::NativeExtendedGlyphOrigin::authored;
    check(!extended_label_base_payload_matches(mutated),"Mutated authored glyph origin was accepted");
    rejected([&]{extended_label_base_receipt_json(mutated);},ExtendedLabelGdiFailure::settings_changed,
        "Modified preparation still emitted an apparently valid receipt");
}
PreparedExtendedLabelBase actual_explicit_width_coverage(const PreparedExtendedLabelBase &baseline) {
    auto q=request();q.gdi.width=11;
    const std::vector labels{label("A \xD0\x94" "A")};
    auto result=prepare_extended_label_base(q,labels);
    check(result.receipt.requested_logfont.width==11 && result.receipt.applied_logfont.width==11 &&
        result.receipt.applied_logfont.height==18 && extended_label_base_payload_matches(result),
        "Positive explicit native LOGFONT width was silently ignored/defaulted");
    check(result.receipt.applied_settings_sha256!=baseline.receipt.applied_settings_sha256 &&
        result.receipt.coverage_sha256!=baseline.receipt.coverage_sha256 &&
        result.labels[0].advance_width!=baseline.labels[0].advance_width,
        "Positive width did not change actual raster/metrics/settings from compatibility width0");
    ActualFont independent(result.receipt.applied_logfont);IndependentBitmap bitmap(independent.dc);
    for (const auto &g:result.receipt.generated_glyphs) {
        SIZE extent{};const auto character=static_cast<wchar_t>(g.code_point);
        check(GetTextExtentPoint32W(independent.dc,&character,1,&extent) && extent.cx==g.extent_width,
            "Positive-width advance does not match independent actual GDI font");
        bitmap.matches_plane(result,g);
    }
    auto changed=result;++changed.receipt.request.gdi.width;
    check(!extended_label_base_payload_matches(changed),"Changed explicit positive-width request was accepted");
    changed=result;++changed.receipt.applied_logfont.width;
    check(!extended_label_base_payload_matches(changed),"Changed observed applied positive LOGFONT width was accepted");
    return result;
}
void actual_failures() {
    auto q=request();const std::vector labels{label("A\xD0\x99")};
    auto altered=q;altered.gdi.height=48;
    rejected([&]{prepare_extended_label_base(altered,labels);},ExtendedLabelGdiFailure::raster_bounds,
        "Tall actual glyph was silently clipped into the fixed DIB");
    altered=q;altered.gdi.atlas_width=1;altered.gdi.atlas_height=1;
    rejected([&]{prepare_extended_label_base(altered,labels);},ExtendedLabelGdiFailure::atlas_overflow,
        "Insufficient complete-coverage atlas was silently truncated");
    altered=q;altered.gdi.face="CivisDefinitelyMissingFace";
    rejected([&]{prepare_extended_label_base(altered,labels);},ExtendedLabelGdiFailure::substituted_face,
        "Nonexistent requested face silently fell back to a Windows font");
    const std::vector missing{label("\xCD\xB8")}; // U+0378, unassigned: no nominal Arial glyph.
    rejected([&]{prepare_extended_label_base(q,missing);},ExtendedLabelGdiFailure::missing_glyph,
        "Missing nominal glyph silently used a fallback/tofu");
    auto authored=labels;authored[0].flags={true,false,true};
    rejected([&]{prepare_extended_label_base(q,authored);},ExtendedLabelGdiFailure::authored_route,
        "ForceNonIME label incorrectly entered whole-label generated mode");
    auto corrupt=labels;corrupt[0].code_points[0]='B';
    rejected([&]{prepare_extended_label_base(q,corrupt);},ExtendedLabelGdiFailure::unsupported_text,
        "Changed decoded text identity was accepted");
    altered=q;altered.gdi.height=0;
    rejected([&]{prepare_extended_label_base(altered,labels);},ExtendedLabelGdiFailure::invalid_request,
        "Missing explicit height was guessed from selected font name");
    altered=q;altered.gdi.width=-1;
    rejected([&]{prepare_extended_label_base(altered,labels);},ExtendedLabelGdiFailure::invalid_request,
        "Invalid explicit negative LOGFONT width was accepted");
    altered=q;altered.language_width_scale=std::numeric_limits<float>::quiet_NaN();
    rejected([&]{prepare_extended_label_base(altered,labels);},ExtendedLabelGdiFailure::invalid_request,
        "Invalid caller language settings were accepted");
    altered=q;altered.manifest_sha256="unknown";
    rejected([&]{prepare_extended_label_base(altered,labels);},ExtendedLabelGdiFailure::invalid_source_identity,
        "Missing selected font identity was accepted");
    rejected([&]{label("\xF0\x9F\x98\x80");},ExtendedLabelGdiFailure::unsupported_text,"Non-BMP shaping support was invented");
    rejected([&]{label("\xC0\xAF");},ExtendedLabelGdiFailure::unsupported_text,"Malformed UTF-8 was accepted");
    rejected([&]{label("A\nB");},ExtendedLabelGdiFailure::unsupported_text,"Multiline control was rasterized as an ordinary glyph");
}
} // namespace
int main(int argc,char **argv) {
    try {
        const auto result=actual_complete_coverage();
        zero_ink_and_payload_mutations(result);actual_failures();
        const auto positive_width=actual_explicit_width_coverage(result);
        if (argc==2) {
            std::ofstream out(argv[1],std::ios::binary);out<<extended_label_base_receipt_json(result)<<'\n';
            if (!out) throw std::runtime_error("Could not retain actual GDI fixture receipt");
            std::ofstream width_out(std::string(argv[1])+".positive-width.json",std::ios::binary);
            width_out<<extended_label_base_receipt_json(positive_width)<<'\n';
            if (!width_out) throw std::runtime_error("Could not retain actual positive-width GDI fixture receipt");
        } else if (argc!=1) throw std::runtime_error("Expected optional output receipt path only");
        std::cout<<"PASS real GDI entire ASCII/RU coverage, positive/default width, actual IDs/LOGFONT, zero-ink spaces, rejection and receipt mutation guards (fixture; no native parity)\n";
    } catch (const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; }
}
