#pragma once
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <thread>
#include <vector>
#include "civ5/ui/detail/bnw_label_actual_paint_call_frame.hpp"
#include "civ5/ui/detail/bnw_normal_root_batch_history.hpp"
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics {class GlBnwNonFontPipeline;class GlBnwSystemFontAtlas;class GlBnwFontShader;}
namespace civ5::ui {class UiScreenGlRenderer;class BnwWorldViewControlTree;class BnwWorldViewControlRenderPublication;struct ComposedUiFrame;}
namespace civ5::ui::detail {
class NotificationContextGuiPort;
class BnwNormalGuiBatchPrefix;
class BnwNormalRootContextEntry;
class BnwNormalRootBatchExecution;
class BnwNormalNonFontPaintInvocation;
class BnwNormalNonFontCurrentReceiverGuard;
class BnwNormalNonFontResourceBank;
class BnwNormalNonFontRecordLoan;
class BnwNormalNonFontSlotLoan;
class BnwNormalNonFontItemOccurrence;
class BnwNormalNonFontMaterialLoan;
class BnwNormalNonFontNativePool;
class BnwNormalNonFontNativeSlots;
class BnwNormalNonFontResourceLoan;
class BnwNormalNonFontDrainPermission;
class BnwNormalNonFontDrawCompletion;
class BnwNormalNonFontOwnReturn;
class BnwNormalNonFontButtonProducer;
class BnwLabelPaintTextLoan;class BnwPositionedLabelRunLoan;
class BnwStandalonePositionedLabelRunLoan;class BnwNormalLabelPaintEmission;
class BnwNormalLabelMaterialLoan;class BnwNormalItemOccurrence;
class BnwLabelSystemFontBorrower;class BnwLabelSystemFontPreparedRun;
// Frozen declarations only. Canonical current storage/receiver issuers and
// historical selected target/codec are external; no metadata/default issuer.
class BnwNormalRootPaintCallFrame final {
 friend class BnwNormalNonFontCurrentReceiverGuard;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class ::civ5::ui::BnwWorldViewControlRenderPublication;
 friend class BnwPositionedLabelRunLoan;
 friend class BnwStandalonePositionedLabelRunLoan;
 friend class BnwNormalLabelPaintEmission;
 friend class BnwNormalGuiBatchPrefix;
 friend class BnwNormalLabelMaterialLoan;
 friend class NotificationContextGuiPort;
 friend class BnwNormalRootBatchHistory;
 friend class BnwNormalItemOccurrence;
 friend class BnwLabelPaintTextLoan;
 friend class BnwLabelSystemFontBorrower;
 friend class BnwLabelSystemFontPreparedRun;
 friend class ::civ5::graphics::GlBnwSystemFontAtlas;
 friend class BnwNormalNonFontPaintInvocation;
 friend class BnwNormalNonFontItemOccurrence;
 friend class BnwNormalNonFontMaterialLoan;
 friend class BnwNormalNonFontButtonProducer;
 friend class BnwNormalNonFontRecordLoan;
 friend class BnwNormalNonFontSlotLoan;
 friend class BnwNormalNonFontDrainPermission;
 friend class ::civ5::graphics::GlBnwNonFontPipeline;
 friend class ::civ5::graphics::GlBnwFontShader;
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 // Other frame consumers may name this key, but only the renderer can mint it.
 class IssuerKey final {
  friend class ::civ5::ui::UiScreenGlRenderer;
  IssuerKey()=default;
  IssuerKey(const IssuerKey&)=delete;
 };
 explicit BnwNormalRootPaintCallFrame(const IssuerKey&,std::shared_ptr<Impl>);
 class Guard;
 //A separate capability extracted under the original ACTIVE Frame's FULL.
 //Later checks use actual root life/scope/Source and OWN frame storage. They
 //never call old Frame::first or require another current item/publication.
 class RootScopeHealthGuard final {
  friend class Guard;
  friend class BnwNormalLabelChildScopeHealthBridge;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  friend class NotificationContextGuiPort;
  friend class BnwNormalRootBatchHistory;
  friend class BnwNormalItemOccurrence;
  friend class BnwNormalLabelMaterialLoan;
  friend class BnwNormalGuiBatchPrefix;
  friend class BnwNormalNonFontPaintInvocation;
 friend class BnwNormalNonFontItemOccurrence;
 friend class BnwNormalNonFontMaterialLoan;
 friend class BnwNormalNonFontButtonProducer;
 friend class BnwNormalNonFontRecordLoan;
 friend class BnwNormalNonFontSlotLoan;
 friend class BnwNormalNonFontDrainPermission;
 friend class ::civ5::graphics::GlBnwNonFontPipeline;
 friend class ::civ5::graphics::GlBnwFontShader;
  friend class ::civ5::ui::UiScreenGlRenderer;
  class CaptureKey final {
   friend class Guard;
   CaptureKey()=default;
   CaptureKey(const CaptureKey&)=delete;
  };
  struct Impl;
  const std::shared_ptr<const Impl> impl_;
  explicit RootScopeHealthGuard(std::shared_ptr<const Impl>,const CaptureKey&);
  void require_physical_current_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_same_root_scope_without_callbacks(const RootScopeHealthGuard&)const;
  void require_actual_label_child_scope_without_callbacks(
      const BnwLabelActualPaintCallFrame::RootScopeHealthGuard&,const BnwNormalRootContextEntry&)const;
  void require_actual_renderer_without_callbacks(const ::civ5::ui::UiScreenGlRenderer&)const;
  //Publication is an immutable association only here, with no raw dereference.
  //The later Root/Item holder must retain SAME original Publication separately.
  void require_actual_publication_without_callbacks(const ::civ5::ui::BnwWorldViewControlRenderPublication&)const;
  void require_actual_frame_owner_without_callbacks(const std::shared_ptr<const ::civ5::ui::ComposedUiFrame>&)const;
  std::array<std::uint32_t,2> numeric_controls_without_callbacks()const;
 public:
  RootScopeHealthGuard(const RootScopeHealthGuard&)=default;
  RootScopeHealthGuard&operator=(const RootScopeHealthGuard&)=delete;
  ~RootScopeHealthGuard()=default;
 };
 class Guard final {
  friend class BnwNormalRootPaintCallFrame;
  friend class BnwNormalNonFontCurrentReceiverGuard;
  friend class ::civ5::ui::BnwWorldViewControlTree;
  friend class ::civ5::ui::BnwWorldViewControlRenderPublication;
  friend class BnwPositionedLabelRunLoan;
  friend class BnwStandalonePositionedLabelRunLoan;
  friend class BnwNormalLabelPaintEmission;
  friend class BnwNormalGuiBatchPrefix;
  friend class BnwNormalLabelMaterialLoan;
  friend class NotificationContextGuiPort;
  friend class BnwNormalRootBatchHistory;
  friend class BnwNormalItemOccurrence;
  friend class BnwLabelPaintTextLoan;
  friend class BnwLabelSystemFontBorrower;
  friend class BnwLabelSystemFontPreparedRun;
  friend class ::civ5::ui::UiScreenGlRenderer;
  friend class ::civ5::graphics::GlBnwSystemFontAtlas;
  friend class BnwNormalNonFontPaintInvocation;
 friend class BnwNormalNonFontItemOccurrence;
 friend class BnwNormalNonFontMaterialLoan;
 friend class BnwNormalNonFontButtonProducer;
 friend class BnwNormalNonFontRecordLoan;
 friend class BnwNormalNonFontSlotLoan;
 friend class BnwNormalNonFontDrainPermission;
 friend class ::civ5::graphics::GlBnwNonFontPipeline;
 friend class ::civ5::graphics::GlBnwFontShader;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_current_without_callbacks()const;
  void require_actual_paint_item_without_callbacks(const ::civ5::ui::BnwWorldViewControlRenderPublication&,std::size_t)const;
  void require_actual_context_entry_without_callbacks(const ::civ5::ui::BnwWorldViewControlRenderPublication&,const BnwNormalRootContextEntry&)const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_actual_control_without_callbacks(const ::civ5::ui::BnwWorldViewControlTree&,std::uint64_t)const;
  void require_actual_renderer_without_callbacks(const ::civ5::ui::UiScreenGlRenderer&)const;
  RootScopeHealthGuard capture_root_scope_health_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_actual_publication_without_callbacks(const ::civ5::ui::BnwWorldViewControlRenderPublication&)const;
  void require_actual_frame_owner_without_callbacks(const std::shared_ptr<const ::civ5::ui::ComposedUiFrame>&)const;
  // Real raw x87 CW and MXCSR captured at this renderer entry, not caller DATA.
  std::array<std::uint32_t,2> actual_numeric_profile_without_callbacks()const;
 public:
  Guard(const Guard&)=default;
  Guard&operator=(const Guard&)=delete;
  ~Guard()=default;
 };
 Guard capture_guard_without_callbacks()const;
public:
 ~BnwNormalRootPaintCallFrame(); // Retires lexical authority FIRST.
 BnwNormalRootPaintCallFrame(const BnwNormalRootPaintCallFrame&)=delete;
 BnwNormalRootPaintCallFrame&operator=(const BnwNormalRootPaintCallFrame&)=delete;
};

// A physical mapping is supplied only by the canonical keyed Root bridge.
// native_base is an earned 32-bit address, never a truncated host pointer.
class BnwNormalNonFontCaptureKey final {
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class NotificationContextGuiPort;
 BnwNormalNonFontCaptureKey()=default;
 BnwNormalNonFontCaptureKey(const BnwNormalNonFontCaptureKey&)=delete;
};
struct BnwNormalNonFontNativeRegion final {
 std::shared_ptr<void> owner;
 std::byte* bytes;
 std::size_t extent;
 std::uint32_t native_base;
};
class BnwNormalNonFontNativePool final {
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class NotificationContextGuiPort;
 friend class BnwNormalNonFontPaintInvocation;
 friend class BnwNormalNonFontRecordLoan;
 friend class BnwNormalNonFontItemOccurrence;
 friend class BnwNormalNonFontButtonProducer;
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 struct CapturedFields final {
  BnwNormalNonFontNativeRegion owner,payload,queue,cleanup_owner;
  BnwNormalRootBatchHistory::Guard root;
  const runtime::BnwWorldViewSourceOperationLoan* operation;
  std::uint64_t serial;
 };
 static std::shared_ptr<const BnwNormalNonFontNativePool> capture_owned(
  const BnwNormalNonFontCaptureKey&,const runtime::BnwWorldViewSourceOperationLoan&,CapturedFields);
 explicit BnwNormalNonFontNativePool(std::shared_ptr<Impl>);
 void require_physical_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 void require_record_available_without_callbacks()const;
 BnwNormalNonFontNativeRegion owner_region_without_callbacks()const;
 BnwNormalNonFontNativeRegion payload_region_without_callbacks()const;
 BnwNormalNonFontNativeRegion queue_region_without_callbacks()const;
 BnwNormalNonFontNativeRegion cleanup_owner_region_without_callbacks()const;
 void cleanup_native_8eefe0_without_callbacks()const;
public:
 ~BnwNormalNonFontNativePool();
 BnwNormalNonFontNativePool(const BnwNormalNonFontNativePool&)=delete;
 BnwNormalNonFontNativePool&operator=(const BnwNormalNonFontNativePool&)=delete;
};
class BnwNormalNonFontNativeSlots final {
 friend class BnwNormalNonFontRecordLoan;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class NotificationContextGuiPort;
 friend class BnwNormalNonFontPaintInvocation;
 friend class BnwNormalNonFontSlotLoan;
 friend class BnwNormalNonFontItemOccurrence;
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 struct CapturedFields final {
  BnwNormalNonFontNativeRegion manager,descriptors;
  std::vector<BnwNormalNonFontNativeRegion> mapped_regions;
  BnwNormalRootBatchHistory::Guard root;
  const runtime::BnwWorldViewSourceOperationLoan* operation;
  std::uint64_t serial;
 };
 static std::shared_ptr<const BnwNormalNonFontNativeSlots> capture_owned(
  const BnwNormalNonFontCaptureKey&,const runtime::BnwWorldViewSourceOperationLoan&,CapturedFields);
 explicit BnwNormalNonFontNativeSlots(std::shared_ptr<Impl>);
 void require_physical_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 void require_slot_and_release_available_without_callbacks()const;
 BnwNormalNonFontNativeRegion manager_region_without_callbacks()const;
 BnwNormalNonFontNativeRegion descriptor_region_without_callbacks()const;
 // Resolves only a retained original pointer range; no bucket allocation.
 BnwNormalNonFontNativeRegion actual_bucket_region_without_callbacks(std::uint32_t,std::size_t)const;
public:
 ~BnwNormalNonFontNativeSlots();
 BnwNormalNonFontNativeSlots(const BnwNormalNonFontNativeSlots&)=delete;
 BnwNormalNonFontNativeSlots&operator=(const BnwNormalNonFontNativeSlots&)=delete;
};
class BnwNormalNonFontResourceLoan final {
 friend class BnwNormalNonFontResourceBank;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class NotificationContextGuiPort;
 friend class BnwNormalNonFontPaintInvocation;
 friend class BnwNormalNonFontButtonProducer;
 friend class BnwNormalNonFontItemOccurrence;
 friend class ::civ5::graphics::GlBnwNonFontPipeline;
 struct Impl;
 const std::shared_ptr<const Impl> impl_;
 struct CapturedFields final {
  std::shared_ptr<const void> canonical_resource_owner,gpu_storage_owner;
  BnwNormalRootBatchHistory::Guard root;
  const runtime::BnwWorldViewSourceOperationLoan* operation;
  std::uint64_t serial;
  std::uint32_t resource_id;
  std::array<std::uint32_t,2> dimensions;
  std::uint32_t mask_bit_count;
 };
 static std::shared_ptr<const BnwNormalNonFontResourceLoan> capture_owned(
  const BnwNormalNonFontCaptureKey&,const runtime::BnwWorldViewSourceOperationLoan&,CapturedFields);
 explicit BnwNormalNonFontResourceLoan(std::shared_ptr<const Impl>);
 void require_physical_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 void require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard&)const;
 void require_actual_resource_owner_without_callbacks(const std::shared_ptr<const void>&)const;
 std::uint32_t resource_id_without_callbacks()const;
 std::array<std::uint32_t,2> dimension_words_without_callbacks()const;
 std::uint32_t mask_bit_count_without_callbacks()const;
public:
 ~BnwNormalNonFontResourceLoan();
 BnwNormalNonFontResourceLoan(const BnwNormalNonFontResourceLoan&)=delete;
 BnwNormalNonFontResourceLoan&operator=(const BnwNormalNonFontResourceLoan&)=delete;
};

