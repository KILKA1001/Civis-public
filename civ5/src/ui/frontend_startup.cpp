#include "civ5/ui/frontend_startup.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::ui {
namespace {
WorldViewPaint box(WorldViewRect r, std::string style, std::uint32_t rgba) {
    WorldViewPaint p; p.destination = r; p.style = std::move(style); p.rgba = rgba; return p;
}
WorldViewPaint label(WorldViewRect r, std::string value, bool centered = false) {
    WorldViewPaint p; p.kind = WorldViewPaintKind::text; p.destination = r;
    p.text = std::move(value); p.style = centered ? "TwCenMT20:center" : "TwCenMT16";
    p.clip = r; p.rgba = 0xeae3caff; return p;
}
WorldViewPaint image(WorldViewRect r, std::string key, WorldViewRect clip) {
    WorldViewPaint p; p.kind = WorldViewPaintKind::texture; p.destination = r;
    p.source_key = std::move(key); p.clip = clip; return p;
}
const content::XmlNode *first(const content::XmlNode &node, std::string_view name) {
    for (const auto &child : node.children) {
        if (child.name == name) return &child;
        if (const auto *found = first(child, name)) return found;
    }
    return nullptr;
}
std::string attribute(const content::XmlNode &node, std::string_view key) {
    return node.attribute(std::string(key));
}
} // namespace

FrontendStartupSkin FrontendStartupSkin::from_sources(const FrontendSourceCatalog &sources) {
    const auto *front = sources.find("FrontEnd.xml"), *legal = sources.find("LegalScreen.xml");
    if (!front || !legal || !legal->find_control("ContinueButton") ||
        front->lua_text.find("CivilzationVAtlas.dds") == std::string::npos)
        throw std::runtime_error("Original startup FrontEnd/Legal sources are unavailable");
    FrontendStartupSkin result;
    result.background = "CivilzationVAtlas.dds";
    result.selected_legal_source = legal->selected_source_path;
    if (const auto *logo = legal->find_control("Civ5Logo")) result.logo = logo->texture;
    if (const auto *grid = first(legal->document, "Grid")) result.panel_style = attribute(*grid, "Style");
    if (const auto *body = legal->find_control("EULAText")) result.legal_body_key = body->text_key;
    // These ID-less authored Labels are retained in the XML document.
    const auto find_caption = [&](const auto &self, const content::XmlNode &node,
                                  std::string_view key) -> bool {
        if (node.name == "Label" && attribute(node, "String") == key) return true;
        for (const auto &child : node.children) if (self(self, child, key)) return true;
        return false;
    };
    if (find_caption(find_caption, legal->document, "TXT_KEY_ESRB_BODY"))
        result.rating_body_key = "TXT_KEY_ESRB_BODY";
    if (find_caption(find_caption, legal->document, "TXT_KEY_LEGAL_CONTINUE"))
        result.continue_key = "TXT_KEY_LEGAL_CONTINUE";
    if (result.legal_body_key.empty() || result.continue_key.empty())
        throw std::runtime_error("Original startup legal captions are unavailable");
    return result;
}

