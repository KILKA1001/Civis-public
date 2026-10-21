#include "civ5/runtime/bnw_fresh_city_manager.hpp"
#include <utility>
namespace civ5::runtime {
BnwFreshCityManagerRefusal::BnwFreshCityManagerRefusal(BnwFreshCityManagerError code,
                                                     const char* message)
    :std::runtime_error(message),code_(code) {}
namespace {
std::shared_ptr<const BnwCreationConfig> current_config(
    std::shared_ptr<const BnwCreationConfig> config) {
    if(!config)throw BnwFreshCityManagerRefusal(BnwFreshCityManagerError::parent_unavailable,
                                              "CityManager requires actual retained Game config");
    config->require_current();return config;
}
}
struct BnwFreshCityManager::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::shared_ptr<const BnwCreationConfig> config;
    detail::BnwCityManagerPointerStorage<BnwFreshOwnedCity> storage;
    explicit Impl(std::shared_ptr<const BnwCreationConfig> c):config(current_config(std::move(c))) {}
    void current() const {
        if(std::this_thread::get_id()!=thread)
            throw BnwFreshCityManagerRefusal(BnwFreshCityManagerError::wrong_thread,
                                            "CityManager owner belongs to another thread");
        config->require_current();
    }
};
BnwFreshCityManager::BnwFreshCityManager(std::shared_ptr<const BnwCreationConfig> config)
    :impl_(std::make_unique<Impl>(std::move(config))) {}
BnwFreshCityManager::~BnwFreshCityManager()=default;
void BnwFreshCityManager::require_current() const {impl_->current();}
void BnwFreshCityManager::reset() {require_current();impl_->storage.reset_storage();}
void BnwFreshCityManager::shutdown() {require_current();impl_->storage.shutdown_storage();}
BnwFreshCityManagerObservation BnwFreshCityManager::observe() const {
    require_current();return {impl_->storage.size(),impl_->storage.empty_list_size()};
}
}
