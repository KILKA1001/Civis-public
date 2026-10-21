#include "civ5/ui/popup_screens.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
struct CountingBackend final : civ5::ui::UiGameplayBackend {
    mutable int availability_calls = 0;
    int execution_calls = 0;
    civ5::ui::UiCommandResult availability(
        const civ5::ui::UiGameplayCommand &,
        const civ5::ui::UiFrameSnapshot &) const override {
        ++availability_calls;
        return {civ5::ui::UiCommandStatus::ready, {}};
    }
    civ5::ui::UiCommandResult execute(
        const civ5::ui::UiGameplayCommand &,
        const civ5::ui::UiFrameSnapshot &) override {
        ++execution_calls;
        return {civ5::ui::UiCommandStatus::performed, {}};
    }
};
}

int main(int argc, char **argv) {
    try {
        using namespace civ5::ui;
        PopupScreenSource source{"GenericPopup", "selected/GenericPopup.xml",
            civ5::content::read_xml_metadata(R"(<Context>
              <Grid ID="ButtonStackFrame" Size="500,360" Anchor="C,C" Style="Grid9DetailFive140">
                <GridButton ID="CloseButton" Style="BaseButton"/>
              </Grid></Context>)"), {"ButtonStackFrame", "CloseButton"}};
        PopupEvent event;
        event.context_id = "GenericPopup";
        event.event_id = 27;
        event.title = "Live event";
        event.body = "Live body";
        event.choices.push_back({3, "Decision", {}, true});
        event.dismissible = true;
        PopupFrame popup{0, {event}};
        const auto frame = UiFrameSnapshot(1, 0, 1)
            .with<PopupFrame>(std::make_shared<const PopupFrame>(popup));
        PopupGameplayBackend backend;
        const auto plan = plan_popup_screen(source, frame, 1920, 1080, &backend);
        require(plan.paints.size() >= 5 && plan.hits.size() == 2,
                "active popup has panel, text, choice and close paint/hit ops");
        require(plan.hits.front().bounds.x >= 710 && plan.hits.front().bounds.y >= 360,
                "authored center panel geometry is used");
        require(!plan.hits.front().enabled &&
                    plan.hits.front().unavailable_reason.find("not connected") != std::string::npos,
                "unconnected gameplay choice is visibly unavailable");
        require(plan.hits.back().enabled &&
                    std::holds_alternative<UiCloseScreen>(plan.hits.back().command),
                "dismissal is navigation only");
        const auto command = std::get<UiGameplayCommand>(popup_choice_command(
            {"GenericPopup", 27, 3, std::nullopt}));
        require(backend.availability(command, frame).status == UiCommandStatus::unavailable &&
                    backend.execute(command, frame).status == UiCommandStatus::unavailable,
                "choice never reports false success");
        require(backend.availability(std::get<UiGameplayCommand>(popup_choice_command(
                    {"GenericPopup", 26, 3, std::nullopt})), frame).status ==
                    UiCommandStatus::invalid, "stale popup decision rejected");
        const auto stale = UiFrameSnapshot(2, 1, 1)
            .with<PopupFrame>(std::make_shared<const PopupFrame>(popup));
        require(plan_popup_screen(source, stale, 1920, 1080).paints.empty(),
                "event from another player paints nothing");

        PopupEvent long_event = event;
        long_event.choices.clear();
        for (std::int64_t id = 0; id < 12; ++id)
            long_event.choices.push_back({id, "Choice " + std::to_string(id), {}, true});
        const auto long_frame = UiFrameSnapshot(4, 0, 1)
            .with<PopupFrame>(std::make_shared<const PopupFrame>(PopupFrame{0, {long_event}}));
        const auto first_page = plan_popup_screen(source, long_frame, 1920, 1080, &backend);
        require(first_page.scroll_bounds && first_page.hits.size() == 4,
                "overflow exposes scroll buttons, one bounded choice and close");
        require(!first_page.hits[0].enabled && first_page.hits[1].enabled &&
                !first_page.hits[2].enabled,
                "first page cannot scroll up and unconnected choice is unavailable");
        const auto second_frame = long_frame.with<PopupScrollView>(
            std::make_shared<const PopupScrollView>(PopupScrollView{"GenericPopup", 0, 27, 1}));
        const auto second_page = plan_popup_screen(source, second_frame, 1920, 1080, &backend);
        const auto *second_choice = std::get_if<UiGameplayCommand>(&second_page.hits[2].command);
        require(second_choice && second_choice->as<PopupDecision>() &&
                second_choice->as<PopupDecision>()->choice_id == 1,
                "scroll reveals the next live typed choice");
        for (std::size_t i = 0; i < second_page.hits.size() - 1; ++i) {
            const auto &hit = second_page.hits[i];
            require(hit.bounds.y >= second_page.scroll_bounds->y &&
                    hit.bounds.y + hit.bounds.height <=
                        second_page.scroll_bounds->y + second_page.scroll_bounds->height,
                    "scroll controls and choice stay inside the choice region");
        }
        const auto other_event = long_frame.with<PopupScrollView>(
            std::make_shared<const PopupScrollView>(PopupScrollView{"GenericPopup", 0, 28, 8}));
        const auto ignored_scroll = plan_popup_screen(source, other_event, 1920, 1080, &backend);
        const auto *first_choice = std::get_if<UiGameplayCommand>(&ignored_scroll.hits[2].command);
        require(first_choice && first_choice->as<PopupDecision>() &&
                first_choice->as<PopupDecision>()->choice_id == 0,
                "scroll from a different event is ignored");

        if (argc == 2) {
            const std::filesystem::path root(argv[1]);
            auto base = std::make_shared<civ5::assets::LooseResourceIndex>(root);
            auto expansion = std::make_shared<civ5::assets::LooseResourceIndex>(root);
            auto bnw = std::make_shared<civ5::assets::LooseResourceIndex>(root);
            base->scan({"Assets/UI"});
            expansion->scan({"Assets/DLC/Expansion/UI"});
            bnw->scan({"Assets/DLC/Expansion2/UI"});
            civ5::assets::ResourceMountTable resources(civ5::assets::resource_lookup(base));
            resources.mount({1, 0, 0, civ5::assets::resource_lookup(expansion)});
            resources.mount({2, 0, 0, civ5::assets::resource_lookup(bnw)});
            const auto registry = UiContextRegistry::from_resources(resources);
            LiveUiHost host(registry, [&] { return long_frame; }, &backend);
            auto live = host.begin_frame(1920, 1080);
            require(host.dispatch(UiOpenScreen{"GenericPopup"}, live).status ==
                    UiCommandStatus::navigation, "open long popup");
            live = host.begin_frame(1920, 1080);
            const auto *view = live.snapshot.find<PopupScrollView>();
            require(view && view->first_visible == 0, "host starts live event at first choice");
            const auto scroll_down = std::get<UiGameplayCommand>(
                first_page.hits[1].command);
            require(host.dispatch(scroll_down, live).status == UiCommandStatus::navigation,
                    "typed scroll command navigates without gameplay backend");
            require(host.dispatch(scroll_down, live).status == UiCommandStatus::invalid,
                    "scroll invalidates the old UI frame");
            live = host.begin_frame(1920, 1080);
            view = live.snapshot.find<PopupScrollView>();
            require(view && view->first_visible == 1,
                    "next live frame publishes the new list position");
            require(host.wheel_popup(live, first_page.scroll_bounds->x - 1,
                        first_page.scroll_bounds->y, -120,
                        *first_page.scroll_bounds).command.status == UiCommandStatus::invalid,
                    "focused modal rejects wheel outside its choice region");
            require(host.wheel_popup(live, first_page.scroll_bounds->x + 1,
                        first_page.scroll_bounds->y + 1, -120,
                        *first_page.scroll_bounds).command.status == UiCommandStatus::navigation,
                    "wheel advances only local popup navigation");
            live = host.begin_frame(1920, 1080);
            view = live.snapshot.find<PopupScrollView>();
            require(view && view->first_visible == 2,
                    "wheel position is published in the next frame");
            for (std::size_t position = 2; position + 1 < long_event.choices.size();
                 ++position) {
                const auto visible = plan_popup_screen(source, live.snapshot,
                    1920, 1080, &backend);
                const auto *choice = std::get_if<UiGameplayCommand>(
                    &visible.hits[2].command);
                require(choice && choice->as<PopupDecision>() &&
                        choice->as<PopupDecision>()->choice_id ==
                            static_cast<std::int64_t>(position) &&
                        visible.hits[1].enabled,
                        "each wheel position exposes a new live choice");
                require(host.wheel_popup(live, visible.scroll_bounds->x + 1,
                            visible.scroll_bounds->y + 1, -120,
                            *visible.scroll_bounds).command.status ==
                            UiCommandStatus::navigation,
                        "wheel advances to the next distinct page");
                live = host.begin_frame(1920, 1080);
                view = live.snapshot.find<PopupScrollView>();
                require(view && view->first_visible == position + 1,
                        "host and planned scroll position stay equal");
            }
            const auto last_page = plan_popup_screen(source, live.snapshot,
                1920, 1080, &backend);
            const auto *last_choice = std::get_if<UiGameplayCommand>(
                &last_page.hits[2].command);
            require(last_choice && last_choice->as<PopupDecision>() &&
                    last_choice->as<PopupDecision>()->choice_id == 11 &&
                    !last_page.hits[1].enabled,
                    "last live choice is reachable and down button disables");
            require(host.wheel_popup(live, last_page.scroll_bounds->x + 1,
                        last_page.scroll_bounds->y + 1, -120,
                        *last_page.scroll_bounds).command.status ==
                        UiCommandStatus::unavailable,
                    "wheel cannot report navigation past the final choice");
            require(host.dispatch(last_page.hits[0].command, live).status ==
                    UiCommandStatus::navigation,
                    "up button works from the wheel-reached final choice");
            live = host.begin_frame(1920, 1080);
            view = live.snapshot.find<PopupScrollView>();
            require(view && view->first_visible == 10,
                    "up button restores the preceding visible choice");
            live = host.begin_frame(1280, 960);
            view = live.snapshot.find<PopupScrollView>();
            require(view && view->first_visible == 0,
                    "a viewport change rebinds visible-row capacity safely");
            require(host.dispatch(UiCloseScreen{"GenericPopup"}, live).status ==
                    UiCommandStatus::navigation, "close scrollable popup");
            const auto catalog = PopupCatalog::from_resources(resources, registry);
            const auto *goody_source = catalog.find("GoodyHutPopup");
            require(goody_source && goody_source->contains("TitleLabel") &&
                goody_source->contains("DescriptionLabel") &&
                goody_source->contains("CloseButton") &&
                goody_source->selected_source_path.find(
                    "GoodyHutPopup.xml") != std::string::npos &&
                goody_source->selected_source_path.find("Expansion2") ==
                    std::string::npos,
                "active BNW GoodyHutPopup selects its authored base XML");
            PopupEvent goody;
            goody.context_id = "GoodyHutPopup";
            goody.event_id = 17;
            goody.body = "You have received 30 Culture!";
            goody.dismissible = true;
            goody.goody_hut_reward = GoodyHutRewardOwnerEvidence{
                0, 1, 77, 9001, 17, 4, 30, goody_source->selected_source_path};
            const auto goody_snapshot = UiFrameSnapshot(60, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {event, goody}, 1, 77}));
            require(valid_goody_hut_reward_owner(goody_snapshot, goody,
                    goody_source->selected_source_path),
                "current reward owner binds player, turn, sequence and selected XML");
            const auto stale_goody = UiFrameSnapshot(60, 0, 2)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {goody}, 1, 77}));
            require(!valid_goody_hut_reward_owner(stale_goody, goody,
                    goody_source->selected_source_path) &&
                !valid_goody_hut_reward_owner(goody_snapshot, goody,
                    "wrong/GoodyHutPopup.xml") &&
                plan_popup_screen(*goody_source, stale_goody, 1280, 960)
                    .hits.empty(),
                "stale turn and wrong resource provenance cannot expose reward hit");
            const auto no_goody_owner = UiFrameSnapshot(61, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {PopupEvent{.context_id = "GoodyHutPopup",
                        .event_id = 17}}, 1, 77}));
            require(plan_popup_screen(*goody_source, no_goody_owner,
                    1280, 960).hits.empty(),
                "ordinary preview has no invented reward popup");
            auto native_goody = goody.goody_hut_reward;
            int processed_goody = 0;
            LiveUiSources goody_sources;
            goody_sources.active_goody_hut_reward = [&] { return native_goody; };
            goody_sources.goody_hut_processed =
                [&](const GoodyHutRewardOwnerEvidence &owner) {
                    if (!native_goody || owner.token != native_goody->token)
                        return UiCommandResult{UiCommandStatus::unavailable,
                            "Native popup changed"};
                    ++processed_goody;
                    native_goody.reset();
                    return UiCommandResult{UiCommandStatus::performed, {}};
                };
            LiveUiHost goody_host(registry, [&] { return goody_snapshot; },
                                  &backend, goody_sources);
            auto goody_live = goody_host.begin_frame(1280, 960);
            require(goody_host.dispatch(UiOpenScreen{"GenericPopup"}, goody_live)
                        .status == UiCommandStatus::navigation,
                "underlying modal opens before reward");
            goody_live = goody_host.begin_frame(1280, 960);
            require(goody_host.dispatch(UiOpenScreen{"GoodyHutPopup"}, goody_live)
                        .status == UiCommandStatus::navigation,
                "current owner event queues reward popup on modal stack");
            goody_live = goody_host.begin_frame(1280, 960);
            const auto focused_goody = plan_focused_popup(catalog,
                goody_host.screens(), goody_live.snapshot, 1280, 960, &backend,
                [](std::string_view key) -> std::optional<std::string> {
                    if (key == "TXT_KEY_POP_RUINS_EXPLORED")
                        return "Ruins explored";
                    if (key == "TXT_KEY_CLOSE") return "Close";
                    return std::nullopt;
                });
            require(focused_goody && focused_goody->hits.size() == 1 &&
                focused_goody->hits.front().enabled &&
                focused_goody->hit(focused_goody->hits.front().bounds.x + 1,
                    focused_goody->hits.front().bounds.y + 1) &&
                std::holds_alternative<UiGameplayCommand>(
                    focused_goody->hits.front().command),
                "authored close hit belongs only to focused reward");
            require(std::any_of(focused_goody->paints.begin(),
                    focused_goody->paints.end(), [](const WorldViewPaint &paint) {
                        return paint.text == "You have received 30 Culture!";
                    }) &&
                std::none_of(focused_goody->paints.begin(),
                    focused_goody->paints.end(), [](const WorldViewPaint &paint) {
                        return paint.text.find("cultural artifacts which awe")
                            != std::string::npos;
                    }), "reward body is live, not the authored example");
            require(goody_host.dispatch(UiCloseScreen{"GoodyHutPopup"}, goody_live)
                        .status == UiCommandStatus::unavailable &&
                processed_goody == 0,
                "raw navigation cannot skip native PopupProcessed");
            const auto close_goody = focused_goody->hits.front().command;
            require(goody_host.dispatch(close_goody, goody_live).status ==
                    UiCommandStatus::navigation &&
                processed_goody == 1 &&
                goody_host.screens().focused_screen() ==
                    std::optional<std::string>{"GenericPopup"},
                "processed acknowledgement closes reward and restores modal focus");
            require(goody_host.dispatch(close_goody, goody_live).status ==
                    UiCommandStatus::invalid && processed_goody == 1,
                "stale close cannot process the same native event twice");
            goody_live = goody_host.begin_frame(1280, 960);
            require(goody_host.dispatch(UiOpenScreen{"GoodyHutPopup"}, goody_live)
                    .status == UiCommandStatus::unavailable &&
                processed_goody == 1,
                "processed native token cannot be reopened from stale snapshot");
            LiveUiHost disconnected_goody(registry,
                [&] { return goody_snapshot; }, &backend);
            auto disconnected_frame = disconnected_goody.begin_frame(1280, 960);
            require(disconnected_goody.dispatch(UiOpenScreen{"GoodyHutPopup"},
                    disconnected_frame).status == UiCommandStatus::navigation,
                "current reward can be displayed without a processed backend");
            disconnected_frame = disconnected_goody.begin_frame(1280, 960);
            const auto disconnected_plan = plan_popup_screen(*goody_source,
                disconnected_frame.snapshot, 1280, 960, &backend);
            require(disconnected_plan.hits.size() == 1 &&
                !disconnected_plan.hits.front().enabled &&
                disconnected_goody.dispatch(disconnected_plan.hits.front().command,
                    disconnected_frame).status == UiCommandStatus::unavailable,
                "close is visibly disabled and fails closed without native owner");
            const auto *barbarian_source = catalog.find("BarbarianCampPopup");
            require(barbarian_source &&
                barbarian_source->contains("TitleLabel") &&
                barbarian_source->contains("DescriptionLabel") &&
                barbarian_source->contains("CloseButton") &&
                barbarian_source->selected_source_path.find(
                    "BarbarianCampPopup.xml") != std::string::npos &&
                barbarian_source->selected_source_path.find("Expansion2") ==
                    std::string::npos,
                "active BNW barbarian camp reward selects its base XML");
            PopupEvent fake_barbarian;
            fake_barbarian.context_id = "BarbarianCampPopup";
            fake_barbarian.event_id = 18;
            fake_barbarian.title = "A camp was cleared";
            fake_barbarian.body = "You received 25 Gold";
            fake_barbarian.dismissible = true;
            const auto fake_barbarian_frame = UiFrameSnapshot(61, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {fake_barbarian}, 1, 78}));
            const auto barbarian_plan = plan_popup_screen(
                *barbarian_source, fake_barbarian_frame, 1280, 960,
                &backend);
            require(barbarian_plan.paints.empty() &&
                barbarian_plan.hits.empty() &&
                !barbarian_plan.unresolved.empty(),
                "generic reward text cannot paint or close a native camp event");
            LiveUiHost barbarian_host(registry,
                [&] { return fake_barbarian_frame; }, &backend);
            const auto barbarian_live = barbarian_host.begin_frame(1280, 960);
            require(barbarian_host.dispatch(
                    UiOpenScreen{"BarbarianCampPopup"}, barbarian_live)
                    .status == UiCommandStatus::unavailable &&
                !barbarian_host.screens().visible("BarbarianCampPopup") &&
                barbarian_host.dispatch(
                    UiCloseScreen{"BarbarianCampPopup"}, barbarian_live)
                    .status == UiCommandStatus::invalid,
                "missing native camp owner cannot queue or claim a close");
            const auto *wonder_source = catalog.find("NaturalWonderPopup");
            require(wonder_source &&
                wonder_source->contains("Icon") &&
                wonder_source->contains("WonderLabel") &&
                wonder_source->contains("TitleLabel") &&
                wonder_source->contains("DescriptionLabel") &&
                wonder_source->contains("CloseButton") &&
                wonder_source->selected_source_path.find(
                    "NaturalWonderPopup.xml") != std::string::npos &&
                wonder_source->selected_source_path.find("Expansion2") !=
                    std::string::npos,
                "active BNW natural wonder reward selects its Expansion2 XML");
            PopupEvent fake_wonder;
            fake_wonder.context_id = "NaturalWonderPopup";
            fake_wonder.event_id = 19;
            fake_wonder.title = "A natural wonder was found";
            fake_wonder.body = "First finder gold";
            fake_wonder.dismissible = true;
            const auto fake_wonder_frame = UiFrameSnapshot(62, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {fake_wonder}, 1, 79}));
            const auto wonder_plan = plan_popup_screen(
                *wonder_source, fake_wonder_frame, 1280, 960, &backend);
            require(wonder_plan.paints.empty() &&
                wonder_plan.hits.empty() &&
                !wonder_plan.unresolved.empty(),
                "generic text cannot fake wonder plot, portrait or close");
            LiveUiHost wonder_host(registry,
                [&] { return fake_wonder_frame; }, &backend);
            const auto wonder_live = wonder_host.begin_frame(1280, 960);
            require(wonder_host.dispatch(
                    UiOpenScreen{"NaturalWonderPopup"}, wonder_live)
                    .status == UiCommandStatus::unavailable &&
                !wonder_host.screens().visible("NaturalWonderPopup") &&
                wonder_host.dispatch(
                    UiCloseScreen{"NaturalWonderPopup"}, wonder_live)
                    .status == UiCommandStatus::invalid,
                "missing native wonder owner cannot queue or claim a close");
            const auto *great_person_source =
                catalog.find("GreatPersonRewardPopup");
            require(great_person_source &&
                great_person_source->contains("TitleLabel") &&
                great_person_source->contains("DescriptionLabel") &&
                great_person_source->contains("Portrait") &&
                great_person_source->contains("CloseButton"),
                "active great person reward XML requires portrait and close");
            PopupEvent fake_great_person;
            fake_great_person.context_id = "GreatPersonRewardPopup";
            fake_great_person.event_id = 19;
            fake_great_person.title = "A great person was born";
            fake_great_person.body = "Congratulations!";
            fake_great_person.dismissible = true;
            const auto fake_great_person_frame = UiFrameSnapshot(62, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {fake_great_person}, 1, 79}));
            const auto great_person_plan = plan_popup_screen(
                *great_person_source, fake_great_person_frame, 1280, 960,
                &backend);
            require(great_person_plan.paints.empty() &&
                great_person_plan.hits.empty() &&
                !great_person_plan.unresolved.empty(),
                "generic event cannot fake reward portrait, body or close action");
            LiveUiHost great_person_host(registry,
                [&] { return fake_great_person_frame; }, &backend);
            const auto great_person_live = great_person_host.begin_frame(1280, 960);
            require(great_person_host.dispatch(
                    UiOpenScreen{"GreatPersonRewardPopup"}, great_person_live)
                    .status == UiCommandStatus::unavailable &&
                !great_person_host.screens().visible("GreatPersonRewardPopup") &&
                great_person_host.dispatch(
                    UiCloseScreen{"GreatPersonRewardPopup"}, great_person_live)
                    .status == UiCommandStatus::invalid,
                "missing native reward owner cannot queue or claim a close");
            std::size_t assigned = 0;
            for (const auto &context : registry.contexts())
                if (context.owner == UiContextOwner::popup) ++assigned;
            require(catalog.sources().size() + catalog.unresolved().size() == assigned,
                    "every active U8 popup has a selected resource or explicit unresolved reason");
            require(catalog.find("GenericPopup") && catalog.find("ChoosePantheon") &&
                    catalog.find("ChooseReligion") && catalog.find("DeclareWarPopup") &&
                    catalog.find("SetCityName"), "base and BNW popup sources resolve");
            require(catalog.find("ChoosePantheon")->selected_source_path.find("Expansion2")
                        != std::string::npos, "BNW popup overrides expansion XML");
            for (const auto id : {"SetCityName", "SetUnitName"}) {
                const auto *name_source = catalog.find(id);
                require(name_source && name_source->contains(
                    std::string(id) == "SetCityName" ? "EditCityName" : "EditUnitName"),
                    "selected active XML contains the authored rename EditBox");
                PopupEvent name_event;
                name_event.context_id = id;
                name_event.event_id = 81;
                name_event.entered_text = "Delhi";
                name_event.choices.push_back({7, "Accept", {}, true});
                auto name_frame = UiFrameSnapshot(10, 0, 1).with<PopupFrame>(
                    std::make_shared<const PopupFrame>(PopupFrame{0, {name_event}}));
                LiveUiHost name_host(registry, [&] { return name_frame; }, &backend);
                UiScreenCompositor name_compositor(resources, registry, &backend);
                auto live_name = name_host.begin_frame(1280, 960);
                require(name_host.dispatch(UiOpenScreen{id}, live_name).status ==
                        UiCommandStatus::navigation, "open authored rename popup");
                auto presented = present_live_ui_frame(name_host, name_compositor,
                                                       1280, 960);
                require(presented.composed.popup_edit_box &&
                        presented.live.snapshot.find<PopupTextView>() &&
                        presented.live.snapshot.find<PopupTextView>()->text == "Delhi",
                        "live event initializes selected EditBox and local draft");
                const auto box = *presented.composed.popup_edit_box;
                PopupNameEditor editor;
                require(editor.select(name_host, presented.live, presented.composed,
                                      box.x + 2, box.y + 2, box.x + 3, box.y + 3),
                        "same target press and release focus authored EditBox");
                const std::array<char16_t, 2> add{u'X', u'\u0416'};
                require(editor.input(name_host, presented.live, presented.composed, add)
                            .status == UiCommandStatus::navigation,
                        "UTF-16 input changes local popup draft only");
                require(editor.input(name_host, presented.live, presented.composed, add)
                            .status == UiCommandStatus::invalid,
                        "text input cannot reuse stale composed frame");
                presented = present_live_ui_frame(name_host, name_compositor, 1280, 960);
                require(presented.live.snapshot.find<PopupTextView>()->text ==
                        "DelhiX\xd0\x96", "next immutable frame carries edited UTF-8 text");
                require(editor.select(name_host, presented.live, presented.composed,
                                      box.x + 2, box.y + 2, box.x + 3, box.y + 3),
                        "current event can refocus its authored EditBox");
                const auto planned = plan_popup_screen(*name_source,
                    presented.live.snapshot, 1280, 960, &backend);
                const auto *decision = std::get_if<UiGameplayCommand>(&planned.hits.back().command);
                require(decision && decision->as<PopupDecision>() &&
                        decision->as<PopupDecision>()->entered_text ==
                            std::optional<std::string>{"DelhiX\xd0\x96"} &&
                        !planned.hits.back().enabled,
                        "typed decision uses current draft, yet absent backend stays unavailable");
                require(name_host.dispatch(*decision, presented.live).status ==
                        UiCommandStatus::unavailable,
                        "rename never reports gameplay success without a handler");
                const std::array<char16_t, 1> enter{u'\r'};
                const auto enter_result = editor.input(name_host, presented.live,
                                                       presented.composed, enter);
                require(enter_result.status == UiCommandStatus::unavailable,
                        "Enter routes unique live accept choice without false success");
                const std::array<char16_t, 2> packed{u'Y', u'\r'};
                require(editor.input(name_host, presented.live, presented.composed,
                                     packed).status == UiCommandStatus::navigation &&
                        editor.pending_submit(),
                        "same WM_CHAR batch keeps Enter pending after typed draft update");
                presented = present_live_ui_frame(name_host, name_compositor, 1280, 960);
                require(presented.live.snapshot.find<PopupTextView>()->text ==
                        "DelhiX\xd0\x96Y" &&
                        editor.input(name_host, presented.live, presented.composed,
                                     {}).status == UiCommandStatus::unavailable &&
                        !editor.pending_submit(),
                        "fresh frame submits packed Enter with the updated name");
                const auto stale_event_command = *decision;
                name_event.event_id = 82;
                name_frame = UiFrameSnapshot(11, 0, 1).with<PopupFrame>(
                    std::make_shared<const PopupFrame>(PopupFrame{0, {name_event}}));
                presented = present_live_ui_frame(name_host, name_compositor, 1280, 960);
                require(presented.live.snapshot.find<PopupTextView>()->text == "Delhi" &&
                        name_host.dispatch(stale_event_command, presented.live).status ==
                            UiCommandStatus::invalid,
                        "new live event resets draft and rejects prior decision");
                require(editor.input(name_host, presented.live, presented.composed,
                                     add).status == UiCommandStatus::invalid,
                        "EditBox focus does not transfer to another event");
                name_frame = UiFrameSnapshot(12, 1, 1).with<PopupFrame>(
                    std::make_shared<const PopupFrame>(PopupFrame{0, {name_event}}));
                presented = present_live_ui_frame(name_host, name_compositor, 1280, 960);
                require(!presented.live.snapshot.find<PopupTextView>() &&
                        !presented.composed.popup_edit_box,
                        "another player's event cannot expose an input field");
            }
            require(!valid_popup_name("") && !valid_popup_name("   ") &&
                    valid_popup_name("\xd0\x94\xd0\xb5\xd0\xbb\xd0\xb8") &&
                    !valid_popup_name("1234567890123456") &&
                    !valid_popup_name("\xed\xa0\x80"),
                    "authored 15-character rename validation handles EN/RU");
            const auto *religion_source = catalog.find("ChooseReligion");
            require(religion_source &&
                    religion_source->selected_source_path.find("Expansion2") != std::string::npos,
                    "religion strip uses selected BNW XML");
            PopupEvent religion;
            religion.context_id = "ChooseReligion";
            religion.event_id = 81;
            religion.dismissible = true;
            religion.choose_religion = ChooseReligionOwnerEvidence{
                0, 1, 91, 81, 4, 5, ChooseReligionMode::founding,
                religion_source->selected_source_path};
            for (std::int64_t id = 1; id <= 15; ++id)
                religion.choices.push_back({id, "Religion " + std::to_string(id),
                    {}, id != 2, "RELIGION_ATLAS_WHITE",
                    static_cast<std::int32_t>(id - 1)});
            const auto no_owner = UiFrameSnapshot(50, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {PopupEvent{.context_id = "ChooseReligion",
                        .event_id = 81}}, 1, 91}));
            require(plan_popup_screen(*religion_source, no_owner, 1280, 960)
                        .hits.empty(),
                    "saved rules without owner event cannot publish religion symbols");
            LiveUiHost no_owner_host(registry, [&] { return no_owner; }, &backend);
            const auto no_owner_frame = no_owner_host.begin_frame(1280, 960);
            require(no_owner_host.dispatch(UiOpenScreen{"ChooseReligion"},
                        no_owner_frame).status == UiCommandStatus::unavailable,
                    "missing owner evidence cannot take religion modal focus");
            for (const auto &[width, height] :
                 {std::pair{1280, 960}, std::pair{1920, 1080}}) {
                const auto religion_frame = UiFrameSnapshot(50, 0, 1)
                    .with<PopupFrame>(std::make_shared<const PopupFrame>(
                        PopupFrame{0, {religion}, 1, 91}));
                const auto localize_en = [](std::string_view key)
                    -> std::optional<std::string> {
                    return key == "TXT_KEY_CHOOSE_RELIGION_TITLE"
                        ? std::optional<std::string>{"Choose a Religion"} : std::nullopt;
                };
                const auto first = plan_popup_screen(*religion_source,
                    religion_frame, width, height, &backend, localize_en);
                require(first.scroll_bounds && first.scroll_bounds->width == 950 &&
                        first.scroll_bounds->height == 66 &&
                        first.scroll_bounds->x == (width - 990) / 2 + 20 &&
                        first.scroll_bounds->y == (height - 653) / 2 + 6 + 55,
                        "selected XML positions horizontal religion strip");
                require(first.hits.size() == 16 && !first.hits[0].enabled &&
                        first.hits[1].enabled && first.hits[2].enabled &&
                        !first.hits[3].enabled,
                        "long religion list has two arrows, thirteen symbols and close");
                const auto *choice = std::get_if<UiGameplayCommand>(
                    &first.hits[2].command);
                require(choice && choice->as<ReligionUiAction>() &&
                        choice->as<ReligionUiAction>()->event_id == 81 &&
                        choice->as<ReligionUiAction>()->religion_id == 1 &&
                        choice->as<ReligionUiAction>()->kind ==
                            ReligionUiActionKind::select_symbol,
                        "founding symbol hit is typed popup-local selection");
                require(first.hits[2].bounds.width == 64 &&
                        first.hits[2].bounds.height == 64 &&
                        first.hits[4].bounds.x - first.hits[2].bounds.x == 144,
                        "symbol hit rectangles use authored 64px size and 8px padding");
                auto short_religion = religion;
                short_religion.choices.resize(3);
                const auto short_frame = UiFrameSnapshot(50, 0, 1)
                    .with<PopupFrame>(std::make_shared<const PopupFrame>(
                        PopupFrame{0, {short_religion}, 1, 91}));
                const auto centered = plan_popup_screen(*religion_source,
                    short_frame, width, height, &backend);
                require(!centered.scroll_bounds && centered.hits.size() == 4 &&
                        centered.hits[0].bounds.x == first.scroll_bounds->x + 371,
                        "short religion list centers its authored stack");
                std::size_t symbols = 0;
                for (const auto &paint : first.paints)
                    if (paint.kind == WorldViewPaintKind::texture &&
                        paint.source_key.find("ReligiousSymbolsWhite64_Expansion.dds") !=
                            std::string::npos) {
                        require(paint.source.width == 64 && paint.source.height == 64,
                                "symbol uses 64px atlas crop");
                        ++symbols;
                    }
                require(symbols == 13, "every visible choice paints its live atlas symbol");
                require(backend.availability(std::get<UiGameplayCommand>(
                    popup_choice_command({"ChooseReligion", 80, 1, std::nullopt})),
                    religion_frame).status == UiCommandStatus::invalid,
                    "stale religion event token is rejected");
                bool english_title = false;
                for (const auto &paint : first.paints)
                    english_title |= paint.text == "Choose a Religion";
                require(english_title, "EN heading is localized");
                const auto scrolled_frame = religion_frame.with<PopupScrollView>(
                    std::make_shared<const PopupScrollView>(PopupScrollView{
                        "ChooseReligion", 0, 81, 1}));
                const auto scrolled = plan_popup_screen(*religion_source,
                    scrolled_frame, width, height, &backend,
                    [](std::string_view key) -> std::optional<std::string> {
                        return key == "TXT_KEY_CHOOSE_RELIGION_TITLE"
                            ? std::optional<std::string>{"Выберите религию"}
                            : std::nullopt;
                    });
                choice = std::get_if<UiGameplayCommand>(&scrolled.hits[2].command);
                require(choice && choice->as<ReligionUiAction>() &&
                        choice->as<ReligionUiAction>()->religion_id == 2 &&
                        scrolled.hits[0].enabled,
                        "scroll advances focus to the next live symbol");
                const auto end_frame = religion_frame.with<PopupScrollView>(
                    std::make_shared<const PopupScrollView>(PopupScrollView{
                        "ChooseReligion", 0, 81, 14}));
                const auto end_plan = plan_popup_screen(*religion_source,
                    end_frame, width, height, &backend);
                choice = std::get_if<UiGameplayCommand>(&end_plan.hits[2].command);
                require(end_plan.hits.size() == 4 && !end_plan.hits[1].enabled &&
                        choice && choice->as<ReligionUiAction>() &&
                        choice->as<ReligionUiAction>()->religion_id == 15,
                        "last religion stays reachable and right arrow stops");
                bool russian_title = false;
                for (const auto &paint : scrolled.paints)
                    russian_title |= paint.text == "Выберите религию";
                require(russian_title, "RU heading is localized");
                const auto stale_scroll = religion_frame.with<PopupScrollView>(
                    std::make_shared<const PopupScrollView>(PopupScrollView{
                        "ChooseReligion", 0, 82, 1}));
                const auto stale_plan = plan_popup_screen(*religion_source,
                    stale_scroll, width, height, &backend);
                choice = std::get_if<UiGameplayCommand>(&stale_plan.hits[2].command);
                require(choice && choice->as<ReligionUiAction>() &&
                        choice->as<ReligionUiAction>()->religion_id == 1,
                        "stale scroll event cannot move symbol focus");
                require(plan_popup_screen(*religion_source,
                    UiFrameSnapshot(50, 1, 1).with<PopupFrame>(
                        std::make_shared<const PopupFrame>(PopupFrame{0, {religion}, 1, 91})),
                    width, height).paints.empty(),
                    "stale player cannot paint religion choices");
                auto stale_religion = religion;
                stale_religion.choose_religion->owner_sequence = 92;
                require(plan_popup_screen(*religion_source,
                    UiFrameSnapshot(50, 0, 1).with<PopupFrame>(
                        std::make_shared<const PopupFrame>(
                            PopupFrame{0, {stale_religion}, 1, 91})),
                    width, height).hits.empty(),
                    "stale owner token cannot publish religion symbol hits");
                stale_religion = religion;
                stale_religion.choose_religion->selected_source_path =
                    "Assets/DLC/Expansion/UI/InGame/Popups/ChooseReligionPopup.xml";
                require(plan_popup_screen(*religion_source,
                    UiFrameSnapshot(50, 0, 1).with<PopupFrame>(
                        std::make_shared<const PopupFrame>(
                            PopupFrame{0, {stale_religion}, 1, 91})),
                    width, height).hits.empty(),
                    "wrong selected XML cannot publish religion symbol hits");
                LiveUiHost religion_host(registry,
                    [&] { return religion_frame; }, &backend);
                auto focused = religion_host.begin_frame(width, height);
                require(religion_host.dispatch(UiOpenScreen{"ChooseReligion"}, focused)
                            .status == UiCommandStatus::navigation,
                        "religion popup takes modal focus");
                focused = religion_host.begin_frame(width, height);
                require(religion_host.dispatch(popup_choice_command(
                    {"ChooseReligion", 81, 1, std::nullopt}), focused).status ==
                    UiCommandStatus::unavailable,
                    "religion choice cannot reach a gameplay backend before staged confirmation");
                const auto focused_plan = plan_popup_screen(*religion_source,
                    focused.snapshot, width, height, &backend);
                const auto step = std::get<UiGameplayCommand>(
                    focused_plan.hits[1].command);
                require(religion_host.dispatch(step, focused).status ==
                            UiCommandStatus::navigation &&
                        religion_host.dispatch(step, focused).status ==
                            UiCommandStatus::invalid,
                        "focused strip scrolls once and rejects old frame");
                focused = religion_host.begin_frame(width, height);
                const auto *position = focused.snapshot.find<PopupScrollView>();
                require(position && position->first_visible == 1,
                        "next focused frame publishes religion scroll position");
            }
            {
                UiFrameSnapshot selected_snapshot = UiFrameSnapshot(50, 0, 1)
                    .with<PopupFrame>(std::make_shared<const PopupFrame>(
                        PopupFrame{0, {religion}, 1, 91}));
                CountingBackend counting_backend;
                LiveUiHost selected_host(registry,
                    [&] { return selected_snapshot; }, &counting_backend);
                auto selected_frame = selected_host.begin_frame(1280, 960);
                require(selected_host.dispatch(UiOpenScreen{"ChooseReligion"},
                            selected_frame).status == UiCommandStatus::navigation,
                        "owner-backed founding event opens religion popup");
                selected_frame = selected_host.begin_frame(1280, 960);
                const auto symbol_plan = plan_popup_screen(*religion_source,
                    selected_frame.snapshot, 1280, 960, &counting_backend);
                require(symbol_plan.hits[2].enabled &&
                        selected_host.dispatch(symbol_plan.hits[2].command,
                            selected_frame).status == UiCommandStatus::navigation &&
                        counting_backend.availability_calls == 0 &&
                        counting_backend.execution_calls == 0,
                        "founding symbol updates local UI without gameplay transaction");
                require(selected_host.dispatch(symbol_plan.hits[2].command,
                            selected_frame).status == UiCommandStatus::invalid,
                        "religion symbol cannot replay on a stale frame");
                selected_frame = selected_host.begin_frame(1280, 960);
                const auto *selection =
                    selected_frame.snapshot.find<ReligionSelectionView>();
                const auto detail_plan = plan_popup_screen(*religion_source,
                    selected_frame.snapshot, 1280, 960, &counting_backend);
                require(selection && selection->religion_id == 1 &&
                        detail_plan.hits.size() == 17 &&
                        !detail_plan.hits[2].enabled &&
                        !detail_plan.hits[15].enabled &&
                        detail_plan.hits[15].unavailable_reason.find("GameCore") !=
                            std::string::npos,
                        "selected religion paints detail with FoundReligion disabled");
                bool detail_icon = false;
                for (const auto &paint : detail_plan.paints)
                    detail_icon |= paint.kind == WorldViewPaintKind::texture &&
                        paint.source.width == 80 && paint.source.height == 80 &&
                        paint.source_key.find("ReligiousSymbolsWhite80_Expansion.dds") !=
                            std::string::npos;
                require(detail_icon, "selection uses authored 80px religion icon");
                require(selected_host.dispatch(detail_plan.hits[15].command,
                            selected_frame).status == UiCommandStatus::unavailable &&
                        counting_backend.execution_calls == 0,
                        "FoundReligion never reaches backend without beliefs and name");
                require(selected_host.dispatch(UiCloseScreen{"ChooseReligion"},
                            selected_frame).status == UiCommandStatus::navigation,
                        "Close dismisses only local religion selection");
                selected_frame = selected_host.begin_frame(1280, 960);
                require(!selected_frame.snapshot.find<ReligionSelectionView>(),
                        "reopened religion screen cannot inherit closed selection");
                require(selected_host.dispatch(UiOpenScreen{"ChooseReligion"},
                            selected_frame).status == UiCommandStatus::navigation,
                        "current founding event can reopen after local Close");
                selected_frame = selected_host.begin_frame(1280, 960);
                const auto again = plan_popup_screen(*religion_source,
                    selected_frame.snapshot, 1280, 960, &counting_backend);
                require(selected_host.dispatch(again.hits[2].command,
                            selected_frame).status == UiCommandStatus::navigation,
                        "current owner can select a religion again");
                auto changed_owner = religion;
                changed_owner.choose_religion->owner_sequence = 92;
                selected_snapshot = UiFrameSnapshot(51, 0, 1)
                    .with<PopupFrame>(std::make_shared<const PopupFrame>(
                        PopupFrame{0, {changed_owner}, 1, 92}));
                selected_frame = selected_host.begin_frame(1280, 960);
                require(!selected_frame.snapshot.find<ReligionSelectionView>(),
                        "new owner token clears old symbol selection");
                changed_owner.choose_religion->mode =
                    ChooseReligionMode::enhancing;
                selected_snapshot = UiFrameSnapshot(51, 0, 1)
                    .with<PopupFrame>(std::make_shared<const PopupFrame>(
                        PopupFrame{0, {changed_owner}, 1, 92}));
                selected_frame = selected_host.begin_frame(1280, 960);
                const auto enhancing_plan = plan_popup_screen(*religion_source,
                    selected_frame.snapshot, 1280, 960, &counting_backend);
                require(!enhancing_plan.hits[2].enabled &&
                        selected_host.dispatch(enhancing_plan.hits[2].command,
                            selected_frame).status == UiCommandStatus::unavailable,
                        "BNW enhancement ignores symbol selection clicks");
                selected_snapshot = UiFrameSnapshot(52, 0, 1);
                selected_frame = selected_host.begin_frame(1280, 960);
                require(!selected_frame.snapshot.find<ReligionSelectionView>() &&
                        !selected_host.screens().focused_screen() &&
                        plan_popup_screen(*religion_source,
                            selected_frame.snapshot, 1280, 960).hits.empty() &&
                        counting_backend.availability_calls == 0 &&
                        counting_backend.execution_calls == 0,
                        "missing event removes religion modal focus without backend calls");
            }
            {
                auto named_religion = religion;
                named_religion.choose_religion->name_geometry =
                    ChooseReligionOwnerEvidence::NameGeometry{1280, 960,
                        {244, 307, 240, 32}, {390, 350, 500, 260},
                        {442, 453, 396, 20}, {468, 508, 120, 42},
                        {604, 508, 120, 42}, {740, 508, 120, 42}};
                require(valid_religion_name_geometry(
                            *named_religion.choose_religion->name_geometry),
                        "presenter supplies measured religion name geometry");
                auto named_snapshot = UiFrameSnapshot(60, 0, 1)
                    .with<PopupFrame>(std::make_shared<const PopupFrame>(
                        PopupFrame{0, {named_religion}, 1, 91}));
                CountingBackend counting_backend;
                LiveUiHost name_host(registry,
                    [&] { return named_snapshot; }, &counting_backend);
                auto name_frame = name_host.begin_frame(1280, 960);
                require(name_host.dispatch(UiOpenScreen{"ChooseReligion"},
                            name_frame).status == UiCommandStatus::navigation,
                        "current event opens religion name stage");
                name_frame = name_host.begin_frame(1280, 960);
                auto name_plan = plan_popup_screen(*religion_source,
                    name_frame.snapshot, 1280, 960, &counting_backend);
                require(name_host.dispatch(name_plan.hits[2].command,
                            name_frame).status == UiCommandStatus::navigation,
                        "current owner selects symbol before editing its name");
                name_frame = name_host.begin_frame(1280, 960);
                name_plan = plan_popup_screen(*religion_source,
                    name_frame.snapshot, 1280, 960, &counting_backend);
                require(name_plan.hits.size() == 18 &&
                        name_plan.hits[15].bounds.x == 244 &&
                        name_host.dispatch(name_plan.hits[15].command,
                            name_frame).status == UiCommandStatus::navigation,
                        "measured LabelReligionName opens the local name dialog");
                name_frame = name_host.begin_frame(1280, 960);
                name_plan = plan_popup_screen(*religion_source,
                    name_frame.snapshot, 1280, 960, &counting_backend);
                const auto *draft = name_frame.snapshot.find<PopupTextView>();
                require(draft && draft->text == "Religion 1" &&
                        name_plan.edit_box && name_plan.edit_box->x == 442 &&
                        name_plan.hits.size() == 4 &&
                        !name_plan.hits[0].enabled &&
                        !name_plan.scroll_bounds,
                        "authored name dialog covers symbol hits and owns EditBox focus");
                require(name_host.dispatch(UiGameplayCommand::make(
                            "ChooseReligion", ReligionUiAction{
                                ReligionUiActionKind::select_symbol, 91, 81, 3}),
                            name_frame).status == UiCommandStatus::unavailable,
                        "name dialog blocks underlying symbol commands");
                require(name_host.edit_popup_text("", name_frame).status ==
                            UiCommandStatus::navigation,
                        "empty draft can be entered locally to show native error");
                name_frame = name_host.begin_frame(1280, 960);
                name_plan = plan_popup_screen(*religion_source,
                    name_frame.snapshot, 1280, 960, &counting_backend);
                require(name_host.dispatch(name_plan.hits[3].command,
                            name_frame).status == UiCommandStatus::unavailable,
                        "empty religion name cannot be accepted");
                name_frame = name_host.begin_frame(1280, 960);
                const auto *selection =
                    name_frame.snapshot.find<ReligionSelectionView>();
                require(selection && selection->editing_name &&
                        selection->name_error && selection->name == "Religion 1",
                        "empty name leaves original name and dialog open");
                require(name_host.edit_popup_text("New Religion", name_frame)
                            .status == UiCommandStatus::navigation,
                        "name text edits remain local to current popup owner");
                name_frame = name_host.begin_frame(1280, 960);
                name_plan = plan_popup_screen(*religion_source,
                    name_frame.snapshot, 1280, 960, &counting_backend);
                require(name_host.dispatch(name_plan.hits[2].command,
                            name_frame).status == UiCommandStatus::navigation,
                        "Default restores owner-supplied religion label");
                name_frame = name_host.begin_frame(1280, 960);
                require(name_frame.snapshot.find<PopupTextView>()->text ==
                            "Religion 1",
                        "Default changes only the name draft");
                require(name_host.edit_popup_text("New Religion", name_frame)
                            .status == UiCommandStatus::navigation,
                        "edited draft can replace default name");
                name_frame = name_host.begin_frame(1280, 960);
                name_plan = plan_popup_screen(*religion_source,
                    name_frame.snapshot, 1280, 960, &counting_backend);
                require(name_host.dispatch(name_plan.hits[3].command,
                            name_frame).status == UiCommandStatus::navigation,
                        "OK stores only popup-local religion name");
                name_frame = name_host.begin_frame(1280, 960);
                selection = name_frame.snapshot.find<ReligionSelectionView>();
                name_plan = plan_popup_screen(*religion_source,
                    name_frame.snapshot, 1280, 960, &counting_backend);
                require(selection && selection->name == "New Religion" &&
                        !selection->editing_name && !name_plan.edit_box &&
                        !name_plan.hits[16].enabled &&
                        counting_backend.availability_calls == 0 &&
                        counting_backend.execution_calls == 0,
                        "renaming never enables FoundReligion or invokes GameCore");
                require(name_host.dispatch(name_plan.hits[15].command,
                            name_frame).status == UiCommandStatus::navigation,
                        "current name can reopen the local dialog");
                name_frame = name_host.begin_frame(1280, 960);
                require(name_host.edit_popup_text("Discard Me", name_frame)
                            .status == UiCommandStatus::navigation,
                        "second draft remains local");
                name_frame = name_host.begin_frame(1280, 960);
                name_plan = plan_popup_screen(*religion_source,
                    name_frame.snapshot, 1280, 960, &counting_backend);
                require(name_host.dispatch(name_plan.hits[1].command,
                            name_frame).status == UiCommandStatus::navigation,
                        "Cancel closes only the local name dialog");
                name_frame = name_host.begin_frame(1280, 960);
                selection = name_frame.snapshot.find<ReligionSelectionView>();
                require(selection && selection->name == "New Religion" &&
                        !selection->editing_name &&
                        counting_backend.execution_calls == 0,
                        "Cancel discards draft without changing name or gameplay");
                name_frame = name_host.begin_frame(1920, 1080);
                require(!name_frame.snapshot.find<ReligionSelectionView>(),
                        "viewport change discards owner-measured name stage");
                auto new_owner = named_religion;
                new_owner.choose_religion->owner_sequence = 92;
                named_snapshot = UiFrameSnapshot(61, 0, 1)
                    .with<PopupFrame>(std::make_shared<const PopupFrame>(
                        PopupFrame{0, {new_owner}, 1, 92}));
                name_frame = name_host.begin_frame(1280, 960);
                require(!name_frame.snapshot.find<ReligionSelectionView>(),
                        "replacement event cannot inherit typed local name");
            }
            const auto *archaeology_source = catalog.find("ChooseArchaeology");
            require(archaeology_source &&
                    archaeology_source->selected_source_path.find("Expansion2") !=
                        std::string::npos,
                    "archaeology uses selected BNW XML");
            PopupEvent archaeology;
            archaeology.context_id = "ChooseArchaeology";
            archaeology.event_id = 131;
            archaeology.dismissible = true;
            archaeology.choices = {{1, "Landmark", {}, true},
                                   {2, "First player's artifact", {}, true},
                                   {3, "Second player's artifact", {}, true}};
            archaeology.choose_archaeology = ChooseArchaeologyOwnerEvidence{
                0, 1, 72, 131, 6, 90, false,
                archaeology_source->selected_source_path, 1280, 960,
                {{18, 430, 390, 32}, {18, 470, 390, 32},
                 {18, 510, 390, 32}}};
            const auto archaeology_frame = UiFrameSnapshot(80, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {archaeology}, 1, 72}));
            const auto archaeology_plan = plan_popup_screen(*archaeology_source,
                archaeology_frame, 1280, 960, &backend);
            require(!archaeology_plan.scroll_bounds &&
                    archaeology_plan.hits.size() == 4 &&
                    archaeology_plan.hits[0].bounds.x == 18 &&
                    archaeology_plan.hits[0].bounds.y == 430 &&
                    !archaeology_plan.hits[0].enabled &&
                    archaeology_plan.hits[3].enabled,
                    "owner-measured rows stay disabled without synthetic wheel movement");
            const auto *archaeology_choice = std::get_if<UiGameplayCommand>(
                &archaeology_plan.hits.front().command);
            require(archaeology_choice &&
                    archaeology_choice->as<PopupDecision>() &&
                    archaeology_choice->as<PopupDecision>()->choice_id == 1,
                    "archaeology row carries the typed owner choice ID");
            const auto absent_archaeology = UiFrameSnapshot(80, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {PopupEvent{.context_id = "ChooseArchaeology",
                        .event_id = 131}}, 1, 72}));
            require(plan_popup_screen(*archaeology_source,
                        absent_archaeology, 1280, 960).hits.empty(),
                    "saved archaeology rules cannot publish choice hits");
            LiveUiHost absent_archaeology_host(registry,
                [&] { return absent_archaeology; }, &backend);
            const auto absent_archaeology_frame =
                absent_archaeology_host.begin_frame(1280, 960);
            require(absent_archaeology_host.dispatch(
                        UiOpenScreen{"ChooseArchaeology"},
                        absent_archaeology_frame).status ==
                    UiCommandStatus::unavailable,
                    "missing archaeology owner event cannot take modal focus");
            LiveUiHost archaeology_host(registry,
                [&] { return archaeology_frame; }, &backend);
            auto archaeology_focused = archaeology_host.begin_frame(1280, 960);
            require(archaeology_host.dispatch(UiOpenScreen{"ChooseArchaeology"},
                        archaeology_focused).status == UiCommandStatus::navigation,
                    "current archaeology owner event takes modal focus");
            archaeology_focused = archaeology_host.begin_frame(1280, 960);
            require(archaeology_host.dispatch(archaeology_plan.hits.front().command,
                        archaeology_focused).status == UiCommandStatus::unavailable,
                    "archaeology choice cannot bypass confirmation and world effects");
            auto invalid_archaeology = archaeology;
            invalid_archaeology.choose_archaeology->owner_sequence = 73;
            const auto invalid_archaeology_frame = UiFrameSnapshot(80, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {invalid_archaeology}, 1, 72}));
            require(plan_popup_screen(*archaeology_source,
                        invalid_archaeology_frame, 1280, 960).hits.empty(),
                    "stale archaeology owner token cannot publish row hits");
            invalid_archaeology = archaeology;
            invalid_archaeology.choose_archaeology->selected_source_path =
                "Assets/UI/InGame/Popups/ChooseArchaeologyPopup.xml";
            require(plan_popup_screen(*archaeology_source,
                UiFrameSnapshot(80, 0, 1).with<PopupFrame>(
                    std::make_shared<const PopupFrame>(
                        PopupFrame{0, {invalid_archaeology}, 1, 72})),
                1280, 960).hits.empty(),
                "wrong archaeology XML source cannot publish row hits");
            invalid_archaeology = archaeology;
            invalid_archaeology.choose_archaeology->measured_choice_bounds.clear();
            const auto unmeasured = plan_popup_screen(*archaeology_source,
                UiFrameSnapshot(80, 0, 1).with<PopupFrame>(
                    std::make_shared<const PopupFrame>(
                        PopupFrame{0, {invalid_archaeology}, 1, 72})),
                1280, 960);
            require(unmeasured.hits.size() == 1 &&
                    std::holds_alternative<UiCloseScreen>(
                        unmeasured.hits.front().command) &&
                    !unmeasured.unresolved.empty(),
                    "missing dynamic row geometry leaves only authored close navigation");
            invalid_archaeology = archaeology;
            invalid_archaeology.choose_archaeology->written_artifact = true;
            require(plan_popup_screen(*archaeology_source,
                UiFrameSnapshot(80, 0, 1).with<PopupFrame>(
                    std::make_shared<const PopupFrame>(
                        PopupFrame{0, {invalid_archaeology}, 1, 72})),
                1280, 960).hits.empty(),
                "written artifact cannot inherit ordinary artifact choice IDs");
            const auto *pantheon_source = catalog.find("ChoosePantheon");
            require(pantheon_source &&
                    pantheon_source->selected_source_path.find("Expansion2") !=
                        std::string::npos &&
                    pantheon_source->contains("ItemScrollPanel") &&
                    pantheon_source->contains("ChooseConfirm"),
                    "selected BNW pantheon XML exposes item and confirmation controls");
            PopupEvent pantheon;
            pantheon.context_id = "ChoosePantheon";
            pantheon.event_id = 141;
            pantheon.title = "Choose a pantheon";
            pantheon.dismissible = true;
            pantheon.choices = {{11, "Belief A", "Live description A", true},
                                {12, "Belief B", "Live description B", true},
                                {13, "Belief C", "Live description C", true}};
            pantheon.choose_pantheon = ChoosePantheonOwnerEvidence{
                0, 1, 81, 141, 1, ChoosePantheonMode::pantheon,
                pantheon_source->selected_source_path, 1280, 960,
                {{11, {18, 150, 390, 64}}, {12, {18, 220, 390, 64}}}};
            const auto pantheon_frame = UiFrameSnapshot(90, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {pantheon}, 1, 81}));
            const auto pantheon_plan = plan_popup_screen(*pantheon_source,
                pantheon_frame, 1280, 960, &backend);
            require(!pantheon_plan.scroll_bounds &&
                    pantheon_plan.hits.size() == 3 &&
                    !pantheon_plan.hits[0].enabled &&
                    !pantheon_plan.hits[1].enabled &&
                    std::holds_alternative<UiCloseScreen>(
                        pantheon_plan.hits[2].command),
                    "owner-measured pantheon rows remain disabled and have no synthetic wheel");
            const auto *belief_choice = std::get_if<UiGameplayCommand>(
                &pantheon_plan.hits[0].command);
            require(belief_choice && belief_choice->as<PantheonUiAction>() &&
                    belief_choice->as<PantheonUiAction>()->belief_id == 11,
                    "visible pantheon row retains owner belief ID");
            const auto absent_pantheon = UiFrameSnapshot(90, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {PopupEvent{.context_id = "ChoosePantheon",
                        .event_id = 141}}, 1, 81}));
            LiveUiHost absent_pantheon_host(registry,
                [&] { return absent_pantheon; }, &backend);
            const auto absent_pantheon_frame =
                absent_pantheon_host.begin_frame(1280, 960);
            require(plan_popup_screen(*pantheon_source, absent_pantheon,
                        1280, 960).hits.empty() &&
                    absent_pantheon_host.dispatch(UiOpenScreen{"ChoosePantheon"},
                        absent_pantheon_frame).status == UiCommandStatus::unavailable,
                    "saved pantheon rules without live owner event cannot open or hit");
            LiveUiHost pantheon_host(registry,
                [&] { return pantheon_frame; }, &backend);
            auto pantheon_focused = pantheon_host.begin_frame(1280, 960);
            require(pantheon_host.dispatch(UiOpenScreen{"ChoosePantheon"},
                        pantheon_focused).status == UiCommandStatus::navigation,
                    "current pantheon event takes modal focus");
            pantheon_focused = pantheon_host.begin_frame(1280, 960);
            require(pantheon_host.dispatch(pantheon_plan.hits[0].command,
                        pantheon_focused).status == UiCommandStatus::unavailable,
                    "belief choice cannot bypass owner confirmation and GameCore effects");
            const auto wheel = pantheon_host.wheel_popup(pantheon_focused,
                20, 155, -120, {0, 143, 418, 762});
            require(wheel.consumed &&
                    wheel.command.status == UiCommandStatus::unavailable &&
                    pantheon_host.dispatch(UiGameplayCommand::make("ChoosePantheon",
                        PopupScroll{"ChoosePantheon", 141, 0,
                            PopupScrollDirection::down}), pantheon_focused).status ==
                        UiCommandStatus::unavailable,
                    "native pantheon wheel and direct synthetic scroll remain unavailable");
            const auto archaeology_wheel = archaeology_host.wheel_popup(
                archaeology_focused, 20, 430, -120, {0, 422, 418, 484});
            require(archaeology_wheel.consumed &&
                    archaeology_wheel.command.status == UiCommandStatus::unavailable &&
                    archaeology_host.dispatch(UiGameplayCommand::make(
                        "ChooseArchaeology", PopupScroll{"ChooseArchaeology",
                            131, 0, PopupScrollDirection::down}),
                        archaeology_focused).status == UiCommandStatus::unavailable,
                    "native archaeology wheel and synthetic scroll remain unavailable");
            auto stale_pantheon = pantheon;
            stale_pantheon.choose_pantheon->owner_sequence = 82;
            require(plan_popup_screen(*pantheon_source,
                UiFrameSnapshot(90, 0, 1).with<PopupFrame>(
                    std::make_shared<const PopupFrame>(
                        PopupFrame{0, {stale_pantheon}, 1, 81})),
                1280, 960).hits.empty(),
                "stale pantheon owner token cannot publish belief hits");
            stale_pantheon = pantheon;
            stale_pantheon.choose_pantheon->mode = ChoosePantheonMode::reformation;
            require(plan_popup_screen(*pantheon_source,
                UiFrameSnapshot(90, 0, 1).with<PopupFrame>(
                    std::make_shared<const PopupFrame>(
                        PopupFrame{0, {stale_pantheon}, 1, 81})),
                1280, 960).hits.empty(),
                "pantheon mode must agree with the owner Data2 value");
            stale_pantheon = pantheon;
            stale_pantheon.choose_pantheon->visible_choice_bounds.clear();
            const auto no_pantheon_rows = plan_popup_screen(*pantheon_source,
                UiFrameSnapshot(90, 0, 1).with<PopupFrame>(
                    std::make_shared<const PopupFrame>(
                        PopupFrame{0, {stale_pantheon}, 1, 81})),
                1280, 960);
            require(no_pantheon_rows.hits.size() == 1 &&
                    std::holds_alternative<UiCloseScreen>(
                        no_pantheon_rows.hits.front().command),
                    "missing pantheon row measurements leave only close navigation");
            auto staged_pantheon = pantheon;
            staged_pantheon.choose_pantheon->confirm_geometry =
                ChoosePantheonOwnerEvidence::ConfirmGeometry{
                    {390, 325, 500, 310}, {440, 470, 400, 42},
                    {440, 530, 400, 42}};
            staged_pantheon.choose_pantheon->confirm_text_by_choice = {
                {11, "Confirm Belief A?"}, {12, "Confirm Belief B?"}};
            UiFrameSnapshot staged_snapshot = UiFrameSnapshot(91, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {staged_pantheon}, 1, 81}));
            CountingBackend counting_backend;
            LiveUiHost staged_host(registry,
                [&] { return staged_snapshot; }, &counting_backend);
            auto staged_frame = staged_host.begin_frame(1280, 960);
            require(staged_host.dispatch(UiOpenScreen{"ChoosePantheon"},
                        staged_frame).status == UiCommandStatus::navigation,
                    "owner-backed pantheon event opens the local screen");
            staged_frame = staged_host.begin_frame(1280, 960);
            const auto staged_plan = plan_popup_screen(*pantheon_source,
                staged_frame.snapshot, 1280, 960, &counting_backend);
            require(staged_plan.hits.size() == 3 &&
                    staged_plan.hits[0].enabled &&
                    staged_plan.hits[1].enabled,
                    "visible belief rows open only the measured local confirmation");
            require(staged_host.dispatch(staged_plan.hits[0].command,
                        staged_frame).status == UiCommandStatus::navigation &&
                    counting_backend.availability_calls == 0 &&
                    counting_backend.execution_calls == 0,
                    "belief selection changes UI state without gameplay backend or RNG");
            require(staged_host.dispatch(staged_plan.hits[0].command,
                        staged_frame).status == UiCommandStatus::invalid,
                    "selected row cannot be replayed from a stale UI frame");
            staged_frame = staged_host.begin_frame(1280, 960);
            const auto *confirm_view = staged_frame.snapshot.find<PantheonConfirmView>();
            const auto confirm_plan = plan_popup_screen(*pantheon_source,
                staged_frame.snapshot, 1280, 960, &counting_backend);
            require(confirm_view && confirm_view->belief_id == 11 &&
                    confirm_plan.hits.size() == 6 &&
                    !confirm_plan.hits[4].enabled &&
                    confirm_plan.hits[5].enabled,
                    "same owner event shows Yes disabled and No enabled above covered rows");
            require(staged_host.dispatch(confirm_plan.hits[4].command,
                        staged_frame).status == UiCommandStatus::unavailable &&
                    counting_backend.execution_calls == 0,
                    "Yes cannot send Network.SendFoundPantheon without GameCore transaction");
            require(staged_host.dispatch(confirm_plan.hits[5].command,
                        staged_frame).status == UiCommandStatus::navigation,
                    "No closes only local confirmation");
            staged_frame = staged_host.begin_frame(1280, 960);
            require(!staged_frame.snapshot.find<PantheonConfirmView>() &&
                    staged_host.screens().focused_screen() ==
                        std::optional<std::string>{"ChoosePantheon"},
                    "No returns to the same owner-backed belief list");
            const auto reopened_plan = plan_popup_screen(*pantheon_source,
                staged_frame.snapshot, 1280, 960, &counting_backend);
            require(staged_host.dispatch(reopened_plan.hits[0].command,
                        staged_frame).status == UiCommandStatus::navigation,
                    "same current row can open a new local confirmation");
            staged_frame = staged_host.begin_frame(1280, 960);
            require(staged_frame.snapshot.find<PantheonConfirmView>(),
                    "local confirmation is active before owner replacement");
            auto changed_owner = staged_pantheon;
            changed_owner.choose_pantheon->owner_sequence = 82;
            staged_snapshot = UiFrameSnapshot(92, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(
                    PopupFrame{0, {changed_owner}, 1, 82}));
            staged_frame = staged_host.begin_frame(1280, 960);
            require(!staged_frame.snapshot.find<PantheonConfirmView>(),
                    "new popup owner token does not inherit local confirmation");
            staged_snapshot = UiFrameSnapshot(93, 0, 1);
            staged_frame = staged_host.begin_frame(1280, 960);
            require(!staged_frame.snapshot.find<PantheonConfirmView>() &&
                    !staged_host.screens().focused_screen() &&
                    plan_popup_screen(*pantheon_source, staged_frame.snapshot,
                        1280, 960).hits.empty(),
                    "missing owner event removes pantheon modal focus and hit state");
            PopupFrame all_events;
            all_events.player_id = 0;
            for (const auto &entry : catalog.sources()) {
                PopupEvent sample;
                sample.context_id = entry.context_id;
                sample.event_id = 1;
                sample.title = "Live title";
                sample.choices.push_back({1, "Unavailable", {}, false});
                all_events.events.push_back(std::move(sample));
            }
            const auto all_frame = UiFrameSnapshot(3, 0, 1)
                .with<PopupFrame>(std::make_shared<const PopupFrame>(all_events));
            std::size_t painted = 0;
            for (const auto &entry : catalog.sources()) {
                const auto planned = plan_popup_screen(entry, all_frame, 1920, 1080, &backend);
                if (!planned.paints.empty()) ++painted;
                else std::cout << "no fixed 2D panel: " << entry.context_id << '\n';
                for (const auto &hit : planned.hits)
                    if (std::holds_alternative<UiGameplayCommand>(hit.command))
                        require(!hit.enabled,
                            "unavailable live options remain disabled in every U8 context");
            }
            UiScreenController controller(registry);
            require(!plan_focused_popup(catalog, controller, frame, 1920, 1080),
                    "no popup painted without modal focus");
            require(controller.dispatch(UiOpenScreen{"GenericPopup"}, frame).status ==
                    UiCommandStatus::navigation, "open first modal popup");
            auto focused = plan_focused_popup(catalog, controller, frame, 1920, 1080, &backend);
            require(focused && focused->context_id == "GenericPopup" &&
                    !focused->paints.empty(), "focused popup has paint operations");
            require(controller.dispatch(UiOpenScreen{"NewEraPopup"}, frame).status ==
                    UiCommandStatus::navigation, "queue another modal popup");
            focused = plan_focused_popup(catalog, controller, frame, 1920, 1080, &backend);
            require(focused && focused->context_id == "NewEraPopup" &&
                    focused->paints.empty(), "covered popup cannot paint or take input");
            require(controller.dispatch(UiCloseScreen{"NewEraPopup"}, frame).status ==
                    UiCommandStatus::navigation, "close top modal popup");
            focused = plan_focused_popup(catalog, controller, frame, 1920, 1080, &backend);
            require(focused && focused->context_id == "GenericPopup",
                    "previous popup regains focus after close");
            std::cout << "active BNW popup sources " << catalog.sources().size()
                      << ", XML-backed 2D plans " << painted
                      << ", unresolved " << catalog.unresolved().size() << '\n';
            for (const auto &reason : catalog.unresolved()) std::cout << reason << '\n';
        }
        std::cout << "ui popup tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
