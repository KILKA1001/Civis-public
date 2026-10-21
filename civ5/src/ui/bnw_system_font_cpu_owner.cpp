#include "civ5/ui/bnw_system_font_cpu_owner.hpp"
#include "civ5/graphics/detail/bnw_owned_font_paint_effect_fence.hpp"
#include "civ5/ui/detail/bnw_label_system_font_source.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include <cmath>
#include <cstring>
#include <map>
#include <array>
#include <algorithm>
#include <type_traits>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::ui {
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
std::wstring wide(std::string_view s){need(!s.empty()&&s.find('\0')==s.npos&&s.size()<32768,"Invalid selected native font UTF8 name");const auto n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);need(n>0&&n<LF_FACESIZE,"Native LOGFONT face unavailable/outside bounded profile");std::wstring v(n,L'\0');need(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),v.data(),n)==n,"Native font UTF8 conversion failed");return v;}
std::string utf8(std::wstring_view s){const auto n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0,nullptr,nullptr);need(n>0,"Native font actual face conversion failed");std::string v(n,'\0');need(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),v.data(),n,nullptr,nullptr)==n,"Native font face conversion failed");return v;}
}
struct BnwSystemFontCpuOwner::Impl {
 HWND hwnd=nullptr;HDC dc=nullptr;HFONT font=nullptr;HBITMAP bitmap=nullptr;HGDIOBJ old_font=nullptr,old_bitmap=nullptr;void*pixels=nullptr;
 std::shared_ptr<const detail::BnwLabelSystemFontSource>label_source;
 std::shared_ptr<BnwClientTextPolicyOwner>policy;BnwSystemFontCpuFrame frame;std::thread::id thread=std::this_thread::get_id();
 std::map<std::uint16_t,BnwSystemFontGlyphCpu>raster;
 bool operation_active=false;
 mutable bool query_active=false;
 std::weak_ptr<BnwSystemFontCpuOwner> facade;
 struct Query {
  const Impl& owner;
  explicit Query(const Impl& i):owner(i){
   need(std::this_thread::get_id()==i.thread,"Native font source query used on foreign thread");
   need(!i.query_active,"Native font source query callback reentry refused");i.query_active=true;
  }
  ~Query(){owner.query_active=false;}
 };
 struct Operation {
  Impl& owner;
  explicit Operation(Impl& i):owner(i){
   need(std::this_thread::get_id()==i.thread,"Native font operation used on foreign thread");
   need(!i.operation_active&&!i.query_active,"Native font callback reentry refused");i.operation_active=true;
  }
  ~Operation(){owner.operation_active=false;}
 };
 BnwSystemFontGlyphMeasurement measure_codepoint(std::uint32_t);
 BnwSystemFontGlyphMeasurement measure_index(std::uint16_t);
 BnwSystemFontGlyphCpu raster_index(BnwSystemFontGlyphMeasurement);
 void first_owned_paint(const graphics::detail::BnwOwnedFontPaintEffectFence&)const;
 void validate_owned_paint(const graphics::detail::BnwOwnedFontPaintEffectFence&)const;
 BnwSystemFontGlyphMeasurement measure_index_owned_paint(std::uint16_t,const graphics::detail::BnwOwnedFontPaintEffectFence&);
 BnwSystemFontGlyphCpu raster_index_owned_paint(BnwSystemFontGlyphMeasurement,const graphics::detail::BnwOwnedFontPaintEffectFence&);
 ~Impl(){if(dc){if(old_bitmap)SelectObject(dc,old_bitmap);if(old_font)SelectObject(dc,old_font);}if(bitmap)DeleteObject(bitmap);if(font)DeleteObject(font);if(dc)DeleteDC(dc);}
 void validate()const{
  Query query(*this);
  if(label_source){
   const auto source=label_source;
   source->require_current_without_callbacks();
   need(!policy,"Actual GUI native font source cannot coexist with legacy policy");
   need(source->hwnd_without_callbacks()==reinterpret_cast<std::uintptr_t>(hwnd),"Actual GUI native font HWND changed");
   source->require_current_owned();source->require_current_without_callbacks();
   DWORD process=0;const auto window_thread=GetWindowThreadProcessId(hwnd,&process);
   need(IsWindow(hwnd)&&process==GetCurrentProcessId()&&window_thread==GetCurrentThreadId(),"Native font requires SAME current-process HWND thread");
   source->require_current_without_callbacks();
   RECT rect{};need(GetClientRect(hwnd,&rect),"Native font client rectangle unavailable");
   source->require_current_without_callbacks();
   need(rect.right-rect.left==frame.client.width&&rect.bottom-rect.top==frame.client.height,"Stale actual GUI native font client extent");
   source->require_current_owned();source->require_current_without_callbacks();
   return;
  }
  need(std::this_thread::get_id()==thread,"Native font owner used on foreign thread");DWORD process=0;const auto window_thread=GetWindowThreadProcessId(hwnd,&process);need(IsWindow(hwnd)&&process==GetCurrentProcessId()&&window_thread==GetCurrentThreadId(),"Native font requires SAME current-process HWND thread");RECT rect{};need(GetClientRect(hwnd,&rect),"Native font client rectangle unavailable");const auto&now=policy->current();need(now.client.surface_owner==frame.client.surface_owner&&now.client.revision==frame.client.revision&&now.client.width==frame.client.width&&now.client.height==frame.client.height&&rect.right-rect.left==frame.client.width&&rect.bottom-rect.top==frame.client.height&&now.locale==frame.locale&&now.source_row==frame.language_row,"Stale native font client/locale epoch");
 }
};
BnwSystemFontCpuOwner::BnwSystemFontCpuOwner(std::shared_ptr<Impl>i):impl_(std::move(i)){}
BnwSystemFontCpuOwner::~BnwSystemFontCpuOwner()=default;
std::shared_ptr<BnwSystemFontCpuOwner>BnwSystemFontCpuOwner::create(std::uintptr_t hwnd,
 std::shared_ptr<BnwClientTextPolicyOwner>policy,const assets::ResourceMountTable&resources,const BnwNativePlainLabelMetric&borrowed_metric){
 const auto metric=borrowed_metric;const auto*metric_identity=&borrowed_metric;
 need(hwnd&&static_cast<bool>(policy),"Actual native font window/policy missing");const auto p=policy->current();need(&policy->declared_metric(metric.control_index)==metric_identity,"Native font descriptor must be SAME sealed metric owner");need(policy->backend_for_label(metric.control_index)==BnwClientTextBackend::native_system_font,"Selected Label does not use native system font");
 return create_from_source_fields(hwnd,std::move(policy),resources,p,metric.font_key,metric.font_sha256,metric.height);
}
std::shared_ptr<BnwSystemFontCpuOwner>BnwSystemFontCpuOwner::create_from_worldview_title_descriptor(
 std::shared_ptr<BnwClientTextPolicyOwner>policy,const BnwWorldViewTitleFontDescriptor&borrowed){
 // Pin fields/address before callbacks. A copied or foreign descriptor does
 // not pass the private exact-address seal, even with identical values.
 const auto d=borrowed;const auto*identity=&borrowed;
 need(static_cast<bool>(policy),"Actual WorldView font policy missing");
 const auto p=policy->current();need(&policy->declared_worldview_descriptor(d.candidate_index())==identity,"WorldView native font descriptor must be SAME sealed issuer");
 need(p.backend==BnwClientTextBackend::native_system_font,"Effective WorldView Languages row does not use extended font");
 const auto hwnd=policy->worldview_hwnd();const auto&resources=policy->worldview_resources();
 auto result=create_from_source_fields(hwnd,policy,resources,p,d.font_key(),d.font_sha256(),d.adjusted_ascent());
 need(&policy->declared_worldview_descriptor(d.candidate_index())==identity,"WorldView descriptor retired during native CPU preparation");return result;
}
std::shared_ptr<BnwSystemFontCpuOwner>BnwSystemFontCpuOwner::create_from_source_fields(std::uintptr_t hwnd,
 std::shared_ptr<BnwClientTextPolicyOwner>policy,const assets::ResourceMountTable&resources,
 BnwClientTextPolicyFrame p,std::string font_key,std::string font_sha256,std::int32_t adjusted_ascent){
 need(hwnd&&static_cast<bool>(policy),"Actual native font window/policy missing");
 auto i=std::make_shared<Impl>();i->hwnd=reinterpret_cast<HWND>(hwnd);i->policy=std::move(policy);auto&f=i->frame;f.client=p.client;f.locale=p.locale;f.language_row=p.source_row;f.font_key=font_key;f.font_sha256=font_sha256;f.source_label_ascent=adjusted_ascent;i->validate();
 const auto selected=resources.resolve(assets::AssetKey(font_key));const auto*h=std::get_if<assets::AssetHandle>(&selected.asset);need(h,"Selected native font GlyphGen source descriptor missing/ambiguous");const auto bytes=h->read_bytes(4ULL*1024*1024);need(runtime::identify_bytes(bytes).sha256==font_sha256,"Native font source descriptor changed");const auto g=glyphgen_geometry(GlyphGenFont::from_xml(content::read_xml_metadata(std::string(bytes.begin(),bytes.end()))));
 need(g.ascent-g.ascent_adjustment==adjusted_ascent&&g.ascent>0&&g.descent>=0&&g.ascent+g.descent<32768,"Native base font line height/ascent unknown");f.source_line_height=g.ascent+g.descent;
 return create_from_prepared_fields(std::move(i));
}
std::shared_ptr<BnwSystemFontCpuOwner>BnwSystemFontCpuOwner::create_from_actual_gui_source(
 std::shared_ptr<const detail::BnwLabelSystemFontSource>source){
 const auto pinned=std::move(source);
 need(static_cast<bool>(pinned),"Actual GUI native font source missing");
 pinned->require_current_without_callbacks();
 const auto hwnd=pinned->hwnd_without_callbacks();
 pinned->require_current_owned();pinned->require_current_without_callbacks();
 auto prepared=pinned->captured_cpu_frame_owned();
 pinned->require_current_without_callbacks();pinned->require_current_owned();pinned->require_current_without_callbacks();
 need(hwnd&&prepared.client.width>0&&prepared.client.height>0,"Actual GUI native font client missing");
 need(prepared.source_line_height>0&&prepared.source_line_height<32768,"Actual GUI native font line height unsupported");
 auto i=std::make_shared<Impl>();
 pinned->require_current_without_callbacks();pinned->require_current_owned();pinned->require_current_without_callbacks();
 i->label_source=pinned;i->hwnd=reinterpret_cast<HWND>(hwnd);i->frame=std::move(prepared);
 i->validate();
 return create_from_prepared_fields(std::move(i));
}
std::shared_ptr<BnwSystemFontCpuOwner>BnwSystemFontCpuOwner::create_from_prepared_fields(std::shared_ptr<Impl>i){
 auto&f=i->frame;
 // 0x50E593 fild ->float32; multiplications stored float32, then CVTTSS2SI.
 const float base=static_cast<float>(f.source_line_height);const float width=base*f.language_row.width_scale,height=base*f.language_row.height_scale;need(std::isfinite(width)&&std::isfinite(height)&&width>0&&height>0&&width<32768&&height<32768,"Native scaled LOGFONT dimensions unsupported");f.logfont_width=static_cast<int>(width);f.logfont_height=static_cast<int>(height);need(f.logfont_width>0&&f.logfont_height>0,"Native scaled font collapses");
 const auto name=wide(f.language_row.font_name);LOGFONTW lf{};lf.lfWidth=f.logfont_width;lf.lfHeight=f.logfont_height;lf.lfCharSet=DEFAULT_CHARSET;lf.lfQuality=PROOF_QUALITY;lf.lfPitchAndFamily=0;std::memcpy(lf.lfFaceName,name.data(),name.size()*sizeof(wchar_t));f.charset=lf.lfCharSet;f.quality=lf.lfQuality;
 i->dc=CreateCompatibleDC(nullptr);need(i->dc&&SetMapMode(i->dc,MM_TEXT),"Native font memory DC/MM_TEXT unavailable");i->font=CreateFontIndirectW(&lf);need(i->font,"Native selected LOGFONT creation failed");i->old_font=SelectObject(i->dc,i->font);need(i->old_font&&i->old_font!=HGDI_ERROR,"Native font selection failed");f.language_flags=GetFontLanguageInfo(i->dc);need(f.language_flags!=GCP_ERROR,"Native font language information failed");f.gcp_flags=(f.language_flags&0x13a)|GCP_CLASSIN;need((f.language_flags&0x132)==0,"Native CLASSIN stack classes unproven for reorder/shape/ligate/diacritic font path");TEXTMETRICW tm{};need(GetTextMetricsW(i->dc,&tm),"Native TEXTMETRIC unavailable");f.tm_height=tm.tmHeight;f.tm_max_width=tm.tmMaxCharWidth;f.pitch_and_family=tm.tmPitchAndFamily;
 // Civis identity probes, additional reads rather than native constructor calls.
 wchar_t face[LF_FACESIZE]{};need(GetTextFaceW(i->dc,LF_FACESIZE,face)>0,"Actual selected native font face unavailable");f.actual_face=utf8(face);
 const auto binary_size=GetFontData(i->dc,0,0,nullptr,0);need(binary_size!=GDI_ERROR&&binary_size>0&&binary_size<64ULL*1024*1024,"Native font binary identity unavailable");assets::Bytes binary(binary_size);need(GetFontData(i->dc,0,0,binary.data(),binary_size)==binary_size,"Native font binary identity changed during read");f.font_binary_sha256=runtime::identify_bytes(binary).sha256;
 need(f.tm_max_width>0&&f.tm_height>0&&f.tm_max_width<=2048&&f.tm_height<2048,"Native atlas cell dimensions unsupported");f.cell_width=f.tm_max_width+(f.tm_max_width&1);const int hplus=f.tm_height+1;f.cell_height=hplus+(hplus&1);f.columns=2048/f.cell_width;f.rows=2048/f.cell_height;need(f.columns>0&&f.rows>0,"Native atlas has zero capacity");f.atlas_width=f.columns*f.cell_width;f.atlas_height=f.rows*f.cell_height;
 BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=f.cell_width;bi.bmiHeader.biHeight=-f.cell_height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;i->bitmap=CreateDIBSection(i->dc,&bi,DIB_RGB_COLORS,&i->pixels,nullptr,0);need(i->bitmap&&i->pixels,"Native top-down DIB creation failed");i->old_bitmap=SelectObject(i->dc,i->bitmap);need(i->old_bitmap&&i->old_bitmap!=HGDI_ERROR,"Native DIB selection failed");f.text_color=GetTextColor(i->dc);f.background_color=GetBkColor(i->dc);f.background_mode=GetBkMode(i->dc);f.text_align=GetTextAlign(i->dc);f.map_mode=GetMapMode(i->dc);f.graphics_mode=GetGraphicsMode(i->dc);f.text_extra=GetTextCharacterExtra(i->dc);need(f.text_color==RGB(0,0,0)&&f.background_color==RGB(255,255,255)&&f.background_mode==OPAQUE&&f.text_align==(TA_LEFT|TA_TOP|TA_NOUPDATECP)&&f.map_mode==MM_TEXT&&f.graphics_mode==GM_COMPATIBLE&&f.text_extra==0,"Native source DC defaults unavailable/different after DIB selection");
 // This owns real CPU measurement/raster only. 0x68C8D0 also allocates TWO
 // native D3DFMT_L8(0x32) atlas textures and a render/cache owner; not skipped.
 f.measurement_available=true;f.cpu_raster_available=true;f.pending_gpu_producer="Actual retained renderer pair of D3DFMT_L8 atlas textures/source glyph cache slots and native shader sampling (0x68C8D0/0x5084E0); GPU not initialized";i->validate();auto owner=std::shared_ptr<BnwSystemFontCpuOwner>(new BnwSystemFontCpuOwner(std::move(i)));owner->impl_->facade=owner;owner->impl_->validate();return owner;
}
const BnwSystemFontCpuFrame&BnwSystemFontCpuOwner::current()const{const auto pinned=impl_;pinned->validate();need(!pinned->facade.expired(),"Retired native font facade cannot return borrowed frame");return pinned->frame;}
std::uintptr_t BnwSystemFontCpuOwner::native_window_handle()const{const auto pinned=impl_;pinned->validate();need(!pinned->facade.expired(),"Retired native font facade cannot return current HWND identity");return reinterpret_cast<std::uintptr_t>(pinned->hwnd);}
BnwSystemFontGlyphMeasurement BnwSystemFontCpuOwner::Impl::measure_codepoint(std::uint32_t cp){
 auto&i=*this;i.validate();need(cp>0&&cp<=0xffff&&!(cp>=0xd800&&cp<=0xdfff),"Native source WCHAR glyph scope excludes NUL/surrogate/supplementary codepoint");WCHAR input[2]{static_cast<WCHAR>(cp),0},glyph[2]{};UINT order=0;int dx=0;char klass=static_cast<char>(cp&0xff);const auto class_input=static_cast<std::uint8_t>(klass);GCP_RESULTSW result{};result.lStructSize=sizeof(result);result.lpOrder=&order;result.lpDx=&dx;result.lpClass=&klass;result.lpGlyphs=glyph;result.nGlyphs=1;
 need(GetCharacterPlacementW(i.dc,input,1,0,&result,i.frame.gcp_flags)!=0&&result.nGlyphs==1&&result.lpOrder==&order&&result.lpDx==&dx&&result.lpClass==&klass&&result.lpGlyphs==glyph&&input[0]==static_cast<WCHAR>(cp)&&input[1]==0,"Native single WCHAR glyph placement failed");BnwSystemFontGlyphMeasurement out;out.codepoint=cp;out.class_input=class_input;out.class_output=static_cast<std::uint8_t>(klass);out.gcp_flags=i.frame.gcp_flags;out.gcp_structure_size=sizeof(result);out.pointers_preserved=true;out.glyph_index=static_cast<std::uint16_t>(glyph[0]);out.advance=i.measure_index(out.glyph_index).advance;i.validate();return out;
}
BnwSystemFontGlyphMeasurement BnwSystemFontCpuOwner::Impl::measure_index(std::uint16_t glyph){
 auto&i=*this;i.validate();BnwSystemFontGlyphMeasurement out;out.glyph_index=glyph;
 if((i.frame.pitch_and_family&TMPF_TRUETYPE)!=0){ABC abc{};need(GetCharABCWidthsI(i.dc,out.glyph_index,1,nullptr,&abc),"Native glyph ABCI unavailable");const auto sum=static_cast<std::int64_t>(abc.abcA)+abc.abcB;need(sum>=0&&sum<=i.frame.cell_width,"Native glyph A+B outside owned DIB");out.advance=static_cast<int>(sum);}else{int width=0;need(GetCharWidthI(i.dc,out.glyph_index,1,nullptr,&width)&&width>=0&&width<=i.frame.cell_width,"Native glyph widthI unavailable/outside DIB");out.advance=width;}i.validate();return out;
}
BnwSystemFontGlyphMeasurement BnwSystemFontCpuOwner::measure(std::uint32_t cp){
 const auto pinned=impl_;Impl::Operation operation(*pinned);return pinned->measure_codepoint(cp);
}
BnwSystemFontGlyphMeasurement BnwSystemFontCpuOwner::measure_glyph_index(std::uint16_t glyph){
 const auto pinned=impl_;Impl::Operation operation(*pinned);return pinned->measure_index(glyph);
}
void BnwSystemFontCpuOwner::require_native_origin_current_without_callbacks_owned(
 const std::shared_ptr<Impl>&pinned,const std::weak_ptr<Impl>&origin){
 need(std::this_thread::get_id()==pinned->thread,"Native GCP quote used on foreign thread");
 need(pinned->label_source&&!pinned->policy,"Native GCP requires genuine actual GUI source");
 pinned->label_source->require_current_without_callbacks();
 {const auto issued=origin.lock();need(issued&&issued==pinned,"Foreign/retired native GCP CPU/DC quote");}
 need(!pinned->facade.expired(),"Retired native GCP CPU facade");
 pinned->label_source->require_current_without_callbacks();
}
void BnwSystemFontCpuOwner::require_native_glyph_index_current_without_callbacks(const NativeGlyphIndex&quote)const{
 const auto pinned=impl_;require_native_origin_current_without_callbacks_owned(pinned,quote.origin_);
}
void BnwSystemFontCpuOwner::require_native_gcp_current_without_callbacks(const NativeGcpResult&quote)const{
 const auto pinned=impl_;require_native_origin_current_without_callbacks_owned(pinned,quote.origin_);
}
BnwSystemFontCpuOwner::NativeGlyphIndex BnwSystemFontCpuOwner::map_wchar_to_glyph_index_owned(std::uint16_t cp){
 const auto pinned=impl_;Impl::Operation operation(*pinned);
 require_native_origin_current_without_callbacks_owned(pinned,pinned);pinned->validate();
 require_native_origin_current_without_callbacks_owned(pinned,pinned);
 std::uint16_t glyph_index=0;std::uint32_t raw=0;std::uint8_t class_output=0;
 const auto flags=(pinned->frame.language_flags&0x13a)|GCP_CLASSIN;
 {
  // Native550330 consumes only the low WCHAR and low byte CLASSIN; no width query.
  WCHAR input[2]{static_cast<WCHAR>(cp),0},glyph[2]{};
  UINT order=0;int dx=0;char klass=static_cast<char>(cp&0xff);
  GCP_RESULTSW result{};result.lStructSize=sizeof(result);result.lpOrder=&order;result.lpDx=&dx;
  result.lpClass=&klass;result.lpGlyphs=glyph;result.nGlyphs=1;
  raw=GetCharacterPlacementW(pinned->dc,input,1,0,&result,flags);
  require_native_origin_current_without_callbacks_owned(pinned,pinned);pinned->validate();
  require_native_origin_current_without_callbacks_owned(pinned,pinned);
  need(raw!=0&&result.nGlyphs==1&&result.lpOutString==nullptr&&result.lpCaretPos==nullptr&&
      result.lpOrder==&order&&result.lpDx==&dx&&result.lpClass==&klass&&result.lpGlyphs==glyph,
      "Native single WCHAR GCP failed/changed pointers or count");
  glyph_index=static_cast<std::uint16_t>(glyph[0]);class_output=static_cast<std::uint8_t>(klass);
 }
 NativeGlyphIndex out(pinned,glyph_index,raw,flags,static_cast<std::uint8_t>(cp&0xff),class_output);
 pinned->validate();require_native_origin_current_without_callbacks_owned(pinned,out.origin_);
 return out;
}

