#include "../src/ui/bnw_native_category_pattern_match.hpp"
#include "bnw_native_ui_skin_poststage_query_tests.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/detail/bnw_native_resource_record_index.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include "civ5/materials/material_cache_settings.hpp"
#include "civ5/ui/bnw_selected_native_color_projection.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include <algorithm>
#include <iostream>
#include <thread>
#include <type_traits>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
namespace {
void check(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
// This fake borrow is a compile-time access probe, never an actual issuer.
struct UnownedMode7Probe {};
template<class T>concept PublicMode7Admission=requires(T& index,UnownedMode7Probe& borrow,
    std::variant<civ5::assets::AssetHandle,ui::detail::UnreadableLoose> payload){index.insert_mode7("a",payload,borrow);};
static_assert(!PublicMode7Admission<ui::detail::NativeResourceRecordIndex>);
static_assert(ui::detail::native_category_matched_filter_mode(7)==2);
static_assert(ui::detail::native_category_matched_filter_mode(7)!=6);
static_assert(ui::detail::native_category_matched_filter_mode(0x17)==0x12);
void check_native_category_pattern_matches() {
    using namespace ui::native_category_patterns_detail;
    for(const auto& pair:std::vector<std::pair<std::string,std::string>>{
        {"abxc","a?xc"},{"abBBc","a*b?c"},{"abc","a**c"},{"abc","*b*c"},
        {"a","a."},{"a","a.*"},{"a","a.***.*"},{"","*"},{"",".*"},
        {"file.XML","*.xml"},{"C:\\Assets\\Scenarios\\A","*\\Scenarios\\*"}})
        check(match_one_ascii(pair.first,pair.second),"Native81D1C0 positive wildcard/DOS-tail differs");
    for(const auto& pair:std::vector<std::pair<std::string,std::string>>{
        {"abxc","a?c"},{"ab","a??"},{"abc","a*d"},{"abc","*b*x"},
        {"","?"},{"a","a.x"},{"a","a*?"},{"file.XMLx","*.xml"},
        {"C:/Scenarios/A","*\\Scenarios\\*"}})
        check(!match_one_ascii(pair.first,pair.second),"Native81D1C0 negative wildcard/slash differs");
    check(!match_one_ascii("file.XML","*.xml",true),"Native case1 incorrectly uses C toupper");
    check(match_list_ascii("b.xml","*.ggxml;*.xml"),"Native second semicolon token lost");
    check(match_list_ascii("","x;"),"Native trailing empty semicolon token discarded");
    check(!match_list_ascii("b.xml"," *.xml;*.txt"),"Native matcher incorrectly trims token whitespace");
    const std::vector<std::string> rows{"Assets\\Gameplay","Assets\\Map"};
    check(directory_exclusion_list_ascii(rows)=="*\\Sounds\\Speech\\*;*\\Scenarios\\*;Assets\\Gameplay\\*;Assets\\Map\\*",
          "Native exclusion exact base/provider format differs");
    check(directory_excluded_ascii("Assets\\Gameplay\\Units",rows),"Actual other category row exclusion lost");
    check(!directory_excluded_ascii("Assets\\GameplayElse\\Units",rows),"Exclusion replaced wildcard with broad prefix");
    check(directory_excluded_ascii("Assets\\SOUNDS\\Speech\\Shared",{}),"Fixed actual Speech exclusion lost");
    check(!directory_excluded_ascii("Assets\\Sounds\\Other",{}),"Speech exclusion broadened");
    check(explicit_file_uses_find_a_ascii("Folder/*.xml"),"Actual basename '*' did not select FindA");
    check(!explicit_file_uses_find_a_ascii("Folder*/File?.xml"),"Parent star/question-only incorrectly selects FindA");
    check(!explicit_file_uses_find_a_ascii("Folder/File.xml"),"Literal File incorrectly selects glob");
    check(explicit_find_a_attributes_selected(0x20)&&!explicit_find_a_attributes_selected(0x10)&&
          !explicit_find_a_attributes_selected(0x2)&&!explicit_find_a_attributes_selected(0x4),"Actual attrs0x16 File selection differs");
    for(const auto& s:{std::string("x\0y",3),std::string("x\x80y",3),std::string("x\ny")}) {
        bool refused=false;try{(void)match_list_ascii(s,"*");}catch(const std::runtime_error&){refused=true;}
        check(refused,"Unknown codepage/NUL/control input was accepted as native ASCII");
    }
}
void check_native_category_xml_filter() {
    for(const auto* name:{"a.xml","a.XML",".xml","..xml","a.b.xml","a;B.Xml","a.ggxml",".GGXML","a.xml.ggxml"})
        check(ui::detail::native_category_xml_filter_ascii(name),"Native fixed semicolon wildcard failed actual XML token");
    for(const auto* name:{"xml","filexml","a.xml.tmp","a.ggxml.","a.ggxmlx","a.gxml","a.xml ","a.txt","file.XMLx"})
        check(!ui::detail::native_category_xml_filter_ascii(name),"Native wildcard accepted incomplete/nonterminal suffix");
    for(const auto& name:{std::string(),std::string("dir/a.xml"),std::string("dir\\a.xml"),
                         std::string("a\0.xml",6),std::string("a\x80.xml",6),std::string(128,'a')}) {
        bool refused=false;try{(void)ui::detail::native_category_xml_filter_ascii(name);}catch(const std::runtime_error&){refused=true;}
        check(refused,"Native ASCII basename refusal lost finite scope");
    }
}

template<class F> void rejects(F&& action) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    check(rejected, "Native package accepted incomplete/stale/foreign source");
}
}
// A post-Rules VFS cannot be handed to this EARLY source factory.
using LegacyPackageFactory = std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner> (*)(
    const std::filesystem::path&, const std::filesystem::path&, const content::Catalog&,
    const runtime::SavedContentProfile&, std::shared_ptr<const ui::BnwNativeUiColorOwner>);