// Canonical current receiver issuance is ROOT OWNED. Graph has no constructor
// body for this guard. Root must earn exact subtype/receiver/parent/ordinal/
// native backing/arguments on the genuine dispatcher; no XML or tag issuer.
class BnwNormalNonFontCurrentReceiverGuard final {
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class NotificationContextGuiPort;
 friend class BnwNormalNonFontPaintInvocation;
 friend class BnwNormalNonFontButtonProducer;
 struct Impl;
 const std::shared_ptr<const Impl> impl_;
 explicit BnwNormalNonFontCurrentReceiverGuard(std::shared_ptr<const Impl>,const BnwNormalNonFontCaptureKey&);
 void require_current_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 void require_same_receiver_without_callbacks(const BnwNormalNonFontCurrentReceiverGuard&)const;
 void require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard&)const;
 void require_actual_publication_without_callbacks(const ::civ5::ui::BnwWorldViewControlRenderPublication&)const;
 void require_actual_frame_without_callbacks(const BnwNormalRootPaintCallFrame::Guard&)const;
 void require_actual_renderer_without_callbacks(const ::civ5::ui::UiScreenGlRenderer&)const;
 std::uint64_t actual_control_without_callbacks()const;
 std::size_t actual_ordered_ordinal_without_callbacks()const;
 // Actual own118 receiver backing; original 44/48 pointers, source backing,
 // five captured arguments and both global fixed-slot words are Root supplied.
 BnwNormalNonFontNativeRegion receiver_region_without_callbacks()const;
 std::optional<BnwNormalNonFontNativeRegion> source_region_without_callbacks(std::size_t)const;
 std::uint32_t flags_word40_without_callbacks()const; // OUTER Button flags, distinct own118 WORD40
 std::array<std::uint32_t,5> argument_words_without_callbacks()const;
 std::array<std::uint32_t,2> fixed_slot_global_words_without_callbacks()const;
 std::uint32_t material_selector_without_callbacks()const;
 // Exactly two ORIGINAL F32 memory operands: [9EB1E0 SUBSS,9DD390 FADD m32real].
 // Root checks each sealed actual literal association; Graph never guesses value.
 std::array<std::uint32_t,2> arithmetic_literal_words_without_callbacks()const;
