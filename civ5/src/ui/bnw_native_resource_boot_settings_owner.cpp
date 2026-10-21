#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include "bnw_native_installed_boot_construction_origin.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/runtime/bnw_selected_user_options_owner.hpp"
#include "civ5/runtime/bnw_net_message_debug_config_owner.hpp"
#include "civ5/runtime/bnw_net_message_debug_manager_owner.hpp"
#include "civ5/materials/material_cache_settings.hpp"
#include "civ5/ui/bnw_native_user_directory_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "bnw_native_currentness_timing_diagnostic.hpp"
#include <cstring>
#include <fstream>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <d3d9.h>

namespace civ5::ui {
namespace {
void need(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
struct Bytes {
    bool loaded = false;
    std::string value;
    bool operator==(const Bytes&) const = default;
};
Bytes read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    Bytes bytes{true, {}};
    char block[4096];
    while (input.read(block, sizeof(block)) || input.gcount()) {
        need(bytes.value.size() + input.gcount() <= 16*1024*1024,
             "Native graphics INI source budget exceeded");
        bytes.value.append(block, static_cast<std::size_t>(input.gcount()));
    }
    need(input.eof() && !input.bad(), "Native graphics INI source read failed");
    return bytes;
}
std::string actual_command_line() {
    const auto* command = GetCommandLineA();
    need(command != nullptr, "Actual process launch source unavailable");
    return command;
}
}
struct BnwNativeResourceBootSettingsOwner::Impl {
    std::weak_ptr<const BnwNativePackageDiscoveryOwner> packages;
    const assets::ResourceMountTable* table = nullptr;
    bool installation_source=false;
    DWORD process = GetCurrentProcessId();
    std::thread::id thread = std::this_thread::get_id();
    std::string command;
    std::filesystem::path selected_language_game_root;
    //CB8CF9 is in the actual PE virtual-zero .data tail. Named fresh startup
    //54CAE0's negative -LeaderViewer branch skips the only write in that
    //owned startup operation (54CB4B=1). This private fresh slot is immutable;
    //Viewer/reused native global state is a separate unsupported source.
    const std::uint8_t normal_default_loose_debug_byte = 0;
    std::shared_ptr<const BnwNativeUserDirectoryOwner> user_directory;
    std::weak_ptr<const BnwNativeResourceBootSettingsOwner> self;
    std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner> selected_input;
    std::shared_ptr<const runtime::BnwSelectedUserOptionsOwner> selected_user_options;
    bool user_options_attempted=false;
    std::shared_ptr<const runtime::BnwNetMessageDebugConfigOwner> selected_net_config;
    bool net_config_attempted=false;
    std::shared_ptr<const runtime::BnwNetMessageDebugManagerOwner> selected_net_manager;
    bool net_manager_attempted=false;
    std::shared_ptr<const BnwNativeResourceBootSettingsOwner> retain_actual_facade()const {
        need(process==GetCurrentProcessId()&&thread==std::this_thread::get_id(),
             "Native boot settings foreign process/thread facade access");
        auto actual=self.lock();
        need(actual&&actual.use_count()>1,"Native boot settings outside facade retired");
        return actual;
    }
    void require_actual_facade(const std::shared_ptr<const BnwNativeResourceBootSettingsOwner>& actual)const {
        const std::weak_ptr<const BnwNativeResourceBootSettingsOwner> expected=actual;
        // Weak ownership comparison cannot inflate the actual strong count.
        need(actual&&!self.owner_before(expected)&&!expected.owner_before(self)&&actual.use_count()>1,
             "Native boot settings outside facade retired during actual callback");
    }
    Bytes bytes;
    IDirect3D9* dx9 = nullptr;
    D3DADAPTER_IDENTIFIER9 adapter{};
    D3DCAPS9 caps{};
    bool os_32bit = false;
    ~Impl() { if (dx9) dx9->Release(); }
    BnwNativeResourceBootSettingsFrame frame;
    void capture_adapter() {
        need(dx9 == nullptr, "Native adapter source already captured");
        const auto saved = bytes.loaded ? materials::parse_material_cache_ini(bytes.value) :
                                          materials::MaterialCacheSettingsOverrides{};
        // Original8E3128 creates the API object with SDK32. The source uses
        // ordinal0, flags0 and HAL1. No device, rendering or monitor selection.
        struct Api { IDirect3D9* value = Direct3DCreate9(D3D_SDK_VERSION);
                     ~Api() { if (value) value->Release(); } } api;
        auto* query = api.value;
        D3DADAPTER_IDENTIFIER9 observed_adapter{}; D3DCAPS9 observed_caps{};
        need(query != nullptr, "Native read-only Direct3D9 producer unavailable");
        need(SUCCEEDED(query->GetAdapterIdentifier(0, 0, &observed_adapter)) &&
                 SUCCEEDED(query->GetDeviceCaps(0, D3DDEVTYPE_HAL, &observed_caps)),
             "Native read-only default adapter/caps query failed");
        SYSTEM_INFO os{}; GetNativeSystemInfo(&os);
        need(os.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ||
                 os.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
                 os.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64,
             "Native original32-process WOW64 equivalent OS architecture unknown");
        // Native839980 was called from32bit CivV. Its IsWow64Process result
        // must not be reused literally on64bit Civis: native OS architecture
        // determines what that original32-process predicate would return.
        const bool observed_32bit = os.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL;
        const materials::MaterialCacheAdapterInput adapter_input{
            observed_adapter.VendorId, observed_adapter.DeviceId,
            materials::material_cache_dx9_renderer_code({observed_caps.PixelShaderVersion,
                observed_caps.PS20Caps.Caps, observed_caps.PS20Caps.NumTemps}), observed_32bit};
        const auto configured = materials::material_cache_configure(adapter_input, saved);
        need(configured.material.preset <= 5, "Native adapter preset source unavailable");
        auto updated = frame;
        updated.adapter_caps_source_closed = true;
        updated.adapter_preset = configured.material.preset;
        updated.actual_adapter_vendor = observed_adapter.VendorId;
        updated.actual_adapter_device = observed_adapter.DeviceId;
        updated.effective_texture_quality = saved.texture_quality.value_or(
            configured.material.preset == 0 ? 0u : 1u);
        validate_source();
        adapter = observed_adapter; caps = observed_caps; os_32bit = observed_32bit;
        dx9 = api.value; api.value = nullptr; frame = std::move(updated);
    }
    void validate_source() const {
        currentness_diagnostic::Timing diagnostic_timing(currentness_diagnostic::Lane::settings,currentness_diagnostic::Phase::local_guards);
        need(process == GetCurrentProcessId() && thread == std::this_thread::get_id(),
             "Native boot settings foreign process/thread");
        need(actual_command_line() == command, "Native process launch bytes changed");
        diagnostic_timing.next(currentness_diagnostic::Phase::user_directory);
        user_directory->validate_source();
        diagnostic_timing.next(currentness_diagnostic::Phase::ini_read);
        need(read(frame.graphics_ini) == bytes,
             "Native graphics INI path/bytes changed");
        diagnostic_timing.next(currentness_diagnostic::Phase::ini_parse);
        const auto parsed = bytes.loaded ? materials::parse_material_cache_ini(bytes.value) :
                                          materials::MaterialCacheSettingsOverrides{};
        need(parsed.texture_quality == frame.raw_texture_quality_override,
             "Native TextureQuality no longer belongs to same source bytes");
        diagnostic_timing.next(currentness_diagnostic::Phase::adapter_caps);
        if (dx9) {
            D3DADAPTER_IDENTIFIER9 now_adapter{}; D3DCAPS9 now_caps{};
            need(SUCCEEDED(dx9->GetAdapterIdentifier(0, 0, &now_adapter)) &&
                     SUCCEEDED(dx9->GetDeviceCaps(0, D3DDEVTYPE_HAL, &now_caps)),
                 "Native read-only adapter/caps source unavailable");
            SYSTEM_INFO os{}; GetNativeSystemInfo(&os);
            const bool now_32 = os.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL;
            need(std::memcmp(&now_adapter, &adapter, sizeof(adapter)) == 0 &&
                     now_caps.PixelShaderVersion == caps.PixelShaderVersion &&
                     now_caps.PS20Caps.Caps == caps.PS20Caps.Caps &&
                     now_caps.PS20Caps.NumTemps == caps.PS20Caps.NumTemps && now_32 == os_32bit,
                 "Native adapter identifier/caps/OS source epoch changed");
        }
    }
};
BnwNativeResourceBootSettingsOwner::BnwNativeResourceBootSettingsOwner(std::shared_ptr<Impl> impl)
    : impl_(std::move(impl)) {}
BnwNativeResourceBootSettingsOwner::~BnwNativeResourceBootSettingsOwner() = default;
std::shared_ptr<const BnwNativeResourceBootSettingsOwner>
BnwNativeResourceBootSettingsOwner::create(std::shared_ptr<const BnwNativePackageDiscoveryOwner> packages) {
    need(static_cast<bool>(packages), "Native boot settings require SAME early resource owner");
    need(packages->current().primary_dx9_registration_complete,
         "Native boot settings require initial DX9 resource prefix");
    auto i = std::make_shared<Impl>();
    i->table = &packages->resources();
    i->packages = packages;
    i->command = actual_command_line();
    i->selected_language_game_root=std::filesystem::absolute(packages->early_input_lease()->game_root()).lexically_normal();
    // Original54CAE0 imports CRT strstr, case-sensitive substring. We close only
    // its negative normal-launch branch. Whole current command-line absence is
    // conservative even when upstream native caller passes the argument tail;
    // any occurrence (including executable path) refuses unsupported Viewer.
    need(std::strstr(i->command.c_str(), "-LeaderViewer") == nullptr,
         "Native LeaderViewer path requires its own directory/settings producer");
    i->frame.normal_launch_source_closed = true;
    i->user_directory = BnwNativeUserDirectoryOwner::create(packages);
    i->frame.graphics_ini = i->user_directory->current() / L"GraphicsSettingsDX9.ini";
    i->bytes = read(i->frame.graphics_ini);
    i->frame.ini_loaded = i->bytes.loaded;
    const auto overrides = i->bytes.loaded ? materials::parse_material_cache_ini(i->bytes.value) :
                                            materials::MaterialCacheSettingsOverrides{};
    i->frame.raw_texture_quality_override = overrides.texture_quality;
    // 5C99DE..5C9A07 happens after all adapter preset writes. Virtual+8 is
    // 831EF0→827810, whose found branch writes raw DWORD to D80DE0. Absence
    // retains the adapter-dependent default and requires the real producer below.
    i->frame.effective_texture_quality = overrides.texture_quality;
    if (!overrides.texture_quality) {
        i->capture_adapter();
    }
    i->validate_source();
    (void)packages->current();
    auto result=std::shared_ptr<const BnwNativeResourceBootSettingsOwner>(
        new BnwNativeResourceBootSettingsOwner(i));
    i->self=result;
    // Issue the actual registered user-options instance during closed Boot
    // initialization, before this Boot can be returned to startup consumers.
    (void)result->selected_user_options();
    return result;
}
std::shared_ptr<const BnwNativeResourceBootSettingsOwner>
BnwNativeResourceBootSettingsOwner::create_from_installation_source(const BnwNativePackageDiscoveryOwner& packages){
    (void)packages.installation_boot_source_table();
    auto i = std::make_shared<Impl>();
    i->table = &packages.installation_boot_source_table();i->installation_source=true;
    i->packages = packages.installation_boot_source_factory();
    i->command = actual_command_line();
    i->selected_language_game_root=packages.installation_boot_source_game_root();
    // Original54CAE0 imports CRT strstr, case-sensitive substring. We close only
    // its negative normal-launch branch. Whole current command-line absence is
    // conservative even when upstream native caller passes the argument tail;
    // any occurrence (including executable path) refuses unsupported Viewer.
    need(std::strstr(i->command.c_str(), "-LeaderViewer") == nullptr,
         "Native LeaderViewer path requires its own directory/settings producer");
    i->frame.normal_launch_source_closed = true;
    i->user_directory = BnwNativeUserDirectoryOwner::create_from_installation_boot_source(packages);
    i->frame.graphics_ini = i->user_directory->current() / L"GraphicsSettingsDX9.ini";
    i->bytes = read(i->frame.graphics_ini);
    i->frame.ini_loaded = i->bytes.loaded;
    const auto overrides = i->bytes.loaded ? materials::parse_material_cache_ini(i->bytes.value) :
                                            materials::MaterialCacheSettingsOverrides{};
    i->frame.raw_texture_quality_override = overrides.texture_quality;
    // 5C99DE..5C9A07 happens after all adapter preset writes. Virtual+8 is
    // 831EF0→827810, whose found branch writes raw DWORD to D80DE0. Absence
    // retains the adapter-dependent default and requires the real producer below.
    i->frame.effective_texture_quality = overrides.texture_quality;
    if (!overrides.texture_quality) {
        i->capture_adapter();
    }
    i->validate_source();
    (void)packages.current();
    auto result=std::shared_ptr<const BnwNativeResourceBootSettingsOwner>(
        new BnwNativeResourceBootSettingsOwner(i));
    i->self=result;
    if(auto initial=packages.borrow_initial_package_language_for_boot(*result)){
        initial->bind_actual_initial_boot(result);i->selected_input=std::move(initial);
    }
    // Issue the actual registered user-options instance during closed Boot
    // initialization, before this Boot can be returned to startup consumers.
    (void)result->selected_user_options();
    return result;
}
std::shared_ptr<const BnwNativeResourceBootSettingsOwner>
BnwNativeResourceBootSettingsOwner::create_from_installation_source(const BnwNativePackageDiscoveryOwner& packages,const BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin& origin){
    (void)packages.installation_boot_source_table(origin);
    auto i = std::make_shared<Impl>();origin.require_identity(packages);
    i->table = &packages.installation_boot_source_table(origin);i->installation_source=true;
    i->packages = packages.installation_boot_source_factory(origin);
    i->command = actual_command_line();
    i->selected_language_game_root=packages.installation_boot_source_game_root(origin);origin.require_identity(packages);
    // Original54CAE0 imports CRT strstr, case-sensitive substring. We close only
    // its negative normal-launch branch. Whole current command-line absence is
    // conservative even when upstream native caller passes the argument tail;
    // any occurrence (including executable path) refuses unsupported Viewer.
    need(std::strstr(i->command.c_str(), "-LeaderViewer") == nullptr,
         "Native LeaderViewer path requires its own directory/settings producer");
    i->frame.normal_launch_source_closed = true;
    i->user_directory = BnwNativeUserDirectoryOwner::create_from_installation_boot_source(packages,origin);origin.require_identity(packages);
    i->frame.graphics_ini = i->user_directory->current_for_installation_boot(origin) / L"GraphicsSettingsDX9.ini";origin.require_identity(packages);
    i->bytes = read(i->frame.graphics_ini);origin.require_identity(packages);
    i->frame.ini_loaded = i->bytes.loaded;
    const auto overrides = i->bytes.loaded ? materials::parse_material_cache_ini(i->bytes.value) :
                                            materials::MaterialCacheSettingsOverrides{};
    i->frame.raw_texture_quality_override = overrides.texture_quality;origin.require_identity(packages);
    // 5C99DE..5C9A07 happens after all adapter preset writes. Virtual+8 is
    // 831EF0→827810, whose found branch writes raw DWORD to D80DE0. Absence
    // retains the adapter-dependent default and requires the real producer below.
    i->frame.effective_texture_quality = overrides.texture_quality;
    if (!overrides.texture_quality) {
        i->capture_adapter();origin.require_identity(packages);
    }
    i->validate_source();origin.require_identity(packages);
    origin.require_current(packages);
    auto result=std::shared_ptr<const BnwNativeResourceBootSettingsOwner>(
        new BnwNativeResourceBootSettingsOwner(i));
    i->self=result;origin.require_identity(packages);
    if(auto initial=packages.borrow_initial_package_language_for_boot(*result)){
        initial->bind_actual_initial_boot(result);i->selected_input=std::move(initial);origin.require_identity(packages);
    }
    // Issue the actual registered user-options instance during closed Boot
    // initialization, before this Boot can be returned to startup consumers.
    (void)result->selected_user_options();origin.require_identity(packages);
    return result;
}
std::filesystem::path BnwNativeResourceBootSettingsOwner::selected_language_profile_path_without_parent_callbacks()const {
    validate_source();need(!impl_->packages.expired()&&!impl_->self.expired(),"Actual early selected language parent retired");
    const auto profile=impl_->user_directory->source_directory_without_parent_callbacks();
    validate_source();return std::filesystem::absolute(profile/L"config.ini").lexically_normal();
}
std::filesystem::path BnwNativeResourceBootSettingsOwner::selected_language_game_root_without_parent_callbacks()const {
    validate_source();need(!impl_->packages.expired()&&!impl_->self.expired(),"Actual selected game-root parent retired");
    const auto source=impl_->selected_language_game_root;validate_source();return source;
}
std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner> BnwNativeResourceBootSettingsOwner::selected_language_input()const {
    const auto actual=impl_->self.lock();need(actual&&actual.use_count()>1,"Actual early settings facade no longer retained");
    const auto path=selected_language_profile_path_without_parent_callbacks();
    if(!impl_->selected_input) {
        auto produced=runtime::BnwSelectedLanguageInputOwner::capture_from_early_boot(actual,path);
        validate_source();impl_->selected_input=std::move(produced);
    }
    impl_->selected_input->require_current();validate_source();return impl_->selected_input;
}
const BnwNativeResourceBootSettingsFrame& BnwNativeResourceBootSettingsOwner::current() const {
    const auto input=impl_;const auto actual=input->retain_actual_facade();
    if(input->installation_source){
        BnwNativePackageDiscoveryOwner::with_installation_boot_parent_loan(*actual,input->packages,
            [&](const auto& packages){input->require_actual_facade(actual);input->validate_source();input->require_actual_facade(actual);
                need(&packages.installation_boot_source_table(actual.get())==input->table,"Installed Boot resource table epoch replaced");
                input->require_actual_facade(actual);});
        input->require_actual_facade(actual);return input->frame;
    }
    const auto packages=input->packages.lock();
    need(static_cast<bool>(packages), "Native boot settings resource owner expired");
    (void)packages->current(); // Real client callback may retire outside Boot.
    input->require_actual_facade(actual);input->validate_source();
    const auto* table=&packages->resources();input->require_actual_facade(actual);
    need(table==input->table,"Native boot settings resource epoch replaced");
    return input->frame;
}
void BnwNativeResourceBootSettingsOwner::require_same_resources(const BnwNativePackageDiscoveryOwner& packages) const {
    const auto input=impl_;const auto actual=input->retain_actual_facade();
    if(input->installation_source){
        const auto* supplied=&packages;
        BnwNativePackageDiscoveryOwner::with_installation_boot_parent_loan(*actual,input->packages,[&](const auto& parent){
            need(&parent==supplied,"Installed Boot supplied to foreign resource owner");
            input->require_actual_facade(actual);input->validate_source();input->require_actual_facade(actual);
            need(&parent.installation_boot_source_table(actual.get())==input->table,"Installed Boot supplied to foreign resource table");
            input->require_actual_facade(actual);});return;
    }
    (void)current();input->require_actual_facade(actual);
    need(input->packages.lock().get()==&packages,"Native boot settings supplied to foreign resource owner");
    const auto* table=&packages.resources();input->require_actual_facade(actual);
    need(table==input->table,"Native boot settings supplied to foreign resource table");
}
void BnwNativeResourceBootSettingsOwner::require_installation_parent_without_callbacks(
    const BnwNativePackageDiscoveryOwner& parent,const std::weak_ptr<const BnwNativePackageDiscoveryOwner>& factory,
    const assets::ResourceMountTable& table)const{
    const auto p=impl_;const auto actual=p->retain_actual_facade();
    need(actual.get()==this&&p->installation_source&&!p->packages.expired()&&
         !p->packages.owner_before(factory)&&!factory.owner_before(p->packages)&&p->table==&table,
         "Installed Boot foreign actual factory/table/source domain");
    const auto canonical=p->packages.lock();need(canonical&&canonical.get()==&parent,
         "Installed Boot actual Parent identity changed");
    //Both callback-free locks release before Parent/client/source callbacks.
}
void BnwNativeResourceBootSettingsOwner::require_installation_directory_without_callbacks(
    const BnwNativeUserDirectoryOwner& directory,const std::weak_ptr<const BnwNativePackageDiscoveryOwner>& factory,
    const assets::ResourceMountTable& table)const{
    const auto p=impl_;const auto actual=p->retain_actual_facade();
    need(actual.get()==this&&p->installation_source&&p->user_directory.get()==&directory&&
         !p->packages.expired()&&!p->packages.owner_before(factory)&&!factory.owner_before(p->packages)&&p->table==&table,
         "Installed Boot has another actual Directory/Parent/table source");
}
void BnwNativeResourceBootSettingsOwner::capture_native_dx9_adapter_source() const {
    const auto input=impl_;const auto actual=input->retain_actual_facade();
    (void)current();input->require_actual_facade(actual);
    if(input->installation_source){
        BnwNativePackageDiscoveryOwner::with_installation_boot_parent_loan(*actual,input->packages,[&](const auto& parent){
            need(!parent.current().low_resolution_source_branch_complete,"Native adapter source capture after texture publication");
            input->require_actual_facade(actual);input->capture_adapter();input->require_actual_facade(actual);});
        (void)current();input->require_actual_facade(actual);return;
    }
    const auto packages=input->packages.lock();need(bool(packages),"Native adapter Package retired");
    const bool published=packages->current().low_resolution_source_branch_complete;
    input->require_actual_facade(actual);
    need(!published,"Native adapter source capture after texture publication");
    input->capture_adapter();input->require_actual_facade(actual);
    (void)current();input->require_actual_facade(actual);
}
const BnwNativeResourceBootSettingsFrame& BnwNativeResourceBootSettingsOwner::common_operation_frame(const BnwNativePackageDiscoveryOwner& expected)const {
    need(impl_->packages.lock().get()==&expected,"Common settings has another actual Package");
    impl_->validate_source();
    need(impl_->packages.lock().get()==&expected,"Common settings Package retired during source check");
    return impl_->frame;
}
unsigned BnwNativeResourceBootSettingsOwner::normal_default_loose_mode(
    const BnwNativePackageDiscoveryOwner& expected)const {
    need(impl_->packages.lock().get()==&expected,"Default loose operand belongs to another Package");
    impl_->validate_source();
    //Read original retained bytes directly: frame normal_launch_source_closed
    //alone cannot authorize this native operand. Full command absence implies
    //absence in the actual54CAE0 argument tail, retaining the strict old scope.
    need(std::strstr(impl_->command.c_str(),"-LeaderViewer")==nullptr,
         "Default loose positive Viewer mode6 requires actual Viewer startup operand owner");
    need(impl_->packages.lock().get()==&expected,"Default loose operand Package retired during source check");
    return (impl_->normal_default_loose_debug_byte!=0?6u:1u);
}
void BnwNativeResourceBootSettingsOwner::validate_source() const { impl_->validate_source(); }
}

