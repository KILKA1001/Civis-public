#include "civ5/ui/economic_overview_audio.hpp"
#include "civ5/runtime/file_identity.hpp"

namespace civ5::ui {
namespace {
constexpr auto economic_id = "BUTTONPOPUP_ECONOMIC_OVERVIEW";
constexpr auto popup_audio_key = "Assets/Sounds/XML/ButtonPopupSounds.xml";

bool explicit_audio(std::string_view lua) {
    return lua.find("AudioPlay2DSound") != std::string_view::npos ||
           lua.find("PlaySound") != std::string_view::npos;
}

std::optional<std::string> selected_lua(
    const assets::ResourceMountTable &resources, const char *key,
    std::string_view expected_sha256) {
    const auto selected = resources.resolve(assets::AssetKey(key));
    const auto *handle = std::get_if<assets::AssetHandle>(&selected.asset);
    if (!handle) return std::nullopt;
    const auto bytes = handle->read_bytes(1024 * 1024);
    if (runtime::identify_bytes(bytes).sha256 != expected_sha256)
        return std::nullopt;
    return std::string(bytes.begin(), bytes.end());
}
} // namespace

EconomicAudioSourceProof inspect_economic_audio_source(
    const assets::ResourceMountTable &resources, const UiAudioCatalog &catalog,
    const EconomicGoldSourceProof &open_source,
    const EconomicCloseSourceProof &close_source) {
    EconomicAudioSourceProof proof;
    if (!open_source.verified || !close_source.verified ||
        open_source.economic_lua_sha256 != close_source.lua_sha256 ||
        !catalog.popup_source()) {
        proof.blocker = "EconomicOverview event, close or selected popup audio catalog is unverified";
        return proof;
    }
    try {
        const auto top = selected_lua(resources, "Assets/UI/InGame/TopPanel.lua",
            open_source.top_panel_lua_sha256);
        const auto economic = selected_lua(resources,
            "Assets/UI/InGame/Popups/EconomicOverview.lua",
            open_source.economic_lua_sha256);
        const auto selected = resources.resolve(assets::AssetKey(popup_audio_key));
        const auto *handle = std::get_if<assets::AssetHandle>(&selected.asset);
        const auto *mapping = catalog.find(UiAudioKind::popup, economic_id);
        if (!top || !economic || !handle || !mapping ||
            explicit_audio(*top) || explicit_audio(*economic) ||
            mapping->script_id != "AS2D_PLACEHOLDER_BLANK") {
            proof.blocker = "Selected EconomicOverview has an unresolved or changed authored audio route";
            return proof;
        }
        const auto &audio_source = *catalog.popup_source();
        if (audio_source.selected_name != handle->candidate().original_name ||
            audio_source.mount_id != selected.mount_id) {
            proof.blocker = "Popup audio catalog and selected VFS source disagree";
            return proof;
        }
        const auto bytes = handle->read_bytes(4 * 1024 * 1024);
        const auto document = content::read_xml_metadata(
            std::string(bytes.begin(), bytes.end()));
        std::size_t matching_rows = 0;
        for (const auto &row : document.children) {
            if (row.name != "ButtonPopupSound") continue;
            const auto *id = row.child("ID");
            if (!id || id->text != economic_id) continue;
            const auto *script = row.child("AudioScript");
            if (!script || script->text != "AS2D_PLACEHOLDER_BLANK") {
                proof.blocker = "Selected EconomicOverview popup audio row is not silent";
                return proof;
            }
            ++matching_rows;
        }
        if (document.name != "ButtonPopupSounds" || matching_rows != 1) {
            proof.blocker = "Selected EconomicOverview popup audio row is missing or ambiguous";
            return proof;
        }
        proof.verified = true;
        proof.popup_is_silent = true;
        proof.popup_audio_xml_path = handle->candidate().provenance.source_relative_path;
        proof.popup_audio_xml_sha256 = runtime::identify_bytes(bytes).sha256;
        proof.popup_audio_selected_name = audio_source.selected_name;
        proof.popup_audio_mount_id = audio_source.mount_id;
    } catch (const std::exception &) {
        proof.blocker = "Selected EconomicOverview audio source cannot be verified";
    }
    return proof;
}

EconomicOverviewAudioInteraction::EconomicOverviewAudioInteraction(
    EconomicOverviewInteraction &interaction, UiAudioCatalog catalog,
    EconomicAudioSourceProof source)
    : interaction_(&interaction), catalog_(std::move(catalog)),
      queue_(catalog_), source_(std::move(source)) {
    const auto *mapping = catalog_.find(UiAudioKind::popup, economic_id);
    catalog_matches_ = mapping &&
        mapping->script_id == "AS2D_PLACEHOLDER_BLANK" &&
        catalog_.popup_source() &&
        catalog_.popup_source()->selected_name == source_.popup_audio_selected_name &&
        catalog_.popup_source()->mount_id == source_.popup_audio_mount_id;
}

EconomicAudioOpenResult EconomicOverviewAudioInteraction::gold_click(
    const runtime::LiveSession &session, const UiFrameSnapshot &frame,
    const EconomicGoldClick &measurement, std::int32_t x, std::int32_t y) {
    EconomicAudioOpenResult result;
    if (!source_.verified || !source_.popup_is_silent || !catalog_matches_ ||
        source_.popup_audio_xml_sha256.empty()) {
        result.popup.blocker = "Selected EconomicOverview audio source is unverified";
        return result;
    }
    result.popup = interaction_->gold_click(session, frame, measurement, x, y);
    if (result.popup.status == EconomicPopupQueueStatus::queued)
        result.audio = queue_.emit(UiAudioKind::popup, economic_id);
    return result;
}

EconomicCloseResult EconomicOverviewAudioInteraction::close(
    const runtime::LiveSession &session, const UiFrameSnapshot &frame,
    const EconomicCloseMeasurement &measurement, EconomicCloseInput input,
    std::int32_t x, std::int32_t y) {
    if (!source_.verified || !source_.popup_is_silent || !catalog_matches_)
        return {false, EconomicCloseStatus::unavailable,
                "Selected EconomicOverview audio source is unverified"};
    return interaction_->close(session, frame, measurement, input, x, y);
}

} // namespace civ5::ui
