#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace civ5::ui { class BnwNativePackageDiscoveryOwner; }
namespace civ5::ui::detail {
class BnwNativeGlobalResourceWarmOwner;
// New per-processed-slot temporaries of actual4691E0. Construction and every
// effect need distinct authority from the genuine installed global operation.
// C++ value/container effects only: no Original allocator/layout parity.
class BnwNativeInstalledGlobalSlotOwner final
    : public std::enable_shared_from_this<BnwNativeInstalledGlobalSlotOwner> {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    using Constraints=std::map<std::string,std::string,std::less<>>;
    using Indices=std::vector<std::size_t>;
    enum class Key { set,platform };
    enum class Phase { constructing,live,closing,entries_cleared,
        constraints_released,indices_cleared,closed };
    class Control {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend class BnwNativeGlobalResourceWarmOwner;
      protected:
        Control()=default;
      public:
        virtual ~Control()=default;
        Control(const Control&)=delete;
        // Bind SAME Parent/Warm/Input/Boot, closed E1, NEW physical
        // Header/Loaded/Bank operation and the currently processed slot.
        // Metadata is not construction/write/match/read/close authority.
        virtual void require_actual_scope_current(const BnwNativeInstalledGlobalSlotOwner&)const=0;
        virtual void require_actual_slot_construction_current(unsigned slot)const=0;
        virtual void require_actual_constraint_write_current(
            const BnwNativeInstalledGlobalSlotOwner&,Key,std::string_view)const=0;
        virtual void require_actual_index_reset_current(const BnwNativeInstalledGlobalSlotOwner&)const=0;
        // Authenticate the actual current matched occurrence and its dense
        // roster index; caller-supplied integers alone grant no append.
        virtual void require_actual_index_append_current(
            const BnwNativeInstalledGlobalSlotOwner&,std::size_t)const=0;
        virtual void require_actual_constraint_read_current(const BnwNativeInstalledGlobalSlotOwner&)const=0;
        virtual void require_actual_temporary_close_current(const BnwNativeInstalledGlobalSlotOwner&)const=0;
        virtual void invalidate_actual_global_operation()const noexcept=0;
    };
    class ConstraintRead final {
        friend class BnwNativeInstalledGlobalSlotOwner;
        const std::thread::id thread_=std::this_thread::get_id();
        const std::weak_ptr<BnwNativeInstalledGlobalSlotOwner> owner_;
        const std::uint64_t generation_;
        mutable bool active_=false;
        ConstraintRead(const std::shared_ptr<BnwNativeInstalledGlobalSlotOwner>& owner,std::uint64_t generation)
            :owner_(owner),generation_(generation) {}
      public:
        ConstraintRead(const ConstraintRead&)=delete;
        // A retained loan has only a weak owner and fails outside the exact
        // active read callback/generation. No copied ready table is returned.
        void with_actual_entries(const std::function<void(const Constraints&)>& input)const {
            if(thread_!=std::this_thread::get_id())throw std::runtime_error("Installed global constraint read foreign thread");
            const auto owner=owner_.lock();
            need(bool(owner),"Installed global constraint read owner expired");
            // Stale calls have no active frame-local strong loan. Reject
            // without providers BEFORE active_/raw Exit or any callback.
            owner->require_read_identity_without_callbacks(*this);
            if(active_){owner->fail();throw std::runtime_error("Installed global constraint read reentry");}
            active_=true;
            struct Exit {const ConstraintRead& loan;~Exit(){loan.active_=false;}} exit{*this};
            try {
                owner->require_read_current(*this);
                {
                    const auto consume=input;owner->require_read_current(*this);
                    need(bool(consume),"Installed global constraint read target absent");
                    consume(*owner->constraints_);owner->require_read_current(*this);
                }
                owner->require_read_current(*this); // copied target destroyed before POST
            }catch(...){owner->fail();throw;}
        }
    };
    const std::thread::id thread_=std::this_thread::get_id();
    const std::weak_ptr<const Control> control_;
    const unsigned slot_;
    std::unique_ptr<Constraints> constraints_;
    std::unique_ptr<Indices> indices_;
    Phase phase_=Phase::constructing;
    std::uint64_t generation_=0;
    const ConstraintRead* active_read_=nullptr;
    bool busy_=false,failed_=false,body_attempted_=false,body_active_=false;
    bool body_returned_=false,closed_returned_=false;
    static void need(bool value,const char* message) {
        if(!value)throw std::runtime_error(message);
    }
    void require_creator()const {
        need(thread_==std::this_thread::get_id(),"Installed global slot foreign creator thread");
    }
    std::shared_ptr<const Control> current_control()const {
        require_creator();need(!failed_,"Installed global slot failed");
        const auto control=control_.lock();need(bool(control),"Installed global slot control expired");
        return control;
    }
    void require_current()const {
        const auto control=current_control();
        const auto phase=phase_;const auto* table=constraints_.get();const auto* indices=indices_.get();
        control->require_actual_scope_current(*this);
        need(!failed_&&phase_==phase&&constraints_.get()==table&&indices_.get()==indices,
            "Installed global slot changed during currentness callback");
    }
    void fail()noexcept {
        if(thread_!=std::this_thread::get_id())return;
        failed_=true;
        if(const auto control=control_.lock())control->invalidate_actual_global_operation();
    }
    struct Activity {
        BnwNativeInstalledGlobalSlotOwner& owner;
        explicit Activity(BnwNativeInstalledGlobalSlotOwner& actual):owner(actual) {
            owner.require_creator();
            if(owner.busy_){owner.fail();throw std::runtime_error("Installed global slot effect/read reentry");}
            owner.busy_=true;
        }
        ~Activity(){owner.busy_=false;}
    };
    void require_live_body()const {
        require_current();
        need(phase_==Phase::live&&body_active_&&!body_returned_&&constraints_&&indices_,
            "Installed global slot effect outside actual slot body");
    }
    void advance_generation() {
        need(generation_!=std::numeric_limits<std::uint64_t>::max(),"Installed global slot generation exhausted");
        ++generation_;
    }
    explicit BnwNativeInstalledGlobalSlotOwner(const std::shared_ptr<const Control>& control,unsigned slot)
        :control_(control),slot_(slot) {
        require_creator();need(bool(control)&&slot<=1,"Installed global slot genuine construction absent");
        try {
            require_current();control->require_actual_slot_construction_current(slot_);require_current();
            constraints_=std::make_unique<Constraints>();require_current();
            indices_=std::make_unique<Indices>();require_current();
            control->require_actual_slot_construction_current(slot_);require_current();
            phase_=Phase::live;require_current();
        }catch(...){fail();throw;}
    }
    void set_actual_constraint_778e40(Key key,std::string_view value) {
        Activity activity(*this);
        try {
            require_live_body();const auto control=current_control();
            control->require_actual_constraint_write_current(*this,key,value);require_live_body();
            auto actual=std::string(value.substr(0,value.find('\0')));require_live_body();
            control->require_actual_constraint_write_current(*this,key,actual);require_live_body();
            constraints_->insert_or_assign(key==Key::set?"set":"platform",std::move(actual));
            advance_generation();require_live_body();
            control->require_actual_constraint_write_current(*this,key,
                constraints_->at(key==Key::set?"set":"platform"));require_live_body();
        }catch(...){fail();throw;}
    }
    // Only the actual6FA850(reset1) list reset: no constraints/enabled/pairs
    // or Consumer journal reset. Appends deliberately preserve duplicates.
    void reset_actual_indices_5e5490() {
        Activity activity(*this);
        try {
            require_live_body();const auto control=current_control();
            control->require_actual_index_reset_current(*this);require_live_body();
            indices_->clear();advance_generation();require_live_body();
            need(indices_->empty(),"Installed global actual index reset did not clear list");
            control->require_actual_index_reset_current(*this);require_live_body();
        }catch(...){fail();throw;}
    }
    void append_actual_index_6fa850(std::size_t index) {
        Activity activity(*this);
        try {
            require_live_body();const auto control=current_control();
            control->require_actual_index_append_current(*this,index);require_live_body();
            indices_->push_back(index);advance_generation();require_live_body();
            control->require_actual_index_append_current(*this,index);require_live_body();
        }catch(...){fail();throw;}
    }
    void require_actual_index_alias(const Indices& actual)const {
        require_live_body();need(indices_.get()==&actual,"Installed global substituted index list");
    }
    void require_read_identity_without_callbacks(const ConstraintRead& loan)const {
        require_creator();
        need(!failed_&&phase_==Phase::live&&body_active_&&!body_returned_&&constraints_&&indices_,
            "Installed global constraint loan outside live slot body");
        need(busy_&&active_read_==&loan&&loan.generation_==generation_&&loan.thread_==thread_&&
            loan.owner_.lock().get()==this,"Installed global constraint loan retired/replaced");
        // No Control/currentness/provider callback here. A passing active_read_
        // is anchored by the SAME strong local loan in with_actual_constraint_read.
    }
    void require_read_current(const ConstraintRead& loan)const {
        require_read_identity_without_callbacks(loan);
        require_live_body();
        const auto control=current_control();control->require_actual_constraint_read_current(*this);
        require_live_body();require_read_identity_without_callbacks(loan);
    }
    void with_actual_constraint_read(const std::function<void(const std::shared_ptr<const ConstraintRead>&)>& input) {
        Activity activity(*this);
        try {
            require_live_body();const auto storage=shared_from_this();
            const auto control=current_control();control->require_actual_constraint_read_current(*this);require_live_body();
            need(constraints_->size()==2&&constraints_->find("set")!=constraints_->end()&&
                constraints_->find("platform")!=constraints_->end(),"Installed global read lacks actual set/platform writes");
            const auto loan=std::shared_ptr<const ConstraintRead>(new ConstraintRead(storage,generation_));
            active_read_=loan.get();
            struct Exit {BnwNativeInstalledGlobalSlotOwner& owner;~Exit(){owner.active_read_=nullptr;}} exit{*this};
            require_read_current(*loan);
            {
                const auto consume=input;require_read_current(*loan);
                need(bool(consume),"Installed global constraint loan target absent");
                consume(loan);require_read_current(*loan);
            }
            require_read_current(*loan); // target destruction belongs to this read occurrence
        }catch(...){fail();throw;}
    }
    void close_actual_temporaries() {
        Activity activity(*this);
        try {
            require_current();
            need(phase_==Phase::live&&body_attempted_&&body_returned_&&!body_active_&&
                !active_read_&&constraints_&&indices_,"Installed global temporary close before actual slot return");
            const auto control=current_control();
            const auto check=[&]{require_current();control->require_actual_temporary_close_current(*this);require_current();};
            check();phase_=Phase::closing;check();
            constraints_->clear();advance_generation();phase_=Phase::entries_cleared;check();
            need(constraints_->empty(),"Installed global constraint values did not clear");
            constraints_.reset();phase_=Phase::constraints_released;check();
            indices_->clear();advance_generation();phase_=Phase::indices_cleared;check();
            need(indices_->empty(),"Installed global observation list did not clear");
            indices_.reset();phase_=Phase::closed;check();
            closed_returned_=true;require_current();
        }catch(...){fail();throw;}
    }
    // The genuine Source calls this ONCE around this slot's whole kernel body.
    // It lends the SAME owned index list; optional660 calls and their physical
    // POST must return before target destruction and real temporary teardown.
    void with_actual_slot_body(const std::function<void(Indices&)>& input) {
        require_creator();
        if(body_attempted_){fail();throw std::runtime_error("Installed global slot body replay");}
        body_attempted_=true;body_active_=true;
        struct Exit {BnwNativeInstalledGlobalSlotOwner& owner;~Exit(){owner.body_active_=false;}} exit{*this};
        const auto storage=shared_from_this();
        try {
            require_live_body();
            {
                const auto consume=input;require_live_body();
                need(bool(consume),"Installed global slot kernel body absent");
                consume(*indices_);require_live_body();
            }
            require_live_body(); // actual copied kernel target is now destroyed
            body_active_=false;body_returned_=true;
            close_actual_temporaries();require_closed_returned_current();
        }catch(...){fail();throw;}
    }
    void require_closed_returned_current()const {
        require_current();
        need(phase_==Phase::closed&&closed_returned_&&body_returned_&&!body_active_&&!busy_&&
            !active_read_&&!constraints_&&!indices_,"Installed global slot has no actual closed temporary return");
    }
  public:
    ~BnwNativeInstalledGlobalSlotOwner()=default; // no controller/I/O callbacks on destruction
    BnwNativeInstalledGlobalSlotOwner(const BnwNativeInstalledGlobalSlotOwner&)=delete;
    BnwNativeInstalledGlobalSlotOwner& operator=(const BnwNativeInstalledGlobalSlotOwner&)=delete;
};
} // namespace civ5::ui::detail