namespace civ5::ui {
std::shared_ptr<const runtime::BnwSelectedUserOptionsOwner> BnwNativeResourceBootSettingsOwner::selected_user_options()const{
 const auto actual=impl_->self.lock();need(actual&&actual.get()==this&&actual.use_count()>1,"Actual UserSettings Boot facade retired");
 validate_source();
 if(!impl_->selected_user_options){
  need(!impl_->user_options_attempted,"Actual UserSettings closed factory already attempted/uncertain");
  impl_->user_options_attempted=true;
  const auto produced=runtime::BnwSelectedUserOptionsOwner::capture_from_boot(actual);
  validate_source();produced->require_same_boot(*actual);impl_->selected_user_options=produced;
 }
 impl_->selected_user_options->require_same_boot(*actual);validate_source();return impl_->selected_user_options;
}
}

namespace civ5::ui {
std::shared_ptr<const BnwNativeResourceBootSettingsOwner>
BnwNativeResourceBootSettingsOwner::retain_factory_owner_without_callbacks(
    const BnwNativePackageDiscoveryOwner& packages,
    const std::weak_ptr<const BnwNativePackageDiscoveryOwner>& expected,
    const assets::ResourceMountTable& table)const {
    const auto p=impl_;
    need(p&&p->process==GetCurrentProcessId()&&p->thread==std::this_thread::get_id(),
         "Completed options Boot foreign process/creator before factory borrow");
    const auto actual=p->self.lock();
    need(actual&&actual.get()==this&&!actual.owner_before(p->self)&&!p->self.owner_before(actual)&&actual.use_count()>1,
         "Completed options Boot actual canonical facade retired");
    // The transient package pin ends in this callback-free function. It never
    // masks external Parent authority across a provider or a source check.
    const auto parent=p->packages.lock();
    need(parent&&parent.get()==&packages&&!p->packages.owner_before(expected)&&!expected.owner_before(p->packages)&&
         p->table==&table&&p->user_options_attempted&&p->selected_user_options,
         "Completed options Boot belongs to another Package/table or lacks original capture");
    return actual;
}
std::shared_ptr<const runtime::BnwSelectedUserOptionsOwner>
BnwNativeResourceBootSettingsOwner::borrow_retained_user_options_for_completed_source()const {
    const auto p=impl_;
    need(p&&p->process==GetCurrentProcessId()&&p->thread==std::this_thread::get_id(),
         "Completed retained options foreign Boot creator");
    const auto actual=p->self.lock();
    need(actual&&actual.get()==this&&actual.use_count()>1&&!actual.owner_before(p->self)&&!p->self.owner_before(actual),
         "Completed retained options actual Boot facade retired");
    const auto result=p->selected_user_options;
    need(p->user_options_attempted&&result,"Completed retained options were not captured by original Boot factory");
    const auto pure=[&] {
        p->require_actual_facade(actual);
        need(p->user_options_attempted&&p->selected_user_options.get()==result.get()&&
             !p->selected_user_options.owner_before(result)&&!result.owner_before(p->selected_user_options)&&
             !p->packages.expired(),"Completed retained options Boot/store/Parent changed");
    };
    pure();p->validate_source();pure();
    result->require_same_boot(*actual);pure();p->validate_source();pure();
    return result; // No lazy factory, default insertion, INI initialization or Parent callback.
}
}

