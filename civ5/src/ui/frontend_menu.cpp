#include "civ5/ui/frontend_menu.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace civ5::ui {
namespace {
UiCommandResult ready() { return {UiCommandStatus::ready, {}}; }
UiCommandResult navigation() { return {UiCommandStatus::navigation, {}}; }
UiCommandResult unavailable(std::string reason) {
    return {UiCommandStatus::unavailable, std::move(reason)};
}
bool is_ready(const UiCommandResult &result) { return result.status == UiCommandStatus::ready; }
std::string package_label(const FrontendPackage &package, bool detail = false) {
    auto label = package.name + " v" + std::to_string(package.version);
    const auto &source = detail || package.source_label.empty() ? package.source_detail : package.source_label;
    if (!source.empty()) label += " [" + source + "]";
    return label;
}
struct QueryGuard {
    bool &flag;
    explicit QueryGuard(bool &flag) : flag(flag) { flag = true; }
    ~QueryGuard() { flag = false; }
};
bool valid_value(const FrontendField &field, const FrontendValue &value) {
    switch (field.kind) {
    case FrontendFieldKind::toggle: return std::holds_alternative<bool>(value);
    case FrontendFieldKind::integer:
        if (const auto *integer = std::get_if<std::int64_t>(&value))
            return *integer >= field.minimum && *integer <= field.maximum;
        return false;
    case FrontendFieldKind::text:
        if (const auto *string = std::get_if<std::string>(&value))
            return string->size() <= field.maximum_text_bytes && string->find('\0') == std::string::npos;
        return false;
    case FrontendFieldKind::choice:
        return std::any_of(field.choices.begin(), field.choices.end(),
            [&](const auto &choice) { return choice.value == value; });
    }
    return false;
}
bool shown_on(const FrontendField &field, std::string_view screen) {
    return field.screen == screen || std::find(field.additional_screens.begin(),
        field.additional_screens.end(), screen) != field.additional_screens.end();
}
bool binds(const FrontendField &field, std::string_view screen, std::string_view control) {
    if (!shown_on(field, screen)) return false;
    const auto binding = field.controls.find(screen);
    return (binding == field.controls.end() ? std::string_view(field.id) : binding->second) == control;
}
std::string key_name(std::string_view key) {
    if (key.starts_with("{TXT_KEY_") && key.ends_with("}")) {
        key.remove_prefix(1); key.remove_suffix(1);
        key = key.substr(0, key.find(':'));
    }
    return std::string(key);
}
std::string value_text(const FrontendValue &value) {
    if (const auto *v = std::get_if<std::string>(&value)) return *v;
    if (const auto *v = std::get_if<std::int64_t>(&value)) return std::to_string(*v);
    return std::get<bool>(value) ? "[x]" : "[ ]";
}
bool conditional_owner_control(std::string_view source, const FrontendControlSource &control) {
    // These callbacks require a real pending dialog or network-session state.
    // Static Hidden is not a readiness gate for ordinary Lua-populated fields.
    if ((source == "mpgamesetupscreen.xml" && control.id == "ExitButton") ||
        (source == "stagingroom.xml" && control.id == "CountdownButton")) return true;
    static const std::map<std::string, std::vector<std::string>, std::less<>> modals{
        {"loadmenu.xml", {"DeleteConfirm", "ReferencedPackagesWindow"}},
        {"loadreplaymenu.xml", {"DeleteConfirm", "ReferencedPackagesWindow"}},
        {"optionsmenu.xml", {"GraphicsChangedPopup", "Countdown"}},
        {"installedpanel.xml", {"DeleteConfirm", "OptionsPopup"}},
        {"savemenu.xml", {"DeleteConfirm"}},
        {"savemapmenu.xml", {"DeleteConfirm"}},
        {"stagingroom.xml", {"HotJoinPopup"}}};
    const auto found = modals.find(source);
    if (found == modals.end()) return false;
    return std::any_of(found->second.begin(), found->second.end(), [&](const auto &id) {
        return std::find(control.ancestor_ids.begin(), control.ancestor_ids.end(), id) != control.ancestor_ids.end();
    });
}
std::string unbound_caption(std::string_view source, std::string_view control) {
    // These original controls are labelled by a sibling or by Lua. A caption
    // makes their unavailable owner visible; it supplies no setting value.
    // Parameterized Lua captions (volume and city-state count) use menu-only
    // labels so an absent value never becomes a fabricated zero.
    static const std::map<std::string, std::map<std::string, std::string, std::less<>>, std::less<>> captions{
        {"advancedsetup.xml", {
            {"MapTypePullDown", "TXT_KEY_AD_SETUP_MAP_TYPE"},
            {"MapSizePullDown", "TXT_KEY_AD_SETUP_MAP_SIZE"},
            {"HandicapPullDown", "TXT_KEY_AD_SETUP_HANDICAP"},
            {"GameSpeedPullDown", "TXT_KEY_AD_SETUP_GAME_SPEED"},
            {"EraPullDown", "TXT_KEY_AD_SETUP_GAME_ERA"},
            {"MinorCivsSlider", "frontend.caption.city_states"},
            {"MaxTurnsEdit", "TXT_KEY_AD_SETUP_MAX_TURNS"}}},
        {"mpgamesetupscreen.xml", {
            {"MapTypePullDown", "TXT_KEY_AD_SETUP_MAP_TYPE"},
            {"MapSizePullDown", "TXT_KEY_AD_SETUP_MAP_SIZE"},
            {"GameSpeedPullDown", "TXT_KEY_AD_SETUP_GAME_SPEED"},
            {"EraPull", "TXT_KEY_AD_SETUP_GAME_ERA"},
            {"TurnModePull", "TXT_KEY_AD_SETUP_GAME_TURN_MODE"},
            {"MinorCivsSlider", "frontend.caption.city_states"},
            {"MaxTurnsEdit", "TXT_KEY_AD_SETUP_MAX_TURNS"},
            {"TurnTimerEdit", "TXT_KEY_GAME_OPTION_END_TURN_TIMER_ENABLED"}}},
        {"optionsmenu.xml", {
            {"MusicVolumeSlider", "frontend.caption.volume_music"},
            {"EffectsVolumeSlider", "frontend.caption.volume_effects"},
            {"SpeechVolumeSlider", "frontend.caption.volume_speech"},
            {"AmbienceVolumeSlider", "frontend.caption.volume_ambience"},
            {"TurnNotifySmtpPassRetypeEdit", "TXT_KEY_OPSCREEN_TURN_NOTIFY_SMTP_RETYPE_PASSWORD"}}},
        {"setcivnames.xml", {
            {"EditNickName", "TXT_KEY_MP_NICK_NAME"},
            {"EditCivLeader", "TXT_KEY_PEDIA_LEADER_NAME"},
            {"EditCivName", "TXT_KEY_PEDIA_CIVILIZATION_NAME"},
            {"EditCivShortName", "TXT_KEY_PEDIA_CIVILIZATION_SHORT_NAME"},
            {"EditCivAdjective", "TXT_KEY_PEDIA_CIVILIZATION_ADJECTIVE"},
            {"EditPassword", "TXT_KEY_MP_PASSWORD"},
            {"EditRetypePassword", "TXT_KEY_MP_RETYPE_PASSWORD"}}},
        {"loadmenu.xml", {{"SortByPullDown", "TXT_KEY_SORTBY_LASTMODIFIED"}}},
        {"loadreplaymenu.xml", {{"SortByPullDown", "TXT_KEY_SORTBY_LASTMODIFIED"}}},
        {"lobby.xml", {{"ListTypePulldown", "frontend.caption.server_list_type"}}}};
    const auto screen = captions.find(source);
    if (screen == captions.end()) return {};
    const auto entry = screen->second.find(control);
    return entry == screen->second.end() ? std::string{} : entry->second;
}
bool contains(const WorldViewRect &r, std::int32_t x, std::int32_t y) {
    return r.width > 0 && r.height > 0 && x >= r.x && y >= r.y &&
        std::int64_t(x) < std::int64_t(r.x) + r.width &&
        std::int64_t(y) < std::int64_t(r.y) + r.height;
}
std::vector<std::string> sorted(std::vector<std::string> values) {
    std::sort(values.begin(), values.end());
    return values;
}
bool launch_operation(std::string_view op) {
    return op == "start_game" || op == "quick_game" || op == "load_game" ||
        op == "start_scenario" || op == "start_tutorial" || op == "start_multiplayer";
}
WorldViewPaint rectangle(WorldViewRect bounds, std::string style, std::uint32_t color) {
    WorldViewPaint paint;
    paint.kind = WorldViewPaintKind::rectangle;
    paint.destination = bounds; paint.style = std::move(style); paint.rgba = color;
    return paint;
}
WorldViewPaint label(WorldViewRect bounds, std::string text, bool centered = false,
                     std::uint32_t color = 0xeae3caff) {
    WorldViewPaint paint;
    paint.kind = WorldViewPaintKind::text; paint.destination = bounds;
    paint.text = std::move(text); paint.style = centered ? "TwCenMT20:center" : "TwCenMT16";
    paint.rgba = color; paint.clip = bounds;
    return paint;
}
} // namespace

