#include "civ5/ui/frontend_existing.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::ui {
namespace {
struct FocusSearch {};
constexpr std::string_view input_source = "CivisFrontEndCivilopedia";
std::string action_control(const SystemPediaAction &action) {
    switch (action.kind) {
    case SystemPediaActionKind::select_article: return "PediaEntry:" + action.article_id;
    case SystemPediaActionKind::follow_link: return "PediaLink:" + action.article_id;
    case SystemPediaActionKind::previous_page: return "PediaPrevious";
    case SystemPediaActionKind::next_page: return "PediaNext";
    case SystemPediaActionKind::search: return "PediaSearch:" + action.article_id;
    case SystemPediaActionKind::submit_search: return "SearchButton";
    case SystemPediaActionKind::history_back: return "BackButton";
    case SystemPediaActionKind::history_forward: return "ForwardButton";
    case SystemPediaActionKind::article_scroll_up: return "PediaArticleUp";
    case SystemPediaActionKind::article_scroll_down: return "PediaArticleDown";
    case SystemPediaActionKind::select_category: return "CategoryButton" + action.article_id;
    }
    return {};
}
void append_utf8(std::string &text, std::uint32_t value) {
    if (value < 0x80) text.push_back(static_cast<char>(value));
    else if (value < 0x800) {
        text.push_back(static_cast<char>(0xc0 | (value >> 6)));
        text.push_back(static_cast<char>(0x80 | (value & 0x3f)));
    } else if (value < 0x10000) {
        text.push_back(static_cast<char>(0xe0 | (value >> 12)));
        text.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3f)));
        text.push_back(static_cast<char>(0x80 | (value & 0x3f)));
    } else {
        text.push_back(static_cast<char>(0xf0 | (value >> 18)));
        text.push_back(static_cast<char>(0x80 | ((value >> 12) & 0x3f)));
        text.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3f)));
        text.push_back(static_cast<char>(0x80 | (value & 0x3f)));
    }
}
} // namespace

FrontendExistingScreens::FrontendExistingScreens(const assets::ResourceMountTable &resources,
                                                 FrontendLocalizer localize)
    : localize_(std::move(localize)) {
    try {
        auto registry = UiContextRegistry::from_resources(resources);
        const auto sources = read_system_sources(resources, registry);
        const auto source = std::find_if(sources.begin(), sources.end(), [](const auto &value) {
            return value.context_id == "CivilopediaScreen" && value.source_available;
        });
        if (source == sources.end()) throw std::runtime_error("Selected Civilopedia source unavailable");
        pedia_source_ = *source;
        pedia_data_.pedia_entries = read_civilopedia_entries(resources, localize_);
        if (pedia_data_.pedia_entries.empty()) throw std::runtime_error("Civilopedia article data is unavailable");
        pedia_controller_.emplace(std::move(registry));
        pedia_frame_ = snapshot(1280, 800);
        const auto opened = pedia_controller_->dispatch(UiOpenScreen{"CivilopediaScreen"}, *pedia_frame_);
        if (opened.status != UiCommandStatus::navigation) throw std::runtime_error(opened.reason);
    } catch (const std::exception &error) {
        blocker_ = error.what();
        pedia_controller_.reset(); pedia_frame_.reset();
    }
}

UiFrameSnapshot FrontendExistingScreens::snapshot(std::int32_t width, std::int32_t height) {
    pedia_data_.pedia_article_layout = civilopedia_article_layout(width, height);
    UiFrameSnapshot base(++frame_, std::nullopt, std::nullopt);
    return pedia_session_.with_pedia_state(base.with<SystemScreenFrame>(
        std::make_shared<const SystemScreenFrame>(pedia_data_)));
}

std::optional<FrontendMenuPlan> FrontendExistingScreens::present(const FrontendScreen &screen,
    const FrontendDraft &, std::int32_t width, std::int32_t height) {
    drawable_pedia_ = false;
    if (screen.id != "pedia") { high_surrogate_.reset(); return std::nullopt; }
    FrontendMenuPlan result;
    if (!blocker_.empty() || !pedia_source_ || !pedia_controller_) {
        result.unresolved.push_back(blocker_.empty() ? "Civilopedia owner unavailable" : blocker_);
        WorldViewPaint label;
        label.kind = WorldViewPaintKind::text;
        label.destination = {24, 48, std::max(1, width - 48), 40};
        label.style = "TwCenMT20"; label.text = result.unresolved.front();
        if (localize_) if (const auto text = localize_(label.text)) label.text = *text;
        result.paints.push_back(std::move(label));
        return result;
    }
    // Retain the original screen owner's geometry. Its fixed 16-category row
    // and article columns do not fit a compact viewport.
    if (width < 840 || height < 480) {
        high_surrogate_.reset();
        WorldViewPaint label;
        label.kind = WorldViewPaintKind::text;
        label.destination = {24, 48, std::max(1, width - 48), std::max(1, std::min(80, height - 48))};
        label.style = "TwCenMT16";
        label.text = "Enlarge the window\nto view the Civilopedia";
        if (localize_) if (const auto translated = localize_("Enlarge the window to view the Civilopedia"))
            label.text = *translated;
        label.clip = label.destination;
        result.paints.push_back(std::move(label));
        return result; // The frontend supplies its own retained Back route.
    }
    drawable_pedia_ = true;
    pedia_frame_ = snapshot(width, height);
    const auto plan = plan_system_screen(*pedia_source_, *pedia_frame_, width, height,
                                        localize_, &pedia_backend_);
    result.paints = plan.paints; result.unresolved = plan.unresolved;
    for (const auto &hit : plan.hits) {
        if (const auto *close = std::get_if<UiCloseScreen>(&hit.command);
            close && close->id == "CivilopediaScreen")
            result.hits.push_back({hit.bounds, "back", hit.enabled, hit.unavailable_reason, {}});
        else result.hits.push_back({hit.bounds, {}, hit.enabled, hit.unavailable_reason, hit.command});
    }
    if (plan.search_edit_box) result.hits.push_back({*plan.search_edit_box, {}, true, {},
        UiGameplayCommand::make(std::string(input_source), FocusSearch{})});
    return result;
}

