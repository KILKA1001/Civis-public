#pragma once
#include "civ5/terrain/plot_graphics_state.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace civ5::runtime {
// Explicit caller-supplied GC.Builds slot order. Empty entries represent null
// GC.getBuildInfo slots. SQL row order is not evidence of this native order.
struct BnwDiagnosticBuildInfo {
    std::uint32_t type_hash=0;
    std::int32_t improvement=-1, route=-1;
};
// Already decoded ordered wire records. A zero hash consumes no value. Unknown
// nonzero hashes require the original error/int-dummy wire path and are refused.
struct BnwDiagnosticSavedBuildProgress {
    std::uint32_t type_hash=0;
    std::optional<std::int64_t> value;
};
struct BnwPlotBuildProgressDiagnostic {
    bool allocated=false;
    // Dense slot order, including unavailable info slots. The source's short
    // values are preserved; nonpositive progress is not a half-built selection.
    std::vector<terrain::PlotBuildProgress> builds;
    // This pure transform does not establish a current GC.Builds/session owner.
    bool can_publish_live() const noexcept {return false;}
};
// Bounded known-hash CvPlot::read/BuildArrayHelpers::Read transformation:
// iCount is an allocation gate, not the nested-record count; later writes to
// the same selected slot win. No DB/VFS/model/native-event backend is created.
BnwPlotBuildProgressDiagnostic restore_bnw_plot_build_progress_diagnostic(
    std::int64_t declared_count,
    std::span<const std::optional<BnwDiagnosticBuildInfo>> selected_slots,
    std::span<const BnwDiagnosticSavedBuildProgress> records);
} // namespace civ5::runtime
