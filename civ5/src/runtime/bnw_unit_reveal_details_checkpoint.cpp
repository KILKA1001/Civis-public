#include "civ5/runtime/bnw_unit_reveal_details_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','R','D','T','1'};
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *s) {return s?std::string_view(s):std::string_view();}
void hash_shape(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW reveal-detail SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u8(std::uint8_t v) {bytes.push_back(v);}
    void u32(std::uint32_t v) {for(unsigned i=0;i<4;++i) u8(v>>(8*i));}
    void u64(std::uint64_t v) {for(unsigned i=0;i<8;++i) u8(v>>(8*i));}
    void i64(std::int64_t v) {u64(std::bit_cast<std::uint64_t>(v));}
    void raw(std::string_view s) {bytes.insert(bytes.end(),s.begin(),s.end());}
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint8_t u8() {
        require(at<bytes.size(),"Truncated BNW reveal-detail checkpoint");
        return bytes[at++];
    }
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,
                "Truncated BNW reveal-detail checkpoint");
        std::uint32_t v=0;
        for(unsigned i=0;i<4;++i) v|=std::uint32_t(u8())<<(8*i);
        return v;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,
                "Truncated BNW reveal-detail checkpoint");
        std::uint64_t v=0;
        for(unsigned i=0;i<8;++i) v|=std::uint64_t(u8())<<(8*i);
        return v;
    }
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    std::string_view raw(std::size_t n) {
        require(at<=bytes.size() && bytes.size()-at>=n,
                "Truncated BNW reveal-detail digest");
        const auto *p=reinterpret_cast<const char *>(bytes.data()+at);
        at+=n;return {p,n};
    }
};
const saves::PlotArrayObservation &array(
    const std::vector<saves::PlotArrayObservation> &arrays,
    std::string_view key,std::size_t width) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &item:arrays) if(name(item.name)==key) {
        require(!found,"Duplicate BNW reveal-detail array");found=&item;
    }
    require(found && found->columns==1 && found->values.size()==width,
            "Missing or malformed BNW reveal-detail array");
    return *found;
}
saves::PlotArrayObservation &array(
    std::vector<saves::PlotArrayObservation> &arrays,
    std::string_view key,std::size_t width) {
    (void)array(static_cast<const std::vector<saves::PlotArrayObservation>&>(
        arrays),key,width);
    for(auto &item:arrays) if(name(item.name)==key) return item;
    throw std::runtime_error("Missing BNW reveal-detail array");
}
std::string_view improvement_key(const saves::PlotObservation &plot) {
    require(plot.version.has_value(),"BNW reveal-detail plot version missing");
    return *plot.version>=6?"revealed_improvement_hashes":
                              "revealed_improvements";
}
std::int64_t city_scalar(const saves::SavedCityObservation &city,
                         std::string_view key) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &item:city.core.fields) if(name(item.name)==key) {
        require(!found,"Duplicate BNW reveal-detail city scalar");found=&item;
    }
    require(found && found->encoding==saves::CityCoreEncoding::i32 &&
            found->values.size()==1,
            "Missing BNW reveal-detail city scalar");
    return found->values.front();
}
const std::vector<std::int64_t> &city_revealed(
    const saves::SavedCityObservation &city) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &item:city.core.fields)
        if(name(item.name)=="m_abRevealed") {
            require(!found,"Duplicate BNW reveal-detail city visibility");
            found=&item;
        }
    require(found && found->encoding==saves::CityCoreEncoding::vector_bool &&
            found->values.size()==80,
            "Missing BNW reveal-detail city visibility");
    for(const auto v:found->values)
        require(v==0 || v==1,"Invalid BNW city reveal flag");
    return found->values;
}
std::vector<std::int64_t> &city_revealed(saves::SavedCityObservation &city) {
    (void)city_revealed(static_cast<const saves::SavedCityObservation&>(city));
    for(auto &item:city.core.fields)
        if(name(item.name)=="m_abRevealed") return item.values;
    throw std::runtime_error("Missing BNW city reveal field");
}
void shape(const saves::SavedSessionSnapshot &world,std::int64_t team) {
    require(team>=0 && team<64 && world.map_header.layout==bnw &&
            world.map_plots && world.map_plots->layout==bnw &&
            world.map_plots->plots.size()==world.map_header.grid.size() &&
            world.teams && world.teams->layout==bnw &&
            world.teams->teams.size()==64 && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->players.size()==64,
            "BNW reveal-detail world or roster is incomplete");
    for(std::size_t i=0;i<world.map_plots->plots.size();++i) {
        const auto &plot=world.map_plots->plots[i];
        const auto xy=world.map_header.grid.coordinates(i);
        require(plot.index==i && plot.x==xy[0] && plot.y==xy[1],
                "BNW reveal-detail plot roster changed");
        (void)array(plot.arrays,"revealed_owners",80);
        (void)array(plot.arrays,improvement_key(plot),80);
        (void)array(plot.arrays,"revealed_routes",80);
    }
    for(std::size_t t=0;t<64;++t) {
        const auto &saved=world.teams->teams[t];
        require(saved.index==t,"BNW reveal-detail team roster changed");
        const auto &flags=array(saved.arrays,"found_player_territory",64).values;
        for(const auto flag:flags)
            require(flag==0 || flag==1,"Invalid BNW territory-found flag");
    }
    for(std::size_t p=0;p<64;++p) {
        const auto &player=world.all_players->players[p];
        require(player.index==p && player.saved_player_id==
                    static_cast<std::int64_t>(p),
                "BNW reveal-detail player roster changed");
        std::set<std::int64_t> city_ids;
        for(const auto &city:player.cities.cities) {
            require(city.core.layout==bnw &&
                    city_scalar(city,"m_eOwner")==static_cast<std::int64_t>(p) &&
                    city_ids.insert(city_scalar(city,"m_iID")).second,
                    "BNW reveal-detail city roster changed");
            (void)city_scalar(city,"m_iX");(void)city_scalar(city,"m_iY");
            (void)city_revealed(city);
        }
    }
}
std::string digest(const saves::SavedSessionSnapshot &world,std::int64_t team) {
    shape(world,team);
    Writer out;
    out.u64(world.map_header.grid.width);out.u64(world.map_header.grid.height);
    out.u64(world.map_plots->plots.size());out.i64(team);
    const auto slot=static_cast<std::size_t>(team);
    for(const auto &plot:world.map_plots->plots) {
        out.u64(plot.index);out.u64(*plot.version);
        out.i64(array(plot.arrays,"revealed_owners",80).values[slot]);
        out.i64(array(plot.arrays,improvement_key(plot),80).values[slot]);
        out.i64(array(plot.arrays,"revealed_routes",80).values[slot]);
    }
    for(const auto &player:world.all_players->players) {
        out.i64(player.saved_player_id);out.u64(player.cities.cities.size());
        for(const auto &city:player.cities.cities) {
            for(const auto key:{"m_eOwner","m_iID","m_iX","m_iY"})
                out.i64(city_scalar(city,key));
            out.i64(city_revealed(city)[slot]);
        }
    }
    const auto &flags=array(world.teams->teams[slot].arrays,
                            "found_player_territory",64).values;
    for(const auto flag:flags) out.i64(flag);
    return identify_bytes(out.bytes).sha256;
}
struct PlotDelta {std::uint64_t index;std::int64_t owner,improvement,route;};
struct CityDelta {std::uint32_t player,index;std::int64_t id,revealed;};
std::vector<PlotDelta> plot_deltas(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,std::int64_t team) {
    require(world.map_header.grid.width==source.map_header.grid.width &&
            world.map_header.grid.height==source.map_header.grid.height,
            "BNW reveal-detail map dimensions changed");
    std::vector<PlotDelta> result;
    const auto slot=static_cast<std::size_t>(team);
    for(std::size_t i=0;i<source.map_plots->plots.size();++i) {
        const auto &a=source.map_plots->plots[i],&b=world.map_plots->plots[i];
        require(a.index==b.index && a.version==b.version,
                "BNW reveal-detail plot identity changed");
        const auto own=std::pair{
            &array(a.arrays,"revealed_owners",80).values,
            &array(b.arrays,"revealed_owners",80).values};
        const auto imp=std::pair{
            &array(a.arrays,improvement_key(a),80).values,
            &array(b.arrays,improvement_key(b),80).values};
        const auto route=std::pair{
            &array(a.arrays,"revealed_routes",80).values,
            &array(b.arrays,"revealed_routes",80).values};
        for(std::size_t t=0;t<80;++t) if(t!=slot)
            require((*own.first)[t]==(*own.second)[t] &&
                    (*imp.first)[t]==(*imp.second)[t] &&
                    (*route.first)[t]==(*route.second)[t],
                    "BNW reveal-detail changed another team plot slot");
        if((*own.first)[slot]==(*own.second)[slot] &&
           (*imp.first)[slot]==(*imp.second)[slot] &&
           (*route.first)[slot]==(*route.second)[slot]) continue;
        require((*own.second)[slot]>=-1 && (*own.second)[slot]<64 &&
                (*route.second)[slot]>=-1 &&
                (*route.second)[slot]<=INT32_MAX,
                "BNW reveal-detail target owner/route invalid");
        result.push_back({i,(*own.second)[slot],(*imp.second)[slot],
                          (*route.second)[slot]});
    }
    return result;
}
std::vector<CityDelta> city_deltas(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,std::int64_t team) {
    std::vector<CityDelta> result;
    const auto slot=static_cast<std::size_t>(team);
    for(std::size_t p=0;p<64;++p) {
        const auto &a=source.all_players->players[p].cities.cities;
        const auto &b=world.all_players->players[p].cities.cities;
        require(a.size()==b.size(),"BNW reveal-detail city count changed");
        for(std::size_t i=0;i<a.size();++i) {
            for(const auto key:{"m_eOwner","m_iID","m_iX","m_iY"})
                require(city_scalar(a[i],key)==city_scalar(b[i],key),
                        "BNW reveal-detail city identity changed");
            const auto &old=city_revealed(a[i]),&now=city_revealed(b[i]);
            for(std::size_t t=0;t<80;++t) if(t!=slot)
                require(old[t]==now[t],
                        "BNW reveal-detail changed another team city slot");
            if(old[slot]!=now[slot]) {
                require(i<=UINT32_MAX,
                        "BNW reveal-detail city index exceeds wire range");
                result.push_back({static_cast<std::uint32_t>(p),
                    static_cast<std::uint32_t>(i),city_scalar(b[i],"m_iID"),
                    now[slot]});
            }
        }
    }
    return result;
}
std::vector<std::pair<std::uint32_t,std::int64_t>> territory_deltas(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,std::int64_t team) {
    const auto slot=static_cast<std::size_t>(team);
    for(std::size_t t=0;t<64;++t) if(t!=slot)
        require(array(world.teams->teams[t].arrays,"found_player_territory",64).values==
                array(source.teams->teams[t].arrays,"found_player_territory",64).values,
                "BNW reveal-detail changed another team's territory flags");
    const auto &a=array(source.teams->teams[slot].arrays,
                        "found_player_territory",64).values;
    const auto &b=array(world.teams->teams[slot].arrays,
                        "found_player_territory",64).values;
    std::vector<std::pair<std::uint32_t,std::int64_t>> result;
    for(std::uint32_t i=0;i<64;++i)
        if(a[i]!=b[i]) result.emplace_back(i,b[i]);
    return result;
}
} // namespace

