#include "civ5/ui/bnw_city_banner_appearance.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/pregame_roster.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include <atomic>
#include <bit>
#include <cmath>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <tuple>

namespace civ5::ui {
namespace {
std::atomic<std::uint64_t> next_owner{1};
std::string digest(std::string_view value) {
    return runtime::identify_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(value.data()),value.size())).sha256;
}
struct Source {
    std::string sha,identity;
};
Source source(const assets::ResourceMountTable &resources,const char *key,const char *expected) {
    const auto selected=resources.resolve(assets::AssetKey(std::string(key)));
    const auto *handle=std::get_if<assets::AssetHandle>(&selected.asset);
    if(!handle)throw std::runtime_error("Selected CityBanner source is unavailable or ambiguous");
    const auto bytes=handle->read_bytes(4ull*1024*1024);
    const auto sha=runtime::identify_bytes(bytes).sha256;
    if(sha!=expected)throw std::runtime_error("Selected CityBanner color source differs from inspected source");
    const auto &candidate=handle->candidate();const auto &p=candidate.provenance;
    std::ostringstream identity;identity<<(selected.mount_id?std::to_string(*selected.mount_id):"default")<<'\n'
        <<candidate.original_name<<'\n'<<p.source_id<<'\n'<<p.archive_relative_path.value_or("")<<'\n'<<sha;
    return {sha,digest(identity.str())};
}
std::string colors(const UiFloatColor &color) {
    std::ostringstream out;
    for(const auto channel:color)out<<std::bit_cast<std::uint32_t>(channel)<<',';
    return out.str();
}
std::string seal(const BnwCityBannerAppearance &a) {
    std::ostringstream out;out<<a.owner_id<<','<<a.frame_number<<','<<a.active_player_id<<','<<a.player_id<<','<<a.city_id<<','
        <<a.team_id<<','<<a.active_team_id<<','<<a.turn<<','<<a.minor<<','<<a.is_active_team<<'\n'
        <<colors(a.background)<<colors(a.foreground)<<colors(a.shadow)<<'\n'
        <<a.owner_sha256<<a.rules_rows_sha256<<a.lua_sha256<<a.xml_sha256;
    return digest(out.str());
}
}
struct BnwCityBannerAppearanceOwner::Impl {
    const runtime::LiveSession &session;
    const runtime::PregameRoster &roster;
    const content::RulesDatabase &rules;
    const assets::ResourceMountTable &resources;
    const std::uint64_t generation;
    const std::thread::id thread=std::this_thread::get_id();
    const std::uint64_t id=next_owner.fetch_add(1);
    mutable std::mutex mutex;
    std::uint64_t frame=0;
    std::map<std::tuple<std::int64_t,std::int64_t,std::int64_t>,std::string> issued;
    std::string blocker;
    Impl(const runtime::LiveSession &s,const runtime::PregameRoster &p,const content::RulesDatabase &r,const assets::ResourceMountTable &a)
        :session(s),roster(p),rules(r),resources(a),generation(s.state_generation()) {}
    BnwCityBannerAppearance read(std::int64_t active,std::int64_t player,std::int64_t city_id,std::uint64_t number) const {
        if(session.state_generation()!=generation)throw std::runtime_error("City appearance live state was replaced; rebind its owner");
        if(active<0||player<0||city_id<0||!number||roster.active_player!=active)throw std::runtime_error("City appearance active player/frame is not current");
        const auto &state=session.state();
        const auto slot=roster.slots.find(player);
        const auto ap=state.setup.players.find(active),p=state.setup.players.find(player);
        const auto cp=state.cities.players.find(player);
        if(slot==roster.slots.end()||ap==state.setup.players.end()||p==state.setup.players.end()||cp==state.cities.players.end()||
            !ap->second.alive||!p->second.alive||ap->second.team<0||p->second.team<0||slot->second.minor!=cp->second.minor)
            throw std::runtime_error("City appearance current player/team/minor owner is unavailable or inconsistent");
        const auto city=cp->second.cities.find(city_id);
        if(city==cp->second.cities.end()||city->second.owner!=player||city->second.id!=city_id||city->second.population<=0)
            throw std::runtime_error("City appearance city identity/owner is not current");
        const auto color_slot=slot->second;
        if(color_slot.player_color<0||color_slot.player_color_key.empty())throw std::runtime_error("Current roster has no resolved player color");
        BnwCityBannerAppearance result;
        result.owner_id=id;result.frame_number=number;result.active_player_id=active;result.player_id=player;result.city_id=city_id;
        result.team_id=p->second.team;result.active_team_id=ap->second.team;result.turn=state.setup.game_turn;
        result.minor=color_slot.minor;result.is_active_team=result.team_id==result.active_team_id;
        std::ostringstream owner;owner<<generation<<','<<active<<','<<player<<','<<city_id<<','<<number<<','<<result.turn<<','<<result.team_id<<','<<result.active_team_id
            <<','<<color_slot.status<<','<<color_slot.minor<<','<<color_slot.player_color<<','<<color_slot.player_color_key<<','
            <<city->second.owner<<','<<city->second.id<<','<<city->second.population<<','<<city->second.x<<','<<city->second.y;
        result.owner_sha256=digest(owner.str());
        const auto lua=source(resources,"Assets/UI/InGame/CityBannerManager.lua","c661ebcd04ead9aef5e30b5f8daef4498a7e51e8e8702b5ec21c778ad85b00d2");
        const auto xml=source(resources,"Assets/UI/InGame/CityBannerManager.xml","5762dcd6736473ac470aed3a06f6140134a925a94d8112480d927243d9ade7e2");
        result.lua_sha256=lua.sha;result.xml_sha256=xml.sha;
        UiFloatColor primary{},secondary{};std::string row_projection;
        const auto query=rules.query_rows({
            "SELECT p.ID AS selected_id,p.Type AS selected_type,p.PrimaryColor AS primary_key,p.SecondaryColor AS secondary_key,"
            "a.Red AS ar,a.Green AS ag,a.Blue AS ab,a.Alpha AS aa,b.Red AS br,b.Green AS bg,b.Blue AS bb,b.Alpha AS ba,"
            "typeof(a.Red) AS tar,typeof(a.Green) AS tag,typeof(a.Blue) AS tab,typeof(a.Alpha) AS taa,"
            "typeof(b.Red) AS tbr,typeof(b.Green) AS tbg,typeof(b.Blue) AS tbb,typeof(b.Alpha) AS tba "
            "FROM PlayerColors p JOIN Colors a ON a.Type=p.PrimaryColor JOIN Colors b ON b.Type=p.SecondaryColor WHERE p.ID=? AND p.Type=?",
            {color_slot.player_color,color_slot.player_color_key}},[&](const content::RulesRowView &row) {
                const char *names[]{"ar","ag","ab","aa","br","bg","bb","ba"};
                const char *types[]{"tar","tag","tab","taa","tbr","tbg","tbb","tba"};
                std::ostringstream projection;projection<<row.integer("selected_id")<<','<<row.text("selected_type").value_or("")<<','
                    <<row.text("primary_key").value_or("")<<','<<row.text("secondary_key").value_or("")<<','<<lua.identity<<','<<xml.identity;
                for(std::size_t n=0;n<8;++n) {
                    const auto type=row.text(types[n]);const auto value=row.real(names[n]);
                    if(!type||(*type!="real"&&*type!="integer")||!std::isfinite(value)||value<0||value>1)
                        throw std::runtime_error("Current color row contains missing/nonfinite/out-of-range channels");
                    projection<<','<<std::bit_cast<std::uint64_t>(value);
                    (n<4?primary:secondary)[n%4]=static_cast<float>(value);
                }
                row_projection=projection.str();
            },2);
        if(!query.all_succeeded()||query.rows!=1||row_projection.empty())throw std::runtime_error("Current selected player color rows are missing, ambiguous or inconsistent");
        result.rules_rows_sha256=digest(row_projection);
        if(result.minor)std::swap(primary,secondary);
        result.background={secondary[0],secondary[1],secondary[2],0.7f};
        result.foreground={primary[0],primary[1],primary[2],1.f};result.shadow={0,0,0,0.5f};
        // Resolve observers can reenter application code. Check borrowed live
        // authorities after every external lookup, before accepting old values.
        if(session.state_generation()!=generation)throw std::runtime_error("City appearance live state was replaced during lookup");
        const auto &now=session.state();const auto &new_slot=roster.slots.at(player);
        const auto &new_player=now.setup.players.at(player),&new_active=now.setup.players.at(active);
        const auto &new_owner=now.cities.players.at(player);const auto &new_city=new_owner.cities.at(city_id);
        std::ostringstream recheck;recheck<<generation<<','<<active<<','<<player<<','<<city_id<<','<<number<<','<<now.setup.game_turn<<','<<new_player.team<<','<<new_active.team
            <<','<<new_slot.status<<','<<new_slot.minor<<','<<new_slot.player_color<<','<<new_slot.player_color_key<<','
            <<new_city.owner<<','<<new_city.id<<','<<new_city.population<<','<<new_city.x<<','<<new_city.y;
        if(roster.active_player!=active||!new_active.alive||!new_player.alive||new_owner.minor!=new_slot.minor||
            digest(recheck.str())!=result.owner_sha256)throw std::runtime_error("City appearance live owner changed during source/rules lookup");
        return result;
    }
};
BnwCityBannerAppearanceOwner::BnwCityBannerAppearanceOwner(const runtime::LiveSession &s,const runtime::PregameRoster &p,
    const content::RulesDatabase &r,const assets::ResourceMountTable &a):impl_(std::make_unique<Impl>(s,p,r,a)) {}
