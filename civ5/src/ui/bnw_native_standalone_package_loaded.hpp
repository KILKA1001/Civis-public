#pragma once
#include "bnw_native_package_load_actor.hpp"
namespace civ5::ui::detail {
//Actual standalone successful738F00 return capability. Only Load's real owned
//metadata completion constructs it. It is not Original Steam/FString ABI parity.
class BnwNativeStandalonePackageLoaded final {
    friend class BnwNativeInitialDescriptorSource;
    friend class BnwNativePreDx9DiscoveryOwner;
    friend class BnwNativePackageLoadActor;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativePackageResourceCheckpoint;
    friend class BnwNativeSkinDocumentRegistration;
    friend class BnwNativeSelectedCategoryRecordBorrower;
    using Load=BnwNativePackageLoadActor;
    using Core=BnwNativePackageDescriptorCoreWriter;
    const std::weak_ptr<const Load> actor_;
    const Core::Metadata* const actual_metadata_;
    const BnwNativePackageFStringField0* const actual_receiver_;
    std::weak_ptr<const BnwNativeStandalonePackageLoaded> self_;
    BnwNativeStandalonePackageLoaded(std::weak_ptr<const Load> actual,const Core::Metadata& metadata,
            const BnwNativePackageFStringField0& receiver)
        :actor_(std::move(actual)),actual_metadata_(&metadata),actual_receiver_(&receiver){}
    static void need(bool value,const char* error){if(!value)throw std::runtime_error(error);}
    void check(const std::shared_ptr<const BnwNativeStandalonePackageLoaded>& retained,
               const std::shared_ptr<const Load>& actual)const {
        need(retained&&retained.get()==this&&retained.use_count()>1,
            "Actual returned Loaded lost external factory owner");
        need(actual&&actual.use_count()>1&&!actor_.owner_before(actual)&&!actual.owner_before(actor_),
            "Actual returned Loaded lost externally owned SAME receiver");
        const auto& state=actual->state_;state->current();
        need(!state->returned_loaded.owner_before(retained)&&!retained.owner_before(state->returned_loaded)&&
             !state->returned_loaded.expired()&&state->completed_metadata_identity==actual_metadata_&&
             state->receiver.get()==actual_receiver_&&state->metadata_input,
            "Loaded is not SAME actual completed writer/field0 occurrence");
        if(state->pre_dx9_lookup_source&&!state->adopted_warm){
            const auto initial=state->pre_dx9_lookup_source->lock();need(bool(initial),"Initial returned package issuer retired");
            initial->require_returned_package_current(*this);
        }else state->warm_consumer()->require_returned_standalone_package_loaded_current(*this);
        need(actual.use_count()>1&&retained.use_count()>1,"Actual returned factory ownership retired in source callback");
    }
    void check_completed(const std::shared_ptr<const BnwNativeStandalonePackageLoaded>& retained,
        const std::shared_ptr<const Load>& actual,const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& cold)const {
        loan.require_same_cold(cold);
        need(retained&&retained.get()==this&&retained.use_count()>1&&!self_.owner_before(retained)&&!retained.owner_before(self_),
            "Completed Loaded lost same external factory token/control");
        need(actual&&actual.use_count()>1&&!actor_.owner_before(actual)&&!actual.owner_before(actor_),
            "Completed Loaded lost externally owned same receiver/control");
        const auto& state=actual->state_;state->current_completed(loan,cold);
        need(!state->returned_loaded.owner_before(retained)&&!retained.owner_before(state->returned_loaded)&&
            !state->returned_loaded.expired()&&state->completed_metadata_identity==actual_metadata_&&
            state->receiver.get()==actual_receiver_&&state->metadata_input&&(state->actual_lookup_source||state->adopted_warm),
            "Completed Loaded is another actual metadata/receiver/selected-source occurrence");
        const auto factory=state->warm_consumer();need(bool(factory),"Completed actual factory Warm retired");
        factory->require_returned_standalone_package_loaded_current_completed(*this,loan,cold);
        need(actual.use_count()>1&&retained.use_count()>1,"Completed actual factory ownership retired during check");
        state->current_completed(loan,cold);
    }
    class Loan final {
        friend class BnwNativeStandalonePackageLoaded;
        friend class BnwNativeGlobalResourceWarmOwner;
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        const BnwNativeStandalonePackageLoaded& token;
        const std::shared_ptr<const BnwNativeStandalonePackageLoaded>& retained;
        const std::shared_ptr<const Load>& actor;
        const Load::ResourceLoan& resource;
        const std::thread::id creator_thread=std::this_thread::get_id();
        const BnwNativeCompletedColdSourceLoan* const completed_source=nullptr;
        const BnwNativeColdFilesystemOwner* const completed_cold=nullptr;
        Loan(const BnwNativeStandalonePackageLoaded& value,
             const std::shared_ptr<const BnwNativeStandalonePackageLoaded>& held,
             const std::shared_ptr<const Load>& actual,const Load::ResourceLoan& source)
            :token(value),retained(held),actor(actual),resource(source){}
        Loan(const BnwNativeStandalonePackageLoaded& value,const std::shared_ptr<const BnwNativeStandalonePackageLoaded>& held,
            const std::shared_ptr<const Load>& actual,const Load::ResourceLoan& source,
            const BnwNativeCompletedColdSourceLoan& completed,const BnwNativeColdFilesystemOwner& cold)
            :token(value),retained(held),actor(actual),resource(source),completed_source(&completed),completed_cold(&cold){}
      public:
        void require_current()const {
            if(completed_source) {
                need(creator_thread==std::this_thread::get_id(),"Completed Loaded loan foreign creator thread");
                need(completed_cold,"Completed Loaded loan actual Cold binding missing");
                resource.require_same_completed_binding(*completed_source,*completed_cold);
                token.check_completed(retained,actor,*completed_source,*completed_cold);resource.require_current();
                token.check_completed(retained,actor,*completed_source,*completed_cold);
            }else {need(creator_thread==std::this_thread::get_id(),"Loaded validation foreign creator thread");token.check(retained,actor);resource.require_current();token.check(retained,actor);}
        }
        const auto& metadata()const {require_current();return *token.actual_metadata_;}
        const auto& fields()const {require_current();return resource.fields();}
        const auto& document()const {require_current();return resource.document();}
        const auto& actual_resource_loan()const {require_current();return resource;}
        std::string copied_receiver_for_62ec20()const {require_current();auto value=resource.copied_receiver_for_62ec20();require_current();return value;}
        std::string platform_lowercase_ascii_for_83b4e0(std::string_view value)const {
            require_current();auto result=resource.platform_lowercase_ascii_for_83b4e0(value);require_current();return result;
        }
        template<class Consumer>void with_native_record(const std::string& query,Consumer&& consumer)const {
            require_current();resource.with_native_record(query,std::forward<Consumer>(consumer));require_current();
        }
        void invalidate()const noexcept {resource.invalidate();}
        Loan(const Loan&)=delete;Loan&operator=(const Loan&)=delete;
    };
    template<class Consumer>void with_actual_descriptor(Consumer&& consumer)const {
        //One pin of each facade across the callback. They do not replace the
        //actual external Warm factory vector and retained receiver owners.
        const auto retained=self_.lock();const auto actual=actor_.lock();
        try {
            check(retained,actual);const auto& state=actual->state_;
            const auto consume=[&](const Load::ResourceLoan& resource) {
                const Loan loan(*this,retained,actual,resource);
                loan.require_current();consumer(loan);loan.require_current();
            };
            if(state->busy.load()) {
                need(state->active_resource_loan&&state->active_resource_loan_address,
                    "Nested Loaded borrow is outside SAME actual held ResourceLoan");
                consume(*state->active_resource_loan_address); //No second Actor/Core Scope.
            }else actual->with_standalone_metadata_before_category_label([&](const auto& metadata){
                need(&metadata.metadata()==actual_metadata_,"Loaded metadata differs from SAME owned Core");consume(metadata);
            });
            check(retained,actual);
        }catch(...){if(actual)actual->state_->failed.store(true);throw;}
    }
    template<class Consumer>void with_actual_descriptor_completed(const BnwNativeCompletedColdSourceLoan& completed,
        const BnwNativeColdFilesystemOwner& cold,Consumer&& consumer)const {
        completed.require_same_cold(cold);
        const auto retained=self_.lock();const auto actual=actor_.lock();
        try {
            check_completed(retained,actual,completed,cold);const auto& state=actual->state_;
            const auto consume=[&](const Load::ResourceLoan& resource){
                resource.require_same_completed_binding(completed,cold);
                const Loan loan(*this,retained,actual,resource,completed,cold);loan.require_current();
                {
                    auto target=std::forward<Consumer>(consumer);loan.require_current();target(loan);loan.require_current();
                } // User target destruction stays inside genuine Actor/Core/Parser/Reader/Cold.
                loan.require_current();
            };
            if(state->busy.load()) {
                need(state->active_resource_loan&&state->active_resource_loan_address,
                    "Nested completed Loaded borrow outside actual active ResourceLoan");
                // Busy alone grants nothing. Exact immutable source binding,
                // active address and generation are checked before consume.
                state->active_resource_loan_address->require_same_completed_binding(completed,cold);
                consume(*state->active_resource_loan_address);
            }else actual->with_standalone_metadata_completed(completed,cold,[&](const auto& metadata){
                need(&metadata.metadata()==actual_metadata_,"Completed Loaded metadata differs from actual Core");consume(metadata);
            });
            check_completed(retained,actual,completed,cold);
        }catch(...){if(actual)actual->state_->failed.store(true);throw;}
    }
public:
    BnwNativeStandalonePackageLoaded(const BnwNativeStandalonePackageLoaded&)=delete;
    BnwNativeStandalonePackageLoaded&operator=(const BnwNativeStandalonePackageLoaded&)=delete;
};
inline std::shared_ptr<const BnwNativeStandalonePackageLoaded>
BnwNativePackageLoadActor::write_standalone_descriptor_metadata_738f00()const {
    const auto owned=state_;State::Scope operation(owned);
    try {
        owned->current();Filename::need(owned->receiver&&owned->descriptor_bytes&&owned->parsed_xml&&
            owned->descriptor_core&&(owned->actual_lookup_source||owned->pre_dx9_lookup_source)&&owned->metadata_input&&
            !owned->completed_metadata_identity&&owned->returned_loaded.expired(),
            "Standalone factory return requires actual typed-core/read/parser/selected-input/new metadata occurrence");
        //Old partial Core has no retained actual arithmetic input and cannot
        //be upgraded into a Loaded token by a later caller-selected input.
        const auto selected=owned->metadata_input;
        require_metadata_input_current(selected);owned->current();
        owned->descriptor_core->write_actual_standalone_metadata_738f00(selected);owned->current();
        std::shared_ptr<const BnwNativeStandalonePackageLoaded> returned;
        owned->descriptor_core->with_current_standalone_metadata([&](const auto&,const auto& metadata,const auto&,const auto& guard){
            guard.require_current();owned->current();require_metadata_input_current(selected);
            const auto bytes=owned->receiver->copied_input_for_498940_global_lookup();
            (void)bytes; //Actual same receiver check, never a copied-string issuer.
            auto token=std::shared_ptr<BnwNativeStandalonePackageLoaded>(new BnwNativeStandalonePackageLoaded(owned->actor,metadata,*owned->receiver));
            token->self_=token;returned=std::move(token);
            owned->completed_metadata_identity=&metadata;owned->returned_loaded=returned;
            guard.require_current();owned->current();selected->require_current();
        });owned->current();
        Filename::need(bool(returned),"Actual metadata writer did not return its owned completion");
        return returned; //Only this successful real writer return can enter factory vector.
    }catch(...){owned->failed.store(true);throw;}
}
static_assert(!std::is_default_constructible_v<BnwNativeStandalonePackageLoaded>);
static_assert(!std::is_constructible_v<BnwNativeStandalonePackageLoaded,std::string>);
} //namespace civ5::ui::detail
