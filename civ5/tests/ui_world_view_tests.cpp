#include "civ5/ui/world_view.hpp"
#include "civ5/ui/live_city_banner_projection.hpp"
#include "civ5/ui/live_pointer_priority.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
using namespace civ5;
namespace fs = std::filesystem;

void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}

struct Backend final : ui::UiGameplayBackend {
    int executed = 0;
    ui::UiCommandResult availability(const ui::UiGameplayCommand &command,
                                      const ui::UiFrameSnapshot &) const override {
        return command.as<ui::WorldViewAction>()
            ? ui::UiCommandResult{ui::UiCommandStatus::ready, {}}
            : ui::UiCommandResult{ui::UiCommandStatus::unavailable, "Unknown action"};
    }
    ui::UiCommandResult execute(const ui::UiGameplayCommand &command,
                                 const ui::UiFrameSnapshot &) override {
        if (!command.as<ui::WorldViewAction>())
            return {ui::UiCommandStatus::unavailable, "Unknown action"};
        ++executed;
        return {ui::UiCommandStatus::performed, {}};
    }
};

struct Fixture {
    fs::path root = fs::temp_directory_path() / "civis-u4-world-view-test";
    std::shared_ptr<assets::LooseResourceIndex> source;
    std::unique_ptr<assets::ResourceMountTable> resources;
    Fixture() {
        fs::create_directories(root);
        source = std::make_shared<assets::LooseResourceIndex>(root);
        resources = std::make_unique<assets::ResourceMountTable>(assets::resource_lookup(source));
    }
    ~Fixture() { std::error_code ignored; fs::remove_all(root, ignored); }
    void add(std::string_view name, std::string_view xml) {
        const auto path = root / fs::path(name);
        fs::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary);
        out << xml;
        out.close();
        source->add_file(fs::path(name));
    }
};
} // namespace