void FrontendRegistry::add_screen(FrontendScreen screen) {
    if (screen.id.empty() || this->screen(screen.id))
        throw std::invalid_argument("Frontend screen ID must be unique and nonempty");
    screens_.push_back(std::move(screen));
}
void FrontendRegistry::add_item(FrontendItem item) {
    if (item.id.empty() || this->item(item.id) || item.id == "back" || item.id == "confirm" ||
        item.id == "cancel" || item.id.find(':') != std::string::npos ||
        !screen(item.screen) || (item.target_screen.empty() == item.operation.empty()) ||
        (!item.target_screen.empty() && !screen(item.target_screen)))
        throw std::invalid_argument("Invalid or duplicate frontend item/route");
    items_.push_back(std::move(item));
}
const FrontendScreen *FrontendRegistry::screen(std::string_view id) const {
    for (const auto &entry : screens_) if (entry.id == id) return &entry;
    return nullptr;
}
const FrontendItem *FrontendRegistry::item(std::string_view id) const {
    for (const auto &entry : items_) if (entry.id == id) return &entry;
    return nullptr;
}
std::vector<const FrontendItem *> FrontendRegistry::items(std::string_view screen) const {
    std::vector<const FrontendItem *> result;
    for (const auto &entry : items_) if (entry.screen == screen) result.push_back(&entry);
    std::stable_sort(result.begin(), result.end(), [](const auto *a, const auto *b) {
        return a->order < b->order;
    });
    return result;
}
FrontendRegistry FrontendRegistry::original() {
    FrontendRegistry r;
    const auto screen = [&](const char *id, const char *title, const char *source, bool external = false) {
        r.add_screen({id, title, source, external});
    };
    screen("main", "TXT_KEY_MAIN_MENU", "MainMenu.xml");
    screen("single", "TXT_KEY_MODDING_SINGLE_PLAYER", "SinglePlayer.xml");
    screen("setup", "TXT_KEY_GAME_SELECTION_SCREEN", "GameSetupScreen.xml");
    screen("advanced", "TXT_KEY_AD_SETUP_ADVANCED_OPTIONS", "AdvancedSetup.xml");
    screen("load", "TXT_KEY_LOAD_GAME", "LoadMenu.xml");
    screen("scenarios", "TXT_KEY_SCENARIOS", "ScenariosMenu.xml");
    screen("tutorial", "TXT_KEY_TUTORIAL", "LoadTutorial.xml");
    screen("multiplayer", "TXT_KEY_MULTIPLAYER", "MultiplayerSelect.xml");
    screen("mods", "TXT_KEY_MODS", "ModsBrowser.xml");
    screen("mods_multiplayer", "TXT_KEY_MODDING_MULTIPLAYER", "ModsMultiplayer.xml");
    screen("custom_mod", "TXT_KEY_MODDING_CUSTOMGAME", "CustomMod.xml", true);
    screen("names", "TXT_KEY_NAME_CIV_TITLE", "SetCivNames.xml", true);
    screen("select_civilization", "TXT_KEY_AD_SETUP_CIVILIZATION", "SelectCivilization.xml");
    screen("select_map_type", "TXT_KEY_AD_SETUP_MAP_TYPE", "SelectMapType.xml");
    screen("select_map_size", "TXT_KEY_AD_SETUP_MAP_SIZE", "SelectMapSize.xml");
    screen("select_difficulty", "TXT_KEY_AD_SELECT_DIFFICULTY", "SelectDifficulty.xml");
    screen("select_speed", "TXT_KEY_AD_SETUP_GAME_SPEED", "SelectGameSpeed.xml");
    screen("dlc", "TXT_KEY_LOAD_MENU_DLC", "PremiumContentMenu.xml");
    screen("options", "TXT_KEY_OPTIONS", "OptionsMenu.xml");
    screen("options_game", "TXT_KEY_GAME_OPTIONS", "OptionsMenu.xml", true);
    screen("options_interface", "TXT_KEY_INTERFACE_OPTIONS", "OptionsMenu.xml", true);
    screen("options_video", "TXT_KEY_VIDEO_OPTIONS", "OptionsMenu.xml", true);
    screen("options_audio", "TXT_KEY_AUDIO_OPTIONS", "OptionsMenu.xml", true);
    screen("options_multiplayer", "TXT_KEY_MULTIPLAYER_OPTIONS", "OptionsMenu.xml", true);
    screen("other", "TXT_KEY_OTHER", "OtherMenu.xml");
    screen("pedia", "TXT_KEY_CIVILOPEDIA", "CivilopediaScreen.xml", true);
    screen("hall", "TXT_KEY_HALL_OF_FAME", "HallOfFame.xml", true);
    screen("replays", "TXT_KEY_OTHER_MENU_VIEW_REPLAYS", "LoadReplayMenu.xml", true);
    screen("credits", "TXT_KEY_CREDITS", "Credits.xml", true);
    screen("news", "TXT_KEY_LATEST_NEWS", "LatestNews.xml", true);
    screen("leaderboard", "TXT_KEY_LEADERBOARD", "Leaderboard.xml", true);
    screen("touch", "TXT_KEY_TOUCH_HELP", "TouchControlsMenu.xml", true);
    screen("lobby", "TXT_KEY_MULTIPLAYER", "Lobby.xml", true);
    screen("hotseat", "TXT_KEY_MULTIPLAYER_HOTSEAT_GAME", "MPGameSetupScreen.xml", true);
    screen("pitboss", "TXT_KEY_MULTIPLAYER_PITBOSS_GAME", "DedicatedServer.xml", true);
    const auto nav = [&](const char *id, const char *from, const char *source,
                         const char *control, const char *title, const char *to, int order,
                         const char *mode = "") {
        FrontendItem item{id, from, title, order, source, control, to, {}, false};
        if (*mode) item.draft_updates.emplace("multiplayer_mode", std::string(mode));
        r.add_item(std::move(item));
    };
    const auto op = [&](const char *id, const char *from, const char *source,
                        const char *control, const char *title, const char *action, int order,
                        bool confirm = false) {
        r.add_item({id, from, title, order, source, control, {}, action, confirm});
    };
    nav("single", "main", "MainMenu.xml", "SinglePlayerButton", "TXT_KEY_MODDING_SINGLE_PLAYER", "single", 0);
    nav("multiplayer", "main", "MainMenu.xml", "MultiplayerButton", "TXT_KEY_MULTIPLAYER", "multiplayer", 1);
    nav("mods", "main", "MainMenu.xml", "ModsButton", "TXT_KEY_MODS", "mods", 2);
    nav("dlc", "main", "MainMenu.xml", "ExpansionRulesSwitch", "TXT_KEY_LOAD_MENU_DLC", "dlc", 3);
    nav("options", "main", "MainMenu.xml", "OptionsButton", "TXT_KEY_OPTIONS", "options", 4);
    nav("other", "main", "MainMenu.xml", "OtherButton", "TXT_KEY_OTHER", "other", 5);
    op("exit", "main", "MainMenu.xml", "ExitButton", "TXT_KEY_EXIT_BUTTON", "exit_game", 6, true);
    op("quick", "single", "SinglePlayer.xml", "StartGameButton", "TXT_KEY_PLAY_NOW", "quick_game", 0);
    nav("setup", "single", "SinglePlayer.xml", "GameSetupButton", "TXT_KEY_SETUP_GAME", "setup", 1);
    nav("load", "single", "SinglePlayer.xml", "LoadGameButton", "TXT_KEY_LOAD_GAME", "load", 2);
    nav("scenarios", "single", "SinglePlayer.xml", "ScenariosButton", "TXT_KEY_SCENARIOS", "scenarios", 3);
    nav("tutorial", "single", "SinglePlayer.xml", "LoadTutorialButton", "TXT_KEY_TUTORIAL", "tutorial", 4);
    nav("advanced", "setup", "GameSetupScreen.xml", "AdvancedButton", "TXT_KEY_GAME_ADVANCED_SETUP", "advanced", 1);
    op("randomize", "setup", "GameSetupScreen.xml", "RandomizeButton", "TXT_KEY_GAME_SETUP_RANDOMIZE", "randomize_setup", 2);
    op("start", "setup", "GameSetupScreen.xml", "StartButton", "TXT_KEY_START_GAME", "start_game", 3);
    nav("setup_civilization", "setup", "GameSetupScreen.xml", "CivilizationButton", "TXT_KEY_AD_SETUP_CIVILIZATION", "select_civilization", 4);
    nav("setup_map_type", "setup", "GameSetupScreen.xml", "MapTypeButton", "TXT_KEY_AD_SETUP_MAP_TYPE", "select_map_type", 5);
    nav("setup_map_size", "setup", "GameSetupScreen.xml", "MapSizeButton", "TXT_KEY_AD_SETUP_MAP_SIZE", "select_map_size", 6);
    nav("setup_difficulty", "setup", "GameSetupScreen.xml", "DifficultyButton", "TXT_KEY_AD_SELECT_DIFFICULTY", "select_difficulty", 7);
    nav("setup_speed", "setup", "GameSetupScreen.xml", "GameSpeedButton", "TXT_KEY_AD_SETUP_GAME_SPEED", "select_speed", 8);
    nav("setup_names", "setup", "GameSetupScreen.xml", "EditButton", "TXT_KEY_EDIT_BUTTON", "names", 9);
    nav("advanced_names", "advanced", "AdvancedSetup.xml", "EditButton", "TXT_KEY_EDIT_BUTTON", "names", 4);
    op("names_apply", "names", "SetCivNames.xml", "AcceptButton", "TXT_KEY_ACCEPT_BUTTON", "apply_civ_names", 0);
    op("setup_clear_names", "setup", "GameSetupScreen.xml", "RemoveButton", "TXT_KEY_CANCEL_BUTTON", "clear_civ_names", 10);
    op("advanced_clear_names", "advanced", "AdvancedSetup.xml", "RemoveButton", "TXT_KEY_CANCEL_BUTTON", "clear_civ_names", 5);
    op("advanced_start", "advanced", "AdvancedSetup.xml", "StartButton", "TXT_KEY_START_GAME", "start_game", 3);
    op("advanced_default", "advanced", "AdvancedSetup.xml", "DefaultButton", "TXT_KEY_AD_SETUP_DEFAULT", "reset_setup", 1);
    op("advanced_add_ai", "advanced", "AdvancedSetup.xml", "AddAIButton", "TXT_KEY_AD_SETUP_ADD_AI_PLAYER", "add_ai", 2);
    op("advanced_victories", "advanced", "AdvancedSetup.xml", "VictoryConditionsStack", "TXT_KEY_VICTORYS_FORMAT", "edit_victory_conditions", 6);
    op("advanced_options", "advanced", "AdvancedSetup.xml", "GameOptionsStack", "TXT_KEY_AD_SETUP_GAME_OPTIONS", "edit_game_options", 7);
    op("advanced_map_options", "advanced", "AdvancedSetup.xml", "DropDownOptionsStack", "frontend.caption.map_options", "edit_map_options", 8);
    op("hotseat_victories", "hotseat", "MPGameSetupScreen.xml", "VictoryConditionsStack", "TXT_KEY_VICTORYS_FORMAT", "edit_victory_conditions", 0);
    op("hotseat_options", "hotseat", "MPGameSetupScreen.xml", "GameOptionsStack", "TXT_KEY_AD_SETUP_GAME_OPTIONS", "edit_game_options", 1);
    op("hotseat_map_options", "hotseat", "MPGameSetupScreen.xml", "DropDownOptionsStack", "frontend.caption.map_options", "edit_map_options", 2);
    op("load_start", "load", "LoadMenu.xml", "StartButton", "TXT_KEY_LOAD_GAME", "load_game", 0);
    op("load_delete", "load", "LoadMenu.xml", "Delete", "TXT_KEY_DELETE_BUTTON", "delete_save", 1, true);
    op("load_show_dlc", "load", "LoadMenu.xml", "ShowDLCButton", "TXT_KEY_LOAD_MENU_DLC", "show_save_dlc", 2);
    op("load_show_mods", "load", "LoadMenu.xml", "ShowModsButton", "TXT_KEY_LOAD_MENU_MODS", "show_save_mods", 3);
    op("scenario_start", "scenarios", "ScenariosMenu.xml", "StartButton", "TXT_KEY_LOAD_SCENARIO", "start_scenario", 0);
    op("tutorial_start", "tutorial", "LoadTutorial.xml", "StartButton", "TXT_KEY_START_TUTORIAL_BUTTON", "start_tutorial", 0);
    op("tutorial_learn", "tutorial", "LoadTutorial.xml", "LearnButton", "TXT_KEY_TUTORIAL_INSTRUCT", "select_tutorial_intro", 1);
    nav("mp_standard", "multiplayer", "MultiplayerSelect.xml", "StandardButton", "TXT_KEY_MULTIPLAYER_STANDARD_GAME", "lobby", 0, "standard");
    nav("mp_internet", "multiplayer", "MultiplayerSelect.xml", "InternetButton", "TXT_KEY_MULTIPLAYER_INTERNET_GAME", "lobby", 1, "internet");
    nav("mp_lan", "multiplayer", "MultiplayerSelect.xml", "LANButton", "TXT_KEY_MULTIPLAYER_LAN_GAME", "lobby", 2, "lan");
    nav("mp_hotseat", "multiplayer", "MultiplayerSelect.xml", "HotSeatButton", "TXT_KEY_MULTIPLAYER_HOTSEAT_GAME", "hotseat", 3, "hotseat");
    nav("mp_pitboss", "multiplayer", "MultiplayerSelect.xml", "PitbossButton", "TXT_KEY_MULTIPLAYER_PITBOSS_GAME", "pitboss", 4, "pitboss");
    op("mp_reconnect", "multiplayer", "MultiplayerSelect.xml", "ReconnectButton", "TXT_KEY_MULTIPLAYER_RECONNECT", "reconnect", 5);
    // Both modded and unmodded games navigate to these same retained drafts.
    nav("mods_setup", "mods", "ModsBrowser.xml", "LargeButton", "TXT_KEY_SETUP_GAME", "setup", 0);
    nav("mods_load", "mods", "ModsSinglePlayer.xml", "LoadGameButton", "TXT_KEY_LOAD_GAME", "load", 1);
    op("mods_workshop", "mods", "ModsBrowser.xml", "SmallButton2", "TXT_KEY_MODDING_WORKSHOP", "open_workshop", 2);
    nav("mods_multiplayer", "mods", "ModsMenu.xml", "MultiPlayerButton", "TXT_KEY_MODDING_MULTIPLAYER", "mods_multiplayer", 3);
    nav("mods_custom", "mods", "ModsSinglePlayer.xml", "CustomGameButton", "TXT_KEY_MODDING_CUSTOMGAME", "custom_mod", 4);
    op("mods_delete", "mods", "ModsBrowser.xml", "SmallButton1", "TXT_KEY_MODDING_DELETEMOD", "delete_mod", 5, true);
    // InstalledPanel's static child functions and selected-row prototypes keep
    // explicit unavailable routes until the existing mod-management owner is
    // attached. They do not duplicate the package draft selection rows.
    op("mods_details", "mods", "InstalledPanel.xml", "Button", "TXT_KEY_MODDING_DESCRIPTION", "show_mod_details", 6);
    op("mods_options", "mods", "InstalledPanel.xml", "OptionsButton", "TXT_KEY_MODDING_OPTIONS", "open_mod_options", 7);
    op("mods_sort_title", "mods", "InstalledPanel.xml", "SortbyName", "TXT_KEY_MODDING_SORT_TITLE", "sort_mods_by_title", 8);
    op("mods_sort_enabled", "mods", "InstalledPanel.xml", "SortByEnabled", "TXT_KEY_MODDING_SORT_ENABLED", "sort_mods_by_enabled", 9);
    op("mods_update", "mods", "InstalledPanel.xml", "UpdateButton", "TXT_KEY_MODDING_UPDATEMOD", "update_mod", 10);
    op("custom_mod_start", "custom_mod", "CustomMod.xml", "StartButton", "TXT_KEY_LOAD_MOD", "load_custom_mod", 0);
    nav("mods_internet", "mods_multiplayer", "ModsMultiplayer.xml", "InternetButton", "TXT_KEY_MULTIPLAYER_INTERNET_GAME", "lobby", 0, "internet");
    nav("mods_lan", "mods_multiplayer", "ModsMultiplayer.xml", "LANButton", "TXT_KEY_MULTIPLAYER_LAN_GAME", "lobby", 1, "lan");
    nav("mods_hotseat", "mods_multiplayer", "ModsMultiplayer.xml", "HotSeatButton", "TXT_KEY_MULTIPLAYER_HOTSEAT_GAME", "hotseat", 2, "hotseat");
    op("mods_reconnect", "mods_multiplayer", "ModsMultiplayer.xml", "ReconnectButton", "TXT_KEY_MULTIPLAYER_RECONNECT", "reconnect", 3);
    nav("options_game", "options", "OptionsMenu.xml", "GameButton", "TXT_KEY_GAME_OPTIONS", "options_game", 0);
    nav("options_interface", "options", "OptionsMenu.xml", "IFaceButton", "TXT_KEY_INTERFACE_OPTIONS", "options_interface", 1);
    nav("options_video", "options", "OptionsMenu.xml", "VideoButton", "TXT_KEY_VIDEO_OPTIONS", "options_video", 2);
    nav("options_audio", "options", "OptionsMenu.xml", "AudioButton", "TXT_KEY_AUDIO_OPTIONS", "options_audio", 3);
    nav("options_multiplayer", "options", "OptionsMenu.xml", "MultiplayerButton", "TXT_KEY_MULTIPLAYER_OPTIONS", "options_multiplayer", 4);
    op("options_apply", "options", "OptionsMenu.xml", "AcceptButton", "TXT_KEY_OPSCREEN_SAVE_BUTTON", "apply_options", 5);
    op("options_reset", "options", "OptionsMenu.xml", "DefaultButton", "TXT_KEY_OPSCREEN_DEFAULTS_BUTTON", "reset_options", 6, true);
    op("options_reset_tutorial", "options_game", "OptionsMenu.xml", "ResetTutorialButton", "TXT_KEY_OPSCREEN_RESET_TUTORIAL", "reset_tutorial", 0, true);
    op("options_resolution", "options_video", "OptionsMenu.xml", "ApplyResButton", "TXT_KEY_OPSCREEN_APPLY_RESOLUTION", "apply_resolution", 0);
    nav("pedia", "other", "OtherMenu.xml", "CivilopediaButton", "TXT_KEY_CIVILOPEDIA", "pedia", 0);
    nav("hall", "other", "OtherMenu.xml", "HallOfFameButton", "TXT_KEY_HALL_OF_FAME", "hall", 1);
    nav("replays", "other", "OtherMenu.xml", "ViewReplaysButton", "TXT_KEY_OTHER_MENU_VIEW_REPLAYS", "replays", 2);
    nav("credits", "other", "OtherMenu.xml", "CreditsButton", "TXT_KEY_CREDITS", "credits", 3);
    nav("news", "other", "OtherMenu.xml", "LatestNewsButton", "TXT_KEY_LATEST_NEWS", "news", 4);
    nav("leaderboard", "other", "OtherMenu.xml", "LeaderboardButton", "TXT_KEY_LEADERBOARD", "leaderboard", 5);
    nav("touch", "other", "MainMenu.xml", "TouchHelpButton", "TXT_KEY_TOUCH_HELP", "touch", 6);
    return r;
}