std::map<std::string, std::string, std::less<>> frontend_startup_captions(bool russian) {
    return {
        {"frontend.startup.title", russian ? "Загрузка Civis" : "Loading Civis"},
        {"frontend.startup.resources", russian ? "Подготовка ресурсов" : "Preparing resources"},
        {"frontend.startup.localization", russian ? "Загрузка языка и экранов" : "Loading language and screens"},
        {"frontend.startup.presentation", russian ? "Подготовка интерфейса" : "Preparing the interface"},
        {"frontend.startup.content", russian ? "Проверка установленного содержимого" : "Checking installed content"},
        {"frontend.startup.menu", russian ? "Подготовка главного меню" : "Preparing the main menu"},
        {"frontend.startup.ready", russian ? "Подготовка завершена" : "Preparation complete"},
        {"frontend.startup.failure", russian ? "Не удалось запустить Civis" : "Civis could not start"},
        {"frontend.startup.exit", russian ? "Выход" : "Exit"},
        {"frontend.startup.cancel", russian ? "Отмена запуска…" : "Cancelling startup…"},
        {"frontend.startup.legal", russian ? "Уведомления" : "Notices"},
        {"frontend.startup.continue", russian ? "Продолжить" : "Continue"},
        {"frontend.startup.scroll", russian ? "Прокрутите колесом мыши" : "Scroll with the mouse wheel"}
    };
}
FrontendStartupTextBlock layout_frontend_startup_text(const GlyphGenGeometry &font,
    std::string_view input, std::int32_t width) {
    if (width <= 0 || font.line_height <= 0) throw std::invalid_argument("Startup text layout has no geometry");
    std::string normalized(input);
    for (std::size_t at = 0; (at = normalized.find("[NEWLINE]", at)) != std::string::npos; ++at)
        normalized.replace(at, 9, "\n");
    FrontendStartupTextBlock result{{{}}, font.line_height};
    std::string word;
    const auto append_word = [&] {
        if (word.empty()) return;
        auto &line = result.lines.back();
        const auto joined = line.empty() ? word : line + " " + word;
        if (!line.empty() && project_glyphgen_line(font, joined).advance_width > width)
            result.lines.emplace_back();
        // Very narrow windows and long tokens break only at UTF-8 boundaries.
        for (std::size_t at = 0; at < word.size();) {
            auto end = at + 1;
            while (end < word.size() && (static_cast<unsigned char>(word[end]) & 0xc0) == 0x80) ++end;
            const auto glyph = word.substr(at, end - at);
            auto &destination = result.lines.back();
            const auto prefix = at == 0 && !destination.empty() ? " " : "";
            if (!destination.empty() && project_glyphgen_line(font, destination + prefix + glyph).advance_width > width)
                result.lines.emplace_back();
            else result.lines.back() += prefix;
            result.lines.back() += glyph;
            // Validate even a single wider-than-viewport glyph as selected UTF-8.
            (void)project_glyphgen_line(font, result.lines.back());
            at = end;
        }
        word.clear();
    };
    for (char c : normalized) {
        if (c == '\n' || c == ' ' || c == '\r' || c == '\t') {
            append_word(); if (c == '\n') result.lines.emplace_back();
        } else word += c;
    }
    append_word(); return result;
}
FrontendStartupSession::FrontendStartupSession(std::vector<runtime::StartupLoadingStage> stages,
    FrontendLocalizer localize, runtime::StartupLoading::Observer observer, bool legal_already_shown)
    : loading_(std::move(stages), std::move(observer)), localize_(std::move(localize)),
      legal_shown_(legal_already_shown) {}
