#include "civ5/content/manifest.hpp"
#include "civ5/ui/top_panel.hpp"
#include "civ5/ui/top_panel_saved_state.hpp"
#include "civ5/ui/top_panel_live.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace civ5;
namespace {
int checks = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action) {
    bool failed = false;
    try {
        action();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, "Expected TopPanel input to be rejected");
}

const char *xml() {
    return R"xml(<?xml version="1.0" encoding="utf-8"?>
<Context Font="TwCenMT16">
  <Image Texture="assets\UI\Art\WorldView\TopPanelBar.dds" Anchor="C,T" Size="Full.32"/>
  <Stack ID="TopPanelInfoStack" Offset="0,0" Padding="0" StackGrowth="Right"><Label ID="Science"/><Label ID="Gold"/></Stack>
  <Image Texture="assets\UI\Art\WorldView\TopPanelBar.dds" Anchor="R,T" Size="295.32"/>
  <Stack Anchor="R,T"><Button ID="Menu"/></Stack>
  <ToolTipType Name="TooltipTypeTopPanel"><Grid ID="TopPanelMouseover" Style="GridBlack8"><Label ID="TooltipLabel" Offset="12,12" LeadingOffset="-5" WrapWidth="512"/></Grid></ToolTipType>
</Context>)xml";
}

civ5::ui::TopPanelTextLocalizer localizer() {
    return {.text = [](std::string_view key, const std::vector<std::string> &args) {
        if (key == "TXT_KEY_RETURN")
            return std::string("Return");
        if (key == "TXT_KEY_MENU")
            return std::string("Menu");
        if (key == "TXT_KEY_GOLDEN_AGE_ANNOUNCE")
            return std::string("Golden age");
        if (key == "TXT_KEY_IRON")
            return std::string("Iron");
        if (key == "TXT_KEY_RESOURCE_IRON")
            return std::string("Iron");
        std::string result(key);
        for (const auto &arg : args)
            result += "(" + arg + ")";
        return result;
    }};
}

void layout_contract() {
    const auto root = content::read_xml_metadata(xml());
    const auto layout = ui::TopPanelLayout::from_xml(root);
    check(layout.root.name == "Context", "TopPanel root was not retained");
    check(layout.root.attribute("Font") == "TwCenMT16", "TopPanel root attribute was not retained");
    check(layout.center_bar.attribute("Size") == "Full.32", "Centre bar size was not retained");
    check(layout.right_bar.attribute("Size") == "295.32", "Right bar size was not retained");
    check(layout.info_stack.children.size() == 2 && layout.info_stack.children[1].attribute("ID") == "Gold",
          "Info stack order was not retained");
    check(layout.right_stack.children.size() == 1 && layout.right_stack.children.front().name == "Button",
          "Right stack children were not retained");
    check(layout.tooltip_grid.attribute("Style") == "GridBlack8", "Tooltip grid was not retained");
    check(layout.tooltip_label.attribute("ID") == "TooltipLabel", "TopPanel tooltip label was not retained");
    const auto info_stack = ui::plan_top_panel_stack(layout.info_stack);
    check(info_stack.offset_x == 0 && info_stack.offset_y == 0 && info_stack.padding == 0 &&
              info_stack.growth == ui::TopPanelStackGrowth::right,
          "Synthetic TopPanel info stack placement changed");
    rejects([&] { ui::plan_top_panel_stack(layout.right_stack); });
    rejects([&] { ui::TopPanelLayout::from_xml(content::read_xml_metadata("<Wrong/>")); });
    rejects([&] {
        ui::TopPanelLayout::from_xml(content::read_xml_metadata(
            R"xml(<Context><Image Texture="assets\UI\Art\WorldView\TopPanelBar.dds" Anchor="C,T"/><Image Texture="assets\UI\Art\WorldView\TopPanelBar.dds" Anchor="C,T"/><Stack ID="TopPanelInfoStack"/><Image Texture="assets\UI\Art\WorldView\TopPanelBar.dds" Anchor="R,T"/><Stack Anchor="R,T"/><ToolTipType Name="TooltipTypeTopPanel"><Grid ID="TopPanelMouseover"/></ToolTipType></Context>)xml"));
    });
}

