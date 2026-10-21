#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <utility>
#include <optional>
#include <string_view>
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
namespace civ5::ui {class BnwOwnedCameraGuiScreen;class UiScreenGlRenderer;class BnwClientSceneFrame;}
namespace civ5::runtime {class BnwPendingCreatedUnitRead;class ScriptUnitReadReference;class BnwSelectedStockLuaBootstrap;class LuaScriptHost;class BnwWorldViewSourceOperationLoan;}
namespace civ5::graphics {
class BnwInitialUnitSetupGraphicalRendererLoan;
class BnwInitialUnitSetupGraphicalEnqueueProof;
// Own Civis graphics lifecycle, disjoint from Game::is_final_initialized.
// Creation is UNISSUED; neither missing state nor construction means false.
class BnwUnitSetupGraphicsState final {
 friend class ui::UiScreenGlRenderer;
 friend class BnwInitialUnitSetupGraphicalRendererLoan;
 struct State;std::shared_ptr<State> state_;
 explicit BnwUnitSetupGraphicsState(std::shared_ptr<State>);
 static std::shared_ptr<BnwUnitSetupGraphicsState> capture_actual_renderer_origin(
  const BnwOwnedCameraRendererWindowLease::Loan&,const ui::BnwOwnedCameraGuiScreen&);
 // Only actual renderer lifecycle call sites may publish these transitions.
 // The after-initialize call site must follow real successful resource setup.
 void publish_before_actual_renderer_initialize();
 void publish_after_actual_renderer_initialize();
 void retire()noexcept;
 class BorrowedOwner final {
  friend class BnwInitialUnitSetupGraphicalRendererLoan;
  std::shared_ptr<BnwUnitSetupGraphicsState> owner_;
  explicit BorrowedOwner(std::shared_ptr<BnwUnitSetupGraphicsState>);
  const BnwUnitSetupGraphicsState&current()const;
  public: ~BorrowedOwner();
  BorrowedOwner(const BorrowedOwner&)=delete;
  BorrowedOwner&operator=(const BorrowedOwner&)=delete;
 };
 void pure()const;
 void full()const;
 bool read_is_graphics_initialized()const;
 std::uint64_t revision()const;
 void require_same_gui(const ui::BnwOwnedCameraGuiScreen&)const;
 public:
 ~BnwUnitSetupGraphicsState();
 BnwUnitSetupGraphicsState(const BnwUnitSetupGraphicsState&)=delete;
 BnwUnitSetupGraphicsState&operator=(const BnwUnitSetupGraphicsState&)=delete;
};
// Typed SDK calls, not asserted retail wire event IDs or renderer/Lua ACKs.
enum class BnwUnitSetupGraphicalEventKind {
 embark,created,should_dim,fortify,set_damage,work,visibility,garrison
};
struct BnwUnitSetupGraphicalEvent {
 BnwUnitSetupGraphicalEventKind kind;
 std::int32_t owner,full_id;
 // Boolean operations use 0/1; Damage uses (newDamage,0); Work uses BuildType.
 std::int32_t first=0,second=0;
 // Present ONLY for real created producer reads, never supplied by Lua IDs.
 std::optional<std::int32_t> original_fog;
 std::optional<bool> original_not_invisible;
};
class BnwUnitSetupGraphicalQueue final {
 friend class BnwInitialUnitSetupGraphicalEnqueueProof;
 friend class runtime::LuaScriptHost;
 friend class runtime::BnwSelectedStockLuaBootstrap;
 friend class ui::BnwClientSceneFrame;
 friend class BnwInitialUnitSetupGraphicalRendererLoan;
 struct State;struct Pin;struct Batch;struct PendingReadLifetime;class PendingReadReference;
 std::shared_ptr<State> state_;
 explicit BnwUnitSetupGraphicalQueue(std::shared_ptr<State>);
 // Owner must bind this actual storage to its own selected Scene, not a caller
 // callback or an arbitrary operation serial. Queue binding does not initialize graphics.
 static std::shared_ptr<BnwUnitSetupGraphicalQueue> bind_actual_scene(const ui::BnwClientSceneFrame&);
 void pure()const;
 void require_same_scene(const ui::BnwClientSceneFrame&)const;
 void require_same_factory(const std::weak_ptr<BnwUnitSetupGraphicalQueue>&)const;
 static void quarantine_from_owned_weak(const std::weak_ptr<BnwUnitSetupGraphicalQueue>&)noexcept;
 // Original setupGraphical queue occurrence; no payload DTO can issue this.
 class CreatedOccurrenceLoan final {
  friend class BnwUnitSetupGraphicalQueue;
  friend struct PendingReadLifetime;
  friend class PendingReadReference;
  friend class runtime::BnwSelectedStockLuaBootstrap;
  friend class runtime::LuaScriptHost;
  struct Impl;const std::unique_ptr<Impl> impl_;
  explicit CreatedOccurrenceLoan(std::unique_ptr<Impl>);
  ~CreatedOccurrenceLoan();
  void require_origin_without_callbacks()const;
  void require_current()const;
  void require_same_source(const runtime::BnwWorldViewSourceOperationLoan&)const;
  std::pair<std::int32_t,std::int32_t> actual_owner_full_id()const;
  std::pair<std::int32_t,bool> actual_original_fog_not_invisible()const;
  std::int32_t read_pending_integer_owned(std::string_view)const;
  bool read_pending_boolean_owned(std::string_view)const;
  std::shared_ptr<const runtime::ScriptUnitReadReference>retain_pending_unit_reference_owned()const;
  void record_original_constructor_returned()const;
  CreatedOccurrenceLoan(const CreatedOccurrenceLoan&)=delete;
  CreatedOccurrenceLoan&operator=(const CreatedOccurrenceLoan&)=delete;
 };
 static void require_render_scene_source(const runtime::BnwWorldViewSourceOperationLoan&,
  const std::weak_ptr<ui::BnwClientSceneFrame>&);
 // Only Bootstrap can supply its real original Lua construction consumer.
 static bool with_first_created_owned(const std::weak_ptr<BnwUnitSetupGraphicalQueue>&,
  const runtime::BnwWorldViewSourceOperationLoan&,
  const std::function<void(const CreatedOccurrenceLoan&)>&);
 class Completed final {
  friend class BnwUnitSetupGraphicalQueue;
  friend class BnwInitialUnitSetupGraphicalRendererLoan;
  friend class BnwInitialUnitSetupGraphicalEnqueueProof;
  std::weak_ptr<BnwUnitSetupGraphicalQueue> queue_;
  std::shared_ptr<const Batch> batch_;
  std::uint64_t serial_;
  Completed(std::weak_ptr<BnwUnitSetupGraphicalQueue>,std::shared_ptr<const Batch>,std::uint64_t);
  void require_member(const BnwInitialUnitSetupGraphicalRendererLoan&)const;
 };
 static void preflight_from_owned_weak(const std::weak_ptr<BnwUnitSetupGraphicalQueue>&,
  const BnwInitialUnitSetupGraphicalRendererLoan&);
 static std::shared_ptr<const Completed> enqueue_from_owned_weak(
  const std::weak_ptr<BnwUnitSetupGraphicalQueue>&,const BnwInitialUnitSetupGraphicalRendererLoan&,
  std::vector<BnwUnitSetupGraphicalEvent>,
  std::shared_ptr<const runtime::BnwPendingCreatedUnitRead> pending_read={});
 public:
 ~BnwUnitSetupGraphicalQueue();
 BnwUnitSetupGraphicalQueue(const BnwUnitSetupGraphicalQueue&)=delete;
 BnwUnitSetupGraphicalQueue&operator=(const BnwUnitSetupGraphicalQueue&)=delete;
};
}
