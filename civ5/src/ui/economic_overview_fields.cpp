#include "civ5/ui/economic_overview_fields.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>

namespace civ5::ui {
namespace {
struct SelectedText {
    std::string text, path, sha256;
};

std::optional<SelectedText> selected_text(
    const assets::ResourceMountTable &resources, const char *key) {
    const auto chosen = resources.resolve(assets::AssetKey(key));
    const auto *handle = std::get_if<assets::AssetHandle>(&chosen.asset);
    if (!handle) return std::nullopt;
    const auto bytes = handle->read_bytes(4 * 1024 * 1024);
    const auto &source = handle->candidate().provenance;
    const auto path = source.archive_relative_path
        ? *source.archive_relative_path + ":" + source.source_relative_path
        : source.source_relative_path;
    return SelectedText{std::string(bytes.begin(), bytes.end()), path,
                        runtime::identify_bytes(bytes).sha256};
}

void collect_value_ids(const content::XmlNode &node,
    std::vector<std::string> &ids) {
    const auto id = node.attribute("ID");
    if (node.name == "Label" && id.ends_with("Value") &&
        std::find(ids.begin(), ids.end(), id) == ids.end())
        ids.push_back(id);
    for (const auto &child : node.children) collect_value_ids(child, ids);
}

void lua_bindings(std::string_view lua, const std::vector<std::string> &ids,
    std::set<std::string> &bound) {
    for (const auto &id : ids)
        if (lua.find("Controls." + id + ":SetText(") != std::string_view::npos)
            bound.insert(id);
}

std::optional<std::string> unique_row(const OverviewPage &page,
    std::string_view id) {
    std::optional<std::string> value;
    for (const auto &row : page.rows) {
        if (row.label != id) continue;
        if (value || row.value.empty()) return std::nullopt;
        value = row.value;
    }
    return value;
}
} // namespace

EconomicFieldSourceProof inspect_economic_field_source(
    const assets::ResourceMountTable &resources,
    const EconomicTabSourceProof &tab_source) {
    EconomicFieldSourceProof proof;
    if (!tab_source.verified || tab_source.lua_sha256.empty()) {
        proof.blocker = "EconomicOverview parent tab source is unverified";
        return proof;
    }
    try {
        const auto parent = selected_text(resources,
            "Assets/UI/InGame/Popups/EconomicOverview.lua");
        const auto general_xml = selected_text(resources,
            "Assets/UI/InGame/Popups/EconomicGeneralInfo.xml");
        const auto happiness_xml = selected_text(resources,
            "Assets/UI/InGame/Popups/HappinessInfo.xml");
        const auto general_lua = selected_text(resources,
            "Assets/UI/InGame/Popups/EconomicGeneralInfo.lua");
        const auto happiness_lua = selected_text(resources,
            "Assets/UI/InGame/Popups/HappinessInfo.lua");
        if (!parent || parent->sha256 != tab_source.lua_sha256 ||
            !general_xml || !happiness_xml || !general_lua || !happiness_lua) {
            proof.blocker = "Selected EconomicOverview child XML/Lua is missing or ambiguous";
            return proof;
        }
        const auto general = content::read_xml_metadata(general_xml->text);
        const auto happiness = content::read_xml_metadata(happiness_xml->text);
        if (general.name != "Context" || happiness.name != "Context") {
            proof.blocker = "Selected economic child XML has no Context root";
            return proof;
        }
        collect_value_ids(general, proof.general_ids);
        collect_value_ids(happiness, proof.happiness_ids);
        lua_bindings(general_lua->text, proof.general_ids,
                     proof.general_lua_bound);
        lua_bindings(happiness_lua->text, proof.happiness_ids,
                     proof.happiness_lua_bound);
        proof.positive_happiness_binding =
            happiness_lua->text.find(
                "Controls.TotalHappinessValue:SetText(\"[COLOR_POSITIVE_TEXT]\" .. pPlayer:GetHappiness()")
                != std::string::npos;
        if (std::find(proof.general_ids.begin(), proof.general_ids.end(),
                      "TotalGoldValue") == proof.general_ids.end() ||
            std::find(proof.happiness_ids.begin(), proof.happiness_ids.end(),
                      "TotalHappinessValue") == proof.happiness_ids.end() ||
            !proof.general_lua_bound.contains("TotalGoldValue") ||
            !proof.positive_happiness_binding) {
            proof.blocker = "Selected economic child controls differ from verified Lua bindings";
            return proof;
        }
        proof.general_xml_path = general_xml->path;
        proof.general_xml_sha256 = general_xml->sha256;
        proof.happiness_xml_path = happiness_xml->path;
        proof.happiness_xml_sha256 = happiness_xml->sha256;
        proof.general_lua_path = general_lua->path;
        proof.general_lua_sha256 = general_lua->sha256;
        proof.happiness_lua_path = happiness_lua->path;
        proof.happiness_lua_sha256 = happiness_lua->sha256;
        proof.verified = true;
    } catch (const std::exception &) {
        proof.blocker = "Selected economic child sources cannot be verified";
    }
    return proof;
}

EconomicOverviewFields::EconomicOverviewFields(
    EconomicOverviewLifecycle &lifecycle, EconomicOverviewActiveTabs &tabs,
    EconomicFieldSourceProof source)
    : lifecycle_(&lifecycle), tabs_(&tabs), source_(std::move(source)) {}

EconomicBoundFields EconomicOverviewFields::capture(
    const runtime::LiveSession &session, const UiFrameSnapshot &frame,
    std::int32_t viewport_width, std::int32_t viewport_height) const {
    EconomicBoundFields result;
    if (!source_.verified || source_.general_xml_sha256.empty() ||
        source_.happiness_xml_sha256.empty()) {
        result.blocker = "Selected economic child field sources are unverified";
        return result;
    }
    const auto owner = lifecycle_->current_owner(session, frame,
        viewport_width, viewport_height);
    const auto tab = tabs_->view(session, frame);
    const auto *overview = frame.find<OverviewFrameState>();
    if (!owner || !tab.available || !overview ||
        overview->player_id != owner->player_id ||
        frame.active_player_id() != owner->player_id ||
        frame.turn() != owner->turn || frame.frame() != owner->frame) {
        result.blocker = "EconomicOverview popup, active tab or live frame data is unavailable";
        return result;
    }
    result.available = true;
    result.tab = tab.selected;
    result.player_id = owner->player_id;
    result.turn = owner->turn;
    result.frame = owner->frame;
    result.owner_sequence = owner->owner_sequence;
    const bool general = tab.selected == EconomicTab::general;
    const auto &ids = general ? source_.general_ids : source_.happiness_ids;
    const auto &bindings = general ? source_.general_lua_bound
                                   : source_.happiness_lua_bound;
    const auto page = overview->pages.find(general ? "EconomicGeneralInfo"
                                               : "HappinessInfo");
    for (const auto &id : ids) {
        EconomicFieldValue field{id, std::nullopt, {}};
        if (!bindings.contains(id)) {
            field.unavailable_reason = "Selected Lua has no verified binding for this control";
        } else if (general && page != overview->pages.end()) {
            field.value = unique_row(page->second, id);
            if (!field.value)
                field.unavailable_reason = "No completed live value for this field";
        } else if (!general && id == "TotalHappinessValue") {
            // The native Lua reads GetHappiness(), while the current frame
            // only publishes GetExcessHappiness(). Reading LiveSession here
            // could mix state from after this immutable frame was sampled.
            field.unavailable_reason =
                "Frame-bound GetHappiness value is unavailable";
        } else {
            field.unavailable_reason = general
                ? "Live economic page is unavailable"
                : "No completed native happiness component for this control";
        }
        result.fields.push_back(std::move(field));
    }
    return result;
}

} // namespace civ5::ui
