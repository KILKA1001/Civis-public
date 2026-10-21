#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
namespace civ5::runtime {class BnwSelectedStockLuaSourceOwner;class BnwNativeUnitAllocatorCore;class BnwUnitMovePending;struct LiveSessionState;}
namespace civ5::graphics {class BnwTrainPresentedFrameProof;}
namespace civ5::ui {
class SelectedBnwPreviewHost;class SelectedCityTrainCreateSource;class BnwClientSceneFrame;class BnwOwnedCameraGuiScreen;
// Normal return of a genuine incomplete native TRAIN prefix. This is neither
// full Init/CreateUnit return nor a GUI-present/loading/gameplay receipt.
class BnwTrainRendererRefreshReceipt final {
 public:
    ~BnwTrainRendererRefreshReceipt();
    BnwTrainRendererRefreshReceipt(const BnwTrainRendererRefreshReceipt&)=delete;
    BnwTrainRendererRefreshReceipt& operator=(const BnwTrainRendererRefreshReceipt&)=delete;
    void require_pending_current()const;
 private:
    friend class runtime::BnwSelectedStockLuaSourceOwner;
    friend class graphics::BnwTrainPresentedFrameProof;
    friend class BnwOwnedCameraGuiScreen;
    friend class BnwClientSceneFrame;
    // Available ONLY to the concrete authentic graphics issuer. These retained
    // identities never grant presentation from a public snapshot/counter/bool.
    struct RendererOrigin {
        std::shared_ptr<const SelectedBnwPreviewHost> host;
        std::shared_ptr<BnwClientSceneFrame> scene;
        std::shared_ptr<const BnwOwnedCameraGuiScreen> opening_gui;
        std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore> core;
        std::shared_ptr<const runtime::BnwUnitMovePending> pending;
        std::shared_ptr<const runtime::LiveSessionState> state;
        std::uint64_t generation=0,operation_serial=0,earned_revision=0;
    };
    std::shared_ptr<const BnwTrainRendererRefreshReceipt> retain_factory_owner()const;
    RendererOrigin borrow_renderer_origin()const;
    // Closed graphics lender bookends AFTER true action completion. The
    // barrier remains held through target COPY/invoke/DTOR on both paths.
    void require_presented_scope_without_callbacks()const;
    void finish_presented_scope_after_target()const;
    void quarantine_presented_scope_without_callbacks()const noexcept;
    BnwTrainRendererRefreshReceipt(
        std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner>,
        std::shared_ptr<const SelectedCityTrainCreateSource>,
        std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore>,
        std::shared_ptr<const runtime::BnwUnitMovePending>,
        std::shared_ptr<BnwClientSceneFrame>,std::shared_ptr<const BnwOwnedCameraGuiScreen>,
        std::shared_ptr<const SelectedBnwPreviewHost>,std::shared_ptr<const runtime::LiveSessionState>,
        std::shared_ptr<const void> operation_control,const void* actual_vm,
        std::uint64_t generation,std::uint64_t serial,std::uint64_t revision);
    const std::thread::id creator_=std::this_thread::get_id();
    std::weak_ptr<const BnwTrainRendererRefreshReceipt> factory_self_;
    std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner> source_owner_;
    std::shared_ptr<const SelectedCityTrainCreateSource> train_source_;
    RendererOrigin origin_;
    std::shared_ptr<const void> operation_control_;
    const void* actual_vm_=nullptr;
    mutable std::atomic<bool> invalidated_{false};
    mutable bool presented_=false; // Only genuine SourceOwner after sealed proof may store.
};
}