namespace civ5::ui {
std::shared_ptr<const runtime::BnwNetMessageDebugConfigOwner>
BnwNativeResourceBootSettingsOwner::selected_net_debug_config()const {
    // Creator/process gate is inside retain_actual_facade, before mutable anchor reads.
    const auto actual=impl_->retain_actual_facade();
    need(actual.get()==this,"Net config requires SAME canonical Boot facade");
    const auto current=[&]{impl_->require_actual_facade(actual);validate_source();impl_->require_actual_facade(actual);};
    current();
    if(!impl_->selected_net_config){
        need(!impl_->net_config_attempted,"Net config first issue already attempted or failed");
        impl_->net_config_attempted=true;
        auto produced=runtime::BnwNetMessageDebugConfigOwner::capture_from_boot(actual);
        current();produced->require_same_boot(*actual);current();
        impl_->selected_net_config=std::move(produced);
    }
    const auto result=impl_->selected_net_config;
    result->require_same_boot(*actual);current();
    need(result.get()==impl_->selected_net_config.get()&&
        !result.owner_before(impl_->selected_net_config)&&!impl_->selected_net_config.owner_before(result),
        "Net config canonical first-issue owner changed");
    return result;
}
}

namespace civ5::ui {
std::shared_ptr<const runtime::BnwNetMessageDebugManagerOwner>
BnwNativeResourceBootSettingsOwner::selected_net_debug_manager()const {
    // Actual immutable process/creator gate precedes every mutable factory read.
    const auto actual=impl_->retain_actual_facade();
    need(actual.get()==this,"Net manager requires SAME canonical Boot facade");
    const auto current=[&]{impl_->require_actual_facade(actual);validate_source();impl_->require_actual_facade(actual);};
    current();
    if(!impl_->selected_net_manager){
        need(!impl_->net_manager_attempted,"Net manager first issue already attempted or failed");
        impl_->net_manager_attempted=true;
        auto produced=runtime::BnwNetMessageDebugManagerOwner::capture_from_boot(actual);
        current();produced->require_same_boot(*actual);current();
        impl_->selected_net_manager=std::move(produced);
    }
    const auto result=impl_->selected_net_manager;
    result->require_same_boot(*actual);current();
    need(result.get()==impl_->selected_net_manager.get()&&
        !result.owner_before(impl_->selected_net_manager)&&!impl_->selected_net_manager.owner_before(result),
        "Net manager canonical first-issue owner changed");
    return result;
}
}
