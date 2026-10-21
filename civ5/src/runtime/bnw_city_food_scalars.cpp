#include "civ5/runtime/bnw_city_food_scalars.hpp"
#include <limits>
#include <stdexcept>
#include <string_view>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::int32_t i32(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"BNW food scalar native int32 overflow");return static_cast<std::int32_t>(n);}
saves::CityCoreField& field(saves::SavedCityObservation& city,std::string_view name){
    saves::CityCoreField* found=nullptr;
    for(auto& f:city.core.fields)if(f.name&&std::string_view(f.name)==name){need(!found,"BNW food scalar duplicate raw field");found=&f;}
    need(found&&found->encoding==saves::CityCoreEncoding::i32&&found->values.size()==1&&!found->wire_count&&found->bytes.empty()&&found->hashes.empty()&&found->row_counts.empty()&&found->row_keys.empty(),"BNW food scalar raw field shape unavailable");
    (void)i32(found->values.front());return *found;
}
saves::SavedCityObservation& city(saves::SavedSessionSnapshot& world,std::int64_t owner,std::int64_t id){
    need(world.game_state.layout==bnw&&world.all_players&&world.all_players->layout==bnw,"BNW food scalar mixed/missing world profile");
    saves::SavedCityObservation* found=nullptr;bool player_found=false;
    for(auto& p:world.all_players->players)if(p.saved_player_id==owner){
        need(!player_found&&p.cities.layout==bnw,"BNW food scalar duplicate/mixed owner");player_found=true;
        for(auto& c:p.cities.cities)if(field(c,"m_iID").values.front()==id){need(!found&&c.core.layout==bnw,"BNW food scalar duplicate/mixed city");found=&c;}
    }
    need(found&&field(*found,"m_eOwner").values.front()==owner,"BNW food scalar city absent/wrong owner");return *found;
}
BnwCityFoodScalarMirror read(saves::SavedSessionSnapshot& world,std::int64_t owner,std::int64_t id){
    auto& c=city(world,owner,id);
    return {owner,id,field(c,"m_iPopulation").values.front(),field(c,"m_iRazingTurns").values.front(),{c.core.start_offset,c.core.end_offset},i32(field(c,"m_iFood").values.front()),i32(field(c,"m_iFoodKept").values.front())};
}
}
void BnwCityFoodScalarOwner::check()const{
    // Thread first before non-atomic mirror/generation reads. No callbacks,
    // borrowed return, source facade reentry or external effect in this module.
    need(thread_==std::this_thread::get_id(),"BNW food scalar foreign thread");
    need(world_&&source_&&read(*world_,mirror_.owner,mirror_.city)==mirror_,"BNW food raw/live mirror stale");
}
BnwCityFoodScalarMirror BnwCityFoodScalarOwner::current()const{check();return mirror_;}
std::uint64_t BnwCityFoodScalarOwner::generation()const{check();return generation_;}
std::shared_ptr<const BnwCityFoodScalarOwner> import_bnw_city_food_scalars(
    const saves::SavedSessionSnapshot& source,saves::SavedSessionSnapshot& world,
    const SessionCheckpointIdentity& identity,std::int64_t owner,std::int64_t id){
    // Reuse the complete codec's SAME original roster/schema/companions and
    // i32 width checks. This imports a scalar mirror, never turn/cache authority.
    (void)make_bnw_city_core_checkpoint(world,source,identity,true);
    auto out=std::shared_ptr<BnwCityFoodScalarOwner>(new BnwCityFoodScalarOwner);
    out->mirror_=read(world,owner,id);out->world_=&world;
    out->source_=std::make_shared<const saves::SavedSessionSnapshot>(source);
    out->identity_=identity;out->check();return out;
}
BnwCityFoodScalarQuery stage_bnw_city_food_scalar_change(
    const std::shared_ptr<const BnwCityFoodScalarOwner>& owner,std::int64_t delta){
    need(bool(owner),"BNW food scalar owner absent");owner->check();
    need(owner->generation_!=UINT64_MAX,"BNW food scalar generation exhausted");
    BnwCityFoodScalarQuery query;query.owner_=owner;query.before_=owner->mirror_;query.after_=query.before_;
    query.delta_=i32(delta);query.generation_=owner->generation_;
    // Source changeFoodTimes100 then changeFoodKept(iDiff/100), NOT division
    // of the final food stock, and NOT floor division for negative fractions.
    query.after_.food_times100=i32(std::int64_t(query.before_.food_times100)+query.delta_);
    query.after_.food_kept=i32(std::int64_t(query.before_.food_kept)+query.delta_/100);
    (void)make_bnw_city_core_checkpoint(*owner->world_,*owner->source_,owner->identity_,true);
    query.baseline_=std::make_shared<const saves::SavedSessionSnapshot>(*owner->world_);
    auto trial=*query.baseline_;auto& raw=city(trial,query.before_.owner,query.before_.city);
    if(query.before_.food_times100!=query.after_.food_times100)field(raw,"m_iFood").values.front()=query.after_.food_times100;
    // Source setFoodKept unconditionally assigns, including zero delta.
    field(raw,"m_iFoodKept").values.front()=query.after_.food_kept;
    query.section_=make_bnw_city_core_checkpoint(trial,*query.baseline_,owner->identity_,true);
    owner->check();return query;
}
BnwCityFoodScalarJournal apply_bnw_city_food_scalar_change(BnwCityFoodScalarQuery&& query){
    need(!query.consumed_&&query.baseline_,"BNW food scalar query consumed/moved");
    const auto owner=query.owner_.lock();need(bool(owner),"BNW food scalar source owner expired");owner->check();
    need(owner->generation_==query.generation_&&owner->mirror_==query.before_,"BNW food scalar query generation stale");
    // Existing codec prepares ALL writes/validates destination before its
    // noexcept stores. No second field setter, partial writes or full-world swap.
    restore_bnw_city_core_checkpoint(query.section_,*owner->world_,*query.baseline_,owner->identity_);
    // No fallible callback/hash/allocation/validation follows source stores.
    owner->mirror_=query.after_;++owner->generation_;query.consumed_=true;
    return BnwCityFoodScalarJournal(query.before_,query.after_,query.delta_);
}
}
