#pragma once
#include "civ5/runtime/bnw_creation_config.hpp"
#include <thread>
namespace civ5::runtime {
class BnwFreshGame;
struct BnwNewGameTurnIncrement {
    std::int32_t months=0,turns=0;
};
struct BnwNewGameVictoryInitRule {
    BnwCreationInfoBinding identity;
    bool wins_game=false,end_score=false;
};
struct BnwNewGameSetupRuleObservation {
    BnwCreationMetadata selected;
    std::vector<BnwNewGameTurnIncrement> turns;
    std::vector<std::optional<BnwNewGameVictoryInitRule>> victories;
    std::int32_t start_percent=0,start_year=0;
    bool no_religion=false;
};
// Gameplay rules for Civis' own initialized Game. This is not a grant of
// completed original Globals/Skin, initialized players, or a playable session.
// The factory authenticates real activated Content through the SAME Config.
class BnwNewGameSetupRules final {
 public:
    static std::shared_ptr<const BnwNewGameSetupRules> from_creation_config(
        std::shared_ptr<const BnwCreationConfig>);
    void require_current()const;
    BnwNewGameSetupRuleObservation observe()const;
    BnwNewGameSetupRules(const BnwNewGameSetupRules&)=delete;
 private:
    friend class BnwFreshGame;
    explicit BnwNewGameSetupRules(std::shared_ptr<const BnwCreationConfig>);
    void require_same_config(const BnwCreationConfig&)const;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const BnwCreationConfig> config_;
    const std::shared_ptr<const BnwFreshGameContentResult> content_;
    const content::RulesDatabase* const database_;
    BnwNewGameSetupRuleObservation rules_;
};
}
