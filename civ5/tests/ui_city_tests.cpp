#include "civ5/ui/city_screens.hpp"
#include <algorithm>
#include <filesystem>
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
        using namespace civ5::ui;
        civ5::runtime::CityYieldContext live;
        auto &delhi = live.players[0].cities[0];
        delhi.owner = 0;
        delhi.id = 0;
        delhi.x = 22;
        delhi.y = 15;
        delhi.population = 2;
        delhi.source_core = std::pair<std::uint64_t, std::uint64_t>{100, 300};
        delhi.production_orders = std::vector<civ5::runtime::CityYieldState::ProductionOrder>{
            {1, 5, -1, false, false, std::nullopt}};
        delhi.production_progress.emplace();
        delhi.production_progress->buildings[5] = 235;
        civ5::saves::SavedSessionSnapshot saved;
        saved.all_players.emplace();
        auto &saved_player = saved.all_players->players.emplace_back();
        saved_player.saved_player_id = 0;
        auto &saved_city = saved_player.cities.cities.emplace_back();
        saved_city.core.start_offset = 100;
        saved_city.core.end_offset = 300;
        saved_city.core.fields.push_back({"m_iID", civ5::saves::CityCoreEncoding::i32});
        saved_city.core.fields.back().values = {0};
        saved_city.core.fields.push_back({"m_iX", civ5::saves::CityCoreEncoding::i32});
        saved_city.core.fields.back().values = {22};
        saved_city.core.fields.push_back({"m_iY", civ5::saves::CityCoreEncoding::i32});
        saved_city.core.fields.back().values = {15};
        saved_city.core.fields.push_back({"m_eOwner", civ5::saves::CityCoreEncoding::i32});
        saved_city.core.fields.back().values = {0};
        saved_city.core.fields.push_back({"m_strName", civ5::saves::CityCoreEncoding::string32});
        const std::string key = "TXT_KEY_CITY_NAME_DELHI";
        saved_city.core.fields.back().bytes.assign(key.begin(), key.end());
        auto state = capture_city_screen_frame(live, saved, 0, 0,
            [](std::string_view value) -> std::optional<std::string> {
                return value == "TXT_KEY_CITY_NAME_DELHI" ? std::optional("Delhi") : std::nullopt;
            });
        require(state.cities.size() == 1 && state.cities[0].population == 2 &&
                state.cities[0].queue_resolved && state.cities[0].queue.size() == 1 &&
                state.cities[0].name == "Delhi" &&
                state.cities[0].current_production_progress_times100 == 235,
                "live city population and production queue with matched saved name");
        auto stale_coordinates = live;
        stale_coordinates.players[0].cities.at(0).x = 23;
        require(!capture_city_screen_frame(stale_coordinates, saved, 0).cities.front().name,
                "saved city name is withheld when live coordinates change");
        civ5::content::RulesDatabase database;
        database.apply_sql("CREATE TABLE Buildings (ID INTEGER, Type TEXT, Description TEXT);"
                           "INSERT INTO Buildings VALUES (5, 'BUILDING_MONUMENT', 'TXT_KEY_MONUMENT');");
        resolve_city_queue_names(state, database, [](std::string_view key) {
            return key == "TXT_KEY_MONUMENT" ? std::string("Monument") : std::string(key);
        });
        require(state.cities[0].queue[0].name == "Monument",
                "active Info and resolved text supply current production name");
        auto untranslated = state;
        untranslated.cities[0].queue[0].name.reset();
        resolve_city_queue_names(untranslated, database, [](std::string_view key) {
            return std::string(key);
        });
        require(!untranslated.cities[0].queue[0].name,
                "untranslated production key is not shown as a name");
        auto frame = UiFrameSnapshot(1, 0, 1)
            .with<CityScreenFrame>(std::make_shared<const CityScreenFrame>(state));
        require(selected_city(frame) && selected_city(frame)->name == "Delhi",
                "selected active player city");
        require(!selected_city(UiFrameSnapshot(2, 1, 1)
            .with<CityScreenFrame>(std::make_shared<const CityScreenFrame>(state))),
                "stale player city is rejected");
        auto foreign_owner = state;
        foreign_owner.cities.front().owner = 1;
        require(!selected_city(UiFrameSnapshot(3, 0, 1)
            .with<CityScreenFrame>(
                std::make_shared<const CityScreenFrame>(foreign_owner))),
            "selected city with a different current owner is rejected");
        CityGameplayBackend backend;
        const auto command = std::get<UiGameplayCommand>(city_choose_production(0, 1, 5));
        require(backend.availability(command, frame).status == UiCommandStatus::unavailable &&
                backend.execute(command, frame).status == UiCommandStatus::unavailable,
                "no false success for production mutation");
        require(backend.availability(std::get<UiGameplayCommand>(city_choose_production(1, 1, 5)),
                                     frame).status == UiCommandStatus::invalid,
                "wrong city production rejected");
        require(std::get<UiOpenScreen>(city_open_command()).id == "CityView" &&
                std::get<UiOpenScreen>(city_production_popup_command()).id == "ProductionPopup",
                "typed city navigation");

        if (argc == 2) {
            const std::filesystem::path root(argv[1]);
            auto source = std::make_shared<civ5::assets::LooseResourceIndex>(root);
            source->add_file(root / "Assets/DLC/Expansion2/UI/InGame/CityView/CityView.xml");
            source->add_file(root / "Assets/DLC/Expansion2/UI/InGame/Popups/ProductionPopup.xml");
            civ5::assets::ResourceMountTable resources(civ5::assets::resource_lookup(source));
            auto view = read_city_screen_source(resources, "CityView");
            auto popup = read_city_screen_source(resources, "ProductionPopup");
            require(view.contains("ReturnToMapButton") && view.contains("ProdQueueBackground") &&
                    popup.contains("StackOStacks") && popup.contains("CloseButton") &&
                    popup.contains("ScrollPanel") && popup.contains("UpButton") &&
                    popup.contains("DownButton") &&
                    production_popup_visible_rows(popup, 960) == 13 &&
                    production_popup_visible_rows(popup, 1080) == 16,
                    "active BNW city controls");
            require(view.selected_source_path.find("Expansion2") != std::string::npos &&
                    popup.selected_source_path.find("Expansion2") != std::string::npos,
                    "BNW resource provenance");
            const auto city_960 = layout_city_view(view, 1280, 960);
            const auto city_1080 = layout_city_view(view, 1920, 1080);
            require(city_960 && city_1080 &&
                    city_960->title_panel.x == 384 && city_960->title_panel.y == 36 &&
                    city_1080->title_panel.x == 704 && city_1080->title_panel.y == 36 &&
                    city_960->production_panel.height == 280 &&
                    city_960->production_panel.y == 688 &&
                    city_960->production_button.x == 244 &&
                    city_960->production_button.y == 683 &&
                    city_1080->production_button.y == 803 &&
                    city_960->return_button.x == 500 &&
                    city_960->return_button.y == 840,
                    "selected CityView XML and default Lua panel geometry");
            auto changed_view = view;
            const auto change_button_size = [&](const auto &self,
                    civ5::content::XmlNode &node) -> bool {
                if (node.attribute("ID") == "ProductionButton") {
                    for (auto &[key, value] : node.attributes)
                        if (key == "Size") { value = "220,35"; return true; }
                }
                for (auto &child : node.children)
                    if (self(self, child)) return true;
                return false;
            };
            require(change_button_size(change_button_size, changed_view.document) &&
                    !layout_city_view(changed_view, 1280, 960),
                    "changed active XML cannot inherit old button coordinates");
            changed_view.selected_source_path.clear();
            require(!layout_city_view(changed_view, 1280, 960),
                    "different city source fails closed");
            const auto localize_button = [](std::string_view key)
                -> std::optional<std::string> {
                if (key == "TXT_KEY_CITYVIEW_CHOOSE_PROD") return "Choose Production";
                if (key == "TXT_KEY_CITYVIEW_CHANGE_PROD") return "Change Production";
                return std::nullopt;
            };
            const auto view_plan = plan_city_screen(view, frame, 1920, 1080,
                                                    &backend, localize_button);
            const auto popup_plan = plan_city_screen(popup, frame, 1920, 1080, &backend);
            const auto has_label = [](const CityScreenPlan &plan, std::string_view label) {
                return std::any_of(plan.paints.begin(), plan.paints.end(),
                    [&](const WorldViewPaint &paint) { return paint.text == label; });
            };
            require(has_label(view_plan, "Change Production"),
                    "known live queue uses authored Change Production label");
            auto empty_queue = state;
            empty_queue.cities[0].queue.clear();
            const auto empty_frame = UiFrameSnapshot(2, 0, 1).with<CityScreenFrame>(
                std::make_shared<const CityScreenFrame>(empty_queue));
            const auto empty_plan = plan_city_screen(view, empty_frame, 1920, 1080,
                                                      &backend, localize_button);
            require(has_label(empty_plan, "Choose Production") &&
                    !has_label(empty_plan, "Change Production"),
                    "known empty live queue uses authored Choose Production label");
            empty_queue.cities[0].queue_resolved = false;
            const auto unknown_frame = UiFrameSnapshot(3, 0, 1).with<CityScreenFrame>(
                std::make_shared<const CityScreenFrame>(empty_queue));
            const auto unknown_plan = plan_city_screen(view, unknown_frame, 1920, 1080,
                                                        &backend, localize_button);
            require(!has_label(unknown_plan, "Choose Production") &&
                    !has_label(unknown_plan, "Change Production"),
                    "unknown queue cannot imply a choose or change state");
            require(view_plan.paints.size() >= 5 && view_plan.hits.size() >= 2 &&
                    popup_plan.paints.size() >= 2 && popup_plan.hits.size() == 1,
                    "city screens emit visible 2D paint and safe close operations");
            require(view_plan.paints.front().kind == WorldViewPaintKind::texture &&
                    view_plan.paints.front().source_key == city_1080->title_texture &&
                    view_plan.paints.front().destination.x == city_1080->title_panel.x &&
                    view_plan.paints[1].text == "Delhi" &&
                    view_plan.paints[1].destination.x > 700 &&
                    view_plan.hits.front().bounds.x == city_1080->production_button.x &&
                    view_plan.hits.front().bounds.y == city_1080->production_button.y,
                    "Delhi title and production hit use selected BNW city geometry");
            require(popup_plan.unresolved.front().find("choices") != std::string::npos,
                    "saved queue is not misrepresented as buildable choices");
        }
        std::cout << "ui city tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
