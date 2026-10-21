#include "civ5/runtime/bnw_unit_sight_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <span>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','S','G','T','1'};
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void check_hash(std::string_view value) {
    require(value.size()==64 && std::all_of(value.begin(),value.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW sight checkpoint SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u8(std::uint8_t value) {bytes.push_back(value);}
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i) u8(static_cast<std::uint8_t>(value>>(i*8)));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i) u8(static_cast<std::uint8_t>(value>>(i*8)));
    }
    void i64(std::int64_t value) {u64(std::bit_cast<std::uint64_t>(value));}
    void raw(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint8_t u8() {
        require(at<bytes.size(),"Truncated BNW sight checkpoint");
        return bytes[at++];
    }
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,
                "Truncated BNW sight checkpoint");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(u8())<<(i*8);
        return value;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,
                "Truncated BNW sight checkpoint");
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i) value|=std::uint64_t(u8())<<(i*8);
        return value;
    }
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    std::string_view raw(std::size_t count) {
        require(at<=bytes.size() && count<=bytes.size()-at,
                "Truncated BNW sight checkpoint payload");
        auto result=std::string_view(
            reinterpret_cast<const char *>(bytes.data()+at),count);
        at+=count;return result;
    }
    void expect(std::string_view expected) {
        require(raw(expected.size())==expected,
                "BNW sight source, rules, or digest changed");
    }
};
const saves::PlotArrayObservation &array(
    const std::vector<saves::PlotArrayObservation> &arrays,
    std::string_view wanted,std::size_t width) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &candidate:arrays)
        if(name(candidate.name)==wanted) {
            require(!found,"Duplicate BNW sight array");
            found=&candidate;
        }
    require(found && found->columns==1 && found->values.size()==width,
            "BNW sight array is missing or malformed");
    return *found;
}
saves::PlotArrayObservation &array(
    std::vector<saves::PlotArrayObservation> &arrays,
    std::string_view wanted,std::size_t width) {
    (void)array(static_cast<const std::vector<saves::PlotArrayObservation>&>(
        arrays),wanted,width);
    for(auto &candidate:arrays)
        if(name(candidate.name)==wanted) return candidate;
    throw std::runtime_error("Missing BNW sight array");
}
std::int64_t scalar(const saves::MapRegionRecord &record,
                    std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &field:record.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW sight area scalar");
            found=&field;
        }
    require(found,"Missing BNW sight area scalar");return found->value;
}
const saves::MapRegionContainer &areas(const saves::MapRegionsObservation &regions) {
    const saves::MapRegionContainer *found=nullptr;
    for(const auto &container:regions.containers)
        if(name(container.name)=="areas") {
            require(!found,"Duplicate BNW sight area container");
            found=&container;
        }
    require(found && found->wire_count>=0 &&
            static_cast<std::size_t>(found->wire_count)==found->records.size(),
            "Missing or malformed BNW sight area container");
    return *found;
}
saves::MapRegionContainer &areas(saves::MapRegionsObservation &regions) {
    (void)areas(static_cast<const saves::MapRegionsObservation &>(regions));
    for(auto &container:regions.containers)
        if(name(container.name)=="areas") return container;
    throw std::runtime_error("Missing BNW sight area container");
}
void shape(const saves::SavedSessionSnapshot &world,std::int64_t team) {
    require(team>=0 && team<64 && world.game_state.layout==bnw &&
            world.map_header.layout==bnw && world.map_plots &&
            world.map_plots->layout==bnw && world.map_projection &&
            world.map_regions && world.map_regions->layout==bnw &&
            world.map_plots->plots.size()==world.map_header.grid.size() &&
            world.map_projection->cells.size()==world.map_header.grid.size(),
            "BNW sight requires complete map/areas and a civil team");
    const auto slot=static_cast<std::size_t>(team);
    const auto word=slot/32;
    const auto bit=std::uint32_t{1}<<(slot%32);
    for(std::size_t i=0;i<world.map_plots->plots.size();++i) {
        const auto &plot=world.map_plots->plots[i];
        const auto &cell=world.map_projection->cells[i];
        const auto xy=world.map_header.grid.coordinates(i);
        require(plot.index==i && plot.x==xy[0] && plot.y==xy[1] &&
                cell.index==i && cell.x==xy[0] && cell.y==xy[1] &&
                cell.revealed_teams,
                "BNW sight plot identity or reveal projection changed");
        (void)array(plot.arrays,"visibility",80);
        (void)array(plot.arrays,"invisible_visibility",80);
        const auto &words=array(plot.arrays,"revealed_words",4).values;
        require(words[word]>=0 && words[word]<=UINT32_MAX &&
                ((static_cast<std::uint32_t>(words[word])&bit)!=0)==
                    cell.revealed_teams->contains(team),
                "BNW sight raw/projected reveal flags disagree");
    }
    const auto &container=areas(*world.map_regions);
    for(const auto &record:container.records)
        (void)array(record.arrays,"revealed_tiles",80);
}
std::string digest(const saves::SavedSessionSnapshot &world,std::int64_t team) {
    shape(world,team);
    Writer out;
    out.u64(world.map_header.grid.width);out.u64(world.map_header.grid.height);
    out.u64(world.map_plots->plots.size());
    for(std::size_t i=0;i<world.map_plots->plots.size();++i) {
        const auto &plot=world.map_plots->plots[i];
        for(auto wanted:{"visibility","invisible_visibility","revealed_words"}) {
            const auto &values=array(plot.arrays,wanted,
                std::string_view(wanted)=="revealed_words"?4:80).values;
            for(const auto value:values) out.i64(value);
        }
        const auto &revealed=*world.map_projection->cells[i].revealed_teams;
        out.u32(static_cast<std::uint32_t>(revealed.size()));
        for(const auto value:revealed) out.i64(value);
    }
    const auto &container=areas(*world.map_regions);
    out.u64(container.records.size());
    for(const auto &record:container.records) {
        out.i64(scalar(record,"id"));
        for(const auto value:array(record.arrays,"revealed_tiles",80).values)
            out.i64(value);
    }
    return identify_bytes(out.bytes).sha256;
}
struct PlotDelta {
    std::uint64_t plot=0;
    std::int64_t visibility=0,invisible=0,word=0;
};
struct AreaDelta {
    std::uint64_t record=0;
    std::int64_t id=0,revealed=0;
};
std::vector<PlotDelta> plot_deltas(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,std::int64_t team) {
    require(world.map_header.grid.width==source.map_header.grid.width &&
            world.map_header.grid.height==source.map_header.grid.height,
            "BNW sight map dimensions changed");
    const auto slot=static_cast<std::size_t>(team),word=slot/32;
    std::vector<PlotDelta> result;
    for(std::size_t i=0;i<source.map_plots->plots.size();++i) {
        const auto &a=source.map_plots->plots[i],&b=world.map_plots->plots[i];
        const auto &av=array(a.arrays,"visibility",80).values;
        const auto &bv=array(b.arrays,"visibility",80).values;
        const auto &ai=array(a.arrays,"invisible_visibility",80).values;
        const auto &bi=array(b.arrays,"invisible_visibility",80).values;
        const auto &aw=array(a.arrays,"revealed_words",4).values;
        const auto &bw=array(b.arrays,"revealed_words",4).values;
        for(std::size_t t=0;t<80;++t)
            if(t!=slot) require(av[t]==bv[t] && ai[t]==bi[t],
                "BNW sight changed another team visibility slot");
        for(std::size_t w=0;w<4;++w)
            if(w!=word) require(aw[w]==bw[w],
                "BNW sight changed another reveal word");
        const auto bit=std::uint32_t{1}<<(slot%32);
        require((static_cast<std::uint32_t>(aw[word])&~bit)==
                (static_cast<std::uint32_t>(bw[word])&~bit),
                "BNW sight changed another team's reveal bit");
        auto before=*source.map_projection->cells[i].revealed_teams;
        auto after=*world.map_projection->cells[i].revealed_teams;
        before.erase(team);after.erase(team);
        require(before==after,
                "BNW sight changed another projected reveal team");
        if(av[slot]==bv[slot] && ai[slot]==bi[slot] && aw[word]==bw[word])
            continue;
        require(bv[slot]>=0 && bv[slot]<=INT16_MAX &&
                bi[slot]>=0 && bi[slot]<=INT16_MAX &&
                bw[word]>=0 && bw[word]<=UINT32_MAX,
                "BNW sight target counter exceeds SDK wire range");
        result.push_back({i,bv[slot],bi[slot],bw[word]});
    }
    return result;
}
std::vector<AreaDelta> area_deltas(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,std::int64_t team) {
    const auto &a=areas(*source.map_regions),&b=areas(*world.map_regions);
    require(a.records.size()==b.records.size(),
            "BNW sight area roster changed");
    std::vector<AreaDelta> result;
    for(std::size_t i=0;i<a.records.size();++i) {
        const auto id=scalar(a.records[i],"id");
        require(id==scalar(b.records[i],"id"),
                "BNW sight area ID changed");
        const auto &old=array(a.records[i].arrays,"revealed_tiles",80).values;
        const auto &now=array(b.records[i].arrays,"revealed_tiles",80).values;
        for(std::size_t t=0;t<80;++t)
            if(t!=static_cast<std::size_t>(team))
                require(old[t]==now[t],
                        "BNW sight changed another team's area reveal count");
        if(old[static_cast<std::size_t>(team)]==
           now[static_cast<std::size_t>(team)]) continue;
        const auto value=now[static_cast<std::size_t>(team)];
        require(value>=0 && value<=INT32_MAX,
                "BNW sight area reveal counter exceeds SDK i32");
        result.push_back({i,id,value});
    }
    return result;
}
} // namespace

