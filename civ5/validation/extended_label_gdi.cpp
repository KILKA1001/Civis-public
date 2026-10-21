#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "extended_label_gdi.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <windows.h>
#include <algorithm>
#include <limits>
#include <set>
#include <bit>
#include <cmath>
#include <map>
#include <memory>
#include <sstream>
#include <iomanip>
#include <locale>

namespace civ5::validation {
namespace {
using Failure = ExtendedLabelGdiFailure;
// Local lifetime owner follows the existing PreviewGdiFont design without
// changing that frozen helper. The LOGFONT construction below additionally
// applies explicit width, verified in original 0x68c945/949/94d/952/987.
// Fixed DIB dimensions/placement are STILL a bounded compatibility raster path.
struct ExtendedLabelGdiFont {
    HDC dc=nullptr;
    HFONT font=nullptr;
    HBITMAP bitmap=nullptr;
    HGDIOBJ old_font=nullptr,old_bitmap=nullptr;
    void *pixels=nullptr;
    static constexpr int side=64;
    ExtendedLabelGdiFont(std::string_view face,std::int32_t height,std::int32_t width) {
        LOGFONTW description{};
        description.lfHeight=height;description.lfWidth=width;
        description.lfCharSet=DEFAULT_CHARSET;description.lfQuality=PROOF_QUALITY;
        std::copy(face.begin(),face.end(),description.lfFaceName);
        dc=CreateCompatibleDC(nullptr);
        if (!dc) throw std::runtime_error("Could not create explicit-width GDI context");
        font=CreateFontIndirectW(&description);
        BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth=side;info.bmiHeader.biHeight=-side;
        info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
        bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
        if (!font || !bitmap || !pixels) {
            release();throw std::runtime_error("Could not create explicit-width GDI font/bitmap");
        }
        old_font=SelectObject(dc,font);old_bitmap=SelectObject(dc,bitmap);
        if (!old_font || old_font==HGDI_ERROR || !old_bitmap || old_bitmap==HGDI_ERROR) {
            release();throw std::runtime_error("Could not select explicit-width GDI objects");
        }
        SetMapMode(dc,MM_TEXT);SetTextColor(dc,RGB(255,255,255));
        SetBkMode(dc,TRANSPARENT);SetTextAlign(dc,TA_LEFT|TA_BASELINE);
    }
    void release() noexcept {
        if (dc) {
            if(old_font && old_font!=HGDI_ERROR) SelectObject(dc,old_font);
            if(old_bitmap && old_bitmap!=HGDI_ERROR) SelectObject(dc,old_bitmap);
        }
        if(font) DeleteObject(font);if(bitmap) DeleteObject(bitmap);if(dc) DeleteDC(dc);
        dc=nullptr;font=nullptr;bitmap=nullptr;
    }
    ~ExtendedLabelGdiFont(){release();}
    ExtendedLabelGdiFont(const ExtendedLabelGdiFont&)=delete;
    ExtendedLabelGdiFont&operator=(const ExtendedLabelGdiFont&)=delete;
};
[[noreturn]] void fail(Failure kind, const char *message,
                      std::optional<std::uint32_t> code_point = std::nullopt) {
    throw ExtendedLabelGdiError(kind, message, code_point);
}
bool digest_syntax(std::string_view digest) {
    return digest.size() == 64 && std::all_of(digest.begin(), digest.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}
std::string bytes_digest(std::string_view bytes) {
    return civ5::runtime::identify_bytes({reinterpret_cast<const std::uint8_t *>(bytes.data()), bytes.size()}).sha256;
}
std::string ascii_face(const wchar_t *face) {
    std::string result;
    for (; *face; ++face) {
        if (*face > 127) fail(Failure::substituted_face, "Resolved GDI face is outside supported ASCII declarations");
        result += static_cast<char>(*face);
    }
    return result;
}
bool same_face(std::string_view a, std::string_view b) {
    const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
        [&](char x, char y) { return lower(x) == lower(y); });
}
ExtendedLabelLogFont logfont_receipt(const LOGFONTW &f) {
    return {f.lfHeight,f.lfWidth,f.lfEscapement,f.lfOrientation,f.lfWeight,
        f.lfItalic,f.lfUnderline,f.lfStrikeOut,f.lfCharSet,f.lfOutPrecision,
        f.lfClipPrecision,f.lfQuality,f.lfPitchAndFamily,ascii_face(f.lfFaceName)};
}
struct Encoding {
    std::vector<std::uint8_t> bytes;
    void number(std::uint32_t value) {
        for (int i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value >> (i*8)));
    }
    void text(std::string_view value) {
        number(static_cast<std::uint32_t>(value.size()));
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    std::string digest() const { return civ5::runtime::identify_bytes(bytes).sha256; }
};
std::string settings_digest(const ExtendedLabelBaseReceipt &r) {
    Encoding e;
    e.text("extended-label-gdi-applied-settings-v1");
    const auto &f=r.applied_logfont;
    for (const auto x:{f.height,f.width,f.escapement,f.orientation,f.weight}) e.number(x);
    for (const auto x:{f.italic,f.underline,f.strikeout,f.charset,f.out_precision,
                      f.clip_precision,f.quality,f.pitch_and_family}) e.number(x);
    e.text(f.face); e.text(r.resolved_face);
    for (const auto x:{r.text_color,r.text_align,r.ext_text_out_options}) e.number(x);
    for (const auto x:{r.background_mode,r.map_mode,r.graphics_mode,r.dpi_x,r.dpi_y,
                      r.bitmap_side,r.draw_origin_x,r.draw_baseline_y}) e.number(x);
    return e.digest();
}
std::string geometry_digest(const civ5::ui::GlyphGenGeometry &g) {
    Encoding e; e.text("extended-label-gdi-geometry-v1");
    e.number(g.atlas_width); e.number(g.atlas_height);
    for (const auto x:{g.line_height,g.ascent,g.descent,g.ascent_adjustment,g.spacing,g.advance_adjustment}) e.number(x);
    e.number(static_cast<std::uint32_t>(g.glyphs.size()));
    for (const auto &glyph:g.glyphs) {
        e.number(glyph.code_point);
        for (const auto x:{glyph.atlas_x,glyph.atlas_y,glyph.width,glyph.height,
                          glyph.a,glyph.b,glyph.c,glyph.origin_x,glyph.origin_y}) e.number(x);
    }
    return e.digest();
}
std::string observation_digest(const PreparedExtendedLabelBase &result) {
    const auto &r=result.receipt; const auto &q=r.request;
    Encoding e; e.text("extended-label-gdi-observation-v1");
    for (const auto *s:{&q.font,&q.style,&q.manifest_sha256,&q.locale_source_sha256,
        &q.language_settings_source_sha256,&q.language_font_name,&q.gdi.face,
        &r.applied_settings_sha256,&r.coverage_sha256,&r.geometry_sha256}) e.text(*s);
    e.number(std::bit_cast<std::uint32_t>(q.language_width_scale));
    e.number(std::bit_cast<std::uint32_t>(q.language_height_scale));
    e.number(q.gdi.height); e.number(q.gdi.width); e.number(q.gdi.atlas_width); e.number(q.gdi.atlas_height);
    const auto &f=r.requested_logfont;
    for (const auto x:{f.height,f.width,f.escapement,f.orientation,f.weight}) e.number(x);
    for (const auto x:{f.italic,f.underline,f.strikeout,f.charset,f.out_precision,f.clip_precision,f.quality,f.pitch_and_family}) e.number(x);
    e.text(f.face);
    const auto &m=r.metrics;
    for (const auto x:{m.height,m.ascent,m.descent,m.internal_leading,m.external_leading,m.average_width,
        m.maximum_width,m.weight,m.overhang,m.digitized_aspect_x,m.digitized_aspect_y}) e.number(x);
    for (const auto x:{m.first_char,m.last_char,m.default_char,m.break_char}) e.number(x);
    for (const auto x:{m.italic,m.underline,m.strikeout,m.pitch_and_family,m.charset}) e.number(x);
    e.number(static_cast<std::uint32_t>(r.generated_glyphs.size()));
    for (const auto &g:r.generated_glyphs) {
        e.number(g.code_point); e.number(g.native_glyph_id);
        e.number(g.extent_width); e.number(g.extent_height);
        e.number(g.metric_black_box_width); e.number(g.metric_black_box_height);
        e.number(g.metric_origin_x); e.number(g.metric_origin_y);
        e.number(g.metric_cell_increment_x); e.number(g.metric_cell_increment_y); e.number(g.zero_ink);
        const auto &p=g.geometry; e.number(p.code_point);
        for (const auto x:{p.atlas_x,p.atlas_y,p.width,p.height,p.a,p.b,p.c,p.origin_x,p.origin_y}) e.number(x);
    }
    e.number(static_cast<std::uint32_t>(result.labels.size()));
    for (const auto &l:result.labels) {
        e.text(l.label.utf8); e.text(l.label.text_sha256); e.text(l.label.source_sha256);
        e.number(l.label.flags.use_extended_font); e.number(l.label.flags.force_ime); e.number(l.label.flags.force_non_ime);
        e.number(static_cast<std::uint32_t>(l.label.code_points.size()));
        for (const auto cp:l.label.code_points) e.number(cp);
        e.number(l.advance_width); e.number(static_cast<std::uint32_t>(l.glyphs.size()));
        for (const auto &g:l.glyphs) {
            e.number(g.code_point); e.number(static_cast<std::uint32_t>(g.origin));
            for (const auto x:{g.rectangle.x,g.rectangle.y,g.rectangle.width,g.rectangle.height}) e.number(x);
        }
    }
    return e.digest();
}
bool same_geometry(const civ5::ui::GlyphGenGlyphGeometry &a,
                   const civ5::ui::GlyphGenGlyphGeometry &b) {
    return a.code_point==b.code_point && a.atlas_x==b.atlas_x && a.atlas_y==b.atlas_y &&
        a.width==b.width && a.height==b.height && a.a==b.a && a.b==b.b && a.c==b.c &&
        a.origin_x==b.origin_x && a.origin_y==b.origin_y;
}
ExtendedPreparedLabel project_label(const DecodedPlainLabel &label,
    const std::map<std::uint32_t,civ5::ui::GlyphGenGlyphGeometry> &glyphs) {
    ExtendedPreparedLabel result; result.label=label;
    std::int64_t pen=0;
    for (const auto cp:label.code_points) {
        const auto &g=glyphs.at(cp);
        const auto x=pen+g.origin_x;
        const auto next=pen+g.advance();
        if (x<std::numeric_limits<std::int32_t>::min() || x>std::numeric_limits<std::int32_t>::max() ||
            x+g.width>std::numeric_limits<std::int32_t>::max() || next<0 ||
            next>std::numeric_limits<std::int32_t>::max())
            fail(Failure::advance_overflow,"Prepared entire-label placement exceeds int32",cp);
        result.glyphs.push_back({cp,{static_cast<std::int32_t>(x),-g.origin_y,g.width,g.height},
            civ5::ui::NativeExtendedGlyphOrigin::generated_base});
        pen=next;
    }
    result.advance_width=static_cast<std::int32_t>(pen);
    return result;
}
} // namespace

ExtendedLabelGdiError::ExtendedLabelGdiError(Failure failure_value,const char *message,
    std::optional<std::uint32_t> cp):std::runtime_error(message),failure(failure_value),code_point(cp) {}

DecodedPlainLabel decode_extended_plain_label(std::string_view utf8,std::string source_sha256,
    civ5::ui::NativeExtendedFontFlags flags) {
    if (!digest_syntax(source_sha256)) fail(Failure::invalid_source_identity,"Label source SHA-256 is missing/invalid");
    if (utf8.empty() || utf8.size()>static_cast<std::size_t>(std::numeric_limits<int>::max()))
        fail(Failure::unsupported_text,"Plain label is empty or exceeds Win32 length");
    const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8.data(),static_cast<int>(utf8.size()),nullptr,0);
    if (count<=0) fail(Failure::unsupported_text,"Plain label is not valid UTF-8");
    std::wstring wide(static_cast<std::size_t>(count),L'\0');
    if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8.data(),static_cast<int>(utf8.size()),wide.data(),count)!=count)
        fail(Failure::unsupported_text,"Plain label decoding changed length");
    DecodedPlainLabel result{std::string(utf8),bytes_digest(utf8),std::move(source_sha256),flags,{}};
    for (const auto c:wide) {
        const auto cp=static_cast<std::uint32_t>(c);
        if (cp<32 || cp==127 || (cp>=0xd800 && cp<=0xdfff))
            fail(Failure::unsupported_text,"Plain label requires control/non-BMP/shaping path",cp);
        result.code_points.push_back(cp);
    }
    return result;
}

