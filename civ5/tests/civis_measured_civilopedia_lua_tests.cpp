#include "civ5/ui/civis_measured_civilopedia_lua.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace civ5::ui;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
std::string first_layout_block(const std::filesystem::path &root) {
    std::ifstream input(root / "Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    const std::string first = "local _, screenSizeY = UIManager:GetScreenSizeVal();";
    const std::string last = "Controls.LeftScrollPanel:CalculateInternalSize();";
    const auto start = text.find(first), finish = text.find(last, start);
    check(start != std::string::npos && finish != std::string::npos,
          "Installed source first layout block changed");
    return text.substr(start, finish + last.size() - start);
}
}
int main(int argc, char **argv) {
    if (argc != 2) throw std::runtime_error("Expected installed game root");
    const auto root = std::filesystem::path(argv[1]);
    const auto sources = inspect_bnw_civilopedia_layout_sources(root);
    if (!sources.verified) throw std::runtime_error(sources.blocker);
    auto base = std::make_shared<civ5::assets::LooseResourceIndex>(root);
    base->add_file(root / "Assets/UI/Styles.xml");
    civ5::assets::ResourceMountTable resources(
        civ5::assets::resource_lookup(base), civ5::assets::resource_lookup_all(base));
    const civ5::runtime::SessionCheckpointIdentity identity{std::string(64, 'a'), std::string(64, 'b')};
    const auto block = first_layout_block(root);
    for (const auto height : {960, 1080}) {
        CivilopediaMeasuredLayout measured;
        measured.frame_number = 31; measured.owner_token = "CivilopediaScreen:Lua-measured-child-test";
        measured.player_id = 0; measured.game_turn = 123;
        measured.payload_sha256 = identity.payload_sha256; measured.rules_sha256 = identity.rules_sha256;
        measured.resolved_styles_sha256 = sources.styles_sha256;
        measured.viewport_width = height == 960 ? 1280 : 1920; measured.viewport_height = height;
        measured.scrollbar_width = 18; measured.scroll_panel_width = 1002; measured.scroll_panel_height = 980;
        measured.left_panel_width = 294; measured.left_panel_height = 980;
        const auto plan = plan_bnw_civilopedia_native_layout(sources, identity, 0, 123, 31, measured);
        for (const bool have_renderer : {false, true}) {
            std::string blocker;
            auto tree = CivisMeasuredCivilopediaControlTree::open(resources, sources, plan, measured,
                CivisUiExecutionScope::diagnostic, blocker);
            if (!tree) throw std::runtime_error(blocker);
            std::vector<std::string> measurements;
            CivisCivilopediaScrollMeasurementProvider renderer;
            if (have_renderer) renderer = [&](const auto &owner, std::string_view id, const auto &viewport) {
                measurements.emplace_back(id);
                CivisMeasuredScrollSnapshot snapshot;
                snapshot.owner = owner; snapshot.panel_id = std::string(id); snapshot.geometry_revision = 1;
                snapshot.viewport_width = viewport.width; snapshot.viewport_height = viewport.height;
                snapshot.participants_complete = true;
                // Deterministic renderer-resolved fixture, not a native proof.
                snapshot.children = {{"ResolvedVisibleContent", true, CivisMeasuredRect{0, 0, 200, 1200}},
                                     {"ResolvedHiddenContent", false, {}}};
                return snapshot;
            };
            auto provider = bind_civis_measured_civilopedia_layout(*tree, sources, measured, std::move(renderer));
            civ5::runtime::LuaScriptHost host({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, std::move(provider));
            bool completed = false;
            try {
                host.execute("CivilopediaScreen", block, "@installed-BNW-Civilopedia-first-layout-block");
                completed = true;
            } catch (const civ5::runtime::ScriptError &error) { blocker = error.what(); }
            if (!have_renderer) {
                check(!completed && tree->completed_operations() == 9 &&
                      blocker.find("renderer measurement provider") != std::string::npos &&
                      !tree->scroll_state(plan.owner, "ScrollPanel").applied,
                      "Actual stock Lua must fail on missing renderer rather than fake native success");
            } else {
                check(completed && tree->completed_operations() == 11 &&
                      measurements == std::vector<std::string>({"ScrollPanel", "LeftScrollPanel"}) &&
                      tree->scroll_state(plan.owner, "ScrollPanel").state.content_height == 1200 &&
                      tree->scroll_state(plan.owner, "ScrollPanel").state.max_scroll_y == 1200 - (height - 126) &&
                      !tree->native_parity_verified(),
                      "Stock Lua must execute both real measurements and calculations in source order");
            }
        }
    }
    std::cout << "Installed stock Lua first block passed at1280x960 and1920x1080:9 setters+2 measured calculations; missing renderer rejected; native parity unverified\n";
}
