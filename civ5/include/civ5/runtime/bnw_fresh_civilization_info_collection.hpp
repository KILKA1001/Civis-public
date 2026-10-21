#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace civ5::content {class RulesDatabase;class InfoTypeCatalog;}
namespace civ5::runtime {
class BnwFreshPregameSpeed;
struct BnwFreshOrderedInfoObservation {
 std::vector<std::string> completed_tables,source_order;
 std::string mandatory_next;
 std::uint64_t color_slots=0,player_color_slots=0,civilization_slots=0;
 bool global_cache_complete=false;
};
// Closed SAME Pregame-owned coordinator, not a stand-alone Civilization cache.
// Owns original ordered PrefetchGameData nodes; absent predecessors refuse.
class BnwFreshCivilizationInfoCollection final {
 public:
 ~BnwFreshCivilizationInfoCollection();
 BnwFreshCivilizationInfoCollection(const BnwFreshCivilizationInfoCollection&)=delete;
 BnwFreshCivilizationInfoCollection&operator=(const BnwFreshCivilizationInfoCollection&)=delete;
 BnwFreshOrderedInfoObservation observe()const;
 private:
 friend class BnwFreshPregameSpeed;
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwFreshCivilizationInfoCollection(std::shared_ptr<Impl>);
 static std::shared_ptr<BnwFreshCivilizationInfoCollection> from_same_pregame(
  const std::shared_ptr<BnwFreshPregameSpeed>&,const content::RulesDatabase&,const content::InfoTypeCatalog&);
 void advance_ordered_prefix();
};
}
