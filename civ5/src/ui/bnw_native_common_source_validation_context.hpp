#pragma once
#include <functional>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>
#include <unordered_map>

namespace civ5::ui {
class BnwNativePackageDiscoveryOwner;
class BnwNativeEarlyInputLease;
namespace detail {
class BnwNativeCommon13ArchiveChannel;
class BnwNativeColdFilesystemOwner;
class BnwNativeColdArchiveRequest;
class BnwNativeOriginalStatService;
class BnwNativeOriginalStatRead;
class BnwNativeArchiveConstruction;
class BnwNativeArchiveV6StreamHeaderOwner;
class BnwNativeGlobalArchivePrefixOwner;
class BnwNativeGlobalArchivePrefixRecord;
class BnwNativeDefaultNamespacePolicyOwner;
class BnwNativeArchiveEntryAdmissionArena;
class BnwNativeArchiveV6EntryAdmissionOwner;
class BnwNativeArchiveBasenameCacheOwner;
class BnwNativeArchiveCachePublicationOwner;

// A source-free visit ledger. It authenticates no native owner or source.
// Only the private typed Context below supplies real Parent/source custody.
class BnwNativeCommonSourceVisitLedger final {
  public:
    enum class Kind {early,cold,request,stat_service,stat_read,construction,
                     header,prefix_registry,prefix_record,policy,arena,entries,cache,publication};
    enum class Work {validate_source,identity_only};
  private:
    friend class BnwNativeCommonSourceValidationContext;
    enum class Phase {pre,post,closed};
    enum class State {unvisited,entered,complete};
    struct Record {
        Kind kind;
        const void* address;
        std::weak_ptr<const void> control;
        std::uint64_t sequence;
        State pre=State::unvisited,post=State::unvisited;
    };
    const std::thread::id creator_=std::this_thread::get_id();
    const std::function<void()>& controller_; // Borrowed for exactly this synchronous ledger.
    Phase phase_=Phase::pre;
    mutable bool failed_=false;
    bool pre_sealed_=false;
    std::size_t active_=0;
    std::vector<Record> records_; // Weak controls only; no facade owner pins.
    struct Key final {
        Kind kind;
        const void* address;
        bool operator==(const Key& other)const noexcept {
            return kind==other.kind&&address==other.address;
        }
    };
    struct KeyHash final {
        std::size_t operator()(const Key& key)const noexcept {
            return std::hash<const void*>{}(key.address)^
                   std::hash<std::size_t>{}(static_cast<std::size_t>(key.kind));
        }
    };
    // Private lookup accelerator for exactly this synchronous PRE/POST ledger.
    // It stores no source/control/readiness authority and never outlives it.
    std::unordered_map<Key,std::size_t,KeyHash> record_index_;