PreparedExtendedLabelBase prepare_extended_label_base(
    const SelectedExtendedFontRequest &request,std::span<const DecodedPlainLabel> labels) {
    if (request.font.empty() || request.style.empty() || request.language_font_name.empty() ||
        !std::isfinite(request.language_width_scale) || !std::isfinite(request.language_height_scale) ||
        request.language_width_scale<=0 || request.language_height_scale<=0 || labels.empty() ||
        request.gdi.face.empty() || request.gdi.face.size()>=LF_FACESIZE ||
        !std::all_of(request.gdi.face.begin(),request.gdi.face.end(),[](unsigned char c){return c>=32 && c<127;}) ||
        request.gdi.height<=0 || request.gdi.height>48 || request.gdi.width<0 || request.gdi.width>48 || request.gdi.atlas_width==0 ||
        request.gdi.atlas_height==0 || request.gdi.atlas_width>4096 || request.gdi.atlas_height>4096)
        fail(Failure::invalid_request,"Selected explicit compatibility font request is outside supported limits");
    for (const auto *digest:{&request.manifest_sha256,&request.locale_source_sha256,&request.language_settings_source_sha256})
        if (!digest_syntax(*digest)) fail(Failure::invalid_source_identity,"Selected font/locale/settings source SHA-256 is missing/invalid");
    std::set<std::uint32_t> requested;
    for (const auto &label:labels) {
        const auto decoded=decode_extended_plain_label(label.utf8,label.source_sha256,label.flags);
        if (decoded.code_points!=label.code_points || decoded.text_sha256!=label.text_sha256)
            fail(Failure::unsupported_text,"Retained decoded label disagrees with exact text payload");
        if (civ5::ui::native_extended_font_route(label.flags)!=civ5::ui::NativeExtendedFontRoute::extended_label)
            fail(Failure::authored_route,"Whole label is not selected for the extended font route");
        requested.insert(label.code_points.begin(),label.code_points.end());
    }
    std::unique_ptr<ExtendedLabelGdiFont> holder;
    try { holder=std::make_unique<ExtendedLabelGdiFont>(request.gdi.face,request.gdi.height,request.gdi.width); }
    catch (const std::runtime_error &) { fail(Failure::font_creation,"Existing compatibility GDI object creation failed"); }
    auto &gdi=*holder;
    PreparedExtendedLabelBase result;
    auto &receipt=result.receipt;
    receipt.request=request;
    LOGFONTW wanted{};
    wanted.lfHeight=request.gdi.height; wanted.lfWidth=request.gdi.width;
    wanted.lfCharSet=DEFAULT_CHARSET; wanted.lfQuality=PROOF_QUALITY;
    std::copy(request.gdi.face.begin(),request.gdi.face.end(),wanted.lfFaceName);
    receipt.requested_logfont=logfont_receipt(wanted);
    LOGFONTW applied{}; TEXTMETRICW metrics{};
    const auto face_length=GetTextFaceW(gdi.dc,0,nullptr);
    if (GetObjectW(gdi.font,sizeof(applied),&applied)!=sizeof(applied) ||
        !GetTextMetricsW(gdi.dc,&metrics) || face_length<=0)
        fail(Failure::settings_changed,"Could not observe applied GDI font settings/metrics");
    std::wstring resolved(static_cast<std::size_t>(face_length),L'\0');
    if (GetTextFaceW(gdi.dc,face_length,resolved.data())<=0)
        fail(Failure::settings_changed,"Could not observe resolved GDI face");
    receipt.applied_logfont=logfont_receipt(applied);
    if (receipt.applied_logfont!=receipt.requested_logfont)
        fail(Failure::settings_changed,"Applied LOGFONT differs from the explicit width/height/declaration request");
    receipt.resolved_face=ascii_face(resolved.c_str());
    if (!same_face(request.gdi.face,receipt.resolved_face))
        fail(Failure::substituted_face,"GDI substituted a different font face");
    receipt.metrics={metrics.tmHeight,metrics.tmAscent,metrics.tmDescent,metrics.tmInternalLeading,
        metrics.tmExternalLeading,metrics.tmAveCharWidth,metrics.tmMaxCharWidth,metrics.tmWeight,
        metrics.tmOverhang,metrics.tmDigitizedAspectX,metrics.tmDigitizedAspectY,
        metrics.tmFirstChar,metrics.tmLastChar,metrics.tmDefaultChar,metrics.tmBreakChar,
        metrics.tmItalic,metrics.tmUnderlined,metrics.tmStruckOut,metrics.tmPitchAndFamily,metrics.tmCharSet};
    receipt.text_color=GetTextColor(gdi.dc); receipt.text_align=GetTextAlign(gdi.dc);
    receipt.background_mode=GetBkMode(gdi.dc); receipt.map_mode=GetMapMode(gdi.dc);
    receipt.graphics_mode=GetGraphicsMode(gdi.dc);
    receipt.dpi_x=GetDeviceCaps(gdi.dc,LOGPIXELSX); receipt.dpi_y=GetDeviceCaps(gdi.dc,LOGPIXELSY);
    receipt.ext_text_out_options=ETO_GLYPH_INDEX;
    if (receipt.text_color!=RGB(255,255,255) || receipt.text_align!=(TA_LEFT|TA_BASELINE) ||
        receipt.background_mode!=TRANSPARENT || receipt.map_mode!=MM_TEXT ||
        receipt.graphics_mode!=GM_COMPATIBLE || receipt.dpi_x<=0 || receipt.dpi_y<=0 || metrics.tmHeight<=0)
        fail(Failure::settings_changed,"Applied GDI raster state disagrees with supported compatibility policy");
    receipt.applied_settings_sha256=settings_digest(receipt);
    result.coverage.width=request.gdi.atlas_width; result.coverage.height=request.gdi.atlas_height;
    result.coverage.alpha.resize(static_cast<std::size_t>(result.coverage.width)*result.coverage.height,0);
    result.geometry.atlas_width=result.coverage.width; result.geometry.atlas_height=result.coverage.height;
    result.geometry.line_height=metrics.tmHeight; result.geometry.ascent=metrics.tmAscent; result.geometry.descent=metrics.tmDescent;
    std::int32_t atlas_x=0,atlas_y=0,row_height=0;
    std::map<std::uint32_t,civ5::ui::GlyphGenGlyphGeometry> geometry;
    MAT2 transform{}; transform.eM11.value=1; transform.eM22.value=1;
    for (const auto cp:requested) {
        const auto character=static_cast<wchar_t>(cp);
        WORD glyph=0xffff;
        if (GetGlyphIndicesW(gdi.dc,&character,1,&glyph,GGI_MARK_NONEXISTING_GLYPHS)==GDI_ERROR || glyph==0xffff)
            fail(Failure::missing_glyph,"Selected GDI face has no requested nominal BMP glyph",cp);
        GLYPHMETRICS gm{};
        if (GetGlyphOutlineW(gdi.dc,glyph,GGO_METRICS|GGO_GLYPH_INDEX,&gm,0,nullptr,&transform)==GDI_ERROR)
            fail(Failure::glyph_metrics,"Could not inspect GDI glyph metrics before rasterization",cp);
        const auto left_metric=std::int64_t{receipt.draw_origin_x}+gm.gmptGlyphOrigin.x;
        const auto top_metric=std::int64_t{receipt.draw_baseline_y}-gm.gmptGlyphOrigin.y;
        if (gm.gmBlackBoxX && gm.gmBlackBoxY &&
            (left_metric<=0 || top_metric<=0 || left_metric+gm.gmBlackBoxX>=ExtendedLabelGdiFont::side ||
             top_metric+gm.gmBlackBoxY>=ExtendedLabelGdiFont::side))
            fail(Failure::raster_bounds,"Nominal glyph would reach/clamp the fixed compatibility bitmap border",cp);
        SIZE extent{};
        if (!GetTextExtentPoint32W(gdi.dc,&character,1,&extent) || extent.cx<0)
            fail(Failure::glyph_metrics,"Could not observe nonnegative nominal glyph advance",cp);
        std::fill_n(static_cast<std::uint32_t *>(gdi.pixels),ExtendedLabelGdiFont::side*ExtendedLabelGdiFont::side,0u);
        const auto glyph_character=static_cast<wchar_t>(glyph);
        if (!ExtTextOutW(gdi.dc,receipt.draw_origin_x,receipt.draw_baseline_y,receipt.ext_text_out_options,
                         nullptr,&glyph_character,1,nullptr) || !GdiFlush())
            fail(Failure::rasterization,"Actual glyph-index draw/flush failed",cp);
        const auto *pixels=static_cast<const std::uint8_t *>(gdi.pixels);
        int left=ExtendedLabelGdiFont::side,top=ExtendedLabelGdiFont::side,right=0,bottom=0;
        for (int y=0;y<ExtendedLabelGdiFont::side;++y) for (int x=0;x<ExtendedLabelGdiFont::side;++x)
            if (pixels[(y*ExtendedLabelGdiFont::side+x)*4+1]) {
                left=std::min(left,x);top=std::min(top,y);right=std::max(right,x+1);bottom=std::max(bottom,y+1);
            }
        const bool zero_ink=right==0;
        if (!zero_ink && (left==0 || top==0 || right==ExtendedLabelGdiFont::side || bottom==ExtendedLabelGdiFont::side))
            fail(Failure::raster_bounds,"Actual glyph ink touches the fixed compatibility bitmap border",cp);
        const auto width=zero_ink?0:right-left,height=zero_ink?0:bottom-top;
        if (static_cast<std::uint32_t>(width)>result.coverage.width || static_cast<std::uint32_t>(height)>result.coverage.height)
            fail(Failure::atlas_overflow,"Generated glyph exceeds runtime plane dimensions",cp);
        if (width && atlas_x+width>static_cast<std::int32_t>(result.coverage.width)) {
            atlas_x=0;atlas_y+=row_height+1;row_height=0;
        }
        if (height && atlas_y+height>static_cast<std::int32_t>(result.coverage.height))
            fail(Failure::atlas_overflow,"Complete generated label coverage exceeds runtime plane",cp);
        civ5::ui::GlyphGenGlyphGeometry gg{cp,zero_ink?0:atlas_x,zero_ink?0:atlas_y,width,height,
            0,extent.cx,0,zero_ink?0:left-receipt.draw_origin_x,zero_ink?0:receipt.draw_baseline_y-top};
        for (int y=0;y<height;++y) for (int x=0;x<width;++x)
            result.coverage.alpha[static_cast<std::size_t>(atlas_y+y)*result.coverage.width+atlas_x+x]=
                pixels[((top+y)*ExtendedLabelGdiFont::side+left+x)*4+1];
        if (!zero_ink) { atlas_x+=width+1;row_height=std::max(row_height,height); }
        result.geometry.glyphs.push_back(gg); geometry.emplace(cp,gg);
        receipt.generated_glyphs.push_back({cp,glyph,extent.cx,extent.cy,gm.gmBlackBoxX,gm.gmBlackBoxY,
            gm.gmptGlyphOrigin.x,gm.gmptGlyphOrigin.y,gm.gmCellIncX,gm.gmCellIncY,zero_ink,gg});
    }
    for (const auto &label:labels) result.labels.push_back(project_label(label,geometry));
    receipt.coverage_sha256=civ5::runtime::identify_bytes(result.coverage.alpha).sha256;
    receipt.geometry_sha256=geometry_digest(result.geometry);
    receipt.observation_sha256=observation_digest(result);
    return result;
}

