#pragma once
#include "bnw_native_package_fstring_field0.hpp"
#include "bnw_native_pre_dx9_discovery_owner.hpp"
#include "bnw_native_package_descriptor_byte_reader.hpp"
#include "bnw_native_package_xml_parser_owner.hpp"
#include "bnw_native_package_descriptor_core_writer.hpp"
#include "bnw_native_global_resource_warm_owner.hpp"
#include "bnw_native_skin_resource_query_output_producer.hpp"
#include "bnw_native_archive_v6_entry_admission.hpp"
#include <optional>
#include <type_traits>

namespace civ5::ui::detail {
class BnwNativePackageCategory2Pair;class BnwNativeInitialDescriptorSource;
class BnwNativeSkinResourceQueryOutputProducer;
class BnwNativeSelectedCategoryManagerSource;
class BnwNativeSelectedCategoryRecordBorrower;
class BnwNativeStandalonePackageLoaded;

//40CD60's new receiver before498940. The actual Discovery filename source
//owns the ACP bytes and their registered-source currentness. Existing parsed
//RetainedDescriptor/FileIdentity/manifest.path cannot construct this owner.
//This packet implements6F7D30's empty field VALUE and498940's name-copy
//prefix, an owned canonical rewrite bridge and descriptor byte reader.
//It does not implement native FString ABI, actual stream issuer,
//738F00 validator or the pre-Category filename scheduler binding.
class BnwNativePackageLoadActor final {
    friend class BnwNativeInitialDescriptorSource;
    friend class BnwNativeGlobalColorRegistryBank;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativePreDx9DiscoveryOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeSkinResourceQueryOutputProducer;
    friend class BnwNativePackageResourceCheckpoint;
    friend class BnwNativeSelectedCategoryManagerSource;
    friend class BnwNativeSelectedCategoryRecordBorrower;
    friend class BnwNativeStandalonePackageLoaded;
    using Discovery=BnwNativeResourceDiscoveryActor;
    using Filename=BnwNativeResourceDiscoveryFilename;
    using Field=BnwNativePackageFStringField0;
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Reader=BnwNativePackageDescriptorByteReader;
    using Parser=BnwNativePackageXmlParserOwner;
    using CoreWriter=BnwNativePackageDescriptorCoreWriter;
    using Output=BnwNativeSkinResourceQueryOutputProducer;
    class ResourceLoan;
    struct State {
        const std::thread::id thread=std::this_thread::get_id();
        const std::weak_ptr<const Discovery> discovery;
        const std::weak_ptr<const Filename::Source> filename;
        std::weak_ptr<const BnwNativePackageLoadActor> actor;
        std::atomic<bool> closed{false},failed{false},busy{false};
        std::unique_ptr<Field> receiver;
        std::shared_ptr<BnwNativePackageCategory2Pair> initial_category2_pair;
        std::optional<std::weak_ptr<Warm>> actual_lookup_source;
        // Actual initial installation namespace, never relabelled as Warm or saved lease.
        std::optional<std::weak_ptr<BnwNativePreDx9DiscoveryOwner>> pre_dx9_lookup_source;
        std::optional<std::weak_ptr<Warm>> adopted_warm; // Additional consumer, never a rewritten initial source.
        std::shared_ptr<Reader> descriptor_bytes;
        std::shared_ptr<Parser> parsed_xml;
        std::shared_ptr<CoreWriter> descriptor_core;
        std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner> metadata_input;
        std::uint64_t resource_loan_epoch=0,active_resource_loan=0;
        //6F7D30 actual +50/+58/+60 are zero. Only the genuine manager
        //issuer can attach an actual selected source; expired != native zero.
        std::array<std::optional<std::weak_ptr<const BnwNativeSelectedCategoryManagerSource>>,3> category_slots{};
        const ResourceLoan* active_resource_loan_address=nullptr;
        std::uint64_t canonical_input_epoch=0,active_canonical_input_generation=0;
        const Warm::PreselectionCanonicalInputLoan* active_canonical_input=nullptr;
        std::weak_ptr<const BnwNativeStandalonePackageLoaded> returned_loaded;
        const CoreWriter::Metadata* completed_metadata_identity=nullptr;
        State(std::weak_ptr<const Discovery> actual,const Filename& name)
            :discovery(std::move(actual)),filename(name.source_){}
        void live()const {
            Filename::need(thread==std::this_thread::get_id(),"Package load foreign thread");
            Filename::need(!closed.load()&&!failed.load()&&!actor.expired(),
                "Package load receiver retired");
            Filename::need(!discovery.expired(),"Package load actual Discovery retired");
        }
        std::shared_ptr<Warm> warm_consumer()const {
            live();Filename::need(!(actual_lookup_source&&adopted_warm),"Package has two Warm consumer origins");
            const auto& binding=actual_lookup_source?actual_lookup_source:adopted_warm;
            Filename::need(bool(binding),"Package has no actual Warm consumer");
            const auto actual=binding->lock();Filename::need(bool(actual),"Package actual Warm consumer retired");
            if(adopted_warm){Filename::need(bool(pre_dx9_lookup_source),"Adopted receiver lost initial namespace source");
                const auto initial=pre_dx9_lookup_source->lock();Filename::need(bool(initial),"Initial receiver source retired");
                initial->require_adopted_warm_identity_without_callbacks(*actual,actor);}
            return actual;
        }
        template<class Control> bool matches_warm_consumer_without_callbacks(const Control& control)const {
            const auto actual=warm_consumer();return !actual.owner_before(control)&&!control.owner_before(actual);
        }
        void current()const {
            live();const auto source=filename.lock();
            Filename::need(bool(source),"Package load actual filename occurrence expired");
            Filename::need(!discovery.owner_before(source->actor)&&
                          !source->actor.owner_before(discovery),
                          "Package load filename belongs to another actual Discovery");
            source->check();live();
            if(pre_dx9_lookup_source) {
                const auto initial=pre_dx9_lookup_source->lock();
                Filename::need(bool(initial),"Package actual initial Default issuer expired");
                initial->require_current();live();
            }
            if(actual_lookup_source||adopted_warm) {
                const auto lookup=warm_consumer();
                Filename::need(bool(lookup),"Package actual canonical lookup source expired");
                lookup->require_current();live();
            }
            Filename::need(!discovery.owner_before(source->actor)&&
                          !source->actor.owner_before(discovery),
                          "Package load filename actor changed during source callback");
        }
        void current(const Filename::Source::ConstructionOperation& operation)const {
            live();const auto source=filename.lock();Filename::need(bool(source),"Installed load Source expired");
            Filename::need(!actual_lookup_source&&!adopted_warm&&!pre_dx9_lookup_source&&!discovery.owner_before(source->actor)&&
                !source->actor.owner_before(discovery),"Installed constructor changed phase/Discovery");
            operation.require_same(source);live();
        }
        void current_completed(const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)const {
            live();loan.require_same_cold(cold);const auto source=filename.lock();
            Filename::need(source&&!discovery.owner_before(source->actor)&&!source->actor.owner_before(discovery),
                "Completed Load has another actual filename/Discovery control");
            source->check_completed(loan,cold);live();
            if(actual_lookup_source||adopted_warm) {
                const auto lookup=warm_consumer();Filename::need(bool(lookup),"Completed actual Warm source retired");
                lookup->require_current_completed(loan,cold);live();
            }
            Filename::need(!discovery.owner_before(source->actor)&&!source->actor.owner_before(discovery),
                "Completed actual filename Discovery changed during source check");loan.require_same_cold(cold);
        }
        struct Scope {
            std::shared_ptr<State> owned;
            explicit Scope(std::shared_ptr<State> value):owned(std::move(value)) {
                owned->live();
                if(owned->busy.exchange(true)) {
                    owned->failed.store(true);throw std::runtime_error("Package load reentry");
                }
            }
            ~Scope(){owned->busy.store(false);}
        };
    };
    const std::shared_ptr<State> state_;
    BnwNativePackageLoadActor(std::weak_ptr<const Discovery> actual,const Filename& name)
        :state_(std::make_shared<State>(std::move(actual),name)){}