void FrontendStartupSession::require_mutation_allowed() const {
    (void)loading_.snapshot(); // Keep all presentation mutations on the owning UI thread.
    if (composing_) throw std::logic_error("Startup presentation callback reentry");
}
runtime::StartupLoadingPhase FrontendStartupSession::advance() {
    require_mutation_allowed(); return loading_.advance();
}
void FrontendStartupSession::set_skin(FrontendStartupSkin skin) {
    require_mutation_allowed();
    skin_ = std::move(skin); ++revision_;
}
void FrontendStartupSession::set_text_layout(FrontendStartupTextLayout layout) {
    require_mutation_allowed();
    text_layout_ = std::move(layout); ++revision_;
}
void FrontendStartupSession::scroll(std::int32_t rows) {
    require_mutation_allowed();
    const auto next = std::clamp<std::int64_t>(static_cast<std::int64_t>(scroll_) + rows, 0,
        static_cast<std::int64_t>(maximum_scroll_));
    if (scroll_ != static_cast<std::size_t>(next)) { scroll_ = static_cast<std::size_t>(next); ++revision_; }
}
void FrontendStartupSession::request_exit() {
    require_mutation_allowed();
    if (!exit_requested_) { exit_requested_ = true; ++revision_; }
    loading_.request_cancel();
}
FrontendStartupPhase FrontendStartupSession::phase() const {
    const auto value = loading_.snapshot().phase;
    if (exit_requested_ && (value == runtime::StartupLoadingPhase::completed ||
                            value == runtime::StartupLoadingPhase::failed)) return FrontendStartupPhase::cancelled;
    if (value == runtime::StartupLoadingPhase::failed) return FrontendStartupPhase::failed;
    // A close during a running callback is only committed when that callback
    // returns to its safe boundary; do not destroy its resources from a WndProc.
    if (value == runtime::StartupLoadingPhase::cancelled ||
        (exit_requested_ && value == runtime::StartupLoadingPhase::completed)) return FrontendStartupPhase::cancelled;
    if (value == runtime::StartupLoadingPhase::completed)
        return legal_shown_ ? FrontendStartupPhase::menu : FrontendStartupPhase::legal;
    return FrontendStartupPhase::loading;
}
std::string FrontendStartupSession::text(std::string_view key) const {
    if (localize_) if (const auto value = localize_(key)) return *value;
    const auto captions = frontend_startup_captions(false);
    const auto found = captions.find(key);
    return found == captions.end() ? std::string(key) : found->second;
}
FrontendMenuPlan FrontendStartupSession::compose(std::int32_t width, std::int32_t height,
    std::optional<WorldViewPoint> pointer) const {
    if (width < 1 || height < 1) throw std::invalid_argument("Startup viewport is empty");
    const auto value = loading_.snapshot();
    if (composing_) throw std::logic_error("Startup compose reentry");
    composing_ = true;
    struct Reset { bool &flag; ~Reset() { flag = false; } } reset{composing_};
    const auto screen = phase();
    FrontendMenuPlan plan;
    plan.owner = this; plan.revision = value.revision + revision_;
    plan.screen = screen == FrontendStartupPhase::legal ? "startup_legal" : "startup_loading";
    const WorldViewRect viewport{0, 0, width, height};
    plan.paints.push_back(box(viewport, {}, 0x000000ff));
    if (!skin_.background.empty())
        plan.paints.push_back(image({(width - 1920) / 2, (height - 1200) / 2, 1920, 1200}, skin_.background, viewport));
    if (!skin_.logo.empty() && height >= 480) {
        const auto w = std::min(630, width);
        plan.paints.push_back(image({width - w, 0, w, w * 200 / 630}, skin_.logo, viewport));
    }
    const auto panel_w = std::min(836, std::max(1, width - 24));
    const auto panel_h = std::min(504, std::max(1, height - 24));
    const auto x = (width - panel_w) / 2, y = (height - panel_h) / 2;
    plan.paints.push_back(box({x, y, panel_w, panel_h}, skin_.panel_style, 0xffffffff));
    // A bootstrap with no selected Grid9 yet uses an opaque, readable rectangle.
    if (skin_.panel_style.empty()) plan.paints.back().rgba = 0x101b2bf0;
    const auto inset = std::min(28, std::max(1, panel_w / 8));
    const auto content_w = std::max(1, panel_w - 2 * inset);
    const auto title_y = y + std::min(18, std::max(0, panel_h - 30));
    plan.paints.push_back(label({x + inset, title_y, content_w, std::min(30, panel_h)},
        text(screen == FrontendStartupPhase::legal ? "frontend.startup.legal" :
             screen == FrontendStartupPhase::failed ? "frontend.startup.failure" : "frontend.startup.title"), true));
    const WorldViewRect action{x + std::max(0, (panel_w - std::min(300, content_w)) / 2),
        y + std::max(0, panel_h - 52), std::min(300, content_w), std::min(32, panel_h)};
    const auto body = [&](WorldViewRect bounds, std::string value) {
        if (!text_layout_) {
            plan.unresolved.push_back("Startup text needs the selected font layout adapter");
            plan.paints.push_back(label(bounds, std::move(value))); return;
        }
        const auto block = text_layout_(value, bounds.width);
        if (block.line_height <= 0) throw std::runtime_error("Startup text layout returned an invalid line height");
        const auto rows = std::max<std::size_t>(1, bounds.height / block.line_height);
        maximum_scroll_ = block.lines.size() > rows ? block.lines.size() - rows : 0;
        const auto offset = std::min(scroll_, maximum_scroll_);
        std::string visible;
        for (auto row = offset; row < block.lines.size() && row - offset < rows; ++row) {
            if (row != offset) visible += '\n'; visible += block.lines[row];
        }
        plan.first_row = offset; plan.total_rows = block.lines.size(); plan.visible_rows = rows;
        plan.paints.push_back(label(bounds, std::move(visible)));
        if (maximum_scroll_)
            plan.paints.push_back(label({bounds.x, action.y - 22, bounds.width, 20}, text("frontend.startup.scroll"), true));
    };
    if (screen == FrontendStartupPhase::legal) {
        if (skin_.legal_body_key.empty() || skin_.continue_key.empty()) {
            plan.unresolved.push_back("Startup legal source has not been selected");
            plan.paints.push_back(label({x + inset, title_y + 34, content_w, std::max(1, panel_h - 130)},
                text("frontend.startup.failure")));
        } else {
            // Compact adaptation keeps the entire selected notice/rating
            // reachable through the same measured wrap and scroll adapter.
            auto notice = text(skin_.legal_body_key);
            if (!skin_.rating_body_key.empty()) notice += "\n\n" + text(skin_.rating_body_key);
            body({x + inset, title_y + 34, content_w, std::max(1, panel_h - 130)}, std::move(notice));
            plan.hits.push_back({action, "startup_continue", bool(text_layout_),
                text_layout_ ? "" : "Startup text layout is unavailable", {}});
            plan.paints.push_back(label(action, text(width < 600 ? "frontend.startup.continue" : skin_.continue_key), true));
        }
    } else if (screen == FrontendStartupPhase::loading) {
        const auto *stage = value.current();
        const auto body_y = title_y + 42;
        if (stage) {
            plan.paints.push_back(label({x + inset, body_y, content_w, 32}, text(stage->title_key), true));
            if (!stage->detail.empty() && panel_h >= 300)
                plan.paints.push_back(label({x + inset, body_y + 38, content_w, 52}, text(stage->detail)));
            if (stage->total_units) {
                const auto counts = std::to_string(stage->completed_units) + " / " + std::to_string(*stage->total_units);
                plan.paints.push_back(label({x + inset, body_y + (panel_h >= 300 ? 98 : 38), content_w, 30}, counts, true));
                if (const auto fraction = stage->fraction(); fraction && panel_h >= 260) {
                    const WorldViewRect bar{x + inset, body_y + 136, content_w, 10};
                    plan.paints.push_back(box(bar, {}, 0x000000c8));
                    auto fill = bar; fill.width = static_cast<std::int32_t>(bar.width * *fraction);
                    if (fill.width > 0) plan.paints.push_back(box(fill, {}, 0xbdab73ff));
                }
            }
        }
        // Stage count and elapsed time are diagnostic data in snapshot(), not
        // a time percentage or extra technical text in the product screen.
        plan.paints.push_back(label(action, text(exit_requested_ ? "frontend.startup.cancel" : "frontend.startup.exit"), true));
        if (!exit_requested_) plan.hits.push_back({action, "startup_exit", true, {}, {}});
    } else if (screen == FrontendStartupPhase::failed) {
        body({x + inset, title_y + 42, content_w, std::max(1, panel_h - 130)}, text(value.error));
        plan.hits.push_back({action, "startup_exit", true, {}, {}});
        plan.paints.push_back(label(action, text("frontend.startup.exit"), true));
    }
    if (pointer) if (const auto *hit = plan.hit(pointer->x, pointer->y); hit && hit->enabled)
        plan.paints.insert(plan.paints.end() - 1, box(hit->bounds, "Grid9FrameTurnsHL", 0xffffffff));
    return plan;
}
UiCommandResult FrontendStartupSession::click(const FrontendMenuPlan &plan, std::int32_t dx,
    std::int32_t dy, std::int32_t ux, std::int32_t uy) {
    require_mutation_allowed();
    const auto revision = loading_.snapshot().revision + revision_;
    if (plan.owner != this || plan.revision != revision)
        return {UiCommandStatus::invalid, "Stale startup frame"};
    const auto *down = plan.hit(dx, dy), *up = plan.hit(ux, uy);
    if (!down || !up || down->id != up->id || !down->enabled || !up->enabled)
        return {UiCommandStatus::invalid, "Startup press/release do not match"};
    if (up->id == "startup_continue" && phase() == FrontendStartupPhase::legal) {
        legal_shown_ = true; ++revision_;
        return {UiCommandStatus::navigation, {}};
    }
    if (up->id == "startup_exit") {
        request_exit(); return {UiCommandStatus::navigation, {}};
    }
    return {UiCommandStatus::invalid, "Startup action is unavailable"};
}
} // namespace civ5::ui