bool extended_label_base_payload_matches(const PreparedExtendedLabelBase &result) {
    try {
        const auto &r=result.receipt;
        if (result.coverage.width!=result.geometry.atlas_width || result.coverage.height!=result.geometry.atlas_height ||
            result.coverage.width!=r.request.gdi.atlas_width || result.coverage.height!=r.request.gdi.atlas_height ||
            result.coverage.alpha.size()!=std::uint64_t{result.coverage.width}*result.coverage.height ||
            civ5::runtime::identify_bytes(result.coverage.alpha).sha256!=r.coverage_sha256 ||
            geometry_digest(result.geometry)!=r.geometry_sha256 || settings_digest(r)!=r.applied_settings_sha256 ||
            observation_digest(result)!=r.observation_sha256 ||
            r.generated_glyphs.size()!=result.geometry.glyphs.size() || result.labels.empty()) return false;
        std::map<std::uint32_t,civ5::ui::GlyphGenGlyphGeometry> geometry;
        std::set<std::uint32_t> requested;
        for (std::size_t i=0;i<result.geometry.glyphs.size();++i) {
            const auto &g=result.geometry.glyphs[i]; const auto &observed=r.generated_glyphs[i];
            if (!same_geometry(g,observed.geometry) || g.code_point!=observed.code_point ||
                observed.native_glyph_id==0xffff || !geometry.emplace(g.code_point,g).second ||
                observed.zero_ink!=(g.width==0 && g.height==0) || g.b!=observed.extent_width) return false;
        }
        for (const auto &label:result.labels) {
            const auto decoded=decode_extended_plain_label(label.label.utf8,label.label.source_sha256,label.label.flags);
            if (decoded.code_points!=label.label.code_points || decoded.text_sha256!=label.label.text_sha256 ||
                civ5::ui::native_extended_font_route(label.label.flags)!=civ5::ui::NativeExtendedFontRoute::extended_label) return false;
            const auto expected=project_label(label.label,geometry);
            if (expected.advance_width!=label.advance_width || expected.glyphs.size()!=label.glyphs.size()) return false;
            for (std::size_t i=0;i<label.glyphs.size();++i) {
                const auto &a=expected.glyphs[i],&b=label.glyphs[i];
                if (a.code_point!=b.code_point || a.rectangle!=b.rectangle || a.origin!=b.origin) return false;
            }
            requested.insert(label.label.code_points.begin(),label.label.code_points.end());
        }
        if (requested.size()!=geometry.size()) return false;
        for (const auto cp:requested) if (!geometry.contains(cp)) return false;
        return true;
    } catch (const std::exception &) { return false; }
}