    void require_owned_activation_pre_identity_without_callbacks()const {
        require_condition(creator_==std::this_thread::get_id()&&!failed_&&
            phase_==Phase::pre&&pre_sealed_&&active_==0,
            "Activation descriptor Common closure retired/failed/reentered");
    }
    State& state(std::size_t index) {
        return phase_==Phase::pre?records_.at(index).pre:records_.at(index).post;
    }
  public:
    explicit BnwNativeCommonSourceVisitLedger(const std::function<void()>& controller)
        :controller_(controller){check();}
    ~BnwNativeCommonSourceVisitLedger()noexcept{phase_=Phase::closed;}
    BnwNativeCommonSourceVisitLedger(const BnwNativeCommonSourceVisitLedger&)=delete;
    BnwNativeCommonSourceVisitLedger& operator=(const BnwNativeCommonSourceVisitLedger&)=delete;
    void require_condition(bool value,const char* message)const {
        if(!value){failed_=true;throw std::runtime_error(message);}
    }
    void check()const {
        require_condition(creator_==std::this_thread::get_id()&&!failed_&&phase_!=Phase::closed,
                          "Common validation ledger foreign/failed/closed");
        try{controller_();}catch(...){failed_=true;throw;}
    }
    class Visit final {
        friend class BnwNativeCommonSourceVisitLedger;
        BnwNativeCommonSourceVisitLedger& ledger_;
        const std::size_t index_;
        const Phase phase_;
        const Work work_;
        bool complete_=false;
        Visit(BnwNativeCommonSourceVisitLedger& ledger,std::size_t index,Work work)
            :ledger_(ledger),index_(index),phase_(ledger.phase_),work_(work){++ledger_.active_;}
      public:
        Visit(const Visit&)=delete;Visit& operator=(const Visit&)=delete;
        Visit(Visit&&)=delete;Visit& operator=(Visit&&)=delete;
        ~Visit()noexcept {
            if(!complete_)ledger_.failed_=true;
            --ledger_.active_;
        }
        Work work()const noexcept{return work_;}
        void complete() {
            ledger_.check();
            ledger_.require_condition(!complete_&&ledger_.phase_==phase_,"Common validation visit replay/phase changed");
            if(work_==Work::validate_source) {
                ledger_.require_condition(ledger_.state(index_)==State::entered,"Common validation visit not entered");
                ledger_.state(index_)=State::complete;
            }
            complete_=true;
        }
    };
    Visit enter(Kind kind,const void* address,const std::weak_ptr<const void>& control,
                std::uint64_t sequence=0) {
        check();require_condition(address&&!control.expired(),"Common validation requires live actual owner/issuer control");
        const Key key{kind,address};
        const auto found=record_index_.find(key);
        const auto index=found==record_index_.end()?records_.size():found->second;
        if(found==record_index_.end()) {
            require_condition(phase_==Phase::pre&&!pre_sealed_,"Common sealed operation encountered a source absent from PRE");
            try {
                records_.push_back({kind,address,control,sequence});
                require_condition(record_index_.emplace(key,index).second,
                                  "Common validation index repeated an actual owner address");
            }catch(...){failed_=true;throw;}
        }
        const auto& record=records_.at(index);
        require_condition(record.kind==kind&&record.address==address&&
                          record.sequence==sequence&&!record.control.owner_before(control)&&!control.owner_before(record.control),
                          "Common validation owner control/sequence changed at reused address");
        require_condition(state(index)!=State::entered,"Common validation owner reentered before completion");
        const auto work=state(index)==State::complete?Work::identity_only:Work::validate_source;
        if(work==Work::validate_source)state(index)=State::entered;
        return Visit(*this,index,work);
    }
    void require_borrowed(Kind kind,const void* address,const std::weak_ptr<const void>& control,
                          std::uint64_t sequence=0)const {
        check();require_condition(address&&!control.expired(),"Common scalar borrower lost actual owner/issuer control");
        const auto found=record_index_.find(Key{kind,address});
        if(found!=record_index_.end()) {
            const auto& record=records_.at(found->second);
            require_condition(record.kind==kind&&record.address==address&&
                              record.sequence==sequence&&!record.control.owner_before(control)&&!control.owner_before(record.control)&&
                              (phase_==Phase::pre?record.pre:record.post)==State::complete,
                              "Common scalar borrower has incomplete/foreign source phase");
            return;
        }
        require_condition(false,"Common scalar borrower has no completed source visit");
    }
    void seal_pre_closure() {
        check();require_condition(phase_==Phase::pre&&!pre_sealed_&&active_==0,"Common operation seal overlaps/replays PRE visits");
        for(const auto& record:records_)require_condition(record.pre==State::complete,"Common operation seal has incomplete PRE source");
        pre_sealed_=true;check();
    }
    void begin_post() {
        check();require_condition(phase_==Phase::pre&&active_==0,"Common POST overlaps PRE borrowed visits");
        for(const auto& record:records_)require_condition(record.pre==State::complete,"Common PRE source incomplete");
        phase_=Phase::post;check();
    }
    void finish() {
        check();require_condition(phase_==Phase::post&&active_==0,"Common context finish outside POST");
        for(const auto& record:records_)require_condition(record.post==State::complete,"Common POST omitted actual PRE source");
        phase_=Phase::closed;
    }
  private:
    // Only Context may consume this immediately after its real controller FULL
    // and exact private Parent/Directory/source identity hook. Never public.
    void require_adjacent_operation_without_callbacks()const {
        if(creator_!=std::this_thread::get_id())
            throw std::runtime_error("Adjacent Common ledger foreign creator");
        require_condition(!failed_&&phase_!=Phase::closed,
                          "Adjacent Common ledger failed/closed");
    }
    Visit enter_after_controller(Kind kind,const void* address,
        const std::weak_ptr<const void>& control,std::uint64_t sequence,
        const std::function<void()>& identity) {
        require_adjacent_operation_without_callbacks();require_condition(address&&!control.expired(),"Common validation requires live actual owner/issuer control");
        const Key key{kind,address};
        const auto found=record_index_.find(key);
        const auto index=found==record_index_.end()?records_.size():found->second;
        if(found==record_index_.end()) {
            require_condition(phase_==Phase::pre&&!pre_sealed_,"Common sealed operation encountered a source absent from PRE");
            try {
                records_.push_back({kind,address,control,sequence});
                require_condition(record_index_.emplace(key,index).second,
                                  "Common validation index repeated an actual owner address");
            }catch(...){failed_=true;throw;}
        }
        const auto& record=records_.at(index);
        require_condition(record.kind==kind&&record.address==address&&
                          record.sequence==sequence&&!record.control.owner_before(control)&&!control.owner_before(record.control),
                          "Common validation owner control/sequence changed at reused address");
        require_condition(state(index)!=State::entered,"Common validation owner reentered before completion");
        const auto work=state(index)==State::complete?Work::identity_only:Work::validate_source;
        // New record/index allocation remains inside the real Header scope.
        // Authenticate after it before publishing even a lexical Visit.
        try{identity();}catch(...){failed_=true;throw;}
        require_adjacent_operation_without_callbacks();
        if(work==Work::validate_source)state(index)=State::entered;
        return Visit(*this,index,work);
    }
    void require_borrowed_after_controller(Kind kind,const void* address,
        const std::weak_ptr<const void>& control,std::uint64_t sequence=0)const {
        require_adjacent_operation_without_callbacks();require_condition(address&&!control.expired(),"Common scalar borrower lost actual owner/issuer control");
        const auto found=record_index_.find(Key{kind,address});
        if(found!=record_index_.end()) {
            const auto& record=records_.at(found->second);
            require_condition(record.kind==kind&&record.address==address&&
                              record.sequence==sequence&&!record.control.owner_before(control)&&!control.owner_before(record.control)&&
                              (phase_==Phase::pre?record.pre:record.post)==State::complete,
                              "Common scalar borrower has incomplete/foreign source phase");
            return;
        }
        require_condition(false,"Common scalar borrower has no completed source visit");
    }
};

// Synchronous pointer custody only: this template issues no Context/source.
// The actual owner supplies its private typed Context and genuine Header gate.
template<class Context> class BnwNativeCommonSourceContextSlot final {
    Context*& slot_;
    const Context*& identity_;
    Context* const issued_;
    bool& poisoned_;
    bool complete_=false;
    void require_current() {
        if(poisoned_||slot_!=issued_||identity_!=issued_){poisoned_=true;throw std::runtime_error("Common operation context slot changed/poisoned");}
    }
  public:
    BnwNativeCommonSourceContextSlot(Context*& slot,const Context*& identity,Context& issued,bool& poisoned)
        :slot_(slot),identity_(identity),issued_(&issued),poisoned_(poisoned){
        if(poisoned_||slot_||identity_){poisoned_=true;throw std::runtime_error("Common operation context slot is not fresh");}
        slot_=issued_;identity_=issued_;
    }
    BnwNativeCommonSourceContextSlot(const BnwNativeCommonSourceContextSlot&)=delete;
    BnwNativeCommonSourceContextSlot& operator=(const BnwNativeCommonSourceContextSlot&)=delete;
    ~BnwNativeCommonSourceContextSlot()noexcept {
        if(!complete_||slot_!=issued_||identity_!=issued_)poisoned_=true;
        slot_=nullptr;
    }
    void complete(){require_current();if(complete_){poisoned_=true;throw std::runtime_error("Common operation slot completion replay");}complete_=true;}
};

// Parent-minted, synchronous source validation only. A descriptor method may
// retain it only until its SAME Header fence has earned full source POST.
// Never retained by a leaf or published as native read/admission authority.
class BnwNativeCommonSourceValidationContext final {
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class ::civ5::ui::BnwNativeEarlyInputLease;
    friend class BnwNativeCommon13ArchiveChannel;
    friend class BnwNativeColdFilesystemOwner;
    friend class BnwNativeColdArchiveRequest;
    friend class BnwNativeOriginalStatService;
    friend class BnwNativeOriginalStatRead;
    friend class BnwNativeArchiveConstruction;
    friend class BnwNativeArchiveV6StreamHeaderOwner;
    friend class BnwNativeGlobalArchivePrefixOwner;
    friend class BnwNativeGlobalArchivePrefixRecord;
    friend class BnwNativeDefaultNamespacePolicyOwner;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeArchiveV6EntryAdmissionOwner;
    friend class BnwNativeArchiveBasenameCacheOwner;
    friend class BnwNativeArchiveCachePublicationOwner;
    using Kind=BnwNativeCommonSourceVisitLedger::Kind;
    using Work=BnwNativeCommonSourceVisitLedger::Work;
    using Visit=BnwNativeCommonSourceVisitLedger::Visit;
    const std::thread::id creator_=std::this_thread::get_id();
    const BnwNativePackageDiscoveryOwner* const parent_;
    const std::weak_ptr<const BnwNativePackageDiscoveryOwner> parent_control_;
    const BnwNativeEarlyInputLease* const early_;
    const std::weak_ptr<const BnwNativeEarlyInputLease> early_control_;
    const std::function<void()>& controller_; // Borrowed genuine Parent fence; no target copy.
    const std::function<void()> ledger_controller_;
    class AdjacentInstalledFieldIssuer final {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend class BnwNativeCommonSourceValidationContext;
        const std::thread::id creator_=std::this_thread::get_id();
        const std::function<void()>& identity_;
        explicit AdjacentInstalledFieldIssuer(const std::function<void()>& identity):identity_(identity){}
        AdjacentInstalledFieldIssuer(const AdjacentInstalledFieldIssuer&)=delete;
        AdjacentInstalledFieldIssuer& operator=(const AdjacentInstalledFieldIssuer&)=delete;
    };
    const AdjacentInstalledFieldIssuer* adjacent_installed_issuer_=nullptr;
    const std::function<void()>* adjacent_identity_controller_=nullptr; // Borrowed, no target/owner copy.
    BnwNativeCommonSourceVisitLedger ledger_; // Destroyed before its borrowed controller.

