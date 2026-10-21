#include "civ5/ui/ui_runtime.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
using namespace civ5;

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

struct ChooseTechnology { std::int64_t technology_id; };
struct ResearchBackend final : ui::UiGameplayBackend {
    int calls = 0;
    ui::UiCommandResult availability(const ui::UiGameplayCommand &command,
                                      const ui::UiFrameSnapshot &frame) const override {
        const auto *choice = command.as<ChooseTechnology>();
        if (!choice || !frame.active_player_id() || choice->technology_id <= 0)
            return {ui::UiCommandStatus::unavailable, "Technology is unavailable"};
        return {ui::UiCommandStatus::ready, {}};
    }
    ui::UiCommandResult execute(const ui::UiGameplayCommand &, const ui::UiFrameSnapshot &) override {
        ++calls;
        return {ui::UiCommandStatus::performed, {}};
    }
};
} // namespace

int main(int argc, char **argv) {
    try {
        const auto in_game = content::read_xml_metadata(R"(<Context Name="InGame"><Container ID="BulkUI">
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/TopPanel" ID="TopPanel"/>
          <LuaContext FileName="Assets/UI/InGame/TechTree/TechTree" ID="TechTree" Hidden="True"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/SocialPolicyPopup" ID="SocialPolicyPopup" Hidden="1"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/GenericPopup" ID="GenericPopup"/>
          <LuaContext FileName="Assets/UI/InGame/DebugMenu" ID="DebugMenu" Hidden="True"/>
        </Container></Context>)");
        const auto world_view = content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/MiniMapPanel" ID="MiniMapPanel"/>
          <LuaContext FileName="Assets/UI/InGame/WorldView/MPTurnPanel" ID="MPTurnPanel"/>
        </Context>)");
        const auto registry = ui::UiContextRegistry::from_documents(in_game, world_view);
        require(registry.contexts().size() == 6, "single-player context count");
        require(registry.contexts()[1].id == "MiniMapPanel", "nested source order");
        require(registry.find("DebugMenu") == nullptr && registry.find("MPTurnPanel") == nullptr,
                "exclude debug and multiplayer");
        require(registry.find("TopPanel")->owner == ui::UiContextOwner::top_panel,
                "TopPanel ownership");
        require(registry.find("TechTree")->initially_hidden, "source hidden flag");
        require(registry.find("SocialPolicyPopup")->modal, "popup classification");

        ui::UiScreenController controller(registry);
        const ui::UiFrameSnapshot anonymous(0, std::nullopt, 0);
        const ui::UiFrameSnapshot active(1, 0, 0);
        const auto active_with_data = active.with<int>(std::make_shared<const int>(42));
        require(active.find<int>() == nullptr && *active_with_data.find<int>() == 42,
                "immutable typed frame payload");
        require(!controller.visible("TechTree") && !controller.visible("GenericPopup") &&
                    controller.visible("TopPanel"),
                "initial visibility");
        require(controller.dispatch(ui::UiOpenScreen{"TechTree"}, active).status ==
                    ui::UiCommandStatus::navigation && controller.visible("TechTree"),
                "open screen");
        require(controller.dispatch(ui::UiOpenScreen{"SocialPolicyPopup"}, active).status ==
                    ui::UiCommandStatus::navigation &&
                    controller.focused_screen() == "SocialPolicyPopup",
                "modal focus");
        const auto blocked_choice = ui::UiGameplayCommand::make("TechTree", ChooseTechnology{7});
        require(controller.dispatch(blocked_choice, active).status ==
                    ui::UiCommandStatus::invalid, "modal screen blocks background actions");
        require(controller.dispatch(ui::UiCloseScreen{"TechTree"}, active).status ==
                    ui::UiCommandStatus::invalid, "cannot close non-focused screen");
        require(controller.dispatch(ui::UiCloseScreen{"SocialPolicyPopup"}, active).status ==
                    ui::UiCommandStatus::navigation, "close focused popup");
        require(controller.dispatch(ui::UiOpenScreen{"MissingScreen"}, active).status ==
                    ui::UiCommandStatus::invalid, "reject unknown screen");
        require(controller.dispatch(ui::UiGameplayCommand::make("SocialPolicyPopup", ChooseTechnology{7}),
                                    active).status == ui::UiCommandStatus::invalid,
                "hidden popup cannot submit actions");

        ResearchBackend backend;
        const auto choice = ui::UiGameplayCommand::make("TechTree", ChooseTechnology{7});
        require(controller.dispatch(choice, active).status == ui::UiCommandStatus::unavailable,
                "missing backend is unavailable");
        require(controller.dispatch(choice, anonymous, &backend).status ==
                    ui::UiCommandStatus::unavailable && backend.calls == 0,
                "unavailable action does not execute");
        require(controller.dispatch(choice, active, &backend).status ==
                    ui::UiCommandStatus::performed && backend.calls == 1,
                "available action executes exactly once");

        if (argc == 3) {
            auto read_file = [](const char *path) {
                std::ifstream input(path, std::ios::binary);
                if (!input) throw std::runtime_error(std::string("Cannot read ") + path);
                return std::string(std::istreambuf_iterator<char>(input), {});
            };
            const auto real_registry = ui::UiContextRegistry::from_documents(
                content::read_xml_metadata(read_file(argv[1])),
                content::read_xml_metadata(read_file(argv[2])));
            require(real_registry.contexts().size() > 40, "BNW context coverage");
            require(real_registry.find("TopPanel") && real_registry.find("MiniMapPanel") &&
                        real_registry.find("CityView"), "BNW principal contexts");
            std::cout << "BNW single-player contexts: " << real_registry.contexts().size() << '\n';
            for (const auto &entry : real_registry.contexts())
                std::cout << entry.id << '\t' << static_cast<int>(entry.owner) << '\t'
                          << entry.source_key << '\n';
        }
        if (argc == 4) {
            const std::filesystem::path root(argv[3]);
            auto base = std::make_shared<assets::LooseResourceIndex>(root);
            auto expansion = std::make_shared<assets::LooseResourceIndex>(root);
            auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
            base->scan({"Assets/UI"});
            expansion->scan({"Assets/DLC/Expansion/UI"});
            bnw->scan({"Assets/DLC/Expansion2/UI"});
            assets::ResourceMountTable mounted(assets::resource_lookup(base));
            mounted.mount({1, 0, 0, assets::resource_lookup(expansion)});
            mounted.mount({2, 0, 0, assets::resource_lookup(bnw)});
            require(mounted.resolve(assets::AssetKey("InGame.xml")).mount_id == 2,
                    "base -> Expansion -> Expansion2 precedence");
            const auto selected = ui::UiContextRegistry::from_resources(mounted);
            require(selected.contexts().size() == 102, "selected BNW nested context count");
            require(selected.find("LeagueOverviewPopup") && selected.find("TopPanel"),
                    "selected BNW context identities");
            require(selected.find("UnitList") && selected.find("CityList") &&
                        selected.find("NotificationPanel"), "nested BNW contexts");
            std::cout << "PASS active BNW resource order and "
                      << selected.contexts().size() << " nested contexts\n";
            if (std::getenv("CIVIS_UI_LIST"))
                for (const auto &entry : selected.contexts())
                    std::cout << "CONTEXT\t" << entry.id << '\t'
                              << static_cast<int>(entry.owner) << '\t'
                              << entry.source_key << '\n';
        }
        std::cout << "PASS UI runtime context, frame, routing and action checks\n";
        return EXIT_SUCCESS;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
