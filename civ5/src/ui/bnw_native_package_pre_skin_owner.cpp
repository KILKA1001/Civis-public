#include "civ5/ui/bnw_native_package_pre_skin_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include <algorithm>
#include <thread>
#include <fstream>
#include "bnw_native_package_key_source.hpp"
#include "bnw_native_package_category_source_owner.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::ui {
namespace {
void need(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
struct Pin { std::filesystem::path path; runtime::FileIdentity identity; };
std::string source_path(const std::filesystem::path& path) {
    const auto raw = assets::path_utf8(path);
    need(std::all_of(raw.begin(), raw.end(), [](unsigned char c) { return c < 128; }),
         "Native package declared non-ASCII CP_ACP path needs source owner");
    return raw;
}
std::filesystem::path declared_unopened_path(const std::filesystem::path& base,
    const std::string& authored, const std::filesystem::path& game_root) {
    const std::filesystem::path relative(authored);
    need(!authored.empty() && !relative.is_absolute() && !relative.has_root_name(),
         "Native declared path absolute/empty schema requires source owner");
    for (const auto& component : relative)
        need(component != std::filesystem::path(".."),
             "Native declared parent path requires source confinement owner");
    const auto candidate = (base / relative).lexically_normal();
    const auto inside = candidate.lexically_relative(game_root);
    need(!inside.empty() && !inside.is_absolute() && *inside.begin() != std::filesystem::path(".."),
         "Native declared input escaped original installation");
    // Eligibility must probe the authored target even when absent. A generic
    // package payload resolver would throw before native8910D0 FPK fallback.
    // Missing descendants are allowed; existing reparse parents are unknown.
    for (auto parent = candidate.parent_path(); parent != game_root; parent = parent.parent_path()) {
        need(!parent.empty(), "Native declared parent chain escaped original root");
        const auto attrs = GetFileAttributesA(source_path(parent).c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES)
            need((attrs & FILE_ATTRIBUTE_REPARSE_POINT) == 0,
                 "Native declared input reparse parent requires target owner");
        else {
            const auto error = GetLastError();
            need(error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND,
                 "Native declared parent probe requires OS failure owner");
        }
    }
    return candidate;
}
BnwNativeDeclaredPackageProbe probe(const char* selector, const std::string& declared,
                                     const std::filesystem::path& path) {
    BnwNativeDeclaredPackageProbe out{selector, declared, source_path(path)};
    SetLastError(ERROR_SUCCESS);
    out.attributes = GetFileAttributesA(out.actual_source_path.c_str());
    out.exists = out.attributes != INVALID_FILE_ATTRIBUTES;
    out.os_error = out.exists ? ERROR_SUCCESS : GetLastError();
    // Native829560 accepts any valid attributes, including a directory. This
    // is declared-input eligibility only; it is NOT a DLL or payload load.
    return out;
}
}
struct BnwNativePackagePreSkinOwner::Impl {
    std::weak_ptr<const BnwNativePackageDiscoveryOwner> resources;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    std::filesystem::path assets_root;
    std::vector<Pin> pins;
    BnwNativePackagePreSkinFrame frame;
    // Private comparison evidence, not caller flags or category completion.
    std::vector<native_package_key_detail::Comparison> key_comparisons;
    std::thread::id thread = std::this_thread::get_id();
    UINT code_page = GetACP();
    void validate(const BnwNativePackagePreSkinOwner* category_borrow=nullptr) const {
        need(thread == std::this_thread::get_id() && GetACP() == code_page,
             "Native package predecessor foreign thread/code page");
        const auto parent = resources.lock();
        need(static_cast<bool>(parent), "Native pre-Skin actual package parent expired");
        lease->require_current();
        if(category_borrow)parent->require_category_pre_skin_borrow_current(*category_borrow);
        else need(parent->current().common_high_quality_registration_complete,
             "Native pre-Skin predecessor lost SAME Common resource epoch");
        for (const auto& pin : pins) {
            const auto id = runtime::identify_file(pin.path);
            need(id.bytes == pin.identity.bytes && id.sha256 == pin.identity.sha256,
                 "Native pre-Skin descriptor changed");
        }
        need(key_comparisons.size()==frame.packages.size(),"Native Key retained comparison order changed");
        for(std::size_t n=0;n<frame.packages.size();++n) {
            const auto& row=frame.packages[n];
            if(key_comparisons[n]==native_package_key_detail::Comparison::matched)
                need((row.known_flags_mask&1) && (row.known_flags_value&1) && row.key_integrity_bit==true,
                     "Native Key SAME retained equality lost OR bit0");
            else
                need(!(row.known_flags_mask&1) && !row.key_integrity_bit,
                     "Native rejected/mismatched Key cannot manufacture or clear bit0 knowledge");
        }
        for (const auto& row : frame.packages)
            for (const auto& p : row.probes) {
                const auto now = probe(p.selector.c_str(), p.declared_path, p.actual_source_path);
                need(now.attributes == p.attributes && now.os_error == p.os_error && now.exists == p.exists,
                     "Native declared package OS source probe changed");
            }
        if(category_borrow)parent->require_category_pre_skin_borrow_current(*category_borrow);
        else (void)parent->current();
        lease->require_current();
    }
};
BnwNativePackagePreSkinOwner::BnwNativePackagePreSkinOwner(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}
BnwNativePackagePreSkinOwner::~BnwNativePackagePreSkinOwner() = default;
std::shared_ptr<const BnwNativePackagePreSkinOwner> BnwNativePackagePreSkinOwner::create(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> resources) {
    need(resources && resources->current().common_high_quality_registration_complete,
         "Native pre-Skin predecessor requires SAME actual Common stage");
    auto i = std::make_shared<Impl>(); i->resources = resources;
    i->lease = resources->early_input_lease();
    i->assets_root = resources->original_assets_root_for_pre_skin();
    const auto& discovery = resources->current();
    bool tablet_transition_applied = false;
    for (const auto& relative : discovery.original_descriptor_order) {
        const auto path = content::package_file(i->assets_root, relative);
        const auto before = runtime::identify_file(path);
        const auto& manifest = resources->retained_descriptor_for_pre_skin(relative);
        need(manifest.path==path,"Native PreSkin descriptor path differs from SAME retained parent");
        const auto after = runtime::identify_file(path);
        need(before.bytes == after.bytes && before.sha256 == after.sha256,
             "Native pre-Skin descriptor changed during read");
        i->pins.push_back({path, before});
        BnwNativePackagePreSkinRow row;
        row.descriptor = relative; row.guid = content::canonical_guid(manifest.id);
        // Actual738F00->8916A0/891460->641A40 path from SAME parent's XML,
        // with original-file lexical witness and existing lease/source bookends.
        // No selected-save manifest, normalized manifest.id/version or caller buffer.
        resources->current(); i->lease->require_current();
        need(before.bytes<=64ULL*1024*1024,"Native Key descriptor exceeds source budget");
        std::ifstream raw_input(path,std::ios::binary);
        need(static_cast<bool>(raw_input),"Native Key original descriptor open failed");
        std::string original_xml(static_cast<std::size_t>(before.bytes),'\0');
        raw_input.read(original_xml.data(),static_cast<std::streamsize>(original_xml.size()));
        need(raw_input.gcount()==static_cast<std::streamsize>(original_xml.size()) &&
             raw_input.peek()==std::char_traits<char>::eof(),"Native Key original descriptor read changed size");
        const auto key_result=native_package_key_detail::compare_retained_xml(manifest.document,original_xml);
        const auto key_after=runtime::identify_file(path);
        need(key_after.bytes==before.bytes && key_after.sha256==before.sha256,
             "Native Key original descriptor changed during source computation");
        resources->current(); i->lease->require_current();
        if(key_result==native_package_key_detail::Comparison::matched) {
            // Exact641AF0 OR1. Mismatch/rejected helper paths perform no bit write.
            row.known_flags_value|=1U;row.known_flags_mask|=1U;row.key_integrity_bit=true;
        }
        i->key_comparisons.push_back(key_result);
        bool ownership_seen = false, upgrade_seen = false;
        for (const auto& node : manifest.document.children) {
            if (node.name == "Ownership") {
                need(!ownership_seen, "Native repeated Ownership requires first-node parser owner");
                ownership_seen = true;
            }
            if (node.name == "Ownership" && runtime::ascii_lower(node.text) == "free") row.known_flags_value |= 0x10;
            if (node.name == "Upgrade") {
                need(!upgrade_seen, "Native repeated Upgrade requires first-node parser owner");
                upgrade_seen = true;
                const auto value = content::decimal(node.text);
                need(value <= UINT32_MAX, "Native package Upgrade DWORD source unsupported");
                if (value > 0) row.known_flags_value |= 0x20;
            }
        }
        // Exact54CCD6..54CD3A source GUID transition. Source-found package
        // only; absence is not converted into a synthetic Tablet package.
        if (!tablet_transition_applied && row.guid == "8871e748-29a4-4910-8c57-8c99e32d0167") {
            row.known_flags_value |= 0x20; tablet_transition_applied = true;
        }
        row.early_query_gate_closed = true; // known bit2=1; bit0 is irrelevant to54B902 branch.
        bool valid = true;
        for (const auto* selector : {"GameCore", "Paks"}) {
            if (!valid) break; //686D80 never validates Paks after failed GameCore.
            content::PackageResourceQuery query;
            query.selector = selector; query.enabled_categories = {true,true,true};
            for (const auto& declaration : content::package_resources(manifest, query)) {
                const auto declared = declared_unopened_path(path.parent_path(), declaration.value,
                                                             i->lease->game_root());
                auto selected = probe(selector, declaration.value, declared);
                row.probes.push_back(selected);
                if (selected.exists) continue;
                //8910D0 only permits fallback for a missing .fpk. The basename
                // stem, not its authored package directory, is used in paths.
                if (runtime::ascii_lower(assets::path_utf8(declared.extension())) == ".fpk") {
                    for (const auto* prefix : {"Resource/DX9", "Resource/Common"}) {
                        const auto fallback = i->assets_root.parent_path() / prefix / declared.filename();
                        selected = probe(selector, declaration.value, fallback);
                        row.probes.push_back(selected);
                        if (selected.exists) break;
                    }
                }
                if (!selected.exists) { valid = false; break; }
            }
        }
        row.declared_inputs_valid = valid;
        if (!valid) row.known_flags_value |= 0x40;
        i->frame.packages.push_back(std::move(row));
    }
    i->frame.tablet_guid_transition_complete = true;
    i->frame.declared_input_validation_complete = true;
    i->frame.next_dependency = "656440 category0/1 paired Paks caches, original archive times, mode7 loose insertion,67B4E0 preference and5D5550 registration before52D590";
    // Key equality is computed only from supported SAME retained source operands.
    // This does not construct category managers, enable sources or run Skin loading.
    i->validate();
    return std::shared_ptr<const BnwNativePackagePreSkinOwner>(new BnwNativePackagePreSkinOwner(std::move(i)));
}
const BnwNativePackagePreSkinFrame& BnwNativePackagePreSkinOwner::current() const {
    impl_->validate(); return impl_->frame;
}
void BnwNativePackagePreSkinOwner::require_same_resources(const BnwNativePackageDiscoveryOwner& resources) const {
    impl_->validate(); need(&resources == impl_->resources.lock().get(), "Native package predecessor requires SAME resource owner");
    impl_->validate();
}
void BnwNativePackagePreSkinOwner::require_category_mount_stage_complete() const {
    impl_->validate();const auto parent=impl_->resources.lock();
    need(bool(parent),"Native Category completion Package expired");
    const detail::BnwNativeCategoryCompletionParentPin borrowed_parent(parent,*this);
    parent->require_category_completion_for(*this,borrowed_parent);
    // Completion validated SAME PreSkin under the active source fences. No
    // callback may follow its final external-owner bookend under this pin.
    need(parent.use_count()>1,"Native Category completion lost external Package facade");
}
const BnwNativePackagePreSkinFrame& BnwNativePackagePreSkinOwner::category_operation_frame(
    const BnwNativePackageDiscoveryOwner& parent)const {
    need(impl_->resources.lock().get()==&parent,"Category borrowed another PreSkin parent");
    parent.require_category_pre_skin_borrow_current(*this);
    impl_->validate(this);return impl_->frame;
}
}
