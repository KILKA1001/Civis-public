#include "civ5/runtime/vp_espionage_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <set>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','V','E','S','0','1'};
constexpr auto section_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::vp_espionage_profile);
using CityKey=std::pair<std::int64_t,std::int64_t>;
void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void hash(std::string_view value){
    require(value.size()==64&&std::all_of(value.begin(),value.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');}),"Invalid VP espionage checkpoint SHA-256");
}
void charge(std::uint64_t &used,std::uint64_t count,std::uint64_t limit){
    require(used<=limit&&count<=limit-used,"VP espionage checkpoint entry budget exceeded");
    used+=count;
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    std::uint64_t limit;
    explicit Writer(std::uint64_t maximum):limit(maximum){}
    void raw(std::span<const std::uint8_t> value){
        require(bytes.size()<=limit&&value.size()<=limit-bytes.size(),
            "VP espionage checkpoint byte budget exceeded");
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void u8(std::uint8_t value){raw({&value,1});}
    void u32(std::uint32_t value){for(unsigned i=0;i<4;++i)u8(static_cast<std::uint8_t>(value>>(8*i)));}
    void u64(std::uint64_t value){for(unsigned i=0;i<8;++i)u8(static_cast<std::uint8_t>(value>>(8*i)));}
    void i64(std::int64_t value){u64(std::bit_cast<std::uint64_t>(value));}
    void boolean(bool value){u8(value?1:0);}
    void text(std::string_view value){
        u64(value.size());raw({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});
    }
    void digest(std::string_view value){
        hash(value);raw({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    explicit Reader(std::span<const std::uint8_t> source):bytes(source){}
    std::size_t remaining()const{return bytes.size()-at;}
    std::span<const std::uint8_t> raw(std::size_t count){
        require(count<=remaining(),"Truncated VP espionage checkpoint");
        const auto result=bytes.subspan(at,count);at+=count;return result;
    }
    std::uint8_t u8(){return raw(1)[0];}
    bool boolean(){const auto value=u8();require(value<=1,"Invalid VP espionage boolean");return value!=0;}
    std::uint32_t u32(){std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::uint32_t(u8())<<(8*i);return value;}
    std::uint64_t u64(){std::uint64_t value=0;for(unsigned i=0;i<8;++i)value|=std::uint64_t(u8())<<(8*i);return value;}
    std::int64_t i64(){return std::bit_cast<std::int64_t>(u64());}
    std::uint64_t count(std::uint64_t maximum,std::size_t minimum){
        const auto value=u64();
        require(value<=maximum&&value<=remaining()/minimum,
            "Oversized VP espionage checkpoint collection");
        return value;
    }
    std::string text(){
        const auto length=u64();
        require(length<=remaining(),"Oversized VP espionage checkpoint text");
        if(!length)return {};
        const auto value=raw(static_cast<std::size_t>(length));
        return {reinterpret_cast<const char*>(value.data()),value.size()};
    }
    void digest(std::string_view expected,const char *message){
        hash(expected);const auto actual=raw(expected.size());
        require(std::equal(actual.begin(),actual.end(),expected.begin()),message);
    }
};
std::uint64_t validate(const ResearchContext &setup,const CityYieldContext &cities,
    const SessionCheckpointIdentity &identity,const VpEspionageCheckpointBudget &budget){
    hash(identity.payload_sha256);hash(identity.rules_sha256);
    require(setup.profile==content::TechnologyProfile::vp542&&cities.profile==setup.profile&&
        setup.players.size()<=budget.players&&cities.players.size()==setup.players.size(),
        "VP espionage checkpoint requires one VP player roster");
    const auto *profile=find_vp_espionage_profile(setup);
    require(profile&&profile->players.size()==setup.players.size(),
        "Missing VP espionage checkpoint player profile");
    for(const auto &[id,player]:setup.players){
        (void)player;
        require(profile->players.contains(id)&&cities.players.contains(id),
            "VP espionage checkpoint player binding changed");
    }
    std::uint64_t count=0;
    for(const auto &[owner,player]:cities.players)
        for(const auto &[id,city]:player.cities){
            (void)city;
            require(count<budget.cities&&find_vp_city_espionage(cities,owner,id),
                "Missing VP espionage checkpoint city profile");
            ++count;
        }
    return count;
}
void write_spy(Writer &out,const VpSpy &spy){
    require(spy.state>=0&&spy.state<=9,"Invalid live VP espionage spy state");
    out.text(spy.name);
    out.i64(spy.rank);out.i64(spy.experience);out.i64(spy.city_x);out.i64(spy.city_y);
    out.i64(spy.state);out.i64(spy.focus);out.i64(spy.revive_counter);
    out.i64(spy.vassal_diplomat_player);out.i64(spy.counterspy_mission_turn);
    out.i64(spy.active_mission_turn);
    out.boolean(spy.diplomat);out.boolean(spy.evaluate_reassignment);out.boolean(spy.passive);
}
VpSpy read_spy(Reader &in){
    VpSpy spy;
    spy.name=in.text();
    spy.rank=in.i64();spy.experience=in.i64();spy.city_x=in.i64();spy.city_y=in.i64();
    spy.state=in.i64();spy.focus=in.i64();spy.revive_counter=in.i64();
    spy.vassal_diplomat_player=in.i64();spy.counterspy_mission_turn=in.i64();
    spy.active_mission_turn=in.i64();
    spy.diplomat=in.boolean();spy.evaluate_reassignment=in.boolean();spy.passive=in.boolean();
    require(spy.state>=0&&spy.state<=9,"Invalid VP espionage checkpoint spy state");
    return spy;
}
void write_notification(Writer &out,const VpSpyNotification &message){
    out.i64(message.city_x);out.i64(message.city_y);out.i64(message.attacking_player);
    out.i64(message.spy_result);out.i64(message.amount_stolen);out.i64(message.stolen_tech);
    out.i64(message.mission);out.i64(message.great_work);
}
VpSpyNotification read_notification(Reader &in){
    VpSpyNotification message;
    message.city_x=in.i64();message.city_y=in.i64();message.attacking_player=in.i64();
    message.spy_result=in.i64();message.amount_stolen=in.i64();message.stolen_tech=in.i64();
    message.mission=in.i64();message.great_work=in.i64();
    return message;
}
void write_intrigue(Writer &out,const VpIntrigue &message){
    out.i64(message.discovering_player);out.i64(message.source_player);
    out.i64(message.target_player);out.i64(message.diplomacy_player);
    out.i64(message.building);out.i64(message.project);out.i64(message.unit);
    out.i64(message.type);out.i64(message.turn);out.i64(message.city_x);out.i64(message.city_y);
    out.text(message.spy_name);out.boolean(message.shared);out.i64(message.spy_id);
}
VpIntrigue read_intrigue(Reader &in){
    VpIntrigue message;
    message.discovering_player=in.i64();message.source_player=in.i64();
    message.target_player=in.i64();message.diplomacy_player=in.i64();
    message.building=in.i64();message.project=in.i64();message.unit=in.i64();
    message.type=in.i64();message.turn=in.i64();message.city_x=in.i64();message.city_y=in.i64();
    message.spy_name=in.text();message.shared=in.boolean();message.spy_id=in.i64();
    return message;
}
} // namespace

saves::CivisSaveSection make_vp_espionage_checkpoint(
    const ResearchContext &setup,const CityYieldContext &cities,
    const SessionCheckpointIdentity &identity,VpEspionageCheckpointBudget budget){
    const auto city_count=validate(setup,cities,identity,budget);
    const auto &players=find_vp_espionage_profile(setup)->players;
    Writer out(budget.bytes);std::uint64_t entries=0;
    out.raw(magic);out.u32(2);out.digest(identity.payload_sha256);out.digest(identity.rules_sha256);
    out.u64(players.size());
    for(const auto &[id,player]:players){
        out.i64(id);
        out.i64(player.extra_rank_bonus);out.i64(player.offensive_strength_modifier);
        out.i64(player.network_points_bonus);out.i64(player.policy_faster_modifier);
        out.i64(player.security_modifier);
        charge(entries,player.cultural_influence_times100.size(),budget.entries);
        out.u64(player.cultural_influence_times100.size());
        for(const auto &[target,value]:player.cultural_influence_times100){out.i64(target);out.i64(value);}
        charge(entries,player.spies.size(),budget.entries);
        out.u64(player.spies.size());
        for(const auto &spy:player.spies)write_spy(out,spy);
        charge(entries,player.spy_messages.size(),budget.entries);
        out.u64(player.spy_messages.size());
        for(const auto &message:player.spy_messages)write_notification(out,message);
        charge(entries,player.intrigue.size(),budget.entries);
        out.u64(player.intrigue.size());
        for(const auto &message:player.intrigue)write_intrigue(out,message);
    }
    out.u64(city_count);
    for(const auto &[owner,player]:cities.players)
        for(const auto &[id,city]:player.cities){
            (void)city;
            const auto &state=*find_vp_city_espionage(cities,owner,id);
            out.i64(owner);out.i64(id);
            out.i64(state.security_modifier);out.i64(state.security_per_x_pop);
            out.i64(state.previous_spy_missions);out.i64(state.happiness_delta);
            charge(entries,state.fields.size(),budget.entries);
            out.u64(state.fields.size());
            for(const auto &[name,values]:state.fields){
                out.text(name);charge(entries,values.size(),budget.entries);out.u64(values.size());
                for(const auto &[target,value]:values){out.i64(target);out.i64(value);}
            }
            charge(entries,state.next_passive_bonus.size(),budget.entries);
            out.u64(state.next_passive_bonus.size());
            for(const auto &[target,next]:state.next_passive_bonus){
                out.i64(target);out.boolean(next.initialized);out.boolean(next.threshold.has_value());
                if(next.threshold)out.i64(*next.threshold);
            }
        }
    return {section_id,0,std::move(out.bytes)};
}

void restore_vp_espionage_checkpoint(
    const saves::CivisSaveSection &section,ResearchContext &setup,CityYieldContext &cities,
    const SessionCheckpointIdentity &identity,VpEspionageCheckpointBudget budget){
    const auto city_count=validate(setup,cities,identity,budget);
    require(section.id==section_id&&section.flags==0&&section.bytes.size()<=budget.bytes,
        "Unsupported VP espionage checkpoint section");
    Reader in(section.bytes);std::uint64_t entries=0;
    const auto head=in.raw(magic.size());
    require(std::equal(head.begin(),head.end(),magic.begin()),
        "Unsupported VP espionage checkpoint magic");
    const auto version=in.u32();
    require(version==1||version==2,
        "Unsupported VP espionage checkpoint version");
    in.digest(identity.payload_sha256,"VP espionage checkpoint source payload changed");
    in.digest(identity.rules_sha256,"VP espionage checkpoint active rules changed");
    auto staged_players=*find_vp_espionage_profile(setup);
    const auto player_count=in.count(budget.players,56);
    require(player_count==staged_players.players.size(),"VP espionage checkpoint player roster changed");
    std::set<std::int64_t> seen_players;
    for(std::uint64_t i=0;i<player_count;++i){
        const auto id=in.i64();
        require(staged_players.players.contains(id)&&seen_players.insert(id).second,
            "Unknown or duplicate VP espionage checkpoint player");
        auto &player=staged_players.players.at(id);
        player.extra_rank_bonus=in.i64();player.offensive_strength_modifier=in.i64();
        player.network_points_bonus=in.i64();player.policy_faster_modifier=in.i64();
        player.security_modifier=in.i64();
        player.cultural_influence_times100.clear();
        const auto influences=in.count(budget.entries-entries,16);charge(entries,influences,budget.entries);
        for(std::uint64_t j=0;j<influences;++j){
            const auto target=in.i64(),value=in.i64();
            require(player.cultural_influence_times100.emplace(target,value).second,
                "Duplicate VP espionage checkpoint influence target");
        }
        player.spies.clear();
        const auto spies=in.count(budget.entries-entries,8);charge(entries,spies,budget.entries);
        for(std::uint64_t j=0;j<spies;++j)player.spies.push_back(read_spy(in));
        if(version>=2){
            player.spy_messages.clear();
            const auto messages=in.count(budget.entries-entries,64);
            charge(entries,messages,budget.entries);
            for(std::uint64_t j=0;j<messages;++j)
                player.spy_messages.push_back(read_notification(in));
            player.intrigue.clear();
            const auto intrigues=in.count(budget.entries-entries,105);
            charge(entries,intrigues,budget.entries);
            for(std::uint64_t j=0;j<intrigues;++j)
                player.intrigue.push_back(read_intrigue(in));
        }
    }
    const auto saved_cities=in.count(budget.cities,56);
    require(saved_cities==city_count,"VP espionage checkpoint city roster changed");
    std::map<CityKey,VpCityEspionageState> staged_cities;
    for(std::uint64_t i=0;i<saved_cities;++i){
        const auto owner=in.i64(),id=in.i64();
        const auto *live=find_vp_city_espionage(cities,owner,id);
        require(live&&!staged_cities.contains({owner,id}),
            "Unknown or duplicate VP espionage checkpoint city");
        auto state=*live;
        state.security_modifier=in.i64();state.security_per_x_pop=in.i64();
        state.previous_spy_missions=in.i64();state.happiness_delta=in.i64();
        state.fields.clear();
        const auto fields=in.count(budget.entries-entries,8);charge(entries,fields,budget.entries);
        for(std::uint64_t j=0;j<fields;++j){
            const auto name=in.text();
            auto [field,inserted]=state.fields.emplace(name,std::map<std::int64_t,std::int64_t>{});
            require(inserted,"Duplicate VP espionage checkpoint city field");
            const auto values=in.count(budget.entries-entries,16);charge(entries,values,budget.entries);
            for(std::uint64_t k=0;k<values;++k){
                const auto target=in.i64(),value=in.i64();
                require(field->second.emplace(target,value).second,
                    "Duplicate VP espionage checkpoint city field target");
            }
        }
        for(const auto &[name,values]:live->fields){
            (void)values;
            require(state.fields.contains(name),"Missing source VP espionage checkpoint city field");
        }
        state.next_passive_bonus.clear();
        const auto next_count=in.count(budget.entries-entries,10);charge(entries,next_count,budget.entries);
        for(std::uint64_t j=0;j<next_count;++j){
            const auto target=in.i64();
            const auto initialized=in.boolean(),has_threshold=in.boolean();
            VpCityEspionageState::NextPassiveBonus next{initialized,{}};
            if(has_threshold)next.threshold=in.i64();
            require(state.next_passive_bonus.emplace(target,next).second,
                "Duplicate VP espionage checkpoint passive target");
        }
        for(const auto &[target,next]:live->next_passive_bonus){
            (void)next;
            require(state.next_passive_bonus.contains(target),
                "Missing source VP espionage checkpoint passive target");
        }
        staged_cities.emplace(CityKey{owner,id},std::move(state));
    }
    require(in.remaining()==0,"Trailing VP espionage checkpoint bytes");
    ensure_vp_espionage_profile(setup)=std::move(staged_players);
    for(auto &[key,state]:staged_cities)
        ensure_vp_city_espionage(cities,key.first,key.second)=std::move(state);
}
} // namespace civ5::runtime
