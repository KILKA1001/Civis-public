namespace civ5::assets {class CivisResourceBankOwner;}
namespace civ5::ui {class BnwCivilopediaNativeRuntimeOwner;}
#pragma once
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <thread>
namespace civ5::runtime {class BnwSelectedStockLuaSourceOwner;class BnwOwnedGameplayEventPumpLoan;class BnwStockLuaAttemptLease;class BnwSelectedStockLuaBootstrap;class BnwWorldViewSourceOperationLoan;struct LiveSessionState;class BnwResearchChoiceCommit;enum class BnwResearchChoiceDirty;}
namespace civ5::graphics {class BnwOwnedCameraRendererWindowLease;class BnwSelectedPendingUnitVisualLoan;class BnwInitialCenterUnitRendererLoan;class BnwInitialUnitShouldDimRendererLoan;class BnwTrainPresentedFrameProof;}
namespace civ5::graphics {class BnwUnitDamageCommandOwner;class BnwInitialUnitSetupGraphicalRendererLoan;class BnwUnitSetupGraphicsState;}
namespace civ5::ui {
class UiScreenCompositor;class UiScreenGlRenderer;class BnwAlphaProgressionBackend;class LiveUiHost;class SelectedBnwPreviewHost;class BnwTrainRendererRefreshReceipt;
class BnwClientSceneFrame;struct BnwClientFrame;struct LiveUiFrame;struct ComposedUiFrame;
class BnwNotificationPublicationLender;
// Civis compositor publication, not an Original GUI-global/registry receipt.
// Neither physical HWND extent nor copied public frame data can construct it.
class BnwOwnedCameraGuiScreen final {
 public:
    class Loan final {
        friend class BnwOwnedCameraGuiScreen;
        friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
        friend class UiScreenGlRenderer;
        const BnwOwnedCameraGuiScreen& owner_;
        explicit Loan(const BnwOwnedCameraGuiScreen& owner):owner_(owner){}
      public:
        Loan(const Loan&)=delete;Loan& operator=(const Loan&)=delete;
        void require_current()const;
        std::int32_t width()const;
        std::int32_t height()const;
        std::uint64_t frame_revision()const;
    };
    // Event storage/currentness only: no frame, viewport, projection or Grid.
    class UnitFlagLifecycleLoan final {
        friend class BnwOwnedCameraGuiScreen;
        friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
        struct State;
        const std::shared_ptr<State> state_;
        UnitFlagLifecycleLoan(const BnwOwnedCameraGuiScreen&,const runtime::BnwStockLuaAttemptLease&);
        ~UnitFlagLifecycleLoan();
        void require_origin_without_callbacks()const;
        void require_current_owned()const;
        std::shared_ptr<State> retain_storage_without_callbacks()const;
        static void require_origin_from_storage_without_callbacks(const std::shared_ptr<State>&);
        static void require_current_from_storage_owned(const std::shared_ptr<State>&);
        static void require_source_origin_from_storage_without_callbacks(
            const std::shared_ptr<State>&,
            const std::weak_ptr<const SelectedBnwPreviewHost>&,
            const std::weak_ptr<const runtime::LiveSessionState>&,std::uint64_t);
        void require_source_origin_without_callbacks(
            const std::weak_ptr<const SelectedBnwPreviewHost>&,
            const std::weak_ptr<const runtime::LiveSessionState>&,std::uint64_t)const;
      public:
        UnitFlagLifecycleLoan(const UnitFlagLifecycleLoan&)=delete;
        UnitFlagLifecycleLoan& operator=(const UnitFlagLifecycleLoan&)=delete;
    };
    class ResearchChoiceLoan final {
        friend class BnwOwnedCameraGuiScreen;
        struct Action;Action& action_;
        explicit ResearchChoiceLoan(Action& action):action_(action){}
      public:
        ResearchChoiceLoan(const ResearchChoiceLoan&)=delete;
        ResearchChoiceLoan& operator=(const ResearchChoiceLoan&)=delete;
        void require_current()const;
        void require_ready_for_team(std::int64_t)const;
        std::shared_ptr<const BnwOwnedCameraGuiScreen> accept_commit(const runtime::BnwResearchChoiceCommit&)const;
        std::shared_ptr<const BnwOwnedCameraGuiScreen> deliver_dirty(runtime::BnwResearchChoiceDirty,std::int64_t)const;
    };
    void with_actual_research_choice_loan(const std::function<void(const ResearchChoiceLoan&)>&)const;
    ~BnwOwnedCameraGuiScreen();
    BnwOwnedCameraGuiScreen(const BnwOwnedCameraGuiScreen&)=delete;
    BnwOwnedCameraGuiScreen& operator=(const BnwOwnedCameraGuiScreen&)=delete;
    void require_current()const;
    std::shared_ptr<const BnwOwnedCameraGuiScreen> retain_owner()const;
    void require_same_selected_host(const SelectedBnwPreviewHost&)const;
    void require_same_window(const graphics::BnwOwnedCameraRendererWindowLease&)const;
    void require_same_research_choice(const runtime::BnwResearchChoiceCommit&)const;
    void require_same_research_refresh_origin(const BnwOwnedCameraGuiScreen&,
                                             const runtime::BnwResearchChoiceCommit&)const;
    void with_actual_screen_loan(const std::function<void(const Loan&)>&)const;
    // Renderer retires the publication before its borrowed source targets die.
    void retire()const noexcept;
 private:
    friend class ::civ5::graphics::BnwInitialUnitSetupGraphicalRendererLoan;
    friend class ::civ5::graphics::BnwUnitSetupGraphicsState;
    friend class ::civ5::graphics::BnwUnitDamageCommandOwner;
    friend class UiScreenCompositor;
    friend class BnwCivilopediaNativeRuntimeOwner;
    void require_civis_color_source_origin_without_callbacks(
        const std::weak_ptr<const SelectedBnwPreviewHost>&,
        const assets::CivisResourceBankOwner&)const;
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    friend class ::civ5::runtime::BnwSelectedStockLuaBootstrap;
    friend class UiScreenGlRenderer;
    friend class ::civ5::graphics::BnwSelectedPendingUnitVisualLoan;
    friend class ::civ5::graphics::BnwInitialCenterUnitRendererLoan;
    friend class ::civ5::graphics::BnwInitialUnitShouldDimRendererLoan;
    friend class ::civ5::graphics::BnwTrainPresentedFrameProof;
    mutable const void* train_presented_scope_=nullptr;
    mutable std::uint64_t train_presented_serial_=0;
    mutable bool train_presented_failed_=false;
    void require_train_presented_origin(const BnwTrainRendererRefreshReceipt&)const;
    void require_worldview_source_origin_without_callbacks(const Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease&,
        const std::weak_ptr<const SelectedBnwPreviewHost>&,
        const std::weak_ptr<const runtime::LiveSessionState>&,std::uint64_t)const;
    void with_actual_worldview_context_loans(const std::function<void(const Loan&,
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&)>&)const;
    void require_worldview_scene_origin_without_callbacks(const Loan&,const std::weak_ptr<BnwClientSceneFrame>&)const;
    std::weak_ptr<BnwClientSceneFrame> context_gui_scene_origin_without_callbacks(const Loan&)const;
    friend class BnwAlphaProgressionBackend;
    friend class ::civ5::runtime::BnwSelectedStockLuaSourceOwner;
    std::shared_ptr<LiveUiHost> retain_policy_action_ui()const;
    void with_worldview_frame_storage_owned(const Loan&,
        const std::function<void(const BnwClientSceneFrame&,const BnwClientFrame&,
            const LiveUiFrame&,const ComposedUiFrame&)>&)const;
    friend class BnwNotificationPublicationLender;
    void require_notification_storage_origin_without_callbacks(
        const Loan&,const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
        const BnwClientSceneFrame&,const BnwClientFrame&,const LiveUiFrame&,
        const ComposedUiFrame&,
        const std::weak_ptr<const BnwOwnedCameraGuiScreen>&,
        const std::weak_ptr<const BnwClientSceneFrame>&,
        const std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease>&,
        const std::weak_ptr<const SelectedBnwPreviewHost>&,
        const std::weak_ptr<const runtime::LiveSessionState>&,std::uint64_t)const;
    void with_actual_notification_publication_owned(
        const runtime::BnwOwnedGameplayEventPumpLoan&,
        const runtime::BnwStockLuaAttemptLease&,
        const std::function<void(BnwNotificationPublicationLender&)>&)const;
    void with_actual_unitflag_lifecycle_owned(const runtime::BnwStockLuaAttemptLease&,
        const std::function<void(const UnitFlagLifecycleLoan&)>&)const;
    struct Impl;class Pin;
    const std::shared_ptr<Impl> impl_;
    const std::thread::id thread_=std::this_thread::get_id();
    std::weak_ptr<const BnwOwnedCameraGuiScreen> self_;
    mutable std::size_t pins_=0;
    explicit BnwOwnedCameraGuiScreen(std::shared_ptr<Impl>);
    void check()const;
    void require_origin_lifetime_without_frame_callbacks()const;
};
}