saves::CivisSaveSection make_bnw_unit_sight_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    std::int64_t team) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    shape(source,team);shape(world,team);
    const auto plots=plot_deltas(world,source,team);
    const auto regions=area_deltas(world,source,team);
    require(plots.size()<=UINT32_MAX && regions.size()<=UINT32_MAX,
            "BNW sight delta count exceeds section wire range");
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(digest(source,team));out.raw(digest(world,team));
    out.u64(source.map_header.grid.width);out.u64(source.map_header.grid.height);
    out.i64(team);
    out.u32(static_cast<std::uint32_t>(plots.size()));
    out.u32(static_cast<std::uint32_t>(regions.size()));
    for(const auto &delta:plots) {
        out.u64(delta.plot);out.i64(delta.visibility);
        out.i64(delta.invisible);out.i64(delta.word);
    }
    for(const auto &delta:regions) {
        out.u64(delta.record);out.i64(delta.id);out.i64(delta.revealed);
    }
    return {bnw_unit_sight_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_unit_sight_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    require(section.id==bnw_unit_sight_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+64*4+8*3+4*2 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW sight checkpoint section");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW sight checkpoint version");
    in.expect(identity.payload_sha256);in.expect(identity.rules_sha256);
    const auto source_hash=in.raw(64),target_hash=in.raw(64);
    check_hash(source_hash);check_hash(target_hash);
    const auto width=in.u64(),height=in.u64();
    const auto team=in.i64();
    const auto plot_count=in.u32(),area_count=in.u32();
    require(team>=0 && team<64 && width==world.map_header.grid.width &&
            height==world.map_header.grid.height &&
            plot_count<=world.map_header.grid.size() &&
            world.map_regions &&
            area_count<=areas(*world.map_regions).records.size(),
            "BNW sight checkpoint dimensions or team changed");
    require(source_hash==digest(world,team),
            "BNW sight checkpoint source state digest changed");
    auto replacement=world;
    std::uint64_t previous=0;
    for(std::uint32_t i=0;i<plot_count;++i) {
        const auto index=in.u64();
        const auto visibility=in.i64();
        const auto invisible=in.i64(),word_value=in.i64();
        require(index<replacement.map_header.grid.size() &&
                (i==0 || index>previous) &&
                visibility>=0 && visibility<=INT16_MAX &&
                invisible>=0 && invisible<=INT16_MAX &&
                word_value>=0 && word_value<=UINT32_MAX,
                "BNW sight plot delta is malformed");
        previous=index;
        const auto slot=static_cast<std::size_t>(team);
        auto &plot=replacement.map_plots->plots[index];
        array(plot.arrays,"visibility",80).values[slot]=visibility;
        array(plot.arrays,"invisible_visibility",80).values[slot]=invisible;
        array(plot.arrays,"revealed_words",4).values[slot/32]=word_value;
        auto &revealed=*replacement.map_projection->cells[index].revealed_teams;
        const auto bit=std::uint32_t{1}<<(slot%32);
        if(static_cast<std::uint32_t>(word_value)&bit) revealed.insert(team);
        else revealed.erase(team);
    }
    previous=0;
    for(std::uint32_t i=0;i<area_count;++i) {
        const auto index=in.u64();
        const auto id=in.i64(),revealed=in.i64();
        auto &container=areas(*replacement.map_regions);
        require(index<container.records.size() &&
                (i==0 || index>previous) &&
                id==scalar(container.records[index],"id") &&
                revealed>=0 && revealed<=INT32_MAX,
                "BNW sight area delta is malformed");
        previous=index;
        array(container.records[index].arrays,"revealed_tiles",80)
            .values[static_cast<std::size_t>(team)]=revealed;
    }
    require(in.at==section.bytes.size(),
            "BNW sight checkpoint has trailing bytes");
    (void)plot_deltas(replacement,world,team);
    (void)area_deltas(replacement,world,team);
    require(target_hash==digest(replacement,team),
            "BNW sight checkpoint target state digest changed");
    world=std::move(replacement);
}
} // namespace civ5::runtime
