#include "civ5/ui/economic_overview_tabs.hpp"
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
}

int main(int argc, char **argv) {
    try {
        using namespace civ5;
        using namespace civ5::ui;
        require(argc == 2, "installed game root required");
        const std::filesystem::path root(argv[1]);
        const auto lua_path = root / "Assets/UI/InGame/Popups/EconomicOverview.lua";
        const auto xml_path = root /
            "Assets/DLC/Expansion2/UI/InGame/Popups/EconomicOverview.xml";
        auto base = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
        base->add_file(lua_path);
        bnw->add_file(xml_path);
        assets::ResourceMountTable mounts(assets::resource_lookup(base), {},
                                           assets::resource_keys(base));
        mounts.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {},
                      assets::resource_keys(bnw)});
        const auto xml = content::read_xml(xml_path);
        require(xml.name == "Context", "selected Expansion2 overview XML has Context root");
        OverviewSource source{"EconomicOverview",
            "Assets/UI/InGame/Popups/EconomicOverview.xml",
            "Assets/DLC/Expansion2/UI/InGame/Popups/EconomicOverview.xml", {}};
        std::function<void(const content::XmlNode &)> collect;
        collect = [&](const content::XmlNode &node) {
            const auto id = node.attribute("ID");
            if (!id.empty()) source.controls.push_back(
                OverviewControl{id, node.name, {}, {}, {}, {}, {}, {}});
            for (const auto &child : node.children) collect(child);
        };
        collect(xml);
        const auto proof = inspect_economic_tab_source(mounts, source);
        require(proof.verified && !proof.lua_sha256.empty() &&
                !proof.lua_path.empty() && !proof.xml_path.empty(),
                "selected Lua callbacks and XML controls verified");

        UiFrameSnapshot frame(11, 7, 51);
        PopupFrame popups;
        popups.player_id = 7;
        popups.turn = 51;
        popups.owner_sequence = 9;
        PopupEvent event;
        event.context_id = "EconomicOverview";
        event.event_id = 42;
        popups.events.push_back(event);
        EconomicTabOwner owner{7, 42, 51, 11, 9, proof.lua_sha256,
                               proof.xml_path, 1200, 800};
        EconomicTabMeasurement geometry{owner, {100, 100, 300, 24},
                                        {400, 100, 300, 24}};
        EconomicOverviewTabs tabs;
        std::string blocker;
        require(tabs.bind(frame, popups, proof, geometry, true, blocker),
                "current owner can bind measured tabs");
        auto view = tabs.view(owner);
        require(view.available && view.general_visible && view.general_highlight &&
                !view.happiness_visible && !view.happiness_highlight,
                "source starts with General tab");
        const auto switched = tabs.click(owner, 410, 110);
        require(switched.consumed && switched.performed &&
                switched.selected == EconomicTab::happiness,
                "authored Happiness callback executes local state change");
        view = tabs.view(owner);
        require(view.happiness_visible && view.happiness_highlight &&
                !view.general_visible && !view.general_highlight,
                "four SetHide effects reflected together");
        require(!tabs.click(owner, 900, 110).performed,
                "outside click causes no transition");
        auto next = owner;
        next.frame = 12;
        UiFrameSnapshot next_frame(12, 7, 51);
        geometry.owner = next;
        require(tabs.bind(next_frame, popups, proof, geometry, true, blocker) &&
                tabs.view(next).happiness_visible,
                "same popup preserves tab across a fresh measured frame");
        require(!tabs.click(owner, 110, 110).performed,
                "old frame cannot act on new geometry");
        require(tabs.click(next, 110, 110).performed &&
                tabs.view(next).general_visible,
                "General callback restores both panel and highlight");
        require(tabs.bind(next_frame, popups, proof, geometry, std::nullopt, blocker),
                "unknown happiness option leaves General usable");
        require(tabs.click(next, 410, 110).consumed &&
                !tabs.click(next, 410, 110).performed &&
                tabs.view(next).general_visible,
                "unknown GAMEOPTION_NO_HAPPINESS blocks Happiness transition");
        auto stale = popups;
        stale.owner_sequence = 10;
        require(!tabs.bind(next_frame, stale, proof, geometry, true, blocker) &&
                !tabs.view(next).available,
                "owner sequence mismatch closes tab hits");
        auto missing = popups;
        missing.events.clear();
        require(!tabs.bind(next_frame, missing, proof, geometry, true, blocker),
                "no active popup event means no tab state");
        auto duplicate = popups;
        duplicate.events.push_back(duplicate.events.front());
        require(!tabs.bind(next_frame, duplicate, proof, geometry, true, blocker),
                "duplicate popup event rejected");
        geometry.happiness_button = {200, 100, 300, 24};
        require(!tabs.bind(next_frame, popups, proof, geometry, true, blocker),
                "overlapping measured tab controls rejected");
        std::cout << "selected BNW economic tab owner/callback fixture passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
