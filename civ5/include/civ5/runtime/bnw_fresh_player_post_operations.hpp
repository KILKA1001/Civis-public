#pragma once

#include "civ5/runtime/bnw_fresh_player_operations_uninit.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

// Expansion2 CvPlayer.h:63 uses FStaticVector<int,152*96,true,...>.
// BaseVector::clear destroys no POD ints and sets m_uiCurrSize=0 while
// retaining the storage. ConqueredByBoolField::ClearAll writes four DWORDs.
struct BnwFreshPlayerPostOperationsState {
    static constexpr std::size_t kPlotsInitialCapacity = 152u * 96u;
    std::vector<std::int32_t> plot_indices;
    std::array<std::uint32_t,4> ever_conquered_by_words{};
    bool operator==(const BnwFreshPlayerPostOperationsState &) const = default;
};

enum class BnwFreshPlayerPostOperationsBlocker {
    none,
    predecessor_or_rules_invalid,
    plot_vector_capacity_unverified,
};
enum class BnwFreshPlayerPostOperationsEventKind {
    plots_size_zero,
    conquered_by_words_zero,
};
struct BnwFreshPlayerPostOperationsEvent {
    BnwFreshPlayerPostOperationsEventKind kind;
    std::string_view sdk_call;
    bool operator==(const BnwFreshPlayerPostOperationsEvent &) const = default;
};
struct BnwFreshPlayerPostOperationsDraft {
    BnwFreshPlayerPostOperationsState state;
    std::vector<BnwFreshPlayerPostOperationsEvent> source_order;
    std::string selected_rules_sha256;
    BnwFreshPlayerPostOperationsBlocker blocker =
        BnwFreshPlayerPostOperationsBlocker::none;
    std::string_view blocked_at;
    bool detached_effect_complete = false;
    bool archive_clear_delta_verified = false;
    bool archive_traversal_verified = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// Expansion2 CvPlayer.cpp:727-728. archive.clearDelta at line 731 is the
// next typed frontier; no readiness or live CvPlayer mutation is implied.
BnwFreshPlayerPostOperationsDraft project_bnw_fresh_player_post_operations(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshPlayerOperationsDraft &predecessor,
    std::string_view selected_rules_sha256,
    BnwFreshPlayerPostOperationsState initial);

} // namespace civ5::runtime
