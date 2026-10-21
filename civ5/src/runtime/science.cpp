#include "civ5/runtime/science.hpp"
#include "civ5/runtime/vp_science_profile.hpp"
#include "civ5/runtime/vp_treasury_minor_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;
template<class T> const T &named(const std::vector<T> &v,std::string_view name) {
    const T *found=nullptr;
    for(const auto &f:v) if(f.name&&std::string_view(f.name)==name) {
        if(found) throw std::runtime_error("Duplicate science field: "+std::string(name));
        found=&f;
    }
    if(!found) throw std::runtime_error("Missing science field: "+std::string(name));
    return *found;
}
template<class T> std::int64_t scalar(const std::vector<T> &v,std::string_view name) {
    const auto &values=named(v,name).values;
    if(values.size()!=1) throw std::runtime_error("Invalid science scalar");
    return values.front();
}
std::int64_t count64(std::uint64_t n) {
    if(n>INT64_MAX) throw std::overflow_error("Science collection exceeds int64");
    return static_cast<std::int64_t>(n);
}
std::int64_t integer(const content::GlobalDefineCache &d,std::string_view name) {
    const auto *v=d.find(name);
    if(!v||!v->lookup.error.empty()||!std::holds_alternative<std::int64_t>(v->lookup.value))
        throw ScienceUnavailable("Unresolved science define: "+std::string(name));
    return std::get<std::int64_t>(v->lookup.value);
}
std::int64_t product(std::int64_t a,std::int64_t b) { return wide::multiply_divide(a,b,1); }
std::int64_t percent(std::int64_t a,std::int64_t b) { return wide::multiply_divide(a,b,100); }
YieldValues sparse(const std::vector<std::int64_t> &v) {
    YieldValues result;
    for(std::size_t i=0;i<v.size();++i) if(v[i]) result.emplace(count64(i),v[i]);
    return result;
}
std::optional<std::pair<std::int64_t,std::int64_t>> spy_city(const saves::SavedSessionSnapshot &s,
    const CityYieldContext &cities,std::int64_t x,std::int64_t y) {
    // INVALID_PLOT_COORD is a source sentinel, not an owned-coordinate limit.
    if(x==-2147483647LL||y==-2147483647LL) return {};
    const auto coordinate=[](std::int64_t n,std::uint64_t size,bool wrap)->std::optional<std::uint64_t> {
        const auto width=count64(size); if(width<=0) return {};
        if(wrap) {
            // Preserve CvMap::coordRange, including its negative-multiple edge.
            if(n<0) n=wide::add(width,n%width);
            else if(n>=width) n%=width;
        }
        if(n<0||n>=width) return {};
        return static_cast<std::uint64_t>(n);
    };
    const auto px=coordinate(x,s.map_header.grid.width,s.map_header.wrap_x);
    const auto py=coordinate(y,s.map_header.grid.height,s.map_header.wrap_y);
    if(!px||!py) return {};
    const auto &plot=s.map_plots->plots.at(s.map_header.grid.index(*px,*py));
    if(!named(plot.fields,"is_city").value) return {};
    const auto owner=named(plot.fields,"owning_city_owner").value, city=named(plot.fields,"owning_city_id").value;
    if(owner==-1||city==-1||!cities.players.contains(owner)||!cities.players.at(owner).cities.contains(city)) return {};
    return std::pair{owner,city};
}
}
ScienceContext restore_science(const saves::SavedSessionSnapshot &snap,ResearchContext &setup,
    const MaintenanceWorld &world,const content::RulesDatabase &db) {
    const bool vp=setup.profile==TechnologyProfile::vp542;
    const auto layout=vp?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1;
    if(!snap.all_players||!snap.teams||snap.all_players->layout!=layout||snap.teams->layout!=layout||
       world.setup.profile!=setup.profile||world.cities.profile!=setup.profile||world.diplomacy.profile!=setup.profile||
       (vp&&(!snap.map_plots||snap.map_plots->layout!=layout||snap.map_header.layout!=layout)))
        throw std::runtime_error("Science needs a matching complete saved session");
    ScienceContext s;s.profile=setup.profile;
    s.no_science=setup.game_option_value(db,"GAMEOPTION_NO_SCIENCE")==1;
    const auto &wire=snap.teams->dimensions;
    for(const auto &saved:snap.all_players->players) {
        const auto id=saved.saved_player_id;
        if(saved.core.layout!=layout||!setup.players.contains(id)||!world.cities.players.contains(id)||
           !world.diplomacy.players.contains(id)) throw std::runtime_error("Science player identity mismatch");
        SciencePlayer p;
        p.happiness_to_science=scalar(saved.core.fields,"m_iHappinessToScience");
        p.minor_science_allies=scalar(saved.core.fields,"m_iMinorScienceAlliesCount")>0;
        for(auto [who,n]:sparse(named(saved.core.fields,"m_pabGetsScienceFromPlayer").values)) p.science_from_players.insert(who);
        p.median_tech_percent=scalar(saved.core.fields,"m_iMedianTechPercentage");
        if(vp) {
            auto &profile=ensure_vp_science_profile(p);
            if(saved.strategy.layout!=layout||saved.espionage.layout!=layout||saved.traits.layout!=layout||saved.tail.layout!=layout)
                throw std::runtime_error("Science subsystem profile mismatch");
            profile.cached_happiness_total=scalar(saved.core.fields,"m_iHappinessTotal");
            profile.last_turn_times100=named(saved.core.fields,"m_aiLastTurnYieldsTimes100").values.at(3);
            profile.vassal_modifier=scalar(saved.core.fields,"m_iVassalYieldBonusModifier");
            profile.per_trade_partner=named(saved.traits.fields,"m_iYieldChangePerTradePartner").values.at(3);
            profile.jfd_minors=named(saved.core.fields,"m_aiYieldFromMinors").values.at(3);
            profile.annexed_minors_times100=named(saved.core.fields,"m_piYieldPerTurnFromAnnexedMinorsTimes100").values.at(3);
            profile.incoming_espionage=named(saved.tail.fields,"m_aiIncomingEspionageYields").values.at(3);
            profile.outgoing_espionage=named(saved.tail.fields,"m_aiOutgoingEspionageYields").values.at(3);
            const auto &policies=named(saved.strategy.blocks,"policies");
            const auto &modifiers=named(policies.arrays,"policy_modifiers").values;
            // Selected VP wire profile: 60 policy modifiers, including the
            // HH natural-wonder enum entry. This does not bound owned state.
            if(modifiers.size()!=60) throw ScienceUnavailable("Unverified VP policy-modifier wire layout");
            if(policies.end_offset<244||policies.start_offset>policies.end_offset-244) throw std::runtime_error("Invalid saved policy range");
            profile.source_policy_modifiers=policies.end_offset-240;
            profile.source_espionage={{saved.espionage.start_offset,saved.espionage.end_offset}};
            profile.passive_spy_modifier=modifiers[43];
            for(const auto &spy:named(saved.espionage.blocks,"player_espionage").children) if(std::string_view(spy.name)=="spy") {
                const auto state=named(spy.fields,"m_eSpyState").value;
                profile.spies.push_back({state!=8&&state!=9,spy_city(snap,world.cities,
                    named(spy.fields,"m_iCityX").value,named(spy.fields,"m_iCityY").value)});
            }
            for(const auto &city:saved.cities.cities) {
                const auto cid=scalar(city.core.fields,"m_iID");
                if(city.tail.layout!=layout||scalar(city.core.fields,"m_eOwner")!=id||!world.cities.players.at(id).cities.contains(cid))
                    throw std::runtime_error("Science city identity mismatch");
                const auto &values=named(named(city.tail.blocks,"espionage").children,"m_aiSciencePassivePerTurn").values;
                if(!profile.passive_spy_percent.emplace(cid,sparse(values)).second) throw std::runtime_error("Duplicate science city");
            }
            if(profile.passive_spy_percent.size()!=world.cities.players.at(id).cities.size()) throw std::runtime_error("Incomplete science city roster");
        }
        if(!s.players.emplace(id,std::move(p)).second) throw std::runtime_error("Duplicate science player");
        if(id>=count64(wire.major)&&id<count64(wire.civ_teams)) s.minor_players.insert(id);
        if(id==count64(wire.civ_teams)) s.barbarian_players.insert(id);
    }
    for(const auto &saved:snap.teams->teams) {
        const auto id=named(saved.fields,"id").value;
        if(!world.diplomacy.teams.contains(id)) throw std::runtime_error("Science team identity mismatch");
        const auto &array=named(saved.arrays,"research_agreement").values;
        std::set<std::int64_t> agreements;
        for(const auto &[other,unused]:world.diplomacy.teams) {
            if(other<0||static_cast<std::uint64_t>(other)>=array.size()) throw std::runtime_error("Science RA wire row missing");
            if(array[static_cast<std::size_t>(other)]) agreements.insert(other);
        }
        if(!s.research_agreements.emplace(id,std::move(agreements)).second) throw std::runtime_error("Duplicate science team");
    }
    if(s.players.size()!=setup.players.size()||s.research_agreements.size()!=world.diplomacy.teams.size())
        throw std::runtime_error("Incomplete science roster");
    return s;
}
ScienceQuote science_times100(const ScienceContext &s,const MaintenanceWorld &w,const TreasuryContext &treasury,
    MaintenanceContext &m,const TreasuryRules &tr,const CityYieldRules &cr,const MaintenanceRules &mr,
    const content::GlobalDefineCache &d,const content::CustomModOptions *o,std::int64_t player,
    bool exclude,const TreasuryServices &services) {
    const bool vp=s.profile==TechnologyProfile::vp542;
    if(s.profile!=w.setup.profile||s.profile!=w.cities.profile||s.profile!=w.diplomacy.profile||s.profile!=treasury.profile||
       s.profile!=m.profile||s.profile!=tr.profile||s.profile!=cr.profile||s.profile!=mr.profile||
       d.profile()!=(vp?content::GlobalDefineProfile::vp542:content::GlobalDefineProfile::bnw)||
       (vp&&(!o||!o->all_succeeded()))||(!vp&&(o||exclude))) throw std::runtime_error("Science profile/rules mismatch");
    const auto option=[&](const char *name){return o&&o->enabled(name);};
    struct Frame {
        std::int64_t id; bool exclude; unsigned phase=0;
        ScienceQuote q;
        std::vector<std::int64_t> others,vassals;
        std::size_t next=0;
    };
    std::vector<Frame> stack;std::set<std::int64_t> active;
    const auto push=[&](std::int64_t id,bool excluded) {
        if(!active.insert(id).second) throw ScienceUnavailable("Cyclic science dependency at player "+std::to_string(id));
        Frame f{};f.id=id;f.exclude=excluded;stack.push_back(std::move(f));
    };
    push(player,exclude);
    while(!stack.empty()) {
        auto &f=stack.back();const auto id=f.id;const auto &p=s.players.at(id);
        const auto team=w.setup.players.at(id).team;
        if(f.phase==0) {
            f.q.research_agreements_excluded=f.exclude;
            if(vp) ensure_vp_science_quote(f.q);
            if(treasury.players.at(id).anarchy) { f.q.anarchy=true;f.phase=5; }
            else {
                f.q.cities=science_from_cities_times100(w.cities,cr,d,id);
                if(vp&&!option("BALANCE_VP")) {
                    std::set<std::int64_t> partners;
                    for(const auto &route:treasury.trade_routes) {
                        if(route.origin_x==-1&&route.origin_y==-1&&route.destination_x==-1&&route.destination_y==-1) continue;
                        if(route.origin_owner==route.destination_owner) continue;
                        if(route.origin_owner==id) partners.insert(route.destination_owner);
                        else if(route.destination_owner==id) partners.insert(route.origin_owner);
                    }
                    f.q.traits=product(product(vp_science_profile(p).per_trade_partner,count64(partners.size())),100);
                }
                if(!vp||w.diplomacy.team_is_major(w.setup,team)) for(auto minor:s.minor_players) {
                    const auto &cs=treasury.players.at(minor);
                    const auto *minor_profile=vp?&vp_treasury_minor_profile(cs):nullptr;
                    const bool allied=cs.ally==id||(minor_profile&&minor_profile->permanent_ally==id&&!minor_profile->no_ally);
                    if((p.minor_science_allies&&allied)||p.science_from_players.contains(minor)) f.others.push_back(minor);
                }
                f.phase=1;
            }
        }
        if(f.phase==1) {
            if(f.next<f.others.size()) {
                const auto child=f.others[f.next++];
                push(child,false);continue;
            }
            if(!w.cities.no_happiness&&p.happiness_to_science!=0) {
                std::int64_t excess=0;
                if(vp) {
                    const auto &cities=w.cities.players.at(id);
                    excess=cities.minor||s.barbarian_players.contains(id)||cities.cities.empty()
                        ? (option("BALANCE_VP")?integer(d,"UNHAPPY_THRESHOLD"):0) : vp_science_profile(p).cached_happiness_total;
                } else {
                    const auto &value=w.cities.players.at(id).excess_happiness;
                    if(!value) throw ScienceUnavailable("BNW science requires resolved excess happiness");
                    excess=*value;
                }
                if(excess>=0) f.q.happiness=percent(f.q.cities,p.happiness_to_science);
            }
            if(!f.exclude) {
                const auto &agreements=s.research_agreements.at(team);
                std::int64_t count=0;
                for(auto other:w.diplomacy.civ_teams) if(w.diplomacy.teams.at(other).alive_count>0&&agreements.contains(other)) count=wide::add(count,1);
                f.q.agreements=percent(f.q.cities,product(integer(d,"RESEARCH_AGREEMENT_MOD"),count));
                if(vp) for(auto other:w.diplomacy.major_players) {
                    const auto other_team=w.setup.players.at(other).team;
                    if(other==id||other_team==team||!agreements.contains(other_team)) continue;
                    const auto previous=vp_science_profile(s.players.at(other)).last_turn_times100;
                    const auto average=integer(d,"RESEARCH_AGREEMENT_PLAYER_AVERAGE_YIELD_PERCENT");
                    const auto last=vp_science_profile(p).last_turn_times100;
                    const auto numerator=wide::add(product(wide::subtract(100,average),std::min(last,previous)),
                        wide::multiply_divide(average,wide::add(last,previous),2));
                    auto bonus=wide::multiply_divide(numerator,1,product(100,integer(d,"RESEARCH_AGREEMENT_BOOST_DIVISOR")));
                    bonus=percent(bonus,p.median_tech_percent);
                    f.q.agreements=wide::add(f.q.agreements,percent(bonus,integer(d,"RESEARCH_AGREEMENT_PER_TURN_YIELD_PERCENT")));
                }
            }
            f.q.gold_rate_times100=net_gold_times100(treasury,w,m,tr,cr,mr,d,o,id,services).net_times100;
            f.q.budget_deficit=std::min<std::int64_t>(0,wide::add(treasury.players.at(id).balance_times100,*f.q.gold_rate_times100));
            if(vp&&!s.no_science) for(auto other:w.diplomacy.major_players)
                if(w.diplomacy.teams.at(w.setup.players.at(other).team).master==team) f.vassals.push_back(other);
            f.phase=2;f.next=0;
        }
        if(f.phase==2) {
            if(f.next<f.vassals.size()) {
                const auto child=f.vassals[f.next++];
                push(child,false);continue;
            }
            if(vp) {
                auto &vp_quote=ensure_vp_science_quote(f.q);
                const auto &profile=vp_science_profile(p);
                if(option("BALANCE_CORE_JFD")) vp_quote.jfd_minors=product(profile.jfd_minors,100);
                for(auto minor:s.minor_players) if(w.setup.players.at(minor).alive)
                    vp_quote.minors=wide::add(vp_quote.minors,minor_yield_times100(treasury,w,m,tr,d,*o,minor,id,w.cities.science).total_times100);
                vp_quote.annexed_minors=profile.annexed_minors_times100;
                vp_quote.incoming_espionage=product(profile.incoming_espionage,100);
                vp_quote.outgoing_espionage=product(profile.outgoing_espionage,100);
                if(std::any_of(profile.spies.begin(),profile.spies.end(),[](const auto &spy){return spy.alive;})) {
                    // Native code checks alive count once, then visits every spy,
                    // including dead entries retaining a city and duplicates.
                    for(const auto &spy:profile.spies) if(spy.city) {
                        const auto [owner,cid]=*spy.city;
                        const auto &bonuses=vp_science_profile(s.players.at(owner)).passive_spy_percent.at(cid);
                        const auto b=bonuses.find(id);const auto modifier=b==bonuses.end()?0:b->second;
                        const auto science=city_yield_times100(w.cities,cr,d,owner,cid,w.cities.science).total_times100;
                        vp_quote.passive_spies=wide::add(vp_quote.passive_spies,percent(science,modifier));
                    }
                    vp_quote.passive_spies=percent(vp_quote.passive_spies,wide::add(100,profile.passive_spy_modifier));
                }
            }
            for(auto n:{f.q.cities,f.q.religion,f.q.traits,f.q.other_players,f.q.happiness,f.q.agreements,f.q.budget_deficit})
                f.q.total_times100=wide::add(f.q.total_times100,n);
            if(vp) {
                const auto &vp_quote=vp_science_quote(f.q);
                for(auto n:{vp_quote.vassals,vp_quote.jfd_minors,vp_quote.minors,vp_quote.annexed_minors,vp_quote.incoming_espionage})
                    f.q.total_times100=wide::add(f.q.total_times100,n);
                f.q.total_times100=wide::subtract(f.q.total_times100,vp_quote.outgoing_espionage);
                f.q.total_times100=wide::add(f.q.total_times100,vp_quote.passive_spies);
            }
            f.q.total_times100=std::max<std::int64_t>(0,f.q.total_times100);
            f.phase=5;
        }
        if(f.phase==5) {
            const auto result=f.q;active.erase(id);stack.pop_back();
            if(stack.empty()) return result;
            auto &parent=stack.back();
            const auto modifier=parent.phase==1?integer(d,"MINOR_CIV_SCIENCE_BONUS_MULTIPLIER"):
                wide::add(integer(d,"VASSALAGE_FREE_YIELD_FROM_VASSAL_PERCENT"),
                    vp_science_profile(s.players.at(parent.id)).vassal_modifier);
            if(parent.phase==1) parent.q.other_players=wide::add(parent.q.other_players,percent(result.total_times100,modifier));
            else {
                auto &vp_quote=ensure_vp_science_quote(parent.q);
                vp_quote.vassals=wide::add(vp_quote.vassals,percent(result.total_times100,modifier));
            }
        }
    }
    throw std::logic_error("Empty science evaluation");
}
} // namespace civ5::runtime
