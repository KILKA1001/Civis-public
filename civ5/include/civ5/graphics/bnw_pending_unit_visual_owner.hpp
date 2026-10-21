#pragma once
#include <memory>
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include <array>
#include <cstdint>

namespace civ5::materials { class MaterialWorld; }
namespace civ5::validation { struct PreviewPresenter; }
namespace civ5::runtime { class BnwNativeUnitAllocatorCore; class BnwUnitMovePending; }
namespace civ5::ui { class BnwClientSceneFrame; struct BnwClientFrame; }
namespace civ5::graphics {
class BnwPendingUnitVisualOwner;
// Issued only after the SAME renderer slot's actual false store/bookends.
// A retained proof is not a live receiver, Core successor or roster grant.
class BnwPendingUnitHiddenSlotProof final {
public:
    BnwPendingUnitHiddenSlotProof(const BnwPendingUnitHiddenSlotProof&)=delete;
    BnwPendingUnitHiddenSlotProof& operator=(const BnwPendingUnitHiddenSlotProof&)=delete;
private:
    friend class BnwPendingUnitVisualOwner;
    friend class runtime::BnwNativeUnitAllocatorCore;
    BnwPendingUnitHiddenSlotProof(std::weak_ptr<BnwPendingUnitVisualOwner> owner,
        const void* slot,std::weak_ptr<const void> node,
        std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore> core,
        std::weak_ptr<const runtime::BnwUnitMovePending> pending);
    const std::weak_ptr<BnwPendingUnitVisualOwner> owner_;
    const void* const slot_;
    const std::weak_ptr<const void> node_;
    const std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore> core_;
    const std::weak_ptr<const runtime::BnwUnitMovePending> pending_;
};

// Issued only after a genuine renderer-world transform has been stored in
// the SAME hidden slot. No plot-coordinate intent or public placement ACK.
class BnwPendingUnitPositionProof final {
public:
    BnwPendingUnitPositionProof(const BnwPendingUnitPositionProof&)=delete;
    BnwPendingUnitPositionProof& operator=(const BnwPendingUnitPositionProof&)=delete;
private:
    friend class BnwPendingUnitVisualOwner;
    friend class runtime::BnwNativeUnitAllocatorCore;
    BnwPendingUnitPositionProof(std::weak_ptr<BnwPendingUnitVisualOwner>,const void*,
        std::weak_ptr<const void>,std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore>,
        std::weak_ptr<const runtime::BnwUnitMovePending>,
        std::weak_ptr<const BnwPendingUnitHiddenSlotProof>,
        std::weak_ptr<const materials::MaterialWorld>,const void*,const void*,
        std::array<float,3>,std::int32_t,std::int32_t,std::uint64_t);
    const std::weak_ptr<BnwPendingUnitVisualOwner> owner_;
    const void* const slot_;
    const std::weak_ptr<const void> node_;
    const std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore> core_;
    const std::weak_ptr<const runtime::BnwUnitMovePending> pending_;
    const std::weak_ptr<const BnwPendingUnitHiddenSlotProof> hidden_;
    const std::weak_ptr<const materials::MaterialWorld> world_;
    const void* const layout_;
    const void* const height_;
    const std::array<float,3> world_position_;
    const std::int32_t map_x_,map_y_;
    const std::uint64_t revision_;
};

// The renderer's unpublished-node slots. No UnitID import, roster insertion,
// model admission or visible transition is provided by this bounded receiver.
class BnwSelectedPendingUnitVisualLoan;
class BnwInitialCenterUnitRendererLoan;
class BnwInitialCenterUnitVisibilityProof;
class BnwPendingUnitVisualOwner final {
public:
    ~BnwPendingUnitVisualOwner();
    BnwPendingUnitVisualOwner(const BnwPendingUnitVisualOwner&)=delete;
    BnwPendingUnitVisualOwner& operator=(const BnwPendingUnitVisualOwner&)=delete;
    static std::shared_ptr<BnwPendingUnitVisualOwner> attach_to_selected_scene(
        const std::shared_ptr<ui::BnwClientSceneFrame>&);
    void require_current() const;
    // Actual paint consumes the SAME slot written by the Core-only receiver.
    // Hidden unpublished slots are excluded; this never imports a saved unit.
    void draw_pending_slots(const std::shared_ptr<const ui::BnwClientFrame>&) const;
private:
    friend class runtime::BnwNativeUnitAllocatorCore;
    friend struct ::civ5::validation::PreviewPresenter;
    // PRIVATE actual paint owner only. The full producer implementation must
    // attest same Scene/Host/render world/lifetime before this attachment exists.
    void bind_actual_camera_terrain(const std::shared_ptr<const materials::MaterialWorld>&,
                                    const std::weak_ptr<const void>& actual_paint_lifetime) const;
    std::shared_ptr<const BnwPendingUnitPositionProof> apply_initial_set_position(
        const runtime::BnwNativeUnitAllocatorCore&,const runtime::BnwUnitMovePending&,
        const BnwPendingUnitHiddenSlotProof&) const;
    void require_initial_set_position_current(
        const runtime::BnwNativeUnitAllocatorCore::InitialVisibilityBinding&,
        const BnwPendingUnitHiddenSlotProof&,const BnwPendingUnitPositionProof&) const;
    void require_initial_set_position_identity_without_callbacks(
        const runtime::BnwNativeUnitAllocatorCore::InitialVisibilityBinding&,
        const BnwPendingUnitHiddenSlotProof&,const BnwPendingUnitPositionProof&) const;
    std::shared_ptr<BnwPendingUnitVisualOwner> retain_factory_owner() const;
    static std::shared_ptr<const BnwPendingUnitHiddenSlotProof> apply_initial_visibility_false_to_selected_scene(
        const runtime::BnwNativeUnitAllocatorCore&,const runtime::BnwUnitMovePending&,
        const std::shared_ptr<ui::BnwClientSceneFrame>&);
    std::shared_ptr<const BnwPendingUnitHiddenSlotProof> apply_initial_visibility_false(const runtime::BnwNativeUnitAllocatorCore&,
                                        const runtime::BnwUnitMovePending&) const;
    friend class BnwSelectedPendingUnitVisualLoan;
    friend class BnwInitialCenterUnitRendererLoan;
    friend class BnwInitialCenterUnitVisibilityProof;
    void require_center_hidden_node(const runtime::BnwNativeUnitAllocatorCore&,
        const void*,const std::weak_ptr<const void>&) const;
    std::uint64_t store_center_hidden_false(const runtime::BnwNativeUnitAllocatorCore&,
        const void*,const std::weak_ptr<const void>&) const;
    void require_center_hidden_store(const runtime::BnwNativeUnitAllocatorCore&,
        const void*,const std::weak_ptr<const void>&,std::uint64_t) const;
    std::shared_ptr<const BnwPendingUnitHiddenSlotProof> require_action_hidden_slot_proof(
        const runtime::BnwNativeUnitAllocatorCore&,const runtime::BnwUnitMovePending&) const;
    void require_action_hidden_slot_proof_current(const BnwPendingUnitHiddenSlotProof&) const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwPendingUnitVisualOwner(std::unique_ptr<Impl>);
};
}