BnwSystemFontCpuOwner::NativeGlyphIndex BnwSystemFontCpuOwner::map_wchar_to_glyph_index_owned(
 std::uint16_t cp,const graphics::detail::BnwOwnedFontPaintEffectFence& fence){
 const auto pinned=impl_;pinned->first_owned_paint(fence);
 std::uint16_t glyph_index=0;std::uint32_t raw=0,flags=0;std::uint8_t class_output=0;
 {
  Impl::Operation operation(*pinned);pinned->first_owned_paint(fence);
  {
   // Native550330 maps exactly low16 WCHAR and its low8 CLASSIN. No whole-run
   // class-mask restriction, scalar-domain filter or native width query.
   WCHAR input[2]{static_cast<WCHAR>(cp),0},glyph[2]{};
   UINT order=0;int dx=0;char klass=static_cast<char>(cp&0xff);
   GCP_RESULTSW result{};result.lStructSize=sizeof(result);result.lpOrder=&order;result.lpDx=&dx;
   result.lpClass=&klass;result.lpGlyphs=glyph;result.nGlyphs=1;
   flags=(pinned->frame.language_flags&0x13a)|GCP_CLASSIN;pinned->first_owned_paint(fence);
   pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
   raw=GetCharacterPlacementW(pinned->dc,input,1,0,&result,flags);
   pinned->first_owned_paint(fence);pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
   need(raw!=0&&result.nGlyphs==1&&result.lpOutString==nullptr&&result.lpCaretPos==nullptr&&
       result.lpOrder==&order&&result.lpDx==&dx&&result.lpClass==&klass&&result.lpGlyphs==glyph,
       "Paint single WCHAR GCP failed/changed pointers or count");pinned->first_owned_paint(fence);
   glyph_index=static_cast<std::uint16_t>(glyph[0]);class_output=static_cast<std::uint8_t>(klass);
   pinned->first_owned_paint(fence);
  } // GCP/input stack DATA cleaned before the final actual Source checks.
  pinned->first_owned_paint(fence);pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
 } // Operation cleanup precedes final Source POST; output construction does not allocate.
 pinned->first_owned_paint(fence);
 NativeGlyphIndex out(pinned,glyph_index,raw,flags,static_cast<std::uint8_t>(cp&0xff),class_output);
 pinned->first_owned_paint(fence);pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
 require_native_origin_current_without_callbacks_owned(pinned,out.origin_);pinned->first_owned_paint(fence);return out;
}

