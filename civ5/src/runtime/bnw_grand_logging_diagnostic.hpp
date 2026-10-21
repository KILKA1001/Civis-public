#pragma once
#include <array>
#include <cstdio>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime::diagnostic {
// Modeled observations only. No LiveSession, genuine owner, writer or phase receipt.
struct ModeledGcLoggingByte { std::uint8_t value; };
enum class GrandLogPredicate { missing_logging, missing_ai_logging,
    modeled_logging_zero_return, modeled_ai_logging_zero_return, native_string_effect_frontier };
struct GrandLogPredicateTrace { GrandLogPredicate outcome; unsigned attempted=0, read=0; };
constexpr GrandLogPredicateTrace grand_log_predicates(
    std::optional<ModeledGcLoggingByte> logging,
    std::optional<ModeledGcLoggingByte> ai_logging) noexcept {
    if(!logging)return {GrandLogPredicate::missing_logging,1,0};
    if(logging->value==0)return {GrandLogPredicate::modeled_logging_zero_return,1,1};
    if(!ai_logging)return {GrandLogPredicate::missing_ai_logging,3,1};
    if(ai_logging->value==0)return {GrandLogPredicate::modeled_ai_logging_zero_return,3,3};
    return {GrandLogPredicate::native_string_effect_frontier,3,3};
}
// This literal represents the present production boundary, independent of any
// modeled bytes. No producer/authenticated source lifecycle owner exists here.
enum class GrandLogActualFrontier { genuine_gc_logging_lifecycle_owner_required };
constexpr GrandLogActualFrontier actual_grand_log_frontier() noexcept {
    return GrandLogActualFrontier::genuine_gc_logging_lifecycle_owner_required;
}

enum class TextObservation { unobserved, native_null, native_value };
struct ModeledLogText { TextObservation state=TextObservation::unobserved; std::string_view value; };
inline std::string text(ModeledLogText t,std::string_view native_null) {
    if(t.state==TextObservation::unobserved)throw std::runtime_error("Grand log source text unobserved");
    if(t.state==TextObservation::native_null)return std::string(native_null);
    if(t.state!=TextObservation::native_value)throw std::runtime_error("Grand log text observation invalid");
    return std::string(t.value);
}
struct ModeledGrandLogInputs {
    std::int32_t elapsed=0,active=-1;
    ModeledLogText civilization;
    std::optional<ModeledGcLoggingByte> split;
    std::array<ModeledLogText,4> strategy_types;
    std::array<std::int32_t,4> priorities{},pressure_changes{};
};
struct ModeledLogPlan { std::string filename; std::vector<std::string> messages; unsigned get_log_calls=1; };
inline std::string prefix(std::int32_t elapsed,const std::string&civilization) {
    char buffer[32];const auto n=std::snprintf(buffer,sizeof(buffer),"%03d, ",static_cast<int>(elapsed));
    if(n<0||static_cast<std::size_t>(n)>=sizeof(buffer))throw std::runtime_error("Grand log turn format unavailable");
    return std::string(buffer)+civilization+", ";
}
inline ModeledLogPlan grand_strategy_log_plan(const ModeledGrandLogInputs&i) {
    // Pure source formatting, not delivered logs and never source completion.
    const auto civ=text(i.civilization,"");
    if(!i.split)throw std::runtime_error("Grand log source split flag unobserved");
    ModeledLogPlan p;p.filename=i.split->value?"GrandStrategyAI_Log_"+civ+".csv":"GrandStrategyAI_Log.csv";
    for(std::size_t slot=0;slot<i.strategy_types.size();++slot) {
        auto type=text(i.strategy_types[slot],"Unknown Type");
        if(i.active==static_cast<std::int32_t>(slot))type="*** "+type;
        p.messages.push_back(prefix(i.elapsed,civ)+type+", "+std::to_string(i.priorities[slot])+", "+std::to_string(i.pressure_changes[slot]));
    }
    return p;
}
inline ModeledLogPlan new_grand_specialization_log_plan(const ModeledGrandLogInputs&i) {
    const auto civ=text(i.civilization,"");
    if(!i.split)throw std::runtime_error("Specialization log source split flag unobserved");
    ModeledLogPlan p;p.filename=i.split->value?"CitySpecializationLog_"+civ+".csv":"CitySpecializationLog.csv";
    std::string reason;
    if(i.active>=0&&i.active<4) {
        const auto&t=i.strategy_types[static_cast<std::size_t>(i.active)];
        if(t.state==TextObservation::unobserved)throw std::runtime_error("Specialization log active Info unobserved");
        if(t.state!=TextObservation::native_null)reason="Update: New grand strategy: "+text(t,"");
    } // source GetInfo(-1/outside) -> NULL, strTypeString remains empty
    p.messages.push_back(prefix(i.elapsed,civ)+reason);
    return p;
}
} // namespace civ5::runtime::diagnostic
