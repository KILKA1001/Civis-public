#pragma once
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace civ5::ui { class SelectedBnwPreviewHost; }
namespace civ5::runtime {
class LiveSession;
class BnwResearchChoiceCommit;
enum class BnwResearchChoiceEvent { player_can_ever_research, player_can_research };
enum class BnwResearchChoiceDirty { research_buttons, game_data, score };
struct BnwResearchChoiceCommand {
    std::int64_t player=-1, technology=-1, expected_turn=-1;
    std::uint64_t expected_generation=0, expected_ui_source_revision=0;
    bool clear_queue=true;
};
// First slice supports a currently researchable technology only. Every Lua
// TestAll occurs before staged queue writes: canonical SAME-VM getters remain
// accurate. Future-technology prerequisite planning needs another binding.
class BnwResearchChoiceHooks final {
  public:
    BnwResearchChoiceHooks(const BnwResearchChoiceHooks&)=delete;
    BnwResearchChoiceHooks& operator=(const BnwResearchChoiceHooks&)=delete;
    using TestAll=std::function<std::optional<bool>(BnwResearchChoiceEvent,
        std::int64_t player,std::int64_t technology)>;
    using AcceptCommit=std::function<void(const BnwResearchChoiceCommit&)>;
    using Dirty=std::function<void(BnwResearchChoiceDirty,std::int64_t team)>;
  private:
    friend class BnwSelectedStockLuaSourceOwner;
    friend class BnwResearchChoiceCommit;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwResearchChoiceHooks(std::unique_ptr<Impl>);
    // SourceOwner supplies its genuine activated VM, canonical Host and actual
    // active attempt. No public callback/ready-flag factory exists. TestAll must
    // actually dispatch selected GameEvents and be observational; nullopt means
    // that this actual dispatch found no listener, never unavailable activation.
    static std::shared_ptr<const BnwResearchChoiceHooks> issue(
        std::shared_ptr<const ui::SelectedBnwPreviewHost>,BnwStockLuaAttemptLease,
        TestAll,AcceptCommit,Dirty);
  public:
    ~BnwResearchChoiceHooks();
};
enum class BnwResearchChoicePhase { prepared, committed, complete, uncertain };
// Opaque native commit, NOT a full turn/save/render grant. A hook is consumed
// before its first callback. A failed dirty tail cannot replay the queue stores.
class BnwResearchChoiceCommit final {
  public:
    ~BnwResearchChoiceCommit();
    BnwResearchChoiceCommit(const BnwResearchChoiceCommit&)=delete;
    BnwResearchChoiceCommit& operator=(const BnwResearchChoiceCommit&)=delete;
    static std::shared_ptr<const BnwResearchChoiceCommit> execute(
        LiveSession&,std::shared_ptr<const BnwResearchChoiceHooks>,
        const BnwResearchChoiceCommand&);
    BnwResearchChoicePhase phase()const;
    std::uint64_t before_revision()const;
    std::uint64_t after_revision()const;
    std::int64_t player()const;
    std::int64_t team()const;
    std::size_t completed_dirty_calls()const;
    const std::vector<std::int64_t>& queue()const;
    // PRIVATE AcceptCommit receiver checks this before updating only its actual
    // operation_world_revision. This checks actual stored raw/owned values,
    // allocation/generation/identity; deliberately does not check the old Lua
    // revision guard, which must still equal before_revision until acceptance.
    void require_stored_current(const ui::SelectedBnwPreviewHost&)const;
  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwResearchChoiceCommit(std::unique_ptr<Impl>);
};
}
