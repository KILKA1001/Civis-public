#pragma once
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>

namespace civ5::assets {class ResourceMountTable;}
namespace civ5::ui {class BnwNativePackageDiscoveryOwner;class BnwNativePinnedInputEpoch;}
namespace civ5::ui::detail {
class BnwNativeGlobalResourceWarmOwner;
// The real Parent alone issues this owner from its actual returned full Skin
// kernel, final Bank receiver and SAME resource storage. File input provenance
// and an executed resource query loan remain separate capabilities.
class BnwNativeCompletedResourceBankCapability final
    :public std::enable_shared_from_this<BnwNativeCompletedResourceBankCapability> {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    class Source {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend class BnwNativeGlobalResourceWarmOwner;
      protected:
        Source()=default;
      public:
        virtual ~Source()=default;
        Source(const Source&)=delete;
        virtual void require_same_completed_owner_current(const BnwNativeCompletedResourceBankCapability&)const=0;
        virtual void with_actual_resource_query_loan(const BnwNativeCompletedResourceBankCapability&,
            const std::function<void(const assets::ResourceMountTable&)>&)const=0;
    };
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const Source> source_;
    const std::shared_ptr<const BnwNativePinnedInputEpoch> input_epoch_;
    explicit BnwNativeCompletedResourceBankCapability(std::shared_ptr<const Source> source,
        std::shared_ptr<const BnwNativePinnedInputEpoch> epoch):source_(std::move(source)),input_epoch_(std::move(epoch)) {
        if(!source_||!input_epoch_)throw std::runtime_error("Completed resource owner lacks actual source/file epoch");
    }
  public:
    BnwNativeCompletedResourceBankCapability(const BnwNativeCompletedResourceBankCapability&)=delete;
    BnwNativeCompletedResourceBankCapability&operator=(const BnwNativeCompletedResourceBankCapability&)=delete;
    // This checks closed owner/storage/file provenance. It grants no native
    // winner, MISS or stream credit; those require the distinct real loan.
    void require_identity_current()const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Completed resource owner foreign creator thread");
        const auto storage=shared_from_this();source_->require_same_completed_owner_current(*this);
    }
    void with_actual_pinned_input_epoch(
        const std::function<void(const std::shared_ptr<const BnwNativePinnedInputEpoch>&)>& input)const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Completed input borrower foreign creator thread");
        const auto storage=shared_from_this();require_identity_current();
        {const auto consume=input;require_identity_current();
            if(!consume)throw std::runtime_error("Completed input epoch consumer absent");
            consume(input_epoch_);require_identity_current();
        }require_identity_current();
    }
    void with_actual_resource_query_loan(const std::function<void(const assets::ResourceMountTable&)>& input)const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Completed resource query foreign creator thread");
        const auto storage=shared_from_this();require_identity_current();
        {const auto consume=input;require_identity_current();
            if(!consume)throw std::runtime_error("Completed resource query consumer absent");
            source_->with_actual_resource_query_loan(*this,consume);require_identity_current();
        }require_identity_current();
    }
};
}
