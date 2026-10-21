#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/live_pointer_priority.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *reason) {
    if (!condition) throw std::runtime_error(reason);
}
}

int main() {
    try {
        using namespace civ5::ui;
        const auto in_game = civ5::content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/EconomicOverview" ID="EconomicOverview" Hidden="1"/>
        </Context>)");
        UiScreenController shared(UiContextRegistry::from_documents(
            in_game, civ5::content::read_xml_metadata("<Context/>")));
        std::uint64_t frame_number = 0;
        LiveUiHost host(shared, [&] { return UiFrameSnapshot(
            ++frame_number, 7, 12); }, nullptr);
        auto before = host.begin_frame(1280, 960);
        require(&host.screens() == &shared &&
                !shared.visible("EconomicOverview"),
                "host borrows precisely the owner controller");
        require(shared.dispatch(UiOpenScreen{"EconomicOverview"}, before.snapshot)
                    .status == UiCommandStatus::navigation &&
                host.screens().focused_screen() ==
                    std::optional<std::string>{"EconomicOverview"},
                "owner focus is visible to the same host");
        const auto current = host.begin_frame(1280, 960);
        require(!host.is_current_frame(before) && host.is_current_frame(current),
                "old owner frame sequence cannot be reused");
        require(live_ui_blocks_camera_pointer({}, true, true, 500, 400) &&
                live_ui_blocks_camera_pointer({}, true, true, 501, 401),
                "modal press and release outside controls cannot reach camera");
        std::cout << "EconomicOverview shared host focus: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
