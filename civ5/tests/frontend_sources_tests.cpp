#include "civ5/ui/frontend_sources.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace civ5;
namespace {
unsigned checks = 0;
void need(bool value, const char *message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
void write(const fs::path &path, std::string_view data) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << data;
    if (!file) throw std::runtime_error("Cannot write frontend source fixture");
}
bool diagnostic(const ui::FrontendSourceCatalog &catalog, std::string_view value) {
    return std::any_of(catalog.diagnostics().begin(), catalog.diagnostics().end(),
        [&](const auto &line) { return line.find(value) != std::string::npos; });
}
void synthetic(const fs::path &root) {
    write(root / "Root.xml", R"(<Context Name="Root"><Container ID="Panel" Hidden="1">
        <GridButton ID="Open" Disabled="true" Style="ZoomButton"><Stack><Label String="TXT_KEY_OPEN"/></Stack></GridButton>
        <TextButton ID="TooltipOnly" ToolTip="TXT_KEY_HELP"/>
        <Label String="TXT_KEY_AUTOSAVE_INTERVAL"><Box><EditBox ID="ParentCaption"/></Box></Label>
        <Slider ID="ChildTooltip"><Label ToolTip="TXT_KEY_DELAY"/></Slider>
        <GridButton ID="StringBeforeTooltip"><Label ToolTip="TXT_KEY_WRONG_TT"/><Stack><Label String="TXT_KEY_RIGHT"/></Stack></GridButton>
        </Container><Instance Name="Row"><Button ID="TemplateButton"/>
        <LuaContext FileName="IgnoredTemplate"/></Instance>
        <LuaContext FileName="Assets/UI/A" ID="AScreen" Hidden="1" DeferLoad="1"/>
        <LuaContext FileName="Missing"/><LuaContext FileName="Broken"/>
        <LuaContext FileName="Ambiguous"/><LuaContext FileName="InGame"/>
        <LuaContext FileName="UITestMenu"/></Context>)");
    write(root / "Root.lua", "Controls.Open:RegisterCallback(Mouse.eLClick, Open)\n");
    write(root / "A.xml", R"(<Context><Button ID="Old"/><LuaContext FileName="Root"/></Context>)");
    write(root / "override/A.XML", R"(<Context><Button ID="New" String="NEW"/><LuaContext FileName="Root"/></Context>)");
    write(root / "Broken.xml", "<Context><Button></Context>");
    auto index = std::make_shared<assets::LooseResourceIndex>(root);
    index->add_file("Root.xml"); index->add_file("Root.lua");
    index->add_file("A.xml"); index->add_file("Broken.xml");
    auto overlay = std::make_shared<assets::LooseResourceIndex>(root);
    overlay->add_file("override/A.XML");
    assets::ResourceMountTable table(assets::resource_lookup(index), {}, assets::resource_keys(index));
    table.mount({17, 0, 0, assets::resource_lookup(overlay), true, true, {}, assets::resource_keys(overlay)});
    table.mount({18, 0, 0, [](const assets::AssetKey &key) -> assets::ResourceMountHit {
        if (assets::resource_basename(key.original_name()) != "ambiguous.xml") return {};
        return {assets::AmbiguousAsset{key, {assets::SourceKind::loose, "fixture", "Ambiguous.xml", {}}, {}}, true};
    }, true, true, {}, {}});
    const auto catalog = ui::FrontendSourceCatalog::from_resources(table, {}, "Assets/UI/Root");
    need(catalog.screens().size() == 2, "Cycle, broken XML or missing resource created a screen");
    const auto *screen = catalog.find("ROOT.XML");
    need(screen && screen->document.name == "Context", "Source document not preserved");
    const auto *button = screen->find_control("Open");
    need(button && button->interactive && button->text_key == "TXT_KEY_OPEN" && button->style == "ZoomButton",
        "Authored button caption/style lost");
    need(button->static_hidden && button->static_disabled && !button->static_state_reason.empty(),
        "Inherited static state observation lost");
    need(button->ancestor_ids == std::vector<std::string>{"Panel"}, "Control ancestry lost");
    need(screen->find_control("TooltipOnly")->interactive &&
        screen->find_control("TooltipOnly")->text_key == "TXT_KEY_HELP", "TextButton/tooltip caption lost");
    need(screen->find_control("ParentCaption")->text_key == "TXT_KEY_AUTOSAVE_INTERVAL",
        "Input did not inherit its enclosing authored Label.String");
    need(screen->find_control("ChildTooltip")->text_key == "TXT_KEY_DELAY" &&
        screen->find_control("StringBeforeTooltip")->text_key == "TXT_KEY_RIGHT",
        "Recursive child tooltip fallback ignored authored String precedence");
    need(!screen->find_control("TemplateButton") && screen->contexts.size() == 6,
        "Template prototypes were treated as commands or contexts");
    need(screen->contexts.front().hidden && screen->contexts.front().defer_load,
        "Authored context flags lost");
    need(screen->lua_text.find("RegisterCallback") != std::string::npos, "Selected Lua source missing");
    need(catalog.find("A")->mount_id == 17 && catalog.find("A")->find_control("New") &&
        !catalog.find("A")->find_control("Old"), "Authoritative mounted selection ignored");
    need(diagnostic(catalog, "missing selected") && diagnostic(catalog, "ambiguous selected") &&
        diagnostic(catalog, "broken.xml") && diagnostic(catalog, "game-world context excluded"),
        "Unavailable/malformed/excluded sources need diagnostics");
    need(!diagnostic(catalog, "ignoredtemplate"), "Template context followed");
    ui::FrontendSourceBudget tiny;
    tiny.maximum_screens = 1;
    need(diagnostic(ui::FrontendSourceCatalog::from_resources(table, tiny, "Root"), "screen budget"),
        "Screen traversal not bounded");
    tiny = {}; tiny.maximum_nodes_per_screen = 2;
    need(diagnostic(ui::FrontendSourceCatalog::from_resources(table, tiny, "Root"), "node/depth budget"),
        "Node traversal not bounded");
    tiny = {}; tiny.maximum_depth = 1;
    need(diagnostic(ui::FrontendSourceCatalog::from_resources(table, tiny, "Root"), "node/depth budget"),
        "Depth traversal not bounded");
    tiny = {}; tiny.maximum_resource_bytes = 8;
    need(diagnostic(ui::FrontendSourceCatalog::from_resources(table, tiny, "Root"), "byte budget"),
        "Resource reads not bounded");
    tiny = {}; tiny.maximum_total_bytes = fs::file_size(root / "Root.xml");
    const auto total = ui::FrontendSourceCatalog::from_resources(table, tiny, "Root");
    need(total.find("Root") && !total.find("A") && diagnostic(total, "byte budget"),
        "Aggregate budget not enforced");
    write(root / "Assets/DLC/UI_bc1/Root.xml", "<Context><Button ID=\"EUI\"/></Context>");
    auto eui = std::make_shared<assets::LooseResourceIndex>(root);
    eui->add_file("Assets/DLC/UI_bc1/Root.xml");
    table.mount({19, 0, 0, assets::resource_lookup(eui), true, true, {}, {}});
    const auto excluded = ui::FrontendSourceCatalog::from_resources(table, {}, "Root");
    need(excluded.screens().empty() && diagnostic(excluded, "EUI/VPUI"), "Baseline accepted EUI override");
    table.set_enabled(19, false);
    need(ui::FrontendSourceCatalog::from_resources(table, {}, "Root").find("Root") != nullptr,
        "Disabled override changed selection");
    table.set_default_enabled(false);
    need(ui::FrontendSourceCatalog::from_resources(table, {}, "Root").screens().empty(),
        "Catalog bypassed resource table activation gate");
}
void actual(const fs::path &root) {
    // Explicit immutable extracted fixture roots. Production never constructs
    // this diagnostic selection; it receives the already active resource table.
    auto base = std::make_shared<assets::LooseResourceIndex>(root);
    base->scan({"Assets/UI", {}, false});
    assets::ResourceMountTable table(assets::resource_lookup(base), {}, assets::resource_keys(base));
    unsigned id = 1;
    for (const auto &path : {"Assets/DLC/Expansion/UI", "Assets/DLC/Expansion2/UI", "Assets/DLC/Shared/UI"}) {
        auto dlc = std::make_shared<assets::LooseResourceIndex>(root);
        dlc->scan({path, {}, false});
        table.mount({id++, 0, 0, assets::resource_lookup(dlc), true, true, {}, assets::resource_keys(dlc)});
    }
    const auto catalog = ui::FrontendSourceCatalog::from_resources(table);
    const auto control = [&](const char *screen, const char *id) -> const ui::FrontendControlSource & {
        const auto *source = catalog.find(screen);
        need(source != nullptr, "Actual frontend screen missing");
        const auto *value = source->find_control(id);
        need(value != nullptr, "Actual frontend control missing");
        return *value;
    };
    need(control("MainMenu", "SinglePlayerButton").style == "ZoomButton", "Main menu source style lost");
    control("MainMenu", "ExpansionRulesSwitch"); control("SinglePlayer", "ScenariosButton");
    control("SinglePlayer", "LoadTutorialButton"); control("GameSetupScreen", "AdvancedButton");
    control("AdvancedSetup", "MaxTurnsEdit"); control("LoadMenu", "CloudCheck");
    control("OtherMenu", "LeaderboardButton"); control("OptionsMenu", "TurnNotifySmtpPassEdit");
    control("ModsBrowser", "LargeButton"); control("MultiplayerSelect", "PitbossButton");
    const auto &reconnect = control("MultiplayerSelect", "ReconnectButton");
    need(reconnect.static_hidden && reconnect.static_disabled, "Reconnect XML observation wrong");
    control("ExitConfirm", "Yes");
    need(catalog.find("CivilopediaScreen") && catalog.find("ReplayViewer") && catalog.find("PremiumContentMenu"),
        "Deferred frontend children not discovered");
    const auto &music = control("OptionsMenu", "MusicVolumeSlider");
    need(std::find(music.ancestor_ids.begin(), music.ancestor_ids.end(), "AudioPanel") != music.ancestor_ids.end(),
        "Option page ancestry lost");
    need(!catalog.find("LoadMenu")->find_control("InstanceRoot"), "Actual save template treated as a command");
    need(catalog.find("MainMenu")->lua_text.find("ActivateDLC") != std::string::npos,
        "Original lifecycle source unavailable");
    need(!diagnostic(catalog, "EUI/VPUI") && !diagnostic(catalog, "budget exceeded"),
        "Actual baseline unexpectedly rejected or truncated");
    std::cout << "Actual source screens: " << catalog.screens().size() << '\n';
    for (const auto &line : catalog.diagnostics()) std::cout << "Source observation: " << line << '\n';
}
} // namespace
int main(int argc, char **argv) {
    const auto root = fs::current_path() / "fixtures" /
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    try {
        fs::create_directories(root);
        synthetic(root);
        if (argc > 1) actual(fs::path(argv[1]));
        fs::remove_all(root);
        std::cout << "Frontend source checks passed: " << checks << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
