#include "civ5/runtime/bnw_team_tech_domain_yields.hpp"
#include <array>
#include <climits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
saves::PlotArrayObservation &extra_moves(saves::TeamObservation &team) {
    saves::PlotArrayObservation *found=nullptr;
    for(auto &array:team.arrays)
        if(array.name && std::string_view(array.name)=="extra_moves") {
            require(!found,"Duplicate BNW extra-moves array");
            found=&array;
        }
    require(found && found->columns==1 && found->values.size()==5,
            "BNW five-domain extra-moves array unavailable");
    return *found;
}
std::uint64_t count_rows(const content::RulesDatabase &database,
                         const char *table,bool dense=false) {
    std::uint64_t count=0;
    const auto report=database.query_rows(
        {std::string("SELECT ID FROM ")+table+" ORDER BY ID",{}},
        [&](const content::RulesRowView &row) {
            require(row.integer("ID")>=0 && row.integer("ID")<=INT32_MAX,
                    "BNW active info ID outside source range");
            require(!dense || row.integer("ID")==static_cast<std::int64_t>(count),
                    "BNW source yield IDs are not dense slots");
            ++count;
        });
    require(report.all_succeeded(),"BNW active building/improvement/yield rules unavailable");
    return count;
}
}

BnwTechDomainYieldPrefix run_bnw_barbarian_domain_yield_prefix(
    LiveSessionState &state,const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    std::int64_t technology) {
    BnwTechDomainYieldPrefix result;
    // The route/build prefix and this suffix form one detached transaction:
    // a later rule error leaves the caller's state untouched.
    auto draft=state;
    const auto prior=run_bnw_barbarian_route_build_prefix(
        draft,technologies,database,types,technology);
    if(!prior.applied) {
        result.blocker=prior.blocker;
        return result;
    }
    try {
        const auto *info=technologies.find(static_cast<std::uint64_t>(technology));
        require(info && !info->base.type.empty() && draft.world.teams &&
                draft.world.teams->teams.size()==64 &&
                draft.world.teams->dimensions.domains==5,
                "BNW technology or five-domain team state unavailable");
        auto &moves=extra_moves(draft.world.teams->teams[63]);
        require(info->domain_extra_moves.extent>=5,
                "BNW technology domain movement rules incomplete");
        for(std::int64_t domain=0;domain<5;++domain) {
            const auto before=moves.values[static_cast<std::size_t>(domain)];
            const auto delta=info->domain_extra_moves.at(
                static_cast<std::uint64_t>(domain));
            require(before>=0 && before<=INT32_MAX &&
                    delta>=INT32_MIN && delta<=INT32_MAX &&
                    before+delta>=0 && before+delta<=INT32_MAX,
                    "BNW domain movement delta exceeds source range");
            result.domain_calls.push_back({domain,before,delta,before+delta});
        }

        // CvTeam::processTech 6120-6134: an obsolete building updates its
        // team counter and may process each city building; enhancement changes
        // city yields. Keep either as a required world-effect gate.
        const auto buildings=database.query_rows(
            {"SELECT ObsoleteTech,EnhancedYieldTech FROM Buildings",{}},
            [&](const content::RulesRowView &row) {
                if(row.text("ObsoleteTech")==info->base.type)
                    ++result.building_effects;
                if(row.text("EnhancedYieldTech")==info->base.type)
                    ++result.building_effects;
            });
        require(buildings.all_succeeded(),"BNW active building technology rules unavailable");
        if(result.building_effects) {
            result.blocker="Building obsolescence/enhancement requires city effects";
            return result;
        }

        // Improvement tech arrays are zero-initialized by Initialize2DArray.
        // The three source calls run for every active improvement/yield pair;
        // any nonzero delta calls CvTeam::updateYield(), a world effect.
        const auto improvements=count_rows(database,"Improvements");
        const auto yields=count_rows(database,"Yields",true);
        require(yields==draft.world.teams->dimensions.yields && yields==6,
                "BNW source yield dimension does not match saved team state");
        require(improvements<=UINT64_MAX/(3*yields),
                "BNW improvement tech call count exceeds host range");
        result.improvement_calls=improvements*3*yields;
        constexpr std::array<const char*,3> relations={
            "Improvement_TechYieldChanges",
            "Improvement_TechNoFreshWaterYieldChanges",
            "Improvement_TechFreshWaterYieldChanges"};
        for(const auto *relation:relations) {
            const auto sql=std::string("SELECT ")+relation+".Yield FROM "+relation+
                " INNER JOIN Improvements ON "+relation+
                ".ImprovementType=Improvements.Type INNER JOIN Yields ON "+
                relation+".YieldType=Yields.Type INNER JOIN Technologies ON "+
                relation+".TechType=Technologies.Type WHERE Technologies.ID=?";
            const auto report=database.query_rows({sql,{technology}},
                [&](const content::RulesRowView &row) {
                    if(row.integer_at(0)!=0) ++result.improvement_nonzero;
                });
            require(report.all_succeeded(),
                    "BNW active improvement technology rules unavailable");
        }
        if(result.improvement_nonzero) {
            result.blocker="Nonzero improvement technology yield requires updateYield";
            return result;
        }

        // No building or improvement call has an effect. Domain movement is
        // the only mutable operation in this interval and is a plain array
        // addition; publish it on the caller's detached draft only now.
        for(const auto &call:result.domain_calls)
            moves.values[static_cast<std::size_t>(call.domain)]=call.after;
        result.blocker="Player trait reset, map yield, Lua and remaining acquisition effects remain";
        state=std::move(draft);
        result.applied=true;
    } catch(const std::exception &error) {
        result.blocker=error.what();
    }
    return result;
}

} // namespace civ5::runtime
