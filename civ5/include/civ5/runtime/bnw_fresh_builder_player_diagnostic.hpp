#pragma once

#include "civ5/runtime/bnw_fresh_archive_vector.hpp"
#include "civ5/runtime/bnw_fresh_player_uninit_plan.hpp"
#include "civ5/runtime/bnw_fresh_player_uninit_prefix.hpp"
#include <span>
#include <string>
#include <string_view>

namespace civ5::runtime {

enum class BnwFreshBuilderPlayerDiagnosticBlocker {
    none,
    roster_or_selected_rules_invalid,
    player_archive_preflight_invalid,
    uninit_projection_invalid,
    archive_reset_traversal_pending,
};
struct BnwFreshBuilderPlayerDiagnostic {
    BnwFreshPlayerUninitPrefixDraft detached_uninit;
    std::string selected_rules_sha256;
    BnwFreshBuilderPlayerDiagnosticBlocker blocker =
        BnwFreshBuilderPlayerDiagnosticBlocker::none;
    std::string_view actual_source_frontier;
    bool diagnostic_complete = false;
    bool source_prefix_advanced = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// Join one selected first-major roster, its constructor/preflight states and
// member archive projection to the complete detached Uninit prefix. This is
// a diagnostic AFTER the opaque m_syncArchive.reset, never a committed SDK
// step. The actual builder source prefix must still stop at that reset.
BnwFreshBuilderPlayerDiagnostic draft_bnw_fresh_builder_player_diagnostic(
    const BnwFreshPregameRosterDraft &roster,
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshPlayerUninitPlan &plan,
    const BnwFreshArchiveVectorProjection &first_resource_vector,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants);

} // namespace civ5::runtime