public:
 BnwNormalNonFontCurrentReceiverGuard(const BnwNormalNonFontCurrentReceiverGuard&)=default;
 BnwNormalNonFontCurrentReceiverGuard&operator=(const BnwNormalNonFontCurrentReceiverGuard&)=delete;
 ~BnwNormalNonFontCurrentReceiverGuard()=default;
};
class BnwNormalNonFontResourceBank final {
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class NotificationContextGuiPort;
 friend class BnwNormalNonFontPaintInvocation;
 friend class BnwNormalNonFontButtonProducer;
 struct Impl;
 const std::shared_ptr<const Impl> impl_;
 explicit BnwNormalNonFontResourceBank(std::shared_ptr<const Impl>,const BnwNormalNonFontCaptureKey&);
 void require_physical_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 void require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard&)const;
 // Root actual8E1990 supplier: nullptr ONLY actual unsignedID>=liveCount.
 // A missing/unqualified supplier throws; nativeID0 remains valid.
 std::shared_ptr<const BnwNormalNonFontResourceLoan> resolve_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,std::uint32_t)const;
public:
 ~BnwNormalNonFontResourceBank();
 BnwNormalNonFontResourceBank(const BnwNormalNonFontResourceBank&)=delete;
 BnwNormalNonFontResourceBank&operator=(const BnwNormalNonFontResourceBank&)=delete;
};

