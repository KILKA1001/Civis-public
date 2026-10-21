#pragma once

#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include <string_view>
#include <utility>

namespace civ5::graphics {

// Immutable source sections produced from the SHA-pinned selected Gandhi
// Civ5Save. A container's claimed source hash alone is insufficient: all
// payload, metadata and setup bytes must match before replaying its captures.
inline bool has_gandhi_bnw_scene_capture_baseline(
    const saves::CivisSaveDocument &document) {
    // make_civis_import_document sets the imported-source envelope flag (1).
    if(document.version!=1 || document.flags!=1 ||
       ui::unbound_bnw_scene_section(document,
           ui::BnwSceneOverlayPolicy::treasury_presentation) ||
       (ui::unbound_bnw_scene_section(document) &&
        !bnw_treasury_presentation_digest(document))) return false;
    constexpr std::array<std::pair<std::uint32_t,std::string_view>,3> expected{{
        {1,"e39659d7df06499ba1a07fe8e945bc5e75ab3616d8e84b1e616b2daa3ea4ec2a"},
        {4,"7d22162dbb1de02f229c803d2996e86aa7c2a7befcec7219e02a1d848c533e3f"},
        {5,"928d8cedbee91ff6655ab3c79263d5533db1bc94ae717efb72abf71addc229ff"}
    }};
    for(const auto &[id,sha]:expected) {
        const saves::CivisSaveSection *found=nullptr;
        for(const auto &section:document.sections)
            if(section.id==id) {
                if(found || section.flags!=0) return false;
                found=&section;
            }
        if(!found || runtime::identify_bytes(found->bytes).sha256!=sha)
            return false;
    }
    return true;
}

} // namespace civ5::graphics