UiCommandResult FrontendExistingScreens::dispatch(const FrontendScreen &screen,
    const FrontendDraft &, const UiCommand &command) {
    if (screen.id != "pedia" || !drawable_pedia_ || !pedia_frame_ || !pedia_controller_ ||
        pedia_controller_->focused_screen() != "CivilopediaScreen")
        return {UiCommandStatus::unavailable, blocker_.empty() ? "Existing screen owner unavailable" : blocker_};
    const auto *gameplay = std::get_if<UiGameplayCommand>(&command);
    if (!gameplay) return {UiCommandStatus::invalid, "Not a Civilopedia action"};
    if (gameplay->source_id() == input_source && gameplay->as<FocusSearch>()) {
        if (!search_focused_) search_input_ = pedia_session_.pedia_search();
        search_focused_ = true; high_surrogate_.reset();
        return {UiCommandStatus::performed, {}};
    }
    const auto *action = gameplay->as<SystemPediaAction>();
    if (gameplay->source_id() != "CivilopediaScreen" || !action)
        return {UiCommandStatus::invalid, "Not a Civilopedia action"};
    if (action->kind != SystemPediaActionKind::search && action->kind != SystemPediaActionKind::submit_search) {
        search_focused_ = false; high_surrogate_.reset();
    }
    const auto control = action_control(*action);
    if (control.empty()) return {UiCommandStatus::invalid, "Unknown Civilopedia action"};
    const auto result = pedia_session_.activate("CivilopediaScreen", control, *pedia_frame_,
                                              *pedia_controller_, &pedia_backend_);
    if (result.status == UiCommandStatus::performed && action->kind == SystemPediaActionKind::search)
        search_input_ = action->article_id;
    pedia_frame_ = pedia_session_.with_pedia_state(*pedia_frame_);
    return result;
}

std::optional<UiCommand> FrontendExistingScreens::character(std::uint32_t unit) {
    if (!text_focused() || unit < 32 || unit == 127 || unit > 0xffff) {
        high_surrogate_.reset(); return std::nullopt;
    }
    if (unit >= 0xd800 && unit <= 0xdbff) { high_surrogate_ = unit; return std::nullopt; }
    std::uint32_t value = unit;
    if (unit >= 0xdc00 && unit <= 0xdfff) {
        if (!high_surrogate_) return std::nullopt;
        value = 0x10000 + ((*high_surrogate_ - 0xd800) << 10) + unit - 0xdc00;
    }
    high_surrogate_.reset();
    auto query = search_input_;
    const auto characters = std::count_if(query.begin(), query.end(), [](unsigned char ch) {
        return (ch & 0xc0) != 0x80;
    });
    if (characters >= 50) return std::nullopt;
    append_utf8(query, value);
    return civilopedia_search_command(query);
}

std::optional<UiCommand> FrontendExistingScreens::key_down(std::uint32_t key) {
    if (!text_focused()) { high_surrogate_.reset(); return std::nullopt; }
    if (key == 0x0d || key == 0x08) high_surrogate_.reset();
    if (key == 0x0d) return UiGameplayCommand::make("CivilopediaScreen",
        SystemPediaAction{SystemPediaActionKind::submit_search, {}});
    if (key != 0x08) return std::nullopt;
    auto query = search_input_;
    if (query.empty()) return std::nullopt;
    auto begin = query.size() - 1;
    while (begin && (static_cast<unsigned char>(query[begin]) & 0xc0) == 0x80) --begin;
    query.resize(begin);
    return civilopedia_search_command(query);
}

std::optional<UiCommand> FrontendExistingScreens::wheel(std::int32_t delta) const {
    if (!drawable_pedia_ || !pedia_frame_ || !pedia_controller_ ||
        pedia_controller_->focused_screen() != "CivilopediaScreen" || !delta) return std::nullopt;
    return UiGameplayCommand::make("CivilopediaScreen", SystemPediaAction{
        delta > 0 ? SystemPediaActionKind::article_scroll_up : SystemPediaActionKind::article_scroll_down, {}});
}

std::vector<std::string> FrontendExistingScreens::font_texts() const {
    std::vector<std::string> result;
    for (const auto &entry : pedia_data_.pedia_entries) {
        result.push_back(entry.title); result.push_back(entry.description);
    }
    return result;
}
} // namespace civ5::ui