class BnwNormalNonFontPaintInvocation final {
 friend class BnwNormalNonFontOwnReturn;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class NotificationContextGuiPort;
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class BnwNormalNonFontButtonProducer;
 friend class BnwNormalNonFontRecordLoan;
 friend class BnwNormalNonFontSlotLoan;
 friend class BnwNormalNonFontItemOccurrence;
 friend class BnwNormalNonFontMaterialLoan;
 struct Impl;
 const std::shared_ptr<const Impl> impl_;
 struct CapturedFields final {
  BnwNormalNonFontCurrentReceiverGuard receiver;
  BnwNormalRootPaintCallFrame::Guard frame;
  BnwNormalRootBatchHistory::Guard root;
  std::shared_ptr<const ::civ5::ui::BnwWorldViewControlRenderPublication> publication;
  std::shared_ptr<const BnwWindowedRendererMatrixDataLoan> matrix;
  std::shared_ptr<const BnwNormalNonFontNativePool> pool;
  std::shared_ptr<const BnwNormalNonFontNativeSlots> slots;
  std::shared_ptr<const BnwNormalNonFontResourceBank> resources;
 };
 static BnwNormalNonFontPaintInvocation capture_owned(const BnwNormalNonFontCaptureKey&,
  const runtime::BnwWorldViewSourceOperationLoan&,CapturedFields);
 explicit BnwNormalNonFontPaintInvocation(std::shared_ptr<const Impl>);
 void require_current_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 void require_same_invocation_without_callbacks(const BnwNormalNonFontPaintInvocation&)const;
 void require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard&)const;
 void require_actual_publication_without_callbacks(const ::civ5::ui::BnwWorldViewControlRenderPublication&)const;
 void require_actual_renderer_without_callbacks(const ::civ5::ui::UiScreenGlRenderer&)const;
 void require_actual_frame_without_callbacks(const BnwNormalRootPaintCallFrame::Guard&)const;
 std::uint64_t actual_control_without_callbacks()const;
 std::size_t actual_ordered_ordinal_without_callbacks()const;
 std::uint32_t flags_word40_without_callbacks()const;
 BnwNormalRootBatchHistory::Guard captured_root_without_callbacks()const;
 std::shared_ptr<const BnwWindowedRendererMatrixDataLoan> actual_matrix_owned_without_callbacks()const;
 std::shared_ptr<const BnwNormalNonFontNativePool> native_pool_owned_without_callbacks()const;
 std::shared_ptr<const BnwNormalNonFontNativeSlots> native_slots_owned_without_callbacks()const;
 std::shared_ptr<const BnwNormalNonFontResourceBank> resource_bank_owned_without_callbacks()const;
 BnwNormalNonFontCurrentReceiverGuard captured_receiver_without_callbacks()const;
 std::array<std::uint32_t,2> numeric_controls_without_callbacks()const;
 std::uint32_t material_selector_without_callbacks()const;
 std::uint16_t native_configuration_without_callbacks()const;
 // Includes authentic literal/numeric/resource suppliers; fails before effects.
 void require_button_suppliers_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
