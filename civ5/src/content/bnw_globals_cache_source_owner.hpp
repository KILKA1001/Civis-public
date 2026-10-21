#pragma once
#include "civ5/content/rules_database.hpp"
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <thread>
namespace civ5::ui { class SelectedBnwPreviewHost; }
namespace civ5::content {
namespace globals_cache_detail {
// Isolated source-algorithm state. This is not a lifecycle receipt.
class LeafOperation;
struct LeafState {
    LeafState() = default;
    LeafState(const LeafState&) = delete;
    LeafState& operator=(const LeafState&) = delete;
    LeafState(LeafState&&) = delete;
    LeafState& operator=(LeafState&&) = delete;
    std::int32_t heavy = 3;
    std::map<std::string, CachedGlobalDefine, std::less<>> values;
    std::vector<std::string> write_order;
    std::vector<std::string> policy_correction_order;
  private:
    friend class LeafOperation;
    const std::thread::id thread_ = std::this_thread::get_id();
    std::atomic<bool> poisoned_{false};
    bool active_ = false; // Accessed only after the originating-thread check.
};
std::int32_t source_int(std::int64_t value) noexcept;
using WriteObserver = std::function<void(std::size_t, const CachedGlobalDefine&)>;
// No callback boundary: caller retains both borrowed objects for this call.
void ordered_lookup_write(const RulesDatabase&, LeafState&);
// Callback boundary: by-value genuine shared owners retain both exact objects.
// Observer receives a copy, borrowed only for its invocation. Direct concurrent
// external field writes are outside this single-thread diagnostic contract.
void ordered_lookup_write(std::shared_ptr<const RulesDatabase>, std::shared_ptr<LeafState>,
                          const WriteObserver&);
}
// Intentionally has no public constructor, stage setter, cache accessor or
// completion permission. Only the genuine parent may create this child.
class BnwGlobalsCacheSourceOwner final {
    friend class civ5::ui::SelectedBnwPreviewHost;
    BnwGlobalsCacheSourceOwner(const BnwGlobalsCacheSourceOwner&) = delete;
    BnwGlobalsCacheSourceOwner& operator=(const BnwGlobalsCacheSourceOwner&) = delete;
    BnwGlobalsCacheSourceOwner(BnwGlobalsCacheSourceOwner&&) = delete;
    BnwGlobalsCacheSourceOwner& operator=(BnwGlobalsCacheSourceOwner&&) = delete;
    explicit BnwGlobalsCacheSourceOwner(std::shared_ptr<const RulesDatabase>);
    void require_current(const RulesDatabase&) const;
    void cache_game_database_data();
    void flush_game_database_data();
    std::shared_ptr<const RulesDatabase> database_;
    std::function<bool()> current_;
    std::thread::id thread_;
    globals_cache_detail::LeafState globals_;
    bool needs_caching_ = true;
};
}
