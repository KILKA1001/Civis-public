#pragma once
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include <optional>
namespace civ5::runtime {
class BnwSelectedStockLuaBootstrap;
// A scoped production income query, not permission to run detached DoGold.
// Only the actual SourceOwner can issue it inside its existing active VM scope.
class BnwTreasurySameVmIncomeLoan final {
 public:
    BnwTreasurySameVmIncomeLoan(const BnwTreasurySameVmIncomeLoan&)=delete;
    BnwTreasurySameVmIncomeLoan& operator=(const BnwTreasurySameVmIncomeLoan&)=delete;
    std::optional<std::int64_t> founder_benefits_read_only(
        std::int64_t actor,const LiveSessionState& world) const;
 private:
    friend class BnwSelectedStockLuaSourceOwner;
    BnwTreasurySameVmIncomeLoan(BnwSelectedStockLuaBootstrap&,
        BnwStockLuaAttemptLease,std::shared_ptr<const LiveSessionState>,std::int64_t);
    void require_current(const LiveSessionState&) const;
    const std::thread::id creator_=std::this_thread::get_id();
    BnwSelectedStockLuaBootstrap* const vm_;
    const BnwStockLuaAttemptLease lease_;
    const std::shared_ptr<const LiveSessionState> world_;
    const std::int64_t actor_;
    const std::uint64_t serial_,revision_;
    mutable bool querying_=false,failed_=false;
};
}
