#pragma once
#include "civ5/ui/bnw_client_text_policy.hpp"
#include <span>
#include <array>
namespace civ5::graphics::detail {class BnwOwnedFontPaintEffectFence;}
#include <string_view>
namespace civ5::ui {
class BnwSystemFontCacheOwner;
namespace detail {class BnwLabelSystemFontSource;class NotificationContextGuiPort;
class BnwLabelSystemFontBorrower;class BnwLabelSystemFontPreparedRun;struct BnwSystemFontNativeRunStage;}
struct BnwSystemFontCpuFrame {
 runtime::BnwStockLuaClientExtent client;
 BnwLabelLocaleSelection locale;
 BnwLanguageFontRow language_row;
 std::string font_key,font_sha256,actual_face,font_binary_sha256;
 std::int32_t source_line_height=0,source_label_ascent=0,logfont_width=0,logfont_height=0;
 std::int32_t tm_height=0,tm_max_width=0,cell_width=0,cell_height=0;
 std::uint32_t language_flags=0,gcp_flags=0,charset=0,quality=0,pitch_and_family=0;
 std::uint32_t text_color=0,background_color=0,text_align=0;
 std::int32_t background_mode=0,map_mode=0,graphics_mode=0,text_extra=0;
 std::int32_t atlas_width=0,atlas_height=0,columns=0,rows=0;
 bool measurement_available=false,cpu_raster_available=false,gpu_pair_ready=false;
 std::string pending_gpu_producer;
};
struct BnwSystemFontGlyphMeasurement {
 std::uint16_t glyph_index=0;
 std::int32_t advance=0;
 std::uint32_t codepoint=0,gcp_flags=0,gcp_structure_size=0;
 std::uint8_t class_input=0,class_output=0;
 bool pointers_preserved=false;
};
struct BnwSystemFontGlyphCpu {
 std::uint16_t glyph_index=0;
 std::int32_t advance=0,width=0,height=0;
 // Native0x5084E0 uploads brightness, not conventional alpha. Preserve bytes.
 assets::Bytes green_brightness,shadow_brightness;
};
struct BnwSystemFontWholeRunGcp {
 std::string utf8;
 std::vector<std::uint16_t> utf16,glyphs;
 std::vector<std::uint32_t> order;
 std::vector<std::int32_t> dx;
 std::vector<std::uint8_t> class_input,class_output;
 std::vector<BnwSystemFontGlyphMeasurement> single_lookups;
 std::uint32_t packed_return=0,gcp_flags=0,gcp_structure_size=0;
 bool pointers_preserved=false,explicit_zero_class_input=false;
 // Original whole-run stack classes are unknown. This is the no-reorder,
 // no-shaping bounded route with explicit Civis input, not native stack parity.
 bool original_class_inputs_verified=false,native_label_initialized=false;
};
class BnwSystemFontCpuOwner {
 public:
 static std::shared_ptr<BnwSystemFontCpuOwner> create(std::uintptr_t actual_hwnd,
  std::shared_ptr<BnwClientTextPolicyOwner>,const assets::ResourceMountTable&,
  const BnwNativePlainLabelMetric&);
 // Same DC/raster/cache construction, admitted only by the sealed current
 // WorldView candidate issuer. This does not mark native Label initialized.
 static std::shared_ptr<BnwSystemFontCpuOwner> create_from_worldview_title_descriptor(
     std::shared_ptr<BnwClientTextPolicyOwner>,const BnwWorldViewTitleFontDescriptor&);
 ~BnwSystemFontCpuOwner();
 BnwSystemFontCpuOwner(const BnwSystemFontCpuOwner&)=delete;
 BnwSystemFontCpuOwner&operator=(const BnwSystemFontCpuOwner&)=delete;
 BnwSystemFontCpuOwner(BnwSystemFontCpuOwner&&)=delete;
 BnwSystemFontCpuOwner&operator=(BnwSystemFontCpuOwner&&)=delete;
 const BnwSystemFontCpuFrame&current()const;
 std::uintptr_t native_window_handle()const;
 BnwSystemFontGlyphMeasurement measure(std::uint32_t bmp_codepoint);
 // Direct source0x4065E0 input: an already returned uint16 GCP glyph index.
 // No Unicode mapping or CLASSIN input is manufactured by this method.
 BnwSystemFontGlyphMeasurement measure_glyph_index(std::uint16_t glyph_index);
 BnwSystemFontGlyphCpu raster_glyph_index(std::uint16_t glyph_index);
 BnwSystemFontWholeRunGcp measure_plain_run(std::string_view utf8);
 BnwSystemFontGlyphCpu measure_and_raster(std::uint32_t bmp_codepoint);
 private:
 friend class detail::NotificationContextGuiPort;
 static std::shared_ptr<BnwSystemFontCpuOwner> create_from_actual_gui_source(
     std::shared_ptr<const detail::BnwLabelSystemFontSource>);
 static std::shared_ptr<BnwSystemFontCpuOwner> create_from_source_fields(
     std::uintptr_t,std::shared_ptr<BnwClientTextPolicyOwner>,const assets::ResourceMountTable&,
     BnwClientTextPolicyFrame,std::string font_key,std::string font_sha256,std::int32_t adjusted_ascent);
 struct Impl;
 static std::shared_ptr<BnwSystemFontCpuOwner> create_from_prepared_fields(std::shared_ptr<Impl>);
 friend class BnwSystemFontCacheOwner;
 friend class detail::BnwLabelSystemFontBorrower;
 friend class detail::BnwLabelSystemFontPreparedRun;
 friend struct detail::BnwSystemFontNativeRunStage;
 // Read-only native results. Only this SAME sealed CPU/DC can issue them.
 class NativeGlyphIndex {
  public:
  NativeGlyphIndex(NativeGlyphIndex&&)noexcept=default;
  NativeGlyphIndex(const NativeGlyphIndex&)=delete;
  NativeGlyphIndex&operator=(const NativeGlyphIndex&)=delete;
  NativeGlyphIndex&operator=(NativeGlyphIndex&&)=delete;
  std::uint16_t glyph_index()const noexcept{return glyph_;}
  std::uint32_t packed_return()const noexcept{return raw_;}
  std::uint32_t gcp_flags()const noexcept{return flags_;}
  std::uint8_t class_input()const noexcept{return input_class_;}
  std::uint8_t class_output()const noexcept{return output_class_;}
  private:
  friend class BnwSystemFontCpuOwner;
  NativeGlyphIndex(std::weak_ptr<Impl>origin,std::uint16_t glyph,std::uint32_t raw,std::uint32_t flags,
      std::uint8_t input_class,std::uint8_t output_class)noexcept:
      origin_(std::move(origin)),glyph_(glyph),raw_(raw),flags_(flags),input_class_(input_class),output_class_(output_class){}
  std::weak_ptr<Impl>origin_;
  std::uint16_t glyph_;
  std::uint32_t raw_,flags_;
  std::uint8_t input_class_,output_class_;
 };
 class NativeGcpResult {
  public:
  NativeGcpResult(NativeGcpResult&&)noexcept=default;
  NativeGcpResult(const NativeGcpResult&)=delete;
  NativeGcpResult&operator=(const NativeGcpResult&)=delete;
  NativeGcpResult&operator=(NativeGcpResult&&)=delete;
  std::span<const std::uint16_t>glyphs()const noexcept{return glyphs_;}
  std::span<const std::int32_t>dx()const noexcept{return dx_;}
  std::uint32_t glyph_count()const noexcept{return static_cast<std::uint32_t>(glyphs_.size());}
  std::uint32_t packed_return()const noexcept{return raw_;}
  std::uint32_t gcp_flags()const noexcept{return flags_;}
  std::uint32_t gcp_structure_size()const noexcept{return structure_size_;}
  bool pointers_preserved()const noexcept{return true;}
  bool explicit_zero_class_input()const noexcept{return executed_;}
  bool original_class_inputs_verified()const noexcept{return false;}
  bool executed()const noexcept{return executed_;}
  private:
  friend class BnwSystemFontCpuOwner;
  NativeGcpResult(std::weak_ptr<Impl>origin,std::vector<std::uint16_t>glyphs,std::vector<std::int32_t>dx,
      std::uint32_t raw,std::uint32_t flags,std::uint32_t structure_size,bool executed)noexcept:
      origin_(std::move(origin)),glyphs_(std::move(glyphs)),dx_(std::move(dx)),raw_(raw),flags_(flags),
      structure_size_(structure_size),executed_(executed){}
  std::weak_ptr<Impl>origin_;
  std::vector<std::uint16_t>glyphs_;
  std::vector<std::int32_t>dx_;
  std::uint32_t raw_,flags_,structure_size_;
  bool executed_;
 };
 BnwSystemFontGlyphCpu raster_glyph_index_owned(std::uint16_t,const graphics::detail::BnwOwnedFontPaintEffectFence&);
 std::array<std::int32_t,6> native_raster_geometry_owned(const graphics::detail::BnwOwnedFontPaintEffectFence&)const;
 NativeGlyphIndex map_wchar_to_glyph_index_owned(std::uint16_t);
 NativeGlyphIndex map_wchar_to_glyph_index_owned(std::uint16_t,const graphics::detail::BnwOwnedFontPaintEffectFence&);
 NativeGcpResult whole_gcp_utf16_owned(std::u16string_view);
 NativeGcpResult whole_gcp_utf16_owned(std::u16string_view,const graphics::detail::BnwOwnedFontPaintEffectFence&);
 BnwSystemFontGlyphMeasurement measure_glyph_index_owned(std::uint16_t,const graphics::detail::BnwOwnedFontPaintEffectFence&);
 void require_native_glyph_index_current_without_callbacks(const NativeGlyphIndex&)const;
 void require_native_gcp_current_without_callbacks(const NativeGcpResult&)const;
 static void require_native_origin_current_without_callbacks_owned(
     const std::shared_ptr<Impl>&,const std::weak_ptr<Impl>&);
 std::shared_ptr<Impl>impl_;
 explicit BnwSystemFontCpuOwner(std::shared_ptr<Impl>);
};
}