BnwSystemFontCpuOwner::NativeGcpResult BnwSystemFontCpuOwner::whole_gcp_utf16_owned(std::u16string_view borrowed){
 const auto pinned=impl_;Impl::Operation operation(*pinned);
 require_native_origin_current_without_callbacks_owned(pinned,pinned);pinned->validate();
 require_native_origin_current_without_callbacks_owned(pinned,pinned);
 // Caller retains the immutable converted C4 owner across the guards above.
 // No caller bytes are read after this callback-free owned stack snapshot.
 need(borrowed.size()<=4096,"Native whole GCP exceeds original bounded unit capacity");
 std::vector<std::uint16_t>output_glyphs;std::vector<std::int32_t>output_dx;
 std::uint32_t raw=0;const auto flags=(pinned->frame.language_flags&0x13a)|GCP_CLASSIN;
 const bool executed=!borrowed.empty();
 {
  std::array<WCHAR,4096>input{};const auto count=static_cast<int>(borrowed.size());
  for(int n=0;n<count;++n)input[n]=static_cast<WCHAR>(borrowed[n]);
  if(executed){
   need((pinned->frame.language_flags&0x132)==0,"Native whole CLASSIN inputs unproven for actual reorder/shaping flags");
   // Native6721E0 ignores lpOrder. Zero classes are admitted ONLY by actual no0x132 flags.
   std::array<UINT,4096>order{};std::array<int,4096>dx{};
   std::array<char,4096>classes{};std::array<WCHAR,4096>glyphs{};
   GCP_RESULTSW result{};result.lStructSize=sizeof(result);result.lpOrder=order.data();result.lpDx=dx.data();
   result.lpClass=classes.data();result.lpGlyphs=glyphs.data();result.nGlyphs=count;
   pinned->validate();require_native_origin_current_without_callbacks_owned(pinned,pinned);
   raw=GetCharacterPlacementW(pinned->dc,input.data(),count,0,&result,flags);
   require_native_origin_current_without_callbacks_owned(pinned,pinned);pinned->validate();
   require_native_origin_current_without_callbacks_owned(pinned,pinned);
   need(raw!=0&&result.nGlyphs<=4096&&result.lpOutString==nullptr&&result.lpCaretPos==nullptr&&
       result.lpOrder==order.data()&&result.lpDx==dx.data()&&result.lpClass==classes.data()&&result.lpGlyphs==glyphs.data(),
       "Native whole GCP failed/changed bounded output pointers or capacity");
   const auto produced=result.nGlyphs;
   output_glyphs.resize(produced);pinned->validate();require_native_origin_current_without_callbacks_owned(pinned,pinned);
   output_dx.resize(produced);pinned->validate();require_native_origin_current_without_callbacks_owned(pinned,pinned);
   for(UINT n=0;n<produced;++n){output_glyphs[n]=static_cast<std::uint16_t>(glyphs[n]);output_dx[n]=dx[n];}
  }
 }
 // Stack inputs/GCP buffers are gone before the final FULL/FIRST. Move-only DTO adds no allocation.
 NativeGcpResult out(pinned,std::move(output_glyphs),std::move(output_dx),raw,flags,sizeof(GCP_RESULTSW),executed);
 pinned->validate();require_native_origin_current_without_callbacks_owned(pinned,out.origin_);
 return out;
}