static_assert(!std::is_invocable_v<decltype(static_cast<LegacyPackageFactory>(
    &ui::BnwNativePackageDiscoveryOwner::create)),
    content::LooseResourcePlan&&, const std::filesystem::path&, const std::filesystem::path&,
    const content::Catalog&, const runtime::SavedContentProfile&,
    std::shared_ptr<const ui::BnwNativeUiColorOwner>>);
int main(int argc, char** argv) {
    ui::poststage_query_authored_tests::ordinary_data_operations();
    check_native_category_xml_filter();
    check_native_category_pattern_matches();
    check(argc == 4 || (argc == 5 && std::string(argv[4]) == "-LeaderViewer"), "Actual save/install/shared sources required");
    auto selected = ui::bootstrap_selected_bnw_preview(argv[1], argv[2], 0);
    check(static_cast<bool>(selected.preview), "Actual Gandhi unavailable");
    auto host = std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(selected.preview.host));
    struct Window { HWND h = nullptr; ~Window() { if (h) DestroyWindow(h); } } window;
    window.h = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW, 0, 0, 841, 621,
                              nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    check(window.h && !IsWindowVisible(window.h), "Actual current-process hidden HWND required");
    std::uint64_t revision = 5;
    auto client = runtime::bnw_stock_lua_win32_client_extent(
        reinterpret_cast<std::uintptr_t>(window.h), "actual-native-pkg-discovery", [&] { return revision; });
    auto rules = ui::BnwNativeUiColorOwner::create(host,
        reinterpret_cast<std::uintptr_t>(window.h), client);
    const auto install = std::filesystem::path(argv[2]);
    const auto shared = std::filesystem::canonical(argv[3]);
    const auto original = content::discover_packages({install / "Assets/DLC"});
    check(original.diagnostics.empty(), "Actual original catalog unavailable");
    auto bad_profile = selected.profile;
    bad_profile.enabled_dlc.pop_back();
    rejects([&] { ui::BnwNativePackageDiscoveryOwner::create(
        install, shared, original, bad_profile, rules); });
    auto corrupt = original;
    corrupt.packages.at(selected.profile.enabled_dlc.front()).document.attributes.emplace_back(
        "UnboundSource", "1");
    rejects([&] { ui::BnwNativePackageDiscoveryOwner::create(
        install, shared, corrupt, selected.profile, rules); });
    auto duplicate = selected.profile;
    duplicate.enabled_dlc.push_back(duplicate.enabled_dlc.front());
    rejects([&] { ui::BnwNativePackageDiscoveryOwner::create(
        install, shared, original, duplicate, rules); });
    auto incomplete = original;
    incomplete.diagnostics.push_back("missing original source");
    rejects([&] { ui::BnwNativePackageDiscoveryOwner::create(
        install, shared, incomplete, selected.profile, rules); });
    auto owner = ui::BnwNativePackageDiscoveryOwner::create(
        install, shared, original, selected.profile, rules);
    owner->require_same_selected_host(*host);
    auto foreign_selected = ui::bootstrap_selected_bnw_preview(argv[1], argv[2], 0);
    check(static_cast<bool>(foreign_selected.preview), "Actual identical-valued second host unavailable");
    rejects([&] { owner->require_same_selected_host(*foreign_selected.preview.host); });
    const auto& frame = owner->current();
    std::cout << "COUNTS packages=" << frame.packages << "/" << original.packages.size() << " saved=" << selected.profile.enabled_dlc.size() << " skins=" << frame.skin_contexts << " slots=" << frame.slot_maps << " misses=" << frame.descriptor_index_misses << " reads=" << frame.descriptor_direct_reads << std::endl;
    check(frame.packages == static_cast<std::size_t>(std::count_if(original.packages.begin(), original.packages.end(), [](const auto& p) { return p.kind == content::PackageKind::dlc; })) &&
              frame.packages > selected.profile.enabled_dlc.size() && frame.skin_contexts > 4 &&
              frame.slot_maps == frame.skin_contexts * 2 &&
              frame.descriptor_index_misses == frame.packages &&
              frame.descriptor_direct_reads == frame.packages &&
              frame.potential_package_color_maps_complete &&
              !frame.color_source_set_closed && !frame.native_skin_loading_complete,
          "Original source universe/early read boundary falsely selected or closed");
    const auto* same_table = &owner->resources();
    ui::poststage_query_authored_tests::actual_unsealed_owner(owner);
    check(owner->resources().mounts().empty() && owner->resources().keys().empty(),
          "Descriptor read fabricated native registration");
    check(std::holds_alternative<std::monostate>(owner->resources().resolve(
              assets::AssetKey(frame.original_descriptor_order.front())).asset) &&
              std::holds_alternative<std::monostate>(owner->resources().resolve(
              assets::AssetKey("GlobalDefines.xml")).asset),
          "Early resource epoch reused post-Rules/modified shared metadata");
    const auto registry = owner->color_provider().current();
    check(registry && registry->unresolved_provider &&
              registry->contexts.size() == frame.skin_contexts && registry->global_map,
          "Package projection removed mandatory default/dynamic Skin producer");
    for (const auto& context : registry->contexts)
        for (std::size_t slot = 0; slot < 2; ++slot)
            check(!context.enabled[slot].has_value() && context.maps[slot] &&
                      context.maps[slot]->empty(), "Potential maps fabricated enabled flags");
    auto color = ui::BnwNativeUiColorOwner::create(host,
        reinterpret_cast<std::uintptr_t>(window.h), client, owner->color_provider());
    rejects([&] { color->quote("0,0,0,0"); });
    rejects([&] { color->quote("255,255,255,0"); });
    rejects([&] { color->quote("Beige"); });
    rejects([&] { ui::BnwNativeResourceBootSettingsOwner::create(owner); });
    rejects([&] { ui::BnwNativeResourceBootSettingsOwner::create({}); });
    rejects([&] { owner->register_native_common_high_quality_prefix(); });
    owner->register_native_dx9_primary_prefix();
    const auto& registered = owner->current();
    check(&owner->resources() == same_table && owner->resources().mounts().empty() &&
              registered.primary_dx9_registration_complete &&
              registered.archive_registration_requests == 32 && registered.archives_registered > 0 &&
              registered.default_indexed_names > 0 &&
              registered.resource_registration_order.size() == 32 &&
              registered.resource_registration_order.front() == "Resource/DX9/AmericanWashingtonTextures.fpk" &&
              registered.resource_registration_order.back() == "Resource/DX9/WonderTextures.fpk" &&
              !registered.next_resource_dependency.empty(),
          "Primary source prefix substituted archive mounts, reset table or skipped low-res barrier");
    const auto atlas = owner->resources().resolve(assets::AssetKey("Assets/UI/ColorAtlas.xml"));
    check(std::holds_alternative<std::monostate>(atlas.asset),
          "Primary texture prefix invented default Color registration");
    const auto texture = owner->resources().resolve(assets::AssetKey("assets/ui/civilzationv_logo.dds"));
    // Source initial UITextures archive carries the original misspelled logo; directories are
    // ignored by indexed native basename lookup, preserving canonical handle.
    const auto* texture_handle = std::get_if<assets::AssetHandle>(&texture.asset);
    check(texture_handle && !texture.mount_id &&
              texture_handle->candidate().provenance.kind == assets::SourceKind::fpk &&
              texture_handle->candidate().provenance.archive_relative_path == "Resource/DX9/UITextures.fpk",
          "Actual native texture prefix did not select canonical default archive");
    check(!texture_handle->read_bytes(64*1024*1024).empty(), "Canonical FPK payload unavailable");
    rejects([&] { owner->register_native_dx9_primary_prefix(); });
    check(owner->color_provider().current()->unresolved_provider.has_value(),
          "Primary DX9 prefix removed mandatory Color provider");
    std::cout << "PRIMARY DX9 requests=" << registered.archive_registration_requests
              << " archives=" << registered.archives_registered
              << " indexed=" << registered.default_indexed_names << std::endl;
    if (argc == 5) {
        rejects([&] { ui::BnwNativeResourceBootSettingsOwner::create(owner); });
        std::cout << "PASS: actual current-process LeaderViewer substring rejected; no caller launch bool" << std::endl;
        return 0;
    }
    const auto primary_default_count = owner->current().default_indexed_names;
    rejects([&] { owner->register_native_common_high_quality_prefix(); });
    auto boot = ui::BnwNativeResourceBootSettingsOwner::create(owner);
    if (!boot->current().adapter_caps_source_closed) boot->capture_native_dx9_adapter_source();
    rejects([&] { boot->capture_native_dx9_adapter_source(); });
    const auto& settings = boot->current();
    const auto parsed = materials::load_material_cache_ini(settings.graphics_ini);
    check(settings.normal_launch_source_closed && !settings.leader_viewer &&
              !settings.material_adapter_ready && settings.ini_loaded == parsed.loaded &&
              settings.raw_texture_quality_override == parsed.overrides.texture_quality &&
              (settings.raw_texture_quality_override ?
                   settings.effective_texture_quality == settings.raw_texture_quality_override :
                   settings.adapter_caps_source_closed && settings.adapter_preset &&
                   settings.effective_texture_quality.has_value()),
          "Actual INI override manufactured adapter readiness/default DWORD");
    rejects([&] { owner->register_native_dx9_high_quality_branch({}); });
    if (settings.effective_texture_quality && *settings.effective_texture_quality != 0) {
        owner->register_native_dx9_high_quality_branch(boot);
        check(owner->current().low_resolution_source_branch_complete &&
                  owner->current().high_texture_quality_source_branch &&
                  owner->current().archive_registration_requests == 32 &&
                  owner->resources().mounts().empty() && &owner->resources() == same_table &&
                  owner->color_provider().current()->unresolved_provider,
              "Actual high-quality source branch reset resources/skipped mandatory Color");
        rejects([&] { owner->register_native_dx9_high_quality_branch(boot); });
    } else {
        rejects([&] { owner->register_native_dx9_high_quality_branch(boot); });
        check(!owner->current().low_resolution_source_branch_complete,
              "Zero setting falsely executed high-quality source branch");
        owner->register_native_dx9_texture_quality_branch(boot);
        check(owner->current().low_resolution_source_branch_complete &&
                  !owner->current().high_texture_quality_source_branch &&
                  owner->current().low_resource_mount_present &&
                  owner->current().archive_registration_requests == 54 &&
                  owner->resources().mounts().size() == 1 &&
                  &owner->resources() == same_table && owner->current().default_indexed_names == primary_default_count,
              "Actual low-quality branch did not retain SAME default and separate source mount");
        const auto names = owner->resources().mounts().front().key_lookup({});
        check(!names.empty(), "Actual native low archive source unavailable");
        const auto low_asset = owner->resources().resolve(assets::AssetKey(names.front()));
        const auto* low_handle = std::get_if<assets::AssetHandle>(&low_asset.asset);
        check(low_asset.mount_id == 1 && low_handle &&
                  low_handle->candidate().provenance.archive_relative_path &&
                  low_handle->candidate().provenance.archive_relative_path->starts_with("Resource/DX9_LOW/") &&
                  !low_handle->read_bytes(64*1024*1024).empty(),
              "Low source did not select preferred canonical mounted FPK payload");
        rejects([&] { owner->register_native_dx9_texture_quality_branch(boot); });
    }
    if (settings.effective_texture_quality && *settings.effective_texture_quality != 0) {
        owner->register_native_common_high_quality_prefix();
        const auto& common = owner->current();
        check(&owner->resources() == same_table && owner->resources().mounts().empty() &&
                  common.common_high_quality_registration_complete &&
                  common.archive_registration_requests == 45 && common.loose_registration_requests == 3 &&
                  common.loose_files_registered > 0 &&
                  common.loose_file_registration_requests >= common.loose_files_registered &&
                  common.native_loose_code_page == GetACP() && common.resource_registration_order.size() == 48 &&
                  common.resource_registration_order[32] == "Resource/Common/BuildingModels.fpk" &&
                  common.resource_registration_order[44] == "Resource/Common/WonderModels.fpk" &&
                  common.resource_registration_order[45] == "Resource/Overrides" &&
                  common.resource_registration_order[46] == "Assets" &&
                  common.resource_registration_order[47] == "Assets/Sounds/Speech/Shared",
              "Common/default stage changed table or source registration order");
        for (const auto* name : {"Civ5ArtDefines_UnitMembers.xml",
                                 "Assets/UI/ColorAtlas.xml"}) {
            const auto hit = owner->resources().resolve(assets::AssetKey(name));
            const auto* handle = std::get_if<assets::AssetHandle>(&hit.asset);
            check(handle && !hit.mount_id && handle->candidate().provenance.kind == assets::SourceKind::loose &&
                      handle->candidate().provenance.source_id == assets::path_utf8(std::filesystem::canonical(argv[3])) &&
                      !handle->read_bytes(64*1024*1024).empty(),
                  "Native Common/default source missing canonical production unit/Color input");
        }
        const auto ini_hit = owner->resources().resolve(assets::AssetKey("Assets/Units/UnitVisConfig.ini"));
        const auto* ini_handle = std::get_if<assets::AssetHandle>(&ini_hit.asset);
        const auto native_unit_models = assets::FpkSource::open(std::filesystem::path(argv[2]) /
            "Resource/Common/UnitModels.fpk", "Resource/Common/UnitModels.fpk");
        const auto* original_units = std::get_if<std::shared_ptr<assets::FpkSource>>(&native_unit_models);
        check(ini_handle && !ini_hit.mount_id && original_units &&
              ini_handle->candidate().provenance.kind == assets::SourceKind::fpk &&
              ini_handle->candidate().provenance.archive_relative_path == "Resource/Common/UnitModels.fpk",
              "UnitVisConfig native basename hit must use Common UnitModels FPK, not nonexistent loose INI");
        const auto native_ini = (*original_units)->find(assets::AssetKey("UnitVisConfig.ini"));
        check(std::holds_alternative<assets::AssetHandle>(native_ini) &&
              std::get<assets::AssetHandle>(native_ini).read_bytes(64*1024*1024) == ini_handle->read_bytes(64*1024*1024),
              "Common UnitVisConfig canonical archive payload differs from original source");
        std::cout << "COMMON INPUT UnitVisConfig.ini -> canonical UnitModels.fpk original bytes equal; ArtDefines+ColorAtlas -> canonical loose" << std::endl;
        // Native collision is observable: both queries must select the early
        // SoundData record, including actual read bytes/provenance. A fresh
        // controlled SAME-algorithm index then proves mode0 preserve and mode2
        // deletion/replacement atomically update lookup + enumerated payload.
        const auto first = owner->resources().resolve(assets::AssetKey("explosion200poundwater-002.wav"));
        const auto second = owner->resources().resolve(assets::AssetKey("work_boat_death.dge"));
        const auto& sound = std::get<assets::AssetHandle>(first.asset);
        const auto& aliased = std::get<assets::AssetHandle>(second.asset);
        check(!first.mount_id && !second.mount_id &&
              sound.candidate().provenance.archive_relative_path == "Resource/Common/SoundData.fpk" &&
              aliased.candidate().original_name == sound.candidate().original_name &&
              aliased.read_bytes(64*1024*1024) == sound.read_bytes(64*1024*1024),
              "Native CRC-only differential lookup did not retain original SoundData winner");
        const auto original_sound = assets::FpkSource::open(std::filesystem::path(argv[2]) /
            "Resource/Common/SoundData.fpk", "Resource/Common/SoundData.fpk");
        const auto* original_sound_source = std::get_if<std::shared_ptr<assets::FpkSource>>(&original_sound);
        check(original_sound_source != nullptr, "Original collision archive TOC unavailable");
        const auto original_winner = (*original_sound_source)->find(assets::AssetKey("explosion200poundwater-002.wav"));
        check(std::holds_alternative<assets::AssetHandle>(original_winner) &&
              std::get<assets::AssetHandle>(original_winner).read_bytes(64*1024*1024) == sound.read_bytes(64*1024*1024),
              "Native original selected collision payload differs from canonical SoundData");
        const auto canonical_unit_models = assets::FpkSource::open(std::filesystem::path(argv[3]) /
            "Resource/Common/UnitModels.fpk", "Resource/Common/UnitModels.fpk");
        const auto incoming_asset = std::get<std::shared_ptr<assets::FpkSource>>(canonical_unit_models)->
            find(assets::AssetKey("work_boat_death.dge"));
        const auto& incoming = std::get<assets::AssetHandle>(incoming_asset);
        const auto other = owner->resources().resolve(assets::AssetKey("Civ5ArtDefines_UnitMembers.xml"));
        const auto& loose = std::get<assets::AssetHandle>(other.asset);
        // Actual canonical handles in a pure isolated index do not issue the
        // namespace/stat/cache Borrow required by private category mode7.
        ui::detail::NativeResourceRecordIndex unowned;
        check(unowned.insert("explosion200poundwater-002.wav",sound,0),"Mode7 refusal fixture missing actual archive payload");
        const auto old_first=unowned.first_record_names;
        const auto old_order=unowned.bucket_order.ordered_nodes();
        rejects([&]{(void)unowned.insert("work_boat_death.dge",incoming,7);});
        check(unowned.first_record_names==old_first && unowned.bucket_order.ordered_nodes()==old_order &&
              unowned.collisions.empty() && unowned.records.size()==1 &&
              std::get<assets::AssetHandle>(unowned.records.at(0x6b70aef3)).candidate().provenance.archive_relative_path==
                  sound.candidate().provenance.archive_relative_path,
              "Unowned mode7 refusal changed archive payload/FIRST/order/collision diagnostics");
        check(!unowned.insert("work_boat_death.dge",incoming,1) &&
              unowned.insert("work_boat_death.dge",incoming,4) &&
              unowned.first_record_names==old_first && unowned.bucket_order.live_nodes()==1 &&
              unowned.collisions.size()==2 && unowned.collisions[0].mode==1 && !unowned.collisions[0].replaced &&
              unowned.collisions[1].mode==4 && unowned.collisions[1].replaced,
              "Legacy loose-only/archive-only collision replacement changed");
        ui::detail::NativeResourceRecordIndex loose_only;
        const auto loose_name=loose.candidate().original_name;
        check(loose_only.insert(loose_name,loose,0) && !loose_only.insert(loose_name,loose,4) &&
              loose_only.insert(loose_name,loose,1) && loose_only.insert(loose_name,loose,2) &&
              loose_only.records.size()==1 && loose_only.bucket_order.live_nodes()==1,
              "Legacy loose0/1/2/archive-only4 branch changed");
        ui::detail::NativeResourceRecordIndex fixture;
        check(fixture.insert("explosion200poundwater-002.wav", sound, 0) &&
              !fixture.insert("work_boat_death.dge", incoming, 0) && fixture.records.size() == 1 &&
              std::get<assets::AssetHandle>(fixture.lookup(assets::AssetKey("work_boat_death.dge")).asset).
                  candidate().provenance.archive_relative_path == "Resource/Common/SoundData.fpk" &&
              fixture.insert("work_boat_death.dge", incoming, 2) && fixture.records.size() == 1 &&
              std::get<assets::AssetHandle>(fixture.lookup(assets::AssetKey("explosion200poundwater-002.wav")).asset).
                  candidate().original_name == incoming.candidate().original_name &&
              fixture.keys({}) == std::vector<std::string>{incoming.candidate().original_name} &&
              fixture.collisions.size() == 2 && !fixture.collisions[0].replaced && fixture.collisions[1].replaced,
              "Native collision mode0/mode2 controlled index transition diverged");
        check(fixture.first_record_names.at(0x6b70aef3) == "explosion200poundwater-002.wav",
              "Native mode2 replacement changed immutable FIRST CRC filename");
        // Selected name is now incoming nameB, but loose reconstruction still
        //uses global first nameA. Reject before mutating any selected state.
        const auto prior_collisions = fixture.collisions.size();
        rejects([&] { fixture.insert("work_boat_death.dge", loose, 2); });
        check(fixture.first_record_names.at(0x6b70aef3) == "explosion200poundwater-002.wav" &&
              fixture.record_names.at(0x6b70aef3) == "work_boat_death.dge" &&
              fixture.collisions.size() == prior_collisions &&
              std::get<assets::AssetHandle>(fixture.records.at(0x6b70aef3)).candidate().original_name ==
                  incoming.candidate().original_name,
              "Native same-selected-name loose transition ignored FIRST name or mutated rejected state");
        ui::detail::NativeResourceRecordIndex guarded_loose;
        check(guarded_loose.insert("explosion200poundwater-002.wav", sound, 0), "Loose guard fixture initial insertion failed");
        rejects([&] { guarded_loose.insert("work_boat_death.dge", loose, 2); });
        check(guarded_loose.records.size() == 1 &&
              std::get<assets::AssetHandle>(guarded_loose.lookup(assets::AssetKey("work_boat_death.dge")).asset).
                  candidate().original_name == sound.candidate().original_name,
              "Unsupported global-name loose replacement changed prior selected record");
        rejects([&] { fixture.insert("unknown-\xc3\x92-name.wav", sound, 0); });
        rejects([&] { fixture.lookup(assets::AssetKey("unknown-\xc3\x92-name.wav")); });
        check(fixture.records.size() == 1, "Unsupported UTF8 CRC request changed isolated index");
        const auto receipt = std::find_if(common.resource_crc_collisions.begin(), common.resource_crc_collisions.end(),
            [](const auto& entry) { return entry.hash == 0x6b70aef3 && entry.previous_name ==
                "explosion200poundwater-002.wav" && entry.incoming_name == "work_boat_death.dge"; });
        check(receipt != common.resource_crc_collisions.end() && receipt->mode == 0 && !receipt->replaced,
              "Actual Common collision source receipt missing");
        std::cout << "CRC COLLISION 6b70aef3 SoundData.wav preserves UnitModels.dge; BOTH canonical reads equal; mode2 fixture PASS" << std::endl;
        check(owner->color_provider().current()->unresolved_provider &&
                  !owner->current().native_skin_loading_complete && !owner->current().color_source_set_closed,
              "Indexing default Color payload fabricated actual native Color/Skin load order");
        for (const auto& unreadable : common.indexed_unreadable_loose) {
            check(!unreadable.native_basename.empty() && unreadable.bytes > 0 && unreadable.last_write_time_raw > 0 &&
                      (unreadable.open_error == ERROR_FILE_NOT_FOUND || unreadable.open_error == ERROR_PATH_NOT_FOUND),
                  "Native unreadable record missing actual FindA metadata/open failure");
            const auto keys = owner->resources().keys();
            check(std::find(keys.begin(), keys.end(), unreadable.relative_path) != keys.end(),
                  "Native unreadable loose record was silently dropped from the index");
            rejects([&] { owner->resources().resolve(assets::AssetKey(unreadable.native_basename)); });
            std::cout << "INDEXED UNREADABLE " << unreadable.relative_path << " size=" << unreadable.bytes
                      << " time=" << unreadable.last_write_time_raw << " error=" << unreadable.open_error << std::endl;
        }
        rejects([&] { color->quote("Beige"); });
        rejects([&] { owner->register_native_common_high_quality_prefix(); });
        std::cout << "COMMON requests=" << common.archive_registration_requests
                  << " archives=" << common.archives_registered << " loose=" << common.loose_files_registered
                  << " indexed=" << common.default_indexed_names << std::endl;
    } else {
        rejects([&] { owner->register_native_common_high_quality_prefix(); });
    }
    std::cout << "SETTINGS actual INI=" << assets::path_utf8(settings.graphics_ini)
              << " raw=" << (settings.raw_texture_quality_override ?
                    std::to_string(*settings.raw_texture_quality_override) : "absent")
              << " effective=" << *settings.effective_texture_quality
              << " vendor=" << settings.actual_adapter_vendor << " device=" << settings.actual_adapter_device
              << " archives=" << owner->current().archives_registered
              << " low-names=" << owner->current().low_indexed_names << std::endl;
    ++revision;
    rejects([&] { boot->current(); });
    rejects([&] { owner->current(); });
    rejects([&] { owner->require_native_package_category_sources_complete(); });
    rejects([&] { owner->resources(); });
    rejects([&] { owner->color_provider(); });
    --revision;
    check(&owner->resources() == same_table, "Early ResourceMountTable identity changed");
    bool foreign = false;
    std::thread other([&] {
        try { boot->current(); } catch (const std::exception&) { foreign = true; }
    });
    other.join();
    check(foreign && !host->session().readiness().can_continue(),
          "Native discovery admitted foreign thread or gameplay readiness");
    DestroyWindow(window.h);
    window.h = nullptr;
    rejects([&] { owner->current(); });
    rejects([&] { owner->require_native_package_category_sources_complete(); });
    rejects([&] { boot->current(); });
    std::cout << "PASS: original " << frame.packages << " package descriptors / "
              << frame.skin_contexts << " potential Skins / " << frame.slot_maps
              << " empty slot maps; SAME fresh early table, indexed misses/direct OS reads; "
                 "no metadata mount or postRules overload; actual process/INI and adapter/caps source and mandatory "
                 "default/dynamic Color boundary; no native Skin/loading/gameplay readiness\n";
}
