#pragma once
#include "civ5/runtime/bnw_creation_config.hpp"
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace civ5::runtime {
class BnwFreshGame;
class BnwFreshOwnedCity; // Future genuine allocator owns these objects.
enum class BnwFreshCityManagerError { wrong_thread, parent_unavailable };
class BnwFreshCityManagerRefusal : public std::runtime_error {
 public:
    BnwFreshCityManagerRefusal(BnwFreshCityManagerError,const char*);
    BnwFreshCityManagerError code() const noexcept { return code_; }
 private: BnwFreshCityManagerError code_;
};
namespace detail {
// A shared storage algorithm, not an issuing city/realm interface. Detached
// tests instantiate it with ordinary local objects, never imported city IDs.
// Native map order uses pointer total ordering; no ID ordering or tie repair.
template<class City> class BnwCityManagerPointerStorage {
 public:
    using CityList=std::vector<City*>;
    using CityMap=std::map<City*,CityList,std::less<City*>>;
    BnwCityManagerPointerStorage()=default;
    BnwCityManagerPointerStorage(const BnwCityManagerPointerStorage&)=delete;
    BnwCityManagerPointerStorage& operator=(const BnwCityManagerPointerStorage&)=delete;
    // Detached storage access cannot produce a BnwFreshOwnedCity allocation,
    // genuine owner, Game phase, city-created call, or readiness receipt.
    CityMap& detached_map() { require_thread();return cities_; }
    CityList& detached_empty_list() { require_thread();return empty_; }
    std::size_t size() const { require_thread();return cities_.size(); }
    std::size_t empty_list_size() const { require_thread();return empty_.size(); }
    void reset_storage() { require_thread();cities_.clear(); }
    void shutdown_storage() { require_thread();cities_.clear(); }
 private:
    void require_thread() const {
        if(std::this_thread::get_id()!=thread_)
            throw BnwFreshCityManagerRefusal(BnwFreshCityManagerError::wrong_thread,
                                            "CityManager storage belongs to another thread");
    }
    const std::thread::id thread_=std::this_thread::get_id();
    CityMap cities_;
    CityList empty_; // Native Reset/Shutdown do not clear this separate list.
};
}
struct BnwFreshCityManagerObservation { std::size_t city_entries,empty_list_entries; };
// No public factory/mutator. Only the future real Game facade can allocate,
// retain and reset this realm. An external observation handle must alias the
// SAME GameImpl allocation; this owner never holds Game/Host backedges.
class BnwFreshCityManager {
 public:
    ~BnwFreshCityManager();
    BnwFreshCityManager(const BnwFreshCityManager&)=delete;
    BnwFreshCityManager& operator=(const BnwFreshCityManager&)=delete;
    BnwFreshCityManagerObservation observe() const;
 private:
    friend class BnwFreshGame;
    explicit BnwFreshCityManager(std::shared_ptr<const BnwCreationConfig>);
    void require_current() const;
    void reset();
    void shutdown();
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