BnwSystemFontGlyphMeasurement BnwSystemFontCpuOwner::measure_glyph_index_owned(
 std::uint16_t glyph,const graphics::detail::BnwOwnedFontPaintEffectFence& fence){
 const auto pinned=impl_;pinned->first_owned_paint(fence);
 BnwSystemFontGlyphMeasurement out;
 {
  Impl::Operation operation(*pinned);pinned->first_owned_paint(fence);
  out=pinned->measure_index_owned_paint(glyph,fence);pinned->first_owned_paint(fence);
 }
 pinned->first_owned_paint(fence);pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);return out;
}
BnwSystemFontCpuOwner::NativeGcpResult BnwSystemFontCpuOwner::whole_gcp_utf16_owned(
 std::u16string_view borrowed,const graphics::detail::BnwOwnedFontPaintEffectFence& fence){
 const auto pinned=impl_;pinned->first_owned_paint(fence);
 need(borrowed.size()<=4096,"Paint whole GCP exceeds original bounded unit capacity");
 const auto count=static_cast<int>(borrowed.size());pinned->first_owned_paint(fence);
 std::vector<std::uint16_t> output_glyphs;std::vector<std::int32_t> output_dx;
 std::uint32_t raw=0,flags=0;const bool executed=count!=0;
 {
  Impl::Operation operation(*pinned);pinned->first_owned_paint(fence);
  {
   // Snapshot actual retained converted UTF16 into fixed stack DATA BEFORE
   // any Source callback. No raw borrowed input is reread after this cut.
   std::array<WCHAR,4096> input{};
   for(int n=0;n<count;++n)input[n]=static_cast<WCHAR>(borrowed[n]);
   pinned->first_owned_paint(fence);pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
   flags=(pinned->frame.language_flags&0x13a)|GCP_CLASSIN;pinned->first_owned_paint(fence);
   if(executed){
    need((pinned->frame.language_flags&0x132)==0,
     "Paint whole CLASSIN inputs unproven for actual reorder/shaping flags");pinned->first_owned_paint(fence);
    std::array<UINT,4096> order{};std::array<int,4096> dx{};
    std::array<char,4096> classes{};std::array<WCHAR,4096> glyphs{};
    GCP_RESULTSW result{};result.lStructSize=sizeof(result);result.lpOrder=order.data();result.lpDx=dx.data();
    result.lpClass=classes.data();result.lpGlyphs=glyphs.data();result.nGlyphs=count;
    pinned->first_owned_paint(fence);pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
    raw=GetCharacterPlacementW(pinned->dc,input.data(),count,0,&result,flags);
    pinned->first_owned_paint(fence);pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
    need(raw!=0&&result.nGlyphs<=4096&&result.lpOutString==nullptr&&result.lpCaretPos==nullptr&&
         result.lpOrder==order.data()&&result.lpDx==dx.data()&&result.lpClass==classes.data()&&result.lpGlyphs==glyphs.data(),
     "Paint whole GCP failed/changed bounded output pointers or capacity");pinned->first_owned_paint(fence);
    const auto produced=result.nGlyphs;
    output_glyphs.resize(produced);pinned->first_owned_paint(fence);
    pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
    output_dx.resize(produced);pinned->first_owned_paint(fence);
    pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
    for(UINT n=0;n<produced;++n){output_glyphs[n]=static_cast<std::uint16_t>(glyphs[n]);output_dx[n]=dx[n];}
    pinned->first_owned_paint(fence);
   }
  } // All input/GCP stack buffers cleaned up before final actual Source check.
  pinned->first_owned_paint(fence);pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
 }
 pinned->first_owned_paint(fence);
 NativeGcpResult out(pinned,std::move(output_glyphs),std::move(output_dx),raw,flags,sizeof(GCP_RESULTSW),executed);
 pinned->first_owned_paint(fence);pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
 require_native_origin_current_without_callbacks_owned(pinned,out.origin_);pinned->first_owned_paint(fence);return out;
}

