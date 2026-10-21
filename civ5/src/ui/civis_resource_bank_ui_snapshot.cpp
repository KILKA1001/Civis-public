#include "civ5/ui/civis_resource_bank_ui_snapshot.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>
namespace civ5::ui {
namespace {
bool excluded(std::string_view id, std::string_view source) {
    return id == "StagingRoom" || id == "MPTurnPanel" || id == "MPList" ||
           id == "DebugMenu" || id == "NetworkDebug" ||
           source.find("/Multiplayer/") != std::string_view::npos;
}
std::string context_id(const content::XmlNode& node) {
    auto id = node.attribute("ID");
    if (!id.empty()) return id;
    const auto source = node.attribute("FileName");
    const auto slash = source.find_last_of("/\\");
    return source.substr(slash == std::string::npos ? 0 : slash + 1);
}
}
CivisResourceBankUiSnapshot::CivisResourceBankUiSnapshot(
    std::shared_ptr<const assets::CivisResourceBankOwner> bank,
    UiContextRegistry registry, WorldViewSkin skin,
    std::vector<CivisResourceBankUiDocument> sources)
    : bank_(std::move(bank)), capability_(bank_->capability()),
      registry_(std::move(registry)), world_view_skin_(std::move(skin)),
      sources_(std::move(sources)) {
    require_current();
}
std::shared_ptr<const CivisResourceBankUiSnapshot> CivisResourceBankUiSnapshot::capture(
    const std::shared_ptr<assets::CivisResourceBankOwner>& supplied) {
    if (!supplied) throw std::invalid_argument("Civis UI snapshot requires an actual resource bank");
    const auto bank = supplied->retain_owner();
    if (!bank || bank.get() != supplied.get() || bank.owner_before(supplied) ||
        supplied.owner_before(bank))
        throw std::invalid_argument("Civis UI snapshot requires the canonical bank control block");
    bank->require_boundary();
    const auto capability = bank->capability();
    std::vector<CivisResourceBankUiDocument> sources;
    auto document = [&](const std::string& key) {
        bank->load(assets::AssetKey(key), assets::CivisResourceParse::xml);
        const auto loaded = capability.loaded(assets::AssetKey(key));
        if (!loaded.xml || loaded.xml->name != "Context")
            throw std::runtime_error("Civis UI XML has no actual Context root: " + key);
        if (std::none_of(sources.begin(), sources.end(),
            [&](const auto& source) { return source.requested_key == key; }))
            sources.push_back({key, loaded.candidate, loaded.payload_identity});
        return *loaded.xml;
    };
    auto in_game = document("Assets/UI/InGame/InGame.xml");
    auto world_view = document("Assets/UI/InGame/WorldView/WorldView.xml");
    // Match the existing registry's authored single-player hydration. Optional
    // absent declarations remain absent; ambiguity and cycles are not hidden.
    std::vector<std::string> stack;
    auto hydrate = [&](auto&& self, content::XmlNode& node) -> void {
        bool pushed = false;
        if (node.name == "LuaContext") {
            const auto source = node.attribute("FileName");
            const auto id = context_id(node);
            if (!source.empty() && id != "WorldView" && !excluded(id, source)) {
                const auto key = source + ".xml";
                if (std::find(stack.begin(), stack.end(), key) != stack.end())
                    throw std::runtime_error("Cycle in Civis UI context XML: " + key);
                if (bank->has_indexed(assets::AssetKey(key))) {
                    if (stack.size() >= 128 || sources.size() >= 4096)
                        throw std::runtime_error("Civis UI context source closure exceeds its bound");
                    auto nested = document(key);
                    stack.push_back(key);
                    pushed = true;
                    for (auto& child : nested.children)
                        node.children.push_back(std::move(child));
                }
            }
        }
        for (auto& child : node.children) self(self, child);
        if (pushed) stack.pop_back();
    };
    hydrate(hydrate, in_game);
    hydrate(hydrate, world_view);
    auto registry = UiContextRegistry::from_documents(in_game, world_view);
    for (const auto& context : registry.contexts())
        if (context.owner == UiContextOwner::world_view)
            (void)document(context.source_key + ".xml");
    (void)document("Assets/UI/InGame/WorldView/NotificationPanel.xml");
    // WorldViewSkin also consumes authored color and city shadow sources.
    // Stage these real optional inputs before its memory-only VFS reads.
    auto optional_source = [&](const char* key, assets::CivisResourceParse parse) {
        const assets::AssetKey request(key);
        if (!bank->has_indexed(request)) return;
        bank->load(request, parse);
        const auto loaded = capability.loaded(request);
        if (std::none_of(sources.begin(), sources.end(),
            [&](const auto& source) { return source.requested_key == key; }))
            sources.push_back({key, loaded.candidate, loaded.payload_identity});
    };
    optional_source("Assets/UI/ColorAtlas.xml", assets::CivisResourceParse::bytes);
    optional_source("Assets/UI/InGame/CityBannerManager.lua", assets::CivisResourceParse::bytes);
    capability.require_current();
    auto skin = WorldViewSkin::from_resources(bank->resources(capability), registry);
    // Every actual read is outside a live Lua/client operation. Publish this
    // immutable UI object only after the complete bank/source POST boundary.
    bank->require_boundary();
    capability.require_current();
    auto result = std::shared_ptr<CivisResourceBankUiSnapshot>(
        new CivisResourceBankUiSnapshot(bank, std::move(registry), std::move(skin),
                                       std::move(sources)));
    result->canonical_self_ = result;
    bank->require_boundary();
    result->require_current();
    return result;
}
std::shared_ptr<const CivisResourceBankUiSnapshot>
CivisResourceBankUiSnapshot::retain_owner() const {
    require_current();
    const auto owner = canonical_self_.lock();
    if (!owner || owner.get() != this)
        throw std::runtime_error("Civis UI snapshot canonical factory owner retired");
    return owner;
}
void CivisResourceBankUiSnapshot::require_current() const {
    capability_.require_current();
    bank_->require_current();
}
void CivisResourceBankUiSnapshot::require_boundary() const {
    // Keep actual storage alive across the real client callback. This local
    // borrow must not substitute for the last outside snapshot owner.
    const auto actual = retain_owner();
    if (actual.use_count() <= 1)
        throw std::runtime_error("Civis UI boundary has no outside snapshot owner");
    const auto bank = actual->bank_;
    bank->require_boundary();
    if (actual.use_count() <= 1)
        throw std::runtime_error("Civis UI snapshot retired during source boundary");
    actual->require_current();
}
const UiContextRegistry& CivisResourceBankUiSnapshot::registry() const {
    require_current(); return registry_;
}
const WorldViewSkin& CivisResourceBankUiSnapshot::world_view_skin() const {
    require_current(); return world_view_skin_;
}
const std::vector<CivisResourceBankUiDocument>&
CivisResourceBankUiSnapshot::document_sources() const {
    require_current(); return sources_;
}
void CivisResourceBankUiSnapshot::require_same_bank(
    const assets::CivisResourceBankOwner& bank) const {
    require_current();
    if (bank_.get() != &bank)
        throw std::runtime_error("Civis UI snapshot belongs to another actual bank");
}
} // namespace civ5::ui
