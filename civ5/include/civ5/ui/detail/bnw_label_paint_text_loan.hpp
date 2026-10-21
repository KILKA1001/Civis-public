#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string_view>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics {class GlBnwSystemFontAtlas;class GlBnwFontShader;}
namespace civ5::ui {class UiScreenGlRenderer;class BnwWorldViewControlTree;class BnwWorldViewControlRenderPublication;}
namespace civ5::ui::detail {
class BnwLabelPaintPositionedData;
class BnwLabelPaintPreparedFlushLoan;
class BnwNormalFontPayloadPrefix;
class BnwNormalCaptionFp32RectLoan;
class BnwNormalLabelMaterialLoan;
class BnwNormalItemOccurrence;
class BnwCompletedFontLoan;
class BnwLabelActualPaintCallFrame;
class BnwPositionedLabelRunLoan;
class BnwStandalonePositionedLabelRunLoan;
class BnwLabelSystemFontBorrower;
class BnwLabelSystemFontPreparedRun;
// Sealed lexical actual591020 text input. Physical C4 retention never renews
// Run, Source, cache, renderer/frame/publication or emitted-glyph membership.
class BnwLabelPaintTextLoan final {
 friend class BnwLabelPaintPositionedData;
 friend class BnwLabelPaintPreparedFlushLoan;
 friend class BnwNormalFontPayloadPrefix;
 friend class BnwNormalLabelMaterialLoan;
 friend class BnwNormalItemOccurrence;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class BnwLabelSystemFontBorrower;
 friend class BnwLabelSystemFontPreparedRun;
 friend class ::civ5::graphics::GlBnwSystemFontAtlas;
 friend class ::civ5::graphics::GlBnwFontShader;
 struct PositionedState;
 struct Impl;
 struct StandaloneImpl;
 const std::shared_ptr<StandaloneImpl> standalone_{};
 const std::shared_ptr<Impl> impl_;
 explicit BnwLabelPaintTextLoan(std::shared_ptr<Impl>);
 explicit BnwLabelPaintTextLoan(std::shared_ptr<StandaloneImpl>);
 // Renderer supplies its real entered tree/control. The id is only compared
 // against BOTH sealed current Frame and Run, never used to issue authority.
 static std::unique_ptr<BnwLabelPaintTextLoan> begin_owned(
     const runtime::BnwWorldViewSourceOperationLoan&,
     const std::shared_ptr<const ::civ5::ui::BnwWorldViewControlTree>&,
     std::uint64_t actual_entered_control_id,
     const BnwPositionedLabelRunLoan&,const BnwLabelActualPaintCallFrame&);
 // Separate actual embedded178 entry; owns originals and rect before entered Run Source cuts.
 static std::unique_ptr<BnwLabelPaintTextLoan> begin_caption_owned(
     const runtime::BnwWorldViewSourceOperationLoan&,
     std::shared_ptr<const ::civ5::ui::BnwWorldViewControlTree>,std::uint64_t,
     std::shared_ptr<const BnwPositionedLabelRunLoan>,
     std::shared_ptr<const ::civ5::ui::BnwWorldViewControlRenderPublication>,std::size_t,
     const BnwLabelActualPaintCallFrame&);
 // Separate closed normal-Label entry. Exact item membership is Frame9;
 // Run provenance is Source8 through Publication NormalInvocation10.
 static std::unique_ptr<BnwLabelPaintTextLoan> begin_standalone_owned(
     const runtime::BnwWorldViewSourceOperationLoan&,
     const std::shared_ptr<const ::civ5::ui::BnwWorldViewControlTree>&,std::uint64_t,
     std::shared_ptr<const BnwStandalonePositionedLabelRunLoan>,
     const ::civ5::ui::BnwWorldViewControlRenderPublication&,std::size_t,
     const BnwLabelActualPaintCallFrame&);
 // CPU DATA only. Atlas association is checked by its genuine closed issuer.
 std::unique_ptr<BnwLabelPaintPositionedData> positioned_data_owned(
     const runtime::BnwWorldViewSourceOperationLoan&,
     std::shared_ptr<::civ5::graphics::GlBnwSystemFontAtlas>)const;
 class Guard final {
  friend class BnwLabelPaintPositionedData;
  friend class BnwNormalFontPayloadPrefix;
  friend class BnwNormalLabelMaterialLoan;
  friend class BnwNormalItemOccurrence;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  friend class BnwLabelPaintTextLoan;
  friend class ::civ5::ui::UiScreenGlRenderer;
  friend class BnwLabelSystemFontBorrower;
  friend class BnwLabelSystemFontPreparedRun;
  friend class ::civ5::graphics::GlBnwSystemFontAtlas;
  friend class ::civ5::graphics::GlBnwFontShader;
  const std::shared_ptr<StandaloneImpl> standalone_{};
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  explicit Guard(std::shared_ptr<StandaloneImpl>);
  void require_same_paint_guard_without_callbacks(const Guard&)const;
  void require_current_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_system_font_borrower_without_callbacks(const BnwLabelSystemFontBorrower&)const;
  void require_actual_renderer_without_callbacks(const ::civ5::ui::UiScreenGlRenderer&)const;
  std::string_view actual_utf8_without_callbacks()const;
  std::string_view actual_c4_bytes_without_callbacks()const; // Includes true NUL.
  std::array<std::uint32_t,2> actual_numeric_profile_without_callbacks()const;
  std::shared_ptr<const BnwNormalCaptionFp32RectLoan> actual_caption_fp32_rect_without_callbacks()const;
  // Immutable selected record DATA only; no7505 membership/flush/draw grant.
  struct RecordData final {
   std::uint32_t count,begin,byte_count,style,source_discriminator,inline_payload;
   std::array<std::uint32_t,3> packed_colors,style_pass_selectors;
   std::array<std::int32_t,3> font_metrics;
   std::uint8_t newline;
   std::int16_t leading,truncate;
   std::uint32_t flags;
  };
  RecordData actual_record_without_callbacks()const;
  struct PositionedRecordData final {
   std::uint32_t count,begin,byte_count,style,source_discriminator,inline_payload;
   std::array<std::uint32_t,3> packed_colors;
   std::uint8_t newline;std::int16_t leading,truncate;std::uint32_t flags;
  };
  PositionedRecordData actual_positioned_record_without_callbacks()const;
  std::array<std::uint32_t,3> actual_positioned_style_passes_without_callbacks()const;
  std::array<std::int32_t,3> actual_positioned_font_metrics_without_callbacks()const;
 };
 // Consumers capture this strong physical DATA guard BEFORE any allocating
 // copy/callback; they do not reread the lexical facade afterwards.
 Guard capture_guard_without_callbacks()const;
public:
 ~BnwLabelPaintTextLoan(); // Retires lexical authority FIRST.
 BnwLabelPaintTextLoan(const BnwLabelPaintTextLoan&)=delete;
 BnwLabelPaintTextLoan&operator=(const BnwLabelPaintTextLoan&)=delete;
};
}