public:
 BnwNormalNonFontPaintInvocation(const BnwNormalNonFontPaintInvocation&)=default;
 BnwNormalNonFontPaintInvocation&operator=(const BnwNormalNonFontPaintInvocation&)=delete;
 ~BnwNormalNonFontPaintInvocation();
};
class BnwNormalNonFontRecordLoan final {
 friend class BnwNormalNonFontButtonProducer;
 friend class BnwNormalNonFontItemOccurrence;
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 explicit BnwNormalNonFontRecordLoan(std::shared_ptr<Impl>);
 static std::unique_ptr<BnwNormalNonFontRecordLoan> capture_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const BnwNormalNonFontPaintInvocation&);
 void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,const BnwNormalNonFontPaintInvocation&)const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,const BnwNormalNonFontPaintInvocation&)const;
 void write_word_without_callbacks(std::uint32_t,std::uint32_t);
 void write_word16_without_callbacks(std::uint32_t,std::uint16_t);
 void write_byte_without_callbacks(std::uint32_t,std::uint8_t);
 void seal_complete_without_callbacks(const BnwNormalNonFontPaintInvocation&);
 class Guard final {
  friend class BnwNormalNonFontRecordLoan;friend class BnwNormalNonFontItemOccurrence;
  friend class BnwNormalNonFontButtonProducer;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_physical_without_callbacks(const BnwNormalNonFontPaintInvocation&)const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,const BnwNormalNonFontPaintInvocation&)const;
 public: Guard(const Guard&)=default;Guard&operator=(const Guard&)=delete;
 };
 Guard capture_guard_without_callbacks()const;