    static void need(bool value,const char* message) {
        if(!value)throw std::runtime_error(message);
    }
    void require_owned_activation_identity_without_callbacks(
        const std::weak_ptr<const BnwNativePackageDiscoveryOwner>& parent,
        const std::shared_ptr<const BnwNativeEarlyInputLease>& early)const {
        need(creator_==std::this_thread::get_id()&&parent_&&!parent_control_.expired()&&
            !parent_control_.owner_before(parent)&&!parent.owner_before(parent_control_)&&
            early&&early.get()==early_&&!early_control_.expired()&&
            !early.owner_before(early_control_)&&!early_control_.owner_before(early),
            "Activation descriptor Common Parent/Early custody changed");
        ledger_.require_owned_activation_pre_identity_without_callbacks();
    }
    void check()const{ledger_.check();}
    void require_adjacent_identity_without_callbacks()const {
        ledger_.require_adjacent_operation_without_callbacks();
        ledger_.require_condition(adjacent_installed_issuer_&&adjacent_identity_controller_&&
            adjacent_installed_issuer_->creator_==std::this_thread::get_id()&&
            adjacent_identity_controller_==&adjacent_installed_issuer_->identity_&&parent_&&early_&&
            creator_==std::this_thread::get_id()&&!parent_control_.expired()&&!early_control_.expired(),
            "Adjacent Common input/Parent/context retired");
        try{(*adjacent_identity_controller_)();}catch(...){ledger_.failed_=true;throw;}
        ledger_.require_adjacent_operation_without_callbacks();
    }
    template<class T> static constexpr Kind kind() {
        if constexpr(std::is_same_v<T,BnwNativeEarlyInputLease>)return Kind::early;
        else if constexpr(std::is_same_v<T,BnwNativeColdFilesystemOwner>)return Kind::cold;
        else if constexpr(std::is_same_v<T,BnwNativeColdArchiveRequest>)return Kind::request;
        else if constexpr(std::is_same_v<T,BnwNativeOriginalStatService>)return Kind::stat_service;
        else if constexpr(std::is_same_v<T,BnwNativeOriginalStatRead>)return Kind::stat_read;
        else if constexpr(std::is_same_v<T,BnwNativeArchiveConstruction>)return Kind::construction;
        else if constexpr(std::is_same_v<T,BnwNativeArchiveV6StreamHeaderOwner>)return Kind::header;
        else if constexpr(std::is_same_v<T,BnwNativeGlobalArchivePrefixOwner>)return Kind::prefix_registry;
        else if constexpr(std::is_same_v<T,BnwNativeGlobalArchivePrefixRecord>)return Kind::prefix_record;
        else if constexpr(std::is_same_v<T,BnwNativeDefaultNamespacePolicyOwner>)return Kind::policy;
        else if constexpr(std::is_same_v<T,BnwNativeArchiveEntryAdmissionArena>)return Kind::arena;
        else if constexpr(std::is_same_v<T,BnwNativeArchiveV6EntryAdmissionOwner>)return Kind::entries;
        else if constexpr(std::is_same_v<T,BnwNativeArchiveBasenameCacheOwner>)return Kind::cache;
        else if constexpr(std::is_same_v<T,BnwNativeArchiveCachePublicationOwner>)return Kind::publication;
        else static_assert(!sizeof(T),"Unsupported Common validation owner");
    }
    template<class T> Visit enter(const T& owner,const std::weak_ptr<const void>& control,
                                  std::uint64_t sequence=0) {
        return ledger_.enter(kind<T>(),&owner,control,sequence);
    }
    template<class T> Visit enter(const T& owner,const std::shared_ptr<const T>& canonical) {
        check();ledger_.require_condition(canonical&&canonical.get()==&owner,"Common validation requires actual canonical owner");
        return enter(owner,std::weak_ptr<const void>(canonical));
    }
    Visit enter_early(const BnwNativeEarlyInputLease& owner) {
        if(adjacent_identity_controller_) {
            check();require_adjacent_identity_without_callbacks();
            ledger_.require_condition(early_==&owner,"Common validation has foreign actual Early source");
            return ledger_.enter_after_controller(kind<BnwNativeEarlyInputLease>(),&owner,early_control_,0,
                                                  *adjacent_identity_controller_);
        }
        check();ledger_.require_condition(early_==&owner,"Common validation has foreign actual Early source");
        return enter(owner,early_control_);
    }
    template<class T> void require_borrowed(const T& owner,const std::weak_ptr<const void>& control,
                                          std::uint64_t sequence=0)const {
        ledger_.require_borrowed(kind<T>(),&owner,control,sequence);
    }
    template<class T> void require_borrowed(const T& owner,const std::shared_ptr<const T>& canonical)const {
        if(adjacent_identity_controller_) {
            check();require_adjacent_identity_without_callbacks();
            ledger_.require_condition(canonical&&canonical.get()==&owner,"Common scalar borrower has foreign canonical owner");
            ledger_.require_borrowed_after_controller(kind<T>(),&owner,std::weak_ptr<const void>(canonical));
            return;
        }
        check();ledger_.require_condition(canonical&&canonical.get()==&owner,"Common scalar borrower has foreign canonical owner");
        require_borrowed(owner,std::weak_ptr<const void>(canonical));
    }
    void require_early_borrowed(const BnwNativeEarlyInputLease& owner)const {
        if(adjacent_identity_controller_) {
            check();require_adjacent_identity_without_callbacks();
            ledger_.require_condition(early_==&owner,"Common scalar borrower has foreign Early source");
            ledger_.require_borrowed_after_controller(kind<BnwNativeEarlyInputLease>(),&owner,early_control_);
            return;
        }
        check();ledger_.require_condition(early_==&owner,"Common scalar borrower has foreign Early source");
        require_borrowed(owner,early_control_);
    }
    void require_binding(const std::weak_ptr<const BnwNativePackageDiscoveryOwner>& parent,
                         const std::shared_ptr<const BnwNativeEarlyInputLease>& early)const {
        check();ledger_.require_condition(!parent.expired()&&!parent_control_.owner_before(parent)&&!parent.owner_before(parent_control_)&&
                                        early&&early.get()==early_&&!early_control_.owner_before(early)&&!early.owner_before(early_control_),
                                        "Common operation has foreign Parent/Early source control");
        check();
    }
    explicit BnwNativeCommonSourceValidationContext(const BnwNativePackageDiscoveryOwner& parent,
                                                  const std::weak_ptr<const BnwNativePackageDiscoveryOwner>& parent_control,
                                                  const std::shared_ptr<const BnwNativeEarlyInputLease>& early,
                                                  const std::function<void()>& controller)
        :parent_(&parent),parent_control_(parent_control),early_(early.get()),early_control_(early),controller_(controller),
         ledger_controller_([this]{
             need(parent_&&creator_==std::this_thread::get_id()&&early_&&!early_control_.expired(),
                  "Common context lost genuine Parent/Early source or creator");
             // Release this transient comparison pin before the genuine
             // outside-Parent controller observes facade use_count.
             {const auto actual=parent_control_.lock();need(actual&&actual.get()==parent_,"Common context changed actual Parent control");}
             controller_();
         }),ledger_(ledger_controller_){check();}
    // Only the executing Installed descriptor owner supplies this borrowed
    // callback-free hook; old activation/general issuers keep the old route.
    explicit BnwNativeCommonSourceValidationContext(const BnwNativePackageDiscoveryOwner& parent,
        const std::weak_ptr<const BnwNativePackageDiscoveryOwner>& parent_control,
        const std::shared_ptr<const BnwNativeEarlyInputLease>& early,
        const std::function<void()>& controller,const AdjacentInstalledFieldIssuer& issuer)
        :BnwNativeCommonSourceValidationContext(parent,parent_control,early,controller) {
        ledger_.require_condition(issuer.creator_==std::this_thread::get_id()&&bool(issuer.identity_),
                                 "Adjacent Common issuer foreign or identity hook absent");
        adjacent_installed_issuer_=&issuer;
        adjacent_identity_controller_=&issuer.identity_;
        require_adjacent_identity_without_callbacks();
    }
    ~BnwNativeCommonSourceValidationContext()noexcept=default;
    BnwNativeCommonSourceValidationContext(const BnwNativeCommonSourceValidationContext&)=delete;
    BnwNativeCommonSourceValidationContext& operator=(const BnwNativeCommonSourceValidationContext&)=delete;
    void seal_pre_closure(){ledger_.seal_pre_closure();}
    void begin_post(){ledger_.begin_post();}
    void finish(){ledger_.finish();}
};
}
}
