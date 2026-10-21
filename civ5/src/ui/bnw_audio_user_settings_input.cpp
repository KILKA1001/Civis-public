#include "civ5/ui/bnw_audio_user_settings_input.hpp"
#include "civ5/ui/bnw_audio_saved_volume_numeric.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "bnw_audio_user_settings_input_detail.hpp"
#include <atomic>
#include <cstdio>
#include <fstream>
#include <thread>

namespace civ5::ui {
using audio_settings_detail::settings_need;
namespace {
bool same_identity(const runtime::FileIdentity& a, const runtime::FileIdentity& b) {
    return a.bytes == b.bytes && a.sha256 == b.sha256;
}
}
struct BnwAudioUserSettingsInput::Impl {
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> packages;
    std::shared_ptr<const BnwNativeUserDirectoryOwner> directory;
    std::shared_ptr<const BnwAudioSourceCatalog> catalog;
    std::shared_ptr<const assets::ResourceMountTable> resources;
    std::filesystem::path logical_path, selected_path;
    runtime::FileIdentity file;
    std::array<BnwAudioUserVolumeInput, 4> values;
    const std::thread::id thread = std::this_thread::get_id();
    std::atomic<bool> retired{false};
    bool busy = false;
    void check() const { settings_need(!retired.load(), "Audio settings input retired"); }
    void validate() const {
        check(); directory->require_same_resources(*packages); check();
        catalog->require_current(); check();
        settings_need(catalog->resources().get() == resources.get() &&
                      &packages->resources() == resources.get(), "Audio settings foreign resource allocation");
        check();
        settings_need(directory->current() / L"UserSettings.ini" == logical_path &&
                      std::filesystem::weakly_canonical(logical_path) == selected_path,
                      "Native UserSettings namespace/selected target changed");
        check();
        settings_need(std::filesystem::is_regular_file(selected_path) &&
                      std::filesystem::file_size(selected_path) <= 1024 * 1024,
                      "Native UserSettings selected file/budget changed");
        settings_need(same_identity(runtime::identify_bytes(audio_settings_detail::read_settings_raw(selected_path)), file),
                      "Native UserSettings bytes changed");
        check();
    }
    struct Call {
        std::shared_ptr<Impl> owner;
        explicit Call(std::shared_ptr<Impl> p) : owner(std::move(p)) {
            settings_need(owner && owner->thread == std::this_thread::get_id(), "Audio settings input foreign thread");
            owner->check();
            if (owner->busy) { owner->retired.store(true); throw std::runtime_error("Audio settings input reentry"); }
            owner->busy = true;
        }
        ~Call() { owner->busy = false; }
    };
};
BnwAudioUserSettingsInput::BnwAudioUserSettingsInput(std::shared_ptr<Impl> p) : impl_(std::move(p)) {}
std::shared_ptr<const BnwAudioUserSettingsInput> BnwAudioUserSettingsInput::retain_factory_owner(
    const std::shared_ptr<const BnwAudioUserSettingsInput>& supplied) const {
    const auto genuine = factory_owner_.lock();
    settings_need(genuine && genuine.get() == this && supplied.get() == this &&
         !genuine.owner_before(supplied) && !supplied.owner_before(genuine),
         "BnwAudioUserSettingsInput supplied forged factory ownership");
    return genuine;
}
std::shared_ptr<const BnwAudioUserSettingsInput> BnwAudioUserSettingsInput::from_existing_native_directory(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> packages,
    std::shared_ptr<const BnwNativeUserDirectoryOwner> directory,
    std::shared_ptr<const BnwAudioSourceCatalog> catalog) {
    settings_need(packages && directory && catalog, "Audio settings requires actual retained source owners");
    // Authenticate and retain all actual dependency control blocks BEFORE
    // any currentness/resource observer can release caller-owned facades.
    // Actual73 callback-free canonical helper; compare caller ownership
    // BEFORE any currentness/resource callback or dependency storage.
    const auto canonical_packages=packages->retain_factory_owner();
    settings_need(canonical_packages&&canonical_packages.get()==packages.get()&&
         !canonical_packages.owner_before(packages)&&!packages.owner_before(canonical_packages),
         "Audio supplied forged package factory ownership");
    packages=canonical_packages;
    directory = directory->retain_factory_owner(directory);
    catalog = catalog->retain_factory_owner(catalog);
    auto p = std::make_shared<Impl>();
    p->packages = std::move(packages); p->directory = std::move(directory); p->catalog = std::move(catalog);
    p->directory->require_same_resources(*p->packages);
    p->resources = p->catalog->resources();
    settings_need(&p->packages->resources() == p->resources.get(), "Audio settings supplied foreign Source174 catalog");
    p->logical_path = p->directory->current() / L"UserSettings.ini";
    p->selected_path = std::filesystem::weakly_canonical(p->logical_path);
    settings_need(std::filesystem::is_regular_file(p->selected_path) &&
                  std::filesystem::file_size(p->selected_path) <= 1024 * 1024,
                  "Native UserSettings selected existing file unavailable/budget exceeded");
    const auto before = runtime::identify_bytes(audio_settings_detail::read_settings_raw(p->selected_path));
    const auto bytes = audio_settings_detail::read_settings_raw(p->selected_path);
    p->file = runtime::identify_bytes(bytes);
    settings_need(same_identity(before, p->file), "UserSettings changed before parse");
    const auto raw = audio_settings_detail::parse_audio_settings_existing_subset(
        std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
    constexpr std::array<const char*, 4> keys{"Music.Volume", "Effects.Volume", "Ambience.Volume", "Speech.Volume"};
    constexpr std::array<const char*, 4> knobs{"USER_VOLUME_MUSIC", "USER_VOLUME_SFX", "USER_VOLUME_AMBIENCE", "USER_VOLUME_SPEECH"};
    for (std::size_t i = 0; i < raw.size(); ++i) p->values[i] = {
        keys[i], knobs[i], raw[i], audio_settings_detail::knob_from_saved_volume_percent(raw[i])};
    p->validate();
    auto result = std::shared_ptr<BnwAudioUserSettingsInput>(new BnwAudioUserSettingsInput(std::move(p)));
    result->factory_owner_ = result; // Private factory, weak pin; no callback.
    return result;
}
std::shared_ptr<const BnwAudioUserSettingsInput> BnwAudioUserSettingsInput::from_completed_resources(
    std::shared_ptr<const BnwAudioCompletedResourceOwner> owner,
    std::shared_ptr<const BnwAudioSourceCatalog> catalog) {
    settings_need(owner&&catalog,"Completed audio settings requires genuine source owners");
    owner=owner->retain_factory_owner(owner);catalog=catalog->retain_factory_owner(catalog);
    catalog->require_same_completed_resources(*owner);
    throw std::runtime_error("Completed audio settings requires actual completed native Directory/UserSettings source binding");
}
void BnwAudioUserSettingsInput::require_current() const {
    const auto p = impl_; Impl::Call call(p);
    try { p->validate(); } catch (...) { p->retired.store(true); throw; }
}
void BnwAudioUserSettingsInput::require_same_catalog(const BnwAudioSourceCatalog& catalog) const {
    const auto p = impl_; Impl::Call call(p);
    try { p->validate(); settings_need(p->catalog.get() == &catalog, "Audio settings supplied to foreign catalog"); }
    catch (...) { p->retired.store(true); throw; }
}
std::array<BnwAudioUserVolumeInput, 4> BnwAudioUserSettingsInput::inputs() const {
    const auto p = impl_; Impl::Call call(p);
    try { p->validate(); const auto result = p->values; p->validate(); return result; }
    catch (...) { p->retired.store(true); throw; }
}
} // namespace civ5::ui