BnwSystemFontWholeRunGcp BnwSystemFontCpuOwner::measure_plain_run(std::string_view borrowed){
 // Pin bytes and SAME Impl before any policy/resource callback. Callbacks may
 // replace caller storage or release the last public facade reference.
 need(!borrowed.empty()&&borrowed.size()<=4096,"Native plain run outside bounded input budget");
 const std::string pinned_text(borrowed);const auto pinned=impl_;
 const std::string_view text(pinned_text);auto&i=*pinned;Impl::Operation operation(i);i.validate();need(!text.empty()&&text.size()<=4096&&text.find('\0')==text.npos,"Native plain run must be complete UTF8 within 4096 bytes");
 const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0);need(count>0&&count<=4096,"Native plain run malformed/oversized UTF8");std::wstring input(count,L'\0');need(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),input.data(),count)==count,"Native plain run conversion failed");
 for(const auto cp:input)need(cp>=0x20&&cp!=0x7f&&!(cp>=0x80&&cp<0xa0)&&!(cp>=0xd800&&cp<=0xdfff)&&cp!=L'['&&cp!=L']',"Native plain run excludes controls/markup/surrogate shaping scope");
 need((i.frame.language_flags&0x132)==0,"Native whole run requires unknown CLASSIN/reorder/shaping producer");
 // Native supplies four 4096 element buffers; its class buffer is uninitialized.
 // Civis explicitly initializes classes only within the verified no0x132 route.
 std::array<UINT,4096>order{};std::array<int,4096>dx{};std::array<char,4096>classes{};std::array<WCHAR,4096>glyphs{};GCP_RESULTSW result{};result.lStructSize=sizeof(result);result.lpOrder=order.data();result.lpDx=dx.data();result.lpClass=classes.data();result.lpGlyphs=glyphs.data();result.nGlyphs=count;
 const auto raw=GetCharacterPlacementW(i.dc,input.data(),count,0,&result,i.frame.gcp_flags);need(raw!=0&&result.nGlyphs==static_cast<UINT>(count)&&result.lpOutString==nullptr&&result.lpCaretPos==nullptr&&result.lpOrder==order.data()&&result.lpDx==dx.data()&&result.lpClass==classes.data()&&result.lpGlyphs==glyphs.data(),"Native whole run GCP failed or changed bounded count/pointers");
 BnwSystemFontWholeRunGcp out;out.utf8=text;out.packed_return=raw;out.gcp_flags=i.frame.gcp_flags;out.gcp_structure_size=sizeof(result);out.pointers_preserved=true;out.explicit_zero_class_input=true;out.class_input.assign(count,0);
 for(int n=0;n<count;++n){need(order[n]==static_cast<UINT>(n)&&glyphs[n]!=0&&glyphs[n]!=0xffff&&dx[n]>=0&&dx[n]<=32767,"Native whole run requires unsupported reordering/missing glyph/advance");const auto single=i.measure_codepoint(static_cast<std::uint16_t>(input[n]));need(single.glyph_index==glyphs[n],"Native whole run and single lookup glyph mapping differ");out.utf16.push_back(static_cast<std::uint16_t>(input[n]));out.glyphs.push_back(glyphs[n]);out.order.push_back(order[n]);out.dx.push_back(dx[n]);out.class_output.push_back(static_cast<std::uint8_t>(classes[n]));out.single_lookups.push_back(single);}
 i.validate();return out;
}
BnwSystemFontGlyphCpu BnwSystemFontCpuOwner::Impl::raster_index(BnwSystemFontGlyphMeasurement measured){
 auto&i=*this;const auto code=measured.glyph_index;if(const auto old=i.raster.find(code);old!=i.raster.end()){i.validate();return old->second;}
 BnwSystemFontGlyphCpu out;out.glyph_index=code;out.height=i.frame.cell_height;out.width=out.advance=measured.advance;WCHAR glyph[2]{static_cast<WCHAR>(code),0};
 std::memset(i.pixels,0xff,static_cast<std::size_t>(i.frame.cell_width)*i.frame.cell_height*4);need(ExtTextOutW(i.dc,0,1,ETO_OPAQUE|ETO_GLYPH_INDEX,nullptr,glyph,1,nullptr),"Native glyph-index DIB raster failed");need(GdiFlush(),"Native glyph raster flush failed");out.green_brightness.resize(static_cast<std::size_t>(out.width)*out.height);out.shadow_brightness.assign(out.green_brightness.size(),0xff);const auto*pixel=static_cast<const std::uint8_t*>(i.pixels);for(int y=0;y<out.height;++y)for(int x=0;x<out.width;++x)out.green_brightness[static_cast<std::size_t>(y)*out.width+x]=pixel[(static_cast<std::size_t>(y)*i.frame.cell_width+x)*4+1];
 for(int y=0;y<out.height-1;++y)for(int x=0;x<out.width-1;++x){const auto at=static_cast<std::size_t>(y)*out.width+x;const unsigned a=out.green_brightness[at+1],b=out.green_brightness[at+out.width],c=out.green_brightness[at+out.width+1];out.shadow_brightness[at]=static_cast<std::uint8_t>(255-(1275-2*(a+b)-c)/10);}
 i.validate();i.raster.emplace(code,out);return out;
}