saves::CivisSaveSection make_bnw_unit_reveal_details_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,std::int64_t team) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    shape(source,team);shape(world,team);
    const auto plots=plot_deltas(world,source,team);
    const auto cities=city_deltas(world,source,team);
    const auto territory=territory_deltas(world,source,team);
    require(plots.size()<=UINT32_MAX && cities.size()<=UINT32_MAX,
            "BNW reveal-detail delta count exceeds wire range");
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(digest(source,team));out.raw(digest(world,team));
    out.u64(source.map_header.grid.width);out.u64(source.map_header.grid.height);
    out.i64(team);
    out.u32(static_cast<std::uint32_t>(plots.size()));
    out.u32(static_cast<std::uint32_t>(cities.size()));
    out.u32(static_cast<std::uint32_t>(territory.size()));
    for(const auto &d:plots) {
        out.u64(d.index);out.i64(d.owner);out.i64(d.improvement);out.i64(d.route);
    }
    for(const auto &d:cities) {
        out.u32(d.player);out.u32(d.index);out.i64(d.id);out.i64(d.revealed);
    }
    for(const auto &[owner,found]:territory) {out.u32(owner);out.i64(found);}
    return {bnw_unit_reveal_details_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_unit_reveal_details_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(section.id==bnw_unit_reveal_details_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+64*4+8*3+4*3 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW reveal-detail section");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW reveal-detail section version");
    require(in.raw(64)==identity.payload_sha256 &&
            in.raw(64)==identity.rules_sha256,
            "BNW reveal-detail payload or rules identity changed");
    const auto source_hash=in.raw(64),target_hash=in.raw(64);
    hash_shape(source_hash);hash_shape(target_hash);
    const auto width=in.u64(),height=in.u64();
    const auto team=in.i64();
    const auto plot_count=in.u32(),city_count=in.u32(),territory_count=in.u32();
    require(team>=0 && team<64 && width==world.map_header.grid.width &&
            height==world.map_header.grid.height &&
            plot_count<=world.map_header.grid.size() &&
            territory_count<=64 && world.all_players &&
            city_count<=world.map_header.grid.size(),
            "BNW reveal-detail dimensions or team changed");
    require(source_hash==digest(world,team),
            "BNW reveal-detail source state digest changed");
    auto replacement=world;
    std::uint64_t previous_plot=0;
    for(std::uint32_t i=0;i<plot_count;++i) {
        const auto index=in.u64();
        const auto owner=in.i64(),improvement=in.i64(),route=in.i64();
        require(index<replacement.map_header.grid.size() &&
                (i==0 || index>previous_plot) && owner>=-1 && owner<64 &&
                route>=-1 && route<=INT32_MAX,
                "Malformed BNW reveal-detail plot delta");
        previous_plot=index;
        auto &plot=replacement.map_plots->plots[index];
        const auto slot=static_cast<std::size_t>(team);
        array(plot.arrays,"revealed_owners",80).values[slot]=owner;
        array(plot.arrays,improvement_key(plot),80).values[slot]=improvement;
        array(plot.arrays,"revealed_routes",80).values[slot]=route;
    }
    std::pair<std::uint32_t,std::uint32_t> previous_city{};
    for(std::uint32_t i=0;i<city_count;++i) {
        const auto player=in.u32(),index=in.u32();
        const auto id=in.i64(),value=in.i64();
        require(player<64 &&
                index<replacement.all_players->players[player].cities.cities.size() &&
                (i==0 || std::pair{player,index}>previous_city) &&
                (value==0 || value==1),
                "Malformed BNW reveal-detail city delta");
        previous_city={player,index};
        auto &city=replacement.all_players->players[player].cities.cities[index];
        require(city_scalar(city,"m_iID")==id,
                "BNW reveal-detail city ID changed");
        city_revealed(city)[static_cast<std::size_t>(team)]=value;
    }
    std::uint32_t previous_owner=0;
    for(std::uint32_t i=0;i<territory_count;++i) {
        const auto owner=in.u32();const auto value=in.i64();
        require(owner<64 && (i==0 || owner>previous_owner) &&
                (value==0 || value==1),
                "Malformed BNW reveal-detail territory delta");
        previous_owner=owner;
        array(replacement.teams->teams[static_cast<std::size_t>(team)].arrays,
              "found_player_territory",64).values[owner]=value;
    }
    require(in.at==section.bytes.size(),
            "BNW reveal-detail checkpoint has trailing bytes");
    (void)plot_deltas(replacement,world,team);
    (void)city_deltas(replacement,world,team);
    (void)territory_deltas(replacement,world,team);
    require(target_hash==digest(replacement,team),
            "BNW reveal-detail target state digest changed");
    world=std::move(replacement);
}
} // namespace civ5::runtime
