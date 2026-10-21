#include "civ5/runtime/bnw_selected_locale_owner.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <thread>

using namespace civ5;
namespace {
void need(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class F> void refuses(F f, runtime::BnwSelectedLocaleError code) {
    try { f(); } catch (const runtime::BnwSelectedLocaleRefusal& e) {
        need(e.code() == code, "Unexpected selected locale refusal"); return;
    }
    throw std::runtime_error("Selected locale unexpectedly accepted");
}
template<class F> void lookup_refuses(F f) {
    try { f(); } catch (const runtime::ScriptError&) { return; }
    throw std::runtime_error("Unsupported localized key unexpectedly accepted");
}
runtime::BnwCreationChoices choices() {
    return {"WORLDSIZE_STANDARD", "CLIMATE_TEMPERATE", "SEALEVEL_MEDIUM",
            "GAMESPEED_STANDARD", "ERA_ANCIENT", "HANDICAP_PRINCE"};
}
void write_fixture(const std::filesystem::path& path, std::string_view bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.close(); need(!output.fail(), "Private INI fixture write failed");
}
std::shared_ptr<runtime::BnwFreshGameContentResult> isolated_metadata() {
    // Private metadata fixture, not a selected installation/world authority.
    auto p = std::make_shared<runtime::BnwFreshGameContentResult>();
    p->profile.kind = runtime::ContentProfileKind::dlc;
    p->profile.can_activate = p->profile.all_requirements_located = true;
    p->profile.enabled_dlc = {0};
    runtime::ContentRequirement requirement;
    requirement.kind = content::PackageKind::dlc;
    requirement.id = "6da07636-4123-4018-b643-6575b4ec336b";
    requirement.status = runtime::RequirementStatus::located; requirement.package = 0;
    p->profile.requirements.push_back(requirement);
    p->activation.emplace(); p->activation->profile_kind = p->profile.kind;
    p->activation->rules_database_ready = true;
    p->rules = std::make_unique<content::RulesDatabase>();
    p->rules->apply_sql(R"sql(
CREATE TABLE Worlds(ID INTEGER,Type TEXT); INSERT INTO Worlds VALUES(5,'WORLDSIZE_STANDARD');
CREATE TABLE Climates(ID INTEGER,Type TEXT); INSERT INTO Climates VALUES(6,'CLIMATE_TEMPERATE');
CREATE TABLE SeaLevels(ID INTEGER,Type TEXT); INSERT INTO SeaLevels VALUES(7,'SEALEVEL_MEDIUM');
CREATE TABLE GameSpeeds(ID INTEGER,Type TEXT); INSERT INTO GameSpeeds VALUES(2,'GAMESPEED_STANDARD');
CREATE TABLE Eras(ID INTEGER,Type TEXT); INSERT INTO Eras VALUES(4,'ERA_ANCIENT');
CREATE TABLE HandicapInfos(ID INTEGER,Type TEXT); INSERT INTO HandicapInfos VALUES(7,'HANDICAP_PRINCE');
)sql");
    content::RulesDatabase text;
    text.apply_sql(R"sql(
CREATE TABLE Languages(Type TEXT PRIMARY KEY,Name TEXT);
CREATE TABLE LanguagePriorities(LanguageSet TEXT,Language TEXT,Priority INTEGER);
CREATE TABLE LocalizedText(Language TEXT,Tag TEXT,Text TEXT);
INSERT INTO Languages VALUES('en_US','English'),('ru_RU','Russian');
INSERT INTO LanguagePriorities VALUES('ru_RU','ru_RU',100),('en_US','en_US',100);
INSERT INTO LocalizedText VALUES('ru_RU','TXT_CHOSEN','Chosen Russian'),('en_US','TXT_CHOSEN','Chosen English');
INSERT INTO LocalizedText VALUES('en_US','TXT_FALLBACK','Actual English fallback');
INSERT INTO LocalizedText VALUES('ru_RU','TXT_FORMAT','{1} native grammar');
)sql");
    p->rules->attach_localization(text);
    p->rules->seal_immutable_preview_fingerprints();
    return p;
}
void isolated_cases(const std::filesystem::path& dir) {
    using C = runtime::BnwCreationConfig; using L = runtime::BnwSelectedLocaleOwner;
    using E = runtime::BnwSelectedLocaleError;
    auto p = isolated_metadata(); auto c = C::from_selected_rules(p, choices());
    const auto ini = dir / "selected.ini";
    const std::string selected = "; bounded private fixture\r\n[User Settings]\r\nLanguage = ru_RU\r\n";
    write_fixture(ini, selected);
    refuses([&] { (void)L::from_creation_config({}, ini); }, E::parent_unavailable);
    auto owner = L::from_creation_config(c, ini);
    need(owner->language() == "ru_RU" && owner->localized_text("TXT_CHOSEN") == "Chosen Russian",
         "Selected actual Language must override English");
    need(owner->localized_text("TXT_FALLBACK") == "Actual English fallback", "Existing priority/fallback semantics");
    owner->require_same_config(*c);
    auto other = C::from_selected_rules(p, choices());
    refuses([&] { owner->require_same_config(*other); }, E::wrong_config);
    lookup_refuses([&] { (void)owner->localized_text(""); });
    lookup_refuses([&] { (void)owner->localized_text("TXT_ABSENT"); });
    lookup_refuses([&] { (void)owner->localized_text("TXT_FORMAT"); });
    lookup_refuses([&] { (void)owner->localized_text(std::string_view("X\0Y", 3)); });
    owner->require_current();
    bool wrong_thread = false;
    std::thread foreign([&] {
        try { (void)owner->localized_text("TXT_CHOSEN"); }
        catch (const runtime::BnwSelectedLocaleRefusal& e) { wrong_thread = e.code() == E::wrong_thread; }
    });
    foreign.join(); need(wrong_thread, "Foreign thread must refuse before file/DB access"); owner->require_current();
    const auto absent = dir / "absent.ini"; write_fixture(absent, "[User Settings]\nSpokenLanguage=en_US\n");
    refuses([&] { (void)L::from_creation_config(c, absent); }, E::invalid_language_input);
    const auto duplicate = dir / "duplicate.ini"; write_fixture(duplicate, selected + "language=en_US\n");
    refuses([&] { (void)L::from_creation_config(c, duplicate); }, E::invalid_language_input);
    const auto wrong_section = dir / "wrong-section.ini"; write_fixture(wrong_section, "[Debugging]\nLanguage=ru_RU\n");
    refuses([&] { (void)L::from_creation_config(c, wrong_section); }, E::invalid_language_input);
    const auto unknown = dir / "unknown.ini"; write_fixture(unknown, "[User Settings]\nLanguage=not_installed\n");
    lookup_refuses([&] { (void)L::from_creation_config(c, unknown); });
    refuses([&] { (void)L::from_creation_config(c, dir / "missing.ini"); }, E::input_unavailable);
    write_fixture(ini, selected + "; changed input\n");
    refuses([&] { owner->require_current(); }, E::input_changed);
    write_fixture(ini, selected);
    refuses([&] { owner->require_current(); }, E::input_changed); // No resurrection after observed change.
    auto held = L::from_creation_config(c, ini);
    std::weak_ptr<const C> weak_config = c;
    std::weak_ptr<const runtime::BnwFreshGameContentResult> weak_parent = p;
    other.reset(); owner.reset(); c.reset(); p.reset();
    need(!weak_config.expired() && !weak_parent.expired() && held->localized_text("TXT_CHOSEN") == "Chosen Russian",
         "Sole actual locale owner retains SAME config/rules/metadata parent");
    held.reset(); need(weak_config.expired() && weak_parent.expired(), "Selected locale has no strong cycle");
    auto retired = isolated_metadata(); auto retired_config = C::from_selected_rules(retired, choices());
    auto old = L::from_creation_config(retired_config, ini);
    retired->rules.reset();
    bool db_retired = false;
    try { old->require_current(); } catch (const runtime::BnwCreationConfigRefusal& e) {
        db_retired = e.code() == runtime::BnwCreationConfigError::rules_retired;
    }
    need(db_retired, "Destroyed SAME rules must refuse before dangling locale borrow");
    auto poisoned = isolated_metadata(); auto poisoned_config = C::from_selected_rules(poisoned, choices());
    auto poisoned_owner = L::from_creation_config(poisoned_config, ini);
    try { poisoned->rules->apply_sql("UPDATE Localization.LocalizedText SET Text='Wrong';"); }
    catch (const std::exception&) { /* Expected immutable authorizer refusal; checked below. */ }
    need(!poisoned->rules->immutable_preview_fingerprints_current(), "Attached Localization writer poisons SAME rules");
    bool rules_poisoned = false;
    try { poisoned_owner->require_current(); } catch (const runtime::BnwCreationConfigRefusal&) { rules_poisoned = true; }
    need(rules_poisoned, "Poisoned rules must not issue text");
}
void actual_selected(const std::filesystem::path& installation, const std::filesystem::path& ini) {
    const auto before = runtime::identify_file(ini);
    content::Catalog catalog;
    catalog.packages.push_back(content::read_manifest(installation / "Assets/DLC/Expansion/Expansion1.Civ5Pkg"));
    catalog.packages.push_back(content::read_manifest(installation / "Assets/DLC/Expansion2/Expansion2.Civ5Pkg"));
    constexpr std::array<std::size_t, 2> selected{0, 1};
    auto p = std::make_shared<runtime::BnwFreshGameContentResult>(
        runtime::bootstrap_bnw_fresh_game_content(installation, catalog, selected));
    need(p->content_ready(), "Actual selected BNW content activation failed");
    p->rules->seal_immutable_preview_fingerprints();
    auto c = runtime::BnwCreationConfig::from_selected_rules(p, choices());
    auto locale = runtime::BnwSelectedLocaleOwner::from_creation_config(c, ini);
    const auto source = locale->source();
    need(source.language == "ru_RU", "This acceptance fixture requires actual selected ru_RU input");
    need(source.file.bytes == before.bytes && source.file.sha256 == before.sha256, "Exact actual INI provenance");
    // Independent SQL witness requires installed Russian text different from English.
    std::optional<std::string> tag, russian;
    const auto report = p->rules->query_rows({
        "SELECT r.Tag AS Tag,r.Text AS Russian FROM Localization.LocalizedText r "
        "JOIN Localization.LocalizedText e ON e.Tag=r.Tag AND e.Language='en_US' "
        "WHERE r.Language='ru_RU' AND r.Text<>e.Text AND length(r.Text)>0 "
        "AND instr(r.Text,'{')=0 AND instr(r.Text,'}')=0 ORDER BY r.Tag LIMIT 1", {}},
        [&](const content::RulesRowView& row) { tag = row.text("Tag"); russian = row.text("Russian"); }, 1);
    need(report.all_succeeded() && tag && russian, "Actual distinct installed Russian lookup witness missing");
    need(locale->localized_text(*tag) == *russian, "Actual selected language lookup must match genuine Russian source");
    need(p->has_blocker(runtime::FreshGameContentBlockerCode::lua_loading_pending) &&
         p->has_blocker(runtime::FreshGameContentBlockerCode::world_setup_pending), "Localization must not grant hooks/world");
    c.reset(); p.reset(); need(locale->localized_text(*tag) == *russian, "Actual sole locale parent retention");
    const auto after = runtime::identify_file(ini);
    need(before.bytes == after.bytes && before.sha256 == after.sha256, "Actual user INI must remain untouched");
    std::cout << "actual_selected_language=ru_RU actual_distinct_text=1 world_created=0 hooks_completed=0\n";
}
} // namespace
int main(int argc, char** argv) {
    try {
        need(argc == 4, "Expected installation, actual selected config.ini, private fixture parent directory");
        const auto root = std::filesystem::weakly_canonical(std::filesystem::path(argv[3]));
        const auto build = std::filesystem::weakly_canonical(std::filesystem::path("C:/CrazyProject/build"));
        const auto relative = root.lexically_relative(build);
        need(!relative.empty() && !relative.is_absolute() && *relative.begin() != "..", "INI fixtures must stay in private build directory");
        const auto dir = root / ("locale-fixture-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        need(std::filesystem::create_directories(dir), "Private fixture directory must be new");
        isolated_cases(dir); actual_selected(std::filesystem::path(argv[1]), std::filesystem::path(argv[2]));
        std::cout << "Selected locale ownership cases passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
