#include "civ5/ui/ui_runtime.hpp"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace civ5::ui {
namespace {

bool excluded_from_single_player(std::string_view id, std::string_view source) {
    return id == "StagingRoom" || id == "MPTurnPanel" || id == "MPList" ||
           id == "DebugMenu" || id == "NetworkDebug" ||
           source.find("/Multiplayer/") != std::string_view::npos;
}

std::string context_id(const content::XmlNode &node) {
    auto id = node.attribute("ID");
    if (!id.empty()) return id;
    const auto source = node.attribute("FileName");
    const auto slash = source.find_last_of("/\\");
    return source.substr(slash == std::string::npos ? 0 : slash + 1);
}

UiContextOwner owner_of(std::string_view id, std::string_view source) {
    if (id == "TopPanel") return UiContextOwner::top_panel;
    if (id == "MiniMapPanel") return UiContextOwner::minimap;
    if (id == "SimpleDiplo") return UiContextOwner::popup;
    if (id == "TechTree" || id == "SocialPolicyPopup" || id == "ChooseIdeology" ||
        id == "TechPopup")
        return UiContextOwner::progression;
    if (id == "CityView" || id == "ProductionPopup") return UiContextOwner::city;
    if (id == "CivilopediaScreen" || id == "GameMenu" || id == "EndGameMenu" ||
        id == "ExitConfirm" || id == "PlayerChange" || id == "ContentSwitch" ||
        id == "LoadMenu" || id == "SaveMenu" || id == "SaveMapMenu" ||
        id == "OptionsMenu_InGame" || id == "TouchControlsMenu" ||
        id == "ChangePassword")
        return UiContextOwner::system;
    if (source.find("Overview") != std::string_view::npos || id == "Demographics" ||
        id == "VictoryProgress" || id == "WhosWinningPopup" ||
        id == "EconomicGeneralInfo" || id == "HappinessInfo" ||
        id == "DealsPanel" || id == "RelationsPanel" || id == "GlobalPanel" ||
        id == "Ranking" || id == "EndGameReplay" || id == "EndGameDemographics")
        return UiContextOwner::overview;
    if (source.find("/WorldView/") != std::string_view::npos ||
        (source.find("/Popups/") == std::string_view::npos &&
            (id == "WorldView" || id == "YieldIconManager" || id == "PlotHelpManager" ||
             id == "ResourceIconManager" || id == "GenericWorldAnchor" ||
             id == "UnitFlagManager" || id == "CityBannerManager" ||
             id == "UnitMemberOverlay" || id == "InfoCorner" || id == "NewTurn" ||
             id == "TurnProcessing" || id == "TaskList" || id == "Tutorial" ||
             id == "UnitList" || id == "CityList" || id == "ResourceList" ||
             id == "GPList" || id == "TechPanel" || id == "DiploList")))
        return UiContextOwner::world_view;
    return UiContextOwner::popup;
}

bool hidden_attribute(std::string_view value) {
    return value == "True" || value == "true" || value == "1";
}

content::XmlNode read_selected_xml(const assets::ResourceMountTable &resources, std::string_view key) {
    const auto mounted = resources.resolve(assets::AssetKey(std::string(key)));
    const auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset);
    if (!handle) throw std::runtime_error("UI context XML is missing or ambiguous: " + std::string(key));
    const auto bytes = handle->read_bytes(64ULL * 1024 * 1024);
    return content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
}

} // namespace

UiFrameSnapshot::UiFrameSnapshot(std::uint64_t frame,
                                 std::optional<std::int64_t> active_player_id,
                                 std::optional<std::int32_t> turn)
    : frame_(frame), active_player_id_(active_player_id), turn_(turn) {}

UiGameplayCommand::UiGameplayCommand(std::string source_id, std::type_index type,
                                     std::shared_ptr<const void> payload)
    : source_id_(std::move(source_id)), type_(type), payload_(std::move(payload)) {}

UiContextRegistry UiContextRegistry::from_documents(const content::XmlNode &in_game,
                                                      const content::XmlNode &world_view) {
    if (in_game.name != "Context" || world_view.name != "Context")
        throw std::runtime_error("BNW UI context roots must be Context elements");
    UiContextRegistry result;
    std::unordered_map<std::string, std::string> sources_by_id;
    bool inserted_world_view = false;
    auto walk = [&](auto &&self, const content::XmlNode &node,
                    const std::vector<std::string> &parents, bool expand_world) -> void {
        std::vector<std::string> path = parents;
        const auto element_id = node.attribute("ID");
        path.push_back(element_id.empty() ? node.name : node.name + "#" + element_id);
        if (node.name == "LuaContext") {
            const auto source = node.attribute("FileName");
            if (source.empty()) throw std::runtime_error("UI LuaContext has no FileName");
            const auto id = context_id(node);
            if (id.empty()) throw std::runtime_error("UI LuaContext has no usable ID");
            if (!excluded_from_single_player(id, source)) {
                const auto [found, inserted] = sources_by_id.emplace(id, source);
                if (!inserted && found->second != source)
                    throw std::runtime_error("Duplicate BNW UI context ID with different source: " + id);
                // Some shared popup XML is instantiated under more than one
                // parent. Keep one typed route for an identical source.
                if (inserted) {
                    const bool modal = source.find("/Popups/") != std::string::npos ||
                                       id.find("Popup") != std::string::npos ||
                                       id == "GameMenu" || id == "SaveMenu" ||
                                       id == "PlayerChange" || id == "ExitConfirm" ||
                                       id == "SimpleDiplo";
                    result.contexts_.push_back({id, source, path, owner_of(id, source),
                                                hidden_attribute(node.attribute("Hidden")), modal});
                }
                if (expand_world && id == "WorldView") {
                    if (inserted_world_view) throw std::runtime_error("Duplicate WorldView insertion");
                    inserted_world_view = true;
                    for (const auto &child : world_view.children) self(self, child, path, false);
                }
            }
        }
        for (const auto &child : node.children) self(self, child, path, expand_world);
    };
    walk(walk, in_game, {}, true);
    if (!inserted_world_view) throw std::runtime_error("BNW InGame context has no WorldView");
    return result;
}

