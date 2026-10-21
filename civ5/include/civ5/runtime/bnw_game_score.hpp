namespace civ5::ui {class BnwEngineUiTurnOwner;}
#pragma once

#include "civ5/runtime/live_session.hpp"
#include <memory>

namespace civ5::runtime {

class BnwRetainedTurnCursor;
// Internal prepared scalar/replay values, never an externally issuable completion.
// The cursor obtains all rules from its SAME actual Host. No cloned live world.
class BnwGameScorePreparedWrites final {
    friend class BnwRetainedTurnCursor;
    friend class ui::BnwEngineUiTurnOwner;
    static std::int32_t read_player_score(const LiveSessionState&,const content::RulesDatabase&,const content::InfoTypeCatalog&,const content::GlobalDefineCache&,std::int32_t);
    friend void update_bnw_game_score(LiveSessionState&,
        const content::RulesDatabase&,const content::InfoTypeCatalog&,
        const content::GlobalDefineCache&);
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwGameScorePreparedWrites(std::unique_ptr<Impl>) noexcept;
    static std::unique_ptr<BnwGameScorePreparedWrites> prepare(
        const LiveSessionState&,const content::RulesDatabase&,
        const content::InfoTypeCatalog&,const content::GlobalDefineCache&);
    bool dirty()const noexcept;
    std::size_t changed_player_scores()const noexcept;
    void require_same_state(const LiveSessionState&)const;
    void clear_dirty(LiveSessionState&)noexcept;
    bool store_player_score(LiveSessionState&,std::size_t rank)noexcept;
    void store_player_replay(LiveSessionState&,std::size_t rank)noexcept;
    void store_team_scores(LiveSessionState&)noexcept;
 public:
    ~BnwGameScorePreparedWrites();
    BnwGameScorePreparedWrites(const BnwGameScorePreparedWrites&)=delete;
    BnwGameScorePreparedWrites& operator=(const BnwGameScorePreparedWrites&)=delete;
};

// CvGame::updateScore(false), including all 63 civilization player and team
// rankings and REPLAYDATASET_SCORE for the current turn. Requires a complete
// BNW world and selected rules. Throws before mutating state if any required
// score input is unresolved.
void update_bnw_game_score(LiveSessionState &state,
                           const content::RulesDatabase &rules,
                           const content::InfoTypeCatalog &types,
                           const content::GlobalDefineCache &defines);

} // namespace civ5::runtime
