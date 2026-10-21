#include "civ5/ui/frontend_content.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace civ5;
namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
void need(bool value, const char *message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
void write(const fs::path &path, const std::string &text) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << text;
    if (!file) throw std::runtime_error("Cannot write content fixture");
}
content::PackageManifest mod(const fs::path &path, const std::string &id,
        std::uint64_t version = 1, const std::string &dependencies = {},
        const std::string &declared = {}, bool present = true) {
    std::string files;
    if (!declared.empty()) {
        if (present) write(path.parent_path() / declared, "<GameData/>\n");
        const auto md5 = present ? runtime::file_md5(path.parent_path() / declared) : std::string(32, '0');
        files = "<Files><File md5=\"" + md5 + "\" import=\"1\">" + declared + "</File></Files>";
    }
    write(path, "<Mod id=\"" + id + "\" version=\"" + std::to_string(version) +
        "\"><Properties><Name>" + id + "</Name></Properties>" + files + dependencies + "</Mod>");
    return content::read_manifest(path);
}
void imported(const fs::path &path, const saves::SaveHeader &header) {
    saves::CivisSaveDocument document;
    document.flags = 1;
    document.sections.push_back({static_cast<std::uint32_t>(saves::CivisSaveSectionId::imported_payload), 0, {1, 2, 3}});
    document.sections.push_back(saves::make_save_metadata_section({std::string(64, 'a'), header}));
    saves::write_civis_save(path, document);
}
void header_only(const fs::path &path, const saves::SaveHeader &header) {
    // A real supported retail header without a gameplay payload. Selection
    // may inspect this metadata; a strict load must still require the payload.
    std::string bytes = "CIV5";
    const auto word = [&](std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) bytes.push_back(static_cast<char>(value >> (8 * i)));
    };
    const auto string = [&](const std::string &value) {
        word(static_cast<std::uint32_t>(value.size())); bytes += value;
    };
    word(8); string("1.0.3.279 (403694)"); string("403694");
    word(static_cast<std::uint32_t>(header.turn)); bytes.push_back(0);
    for (const auto *value : {&header.civilization, &header.handicap, &header.starting_era,
         &header.current_era, &header.speed, &header.world_size, &header.map_source}) string(*value);
    word(static_cast<std::uint32_t>(header.dlcs.size()));
    for (const auto &dlc : header.dlcs) {
        bytes.append(reinterpret_cast<const char *>(dlc.wire_id.data()), dlc.wire_id.size());
        word(dlc.wire_value); string(dlc.name);
    }
    word(static_cast<std::uint32_t>(header.mods.size()));
    for (const auto &mod : header.mods) { string(mod.id); word(mod.wire_value); string(mod.name); }
    write(path, bytes);
}
bool ready(const ui::FrontendContentCheck &check) { return check.result.status == ui::UiCommandStatus::ready; }
bool reason(const ui::FrontendContentCheck &check, std::string_view text) {
    return check.result.reason.find(text) != std::string::npos;
}
ui::FrontendRequest start(std::vector<std::string> packages = {}) {
    ui::FrontendRequest request;
    request.operation = "start_game";
    request.draft.packages = std::move(packages);
    return request;
}
ui::FrontendRequest load(const ui::FrontendCatalogContentAdapter &adapter,
                        std::size_t index, std::vector<std::string> packages = {}) {
    auto request = start(std::move(packages));
    request.operation = "load_game";
    request.save = adapter.data().saves.at(index);
    request.draft.save = request.save->id;
    return request;
}
void synthetic(const fs::path &root) {
    const std::string base_id = "11111111-1111-1111-1111-111111111111";
    const std::string main_id = "22222222-2222-2222-2222-222222222222";
    const auto base = mod(root / "base/base.modinfo", base_id, 1, {}, "rules.xml");
    const auto main = mod(root / "main/main.modinfo", main_id, 1,
        "<Dependencies><Mod id=\"" + base_id + "\" minversion=\"1\" maxversion=\"1\"/></Dependencies>");
    const auto extra = mod(root / "extra/extra.modinfo", "semper-fixture", 1);
    const auto missing = mod(root / "missing/missing.modinfo", "missing-file", 1, {}, "missing.xml", false);
    const auto native = mod(root / "native/native.modinfo", "native-mod", 1, {}, "GameCore.dll", false);
    auto catalog = std::make_shared<content::Catalog>();
    catalog->packages = {base, main, extra, missing, native};
    const auto key = ui::FrontendCatalogContentAdapter::package_key;
    saves::SaveHeader header;
    header.turn = 7;
    header.civilization = "CIVILIZATION_AMERICA";
    header.mods = {{base.id, base.name, 1}, {main.id, main.name, 1}};
    imported(root / "selected.CivisSave", header);
    auto bad_version = header; bad_version.mods.back().wire_value = 99;
    imported(root / "wrong-version.CivisSave", bad_version);
    auto absent_mod = header; absent_mod.mods.push_back({"absent-mod", "Absent", 1});
    imported(root / "missing-mod.CivisSave", absent_mod);
    write(root / "broken.CivisSave", "corrupt container");
    write(root / "broken.Civ5Save", "corrupt header");
    ui::FrontendCatalogContentAdapter adapter(catalog, {root / "selected.CivisSave",
        root / "wrong-version.CivisSave", root / "missing-mod.CivisSave", root / "broken.CivisSave",
        root / "broken.Civ5Save", root / "selected.CivisSave"});
    need(adapter.data().packages.size() == 5 && adapter.data().saves.size() == 5,
        "Adapter hid installed packages or duplicated explicit save rows");
    const auto vanilla = adapter.check_content(start());
    need(ready(vanilla) && vanilla.required_packages.empty() && reason(vanilla, "preflight only"),
        "Empty selection is not a vanilla content preflight");
    const auto dependency = adapter.check_content(start({key(main)}));
    need(ready(dependency) && dependency.required_packages == std::vector<std::string>{key(base), key(main)},
        "Shared dependency resolver was not reused in dependency order");
    need(reason(adapter.check_content(start({key(missing)})), "files missing"), "Missing mod files accepted");
    need(reason(adapter.check_content(start({key(native)})), "requires a Civis port"),
        "Missing native DLL was misclassified as supported or ordinary missing content");
    need(!ready(adapter.check_content(start({"untrusted-key"}))), "Unknown package key accepted");
    need(!ready(adapter.check_content(start({key(main), key(main)}))), "Duplicate selected package accepted");
    need(ready(adapter.check_content(start({key(extra)}))), "Authored non-RFC mod ID was rejected");
    const auto correct = adapter.check_content(load(adapter, 0, {key(main), key(base)}));
    need(ready(correct) && correct.required_packages.size() == 2, "Legitimate Civis save preflight failed");
    const auto extras = adapter.check_content(load(adapter, 0, {key(main), key(base), key(extra)}));
    need(!ready(extras) && extras.required_packages.size() == 2 && reason(extras, "differs"),
        "Extra installed content was silently activated for a save");
    need(reason(adapter.check_content(load(adapter, 0)), "differs"), "Required save content silently enabled");
    need(reason(adapter.check_content(load(adapter, 1)), "version mismatch"), "Saved mod version auto-upgraded");
    need(reason(adapter.check_content(load(adapter, 2)), "package missing"), "Absent saved package accepted");
    need(!ready(adapter.check_content(load(adapter, 3))) && !ready(adapter.check_content(load(adapter, 4))),
        "Corrupt save accepted as usable payload");
    auto injected = load(adapter, 0); injected.save->path = root / "wrong-version.CivisSave";
    need(reason(adapter.check_content(injected), "identity/path mismatch"), "Caller replaced an explicit slot path");
    auto no_save = start(); no_save.operation = "load_game";
    need(!ready(adapter.check_content(no_save)), "Load without selected save passed");
    auto retained = start({key(extra)}); retained.draft.save = adapter.data().saves.front().id;
    retained.save = adapter.data().saves.front();
    need(ready(adapter.check_content(retained)), "Retained save selection contaminated new-game content");
    auto variants = std::make_shared<content::Catalog>(*catalog);
    const auto variant = mod(root / "variant/main.modinfo", main_id);
    variants->packages.push_back(variant);
    need(key(variant) != key(main), "Same-ID/version source paths collapsed");
    ui::FrontendCatalogContentAdapter ambiguous(variants, {root / "selected.CivisSave"});
    need(reason(ambiguous.check_content(load(ambiguous, 0)), "ambiguous"), "Saved source ambiguity silently resolved");
    need(ready(ambiguous.check_content(load(ambiguous, 0, {key(main), key(base)}))),
        "Explicit saved mod source pin was not honored");
    auto missing_dependency = std::make_shared<content::Catalog>(); missing_dependency->packages = {main};
    ui::FrontendCatalogContentAdapter absent(missing_dependency);
    need(reason(absent.check_content(start({key(main)})), "package missing"), "Dependency absence ignored");
    auto mismatched_dependency = std::make_shared<content::Catalog>();
    mismatched_dependency->packages = {main, mod(root / "v2/base.modinfo", base_id, 2)};
    ui::FrontendCatalogContentAdapter mismatch(mismatched_dependency);
    need(reason(mismatch.check_content(start({key(main)})), "version mismatch"), "Dependency version range ignored");
    write(root / "selected.CivisSave", "changed bytes");
    need(reason(adapter.check_content(load(adapter, 0, {key(main), key(base)})), "Save changed"),
        "Changed save bytes were not invalidated");
    write(base.path, "changed manifest");
    need(reason(adapter.check_content(start({key(base)})), "manifest changed"), "Stale package manifest accepted");
    need(adapter.data().saves.front().path == fs::absolute(root / "selected.CivisSave").lexically_normal(),
        "Explicit slot path changed");
}
void cached_selection(const fs::path &root) {
    const auto key = ui::FrontendCatalogContentAdapter::package_key;
    const auto base = mod(root / "base/base.modinfo", "cached-base", 1);
    const auto variant = mod(root / "variant/base.modinfo", "cached-base", 1);
    const auto newer = mod(root / "newer/base.modinfo", "cached-base", 2);
    const auto extra = mod(root / "extra/extra.modinfo", "cached-extra", 1);
    const auto native = mod(root / "native/native.modinfo", "cached-native", 1, {}, "GameCore.dll", false);
    const auto declared_missing = mod(root / "declared-missing/mod.modinfo", "cached-file-missing", 1, {}, "absent.xml", false);
    auto manifest_missing = base;
    manifest_missing.id = "cached-manifest-missing"; manifest_missing.path = root / "absent.modinfo";
    auto catalog = std::make_shared<content::Catalog>();
    catalog->packages = {base, variant, newer, extra, native, declared_missing, manifest_missing};
    saves::SaveHeader header;
    header.turn = 9; header.civilization = "CIVILIZATION_AMERICA";
    header.mods = {{base.id, base.name, 1}};
    header_only(root / "selected.Civ5Save", header);
    imported(root / "unknown.CivisSave", header);
    write(root / "broken.Civ5Save", "broken header");
    auto missing_dlc = header;
    saves::DlcRequirement dlc;
    dlc.wire_id.fill(0x33); dlc.id = saves::windows_guid(dlc.wire_id);
    dlc.name = "Absent DLC"; dlc.wire_value = 1;
    missing_dlc.dlcs.push_back(dlc);
    header_only(root / "missing-dlc.Civ5Save", missing_dlc);
    auto missing_mod = header; missing_mod.mods.push_back({"absent-mod", "Absent Mod", 1});
    header_only(root / "missing-mod.Civ5Save", missing_mod);
    auto missing_version = header; missing_version.mods.front().wire_value = 99;
    header_only(root / "wrong-version.Civ5Save", missing_version);
    ui::FrontendCatalogContentAdapter adapter(catalog, {root / "selected.Civ5Save",
        root / "unknown.CivisSave", root / "broken.Civ5Save", root / "missing-dlc.Civ5Save",
        root / "missing-mod.Civ5Save", root / "wrong-version.Civ5Save", root / "absent.Civ5Save"});
    const auto &rows = adapter.data().packages;
    need(rows[0].selection_identity == "mod:cached-base" &&
         rows[0].selection_identity == rows[1].selection_identity &&
         rows[0].selection_identity == rows[2].selection_identity,
         "Cached source/version variants were not grouped by actual owner identity");
    need(rows[0].key != rows[1].key && rows[0].source_detail != rows[1].source_detail && rows[2].version == 2,
         "Cached variant grouping lost source/version distinction");
    auto no_save = start(); no_save.operation = "load_game";
    need(reason(adapter.check_selection(no_save), "No save selected"), "Cached load without selection accepted");
    need(reason(adapter.check_selection(load(adapter, 0)), "ambiguous"),
        "Cached same-ID/version variants silently resolved without explicit source");
    const auto pinned = load(adapter, 0, {key(base)});
    const auto before = adapter.check_selection(pinned);
    need(ready(before) && before.requirements_known && before.required_packages == std::vector<std::string>{key(base)} &&
         reason(before, "Selection only"), "Explicit cached mod source choice lost selection-only boundary");
    const auto other = adapter.check_selection(load(adapter, 0, {key(variant)}));
    need(ready(other) && other.required_packages == std::vector<std::string>{key(variant)},
        "Cached source choice used filesystem preference instead of stable selected key");
    need(reason(adapter.check_selection(load(adapter, 0, {key(newer)})), "version mismatch"),
        "Cached chosen mod source silently upgraded saved version");
    need(reason(adapter.check_selection(load(adapter, 0, {key(base), key(extra)})), "differs"),
        "Cached save accepted unrelated selected content");
    need(reason(adapter.check_selection(start({key(base), key(variant)})), "Multiple selected"),
        "Cached new game accepted duplicate package sources");
    need(reason(adapter.check_selection(start({key(base), key(newer)})), "Multiple selected"),
        "Cached new game accepted two versions of one identity");
    need(!ready(adapter.check_selection(start({"unknown-key"}))), "Cached query trusted an unknown stable key");
    need(reason(adapter.check_selection(start({key(native)})), "requires a Civis port"),
        "Cached native declaration became an activation capability");
    need(reason(adapter.check_selection(start({key(manifest_missing)})), "manifest unavailable"),
        "Cached manifest read failure was forgotten during presentation");
    need(ready(adapter.check_selection(start({key(declared_missing)}))) &&
         reason(adapter.check_content(start({key(declared_missing)})), "files missing"),
        "Cached selection either inspected package files or claimed full content validation");
    const auto new_game = adapter.check_selection(start({key(extra)}));
    need(ready(new_game) && new_game.required_packages == std::vector<std::string>{key(extra)},
        "Cached new-game selection did not preserve explicit keys");
    need(!ready(adapter.check_selection(load(adapter, 2))) && !ready(adapter.check_selection(load(adapter, 6))),
        "Cached malformed/missing save metadata snapshot accepted");
    need(reason(adapter.check_selection(load(adapter, 3, {key(base)})), "package missing"),
        "Cached known required DLC absence accepted");
    need(reason(adapter.check_selection(load(adapter, 4, {key(base)})), "package missing"),
        "Cached known required mod absence accepted");
    need(reason(adapter.check_selection(load(adapter, 5)), "version mismatch"),
        "Cached missing exact saved version accepted");
    auto injected = pinned; injected.save->path = root / "unknown.CivisSave";
    need(reason(adapter.check_selection(injected), "identity/path mismatch"), "Cached caller changed explicit slot path");
    injected = pinned; injected.draft.save = adapter.data().saves.at(1).id;
    need(reason(adapter.check_selection(injected), "identity/path mismatch"), "Cached caller changed explicit slot identity");
    const auto unknown = adapter.check_selection(load(adapter, 1, {key(base)}));
    need(ready(unknown) && !unknown.requirements_known && unknown.required_packages.empty(),
        "Civis missing cached header was treated as known empty requirements");
    need(ready(adapter.check_content(load(adapter, 1, {key(base)}))),
        "Cheap unknown Civis requirements bypassed or broke the original strict content checker");
    // Files intentionally become corrupt after capture. Presentation must stay
    // unchanged, while the original Source3 effect preflight rejects them.
    write(root / "selected.Civ5Save", "changed save bytes");
    write(base.path, "changed manifest bytes");
    const auto after = adapter.check_selection(pinned);
    need(after.result.status == before.result.status && after.result.reason == before.result.reason &&
         after.requirements_known == before.requirements_known && after.required_packages == before.required_packages,
         "Cached presentation reread changed files or re-resolved their sources");
    need(reason(adapter.check_content(pinned), "Save changed"),
        "Original strict Source3 effect preflight ignored mutated selected save");
    need(reason(adapter.check_content(start({key(base)})), "manifest changed"),
        "Original strict Source3 effect preflight ignored mutated manifest");
}

