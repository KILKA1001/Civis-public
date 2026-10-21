#pragma once
#include <memory>
#include <cstdint>

namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::ui::detail {
class BnwLabelPaintPositionedData;
class BnwNormalLabelMaterialLoan;
class BnwNormalQueueDrainPermission;
class BnwWindowedRendererSharedDescriptorIdentity;
}
namespace civ5::graphics {class GlBnwFontShader;}
namespace civ5::graphics::detail {
// A genuine PORT draw-record association, not an original pool ID or GLuint.
// One original Ui-held facade owns the staged transport lease and plane facade.
class BnwNormalLabelDrawRecordBinding final {
 friend class ::civ5::graphics::GlBnwFontShader;
 friend class ::civ5::ui::UiScreenGlRenderer;
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 class IssuerKey final {
  friend class ::civ5::graphics::GlBnwFontShader;
  IssuerKey()=default;
  IssuerKey(const IssuerKey&)=delete;
 };
 explicit BnwNormalLabelDrawRecordBinding(std::shared_ptr<Impl>,const IssuerKey&);
public:
 class Guard;
 class WeakGuard;
 class RendererDescriptorReceipt;
 class OpaqueMaterialIdentity;
 class OpaqueRecordIdentity final {
  friend class Guard;
  friend class RendererDescriptorReceipt;
  friend class ::civ5::graphics::GlBnwFontShader;
  friend class ::civ5::ui::detail::BnwNormalLabelMaterialLoan;
  friend class ::civ5::ui::detail::BnwNormalQueueDrainPermission;
  const std::shared_ptr<Impl> impl_;
  const unsigned pass_;
  explicit OpaqueRecordIdentity(std::shared_ptr<Impl>,unsigned);
  void require_current_without_callbacks()const;
  void require_physical_record_without_callbacks()const;
  void require_same_binding_physical_without_callbacks(const Guard&)const;
  void require_same_binding_without_callbacks(const Guard&)const;
 public:
  OpaqueRecordIdentity(const OpaqueRecordIdentity&)=default;
 };
 class Guard final {
  friend class BnwNormalLabelDrawRecordBinding;
  friend class RendererDescriptorReceipt;
  friend class WeakGuard;
  friend class OpaqueRecordIdentity;
  friend class ::civ5::graphics::GlBnwFontShader;
  friend class ::civ5::ui::UiScreenGlRenderer;
  friend class ::civ5::ui::detail::BnwLabelPaintPositionedData;
  friend class ::civ5::ui::detail::BnwNormalLabelMaterialLoan;
  friend class ::civ5::ui::detail::BnwNormalQueueDrainPermission;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_current_without_callbacks()const;
  void require_physical_record_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  // Pure attachment-time facade association only. Later checks use saved DATA.
  void require_same_positioned_data_without_callbacks(const ::civ5::ui::detail::BnwLabelPaintPositionedData&)const;
  WeakGuard capture_weak_without_callbacks()const;
  OpaqueRecordIdentity originating_record_without_callbacks(unsigned)const;
  void attach_shared_descriptor_without_callbacks(const ::civ5::ui::detail::BnwWindowedRendererSharedDescriptorIdentity&)const;
  RendererDescriptorReceipt finalize_renderer_descriptor_without_callbacks(const OpaqueRecordIdentity&)const;
 public:
  Guard(const Guard&)=default;
 };
 class WeakGuard final {
  friend class Guard;
  friend class ::civ5::graphics::GlBnwFontShader;
  friend class ::civ5::ui::detail::BnwLabelPaintPositionedData;
  friend class ::civ5::ui::detail::BnwNormalLabelMaterialLoan;
  const std::weak_ptr<Impl> impl_;
  explicit WeakGuard(std::weak_ptr<Impl>);
  Guard lock_current_without_callbacks()const;
  void require_current_without_callbacks()const;
 public:
  WeakGuard(const WeakGuard&)=default;
 };
class OpaqueMaterialIdentity final {
 friend class RendererDescriptorReceipt;
 friend class ::civ5::graphics::GlBnwFontShader;
 friend class ::civ5::ui::detail::BnwNormalLabelMaterialLoan;
 const std::shared_ptr<Impl> impl_;
 explicit OpaqueMaterialIdentity(std::shared_ptr<Impl>);
 void require_current_without_callbacks()const;
 void require_same_descriptor_without_callbacks(const RendererDescriptorReceipt&)const;
public:
 OpaqueMaterialIdentity(const OpaqueMaterialIdentity&)=default;
};
class RendererDescriptorReceipt final {
 friend class Guard;
 friend class OpaqueMaterialIdentity;
 friend class ::civ5::graphics::GlBnwFontShader;
 friend class ::civ5::ui::detail::BnwNormalLabelMaterialLoan;
 friend class ::civ5::ui::detail::BnwNormalQueueDrainPermission;
 const std::shared_ptr<Impl> impl_;
 const unsigned pass_;
 const std::uint64_t write_occurrence_;
 explicit RendererDescriptorReceipt(std::shared_ptr<Impl>,unsigned,std::uint64_t)noexcept;
 void require_current_without_callbacks()const;
 void require_physical_record_without_callbacks()const;
 void require_same_binding_physical_without_callbacks(const Guard&)const;
 void require_same_record_physical_without_callbacks(const OpaqueRecordIdentity&)const;
 void require_same_binding_without_callbacks(const Guard&)const;
 void require_same_record_without_callbacks(const OpaqueRecordIdentity&)const;
 OpaqueMaterialIdentity material_identity_without_callbacks()const;
public:
 RendererDescriptorReceipt(const RendererDescriptorReceipt&)=default;
};
 ~BnwNormalLabelDrawRecordBinding();
 BnwNormalLabelDrawRecordBinding(const BnwNormalLabelDrawRecordBinding&)=delete;
 BnwNormalLabelDrawRecordBinding&operator=(const BnwNormalLabelDrawRecordBinding&)=delete;
private:
 Guard capture_guard_without_callbacks()const;
};
}