    //Private actual factory input only, not a caller-path or readiness issuer.
    //No caller has been wired into the Package descriptor scheduler yet.
    static std::shared_ptr<BnwNativePackageLoadActor> construct_preload_receiver_for_498940(
            const Discovery& actual,const Filename& name);
    std::string copied_input_for_498940_global_lookup()const {
        const auto owned=state_;State::Scope operation(owned);
        try {
            owned->current();Filename::need(bool(owned->receiver),"Package load receiver absent");
            auto value=owned->receiver->copied_input_for_498940_global_lookup();
            owned->current();return value;
        }catch(...){owned->failed.store(true);throw;}
    }
    void prepare_canonical_receiver_from_initial_default(const std::shared_ptr<BnwNativePreDx9DiscoveryOwner>& actual)const {
        const auto initial=actual;const auto owned=state_;State::Scope operation(owned);
        try {
            owned->current();Filename::need(initial&&owned->receiver&&!owned->actual_lookup_source&&
                !owned->pre_dx9_lookup_source,"Initial498940 lookup missing/replayed or wrong domain");
            initial->require_initial_default_miss(*this); // Live actual namespace, not a supplied MISS.
            // Original498940 leaves SAME copied field0 unchanged on actual NULL lookup.
            owned->pre_dx9_lookup_source=initial;owned->current();
        }catch(...){owned->failed.store(true);throw;}
    }
    void prepare_canonical_receiver_for_498940(const std::shared_ptr<Warm>& actual)const {
        const auto lookup=actual; //Own facade BEFORE any source/client callback.
        const auto owned=state_;State::Scope operation(owned);
        try {
            owned->current();Filename::need(lookup&&owned->receiver,"Package canonical lookup lacks actual owner");
            Filename::need(!owned->actual_lookup_source&&!owned->pre_dx9_lookup_source,"Package canonical lookup occurrence replay");
            const auto input=owned->receiver->copied_input_for_498940_global_lookup();
            owned->current();lookup->require_current();
            std::size_t callbacks=0;
            lookup->with_native_record(input.c_str(),[&](const BnwNativeArchiveRecordDiagnostic* record,
                                                       const std::u16string* prefix,const std::string* first){
                Filename::need(callbacks++==0,"Package actual lookup invoked callback repeatedly");
                owned->current();
                if(record&&record->offset==0) {
                    //498940 Entry+8 zero only. This is owned839FE0/820040
                    //conversion, BEFORE5EB290's separate81E100 slash pass.
                    auto canonical=Output::canonical_filename_839fe0(prefix,first);
                    owned->current();
                    owned->receiver->replace_from_actual_canonical_839fe0(std::move(canonical));
                    owned->current();
                }
                //Actual archive record or inspected MISS leaves SAME copied
                //field0 intact. Neither outcome means descriptor read success.
            });
            Filename::need(callbacks==1,"Package actual lookup did not invoke its borrowed result");
            lookup->require_current();owned->current();
            owned->actual_lookup_source=lookup;owned->current();
        }catch(...){owned->failed.store(true);throw;}
    }
    bool read_descriptor_bytes_before_834b30(const std::shared_ptr<Reader::Source>& actual_stream)const {
        const auto owned=state_;State::Scope operation(owned);
        try {
            owned->current();
            Filename::need(owned->receiver&&(owned->actual_lookup_source||owned->pre_dx9_lookup_source)&&!owned->descriptor_bytes,
                "Package descriptor bytes require SAME actual lookup/receiver and new read occurrence");
            const auto reader=Reader::create(actual_stream);
            const bool read=reader->read_8348c0(*owned->receiver);
            owned->current();
            if(!read){owned->failed.store(true);return false;}
            owned->descriptor_bytes=reader;owned->current();
            return true; //8348C0 byte prefix, NOT834B30 parser success/Loaded.
        }catch(...){owned->failed.store(true);throw;}
    }
    template<class Consumer> void with_descriptor_bytes_before_834b30(Consumer&& consume)const {
        const auto owned=state_;State::Scope operation(owned);
        try {
            owned->current();Filename::need(bool(owned->descriptor_bytes),"Package descriptor bytes absent");
            owned->descriptor_bytes->with_current_bytes_before_834b30(std::forward<Consumer>(consume));
            owned->current();
        }catch(...){owned->failed.store(true);throw;}
    }
    bool parse_descriptor_owned_bytes_834b30()const {
        const auto owned=state_;State::Scope operation(owned);
        try {
            owned->current();Filename::need(owned->descriptor_bytes&&!owned->parsed_xml,
                "Package actual read/new parser occurrence absent");
            const auto parser=Parser::create(owned->descriptor_bytes);
            const bool parsed=parser->parse_834b30();owned->current();
            if(!parsed){owned->failed.store(true);return false;}
            owned->parsed_xml=parser;owned->current();
            return true; //Actual syntax only, NOT738F00 or Loaded/fallback.
        }catch(...){owned->failed.store(true);throw;}
    }
    bool write_descriptor_core_738f00(
            const std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner>& selected_input={})const {
        auto selected=selected_input;
        const auto owned=state_;State::Scope operation(owned);
        try {
            owned->current();Filename::need(owned->parsed_xml&&!owned->descriptor_core,
                "Package core writer requires actual fresh syntax/new occurrence");
            if(!selected&&owned->pre_dx9_lookup_source){
                const auto initial=owned->pre_dx9_lookup_source->lock();Filename::need(bool(initial),"Initial Core issuer retired");
                selected=initial->borrow_core_platform_input(*this);
            }
            if(selected)require_metadata_input_current(selected);
            const auto writer=CoreWriter::create(owned->parsed_xml,selected);
            const bool written=writer->write_actual_descriptor_core_738f00();owned->current();
            if(!written){owned->failed.store(true);return false;}
            owned->descriptor_core=writer;owned->metadata_input=selected;owned->current();
            return true; //Core bounded fields only; full Loaded still not issued.
        }catch(...){owned->failed.store(true);throw;}
    }
    void require_metadata_input_current(const std::shared_ptr<const runtime::BnwSelectedLanguageInputOwner>& input)const {
        state_->live();Filename::need(bool(input),"Package actual platform/metadata input absent");
        if(state_->pre_dx9_lookup_source){const auto initial=state_->pre_dx9_lookup_source->lock();
            Filename::need(bool(initial),"Initial metadata issuer retired");initial->require_same_metadata_input(input);
        }else state_->warm_consumer()->require_same_selected_language_input(*input);
        state_->live();
    }
    std::shared_ptr<const BnwNativeStandalonePackageLoaded> write_standalone_descriptor_metadata_738f00()const;
    class ResourceLoan {
        friend class BnwNativePackageLoadActor;
        friend class BnwNativeGlobalResourceWarmOwner;
        friend class BnwNativeSelectedCategoryManagerSource;
        friend class BnwNativeSelectedCategoryRecordBorrower;
        friend class BnwNativeStandalonePackageLoaded;
        const std::thread::id creator_thread=std::this_thread::get_id();
        const BnwNativeCompletedColdSourceLoan* const completed_source=nullptr;
        const BnwNativeColdFilesystemOwner* const completed_cold=nullptr;
        const std::weak_ptr<State> state;
        const std::uint64_t epoch;
        const CoreWriter::Fields& actual_fields;
        const Parser::Document& actual_document;
        const CoreWriter::HeldCoreGuard& actual_source_guard;
        ResourceLoan(const std::shared_ptr<State>& actual,std::uint64_t occurrence,
                     const CoreWriter::Fields& fields,const Parser::Document& document,
                     const CoreWriter::HeldCoreGuard& guard)
            :state(actual),epoch(occurrence),actual_fields(fields),actual_document(document),actual_source_guard(guard){}
        ResourceLoan(const std::shared_ptr<State>& actual,std::uint64_t occurrence,
            const CoreWriter::Fields& fields,const Parser::Document& document,const CoreWriter::HeldCoreGuard& guard,
            const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)
            :completed_source(&loan),completed_cold(&cold),state(actual),epoch(occurrence),actual_fields(fields),
             actual_document(document),actual_source_guard(guard){}
        void require_same_completed_binding(const BnwNativeCompletedColdSourceLoan& loan,
            const BnwNativeColdFilesystemOwner& cold)const {
            Filename::need(creator_thread==std::this_thread::get_id(),"Completed ResourceLoan foreign creator thread");
            const auto owned=state.lock();
            Filename::need(owned&&owned->busy.load()&&epoch&&owned->active_resource_loan==epoch&&
                owned->active_resource_loan_address==this,"Completed ResourceLoan outside actual active address/generation");
            Filename::need(completed_source==&loan&&completed_cold==&cold,
                "Nested completed descriptor has another immutable actual Cold binding");require_current();
        }
      public:
        void require_current()const {
            if(completed_source) {
                Filename::need(creator_thread==std::this_thread::get_id(),"Completed ResourceLoan foreign creator thread");
                const auto owned=state.lock();
                Filename::need(owned&&owned->busy.load()&&epoch&&owned->active_resource_loan==epoch&&
                    owned->active_resource_loan_address==this&&completed_cold,
                    "Completed ResourceLoan outside actual active address/generation");
                // Borrowed refs are dereferenced only after actual creator/live
                // private address/generation membership, never ambient busy.
                completed_source->require_same_cold(*completed_cold);
                owned->current_completed(*completed_source,*completed_cold);
                actual_source_guard.require_current();owned->current_completed(*completed_source,*completed_cold);return;
            }
            const auto owned=state.lock();
            Filename::need(owned&&owned->busy.load()&&owned->active_resource_loan==epoch&&epoch&&owned->active_resource_loan_address==this,
                "Package resource checkpoint is outside SAME active private borrow");
            owned->current();actual_source_guard.require_current();owned->current();
        }
        const auto& document()const {require_current();return actual_document;}
        const auto& fields()const {require_current();return actual_fields;}
        std::string copied_receiver_for_62ec20()const {
            require_current();const auto owned=state.lock();
            Filename::need(bool(owned->receiver),"Actual checkpoint field0 absent");
            auto result=completed_source?owned->receiver->copied_input_for_498940_global_lookup_completed(*completed_source,*completed_cold):
                owned->receiver->copied_input_for_498940_global_lookup();
            require_current();return result; //Actual checkpoint receiver, NOT Loaded issuer.
        }
        std::string platform_lowercase_ascii_for_83b4e0(std::string_view text)const {
            require_current();const auto owned=state.lock();
            Filename::need(owned->metadata_input&&(owned->actual_lookup_source||owned->adopted_warm),
                "Selected manager CRC lacks actual retained selected-input/Warm binding");
            const auto actual=owned->warm_consumer();
            Filename::need(bool(actual),"Selected manager CRC actual Warm source retired");
            if(completed_source)actual->require_same_selected_language_input_completed(*owned->metadata_input,*completed_source,*completed_cold);
            else actual->require_same_selected_language_input(*owned->metadata_input);require_current();
            auto result=actual_source_guard.platform_lowercase_ascii(text);
            require_current();return result; //No second Core::Scope or copied input authority.
        }
        template<class Consumer> void with_native_record(const std::string& query,Consumer&& consumer)const {
            require_current();const auto owned=state.lock();
            Filename::need(bool(owned->actual_lookup_source||owned->adopted_warm),"Actual checkpoint global owner absent");
            const auto actual=owned->warm_consumer();
            Filename::need(bool(actual),"Actual checkpoint global owner retired");
            const auto consume=[&](const auto* node,const auto* prefix,const auto* first){
                require_current();consumer(node,prefix,first);require_current();
            };
            if(completed_source)actual->with_native_record_completed(query.c_str(),*completed_source,*completed_cold,consume);
            else if(actual->active_owned_category_preconstructor_)
                actual->with_owned_category_preconstructor_record(this,query.c_str(),consume);
            else actual->with_native_record(query.c_str(),consume);require_current();
        }
        void invalidate()const noexcept {if(const auto owned=state.lock())owned->failed.store(true);}
        ResourceLoan(const ResourceLoan&)=delete;
        ResourceLoan&operator=(const ResourceLoan&)=delete;
    };
    struct HeldResourceLoan {
        const std::shared_ptr<State>& state;const ResourceLoan* loan;
        HeldResourceLoan(const std::shared_ptr<State>& actual,const ResourceLoan& held):state(actual),loan(&held) {
            Filename::need(!state->active_resource_loan_address&&state->active_resource_loan,
                "Actual resource loan address replay/outside epoch");
            state->active_resource_loan_address=loan;
        }
        ~HeldResourceLoan() {
            if(state->active_resource_loan_address!=loan)state->failed.store(true);
            state->active_resource_loan_address=nullptr;
        }
        HeldResourceLoan(const HeldResourceLoan&)=delete;
    };
    class MetadataLoan final : public ResourceLoan {
        friend class BnwNativePackageLoadActor;
        const CoreWriter::Metadata& actual_metadata;
        MetadataLoan(const std::shared_ptr<State>& actual,std::uint64_t occurrence,
                     const CoreWriter::Fields& fields,const CoreWriter::Metadata& metadata,
                     const Parser::Document& document,const CoreWriter::HeldCoreGuard& guard)
            :ResourceLoan(actual,occurrence,fields,document,guard),actual_metadata(metadata){}
        MetadataLoan(const std::shared_ptr<State>& actual,std::uint64_t occurrence,
            const CoreWriter::Fields& fields,const CoreWriter::Metadata& metadata,const Parser::Document& document,
            const CoreWriter::HeldCoreGuard& guard,const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)
            :ResourceLoan(actual,occurrence,fields,document,guard,loan,cold),actual_metadata(metadata){}
      public:
        const auto& metadata()const {require_current();return actual_metadata;}
        MetadataLoan(const MetadataLoan&)=delete;
        MetadataLoan&operator=(const MetadataLoan&)=delete;
    };
    template<class Consumer> void with_standalone_metadata_before_category_label(Consumer&& consumer)const {
        const auto owned=state_;State::Scope operation(owned);
        try {
            owned->current();Filename::need(owned->receiver&&(owned->actual_lookup_source||owned->pre_dx9_lookup_source)&&owned->descriptor_core&&owned->metadata_input,
                "Category label metadata requires SAME real selected input/receiver/read/parser/core/metadata");
            Filename::need(owned->active_resource_loan==0&&owned->resource_loan_epoch!=UINT64_MAX,
                "Standalone metadata checkpoint occurrence replay/overflow");
            const auto epoch=++owned->resource_loan_epoch;
            struct End {
                const std::shared_ptr<State>& state;const std::uint64_t epoch;
                ~End(){if(state->active_resource_loan!=epoch||state->active_resource_loan_address)state->failed.store(true);state->active_resource_loan=0;}
            } end{owned,epoch};
            owned->active_resource_loan=epoch;
            owned->descriptor_core->with_current_standalone_metadata([&](const auto& fields,const auto& metadata,const auto& document,const auto& guard){
                const MetadataLoan loan(owned,epoch,fields,metadata,document,guard);
                const HeldResourceLoan held(owned,loan);
                loan.require_current();consumer(loan);loan.require_current();
            });owned->current();
        }catch(...){owned->failed.store(true);throw;}
    }
    template<class Consumer>void with_standalone_metadata_completed(const BnwNativeCompletedColdSourceLoan& completed,
        const BnwNativeColdFilesystemOwner& cold,Consumer&& consumer)const {
        const auto owned=state_;State::Scope operation(owned);
        try {
            owned->current_completed(completed,cold);
            Filename::need(owned->receiver&&(owned->actual_lookup_source||owned->pre_dx9_lookup_source)&&owned->descriptor_core&&owned->metadata_input&&
                owned->completed_metadata_identity&&!owned->returned_loaded.expired(),
                "Completed descriptor requires same true returned metadata/receiver/source");
            Filename::need(owned->active_resource_loan==0&&owned->resource_loan_epoch!=UINT64_MAX,
                "Completed descriptor actual loan replay/overflow");
            const auto epoch=++owned->resource_loan_epoch;
            struct End {const std::shared_ptr<State>& state;const std::uint64_t epoch;
                ~End(){if(state->active_resource_loan!=epoch||state->active_resource_loan_address)state->failed.store(true);
                    state->active_resource_loan=0;}} end{owned,epoch};
            owned->active_resource_loan=epoch;
            owned->descriptor_core->with_current_standalone_metadata_completed(completed,cold,
                [&](const auto& fields,const auto& metadata,const auto& document,const auto& guard){
                    Filename::need(&metadata==owned->completed_metadata_identity,"Completed metadata is another actual writer output");
                    const MetadataLoan loan(owned,epoch,fields,metadata,document,guard,completed,cold);
                    const HeldResourceLoan held(owned,loan);loan.require_current();
                    {
                        auto consume=std::forward<Consumer>(consumer);loan.require_current();consume(loan);loan.require_current();
                    } // Target destruction before held address/Core/Parser/Reader teardown.
                    loan.require_current();
                });owned->current_completed(completed,cold);
        }catch(...){owned->failed.store(true);throw;}
    }
    template<class Consumer> void with_resource_checkpoint_before_metadata_738f00(Consumer&& consumer)const {
        const auto owned=state_;State::Scope operation(owned);
        try {
            owned->current();Filename::need(owned->receiver&&owned->actual_lookup_source&&owned->descriptor_core,
                "Resource checkpoint requires genuine receiver/lookup/read/parser/core fields");
            Filename::need(owned->active_resource_loan==0&&owned->resource_loan_epoch!=UINT64_MAX,
                "Resource checkpoint occurrence replay/overflow");
            const auto epoch=++owned->resource_loan_epoch;
            struct End {
                const std::shared_ptr<State>& state;const std::uint64_t epoch;
                ~End(){if(state->active_resource_loan!=epoch||state->active_resource_loan_address)state->failed.store(true);state->active_resource_loan=0;}
            } end{owned,epoch};
            owned->active_resource_loan=epoch;
            owned->descriptor_core->with_current_core_fields([&](const auto& fields,const auto& document,const auto& guard){
                const ResourceLoan loan(owned,epoch,fields,document,guard);
                const HeldResourceLoan held(owned,loan);
                loan.require_current();consumer(loan);loan.require_current();
            });owned->current();
        }catch(...){owned->failed.store(true);throw;}
    }
    std::string copied_receiver_for_62ec20_fallback()const {
        const auto owned=state_;State::Scope operation(owned);
        try {
            owned->current();Filename::need(bool(owned->receiver),"Package load receiver absent");
            //The existing field refusal remains authoritative: actual8211F0
            //completion andconditional839FE0/820040 ACP rewrite must precede
            //8348C0 read success. Prefix-only Bank selection cannot issue this.
            auto value=owned->receiver->copied_receiver_for_62ec20_fallback();
            owned->current();return value;
        }catch(...){owned->failed.store(true);throw;}
    }
public:
    ~BnwNativePackageLoadActor(){state_->closed.store(true);}
    BnwNativePackageLoadActor(const BnwNativePackageLoadActor&)=delete;
    BnwNativePackageLoadActor&operator=(const BnwNativePackageLoadActor&)=delete;
};
static_assert(!std::is_default_constructible_v<BnwNativePackageLoadActor>);
static_assert(!std::is_constructible_v<BnwNativePackageLoadActor,std::string>);
} //namespace civ5::ui::detail

#include "bnw_native_standalone_package_loaded.hpp"