FrontendContentCheck FrontendBackend::check_selection(const FrontendRequest &request) const {
    return check_content(request);
}
FrontendContentCheck FrontendBackend::check_content(const FrontendRequest &) const { return {}; }
const FrontendHit *FrontendMenuPlan::hit(std::int32_t x, std::int32_t y) const {
    for (auto i = hits.rbegin(); i != hits.rend(); ++i)
        if (contains(i->bounds, x, y)) return &*i;
    return nullptr;
}
FrontendMenuSession::FrontendMenuSession(FrontendRegistry registry, FrontendSourceCatalog sources,
    FrontendBackend *backend, FrontendLocalizer localize)
    : registry_(std::move(registry)), sources_(std::move(sources)), backend_(backend),
      localize_(std::move(localize)) {
    if (!registry_.screen("main")) throw std::invalid_argument("Frontend requires a main screen");
}
std::string FrontendMenuSession::text(std::string_view key) const {
    if (localize_ && !querying_) {
        QueryGuard guard(querying_);
        if (const auto result = localize_(key_name(key)); result && !result->empty()) return *result;
    }
    return key_name(key);
}
const FrontendField *FrontendMenuSession::field(std::string_view id) const {
    for (const auto &entry : data_.fields) if (entry.id == id) return &entry;
    return nullptr;
}
void FrontendMenuSession::refresh(FrontendData data) {
    if (executing_ || querying_ || composing_) throw std::logic_error("Cannot refresh frontend during an owner callback");
    std::set<std::string> ids;
    for (const auto &entry : data.packages)
        if (entry.key.empty() || !ids.insert(entry.key).second)
            throw std::invalid_argument("Duplicate frontend package identity");
    ids.clear();
    for (const auto &entry : data.saves)
        if (entry.id.empty() || !ids.insert(entry.id).second)
            throw std::invalid_argument("Duplicate frontend save identity");
    ids.clear();
    std::set<std::pair<std::string, std::string>> field_controls;
    for (const auto &entry : data.fields) {
        if (entry.id.empty() || !ids.insert(entry.id).second || !registry_.screen(entry.screen))
            throw std::invalid_argument("Invalid frontend field identity/screen");
        if (entry.minimum > entry.maximum) throw std::invalid_argument("Invalid frontend field range");
        if (entry.initial && !valid_value(entry, *entry.initial))
            throw std::invalid_argument("Invalid owner-provided initial frontend value");
        std::set<std::string> screens{entry.screen};
        for (const auto &screen : entry.additional_screens)
            if (!registry_.screen(screen) || !screens.insert(screen).second)
                throw std::invalid_argument("Invalid additional frontend field screen");
        for (const auto &[screen, control] : entry.controls)
            if (control.empty() || !shown_on(entry, screen) ||
                !field_controls.emplace(screen, control).second)
                throw std::invalid_argument("Invalid or duplicate frontend field control binding");
    }
    data_ = std::move(data);
    for (const auto &entry : data_.fields) if (entry.initial && !draft_.values.contains(entry.id))
        draft_.values.emplace(entry.id, *entry.initial);
    // Keep unavailable selections to explain disappearance; never replace them
    // silently with a different save, package version or field default.
    ++revision_;
}
void FrontendMenuSession::set_external_presenter(FrontendExternalPresenter presenter) {
    if (executing_ || querying_ || composing_) throw std::logic_error("Cannot replace presenter during an owner callback");
    external_ = std::move(presenter); ++revision_;
}
void FrontendMenuSession::set_external_dispatcher(FrontendExternalDispatcher dispatcher) {
    if (executing_ || querying_ || composing_) throw std::logic_error("Cannot replace dispatcher during an owner callback");
    external_dispatch_ = std::move(dispatcher); ++revision_;
}
UiCommandResult FrontendMenuSession::report(UiCommandResult result) {
    message_ = result.reason;
    ++revision_;
    return result;
}
std::optional<FrontendRequest> FrontendMenuSession::request(const FrontendItem &item) const {
    if (item.operation.empty()) return std::nullopt;
    FrontendRequest result{item.operation, item.source_key, item.control_id, draft_, {}};
    if (item.operation == "load_game" && draft_.save) for (const auto &entry : data_.saves)
        if (entry.id == *draft_.save) result.save = entry;
    return result;
}
UiCommandResult FrontendMenuSession::request_availability(const FrontendRequest &request, bool executing) const {
    if (querying_) return unavailable("Frontend owner callback is already in progress");
    QueryGuard guard(querying_);
    if (!backend_) return unavailable("Frontend operation owner unavailable: " + request.operation);
    if (request.operation == "load_game") {
        if (!request.save) return unavailable("Select a saved game");
        if (!is_ready(request.save->availability)) return request.save->availability;
    }
    if (launch_operation(request.operation)) {
        for (const auto &key : request.draft.packages) {
            const auto found = std::find_if(data_.packages.begin(), data_.packages.end(),
                [&](const auto &entry) { return entry.key == key; });
            if (found == data_.packages.end()) return unavailable("Selected content is no longer installed: " + key);
            if (!is_ready(found->availability)) return found->availability;
        }
        if (request.operation == "start_game" || request.operation == "quick_game") {
            for (const auto &entry : data_.fields) {
                if (!shown_on(entry, "setup") && !shown_on(entry, "advanced")) continue;
                if (!is_ready(entry.availability)) return entry.availability;
                const auto selected = request.draft.values.find(entry.id);
                if (selected == request.draft.values.end())
                    return unavailable("Select a game setting: " + entry.id);
                if (!valid_value(entry, selected->second))
                    return unavailable("A selected game setting is no longer valid: " + entry.id);
            }
        }
        // Capability and cheap selection checks determine presentation. Full
        // content admission runs only immediately before the executor is used.
        const auto capability = backend_->availability(request);
        if (!is_ready(capability)) return capability;
        const auto check = executing ? backend_->check_content(request) : backend_->check_selection(request);
        if (!is_ready(check.result)) return check.result;
        if (executing && !check.requirements_known)
            return unavailable("Content admission has no verified save requirements");
        if (request.operation == "load_game" && check.requirements_known &&
            sorted(request.draft.packages) != sorted(check.required_packages))
            return unavailable("Saved game requires a different content selection. Select its mods and DLC before loading.");
        return capability;
    }
    return backend_->availability(request);
}
UiCommandResult FrontendMenuSession::availability(std::string_view id) const {
    if (handed_off_) return unavailable("Frontend has handed control to the application");
    if (executing_) return unavailable("Frontend operation in progress");
    if (querying_) return unavailable("Frontend owner callback is already in progress");
    if (id == "load_select_saved_content") {
        if (screen() != "load" || !backend_) return unavailable("Saved-content selection owner unavailable");
        if (!draft_.save) return unavailable("Select a saved game");
        for (const auto &save : data_.saves) if (save.id == *draft_.save) return save.availability;
        return unavailable("Selected save is no longer installed");
    }
    const auto *item = registry_.item(id);
    if (!item) return {UiCommandStatus::invalid, "Unknown frontend item"};
    if (!item->source_key.empty()) {
        const auto *source = sources_.find(item->source_key);
        if (!source) return unavailable("Menu source unavailable: " + item->source_key);
        if (!item->control_id.empty() && !source->find_control(item->control_id))
            return unavailable("Authored control unavailable: " + item->control_id);
    }
    if (!item->target_screen.empty()) {
        const auto *target = registry_.screen(item->target_screen);
        if (!target || (!target->source_key.empty() && !sources_.find(target->source_key)))
            return unavailable("Target menu source unavailable: " + item->target_screen);
        return ready();
    }
    try { return request_availability(*request(*item)); }
    catch (const std::exception &error) { return unavailable(error.what()); }
}
UiCommandResult FrontendMenuSession::perform(FrontendRequest request) {
    try {
        const auto result = request_availability(request, true);
        if (!is_ready(result)) return report(result);
        executing_ = true;
        const auto executed = backend_->execute(request);
        executing_ = false;
        if (executed.status == UiCommandStatus::performed &&
            (launch_operation(request.operation) || request.operation == "exit_game"))
            handed_off_ = true;
        return report(executed);
    } catch (const std::exception &error) {
        executing_ = false;
        return report(unavailable(error.what()));
    }
}
UiCommandResult FrontendMenuSession::activate(std::string_view id) {
    if (executing_ || querying_ || composing_ || handed_off_) return unavailable("Frontend is not accepting input");
    if (id == "back" || id == "cancel") return back();
    if (pending_) {
        if (id != "confirm") return unavailable("Resolve the current confirmation first");
        const auto pending = *pending_;
        // Recheck the current slot and content after confirmation, not a retained
        // availability answer. Failed confirmations remain open for cancellation.
        auto current = pending;
        current.draft = draft_;
        if (pending.operation == "load_game") {
            current.save.reset();
            for (const auto &slot : data_.saves) if (draft_.save && slot.id == *draft_.save)
                current.save = slot;
        }
        const auto result = perform(std::move(current));
        if (result.status == UiCommandStatus::performed) { pending_.reset(); keyboard_focus_.reset(); ++revision_; }
        return result;
    }
    if (choice_field_) {
        if (!id.starts_with("choice:")) return unavailable("Select a value or cancel");
        const auto *definition = field(*choice_field_);
        if (!definition) return report(unavailable("Value owner no longer available"));
        const auto index_text = id.substr(7);
        std::size_t index = 0;
        if (index_text.empty()) return {UiCommandStatus::invalid, "Invalid choice"};
        for (const char c : index_text) {
            if (c < '0' || c > '9' || index > (std::numeric_limits<std::size_t>::max() - 9) / 10)
                return {UiCommandStatus::invalid, "Invalid choice"};
            index = index * 10 + std::size_t(c - '0');
        }
        if (index >= definition->choices.size()) return {UiCommandStatus::invalid, "Invalid choice"};
        const auto field_id = *choice_field_;
        const auto result = set_value(field_id, definition->choices[index].value);
        if (result.status == UiCommandStatus::navigation) { choice_field_.reset(); keyboard_focus_.reset(); ++revision_; }
        return result;
    }
    if (id == "load_select_saved_content") return select_saved_content();
    if (id.starts_with("save:")) return select_save(id.substr(5));
    if (id.starts_with("package:")) return toggle_package(id.substr(8));
    if (id.starts_with("field:")) {
        const auto *definition = field(id.substr(6));
        if (!definition || !shown_on(*definition, screen())) return {UiCommandStatus::invalid, "Unknown field"};
        if (!is_ready(definition->availability)) return report(definition->availability);
        if (definition->kind == FrontendFieldKind::toggle) {
            const auto found = draft_.values.find(definition->id);
            if (found == draft_.values.end() || !std::holds_alternative<bool>(found->second))
                return report(unavailable("Toggle value unavailable"));
            return set_value(definition->id, !std::get<bool>(found->second));
        }
        if (!definition->choices.empty()) { choice_field_ = definition->id; keyboard_focus_.reset(); ++revision_; return navigation(); }
        return report(unavailable("Value requires the existing editor adapter"));
    }
    const auto *item = registry_.item(id);
    if (!item || item->screen != screen()) return {UiCommandStatus::invalid, "Item belongs to another screen"};
    const auto result = availability(id);
    if (!is_ready(result)) return report(result);
    if (!item->target_screen.empty()) {
        if (history_.size() >= 128) return report(unavailable("Frontend navigation depth exceeded"));
        for (const auto &[key, value] : item->draft_updates) draft_.values.insert_or_assign(key, value);
        history_.push_back(item->target_screen); keyboard_focus_.reset(); message_.clear(); ++revision_; return navigation();
    }
    auto operation = *request(*item);
    if (item->confirmation) { pending_ = std::move(operation); keyboard_focus_.reset(); message_.clear(); ++revision_; return navigation(); }
    return perform(std::move(operation));
}
UiCommandResult FrontendMenuSession::request_exit() {
    if (executing_ || querying_ || composing_ || handed_off_) return unavailable("Frontend is not accepting input");
    if (pending_) return navigation();
    // Native window close belongs to the process owner even if authored menu
    // files are unavailable. The on-screen ExitButton remains source-gated.
    FrontendRequest exit_request{"exit_game", {}, {}, draft_, {}};
    UiCommandResult result;
    try { result = request_availability(exit_request); }
    catch (const std::exception &error) { return report(unavailable(error.what())); }
    if (!is_ready(result)) return report(result);
    choice_field_.reset(); pending_ = std::move(exit_request); keyboard_focus_.reset(); message_.clear(); ++revision_;
    return navigation();
}
UiCommandResult FrontendMenuSession::back() {
    if (executing_ || querying_ || composing_ || handed_off_) return unavailable("Frontend is not accepting input");
    if (pending_) pending_.reset();
    else if (choice_field_) choice_field_.reset();
    else if (history_.size() > 1) history_.pop_back();
    else {
        if (keyboard_focus_) { keyboard_focus_.reset(); ++revision_; }
        return navigation();
    }
    keyboard_focus_.reset(); message_.clear(); ++revision_; return navigation();
}
UiCommandResult FrontendMenuSession::set_value(std::string_view id, FrontendValue value) {
    if (executing_ || querying_ || composing_ || handed_off_) return unavailable("Frontend is not accepting input");
    const auto *definition = field(id);
    if (!definition) return {UiCommandStatus::invalid, "Unknown setting"};
    if (!is_ready(definition->availability)) return report(definition->availability);
    if (!valid_value(*definition, value))
        return report({UiCommandStatus::invalid, "Setting value is outside its owner-provided choices/range"});
    draft_.values[definition->id] = std::move(value);
    message_.clear(); ++revision_; return navigation();
}
UiCommandResult FrontendMenuSession::select_save(std::string_view id) {
    if (executing_ || querying_ || composing_ || handed_off_) return unavailable("Frontend is not accepting input");
    for (const auto &entry : data_.saves) if (entry.id == id) {
        draft_.save = entry.id; ++revision_;
        message_ = text("Selected saved game") + ": " + entry.name;
        if (!entry.detail.empty()) message_ += " · " + text(entry.detail);
        return is_ready(entry.availability) ? navigation() : report(entry.availability);
    }
    return report({UiCommandStatus::invalid, "Unknown saved game"});
}
UiCommandResult FrontendMenuSession::toggle_package(std::string_view key) {
    if (executing_ || querying_ || composing_ || handed_off_) return unavailable("Frontend is not accepting input");
    const auto found = std::find_if(data_.packages.begin(), data_.packages.end(),
        [&](const auto &entry) { return entry.key == key; });
    if (found == data_.packages.end()) return report({UiCommandStatus::invalid, "Unknown content package"});
    auto selected = std::find(draft_.packages.begin(), draft_.packages.end(), key);
    if (selected != draft_.packages.end()) draft_.packages.erase(selected);
    else {
        if (!is_ready(found->availability)) return report(found->availability);
        // Only the explicitly clicked ready source is selected. Do not infer
        // a preferred version, priority or ruleset from the catalog order.
        const auto previous_size = draft_.packages.size();
        if (!found->selection_identity.empty())
            draft_.packages.erase(std::remove_if(draft_.packages.begin(), draft_.packages.end(),
                [&](const auto &selected_key) {
                    const auto old = std::find_if(data_.packages.begin(), data_.packages.end(),
                        [&](const auto &entry) { return entry.key == selected_key; });
                    return old != data_.packages.end() && old->mod == found->mod &&
                           old->selection_identity == found->selection_identity;
                }), draft_.packages.end());
        draft_.packages.push_back(found->key);
        return report({UiCommandStatus::navigation,
            text(draft_.packages.size() <= previous_size ? "Replaced source" : "Selected source") + ": " +
                package_label(*found, true)});
    }
    message_.clear(); ++revision_; return navigation();
}
UiCommandResult FrontendMenuSession::select_saved_content() {
    if (executing_ || querying_ || composing_ || handed_off_ || pending_ || choice_field_)
        return unavailable("Frontend is not accepting input");
    if (screen() != "load") return report({UiCommandStatus::invalid, "Open the saved-game menu"});
    const auto available = availability("load_select_saved_content");
    if (!is_ready(available)) return report(available);
    const auto *load = registry_.item("load_start");
    if (!load || load->operation != "load_game")
        return report(unavailable("Saved-game selection request unavailable"));
    auto proposed = *request(*load);
    // Empty explicit choices make the owner refuse ambiguous requirements,
    // rather than resolving them by the current or first catalog variant.
    proposed.draft.packages.clear();
    try {
        QueryGuard guard(querying_);
        const auto requirements = backend_->check_selection(proposed);
        if (!requirements.requirements_known)
            return report(unavailable("Saved content requirements are not known"));
        proposed.draft.packages = requirements.required_packages;
        std::set<std::string> keys;
        std::set<std::pair<bool, std::string>> identities;
        for (const auto &key : proposed.draft.packages) {
            const auto row = std::find_if(data_.packages.begin(), data_.packages.end(),
                [&](const auto &entry) { return entry.key == key; });
            if (row == data_.packages.end() || !keys.insert(key).second)
                return report(unavailable("Saved content has an unknown or repeated source"));
            if (!is_ready(row->availability)) return report(row->availability);
            if (!row->selection_identity.empty() &&
                !identities.emplace(row->mod, row->selection_identity).second)
                return report(unavailable("Saved content source is ambiguous; choose it explicitly"));
        }
        // A probe can publish the complete requirements while refusing the
        // empty draft. Earn a ready exact selection before changing any draft.
        const auto confirmed = backend_->check_selection(proposed);
        if (!is_ready(confirmed.result)) return report(confirmed.result);
        if (!confirmed.requirements_known ||
            sorted(confirmed.required_packages) != sorted(proposed.draft.packages))
            return report(unavailable("Saved content requirements changed during selection"));
    } catch (const std::exception &error) { return report(unavailable(error.what())); }
    draft_.packages = std::move(proposed.draft.packages);
    return report({UiCommandStatus::navigation, text("Selected packages required by this saved game")});
}
const FrontendHit *FrontendMenuSession::focused_hit(const FrontendMenuPlan &plan) const {
    if (executing_ || querying_ || composing_ || handed_off_ || !keyboard_focus_ ||
        plan.owner != this || plan.revision != revision_ || plan.screen != screen()) return nullptr;
    const auto found = std::find_if(plan.hits.begin(), plan.hits.end(), [&](const auto &hit) {
        return hit.enabled && hit.id == *keyboard_focus_;
    });
    return found == plan.hits.end() ? nullptr : &*found;
}
UiCommandResult FrontendMenuSession::focus_next(const FrontendMenuPlan &plan, bool backwards) {
    if (executing_ || querying_ || composing_ || handed_off_) return unavailable("Frontend is not accepting input");
    if (plan.owner != this || plan.revision != revision_ || plan.screen != screen())
        return {UiCommandStatus::invalid, "Frontend input frame is stale or belongs to another session"};
    const auto &order = plan.keyboard_order;
    if (order.empty()) return report(unavailable(text("No available button on this screen")));
    auto found = std::find_if(order.begin(), order.end(), [&](const auto &target) {
        return keyboard_focus_ && target.id == *keyboard_focus_;
    });
    std::size_t index = 0;
    if (found != order.end()) {
        const auto previous = std::size_t(found - order.begin());
        index = backwards ? (previous ? previous - 1 : order.size() - 1) : (previous + 1) % order.size();
    } else {
        // A wheel scroll clears focus; begin with a button actually on this page.
        const auto visible = backwards ? std::find_if(plan.hits.rbegin(), plan.hits.rend(),
            [](const auto &hit) { return hit.enabled && !hit.id.empty(); }) : plan.hits.rend();
        const auto forward = !backwards ? std::find_if(plan.hits.begin(), plan.hits.end(),
            [](const auto &hit) { return hit.enabled && !hit.id.empty(); }) : plan.hits.end();
        const auto initial = backwards ? (visible == plan.hits.rend() ? std::string{} : visible->id) :
            (forward == plan.hits.end() ? std::string{} : forward->id);
        const auto shown = std::find_if(order.begin(), order.end(), [&](const auto &target) { return target.id == initial; });
        index = shown == order.end() ? (backwards ? order.size() - 1 : 0) : std::size_t(shown - order.begin());
    }
    const auto &target = order[index];
    if (target.row) {
        if (!plan.visible_rows) return report(unavailable(text("No available button on this screen")));
        auto &offset = scroll_[choice_field_ ? "choice:" + *choice_field_ : screen()];
        if (*target.row < plan.first_row) offset = *target.row;
        else if (*target.row - plan.first_row >= plan.visible_rows) offset = *target.row - plan.visible_rows + 1;
    }
    keyboard_focus_ = target.id; message_.clear(); ++revision_;
    return navigation();
}
UiCommandResult FrontendMenuSession::activate_focused(const FrontendMenuPlan &plan) {
    if (executing_ || querying_ || composing_ || handed_off_) return unavailable("Frontend is not accepting input");
    if (plan.owner != this || plan.revision != revision_ || plan.screen != screen())
        return {UiCommandStatus::invalid, "Frontend input frame is stale or belongs to another session"};
    const auto *hit = focused_hit(plan);
    if (!hit) return report(unavailable(text("Select a button with Tab first")));
    if (hit->command) return dispatch(*hit->command);
    return activate(hit->id);
}
void FrontendMenuSession::scroll(std::int32_t rows) {
    if (executing_ || querying_ || composing_ || handed_off_) return;
    keyboard_focus_.reset();
    auto &offset = scroll_[choice_field_ ? "choice:" + *choice_field_ : screen()];
    const auto next = std::int64_t(std::min<std::size_t>(offset, 1000000)) + rows;
    offset = std::size_t(std::clamp<std::int64_t>(next, 0, 1000000)); ++revision_;
}
UiCommandResult FrontendMenuSession::dispatch(const UiCommand &command) {
    if (const auto *gameplay = std::get_if<UiGameplayCommand>(&command))
        if (gameplay->source_id() == "CivisFrontEnd")
            if (const auto *action = gameplay->as<FrontendActivate>()) return activate(action->item);
    if (executing_ || querying_ || composing_ || handed_off_ || pending_ || choice_field_)
        return unavailable("Frontend is not accepting input");
    const auto *current = registry_.screen(screen());
    if (!current || !current->external || !external_ || !external_dispatch_)
        return {UiCommandStatus::invalid, "Command has no active screen adapter"};
    try {
        QueryGuard guard(executing_);
        return report(external_dispatch_(*current, draft_, command));
    } catch (const std::exception &error) { return report(unavailable(error.what())); }
}
UiCommandResult FrontendMenuSession::click(const FrontendMenuPlan &plan,
    std::int32_t press_x, std::int32_t press_y, std::int32_t release_x, std::int32_t release_y) {
    if (plan.owner != this || plan.revision != revision_ || plan.screen != screen())
        return {UiCommandStatus::invalid, "Frontend input frame is stale or belongs to another session"};
    const auto *press = plan.hit(press_x, press_y);
    const auto *release = plan.hit(release_x, release_y);
    if (!press || press != release) return {UiCommandStatus::invalid, "Press and release must target the same control"};
    if (!release->enabled) return report(unavailable(release->unavailable_reason));
    if (release->command) return dispatch(*release->command);
    return activate(release->id);
}