void selected_asset_contract() {
    namespace fs = std::filesystem;
    auto root = fs::current_path();
    for (unsigned i = 0; i != 8 && !fs::exists(root / "civ5-assets/Resource/DX9/UITextures.fpk"); ++i)
        root = root.parent_path();
    check(fs::exists(root / "civ5-assets/Resource/DX9/UITextures.fpk"), "Civ V UI archive fixture is unavailable");
    const auto opened = assets::FpkSource::open(root / "civ5-assets/Resource/DX9/UITextures.fpk",
                                                "Resource/DX9/UITextures.fpk");
    check(std::holds_alternative<std::shared_ptr<assets::FpkSource>>(opened), "UI archive could not be opened");
    const auto source = std::get<std::shared_ptr<assets::FpkSource>>(opened);
    const auto layout = ui::TopPanelLayout::from_xml(content::read_xml_metadata(xml()));
    const auto binding = ui::bind_top_panel_bar(layout, source);
    check(binding.entry_name == "toppanelbar.dds" && binding.width == 22 && binding.height == 2,
          "TopPanelBar DDS metadata was not retained");
    check(binding.mip_count == 1 && binding.provenance.kind == assets::SourceKind::fpk,
          "TopPanelBar source provenance was not retained");
    const auto plan = ui::plan_top_panel(layout, 1280, 720);
    check(plan.center_bar.x == 0 && plan.center_bar.y == 0 && plan.center_bar.width == 1280 &&
              plan.center_bar.height == 32,
          "TopPanel centre bar render plan changed");
    check(plan.right_bar.x == 985 && plan.right_bar.y == 0 && plan.right_bar.width == 295 &&
              plan.right_bar.height == 32,
          "TopPanel right bar render plan changed");
    rejects([&] { ui::plan_top_panel(layout, 1280, 0); });

    const auto authored_layout = ui::read_top_panel(
        assets::path_utf8(root / "civ5-assets/Assets/UI/InGame/TopPanel.xml"));
    ui::TopPanelControls controls(authored_layout, localizer());
    const auto initial = controls.apply(std::nullopt);
    check(initial.show_info_stack && initial.show_unit_supply && initial.menu_button &&
              initial.menu_button->text == "TXT_KEY_MENU_CAPS" &&
              initial.science_text == "000 Turn" && initial.current_turn_text == "Turn: 1" &&
              initial.current_date_text == "Date: 2100 AD",
          "TopPanel missing-player initialization lost authored widget defaults");
    ui::TopPanelState frame;
    frame.game_turn = 8;
    frame.turn_date = "3760 BC";
    frame.player.emplace();
    const auto empty = controls.apply(ui::present_top_panel(frame, localizer()));
    check(!empty.show_info_stack && !empty.show_unit_supply && empty.menu_button &&
              empty.menu_button->text == initial.menu_button->text &&
              empty.science_text == initial.science_text &&
              empty.current_turn_text == "TXT_KEY_TP_TURN_COUNTER(8)",
          "TopPanel first cityless update changed retained widgets");
    frame.player->city_count = 1;
    frame.player->unit_production_maintenance_mod = 25;
    frame.head_selected_city = true;
    frame.city_screen_up = true;
    const auto city = controls.apply(ui::present_top_panel(frame, localizer()));
    check(city.show_info_stack && city.show_unit_supply && city.unit_supply_tooltip &&
              city.menu_button && city.menu_button->text == "RETURN",
          "TopPanel city-screen update did not replace authored defaults");
    frame.player->city_count = 0;
    frame.game_turn = 9;
    const auto cityless = controls.apply(ui::present_top_panel(frame, localizer()));
    check(!cityless.show_info_stack && cityless.show_unit_supply && cityless.unit_supply_tooltip &&
              cityless.menu_button->text == "RETURN" && cityless.science_text == city.science_text &&
              cityless.current_turn_text == "TXT_KEY_TP_TURN_COUNTER(9)",
          "TopPanel cityless update lost retained text or skipped supply update");
    frame.player.reset();
    frame.game_turn = 10;
    frame.turn_date = "3720 BC";
    const auto inactive = controls.apply(ui::present_top_panel(frame, localizer()));
    check(inactive.current_turn_text == cityless.current_turn_text &&
              inactive.current_date_text == cityless.current_date_text && !inactive.show_info_stack &&
              inactive.menu_button->text == cityless.menu_button->text,
          "TopPanel missing-player update changed existing widgets");
    frame.player.emplace().city_count = 1;
    frame.city_screen_up = false;
    const auto restored = controls.apply(ui::present_top_panel(frame, localizer()));
    check(restored.show_info_stack && !restored.show_unit_supply && !restored.unit_supply_tooltip &&
              restored.menu_button->text == "MENU" && restored.current_date_text == "3720 BC",
          "TopPanel restored player did not clear supply indicator or refresh menu");
    check(authored_layout.tooltip_label.attribute("Offset") == "12,12" &&
              authored_layout.tooltip_label.attribute("LeadingOffset") == "-5" &&
              authored_layout.tooltip_label.attribute("WrapWidth") == "512",
          "TopPanel authored tooltip label placement changed");
    const auto authored_info_stack = ui::plan_top_panel_stack(authored_layout.info_stack);
    const auto authored_right_stack = ui::plan_top_panel_stack(authored_layout.right_stack);
    check(authored_info_stack.offset_x == 10 && authored_info_stack.offset_y == 6 &&
              authored_info_stack.padding == 10 && authored_info_stack.growth == ui::TopPanelStackGrowth::right &&
              authored_right_stack.offset_x == 10 && authored_right_stack.offset_y == 4 &&
              authored_right_stack.padding == 10 && authored_right_stack.growth == ui::TopPanelStackGrowth::left,
          "TopPanel authored stack placement changed");
    const auto authored_divider = std::find_if(
        authored_layout.right_stack.children.begin(), authored_layout.right_stack.children.end(),
        [](const ui::TopPanelElement &element) {
            return element.name == "Image" &&
                   element.attribute("Texture") == "assets\\UI\\Art\\WorldView\\Divider.dds";
        });
    check(authored_divider != authored_layout.right_stack.children.end(),
          "TopPanel authored divider is missing");
    const auto divider_size = ui::top_panel_element_size(*authored_divider);
    const auto divider_offset = ui::top_panel_element_texture_offset(*authored_divider);
    check(divider_size.width == 5 && divider_size.height == 16 && divider_offset.x == 5 && divider_offset.y == 0,
          "TopPanel authored divider geometry changed");
    const auto authored_supply = std::find_if(
        authored_layout.info_stack.children.begin(), authored_layout.info_stack.children.end(),
        [](const ui::TopPanelElement &element) { return element.attribute("ID") == "UnitSupplyString"; });
    check(authored_supply != authored_layout.info_stack.children.end(), "TopPanel authored supply is missing");
    const auto supply_size = ui::top_panel_element_size(*authored_supply);
    const auto supply_offset = ui::top_panel_element_offset(*authored_supply);
    check(supply_size.width == 20 && supply_size.height == 20 && supply_offset.x == 0 && supply_offset.y == -3,
          "TopPanel authored supply geometry changed");
    check(authored_layout.info_stack.children.size() == 7 &&
              authored_layout.info_stack.children.back().attribute("ID") == "UnitSupplyString" &&
              authored_layout.info_stack.children.back().attribute("Texture") ==
                  "assets\\UI\\Art\\WorldView\\OverSupplyLimit.dds" &&
              authored_layout.info_stack.children.back().attribute("Size") == "20.20",
          "TopPanel unit-supply indicator source contract changed");
    auto base_ui = std::make_shared<assets::LooseSource>(root / "civ5-assets");
    assets::ResourceMountTable ui_resources(assets::resource_lookup(base_ui), {}, assets::resource_keys(base_ui));
    const auto discovered_localization = ui_resources.keys("Assets/Gameplay/XML/NewText/EN_US");
    check(discovered_localization.size() > 10 &&
              std::find(discovered_localization.begin(), discovered_localization.end(),
                        "Assets/Gameplay/XML/NewText/EN_US/CIV5GameTextInfos_InGameScreens.xml") !=
                  discovered_localization.end(),
          "Active VFS did not enumerate the authored localization key set");
    const auto vfs_layout = ui::read_top_panel(ui_resources);
    const auto vfs_style = ui::read_top_panel_grid9_style(ui_resources, "GridBlack8");
    const auto vfs_font = ui::read_glyphgen(ui_resources, "Assets/UI/Fonts/Tw Cent MT/TwCenMT16.ggxml");
    check(ui::plan_top_panel_stack(vfs_layout.info_stack).padding == 10 && vfs_style.name == "GridBlack8" &&
              !vfs_font.glyphs.empty(),
          "TopPanel authored XML or GlyphGen font did not load through the active VFS");
    const auto override_root = fs::temp_directory_path() /
                               ("civis-top-panel-vfs-" +
                                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(override_root);
    std::ifstream source_xml(root / "civ5-assets/Assets/UI/InGame/TopPanel.xml", std::ios::binary);
    std::ostringstream source_text;
    source_text << source_xml.rdbuf();
    const auto padding_marker = source_text.str().find("Padding=\"10\"");
    check(padding_marker != std::string::npos, "TopPanel override fixture lost its padding marker");
    auto overridden_xml = source_text.str();
    overridden_xml.replace(padding_marker, std::string("Padding=\"10\"").size(), "Padding=\"11\"");
    {
        std::ofstream override_file(override_root / "TopPanel.xml", std::ios::binary);
        override_file << overridden_xml;
    }
    const auto custom_localization = override_root /
                                     "Assets/Gameplay/XML/NewText/EN_US/CIV5GameTextInfos_Custom.xml";
    fs::create_directories(custom_localization.parent_path());
    {
        std::ofstream custom_file(custom_localization, std::ios::binary);
        custom_file << "<GameData><Language_en_US><Row Tag=\"TXT_KEY_CUSTOM_VFS\"><Text>Custom VFS</Text>"
                       "</Row></Language_en_US></GameData>";
    }
    auto override_source = std::make_shared<assets::LooseResourceIndex>(override_root);
    override_source->add_file(override_root / "TopPanel.xml");
    override_source->add_file(custom_localization);
    ui_resources.mount({77,
                        1,
                        0,
                        assets::resource_lookup(override_source),
                        true,
                        true,
                        assets::resource_lookup_all(override_source),
                        assets::resource_keys(override_source)});
    check(ui::plan_top_panel_stack(ui::read_top_panel(ui_resources).info_stack).padding == 11,
          "TopPanel VFS mount order did not select the active override");
    const auto active_localization_keys = ui_resources.keys("Assets/Gameplay/XML/NewText/EN_US");
    check(std::find(active_localization_keys.begin(), active_localization_keys.end(),
                    "Assets/Gameplay/XML/NewText/EN_US/CIV5GameTextInfos_Custom.xml") !=
              active_localization_keys.end(),
          "Active VFS did not enumerate a newly mounted localization filename");
    const auto active_localization =
        ui::TopPanelLocalizationCatalog::read_resources(ui_resources, active_localization_keys);
    check(active_localization.rows.at("TXT_KEY_CUSTOM_VFS") == "Custom VFS",
          "Active VFS localization reader did not load a newly mounted filename");
    fs::remove_all(override_root);
    std::vector<std::string> localization_keys;
    const auto localization_root = root / "civ5-assets/Assets/Gameplay/XML/NewText/EN_US";
    for (const auto &entry : fs::recursive_directory_iterator(localization_root))
        if (entry.is_regular_file() && entry.path().extension() == ".xml")
            localization_keys.push_back(assets::path_utf8(entry.path().lexically_relative(root / "civ5-assets")));
    std::sort(localization_keys.begin(), localization_keys.end());
    const auto vfs_localization = ui::TopPanelLocalizationCatalog::read_resources(ui_resources, localization_keys);
    const auto localization = ui::TopPanelLocalizationCatalog::read_directory(
        assets::path_utf8(root / "civ5-assets/Assets/Gameplay/XML/NewText/EN_US"));
    check(vfs_localization.rows == localization.rows && localization.rows.size() > 500 &&
              localization.rows.at("TXT_KEY_MENU_CAPS") == "MENU" &&
              localization.rows.at("TXT_KEY_HELP_CAPS") == "HELP",
          "TopPanel English localization catalog is incomplete or bypassed the active VFS");
    check(localization.format("TXT_KEY_TOP_PANEL_GOLD", {"100", "2"}) ==
              "[ICON_GOLD][COLOR:255:235:0:255]100 (+2)[/COLOR]",
          "TopPanel localization number formatting changed");
    check(localization.format("TXT_KEY_TP_TURN_COUNTER", {"1"}) == "Turn: 1",
          "TopPanel localization turn formatting changed");
    check(localization.format("TXT_KEY_TIME_BC", {"4000"}) == "4000 BC",
          "TopPanel English turn-zero date formatting changed");
    const auto russian_keys = ui_resources.keys("Assets/Gameplay/XML/NewText/RU_RU");
    const auto russian_localization =
        ui::TopPanelLocalizationCatalog::read_resources(ui_resources, russian_keys, "Language_RU_RU");
    check(russian_localization.rows.size() > 500 &&
              russian_localization.rows.at("TXT_KEY_MENU_CAPS") == "\xd0\x9c\xd0\xb5\xd0\xbd\xd1\x8e" &&
              russian_localization.rows.at("TXT_KEY_HELP_CAPS") == "\xd0\x9f\xd0\xbe\xd0\xbc\xd0\xbe\xd1\x89\xd1\x8c" &&
              russian_localization.format("TXT_KEY_TP_TURN_COUNTER", {"1"}) ==
                  "\xd0\xa5\xd0\xbe\xd0\xb4: 1" &&
              russian_localization.format("TXT_KEY_TIME_BC", {"4000"}) ==
                  "4000 \xd0\xb3. \xd0\xb4\xd0\xbe \xd0\xbd.\xd1\x8d.",
          "TopPanel Russian localization is incomplete or bypassed the active VFS");
    rejects([&] { localization.format("TXT_KEY_TOP_PANEL_GOLD", {"100"}); });
    ui::TopPanelState localized_state;
    localized_state.player = ui::TopPanelPlayerState{.city_count = 2,
                                                       .science_per_turn = 5,
                                                       .gold = 100,
                                                       .gold_per_turn = 2,
                                                       .excess_happiness = 3,
                                                       .golden_age_progress = 10,
                                                       .golden_age_threshold = 20,
                                                       .culture = 30,
                                                       .next_policy_cost = 100,
                                                       .strategic_resources = {{"[ICON_RES_IRON]", 2, 0, true,
                                                                                 "TXT_KEY_RESOURCE_IRON", 2}}};
    const auto localized_presentation = ui::present_top_panel(
        localized_state, {.text = [&](std::string_view key, const std::vector<std::string> &args) {
                              return localization.format(key, args);
                          }});
    check(localized_presentation && localized_presentation->science_tooltip &&
              localized_presentation->gold_tooltip && localized_presentation->science_tooltip->find("[ICON_RESEARCH]") !=
                  std::string::npos && localized_presentation->gold_tooltip->find("[ICON_GOLD]") != std::string::npos,
          "TopPanel localized tooltip bodies are incomplete");
    const auto russian_presentation = ui::present_top_panel(
        localized_state, {.text = [&](std::string_view key, const std::vector<std::string> &args) {
                              return russian_localization.format(key, args);
                          }});
    check(russian_presentation && russian_presentation->show_info_stack &&
              russian_presentation->menu_button &&
              russian_presentation->menu_button->text == "\xd0\x9c\xd0\x95\xd0\x9d\xd0\xae" &&
              russian_presentation->current_turn_text == "\xd0\xa5\xd0\xbe\xd0\xb4: 0" &&
              russian_presentation->science_tooltip && russian_presentation->gold_tooltip &&
              russian_presentation->happiness_tooltip && russian_presentation->golden_age_tooltip &&
              russian_presentation->culture_tooltip && russian_presentation->resource_tooltip,
          "Russian TopPanel state or tooltip construction is incomplete");
    auto russian_city_screen = localized_state;
    russian_city_screen.head_selected_city = true;
    russian_city_screen.city_screen_up = true;
    russian_city_screen.player->golden_age_turns = 2;
    const auto russian_city_presentation = ui::present_top_panel(
        russian_city_screen, {.text = [&](std::string_view key, const std::vector<std::string> &args) {
                                  return russian_localization.format(key, args);
                              }});
    check(russian_city_presentation && russian_city_presentation->menu_button &&
              russian_city_presentation->menu_button->text ==
                  "\xd0\x9d\xd0\x90\xd0\x97\xd0\x90\xd0\x94" &&
              russian_city_presentation->golden_age_text &&
              russian_city_presentation->golden_age_text->find(
                  "\xd0\x97\xd0\x9e\xd0\x9b. \xd0\x92\xd0\x95\xd0\x9a! (2)") != std::string::npos,
          "Russian TopPanel did not apply Locale.ToUpper in city and Golden Age branches");
    const auto grid_style = ui::read_top_panel_grid9_style(
        assets::path_utf8(root / "civ5-assets/Assets/UI/Styles.xml"), "GridBlack8");
    check(grid_style.texture == "Assets/UI/Art/Controls/9 Grids/Grid9Black8.dds" &&
              grid_style.source[4].x == 8 && grid_style.source[4].y == 8 &&
              grid_style.source[4].width == 28 && grid_style.source[4].height == 28,
          "TopPanel GridBlack8 source contract changed");
    const auto grid_asset = source->find(assets::AssetKey("Grid9Black8.dds"));
    check(std::holds_alternative<assets::AssetHandle>(grid_asset), "Grid9Black8 DDS is unavailable");
    const auto grid_data = assets::DdsData::parse(std::get<assets::AssetHandle>(grid_asset).read_bytes());
    check(grid_data.width() == 64 && grid_data.height() == 64, "Grid9Black8 DDS dimensions changed");
    const auto supply_asset = source->find(assets::AssetKey("OverSupplyLimit.dds"));
    check(std::holds_alternative<assets::AssetHandle>(supply_asset),
          "OverSupplyLimit.dds is unavailable or ambiguous");
    const auto supply_data = assets::DdsData::parse(std::get<assets::AssetHandle>(supply_asset).read_bytes());
    check(supply_data.width() == 22 && supply_data.height() == 16,
          "OverSupplyLimit.dds dimensions changed from the authored 22x16 source");
    const auto tooltip = ui::plan_top_panel_grid9(authored_layout, grid_style, {10, 20, 530, 90});
    check(tooltip.content.x == 18 && tooltip.content.y == 28 && tooltip.content.width == 514 &&
              tooltip.content.height == 74,
          "TopPanel tooltip content padding changed");
    check(tooltip.destination[4].x == 18 && tooltip.destination[4].y == 28 &&
              tooltip.destination[4].width == 514 && tooltip.destination[4].height == 74,
          "TopPanel Grid9 centre destination changed");
    rejects([&] { ui::plan_top_panel_grid9(authored_layout, grid_style, {0, 0, 15, 15}); });

    const auto top_panel_font = ui::read_glyphgen(
        root / "civ5-assets/Assets/UI/Fonts/Tw Cent MT/TwCenMT16.ggxml");
    const auto glyph_geometry = ui::glyphgen_geometry(top_panel_font);
    const auto tooltip_text = std::string("This is a deliberately long TopPanel tooltip that must wrap across several authored lines so its frame can auto-size. ") +
                              "The same source continues with more explanation and exact values. "+
                              "The same source continues with more explanation and exact values. "+
                              "The same source continues with more explanation and exact values. "+
                              "The same source continues with more explanation and exact values. "+
                              "The same source continues with more explanation and exact values. "+
                              "The same source continues with more explanation and exact values. "+
                              "The same source continues with more explanation and exact values.";
    const auto tooltip_plan = ui::plan_top_panel_tooltip(
        authored_layout, grid_style, glyph_geometry, glyph_geometry, tooltip_text,
        [](std::string_view) -> std::optional<std::uint32_t> { return std::nullopt; }, {640, 38}, 1280, 960);
    check(tooltip_plan.lines.size() > 1 && tooltip_plan.grid.frame.width <= 1280 &&
              tooltip_plan.grid.frame.height > 90 && tooltip_plan.text_origin.x >= 0 &&
              tooltip_plan.text_origin.y >= 0 && tooltip_plan.line_step == 11,
          "TopPanel tooltip did not wrap and auto-size from authored label metrics");
    check(tooltip_plan.grid.frame.x + tooltip_plan.grid.frame.width <= 1280 &&
              tooltip_plan.grid.frame.y + tooltip_plan.grid.frame.height <= 960,
          "TopPanel tooltip auto-size escaped the viewport");
    bool ink_inside_content = true;
    for (std::size_t line = 0; line < tooltip_plan.lines.size(); ++line)
        for (const auto &glyph : tooltip_plan.lines[line].glyphs) {
            const auto x = tooltip_plan.text_origin.x + glyph.left_from_line_origin;
            const auto y = tooltip_plan.text_origin.y + static_cast<int>(line) * tooltip_plan.line_step -
                           glyph.top_from_baseline;
            const auto &content = tooltip_plan.grid.content;
            ink_inside_content &= x >= static_cast<int>(content.x) && y >= static_cast<int>(content.y) &&
                                  x + glyph.width <= static_cast<int>(content.x + content.width) &&
                                  y + glyph.height <= static_cast<int>(content.y + content.height);
        }
    check(ink_inside_content, "TopPanel tooltip measurement clipped glyph ascenders or descenders");
    rejects([&] {
        ui::plan_top_panel_tooltip(authored_layout, grid_style, glyph_geometry, glyph_geometry,
                                   "unsupported [ICON_DOES_NOT_EXIST] control", [](std::string_view) {
                                       return std::optional<std::uint32_t>{};
                                   },
                                   {640, 38}, 1280, 960);
    });
}

void expansion2_panel_contract() {
    namespace fs = std::filesystem;
    auto root = fs::current_path();
    for (unsigned i = 0; i != 8 &&
         !fs::exists(root / "civ5-assets/Assets/DLC/Expansion2/UI/InGame/TopPanel.xml"); ++i)
        root = root.parent_path();
    const auto layout = ui::read_top_panel(assets::path_utf8(
        root / "civ5-assets/Assets/DLC/Expansion2/UI/InGame/TopPanel.xml"));
    const std::vector<std::string> expected{
        "SciencePerTurn", "GoldPerTurn", "InternationalTradeRoutes", "HappinessString",
        "GoldenAgeString", "CultureString", "TourismString", "FaithString",
        "ResourceString", "UnitSupplyString"};
    check(layout.info_stack.children.size() == expected.size(), "BNW TopPanel control count changed");
    for (std::size_t i = 0; i < expected.size(); ++i)
        check(layout.info_stack.children[i].attribute("ID") == expected[i],
              "BNW TopPanel authored order changed");
    ui::TopPanelControls controls(layout, localizer());
    const auto &initial = controls.presentation();
    check(initial.international_trade_routes_text == "000,000 Turn: Total 000,000" &&
              initial.tourism_text == "Tourism!" && initial.faith_text == "FAITH!",
          "BNW TopPanel authored defaults changed");
    ui::TopPanelState state;
    state.expansion2_ui = true;
    state.player.emplace();
    state.player->city_count = 1;
    state.player->international_trade_routes_used = 2;
    state.player->international_trade_routes_available = 4;
    state.player->tourism = 7;
    state.player->faith = 19;
    state.player->faith_per_turn = 3;
    const auto shown = controls.apply(ui::present_top_panel(state, localizer()));
    check(shown.international_trade_routes_text ==
              "TXT_KEY_TOP_PANEL_INTERNATIONAL_TRADE_ROUTES(2)(4)" &&
              shown.international_trade_routes_tooltip ==
                  "TXT_KEY_TOP_PANEL_INTERNATIONAL_TRADE_ROUTES_TT(2)(4)" &&
              shown.tourism_text == "[ICON_TOURISM] +7" &&
              shown.faith_text == "[ICON_PEACE]19 (+3)",
          "BNW TopPanel city fields differ from the authored UpdateData branch");
    state.no_religion = true;
    const auto disabled = controls.apply(ui::present_top_panel(state, localizer()));
    check(disabled.faith_text == "TXT_KEY_TOP_PANEL_RELIGION_OFF" &&
              disabled.faith_tooltip == "TXT_KEY_TOP_PANEL_RELIGION_OFF_TOOLTIP",
          "BNW TopPanel religion-off branch changed");
}

void presentation_contract() {
    ui::TopPanelState state;
    state.player = ui::TopPanelPlayerState{.city_count = 2,
                                           .science_per_turn = 5,
                                           .gold = 100,
                                           .gold_per_turn = 2,
                                           .excess_happiness = 3,
                                           .golden_age_turns = 2,
                                           .golden_age_progress = 10,
                                           .golden_age_threshold = 20,
                                           .culture = 30,
                                           .next_policy_cost = 100,
                                           .culture_per_turn = 4,
                                           .culture_for_free = 1,
                                           .culture_from_cities = 2,
                                           .culture_from_happiness = 3,
                                           .culture_from_minors = 4,
                                           .num_cities_policy_cost_mod = 20,
                                           .strategic_resources = {{"TXT_KEY_IRON", 2, 0, true,
                                                                     "TXT_KEY_IRON", 2}},
                                           .unit_production_maintenance_mod = 25,
                                           .units_supplied = 4,
                                           .units_out_of_supply = 1};
    state.game_turn = 12;
    state.turn_date = "4000 BC";
    state.head_selected_city = true;
    state.city_screen_up = true;
    const auto result = ui::present_top_panel(state, localizer());
    check(result.has_value() && result->show_info_stack, "TopPanel did not show the active player stack");
    check(result->menu_button && result->menu_button->text == "RETURN" &&
              result->menu_button->action == ui::TopPanelAction::exit_city_screen,
          "TopPanel menu action did not follow the city-screen branch");
    check(result->science_action == ui::TopPanelAction::open_technology_tree &&
              result->culture_action == ui::TopPanelAction::open_policy_choice,
          "TopPanel science/culture callbacks changed");
    check(result->science_text == "[ICON_RESEARCH][COLOR:33:190:247:255]+5[/COLOR]",
          "TopPanel science presentation changed");
    check(result->gold_text == "TXT_KEY_TOP_PANEL_GOLD(100)(2)", "TopPanel gold arguments changed");
    check(result->golden_age_text == "[ICON_GOLDEN_AGE][COLOR:255:255:255:255]GOLDEN AGE (2)[/COLOR]",
          "TopPanel golden-age presentation changed");
    check(result->resource_text == "[COLOR_POSITIVE_TEXT]2 Iron   [ENDCOLOR]",
          "TopPanel resource presentation changed");
    state.player->strategic_resources = {{"[ICON_RES_IRON]", 2, 0, true, "TXT_KEY_RESOURCE_IRON", 2}};
    const auto markup_resource = ui::present_top_panel(state, localizer());
    check(markup_resource && markup_resource->resource_text ==
                             "[COLOR_POSITIVE_TEXT]2 [ICON_RES_IRON]   [ENDCOLOR]",
          "TopPanel resource icon markup was not retained");
    state.player->strategic_resources = {{"TXT_KEY_IRON", 2, 0, true, "TXT_KEY_RESOURCE_IRON", 2}};
    check(result->unit_supply_tooltip == "TXT_KEY_UNIT_SUPPLY_REACHED_TOOLTIP(4)(1)(-25)",
          "TopPanel supply tooltip arguments changed");
    state.player->science_from_budget_deficit_times100 = -125;
    state.player->science_from_cities_times100 = 150;
    state.player->science_from_other_players_times100 = 25;
    state.player->science_from_happiness_times100 = 50;
    state.player->science_from_research_agreements_times100 = 75;
    state.player->gold_per_turn_from_diplomacy = -3;
    state.player->gold_from_cities_times100 = 125;
    state.player->city_connection_gold_times100 = 75;
    state.player->unit_cost = 2;
    state.player->unit_supply = 1;
    state.player->building_maintenance = 4;
    state.player->improvement_maintenance = 3;
    state.player->strategic_resources = {{"TXT_KEY_IRON", 2, 0, true, "TXT_KEY_RESOURCE_IRON", 2, 1}};
    state.player->happiness_from_policies = 2;
    state.player->happiness_from_resources = 5;
    state.player->extra_happiness_per_luxury = 1;
    state.player->happiness_from_buildings = 3;
    state.player->happiness_from_garrisoned_units = 1;
    state.player->happiness_from_trade_routes = 1;
    state.player->happiness_from_religion = 1;
    state.player->happiness_from_natural_wonders = 1;
    state.player->happiness_from_minors = 2;
    state.player->extra_happiness_per_city = 1;
    state.player->happiness_from_handicap = 4;
    state.player->happiness_from_resource_variety = 1;
    state.player->total_happiness = 22;
    state.player->total_unhappiness = 4;
    state.player->unhappiness_from_units_times100 = 125;
    state.player->unhappiness_from_city_count_times100 = 250;
    state.player->unhappiness_from_puppet_city_population_times100 = 50;
    state.player->unhappiness_from_city_specialists_times100 = 25;
    state.player->unhappiness_from_city_population_times100 = 400;
    state.player->unhappiness_from_occupied_cities_times100 = 100;
    const auto detailed_tooltips = ui::present_top_panel(state, localizer());
    check(detailed_tooltips && detailed_tooltips->science_tooltip &&
              detailed_tooltips->science_tooltip->find("TXT_KEY_TP_SCIENCE_FROM_CITIES(1.5)") != std::string::npos &&
              detailed_tooltips->science_tooltip->find("TXT_KEY_TP_SCIENCE_FROM_RESEARCH_AGREEMENTS(0.75)") !=
                  std::string::npos,
          "TopPanel science source tooltip details were not retained");
    check(detailed_tooltips->gold_tooltip &&
              detailed_tooltips->gold_tooltip->find("TXT_KEY_TP_CITY_OUTPUT(1.25)") != std::string::npos &&
              detailed_tooltips->gold_tooltip->find("TXT_KEY_TP_GOLD_TILE_MAINT(3)") != std::string::npos &&
              detailed_tooltips->gold_tooltip->find("TXT_KEY_TP_GOLD_TO_OTHERS(3)") != std::string::npos,
          "TopPanel gold source tooltip details were not retained");
    state.no_basic_help = true;
    const auto compact_tooltips = ui::present_top_panel(state, localizer());
    check(compact_tooltips && compact_tooltips->science_tooltip &&
              compact_tooltips->science_tooltip->find("TXT_KEY_TP_SCIENCE(5)") == std::string::npos &&
              compact_tooltips->gold_tooltip &&
              compact_tooltips->gold_tooltip->find("TXT_KEY_TP_AVAILABLE_GOLD") == std::string::npos,
          "TopPanel no-basic-help tooltip branch changed");
    check(result->current_turn_text == "TXT_KEY_TP_TURN_COUNTER(12)" && result->current_date_text == "4000 BC",
          "TopPanel turn/date presentation changed");
    check(detailed_tooltips->golden_age_tooltip &&
              detailed_tooltips->golden_age_tooltip->find("TXT_KEY_TP_GOLDEN_AGE_EFFECT") != std::string::npos &&
              detailed_tooltips->culture_tooltip &&
              detailed_tooltips->culture_tooltip->find("TXT_KEY_NEXT_POLICY_TURN_LABEL(18)") != std::string::npos &&
              detailed_tooltips->culture_tooltip->find("TXT_KEY_TP_CULTURE_FROM_MINORS(4)") != std::string::npos,
          "TopPanel Golden Age and culture tooltip source details were not retained");
    check(detailed_tooltips->resource_tooltip &&
              detailed_tooltips->resource_tooltip->find("TXT_KEY_TP_RESOURCE_INFO(2)(0)") != std::string::npos,
          "TopPanel resource tooltip details were not retained");
    check(detailed_tooltips->happiness_tooltip &&
              detailed_tooltips->happiness_tooltip->find("TXT_KEY_TP_TOTAL_HAPPINESS(3)") != std::string::npos &&
              detailed_tooltips->happiness_tooltip->find("TXT_KEY_TP_HAPPINESS_EACH_RESOURCE(1)") !=
                  std::string::npos &&
              detailed_tooltips->happiness_tooltip->find("TXT_KEY_TP_UNHAPPINESS_POPULATION(3.25)") !=
                  std::string::npos &&
              detailed_tooltips->happiness_tooltip->find("TXT_KEY_TP_HAPPINESS_EXPLANATION") != std::string::npos,
          "TopPanel happiness tooltip source details were not retained");

    state.no_science = true;
    state.no_happiness = true;
    state.no_policies = true;
    const auto disabled_modes = ui::present_top_panel(state, localizer());
    check(disabled_modes && disabled_modes->science_tooltip == "TXT_KEY_TOP_PANEL_SCIENCE_OFF_TOOLTIP" &&
              disabled_modes->happiness_tooltip == "TXT_KEY_TOP_PANEL_HAPPINESS_OFF_TOOLTIP" &&
              disabled_modes->golden_age_tooltip == "TXT_KEY_TOP_PANEL_HAPPINESS_OFF_TOOLTIP" &&
              disabled_modes->culture_tooltip == "TXT_KEY_TOP_PANEL_POLICIES_OFF_TOOLTIP",
          "TopPanel disabled-mode tooltip branches changed");
    state.available_metrics.emplace();
    const auto disabled_without_quotes = ui::present_top_panel(state, localizer());
    check(disabled_without_quotes &&
              disabled_without_quotes->science_text == "TXT_KEY_TOP_PANEL_SCIENCE_OFF" &&
              disabled_without_quotes->science_tooltip == "TXT_KEY_TOP_PANEL_SCIENCE_OFF_TOOLTIP" &&
              disabled_without_quotes->happiness_text == "TXT_KEY_TOP_PANEL_HAPPINESS_OFF" &&
              disabled_without_quotes->happiness_tooltip == "TXT_KEY_TOP_PANEL_HAPPINESS_OFF_TOOLTIP" &&
              disabled_without_quotes->golden_age_text == "TXT_KEY_TOP_PANEL_GOLDEN_AGES_OFF" &&
              disabled_without_quotes->culture_text == "TXT_KEY_TOP_PANEL_POLICIES_OFF" &&
              disabled_without_quotes->culture_tooltip == "TXT_KEY_TOP_PANEL_POLICIES_OFF_TOOLTIP" &&
              disabled_without_quotes->gold_text == "N/A",
          "Known disabled game options were hidden by absent live metric quotes");
    state.available_metrics.reset();

    state.city_screen_up = false;
    const auto selected_city_in_world = ui::present_top_panel(state, localizer());
    check(selected_city_in_world && selected_city_in_world->menu_button &&
              selected_city_in_world->menu_button->text == "MENU" &&
              selected_city_in_world->menu_button->action == ui::TopPanelAction::open_game_menu,
          "TopPanel selected-city world-view branch incorrectly exits the city screen");

    state.player->city_count = 0;
    const auto no_cities = ui::present_top_panel(state, localizer());
    check(no_cities && !no_cities->show_info_stack && !no_cities->menu_button && !no_cities->science_text,
          "TopPanel empty-city branch did not suppress the info stack");
    state.player.reset();
    check(!ui::present_top_panel(state, localizer()), "TopPanel missing-player branch did not preserve no-update state");
}

void action_route_contract() {
    const auto civilopedia = ui::top_panel_action_event(ui::TopPanelAction::open_civilopedia);
    check(civilopedia.action == ui::TopPanelAction::open_civilopedia &&
              civilopedia.route == ui::TopPanelActionRoute::search_for_pedia_entry,
          "TopPanel Civilopedia action route changed");
    const auto menu = ui::top_panel_action_event(ui::TopPanelAction::open_game_menu);
    check(menu.route == ui::TopPanelActionRoute::queue_game_menu,
          "TopPanel menu action route changed");
    const auto exit = ui::top_panel_action_event(ui::TopPanelAction::exit_city_screen);
    check(exit.route == ui::TopPanelActionRoute::serial_event_exit_city_screen,
          "TopPanel city exit action route changed");
    const auto policy = ui::top_panel_action_event(ui::TopPanelAction::open_policy_choice);
    check(policy.route == ui::TopPanelActionRoute::choose_policy_popup,
          "TopPanel policy action route changed");
    const auto technology = ui::top_panel_action_event(ui::TopPanelAction::open_technology_tree);
    check(technology.route == ui::TopPanelActionRoute::tech_tree_popup,
          "TopPanel technology action route changed");
}

void runtime_owner_contract() {
    ui::TopPanelState state;
    state.game_turn = 7;
    state.turn_date = "3760 BC";
    state.player = ui::TopPanelPlayerState{.city_count = 1, .science_per_turn = 3, .gold = 12,
                                           .gold_per_turn = 1, .culture = 4, .next_policy_cost = 25};
    unsigned snapshots = 0;
    std::vector<ui::TopPanelActionEvent> events;
    ui::TopPanelRuntime runtime(
        [&] {
            ++snapshots;
            return state;
        },
        localizer(), [&](const ui::TopPanelActionEvent &event) { events.push_back(event); });
    const auto first = runtime.present();
    check(first && first->current_turn_text == "TXT_KEY_TP_TURN_COUNTER(7)" && snapshots == 1,
          "TopPanel runtime did not publish one caller-owned state snapshot");
    state.game_turn = 8;
    const auto second = runtime.present();
    check(second && second->current_turn_text == "TXT_KEY_TP_TURN_COUNTER(8)" && snapshots == 2,
          "TopPanel runtime did not resample the current frame state");
    const auto event = runtime.dispatch(ui::TopPanelAction::open_technology_tree);
    check(event.route == ui::TopPanelActionRoute::tech_tree_popup && events.size() == 1 &&
              events.front().route == ui::TopPanelActionRoute::tech_tree_popup,
          "TopPanel runtime did not forward the typed popup route");
    rejects([&] { ui::TopPanelRuntime({}, localizer()); });
    rejects([&] { ui::TopPanelRuntime([] { return ui::TopPanelState{}; }, {}); });
}

saves::PlayerCoreField core_field(const char *name, std::int64_t value) {
    saves::PlayerCoreField result{};
    result.name = name;
    result.encoding = saves::PlayerCoreEncoding::i32;
    result.values = {value};
    return result;
}

saves::PlayerAdministrationRecord administration_block(
    const char *name, std::initializer_list<std::pair<const char *, std::int64_t>> values) {
    saves::PlayerAdministrationRecord result{};
    result.name = name;
    for (const auto &[field_name, value] : values)
        result.fields.push_back({field_name, value});
    return result;
}

void saved_projection_contract() {
    saves::SavedPlayerObservation observation{};
    observation.core.layout = saves::GameStateLayout::expansion2_version1;
    observation.core.fields = {core_field("m_iGoldenAgeProgressMeter", 11),
                               core_field("m_iGoldenAgeTurns", 2), core_field("m_iJONSCulture", 37),
                               core_field("m_iFaith", 19),
                               core_field("m_iCostNextPolicy", 90),
                               core_field("m_iUnitProductionMaintenanceMod", 25),
                               core_field("m_iHappinessFromBuildings", 8),
                               core_field("m_iExtraHappinessPerLuxury", 2),
                               core_field("m_iHappinessPerCity", 1)};
    observation.administration.blocks.push_back(
        administration_block("treasury", {{"m_iGold", 700}, {"m_iGoldPerTurnFromDiplomacy", -3},
                                           {"m_iExpensePerTurnUnitMaintenance", 4},
                                           {"m_iExpensePerTurnUnitSupply", 2},
                                           {"m_iCityConnectionGoldTimes100", 125},
                                           {"m_iBaseBuildingGoldMaintenance", 6},
                                           {"m_iBaseImprovementGoldMaintenance", 1}}));
    observation.cities.cities.resize(3);

    const auto projection = ui::project_top_panel_saved_player(observation);
    check(projection.city_count == 3 && projection.gold == 700 &&
              projection.gold_per_turn_from_diplomacy == -3 && projection.city_connection_gold_times100 == 125 &&
              projection.unit_cost == 4 && projection.unit_supply == 2 && projection.building_maintenance == 6 &&
              projection.improvement_maintenance == 1 && projection.golden_age_progress &&
              projection.golden_age_progress->raw == 11 && projection.golden_age_progress->scale == 1 &&
              projection.golden_age_turns == 2 && projection.culture && projection.culture->raw == 37 &&
              projection.culture->scale == 1 && projection.faith && projection.faith->raw == 19 &&
              projection.faith->scale == 1 && projection.next_policy_cost == 90 &&
              projection.unit_production_maintenance_mod == 25 && projection.happiness_from_buildings == 8 &&
              projection.extra_happiness_per_luxury == 2 && projection.happiness_per_city == 1,
          "TopPanel saved projection did not retain exact BNW fields");

    ui::TopPanelPlayerState state;
    state.gold_per_turn = 99;
    state.science_per_turn = 8;
    ui::apply_top_panel_saved_projection(state, projection);
    check(state.city_count == 3 && state.gold == 700 && state.gold_per_turn_from_diplomacy == -3 &&
              state.unit_cost == 4 && state.unit_supply == 2 && state.golden_age_progress == 11 &&
              state.culture == 37 && state.faith == 19 && state.next_policy_cost == 90 &&
              state.unit_production_maintenance_mod == 25 &&
              state.happiness_from_buildings == 8 && state.extra_happiness_per_luxury == 2 &&
              state.extra_happiness_per_city == 1 &&
              state.gold_per_turn == 99 && state.science_per_turn == 8,
          "TopPanel saved projection overwrote unavailable derived values");

    saves::SavedPlayerObservation vp_observation{};
    vp_observation.core.layout = saves::GameStateLayout::vp542_corpus;
    vp_observation.core.fields = {core_field("m_iGoldenAgeProgressMeterTimes100", 1150),
                                  core_field("m_iJONSCultureTimes100", 3725),
                                  core_field("m_iFaithTimes100", 1925),
                                  core_field("m_iHappinessFromMinorCivs", 4),
                                  core_field("m_iHappinessTotal", 17), core_field("m_iUnhappiness", 9)};
    const auto vp_projection = ui::project_top_panel_saved_player(vp_observation);
    check(vp_projection.golden_age_progress && vp_projection.golden_age_progress->raw == 1150 &&
              vp_projection.golden_age_progress->scale == 100 && vp_projection.culture &&
              vp_projection.culture->raw == 3725 && vp_projection.culture->scale == 100 &&
              vp_projection.faith && vp_projection.faith->raw == 1925 &&
              vp_projection.faith->scale == 100 &&
              vp_projection.happiness_from_minors == 4 && vp_projection.total_happiness == 17 &&
              vp_projection.total_unhappiness == 9,
          "TopPanel saved projection lost VP source scales");
    state.culture = 41;
    state.faith = 43;
    state.golden_age_progress = 13;
    ui::apply_top_panel_saved_projection(state, vp_projection);
    check(state.culture == 41 && state.faith == 43 && state.golden_age_progress == 13,
          "TopPanel saved projection truncated VP hundredths into whole-unit state");
    ui::apply_top_panel_saved_projection(state, vp_projection);
    check(state.happiness_from_minors == 4 && state.total_happiness == 17 && state.total_unhappiness == 9,
          "TopPanel saved projection lost exact VP happiness totals");

    saves::SavedSessionSnapshot snapshot;
    snapshot.all_players = saves::AllPlayersObservation{};
    observation.saved_player_id = 4294967297LL;
    vp_observation.saved_player_id = 7;
    snapshot.all_players->players.push_back(vp_observation);
    snapshot.all_players->players.push_back(observation);
    const auto selected = ui::project_top_panel_saved_player(snapshot, observation.saved_player_id);
    check(selected.city_count == 3 && selected.gold == 700,
          "TopPanel session projection selected by collection position instead of saved identity");
    ui::apply_top_panel_saved_projection(state, selected);
    check(state.gold == 700 && state.science_per_turn == 8 && state.gold_per_turn == 99,
          "TopPanel session projection overwrote caller-owned derived rates");
    const auto selected_vp = ui::project_top_panel_saved_player(snapshot, 7);
    check(selected_vp.culture && selected_vp.culture->raw == 3725 && selected_vp.culture->scale == 100,
          "TopPanel session selection discarded source precision");
    saves::SavedSessionSnapshot without_players;
    rejects([&] { (void)ui::project_top_panel_saved_player(without_players, 0); });
    rejects([&] { (void)ui::project_top_panel_saved_player(snapshot, 1); });
    snapshot.all_players->players.push_back(observation);
    rejects([&] { (void)ui::project_top_panel_saved_player(snapshot, observation.saved_player_id); });
}

void live_frame_contract() {
    runtime::ResearchContext game;
    game.players[7].alive = true;
    game.players[8].alive = true;
    runtime::CityYieldContext cities;
    cities.players[7].cities[3] = {};
    cities.players[8] = {};
    runtime::TreasuryContext treasury;
    treasury.players[7].balance_times100 = 700;
    treasury.players[8].balance_times100 = 900;
    runtime::ScienceQuote science;
    science.total_times100 = 500;
    science.cities = 500;
    science.gold_rate_times100 = 200;
    runtime::TreasuryNetQuote net;
    net.net_times100 = 200;
    net.expenses = runtime::MaintenanceQuote{};
    auto live = ui::capture_top_panel_live(game, cities, treasury, {{7, science}}, {{7, net}}, "4000 BC");
    check(live.players.at(7).city_count == 1 && live.players.at(7).values.gold == 7 &&
              live.players.at(7).values.gold_per_turn == 2 &&
              live.players.at(7).available.contains(ui::TopPanelMetric::science) &&
              live.players.at(7).available.contains(ui::TopPanelMetric::gold),
          "TopPanel live capture lost runtime player or mechanic quotes");
    check(live.players.at(7).gold_balance == 7 &&
              live.players.at(8).gold_balance == 9 &&
              live.players.at(8).city_count == 0 &&
              !live.players.at(8).available.contains(ui::TopPanelMetric::gold),
          "TopPanel live capture lost owned balance or fabricated a missing quote");
    const ui::UiFrameSnapshot frame(4, 7, 0);
    const auto presented = ui::present_top_panel_frame(
        frame.with(std::make_shared<const ui::TopPanelLiveFrame>(live)), localizer());
    check(presented && presented->show_info_stack && presented->science_text &&
              presented->science_text->find("+5") != std::string::npos &&
              presented->gold_text && presented->gold_text->find("7") != std::string::npos &&
              presented->culture_text == "N/A" && presented->culture_tooltip == "Live game data unavailable" &&
              presented->current_turn_text == "TXT_KEY_TP_TURN_COUNTER(0)",
          "TopPanel live frame showed synthetic or wrong active-player data");
    live.players.at(7).available.erase(ui::TopPanelMetric::gold);
    const auto partial = ui::present_top_panel_frame(
        frame.with(std::make_shared<const ui::TopPanelLiveFrame>(live)), localizer());
    check(partial && partial->gold_text &&
              partial->gold_text->find("[ICON_GOLD]") != std::string::npos &&
              partial->gold_text->find("7") != std::string::npos &&
              partial->gold_text->find("N/A") != std::string::npos &&
              partial->gold_tooltip &&
              partial->gold_tooltip->find("TXT_KEY_TP_AVAILABLE_GOLD(7)") != std::string::npos &&
              partial->gold_tooltip->find("TXT_KEY_TP_TOTAL_INCOME") == std::string::npos,
          "Known live balance was hidden or partial quote fabricated gold income");
    const auto cityless = ui::present_top_panel_frame(
        ui::UiFrameSnapshot(5, 8, 1).with(std::make_shared<const ui::TopPanelLiveFrame>(live)), localizer());
    check(cityless && !cityless->show_info_stack && !cityless->science_text,
          "TopPanel live frame failed cityless active-player switch");
    check(!ui::present_top_panel_frame(ui::UiFrameSnapshot(6, 9, 1)
              .with(std::make_shared<const ui::TopPanelLiveFrame>(live)), localizer()),
          "TopPanel live frame invented an absent active player");

    const auto in_game = content::read_xml_metadata(
        "<Context><LuaContext ID=\"WorldView\" FileName=\"Assets/UI/InGame/WorldView/WorldView\"/>"
        "<LuaContext ID=\"TechTree\" FileName=\"Assets/UI/InGame/TechTree/TechTree\" Hidden=\"True\"/>"
        "<LuaContext ID=\"CityView\" FileName=\"Assets/UI/InGame/CityView/CityView\" Hidden=\"True\"/>"
        "</Context>");
    const auto world_view = content::read_xml_metadata("<Context/>");
    ui::UiScreenController controller(ui::UiContextRegistry::from_documents(in_game, world_view));
    check(ui::dispatch_top_panel_action(ui::TopPanelAction::open_technology_tree, controller, frame).status ==
              ui::UiCommandStatus::navigation && controller.visible("TechTree"),
          "TopPanel technology action did not route through the shared controller");
    check(ui::dispatch_top_panel_action(ui::TopPanelAction::exit_city_screen, controller, frame).status ==
              ui::UiCommandStatus::invalid,
          "TopPanel reported a city-exit success without an open city screen");
}
} // namespace

int main() {
    layout_contract();
    selected_asset_contract();
    expansion2_panel_contract();
    presentation_contract();
    action_route_contract();
    runtime_owner_contract();
    saved_projection_contract();
    live_frame_contract();
    std::cout << "PASS: " << checks << " native Civ V TopPanel source/presentation/asset checks\n";
}
