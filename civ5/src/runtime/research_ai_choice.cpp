#include "civ5/runtime/research_ai_candidates.hpp"
#include "civ5/runtime/vp_research_ai_choice.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
std::int64_t research_ai_handicap_setting(const ResearchAiChoiceContext &c,std::string_view column) {
    if(c.selection.setup.game_handicap<0)throw ResearchAiUnavailable("Research AI game handicap unavailable");
    std::optional<std::int64_t> value;
    const auto query=c.rules.query_rows({"SELECT "+std::string(column)+" FROM HandicapInfos WHERE ID=?",
        {c.selection.setup.game_handicap}},[&](const auto &row){
        if(value)throw ResearchAiUnavailable("Duplicate research AI handicap");
        value=row.integer_at(0);
    });
    if(!query.all_succeeded()||!value)throw ResearchAiUnavailable("Research AI handicap choice setting unavailable");
    return *value;
}
namespace {
std::int64_t bnw_choose(const ResearchAiCandidates &candidates,std::int64_t count,SimulationRandom &random) {
    if(random.algorithm()!=RandomAlgorithm::bnw)throw ResearchAiUnavailable("BNW TechAI requires the BNW gameplay stream");
    if(candidates.choices.empty())return -1;
    auto choices=candidates.choices;
    for(const auto &choice:choices)if(choice.adjusted_weight<0||choice.adjusted_weight>INT32_MAX)
        throw ResearchAiUnavailable("BNW TechAI candidate weight outside native int32");
    // SDK CvWeightedVector::SortItems uses std::sort, descending weight.
    std::sort(choices.begin(),choices.end(),[](const auto &a,const auto &b){return a.adjusted_weight>b.adjusted_weight;});
    // The native vector totals and count are signed int32. Keep unsupported
    // overflowing sums explicit rather than using the generic wide extension.
    std::int32_t all=0;
    for(const auto &choice:choices){
        if(choice.adjusted_weight>INT32_MAX-all)
            throw ResearchAiUnavailable("BNW TechAI native int32 total overflow unavailable");
        all+=static_cast<std::int32_t>(choice.adjusted_weight);
    }
    if(!all)return -1; // BNW has no zero-weight fallback.
    if(count<=0||count>INT32_MAX)throw ResearchAiUnavailable("BNW TechAI top-choice count outside positive native int32");
    const auto top=std::min<std::uint64_t>(static_cast<std::uint64_t>(count),choices.size());
    std::int32_t total=0;
    // All validated weights are nonnegative and their complete total fits;
    // the top subset therefore also has the native nonoverflow int32 sum.
    for(std::uint64_t i=0;i<top;++i)total+=static_cast<std::int32_t>(choices[static_cast<std::size_t>(i)].adjusted_weight);
    if(total<=0)throw ResearchAiUnavailable("BNW TechAI top choices have no weight");
    // CvGame::getJonRandNum(int) passes this to CvRandom::get(ushort).
    // A narrowed zero still performs the one genuine BNW gameplay draw.
    const auto native_bound=static_cast<std::uint16_t>(total);
    auto value=random.draw(native_bound);
    for(std::uint64_t i=0;i<top;++i) {
        const auto &choice=choices[static_cast<std::size_t>(i)];
        if(value<static_cast<std::uint64_t>(choice.adjusted_weight))return choice.technology;
        value-=static_cast<std::uint64_t>(choice.adjusted_weight);
    }
    throw ResearchAiUnavailable("BNW TechAI weighted roll did not select a technology");
}
}
ResearchAiChoiceResult choose_research_ai(ResearchAiChoiceContext &c,std::int64_t player) {
    const auto profile=c.selection.setup.profile;
    if(profile!=content::TechnologyProfile::bnw&&profile!=content::TechnologyProfile::vp542)
        throw ResearchAiUnavailable("Research AI GameCore profile unsupported");
    if(c.turn.progress.setup.profile!=profile||c.weights.profile!=profile||
       &c.selection.turn!=&c.turn.state||&c.selection.players!=&c.turn.progress.players||
       &c.selection.setup!=&c.turn.progress.setup||
       !c.selection.setup.players.contains(player)||!c.selection.players.players.contains(player))
        throw ResearchAiUnavailable("Research AI choice requires one shared GameCore world");
    if(!c.scripts.loading_complete())throw ResearchAiUnavailable("Research AI Lua content loading is incomplete");
    if(profile==content::TechnologyProfile::bnw&&c.gameplay_random.algorithm()!=RandomAlgorithm::bnw)
        throw ResearchAiUnavailable("BNW TechAI requires the BNW gameplay stream");
    clear_research_queue(c.selection,player);
    ResearchAiChoiceResult result;
    const auto team=c.selection.setup.players.at(player).team;
    // Both source implementations scan player slots in ID order and can append
    // multiple teammate projects before testing the resulting front.
    for(const auto &[id,other]:c.selection.setup.players) {
        if(id==player||!other.alive||other.team!=team)continue;
        const auto it=c.selection.players.players.find(id);
        if(it==c.selection.players.players.end())throw ResearchAiUnavailable("Teammate research state unavailable");
        const auto technology=it->second.current_research();
        if(technology>=0&&can_research(c.selection,player,technology)) {
            if(push_research(c.selection,player,technology))result.teammate=true;
        }
    }
    result.technology=c.selection.players.players.at(player).current_research();
    if(result.technology>=0)return result;
    if(profile==content::TechnologyProfile::vp542) {
        const auto vp=vp_choose_research_ai(c,player);
        result.technology=vp.technology;
        result.script_override=vp.script_override;
        result.weighted=vp.weighted;
    } else {
        const auto candidates=build_research_ai_candidates(c.selection,c.turn,c.weights,player,false);
        result.technology=bnw_choose(candidates,
            research_ai_handicap_setting(c,"TechNumOptionsConsidered"),c.gameplay_random);
        result.weighted=result.technology>=0;
    }
    if(result.technology>=0)push_research(c.selection,player,result.technology);
    return result;
}
ResearchTurnResult do_research_with_ai(ResearchAiChoiceContext &c,std::int64_t player) {
    const auto profile=c.selection.setup.profile;
    if(profile!=content::TechnologyProfile::bnw&&profile!=content::TechnologyProfile::vp542)
        throw ResearchAiUnavailable("Research turn GameCore profile unsupported");
    if(c.selection.state.profile!=profile||c.selection.turn.profile!=profile||
       c.turn.state.profile!=profile||c.turn.progress.setup.profile!=profile||
       c.weights.profile!=profile||&c.selection.turn!=&c.turn.state||
       &c.selection.players!=&c.turn.progress.players||
       &c.selection.setup!=&c.turn.progress.setup||
       &c.selection.technologies!=&c.turn.progress.technologies||
       c.selection.teams.data()!=c.turn.progress.teams.data()||
       c.selection.teams.size()!=c.turn.progress.teams.size()||
       !c.scripts.loading_complete())
        throw ResearchAiUnavailable("Research turn AI requires one loaded GameCore world and Lua host");
    class Services final : public ResearchTurnServices {
        ResearchTurnServices &delegate;
        ResearchAiChoiceContext *choice=nullptr;
      public:
        explicit Services(ResearchTurnServices &services):delegate(services){}
        void bind(ResearchAiChoiceContext &context){choice=&context;}
        void choose_research_ai(std::int64_t id) override {
            if(!choice)throw ResearchAiUnavailable("Research turn AI is not bound");
            runtime::choose_research_ai(*choice,id);
        }
        void choose_research_ui(std::int64_t id) override {delegate.choose_research_ui(id);}
        void research_buttons_dirty(std::int64_t team) override {delegate.research_buttons_dirty(team);}
        void unlock_research_achievement(std::int64_t id,ResearchAchievement achievement) override {
            delegate.unlock_research_achievement(id,achievement);
        }
    } services(c.turn.services);
    ResearchTurnContext turn{c.turn.state,c.turn.progress,c.turn.science,c.turn.economy,
        c.turn.treasury,c.turn.maintenance,c.turn.treasury_rules,c.turn.city_rules,
        c.turn.maintenance_rules,services,c.turn.active_player,c.turn.income_services};
    ResearchAiChoiceContext choice{c.selection,turn,c.weights,c.rules,c.scripts,c.gameplay_random};
    services.bind(choice);
    return do_research(turn,player);
}
} // namespace civ5::runtime
