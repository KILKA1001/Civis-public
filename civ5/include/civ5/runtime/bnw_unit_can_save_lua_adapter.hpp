#pragma once
#include "civ5/runtime/bnw_unit_can_save_buffer.hpp"
#include <memory>
#include <optional>

namespace civ5::runtime {

// An implementation must prove it has forked the selected gameplay VM at the
// current event boundary, with all world/providers detached. LuaScriptHost is
// noncopyable and presently has no native VM fork, so a live host cannot be
// passed as this service. A null fork or incomplete effect batch blocks.
class BnwCanSaveUnitLuaEscrow {
  public:
    virtual ~BnwCanSaveUnitLuaEscrow()=default;
    virtual BnwCanSaveScriptSystem script_system(
        const saves::SavedSessionSnapshot &,
        const BnwUnitKillEntry &)=0;
    virtual std::unique_ptr<LuaScriptHost> fork_exact_detached(
        const saves::SavedSessionSnapshot &,
        const BnwUnitKillEntry &)=0;
    virtual std::optional<std::vector<BnwCanSaveEffectIntent>> collect_all_effects(
        LuaScriptHost &)=0;
};

struct BnwCanSaveUnitLuaAttempt {
    BnwCanSaveResolution resolution;
    // A successful cancellation or continuation retains the mutated forked
    // VM, including Lua global changes. There is no publication API yet.
    std::unique_ptr<LuaScriptHost> detached_host;
};

// Actual LuaScriptHost::test_all call in SDK order and with two exact args.
// The escrow makes the fork/effect capture obligation explicit. Without it,
// this returns blocked before touching any live Lua host or world state.
BnwCanSaveUnitLuaAttempt stage_bnw_can_save_unit_lua_host(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitKillEntry &entry,
    BnwCanSaveUnitLuaEscrow *escrow);

} // namespace civ5::runtime
