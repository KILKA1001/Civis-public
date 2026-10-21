#include "civ5/runtime/bnw_creation_config.hpp"
#include <array>
#include <iostream>
#include <limits>
#include <thread>
#include <utility>
#include <vector>
using namespace civ5;
namespace {
void need(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template<class F> void refuses(F f, runtime::BnwCreationConfigError expected) {
    try { f(); } catch (const runtime::BnwCreationConfigRefusal &e) {
        need(e.code() == expected, "Unexpected creation refusal code"); return;
    }
    throw std::runtime_error("Creation metadata unexpectedly accepted");
}
runtime::BnwCreationChoices choices() {
    return {"WORLDSIZE_STANDARD", "CLIMATE_TEMPERATE", "SEALEVEL_MEDIUM",
            "GAMESPEED_STANDARD", "ERA_ANCIENT", "HANDICAP_PRINCE"};
}
// An actual isolated SQLite metadata fixture, explicitly NOT bootstrap/world
// provenance. Public content reports can never attest to a created session.
std::shared_ptr<runtime::BnwFreshGameContentResult> metadata_fixture(std::string extra = {}) {
    auto p = std::make_shared<runtime::BnwFreshGameContentResult>();
    p->profile.kind = runtime::ContentProfileKind::dlc;
    p->profile.can_activate = p->profile.all_requirements_located = true;
    p->profile.enabled_dlc = {0};
    runtime::ContentRequirement r;
    r.kind = content::PackageKind::dlc;
    r.id = "6da07636-4123-4018-b643-6575b4ec336b";
    r.status = runtime::RequirementStatus::located; r.package = 0;
    p->profile.requirements.push_back(r);
    p->activation.emplace(); p->activation->profile_kind = p->profile.kind;
    p->activation->rules_database_ready = true;
    p->rules = std::make_unique<content::RulesDatabase>();
    p->rules->apply_sql(R"sql(
CREATE TABLE Worlds(ID INTEGER, Type TEXT); INSERT INTO Worlds VALUES(5,'WORLDSIZE_STANDARD');
CREATE TABLE Climates(ID INTEGER, Type TEXT); INSERT INTO Climates VALUES(6,'CLIMATE_TEMPERATE');
CREATE TABLE SeaLevels(ID INTEGER, Type TEXT); INSERT INTO SeaLevels VALUES(7,'SEALEVEL_MEDIUM');
CREATE TABLE GameSpeeds(ID INTEGER, Type TEXT); INSERT INTO GameSpeeds VALUES(2,'GAMESPEED_STANDARD');
CREATE TABLE Eras(ID INTEGER, Type TEXT); INSERT INTO Eras VALUES(4,'ERA_ANCIENT');
CREATE TABLE HandicapInfos(ID INTEGER, Type TEXT); INSERT INTO HandicapInfos VALUES(7,'HANDICAP_PRINCE');
)sql");
    if (!extra.empty()) p->rules->apply_sql(extra);
    return p;
}
void metadata_cases() {
    using C = runtime::BnwCreationConfig; using E = runtime::BnwCreationConfigError;
    refuses([] { (void)C::from_selected_rules({}, choices()); }, E::parent_unavailable);
    auto p = metadata_fixture();
    refuses([&] { (void)C::from_selected_rules(p, choices()); }, E::rules_not_immutable);
    p->rules->seal_immutable_preview_fingerprints();
    auto c = C::from_selected_rules(p, choices());
    const auto data = c->metadata();
    need(data.info[0].id == 5 && data.info[1].id == 6 && data.info[2].id == 7 &&
         data.info[3].id == 2 && data.info[4].id == 4 && data.info[5].id == 7 &&
         data.speed_timer_write == 3, "Native sparse slots/direct cached ID semantics");
    auto lower = choices(); lower.speed = "gamespeed_standard"; lower.era = "era_ancient";
    need(C::from_selected_rules(p, lower)->metadata().info[3].type == "GAMESPEED_STANDARD", "Native ASCII case-insensitive lookup");
    auto missing = choices(); missing.world = "WORLDSIZE_UNKNOWN";
    refuses([&] { (void)C::from_selected_rules(p, missing); }, E::invalid_choice);
    missing = choices(); missing.world = "worldsize_standard";
    refuses([&] { (void)C::from_selected_rules(p, missing); }, E::invalid_choice);
    missing = choices(); missing.world.assign("X\0Y", 3);
    refuses([&] { (void)C::from_selected_rules(p, missing); }, E::invalid_choice);
    bool foreign = false;
    std::thread t([&] { try { (void)c->metadata(); } catch (const runtime::BnwCreationConfigRefusal &e) { foreign = e.code() == E::wrong_thread; } });
    t.join(); need(foreign, "Creation metadata must refuse foreign thread before DB query"); c->require_current();
    std::weak_ptr<const runtime::BnwFreshGameContentResult> weak = p;
    p.reset(); need(!weak.expired() && c->metadata().info[4].id == 4, "Config retains SAME actual metadata parent");
    c.reset(); need(weak.expired(), "Config lifetime has no self-cycle");
    auto wide = metadata_fixture("UPDATE Worlds SET ID=2147483648;");
    wide->rules->seal_immutable_preview_fingerprints();
    refuses([&] { (void)C::from_selected_rules(wide, choices()); }, E::unsupported_native_metadata);
    auto alias = metadata_fixture("INSERT INTO GameSpeeds VALUES(4,'gamespeed_standard');");
    alias->rules->seal_immutable_preview_fingerprints();
    const auto aliased = C::from_selected_rules(alias, choices())->metadata();
    need(aliased.info[3].id == 4 && aliased.speed_assignments.size() == 2,
         "Native every case-insensitive matching slot must retain its setter descriptor");
    need(aliased.speed_assignments[0].speed.id == 2 && aliased.speed_assignments[0].timer_write == 3 &&
         aliased.speed_assignments[1].speed.id == 4 && !aliased.speed_assignments[1].timer_write &&
         aliased.speed_timer_write == 3, "Later no-write speed preserves earlier actual timer write");
    auto no_write = metadata_fixture("DELETE FROM GameSpeeds; INSERT INTO GameSpeeds VALUES(0,'GAMESPEED_MARATHON');");
    no_write->rules->seal_immutable_preview_fingerprints();
    auto marathon = choices(); marathon.speed = "GAMESPEED_MARATHON";
    const auto no_timer = C::from_selected_rules(no_write, marathon)->metadata();
    need(no_timer.speed_assignments.size() == 1 && no_timer.speed_assignments[0].speed.id == 0 &&
         !no_timer.speed_assignments[0].timer_write && !no_timer.speed_timer_write,
         "Native Marathon no-write is not an inferred timer-zero assignment");
    auto collision = metadata_fixture("CREATE TABLE ForeignInfo(ID INTEGER, Type TEXT); INSERT INTO ForeignInfo VALUES(2,'WORLDSIZE_STANDARD');");
    collision->rules->seal_immutable_preview_fingerprints();
    refuses([&] { (void)C::from_selected_rules(collision, choices()); }, E::unsupported_native_metadata);
    auto overlap = metadata_fixture("INSERT INTO GameSpeeds VALUES(2,'GAMESPEED_ALIAS');");
    overlap->rules->seal_immutable_preview_fingerprints();
    auto overlap_choice = choices(); overlap_choice.speed = "GAMESPEED_ALIAS";
    refuses([&] { (void)C::from_selected_rules(overlap, overlap_choice); }, E::unsupported_native_metadata);
    auto retired = metadata_fixture(); retired->rules->seal_immutable_preview_fingerprints();
    auto old = C::from_selected_rules(retired, choices()); retired->rules.reset();
    refuses([&] { (void)old->metadata(); }, E::rules_retired);
    auto profile = metadata_fixture(); profile->rules->seal_immutable_preview_fingerprints();
    auto bound = C::from_selected_rules(profile, choices()); profile->profile.enabled_dlc.clear();
    refuses([&] { (void)bound->metadata(); }, E::rules_retired);
    auto wrong = metadata_fixture(); wrong->profile.requirements.clear(); wrong->rules->seal_immutable_preview_fingerprints();
    refuses([&] { (void)C::from_selected_rules(wrong, choices()); }, E::profile_not_bnw);
}
void actual_bootstrap(const std::filesystem::path &installation) {
    content::Catalog catalog;
    catalog.packages.push_back(content::read_manifest(installation / "Assets/DLC/Expansion/Expansion1.Civ5Pkg"));
    catalog.packages.push_back(content::read_manifest(installation / "Assets/DLC/Expansion2/Expansion2.Civ5Pkg"));
    constexpr std::array<std::size_t, 2> selected{0, 1};
    auto p = std::make_shared<runtime::BnwFreshGameContentResult>(
        runtime::bootstrap_bnw_fresh_game_content(installation, catalog, selected));
    need(p->content_ready(), "Actual canonical installation BNW rules activation unavailable");
    p->rules->seal_immutable_preview_fingerprints();
    const auto c = runtime::BnwCreationConfig::from_selected_rules(p, choices());
    const auto m = c->metadata();
    need(!m.rules_sha256.empty() && m.info[0].type == "WORLDSIZE_STANDARD" && m.info[3].type == "GAMESPEED_STANDARD", "Actual SAME selected BNW metadata read");
    need(p->has_blocker(runtime::FreshGameContentBlockerCode::lua_loading_pending) &&
         p->has_blocker(runtime::FreshGameContentBlockerCode::map_script_pending) &&
         p->has_blocker(runtime::FreshGameContentBlockerCode::world_setup_pending), "Metadata must preserve every mandatory world hook frontier");
    std::cout << "actual_selected_metadata=6 world_created=0 hooks_completed=0\n";
}
} // namespace
int main(int argc, char **argv) {
    try {
        need(argc == 2, "Expected canonical BNW installation root");
        metadata_cases(); actual_bootstrap(std::filesystem::path(argv[1]));
        std::cout << "BNW creation config metadata cases passed\n"; return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
