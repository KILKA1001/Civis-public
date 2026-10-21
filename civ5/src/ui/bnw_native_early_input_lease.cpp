#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "bnw_native_common_source_validation_context.hpp"
#include "civ5/ui/bnw_native_pinned_input_epoch.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <atomic>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace civ5::ui {
namespace {
void need(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
bool same(const runtime::BnwStockLuaClientExtent& a,
          const runtime::BnwStockLuaClientExtent& b) {
    return a.surface_owner == b.surface_owner && a.revision == b.revision &&
           a.width == b.width && a.height == b.height;
}
// Read-only handles deny source replacement/write while this input lease lives.
// They are closed on every failure and release; no persistent files are created.
struct PinnedFile {
    HANDLE handle = INVALID_HANDLE_VALUE;
    runtime::FileIdentity identity;
    const std::filesystem::path source_path;
    BY_HANDLE_FILE_INFORMATION captured{};
    explicit PinnedFile(const std::filesystem::path& path):source_path(path) {
        handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                             nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        need(handle != INVALID_HANDLE_VALUE, "Early BNW source cannot be pinned read-only");
        try {
            need(GetFileInformationByHandle(handle,&captured)!=FALSE,
                 "Early source actual read-only file identity unavailable");
            identity = runtime::identify_file(path);
        }
        catch (...) { CloseHandle(handle); handle = INVALID_HANDLE_VALUE; throw; }
    }
    void require_same_pinned_source()const {
        BY_HANDLE_FILE_INFORMATION held{},current{};
        need(handle!=INVALID_HANDLE_VALUE&&GetFileInformationByHandle(handle,&held)!=FALSE,
             "Pinned input original read-only handle retired");
        const auto same=[](const auto& a,const auto& b) {
            return a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&
                a.nFileIndexHigh==b.nFileIndexHigh&&a.nFileIndexLow==b.nFileIndexLow&&
                a.nFileSizeHigh==b.nFileSizeHigh&&a.nFileSizeLow==b.nFileSizeLow&&
                a.ftLastWriteTime.dwHighDateTime==b.ftLastWriteTime.dwHighDateTime&&
                a.ftLastWriteTime.dwLowDateTime==b.ftLastWriteTime.dwLowDateTime;
        };
        need(same(held,captured),"Pinned input original locked file changed");
        // ORIGINAL handles keep denying write/delete. Reopen only to prove
        // that the path still names the SAME retained file, not a replacement.
        const auto path_handle=CreateFileW(source_path.c_str(),GENERIC_READ,FILE_SHARE_READ,
            nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        need(path_handle!=INVALID_HANDLE_VALUE,"Pinned input source path disappeared");
        const bool read=GetFileInformationByHandle(path_handle,&current)!=FALSE;
        CloseHandle(path_handle);
        need(read&&same(current,captured),"Pinned input path no longer names the original source");
    }
    ~PinnedFile() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
    PinnedFile(const PinnedFile&) = delete;
    PinnedFile& operator=(const PinnedFile&) = delete;
};
}
struct BnwNativeEarlyInputLease::Impl {
    std::filesystem::path save, game, payload;
    saves::SaveHeader header;
    content::Catalog catalog;
    runtime::SavedContentProfile profile;
    HWND hwnd = nullptr;
    DWORD process = GetCurrentProcessId(), thread = GetCurrentThreadId();
    runtime::BnwStockLuaClientExtentProvider client;
    runtime::BnwStockLuaClientExtent extent;
    std::vector<std::unique_ptr<PinnedFile>> files;
    std::atomic<bool> early_facade_alive{true};
    std::weak_ptr<const BnwNativeEarlyInputLease> canonical_facade;
    const runtime::FileIdentity& pin(const std::filesystem::path& path) {
        files.push_back(std::make_unique<PinnedFile>(path));
        return files.back()->identity;
    }
    void require_facade_current_pure()const {
        // Creator precedes canonical weak storage or any mutable source read.
        need(GetCurrentProcessId()==process&&GetCurrentThreadId()==thread,
             "Early canonical input belongs to another process/thread");
        need(early_facade_alive.load()&&!canonical_facade.expired(),
             "Early canonical input facade retired");
    }
    void validate_pinned_source_epoch()const {
        need(GetCurrentProcessId()==process&&GetCurrentThreadId()==thread,
             "Pinned input epoch belongs to another process/thread");
        need(std::filesystem::is_directory(game/"Assets/DLC")&&
             std::filesystem::is_directory(payload/"Assets"),
             "Pinned input source roots disappeared");
        for(const auto& file:files)file->require_same_pinned_source();
    }
    void validate() const {
        require_facade_current_pure();
        need(early_facade_alive.load(),"Early BNW input facade retired");
        need(GetCurrentProcessId() == process && GetCurrentThreadId() == thread,
             "Early BNW input belongs to another process/thread");
        DWORD owner = 0; RECT rc{};
        need(IsWindow(hwnd) && GetWindowThreadProcessId(hwnd, &owner) == thread &&
             owner == process && !IsIconic(hwnd) && GetClientRect(hwnd, &rc) &&
             rc.right - rc.left == extent.width && rc.bottom - rc.top == extent.height,
             "Early BNW input requires the same available owned HWND");
        const auto current = client.current();
        require_facade_current_pure();
        need(current && same(*current, extent), "Early BNW client epoch changed");
        need(early_facade_alive.load(),"Early BNW input retired during actual client callback");
        // The provider may synchronously mutate its HWND while returning the
        // old extent. Recheck the actual window after its callback.
        owner=0; rc={};
        need(IsWindow(hwnd) && GetWindowThreadProcessId(hwnd, &owner) == thread &&
             owner == process && !IsIconic(hwnd) && GetClientRect(hwnd, &rc) &&
             rc.right - rc.left == extent.width && rc.bottom - rc.top == extent.height,
             "Early BNW owned HWND changed during actual client callback");
        need(std::filesystem::is_directory(game / "Assets/DLC") &&
             std::filesystem::is_directory(payload / "Assets"),
             "Early BNW original/canonical resource root disappeared");
    }
};
BnwNativeEarlyInputLease::BnwNativeEarlyInputLease(std::shared_ptr<Impl> impl)
    : impl_(std::move(impl)) {}
BnwNativeEarlyInputLease::~BnwNativeEarlyInputLease() { impl_->early_facade_alive.store(false); }
std::shared_ptr<const BnwNativeEarlyInputLease> BnwNativeEarlyInputLease::acquire(
    const std::filesystem::path& save, const std::filesystem::path& game,
    const std::filesystem::path& payload, std::uintptr_t hwnd,
    runtime::BnwStockLuaClientExtentProvider client) {
    need(hwnd && client.current, "Early BNW actual client is missing");
    auto impl = std::make_shared<Impl>();
    auto facade=std::shared_ptr<const BnwNativeEarlyInputLease>(
        new BnwNativeEarlyInputLease(impl));
    impl->canonical_facade=facade;
    impl->require_facade_current_pure();
    const auto extent = client.current();
    impl->require_facade_current_pure();
    need(extent && !extent->surface_owner.empty() && extent->revision &&
         extent->width > 0 && extent->height > 0, "Early BNW client extent is unavailable");
    impl->save = std::filesystem::canonical(save);
    impl->game = std::filesystem::canonical(game);
    impl->payload = std::filesystem::canonical(payload);
    impl->hwnd = reinterpret_cast<HWND>(hwnd);
    impl->client = std::move(client); impl->extent = *extent;
    impl->validate();
    impl->pin(impl->save);
    // LoadedSave verifies framing for both supported container types. A saved
    // override/profile cannot become a new source for the stock pre-DLC path.
    auto loaded = saves::read_loaded_save(impl->save);
    need(loaded.source_prefix.has_value() && !loaded.saved_database_override &&
         loaded.profile_sections.empty(), "Early BNW input has unowned profile/SQL state");
    impl->header = std::move(loaded.header);
    const auto& exe = impl->pin(impl->game / "CivilizationV.exe");
    need(exe.sha256 == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686",
         "Early BNW original executable differs from the verified source");
    const auto& core = impl->pin(impl->game / "Assets/DLC/Expansion2/CvGameCore_Expansion2.dll");
    need(core.bytes == 4061696 && core.sha256 ==
         "c5483b5857a6fb7ef107af93c5d34282eb713472e2a84d0eb9d3fdc104e155a5",
         "Early BNW Expansion2 GameCore differs from the verified source");
    impl->catalog = content::discover_packages({impl->game / "Assets/DLC"});
    // Pin then rediscover: parsed manifests must be bytes from the retained
    // read-only epoch, not the preceding unlocked discovery window.
    for (const auto& package : impl->catalog.packages) impl->pin(package.path);
    auto pinned_catalog = content::discover_packages({impl->game / "Assets/DLC"});
    need(pinned_catalog.packages.size() == impl->catalog.packages.size(),
         "Early BNW package discovery changed while pinning");
    for (std::size_t n = 0; n < pinned_catalog.packages.size(); ++n)
        need(pinned_catalog.packages[n].path == impl->catalog.packages[n].path,
             "Early BNW package order changed while pinning");
    impl->catalog = std::move(pinned_catalog);
    impl->profile = runtime::resolve_saved_content_profile(impl->header, impl->catalog);
    need(impl->profile.can_activate && impl->profile.enabled_mods.empty() &&
         !impl->profile.native_mods_require_port, "Early BNW selected profile is unavailable");
    const bool bnw = std::any_of(impl->profile.enabled_dlc.begin(), impl->profile.enabled_dlc.end(),
        [&](std::size_t index) { return content::canonical_guid(impl->catalog.packages.at(index).id) ==
            "6da07636-4123-4018-b643-6575b4ec336b"; });
    need(bnw, "Early input does not select BNW");
    impl->validate();
    impl->require_facade_current_pure();
    return facade;
}
std::shared_ptr<const BnwNativeEarlyInputLease> BnwNativeEarlyInputLease::retain_owner() const {
    const auto input=impl_;input->require_facade_current_pure();
    auto actual=input->canonical_facade.lock();
    need(actual&&actual.get()==this,"Early input is not its actual factory facade");
    input->require_facade_current_pure();return actual;
}
void BnwNativeEarlyInputLease::require_current() const { const auto input=impl_;input->validate(); }
void BnwNativeEarlyInputLease::require_cold_roots_profile_identity_without_callbacks(
    const std::filesystem::path& original_root,const std::filesystem::path& canonical_root)const {
    const auto& input=*impl_; // Existing facade owns Impl; no new strong pin.
    input.require_facade_current_pure();
    need(original_root==input.game && canonical_root==input.payload &&
         original_root.is_absolute() && canonical_root.is_absolute(),
         "Cold original logical source base/canonical payload lease changed");
    const auto& profile=input.profile;
    need(profile.can_activate && profile.enabled_mods.empty() && !profile.native_mods_require_port,
         "Cold filesystem requires the pinned stock source profile");
    input.require_facade_current_pure();
}
const saves::SaveHeader& BnwNativeEarlyInputLease::selected_header() const { const auto input=impl_;input->validate();return input->header; }
const content::Catalog& BnwNativeEarlyInputLease::catalog() const { const auto input=impl_;input->validate();return input->catalog; }
const runtime::SavedContentProfile& BnwNativeEarlyInputLease::profile() const { const auto input=impl_;input->validate();return input->profile; }
const std::filesystem::path& BnwNativeEarlyInputLease::save_path() const { const auto input=impl_;input->validate();return input->save; }
const std::filesystem::path& BnwNativeEarlyInputLease::game_root() const { const auto input=impl_;input->validate();return input->game; }
const std::filesystem::path& BnwNativeEarlyInputLease::canonical_payload_root() const { const auto input=impl_;input->validate();return input->payload; }
std::uintptr_t BnwNativeEarlyInputLease::actual_hwnd() const { const auto input=impl_;input->validate();return reinterpret_cast<std::uintptr_t>(input->hwnd); }
const runtime::BnwStockLuaClientExtentProvider& BnwNativeEarlyInputLease::client_provider() const { const auto input=impl_;input->validate();return input->client; }
const runtime::BnwStockLuaClientExtent& BnwNativeEarlyInputLease::client_extent() const { const auto input=impl_;input->validate();return input->extent; }
struct BnwNativePinnedInputEpoch::Impl {
    const std::shared_ptr<const BnwNativeEarlyInputLease::Impl> original;
    explicit Impl(std::shared_ptr<const BnwNativeEarlyInputLease::Impl> input):original(std::move(input)){}
};
BnwNativePinnedInputEpoch::BnwNativePinnedInputEpoch(std::shared_ptr<const Impl> input):impl_(std::move(input)) {require_current();}
BnwNativePinnedInputEpoch::~BnwNativePinnedInputEpoch()=default;
void BnwNativePinnedInputEpoch::require_current()const {const auto epoch=impl_;epoch->original->validate_pinned_source_epoch();}
void BnwNativePinnedInputEpoch::require_same_early_input(const BnwNativeEarlyInputLease& actual)const {
    require_current();need(impl_->original.get()==actual.impl_.get(),"Pinned input epoch substituted original storage");require_current();
}
std::shared_ptr<const BnwNativePinnedInputEpoch> BnwNativeEarlyInputLease::capture_pinned_input_epoch()const {
    const auto input=impl_;input->validate();
    auto epoch=std::shared_ptr<const BnwNativePinnedInputEpoch>(new BnwNativePinnedInputEpoch(
        std::make_shared<const BnwNativePinnedInputEpoch::Impl>(input)));
    input->validate();epoch->require_current();return epoch;
}
const saves::SaveHeader& BnwNativePinnedInputEpoch::selected_header()const {require_current();return impl_->original->header;}
const content::Catalog& BnwNativePinnedInputEpoch::catalog()const {require_current();return impl_->original->catalog;}
const runtime::SavedContentProfile& BnwNativePinnedInputEpoch::profile()const {require_current();return impl_->original->profile;}
const std::filesystem::path& BnwNativePinnedInputEpoch::save_path()const {require_current();return impl_->original->save;}
const std::filesystem::path& BnwNativePinnedInputEpoch::game_root()const {require_current();return impl_->original->game;}
const std::filesystem::path& BnwNativePinnedInputEpoch::canonical_payload_root()const {require_current();return impl_->original->payload;}
void BnwNativeEarlyInputLease::require_common_source_current(detail::BnwNativeCommonSourceValidationContext& context)const {
    context.check();const auto input=impl_;
    const auto check=[&]{context.check();need(input->early_facade_alive.load()&&
        GetCurrentProcessId()==input->process&&GetCurrentThreadId()==input->thread,
        "Common Early input creator/facade retired");};
    check();auto visit=context.enter_early(*this);
    if(visit.work()==detail::BnwNativeCommonSourceValidationContext::Work::validate_source)input->validate();
    check();visit.complete();
}
const std::filesystem::path& BnwNativeEarlyInputLease::game_root_common_source(detail::BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);context.require_early_borrowed(*this);return impl_->game;
}
const std::filesystem::path& BnwNativeEarlyInputLease::canonical_payload_root_common_source(detail::BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);context.require_early_borrowed(*this);return impl_->payload;
}
const runtime::SavedContentProfile& BnwNativeEarlyInputLease::profile_common_source(detail::BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);context.require_early_borrowed(*this);return impl_->profile;
}
}
