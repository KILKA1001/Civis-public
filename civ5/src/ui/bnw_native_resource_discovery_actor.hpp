#pragma once
#include "bnw_native_package_fstring_field0.hpp"
#include "bnw_native_global_color_registry_bank.hpp"
#include "bnw_native_global_resource_warm_owner.hpp"
#include <set>
namespace civ5::ui::detail {
class BnwNativePreDx9DiscoveryOwner;
// Actual callback filename transport. No public constructor, string/path
// factory, copied catalog or caller-supplied Current/ready/empty-MISS input.
// This implements Default's 82B1E0 and selected-manager Directory's 821760
// operations and 821BE0 over the SAME actual retained catalog. This does
// not authenticate absent low-warm producers or authorize global winner/MISS.
class BnwNativeResourceDiscoveryActor final {
    friend class BnwNativeInstalledDescriptorEnumeration;
    friend class BnwNativePreDx9DiscoveryOwner;
    friend class BnwNativeGlobalColorRegistryBank;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class BnwNativePackageLoadActor;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    using Bank=BnwNativeGlobalColorRegistryBank;
    using Warm=BnwNativeGlobalResourceWarmOwner;
    using Filename=BnwNativeResourceDiscoveryFilename;
    struct State {
        const std::thread::id thread=std::this_thread::get_id();
        enum class Origin {postcategory_bank,returned_common_warm,installation_pre_dx9};
        const Origin origin;
        const std::weak_ptr<Bank> bank;
        const std::weak_ptr<Warm> warm;
        const std::weak_ptr<BnwNativePreDx9DiscoveryOwner> pre_dx9;
        std::weak_ptr<const BnwNativeResourceDiscoveryActor> actor;
        std::atomic<bool> closed{false},failed{false},busy{false};
        bool default_attempted=false,global_attempted=false,installed_attempted=false;
        std::set<std::pair<const void*,std::size_t>> directory_attempts;
        std::vector<std::shared_ptr<const Filename::Source>> issued;
        explicit State(std::weak_ptr<Bank> actual):origin(Origin::postcategory_bank),bank(std::move(actual)){}
        explicit State(std::weak_ptr<Warm> actual):origin(Origin::returned_common_warm),warm(std::move(actual)){}
        explicit State(std::weak_ptr<BnwNativePreDx9DiscoveryOwner> actual)
            :origin(Origin::installation_pre_dx9),pre_dx9(std::move(actual)){}
        struct Scope {
            std::shared_ptr<State> state;
            explicit Scope(std::shared_ptr<State> value):state(std::move(value)) {
                Filename::need(state->thread==std::this_thread::get_id(),"Discovery foreign thread");
                Filename::need(!state->closed.load()&&!state->failed.load(),"Discovery actor retired");
                if(state->busy.exchange(true)){state->failed.store(true);throw std::runtime_error("Discovery reentry");}
            }
            ~Scope(){state->busy.store(false);}
        };
        void live()const {
            Filename::need(thread==std::this_thread::get_id(),"Discovery foreign thread");
            Filename::need(!closed.load()&&!failed.load()&&!actor.expired(),"Discovery actual actor expired");
        }
        std::shared_ptr<Bank> current()const;
        std::shared_ptr<Warm> current_warm()const;
        std::shared_ptr<Warm> current_warm_completed(const BnwNativeCompletedColdSourceLoan&,
            const BnwNativeColdFilesystemOwner&)const;
    };
    std::shared_ptr<State> state_;
    explicit BnwNativeResourceDiscoveryActor(const std::shared_ptr<Bank>& actual):state_(std::make_shared<State>(actual)){}
    explicit BnwNativeResourceDiscoveryActor(const std::shared_ptr<Warm>& actual):state_(std::make_shared<State>(actual)){}
    static std::shared_ptr<BnwNativeResourceDiscoveryActor> create(const std::shared_ptr<Bank>&);
    static std::shared_ptr<BnwNativeResourceDiscoveryActor> create_early(const std::shared_ptr<Warm>&);
    explicit BnwNativeResourceDiscoveryActor(const std::shared_ptr<BnwNativePreDx9DiscoveryOwner>& actual)
        :state_(std::make_shared<State>(std::weak_ptr<BnwNativePreDx9DiscoveryOwner>(actual))){}
    static std::shared_ptr<BnwNativeResourceDiscoveryActor> create_pre_dx9(
        const std::shared_ptr<BnwNativePreDx9DiscoveryOwner>&);
    struct Batch {std::vector<Filename> filenames;std::uint32_t native_count=0;};
    Batch enumerate_current_default_82b1e0()const;
    Batch enumerate_current_catalog_821be0()const; // private Package-load consumer only
    Batch enumerate_early_catalog_821be0()const;
    Batch enumerate_installed_package_catalog_821760()const;
    Batch enumerate_pre_dx9_catalog_821760()const;
    Batch enumerate_selected_directory_821760(const Bank::ResourceManager&,std::size_t root_index)const;
    static bool matches_factory_pattern(const std::string&);
public:
    ~BnwNativeResourceDiscoveryActor(){state_->closed.store(true);}
    BnwNativeResourceDiscoveryActor(const BnwNativeResourceDiscoveryActor&)=delete;
};
} //namespace civ5::ui::detail
