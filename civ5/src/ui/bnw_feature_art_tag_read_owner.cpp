#include "civ5/ui/bnw_feature_art_tag_read_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <atomic>
#include <limits>
#include <set>
#include <stdexcept>
#include <thread>

namespace civ5::ui {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool value,const char *message) { if(!value) throw std::invalid_argument(message); }
bool same_types(const content::InfoTypeCatalog &a,const content::InfoTypeCatalog &b) {
    if(!a.report().all_succeeded() || !b.report().all_succeeded() || a.names()!=b.names() || a.hashes()!=b.hashes()) return false;
    for(const auto &[name,entry]:a.entries()) {
        const auto *other=b.find(name);
        if(!other || entry.table!=other->table || entry.id!=other->id || entry.hash!=other->hash) return false;
        const auto *ah=a.find_hash(entry.hash),*bh=b.find_hash(entry.hash);
        if(!ah || !bh || ah->type!=bh->type || ah->table!=bh->table || ah->id!=bh->id) return false;
    }
    return true;
}
}
struct BnwFeatureArtTagReadOwner::Impl {
    struct Info {
        std::int64_t authored_id;
        std::optional<std::string> type,tag;
        bool natural_wonder;
    };
    const runtime::LiveSession *session;
    std::shared_ptr<const runtime::LiveSessionState> state;
    std::uint64_t generation,span=0;
    std::int64_t player;
    runtime::SessionCheckpointIdentity identity;
    content::InfoTypeCatalog types;
    std::map<std::uint64_t,Info> infos;
    std::function<bool()> database_current;
    const std::thread::id thread=std::this_thread::get_id();
    std::atomic<bool> poisoned{false},retired{false};
    bool busy=false;
    std::shared_ptr<const BnwFeatureArtTagReadReceipt> latest;
    Impl(const runtime::LiveSession &s,const content::InfoTypeCatalog &t,
        const runtime::SessionCheckpointIdentity &id,std::int64_t p,std::function<bool()> current)
        : session(&s),state(s.read_state_lease()),generation(s.state_generation()),player(p),
          identity(id),types(t),database_current(std::move(current)) {}
    bool enter() noexcept {
        if(thread!=std::this_thread::get_id()) { poisoned=true; return false; }
        if(busy) { poisoned=true; return false; }
        if(poisoned || retired) return false;
        busy=true; return true;
    }
    struct Call {
        Impl &i;
        bool entered;
        explicit Call(Impl &impl):i(impl),entered(i.enter()) {}
        ~Call() { if(entered) i.busy=false; }
    };
    bool source_current(const runtime::LiveSession &s) const {
        if(poisoned || retired || &s!=session || thread!=std::this_thread::get_id() ||
            !database_current || !database_current() || s.state_generation()!=generation ||
            s.read_state_lease()!=state) return false;
        const auto &live=*state;
        if(live.setup.profile!=content::TechnologyProfile::bnw || live.world.game_state.layout!=bnw ||
            !live.world.pregame || live.world.pregame->layout!=bnw ||
            live.world.map_header.layout!=bnw || !live.world.map_projection) return false;
        const auto selected=live.world.pregame->archive.integers.find("active_player");
        const auto p=live.setup.players.find(player);
        if(selected==live.world.pregame->archive.integers.end() || selected->second!=player ||
            p==live.setup.players.end() || !p->second.alive || p->second.team<0) return false;
        const auto &header=live.world.map_header;
        const auto &map=*live.world.map_projection;
        if(!map.grid.width || !map.grid.height || header.grid.width!=map.grid.width ||
            header.grid.height!=map.grid.height || header.wrap_x!=map.wrap_x || header.wrap_y!=map.wrap_y ||
            map.cells.size()!=map.grid.size()) return false;
        return !poisoned && !retired && database_current() && s.state_generation()==generation && s.read_state_lease()==state;
    }
    BnwFeatureArtTagReadStamp stamp(std::uint64_t frame) const {
        const auto &live=*state;
        const auto &map=*live.world.map_projection;
        return {identity,generation,frame,map.grid.width,map.grid.height,player,
            live.setup.players.at(player).team,live.setup.game_turn,map.wrap_x,map.wrap_y,live.world.map_header.guid};
    }
    BnwFeatureArtTagReadFacts read(std::uint64_t index) const {
        const auto &map=*state->world.map_projection;
        require(index<map.cells.size(),"Feature art plot index is outside current map");
        const auto &cell=map.cells.at(static_cast<std::size_t>(index));
        const auto xy=map.grid.coordinates(index);
        require(cell.index==index && cell.x==xy[0] && cell.y==xy[1],"Feature art current plot coordinates differ");
        BnwFeatureArtTagReadFacts result;
        result.plot_index=index; result.x=cell.x; result.y=cell.y;
        result.wire_feature=cell.feature_type; result.wire_feature_hash=cell.feature_type_hash;
        if(cell.feature_type && cell.feature_type_hash) return result;
        if((cell.feature_type && *cell.feature_type==-1) || (cell.feature_type_hash && *cell.feature_type_hash==0)) {
            result.status=BnwFeatureArtTagReadStatus::no_feature; return result;
        }
        auto slot=cell.feature_type;
        if(cell.feature_type_hash) {
            if(*cell.feature_type_hash>UINT32_MAX) return result;
            const auto *found=types.find_hash(*cell.feature_type_hash);
            if(!found) return result;
            // Global source type cache ID indexes Features; don't substitute a
            // Features-only cache or assume the found row belongs to this table.
            slot=found->id;
        }
        if(!slot || *slot<0 || static_cast<std::uint64_t>(*slot)>=span) return result;
        result.info_slot=static_cast<std::uint64_t>(*slot);
        const auto info=infos.find(*result.info_slot);
        if(info==infos.end()) { result.status=BnwFeatureArtTagReadStatus::null_info_slot; return result; }
        result.authored_id=info->second.authored_id; result.type=info->second.type;
        result.art_define_tag=info->second.tag; result.natural_wonder=info->second.natural_wonder;
        result.status=!result.type ? BnwFeatureArtTagReadStatus::absent_type :
            !result.art_define_tag ? BnwFeatureArtTagReadStatus::absent_art_tag : BnwFeatureArtTagReadStatus::key;
        return result;
    }
    bool current(const runtime::LiveSession &s,const std::shared_ptr<const BnwFeatureArtTagReadReceipt> &receipt,
        std::uint64_t frame) const {
        if(!receipt || latest!=receipt || receipt->owner_.lock().get()!=this || !frame ||
            receipt->stamp().frame_number!=frame || !source_current(s)) return false;
        const auto now=stamp(frame);
        const auto &old=receipt->stamp();
        if(now.state_generation!=old.state_generation || now.active_player!=old.active_player || now.active_team!=old.active_team ||
            now.turn!=old.turn || now.width!=old.width || now.height!=old.height || now.wrap_x!=old.wrap_x ||
            now.wrap_y!=old.wrap_y || now.map_guid!=old.map_guid || now.source.payload_sha256!=old.source.payload_sha256 ||
            now.source.rules_sha256!=old.source.rules_sha256) return false;
        for(const auto &plot:receipt->plots()) if(read(plot.plot_index)!=plot) return false;
        return source_current(s);
    }
};
BnwFeatureArtTagReadReceipt::BnwFeatureArtTagReadReceipt(BnwFeatureArtTagReadStamp stamp,
    std::vector<BnwFeatureArtTagReadFacts> plots,std::weak_ptr<const void> owner)
    : stamp_(std::move(stamp)),plots_(std::move(plots)),owner_(std::move(owner)) {}
