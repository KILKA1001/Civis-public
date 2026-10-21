#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/runtime/vp_science_profile.hpp"
#include "civ5/saves/civis_save.hpp"

namespace civ5::runtime {
struct VpScienceCheckpointBudget {
    std::uint64_t players=1'000'000,bytes=1024ULL*1024*1024;
};
// Live VP GameCore science cache changed by doResearch. Other VP science
// inputs remain imported from the retained source until their turn writers exist.
saves::CivisSaveSection make_vp_science_checkpoint(
    const ScienceContext&,const ResearchContext&,const SessionCheckpointIdentity&,
    VpScienceCheckpointBudget={});
// Validate the complete profile section and active roster before applying it.
void restore_vp_science_checkpoint(
    const saves::CivisSaveSection&,ScienceContext&,const ResearchContext&,
    const SessionCheckpointIdentity&,VpScienceCheckpointBudget={});
} // namespace civ5::runtime
