#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics::detail {class BnwFontTextureBorrower;}
namespace civ5::ui {class UiScreenGlRenderer;class BnwWorldViewControlTree;}
namespace civ5::ui::detail {
class BnwPositionedLabelRunLoan;
class NotificationContextGuiPort;
class BnwLabelSystemFontSource;
class NotificationLayoutEmbeddedLabelState;
class NotificationLayoutStandaloneLabelState;
class NotificationLayoutRuntimeDimensions;
// Actual registry-owned Font return. Data getters do not issue a paint scope,
// text run, rendering command, native layout cache or query renewal.
class BnwCompletedFontLoan final {
 friend class NotificationContextGuiPort;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class BnwLabelSystemFontSource;
 friend class NotificationLayoutEmbeddedLabelState;
 friend class NotificationLayoutStandaloneLabelState;
 friend class NotificationLayoutRuntimeDimensions;
    friend class BnwPositionedLabelRunLoan;
    friend class BnwStandalonePositionedLabelRunLoan;
    friend class BnwNormalLabelPaintInvocation;
    friend class BnwLabelCurrentTextLoan;
 friend class ::civ5::ui::UiScreenGlRenderer;
 struct Impl;const std::shared_ptr<const Impl> impl_;
 explicit BnwCompletedFontLoan(std::shared_ptr<const Impl>);
 void require_current_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 std::shared_ptr<const graphics::detail::BnwFontTextureBorrower>
     texture_borrower_without_callbacks(std::uint32_t actual_texture_index)const;
 std::optional<std::array<std::uint32_t,9>> glyph_bits_without_callbacks(std::uint32_t actual_codepoint)const;
 std::array<std::int32_t,3> metrics_without_callbacks()const;
 // Native6F81B0 lookup DATA. Caller alone truncates AL and earns Label6A2EA0.
 std::uint32_t style_ordinal_without_callbacks(std::string_view actual_style_c_string)const;
 std::array<std::uint32_t,3> style_layer_texture_indices_without_callbacks(std::uint32_t actual_style_ordinal)const;
 struct TextureDrawData final {
  const std::shared_ptr<const graphics::detail::BnwFontTextureBorrower> active,base;
  const bool allow_color;const std::uint32_t bc_bits;
 };
 TextureDrawData texture_draw_data_without_callbacks(std::uint32_t actual_texture_ordinal)const;
 // DATA only. Genuine run checks Font/source BEFORE borrower access and POST
 // after lexical paint callback; renderer supplies actual WorldViewPaintScope.
public:
 ~BnwCompletedFontLoan();
 BnwCompletedFontLoan(const BnwCompletedFontLoan&)=delete;
 BnwCompletedFontLoan&operator=(const BnwCompletedFontLoan&)=delete;
};
}
