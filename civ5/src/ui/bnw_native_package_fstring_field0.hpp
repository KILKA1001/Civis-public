#pragma once
#include "bnw_native_completed_cold_source_loan.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <cstring>
#include <thread>
#include <utility>

namespace civ5::ui::detail {
class BnwNativeResourceDiscoveryActor; // Genuine821760/821BE0 actor NOT implemented here.
class BnwNativePackageLoadActor;       // Private pre-load receiver/copy actor; full lookup/read remains open.
class BnwNativePackageFStringField0;
class BnwNativeGlobalResourceWarmOwner;
class BnwNativeInstalledDescriptorEnumeration;
class BnwNativePreDx9DiscoveryOwner;

// No public constructor/from(string). The future reviewed discovery actor must
// own Source and issue this quote from its actual registered enumeration result.
// A physical manifest path/FileIdentity cannot construct this capability.
class BnwNativeResourceDiscoveryFilename final {
    friend class BnwNativePreDx9DiscoveryOwner;
    friend class BnwNativeResourceDiscoveryActor;
    friend class BnwNativePackageLoadActor;
    friend class BnwNativePackageFStringField0;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeInstalledDescriptorEnumeration;
    struct Source {
        friend class BnwNativePreDx9DiscoveryOwner;
        friend class BnwNativeResourceDiscoveryActor;
        friend class BnwNativePackageLoadActor;
        friend class BnwNativePackageFStringField0;
        friend class BnwNativeGlobalResourceWarmOwner;
        friend class BnwNativeInstalledDescriptorEnumeration;
        const std::thread::id thread=std::this_thread::get_id();
        const std::weak_ptr<const BnwNativeResourceDiscoveryActor> actor;
        const std::string native_acp_filename;
        const std::function<void()> current;
        const std::function<void(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)> completed_current;
        const std::function<void(const Source&)> constructor_identity;
        const std::function<void()> constructor_failure;
      private:
        class ConstructionOperation;
        mutable bool constructor_attempted=false;
        mutable const ConstructionOperation* constructor_operation=nullptr;
      private:
        Source(std::weak_ptr<const BnwNativeResourceDiscoveryActor> owner,
               std::string actual_registered_filename,std::function<void()> genuine_epoch_check)
            :actor(std::move(owner)),native_acp_filename(std::move(actual_registered_filename)),
             current(std::move(genuine_epoch_check)){}
        Source(std::weak_ptr<const BnwNativeResourceDiscoveryActor> owner,std::string actual_registered_filename,
            std::function<void()> genuine_epoch_check,
            std::function<void(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)> genuine_completed_check)
            :actor(std::move(owner)),native_acp_filename(std::move(actual_registered_filename)),
             current(std::move(genuine_epoch_check)),completed_current(std::move(genuine_completed_check)){}
        Source(std::weak_ptr<const BnwNativeResourceDiscoveryActor> owner,std::string name,
            std::function<void()> current_check,
            std::function<void(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)> completed_check,
            std::function<void(const Source&)> identity_check,std::function<void()> failure)
            :actor(std::move(owner)),native_acp_filename(std::move(name)),current(std::move(current_check)),
             completed_current(std::move(completed_check)),constructor_identity(std::move(identity_check)),
             constructor_failure(std::move(failure)){}
        // Genuine installed Source only; one initial empty/name-copy operation.
        // No operation survives into canonical lookup, read, parser or metadata.
        class ConstructionOperation final {
            friend class BnwNativePreDx9DiscoveryOwner;
            friend class BnwNativePackageLoadActor;
            friend class BnwNativePackageFStringField0;
            const std::shared_ptr<const Source> source_;
            bool complete_=false;
            explicit ConstructionOperation(std::shared_ptr<const Source> source):source_(std::move(source)) {
                need(source_&&source_->thread==std::this_thread::get_id(),"Installed construction Source foreign creator");
                need(!source_->constructor_attempted&&!source_->constructor_operation&&source_->constructor_identity&&
                     source_->constructor_failure,"Installed construction Source unissued/replayed");
                source_->check(); // Real row/stat/full Common PRE before grant.
                source_->constructor_attempted=true;source_->constructor_operation=this;
                try {require_same(source_);}catch(...){source_->constructor_operation=nullptr;source_->constructor_failure();throw;}
            }
            void require_same(const std::shared_ptr<const Source>& actual)const {
                need(source_->thread==std::this_thread::get_id(),"Installed construction foreign creator");
                need(actual.get()==source_.get()&&!actual.owner_before(source_)&&!source_.owner_before(actual)&&
                     source_->constructor_operation==this&&!source_->actor.expired(),
                     "Installed construction changed actual Source/active occurrence");
                source_->constructor_identity(*source_);
            }
            void finish() {
                require_same(source_);source_->check(); // Genuine row/stat/full Common POST.
                require_same(source_);complete_=true;
            }
          public:
            ConstructionOperation(const ConstructionOperation&)=delete;
            ConstructionOperation& operator=(const ConstructionOperation&)=delete;
            ~ConstructionOperation()noexcept {
                source_->constructor_operation=nullptr;
                if(!complete_)source_->constructor_failure();
            }
        };
      public:
        void check_completed(const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)const {
            need(thread==std::this_thread::get_id(),"Completed discovery filename foreign creator thread");
            loan.require_same_cold(cold);
            need(!actor.expired()&&bool(completed_current),"Completed discovery source checker absent/actual actor expired");
            // ONLY genuine Discovery can install this checker. Its Parent
            // producer must authenticate this actual occurrence/vector/control.
            completed_current(loan,cold);loan.require_same_cold(cold);
            need(!actor.expired(),"Completed discovery actual actor retired during check");
        }
        void check()const {
            need(thread==std::this_thread::get_id(),"Package discovery filename foreign thread");
            need(!actor.expired()&&bool(current),"Package discovery actual actor expired/unissued");
            current();
            need(!actor.expired(),"Package discovery last actor released in callback");
        }
    };
    std::weak_ptr<const Source> source_;
    explicit BnwNativeResourceDiscoveryFilename(std::weak_ptr<const Source> source):source_(std::move(source)){}
    static void need(bool value,const char* error){if(!value)throw std::runtime_error(error);}
};

// Pre-load receiver+0 ONLY. It is not grafted into an already-loaded
// RetainedDescriptor: actual6F7D30 initialization precedes498940 and XML read.
// Native FString allocation/layout/refcount parity remains unimplemented.
class BnwNativePackageFStringField0 final {
    friend class BnwNativePreDx9DiscoveryOwner;
    friend class BnwNativePackageLoadActor;
    friend class BnwNativeGlobalResourceWarmOwner;
    using Filename=BnwNativeResourceDiscoveryFilename;
    struct State {
        const std::thread::id thread=std::this_thread::get_id();
        const std::weak_ptr<const Filename::Source> filename;
        std::atomic<bool> closed{false},failed{false},busy{false};
        std::string field0; //6F7D3B <-81D060()+0C (05CF2C08): initial empty VALUE.
        bool copy_attempted=false; // Port occurrence fence, not a native field.
        const void* active_canonical_input=nullptr; // Negative physical membership only; no query permission.
        std::uint64_t active_canonical_input_generation=0;
        explicit State(const Filename& source):filename(source.source_){}
        std::shared_ptr<const Filename::Source> check() {
            Filename::need(thread==std::this_thread::get_id(),"Package field0 foreign thread");
            Filename::need(!closed.load()&&!failed.load(),"Package field0 retired");
            auto source=filename.lock();Filename::need(bool(source),"Package discovery filename expired");
            source->check();
            Filename::need(!closed.load()&&!failed.load(),"Package field0 retired in source callback");
            return source;
        }
        std::shared_ptr<const Filename::Source> check_completed(const BnwNativeCompletedColdSourceLoan& loan,
            const BnwNativeColdFilesystemOwner& cold) {
            Filename::need(thread==std::this_thread::get_id(),"Completed field0 foreign creator thread");
            Filename::need(!closed.load()&&!failed.load(),"Completed field0 retired");
            loan.require_same_cold(cold);
            const auto source=filename.lock();Filename::need(bool(source),"Completed actual filename expired");
            source->check_completed(loan,cold);
            Filename::need(!closed.load()&&!failed.load(),"Completed field0 retired during source check");return source;
        }
        std::shared_ptr<const Filename::Source> check(const Filename::Source::ConstructionOperation& operation) {
            Filename::need(thread==std::this_thread::get_id(),"Installed field0 foreign creator");
            Filename::need(!closed.load()&&!failed.load(),"Installed field0 retired");
            const auto source=filename.lock();Filename::need(bool(source),"Installed field0 Source expired");
            operation.require_same(source);
            Filename::need(!closed.load()&&!failed.load(),"Installed field0 changed in constructor");return source;
        }
        struct Scope {
            std::shared_ptr<State> owner;
            explicit Scope(std::shared_ptr<State> value):owner(std::move(value)) {
                Filename::need(owner->thread==std::this_thread::get_id(),"Package field0 scope foreign thread");
                if(owner->busy.exchange(true)){
                    owner->failed.store(true);throw std::runtime_error("Package field0 reentry");
                }
            }
            ~Scope(){owner->busy.store(false);}
        };
    };
    std::shared_ptr<State> state_;
    explicit BnwNativePackageFStringField0(const Filename& actual_discovery_filename)
        :state_(std::make_shared<State>(actual_discovery_filename)) {
        auto owned=state_;
        try {State::Scope scope(owned);owned->check();owned->check();}
        catch(...){owned->failed.store(true);throw;}
    }
    explicit BnwNativePackageFStringField0(const Filename& actual_discovery_filename,const Filename::Source::ConstructionOperation& operation)
        :state_(std::make_shared<State>(actual_discovery_filename)) {
        auto owned=state_;
        try {State::Scope scope(owned);owned->check(operation);owned->check(operation);}
        catch(...){owned->failed.store(true);throw;}
    }
    void copy_discovered_name_for_498940()const {
        const auto owned=state_;
        try {
            State::Scope scope(owned);const auto source=owned->check();
            Filename::need(!owned->copy_attempted,"Package field0 pre-load occurrence already consumed");
            owned->copy_attempted=true;
            //498940 _stricmp mismatch invokes6EDE20 strlen/copy. Native
            //filenames are ACP bytes, not assets::path_utf8(manifest.path).
            const auto length=std::strlen(source->native_acp_filename.c_str());
            std::string replacement(source->native_acp_filename.data(),length);
            const bool differs=_stricmp(owned->field0.c_str(),replacement.c_str())!=0;
            owned->check();
            if(differs)owned->field0.swap(replacement);
            owned->check();
        } catch(...){owned->failed.store(true);throw;}
    }
    void copy_discovered_name_for_498940(const Filename::Source::ConstructionOperation& operation)const {
        const auto owned=state_;
        try {
            State::Scope scope(owned);const auto source=owned->check(operation);
            Filename::need(!owned->copy_attempted,"Package field0 pre-load occurrence already consumed");
            owned->copy_attempted=true;
            //498940 _stricmp mismatch invokes6EDE20 strlen/copy. Native
            //filenames are ACP bytes, not assets::path_utf8(manifest.path).
            const auto length=std::strlen(source->native_acp_filename.c_str());
            std::string replacement(source->native_acp_filename.data(),length);
            const bool differs=_stricmp(owned->field0.c_str(),replacement.c_str())!=0;
            owned->check(operation);
            if(differs)owned->field0.swap(replacement);
            owned->check(operation);
        } catch(...){owned->failed.store(true);throw;}
    }
    void replace_from_actual_canonical_839fe0(std::string actual_acp_value)const {
        const auto owned=state_;
        try {
            State::Scope scope(owned);owned->check();
            Filename::need(owned->copy_attempted,"Package canonical rewrite preceded filename copy");
            //Private LoadActor calls this ONLY inside the actual Warm native
            //record loan, with owned OutputProducer839FE0/820040 bytes.
            actual_acp_value.resize(std::strlen(actual_acp_value.c_str()));
            owned->check();owned->field0.swap(actual_acp_value);owned->check();
        }catch(...){owned->failed.store(true);throw;}
    }
    std::string copied_input_for_498940_global_lookup()const {
        const auto owned=state_;
        try {
            State::Scope scope(owned);owned->check();
            Filename::need(owned->copy_attempted,"Package field0 actual filename copy absent");
            std::string result=owned->field0;owned->check();return result;
        } catch(...){owned->failed.store(true);throw;}
    }
    std::string copied_input_for_498940_global_lookup_completed(const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& cold)const {
        const auto owned=state_;
        try {
            State::Scope scope(owned);owned->check_completed(loan,cold);
            Filename::need(owned->copy_attempted,"Completed field0 actual pre-load copy absent");
            auto result=owned->field0;owned->check_completed(loan,cold);return result;
        }catch(...){owned->failed.store(true);throw;}
    }
    std::string copied_receiver_for_62ec20_fallback()const {
        const auto owned=state_;
        try {
            State::Scope scope(owned);owned->check();
            //498940 still needs actual8211F0 result, conditional839FE0
            //canonical filename and820040 CP_ACP rewrite, then8348C0 success.
            // No caller-ready/phase boolean can bypass these absent effects.
            throw std::runtime_error("Package field0 fallback unissued: genuine global lookup/canonical rewrite/descriptor reader absent");
        } catch(...){owned->failed.store(true);throw;}
    }
    BnwNativePackageFStringField0(const BnwNativePackageFStringField0&)=delete;
    BnwNativePackageFStringField0&operator=(const BnwNativePackageFStringField0&)=delete;
public:
    ~BnwNativePackageFStringField0(){state_->closed.store(true);}
};
} //namespace civ5::ui::detail