// Private actual-Gui Paint raster. Do not call the unfenced legacy helpers.
void BnwSystemFontCpuOwner::Impl::first_owned_paint(
 const graphics::detail::BnwOwnedFontPaintEffectFence& fence)const{
 fence.require_current_without_callbacks();
 need(thread==std::this_thread::get_id()&&label_source&&!policy,
  "Paint raster requires creator actual-Gui CPU source");
 label_source->require_current_without_callbacks();fence.require_current_without_callbacks();
 need(!facade.expired(),"Paint raster CPU facade retired");
 fence.require_current_without_callbacks();
}
void BnwSystemFontCpuOwner::Impl::validate_owned_paint(
 const graphics::detail::BnwOwnedFontPaintEffectFence& fence)const{
 first_owned_paint(fence);
 {
  Query query(*this);first_owned_paint(fence);
  const auto source=label_source;first_owned_paint(fence);
  need(source->hwnd_without_callbacks()==reinterpret_cast<std::uintptr_t>(hwnd),
   "Paint raster actual-Gui HWND changed");first_owned_paint(fence);
  source->require_current_owned();first_owned_paint(fence);
  DWORD process=0;first_owned_paint(fence);
  const auto window_thread=GetWindowThreadProcessId(hwnd,&process);first_owned_paint(fence);
  const auto exists=IsWindow(hwnd);first_owned_paint(fence);
  const auto actual_process=GetCurrentProcessId();first_owned_paint(fence);
  const auto actual_thread=GetCurrentThreadId();first_owned_paint(fence);
  need(exists&&process==actual_process&&window_thread==actual_thread,
   "Paint raster requires SAME current-process HWND thread");first_owned_paint(fence);
  RECT rect{};first_owned_paint(fence);
  const auto got_rect=GetClientRect(hwnd,&rect);first_owned_paint(fence);
  need(got_rect&&rect.right-rect.left==frame.client.width&&rect.bottom-rect.top==frame.client.height,
   "Paint raster actual-Gui client extent changed");first_owned_paint(fence);
  source->require_current_owned();first_owned_paint(fence);
 } // Source/Query physical temporaries and query counter clean up BEFORE POST.
 first_owned_paint(fence);
}
BnwSystemFontGlyphMeasurement BnwSystemFontCpuOwner::Impl::measure_index_owned_paint(
 std::uint16_t glyph,const graphics::detail::BnwOwnedFontPaintEffectFence& fence){
 first_owned_paint(fence);validate_owned_paint(fence);first_owned_paint(fence);
 BnwSystemFontGlyphMeasurement out;out.glyph_index=glyph;
 if((frame.pitch_and_family&TMPF_TRUETYPE)!=0){
  ABC abc{};first_owned_paint(fence);
  const auto measured=GetCharABCWidthsI(dc,glyph,1,nullptr,&abc);first_owned_paint(fence);
  need(measured,"Paint native glyph ABCI unavailable");first_owned_paint(fence);
  const auto sum=static_cast<std::int64_t>(abc.abcA)+abc.abcB;
  need(sum>=0&&sum<=frame.cell_width,"Paint native glyph A+B outside owned DIB");
  out.advance=static_cast<int>(sum);
 }else{
  int width=0;first_owned_paint(fence);
  const auto measured=GetCharWidthI(dc,glyph,1,nullptr,&width);first_owned_paint(fence);
  need(measured&&width>=0&&width<=frame.cell_width,"Paint native glyph widthI unavailable/outside DIB");
  out.advance=width;
 }
 first_owned_paint(fence);validate_owned_paint(fence);first_owned_paint(fence);return out;
}
BnwSystemFontGlyphCpu BnwSystemFontCpuOwner::Impl::raster_index_owned_paint(
 BnwSystemFontGlyphMeasurement measured,const graphics::detail::BnwOwnedFontPaintEffectFence& fence){
 first_owned_paint(fence);const auto code=measured.glyph_index;
 BnwSystemFontGlyphCpu out;
 {
  const auto old=raster.find(code);first_owned_paint(fence);
  if(old!=raster.end()){
   // Split each potentially allocating plane copy. Allocator reentry must
   // return through the bottom fence BEFORE reading retained map bytes.
   validate_owned_paint(fence);first_owned_paint(fence);
   out.glyph_index=old->second.glyph_index;out.advance=old->second.advance;
   out.width=old->second.width;out.height=old->second.height;
   const auto green_size=old->second.green_brightness.size();first_owned_paint(fence);
   out.green_brightness.resize(green_size);first_owned_paint(fence);
   std::copy(old->second.green_brightness.begin(),old->second.green_brightness.end(),out.green_brightness.begin());
   first_owned_paint(fence);
   const auto shadow_size=old->second.shadow_brightness.size();first_owned_paint(fence);
   out.shadow_brightness.resize(shadow_size);first_owned_paint(fence);
   std::copy(old->second.shadow_brightness.begin(),old->second.shadow_brightness.end(),out.shadow_brightness.begin());
   first_owned_paint(fence);
  }else{
   out.glyph_index=code;out.height=frame.cell_height;out.width=out.advance=measured.advance;
   WCHAR glyph[2]{static_cast<WCHAR>(code),0};first_owned_paint(fence);
   std::memset(pixels,0xff,static_cast<std::size_t>(frame.cell_width)*frame.cell_height*4);
   first_owned_paint(fence);
   const auto drawn=ExtTextOutW(dc,0,1,ETO_OPAQUE|ETO_GLYPH_INDEX,nullptr,glyph,1,nullptr);
   first_owned_paint(fence);need(drawn,"Paint native glyph-index DIB raster failed");
   first_owned_paint(fence);const auto flushed=GdiFlush();first_owned_paint(fence);
   need(flushed,"Paint native glyph raster flush failed");first_owned_paint(fence);
   const auto plane_size=static_cast<std::size_t>(out.width)*out.height;
   out.green_brightness.resize(plane_size);first_owned_paint(fence);
   out.shadow_brightness.resize(plane_size);first_owned_paint(fence);
   std::fill(out.shadow_brightness.begin(),out.shadow_brightness.end(),0xff);first_owned_paint(fence);
   const auto*pixel=static_cast<const std::uint8_t*>(pixels);
   for(int y=0;y<out.height;++y)for(int x=0;x<out.width;++x)
    out.green_brightness[static_cast<std::size_t>(y)*out.width+x]=pixel[(static_cast<std::size_t>(y)*frame.cell_width+x)*4+1];
   first_owned_paint(fence);
   for(int y=0;y<out.height-1;++y)for(int x=0;x<out.width-1;++x){
    const auto at=static_cast<std::size_t>(y)*out.width+x;
    const unsigned a=out.green_brightness[at+1],b=out.green_brightness[at+out.width],c=out.green_brightness[at+out.width+1];
    out.shadow_brightness[at]=static_cast<std::uint8_t>(255-(1275-2*(a+b)-c)/10);
   }
   first_owned_paint(fence);validate_owned_paint(fence);first_owned_paint(fence);
   {
    // A single map emplace(code,out) hides node + TWO vector allocations.
    // Preallocate an empty node, then fill its planes one allocation at a time.
    std::map<std::uint16_t,BnwSystemFontGlyphCpu> pending;
    first_owned_paint(fence);auto added=pending.try_emplace(code);first_owned_paint(fence);
    auto& stored=added.first->second;
    stored.glyph_index=out.glyph_index;stored.advance=out.advance;stored.width=out.width;stored.height=out.height;
    first_owned_paint(fence);stored.green_brightness.resize(out.green_brightness.size());first_owned_paint(fence);
    std::copy(out.green_brightness.begin(),out.green_brightness.end(),stored.green_brightness.begin());first_owned_paint(fence);
    stored.shadow_brightness.resize(out.shadow_brightness.size());first_owned_paint(fence);
    std::copy(out.shadow_brightness.begin(),out.shadow_brightness.end(),stored.shadow_brightness.begin());first_owned_paint(fence);
    auto node=pending.extract(added.first);first_owned_paint(fence);
    const auto inserted=raster.insert(std::move(node));first_owned_paint(fence);
    need(inserted.inserted,"Paint raster CPU map unexpectedly reentered");first_owned_paint(fence);
   }
   first_owned_paint(fence);
  }
 } // Map iterators/node handles and staging temporaries are gone before FULL.
 first_owned_paint(fence);validate_owned_paint(fence);first_owned_paint(fence);return out;
}
BnwSystemFontGlyphCpu BnwSystemFontCpuOwner::raster_glyph_index_owned(
 std::uint16_t glyph,const graphics::detail::BnwOwnedFontPaintEffectFence& fence){
 const auto pinned=impl_;pinned->first_owned_paint(fence);
 BnwSystemFontGlyphCpu result;
 {
  Impl::Operation operation(*pinned);pinned->first_owned_paint(fence);
  const auto measured=pinned->measure_index_owned_paint(glyph,fence);pinned->first_owned_paint(fence);
  result=pinned->raster_index_owned_paint(measured,fence);pinned->first_owned_paint(fence);
 }
 pinned->first_owned_paint(fence);pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
 return result;
}
std::array<std::int32_t,6> BnwSystemFontCpuOwner::native_raster_geometry_owned(
 const graphics::detail::BnwOwnedFontPaintEffectFence& fence)const{
 const auto pinned=impl_;pinned->first_owned_paint(fence);
 std::array<std::int32_t,6> result{};
 {
  Impl::Operation operation(*pinned);pinned->first_owned_paint(fence);
  pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);
  const auto& frame=pinned->frame;
  result={frame.cell_width,frame.cell_height,frame.columns,frame.rows,frame.atlas_width,frame.atlas_height};
  pinned->first_owned_paint(fence);
 }
 pinned->first_owned_paint(fence);pinned->validate_owned_paint(fence);pinned->first_owned_paint(fence);return result;
}
static_assert(std::is_nothrow_move_constructible_v<BnwSystemFontGlyphCpu>&&
 std::is_nothrow_move_assignable_v<BnwSystemFontGlyphCpu>,"Paint raster return must not allocate/copy planes");

BnwSystemFontGlyphCpu BnwSystemFontCpuOwner::measure_and_raster(std::uint32_t cp){
 const auto pinned=impl_;Impl::Operation operation(*pinned);
 return pinned->raster_index(pinned->measure_codepoint(cp));
}
BnwSystemFontGlyphCpu BnwSystemFontCpuOwner::raster_glyph_index(std::uint16_t glyph){
 const auto pinned=impl_;Impl::Operation operation(*pinned);
 return pinned->raster_index(pinned->measure_index(glyph));
}
}
