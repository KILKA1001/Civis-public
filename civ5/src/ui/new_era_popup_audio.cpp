#include "civ5/ui/new_era_popup_audio.hpp"
#include "civ5/runtime/file_identity.hpp"

namespace civ5::ui {
namespace {
constexpr auto popup_id = "BUTTONPOPUP_NEW_ERA";
constexpr auto script_id = "AS2D_INTERFACE_NEW_ERA";

std::optional<std::string> bytes_text(const assets::AssetHandle &handle,
    std::uint64_t maximum_bytes, std::string &sha) {
    const auto bytes = handle.read_bytes(maximum_bytes);
    sha = runtime::identify_bytes(bytes).sha256;
    return std::string(bytes.begin(), bytes.end());
}

const content::XmlNode *unique_child(const content::XmlNode &root,
    std::string_view element, std::string_view child_name,
    std::string_view child_text) {
    const content::XmlNode *found = nullptr;
    for (const auto &node : root.children) {
        if (node.name != element) continue;
        const auto *child = node.child(std::string(child_name));
        if (!child || child->text != child_text) continue;
        if (found) return nullptr;
        found = &node;
    }
    return found;
}

bool context_registered(const content::XmlNode &root) {
    std::size_t count = 0;
    const auto visit = [&](const auto &self, const content::XmlNode &node) -> void {
        if (node.name == "LuaContext" && node.attribute("ID") == "NewEraPopup" &&
            node.attribute("FileName") ==
                "Assets/UI/InGame/Popups/NewEraPopup") ++count;
        for (const auto &child : node.children) self(self, child);
    };
    visit(visit, root);
    return count == 1;
}

bool has_unique_id(const content::XmlNode &root, std::string_view id) {
    std::size_t count = 0;
    const auto visit = [&](const auto &self, const content::XmlNode &node) -> void {
        if (node.attribute("ID") == id) ++count;
        for (const auto &child : node.children) self(self, child);
    };
    visit(visit, root);
    return count == 1;
}

bool ordered(std::string_view text,
    std::initializer_list<std::string_view> tokens) {
    std::size_t cursor = 0;
    for (const auto token : tokens) {
        const auto at = text.find(token, cursor);
        if (at == std::string_view::npos) return false;
        cursor = at + token.size();
    }
    return true;
}
} // namespace

NewEraAudioSourceProof inspect_new_era_audio_source(
    const assets::ResourceMountTable &resources, const UiAudioCatalog &catalog) {
    NewEraAudioSourceProof proof;
    const auto in_game = resources.resolve(assets::AssetKey(
        "Assets/UI/InGame/InGame.xml"));
    const auto lua = resources.resolve(assets::AssetKey(
        "Assets/UI/InGame/Popups/NewEraPopup.lua"));
    const auto xml = resources.resolve(assets::AssetKey(
        "Assets/UI/InGame/Popups/NewEraPopup.xml"));
    const auto audio = resources.resolve(assets::AssetKey(
        "Assets/Sounds/XML/ButtonPopupSounds.xml"));
    const auto *ig = std::get_if<assets::AssetHandle>(&in_game.asset);
    const auto *l = std::get_if<assets::AssetHandle>(&lua.asset);
    const auto *x = std::get_if<assets::AssetHandle>(&xml.asset);
    const auto *a = std::get_if<assets::AssetHandle>(&audio.asset);
    const auto *mapping = catalog.find(UiAudioKind::popup, popup_id);
    if (!ig || !l || !x || !a || !mapping || !catalog.popup_source()) {
        proof.blocker = "Selected NewEra context or popup audio source is missing or ambiguous";
        return proof;
    }
    try {
        const auto in_game_text = bytes_text(*ig, 8 * 1024 * 1024,
            proof.in_game_sha256);
        const auto lua_text = bytes_text(*l, 1024 * 1024, proof.lua_sha256);
        const auto xml_text = bytes_text(*x, 8 * 1024 * 1024, proof.xml_sha256);
        const auto audio_text = bytes_text(*a, 4 * 1024 * 1024,
            proof.popup_audio_sha256);
        const auto in_game_doc = content::read_xml_metadata(*in_game_text);
        const auto xml_doc = content::read_xml_metadata(*xml_text);
        const auto audio_doc = content::read_xml_metadata(*audio_text);
        const auto *row = unique_child(audio_doc, "ButtonPopupSound",
            "ID", popup_id);
        const auto *audio_script = row ? row->child("AudioScript") : nullptr;
        std::size_t ordinal = 0;
        for (const auto &candidate : audio_doc.children) {
            if (candidate.name != "ButtonPopupSound") continue;
            if (&candidate == row) break;
            ++ordinal;
        }
        if (in_game_doc.name != "Context" || !context_registered(in_game_doc) ||
            xml_doc.name != "Context" ||
            !has_unique_id(xml_doc, "EraImage") ||
            !has_unique_id(xml_doc, "DescriptionLabel") ||
            !has_unique_id(xml_doc, "CloseButton") ||
            audio_doc.name != "ButtonPopupSounds" || !audio_script ||
            audio_script->text != script_id ||
            mapping->script_id != script_id ||
            mapping->ordinal != ordinal ||
            catalog.popup_source()->selected_name != a->candidate().original_name ||
            catalog.popup_source()->mount_id != audio.mount_id ||
            ig->candidate().provenance.source_relative_path.find(
                "Assets/DLC/Expansion2/") == std::string::npos ||
            l->candidate().provenance.source_relative_path.find(
                "Assets/DLC/Expansion2/") == std::string::npos ||
            x->candidate().provenance.source_relative_path.find(
                "Assets/DLC/Expansion2/") == std::string::npos ||
            !ordered(*lua_text, {"function OnPopup( popupInfo )",
                "BUTTONPOPUP_NEW_ERA", "popupInfo.Data1",
                "UIManager:QueuePopup( ContextPtr, PopupPriority.NewEraPopup )",
                "Events.SerialEventGameMessagePopup.Add( OnPopup )",
                "function ShowHideHandler( bIsHide, bInitState )",
                "Events.SerialEventGameMessagePopupShown(m_PopupInfo)"}) ||
            lua_text->find("AudioPlay2DSound") != std::string::npos) {
            proof.blocker = "Selected NewEra popup or authored audio mapping differs from BNW source";
            return proof;
        }
        proof.lua_selected_name = l->candidate().original_name;
        proof.xml_selected_name = x->candidate().original_name;
        proof.popup_audio_selected_name = a->candidate().original_name;
        proof.popup_audio_mount_id = audio.mount_id;
        proof.popup_audio_ordinal = mapping->ordinal;
        proof.verified = true;
    } catch (const std::exception &) {
        proof.blocker = "Selected NewEra popup/audio sources cannot be read";
    }
    return proof;
}

NewEraAudioPlanResult NewEraPopupAudioPlanner::shown(
    const UiFrameSnapshot &frame, const NewEraPopupShownReceipt &receipt) {
    NewEraAudioPlanResult result;
    const auto *popup = frame.find<PopupFrame>();
    if (!source_.verified || !frame.active_player_id() ||
        !frame.turn() || !popup ||
        receipt.player_id != *frame.active_player_id() ||
        receipt.player_id != popup->player_id ||
        receipt.turn != *frame.turn() || receipt.turn != popup->turn ||
        receipt.frame != frame.frame() ||
        !receipt.owner_sequence ||
        receipt.owner_sequence != popup->owner_sequence ||
        receipt.event_id < 0 || !receipt.cue_sequence ||
        popup->events.size() != 1 ||
        popup->events.front().context_id != "NewEraPopup" ||
        popup->events.front().event_id != receipt.event_id) {
        result.blocker = "Current owner-issued NewEra PopupShown receipt is absent or stale";
        return result;
    }
    if (popup->events.front().audio_cue) {
        result.blocker = "NewEra audio cue is already delegated to the popup host";
        return result;
    }
    const auto key = std::tuple{receipt.player_id, receipt.turn,
        receipt.event_id, receipt.owner_sequence, receipt.cue_sequence};
    if (delivered_.contains(key)) {
        result.duplicate = true;
        return result;
    }
    delivered_.insert(key);
    result.cue = UiAuthoredAudioCue{receipt.cue_sequence, popup_id};
    return result;
}

} // namespace civ5::ui
