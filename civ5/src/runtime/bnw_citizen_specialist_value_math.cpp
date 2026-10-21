#include "civ5/runtime/bnw_citizen_specialist_value_math.hpp"
#include "civ5/runtime/bnw_citizen_plot_value_math.hpp"
#include <climits>
#include <stdexcept>
namespace civ5::runtime::detail {
namespace {std::int32_t native(std::int64_t n){if(n<INT32_MIN||n>INT32_MAX)throw std::invalid_argument("Specialist native32 intermediate unavailable");return static_cast<std::int32_t>(n);}}
std::int32_t bnw_specialist_focus_yield(std::int32_t f)noexcept{
 switch(f){case 0:case 6:case 7:return 0;case 1:return 1;case 2:return 2;case 4:return 3;case 5:return 4;case 8:return 5;default:return -1;}
}
std::int32_t bnw_specialist_weighted_yield(std::int32_t w,std::int32_t y){return native(std::int64_t(w)*y);}
std::int32_t bnw_specialist_total(std::array<std::int32_t,6> v,bool avoid,std::int32_t focus,
 std::int32_t pop,std::int32_t gp,std::int32_t happiness){
 const auto base=bnw_citizen_plot_value_total(v,avoid,focus,pop);
 return native(std::int64_t(native(std::int64_t(base)+gp))+happiness);
}
BnwSpecialistBuildingValue bnw_specialist_building_value(std::int32_t allowed,std::int32_t value){
 const auto a=native(std::int64_t(allowed)-1);
 const auto b=native(std::int64_t(a)*value);
 const auto c=native(std::int64_t(b)*10);
 const auto temp=c/100;const auto adjusted=native(std::int64_t(value)+temp);
 return {adjusted,native(std::int64_t(adjusted)-temp)};
}
saves::TeamHashedArray bnw_specialist_known_hash_projection_math(
 const saves::TeamHashedArray& current,std::uint32_t hash,std::int32_t next){
 if(hash==0||current.wire_count<0||static_cast<std::uint64_t>(current.wire_count)!=current.rows.size()||
  current.rows.size()>UINT32_MAX)throw std::invalid_argument("Specialist native hash projection unavailable");
 auto out=current;saves::TeamHashedValues* last=nullptr;
 for(auto& row:out.rows){
  if(row.type_hash>UINT32_MAX||(row.type_hash==0?!row.values.empty():row.values.size()!=1))
   throw std::invalid_argument("Specialist source hash row shape unavailable");
  if(row.type_hash){(void)native(row.values.front());if(row.type_hash==hash)last=&row;}
 }
 if(last)last->values.front()=next;
 else{
  if(out.rows.size()==UINT32_MAX)throw std::invalid_argument("Specialist native hash wire count exhausted");
  out.rows.push_back({hash,{next}});out.wire_count=static_cast<std::int64_t>(out.rows.size());
 }
 return out;
}
}
