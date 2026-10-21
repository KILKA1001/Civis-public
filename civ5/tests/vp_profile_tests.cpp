#include "civ5/content/vp_profile.hpp"
#include "civ5/content/vp_content_profile.hpp"
#include "civ5/content/vp_lua_assets_profile.hpp"
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <source_location>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace civ5;
namespace {
unsigned checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action, std::source_location at = std::source_location::current()) {
    bool rejected = false;
    try {
        action();
    } catch (const std::exception &) {
        rejected = true;
    }
    ++checks;
    if (!rejected)
        throw std::runtime_error("Expected rejection at " + std::to_string(at.line()));
}
void write(const fs::path &file, const std::string &text) {
    std::ofstream stream(file, std::ios::binary);
    stream << text;
    if (!stream)
        throw std::runtime_error("Cannot write profile fixture");
}
void run(const fs::path &root) {
    fs::create_directories(root);
    struct Case {
        const char *name;
        bool vp, eui, large;
    };
    const Case cases[] = {{"Core", false, false, false},     {"Civ43CPOnly", false, false, true},
                          {"FullNoEUI", true, false, false}, {"FullEUI", true, true, false},
                          {"Civ43NoEUI", true, false, true}, {"Civ43EUI", true, true, true}};
    for (const auto &item : cases) {
        const auto component = content::vp_component(item.name);
        check(content::vp_component_name(component) == item.name, "Component label changed");
        const auto includes = [&](const std::string &path) {
            return content::vp_542_destination(path, component).has_value();
        };
        check(includes("app/MODS/(1) Community Patch/Database Changes/A.sql"), "Core rules excluded");
        check(includes("app/MODS/(1) Community Patch/LUA/CityView.lua") == !item.eui,
              "CP Lua component mismatch");
        check(includes("app/MODS/(2) Vox Populi/Database Changes/A.sql") == item.vp,
              "VP rules component mismatch");
        check(includes("app/MODS/(2) Vox Populi/LUA/CityView.lua") == (item.vp && !item.eui),
              "VP Lua component mismatch");
        check(includes("app/MODS/(3a) VP - EUI Compatibility Files/LUA/CityView.lua") == item.eui,
              "EUI package selection");
        check(includes("app/MODS/(3b) 43 Civs Community Patch/AdvancedSetup.lua") == item.large,
              "43 civ setup selection");
        check(includes("app/MODS/(4a) Squads for VP/A.lua") == item.vp,
              "Optional installed package selection");
        check(includes("app/MODS/(5) Modpack Maker for VP/A.lua"), "Modpack Maker availability changed");
        check(includes("code$GetCIVDir/Assets/DLC/UI_bc1/A.lua") == item.eui, "EUI game overlay selection");
        check(includes("code$GetCIVDir/Assets/DLC/VPUI/A.lua") == item.vp, "VP game overlay selection");
        check(includes("app/Text/VPUI_tips_en_us.xml") == item.vp, "VP text overlay selection");
        check(!includes("code$GetCIVDir/Assets/DLC/Expansion2/"
                        "Expansion2.Civ5Pkg#Uninstall"),
              "Uninstall resource activated");
        check(includes("app/MODS/(1) Community Patch/CvGameCore_Expansion2.dll$0") == !item.large,
              "Standard DLL selected incorrectly");
        check(includes("app/MODS/(1) Community Patch/CvGameCore_Expansion2.dll$1") == item.large,
              "43 DLL selected incorrectly");
        const auto core = content::vp_542_destination(
            "app/MODS/(1) Community Patch/CvGameCore_Expansion2.dll$" + std::string(item.large ? "1" : "0"),
            component);
        check(core == "MODS/(1) Community Patch/CvGameCore_Expansion2.dll",
              "DLL logical name retained collision suffix");
        const std::string audio = "code$GetCIVDir/Assets/DLC/Expansion2/Sounds/XML/"
                                  "MinorCivSounds_VoxPopuli.xml$";
        check(includes(audio + "0") == !item.vp && includes(audio + "1") == item.vp,
              "CP/VP audio variants mixed");
        check(content::vp_542_destination(audio + (item.vp ? "1" : "0"), component) ==
                  "Game/Assets/DLC/Expansion2/Sounds/XML/"
                  "MinorCivSounds_VoxPopuli.xml",
              "Audio target name changed");
    }
    rejects([] { content::vp_component("Uninstall"); });
    rejects([] { content::vp_component("latest"); });
    rejects([] { content::vp_component_name(static_cast<content::VpComponent>(99)); });
    for (const auto *bad :
         {"../secret", "C:/absolute", "app/../secret", "app/MODS/Unknown/A.xml",
          "app/MODS/(1) Community Patch/CvGameCore_Expansion2.dll$2", "code$GetCIVDir/other.xml"})
        rejects([&] { content::vp_542_destination(bad, content::VpComponent::full_43_eui); });
    content::VpProfile profile{content::VpComponent::full, {}, {}, {}, 0};
    const auto xml_path = root / "create.xml", sql_path = root / "change.sql";
    write(xml_path, "<GameData><Table name='State'><Column name='ID' type='integer' "
                    "primarykey='true'/><Column name='Value' type='integer'/></Table>"
                    "<State><Row><ID>4294967297</ID><Value>3</Value></Row></State></"
                    "GameData>");
    write(sql_path, "UPDATE State SET Value=Value*2 WHERE ID=4294967297;");
    profile.files.emplace("mods/fixture/create.xml", content::ProfileFile{"MODS/Fixture/Create.xml", xml_path,
                                                                          runtime::identify_file(xml_path)});
    profile.files.emplace("mods/fixture/change.sql", content::ProfileFile{"MODS/Fixture/Change.sql", sql_path,
                                                                          runtime::identify_file(sql_path)});
    content::PackageManifest manifest{};
    manifest.kind = content::PackageKind::mod;
    manifest.id = "fixture";
    manifest.document =
        content::read_xml_metadata("<Mod><Actions><OnModActivated><UpdateDatabase>Create.xml</"
                                   "UpdateDatabase>"
                                   "<OtherAction>not_database.lua</OtherAction><UpdateDatabase>Change.sql</"
                                   "UpdateDatabase><UpdateDatabase>Change.sql</UpdateDatabase>"
                                   "</OnModActivated></Actions></Mod>");
    profile.installed_mods.push_back({manifest, "MODS/Fixture", {}, false});
    const auto &package = profile.mod("FIXTURE");
    const auto ordered = profile.action_files(package, "OnModActivated", "UpdateDatabase");
    check(ordered.size() == 3 && ordered[0]->source == xml_path && ordered[1] == ordered[2],
          "Action order/repetition lost");
    check(&profile.file("mods\\FIXTURE\\create.xml") == ordered[0],
          "Logical profile lookup changed case/slashes");
    check(profile.action_files(package, "OnGetDLLPath", "SetDllPath").empty(), "Unexpected DLL action");
    const auto missing = profile.action_plan(package, "OnModActivated", "OtherAction");
    check(missing.size() == 1 && !missing[0].resolved &&
              missing[0].requested_path.ends_with("not_database.lua"),
          "Missing action disappeared from ordered plan");
    rejects([&] { profile.action_files(package, "OnModActivated", "OtherAction"); });
    rejects([&] { profile.mod("absent"); });
    rejects([&] { profile.file("MODS/Fixture/../secret"); });
    rejects([&] { profile.file("MODS/Fixture/Missing.lua"); });
    const auto copied_package = package;
    rejects([&] { profile.mod_file(copied_package, "Create.xml"); });
    content::RulesDatabase database;
    for (const auto *source : ordered)
        content::apply_profile_database_file(database, *source);
    database.query({"SELECT ID, Value FROM State", {}}, [](auto row) {
        check(std::get<std::int64_t>(row[0]) == 4294967297LL && std::get<std::int64_t>(row[1]) == 12,
              "Profile-bound rules did not execute in manifest order with 64-bit "
              "values");
    });
    auto asset_stream = ordered[0]->open();
    std::array<std::uint8_t, 9> signature{};
    check(asset_stream.read(signature) == signature.size() &&
              std::string(signature.begin(), signature.end()) == "<GameData",
          "Graphics/shared asset stream did not use the profile binding");
    const auto view = content::publish_vp_542_profile_view(profile, root / "view");
    check(view == fs::absolute(root / "view").lexically_normal() &&
              runtime::identify_file(view / "MODS/Fixture/Create.xml").sha256 ==
                  runtime::identify_file(xml_path).sha256,
          "Package/VFS view did not use the selected profile content");
    check(content::publish_vp_542_profile_view(profile, root / "view") == view,
          "Verified profile view was not reusable");
    write(view / "MODS/Fixture/Create.xml", "changed view");
    rejects([&] { content::publish_vp_542_profile_view(profile, root / "view"); });
    write(sql_path, "DELETE FROM State;");
    rejects([&] { content::apply_profile_database_file(database, *ordered[1]); });
    database.query({"SELECT count(*) FROM State", {}}, [](auto row) {
        check(std::get<std::int64_t>(row[0]) == 1, "Changed profile source executed before verification");
    });
}
void file_omissions(const fs::path &root) {
    fs::create_directories(root);
    const auto manifest_path = root / "vp.modinfo";
    write(manifest_path,
          "<Mod id='vp-fixture' version='17'><Files><File import='1' md5='aa'>LUA/Missing.lua</File>"
          "<File md5='bb'>LUA/Missing.xml</File></Files></Mod>");
    const auto manifest = content::read_manifest(manifest_path);
    content::Catalog catalog;
    catalog.packages.push_back(manifest);
    content::VpProfile installer{content::VpComponent::full_eui, {}, {}, {}, 0};
    installer.installed_mods.push_back(
        {manifest, "MODS/Fixture", {"LUA/Missing.lua", "LUA/Missing.xml"}, false});
    const std::array<std::size_t, 1> selected_index{0};
    const auto selected = runtime::resolve_selected_content_profile({}, selected_index, catalog);
    check(selected.all_requirements_located && !selected.files_complete,
          "Ordinary profile accepted missing VP fixture files");
    const auto omitted = content::vp_manifest_file_omissions(installer, catalog, selected);
    check(omitted.size() == 2 && omitted[0].path == "LUA/Missing.lua" &&
              omitted[1].path == "LUA/Missing.xml",
          "VP profile lost exact Lua/XML omissions");
    const auto imported = content::vp_lua_import_omissions(installer, catalog, selected);
    check(imported.size() == 1 && imported[0].path == "LUA/Missing.lua",
          "Only imported Lua may be omitted from Lua mounts");
    const auto resolved = runtime::resolve_selected_content_profile(
        {}, selected_index, catalog, omitted);
    check(resolved.files_complete && resolved.can_activate,
          "Verified VP fixture omissions did not reach generic preflight");
    const auto changed_path = root / "changed.modinfo";
    write(changed_path, "<Mod id='vp-fixture' version='17'><Files/></Mod>");
    catalog.packages[0].path = changed_path;
    rejects([&] { (void)content::vp_manifest_file_omissions(installer, catalog, selected); });
}
void dispatch(const fs::path &root) {
    fs::create_directories(root);
    std::vector<content::ProfileFile> files;
    files.reserve(6);
    const auto source = [&](const std::string &name,
                            const std::string &text) -> const content::ProfileFile * {
        const auto path = root / name;
        write(path, text);
        files.push_back({"MODS/Fixture/" + name, path, runtime::identify_file(path)});
        return &files.back();
    };
    const auto *create = source("create.xml", "<GameData><!-------- profile -- comment --------><Table "
                                              "name='State'><Column name='ID' type='integer' "
                                              "primarykey='true'/></Table><!- - declaration --><State>/*<Row "
                                              "ID='4294967297'/>*/</WrongState></GameData>");
    const auto *fail_sql = source("prefix.sql", "INSERT INTO State VALUES(2); SELECT * FROM Missing; "
                                                "INSERT INTO State VALUES(3);");
    const auto *fail_xml =
        source("rollback.xml", "<GameData><State><Row ID='4'/><Row ID='2'/></State></GameData>");
    const auto *after = source("after.sql", "INSERT INTO State VALUES((SELECT max(ID)+1 FROM State));");
    const auto *no_fallback = source("absent.sql", "DELETE FROM State;");
    const auto *malformed = source("broken.xml", "<GameData><State>");
    const std::vector<content::ProfileAction> actions{{"create.XmL", create},
                                                      {"absent.xml", nullptr},
                                                      {"missing.lua", nullptr},
                                                      {"prefix.sQl", fail_sql},
                                                      {"rollback.xml", fail_xml},
                                                      {"broken.xml", malformed},
                                                      {std::string(300, 'x') + ".sql", after},
                                                      {"after.SQL", after}};
    content::RulesDatabase db;
    const auto report = content::apply_database_actions(db, actions);
    check(!report.all_succeeded && report.actions.size() == actions.size(),
          "Failure short-circuited actions");
    const content::DatabaseActionStatus expected[]{content::DatabaseActionStatus::applied,
                                                   content::DatabaseActionStatus::missing_source,
                                                   content::DatabaseActionStatus::unsupported_extension,
                                                   content::DatabaseActionStatus::sql_error,
                                                   content::DatabaseActionStatus::xml_error,
                                                   content::DatabaseActionStatus::xml_error,
                                                   content::DatabaseActionStatus::applied,
                                                   content::DatabaseActionStatus::applied};
    for (std::size_t i = 0; i < actions.size(); ++i) {
        check(report.actions[i].status == expected[i] &&
                  report.actions[i].requested_path == actions[i].requested_path,
              "Native action status/order mismatch");
        check(!content::database_action_status_name(expected[i]).empty(), "Missing action status name");
    }
    check(report.actions[3].sql && report.actions[3].sql->completed_statements == 1,
          "SQL failure prefix diagnostic");
    db.query({"SELECT count(*), min(ID), max(ID) FROM State", {}}, [](auto row) {
        check(std::get<std::int64_t>(row[0]) == 4 && std::get<std::int64_t>(row[1]) == 2 &&
                  std::get<std::int64_t>(row[2]) == 4294967299LL,
              "SQL prefix/XML rollback/next action/repetition/64-bit IDs diverged");
    });
    check(content::apply_database_actions(db, {}).all_succeeded, "Empty action list");
    rejects([&] { content::apply_database_actions(db, std::span(actions).first(1), 1); });
    // Integrity errors stop the replay; missing XML never chooses adjacent SQL.
    write(no_fallback->source, "SELECT 1;");
    const std::vector<content::ProfileAction> changed{{"absent.sql", no_fallback}, {"after.sql", after}};
    rejects([&] { content::apply_database_actions(db, changed); });
    db.query({"SELECT count(*) FROM State", {}}, [](auto row) {
        check(std::get<std::int64_t>(row[0]) == 4, "Ran a later action after integrity failure");
    });
}
void catalog_overlay(const fs::path &root) {
    const auto original=root/"game/Assets/DLC/Expansion2/Expansion2.Civ5Pkg";
    const auto view=root/"view";
    const auto replacement=view/"Game/Assets/DLC/Expansion2/Expansion2.Civ5Pkg";
    const auto new_dlc=view/"Game/Assets/DLC/VPUI/VPUI.Civ5Pkg";
    fs::create_directories(original.parent_path());
    fs::create_directories(replacement.parent_path());
    fs::create_directories(new_dlc.parent_path());
    const std::string prefix="<Civ5Package><GUID>{6DA07636-4123-4018-B643-6575B4EC336B}</GUID>"
                             "<Version>1</Version>";
    write(original,prefix+"<AudioGameData>old.xml</AudioGameData></Civ5Package>");
    write(replacement,prefix+"<AudioGameData>vp.xml</AudioGameData></Civ5Package>");
    write(new_dlc,"<Civ5Package><GUID>{11111111-2222-3333-4444-555555555555}</GUID>"
                  "<Version>1</Version><AudioGameData>new.xml</AudioGameData></Civ5Package>");
    const auto original_identity=runtime::identify_file(original);
    content::VpProfile profile{content::VpComponent::full,{}, {}, {},0};
    profile.files.emplace("game/assets/dlc/expansion2/expansion2.civ5pkg",
        content::ProfileFile{"Game/Assets/DLC/Expansion2/Expansion2.Civ5Pkg",replacement,
                             runtime::identify_file(replacement)});
    profile.files.emplace("game/assets/dlc/vpui/vpui.civ5pkg",
        content::ProfileFile{"Game/Assets/DLC/VPUI/VPUI.Civ5Pkg",new_dlc,
                             runtime::identify_file(new_dlc)});
    content::Catalog catalog;
    catalog.packages.push_back(content::read_manifest(original));
    check(content::apply_vp_542_catalog_overlays(catalog,profile,root/"game",view)==2,
          "Installer descriptor was not selected");
    content::PackageResourceQuery query;query.selector="AudioGameData";
    const auto audio=content::package_resources(catalog.packages.front(),query);
    check(audio.size()==1&&audio.front().value=="vp.xml"&&catalog.packages.front().path==fs::canonical(original),
          "VP sound descriptor was not bound to the original DLC identity");
    check(runtime::identify_file(original).sha256==original_identity.sha256,
          "Installer catalog overlay modified the original game");
    check(catalog.packages.size()==2&&catalog.packages.back().path==fs::canonical(new_dlc)&&
              !fs::exists(root/"game/Assets/DLC/VPUI"),
          "Installer-only DLC was not supplied from the profile view");
    write(replacement,prefix+"<AudioGameData>tampered.xml</AudioGameData></Civ5Package>");
    rejects([&]{content::apply_vp_542_catalog_overlays(catalog,profile,root/"game",view);});
}
} // namespace
int main() {
    try {
        run(fs::temp_directory_path() /
            ("civis-profile-test-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())));
        file_omissions(fs::temp_directory_path() /
                       ("civis-profile-omissions-test-" +
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())));
        dispatch(fs::temp_directory_path() /
                 ("civis-dispatch-test-" +
                  std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())));
        catalog_overlay(fs::temp_directory_path() /
                        ("civis-overlay-test-" +
                         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())));
        std::cout << "PASS " << checks << " content profile checks; runtime_bits=" << sizeof(void *) * 8
                  << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL after " << checks << ": " << error.what() << '\n';
        return 1;
    }
}
