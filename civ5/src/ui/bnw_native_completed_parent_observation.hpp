#pragma once
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
namespace civ5::assets {class ResourceMountTable;}
namespace civ5::ui {class BnwNativePackageDiscoveryOwner;class BnwNativePinnedInputEpoch;class BnwNativeUserDirectoryOwner;}
namespace civ5::ui::detail {
//Closed completed Parent/file observation only. It supplies no table access,
//native resource winner/MISS/read, GUI geometry, Rules/Lua or audio readiness.
class BnwNativeCompletedParentObservation final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    class Source;
  public:
    class Loan final {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend class ::civ5::ui::BnwNativeUserDirectoryOwner;
        const std::thread::id thread_=std::this_thread::get_id();
        const BnwNativeCompletedParentObservation& owner_;
        const std::shared_ptr<const Source> source_;
        explicit Loan(const BnwNativeCompletedParentObservation& owner)
            :owner_(owner),source_(owner.source_){}
        void require_same_directory(const ::civ5::ui::BnwNativeUserDirectoryOwner&)const;
      public:
        Loan(const Loan&)=delete;Loan&operator=(const Loan&)=delete;
        Loan(Loan&&)=delete;Loan&operator=(Loan&&)=delete;
        void require_current()const;
        void require_same_resource_table(const ::civ5::assets::ResourceMountTable&)const;
        void with_actual_pinned_input_epoch(
            const std::function<void(const ::civ5::ui::BnwNativePinnedInputEpoch&)>&)const;
    };
  private:
    class Source {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
      protected:
        Source()=default;
      public:
        virtual ~Source()=default;
        Source(const Source&)=delete;Source&operator=(const Source&)=delete;
        virtual void require_actual_owner_current(const BnwNativeCompletedParentObservation&)const=0;
        virtual void require_same_parent(const BnwNativeCompletedParentObservation&,
            const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const=0;
        virtual void with_actual_parent_loan(const BnwNativeCompletedParentObservation&,
            const std::function<void(const Loan&)>&)const=0;
        virtual void require_actual_loan_current(const BnwNativeCompletedParentObservation&,const Loan&)const=0;
        virtual void require_actual_loan_same_table(const BnwNativeCompletedParentObservation&,const Loan&,
            const ::civ5::assets::ResourceMountTable&)const=0;
        virtual void require_actual_loan_same_directory(const BnwNativeCompletedParentObservation&,const Loan&,
            const ::civ5::ui::BnwNativeUserDirectoryOwner&)const=0;
        virtual void with_actual_loan_epoch(const BnwNativeCompletedParentObservation&,const Loan&,
            const std::function<void(const ::civ5::ui::BnwNativePinnedInputEpoch&)>&)const=0;
    };
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const Source> source_;
    std::weak_ptr<const BnwNativeCompletedParentObservation> self_;
    mutable std::size_t self_pins_=0;
    mutable bool checking_=false,loan_active_=false,relation_active_=false,failed_=false;
    static void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
    explicit BnwNativeCompletedParentObservation(std::shared_ptr<const Source> source):source_(std::move(source)) {
        need(bool(source_),"Completed Parent observation actual Source absent");
    }
    class Pin final {
        const BnwNativeCompletedParentObservation& owner_;
        const std::shared_ptr<const BnwNativeCompletedParentObservation> actual_;
      public:
        explicit Pin(const BnwNativeCompletedParentObservation& owner):owner_(owner),actual_(owner.self_.lock()) {
            need(actual_&&actual_.get()==&owner_&&!actual_.owner_before(owner_.self_)&&!owner_.self_.owner_before(actual_)&&
                 owner_.self_pins_<static_cast<std::size_t>(std::numeric_limits<long>::max()),
                 "Completed Parent observation canonical control/pin overflow");
            ++owner_.self_pins_;
        }
        ~Pin(){--owner_.self_pins_;}
        void current()const {
            need(!owner_.failed_&&actual_.use_count()>static_cast<long>(owner_.self_pins_),
                 "Completed Parent observation outside owner retired");
            if(owner_.checking_){owner_.failed_=true;throw std::runtime_error("Completed Parent observation Source reentry");}
            owner_.checking_=true;
            struct Exit {const BnwNativeCompletedParentObservation& owner;~Exit(){owner.checking_=false;}} exit{owner_};
            try {
                owner_.source_->require_actual_owner_current(owner_);
                need(!owner_.failed_&&actual_.use_count()>static_cast<long>(owner_.self_pins_),
                     "Completed Parent observation retired during Source check");
            }catch(...){owner_.failed_=true;throw;}
        }
        const std::shared_ptr<const BnwNativeCompletedParentObservation>& actual()const noexcept{return actual_;}
    };
  public:
    BnwNativeCompletedParentObservation(const BnwNativeCompletedParentObservation&)=delete;
    BnwNativeCompletedParentObservation&operator=(const BnwNativeCompletedParentObservation&)=delete;
    void require_identity_current()const {
        need(thread_==std::this_thread::get_id(),"Completed Parent observation foreign creator thread");
        Pin retained(*this);retained.current();
    }
    std::shared_ptr<const BnwNativeCompletedParentObservation> retain_owner()const {
        need(thread_==std::this_thread::get_id(),"Completed Parent observation retain foreign creator thread");
        Pin retained(*this);retained.current();return retained.actual();
    }
    void require_same_parent(const ::civ5::ui::BnwNativePackageDiscoveryOwner& parent)const {
        need(thread_==std::this_thread::get_id(),"Completed Parent observation relation foreign creator thread");
        Pin retained(*this);
        if(relation_active_){failed_=true;throw std::runtime_error("Completed Parent observation relation reentry");}
        relation_active_=true;
        struct Exit {const BnwNativeCompletedParentObservation& owner;~Exit(){owner.relation_active_=false;}} exit{*this};
        try{retained.current();source_->require_same_parent(*this,parent);retained.current();}
        catch(...){failed_=true;throw;}
    }
    void with_actual_parent_loan(const std::function<void(const Loan&)>& input)const {
        need(thread_==std::this_thread::get_id(),"Completed Parent observation lender foreign creator thread");
        Pin retained(*this);
        if(loan_active_){failed_=true;throw std::runtime_error("Completed Parent observation loan reentry");}
        loan_active_=true;
        struct Exit {const BnwNativeCompletedParentObservation& owner;~Exit(){owner.loan_active_=false;}} exit{*this};
        try {
            retained.current();
            //Concrete Source opens the genuine tracked Warm Parent pin BEFORE
            //any target COPY and destroys the copied target inside that loan.
            source_->with_actual_parent_loan(*this,input);
            retained.current();
        }catch(...){failed_=true;throw;}
    }
};
inline void BnwNativeCompletedParentObservation::Loan::require_current()const {
    need(thread_==std::this_thread::get_id(),"Completed Parent loan foreign creator thread");
    try{source_->require_actual_loan_current(owner_,*this);}catch(...){owner_.failed_=true;throw;}
}
inline void BnwNativeCompletedParentObservation::Loan::require_same_resource_table(
    const ::civ5::assets::ResourceMountTable& table)const {
    need(thread_==std::this_thread::get_id(),"Completed Parent table relation foreign creator thread");
    try{source_->require_actual_loan_same_table(owner_,*this,table);}catch(...){owner_.failed_=true;throw;}
}
inline void BnwNativeCompletedParentObservation::Loan::require_same_directory(
    const ::civ5::ui::BnwNativeUserDirectoryOwner& directory)const {
    need(thread_==std::this_thread::get_id(),"Completed Parent Directory relation foreign creator thread");
    try{source_->require_actual_loan_same_directory(owner_,*this,directory);}catch(...){owner_.failed_=true;throw;}
}
inline void BnwNativeCompletedParentObservation::Loan::with_actual_pinned_input_epoch(
    const std::function<void(const ::civ5::ui::BnwNativePinnedInputEpoch&)>& input)const {
    need(thread_==std::this_thread::get_id(),"Completed Parent epoch lender foreign creator thread");
    try{source_->with_actual_loan_epoch(owner_,*this,input);}catch(...){owner_.failed_=true;throw;}
}
} // namespace civ5::ui::detail
