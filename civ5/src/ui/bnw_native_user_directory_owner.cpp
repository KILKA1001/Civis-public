#include "civ5/ui/bnw_native_user_directory_owner.hpp"
#include "bnw_native_installed_boot_construction_origin.hpp"
#include "bnw_native_filesystem_mapping_owner.hpp"
#include <cstring>
#include <atomic>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>

namespace civ5::ui {
namespace {
void need(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
std::filesystem::path native_directory() {
    wchar_t personal[MAX_PATH]{};
    // Original823EC9 has fCreate TRUE. This read-only binding certifies the
    // existing path only, using FALSE so absence never creates directories.
    need(SHGetSpecialFolderPathW(nullptr, personal, CSIDL_PERSONAL, FALSE) != FALSE,
         "Native existing personal-folder source unavailable");
    need(std::filesystem::is_directory(personal), "Native personal directory missing");
    // Constructor6BDE0B seeds this namespace, not caller path/executable stem.
    return std::filesystem::weakly_canonical(std::filesystem::path(personal) /
               L"My Games" / L"Sid Meier's Civilization 5");
}
std::string command_line() {
    const auto* command = GetCommandLineA();
    need(command != nullptr, "Actual process launch source unavailable");
    return command;
}
}
struct BnwNativeUserDirectoryOwner::Impl {
    // Parent strongly owns Boot, and Boot strongly owns this Directory. The
    // back-edge is weak; a strong Parent here would create a permanent cycle.
    std::weak_ptr<const BnwNativePackageDiscoveryOwner> packages;
    const BnwNativePackageDiscoveryOwner* parent_identity = nullptr;
    const assets::ResourceMountTable* table = nullptr;
    bool installation_source=false;
    std::shared_ptr<BnwNativePackageDiscoveryOwner::InstalledFilesystemDocumentsLoan> filesystem_documents;
    std::weak_ptr<const BnwNativeUserDirectoryOwner> self;
    const BnwNativeUserDirectoryOwner* facade_identity = nullptr;
    std::atomic<bool> retired{false};
    std::size_t self_pins = 0;
    bool busy = false;
    const SelfPin* active_operation_pin = nullptr;
    DWORD process = GetCurrentProcessId();
    std::thread::id thread = std::this_thread::get_id();
    std::string command;
    std::filesystem::path directory;
    void require_creator() const {
        // Reject a foreign first entry before any mutable pin/busy fields.
        need(process == GetCurrentProcessId() && thread == std::this_thread::get_id(),
             "Native user directory foreign process/thread");
    }
    void require_alive() const {
        need(!retired.load(std::memory_order_acquire), "Native user directory owner retired");
    }
    void require_outside(std::size_t temporary_pins = 0) {
        require_creator(); require_alive();
        if (self.use_count() <= static_cast<long>(self_pins + temporary_pins)) {
            retired.store(true, std::memory_order_release);
            throw std::runtime_error("Native user directory genuine facade owner expired");
        }
    }
    void validate_source() const {
        require_creator(); require_alive();
        need(command_line()==command,"Native user directory launch source epoch changed");
        if(installation_source){
            need(bool(filesystem_documents),"Installed user directory lacks SAME FS Documents adoption");
            filesystem_documents->require_actual_directory_current(directory);
        }else need(native_directory()==directory,"Native user directory source epoch changed");
    }
};
struct BnwNativeUserDirectoryOwner::SelfPin {
    std::shared_ptr<Impl> state;
    std::shared_ptr<const BnwNativeUserDirectoryOwner> facade;
    explicit SelfPin(const BnwNativeUserDirectoryOwner& owner) : state(owner.impl_) {
        state->require_creator(); state->require_alive();
        facade = state->self.lock();
        need(facade && facade.get() == &owner && state->facade_identity == &owner &&
             facade->impl_.get() == state.get(), "Native user directory factory storage mismatch");
        state->require_outside(1); // This canonical weak lock is internal.
        ++state->self_pins;
    }
    ~SelfPin() { --state->self_pins; }
    SelfPin(const SelfPin&) = delete;
    SelfPin& operator=(const SelfPin&) = delete;
    void check() const { state->require_outside(); }
};
struct BnwNativeUserDirectoryOwner::Operation {
    SelfPin pin;
    explicit Operation(const BnwNativeUserDirectoryOwner& owner) : pin(owner) {
        if (pin.state->busy) {
            pin.state->retired.store(true, std::memory_order_release);
            throw std::runtime_error("Native user directory operation reentered");
        }
        pin.state->busy = true;
        pin.state->active_operation_pin = &pin;
    }
    ~Operation() {
        if (pin.state->active_operation_pin == &pin) pin.state->active_operation_pin = nullptr;
        pin.state->busy = false;
    }
    Operation(const Operation&) = delete;
    Operation& operator=(const Operation&) = delete;
    void check() const { pin.check(); }
    void fail() const { pin.state->retired.store(true, std::memory_order_release); }
};
BnwNativeUserDirectoryOwner::BnwNativeUserDirectoryOwner(std::shared_ptr<Impl> impl)
    : impl_(std::move(impl)) {}
BnwNativeUserDirectoryOwner::~BnwNativeUserDirectoryOwner() {
    impl_->retired.store(true, std::memory_order_release);
}
std::shared_ptr<const BnwNativeUserDirectoryOwner> BnwNativeUserDirectoryOwner::create(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> packages) {
    need(static_cast<bool>(packages), "Native user directory requires SAME early source/client");
    // Actual73 getter is callback-free. Authenticate the incoming control block
    // BEFORE invoking Parent/currentness, and keep only the genuine owner.
    auto canonical = packages->retain_factory_owner();
    need(canonical.get() == packages.get() && !canonical.owner_before(packages) &&
         !packages.owner_before(canonical), "Native user directory forged Parent control block");
    packages = std::move(canonical);
    (void)packages->current();
    auto i = std::make_shared<Impl>(); i->packages = packages;
    i->parent_identity = packages.get();
    i->table = &packages->resources(); i->command = command_line();
    // Native54CAE0 uses strstr. Negative on whole process command line also
    // implies negative on an upstream argument tail; matched Viewer is refused.
    need(std::strstr(i->command.c_str(), "-LeaderViewer") == nullptr,
         "Native LeaderViewer directory requires its own original source owner");
    i->directory = native_directory(); i->validate_source();
    (void)packages->current();
    auto owner = std::shared_ptr<const BnwNativeUserDirectoryOwner>(
        new BnwNativeUserDirectoryOwner(i));
    i->facade_identity = owner.get(); i->self = owner;
    // The real Directory is issued before its first loan; publication happens
    // only after the genuine Parent77 loan/currentness check completes.
    (void)owner->current();
    return owner;
}
std::shared_ptr<const BnwNativeUserDirectoryOwner> BnwNativeUserDirectoryOwner::create_from_installation_boot_source(
    const BnwNativePackageDiscoveryOwner& packages){
    (void)packages.installation_boot_source_table();
    auto i = std::make_shared<Impl>(); i->packages = packages.installation_boot_source_factory();i->installation_source=true;
    i->parent_identity = &packages;
    i->table = &packages.installation_boot_source_table(); i->command = command_line();
    // Native54CAE0 uses strstr. Negative on whole process command line also
    // implies negative on an upstream argument tail; matched Viewer is refused.
    need(std::strstr(i->command.c_str(), "-LeaderViewer") == nullptr,
         "Native LeaderViewer directory requires its own original source owner");
    i->filesystem_documents=packages.borrow_installed_filesystem_documents();
    i->directory=i->filesystem_documents->copy_actual_directory();i->validate_source();
    (void)packages.current();
    auto owner = std::shared_ptr<const BnwNativeUserDirectoryOwner>(
        new BnwNativeUserDirectoryOwner(i));
    i->facade_identity = owner.get(); i->self = owner;
    packages.register_installation_boot_directory(owner);
    // The real Directory is issued before its first loan; publication happens
    // only after the genuine Parent77 loan/currentness check completes.
    (void)owner->current();
    return owner;
}
std::shared_ptr<const BnwNativeUserDirectoryOwner> BnwNativeUserDirectoryOwner::create_from_installation_boot_source(
    const BnwNativePackageDiscoveryOwner& packages,const BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin& origin){
    (void)packages.installation_boot_source_table(origin);
    auto i = std::make_shared<Impl>();origin.require_identity(packages); i->packages = packages.installation_boot_source_factory(origin);i->installation_source=true;
    i->parent_identity = &packages;
    i->table = &packages.installation_boot_source_table(origin); i->command = command_line();
    // Native54CAE0 uses strstr. Negative on whole process command line also
    // implies negative on an upstream argument tail; matched Viewer is refused.
    need(std::strstr(i->command.c_str(), "-LeaderViewer") == nullptr,
         "Native LeaderViewer directory requires its own original source owner");
    i->filesystem_documents=packages.borrow_installed_filesystem_documents(origin);origin.require_identity(packages);
    i->directory=i->filesystem_documents->copy_actual_directory();origin.require_identity(packages);i->validate_source();origin.require_identity(packages);
    origin.require_current(packages);
    auto owner = std::shared_ptr<const BnwNativeUserDirectoryOwner>(
        new BnwNativeUserDirectoryOwner(i));
    i->facade_identity = owner.get(); i->self = owner;origin.require_identity(packages);
    packages.register_installation_boot_directory(owner,origin);
    // The real Directory is issued before its first loan; publication happens
    // only after the genuine Parent77 loan/currentness check completes.
    (void)owner->current_for_installation_boot(origin);origin.require_identity(packages);
    return owner;
}
std::shared_ptr<const BnwNativeUserDirectoryOwner> BnwNativeUserDirectoryOwner::retain_factory_owner(
    const std::shared_ptr<const BnwNativeUserDirectoryOwner>& supplied) const {
    const auto state = impl_;
    state->require_creator(); state->require_alive();
    auto canonical = state->self.lock();
    need(canonical && canonical.get() == this && state->facade_identity == this &&
         canonical->impl_.get() == state.get(), "Native user directory factory storage mismatch");
    need(supplied.get() == canonical.get() && !supplied.owner_before(canonical) &&
         !canonical.owner_before(supplied), "Native user directory forged facade control block");
    state->require_outside(1);
    return canonical;
}
void BnwNativeUserDirectoryOwner::require_parent_loan_creator_thread() const {
    const auto state = impl_;
    state->require_creator(); state->require_alive();
    // The caller's noncopyable Operation already holds the real canonical
    // facade and SAME Impl. These pure lender hooks add no second weak lock,
    // including no extra Directory lock that could obscure outside counts.
    const auto* issued = state->active_operation_pin;
    need(state->facade_identity == this && state->busy && state->self_pins != 0 &&
         issued && issued->state.get() == state.get() && issued->facade.get() == this &&
         !issued->facade.owner_before(state->self) && !state->self.owner_before(issued->facade),
         "Native user directory Parent loan requires actual canonical operation");
    state->require_outside();
}
void BnwNativeUserDirectoryOwner::require_parent_loan_identity_without_callbacks(
    const BnwNativePackageDiscoveryOwner& actual,
    const std::weak_ptr<const BnwNativePackageDiscoveryOwner>& actual_factory,
    const assets::ResourceMountTable& actual_table) const {
    require_parent_loan_creator_thread(); const auto state = impl_;
    // No Dir/Parent lock or Parent method: the lender supplies one genuine live
    // Parent under its registered StackLoan. The raw identity is compared only.
    need(!state->packages.expired(), "Native user directory source/client owner expired");
    need(&actual == state->parent_identity &&
         !state->packages.owner_before(actual_factory) &&
         !actual_factory.owner_before(state->packages) &&
         &actual_table == state->table,
         "Native user directory supplied to foreign early source/client");
}
void BnwNativeUserDirectoryOwner::require_parent_loan_source_without_callbacks(
    const BnwNativePackageDiscoveryOwner& actual,
    const std::weak_ptr<const BnwNativePackageDiscoveryOwner>& actual_factory,
    const assets::ResourceMountTable& actual_table) const {
    require_parent_loan_identity_without_callbacks(actual,actual_factory,actual_table);
    const auto state = impl_;
    state->validate_source(); state->require_outside();
}
const std::filesystem::path& BnwNativeUserDirectoryOwner::current() const {
    Operation operation(*this); const auto state = operation.pin.state;
    try {
        BnwNativePackageDiscoveryOwner::with_native_user_directory_parent_loan(
            *operation.pin.facade, state->packages,
            [&operation, state](const BnwNativePackageDiscoveryOwner& actual) {
                operation.check(); (void)actual.current(); operation.check();
                state->validate_source(); operation.check();
                need(&(state->installation_source?actual.installation_directory_source_table(*operation.pin.facade):actual.resources()) == state->table,
                     "Native user directory resource epoch replaced");
                operation.check();
            });
        operation.check();
        return state->directory;
    } catch (...) { operation.fail(); throw; }
}
const std::filesystem::path& BnwNativeUserDirectoryOwner::current_for_installation_boot(const BnwNativePackageDiscoveryOwner::InstalledBootConstructionOrigin& origin) const {
    Operation operation(*this); const auto state = operation.pin.state;
    try {
        BnwNativePackageDiscoveryOwner::with_native_user_directory_parent_loan(
            *operation.pin.facade, state->packages,
            [&operation, state,&origin](const BnwNativePackageDiscoveryOwner& actual) {
                operation.check(); origin.require_current(actual); operation.check();
                state->validate_source(); operation.check();
                need(&(state->installation_source?actual.installation_directory_source_table(*operation.pin.facade,origin):actual.resources()) == state->table,
                     "Native user directory resource epoch replaced");
                operation.check();
            });
        operation.check();
        origin.require_identity(*origin.parent_);return state->directory;
    } catch (...) { operation.fail(); throw; }
}
void BnwNativeUserDirectoryOwner::require_same_resources(const BnwNativePackageDiscoveryOwner& packages) const {
    Operation operation(*this); const auto state = operation.pin.state;
    // Compare the supplied identity before any callback; never borrow that
    // supplied reference after a callback may have released its real owner.
    need(&packages == state->parent_identity,
         "Native user directory supplied to foreign early source/client");
    try {
        BnwNativePackageDiscoveryOwner::with_native_user_directory_parent_loan(
            *operation.pin.facade, state->packages,
            [&operation, state](const BnwNativePackageDiscoveryOwner& actual) {
                operation.check(); (void)actual.current(); operation.check();
                state->validate_source(); operation.check();
                need(&(state->installation_source?actual.installation_directory_source_table(*operation.pin.facade):actual.resources()) == state->table,
                     "Native user directory supplied to foreign early source/client");
                operation.check();
            });
        operation.check();
    } catch (...) { operation.fail(); throw; }
}
void BnwNativeUserDirectoryOwner::validate_source() const {
    // Boot's private metadata path is allowed inside the owning operation.
    // It does not enter Parent, obtain a Parent strong lock, or dispatch a loan.
    SelfPin pin(*this); pin.state->validate_source(); pin.check();
}
const std::filesystem::path& BnwNativeUserDirectoryOwner::source_directory_without_parent_callbacks()const {
    SelfPin pin(*this); pin.state->validate_source();
    need(!pin.state->packages.expired(), "Actual selected profile Parent retired");
    pin.check(); return pin.state->directory;
}
}
