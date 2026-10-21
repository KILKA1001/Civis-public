#pragma once
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <thread>
#include <vector>
namespace civ5::ui {class BnwNativePackageDiscoveryOwner;}
namespace civ5::ui::detail {
class BnwNativeGlobalResourceWarmOwner;
class BnwNativeStandalonePackageLoaded;
//Closed actual owned Parser occurrence, issued only by the genuine Warm
//factory roster walk. Opaque identity cannot construct or replace XML nodes.
class BnwNativeOwnedSkinOccurrence final {
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    std::weak_ptr<BnwNativeGlobalResourceWarmOwner> warm_;
    std::weak_ptr<const BnwNativeStandalonePackageLoaded> package_;
    std::weak_ptr<const void> epoch_;
    const void* document_=nullptr;
    const void* node_=nullptr;
    std::size_t factory_ordinal_=0,occurrence_ordinal_=0,roster_index_=0;
    BnwNativeOwnedSkinOccurrence()=default;
  public:
    class ConstructorLoan final {
        friend class BnwNativeGlobalResourceWarmOwner;
        const std::thread::id creator_thread_=std::this_thread::get_id();
        const std::function<void()> check_;
        const std::function<std::uint32_t(unsigned,const char*,std::uint32_t)> query_;
        const std::function<std::optional<std::string>(const char*)> attribute_;
        const std::function<std::vector<std::string>(unsigned,const char*,std::uint32_t)> output_query_;
        const void* const actual_document_;
        const void* const actual_node_;
        const std::size_t factory_,occurrence_;
        ConstructorLoan(std::function<void()> check,
            std::function<std::uint32_t(unsigned,const char*,std::uint32_t)> query,
            std::function<std::optional<std::string>(const char*)> attribute,
            std::function<std::vector<std::string>(unsigned,const char*,std::uint32_t)> output_query,
            const void* document,const void* node,std::size_t factory,std::size_t occurrence)
            :check_(std::move(check)),query_(std::move(query)),attribute_(std::move(attribute)),output_query_(std::move(output_query)),
             actual_document_(document),actual_node_(node),factory_(factory),occurrence_(occurrence){}
      public:
        void require_current()const {if(creator_thread_!=std::this_thread::get_id())throw std::runtime_error("Owned Skin stack Loan foreign creator thread");check_();}
        std::uint32_t actual_null_output_54b8d0(unsigned slot,const char* selector,std::uint32_t filter)const {
            require_current();const auto result=query_(slot,selector,filter);require_current();return result;
        }
        std::vector<std::string> actual_output_54b8d0(unsigned slot,const char* selector,std::uint32_t filter)const {
            require_current();auto result=output_query_(slot,selector,filter);require_current();return result;
        }
        std::optional<std::string> actual_ascii_attribute(const char* name)const {
            require_current();auto result=attribute_(name);require_current();return result;
        }
        const void* actual_document_identity()const {require_current();return actual_document_;}
        const void* actual_occurrence_identity()const {require_current();return actual_node_;}
        std::size_t factory_ordinal()const {require_current();return factory_;}
        std::size_t occurrence_ordinal()const {require_current();return occurrence_;}
        ConstructorLoan(const ConstructorLoan&)=delete;
        ConstructorLoan&operator=(const ConstructorLoan&)=delete;
    };
    void with_actual_constructor_loan(const std::function<void(const ConstructorLoan&)>&)const;
    void require_current()const;
    void invalidate_actual_operation()const noexcept;
    BnwNativeOwnedSkinOccurrence(const BnwNativeOwnedSkinOccurrence&)=delete;
};
static_assert(!std::is_default_constructible_v<BnwNativeOwnedSkinOccurrence>);
static_assert(!std::is_default_constructible_v<BnwNativeOwnedSkinOccurrence::ConstructorLoan>);
}
