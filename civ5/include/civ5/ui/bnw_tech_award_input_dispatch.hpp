#pragma once
#include <cstdint>
#include <memory>
namespace civ5::runtime {
class BnwSelectedStockLuaBootstrap;class BnwSelectedStockLuaSourceOwner;class LuaScriptHost;class BnwStockLuaAttemptLease;
class BnwMajorTechTransportRequest;
}
namespace civ5::ui {
class LiveUiHost;class BnwTechAwardContextOwner;struct LiveUiFrame;
enum class BnwTechAwardInput {continue_button,close_button,escape,return_key};
enum class BnwTechAwardInputPhase {reserved,executing,pending_frame,completed,uncertain};
// Minted only from the actual final issued UiHost frame and its applied popup.
// No closure/ref number/caller boolean or original operation serial is admitted.
class BnwTechAwardInputDispatch final {
 public:
 ~BnwTechAwardInputDispatch();
 BnwTechAwardInputPhase phase()const;
 BnwTechAwardInput target()const;
 std::uint64_t occurrence()const;
 void require_current()const;
 BnwTechAwardInputDispatch(const BnwTechAwardInputDispatch&)=delete;
 BnwTechAwardInputDispatch&operator=(const BnwTechAwardInputDispatch&)=delete;
 private:
 friend class LiveUiHost;friend class BnwTechAwardContextOwner;
 friend class runtime::BnwSelectedStockLuaBootstrap;friend class runtime::LuaScriptHost;
 friend class runtime::BnwSelectedStockLuaSourceOwner;
 friend class runtime::BnwMajorTechTransportRequest;
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwTechAwardInputDispatch(std::shared_ptr<Impl>);
 // Callback-free issuance/control authentication; pin before currentness callbacks.
 std::shared_ptr<const BnwTechAwardInputDispatch> retain_canonical_identity(const std::shared_ptr<const BnwTechAwardInputDispatch>&)const;
 std::shared_ptr<const BnwTechAwardInputDispatch> retain_canonical(const std::shared_ptr<const BnwTechAwardInputDispatch>&)const;
 std::shared_ptr<const BnwTechAwardContextOwner> retain_context_for_dispatch()const;
 // SAMEVM installer must compare its retained canonical owner with this owner
 // before begin. Begin authenticates a NEW actual lease against old source
 // identity; it never calls/rebases the old operation serial.
 void begin_actual_dispatch(const runtime::BnwStockLuaAttemptLease&)const;
 void finish_actual_dispatch(const runtime::BnwStockLuaAttemptLease&)const;
 void fail_actual_dispatch()const noexcept;
 void require_active_dispatch()const;
 void require_same_actual_context(const BnwTechAwardContextOwner&)const;
 void note_actual_effect_sequence(std::uint64_t)const;
 std::int32_t native_key_down()const;
 std::int32_t native_input_key()const;
 std::int32_t native_escape_key()const;
 std::int32_t native_return_key()const;
};
}
