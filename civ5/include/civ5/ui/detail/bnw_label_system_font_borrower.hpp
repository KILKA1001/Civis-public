#pragma once
#include <memory>
#include "civ5/ui/detail/bnw_label_paint_text_loan.hpp"
#include <array>
#include <cstdint>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class UiScreenGlRenderer;class BnwWorldViewControlTree;}
namespace civ5::graphics {class GlBnwSystemFontAtlas;}
namespace civ5::graphics::detail {class BnwLabelSystemFontAtlasMetadata;class BnwOwnedFontPaintEffectFence;}
namespace civ5::ui::detail {
class BnwPositionedLabelRunLoan;
class NotificationContextGuiPort;
class BnwLabelCurrentTextLoan;
class BnwLabelPaintTextLoan;
class BnwLabelSystemFontPreparedRun;
struct BnwSystemFontNativeGlyphData;
class NotificationLayoutEmbeddedLabelState;
class NotificationLayoutStandaloneLabelState;
class NotificationLayoutRuntimeDimensions;
// Weak descriptor of the real canonical Gui+10 signed-height cache entry.
// Constructed only after CPU/cache and the actual renderer atlas pair return.
// Holding this descriptor does not keep Gui, Font, CPU, cache or GPU alive.
class BnwLabelSystemFontBorrower final {
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class NotificationContextGuiPort;
 friend class BnwLabelSystemFontPreparedRun;
 friend class BnwLabelPaintTextLoan;
 friend class NotificationLayoutEmbeddedLabelState;
 friend class NotificationLayoutStandaloneLabelState;
 friend class NotificationLayoutRuntimeDimensions;
    friend class BnwPositionedLabelRunLoan;
    friend class BnwStandalonePositionedLabelRunLoan;
    friend class BnwNormalLabelPaintInvocation;
    friend class BnwLabelCurrentTextLoan;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::graphics::GlBnwSystemFontAtlas;
 struct Impl;const std::shared_ptr<const Impl> impl_;
 explicit BnwLabelSystemFontBorrower(std::shared_ptr<const Impl>);
 void require_current_without_callbacks()const;
 std::shared_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata> atlas_metadata_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 std::shared_ptr<const BnwLabelSystemFontPreparedRun> prepare_label_text_run_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const BnwLabelCurrentTextLoan&)const;
 bool has_glyph_owned(const runtime::BnwWorldViewSourceOperationLoan&,
  const BnwLabelCurrentTextLoan&,std::uint32_t)const;
 std::array<std::uint32_t,9> glyph_bits_owned(const runtime::BnwWorldViewSourceOperationLoan&,
  const BnwLabelCurrentTextLoan&,std::uint32_t)const;
 // Actual Paint text uses the independently retired Root Frame/Run seal.
 std::shared_ptr<const BnwLabelSystemFontPreparedRun> prepare_label_text_run_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const BnwLabelPaintTextLoan&)const;
 bool has_glyph_owned(const runtime::BnwWorldViewSourceOperationLoan&,
  const BnwLabelPaintTextLoan&,std::uint32_t)const;
 std::array<std::uint32_t,9> glyph_bits_owned(const runtime::BnwWorldViewSourceOperationLoan&,
  const BnwLabelPaintTextLoan&,std::uint32_t)const;
 // Source10 snapshots Paint once; successors never reread its raw facade.
 std::shared_ptr<const BnwLabelSystemFontPreparedRun> prepare_label_text_run_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const BnwLabelPaintTextLoan::Guard&)const;
 // Actual Atlas prepare+flush uses its closed bottom fence.
 std::shared_ptr<const BnwLabelSystemFontPreparedRun> prepare_label_text_run_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const BnwLabelPaintTextLoan::Guard&,
  const graphics::detail::BnwOwnedFontPaintEffectFence&)const;
 bool has_glyph_owned(const runtime::BnwWorldViewSourceOperationLoan&,
  const BnwLabelPaintTextLoan::Guard&,std::uint32_t)const;
 std::array<std::uint32_t,9> glyph_bits_owned(const runtime::BnwWorldViewSourceOperationLoan&,
  const BnwLabelPaintTextLoan::Guard&,std::uint32_t)const;
 // Genuine Atlas lexical fence covers both prefix and later native GetGlyph.
 BnwSystemFontNativeGlyphData glyph_data_owned(const runtime::BnwWorldViewSourceOperationLoan&,
  const BnwLabelPaintTextLoan::Guard&,std::uint32_t,
  const graphics::detail::BnwOwnedFontPaintEffectFence&)const;
public:
 ~BnwLabelSystemFontBorrower();
 BnwLabelSystemFontBorrower(const BnwLabelSystemFontBorrower&)=delete;
 BnwLabelSystemFontBorrower&operator=(const BnwLabelSystemFontBorrower&)=delete;
};
}