std::string extended_label_base_receipt_json(const PreparedExtendedLabelBase &result) {
    if (!extended_label_base_payload_matches(result))
        fail(Failure::settings_changed,"Retained extended-label preparation no longer matches its observation");
    const auto quote=[](std::string_view s) {
        constexpr char hex[]="0123456789abcdef"; std::string out="\"";
        for (const auto c:s) {
            const auto b=static_cast<unsigned char>(c);
            if (b=='\"' || b=='\\') {out+='\\';out+=c;}
            else if (b<32) {out+="\\u00";out+=hex[b>>4];out+=hex[b&15];}
            else out+=c;
        }
        return out+'\"';
    };
    const auto font_json=[&](const ExtendedLabelLogFont &f) {
        std::ostringstream o; o.imbue(std::locale::classic());
        o<<"{\"face\":"<<quote(f.face)<<",\"height\":"<<f.height<<",\"width\":"<<f.width
         <<",\"escapement\":"<<f.escapement<<",\"orientation\":"<<f.orientation<<",\"weight\":"<<f.weight
         <<",\"italic\":"<<unsigned(f.italic)<<",\"underline\":"<<unsigned(f.underline)<<",\"strikeout\":"<<unsigned(f.strikeout)
         <<",\"charset\":"<<unsigned(f.charset)<<",\"outPrecision\":"<<unsigned(f.out_precision)
         <<",\"clipPrecision\":"<<unsigned(f.clip_precision)<<",\"quality\":"<<unsigned(f.quality)
         <<",\"pitchAndFamily\":"<<unsigned(f.pitch_and_family)<<'}';
        return o.str();
    };
    const auto &r=result.receipt; const auto &q=r.request; const auto &m=r.metrics;
    std::ostringstream o; o.imbue(std::locale::classic());
    o<<std::setprecision(std::numeric_limits<float>::max_digits10);
    o<<"{\"schema\":"<<quote(r.schema)<<",\"mode\":"<<quote(r.mode)
     <<",\"sourceDerivedLogfontProven\":false,\"nativeMetricsOrPixelsProven\":false"
     <<",\"declaredSources\":{\"font\":"<<quote(q.font)<<",\"style\":"<<quote(q.style)
     <<",\"manifestSha256\":"<<quote(q.manifest_sha256)<<",\"localeSourceSha256\":"<<quote(q.locale_source_sha256)
     <<",\"languageSettingsSourceSha256\":"<<quote(q.language_settings_source_sha256)
     <<",\"languageFontName\":"<<quote(q.language_font_name)<<",\"languageWidthScale\":"<<q.language_width_scale
     <<",\"languageHeightScale\":"<<q.language_height_scale<<'}'
     <<",\"requestedLogfont\":"<<font_json(r.requested_logfont)<<",\"appliedLogfont\":"<<font_json(r.applied_logfont)
     <<",\"resolvedFace\":"<<quote(r.resolved_face)
     <<",\"metrics\":{\"height\":"<<m.height<<",\"ascent\":"<<m.ascent<<",\"descent\":"<<m.descent
     <<",\"internalLeading\":"<<m.internal_leading<<",\"externalLeading\":"<<m.external_leading
     <<",\"averageWidth\":"<<m.average_width<<",\"maximumWidth\":"<<m.maximum_width
     <<",\"weight\":"<<m.weight<<",\"overhang\":"<<m.overhang
     <<",\"digitizedAspectX\":"<<m.digitized_aspect_x<<",\"digitizedAspectY\":"<<m.digitized_aspect_y
     <<",\"firstChar\":"<<m.first_char<<",\"lastChar\":"<<m.last_char<<",\"defaultChar\":"<<m.default_char
     <<",\"breakChar\":"<<m.break_char<<",\"italic\":"<<unsigned(m.italic)<<",\"underline\":"<<unsigned(m.underline)
     <<",\"strikeout\":"<<unsigned(m.strikeout)<<",\"pitchAndFamily\":"<<unsigned(m.pitch_and_family)
     <<",\"charset\":"<<unsigned(m.charset)<<'}'
     <<",\"rasterPolicy\":{\"glyphApi\":\"GetGlyphIndicesW\",\"metricsApi\":\"GetGlyphOutlineW(GGO_METRICS|GGO_GLYPH_INDEX)\""
     <<",\"advanceApi\":\"GetTextExtentPoint32W(single BMP character)\",\"drawApi\":\"ExtTextOutW\",\"drawOptions\":"<<r.ext_text_out_options
     <<",\"channel\":\"green\",\"textColor\":"<<r.text_color<<",\"textAlign\":"<<r.text_align
     <<",\"backgroundMode\":"<<r.background_mode<<",\"mapMode\":"<<r.map_mode<<",\"graphicsMode\":"<<r.graphics_mode
     <<",\"dpiX\":"<<r.dpi_x<<",\"dpiY\":"<<r.dpi_y<<",\"bitmapSide\":"<<r.bitmap_side
     <<",\"drawOriginX\":"<<r.draw_origin_x<<",\"drawBaselineY\":"<<r.draw_baseline_y
     <<",\"shapingOrKerningProven\":false}"
     <<",\"appliedSettingsSha256\":"<<quote(r.applied_settings_sha256)<<",\"observationSha256\":"<<quote(r.observation_sha256)
     <<",\"coverage\":{\"format\":"<<quote(r.pixel_format)<<",\"width\":"<<result.coverage.width
     <<",\"height\":"<<result.coverage.height<<",\"rowStride\":"<<result.coverage.width
     <<",\"sha256\":"<<quote(r.coverage_sha256)<<"},\"geometrySha256\":"<<quote(r.geometry_sha256)<<",\"glyphs\":[";
    bool first=true;
    for (const auto &g:r.generated_glyphs) {
        if (!first) o<<',';first=false;
        const auto &p=g.geometry;
        o<<"{\"codePoint\":"<<g.code_point<<",\"nativeGlyphId\":"<<g.native_glyph_id
         <<",\"extentWidth\":"<<g.extent_width<<",\"extentHeight\":"<<g.extent_height
         <<",\"metricBlackBox\":["<<g.metric_black_box_width<<','<<g.metric_black_box_height<<']'
         <<",\"metricOrigin\":["<<g.metric_origin_x<<','<<g.metric_origin_y<<']'
         <<",\"metricCellIncrement\":["<<g.metric_cell_increment_x<<','<<g.metric_cell_increment_y<<']'
         <<",\"zeroInk\":"<<(g.zero_ink?"true":"false")<<",\"atlasRect\":["<<p.atlas_x<<','<<p.atlas_y<<','<<p.width<<','<<p.height<<']'
         <<",\"abc\":["<<p.a<<','<<p.b<<','<<p.c<<"],\"origin\":["<<p.origin_x<<','<<p.origin_y<<"]}";
    }
    o<<"],\"labels\":["; first=true;
    for (const auto &l:result.labels) {
        if (!first) o<<',';first=false;
        o<<"{\"utf8\":"<<quote(l.label.utf8)<<",\"textSha256\":"<<quote(l.label.text_sha256)
         <<",\"declaredSourceSha256\":"<<quote(l.label.source_sha256)
         <<",\"useExtendedFont\":"<<(l.label.flags.use_extended_font?"true":"false")
         <<",\"forceIME\":"<<(l.label.flags.force_ime?"true":"false")
         <<",\"forceNonIME\":"<<(l.label.flags.force_non_ime?"true":"false")
         <<",\"route\":\"entire-extended-label\",\"advanceWidth\":"<<l.advance_width<<",\"glyphs\":[";
        bool first_glyph=true;
        for (const auto &g:l.glyphs) {
            if (!first_glyph) o<<',';first_glyph=false;
            o<<"{\"codePoint\":"<<g.code_point<<",\"origin\":\"generated-base\",\"rectangle\":["
             <<g.rectangle.x<<','<<g.rectangle.y<<','<<g.rectangle.width<<','<<g.rectangle.height<<"]}";
        }
        o<<"]}";
    }
    o<<"]}";return o.str();
}
} // namespace civ5::validation
