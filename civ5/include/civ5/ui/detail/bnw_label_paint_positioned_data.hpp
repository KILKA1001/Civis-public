#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include "civ5/ui/detail/bnw_label_paint_text_loan.hpp"
#include "civ5/ui/detail/bnw_label_paint_prepared_flush_loan.hpp"
#include "civ5/graphics/detail/bnw_normal_label_draw_record_binding.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics {class GlBnwSystemFontAtlas;class GlBnwFontShader;}
namespace civ5::graphics::detail {class BnwLabelDefaultFontTargetLoan;}
namespace civ5::ui {class UiScreenGlRenderer;class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class BnwLabelPaintTextLoan;
class BnwNormalLabelMaterialLoan;
class BnwNormalItemOccurrence;
class BnwNormalFontPayloadPrefix;
class BnwLabelSystemFontBorrower;
// Original591020 selected positive170 CPU record membership DATA. This is
// neither7505 positions nor submit/atlas/target/clear-dirty readiness.
class BnwLabelPaintPositionedData final {
 friend class ::civ5::graphics::detail::BnwLabelDefaultFontTargetLoan;
 friend class BnwLabelPaintTextLoan;
 friend class BnwNormalLabelMaterialLoan;
 friend class BnwNormalItemOccurrence;
 friend class BnwNormalFontPayloadPrefix;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::graphics::GlBnwSystemFontAtlas;
 friend class ::civ5::graphics::GlBnwFontShader;
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 explicit BnwLabelPaintPositionedData(std::shared_ptr<Impl>);
 struct RecordData final {
  std::uint32_t begin,byte_count,style_ordinal,source_discriminator,inline_payload;
  std::array<std::uint32_t,3> packed_colors,style_pass_selectors;
 };
 struct GlyphData final {
  std::uint32_t record_ordinal,byte_begin,byte_count,codepoint,lookup_codepoint;
  std::uint32_t cursor_x,line_y;
  std::array<std::uint32_t,9> native_bits;
 };
 struct PassData final {
  std::uint32_t glyph_ordinal,pass_ordinal,packed_color;
 };
 class Guard final {
  friend class BnwLabelPaintPositionedData;
  friend class BnwNormalLabelMaterialLoan;
  friend class BnwNormalItemOccurrence;
  friend class BnwNormalFontPayloadPrefix;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  friend class ::civ5::ui::UiScreenGlRenderer;
  friend class ::civ5::graphics::GlBnwSystemFontAtlas;
  friend class ::civ5::graphics::GlBnwFontShader;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_current_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_system_font_borrower_without_callbacks(const BnwLabelSystemFontBorrower&)const;
  void require_actual_renderer_without_callbacks(const ::civ5::ui::UiScreenGlRenderer&)const;
  std::span<const GlyphData> glyphs_without_callbacks()const;
  std::span<const RecordData> records_without_callbacks()const;
  std::span<const PassData> passes_without_callbacks()const;
  std::array<std::uint32_t,2> actual_numeric_profile_without_callbacks()const;
  std::shared_ptr<const BnwNormalCaptionFp32RectLoan> actual_caption_fp32_rect_without_callbacks()const;
  // Existing lexical guards only; no facade, issuer or refreshed phase.
  BnwLabelPaintTextLoan::Guard captured_paint_guard_without_callbacks()const;
  BnwLabelPaintPreparedFlushLoan::Guard captured_early_flush_guard_without_callbacks()const;
  void require_same_paint_guard_without_callbacks(const BnwLabelPaintTextLoan::Guard&)const;
  //Original Graph CPU association captured only under ACTIVE DATA FULL.
  ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::Guard
   capture_normal_draw_records_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_same_normal_draw_records_without_callbacks(
   const ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::Guard&)const;
  void require_same_normal_draw_records_source_owned(
   const runtime::BnwWorldViewSourceOperationLoan&,
   const ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::Guard&)const;
 };
 Guard capture_guard_without_callbacks()const;
 // One closed association on this DATA; actual Shader alone mints the binding.
 void attach_normal_draw_records_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,
  const ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::Guard&)const;
 ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::Guard
  originating_normal_draw_records_without_callbacks()const;
public:
 ~BnwLabelPaintPositionedData(); // Retires own DATA lease FIRST.
 BnwLabelPaintPositionedData(const BnwLabelPaintPositionedData&)=delete;
 BnwLabelPaintPositionedData&operator=(const BnwLabelPaintPositionedData&)=delete;
};
}