int main(int argc, char **argv) {
    try {
        const std::vector<ui::WorldViewRect> pointer_hits{{600, 460, 80, 40}};
        require(ui::live_ui_blocks_camera_pointer(pointer_hits, false, true, 640, 480),
                "measured city banner takes pointer priority over camera");
        require(!ui::live_ui_blocks_camera_pointer(pointer_hits, false, true, 400, 480),
                "empty ordinary WorldView remains available to camera");
        require(ui::live_ui_blocks_camera_pointer(pointer_hits, false, true, 400, 16),
                "visible top panel takes pointer priority over camera");
        require(ui::live_ui_blocks_camera_pointer(pointer_hits, true, true, 400, 480),
                "focused modal takes pointer priority over camera");
        const auto in_game = content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/CityView/CityView" ID="CityView" Hidden="True"/>
        </Context>)");
        const auto world = content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/ActionInfoPanel" ID="ActionInfoPanel"/>
          <LuaContext FileName="Assets/UI/InGame/NewTurn" ID="NewTurn"/>
          <LuaContext FileName="Assets/UI/InGame/TurnProcessing" ID="TurnProcessing"/>
          <LuaContext FileName="Assets/UI/InGame/WorldView/PlotHelpText" ID="PlotHelpText"/>
          <LuaContext FileName="Assets/UI/InGame/WorldView/PathHelpManager" ID="PathHelpManager"/>
          <LuaContext FileName="Assets/UI/InGame/WorldView/UnitPanel" ID="UnitPanel"/>
          <LuaContext FileName="Assets/UI/InGame/WorldView/EnemyUnitPanel" ID="EnemyUnitPanel"/>
          <LuaContext FileName="Assets/UI/InGame/CityBannerManager" ID="CityBannerManager"/>
          <LuaContext FileName="Assets/UI/InGame/UnitFlagManager" ID="UnitFlagManager"/>
          <LuaContext FileName="Assets/UI/InGame/ResourceIconManager" ID="ResourceIconManager"/>
          <LuaContext FileName="Assets/UI/InGame/YieldIconManager" ID="YieldIconManager"/>
          <LuaContext FileName="Assets/UI/InGame/InfoCorner" ID="InfoCorner"/>
        </Context>)");
        const auto registry = ui::UiContextRegistry::from_documents(in_game, world);
        Fixture fixture;
        fixture.add("Assets/UI/InGame/WorldView/WorldView.xml", "<Context/>");
        fixture.add("Assets/UI/InGame/WorldView/ActionInfoPanel.xml",
                    "<Context><GridButton ID='EndTurnButton' Anchor='R,B' Offset='16,200' "
                    "Size='320,36'><GridData Style='Grid9DarkBlue8Button'/></GridButton>"
                    "<Label ID='EndTurnText' String='END TURN'/></Context>");
        fixture.add("Assets/UI/InGame/NewTurn.xml",
                    "<Context><Grid ID='NewTurnGrid' Anchor='C,T' Offset='0,190' "
                    "Size='Full,64'/></Context>");
        fixture.add("Assets/UI/InGame/TurnProcessing.xml",
                    "<Context><Grid ID='TurnProcessingGrid' Anchor='C,B' Offset='0,10' "
                    "Size='312,32'/></Context>");
        fixture.add("Assets/UI/InGame/WorldView/PlotHelpText.xml",
                    "<Context><Grid ID='TextBox' Anchor='R,B' Offset='-345,250' "
                    "Padding='8,8' Style='GridBlack8'><Label ID='Text' "
                    "Font='TwCenMT16'/></Grid></Context>");
        fixture.add("Assets/UI/InGame/WorldView/PathHelpManager.xml",
                    "<Context><Instance Name='TurnIndicator'><WorldAnchor ID='Anchor'>"
                    "<Label ID='TurnLabel' Anchor='C,C' Font='TwCenMT20'/>"
                    "</WorldAnchor></Instance><Container ID='Container'/></Context>");
        fixture.add("Assets/UI/InGame/WorldView/UnitPanel.xml",
                    "<Context><Grid ID='PrimaryStretchy' Style='Grid9DetailFive140'/>"
                    "<Button ID='UnitNameButton' Size='280,22'>"
                    "<Label ID='UnitName' Font='TwCenMT24'/></Button>"
                    "<Button ID='CycleLeft' Size='32,32' Texture='UnitPanelArrows.dds' TextureOffset='7,0'/>"
                    "<Button ID='CycleRight' Size='32,32' Texture='UnitPanelArrows.dds' TextureOffset='30,0'/>"
                    "<Image ID='UnitPortrait' Anchor='L,B' "
                    "Offset='-28,-23' Size='256,256' "
                    "Texture='UnitPortraitsAncientMedieval.dds'/>"
                    "<GridButton ID='UnitActionButton' "
                    "Size='50,50' Style='BaseButton'/></Context>");
        fixture.add("Assets/UI/InGame/WorldView/EnemyUnitPanel.xml", "<Context/>");
        fixture.add("Assets/UI/InGame/CityBannerManager.xml",
                    "<Context><Instance Name='TeamCityBanner'>"
                    "<Button ID='BannerButton' Size='111,40'/>"
                    "<Image ID='CityBannerBackgroundIcon' Size='111,32' "
                    "Texture='hover-only.dds'/>"
                    "<Image ID='CityBannerBackground' Size='111,32' "
                    "Texture='banner-body.dds'/>"
                    "<Image ID='CityBannerRightBackground' Size='32,32' "
                    "Texture='banner-left.dds'/>"
                    "<Image ID='CityBannerLeftBackground' Size='32,32' "
                    "Texture='banner-right.dds'/>"
                    "<Label ID='CityPopulation'/><Label ID='CityName'/>"
                    "</Instance></Context>");
        fixture.add("Assets/UI/InGame/UnitFlagManager.xml",
                    "<Context><Image ID='FlagBaseOutline' "
                    "Texture='Assets/UI/Art/Icons/Units/UnitFlagBase.dds'/></Context>");
        fixture.add("Assets/UI/InGame/ResourceIconManager.xml",
                    "<Context><Instance Name='ResourceIcon'><WorldAnchor>"
                    "<Image Size='64,64' Texture='resource-back.dds'/>"
                    "<Image ID='ResourceIcon' Size='64,64' "
                    "Texture='Assets/UI/Art/Icons/Resources512.dds'/>"
                    "</WorldAnchor></Instance></Context>");
        fixture.add("Assets/UI/InGame/YieldIconManager.xml",
                    "<Context><Image ID='Image' Size='128,128' "
                    "Texture='Assets/UI/Art/Icons/YieldAtlas.dds'/></Context>");
        fixture.add("Assets/UI/InGame/InfoCorner.xml",
                    "<Context><PullDown ID='LeftPull' Anchor='L,T' Offset='0,30' "
                    "Size='45,45'><InstanceData><Button ID='Button' Offset='110,0' "
                    "Size='45,45'/></InstanceData></PullDown></Context>");
        fixture.add("Assets/UI/InGame/WorldView/NotificationPanel.xml",
                    "<Context><Instance Name='GenericItem'><Container><SlideAnim>"
                    "<Button ID='GenericButton' "
                    "Texture='notification-frame.dds'><Image Texture='generic-icon.dds'/>"
                    "</Button></SlideAnim></Container></Instance></Context>");

        const auto skin = ui::WorldViewSkin::from_resources(*fixture.resources, registry);
        require(skin.source_keys().size() == 14, "all world sources including notification");
        require(skin.find("ActionInfoPanel", "EndTurnButton") &&
                    skin.find("ActionInfoPanel", "EndTurnButton")->style == "Grid9DarkBlue8Button",
                "authored end turn style");
        const ui::UiFrameSnapshot base(8, 7, 19);
        require(ui::plan_world_view(skin, registry, base, 1280, 960).paints.empty(),
                "no synthetic world data");
        terrain::WorldLayout banner_layout{};
        banner_layout.source_grid.width = 4;
        banner_layout.source_grid.height = 4;
        banner_layout.source_grid.second_matrix[0] = 1;
        banner_layout.source_grid.second_matrix[5] = 1;
        banner_layout.transforms[16] = 0.25f;
        banner_layout.transforms[21] = 0.25f;
        terrain::HeightOwnerParameters banner_height{};
        banner_height.scale_x = 1.f;
        banner_height.scale_y = 1.f;
        terrain::CameraFrameState banner_camera{};
        banner_camera.view_count = 1;
        banner_camera.views[0] = terrain::identity_matrix();
        banner_camera.projection = terrain::identity_matrix();
        ui::WorldViewState banner_world;
        banner_world.player_id = 7;
        banner_world.map_visibility = ui::WorldViewMapVisibility{
            7, 0, 4, 4, std::vector<ui::WorldViewPlotFog>(16, ui::WorldViewPlotFog::visible)};
        ui::CityScreenFrame banner_cities;
        banner_cities.player_id = 7;
        banner_cities.cities.push_back({7, 6, 2, 1, 2, std::string("Delhi")});
        const auto city_frame = base.with(std::make_shared<const ui::WorldViewState>(banner_world))
            .with(std::make_shared<const ui::CityScreenFrame>(banner_cities));
        std::vector<ui::LiveCityBannerProjectionAudit> banner_audit;
        const auto city_projected = ui::project_live_city_banners(city_frame, banner_layout,
            banner_height, banner_camera, 100, 100, &banner_audit);
        const auto *projected_world = city_projected.find<ui::WorldViewState>();
        require(projected_world && projected_world->city_banners.size() == 1 &&
                    projected_world->city_banners.front().screen.x == 75 &&
                    projected_world->city_banners.front().screen.y == 38 &&
                    projected_world->city_banners.front().label == "Delhi" &&
                    projected_world->city_banners.front().population == 2 &&
                    banner_audit.size() == 1 && banner_audit.front().clip_w == 1.0 &&
                    banner_audit.front().ndc_x == 0.5 &&
                    banner_audit.front().ndc_y == 0.25 &&
                    banner_world.city_banners.empty(),
                "live city projects from current camera without mutating source frame");
        // Planner fixture only: the production frame's borrowed owner fence
        // authenticates this metadata before draw/input/presentation.
        ui::BnwCityBannerAppearance appearance;
        appearance.owner_id=1;appearance.frame_number=8;appearance.active_player_id=7;
        appearance.player_id=7;appearance.city_id=6;appearance.turn=19;
        appearance.background={0.071f,0.533f,0.027f,0.7f};
        appearance.foreground={1.f,0.6f,0.196f,1.f};appearance.shadow={0,0,0,0.5f};
        ui::LiveCityBannerAppearances appearance_map;
        appearance_map.cities[{7,6}]=appearance;
        const auto appearance_frame=city_frame.with(
            std::make_shared<const ui::LiveCityBannerAppearances>(appearance_map));
        const auto appearance_projected=ui::project_live_city_banners(appearance_frame,
            banner_layout,banner_height,banner_camera,100,100);
        require(appearance_projected.find<ui::WorldViewState>()->city_banners.front().appearance &&
            appearance_projected.find<ui::WorldViewState>()->city_banners.front().appearance->background==appearance.background,
            "camera projection lost retained matching float city appearance");
        for (const auto changed : {0,1,2}) {
            auto stale=appearance_map;
            if(changed==0)stale.cities[{7,6}].frame_number=7;
            if(changed==1)stale.cities[{7,6}].turn=18;
            if(changed==2)stale.cities[{7,6}].player_id=8;
            const auto rejected=ui::project_live_city_banners(city_frame.with(
                std::make_shared<const ui::LiveCityBannerAppearances>(std::move(stale))),
                banner_layout,banner_height,banner_camera,100,100);
            require(!rejected.find<ui::WorldViewState>()->city_banners.front().appearance,
                "camera projection attached stale/wrong city appearance stamp");
        }
        banner_camera.views[0][12] = 3.f;
        const auto city_outside = ui::project_live_city_banners(city_frame, banner_layout,
            banner_height, banner_camera, 100, 100);
        require(city_outside.find<ui::WorldViewState>()->city_banners.empty(),
                "offscreen city has no banner or hit");
        banner_camera.views[0] = terrain::identity_matrix();
        banner_cities.cities.front().name.reset();
        const auto unnamed_frame = base.with(std::make_shared<const ui::WorldViewState>(banner_world))
            .with(std::make_shared<const ui::CityScreenFrame>(banner_cities));
        require(ui::project_live_city_banners(unnamed_frame, banner_layout, banner_height,
                    banner_camera, 100, 100).find<ui::WorldViewState>()->city_banners.empty(),
                "unresolved saved city name does not create a fabricated label");
        ui::WorldViewState state;
        state.player_id = 7;
        state.end_turn_label = "Next turn";
        state.end_turn_available = true;
        state.new_turn_text = "Turn 19";
        state.turn_processing_text = "Processing";
        state.plot_help = "Grassland[NEWLINE]River[NEWLINE]Food +2";
        state.path_indicators_visible = true;
        state.path_nodes = {
            {4, 3, 1, ui::WorldViewPoint{240, 210}},
            {5, 3, 1, ui::WorldViewPoint{260, 220}},
            {6, 3, 2, ui::WorldViewPoint{300, 250}},
            {7, 3, 2, ui::WorldViewPoint{320, 260}},
            {8, 3, 3, ui::WorldViewPoint{350, 290}}};
        state.selected_unit = ui::WorldViewUnit{3, "Warrior", "2/2", "8", {{11, "Move", {}, {}, true}}};
        state.enemy_unit = ui::WorldViewUnit{9, "Scout", "2/2", "5", {}};
        state.notifications.push_back({4, "Research needed", "notification.dds", true});
        state.notifications.push_back({10, "Current event", {}, false});
        state.info_corner.push_back({5, "Units", {}, "UnitList", true});
        state.city_banners.push_back({{6, {400, 450}, "Delhi", {}, {}, true}, 2, true});
        state.unit_flags.push_back({7, {500, 500}, "Warrior", {}, {}, true});
        state.resource_icons.push_back({8, {600, 500}, "Wheat",
            "Assets/UI/Art/Icons/Resources512.dds", {0, 0, 64, 64}, true});
        state.yield_icons.push_back({9, {650, 500}, "3",
            "YieldAtlas.dds", {0, 256, 128, 128}, true});
        const auto frame = base.with<ui::WorldViewState>(
            std::make_shared<const ui::WorldViewState>(state));
        const auto disabled = ui::plan_world_view(skin, registry, frame, 1280, 960);
        const auto path_marker = [&](std::string_view number, int x, int y) {
            return std::count_if(disabled.paints.begin(), disabled.paints.end(),
                [&](const ui::WorldViewPaint &paint) {
                    return paint.kind == ui::WorldViewPaintKind::text &&
                        paint.style == "TwCenMT20" && paint.text == number &&
                        paint.destination.x == x - 24 &&
                        paint.destination.y == y - 12 &&
                        paint.destination.width == 48 &&
                        paint.destination.height == 24;
                });
        };
        require(path_marker("1", 260, 220) == 1 &&
                    path_marker("2", 320, 260) == 1 &&
                    path_marker("3", 350, 290) == 1 &&
                    path_marker("1", 240, 210) == 0,
                "path help paints the final world-anchor node of each turn");
        auto hidden_path_state = state;
        hidden_path_state.path_indicators_visible = false;
        const auto hidden_path_frame = base.with<ui::WorldViewState>(
            std::make_shared<const ui::WorldViewState>(hidden_path_state));
        const auto hidden_path_plan = ui::plan_world_view(skin, registry,
            hidden_path_frame, 1280, 960);
        require(std::none_of(hidden_path_plan.paints.begin(), hidden_path_plan.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::text &&
                            paint.style == "TwCenMT20" &&
                            paint.destination.width == 48;
                    }), "DisplayMovementIndicator false hides every path marker");
        auto stale_path_state = state;
        stale_path_state.path_nodes[3].turn = 1;
        const auto stale_path_frame = base.with<ui::WorldViewState>(
            std::make_shared<const ui::WorldViewState>(stale_path_state));
        const auto stale_path_plan = ui::plan_world_view(skin, registry,
            stale_path_frame, 1280, 960);
        require(std::none_of(stale_path_plan.paints.begin(), stale_path_plan.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::text &&
                            paint.style == "TwCenMT20" &&
                            paint.destination.width == 48;
                    }), "out-of-order path owner feed fails closed");
        auto unprojected_path_state = state;
        unprojected_path_state.path_nodes.back().screen.reset();
        const auto unprojected_path_frame = base.with<ui::WorldViewState>(
            std::make_shared<const ui::WorldViewState>(unprojected_path_state));
        const auto unprojected_path_plan = ui::plan_world_view(skin, registry,
            unprojected_path_frame, 1280, 960);
        require(std::none_of(unprojected_path_plan.paints.begin(),
                             unprojected_path_plan.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::text &&
                            paint.style == "TwCenMT20" && paint.text == "3" &&
                            paint.destination.width == 48;
                    }), "path node without current-camera projection has no marker");
        require(std::any_of(disabled.paints.begin(), disabled.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::rectangle &&
                            paint.style == "GridBlack8" &&
                            paint.destination.x == 675 &&
                            paint.destination.y == 634 &&
                            paint.destination.height == 76 &&
                            paint.rgba == 0xffffffc8;
                    }), "plot-help grid keeps authored offset, style and padding");
        require(std::any_of(disabled.paints.begin(), disabled.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::text &&
                            paint.text == "Grassland\nRiver\nFood +2" &&
                            paint.style == "TwCenMT16" &&
                            paint.destination.x == 683 &&
                            paint.destination.y == 642 &&
                            paint.destination.height == 60;
                    }), "plot-help BNW line breaks use authored text font");
        auto empty_help_state = state;
        empty_help_state.plot_help = std::string{};
        const auto empty_help_frame = base.with<ui::WorldViewState>(
            std::make_shared<const ui::WorldViewState>(empty_help_state));
        const auto empty_help_plan = ui::plan_world_view(skin, registry,
            empty_help_frame, 1280, 960);
        require(std::none_of(empty_help_plan.paints.begin(), empty_help_plan.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.style == "GridBlack8";
                    }), "empty live hover does not leave a plot-help panel");
        require(disabled.paints.size() >= 17 && disabled.hits.size() >= 6,
                "all 2D WorldView surfaces project");
        require(std::any_of(disabled.paints.begin(), disabled.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::text &&
                               paint.text == "2";
                    }), "city banner paints observed population");
        require(std::any_of(disabled.paints.begin(), disabled.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::texture &&
                            paint.source_key == "generic-icon.dds" &&
                            paint.destination.width == 80 &&
                            paint.destination.height == 80;
                    }), "notification without type art uses selected generic BNW image");
        require(std::any_of(disabled.paints.begin(), disabled.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::texture &&
                            paint.source_key == "resource-back.dds" &&
                            paint.destination.width == 64;
                    }), "resource icon includes authored background layer");
        require(std::any_of(disabled.paints.begin(), disabled.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::texture &&
                            paint.source_key == "Assets/UI/Art/Icons/Resources512.dds" &&
                            paint.source.x == 0 && paint.source.y == 0 &&
                            paint.source.width == 64 && paint.source.height == 64 &&
                            paint.destination.x == 568 && paint.destination.y == 468 &&
                            paint.destination.width == 64 &&
                            paint.destination.height == 64;
                    }), "resource icon keeps the authored 64-pixel atlas crop");
        require(std::any_of(disabled.paints.begin(), disabled.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::texture &&
                            paint.source_key == "YieldAtlas.dds" &&
                            paint.source.x == 0 && paint.source.y == 256 &&
                            paint.source.width == 128 && paint.source.height == 128 &&
                            paint.destination.x == 586 && paint.destination.y == 436 &&
                            paint.destination.width == 128 &&
                            paint.destination.height == 128;
                    }) &&
                std::none_of(disabled.paints.begin(), disabled.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::text &&
                            (paint.text == "Wheat" ||
                             (paint.text == "3" && paint.destination.x >= 500));
                    }), "plot icon geometry uses authored image size without duplicate labels");
        require(std::any_of(disabled.hits.begin(), disabled.hits.end(),
                    [](const ui::WorldViewHit &hit) {
                        const auto *open = std::get_if<ui::UiOpenScreen>(&hit.command);
                        return open && open->id == "UnitList" && !hit.enabled &&
                            hit.bounds.x == 110 && hit.bounds.y == 75;
                    }), "InfoCorner uses authored entry geometry and disables unpresented UnitList");
        require(std::any_of(disabled.paints.begin(), disabled.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::texture &&
                            paint.source_key == "banner-body.dds" &&
                            paint.destination.x == 345 &&
                            paint.destination.y == 434 &&
                            paint.destination.width == 111 &&
                            paint.destination.height == 32;
                    }) &&
                    std::none_of(disabled.paints.begin(), disabled.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.source_key == "hover-only.dds";
                    }), "normal city banner uses authored body, not hover-only art");
        require(std::any_of(disabled.paints.begin(), disabled.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::text &&
                            paint.text == "Delhi" &&
                            paint.destination.width == 111 &&
                            paint.style == "TwCenMT18";
                    }), "city name is laid over authored banner body");
        require(std::any_of(disabled.paints.begin(), disabled.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::text &&
                               paint.text == "N/A";
                    }), "unavailable selected-unit action is visible without backend");
        const auto *end_turn = disabled.hit(1000, 730);
        require(end_turn && !end_turn->enabled, "missing gameplay backend disables end turn");
        const auto *unconfirmed_city = disabled.hit(400, 450);
        require(unconfirmed_city && !unconfirmed_city->enabled,
                "banner without matching live city cannot open CityView");
        ui::CityScreenFrame city_state;
        city_state.player_id = 7;
        city_state.cities.push_back({7, 6, 32, 15, 2, std::string("Delhi")});
        const auto confirmed_frame = frame.with<ui::CityScreenFrame>(
            std::make_shared<const ui::CityScreenFrame>(city_state));
        const auto confirmed_plan = ui::plan_world_view(skin, registry,
                                                        confirmed_frame, 1280, 960);
        const auto *confirmed_city = confirmed_plan.hit(400, 450);
        require(confirmed_city && confirmed_city->enabled &&
                    std::get<ui::UiGameplayCommand>(confirmed_city->command)
                        .as<ui::WorldViewAction>()->id == 6,
                "matching owned city banner offers typed CityView navigation");
        Backend backend;
        const auto enabled = ui::plan_world_view(skin, registry, frame, 1280, 960, &backend);
        end_turn = enabled.hit(1000, 730);
        require(end_turn && end_turn->enabled, "backend availability enables end turn");
        ui::UiScreenController controller(registry);
        require(controller.dispatch(end_turn->command, frame, &backend).status ==
                    ui::UiCommandStatus::performed && backend.executed == 1,
                "WorldView action reaches typed backend once");
        ui::WorldViewState live_state;
        live_state.player_id = 7;
        live_state.end_turn_unavailable_reason = "Full BNW turn continuation is unavailable";
        const auto live_frame = base.with<ui::WorldViewState>(
            std::make_shared<const ui::WorldViewState>(live_state));
        const auto live_plan = ui::plan_world_view(skin, registry, live_frame,
                                                  1280, 960, &backend);
        const auto *disabled_turn = live_plan.hit(1000, 730);
        require(disabled_turn && !disabled_turn->enabled &&
                    disabled_turn->unavailable_reason ==
                        "Full BNW turn continuation is unavailable" &&
                    std::any_of(live_plan.paints.begin(), live_plan.paints.end(),
                        [](const ui::WorldViewPaint &paint) {
                            return paint.kind == ui::WorldViewPaintKind::text &&
                                   paint.text == "END TURN";
                        }),
                "selected authored label paints disabled live End Turn without backend readiness");
        require(std::none_of(live_plan.paints.begin(), live_plan.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::text &&
                               paint.text == "Warrior";
                    }), "absent selection does not fabricate a unit panel");
        live_state.selected_unit = ui::WorldViewUnit{3, "Warrior", {}, {}, {}};
        const auto empty_action_frame = base.with<ui::WorldViewState>(
            std::make_shared<const ui::WorldViewState>(live_state));
        const auto empty_action_plan = ui::plan_world_view(skin, registry,
            empty_action_frame, 1280, 960, &backend);
        require(std::count_if(empty_action_plan.paints.begin(),
                              empty_action_plan.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::text &&
                               paint.text == "N/A";
                    }) >= 3 &&
                    std::none_of(empty_action_plan.hits.begin(),
                                 empty_action_plan.hits.end(),
                    [](const ui::WorldViewHit &hit) {
                        const auto *command = std::get_if<ui::UiGameplayCommand>(&hit.command);
                        const auto *action = command ? command->as<ui::WorldViewAction>() : nullptr;
                        return action && action->kind == ui::WorldViewActionKind::unit_action;
                    }), "unknown unit stats and actions stay explicitly unavailable");
        require(std::none_of(empty_action_plan.paints.begin(),
                             empty_action_plan.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.style == "UnitPortrait";
                    }), "selected unit without an owner portrait paints no inferred atlas crop");
        auto portrait_state = live_state;
        portrait_state.selected_unit->portrait = ui::WorldViewUnitPortrait{
            "UnitPortraitsAncientMedieval.dds", {768, 0, 256, 256}};
        const auto portrait_frame = base.with<ui::WorldViewState>(
            std::make_shared<const ui::WorldViewState>(portrait_state));
        const auto portrait_plan = ui::plan_world_view(skin, registry,
            portrait_frame, 1280, 960, &backend);
        require(std::count_if(portrait_plan.paints.begin(), portrait_plan.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::texture &&
                            paint.style == "UnitPortrait" &&
                            paint.source_key == "UnitPortraitsAncientMedieval.dds" &&
                            paint.source.x == 768 && paint.source.y == 0 &&
                            paint.source.width == 256 && paint.source.height == 256 &&
                            paint.destination.x == -28 && paint.destination.y == 727 &&
                            paint.destination.width == 256 && paint.destination.height == 256;
                    }) == 1 && portrait_plan.hits.size() == empty_action_plan.hits.size(),
                "explicit selected portrait follows authored XML placement and adds no action");
        require(std::any_of(portrait_plan.paints.begin(),portrait_plan.paints.end(),[](const auto &paint){
            return paint.style=="TwCenMT24:center" && !paint.font_style;
        }),"a plain title fixture must retain its existing Base binding");
        const auto title_skin=[&](std::string declarations,std::string override_attribute={}) {
            fixture.add("Assets/UI/ColorAtlas.xml","<ColorAtlas>"
                "<Color Name='Beige' Color='255.255.200.255'/>"
                "<Color Name='Black' Color='0.0.0.255'/>"+declarations+"</ColorAtlas>");
            fixture.add("Assets/UI/InGame/WorldView/UnitPanel.xml",
                "<Context><Grid ID='PrimaryStretchy' Style='Grid9DetailFive140'/>"
                "<Button ID='UnitNameButton' Size='280,22'><Label ID='UnitName' Font='TwCenMT24' "
                "FontStyle='SoftShadow' ColorSet='Beige_Black_Alpha'"+override_attribute+"/></Button>"
                "<Button ID='CycleLeft' Size='32,32' Texture='UnitPanelArrows.dds' TextureOffset='7,0'/>"
                "<Button ID='CycleRight' Size='32,32' Texture='UnitPanelArrows.dds' TextureOffset='30,0'/>"
                "<Image ID='UnitPortrait' Anchor='L,B' Offset='-28,-23' Size='256,256' "
                "Texture='UnitPortraitsAncientMedieval.dds'/><GridButton ID='UnitActionButton' "
                "Size='50,50' Style='BaseButton'/></Context>");
            return ui::WorldViewSkin::from_resources(*fixture.resources,registry);
        };
        const std::string title_set="<ColorSet Name='Beige_Black_Alpha' Color0='Beige' Color1='Black,200'/>";
        const auto styled_title_skin=title_skin(title_set);
        const auto *styled_title=styled_title_skin.find("UnitPanel","UnitName");
        require(styled_title && styled_title->font_style=="SoftShadow" &&
            styled_title->font_colors[0]==0xffffc8ffu && styled_title->font_colors[1]==0x000000c8u &&
            styled_title->font_colors[2]==0xffffffffu,
            "unique authored title ColorSet retains native absent-Color2 constructor white");
        const auto styled_title_plan=ui::plan_world_view(styled_title_skin,registry,portrait_frame,1280,960);
        require(std::any_of(styled_title_plan.paints.begin(),styled_title_plan.paints.end(),[](const auto &paint){
            return paint.style=="TwCenMT24:center" && paint.font_style &&
                paint.font_style->font=="TwCenMT24" && paint.font_style->style=="SoftShadow" &&
                paint.font_style->colors[2]==0xffffffffu;
        }),"current selected title must issue its exact authored three-layer style");
        const auto adaptive_title=std::find_if(styled_title_plan.paints.begin(),styled_title_plan.paints.end(),
            [](const auto &p){return p.unit_name_font_selection.has_value();});
        require(adaptive_title!=styled_title_plan.paints.end() &&
            adaptive_title->unit_name_font_selection->text==adaptive_title->text &&
            adaptive_title->unit_name_font_selection->selected_unit_id==portrait_state.selected_unit->id &&
            adaptive_title->unit_name_font_selection->player_id==portrait_state.player_id &&
            adaptive_title->unit_name_font_selection->snapshot_frame==portrait_frame.frame() &&
            adaptive_title->unit_name_font_selection->button_width==styled_title_skin.find("UnitPanel","UnitNameButton")->size.x &&
            adaptive_title->unit_name_font_selection->source_xml_key==registry.find("UnitPanel")->source_key+".xml",
            "adaptive command must retain current selected title/frame/actual button/source lineage");
        require(std::count_if(styled_title_plan.paints.begin(),styled_title_plan.paints.end(),
            [](const auto&p){return p.unit_name_font_selection.has_value();})==1,
            "only the current authored UnitName requests font selection");
        for (const auto &declaration: {std::string{},
            std::string("<ColorSet Name='Beige_Black_Alpha' Color0='Beige' Color1='Black,200' Color2=''/>"),
            std::string("<ColorSet Name='Beige_Black_Alpha' Color0='Beige' Color1='Black,200' Color2='UNKNOWN'/>"),
            std::string("<ColorSet Name='Beige_Black_Alpha' Color0='Beige' Color1='Black,200' Color3='1.2.3.4'/>"),
            title_set+title_set}) {
            const auto unresolved=title_skin(declaration);
            require(!unresolved.find("UnitPanel","UnitName")->font_colors[2],
                "missing/ambiguous set or present empty/unknown Color2 must not manufacture white");
        }
        const auto explicit_color=title_skin(
            "<ColorSet Name='Beige_Black_Alpha' Color0='Beige' Color1='Black,200' Color2='0.255.0.255'/>");
        require(explicit_color.find("UnitPanel","UnitName")->font_colors[2]==0x00ff00ffu,
            "declared third color must replace constructor white");
        const auto overridden_color=title_skin(title_set," Color2='255.0.0.255'");
        require(overridden_color.find("UnitPanel","UnitName")->font_colors[2]==0xff0000ffu,
            "explicit title Color2 override must remain distinct from its named set");
        const auto empty_override=title_skin(title_set," Color2=''");
        require(!empty_override.find("UnitPanel","UnitName")->font_colors[2],
            "present empty title Color2 override must stay unresolved");
        const auto fourth_override=title_skin(title_set," Color3='1.2.3.4'");
        require(!fourth_override.find("UnitPanel","UnitName")->font_colors[2],
            "unsupported fourth title color must remain unavailable");
        auto changed_selection=portrait_state;
        changed_selection.selected_unit->id=4;
        changed_selection.selected_unit->name="Scout";
        const auto changed_plan=ui::plan_world_view(styled_title_skin,registry,
            base.with(std::make_shared<const ui::WorldViewState>(changed_selection)),1280,960);
        require(std::any_of(changed_plan.paints.begin(),changed_plan.paints.end(),[](const auto &paint){
            return paint.font_style && paint.font_style->font=="TwCenMT24" && paint.text=="SCOUT";
        }) && std::none_of(changed_plan.paints.begin(),changed_plan.paints.end(),[](const auto &paint){
            return paint.font_style && paint.font_style->font=="TwCenMT24" && paint.text=="WARRIOR";
        }),"a changed current selection must not retain the preceding styled title");
        require(std::any_of(changed_plan.paints.begin(),changed_plan.paints.end(),[](const auto &paint){
            return paint.unit_name_font_selection && paint.unit_name_font_selection->text=="SCOUT" &&
                paint.unit_name_font_selection->selected_unit_id==4;
        }),"a selection change must publish a new text/unit font-selection request");
        changed_selection.selected_unit.reset();
        const auto absent_plan=ui::plan_world_view(styled_title_skin,registry,
            base.with(std::make_shared<const ui::WorldViewState>(changed_selection)),1280,960);
        require(std::none_of(absent_plan.paints.begin(),absent_plan.paints.end(),[](const auto &paint){
            return paint.font_style && paint.font_style->font=="TwCenMT24";
        }),"absent selected unit must issue no styled title");
        changed_selection=portrait_state;
        changed_selection.selected_unit->portrait.reset();
        const auto no_portrait_plan=ui::plan_world_view(styled_title_skin,registry,
            base.with(std::make_shared<const ui::WorldViewState>(changed_selection)),1280,960);
        require(std::none_of(no_portrait_plan.paints.begin(),no_portrait_plan.paints.end(),[](const auto &paint){
            return paint.font_style && paint.font_style->font=="TwCenMT24";
        }),"styled title must preserve the existing selected portrait gate");
        const auto wrong_player = ui::UiFrameSnapshot(9, 8, 19).with<ui::WorldViewState>(
            std::make_shared<const ui::WorldViewState>(state));
        require(ui::plan_world_view(skin, registry, wrong_player, 1280, 960).paints.empty(),
                "stale active player frame is rejected");
        if (argc == 2) {
            const fs::path root(argv[1]);
            auto base = std::make_shared<assets::LooseResourceIndex>(root);
            auto expansion = std::make_shared<assets::LooseResourceIndex>(root);
            auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
            base->scan({root / "Assets/UI/InGame", {}, false});
            base->add_file(root / "Assets/UI/ColorAtlas.xml");
            expansion->scan({root / "Assets/DLC/Expansion/UI/InGame", {}, false});
            bnw->scan({root / "Assets/DLC/Expansion2/UI/InGame", {}, false});
            assets::ResourceMountTable active(assets::resource_lookup(base));
            active.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {}, {}});
            active.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {}, {}});
            const auto actual_registry = ui::UiContextRegistry::from_resources(active);
            const auto actual_skin = ui::WorldViewSkin::from_resources(active, actual_registry);
            require(actual_skin.source_keys().size() >= 20, "active BNW WorldView coverage");
            const auto *actual_title=actual_skin.find("UnitPanel","UnitName");
            require(actual_title && actual_title->font=="TwCenMT24" && actual_title->font_style=="SoftShadow" &&
                actual_title->color_set=="Beige_Black_Alpha" && actual_title->font_colors[0]==0xffffc8ffu &&
                actual_title->font_colors[1]==0x000000c8u && actual_title->font_colors[2]==0xffffffffu,
                "actual selected BNW UnitName must preserve three authored source colors");
            const auto *actual_resource_icon = actual_skin.find("ResourceIconManager",
                                                              "ResourceIcon");
            const auto *actual_yield_icon = actual_skin.find("YieldIconManager", "Image");
            require(actual_resource_icon && actual_yield_icon &&
                        actual_resource_icon->size.x == 64 &&
                        actual_resource_icon->size.y == 64 &&
                        actual_yield_icon->size.x == 128 &&
                        actual_yield_icon->size.y == 128,
                    "active BNW resource and yield image sizes");
            const auto *actual_path_anchor = actual_skin.find("PathHelpManager", "Anchor");
            const auto *actual_path_label = actual_skin.find("PathHelpManager", "TurnLabel");
            require(actual_path_anchor && actual_path_anchor->element == "WorldAnchor" &&
                        actual_path_label && actual_path_label->element == "Label" &&
                        actual_path_label->font == "TwCenMT20",
                    "active BNW path help uses a world anchor and authored font");
            const auto *actual_help_panel = actual_skin.find("PlotHelpText", "TextBox");
            const auto *actual_help_text = actual_skin.find("PlotHelpText", "Text");
            require(actual_help_panel && actual_help_text &&
                        actual_help_panel->style == "GridBlack8" &&
                        actual_help_panel->padding.x == 8 &&
                        actual_help_panel->padding.y == 8 &&
                        actual_help_text->font == "TwCenMT16",
                    "active BNW PlotHelpText authored grid, padding and font");
            require(actual_skin.find("ActionInfoPanel", "EndTurnButton") &&
                        actual_skin.find("UnitPanel", "PrimaryStretchy") &&
                        actual_skin.find("UnitPanel", "UnitPortrait") &&
                        actual_skin.find("UnitPanel", "UnitActionButton") &&
                        actual_skin.find("UnitFlagManager", "FlagBaseOutline") &&
                        actual_skin.find("CityBannerManager", "BannerButton") &&
                        actual_skin.find("CityBannerManager", "CityBannerBackground") &&
                        actual_skin.find("CityBannerManager", "CityName") &&
                        actual_skin.find("CityBannerManager", "CityPopulation"),
                    "active BNW principal widget identities");
            const auto actual_portrait_plan = ui::plan_world_view(actual_skin,
                actual_registry, portrait_frame, 1274, 925, &backend);
            require(std::count_if(actual_portrait_plan.paints.begin(),
                                  actual_portrait_plan.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::texture &&
                            paint.style == "UnitPortrait" &&
                            paint.source.x == 768 && paint.source.y == 0 &&
                            paint.source.width == 256 && paint.source.height == 256 &&
                            paint.destination.x == -28 && paint.destination.y == 692 &&
                            paint.destination.width == 256 && paint.destination.height == 256;
                    }) == 1, "active BNW UnitPortrait uses authored G0 viewport geometry");
            const auto actual_plan = ui::plan_world_view(actual_skin, actual_registry, frame,
                                                         1920, 1080, &backend);
            require(actual_plan.paints.size() >= 17 && actual_plan.hits.size() >= 6,
                    "active BNW frame projects every populated U4 surface");
            require(std::any_of(actual_plan.paints.begin(),actual_plan.paints.end(),[](const auto &paint){
                        return paint.font_style&&paint.font_style->font=="TwCenMT18"&&paint.font_style->style=="Shadow";
                    })&&std::any_of(actual_plan.paints.begin(),actual_plan.paints.end(),[](const auto &paint){
                        return paint.font_style&&paint.font_style->font=="TwCenMT20"&&paint.font_style->style=="Stroke"&&
                            paint.font_style->colors[0]==0xffffc8ff&&paint.font_style->colors[1]==0x000000ff;
                    }),"selected CityBanner XML font sizes, authored styles and explicit population colors reach draw commands");
            auto colored_state=state;
            colored_state.city_banners.front().appearance=appearance;
            const auto colored_plan=ui::plan_world_view(actual_skin,actual_registry,
                frame.with(std::make_shared<const ui::WorldViewState>(colored_state)),1280,960);
            require(std::count_if(colored_plan.paints.begin(),colored_plan.paints.end(),[&](const auto &paint){
                return paint.kind==ui::WorldViewPaintKind::texture&&paint.style=="CityBanner"&&
                    paint.tint==appearance.background;
            })==3,"selected authored banner body/wings did not retain source float tint");
            require(std::any_of(colored_plan.paints.begin(),colored_plan.paints.end(),[&](const auto &paint){
                return paint.text=="Delhi"&&paint.tint==appearance.foreground&&paint.font_style&&
                    paint.font_style->float_colors==std::array<ui::UiFloatColor,2>{appearance.foreground,appearance.shadow};
            }),"selected authored city name lost primary foreground or float shadow alpha");
            require(std::any_of(colored_plan.paints.begin(),colored_plan.paints.end(),[](const auto &paint){
                return paint.font_style&&paint.font_style->style=="Stroke"&&!paint.font_style->float_colors&&
                    paint.font_style->colors[0]==0xffffc8ff;
            }),"player color incorrectly replaced authored population label colors");
            colored_state.city_banners.front().appearance->frame_number=7;
            const auto stale_plan=ui::plan_world_view(actual_skin,actual_registry,
                frame.with(std::make_shared<const ui::WorldViewState>(colored_state)),1280,960);
            require(std::none_of(stale_plan.paints.begin(),stale_plan.paints.end(),[](const auto &paint){
                return paint.style=="CityBanner"&&paint.tint.has_value();
            }),"stale city appearance was consumed by painter");
            require(std::any_of(actual_plan.paints.begin(), actual_plan.paints.end(),
                    [](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::texture &&
                            paint.style == "YieldIcon" &&
                            paint.source.x == 0 && paint.source.y == 256 &&
                            paint.source.width == 128 && paint.source.height == 128 &&
                            paint.destination.x == 586 && paint.destination.y == 436 &&
                            paint.destination.width == 128 &&
                            paint.destination.height == 128;
                    }), "active BNW yield artwork uses authored nominal image geometry");
            const auto *actual_banner = actual_skin.find("CityBannerManager",
                                                          "CityBannerBackground");
            const auto *hover_banner = actual_skin.find("CityBannerManager",
                                                         "CityBannerBackgroundIcon");
            require(actual_banner && hover_banner &&
                std::any_of(actual_plan.paints.begin(), actual_plan.paints.end(),
                    [&](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::texture &&
                            paint.source_key == actual_banner->texture &&
                            paint.destination.width == actual_banner->size.x &&
                            paint.destination.height == actual_banner->size.y;
                    }) &&
                std::none_of(actual_plan.paints.begin(), actual_plan.paints.end(),
                    [&](const ui::WorldViewPaint &paint) {
                        return paint.source_key == hover_banner->texture &&
                            hover_banner->texture != actual_banner->texture;
                    }), "selected BNW city uses its normal authored background");
            std::cout << "PASS active BNW WorldView XML sources: "
                      << actual_skin.source_keys().size() << '\n';
        }
        std::cout << "PASS WorldView XML, live frame, 2D plan and gameplay routing\n";
        return EXIT_SUCCESS;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
