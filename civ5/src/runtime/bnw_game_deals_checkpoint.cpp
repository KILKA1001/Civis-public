#include "civ5/runtime/bnw_game_deals_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','D','L','0','1'};
constexpr std::size_t max_bytes=64*1024*1024;
constexpr std::size_t max_deals=100'000,max_items=1'000'000;
void require(bool valid,const char *why) {
    if(!valid) throw std::runtime_error(why);
}
void digest(std::string_view value) {
    require(value.size()==64 &&
            std::all_of(value.begin(),value.end(),[](char c) {
                return (c>='0'&&c<='9')||(c>='a'&&c<='f');
            }),"Invalid BNW deals checkpoint identity");
}
std::int32_t i32(std::int64_t value) {
    require(value>=INT32_MIN && value<=INT32_MAX,
            "BNW deal value exceeds source i32");
    return static_cast<std::int32_t>(value);
}
std::uint32_t count(std::size_t value,const char *why) {
    require(value<=INT32_MAX,why);
    return static_cast<std::uint32_t>(value);
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void raw(std::span<const std::uint8_t> value) {
        require(value.size()<=max_bytes &&
                bytes.size()<=max_bytes-value.size(),
                "BNW deals checkpoint byte budget exceeded");
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void raw(std::string_view value) {
        raw({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});
    }
    void u8(bool value) {
        require(bytes.size()<max_bytes,"BNW deals checkpoint byte budget exceeded");
        bytes.push_back(value?1:0);
    }
    void u32(std::uint32_t value) {
        require(bytes.size()<=max_bytes-4,
                "BNW deals checkpoint byte budget exceeded");
        for(unsigned i=0;i<4;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i32(std::int64_t value) {
        u32(std::bit_cast<std::uint32_t>(::civ5::runtime::i32(value)));
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::span<const std::uint8_t> raw(std::size_t size) {
        require(at<=bytes.size() && size<=bytes.size()-at,
                "Truncated BNW deals checkpoint");
        const auto value=bytes.subspan(at,size);at+=size;return value;
    }
    bool u8() {
        const auto value=raw(1)[0];
        require(value<=1,"Invalid BNW deals checkpoint boolean");
        return value!=0;
    }
    std::uint32_t u32() {
        const auto bytes=raw(4);
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[i])<<(8*i);
        return value;
    }
    std::int32_t i32() {return std::bit_cast<std::int32_t>(u32());}
    void hash(std::string_view expected,const char *why) {
        const auto value=raw(expected.size());
        require(std::equal(value.begin(),value.end(),expected.begin()),why);
    }
};
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.history.layout==bnw &&
            world.all_players && world.all_players->layout==bnw &&
            world.all_players->context.player_count==64 &&
            world.all_players->players.size()==64,
            "BNW deals checkpoint requires complete 64-player world");
    for(std::size_t p=0;p<64;++p)
        require(world.all_players->players[p].index==p &&
                world.all_players->players[p].saved_player_id==
                    static_cast<std::int64_t>(p),
                "BNW deals checkpoint player roster changed");
}
void validate(const saves::DealsObservation &deals) {
    require(deals.version && *deals.version==1 && !deals.next_deal_id,
            "BNW CvGameDeals version or derived ID changed");
    std::size_t deal_total=0,item_total=0;
    for(const auto &group:deals.groups) {
        require(group.size()<=INT32_MAX && group.size()<=max_deals-deal_total,
                "BNW deal group count exceeds budget");
        deal_total+=group.size();
        for(const auto &deal:group) {
            require(deal.version && *deal.version>=1 && *deal.version<=3 &&
                    !deal.id && !deal.is_gift &&
                    deal.considering_renewal && deal.checked_renewal &&
                    deal.cancelled &&
                    (*deal.version>=3 || !*deal.checked_renewal),
                    "BNW deal version or optional fields changed");
            (void)i32(deal.from_player);(void)i32(deal.to_player);
            (void)i32(deal.final_turn);(void)i32(deal.duration);
            (void)i32(deal.start_turn);(void)i32(deal.peace_treaty);
            (void)i32(deal.surrendering_player);
            (void)i32(deal.demanding_player);
            (void)i32(deal.requesting_player);
            require(deal.items.size()<=INT32_MAX &&
                    deal.items.size()<=max_items-item_total,
                    "BNW traded item count exceeds budget");
            item_total+=deal.items.size();
            for(const auto &item:deal.items) {
                const bool legacy=*deal.version==1;
                require((legacy && !item.version) ||
                        (!legacy && item.version &&
                         *item.version>=1 && *item.version<=2),
                        "BNW traded item version does not match deal");
                require(item.from_renewed && item.to_renewed &&
                        (!legacy || (!*item.from_renewed && !*item.to_renewed)) &&
                        ((legacy || *item.version==1) ?
                            (item.data[2]==0 && !item.flag1):true),
                        "BNW traded item version fields changed");
                (void)i32(item.type);(void)i32(item.duration);
                (void)i32(item.final_turn);(void)i32(item.from_player);
                for(auto value:item.data) (void)i32(value);
            }
        }
    }
}
std::vector<std::uint8_t> encode(const saves::DealsObservation &deals) {
    validate(deals);
    Writer out;
    out.u32(static_cast<std::uint32_t>(*deals.version));
    for(const auto &group:deals.groups) {
        out.u32(count(group.size(),"BNW deal count exceeds source i32"));
        for(const auto &deal:group) {
            out.u32(static_cast<std::uint32_t>(*deal.version));
            out.i32(deal.from_player);out.i32(deal.to_player);
            out.i32(deal.final_turn);out.i32(deal.duration);
            out.i32(deal.start_turn);
            out.u8(*deal.considering_renewal);
            out.u8(*deal.checked_renewal);out.u8(*deal.cancelled);
            out.i32(deal.peace_treaty);
            out.i32(deal.surrendering_player);
            out.i32(deal.demanding_player);
            out.i32(deal.requesting_player);
            out.u32(count(deal.items.size(),
                          "BNW traded item count exceeds source i32"));
            for(const auto &item:deal.items) {
                out.u32(item.version?
                    static_cast<std::uint32_t>(*item.version):0);
                out.i32(item.type);out.i32(item.duration);
                out.i32(item.final_turn);out.i32(item.from_player);
                for(auto value:item.data) out.i32(value);
                out.u8(item.flag1);out.u8(*item.from_renewed);
                out.u8(*item.to_renewed);
            }
        }
    }
    return out.bytes;
}
saves::DealsObservation decode(std::span<const std::uint8_t> bytes) {
    Reader in{bytes};
    saves::DealsObservation result;
    result.version=in.u32();
    require(*result.version==1,"Unsupported BNW CvGameDeals version");
    std::size_t deal_total=0,item_total=0;
    for(auto &group:result.groups) {
        const auto size=in.u32();
        require(size<=INT32_MAX && size<=max_deals-deal_total &&
                size<=(bytes.size()-in.at)/46,
                "BNW deal group count exceeds input or budget");
        deal_total+=size;group.reserve(size);
        for(std::size_t d=0;d<size;++d) {
            auto &deal=group.emplace_back();
            deal.version=in.u32();
            require(*deal.version>=1 && *deal.version<=3,
                    "Unsupported BNW deal version");
            deal.from_player=in.i32();deal.to_player=in.i32();
            deal.final_turn=in.i32();deal.duration=in.i32();
            deal.start_turn=in.i32();
            deal.considering_renewal=in.u8();
            deal.checked_renewal=in.u8();deal.cancelled=in.u8();
            deal.peace_treaty=in.i32();
            deal.surrendering_player=in.i32();
            deal.demanding_player=in.i32();
            deal.requesting_player=in.i32();
            const auto items=in.u32();
            require(items<=INT32_MAX && items<=max_items-item_total &&
                    items<=(bytes.size()-in.at)/35,
                    "BNW traded item count exceeds input or budget");
            item_total+=items;deal.items.reserve(items);
            for(std::size_t i=0;i<items;++i) {
                auto &item=deal.items.emplace_back();
                const auto version=in.u32();
                if(version) item.version=version;
                item.type=in.i32();item.duration=in.i32();
                item.final_turn=in.i32();item.from_player=in.i32();
                for(auto &value:item.data) value=in.i32();
                item.flag1=in.u8();
                item.from_renewed=in.u8();item.to_renewed=in.u8();
            }
        }
    }
    require(in.at==bytes.size(),"Trailing BNW deals checkpoint bytes");
    validate(result);
    return result;
}
} // namespace

