#include "civ5/runtime/bnw_fresh_city_manager.hpp"
#include <atomic>
#include <iostream>
#include <thread>
#include <type_traits>
namespace {
using namespace civ5::runtime;
void need(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
struct DetachedCity {std::int32_t marker;};
using Storage=detail::BnwCityManagerPointerStorage<DetachedCity>;
static_assert(!std::is_constructible_v<BnwFreshCityManager,std::shared_ptr<const BnwCreationConfig>>);
static_assert(!std::is_copy_constructible_v<BnwFreshCityManager>);
void verify_nonempty_reset() {
    DetachedCity a{91},b{92},c{93};
    Storage storage;
    need(storage.size()==0 && storage.empty_list_size()==0,"Native static containers not initially empty");
    auto& map=storage.detached_map();
    map.emplace(&a,Storage::CityList{&c,&b,&c});
    map.emplace(&b,Storage::CityList{&a});
    // Independent pointer-order oracle; no integer-ID/pointer reinterpretation.
    const auto first=std::less<DetachedCity*>{}(&a,&b)?&a:&b;
    need(map.begin()->first==first,"Map did not use actual object-pointer ordering");
    need(map.at(&a).size()==3 && map.at(&a)[0]==&c && map.at(&a)[1]==&b && map.at(&a)[2]==&c,
         "Storage reordered nearby list/ties or removed duplicate pointers");
    // Deliberately detached nonempty fallback proves clear touches only map.
    storage.detached_empty_list().push_back(&c);
    storage.reset_storage();
    need(storage.size()==0 && storage.empty_list_size()==1,"Reset did not clear only native city map");
    need(a.marker==91 && b.marker==92 && c.marker==93,"Reset modified pointed-to city objects");
    map.emplace(&c,Storage::CityList{&a,&b});
    storage.shutdown_storage();
    need(storage.size()==0 && storage.empty_list_size()==1,"Shutdown differs from native map clear");
    storage.reset_storage();need(storage.size()==0,"Empty reset is not idempotent storage clear");
}
void verify_thread_and_storage_lifetime() {
    DetachedCity a{7},b{8};auto storage=std::make_shared<Storage>();
    storage->detached_map().emplace(&a,Storage::CityList{&b});
    auto held=storage;std::weak_ptr<Storage> weak=storage;storage.reset();
    need(!weak.expired() && held->size()==1,"Detached retained storage lost its allocation");
    std::atomic<bool> refused{false};
    std::thread other([held,&refused] {
        try{held->reset_storage();}
        catch(const BnwFreshCityManagerRefusal& e) {refused=e.code()==BnwFreshCityManagerError::wrong_thread;}
    });other.join();
    need(refused.load() && held->size()==1,"Foreign thread reset changed map");
    held->reset_storage();need(held->size()==0 && a.marker==7 && b.marker==8,"Same thread reset failed");
    held.reset();need(weak.expired(),"Detached cache retained a parent cycle");
}
}
int main() {
    try {
        verify_nonempty_reset();verify_thread_and_storage_lifetime();
        std::cout<<"PASS detached native CityManager pointer-map Reset/Shutdown; genuine Game issuance UNOBSERVED\n";
        return 0;
    }catch(const std::exception& e) {
        std::cerr<<"FAIL CityManager storage: "<<e.what()<<'\n';return 1;
    }
}