void package_selection_metadata(const fs::path &root) {
    const std::string id = "570f793e-fc33-417f-ae19-7cecccdd5f85";
    const auto dlc = [&](const fs::path &path, const std::string &guid,
                         const std::string &skin, unsigned version) {
        write(path, "<Civ5Package><GUID>" + guid + "</GUID><Version>" + std::to_string(version) +
            "</Version><Name><Value language=\"en_US\">Enhanced User Interface</Value></Name>" +
            "<UISkin set=\"" + skin + "\"/></Civ5Package>");
        return content::read_manifest(path);
    };
    const auto first = dlc(root / "Assets/DLC/UI_bc1/EUI_0.Civ5Pkg",
        "{570F793E-FC33-417F-AE19-7CECCCDD5F85}", "BaseGame", 1);
    const auto second = dlc(root / "Assets/DLC/UI_bc1/EUI_1.Civ5Pkg", id, "Expansion1", 1);
    const auto newer = dlc(root / "Assets/DLC/UI_bc1/EUI_2.Civ5Pkg", id, "Expansion2", 2);
    const auto same_mod = mod(root / "Mods/Selected.modinfo", id, 1);
    auto catalog = std::make_shared<content::Catalog>();
    catalog->packages = {first, second, newer, same_mod};
    ui::FrontendCatalogContentAdapter adapter(catalog);
    const auto &rows = adapter.data().packages;
    need(rows[0].selection_identity == "dlc:" + id &&
         rows[0].selection_identity == rows[1].selection_identity &&
         rows[0].selection_identity == rows[2].selection_identity,
         "Source/version variants did not publish the same canonical owner identity");
    need(rows[3].selection_identity == "mod:" + id &&
         rows[3].selection_identity != rows[0].selection_identity,
         "Mod and DLC identities were conflated");
    need(rows[0].source_detail == "UI_bc1/EUI_0.Civ5Pkg (BaseGame)" &&
         rows[1].source_detail == "UI_bc1/EUI_1.Civ5Pkg (Expansion1)" &&
         rows[2].source_detail == "UI_bc1/EUI_2.Civ5Pkg (Expansion2)",
         "Actual manifest path/skin metadata was not displayed distinctly");
    need(rows[0].source_label == rows[0].source_detail && rows[1].source_label == rows[1].source_detail &&
         rows[2].source_label == rows[2].source_detail && rows[3].source_label == "Mods/Selected.modinfo",
         "Short row captions lost the actual source filename or authored skin");
    need(rows[3].source_detail != rows[3].source_label &&
         rows[3].source_detail.find(assets::path_utf8(root.filename())) != std::string::npos,
         "Compact row captions discarded the detailed source identity");
    need(rows[0].key != rows[1].key && rows[1].key != rows[2].key && rows[2].version == 2,
         "Presentation grouping replaced genuine per-source/version keys");
    need(reason(adapter.check_selection(start({rows[0].key, rows[1].key})), "Multiple selected") &&
         reason(adapter.check_selection(start({rows[0].key, rows[2].key})), "Multiple selected"),
         "Existing duplicate source/version guard was bypassed");
    need(ready(adapter.check_selection(start({rows[0].key, rows[3].key}))),
         "An unrelated package kind was rejected as a duplicate");

    const auto alpha = mod(root / "alpha/Repeated/Shared.modinfo", "repeated-source", 1);
    const auto beta = mod(root / "beta/Repeated/Shared.modinfo", "repeated-source", 1);
    auto repeated_catalog = std::make_shared<content::Catalog>();
    repeated_catalog->packages = {alpha, beta};
    ui::FrontendCatalogContentAdapter repeated(repeated_catalog);
    const auto &variants = repeated.data().packages;
    need(variants[0].source_label == "alpha/Repeated/Shared.modinfo" &&
         variants[1].source_label == "beta/Repeated/Shared.modinfo",
         "Colliding short paths did not expand to distinguish the real sources");
    need(variants[0].source_detail.find(variants[0].source_label) != std::string::npos &&
         variants[1].source_detail.find(variants[1].source_label) != std::string::npos &&
         variants[0].key != variants[1].key,
         "Source label shortening replaced the real source paths or keys");
    need(reason(repeated.check_selection(start({variants[0].key, variants[1].key})), "Multiple selected"),
         "Compact source labels weakened duplicate-source admission");

    const auto multi_path = root / "Assets/DLC/Expansion2/Expansion2.Civ5Pkg";
    write(multi_path, "<Civ5Package><GUID>6da07636-4123-4018-b643-6575b4ec336b</GUID>"
        "<Version>1</Version><Name><Value language=\"en_US\">Brave New World</Value></Name>"
        "<UISkin set=\"Expansion2\" platform=\"Common\"/>"
        "<UISkin set=\"Expansion2\" platform=\"Tablet\"/>"
        "<UISkin set=\"BaseGame\" platform=\"Common\"/></Civ5Package>");
    auto multi_catalog = std::make_shared<content::Catalog>();
    multi_catalog->packages = {content::read_manifest(multi_path)};
    ui::FrontendCatalogContentAdapter multi(multi_catalog);
    const auto &multi_row = multi.data().packages.front();
    need(multi_row.availability.status == ui::UiCommandStatus::ready &&
         multi_row.source_label == "Expansion2/Expansion2.Civ5Pkg (Expansion2, BaseGame)" &&
         multi_row.source_detail == multi_row.source_label,
         "Multiple authored UISkins broke the catalog or silently discarded a declared set");
}