BnwCityBannerAppearanceOwner::~BnwCityBannerAppearanceOwner()=default;
std::optional<BnwCityBannerAppearance> BnwCityBannerAppearanceOwner::capture(std::int64_t active,std::int64_t player,std::int64_t city,std::uint64_t frame) {
    if(impl_->thread!=std::this_thread::get_id())return {};
    std::unique_lock lock(impl_->mutex,std::try_to_lock);if(!lock.owns_lock())return {};
    try {
        if(frame<impl_->frame)throw std::runtime_error("City appearance frame number moved backwards");
        const auto before=impl_->read(active,player,city,frame),after=impl_->read(active,player,city,frame);
        const auto hash=seal(before);
        if(hash!=seal(after))throw std::runtime_error("City appearance owner changed while read");
        if(frame!=impl_->frame){impl_->issued.clear();impl_->frame=frame;}
        impl_->issued[{active,player,city}]=hash;impl_->blocker.clear();return before;
    }catch(const std::exception &error){impl_->blocker=error.what();return {};}
}
bool BnwCityBannerAppearanceOwner::matches(const BnwCityBannerAppearance &appearance) const {
    if(impl_->thread!=std::this_thread::get_id()||appearance.owner_id!=impl_->id||
        impl_->session.state_generation()!=impl_->generation)return false;
    std::unique_lock lock(impl_->mutex,std::try_to_lock);if(!lock.owns_lock())return false;
    const auto found=impl_->issued.find({appearance.active_player_id,appearance.player_id,appearance.city_id});
    if(appearance.frame_number!=impl_->frame||found==impl_->issued.end()||found->second!=seal(appearance))return false;
    try {
        const auto current=impl_->read(appearance.active_player_id,appearance.player_id,appearance.city_id,appearance.frame_number);
        const auto after=impl_->read(appearance.active_player_id,appearance.player_id,appearance.city_id,appearance.frame_number);
        return seal(current)==found->second&&seal(after)==found->second;
    }catch(const std::exception &){return false;}
}
const std::string &BnwCityBannerAppearanceOwner::blocker() const noexcept{return impl_->blocker;}
} // namespace civ5::ui