BnwFeatureArtTagReadOwner::BnwFeatureArtTagReadOwner(const runtime::LiveSession &session,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const runtime::SessionCheckpointIdentity &identity,std::int64_t player,std::uint64_t maximum) {
    auto current=database.immutable_preview_fingerprint_currentness();
    require(current && current(),"Feature art requires explicit SAME immutable preview Rules scope");
    require(identity.payload_sha256==runtime::identify_bytes(session.source().source.bytes).sha256 &&
        identity.rules_sha256==database.gameplay_fingerprint_sha256(),"Feature art source/rules identity differs");
    require(same_types(types,database.cache_info_types()),"Feature art supplied type cache differs from SAME Rules scope");
    auto impl=std::make_shared<Impl>(session,types,identity,player,std::move(current));
    const auto collection=database.prefetch_info("Features",[&](std::uint64_t slot,std::int64_t id,const content::RulesRowView &row) {
        impl->infos.emplace(slot,Impl::Info{id,row.text("Type"),row.text("ArtDefineTag"),row.boolean("NaturalWonder")});
        return true;
    },{maximum});
    require(collection.all_succeeded(),"Feature art Features collection read failed");
    impl->span=collection.slots;
    require(impl->source_current(session),"Feature art current session/map/active-player source differs");
    impl_=std::move(impl);
}
BnwFeatureArtTagReadOwner::~BnwFeatureArtTagReadOwner() { if(impl_) impl_->retired=true; }
std::shared_ptr<const BnwFeatureArtTagReadReceipt> BnwFeatureArtTagReadOwner::capture(
    const runtime::LiveSession &session,std::span<const std::uint64_t> indices,std::uint64_t frame) {
    const auto impl=impl_;
    Impl::Call call(*impl);
    if(!call.entered) return {};
    impl->latest.reset();
    if(!frame || indices.empty() || indices.size()>1024*1024) return {};
    // No service callback may retarget the caller's span or this operation.
    const std::vector<std::uint64_t> requested(indices.begin(),indices.end());
    if(!impl->source_current(session)) return {};
    std::set<std::uint64_t> unique;
    std::vector<BnwFeatureArtTagReadFacts> plots;
    plots.reserve(requested.size());
    for(const auto index:requested) {
        require(unique.insert(index).second,"Feature art capture has duplicate plot indices");
        plots.push_back(impl->read(index));
    }
    auto receipt=std::shared_ptr<const BnwFeatureArtTagReadReceipt>(new BnwFeatureArtTagReadReceipt(
        impl->stamp(frame),std::move(plots),impl));
    if(!impl->source_current(session)) return {};
    impl->latest=receipt;
    if(!impl->current(session,receipt,frame)) { impl->latest.reset(); return {}; }
    return receipt;
}
bool BnwFeatureArtTagReadOwner::matches(const runtime::LiveSession &session,
    const std::shared_ptr<const BnwFeatureArtTagReadReceipt> &borrowed,std::uint64_t frame) const noexcept {
    const auto receipt=borrowed;
    const auto impl=impl_;
    Impl::Call call(*impl);
    if(!call.entered) return false;
    try { return impl->current(session,receipt,frame); } catch(...) { return false; }
}
bool BnwFeatureArtTagReadOwner::visit_current_art_keys(const runtime::LiveSession &session,
    const std::shared_ptr<const BnwFeatureArtTagReadReceipt> &borrowed,std::uint64_t frame,
    const std::function<void(std::span<const BnwFeatureArtTagReadFacts>)> &visitor) noexcept {
    const auto receipt=borrowed;
    const auto impl=impl_;
    Impl::Call call(*impl);
    if(!call.entered) return false;
    try {
        const auto visitor_pin=visitor;
        if(!visitor_pin || !impl->current(session,receipt,frame)) return false;
        visitor_pin(receipt->plots());
        return impl->current(session,receipt,frame);
    } catch(...) { impl->poisoned=true; return false; }
}
} // namespace civ5::ui