public:
 ~BnwNormalNonFontRecordLoan();
 BnwNormalNonFontRecordLoan(const BnwNormalNonFontRecordLoan&)=delete;
 BnwNormalNonFontRecordLoan&operator=(const BnwNormalNonFontRecordLoan&)=delete;
};
class BnwNormalNonFontSlotLoan final {
 friend class BnwNormalNonFontButtonProducer;
 friend class BnwNormalNonFontItemOccurrence;
 struct Impl;
 const std::shared_ptr<Impl> impl_;
 explicit BnwNormalNonFontSlotLoan(std::shared_ptr<Impl>);
 static std::unique_ptr<BnwNormalNonFontSlotLoan> capture_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const BnwNormalNonFontPaintInvocation&);
 void require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&,const BnwNormalNonFontPaintInvocation&)const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,const BnwNormalNonFontPaintInvocation&)const;
 void write_word_without_callbacks(std::uint32_t,std::uint32_t);
 void write_word16_without_callbacks(std::uint32_t,std::uint16_t);
 void write_byte_without_callbacks(std::uint32_t,std::uint8_t);
 std::uint32_t index_without_callbacks()const;
 std::array<std::byte,0x64> descriptor_words_without_callbacks()const;
 void release_owned(const runtime::BnwWorldViewSourceOperationLoan&,const BnwNormalNonFontPaintInvocation&);
 class Guard final {
  friend class BnwNormalNonFontSlotLoan;friend class BnwNormalNonFontItemOccurrence;
  friend class BnwNormalNonFontButtonProducer;
  const std::shared_ptr<Impl> impl_;
  explicit Guard(std::shared_ptr<Impl>);
  void require_physical_without_callbacks(const BnwNormalNonFontPaintInvocation&)const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&,const BnwNormalNonFontPaintInvocation&)const;
 public: Guard(const Guard&)=default;Guard&operator=(const Guard&)=delete;
 };
 Guard capture_guard_without_callbacks()const;
