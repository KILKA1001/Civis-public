#pragma once

#include "civ5/ui/frontend_sources.hpp"
#include "civ5/ui/world_view.hpp"
#include <functional>
#include <map>

namespace civ5::ui {

using FrontendValue = std::variant<bool, std::int64_t, std::string>;
using FrontendLocalizer = std::function<std::optional<std::string>(std::string_view)>;

struct FrontendScreen {
    std::string id, title_key, source_key;
    // Existing complex screens (pedia/replays/etc.) are rendered by their owner.
    bool external = false;
};
struct FrontendItem {
    std::string id, screen, text_key;
    std::int32_t order = 0;
    std::string source_key, control_id;
    std::string target_screen, operation;
    bool confirmation = false;
    std::map<std::string, FrontendValue, std::less<>> draft_updates{};
};
class FrontendRegistry {
  public:
    static FrontendRegistry original();
    void add_screen(FrontendScreen screen);
    void add_item(FrontendItem item);
    const FrontendScreen *screen(std::string_view id) const;
    const FrontendItem *item(std::string_view id) const;
    std::vector<const FrontendItem *> items(std::string_view screen) const;
    const std::vector<FrontendScreen> &screens() const { return screens_; }
    const std::vector<FrontendItem> &items() const { return items_; }
  private:
    std::vector<FrontendScreen> screens_;
    std::vector<FrontendItem> items_;
};

struct FrontendPackage {
    std::string key, name;
    bool mod = false;
    std::uint64_t version = 0;
    UiCommandResult availability{UiCommandStatus::unavailable, "Content owner unavailable"};
    // Owner-provided (kind, canonical ID), independent of source/version.
    // Empty metadata never authorizes guessing which rows are alternatives.
    std::string selection_identity, source_detail;
    // Compact source/skin caption; the full source stays in source_detail/key.
    std::string source_label;
};
struct FrontendSave {
    std::string id, name, detail;
    std::filesystem::path path;
    // Only the save owner may mark a parsed, current slot usable.
    UiCommandResult availability{UiCommandStatus::unavailable, "Save owner unavailable"};
};
struct FrontendChoice {
    std::string label;
    FrontendValue value;
};
enum class FrontendFieldKind { choice, toggle, integer, text };
struct FrontendField {
    std::string id, screen, text_key;
    FrontendFieldKind kind = FrontendFieldKind::choice;
    std::vector<FrontendChoice> choices;
    std::optional<FrontendValue> initial;
    std::int64_t minimum = 0, maximum = 0;
    std::size_t maximum_text_bytes = 4096;
    UiCommandResult availability{UiCommandStatus::unavailable, "Value owner unavailable"};
    // One setting identity/value may be edited on both basic and advanced screens.
    std::vector<std::string> additional_screens;
    // Bind differently named basic/advanced widgets to this single value.
    // A supplied field replaces the corresponding source fallback/selector.
    std::map<std::string, std::string, std::less<>> controls{};
};
struct FrontendData {
    std::vector<FrontendPackage> packages;
    std::vector<FrontendSave> saves;
    std::vector<FrontendField> fields;
};
struct FrontendDraft {
    // Shared by basic/advanced setup and retained when returning to MainMenu.
    std::map<std::string, FrontendValue, std::less<>> values;
    std::vector<std::string> packages;
    std::optional<std::string> save;
};
struct FrontendRequest {
    std::string operation, source_key, control_id;
    FrontendDraft draft;
    std::optional<FrontendSave> save;
};
struct FrontendContentCheck {
    UiCommandResult result{UiCommandStatus::unavailable, "Content preflight unavailable"};
    std::vector<std::string> required_packages;
    // Cheap selection metadata may not expose container requirements yet.
    // An execution admission must always publish verified requirements.
    bool requirements_known = true;
};

// Readiness here is the application's readiness, not a graphic preview receipt.
// Implementations reuse existing save/settings/network/game owners; menu code
// neither initializes worlds nor treats successful preflight as game readiness.
class FrontendBackend {
  public:
    virtual ~FrontendBackend() = default;
    virtual UiCommandResult availability(const FrontendRequest &) const = 0;
    virtual UiCommandResult execute(const FrontendRequest &) = 0;
    // Presentation queries should use retained metadata/selection only. The
    // default preserves older backends; production owners override this to
    // avoid decoding a save while painting. Execution always checks content.
    virtual FrontendContentCheck check_selection(const FrontendRequest &) const;
    virtual FrontendContentCheck check_content(const FrontendRequest &) const;
};

struct FrontendActivate { std::string item; };
struct FrontendHit {
    WorldViewRect bounds;
    std::string id;
    bool enabled = false;
    std::string unavailable_reason;
    // Existing screen owners retain their typed commands, without inventing
    // registry entries for every article, replay row or settings widget.
    std::optional<UiCommand> command;
};
// UI navigation metadata only. Offscreen rows can be brought into view by Tab;
// activation still requires an enabled hit in a current actually presented plan.
struct FrontendKeyboardTarget {
    std::string id;
    std::optional<std::size_t> row;
};
struct FrontendMenuPlan {
    std::vector<WorldViewPaint> paints;
    std::vector<FrontendHit> hits;
    std::vector<std::string> unresolved;
    std::string screen;
    std::uint64_t revision = 0;
    const void *owner = nullptr;
    std::size_t first_row = 0, total_rows = 0, visible_rows = 0;
    std::vector<FrontendKeyboardTarget> keyboard_order;
    const FrontendHit *hit(std::int32_t x, std::int32_t y) const;
};
using FrontendExternalPresenter = std::function<std::optional<FrontendMenuPlan>(
    const FrontendScreen &, const FrontendDraft &, std::int32_t, std::int32_t)>;
using FrontendExternalDispatcher = std::function<UiCommandResult(
    const FrontendScreen &, const FrontendDraft &, const UiCommand &)>;

class FrontendMenuSession {
  public:
    FrontendMenuSession(FrontendRegistry registry, FrontendSourceCatalog sources,
                        FrontendBackend *backend = nullptr,
                        FrontendLocalizer localize = {});
    FrontendMenuSession(const FrontendMenuSession &) = delete;
    FrontendMenuSession &operator=(const FrontendMenuSession &) = delete;
    const std::string &screen() const { return history_.back(); }
    const FrontendDraft &draft() const { return draft_; }
    const FrontendRegistry &registry() const { return registry_; }
    const FrontendSourceCatalog &sources() const { return sources_; }
    const FrontendData &data() const { return data_; }
    const std::string &message() const { return message_; }
    bool handed_off() const { return handed_off_; }
    bool confirming() const { return pending_.has_value(); }
    std::uint64_t revision() const { return revision_; }
    void refresh(FrontendData data);
    void set_external_presenter(FrontendExternalPresenter presenter);
    void set_external_dispatcher(FrontendExternalDispatcher dispatcher);
    UiCommandResult activate(std::string_view id);
    UiCommandResult request_exit();
    UiCommandResult back();
    UiCommandResult set_value(std::string_view id, FrontendValue value);
    UiCommandResult select_save(std::string_view id);
    UiCommandResult toggle_package(std::string_view key);
    UiCommandResult select_saved_content();
    UiCommandResult availability(std::string_view id) const;
    FrontendMenuPlan compose(std::int32_t width, std::int32_t height,
                            std::optional<WorldViewPoint> pointer = {}) const;
    UiCommandResult click(const FrontendMenuPlan &, std::int32_t press_x,
        std::int32_t press_y, std::int32_t release_x, std::int32_t release_y);
    // Host queues genuine keys and calls these after native DispatchMessage.
    UiCommandResult focus_next(const FrontendMenuPlan &, bool backwards = false);
    const FrontendHit *focused_hit(const FrontendMenuPlan &) const;
    UiCommandResult activate_focused(const FrontendMenuPlan &);
    void scroll(std::int32_t rows);
    UiCommandResult dispatch(const UiCommand &command);

  private:
    FrontendRegistry registry_;
    FrontendSourceCatalog sources_;
    FrontendBackend *backend_;
    FrontendLocalizer localize_;
    FrontendExternalPresenter external_;
    FrontendExternalDispatcher external_dispatch_;
    FrontendData data_;
    FrontendDraft draft_;
    std::vector<std::string> history_{"main"};
    std::map<std::string, std::size_t, std::less<>> scroll_;
    std::optional<FrontendRequest> pending_;
    std::optional<std::string> choice_field_;
    std::optional<std::string> keyboard_focus_;
    std::string message_;
    std::uint64_t revision_ = 1;
    bool handed_off_ = false, executing_ = false;
    mutable bool querying_ = false;
    mutable bool composing_ = false;
    std::string text(std::string_view key) const;
    const FrontendField *field(std::string_view id) const;
    std::optional<FrontendRequest> request(const FrontendItem &) const;
    UiCommandResult request_availability(const FrontendRequest &, bool executing = false) const;
    UiCommandResult perform(FrontendRequest request);
    UiCommandResult report(UiCommandResult result);
};

} // namespace civ5::ui
