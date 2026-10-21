#include "bnw_native_database_validation_settings.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include <fstream>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::ui {
namespace {
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
struct Input {bool present=false;runtime::FileIdentity file;std::vector<std::uint8_t> bytes;};
Input read(const std::filesystem::path& path) {
    Input out;auto attr=GetFileAttributesW(path.c_str());
    if(attr==INVALID_FILE_ATTRIBUTES){auto error=GetLastError();need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,"Actual Config INI probe failed");return out;}
    need((attr&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))==0,"Actual Config backing unsupported");out.present=true;auto before=runtime::identify_file(path);need(before.bytes<=1024*1024,"Actual Config read budget");std::ifstream stream(path,std::ios::binary);need(bool(stream),"Actual Config read failed");out.bytes.assign(std::istreambuf_iterator<char>(stream),{});need(!stream.bad()&&out.bytes.size()==before.bytes,"Actual Config partial read");out.file=runtime::identify_bytes(out.bytes);auto after=runtime::identify_file(path);need(out.file.sha256==before.sha256&&after.sha256==before.sha256&&after.bytes==before.bytes,"Config changed during initial source Load");return out;
}
}
struct BnwNativeDatabaseValidationSettings::Impl {
    detail::BnwNativeCallbackOwnerState callback_state;
    std::weak_ptr<const BnwNativeDatabaseValidationSettings> self;
    using Guard = detail::BnwNativeCallbackOwnerGuard<BnwNativeDatabaseValidationSettings>;
    Guard guard()const noexcept { return Guard(callback_state,self); }
    std::weak_ptr<const BnwNativePackageDiscoveryOwner> package;
    std::shared_ptr<const BnwNativeUserDirectoryOwner> directory;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    const assets::ResourceMountTable* resources=nullptr;
    BnwNativeDatabaseValidationSettingsFrame frame;
    std::vector<std::uint8_t> loaded_snapshot;
    template<class Fence> void validate(const Fence& fence)const {
        const auto check=[&]{need(fence.valid(),"Config callback owner lost/reentered/foreign thread");};
        check();auto p=package.lock();need(bool(p),"Config source Package expired");
        // Directory and Config retain Package weakly. Local p alone cannot
        // authorize returning a borrowed frame after the parent facade retires.
        // Nested directory/Package validation pins are gone at every check.
        const auto parent_check=[&]{need(fence.valid_parent(p),"Config source Package external owner retired");};
        parent_check();lease->require_current();parent_check();
        directory->require_same_resources(*p);parent_check();
        const auto input=p->early_input_lease();parent_check();
        const auto* table=&p->resources();parent_check();
        need(input.get()==lease.get()&&table==resources,"Config SAME early source replaced");
        const auto path=directory->current()/L"config.ini";parent_check();
        need(path==frame.source_path,"Config source directory changed");
        // CB8CFB stays cached; source Load routing remains unavailable.
    }
};
BnwNativeDatabaseValidationSettings::BnwNativeDatabaseValidationSettings(std::unique_ptr<Impl> i):impl_(std::move(i)){}
BnwNativeDatabaseValidationSettings::~BnwNativeDatabaseValidationSettings()=default;
std::shared_ptr<const BnwNativeDatabaseValidationSettings> BnwNativeDatabaseValidationSettings::acquire(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> p,
    std::shared_ptr<const BnwNativeUserDirectoryOwner> d) {
    need(p&&d,"Config genuine source owners absent");
    auto i=std::make_unique<Impl>();
    {
        detail::BnwNativeCallbackConstructionGuard fence(i->callback_state);
        const auto check=[&]{need(fence.valid(),"Config construction reentered/foreign thread");};
        check();d->require_same_resources(*p);check();
        auto lease=p->early_input_lease();check();
        need(bool(lease),"Legacy diagnostic Package cannot own normal Config");
        lease->require_current();check();
        const auto* table=&p->resources();check();
        const auto path=d->current()/L"config.ini";check();
        i->package=p;i->directory=std::move(d);i->lease=std::move(lease);
        i->resources=table;i->frame.source_path=path;
        auto input=read(i->frame.source_path);check();
        auto fields=database_validation_detail::source_fields(std::string_view(
            reinterpret_cast<const char*>(input.bytes.data()),input.bytes.size()));check();
        i->frame.source_file_present=input.present;i->frame.loaded_file=input.file;
        i->frame.os_candidate_enable_tuner=fields.enable_tuner;
        i->frame.os_candidate_validation=fields.validate_database;
        i->frame.enable_tuner_defaulted=fields.tuner_defaulted;
        i->frame.validation_defaulted=fields.validation_defaulted;
        i->loaded_snapshot=std::move(input.bytes);
        i->validate(fence);check();auto after=read(i->frame.source_path);check();
        need(after.present==input.present&&after.file.sha256==input.file.sha256&&
             after.file.bytes==input.file.bytes,"Config changed across normal first-two getters");
    }
    auto owner=std::shared_ptr<const BnwNativeDatabaseValidationSettings>(
        new BnwNativeDatabaseValidationSettings(std::move(i)));
    owner->impl_->self=owner;return owner;
}
const BnwNativeDatabaseValidationSettingsFrame& BnwNativeDatabaseValidationSettings::current()const {
    auto fence=impl_->guard();impl_->validate(fence);
    need(impl_->frame.native_load_routing_owned,"Normal Config SAME native prefix/CRC Load routing pending; cached predicate unavailable");
    need(fence.valid(),"Config callback owner lost before borrowed frame");return impl_->frame;
}
const BnwNativeDatabaseValidationSettingsFrame& BnwNativeDatabaseValidationSettings::input_snapshot()const {
    auto fence=impl_->guard();impl_->validate(fence);
    need(fence.valid(),"Config callback owner lost before borrowed input");return impl_->frame;
}
void BnwNativeDatabaseValidationSettings::require_same_resources(const BnwNativePackageDiscoveryOwner& p)const {
    auto fence=impl_->guard();impl_->validate(fence);
    need(impl_->package.lock().get()==&p,"Config different Package address");
    impl_->validate(fence);
}
}
