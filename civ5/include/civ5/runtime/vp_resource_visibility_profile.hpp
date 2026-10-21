#pragma once
#include "civ5/runtime/resource_visibility.hpp"

namespace civ5::runtime {
// VP-only team observer cache and player trait technology overrides. The
// common resource state retains only the forced-resource vector.
struct VpResourceVisibilityProfile {
    struct AlternateTechnology { std::int64_t reveal=-1,city_trade=-1; };
    std::map<std::int64_t,bool> observers;
    std::map<std::int64_t,std::map<std::int64_t,AlternateTechnology>> alternate_technologies;
};
inline VpResourceVisibilityProfile &ensure_vp_resource_visibility_profile(ResourceVisibilityState &state){
    if(state.profile!=content::TechnologyProfile::vp542)throw std::runtime_error("VP resource visibility requires VP GameCore profile");
    if(auto *profile=state.profile_extensions.find<VpResourceVisibilityProfile>())return *profile;
    return state.profile_extensions.emplace<VpResourceVisibilityProfile>();
}
inline VpResourceVisibilityProfile &vp_resource_visibility_profile(ResourceVisibilityState &state){
    if(state.profile!=content::TechnologyProfile::vp542)throw std::runtime_error("VP resource visibility requires VP GameCore profile");
    auto *profile=state.profile_extensions.find<VpResourceVisibilityProfile>();
    if(!profile)throw std::runtime_error("Missing VP resource visibility profile");
    return *profile;
}
inline const VpResourceVisibilityProfile &vp_resource_visibility_profile(const ResourceVisibilityState &state){
    if(state.profile!=content::TechnologyProfile::vp542)throw std::runtime_error("VP resource visibility requires VP GameCore profile");
    const auto *profile=state.profile_extensions.find<VpResourceVisibilityProfile>();
    if(!profile)throw std::runtime_error("Missing VP resource visibility profile");
    return *profile;
}
} // namespace civ5::runtime
