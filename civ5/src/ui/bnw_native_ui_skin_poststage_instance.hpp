#pragma once
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "bnw_native_ui_skin_raw_text.hpp"
#include "bnw_native_global_color_registry_bank.hpp"
#include "bnw_native_mounted_style_reader.hpp"
#include <bit>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#include <utility>
#include <limits>

namespace civ5::ui::native_ui_skin_query_detail {
// Data kernel only, not a Package/operation/instance issuer.5EB290's ordinary
// null-output route constructs local FString text before the null-output test.
inline std::uint32_t ordinary_null_query(std::string_view retained_text,std::uint32_t filter) {
    if(filter!=0 && filter!=0x80)throw std::runtime_error("Unsupported native null-query filter");
    const auto stop=retained_text.find('\0');
    const auto prefix=retained_text.substr(0,stop);
    if(prefix.size()>std::size_t(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("Native null-query signed FString length unsupported");
    // This allocation/copy must finish normally before a private completion
    // can be recorded. No allocator/abnormalfailure timing parity is claimed.
    const std::string local(prefix);
    return local.empty()?0U:1U;
}
constexpr std::uint32_t completed_entry_count(std::uint32_t prior)noexcept{return prior+1U;}
constexpr bool available(std::uint32_t directory,std::uint32_t paks,std::uint32_t file)noexcept {
    return std::bit_cast<std::int32_t>(directory+paks+file)>0;
}
}

namespace civ5::ui::detail {
class BnwNativeGlobalResourceWarmOwner;
class BnwNativeOwnedSkinConsumer;
class BnwNativeOwnedSkinOccurrence;
class BnwNativeUiSkinPoststageQueryLease final {
  public:
    ~BnwNativeUiSkinPoststageQueryLease()=default;
    BnwNativeUiSkinPoststageQueryLease(const BnwNativeUiSkinPoststageQueryLease&)=delete;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeUiSkinPoststageInstance;
    friend class BnwNativeGlobalColorRegistryBank;
    struct Completed {
        unsigned slot,selector;
        std::uint32_t filter,ordinary_result;
        Completed(unsigned s,unsigned q,std::uint32_t f,std::uint32_t r):slot(s),selector(q),filter(f),ordinary_result(r){}
    };
    std::weak_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> parent_;
    std::shared_ptr<const ::civ5::ui::BnwNativePackageCategorySourceOwner> source_;
    std::shared_ptr<const ::civ5::ui::BnwNativePackagePreSkinOwner> predecessor_;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease_;
    std::shared_ptr<const BnwNativeUiSkinRetainedOccurrence> occurrence_;
    std::shared_ptr<const BnwNativeUiSkinSelectedSourceOwner> raw_;
    std::shared_ptr<const void> epoch_,channel_;
    std::vector<Completed> completed_;
    std::array<std::array<std::uint32_t,3>,2> counts_{};
    std::uint32_t flags_mask_,flags_value_,category_mask_;
    const content::XmlNode* document_;
    const content::XmlNode* node_;
    std::size_t descriptor_,ordinal_,channel_index_;
    bool row_category_closed_;
    const std::thread::id thread_=std::this_thread::get_id();
    BnwNativeUiSkinPoststageQueryLease()=default;
};
// Genuine798CE0 input-constructor phase only. The actual issued query lease
// initializes two availability slots and enabled0. The SAME private bank attaches
// actual empty Color/ColorSet constructor fields; Skin load/namespace/enable
// effects remain unavailable and cannot be inferred from these empty fields.
class BnwNativeUiSkinPoststageInstance final {
  public:
    ~BnwNativeUiSkinPoststageInstance()=default;
    BnwNativeUiSkinPoststageInstance(const BnwNativeUiSkinPoststageInstance&)=delete;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalColorRegistryBank;
    std::shared_ptr<const BnwNativeUiSkinPoststageQueryLease> query_;
    mutable std::array<std::shared_ptr<BnwNativeColorSlotStorage>,2> color_storage_;
    const std::array<bool,2> availability_;
    const std::array<std::uint8_t,2> enabled_{0,0};
    explicit BnwNativeUiSkinPoststageInstance(std::shared_ptr<const BnwNativeUiSkinPoststageQueryLease> query)
        :query_(std::move(query)),availability_{
          native_ui_skin_query_detail::available(query_->counts_[0][0],query_->counts_[0][1],query_->counts_[0][2]),
          native_ui_skin_query_detail::available(query_->counts_[1][0],query_->counts_[1][1],query_->counts_[1][2])}{}
};

// Separate actual798CE0 constructor domain. Its Source must be issued under
// the genuine terminal Warm/Loaded/Parser loan; category762400 count alone
// cannot construct it or enable resources. Legacy content-DOM leases above
// retain their existing semantics.
class BnwNativeCompletedSkinInstanceObservation;
class BnwNativeOwnedUiSkinPoststageInstance final {
    friend class BnwNativeCompletedSkinInstanceObservation;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativeGlobalColorRegistryBank;
    friend class BnwNativeOwnedSkinConsumer;
    class ConstructorSource {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        friend class BnwNativeGlobalResourceWarmOwner;
      protected:
        ConstructorSource()=default;
      public:
        virtual ~ConstructorSource()=default;
        ConstructorSource(const ConstructorSource&)=delete;
        // Validates SAME real returned Loaded membership, held ParserDoc /
        // exact UISkin sibling ordinal, Parent/epoch and live operation fence.
        virtual void require_current()const=0;
        // SAME closed factory/nthUISkin token; Warm checks identity, not counts.
        virtual const BnwNativeOwnedSkinOccurrence& actual_occurrence()const=0;
        virtual std::uint32_t actual_null_output_54b8d0(unsigned slot,
            const char* selector,std::uint32_t filter)const=0;
        virtual void invalidate_actual_operation()const noexcept=0;
    };
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const ConstructorSource> constructor_source_;
    std::array<bool,2> availability_{};
    mutable std::array<std::uint8_t,2> enabled_{0,0};
    //Actual798CE0 two NULL manager pairs. Only genuine Warm/Parent effects
    //can install/write these; PackageCore category_slots[2] stays separate.
    mutable std::array<std::array<BnwNativeSkinDocumentRegistration::DocumentHandle,2>,2> owned_resource_pairs_{};
    //48C800: genuine54B8D0 Skin/Directory extra-output list is produced
    //before798CE0, then transferred here after its normal return and BEFORE
    //Warm strong roster append. Only the private Parent issuer writes it.
    //An empty native list is permitted; this journal is never caller readiness.
    mutable std::vector<std::pair<std::string,std::string>> owned_extra_attributes_;
    mutable bool owned_extra_attributes_issued_=false;
    mutable std::array<std::shared_ptr<BnwNativeColorSlotStorage>,2> color_storage_;
    struct StyleField {
        std::shared_ptr<BnwNativeMountedStyleReader> reader;
        BnwNativeMountedStyleReader::BorrowedNode destination{}; //actual835540 node+4 NULL
    };
    mutable std::array<StyleField,2> style_storage_;
    explicit BnwNativeOwnedUiSkinPoststageInstance(std::shared_ptr<const ConstructorSource> source)
        :constructor_source_(std::move(source)) {
        if(!constructor_source_)throw std::runtime_error("Owned Skin constructor has no actual Loaded issuer");
        try {
            require_current();
            for(unsigned slot=0;slot<2;++slot) {
                const auto directories=constructor_source_->actual_null_output_54b8d0(slot,"Directory",0);
                require_current();
                const auto paks=constructor_source_->actual_null_output_54b8d0(slot,"Paks",0);
                require_current();
                const auto files=constructor_source_->actual_null_output_54b8d0(slot,"File",0x80);
                require_current();
                // Preserve DWORD wrap and signed SETG. None of these queries
                // reads resources, issues a manager or enables either slot.
                availability_[slot]=native_ui_skin_query_detail::available(directories,paks,files);
            }
            require_current();
        }catch(...){constructor_source_->invalidate_actual_operation();throw;}
    }
    void require_current()const {
        if(thread_!=std::this_thread::get_id())throw std::runtime_error("Owned Skin instance foreign thread");
        constructor_source_->require_current();
    }
  public:
    BnwNativeOwnedUiSkinPoststageInstance(const BnwNativeOwnedUiSkinPoststageInstance&)=delete;
};
static_assert(!std::is_default_constructible_v<BnwNativeOwnedUiSkinPoststageInstance>);

static_assert(!std::is_default_constructible_v<BnwNativeUiSkinPoststageQueryLease>);
static_assert(!std::is_default_constructible_v<BnwNativeUiSkinPoststageInstance>);
static_assert(!std::is_copy_constructible_v<BnwNativeUiSkinPoststageQueryLease>);
}
