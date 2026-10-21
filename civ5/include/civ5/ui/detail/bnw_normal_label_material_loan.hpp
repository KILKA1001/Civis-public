#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include "civ5/ui/detail/bnw_label_paint_positioned_data.hpp"
#include "civ5/ui/detail/bnw_normal_root_batch_history.hpp"
#include <optional>
#include "civ5/ui/detail/bnw_windowed_renderer_matrix_data_loan.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics {class GlBnwFontShader;}
namespace civ5::ui {class BnwWorldViewControlTree;class BnwWorldViewControlRenderPublication;class UiScreenGlRenderer;}
namespace civ5::ui::detail {
class NotificationContextGuiPort;
// Interface-only successor. There is NO issuer or implementation in this
// packet: normal batch prefix/Normal10/Matrix DATA cannot construct Material.
// Implementation requires real RootRead -> slot10 -> normal root18 execution,
// physical28/2C/44 and ordered38/78, completed selector1 record append, and
// SAME per-item Frame9/Publication/Tree/Control/Source/Paint/positioned DATA.
class BnwNormalLabelMaterialLoan final {
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class NotificationContextGuiPort;
 friend class BnwNormalRootBatchHistory;
 friend class BnwNormalQueueDrainPermission;
 friend class ::civ5::ui::UiScreenGlRenderer;
 // Closed bridge uses the existing Material friend boundary while Matrix is ACTIVE.
 static BnwWindowedRendererSharedDescriptorIdentity capture_live_root_shared_descriptor_without_callbacks(
  const BnwWindowedRendererMatrixDataLoan&);
 static void require_root_shared_descriptor_without_callbacks(const BnwWindowedRendererSharedDescriptorIdentity&);
 struct Impl;const std::shared_ptr<Impl> impl_;
 explicit BnwNormalLabelMaterialLoan(std::shared_ptr<Impl>);
 class ChildReturnConstructionKey final {
  friend class ::civ5::ui::BnwWorldViewControlTree;
  ChildReturnConstructionKey()=default;
  ChildReturnConstructionKey(const ChildReturnConstructionKey&)=delete;
 };
 explicit BnwNormalLabelMaterialLoan(std::shared_ptr<Impl>,const ChildReturnConstructionKey&);
public:
 class ShaderAccess final {
  friend class BnwNormalLabelMaterialLoan;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  friend class ::civ5::graphics::GlBnwFontShader;
  const std::shared_ptr<Impl> impl_;
  const std::optional<BnwNormalQueueDrainPermission::Guard> selected_;
  const std::optional<::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity> selected_record_;
  const std::optional<::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt> selected_descriptor_;
  explicit ShaderAccess(std::shared_ptr<Impl>);
  explicit ShaderAccess(std::shared_ptr<Impl>,const BnwNormalQueueDrainPermission::Guard&);
  void require_selected_queue_association_without_callbacks(const BnwNormalRootBatchHistory::Guard&,
   const BnwNormalItemOccurrence::Guard&,const BnwNormalQueueDrainPermission::Guard&,
   const ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity&,
   const ::civ5::graphics::detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt&)const;
  void require_current_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_actual_association_without_callbacks(
   const ::civ5::ui::UiScreenGlRenderer&,
   const ::civ5::ui::BnwWorldViewControlRenderPublication&,std::size_t,
   const ::civ5::ui::BnwWorldViewControlTree&,std::uint64_t,
   const BnwLabelPaintPositionedData::Guard&,
   const BnwWindowedRendererMatrixDataLoan::ShaderAccess&)const;
  std::array<std::uint32_t,2> ordered_translation_words_without_callbacks()const;
  std::array<std::uint32_t,2> depth_arg4_arg5_words_without_callbacks()const;
  std::uint32_t composed_alpha_word_without_callbacks()const;
  std::uint32_t allow_color_word_without_callbacks()const;
  std::uint32_t native_selector_without_callbacks()const;
  std::array<std::uint32_t,2> viewport_depth_words_without_callbacks()const;
  std::array<std::int32_t,2> viewport_dimensions_without_callbacks()const;
  std::array<std::uint32_t,2> numeric_controls_without_callbacks()const;
  std::uint64_t normal_batch_revision_without_callbacks()const;
 public:
  ~ShaderAccess()=default;
  ShaderAccess(const ShaderAccess&)=delete;
  ShaderAccess&operator=(const ShaderAccess&)=delete;
  ShaderAccess(ShaderAccess&&)=delete;
  ShaderAccess&operator=(ShaderAccess&&)=delete;
 };
private:
 // Ui final cut after Rows.finish callbacks; never expose Shader operands.
 void require_current_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 ShaderAccess capture_shader_access_without_callbacks()const;
public:
 ~BnwNormalLabelMaterialLoan(); // Future owner must retire authority FIRST.
 BnwNormalLabelMaterialLoan(const BnwNormalLabelMaterialLoan&)=delete;
 BnwNormalLabelMaterialLoan&operator=(const BnwNormalLabelMaterialLoan&)=delete;
};
}
