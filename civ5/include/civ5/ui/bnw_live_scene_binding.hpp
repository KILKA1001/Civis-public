#pragma once

#include "civ5/saves/civis_save.hpp"
#include <cstdint>
#include <optional>

namespace civ5::ui {

enum class BnwSceneOverlayPolicy { source_only, treasury_presentation };

// The current camera renderer restores its own SavedSession from the scene
// file. It has no receipt proving that its map, fog, art and saved UI data
// were restored to the same live overlay revision as LiveSession. Until that
// receipt exists, only an immutable import envelope may feed the renderer.
// The explicit treasury policy admits only the paired raw/owned balance
// sections; the launcher must still prove their identity and restored state.
// Inspect raw section IDs so a newly added live section cannot silently fall
// through an outdated list of LoadedSave optional fields.
inline std::optional<std::uint32_t> unbound_bnw_scene_section(
    const saves::CivisSaveDocument &document,
    BnwSceneOverlayPolicy policy=BnwSceneOverlayPolicy::source_only) {
    bool treasury=false, raw_treasury=false;
    for (const auto &section : document.sections) {
        treasury|=section.id==10;
        raw_treasury|=section.id==30;
    }
    for (const auto &section : document.sections)
        if (section.id != static_cast<std::uint32_t>(
                              saves::CivisSaveSectionId::imported_payload) &&
            section.id != static_cast<std::uint32_t>(
                              saves::CivisSaveSectionId::source_metadata) &&
            section.id != static_cast<std::uint32_t>(
                              saves::CivisSaveSectionId::source_prefix) &&
            !(policy==BnwSceneOverlayPolicy::treasury_presentation &&
              treasury && raw_treasury &&
              (section.id==10 || section.id==30)))
            return section.id;
    return std::nullopt;
}

} // namespace civ5::ui
