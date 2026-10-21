#include "civ5/content/base_gameplay.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#ifdef _WIN32
#include "civ5/platform/windows_path.hpp"
#endif
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace civ5::assets;
namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
void check(bool okay, const char *message) {
    ++checks;
    if (!okay)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action, std::source_location at = std::source_location::current()) {
    bool failed = false;
    try {
        action();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, ("Expected failure at " + std::to_string(at.line())).c_str());
}
void write(const fs::path &path, const std::string &text) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
    if (!output)
        throw std::runtime_error("Fixture write failed");
}
std::string bytes(const AssetLookup &lookup) {
    const auto *handle = std::get_if<AssetHandle>(&lookup);
    check(handle != nullptr, "Expected resource handle");
    const auto data = handle->read_bytes();
    return {data.begin(), data.end()};
}
std::string bytes(const AssetHandle &handle) {
    const auto data = handle.read_bytes();
    return {data.begin(), data.end()};
}
void run(const fs::path &root) {
    write(root / "first/Rows.xml", "first");
    write(root / "second/rows.XML", "second");
    write(root / "second/after.xml", "after");
    write(root / "second/Scenarios/secret.xml", "scenario");
    write(root / "second/Sounds/Speech/en_US/speech.xml", "speech");
    write(root / "second/Sounds/Streamed/music.xml", "music");
    write(root / "second/excluded/excluded.xml", "excluded");
    write(root / "second/excluded-sibling/sibling.xml", "sibling");
    auto index = std::make_shared<LooseResourceIndex>(root);
    index->scan({"first", {}, true});
    index->scan({"second", {"second/excluded"}, true});
    check(bytes(index->find(AssetKey("ROWS.XML"))) == "second", "Later loose source must win");
    check(bytes(index->find(AssetKey("any/path/Rows.xml"))) == "second", "Native basename query");
    const auto all_rows = index->find_all(AssetKey("rows.xml"));
    check(all_rows.size() == 2 && bytes(all_rows[0]) == "first" && bytes(all_rows[1]) == "second",
          "TextData all-match must retain loose insertion order");
    check(bytes(index->find(AssetKey("C:\\old\\Rows.xml"))) == "second", "Absolute native basename query");
    check(bytes(index->find(AssetKey("music.xml"))) == "music", "Speech exclusion hid streamed music");
    check(bytes(index->find(AssetKey("sibling.xml"))) == "sibling", "Directory exclusion crossed boundary");
    check(std::holds_alternative<std::monostate>(index->find(AssetKey("secret.xml"))), "Scenario indexed");
    check(std::holds_alternative<std::monostate>(index->find(AssetKey("speech.xml"))), "Speech indexed");
    check(std::holds_alternative<std::monostate>(index->find(AssetKey("excluded.xml"))),
          "Excluded path indexed");
    check(index->indexed_files() == 5 && index->unique_names() == 4, "Unexpected loose record counts");
#ifdef _WIN32
    const auto segment=std::string(55,'x');
    const auto deep_file=root/"deep"/segment/segment/segment/segment/"deep.xml";
    check(deep_file.native().size()>260,"Deep resource fixture is too short");
    fs::create_directories(civ5::platform::extended_windows_path(deep_file.parent_path()));
    {
        std::ofstream output(civ5::platform::extended_windows_path(deep_file),std::ios::binary);
        output<<"deep";
        check(bool(output),"Cannot create deep resource fixture");
    }
    auto deep_index=std::make_shared<LooseResourceIndex>(root);
    deep_index->scan({"deep",{},false});
    check(bytes(deep_index->find(AssetKey("deep.xml")))=="deep",
          "Deep cached mod asset could not be indexed and read");
#endif
    index->scan({"absent/directory", {}, true});
    check(index->indexed_files() == 5, "Absent declared directory changed the index");
    index->add_file("first/Rows.xml");
    check(bytes(index->find(AssetKey("rows.xml"))) == "first", "Explicit later file must replace");
    check(resource_basename(std::string(8193, 'A') + ".XML").size() == 8197,
          "Legacy filename buffer retained");
    rejects([&] { resource_basename(std::string("Rows\0.xml", 9)); });
    rejects([&] { index->scan({root.parent_path(), {}, false}); });
    rejects([&] { index->scan({"first", {root.parent_path()}, false}); });
    write(root.parent_path() / "outside.xml", "outside");
    rejects([&] { index->add_file(root.parent_path() / "outside.xml"); });
    const auto stale = index->find(AssetKey("rows.xml"));
    write(root / "first/Rows.xml", "changed-length");
    rejects([&] { bytes(stale); });
    write(root / "first/Rows.xml", "first");

    auto empty = std::make_shared<LooseResourceIndex>(root);
    auto first = std::make_shared<LooseResourceIndex>(root);
    auto second = std::make_shared<LooseResourceIndex>(root);
    first->add_file("first/Rows.xml");
    second->add_file("second/rows.XML");
    const AssetKey key("rows.xml");
    ResourceMountTable table(resource_lookup(first), {}, resource_keys(first));
    table.mount({1, 0, 0, resource_lookup(first), true, true, {}, resource_keys(first)});
    table.mount({4294967297ULL, 0, 0, resource_lookup(second), true, true, {}, resource_keys(second)});
    auto hit = table.resolve(key);
    check(hit.mount_id == 4294967297ULL && bytes(hit.asset) == "second", "Reverse mounted lookup or wide ID");
    const auto table_keys = table.keys();
    check(table_keys == std::vector<std::string>{"first/Rows.xml", "second/rows.XML"},
          "Resource mount key enumeration lost active source names");
    ResourceMountTable all_table(resource_lookup(first), resource_lookup_all(first));
    all_table.mount({9, 0, 0, resource_lookup(second), true, true, resource_lookup_all(second)});
    const auto all_mounted = all_table.resolve_all(key);
    check(all_mounted.size() == 2 && !all_mounted[0].mount_id && all_mounted[1].mount_id == 9 &&
              bytes(all_mounted[0].asset) == "first" && bytes(all_mounted[1].asset) == "second",
          "TextData all-match must retain default then mount order");
    all_table.set_enabled(9, false);
    check(all_table.resolve_all(key).size() == 1, "Disabled mount must disappear from all-match");
    table.set_enabled(4294967297ULL, false);
    hit = table.resolve(key);
    check(hit.mount_id == 1 && bytes(hit.asset) == "first", "Disabled mount must be skipped");
    table.unmount(1);
    hit = table.resolve(key);
    check(!hit.mount_id && bytes(hit.asset) == "first", "Default fallback provenance");
    table.set_default_enabled(false);
    table.set_enabled(4294967297ULL, true);
    check(std::holds_alternative<std::monostate>(table.resolve(key).asset),
          "Default gate must disable all lookup");
    rejects([&] { table.set_enabled(99, true); });
    rejects([&] { resource_lookup({}); });

    ResourceMountTable deferred(resource_lookup(first));
    deferred.mount({1, 0, 0, resource_lookup(second), true, false});
    check(!deferred.resolve(key).mount_id, "Default must precede deferred candidates");
    ResourceMountTable no_default(resource_lookup(empty));
    no_default.mount({1, 0, 0, resource_lookup(first), true, false});
    no_default.mount({2, 0, 0, resource_lookup(second), true, false});
    check(no_default.resolve(key).mount_id == 1, "Last encountered deferred candidate must win");
    no_default.mount({1, 0, 0, resource_lookup(first)});
    check(no_default.mounts()[0].id == 1, "Re-registering zero group/priority must not reorder");
    check(no_default.resolve(key).mount_id == 1, "Immediate older source must precede deferred newer source");
    no_default.mount({2, 0, 0, resource_lookup(second, false)});
    check(no_default.resolve(key).mount_id == 1, "Per-result preference flag ignored");
    no_default.set_enabled(1, false);
    check(no_default.resolve(key).mount_id == 2, "Deferred result lost without fallback");
    check(std::holds_alternative<std::monostate>(no_default.resolve(AssetKey("absent.xml")).asset),
          "False match");
    rejects([&] { no_default.resolve(AssetKey("")); }); // Shared AssetKey rejects blank requests.

    ResourceMountTable order(resource_lookup(empty));
    const auto lookup = resource_lookup(first);
    order.mount({1, 2, 10, lookup});
    order.mount({2, 3, 100, lookup});
    order.mount({3, 2, 20, lookup});
    order.mount({4, 2, 5, lookup});
    order.mount({5, 0, 0, lookup});
    std::vector<std::uint64_t> ids;
    for (const auto &mount : order.mounts())
        ids.push_back(mount.id);
    check(ids == std::vector<std::uint64_t>{4, 1, 3, 2, 5}, "Group-local insertion is not global sorting");
    order.mount({1, 2, 4294967297ULL, lookup});
    ids.clear();
    for (const auto &mount : order.mounts())
        ids.push_back(mount.id);
    check(ids == std::vector<std::uint64_t>{4, 3, 1, 2, 5}, "Re-registration or wide priority");
    order.mount({6, 4294967297ULL, UINT64_MAX, lookup});
    check(order.resolve(key).mount_id == 6, "Wide mount group/priority truncated");

    std::vector<std::string> requests{"rows.xml", "missing.xml", "rows.xml"};
    const auto base_plan = civ5::content::plan_base_gameplay(requests, order);
    check(base_plan.size() == 3 && base_plan[0].selected() && !base_plan[1].selected() &&
              base_plan[2].selected(),
          "Base requests must preserve duplicate and missing entries");
    check(base_plan[0].resource.mount_id == 6 && bytes(base_plan[0].resource.asset) == "first",
          "Base requests bypassed shared mount lookup");
    requests.assign(1024, "rows.xml");
    check(civ5::content::plan_base_gameplay(requests, order).size() == 1024,
          "Native 111 request recipe became a resource capacity limit");
    rejects([&] { civ5::content::read_native_gameplay_recipe(root / "first/Rows.xml"); });

    // The old filesystem rejects directory indices >=4096. Exercise more real
    // directories without a packed index, using one tiny payload.
    for (unsigned i = 10000; i < 14097; ++i)
        fs::create_directories(root / "many" / std::to_string(i));
    write(root / "many/14096/wide-directory.xml", "wide");
    empty->scan({"many", {}, false});
    check(bytes(empty->find(AssetKey("wide-directory.xml"))) == "wide", "Legacy directory limit retained");

    const auto plan_path = root / "plan.xml";
    write(plan_path, "<LooseResourceMountPlan root='.'><Default><Scan directory='first'/></Default>"
                     "<Mount id='4294967297' group='0' priority='0'><Scan directory='second' "
                     "excludeScenariosAndSpeech='true'><Exclude directory='second/excluded'/></Scan></Mount>"
                     "</LooseResourceMountPlan>");
    const auto plan = civ5::content::read_loose_resource_plan(plan_path);
    check(bytes(plan.resources.resolve(key).asset) == "second", "Explicit XML mount plan ignored source");
    check(plan.indexed_files == 5, "Explicit plan counts");
    write(plan_path, "<LooseResourceMountPlan root='.'><Default/><Mount id='1' group='0' priority='0'/>"
                     "<Mount id='1' group='0' priority='0'/></LooseResourceMountPlan>");
    rejects([&] { civ5::content::read_loose_resource_plan(plan_path); });
    write(plan_path,
          "<LooseResourceMountPlan root='.' defaultEnabled='maybe'><Default/></LooseResourceMountPlan>");
    rejects([&] { civ5::content::read_loose_resource_plan(plan_path); });
    write(plan_path, "<LooseResourceMountPlan root='.'><Default><File "
                     "path='../outside.xml'/></Default></LooseResourceMountPlan>");
    rejects([&] { civ5::content::read_loose_resource_plan(plan_path); });

    // Build the launch VFS from the selected DLC list, including an overlay
    // outside the Steam root. Installed but unselected DLC must stay invisible.
    const auto game = root / "selected-game";
    const auto overlay = root / "selected-overlay";
    const auto unselected = root / "unselected-overlay";
    write(game / "Assets/Gameplay/XML/Rows.xml", "base");
    write(overlay / "CommonOnly.xml", "common");
    write(overlay / "Gameplay/Rows.xml", "selected");
    write(overlay / "UI/Secret.xml", "skin");
    write(unselected / "Gameplay/Rows.xml", "unselected");
    write(unselected / "Gameplay/Unselected.xml", "unselected");
    const std::string descriptor = "<Civ5Package><Common/><Gameplay><Directory>Gameplay</Directory>"
        "</Gameplay><UISkin><Skin><Directory>UI</Directory></Skin></UISkin></Civ5Package>";
    write(overlay / "A.Civ5Pkg", descriptor);
    write(unselected / "B.Civ5Pkg", descriptor);
    civ5::content::Catalog catalog;
    for (const auto &path : {overlay / "A.Civ5Pkg", unselected / "B.Civ5Pkg"}) {
        civ5::content::PackageManifest package;
        package.kind = civ5::content::PackageKind::dlc;
        package.path = path;
        package.document = civ5::content::read_xml_metadata(descriptor);
        catalog.packages.push_back(std::move(package));
    }
    const std::array<std::size_t, 0> none{};
    auto vanilla = civ5::content::build_selected_loose_resource_plan(game, none, catalog);
    check(bytes(vanilla.resources.resolve(AssetKey("Rows.xml")).asset) == "base" &&
              std::holds_alternative<std::monostate>(vanilla.resources.resolve(AssetKey("CommonOnly.xml")).asset),
          "Empty DLC selection leaked installed overlay resources");
    const std::array<std::size_t, 1> enabled{0};
    auto selected_dlc = civ5::content::build_selected_loose_resource_plan(game, enabled, catalog);
    check(bytes(selected_dlc.resources.resolve(AssetKey("Rows.xml")).asset) == "selected" &&
              bytes(selected_dlc.resources.resolve(AssetKey("CommonOnly.xml")).asset) == "common" &&
              std::holds_alternative<std::monostate>(selected_dlc.resources.resolve(AssetKey("Secret.xml")).asset) &&
              std::holds_alternative<std::monostate>(selected_dlc.resources.resolve(AssetKey("Unselected.xml")).asset),
          "Selected DLC VFS leaked UI or unselected package resources");
    rejects([&] { const std::array<std::size_t, 1> invalid{2};
                  civ5::content::build_selected_loose_resource_plan(game, invalid, catalog); });
}
} // namespace
int main() {
    try {
        const auto root = fs::temp_directory_path() /
                          ("civis-resource-fixtures-" +
                           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())) /
                          "assets";
        run(root);
        std::cout << "PASS " << checks << " resource mount checks; runtime_bits=" << sizeof(void *) * 8
                  << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL after " << checks << ": " << error.what() << '\n';
        return 1;
    }
}
