#pragma once
#include <cstdint>
#include <memory>
#include <utility>
namespace civ5::graphics {class BnwUnitDamageCommandOwner;}
namespace civ5::runtime {
class BnwAttritionDamageDllWrapperPrefix;class BnwNativeUnitAllocatorCore;class BnwNativeAttritionPrefix;
class BnwUnitFlagHealthInstanceLoan;struct LiveSessionState;
// Actual after-HP alias custody; it does not issue a Source/Scene queue loan.
class BnwUnitFlagLivingHealthRead final {
 friend class BnwAttritionDamageDllWrapperPrefix;
 friend class BnwUnitFlagHealthInstanceLoan;
 friend class graphics::BnwUnitDamageCommandOwner;
 struct State;const std::shared_ptr<State>state_;
 explicit BnwUnitFlagLivingHealthRead(std::shared_ptr<State>);
 void require_current_without_callbacks()const;
 void require_same_world_without_callbacks(const LiveSessionState*,std::uint64_t)const;
 std::pair<std::int32_t,std::int32_t>owner_full_id_without_callbacks()const;
 std::int32_t damage_without_callbacks()const;
 std::int32_t max_hit_points_without_callbacks()const;
 public:
 ~BnwUnitFlagLivingHealthRead();
 BnwUnitFlagLivingHealthRead(const BnwUnitFlagLivingHealthRead&)=delete;
 BnwUnitFlagLivingHealthRead&operator=(const BnwUnitFlagLivingHealthRead&)=delete;
};
}
