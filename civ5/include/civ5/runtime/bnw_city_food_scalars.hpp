#pragma once
#include "civ5/runtime/bnw_city_core_checkpoint.hpp"
#include <memory>
#include <thread>
namespace civ5::runtime {
struct BnwCityFoodScalarMirror {
    std::int64_t owner=-1,city=-1,population=0,razing_turns=0;
    std::pair<std::uint64_t,std::uint64_t> source_core;
    std::int32_t food_times100=0,food_kept=0;
    std::int32_t food()const noexcept{return food_times100/100;}
    bool operator==(const BnwCityFoodScalarMirror&)const=default;
};
class BnwCityFoodScalarOwner;
class BnwCityFoodScalarQuery;
class BnwCityFoodScalarJournal;
std::shared_ptr<const BnwCityFoodScalarOwner> import_bnw_city_food_scalars(
    const saves::SavedSessionSnapshot&,saves::SavedSessionSnapshot&,
    const SessionCheckpointIdentity&,std::int64_t,std::int64_t);
// Low-level native scalar-change component, NOT doGrowth. Delta is an explicit
// setter argument. No scalar or resulting receipt grants source-turn permission.
BnwCityFoodScalarQuery stage_bnw_city_food_scalar_change(
    const std::shared_ptr<const BnwCityFoodScalarOwner>&,std::int64_t);
BnwCityFoodScalarJournal apply_bnw_city_food_scalar_change(BnwCityFoodScalarQuery&&);
class BnwCityFoodScalarOwner final {
  public:
    ~BnwCityFoodScalarOwner()=default;
    BnwCityFoodScalarOwner(const BnwCityFoodScalarOwner&)=delete;
    BnwCityFoodScalarOwner& operator=(const BnwCityFoodScalarOwner&)=delete;
    BnwCityFoodScalarMirror current()const;
    std::uint64_t generation()const;
  private:
    friend std::shared_ptr<const BnwCityFoodScalarOwner> import_bnw_city_food_scalars(
        const saves::SavedSessionSnapshot&,saves::SavedSessionSnapshot&,
        const SessionCheckpointIdentity&,std::int64_t,std::int64_t);
    friend BnwCityFoodScalarQuery stage_bnw_city_food_scalar_change(
        const std::shared_ptr<const BnwCityFoodScalarOwner>&,std::int64_t);
    friend BnwCityFoodScalarJournal apply_bnw_city_food_scalar_change(BnwCityFoodScalarQuery&&);
    BnwCityFoodScalarOwner()=default;
    void check()const;
    std::thread::id thread_=std::this_thread::get_id();
    saves::SavedSessionSnapshot* world_=nullptr;
    std::shared_ptr<const saves::SavedSessionSnapshot> source_;
    SessionCheckpointIdentity identity_;
    mutable BnwCityFoodScalarMirror mirror_;
    mutable std::uint64_t generation_=0;
};
class BnwCityFoodScalarQuery final {
  public:
    BnwCityFoodScalarQuery(const BnwCityFoodScalarQuery&)=delete;
    BnwCityFoodScalarQuery& operator=(const BnwCityFoodScalarQuery&)=delete;
    BnwCityFoodScalarQuery(BnwCityFoodScalarQuery&&)noexcept=default;
    BnwCityFoodScalarQuery& operator=(BnwCityFoodScalarQuery&&)=delete;
  private:
    friend BnwCityFoodScalarQuery stage_bnw_city_food_scalar_change(
        const std::shared_ptr<const BnwCityFoodScalarOwner>&,std::int64_t);
    friend BnwCityFoodScalarJournal apply_bnw_city_food_scalar_change(BnwCityFoodScalarQuery&&);
    BnwCityFoodScalarQuery()=default;
    std::weak_ptr<const BnwCityFoodScalarOwner> owner_;
    std::shared_ptr<const saves::SavedSessionSnapshot> baseline_;
    saves::CivisSaveSection section_;
    BnwCityFoodScalarMirror before_,after_;
    std::uint64_t generation_=0;
    std::int32_t delta_=0;
    bool consumed_=false;
};
class BnwCityFoodScalarJournal final {
  public:
    BnwCityFoodScalarJournal(const BnwCityFoodScalarJournal&)=delete;
    BnwCityFoodScalarJournal& operator=(const BnwCityFoodScalarJournal&)=delete;
    BnwCityFoodScalarJournal(BnwCityFoodScalarJournal&&)noexcept=default;
    BnwCityFoodScalarMirror before()const noexcept{return before_;}
    BnwCityFoodScalarMirror after()const noexcept{return after_;}
    std::int32_t delta_times100()const noexcept{return delta_;}
    // Journal covers two scalar assignments only, never effects/full growth.
    static constexpr bool full_growth_ready()noexcept{return false;}
    static constexpr const char* growth_frontier()noexcept{
        return "actual refreshed food yield/consumption and preceding AI-perf/starvation notification, then growth threshold/clamp/population effects";
    }
  private:
    friend BnwCityFoodScalarJournal apply_bnw_city_food_scalar_change(BnwCityFoodScalarQuery&&);
    BnwCityFoodScalarJournal(BnwCityFoodScalarMirror before,BnwCityFoodScalarMirror after,
                            std::int32_t delta)noexcept:before_(before),after_(after),delta_(delta){}
    BnwCityFoodScalarMirror before_,after_;
    std::int32_t delta_=0;
};
}