void actual(const fs::path &game_root, const fs::path &save_path) {
    auto catalog = std::make_shared<content::Catalog>(content::discover_packages({game_root / "Assets/DLC"}));
    ui::FrontendCatalogContentAdapter adapter(catalog, {save_path});
    const auto loaded = saves::read_loaded_save(save_path);
    const auto profile = runtime::resolve_saved_content_profile(loaded.header, *catalog);
    need(profile.can_activate && !profile.native_mods_require_port, "Expected ordinary executable-source save fixture");
    std::vector<std::string> required;
    for (const auto &entry : profile.requirements)
        if (entry.package) required.push_back(ui::FrontendCatalogContentAdapter::package_key(catalog->packages.at(*entry.package)));
    const auto check = adapter.check_content(load(adapter, 0, required));
    if (!ready(check)) std::cerr << "Actual preflight unavailable: " << check.result.reason << '\n';
    need(ready(check) && check.required_packages == required, "Real Civ5 save/profile diverged from shared preflight");
    need(adapter.data().saves.front().name == assets::path_utf8(save_path.stem()), "Actual save row name missing");
    const auto selection = adapter.check_selection(load(adapter, 0, required));
    need(ready(selection) && selection.requirements_known && selection.required_packages == required,
        "Actual cached header requirements diverged from strict content preflight");
    std::cout << "Real save content requirements: " << required.size() << '\n';
}
} // namespace
int main(int argc, char **argv) {
    const auto root = fs::current_path() / "frontend-content-fixtures" /
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    try {
        fs::create_directories(root);
        synthetic(root);
        cached_selection(root / "cached");
        package_selection_metadata(root / "selection-metadata");
        if (argc == 3) actual(fs::path(argv[1]), fs::path(argv[2]));
        fs::remove_all(root);
        std::cout << "Frontend content checks passed: " << checks << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
