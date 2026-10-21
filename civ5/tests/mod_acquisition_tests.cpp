#include "civ5/content/inno_archive.hpp"
#include "civ5/content/mod_acquisition.hpp"
#include "civ5/content/mod_activation.hpp"
#include "civ5/platform/windows_path.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/save_preflight.hpp"
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <shellapi.h>
#include <source_location>
#include <span>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace civ5;
namespace {
unsigned checks = 0;
void check(bool ok, const char *reason) {
    ++checks;
    if (!ok)
        throw std::runtime_error(reason);
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
std::vector<std::uint8_t> bytes(const fs::path &path) {
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
void write(const fs::path &path, const std::vector<std::uint8_t> &data) {
    std::ofstream stream(path, std::ios::binary);
    stream.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!stream)
        throw std::runtime_error("Fixture write failed");
}
// Independent bitwise CRC for mutating the uncompressed fixture header, so
// path validation is exercised after all 7z header checksums have passed.
std::uint32_t crc(std::span<const std::uint8_t> data) {
    std::uint32_t value = UINT32_MAX;
    for (const auto byte : data) {
        value ^= byte;
        for (unsigned bit = 0; bit < 8; ++bit)
            value = (value >> 1) ^ ((value & 1) ? 0xedb88320u : 0u);
    }
    return value ^ UINT32_MAX;
}
std::uint64_t little64(const std::vector<std::uint8_t> &data, std::size_t at) {
    std::uint64_t result = 0;
    for (unsigned i = 0; i < 8; ++i)
        result |= static_cast<std::uint64_t>(data.at(at + i)) << (8 * i);
    return result;
}
void little32(std::vector<std::uint8_t> &data, std::size_t at, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
        data.at(at + i) = static_cast<std::uint8_t>(value >> (8 * i));
}
std::vector<std::uint8_t> rename_header(std::vector<std::uint8_t> data, const std::string &before,
                                        const std::string &after) {
    check(before.size() == after.size(), "Fixture name width");
    std::vector<std::uint8_t> from, to;
    for (const auto c : before) {
        from.push_back(static_cast<std::uint8_t>(c));
        from.push_back(0);
    }
    for (const auto c : after) {
        to.push_back(static_cast<std::uint8_t>(c));
        to.push_back(0);
    }
    const auto offset = little64(data, 12) + 32, size = little64(data, 20);
    check(offset <= data.size() && size == data.size() - offset, "Fixture header span");
    auto found =
        std::search(data.begin() + static_cast<std::ptrdiff_t>(offset), data.end(), from.begin(), from.end());
    check(found != data.end(), "Fixture header name absent");
    std::copy(to.begin(), to.end(), found);
    little32(data, 28,
             crc(std::span(data).subspan(static_cast<std::size_t>(offset), static_cast<std::size_t>(size))));
    little32(data, 8, crc(std::span(data).subspan(12, 20)));
    return data;
}
std::string workshop_xml(const std::string &app = "8930") {
    return "<?xml version=\"1.0\"?><!DOCTYPE "
           "response><response><result>1</result><resultcount>1</"
           "resultcount><publishedfiledetails><publishedfile><publishedfileid>"
           "4294967297</"
           "publishedfileid><result>1</result><consumer_app_id>" +
           app +
           "</consumer_app_id><filename>sample.civ5mod</"
           "filename><file_size>4294967298</"
           "file_size><hcontent_file>18446744073709551614</"
           "hcontent_file><title>Test &amp; "
           "mod</title><file_url>https://example.com/file</file_url></"
           "publishedfile></publishedfiledetails></"
           "response>";
}
void run(const fs::path &root) {
    fs::create_directories(root);
    const auto long_root = root / "long_paths";
    fs::create_directory(long_root);
    const auto segment = std::string(55, 'a');
    const auto relative = segment + "/" + segment + "/" + segment + "/" + segment + "/data.bin";
    const auto long_file = long_root / relative;
    check(fs::absolute(long_file).native().size() > 260, "Long-path fixture is too short");
    fs::create_directories(platform::extended_windows_path(long_file.parent_path()));
    write(platform::extended_windows_path(long_file), {1, 2, 3});
    content::require_plain_directory(long_file.parent_path());
    check(runtime::identify_file(content::package_file(long_root, relative)).bytes == 3,
          "Deep mod path cannot be inspected or digested");
    const fs::path data = CIVIS_TEST_DATA_DIR;
    const auto store = data / "mod-archive-store.7z", solid = data / "mod-archive-solid.7z";
    const auto zip = data / "mod-archive-mixed.zip";
    const auto deflate = data / "mod-archive-deflate.7z";
    const auto original = runtime::identify_file(store);
    const content::ModTarget target{"civis.archive.fixture", "", 7};
    for (const auto *name :
         {L"../outside", L"x/../out", L"C:\\out", L"\\server\\share", L"x//y", L"x/", L"x.", L"x ",
          L"NUL.txt", L"aux.lua", L"COM1", L"lpt9.bin", L"COM\u00b9.txt", L"a:b", L"x?", L"CONOUT$"})
        rejects([&] { content::archive_relative_path(name); });
    rejects([&] { content::archive_relative_path(std::wstring{L'a', 0, L'b'}); });
    rejects([&] { content::archive_relative_path(std::wstring{L'a', 0xd800}); });
    check(content::archive_relative_path(L"folder/\u043a\u0430\u0440\u0442\u0430.xml").filename() ==
              fs::path(L"\u043a\u0430\u0440\u0442\u0430.xml"),
          "Unicode path changed");
    const auto extracted = content::extract_mod_7z(store, root / "store");
    check(extracted.entries.size() == 3 && extracted.expanded_bytes == 276, "Store extraction size");
    check(extracted.peak_decoder_memory > 0 && extracted.peak_decoder_memory < 1024 * 1024,
          "Decoder budget accounting");
    content::extract_mod_7z(solid, root / "solid");
    for (const auto *file : {"Fixture.modinfo", "abcdef.txt", "ghijkl.txt"})
        check(runtime::identify_file(root / "store" / file).sha256 ==
                  runtime::identify_file(root / "solid" / file).sha256,
              "Store/LZMA2 results differ");
    check(runtime::file_md5(root / "store/abcdef.txt") == "b1946ac92492d2347c6235b4d2611184",
          "MD5 known vector");
    const auto deflated = content::extract_mod_7z(deflate, root / "deflate");
    check(deflated.entries.size() == 1 && deflated.expanded_bytes == 1'500'000 &&
              deflated.peak_decoder_memory > 0 &&
              runtime::identify_file(root / "deflate/rows.sql").sha256 ==
                  "67579c335f36f1e59c44493a84d33fbd68561f8049e5b09382ca9a4cd1f6c7e8",
          "7z Deflate payload changed across streaming chunks");
    auto deflate_bad = bytes(deflate);
    deflate_bad.at(32) ^= 1;
    write(root / "deflate_bad.7z", deflate_bad);
    rejects([&] { content::extract_mod_7z(root / "deflate_bad.7z", root / "deflate_bad"); });
    const auto folder = content::extract_mod_zip(data / "mod-archive-folder.zip", root / "folder");
    check(folder.entries.size() == 1 &&
              folder.entries.front().relative == fs::path(L"mod-archive-deflate.7z") &&
              runtime::identify_file(root / "folder/mod-archive-deflate.7z").sha256 ==
                  runtime::identify_file(deflate).sha256,
          "Shared-folder ZIP root marker or nested archive changed");
    const auto unzipped=content::extract_mod_zip(zip,root / "zip");
    check(unzipped.entries.size()==3&&unzipped.entries[0].relative==fs::path(L"Fixture.modinfo")&&
          unzipped.entries[1].directory&&unzipped.entries[2].relative==fs::path(L"data/rows.sql"),
          "ZIP extraction preserves stored and deflated mod files");
    check(bytes(root / "zip/data/rows.sql")==std::vector<std::uint8_t>{'S','E','L','E','C','T',' ','1',';','\n'},
          "ZIP stored file content changed");
    rejects([&]{content::extract_mod_zip(zip,root / "zip");});
    rejects([&]{content::extract_mod_zip(zip,root / "zip_over_bytes",{100,1,1024*1024});});
    rejects([&]{content::extract_mod_zip(zip,root / "zip_over_memory",{100,10000,1});});
    check(!fs::exists(root / "zip_over_bytes")&&!fs::exists(root / "zip_over_memory"),
          "Rejected ZIP budget created an extraction directory");
    const auto zip_raw=bytes(zip);
    auto zip_bad=zip_raw;
    const std::string stored_text="SELECT 1;\n";
    auto stored_at=std::search(zip_bad.begin(),zip_bad.end(),stored_text.begin(),stored_text.end());
    check(stored_at!=zip_bad.end(),"ZIP stored fixture payload absent");
    *stored_at^=1;write(root / "zip_bad_crc.zip",zip_bad);
    rejects([&]{content::extract_mod_zip(root / "zip_bad_crc.zip",root / "zip_crc");});
    zip_bad=zip_raw;
    const std::string original_name="data/rows.sql",bad_name="../outside.tx";
    check(original_name.size()==bad_name.size(),"ZIP traversal fixture name width");
    unsigned replaced=0;
    for(auto it=zip_bad.begin();(it=std::search(it,zip_bad.end(),original_name.begin(),original_name.end()))!=zip_bad.end();it+=original_name.size()){
        std::copy(bad_name.begin(),bad_name.end(),it);++replaced;}
    check(replaced==2,"ZIP local and central names not both replaced");
    write(root / "zip_traversal.zip",zip_bad);
    rejects([&]{content::extract_mod_zip(root / "zip_traversal.zip",root / "zip_traversal");});
    check(!fs::exists(root / "zip_traversal")&&!fs::exists(root / "outside.tx"),
          "ZIP traversal wrote files");
    rejects([&]{content::extract_mod_zip(data / "mod-archive-duplicate.zip",root / "zip_duplicate");});
    rejects([&]{content::extract_mod_zip(data / "mod-archive-symlink.zip",root / "zip_symlink");});
    check(!fs::exists(root / "zip_duplicate")&&!fs::exists(root / "zip_symlink"),
          "ZIP aliases or links created extraction directories");
    const auto zip64=data / "mod-archive-zip64.zip";
    const auto expanded64=content::extract_mod_zip(zip64,root / "zip64");
    check(expanded64.entries.size()==1&&expanded64.entries[0].bytes>0&&
          expanded64.entries[0].relative==fs::path(L"Fixture.modinfo"),
          "ZIP64 sizes and offsets did not decode");
    rejects([&] { content::extract_mod_7z(store, root / "store"); });
    rejects([&] { content::extract_mod_7z(store, root / "over_entries", {0, 10000, 1000000}); });
    rejects([&] { content::extract_mod_7z(store, root / "over_bytes", {100, 1, 1000000}); });
    rejects([&] { content::extract_mod_7z(store, root / "over_memory", {100, 10000, 1}); });
    check(!fs::exists(root / "over_entries") && !fs::exists(root / "over_bytes") &&
              !fs::exists(root / "over_memory"),
          "Rejected header created extraction directory");
    const auto raw = bytes(store);
    auto bad = raw;
    bad[32] ^= 1;
    write(root / "crc.7z", bad);
    rejects([&] { content::extract_mod_7z(root / "crc.7z", root / "bad_crc"); });
    for (const auto length : {0u, 6u, 31u, 40u, 465u}) {
        write(root / "short.7z", {raw.begin(), raw.begin() + length});
        rejects([&] {
            content::extract_mod_7z(root / "short.7z", root / ("truncated" + std::to_string(length)));
        });
    }
    write(root / "traversal.7z", rename_header(raw, "abcdef.txt", "../bad.txt"));
    rejects([&] { content::extract_mod_7z(root / "traversal.7z", root / "traversal"); });
    check(!fs::exists(root / "traversal") && !fs::exists(root / "bad.txt"), "Traversal wrote files");
    write(root / "duplicate.7z", rename_header(raw, "ghijkl.txt", "ABCDEF.TXT"));
    rejects([&] { content::extract_mod_7z(root / "duplicate.7z", root / "duplicate"); });
    check(!fs::exists(root / "duplicate"), "Case duplicate wrote files");
    const auto cache = root / "cache";
    const auto installed = content::install_mod_archive(store, target, cache, original.sha256);
    check(!installed.reused && installed.verified_files == 3 && installed.manifest.id == target.id &&
              installed.manifest.version == 7,
          "Install identity");
    check(fs::equivalent(installed.cache_package.parent_path(), cache / "packages"),
          "Commit escaped cache packages");
    const auto reused = content::install_mod_archive(store, target, cache);
    check(reused.reused && reused.archive_sha256 == original.sha256, "Verified cache was not reused");
    const content::ModTarget zip_target{"civis.zip.fixture","",9};
    const auto zip_identity=runtime::identify_file(zip);
    const auto zip_cache=root / "zip_cache";
    const auto zip_installed=content::install_mod_archive(zip,zip_target,zip_cache,zip_identity.sha256);
    check(!zip_installed.reused&&zip_installed.manifest.id==zip_target.id&&
          zip_installed.manifest.version==zip_target.version&&zip_installed.verified_files==2,
          "Verified ZIP mod did not publish exact manifest and files");
    check(content::install_mod_archive(zip,zip_target,zip_cache).reused,
          "Verified ZIP cache was not reusable");
    const auto long_cache_base=root/"long_cached_mod";
    const auto final_file_base=long_cache_base/"packages"/zip_identity.sha256/
        "content"/"data"/"rows.sql";
    check(final_file_base.native().size()<270,"Long cache fixture root is too deep");
    const auto long_cache=root/("long_cached_mod"+
        std::string(270-final_file_base.native().size(),'x'));
    check((long_cache/"packages"/zip_identity.sha256/"content"/"data"/"rows.sql")
              .native().size()>=270,
          "Long cached package fixture is too short");
    const auto long_installed=content::install_mod_archive(zip,zip_target,long_cache,
                                                            zip_identity.sha256);
    check(content::install_mod_archive(zip,zip_target,long_cache).reused&&
          runtime::identify_file(content::package_file(
              long_installed.manifest.path.parent_path(),"data/rows.sql")).bytes==10,
          "Long cached package did not survive receipt and manifest revalidation");
    const auto stale_archive = data / "mod-archive-stale-md5.zip";
    const auto stale_sha = runtime::identify_file(stale_archive).sha256;
    const content::ModTarget stale_target{"civis.stale.fixture", "", 11};
    rejects([&] { content::install_mod_archive(stale_archive, stale_target,
                                               root / "stale_strict", stale_sha); });
    rejects([&] { content::install_mod_archive(stale_archive, stale_target,
                                               root / "stale_unpinned", {}, {}, {}, true); });
    const auto stale_cache = root / "stale_pinned";
    const auto stale = content::install_mod_archive(stale_archive, stale_target,
                                                    stale_cache, stale_sha, {}, {}, true);
    check(stale.stale_manifest_md5_files == std::vector<std::string>{"rows.sql"} &&
              stale.verified_files == 2,
          "Pinned archive did not report its stale manifest MD5");
    rejects([&] { content::install_mod_archive(stale_archive, stale_target, stale_cache, stale_sha); });
    const auto stale_reused = content::install_mod_archive(stale_archive, stale_target,
                                                           stale_cache, stale_sha, {}, {}, true);
    check(stale_reused.reused && stale_reused.stale_manifest_md5_files ==
              std::vector<std::string>{"rows.sql"} &&
              content::find_cached_mod_by_identity(stale_cache, stale_target).has_value(),
          "Pinned stale-MD5 cache was not verified on reuse");
    auto stale_catalog = content::discover_packages({stale_cache / "packages"});
    check(stale_catalog.diagnostics.empty() && stale_catalog.packages.size() == 1,
          "Verified archive was not discovered as one mod package");
    const auto selected_stale = content::verify_cached_mod_package(stale_cache, stale_catalog.packages[0]);
    check(selected_stale && selected_stale->stale_manifest_hashes.size() == 1 &&
              selected_stale->stale_manifest_hashes[0].actual_md5 ==
                  runtime::file_md5(selected_stale->manifest.path.parent_path() / "rows.sql"),
          "Selected cached package lost its verified actual MD5");
    const auto stale_local = root / "stale_local";
    fs::create_directory(stale_local);
    fs::copy_file(selected_stale->manifest.path, stale_local / "Fixture.modinfo");
    write(stale_local / "rows.sql", {'S','E','L','E','C','T',' ','3',';','\n'});
    auto stale_mirrors = content::discover_packages({stale_local, stale_cache / "packages"});
    check(stale_mirrors.packages.size() == 2 &&
              content::prefer_verified_cached_mods(stale_mirrors, stale_cache / "packages") == 0 &&
              stale_mirrors.packages.size() == 2,
          "Changed local SQL was silently replaced by a same-version author archive");
    write(stale_local / "unlisted.lua", {'x'});
    stale_mirrors = content::discover_packages({stale_local, stale_cache / "packages"});
    check(stale_mirrors.packages.size() == 2 &&
              content::prefer_verified_cached_mods(stale_mirrors, stale_cache / "packages") == 0,
          "An unknown local Lua file was silently discarded for a pinned archive");
    const std::vector<runtime::VerifiedManifestHash> verified_stale{{
        0, selected_stale->stale_manifest_hashes[0].path,
        selected_stale->stale_manifest_hashes[0].declared_md5,
        selected_stale->stale_manifest_hashes[0].actual_md5}};
    check(!runtime::inspect_mod_files(stale_catalog, 0).diagnostics.empty() &&
              runtime::inspect_mod_files(stale_catalog, 0, {}, verified_stale).diagnostics.empty(),
          "Preflight did not apply only the verified archive file hash");
    const std::array<std::size_t, 1> stale_selected{0};
    const auto strict_stale_profile = runtime::resolve_selected_content_profile(
        {}, stale_selected, stale_catalog);
    const auto verified_stale_profile = runtime::resolve_selected_content_profile(
        {}, stale_selected, stale_catalog, {}, verified_stale);
    check(!strict_stale_profile.files_complete && verified_stale_profile.files_complete &&
              verified_stale_profile.can_activate,
          "Pinned archive bytes did not clear preflight for an arbitrary mod");
    const auto missing_archive=data/"mod-archive-missing-asset.zip";
    const auto missing_sha=runtime::identify_file(missing_archive).sha256;
    const content::ModTarget missing_target{"civis.missing.asset.fixture","",12};
    rejects([&]{content::install_mod_archive(missing_archive,missing_target,
                                              root/"missing_strict",missing_sha);});
    rejects([&]{content::install_mod_archive(missing_archive,missing_target,
                                              root/"missing_unpinned",{}, {},{},false,true);});
    const auto missing_cache=root/"missing_pinned";
    const auto missing=content::install_mod_archive(missing_archive,missing_target,
                                                    missing_cache,missing_sha,{}, {},false,true);
    check(missing.missing_manifest_assets.size()==1&&
          missing.missing_manifest_assets[0].path=="art/unused.dds"&&
          missing.missing_manifest_assets[0].declared_md5==
              "00000000000000000000000000000000",
          "Pinned author archive did not record its absent declared asset");
    rejects([&]{content::install_mod_archive(missing_archive,missing_target,
                                              missing_cache,missing_sha);});
    const auto missing_catalog=content::discover_packages({missing_cache/"packages"});
    check(missing_catalog.diagnostics.empty()&&missing_catalog.packages.size()==1,
          "Pinned missing-asset archive was not discoverable");
    const auto verified_missing=content::verify_cached_mod_package(
        missing_cache,missing_catalog.packages[0]);
    const std::vector<runtime::ManifestFileOmission> omitted_asset{{
        missing_target.id,verified_missing->missing_manifest_assets[0].path,
        verified_missing->missing_manifest_assets[0].declared_md5,missing_target.version}};
    check(verified_missing->missing_manifest_assets.size()==1&&
          !runtime::inspect_mod_files(missing_catalog,0).diagnostics.empty()&&
          runtime::inspect_mod_files(missing_catalog,0,omitted_asset).diagnostics.empty(),
          "Verified missing asset did not cross the preflight omission boundary");
    const auto missing_local=root/"missing_local";
    fs::create_directories(missing_local/"art");
    fs::copy_file(verified_missing->manifest.path,missing_local/"Fixture.modinfo");
    fs::copy_file(verified_missing->manifest.path.parent_path()/"rows.sql",
                  missing_local/"rows.sql");
    write(missing_local/"art/portrait.dds",{'C','H','A','N','G','E','D'});
    auto missing_mirrors=content::discover_packages({missing_local,missing_cache/"packages"});
    check(missing_mirrors.packages.size()==2&&
          content::prefer_verified_cached_mods(missing_mirrors,missing_cache/"packages")==1&&
          missing_mirrors.packages.size()==1&&
          fs::equivalent(missing_mirrors.packages[0].path,verified_missing->manifest.path),
          "Verified author archive did not replace only altered presentation assets");
    write(missing_local/"rows.sql",{'S','E','L','E','C','T',' ','6',';','\n'});
    missing_mirrors=content::discover_packages({missing_local,missing_cache/"packages"});
    check(missing_mirrors.packages.size()==2&&
          content::prefer_verified_cached_mods(missing_mirrors,missing_cache/"packages")==0,
          "Changed local SQL was hidden by an unrelated asset difference");
    const auto missing_reused=content::install_mod_archive(missing_archive,missing_target,
                                                           missing_cache,missing_sha,{}, {},false,true);
    check(missing_reused.reused&&missing_reused.missing_manifest_assets.size()==1,
          "Pinned missing-asset receipt was not verified on reuse");
    const auto missing_sql=data/"mod-archive-missing-sql.zip";
    rejects([&]{content::install_mod_archive(missing_sql,
        {"civis.missing.sql.fixture","",13},root/"missing_sql",
        runtime::identify_file(missing_sql).sha256,{}, {},false,true);});
    const auto duplicate_declared=data/"mod-archive-duplicate-declared.zip";
    const auto duplicate_sha=runtime::identify_file(duplicate_declared).sha256;
    const content::ModTarget duplicate_target{"civis.duplicate.declared.fixture","",14};
    const auto duplicate_cache=root/"duplicate_declared_cache";
    const auto duplicate=content::install_mod_archive(duplicate_declared,duplicate_target,
                                                       duplicate_cache,duplicate_sha,{}, {},true);
    check(duplicate.stale_manifest_hashes.size()==1&&
          duplicate.stale_manifest_hashes[0].path=="rows.sql",
          "Identical repeated manifest declaration produced ambiguous verified hashes");
    const auto duplicate_catalog=content::discover_packages({duplicate_cache/"packages"});
    const std::vector<runtime::VerifiedManifestHash> duplicate_hashes{{
        0,duplicate.stale_manifest_hashes[0].path,
        duplicate.stale_manifest_hashes[0].declared_md5,
        duplicate.stale_manifest_hashes[0].actual_md5}};
    check(duplicate_catalog.packages.size()==1&&
          runtime::inspect_mod_files(duplicate_catalog,0,{},duplicate_hashes).diagnostics.empty(),
          "Repeated identical manifest declaration blocked verified preflight");
    runtime::SavedContentProfile stale_profile;
    stale_profile.enabled_mods = {0};
    stale_profile.all_requirements_located = true;
    const auto stale_actions = content::plan_saved_database_actions(stale_profile, stale_catalog);
    check(stale_actions.actions.size() == 1, "Pinned fixture SQL action was not planned");
    content::RulesDatabase stale_strict_db;
    const auto strict_action = content::apply_saved_database_actions(
        stale_strict_db, stale_actions, stale_catalog);
    check(!strict_action.all_succeeded && !strict_action.actions[0].applied,
          "Unverified stale SQL action was executed");
    content::RulesDatabase stale_verified_db;
    const auto verified_action = content::apply_saved_database_actions(
        stale_verified_db, stale_actions, stale_catalog, 64ULL * 1024 * 1024, {}, verified_stale);
    check(verified_action.all_succeeded && verified_action.actions[0].applied &&
              !verified_action.actions[0].manifest_md5_match,
          "Pinned SQL bytes were not applied with a visible manifest discrepancy");
    const auto pinned_sql = selected_stale->manifest.path.parent_path() / "rows.sql";
    const auto original_pinned_sql = bytes(pinned_sql);
    write(pinned_sql, {'S','E','L','E','C','T',' ','4',';','\n'});
    content::RulesDatabase changed_pinned_db;
    const auto changed_pinned_action = content::apply_saved_database_actions(
        changed_pinned_db, stale_actions, stale_catalog, 64ULL * 1024 * 1024, {}, verified_stale);
    check(!runtime::inspect_mod_files(stale_catalog, 0, {}, verified_stale).diagnostics.empty() &&
              !changed_pinned_action.actions[0].applied &&
              changed_pinned_action.actions[0].error == "manifest MD5 mismatch",
          "Changed cached SQL was accepted under the old pinned hash");
    write(pinned_sql, original_pinned_sql);
    const auto stale_receipt = stale.cache_package / "receipt.xml";
    auto receipt_bytes = bytes(stale_receipt);
    const auto original_receipt_bytes = receipt_bytes;
    const std::string declared = "actual=\"";
    auto mismatch_at = std::search(receipt_bytes.begin(), receipt_bytes.end(),
                                   declared.begin(), declared.end());
    check(mismatch_at != receipt_bytes.end(), "Stale MD5 receipt entry missing");
    *(mismatch_at + static_cast<std::ptrdiff_t>(declared.size())) ^= 1;
    write(stale_receipt, receipt_bytes);
    rejects([&] { content::install_mod_archive(stale_archive, stale_target,
                                               stale_cache, stale_sha, {}, {}, true); });
    write(stale_receipt, original_receipt_bytes);
    const auto multi = data / "mod-archive-multipackage.zip";
    const auto selected = content::install_mod_archive(multi, target, root / "multi_cache", {}, {},
                                                       "requested");
    check(selected.verified_files == 3 &&
              selected.manifest.path.parent_path() == selected.cache_package / "content" &&
              !fs::exists(selected.cache_package / "content/unrelated"),
          "Multi-package ZIP published an unrequested mod");
    check(content::install_mod_archive(multi, target, root / "multi_cache", {}, {}, "requested").reused,
          "Selected package cannot be verified on cache reuse");
    const auto invalid_neighbor = data / "mod-archive-multipackage-invalid-neighbor.zip";
    rejects([&] { content::install_mod_archive(invalid_neighbor, target,
                                                root / "invalid_neighbor_full_cache"); });
    const auto isolated = content::install_mod_archive(invalid_neighbor, target,
        root / "invalid_neighbor_selected_cache", {}, {}, "requested");
    check(isolated.verified_files == 3 &&
              !fs::exists(isolated.cache_package / "content/unrelated") &&
              content::install_mod_archive(invalid_neighbor, target,
                  root / "invalid_neighbor_selected_cache", {}, {}, "requested").reused,
          "Unrelated malformed manifest prevented isolated package acquisition");
    rejects([&] { content::install_mod_archive(multi, target, root / "wrong_root_cache", {}, {},
                                                "unrelated"); });
    content::install_mod_archive(multi, target, root / "full_multi_cache");
    rejects([&] { content::install_mod_archive(multi, target, root / "full_multi_cache", {}, {},
                                                "requested"); });
    const content::ModTarget zip64_target{"civis.zip64.fixture","",4294967297ULL};
    const auto zip64_installed=content::install_mod_archive(zip64,zip64_target,root / "zip64_cache");
    check(zip64_installed.manifest.version==zip64_target.version&&zip64_installed.verified_files==1,
          "ZIP64 mod version or manifest was narrowed");
    rejects([&]{content::install_mod_archive(root / "zip_bad_crc.zip",zip_target,root / "zip_bad_cache");});
    check(fs::is_empty(root / "zip_bad_cache/packages"),"Corrupt ZIP was published");
    const auto plan_cache = root / "plan_cache";
    content::install_mod_archive(store, target, plan_cache, original.sha256);
    const content::ModSource fixture_source{target, 4294967297ULL, {}, original.sha256, {}};
    const std::vector<content::ModTarget> cached_target{target};
    const std::vector<content::ModTarget> unknown_target{{"unknown.mod", "", 1}};
    const auto fixture_sources = std::span<const content::ModSource>(&fixture_source, 1);
    const auto cached_plan = content::plan_mod_acquisition(cached_target, fixture_sources, plan_cache);
    check(cached_plan.all_verified && cached_plan.items.size() == 1 &&
              cached_plan.items[0].status == content::ModAcquisitionStatus::cached_verified &&
              cached_plan.items[0].cached && cached_plan.items[0].source_index == 0,
          "Acquisition planner did not verify exact cached target");
    const auto saved_acquisition =
        content::acquire_saved_mods(cached_target, fixture_sources, plan_cache);
    check(saved_acquisition.all_acquired && saved_acquisition.acquired == 1 &&
              saved_acquisition.reused == 1 && saved_acquisition.diagnostics.empty(),
          "Saved acquisition did not reuse the exact verified cache");
    const std::vector<content::ModSource> no_sources;
    const auto independent_plan=content::plan_mod_acquisition(cached_target,no_sources,plan_cache);
    check(independent_plan.all_verified && independent_plan.items[0].cached &&
              independent_plan.items[0].status==content::ModAcquisitionStatus::cached_verified &&
              !independent_plan.items[0].source_index,
          "Save could not find an independently installed verified mod without a source catalog");
    const auto independent_acquisition=content::acquire_saved_mods(cached_target,no_sources,plan_cache);
    check(independent_acquisition.all_acquired && independent_acquisition.acquired==1 &&
              independent_acquisition.reused==1 && independent_acquisition.diagnostics.empty(),
          "Saved acquisition rejected an independently installed exact mod");
    const auto direct_url=content::acquire_mod_from_url(
        target,"https://example.invalid/fixture.7z",plan_cache,original.sha256);
    check(direct_url.reused && direct_url.archive_sha256==original.sha256,
          "Explicit HTTPS source could not reuse an exact pinned archive without downloading");
    const auto workshop_reuse=content::acquire_mod_from_workshop(
        target,4294967297ULL,plan_cache,original.sha256);
    check(workshop_reuse.reused && workshop_reuse.archive_sha256==original.sha256,
          "Explicit Workshop item could not reuse an exact pinned archive without downloading");
    rejects([&]{(void)content::acquire_mod_from_workshop(
        target,0,root/"zero_workshop_cache");});
    check(!fs::exists(root/"zero_workshop_cache"),"Zero Workshop item created a cache");
    rejects([&]{(void)content::acquire_mod_from_url(
        target,"http://example.invalid/fixture.7z",root/"insecure_url_cache");});
    check(!fs::exists(root/"insecure_url_cache"),"Insecure mod URL created a cache");
    content::install_mod_archive(solid,target,plan_cache);
    const auto ambiguous_plan=content::plan_mod_acquisition(cached_target,no_sources,plan_cache);
    check(!ambiguous_plan.all_verified &&
              ambiguous_plan.items[0].status==content::ModAcquisitionStatus::cache_ambiguous,
          "Save silently chose between two cached archives with the same mod ID/version");
    const auto ambiguous_acquisition=content::acquire_saved_mods(cached_target,no_sources,plan_cache);
    check(!ambiguous_acquisition.all_acquired && ambiguous_acquisition.acquired==0 &&
              ambiguous_acquisition.diagnostics.size()==1 &&
              ambiguous_acquisition.diagnostics[0].find("cache_ambiguous")!=std::string::npos,
          "Saved acquisition hid ambiguous independently installed mod bytes");
    const auto pinned_plan=content::plan_mod_acquisition(cached_target,fixture_sources,plan_cache);
    check(pinned_plan.all_verified && pinned_plan.items[0].cached &&
              pinned_plan.items[0].cached->archive_sha256==original.sha256,
          "Pinned catalog source failed to disambiguate two cached mod revisions");
    const auto missing_saved_acquisition =
        content::acquire_saved_mods(unknown_target, fixture_sources, plan_cache);
    check(!missing_saved_acquisition.all_acquired && missing_saved_acquisition.acquired == 0 &&
              missing_saved_acquisition.diagnostics.size() == 1 &&
              missing_saved_acquisition.diagnostics[0].find("source_unresolved") != std::string::npos,
          "Saved acquisition hid an unresolved source");
    const auto download_plan = content::plan_mod_acquisition(cached_target, fixture_sources, root / "empty_plan");
    check(!download_plan.all_verified &&
              download_plan.items[0].status == content::ModAcquisitionStatus::download_required,
          "Acquisition planner did not report missing download");
    const auto unknown_plan = content::plan_mod_acquisition(unknown_target, fixture_sources);
    check(unknown_plan.items[0].status == content::ModAcquisitionStatus::source_unresolved &&
              std::string(content::mod_acquisition_status_name(unknown_plan.items[0].status)) ==
                  "source_unresolved",
          "Acquisition planner accepted an unknown source");
    const std::vector<content::ModTarget> wrong_version{{target.id, "", target.version + 1}};
    const auto version_plan = content::plan_mod_acquisition(wrong_version, fixture_sources);
    check(version_plan.items[0].status == content::ModAcquisitionStatus::version_unavailable,
          "Acquisition planner substituted a different version");
    const content::ModSource installer_source{{"8411a7a8-dad3-4622-a18e-fcc18324c799", "VP", 17}, 0,
                                               {}, std::string(64, 'a'), "vp-5.4.2"};
    const auto installer_sources = std::span<const content::ModSource>(&installer_source, 1);
    const auto installer_plan = content::plan_mod_acquisition(
        std::vector<content::ModTarget>{installer_source.target}, installer_sources);
    check(installer_plan.items[0].status == content::ModAcquisitionStatus::installer_profile_required,
          "Acquisition planner treated an installer profile as a loose mod");
    const auto invalid_plan_cache = root / "invalid_plan_cache";
    fs::create_directories(invalid_plan_cache / "packages");
    const std::string invalid_marker = "<Foreign/>";
    write(invalid_plan_cache / "civis-cache.xml",
          {invalid_marker.begin(), invalid_marker.end()});
    const auto invalid_plan = content::plan_mod_acquisition(cached_target, fixture_sources, invalid_plan_cache);
    check(!invalid_plan.all_verified &&
              invalid_plan.items[0].status == content::ModAcquisitionStatus::cache_invalid &&
              !invalid_plan.items[0].reason.empty(),
          "Acquisition planner hid an invalid cache");
    rejects([&] { content::install_mod_archive(store, {target.id, "", 8}, root / "wrong_version"); });
    check(fs::is_empty(root / "wrong_version/packages"), "Wrong version published");
    rejects([&] { content::install_mod_archive(store, {"different.id", "", 7}, root / "wrong_id"); });
    rejects([&] { content::install_mod_archive(store, target, root / "wrong_hash", std::string(64, '0')); });
    check(!fs::exists(root / "wrong_hash"), "Wrong hash changed cache");
    write(installed.manifest.path.parent_path() / "abcdef.txt", {1, 2, 3});
    rejects([&] { content::install_mod_archive(store, target, cache); });
    const auto extra = content::install_mod_archive(store, target, root / "extra_cache");
    write(extra.manifest.path.parent_path() / "unlisted.lua", {1});
    rejects([&] { content::install_mod_archive(store, target, root / "extra_cache"); });
    fs::create_directory(root / "non_cache");
    write(root / "non_cache/original", {1});
    rejects([&] { content::initialize_mod_cache(root / "non_cache"); });
    check(!fs::exists(root / "non_cache/civis-cache.xml"), "Foreign folder accepted as cache");
    const auto info = content::parse_workshop_details(workshop_xml(), 4294967297ULL);
    check(info.bytes == 4294967298ULL && info.content_handle == 18446744073709551614ULL &&
              info.title == "Test & mod",
          "Workshop uint64 or XML values narrowed");
    rejects([&] { content::parse_workshop_details(workshop_xml("480"), 4294967297ULL); });
    rejects([&] { content::parse_workshop_details(workshop_xml(), 1); });
    auto dtd = workshop_xml();
    dtd.replace(dtd.find("<!DOCTYPE response>"), 19, "<!DOCTYPE response SYSTEM 'file:///secret'>");
    rejects([&] { content::parse_workshop_details(dtd, 4294967297ULL); });
    const auto sources_path = root / "sources.xml";
    const auto source_record = "<Mod id=\"CIVIS.ARCHIVE.FIXTURE\" version=\"4294967298\"><Workshop "
                               "id=\"4294967297\" sha256=\"" +
                               original.sha256 + "\"/></Mod>";
    const auto save_catalog = [&](const std::string &records) {
        const auto xml = "<CivisModSources schema=\"1\">" + records + "</CivisModSources>";
        write(sources_path, {xml.begin(), xml.end()});
    };
    save_catalog(source_record);
    const auto sources = content::read_mod_sources(sources_path);
    check(sources.size() == 1 && sources[0].target.id == target.id &&
              sources[0].target.version == 4294967298ULL && sources[0].workshop_id == 4294967297ULL,
          "Source identity narrowed or changed");
    save_catalog("<Mod id=\"civis.zip.fixture\" version=\"9\"><Archive "
                 "url=\"https://example.com/mod.zip\" sha256=\"" +
                 zip_identity.sha256 + "\"/></Mod>");
    const auto zip_sources=content::read_mod_sources(sources_path);
    check(zip_sources.size()==1&&zip_sources[0].archive_url=="https://example.com/mod.zip"&&
          zip_sources[0].sha256==zip_identity.sha256,
          "Pinned author ZIP URL was not retained");
    save_catalog("<Mod id=\"civis.stale.fixture\" version=\"11\"><Archive "
                 "url=\"https://example.com/stale.zip\" manifest_md5=\"archive\" sha256=\"" +
                 stale_sha + "\"/></Mod>");
    const auto stale_sources = content::read_mod_sources(sources_path);
    check(stale_sources.size() == 1 && stale_sources[0].allow_stale_manifest_md5,
          "Pinned archive manifest MD5 policy was not retained");
    save_catalog("<Mod id=\"civis.missing.asset.fixture\" version=\"12\"><Archive "
                 "url=\"https://example.com/asset.zip\" manifest_assets=\"archive\" sha256=\"" +
                 missing_sha + "\"/></Mod>");
    const auto missing_sources=content::read_mod_sources(sources_path);
    check(missing_sources.size()==1&&missing_sources[0].allow_missing_manifest_assets,
          "Pinned archive missing-asset policy was not retained");
    const auto missing_plan=content::plan_mod_acquisition(
        std::vector<content::ModTarget>{missing_target},missing_sources,missing_cache);
    check(missing_plan.all_verified&&
          missing_plan.items[0].status==content::ModAcquisitionStatus::cached_verified,
          "Pinned missing-asset archive was not selectable");
    auto strict_missing_source=missing_sources;
    strict_missing_source[0].allow_missing_manifest_assets=false;
    check(content::plan_mod_acquisition(
        std::vector<content::ModTarget>{missing_target},strict_missing_source,
        missing_cache).items[0].status==content::ModAcquisitionStatus::cache_invalid,
          "Strict source reused an archive with missing assets");
    save_catalog("<Mod id=\"civis.missing.asset.fixture\" version=\"12\"><Workshop "
                 "id=\"1\" manifest_assets=\"archive\" sha256=\"" +missing_sha+"\"/></Mod>");
    rejects([&]{content::read_mod_sources(sources_path);});
    const auto stale_plan = content::plan_mod_acquisition(
        std::vector<content::ModTarget>{stale_target}, stale_sources, stale_cache);
    check(stale_plan.all_verified &&
              stale_plan.items[0].status == content::ModAcquisitionStatus::cached_verified,
          "Pinned stale-MD5 archive was not selectable by its exact save identity");
    auto strict_stale_source = stale_sources;
    strict_stale_source[0].allow_stale_manifest_md5 = false;
    const auto strict_stale_plan = content::plan_mod_acquisition(
        std::vector<content::ModTarget>{stale_target}, strict_stale_source, stale_cache);
    check(!strict_stale_plan.all_verified &&
              strict_stale_plan.items[0].status == content::ModAcquisitionStatus::cache_invalid,
          "Strict source reused a stale-MD5 archive");
    save_catalog("<Mod id=\"civis.stale.fixture\" version=\"11\"><Workshop "
                 "id=\"1\" manifest_md5=\"archive\" sha256=\"" + stale_sha + "\"/></Mod>");
    rejects([&] { content::read_mod_sources(sources_path); });
    save_catalog("<Mod id=\"civis.zip.fixture\" version=\"9\"><Archive "
                 "url=\"https://example.com/folder.zip\" member=\"mod-archive-deflate.7z\" sha256=\"" +
                 runtime::identify_file(deflate).sha256 + "\"/></Mod>");
    const auto nested_sources = content::read_mod_sources(sources_path);
    check(nested_sources.size() == 1 &&
              nested_sources[0].archive_member == "mod-archive-deflate.7z",
          "Pinned nested archive member was not retained");
    save_catalog("<Mod id=\"civis.archive.fixture\" version=\"7\"><Archive "
                 "url=\"https://example.com/multi.zip\" root=\"requested\" sha256=\"" +
                 runtime::identify_file(multi).sha256 + "\"/></Mod>");
    const auto multi_sources = content::read_mod_sources(sources_path);
    check(multi_sources.size() == 1 && multi_sources[0].package_root == "requested",
          "Selected package root was not retained");
    save_catalog("<Mod id=\"civis.archive.fixture\" version=\"7\"><Archive "
                 "url=\"https://example.com/multi.zip\" root=\"../escape\" sha256=\"" +
                 runtime::identify_file(multi).sha256 + "\"/></Mod>");
    rejects([&] { content::read_mod_sources(sources_path); });
    save_catalog("<Mod id=\"civis.zip.fixture\" version=\"9\"><Archive "
                 "url=\"https://example.com/folder.zip\" member=\"../escape.7z\" sha256=\"" +
                 runtime::identify_file(deflate).sha256 + "\"/></Mod>");
    rejects([&] { content::read_mod_sources(sources_path); });
    content::PackageManifest dependency_manifest{};
    dependency_manifest.kind = content::PackageKind::mod;
    dependency_manifest.document = content::read_xml_metadata(
        "<Mod><Dependencies><Game minversion='1.0.3.124'/><Dlc id='dlc'/><Mod id='"
        "CIVIS.DEPENDENCY' minversion='7' maxversion='9' title='Dependency'/></Dependencies></Mod>");
    const auto dependencies = content::read_mod_dependencies(dependency_manifest);
    check(dependencies.size() == 1 && dependencies[0].id == "civis.dependency" &&
              dependencies[0].name == "Dependency" && dependencies[0].minimum_version == 7 &&
              dependencies[0].maximum_version == 9,
          "Manifest mod dependency range was not retained");
    dependency_manifest.document = content::read_xml_metadata(
        "<Mod><Dependencies><Lua id='unexpected'/></Dependencies></Mod>");
    rejects([&] { (void)content::read_mod_dependencies(dependency_manifest); });
    dependency_manifest.document = content::read_xml_metadata(
        "<Mod><Dependencies><Mod id='bad' minversion='9' maxversion='7'/></Dependencies></Mod>");
    rejects([&] { (void)content::read_mod_dependencies(dependency_manifest); });
    save_catalog(source_record + source_record);
    rejects([&] { content::read_mod_sources(sources_path); });
    save_catalog("<Mod id=\"x\" version=\"1\"><Workshop id=\"2\"/></Mod>");
    rejects([&] { content::read_mod_sources(sources_path); });
    save_catalog("<Mod id=\"x\" version=\"1\"><Archive "
                 "url=\"http://example.com/mod.7z\" sha256=\"" +
                 original.sha256 + "\"/></Mod>");
    rejects([&] { content::read_mod_sources(sources_path); });
    check(platform::form_encode("a&= \xD0\xBC") == "a%26%3D%20%D0%BC", "Form encoding");
    rejects([&] { platform::download_https({"http://example.com/a", {}}, root / "no_http", 100); });
    rejects([&] {
        platform::download_https({"https://user:pass@example.com/a", {}}, root / "no_credentials", 100);
    });
    check(runtime::identify_file(store).sha256 == original.sha256, "Original fixture changed");
    // Round trip via the Windows parser, independently of the quoting
    // implementation.
    for (const std::wstring argument : {L"", L"plain", L"a b", L"quote\"here", L"trail\\", L"\\\\\"x",
                                        L"C:\\path with spaces\\", L"\u043a\u0430\u0440\u0442\u0430"}) {
        auto command = L"program.exe " + content::quote_windows_argument(argument);
        int count = 0;
        auto parsed = CommandLineToArgvW(command.c_str(), &count);
        const bool same = parsed && count == 2 && parsed[1] == argument;
        if (parsed)
            LocalFree(parsed);
        check(same, "Decoder argument changed by Windows parsing");
    }
    rejects([&] { content::quote_windows_argument(std::wstring{L'x', 0, L'y'}); });
    fs::create_directory(root / "untrusted_decoder");
    write(root / "untrusted_decoder/innoextract.exe", {'M', 'Z'});
    rejects([&] { content::verify_inno_decoder(root / "untrusted_decoder"); });
    rejects([&] {
        content::acquire_vp_542_bundle(root / "bad_installer_cache", root / "untrusted_decoder", store);
    });
    check(!fs::exists(root / "bad_installer_cache"), "Untrusted installer changed cache");
    rejects([&] {
        content::acquire_vp_542_bundle(root / "bundle_budget", root / "untrusted_decoder", {}, {}, {1, 1, 1});
    });
    check(!fs::exists(root / "bundle_budget"), "Rejected installer budget changed cache");
    save_catalog("<Mod id=\"d1b6328c-ff44-4b0d-aad7-c657f83610cd\" version=\"151\">"
                 "<Installer bundle=\"vp-5.4.2\" sha256=\"" +
                 std::string(content::vp_542_installer_sha256) + "\"/></Mod>");
    const auto bundle_sources = content::read_mod_sources(sources_path);
    check(bundle_sources.size() == 1 && bundle_sources[0].installer_bundle == "vp-5.4.2",
          "Installer source not retained");
    rejects([&] { content::acquire_mod(bundle_sources[0], root / "must_not_activate"); });
    check(!fs::exists(root / "must_not_activate"), "Installer bundle was treated as an activated mod");
    save_catalog("<Mod id=\"x\" version=\"1\"><Installer bundle=\"vp-5.4.2\" sha256=\"" +
                 std::string(content::vp_542_installer_sha256) + "\"/></Mod>");
    rejects([&] { content::read_mod_sources(sources_path); });
}
} // namespace
int main() {
    try {
        const auto root = fs::temp_directory_path() /
                          ("civis-acquisition-fixture-" +
                           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        run(root);
        std::cout << "PASS " << checks << " acquisition checks; runtime_bits=" << sizeof(void *) * 8 << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL after " << checks << ": " << error.what() << '\n';
        return 1;
    }
}