FrontendMenuPlan FrontendMenuSession::compose(std::int32_t width, std::int32_t height,
    std::optional<WorldViewPoint> pointer) const {
    FrontendMenuPlan plan;
    plan.owner = this; plan.revision = revision_; plan.screen = screen();
    std::string presentation_reason;
    if (querying_ || composing_) {
        plan.unresolved.push_back("Reentrant frontend presentation refused"); return plan;
    }
    QueryGuard composition_guard(composing_);
    if (width <= 0 || height <= 0 || width > 32768 || height > 32768) {
        plan.unresolved.push_back("Invalid frontend viewport"); return plan;
    }
    const auto *current = registry_.screen(screen());
    if (!current) { plan.unresolved.push_back("Frontend screen missing"); return plan; }
    if (external_ && current->external && !pending_ && !choice_field_) {
        std::optional<FrontendMenuPlan> external_plan;
        try {
            QueryGuard guard(querying_);
            external_plan = external_(*current, draft_, width, height);
        } catch (const std::exception &error) {
            presentation_reason = "Existing screen presenter failed: " + std::string(error.what());
            plan.unresolved.push_back(presentation_reason);
        }
        if (external_plan) {
            plan = std::move(*external_plan);
            plan.owner = this; plan.revision = revision_; plan.screen = screen();
            for (auto &hit : plan.hits) if (hit.command && !external_dispatch_) {
                hit.enabled = false; hit.unavailable_reason = "Existing screen/action adapter unavailable";
            }
            // A close target is always frontend-owned; external screen content
            // must supply its own typed actions through its existing owner.
            if (std::none_of(plan.hits.begin(), plan.hits.end(), [](const auto &hit) {
                return hit.id == "back" && hit.enabled;
            })) {
                plan.hits.push_back({{16, height - 48, std::min(160, width - 16), 32}, "back", true, {}, {}});
                plan.paints.push_back(label(plan.hits.back().bounds, text("TXT_KEY_MODDING_MENU_BACK"), true));
            }
            plan.keyboard_order.clear();
            for (const auto &hit : plan.hits) if (hit.enabled && !hit.id.empty())
                plan.keyboard_order.push_back({hit.id, {}});
            if (keyboard_focus_) for (const auto &hit : plan.hits)
                if (hit.enabled && hit.id == *keyboard_focus_)
                    plan.paints.push_back(rectangle(hit.bounds, "Grid9FrameTurnsHL", 0xffffffff));
            return plan;
        }
    }
    const WorldViewRect viewport{0, 0, width, height};
    plan.paints.push_back(rectangle(viewport, {}, 0x000000ff));
    // FrontEnd.lua sets this texture on the centered 1920x1200 AtlasLogo.
    // Its crop/anchoring and all texture decoding stay with the shared renderer.
    if (sources_.find("FrontEnd.xml")) {
        WorldViewPaint background;
        background.kind = WorldViewPaintKind::texture;
        background.source_key = "CivilzationVAtlas.dds";
        background.destination = {(width - 1920) / 2, (height - 1200) / 2, 1920, 1200};
        background.clip = viewport; plan.paints.push_back(std::move(background));
    }
    // Authored Civ V art is resolved by the existing renderer's mounted VFS.
    const auto *main = sources_.find("MainMenu.xml");
    if (screen() == "main" && !pending_ && !choice_field_ && main && height >= 480) if (const auto *logo = main->find_control("Civ5Logo"); logo && !logo->texture.empty()) {
        WorldViewPaint paint;
        paint.kind = WorldViewPaintKind::texture; paint.source_key = logo->texture;
        const auto logo_width = std::min(630, std::max(1, width - 24));
        paint.destination = {width - logo_width, 0, logo_width, logo_width * 200 / 630};
        paint.clip = viewport; plan.paints.push_back(std::move(paint));
    }
    const bool authored_main = screen() == "main" && !pending_ && !choice_field_ &&
        height >= 600 && registry_.items("main").size() == 7;
    const auto panel_width = std::min(screen() == "main" ? 480 : 960, std::max(1, width - 32));
    const auto panel_y = authored_main ? (height - 480) / 2 + 32 :
        height < 480 ? 12 : std::min(140, height / 4);
    const auto panel_height = authored_main ? 480 : std::max(1, height - panel_y - 24);
    const auto panel_x = (width - panel_width) / 2;
    plan.paints.push_back(rectangle({panel_x, panel_y, panel_width, panel_height},
        screen() == "main" ? "Grid9Detail140" : "Grid9DetailFive140", 0xffffffff));
    if (authored_main) {
        plan.paints.push_back(rectangle({width / 2 - 160, panel_y + 44, 320, 393}, {}, 0x000000c8));
        plan.paints.push_back(rectangle({width / 2 - 208, panel_y + 44, 48, 393}, "WindowGrid", 0xffffffff));
        plan.paints.push_back(rectangle({width / 2 + 160, panel_y + 44, 48, 393}, "WindowGrid", 0xffffffff));
    }
    const auto title_y = std::min(height - 32, panel_y + 16);
    if (!authored_main)
        plan.paints.push_back(label({panel_x + 12, title_y, panel_width - 24, 28}, text(current->title_key), true));

    struct Row { std::string id, label; UiCommandResult state; std::string detail{}; };
    std::vector<Row> rows, fixed_rows;
    // The authored LoadMenu keeps its StartButton outside the file ScrollPanel.
    // Compact viewports retain the shared list so a save row stays reachable.
    const bool fixed_load_actions = screen() == "load" && !pending_ && !choice_field_ &&
        width >= 640 && height >= 480;
    const auto *source = sources_.find(current->source_key);
    if (!source) plan.unresolved.push_back("Selected frontend source unavailable: " + current->source_key);
    if (pending_) {
        rows.push_back({"confirm", text("TXT_KEY_YES_BUTTON"), ready()});
        rows.push_back({"cancel", text("TXT_KEY_NO_BUTTON"), ready()});
        plan.paints.push_back(label({panel_x + 24, title_y + 34, panel_width - 48, 26},
            text(pending_->operation == "exit_game" ? "TXT_KEY_MENU_EXIT_WARN" : "TXT_KEY_OPSCREEN_DEFAULTS_BUTTON_TT"), true));
    } else if (choice_field_) {
        if (const auto *definition = field(*choice_field_))
            for (std::size_t i = 0; i < definition->choices.size(); ++i)
                rows.push_back({"choice:" + std::to_string(i), text(definition->choices[i].label), definition->availability});
    } else {
        if (screen() == "load") {
            for (const auto &save : data_.saves)
                rows.push_back({"save:" + save.id,
                    (draft_.save == save.id ? "[x] " + text("Selected saved game") + ": " : "") + save.name +
                    (is_ready(save.availability) ? "" : " · " + text("Unavailable")), ready(),
                    is_ready(save.availability) ? save.detail : save.availability.reason});
            auto &actions = fixed_load_actions ? fixed_rows : rows;
            actions.push_back({"load_select_saved_content", text("Select content for saved game"),
                               availability("load_select_saved_content")});
        }
        if (screen() == "mods" || screen() == "dlc") for (const auto &package : data_.packages) {
            if (package.mod != (screen() == "mods")) continue;
            const bool selected = std::find(draft_.packages.begin(), draft_.packages.end(), package.key) != draft_.packages.end();
            rows.push_back({"package:" + package.key, std::string(selected ? "[x] " : "[ ] ") + package_label(package),
                           selected ? ready() : package.availability, package_label(package, true)});
        }
        for (const auto &definition : data_.fields) if (shown_on(definition, screen())) {
            std::string display = text(definition.text_key);
            if (const auto value = draft_.values.find(definition.id); value != draft_.values.end()) {
                auto rendered = value_text(value->second);
                for (const auto &choice : definition.choices) if (choice.value == value->second)
                    rendered = text(choice.label);
                display += ": " + rendered;
            }
            auto editor = definition.availability;
            if (is_ready(editor) && definition.kind == FrontendFieldKind::toggle) {
                const auto value = draft_.values.find(definition.id);
                if (value == draft_.values.end() || !std::holds_alternative<bool>(value->second))
                    editor = unavailable("Toggle value unavailable");
            } else if (is_ready(editor) && definition.choices.empty()) {
                editor = unavailable("Value requires the existing editor adapter");
            }
            rows.push_back({"field:" + definition.id, std::move(display), std::move(editor)});
        }
        std::set<std::string> mapped;
        for (const auto *item : registry_.items(screen())) {
            if (item->source_key == current->source_key) mapped.insert(item->control_id);
            if (!item->control_id.empty() && std::any_of(data_.fields.begin(), data_.fields.end(),
                [&](const auto &definition) { return binds(definition, screen(), item->control_id); }))
                continue;
            std::string title = item->text_key;
            if (const auto *origin = sources_.find(item->source_key))
                if (const auto *control = origin->find_control(item->control_id);
                    title.empty() && control && !control->text_key.empty())
                    title = control->text_key;
            auto &destination = fixed_load_actions && item->id == "load_start" ? fixed_rows : rows;
            destination.push_back({item->id, text(title), availability(item->id)});
        }
        // Preserve remaining authored inputs/actions as explicitly delegated
        // controls. No unknown callback is made successful by displaying it.
        if (source && screen() != "main") for (const auto &control : source->controls) {
            if (!control.interactive || control.id == "BackButton" || control.id == "CloseButton" ||
                mapped.contains(control.id) || conditional_owner_control(source->source_key, control)) continue;
            if (screen() == "options") continue; // Original fields live in their tab.
            if (screen().starts_with("options_")) {
                const std::map<std::string, std::string, std::less<>> panels{
                    {"options_game", "GamePanel"}, {"options_interface", "IFacePanel"},
                    {"options_video", "VideoPanel"}, {"options_audio", "AudioPanel"},
                    {"options_multiplayer", "MultiplayerPanel"}};
                const auto panel = panels.find(screen());
                if (panel == panels.end() || std::find(control.ancestor_ids.begin(), control.ancestor_ids.end(), panel->second) == control.ancestor_ids.end()) continue;
            }
            const bool supplied = std::any_of(data_.fields.begin(), data_.fields.end(),
                [&](const auto &f) { return binds(f, screen(), control.id); });
            if (supplied) continue;
            auto caption = unbound_caption(source->source_key, control.id);
            if (caption.empty()) caption = control.text_key;
            if (screen() == "setup") {
                // These selectors have Lua-populated value labels, not static
                // captions. Name the settings with the original text catalog;
                // never display the prototype civilization/bonus placeholders.
                const std::map<std::string, std::string, std::less<>> selectors{
                    {"CivilizationButton", "TXT_KEY_AD_SETUP_CIVILIZATION"},
                    {"MapTypeButton", "TXT_KEY_AD_SETUP_MAP_TYPE"},
                    {"MapSizeButton", "TXT_KEY_AD_SETUP_MAP_SIZE"},
                    {"DifficultyButton", "TXT_KEY_AD_SELECT_DIFFICULTY"},
                    {"GameSpeedButton", "TXT_KEY_AD_SETUP_GAME_SPEED"}};
                if (const auto selected = selectors.find(control.id); selected != selectors.end())
                    caption = selected->second;
            }
            if (caption.empty()) continue;
            rows.push_back({"authored:" + control.id,
                text(caption),
                unavailable("Existing screen/action adapter unavailable: " + control.id)});
        }
        if (current->external) {
            const auto unavailable_reason = "Existing screen presenter unavailable: " + current->id;
            plan.unresolved.push_back(unavailable_reason);
            if (presentation_reason.empty()) presentation_reason = unavailable_reason;
        }
    }
    const auto rows_y = authored_main ? panel_y + 54 :
        height < 480 ? (pending_ ? 96 : 76) : title_y + (pending_ ? 70 : 44);
    const auto row_height = height < 600 ? 32 : 45;
    const auto footer_y = std::max(rows_y, height - 64);
    const auto fixed_y = footer_y - std::int32_t(fixed_rows.size()) * row_height - 8;
    const auto count = authored_main ? std::size_t(7) : pending_ ? std::size_t(2) :
        std::size_t(std::max(0, fixed_rows.empty() ? (footer_y - rows_y - 32) / row_height :
            (fixed_y - rows_y - 8) / row_height));
    const auto scroll_key = choice_field_ ? "choice:" + *choice_field_ : screen();
    const auto offset = scroll_.find(scroll_key);
    const auto maximum_offset = rows.size() > count ? rows.size() - count : 0;
    plan.first_row = pending_ ? 0 : std::min(offset == scroll_.end() ? 0 : offset->second, maximum_offset);
    plan.total_rows = rows.size(); plan.visible_rows = std::min(count, rows.size() - plan.first_row);
    const auto row_width = std::min(screen() == "main" ? 320 : 800, std::max(1, panel_width - 48));
    std::string hovered_reason;
    const auto add_button = [&](const Row &row, WorldViewRect bounds) {
        const bool enabled = is_ready(row.state) && !handed_off_ && !executing_;
        const bool hovered = pointer && contains(bounds, pointer->x, pointer->y);
        plan.paints.push_back(rectangle(bounds, "ZoomButton", enabled ? 0xffffffff : 0x888888ff));
        if (enabled && (hovered || (keyboard_focus_ && row.id == *keyboard_focus_)))
            plan.paints.push_back(rectangle(bounds, "Grid9FrameTurnsHL", 0xffffffff));
        if (hovered && !enabled) hovered_reason = row.state.reason;
        else if (hovered && !row.detail.empty()) hovered_reason = row.detail;
        const auto caption = enabled ? row.label : row.label + " · " + text("Unavailable");
        plan.paints.push_back(label(bounds, caption, true, enabled ? 0xeae3caff : 0xaaa69dff));
        plan.hits.push_back({bounds, row.id, enabled, row.state.reason, {}});
    };
    for (std::size_t row = 0; row < plan.visible_rows; ++row)
        add_button(rows[plan.first_row + row], {(width - row_width) / 2,
            authored_main && row == 6 ? panel_y + 391 : rows_y + std::int32_t(row) * row_height,
            row_width, authored_main ? 45 : row_height - 3});
    for (std::size_t row = 0; row < fixed_rows.size(); ++row)
        add_button(fixed_rows[row], {(width - row_width) / 2, fixed_y + std::int32_t(row) * row_height,
            row_width, row_height - 3});
    const bool show_back = !pending_ && (history_.size() > 1 || choice_field_);
    const auto footer_width = std::max(1, panel_width - 48);
    const auto counter_width = std::min(150, show_back ? std::max(1, footer_width / 2) : footer_width);
    const auto back_width = std::min(160, std::max(1, footer_width -
        (rows.size() > count ? counter_width + 8 : 0)));
    if (show_back)
        add_button({"back", text("TXT_KEY_MODDING_MENU_BACK"), ready()},
                   {panel_x + 24, std::min(footer_y, height - 40), back_width, 32});
    if (rows.size() > count) plan.paints.push_back(label(
        {panel_x + panel_width - 24 - counter_width, std::min(footer_y, height - 40), counter_width, 28},
        std::to_string(plan.first_row + (plan.visible_rows ? 1 : 0)) + "-" +
        std::to_string(plan.first_row + plan.visible_rows) + "/" + std::to_string(rows.size())));
    if (!handed_off_ && !executing_) {
        for (std::size_t row = 0; row < rows.size(); ++row)
            if (is_ready(rows[row].state)) plan.keyboard_order.push_back({rows[row].id, row});
        for (const auto &row : fixed_rows)
            if (is_ready(row.state)) plan.keyboard_order.push_back({row.id, {}});
        if (show_back) plan.keyboard_order.push_back({"back", {}});
    }
    const auto status = !hovered_reason.empty() ? hovered_reason : !message_.empty() ? message_ :
        !presentation_reason.empty() ? presentation_reason :
        rows.empty() ? std::string("Existing data/presenter owner unavailable") : std::string{};
    if (!status.empty()) plan.paints.push_back(label({16, height - 26, std::max(1, width - 32), 24}, text(status)));
    for (auto &paint : plan.paints) if (!paint.clip) paint.clip = viewport;
    return plan;
}
} // namespace civ5::ui