public:
 ~BnwNormalNonFontSlotLoan();
 BnwNormalNonFontSlotLoan(const BnwNormalNonFontSlotLoan&)=delete;
 BnwNormalNonFontSlotLoan&operator=(const BnwNormalNonFontSlotLoan&)=delete;
};
class BnwNormalNonFontItemOccurrence final {
 friend class BnwNormalNonFontButtonProducer;
 friend class BnwNormalNonFontMaterialLoan;
 friend class BnwNormalNonFontDrainPermission;
 friend class BnwNormalNonFontOwnReturn;
 friend class ::civ5::ui::BnwWorldViewControlTree;
 friend class NotificationContextGuiPort;
 friend class ::civ5::graphics::GlBnwNonFontPipeline;
 struct Impl;
 const std::shared_ptr<const Impl> impl_;
 explicit BnwNormalNonFontItemOccurrence(std::shared_ptr<const Impl>);
 static std::shared_ptr<const BnwNormalNonFontItemOccurrence> register_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const BnwNormalNonFontPaintInvocation&,
  BnwNormalNonFontRecordLoan&,BnwNormalNonFontSlotLoan&,
  std::array<std::shared_ptr<const BnwNormalNonFontResourceLoan>,4>);
public:
 class Guard final {
  friend class BnwNormalNonFontItemOccurrence;friend class BnwNormalNonFontMaterialLoan;
  friend class BnwNormalNonFontOwnReturn;
  friend class BnwNormalNonFontDrainPermission;friend class BnwNormalNonFontButtonProducer;
  friend class ::civ5::ui::BnwWorldViewControlTree;friend class NotificationContextGuiPort;
  friend class ::civ5::ui::UiScreenGlRenderer;friend class ::civ5::graphics::GlBnwNonFontPipeline;
  const std::shared_ptr<const Impl> impl_;
  explicit Guard(std::shared_ptr<const Impl>);
  void require_physical_current_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_same_item_without_callbacks(const Guard&)const;
  void require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard&)const;
  std::array<std::byte,0xF0> payload_without_callbacks()const;
  std::bitset<0xF0> write_coverage_without_callbacks()const;
  std::array<std::byte,0x64> descriptor_without_callbacks()const;
  std::array<std::byte,0x20> registration_without_callbacks()const;
  std::uint32_t index_without_callbacks()const;
  std::array<std::shared_ptr<const BnwNormalNonFontResourceLoan>,4> resources_owned_without_callbacks()const;
 public: Guard(const Guard&)=default;Guard&operator=(const Guard&)=delete;~Guard()=default;
 };
 ~BnwNormalNonFontItemOccurrence();
 BnwNormalNonFontItemOccurrence(const BnwNormalNonFontItemOccurrence&)=delete;
 BnwNormalNonFontItemOccurrence&operator=(const BnwNormalNonFontItemOccurrence&)=delete;
private:
 Guard capture_guard_without_callbacks()const;
};
class BnwNormalNonFontMaterialLoan final {
 friend class BnwNormalNonFontButtonProducer;friend class BnwNormalNonFontOwnReturn;
 friend class ::civ5::ui::BnwWorldViewControlTree;friend class NotificationContextGuiPort;
 friend class ::civ5::ui::UiScreenGlRenderer;friend class ::civ5::graphics::GlBnwNonFontPipeline;
 struct Impl;
 const std::shared_ptr<const Impl> impl_;
 explicit BnwNormalNonFontMaterialLoan(std::shared_ptr<const Impl>);
 static std::shared_ptr<const BnwNormalNonFontMaterialLoan> capture_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const BnwNormalNonFontItemOccurrence::Guard&);
 void require_physical_current_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 void require_same_item_without_callbacks(const BnwNormalNonFontItemOccurrence::Guard&)const;
 BnwNormalNonFontItemOccurrence::Guard captured_item_without_callbacks()const;
