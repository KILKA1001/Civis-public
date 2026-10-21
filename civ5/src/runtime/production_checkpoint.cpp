#include "civ5/runtime/production_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
using Profile=content::TechnologyProfile;
using Progress=CityYieldState::ProductionProgress;
using Order=CityYieldState::ProductionOrder;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','P','R','D','0','1'};
constexpr std::uint32_t section_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_city_production);
void require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
std::uint32_t profile_wire(Profile p){
    if(p==Profile::bnw)return 1;
    if(p==Profile::vp542)return 2;
    throw std::runtime_error("Unsupported production checkpoint profile");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    std::uint64_t limit;
    explicit Writer(std::uint64_t maximum):limit(maximum){}
    void raw(std::span<const std::uint8_t> value){
        require(value.size()<=limit-bytes.size(),"Production checkpoint exceeds byte budget");
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void u8(std::uint8_t value){raw(std::span(&value,1));}
    void u32(std::uint32_t value){for(unsigned i=0;i<4;++i)u8(static_cast<std::uint8_t>(value>>(8*i)));}
    void u64(std::uint64_t value){for(unsigned i=0;i<8;++i)u8(static_cast<std::uint8_t>(value>>(8*i)));}
    void i64(std::int64_t value){u64(std::bit_cast<std::uint64_t>(value));}
    void string(std::string_view value){u64(value.size());raw({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});}
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    explicit Reader(std::span<const std::uint8_t> source):bytes(source){}
    std::size_t remaining()const{return bytes.size()-at;}
    std::span<const std::uint8_t> raw(std::size_t n){
        require(n<=remaining(),"Truncated production checkpoint");
        const auto result=bytes.subspan(at,n);at+=n;return result;
    }
    std::uint8_t u8(){return raw(1)[0];}
    bool boolean(){const auto n=u8();require(n<=1,"Invalid production checkpoint boolean");return n!=0;}
    std::uint32_t u32(){std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(u8())<<(8*i);return v;}
    std::uint64_t u64(){std::uint64_t v=0;for(unsigned i=0;i<8;++i)v|=std::uint64_t(u8())<<(8*i);return v;}
    std::int64_t i64(){return std::bit_cast<std::int64_t>(u64());}
};
std::string fingerprint(const ProductionProgressRules &rules,const content::InfoTypeCatalog &types){
    require(types.report().all_succeeded(),"Unresolved production checkpoint Info catalog");
    Writer w(128ULL*1024*1024);
    w.u32(profile_wire(rules.profile));w.u64(rules.unit_slots);w.u64(rules.building_slots);w.u64(rules.project_slots);
    const auto ids=[&](const auto &set){w.u64(set.size());for(auto id:set)w.i64(id);};
    ids(rules.units);ids(rules.buildings);ids(rules.projects);ids(rules.processes);ids(rules.specialists);
    w.u64(rules.info_identity.size());
    for(const auto &[table,rows]:rules.info_identity){
        w.string(table);w.u64(rows.size());
        for(const auto &[id,type]:rows){w.i64(id);w.u8(type.has_value());if(type)w.string(*type);}
    }
    std::uint64_t n=0;
    const auto selected=[](std::string_view table){return table=="Units"||table=="Buildings"||table=="Projects"||
        table=="Processes"||table=="Specialists";};
    for(const auto &[name,entry]:types.entries())if(selected(entry.table))++n;
    w.u64(n);
    for(const auto &[name,entry]:types.entries())if(selected(entry.table)){
        w.string(name);w.string(entry.table);w.i64(entry.id);w.u32(entry.hash);
    }
    return identify_bytes(w.bytes).sha256;
}
void pairs(Writer &w,const YieldValues &rows,std::uint64_t &entries,const ProductionCheckpointBudget &budget){
    require(rows.size()<=budget.entries-entries,"Production checkpoint entry budget exceeded");
    entries+=rows.size();w.u64(rows.size());for(const auto &[id,value]:rows){w.i64(id);w.i64(value);}
}
void hashes(Writer &w,const Progress::HashedRows &rows,std::uint64_t &entries,const ProductionCheckpointBudget &budget){
    require(rows.size()<=budget.entries-entries,"Production checkpoint entry budget exceeded");
    entries+=rows.size();w.u64(rows.size());for(const auto &[hash,value]:rows){w.u32(hash);w.i64(value);}
}
std::uint64_t count(Reader &r,std::uint64_t &used,std::uint64_t maximum,std::size_t width){
    const auto n=r.u64();require(n<=maximum-used&&n<=r.remaining()/width,"Oversized production checkpoint collection");
    used+=n;return n;
}
void pairs(Reader &r,YieldValues &rows,std::uint64_t &entries,const ProductionCheckpointBudget &budget){
    const auto n=count(r,entries,budget.entries,16);
    for(std::uint64_t i=0;i<n;++i){const auto id=r.i64(),value=r.i64();
        require(rows.emplace(id,value).second,"Duplicate production checkpoint Info ID");}
}
void hashes(Reader &r,Progress::HashedRows &rows,std::uint64_t &entries,const ProductionCheckpointBudget &budget){
    const auto n=count(r,entries,budget.entries,12);
    for(std::uint64_t i=0;i<n;++i){const auto hash=r.u32();const auto value=r.i64();rows.emplace_back(hash,value);}
}
struct StagedCity {
    std::int64_t owner=0,id=0;
    std::uint64_t start=0,end=0;
    Progress progress;
    std::vector<Order> orders;
};
} // namespace
saves::CivisSaveSection make_production_checkpoint(const CityYieldContext &world,
    const ProductionProgressRules &rules,const content::InfoTypeCatalog &types,ProductionCheckpointBudget budget){
    require(world.profile==rules.profile,"Production checkpoint profile mismatch");
    require(world.players.size()<=budget.players,"Production checkpoint player budget exceeded");
    Writer w(budget.bytes);w.raw(magic);w.u32(1);w.u32(profile_wire(rules.profile));
    const auto signature=fingerprint(rules,types);
    w.raw({reinterpret_cast<const std::uint8_t*>(signature.data()),signature.size()});
    w.u64(world.players.size());std::uint64_t cities=0,entries=0;
    for(const auto &[owner,player]:world.players){
        w.i64(owner);require(player.cities.size()<=budget.cities-cities,"Production checkpoint city budget exceeded");
        cities+=player.cities.size();w.u64(player.cities.size());
        for(const auto &[id,city]:player.cities){
            require(city.owner==owner&&city.id==id&&city.source_core&&city.production_progress&&city.production_orders,
                "Incomplete live city production checkpoint");
            const auto &p=*city.production_progress;
            require(p.source_profile==rules.profile,"City production checkpoint profile mismatch");
            require(!p.fully_resolved || (p.unresolved_units.empty()&&p.unresolved_unit_times.empty()&&
                p.unresolved_buildings.empty()&&p.unresolved_building_times.empty()),
                "Resolved production checkpoint contains unknown hashes");
            w.i64(id);w.u64(city.source_core->first);w.u64(city.source_core->second);
            w.i64(p.overflow_times100);w.i64(p.feature_times100);w.u8(p.fully_resolved);
            pairs(w,p.units,entries,budget);pairs(w,p.unit_times,entries,budget);
            pairs(w,p.buildings,entries,budget);pairs(w,p.building_times,entries,budget);pairs(w,p.projects,entries,budget);
            hashes(w,p.unresolved_units,entries,budget);hashes(w,p.unresolved_unit_times,entries,budget);
            hashes(w,p.unresolved_buildings,entries,budget);hashes(w,p.unresolved_building_times,entries,budget);
            require(city.production_orders->size()<=budget.entries-entries,"Production checkpoint order budget exceeded");
            entries+=city.production_orders->size();w.u64(city.production_orders->size());
            for(const auto &order:*city.production_orders){
                w.i64(order.type);w.i64(order.item);w.i64(order.secondary);w.u8(order.save);w.u8(order.rush);
                w.u8(order.source_hash.has_value());if(order.source_hash)w.u32(*order.source_hash);
            }
        }
    }
    return {section_id,0,std::move(w.bytes)};
}
void restore_production_checkpoint(const saves::CivisSaveSection &section,
    const ProductionProgressRules &rules,const content::InfoTypeCatalog &types,CityYieldContext &world,
    ProductionCheckpointBudget budget){
    require(section.id==section_id&&section.flags==0&&section.bytes.size()<=budget.bytes,
        "Unsupported production checkpoint section");
    Reader r(section.bytes);
    const auto head=r.raw(magic.size());require(std::equal(head.begin(),head.end(),magic.begin())&&r.u32()==1,
        "Unsupported production checkpoint version");
    require(r.u32()==profile_wire(rules.profile)&&world.profile==rules.profile,
        "Production checkpoint profile mismatch");
    const auto signature=fingerprint(rules,types);
    const auto stored=r.raw(signature.size());
    require(std::equal(stored.begin(),stored.end(),signature.begin()),"Production checkpoint Info mapping changed");
    std::uint64_t players=0,cities=0,entries=0;
    const auto player_count=count(r,players,budget.players,16);
    require(player_count==world.players.size(),"Production checkpoint player roster mismatch");
    std::map<std::pair<std::int64_t,std::int64_t>,StagedCity> staged;
    std::set<std::int64_t> owners;
    for(std::uint64_t i=0;i<player_count;++i){
        const auto owner=r.i64();require(owners.insert(owner).second&&world.players.contains(owner),
            "Production checkpoint player identity mismatch");
        const auto city_count=count(r,cities,budget.cities,24);
        require(city_count==world.players.at(owner).cities.size(),"Production checkpoint city roster mismatch");
        for(std::uint64_t j=0;j<city_count;++j){
            StagedCity c;c.owner=owner;c.id=r.i64();c.start=r.u64();c.end=r.u64();
            c.progress.source_profile=rules.profile;
            c.progress.overflow_times100=r.i64();c.progress.feature_times100=r.i64();c.progress.fully_resolved=r.boolean();
            pairs(r,c.progress.units,entries,budget);pairs(r,c.progress.unit_times,entries,budget);
            pairs(r,c.progress.buildings,entries,budget);pairs(r,c.progress.building_times,entries,budget);
            pairs(r,c.progress.projects,entries,budget);
            hashes(r,c.progress.unresolved_units,entries,budget);hashes(r,c.progress.unresolved_unit_times,entries,budget);
            hashes(r,c.progress.unresolved_buildings,entries,budget);hashes(r,c.progress.unresolved_building_times,entries,budget);
            require(!c.progress.fully_resolved || (c.progress.unresolved_units.empty()&&
                c.progress.unresolved_unit_times.empty()&&c.progress.unresolved_buildings.empty()&&
                c.progress.unresolved_building_times.empty()),"Resolved checkpoint has unknown hashes");
            const auto order_count=count(r,entries,budget.entries,27);
            for(std::uint64_t k=0;k<order_count;++k){
                Order order;order.type=r.i64();order.item=r.i64();order.secondary=r.i64();
                order.save=r.boolean();order.rush=r.boolean();
                if(r.boolean())order.source_hash=r.u32();
                c.orders.push_back(order);
            }
            const auto &city=world.players.at(owner).cities.at(c.id);
            require(city.owner==owner&&city.id==c.id&&city.source_core==
                std::optional{std::pair{c.start,c.end}},"Production checkpoint source city mismatch");
            require(staged.emplace(std::pair{owner,c.id},std::move(c)).second,
                "Duplicate production checkpoint city");
        }
    }
    require(r.remaining()==0&&staged.size()==cities,"Trailing or incomplete production checkpoint");
    for(auto &[key,c]:staged){auto &city=world.players.at(c.owner).cities.at(c.id);
        city.production_progress=std::move(c.progress);city.production_orders=std::move(c.orders);}
}
} // namespace civ5::runtime