UiContextRegistry UiContextRegistry::from_resources(const assets::ResourceMountTable &resources) {
    auto in_game = read_selected_xml(resources, "Assets/UI/InGame/InGame.xml");
    auto world_view = read_selected_xml(resources, "Assets/UI/InGame/WorldView/WorldView.xml");
    // LuaContext XML can itself declare child LuaContexts. InGame embeds
    // WorldView separately below, but panels such as InfoCorner and
    // ActionInfoPanel also own active single-player screens.
    auto hydrate = [&](auto &&self, content::XmlNode &node,
                       std::vector<std::string> &source_stack) -> void {
        bool pushed = false;
        if (node.name == "LuaContext") {
            const auto source = node.attribute("FileName");
            const auto id = context_id(node);
            if (!source.empty() && id != "WorldView" &&
                !excluded_from_single_player(id, source)) {
                const auto key = source + ".xml";
                if (std::find(source_stack.begin(), source_stack.end(), key) != source_stack.end())
                    throw std::runtime_error("Cycle in BNW UI context XML: " + key);
                const auto mounted = resources.resolve(assets::AssetKey(key));
                if (const auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset)) {
                    const auto bytes = handle->read_bytes(64ULL * 1024 * 1024);
                    auto document = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
                    if (document.name != "Context")
                        throw std::runtime_error("BNW UI context XML has no Context root: " + key);
                    source_stack.push_back(key);
                    pushed = true;
                    for (auto &child : document.children)
                        node.children.push_back(std::move(child));
                } else if (std::holds_alternative<assets::AmbiguousAsset>(mounted.asset)) {
                    throw std::runtime_error("Ambiguous BNW UI context XML: " + key);
                }
            }
        }
        for (auto &child : node.children) self(self, child, source_stack);
        if (pushed) source_stack.pop_back();
    };
    std::vector<std::string> stack;
    hydrate(hydrate, in_game, stack);
    hydrate(hydrate, world_view, stack);
    return from_documents(in_game, world_view);
}

const UiContextDescriptor *UiContextRegistry::find(std::string_view id) const {
    const auto it = std::find_if(contexts_.begin(), contexts_.end(),
                                 [id](const auto &context) { return context.id == id; });
    return it == contexts_.end() ? nullptr : &*it;
}

UiScreenController::UiScreenController(UiContextRegistry registry) : registry_(std::move(registry)) {
    if (!registry_.find("WorldView")) throw std::runtime_error("UI screen controller needs WorldView");
}

UiCommandResult UiScreenController::dispatch(const UiCommand &command,
                                              const UiFrameSnapshot &frame,
                                              UiGameplayBackend *backend) {
    if (const auto *open = std::get_if<UiOpenScreen>(&command)) {
        if (!registry_.find(open->id)) return {UiCommandStatus::invalid, "Unknown UI screen"};
        if (std::find(opened_.begin(), opened_.end(), open->id) != opened_.end())
            return {UiCommandStatus::navigation, {}};
        opened_.push_back(open->id);
        return {UiCommandStatus::navigation, {}};
    }
    if (const auto *close = std::get_if<UiCloseScreen>(&command)) {
        if (opened_.empty() || opened_.back() != close->id)
            return {UiCommandStatus::invalid, "Only the focused UI screen can close"};
        opened_.pop_back();
        return {UiCommandStatus::navigation, {}};
    }
    const auto &gameplay = std::get<UiGameplayCommand>(command);
    if (!registry_.find(gameplay.source_id()))
        return {UiCommandStatus::invalid, "Gameplay command has an unknown UI source"};
    if (!visible(gameplay.source_id()))
        return {UiCommandStatus::invalid, "Gameplay command source is hidden"};
    if (const auto focus = focused_screen()) {
        const auto *focused = registry_.find(*focus);
        if (focused && focused->modal && gameplay.source_id() != *focus)
            return {UiCommandStatus::invalid, "A modal UI screen owns input focus"};
    }
    if (!backend) return {UiCommandStatus::unavailable, "Gameplay backend is not connected"};
    const auto available = backend->availability(gameplay, frame);
    if (available.status != UiCommandStatus::ready)
        return (available.status == UiCommandStatus::unavailable || available.status == UiCommandStatus::uncertain) ? available
               : UiCommandResult{UiCommandStatus::invalid, "Gameplay backend gave no ready decision"};
    const auto executed = backend->execute(gameplay, frame);
    if (executed.status == UiCommandStatus::performed ||
        executed.status == UiCommandStatus::pending_refresh ||
        executed.status == UiCommandStatus::uncertain ||
        executed.status == UiCommandStatus::unavailable) return executed;
    return {UiCommandStatus::invalid, "Gameplay backend gave no execution result"};
}

bool UiScreenController::visible(std::string_view id) const {
    const auto *context = registry_.find(id);
    if (!context) return false;
    const bool world_overlay = context->owner == UiContextOwner::top_panel ||
                               context->owner == UiContextOwner::minimap ||
                               context->owner == UiContextOwner::world_view;
    return (world_overlay && !context->initially_hidden) ||
           std::find(opened_.begin(), opened_.end(), id) != opened_.end();
}

std::optional<std::string> UiScreenController::focused_screen() const {
    if (opened_.empty()) return std::nullopt;
    return opened_.back();
}

} // namespace civ5::ui
