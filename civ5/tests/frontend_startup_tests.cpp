#include "civ5/runtime/startup_loading.hpp"
#include "civ5/ui/frontend_startup.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

// CPU contracts only: no asset installation, native window, GL, game or Bank.
// This source was authored UNRUN; Root owns the compile/test invocation.
namespace {
using namespace civ5;
using namespace runtime;
using namespace ui;
unsigned checks = 0, cases = 0, failures = 0;
void need(bool condition, const char *message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
template<class Exception, class Function> void must_throw(Function &&function, const char *message) {
    bool caught = false;
    try { function(); } catch (const Exception &) { caught = true; }
    need(caught, message);
}
template<class Function> void run(const char *name, Function &&function) {
    ++cases;
    try { function(); }
    catch (const std::exception &error) { ++failures; std::cerr << name << ": " << error.what() << '\n'; }
    catch (...) { ++failures; std::cerr << name << ": unexpected nonstandard exception\n"; }
}
StartupLoadingStage completed_stage(std::string id, unsigned &calls) {
    return {std::move(id), "frontend.startup.resources", [&calls](const StartupStageProgress &) {
        ++calls; return StartupStepResult::completed;
    }};
}
struct CallbackScope {
    bool &active;
    explicit CallbackScope(bool &value) : active(value) { active = true; }
    ~CallbackScope() { active = false; }
};

void sequence_and_observers() {
    unsigned first_calls = 0, second_calls = 0;
    bool callback_active = false;
    std::vector<std::string> effects;
    std::vector<StartupLoadingEvent> events;
    std::vector<StartupLoadingSnapshot> observed;
    StartupLoading loading({
        {"resources", "frontend.startup.resources", [&](const StartupStageProgress &progress) {
            CallbackScope scope(callback_active);
            ++first_calls; effects.push_back(first_calls == 1 ? "scan-one" : "scan-two");
            progress.report(first_calls, 2, "actual resource record");
            return first_calls == 1 ? StartupStepResult::pending : StartupStepResult::completed;
        }},
        {"content", "frontend.startup.content", [&](const StartupStageProgress &progress) {
            CallbackScope scope(callback_active);
            ++second_calls; effects.push_back("content-return"); progress.report(7);
            return StartupStepResult::completed;
        }}
    }, [&](StartupLoadingEvent event, const StartupLoadingSnapshot &value) {
        need(value.current() != nullptr, "Observer lost its current stage");
        if (event == StartupLoadingEvent::started) {
            need(!callback_active && value.current()->phase == StartupStagePhase::started,
                "Started was published after entering work or without started state");
            need(value.current()->started_after.has_value(), "Started timestamp missing");
        }
        if (event == StartupLoadingEvent::progress)
            need(callback_active && !effects.empty(), "Progress preceded the actual callback effect");
        if (event == StartupLoadingEvent::completed)
            need(!callback_active && value.current()->phase == StartupStagePhase::completed,
                "Completion preceded callback unwind or actual stage completion");
        if (!observed.empty()) need(value.revision > observed.back().revision, "Observer revision did not advance");
        events.push_back(event); observed.push_back(value);
    });
    need(loading.snapshot().phase == StartupLoadingPhase::idle && effects.empty(), "Construction ran work");
    need(loading.advance() == StartupLoadingPhase::loading, "Pending callback completed the run");
    const auto pending = loading.snapshot();
    need(first_calls == 1 && second_calls == 0 && pending.current_stage == 0,
        "One tick executed more than its one pending step");
    need(pending.stages[0].phase == StartupStagePhase::started && !pending.stages[0].finished_after,
        "Pending stage acquired a completion receipt");
    need(pending.stages[0].fraction() && *pending.stages[0].fraction() == .5, "Actual counters lost their fraction");
    need(loading.advance() == StartupLoadingPhase::loading, "First completed stage finished a two-stage run");
    const auto between = loading.snapshot();
    need(first_calls == 2 && second_calls == 0 && between.current_stage == 1,
        "A returned stage executed the next stage in the same tick");
    need(between.stages[1].phase == StartupStagePhase::queued && !between.stages[1].started_after,
        "Next stage started before its actual callback tick");
    need(loading.advance() == StartupLoadingPhase::completed && second_calls == 1, "Final callback did not complete");
    const std::vector<StartupLoadingEvent> expected{StartupLoadingEvent::started, StartupLoadingEvent::progress,
        StartupLoadingEvent::progress, StartupLoadingEvent::completed, StartupLoadingEvent::started,
        StartupLoadingEvent::progress, StartupLoadingEvent::completed};
    need(events == expected, "Started/pending/progress/completed event order changed");
    need(observed.back().phase == StartupLoadingPhase::completed, "Final observer did not see terminal completion");
    const auto final = loading.snapshot();
    need(final.stages[1].completed_units == 7 && !final.stages[1].total_units && !final.stages[1].fraction(),
        "Unknown work was filled with a synthetic denominator or percentage");
    need(final.stages[0].finished_after >= final.stages[0].started_after, "Stage timestamp order went backwards");
    need(loading.advance() == StartupLoadingPhase::completed && first_calls == 2 && second_calls == 1,
        "Terminal advance repeated real work");
    need(loading.snapshot().elapsed == final.elapsed, "Terminal elapsed time kept changing");
}

void unknown_zero_and_late_total() {
    StartupLoading unknown({{"unknown", "caption", [](const StartupStageProgress &p) {
        p.report(3); return StartupStepResult::completed;
    }}});
    need(unknown.advance() == StartupLoadingPhase::completed && !unknown.snapshot().stages[0].fraction(),
        "Unknown denominator acquired a percentage");
    StartupLoading zero({{"zero", "caption", [](const StartupStageProgress &p) {
        p.report(0, 0); return StartupStepResult::completed;
    }}});
    need(zero.advance() == StartupLoadingPhase::completed && !zero.snapshot().stages[0].fraction(),
        "Zero denominator produced a percentage");
    unsigned ticks = 0;
    StartupLoading discovered({{"discovered", "caption", [&](const StartupStageProgress &p) {
        if (++ticks == 1) { p.report(2); p.report(2, 4); return StartupStepResult::pending; }
        p.report(4); return StartupStepResult::completed;
    }}});
    need(discovered.advance() == StartupLoadingPhase::loading, "Discovered total completed unfinished work");
    need(discovered.snapshot().stages[0].total_units == 4, "Real denominator discovery was refused");
    need(discovered.advance() == StartupLoadingPhase::completed &&
        discovered.snapshot().stages[0].fraction() == 1., "Omitting a later total erased the known denominator");
}

void invalid_counters_and_unfinished() {
    for (unsigned kind = 0; kind != 4; ++kind) {
        unsigned next_calls = 0, completed_events = 0, progress_events = 0;
        StartupLoading loading({
            {"bad", "caption", [kind](const StartupStageProgress &p) {
                p.report(2, 4);
                if (kind == 0) p.report(1, 4);
                if (kind == 1) p.report(2, 5);
                if (kind == 2) p.report(5, 4);
                if (kind == 3) p.report(5);
                return StartupStepResult::completed;
            }}, completed_stage("must-not-run", next_calls)
        }, [&](StartupLoadingEvent event, const StartupLoadingSnapshot &) {
            completed_events += event == StartupLoadingEvent::completed;
            progress_events += event == StartupLoadingEvent::progress;
        });
        need(loading.advance() == StartupLoadingPhase::failed, "Invalid progress was accepted");
        const auto value = loading.snapshot();
        need(value.stages[0].completed_units == 2 && value.stages[0].total_units == 4,
            "Rejected counters mutated the previous actual progress");
        need(value.stages[0].phase == StartupStagePhase::failed && !value.error.empty(), "Counter refusal lost failure state");
        need(progress_events == 1 && completed_events == 0 && next_calls == 0, "Refusal published completion or ran next work");
    }
    unsigned completed_events = 0;
    StartupLoading unfinished({{"unfinished", "caption", [](const StartupStageProgress &p) {
        p.report(3, 4); return StartupStepResult::completed;
    }}}, [&](StartupLoadingEvent event, const StartupLoadingSnapshot &) {
        completed_events += event == StartupLoadingEvent::completed;
    });
    need(unfinished.advance() == StartupLoadingPhase::failed, "Returned callback filled in unfinished units");
    need(unfinished.snapshot().stages[0].phase == StartupStagePhase::failed && completed_events == 0,
        "Unfinished work was observable as completed");
}

void first_error_survives_failure_observer() {
    unsigned effects = 0, failed_events = 0, next_calls = 0;
    StartupLoading loading({
        {"failure", "caption", [&](const StartupStageProgress &) -> StartupStepResult {
            ++effects; throw std::runtime_error("original resource failure");
        }}, completed_stage("later", next_calls)
    }, [&](StartupLoadingEvent event, const StartupLoadingSnapshot &value) {
        if (event == StartupLoadingEvent::failed) {
            ++failed_events;
            need(value.phase == StartupLoadingPhase::failed && value.error == "original resource failure",
                "Failure observer lost the first cause");
            throw std::runtime_error("secondary failure observer exception");
        }
    });
    need(loading.advance() == StartupLoadingPhase::failed, "Failure observer exception escaped advance");
    need(loading.snapshot().error == "original resource failure" && failed_events == 1,
        "Secondary observer failure replaced the original cause");
    need(loading.advance() == StartupLoadingPhase::failed && effects == 1 && next_calls == 0,
        "Failure retried effects or executed a later stage");
    StartupLoading nonstandard({{"throw", "caption", [](const StartupStageProgress &) -> StartupStepResult { throw 7; }}});
    need(nonstandard.advance() == StartupLoadingPhase::failed && !nonstandard.snapshot().error.empty(),
        "Nonstandard callback exception escaped without an observable failure");
}

void observer_failure_and_reentry() {
    StartupLoading *owner = nullptr;
    std::optional<StartupStageProgress> retained;
    unsigned rejected = 0, failed_events = 0;
    StartupLoading loading({{"stage", "caption", [&](const StartupStageProgress &p) {
        retained = p; p.report(1, 1); return StartupStepResult::completed;
    }}}, [&](StartupLoadingEvent event, const StartupLoadingSnapshot &) {
        if (event == StartupLoadingEvent::started || event == StartupLoadingEvent::progress) {
            try { owner->advance(); } catch (const std::logic_error &) { ++rejected; }
            if (event == StartupLoadingEvent::progress) {
                const auto revision = owner->snapshot().revision;
                try { retained->report(1, 1, "must-not-publish"); } catch (const std::logic_error &) { ++rejected; }
                need(owner->snapshot().revision == revision && owner->snapshot().current()->detail.empty(),
                    "Refused observer reporter still changed revision or detail");
                throw std::runtime_error("presentation observer failure");
            }
        }
        if (event == StartupLoadingEvent::failed) { ++failed_events; throw std::runtime_error("second failure"); }
    });
    owner = &loading;
    need(loading.advance() == StartupLoadingPhase::failed && rejected == 3,
        "Observer reentry was accepted or progress observer failure was ignored");
    need(loading.snapshot().error == "presentation observer failure" && failed_events == 1,
        "Failure observer replaced the first observer error");
}

void cancellation_at_safe_boundaries() {
    unsigned calls = 0;
    StartupLoading before({completed_stage("not-entered", calls)});
    before.request_cancel();
    need(before.snapshot().phase == StartupLoadingPhase::idle && before.snapshot().cancellation_requested,
        "Cancellation committed before the owner's safe advance boundary");
    need(before.advance() == StartupLoadingPhase::cancelled && calls == 0, "Pre-start cancellation ran work");
    need(before.snapshot().stages[0].phase == StartupStagePhase::cancelled &&
        !before.snapshot().stages[0].started_after, "Never-entered work acquired a started receipt");

    StartupLoading *started_owner = nullptr;
    StartupLoading started({completed_stage("not-entered", calls)}, [&](StartupLoadingEvent event, const StartupLoadingSnapshot &) {
        if (event == StartupLoadingEvent::started) started_owner->request_cancel();
    });
    started_owner = &started;
    need(started.advance() == StartupLoadingPhase::cancelled && calls == 0,
        "Cancellation from the initial presentation still entered the callback");

    StartupLoading *owner = nullptr;
    bool active = false, cancelled_during_work = false;
    unsigned later_calls = 0;
    StartupLoading loading({
        {"current", "caption", [&](const StartupStageProgress &p) {
            CallbackScope scope(active); ++calls; p.report(1, 2);
            need(p.cancellation_requested(), "Callback could not see cancellation requested by progress observer");
            need(owner->snapshot().phase == StartupLoadingPhase::loading,
                "Progress observer committed cancellation before callback returned");
            return StartupStepResult::pending;
        }}, completed_stage("later", later_calls)
    }, [&](StartupLoadingEvent event, const StartupLoadingSnapshot &) {
        if (event == StartupLoadingEvent::progress) owner->request_cancel();
        if (event == StartupLoadingEvent::cancelled) cancelled_during_work = active;
    });
    owner = &loading;
    need(loading.advance() == StartupLoadingPhase::cancelled && calls == 1 && later_calls == 0,
        "Safe-boundary cancellation lost real work order");
    need(!cancelled_during_work && loading.snapshot().stages[0].completed_units == 1,
        "Cancellation preceded unwind or fabricated completed units");
}

void cancellation_after_completed_stage() {
    unsigned first_calls = 0, later_calls = 0;
    StartupLoading *owner = nullptr;
    StartupLoading loading({completed_stage("first", first_calls), completed_stage("later", later_calls)},
        [&](StartupLoadingEvent event, const StartupLoadingSnapshot &) {
            if (event == StartupLoadingEvent::completed) owner->request_cancel();
        });
    owner = &loading;
    need(loading.advance() == StartupLoadingPhase::cancelled && first_calls == 1 && later_calls == 0,
        "Completed-stage observer cancellation started later work");
    need(loading.snapshot().stages[0].phase == StartupStagePhase::completed &&
        loading.snapshot().stages[1].phase == StartupStagePhase::queued,
        "Cancellation rewrote a genuine completed stage or entered future work");
}

void checkpoint_unwinds_only_at_safe_boundary() {
    unsigned no_cancel_calls = 0;
    StartupLoading normal({{"normal", "caption", [&](const StartupStageProgress &p) {
        p.checkpoint(); ++no_cancel_calls; return StartupStepResult::completed;
    }}});
    need(normal.advance() == StartupLoadingPhase::completed && no_cancel_calls == 1,
        "Checkpoint without cancellation skipped actual work");
    StartupLoading *owner = nullptr;
    bool active = false, cancelled_during_work = false;
    unsigned returned_operations = 0, after_checkpoint = 0, later_calls = 0, failed_events = 0;
    StartupLoading loading({
        {"checkpoint", "caption", [&](const StartupStageProgress &p) {
            CallbackScope scope(active);
            ++returned_operations; p.report(1, 3);
            need(owner->snapshot().phase == StartupLoadingPhase::loading,
                "Checkpoint cancellation committed before the safe checkpoint");
            p.checkpoint();
            ++after_checkpoint;
            return StartupStepResult::completed;
        }}, completed_stage("later", later_calls)
    }, [&](StartupLoadingEvent event, const StartupLoadingSnapshot &) {
        if (event == StartupLoadingEvent::progress) owner->request_cancel();
        if (event == StartupLoadingEvent::cancelled) cancelled_during_work = active;
        failed_events += event == StartupLoadingEvent::failed;
    });
    owner = &loading;
    need(loading.advance() == StartupLoadingPhase::cancelled && returned_operations == 1 &&
        after_checkpoint == 0 && later_calls == 0 && !cancelled_during_work,
        "Safe checkpoint failed to unwind or ran subsequent work");
    need(loading.snapshot().error.empty() && failed_events == 0 &&
        loading.snapshot().stages[0].completed_units == 1 && loading.snapshot().stages[0].total_units == 3,
        "Cancellation checkpoint became failure or filled unfinished counters");
}

void retained_and_foreign_progress() {
    std::optional<StartupStageProgress> stale, destroyed;
    unsigned calls = 0;
    bool foreign_report_refused = false, foreign_read_refused = false, foreign_owner_refused = false;
    StartupLoading *owner = nullptr;
    StartupLoading loading({{"pending", "caption", [&](const StartupStageProgress &p) {
        if (++calls == 1) {
            stale = p;
            std::thread foreign([&] {
                try { p.report(99, 100); } catch (const std::logic_error &) { foreign_report_refused = true; } catch (...) {}
                try { (void)p.cancellation_requested(); } catch (const std::logic_error &) { foreign_read_refused = true; } catch (...) {}
                try { (void)owner->snapshot(); } catch (const std::logic_error &) { foreign_owner_refused = true; } catch (...) {}
            });
            foreign.join(); p.report(1, 2); return StartupStepResult::pending;
        }
        must_throw<std::logic_error>([&] { stale->report(2, 2); }, "Old tick reporter wrote into a new callback");
        p.report(2, 2); return StartupStepResult::completed;
    }}});
    owner = &loading;
    need(loading.advance() == StartupLoadingPhase::loading && foreign_report_refused &&
        foreign_read_refused && foreign_owner_refused, "Foreign-thread progress/read reached the UI owner");
    const auto before = loading.snapshot().revision;
    must_throw<std::logic_error>([&] { stale->report(2, 2); }, "Returned callback reporter remained usable");
    must_throw<std::logic_error>([&] { (void)stale->cancellation_requested(); }, "Returned callback reporter remained readable");
    must_throw<std::logic_error>([&] { stale->checkpoint(); }, "Returned callback checkpoint remained usable");
    need(loading.snapshot().revision == before && loading.snapshot().stages[0].completed_units == 1,
        "Refused stale/foreign reporter mutated actual progress");
    need(loading.advance() == StartupLoadingPhase::completed, "Fresh reporter was refused after stale rejection");
    must_throw<std::logic_error>([&] { stale->report(2, 2); }, "Terminal reporter remained usable");
    {
        StartupLoading short_lived({{"short", "caption", [&](const StartupStageProgress &p) {
            destroyed = p; return StartupStepResult::pending;
        }}});
        need(short_lived.advance() == StartupLoadingPhase::loading, "Lifetime fixture did not enter actual callback");
    }
    must_throw<std::logic_error>([&] { destroyed->report(1); }, "Reporter kept a destroyed controller alive");
}

void registration_before_and_after_start() {
    must_throw<std::invalid_argument>([] { StartupLoading empty(std::vector<StartupLoadingStage>{}); },
        "An empty synthetic loader was accepted");
    unsigned first_calls = 0, second_calls = 0;
    StartupLoading loading({completed_stage("first", first_calls)});
    loading.add_stage(completed_stage("second", second_calls));
    need(loading.snapshot().stages.size() == 2 && first_calls == 0 && second_calls == 0,
        "Pre-start registration ran work");
    must_throw<std::invalid_argument>([&] { loading.add_stage(completed_stage("second", second_calls)); },
        "Duplicate stage identity was accepted");
    must_throw<std::invalid_argument>([&] { loading.add_stage({"empty-callback", "caption", {}}); },
        "Stage without actual callback was accepted");
    need(loading.advance() == StartupLoadingPhase::loading && first_calls == 1 && second_calls == 0,
        "Registered work order changed");
    must_throw<std::logic_error>([&] { loading.add_stage(completed_stage("late", second_calls)); },
        "Stage registration changed an active run");
    need(loading.advance() == StartupLoadingPhase::completed && second_calls == 1, "Second registered stage did not run");
    must_throw<std::logic_error>([&] { loading.add_stage(completed_stage("terminal", second_calls)); },
        "Stage registration changed a terminal run");
}

// Variable-width CPU fixture supplied to the real existing GlyphGen projector.
// It proves wrap/UTF-8 behavior, not native font metrics or atlas/GPU coverage.
GlyphGenGeometry geometry() {
    GlyphGenGeometry result;
    result.atlas_width = result.atlas_height = 64; result.line_height = 16; result.ascent = 12;
    for (std::uint32_t cp = 32; cp < 127; ++cp) {
        const auto advance = cp == 'W' ? 11 : cp == 'i' ? 3 : cp == ' ' ? 4 : 6;
        result.glyphs.push_back({cp, 0, 0, 5, 8, 0, advance, 0, 0, 8});
    }
    for (std::uint32_t cp = 0x400; cp < 0x500; ++cp)
        result.glyphs.push_back({cp, 0, 0, 5, 8, 0, 8, 0, 0, 8});
    result.glyphs.push_back({0x2026, 0, 0, 5, 8, 0, 6, 0, 0, 8});
    return result;
}
std::string join(const std::vector<std::string> &lines, std::size_t first, std::size_t count) {
    std::string result;
    for (auto i = first; i < lines.size() && i - first < count; ++i) {
        if (i != first) result += '\n'; result += lines[i];
    }
    return result;
}
void text_layout_uses_geometry() {
    const auto font = geometry();
    const auto words = layout_frontend_startup_text(font, "Wi Wi", 18);
    need(words.lines == std::vector<std::string>{"Wi", "Wi"} && words.line_height == 16,
        "Word wrapping ignored the selected variable-width geometry");
    const auto breaks = layout_frontend_startup_text(font, "A[NEWLINE]B\n\nC", 120);
    need(breaks.lines == std::vector<std::string>{"A", "B", "", "C"}, "Notice newlines/blank lines were lost");
    const auto spaces = layout_frontend_startup_text(font, "  A \t B\r\n C  ", 120);
    need(spaces.lines == std::vector<std::string>{"A B", "C"}, "Whitespace normalization changed notice words");
    const auto token = layout_frontend_startup_text(font, "WWWWW", 18);
    need(token.lines == std::vector<std::string>{"W", "W", "W", "W", "W"}, "Long token was clipped instead of wrapped");
    const std::string russian = "Русский";
    const auto utf8 = layout_frontend_startup_text(font, russian, 16);
    std::string reconstructed;
    for (const auto &line : utf8.lines) {
        need(project_glyphgen_line(font, line).advance_width <= 16, "Cyrillic line exceeded its actual metrics");
        reconstructed += line;
    }
    need(reconstructed == russian && utf8.lines.size() == 4, "UTF-8 long token split or lost code points");
    need(layout_frontend_startup_text(font, "W", 1).lines == std::vector<std::string>{"W"},
        "A single wider-than-viewport glyph caused a loop or data loss");
    must_throw<std::invalid_argument>([&] { (void)layout_frontend_startup_text(font, "A", 0); }, "Zero-width layout was accepted");
    auto invalid = font; invalid.line_height = 0;
    must_throw<std::invalid_argument>([&] { (void)layout_frontend_startup_text(invalid, "A", 120); },
        "Missing font line height was accepted");
    must_throw<std::runtime_error>([&] { (void)layout_frontend_startup_text(font, "\xc0\xaf", 120); },
        "Malformed UTF-8 survived selected GlyphGen validation");
    must_throw<std::runtime_error>([&] { (void)layout_frontend_startup_text(font, "\xf0\x9f\x98\x80", 120); },
        "An absent selected glyph was silently substituted");
}

FrontendStartupSkin skin() {
    return {"CivilzationVAtlas.dds", "CivilzationV_Logo.dds", "Grid9Detail162",
        "notice", "rating", "continue", "CPU-fixture/selected-LegalScreen.xml"};
}
FrontendLocalizer localizer(bool russian, std::string notice = "Legal notice", std::string rating = "Rating notice") {
    auto rows = frontend_startup_captions(russian);
    rows.emplace("notice", std::move(notice)); rows.emplace("rating", std::move(rating));
    rows.emplace("continue", russian ? "Продолжить" : "Continue");
    return [rows = std::move(rows)](std::string_view key) -> std::optional<std::string> {
        const auto found = rows.find(key);
        return found == rows.end() ? std::nullopt : std::optional<std::string>(found->second);
    };
}
const FrontendHit *hit(const FrontendMenuPlan &plan, std::string_view id) {
    const auto found = std::find_if(plan.hits.begin(), plan.hits.end(), [&](const auto &entry) { return entry.id == id; });
    return found == plan.hits.end() ? nullptr : &*found;
}
WorldViewPoint center(const FrontendHit &entry) {
    return {entry.bounds.x + entry.bounds.width / 2, entry.bounds.y + entry.bounds.height / 2};
}
UiCommandResult click(FrontendStartupSession &session, const FrontendMenuPlan &plan, const FrontendHit &entry) {
    const auto point = center(entry); return session.click(plan, point.x, point.y, point.x, point.y);
}
bool has_text(const FrontendMenuPlan &plan, std::string_view value) {
    return std::any_of(plan.paints.begin(), plan.paints.end(), [&](const auto &paint) {
        return paint.kind == WorldViewPaintKind::text && paint.text == value;
    });
}
bool inside(WorldViewRect r, std::int32_t width, std::int32_t height) {
    return r.width > 0 && r.height > 0 && r.x >= 0 && r.y >= 0 &&
        std::int64_t{r.x} + r.width <= width && std::int64_t{r.y} + r.height <= height;
}
bool overlaps(WorldViewRect a, WorldViewRect b) {
    return a.x < std::int64_t{b.x} + b.width && b.x < std::int64_t{a.x} + a.width &&
        a.y < std::int64_t{b.y} + b.height && b.y < std::int64_t{a.y} + a.height;
}
void extents(const FrontendMenuPlan &plan, std::int32_t width, std::int32_t height) {
    for (const auto &paint : plan.paints) {
        if (paint.kind == WorldViewPaintKind::texture) {
            need(paint.clip && inside(*paint.clip, width, height), "Oversized original art lost its viewport clip");
        } else need(inside(paint.destination, width, height), "Startup text/panel escaped the drawable viewport");
    }
    for (const auto &entry : plan.hits) need(inside(entry.bounds, width, height), "Startup input hit escaped the viewport");
}
const WorldViewPaint &notice_paint(const FrontendMenuPlan &plan) {
    const auto found = std::find_if(plan.paints.begin(), plan.paints.end(), [](const auto &paint) {
        return paint.kind == WorldViewPaintKind::text && paint.style == "TwCenMT16";
    });
    need(found != plan.paints.end(), "Measured notice paint is missing"); return *found;
}

void legal_continue_and_stale_hits() {
    unsigned calls = 0;
    FrontendStartupSession session({completed_stage("actual", calls)}, localizer(false));
    const auto loading = session.compose(320, 240);
    need(!session.menu_ready() && !hit(loading, "startup_continue"), "Loading accepted Legal Continue");
    need(session.advance() == StartupLoadingPhase::completed && calls == 1 &&
        session.phase() == FrontendStartupPhase::legal && !session.menu_ready(),
        "Actual work skipped the explicit legal acknowledgement gate");
    const auto missing = session.compose(320, 240);
    need(!missing.unresolved.empty() && !hit(missing, "startup_continue"), "Missing legal source granted Continue");
    must_throw<std::runtime_error>([] { (void)FrontendStartupSkin::from_sources(FrontendSourceCatalog{}); },
        "An empty source catalog invented the original startup skin");
    session.set_skin(skin());
    const auto no_layout = session.compose(320, 240);
    const auto *disabled = hit(no_layout, "startup_continue");
    need(disabled && !disabled->enabled && !no_layout.unresolved.empty(), "Missing measured layout enabled Continue");
    need(click(session, no_layout, *disabled).status == UiCommandStatus::invalid && !session.menu_ready(),
        "Disabled Continue changed navigation");
    const auto font = geometry();
    session.set_text_layout([&font](std::string_view text, std::int32_t width) {
        return layout_frontend_startup_text(font, text, width);
    });
    const auto legal = session.compose(320, 240);
    const auto *proceed = hit(legal, "startup_continue");
    need(proceed && proceed->enabled && legal.unresolved.empty(), "Complete legal layout did not enable Continue");
    const auto point = center(*proceed);
    need(session.click(legal, point.x, point.y, 0, 0).status == UiCommandStatus::invalid &&
        session.click(legal, 0, 0, point.x, point.y).status == UiCommandStatus::invalid && !session.menu_ready(),
        "Mismatched press/release changed navigation");
    need(session.click(loading, point.x, point.y, point.x, point.y).status == UiCommandStatus::invalid,
        "Pre-completion startup plan remained current");
    unsigned other_calls = 0;
    FrontendStartupSession other({completed_stage("actual", other_calls)}, localizer(false));
    need(other.click(legal, point.x, point.y, point.x, point.y).status == UiCommandStatus::invalid,
        "Another session's hit plan was accepted");
    session.set_skin(skin());
    need(click(session, legal, *proceed).status == UiCommandStatus::invalid, "Skin replacement did not invalidate old hits");
    const auto current = session.compose(320, 240);
    need(click(session, current, *hit(current, "startup_continue")).status == UiCommandStatus::navigation && session.menu_ready(),
        "Current matching Continue did not hand off to the menu");
    need(click(session, current, *hit(current, "startup_continue")).status == UiCommandStatus::invalid,
        "Repeated old Continue acknowledged again");
}

void measured_notice_scroll_and_locales() {
    const auto font = geometry();
    const auto english = frontend_startup_captions(false), russian = frontend_startup_captions(true);
    need(english.size() == russian.size(), "EN/RU stage caption sets differ");
    for (const auto &[key, value] : english)
        need(!value.empty() && russian.contains(key) && !russian.at(key).empty(), "A boot caption is missing in a locale");
    for (const bool ru : {false, true}) {
        unsigned calls = 0;
        std::string notice;
        for (unsigned row = 0; row < 24; ++row) notice += ru ? "Полный текст уведомления сохранен. " : "Complete legal notice remains available. ";
        const std::string rating = ru ? "Рейтинг сохранен." : "Rating remains available.";
        FrontendStartupSession session({completed_stage("actual", calls)}, localizer(ru, notice, rating));
        session.set_skin(skin());
        session.set_text_layout([&font](std::string_view text, std::int32_t width) {
            return layout_frontend_startup_text(font, text, width);
        });
        need(session.advance() == StartupLoadingPhase::completed, "Locale fixture work did not finish");
        auto plan = session.compose(320, 240);
        extents(plan, 320, 240);
        need(has_text(plan, ru ? "Уведомления" : "Notices") && has_text(plan, ru ? "Продолжить" : "Continue"),
            "Legal heading/Continue were not localized");
        const auto expected = layout_frontend_startup_text(font, notice + "\n\n" + rating, notice_paint(plan).destination.width);
        need(plan.total_rows == expected.lines.size() && plan.total_rows > plan.visible_rows && plan.first_row == 0,
            "Long notice/rating was silently truncated or lost scroll metadata");
        const auto first_plan = plan;
        bool reached_rating = false;
        for (std::size_t offset = 0; offset <= expected.lines.size(); ++offset) {
            need(notice_paint(plan).text == join(expected.lines, plan.first_row, plan.visible_rows),
                "Measured scrolling dropped, reordered or repeated notice lines");
            if (notice_paint(plan).text.find(ru ? "Рейтинг" : "Rating") != std::string::npos) reached_rating = true;
            extents(plan, 320, 240);
            if (plan.first_row + plan.visible_rows >= plan.total_rows) break;
            session.scroll(1); plan = session.compose(320, 240);
        }
        need(reached_rating, "Rating at the end of the selected notice was unreachable");
        need(click(session, first_plan, *hit(first_plan, "startup_continue")).status == UiCommandStatus::invalid,
            "Scrolled notice retained stale Continue hits");
        session.scroll(std::numeric_limits<std::int32_t>::max());
        plan = session.compose(320, 240);
        need(plan.first_row + plan.visible_rows == plan.total_rows, "Large scroll did not clamp at notice end");
        session.scroll(-std::numeric_limits<std::int32_t>::max());
        plan = session.compose(320, 240);
        need(plan.first_row == 0, "Negative scroll did not clamp at notice start");
        session.set_text_layout({});
        const auto disabled = session.compose(320, 240);
        need(hit(disabled, "startup_continue") && !hit(disabled, "startup_continue")->enabled,
            "Removed layout retained Continue readiness");
    }
}

void loading_progress_and_small_extents() {
    for (const bool known : {false, true}) {
        FrontendStartupSession session({{"scan", "frontend.startup.resources", [known](const StartupStageProgress &p) {
            p.report(1, known ? std::optional<std::uint64_t>(4) : std::nullopt, "Actual resource detail");
            return StartupStepResult::pending;
        }}}, localizer(true));
        session.set_skin(skin());
        need(session.advance() == StartupLoadingPhase::loading, "Pending UI progress completed startup");
        auto small = session.compose(320, 240); extents(small, 320, 240);
        need(has_text(small, "Загрузка Civis") && has_text(small, "Подготовка ресурсов") && !hit(small, "startup_continue"),
            "Loading phase lost its actual stage or exposed Continue");
        const auto *exit = hit(small, "startup_exit");
        need(exit && exit->enabled, "Loading lost its exit action");
        for (const auto &paint : small.paints) if (paint.kind == WorldViewPaintKind::text && paint.text == "1 / 4")
            need(!overlaps(paint.destination, exit->bounds), "320x240 work counter overlaps Exit");
        const auto large = session.compose(1280, 720); extents(large, 1280, 720);
        need(has_text(large, "Actual resource detail"), "Actual progress detail disappeared from the drawable view");
        if (known) need(has_text(large, "1 / 4"), "Known actual units disappeared from progress presentation");
        else {
            need(!session.snapshot().current()->fraction() && !has_text(large, "1 / 4"),
                "Unknown work acquired a fabricated fraction/count");
            for (const auto &paint : large.paints) if (paint.kind == WorldViewPaintKind::text)
                need(paint.text.find('%') == std::string::npos, "Unknown work displayed a synthetic percentage");
        }
        must_throw<std::invalid_argument>([&] { (void)session.compose(0, 240); }, "Empty width was accepted");
        must_throw<std::invalid_argument>([&] { (void)session.compose(320, 0); }, "Empty height was accepted");
    }
}

void ui_failure_and_exit_preserve_cause() {
    const auto font = geometry();
    FrontendStartupSession session({{"error", "caption", [](const StartupStageProgress &) -> StartupStepResult {
        throw std::runtime_error("actual startup failure");
    }}}, localizer(false));
    session.set_text_layout([&font](std::string_view text, std::int32_t width) {
        return layout_frontend_startup_text(font, text, width);
    });
    need(session.advance() == StartupLoadingPhase::failed && session.phase() == FrontendStartupPhase::failed,
        "Callback failure opened the normal menu");
    const auto plan = session.compose(320, 240); extents(plan, 320, 240);
    need(has_text(plan, "actual startup failure") && hit(plan, "startup_exit") && !hit(plan, "startup_continue"),
        "Failed UI hid the original cause or offered Continue");
    need(click(session, plan, *hit(plan, "startup_exit")).status == UiCommandStatus::navigation &&
        session.phase() == FrontendStartupPhase::cancelled && !session.menu_ready(), "Exit could not close a failed startup");
    need(session.snapshot().phase == StartupLoadingPhase::failed && session.snapshot().error == "actual startup failure",
        "Closing the failure view erased the original failure");
}

void ui_exit_waits_for_callback_return() {
    FrontendStartupSession *owner = nullptr;
    unsigned calls = 0, later_calls = 0;
    FrontendStartupSession session({
        {"active", "caption", [&](const StartupStageProgress &p) {
            ++calls; owner->request_exit();
            need(owner->phase() == FrontendStartupPhase::loading && p.cancellation_requested(),
                "Exit committed before the running UI callback returned");
            return StartupStepResult::pending;
        }}, completed_stage("later", later_calls)
    }, localizer(false));
    owner = &session;
    need(session.advance() == StartupLoadingPhase::cancelled && session.phase() == FrontendStartupPhase::cancelled &&
        calls == 1 && later_calls == 0 && !session.menu_ready(), "UI cancellation escaped its safe boundary");
    unsigned not_called = 0;
    FrontendStartupSession before({completed_stage("not-called", not_called)}, localizer(false));
    const auto plan = before.compose(320, 240);
    need(click(before, plan, *hit(plan, "startup_exit")).status == UiCommandStatus::navigation &&
        before.phase() == FrontendStartupPhase::loading, "Exit click committed cancellation outside advance");
    need(before.advance() == StartupLoadingPhase::cancelled && not_called == 0, "Exit before start still ran preparation");
    unsigned finished_calls = 0;
    FrontendStartupSession *finished_owner = nullptr;
    FrontendStartupSession finished({completed_stage("actual", finished_calls)}, localizer(false),
        [&](StartupLoadingEvent event, const StartupLoadingSnapshot &) {
            if (event == StartupLoadingEvent::completed) finished_owner->request_exit();
        });
    finished_owner = &finished;
    need(finished.advance() == StartupLoadingPhase::completed && finished_calls == 1 &&
        finished.phase() == FrontendStartupPhase::cancelled && !finished.menu_ready(),
        "Close observed during final completion still opened Legal/Menu");
}

void explicit_legal_policy_and_invalid_layout() {
    unsigned calls = 0;
    FrontendStartupSession acknowledged({completed_stage("actual", calls)}, localizer(false), {}, true);
    need(!acknowledged.menu_ready(), "Already-shown legal flag skipped actual preparation");
    need(acknowledged.advance() == StartupLoadingPhase::completed && calls == 1 && acknowledged.menu_ready(),
        "Explicit already-shown legal policy did not enter menu after actual work");
    unsigned other_calls = 0;
    FrontendStartupSession invalid({completed_stage("actual", other_calls)}, localizer(false));
    invalid.set_skin(skin());
    invalid.set_text_layout([](std::string_view, std::int32_t) { return FrontendStartupTextBlock{{"notice"}, 0}; });
    need(invalid.advance() == StartupLoadingPhase::completed, "Invalid layout fixture lost actual work");
    must_throw<std::runtime_error>([&] { (void)invalid.compose(320, 240); }, "Invalid measured line height granted a legal plan");
    need(!invalid.menu_ready(), "Failed legal layout changed menu readiness");
}
} // namespace

int main() {
    run("sequence_and_observers", sequence_and_observers);
    run("unknown_zero_and_late_total", unknown_zero_and_late_total);
    run("invalid_counters_and_unfinished", invalid_counters_and_unfinished);
    run("first_error_survives_failure_observer", first_error_survives_failure_observer);
    run("observer_failure_and_reentry", observer_failure_and_reentry);
    run("cancellation_at_safe_boundaries", cancellation_at_safe_boundaries);
    run("cancellation_after_completed_stage", cancellation_after_completed_stage);
    run("checkpoint_unwinds_only_at_safe_boundary", checkpoint_unwinds_only_at_safe_boundary);
    run("retained_and_foreign_progress", retained_and_foreign_progress);
    run("registration_before_and_after_start", registration_before_and_after_start);
    run("text_layout_uses_geometry", text_layout_uses_geometry);
    run("legal_continue_and_stale_hits", legal_continue_and_stale_hits);
    run("measured_notice_scroll_and_locales", measured_notice_scroll_and_locales);
    run("loading_progress_and_small_extents", loading_progress_and_small_extents);
    run("ui_failure_and_exit_preserve_cause", ui_failure_and_exit_preserve_cause);
    run("ui_exit_waits_for_callback_return", ui_exit_waits_for_callback_return);
    run("explicit_legal_policy_and_invalid_layout", explicit_legal_policy_and_invalid_layout);
    std::cout << "frontend_startup_tests: " << (failures ? "FAIL " : "PASS ") << checks
              << " checks in " << cases << " cases, " << failures << " failures\n";
    return failures ? 1 : 0;
}
