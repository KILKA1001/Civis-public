#include "civ5/ui/economic_overview_close_service.hpp"
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
        const auto proof = inspect_economic_close_source(mounts, source);
        require(proof.verified && !proof.lua_sha256.empty(),
                "selected Lua close lifecycle and XML CloseButton verified");

        UiFrameSnapshot frame(20, 7, 51);
        PopupFrame popups;
        popups.player_id = 7;
        popups.turn = 51;
        popups.owner_sequence = 3;
        PopupEvent event;
        event.context_id = "EconomicOverview";
        event.event_id = 90;
        popups.events.push_back(event);
        EconomicTabOwner owner{7, 90, 51, 20, 3, proof.lua_sha256,
                               proof.xml_path, 1200, 800};
        EconomicCloseMeasurement geometry{owner, {100, 700, 150, 32}};
        EconomicOverviewClose close;
        std::string blocker;
        require(close.bind(frame, popups, proof, geometry, true, blocker),
                "current visible popup binds CloseButton");
        int effects = 0;
        EconomicCloseService service = [&](const EconomicTabOwner &received) {
            require(received.owner_sequence == 3 && received.frame == 20,
                    "service receives exact owner");
            ++effects;
            return EconomicCloseReceipt{true, true, true, true};
        };
        require(!close.activate(owner, EconomicCloseInput::button_click, 40, 710,
                                service).consumed && effects == 0,
                "outside measured CloseButton does not call service");
        const auto missing = close.activate(owner, EconomicCloseInput::button_click,
                                            110, 710, {});
        require(missing.consumed && missing.status == EconomicCloseStatus::unavailable &&
                effects == 0,
                "missing native service does not close popup");
        auto stale = owner;
        stale.frame = 21;
        require(!close.activate(stale, EconomicCloseInput::escape_key, 0, 0,
                                service).consumed && effects == 0,
                "stale frame cannot close current popup");
        const auto performed = close.activate(owner, EconomicCloseInput::return_key,
                                              0, 0, service);
        require(performed.consumed &&
                performed.status == EconomicCloseStatus::performed && effects == 1,
                "source Return key executes all four owner effects once");
        require(!close.activate(owner, EconomicCloseInput::button_click, 110, 710,
                                service).consumed && effects == 1,
                "closed popup has no active CloseButton");

        owner.event_id = 91;
        owner.owner_sequence = 4;
        popups.owner_sequence = 4;
        popups.events.front().event_id = 91;
        geometry.owner = owner;
        require(close.bind(frame, popups, proof, geometry, true, blocker),
                "new owner popup binds independently");
        const auto partial = close.activate(owner, EconomicCloseInput::button_click,
            110, 710, [&](const EconomicTabOwner &) {
                ++effects;
                return EconomicCloseReceipt{true, true, false, false};
            });
        require(partial.consumed && partial.status == EconomicCloseStatus::uncertain &&
                effects == 2,
                "partial native effects never become performed");
        require(close.activate(owner, EconomicCloseInput::button_click,
            110, 710, service).status == EconomicCloseStatus::uncertain && effects == 2,
            "partial close cannot automatically retry on same owner");
        auto next = owner;
        next.frame = 21;
        UiFrameSnapshot next_frame(21, 7, 51);
        geometry.owner = next;
        require(close.bind(next_frame, popups, proof, geometry, true, blocker),
                "new frame keeps same popup owner");
        require(close.activate(next, EconomicCloseInput::escape_key,
            0, 0, service).status == EconomicCloseStatus::uncertain && effects == 2,
            "frame refresh cannot retry uncertain close");
        auto temporarily_missing = popups;
        temporarily_missing.events.clear();
        require(!close.bind(next_frame, temporarily_missing, proof, geometry,
                            true, blocker),
                "missing popup feed closes the measured hit");
        require(close.bind(next_frame, popups, proof, geometry, true, blocker) &&
                close.activate(next, EconomicCloseInput::escape_key, 0, 0,
                               service).status == EconomicCloseStatus::uncertain &&
                effects == 2,
                "feed loss and return cannot retry partial effects");

        next.event_id = 92;
        next.owner_sequence = 5;
        popups.owner_sequence = 5;
        popups.events.front().event_id = 92;
        geometry.owner = next;
        require(close.bind(next_frame, popups, proof, geometry, true, blocker),
                "different owner token resets close attempt");
        require(close.activate(next, EconomicCloseInput::escape_key, 0, 0,
            [&](const EconomicTabOwner &) -> EconomicCloseReceipt {
                ++effects;
                throw std::runtime_error("dequeue may have occurred");
            }).status == EconomicCloseStatus::uncertain && effects == 3,
            "throwing native service has uncertain result");
        auto no_event = popups;
        no_event.events.clear();
        require(!close.bind(next_frame, no_event, proof, geometry, true, blocker),
                "saved state alone cannot create active popup close hit");

        const auto in_game = content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/EconomicOverview" ID="EconomicOverview" Hidden="1"/>
        </Context>)");
        const auto world_view = content::read_xml_metadata("<Context/>");
        UiScreenController screens(UiContextRegistry::from_documents(
            in_game, world_view));
        require(screens.dispatch(UiOpenScreen{"EconomicOverview"}, frame).status ==
                    UiCommandStatus::navigation &&
                screens.focused_screen() ==
                    std::optional<std::string>{"EconomicOverview"},
                "Civis UIManager queue owns focused EconomicOverview");
        EconomicOverviewClose actual;
        const EconomicTabOwner active{7, 90, 51, 20, 3, proof.lua_sha256,
                                      proof.xml_path, 1200, 800};
        const EconomicCloseMeasurement active_geometry{active, {100, 700, 150, 32}};
        PopupFrame active_popups;
        active_popups.player_id = 7;
        active_popups.turn = 51;
        active_popups.owner_sequence = 3;
        PopupEvent active_event;
        active_event.context_id = "EconomicOverview";
        active_event.event_id = 90;
        active_popups.events.push_back(active_event);
        require(actual.bind(frame, active_popups, proof, active_geometry,
                            true, blocker),
                "current popup binds owner service test");
        int timer_count = 1;
        int processed_count = 0;
        auto current_owner = [&]() -> std::optional<EconomicTabOwner> {
            return active;
        };
        EconomicCloseOwnerServices incomplete{
            current_owner, {}, [&](const EconomicTabOwner &) {
                ++processed_count;
                return true;
            }};
        require(!make_economic_close_service(screens, frame, incomplete),
                "missing timer owner cannot produce a close service");
        require(actual.activate(active, EconomicCloseInput::button_click,
                110, 710, make_economic_close_service(screens, frame, incomplete))
                    .status == EconomicCloseStatus::unavailable &&
                screens.visible("EconomicOverview"),
                "missing lifecycle provider leaves queue intact");
        EconomicCloseOwnerServices complete{
            current_owner,
            [&](const EconomicTabOwner &received) {
                require(received.owner_sequence == 3 && timer_count == 1,
                        "timer decrement follows exact owner dequeue");
                --timer_count;
                return true;
            },
            [&](const EconomicTabOwner &received) {
                require(received.owner_sequence == 3 && timer_count == 0,
                        "Processed event follows timer decrement");
                ++processed_count;
                return true;
            }};
        auto composed_service = make_economic_close_service(screens, frame, complete);
        require(bool(composed_service), "complete owner services compose");
        const auto confirmed = actual.activate(active,
            EconomicCloseInput::button_click, 110, 710, composed_service);
        require(confirmed.status == EconomicCloseStatus::performed &&
                !screens.visible("EconomicOverview") && timer_count == 0 &&
                processed_count == 1,
                "actual Civis dequeue, hide, timer and Processed effects complete");

        UiScreenController partial_screens(UiContextRegistry::from_documents(
            in_game, world_view));
        require(partial_screens.dispatch(UiOpenScreen{"EconomicOverview"}, frame)
                    .status == UiCommandStatus::navigation,
                "second owned queue opens");
        EconomicOverviewClose partial_close;
        require(partial_close.bind(frame, active_popups, proof, active_geometry,
                                   true, blocker), "second popup binds");
        auto wrong_current = make_economic_close_service(partial_screens, frame,
            EconomicCloseOwnerServices{
                []() -> std::optional<EconomicTabOwner> { return std::nullopt; },
                complete.decrement_turn_timer, complete.publish_processed});
        require(partial_close.activate(active, EconomicCloseInput::escape_key,
                0, 0, wrong_current).status == EconomicCloseStatus::uncertain &&
                partial_screens.visible("EconomicOverview") &&
                timer_count == 0 && processed_count == 1,
                "stale owner reader does not dequeue or emit lifecycle effects");
        std::cout << "selected BNW economic close service fixture passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