public:
 ~BnwNormalNonFontMaterialLoan();
 BnwNormalNonFontMaterialLoan(const BnwNormalNonFontMaterialLoan&)=delete;
 BnwNormalNonFontMaterialLoan&operator=(const BnwNormalNonFontMaterialLoan&)=delete;
};
class BnwNormalNonFontOwnReturn final {
 friend class BnwNormalNonFontButtonProducer;
 friend class ::civ5::ui::BnwWorldViewControlTree;friend class NotificationContextGuiPort;
 friend class ::civ5::ui::UiScreenGlRenderer;
 enum class Outcome {registered,zero_index,missing_resource};
 const Outcome outcome_;
 const std::shared_ptr<const void> invocation_control_;
 const BnwNormalRootBatchHistory::Guard root_;
 const std::uint64_t control_;
 const std::size_t ordinal_;
 const std::array<std::uint32_t,2> numeric_;
 const std::thread::id creator_;
 const std::shared_ptr<const BnwNormalNonFontItemOccurrence> item_;
 const std::shared_ptr<const BnwNormalNonFontMaterialLoan> material_;
 void require_physical_current_without_callbacks()const;
 void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
 void require_same_invocation_control_without_callbacks(const BnwNormalNonFontPaintInvocation&)const;
 Outcome outcome_without_callbacks()const;
 std::uint64_t actual_control_without_callbacks()const;
 std::size_t actual_ordered_ordinal_without_callbacks()const;
 BnwNormalRootBatchHistory::Guard captured_root_without_callbacks()const;
 std::shared_ptr<const BnwNormalNonFontItemOccurrence> item_owned_without_callbacks()const;
 std::shared_ptr<const BnwNormalNonFontMaterialLoan> material_owned_without_callbacks()const;
 explicit BnwNormalNonFontOwnReturn(Outcome,const BnwNormalNonFontPaintInvocation&,
  std::shared_ptr<const BnwNormalNonFontItemOccurrence>,std::shared_ptr<const BnwNormalNonFontMaterialLoan>);
public:
 BnwNormalNonFontOwnReturn(const BnwNormalNonFontOwnReturn&)=default;
 BnwNormalNonFontOwnReturn&operator=(const BnwNormalNonFontOwnReturn&)=delete;
 ~BnwNormalNonFontOwnReturn()=default;
};
class BnwNormalNonFontDrainPermission final {
 friend class ::civ5::ui::BnwWorldViewControlTree;friend class NotificationContextGuiPort;
 struct Impl;
 BnwNormalNonFontDrainPermission()=delete;
public:
 class Guard final {
  friend class BnwNormalNonFontDrainPermission;
  friend class ::civ5::ui::BnwWorldViewControlTree;friend class NotificationContextGuiPort;
  friend class ::civ5::ui::UiScreenGlRenderer;friend class ::civ5::graphics::GlBnwNonFontPipeline;
  const std::shared_ptr<const Impl> impl_;
  explicit Guard(std::shared_ptr<const Impl>,const BnwNormalNonFontCaptureKey&);
  void require_physical_current_without_callbacks()const;
  void require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
  void require_same_item_without_callbacks(const BnwNormalNonFontItemOccurrence::Guard&)const;
  void require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard&)const;
 public: Guard(const Guard&)=default;Guard&operator=(const Guard&)=delete;~Guard()=default;
 };
};
class BnwNormalNonFontDrawCompletion final {
 friend class ::civ5::graphics::GlBnwNonFontPipeline;
 friend class ::civ5::ui::BnwWorldViewControlTree;friend class NotificationContextGuiPort;
 struct Impl;
 const std::shared_ptr<const Impl> impl_;
 class IssuerKey final {
  friend class ::civ5::graphics::GlBnwNonFontPipeline;
  IssuerKey()=default;IssuerKey(const IssuerKey&)=delete;
 };
 explicit BnwNormalNonFontDrawCompletion(std::shared_ptr<const Impl>,const IssuerKey&);
public:
 BnwNormalNonFontDrawCompletion(const BnwNormalNonFontDrawCompletion&)=default;
 BnwNormalNonFontDrawCompletion&operator=(const BnwNormalNonFontDrawCompletion&)=delete;
 ~BnwNormalNonFontDrawCompletion()=default;
};
class BnwNormalNonFontButtonProducer final {
 friend class ::civ5::ui::BnwWorldViewControlTree;friend class NotificationContextGuiPort;
 friend class ::civ5::ui::UiScreenGlRenderer;
 BnwNormalNonFontButtonProducer()=delete;
 static BnwNormalNonFontOwnReturn prepare_button_own118_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,const BnwNormalNonFontPaintInvocation&);
};
}
