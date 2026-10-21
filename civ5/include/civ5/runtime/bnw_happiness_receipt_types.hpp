#pragma once
#include <cstdint>
#include <optional>

namespace civ5::runtime {
struct BnwHappinessNativeStore {
    enum class Stage {handicap,resources,cities,buildings,policies,per_city,religion};
    Stage stage=Stage::handicap;
    std::int32_t operand=0,happiness=0;
};

struct BnwReligionHappinessTerms {
    bool at_peace=false;
    std::int32_t player_happiness=0,following_cities=0,following_happiness=0;
    float per_following_city=0;
    std::int32_t peaceful_foreign_divisor=0,peaceful_foreign_followers=0,foreign_happiness=0,total=0;
    std::optional<std::int32_t> foreign_founder_religion;
    std::uint64_t founder_callback_calls=1;
};

enum class BnwHappinessSuffixStage {wonders,minors,leagues,connections_cache,connections_add,excess,achievement_request,game_data_receiver};
struct BnwHappinessSuffixStore {BnwHappinessSuffixStage stage;std::int32_t operand=0,happiness=0;};
}