saves::CivisSaveSection make_bnw_game_deals_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    shape(source);shape(world);
    const auto base=encode(source.history.deals);
    const auto live=encode(world.history.deals);
    constexpr std::size_t overhead=8+4+128+8;
    require(live.size()<=max_bytes-overhead &&
            base.size()<=max_bytes-overhead-live.size(),
            "BNW deals checkpoint section budget exceeded");
    Writer out;
    out.raw(magic);out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.u32(static_cast<std::uint32_t>(base.size()));out.raw(base);
    out.u32(static_cast<std::uint32_t>(live.size()));out.raw(live);
    return {bnw_game_deals_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_game_deals_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(section.id==bnw_game_deals_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()<=max_bytes &&
            section.bytes.size()>=8+4+128+8 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW deals checkpoint section");
    shape(source);shape(world);
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW deals checkpoint version");
    in.hash(identity.payload_sha256,
            "BNW deals checkpoint source payload changed");
    in.hash(identity.rules_sha256,
            "BNW deals checkpoint active rules changed");
    const auto base=in.raw(in.u32());
    const auto live=in.raw(in.u32());
    require(in.at==section.bytes.size(),
            "Trailing BNW deals checkpoint bytes");
    const auto source_bytes=encode(source.history.deals);
    require(base.size()==source_bytes.size() &&
            std::equal(base.begin(),base.end(),source_bytes.begin()) &&
            encode(world.history.deals)==source_bytes,
            "BNW deals checkpoint source or destination baseline changed");
    auto replacement=decode(live);
    static_assert(std::is_nothrow_move_assignable_v<saves::DealsObservation>);
    world.history.deals=std::move(replacement);
}
} // namespace civ5::runtime
