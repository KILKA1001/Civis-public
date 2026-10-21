#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string_view>
namespace civ5::ui {class UiFrameSnapshot;class LiveUiHost;class SelectedBnwPreviewHost;class UiScreenCompositor;struct LiveUiFrame;class BnwTechAwardContextOwner;class BnwTechAwardInputDispatch;}
namespace civ5::runtime {
class BnwMajorTeamHasTechPending;class BnwSelectedStockLuaSourceOwner;
class BnwSelectedStockLuaBootstrap;class BnwStockLuaAttemptLease;
class BnwMajorTechTransportDispatcher;class BnwMajorTechClientEndpoint;
class BnwMajorTechTransportRequest;
 enum class BnwTechAwardContextEffect {queue,dequeue,timer_increment,timer_decrement,shown,processed};
enum class BnwMajorTechUiTransportKind;struct BnwMajorTechUiTransportEvent;
class BnwMajorTechTransportUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
enum class BnwMajorTechConsumptionPhase { applied,pending_frame,uncertain };
enum class BnwMajorTechRequestPhase { reserved,attempted,applied,uncertain };
class BnwMajorTechTransportConsumption final {
 public:
 ~BnwMajorTechTransportConsumption();
 BnwMajorTechConsumptionPhase phase()const;
 std::uint64_t occurrence()const;
 BnwMajorTechUiTransportKind kind()const;
 void require_same_request(const BnwMajorTeamHasTechPending&)const;
 BnwMajorTechTransportConsumption(const BnwMajorTechTransportConsumption&)=delete;
 BnwMajorTechTransportConsumption&operator=(const BnwMajorTechTransportConsumption&)=delete;
 private:
 friend class BnwMajorTechClientEndpoint;friend class BnwMajorTechTransportRequest;
 friend class BnwMajorTechTransportDispatcher;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwMajorTechTransportConsumption(std::unique_ptr<Impl>);
};
// Opaque current operation request. A DTO, alias, callback or readiness flag
// cannot construct it. Bootstrap alone closes a real stock-popup dispatch.
class BnwMajorTechTransportRequest final {
 public:
 ~BnwMajorTechTransportRequest();
 void require_current()const;
 BnwMajorTechUiTransportEvent event()const;
 BnwMajorTechRequestPhase phase()const;
 std::uint64_t operation_serial()const;
 BnwMajorTechTransportRequest(const BnwMajorTechTransportRequest&)=delete;
 BnwMajorTechTransportRequest&operator=(const BnwMajorTechTransportRequest&)=delete;
 private:
 friend class BnwMajorTechClientEndpoint;friend class BnwMajorTechTransportDispatcher;
 friend class BnwMajorTechTransportConsumption;
 friend class BnwSelectedStockLuaBootstrap;
 friend class ::civ5::ui::BnwTechAwardContextOwner;
 friend class ::civ5::ui::BnwTechAwardInputDispatch;
 friend class ::civ5::ui::LiveUiHost;
 std::shared_ptr<const ui::SelectedBnwPreviewHost> retain_selected_host_for_context()const;
 ui::UiFrameSnapshot retained_ui_snapshot_for_context()const;
 void register_actual_context_owner(const std::shared_ptr<const ui::BnwTechAwardContextOwner>&)const;
 // Applied genuine receiver only: read/publish guard, never VM dispatch or
 // renewal of the old attempt serial. Callback setters retain require_current.
 void require_context_stored_current()const;
 std::shared_ptr<const ui::SelectedBnwPreviewHost> retain_stored_host_for_context()const;
 void require_context_mutation_current()const;
 BnwMajorTechUiTransportEvent context_effect_event()const;
 std::shared_ptr<const ui::SelectedBnwPreviewHost> retain_mutation_host_for_context()const;
 void attach_context_input(const std::shared_ptr<const ui::BnwTechAwardInputDispatch>&,const BnwStockLuaAttemptLease&)const;
 void detach_context_input(const ui::BnwTechAwardInputDispatch&)const;
 void register_context_input_slot(std::string_view)const;
 void apply_context_effect(BnwTechAwardContextEffect)const;
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwMajorTechTransportRequest(std::shared_ptr<Impl>);
 // Call before actual handler argument allocations/copy/callback. A failure
 // leaves attempted/uncertain; no later attempt is allowed.
 void begin_actual_popup_dispatch(const BnwStockLuaAttemptLease&)const;
 // Only after real selected OnPopup/RefreshDisplay/QueuePopup returns and
 // same-VM bookends. This records queued presentation, not paint/present.
 std::shared_ptr<const BnwMajorTechTransportConsumption>
 finish_actual_popup_dispatch(const BnwStockLuaAttemptLease&)const;
 void fail_actual_dispatch()const noexcept;
};
class BnwMajorTechClientEndpoint final {
 public:
 ~BnwMajorTechClientEndpoint();
 void require_current()const;
 void require_same_host(const ui::SelectedBnwPreviewHost&)const;
 BnwMajorTechClientEndpoint(const BnwMajorTechClientEndpoint&)=delete;
 BnwMajorTechClientEndpoint&operator=(const BnwMajorTechClientEndpoint&)=delete;
 private:
 friend class ::civ5::ui::LiveUiHost;friend class ::civ5::ui::UiScreenCompositor;
 friend class ::civ5::ui::BnwTechAwardInputDispatch;
 friend class BnwMajorTechTransportRequest;friend class BnwMajorTechTransportConsumption;
 friend class BnwMajorTechTransportDispatcher;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwMajorTechClientEndpoint(std::unique_ptr<Impl>);
 std::shared_ptr<const BnwMajorTechTransportRequest> prepare_next(
  const std::shared_ptr<const BnwMajorTeamHasTechPending>&,const BnwStockLuaAttemptLease&)const;
 std::shared_ptr<const BnwMajorTechTransportConsumption> consume_dirty(
  const std::shared_ptr<const BnwMajorTechTransportRequest>&)const;
};
class BnwMajorTechTransportDispatcher final {
 private:
 friend class BnwSelectedStockLuaSourceOwner;
 static std::shared_ptr<const BnwMajorTechTransportRequest> prepare_next(
  const std::shared_ptr<const BnwMajorTeamHasTechPending>&,
  const std::shared_ptr<const BnwMajorTechClientEndpoint>&,const BnwStockLuaAttemptLease&);
 static std::shared_ptr<const BnwMajorTechTransportConsumption> consume_dirty(
  const std::shared_ptr<const BnwMajorTechTransportRequest>&);
};
}
