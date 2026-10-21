#include "civ5/runtime/bnw_plot_build_progress_diagnostic.hpp"
#include <climits>
#include <map>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {
    if(!ok) throw std::invalid_argument(why);
}
}
BnwPlotBuildProgressDiagnostic restore_bnw_plot_build_progress_diagnostic(
    std::int64_t declared_count,
    std::span<const std::optional<BnwDiagnosticBuildInfo>> selected_slots,
    std::span<const BnwDiagnosticSavedBuildProgress> records) {
    require(declared_count>=INT32_MIN && declared_count<=INT32_MAX,
            "BNW build allocation gate outside source int range");
    require(selected_slots.size()<=static_cast<std::size_t>(INT32_MAX) &&
            records.size()<=static_cast<std::size_t>(INT32_MAX),
            "BNW selected Build slot count outside source int range");
    // Pin the typed values before evaluating their selected order. There are no
    // callbacks or borrowed outputs; live fencing remains an upstream owner.
    const std::vector<std::optional<BnwDiagnosticBuildInfo>> slots(
        selected_slots.begin(),selected_slots.end());
    const std::vector<BnwDiagnosticSavedBuildProgress> saved(records.begin(),records.end());
    std::map<std::uint32_t,std::size_t> by_hash;
    BnwPlotBuildProgressDiagnostic result;
    result.allocated=declared_count>0;
    result.builds.reserve(slots.size());
    for(std::size_t slot=0;slot<slots.size();++slot) {
        if(!slots[slot]) {
            result.builds.push_back({-1,-1,0});
            continue;
        }
        const auto &info=*slots[slot];
        require(info.type_hash!=0 && info.improvement>=-1 && info.route>=-1,
                "BNW selected Build info invalid");
        require(by_hash.emplace(info.type_hash,slot).second,
                "BNW selected Build hash resolution ambiguous");
        result.builds.push_back({info.improvement,info.route,0});
    }
    if(!result.allocated) {
        require(saved.empty(),"BNW unallocated build array has nested records");
        return result;
    }
    for(const auto &record:saved) {
        if(record.type_hash==0) {
            require(!record.value,"BNW null Build hash must not consume a value");
            continue;
        }
        const auto at=by_hash.find(record.type_hash);
        require(at!=by_hash.end(),
                "BNW unknown Build hash requires original error/int-dummy wire path");
        require(record.value && *record.value>=INT16_MIN && *record.value<=INT16_MAX,
                "BNW known Build progress is not a decoded source short");
        result.builds[at->second].progress=static_cast<std::int16_t>(*record.value);
    }
    return result;
}
} // namespace civ5::runtime
