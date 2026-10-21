#pragma once
#include "civ5/runtime/live_session.hpp"
#include <map>

namespace civ5::runtime {
struct BnwBusySourceFile {std::string_view path,sha256;};
inline constexpr std::array<BnwBusySourceFile,8> kBnwBusySourceFiles{{
    {"CvGameCoreDLL_Expansion2/CvUnit.cpp","646e6c26ffb5ca20b77320d1bb7d78af440072eec210a760c2c76275f5e80b82"},
    {"CvGameCoreDLL_Expansion2/CvPlayer.cpp","9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d"},
    {"CvGameCoreDLL_Expansion2/CvCity.cpp","15e14918392e0f8e9de7e37758a659ed0bf7a571144daa57e83b1b999a548fee"},
    {"CvGameCoreDLL_Expansion2/CvMap.h","e56682f64e01d62db85d683d10991230df3be628842763ca2fb2f0643083a0ac"},
    {"CvGameCoreDLL_Expansion2/CvGameCoreUtils.cpp","dbbc26165c1c15d21c1b602ff363f58d15c276440f83fe15b53bfdf97a515803"},
    {"CvGameCoreDLLUtil/include/CvStructs.h","8586aa53e6c1f8d24ad4a6b2886b9715ec427d77dfdf3c69ff69ef394242324f"},
    {"FirePlace/include/FireWorks/FFreeListTrashArray.h","957dd24055204895ab479f3a1577113c60f1a18303a9bb4cf9c5a4b716c507f8"},
    {"CvGameCoreDLLUtil/include/CvDLLUtilDefines.h","87fa1d6ee2724f4f215537ce50f9ce1fd9a9c5b2773fd9a1601396b7bee0a1db"}
}};
using BnwBusyObjectKey=std::pair<std::int64_t,std::int64_t>; // owner, full ID
struct BnwBusyReference {std::int64_t owner=-1,id=-1;};
struct BnwBusyUnitTransient {
    std::int64_t mission_timer=0;
    BnwBusyReference combat_city;
};
// The omitted BNW runtime fields have an actual fresh-import lifecycle:
// unit reset sets MissionTimer=0; IDInfo constructors default combat links.
// They must be retained thereafter. Continuations cannot infer these defaults.
class BnwBusyStateOwner {
  public:
    const SessionCheckpointIdentity &identity() const noexcept {return identity_;}
    const std::array<std::uint8_t,16> &map_guid() const noexcept {return map_guid_;}
    const auto &units() const noexcept {return units_;}
    const auto &cities() const noexcept {return cities_;}
    void set_unit_mission_timer(BnwBusyObjectKey,std::int64_t);
    void set_unit_combat_city(BnwBusyObjectKey,BnwBusyReference);
    void set_city_combat_unit(BnwBusyObjectKey,BnwBusyReference);
  private:
    friend struct BnwUiTransientCheckpointAccess;
    friend BnwBusyStateOwner bind_bnw_busy_state_original_import(
        const saves::SavedSession &,const SessionCheckpointIdentity &);
    BnwBusyStateOwner(SessionCheckpointIdentity identity,std::array<std::uint8_t,16> guid)
        :identity_(std::move(identity)),map_guid_(guid) {}
    SessionCheckpointIdentity identity_;
    std::array<std::uint8_t,16> map_guid_{};
    std::map<BnwBusyObjectKey,BnwBusyUnitTransient> units_;
    std::map<BnwBusyObjectKey,BnwBusyReference> cities_;
};
BnwBusyStateOwner bind_bnw_busy_state_original_import(
    const saves::SavedSession &,const SessionCheckpointIdentity &);
struct BnwBusyPlayerObservation {
    std::vector<std::int64_t> busy_units,busy_cities;
    bool busy() const noexcept {return !busy_units.empty() || !busy_cities.empty();}
};
// Exact IsBusy predicates over a complete owned roster, including the native
// FFreeListTrashArray generation/slot lookup. No AutoMission or combat executes.
// Unknown transient owners and added/deleted objects require explicit lifecycle
// integration. Out-of-map nonsentinel attack coordinates are rejected, never
// wrapped into a safe plot (plotCheckInvalid does not wrap).
BnwBusyPlayerObservation observe_bnw_busy_player(
    const LiveSessionState &,const BnwBusyStateOwner &,
    const SessionCheckpointIdentity &,std::int64_t player);
} // namespace civ5::runtime
