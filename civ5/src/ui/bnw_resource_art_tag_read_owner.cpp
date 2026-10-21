#include "civ5/ui/bnw_resource_art_tag_read_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include <atomic>
#include <climits>
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
std::int64_t field(const saves::PlotObservation &plot,std::string_view name) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &value:plot.fields) if(value.name && value.name==name) {
        require(!found,"Duplicate Resource art plot field"); found=&value;
    }
    require(found,"Missing Resource art plot field"); return found->value;
}
const saves::PlotArrayObservation &array(const saves::PlotObservation &plot,std::string_view name) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &value:plot.arrays) if(value.name && value.name==name) {
        require(!found,"Duplicate Resource art plot array"); found=&value;
    }
    require(found && found->columns==1,"Missing Resource art plot array"); return *found;
}
std::int32_t city_scalar(const saves::SavedCityObservation &city,std::string_view name) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &value:city.core.fields) if(value.name && value.name==name) {
        require(!found,"Duplicate Resource art city source field"); found=&value;
    }
    require(found && found->encoding==saves::CityCoreEncoding::i32 && found->values.size()==1 &&
        found->values[0]>=INT32_MIN && found->values[0]<=INT32_MAX,"Missing Resource art city source field");
    return static_cast<std::int32_t>(found->values[0]);
}
std::optional<std::uint8_t> first_match(std::int32_t resource,const std::array<std::int16_t,9> &cache) {
    if(resource==-1) return {};
    for(std::uint8_t i=0;i<cache.size();++i) if(resource==cache[i]) return i;
    return {};
}
bool predicate(std::uint8_t index,bool hills,bool water,std::int32_t feature,std::int16_t marsh) {
    return index==0 ? hills : index==1 ? water : feature==marsh;
}
}
std::optional<BnwResourceArtVariant> select_bnw_resource_art_scalar_variant(
    const BnwResourceArtScalarInput &input) noexcept {
    if(input.resource==-1) return {};
    const auto index=first_match(input.resource,input.cached_resource_types);
    const bool alt=index && predicate(*index,input.hills,input.water,input.feature,input.cached_marsh);
    const bool heavy=input.quantity>input.threshold;
    return alt ? (heavy ? BnwResourceArtVariant::alt_heavy : BnwResourceArtVariant::alt) :
        (heavy ? BnwResourceArtVariant::heavy : BnwResourceArtVariant::normal);
}
struct BnwResourceArtTagReadOwner::Impl {
    struct Info {
        std::int64_t authored_id;
        std::optional<std::string> type;
        std::array<std::optional<std::string>,4> tags;
    };
    const runtime::LiveSession *session;
    std::shared_ptr<const runtime::LiveSessionState> state;
    std::uint64_t generation,span=0;
    std::int64_t player;
    runtime::SessionCheckpointIdentity identity;
    content::InfoTypeCatalog types;
    content::GlobalDefineCache defines;
    std::map<std::uint64_t,Info> infos;
    std::array<std::int16_t,9> resource_cache{};
    std::int16_t marsh=-1;
    std::int32_t threshold=0;
    std::function<bool()> database_current;
    const std::thread::id thread=std::this_thread::get_id();
    std::atomic<bool> poisoned{false},retired{false};
    bool busy=false;
    std::shared_ptr<const BnwResourceArtTagReadReceipt> latest;
    Impl(const runtime::LiveSession &s,const content::InfoTypeCatalog &t,
        const runtime::SessionCheckpointIdentity &id,std::int64_t p,std::function<bool()> current)
        :session(&s),state(s.read_state_lease()),generation(s.state_generation()),player(p),
         identity(id),types(t),database_current(std::move(current)) {}
    bool enter() noexcept {
        if(thread!=std::this_thread::get_id()) { poisoned=true; return false; }
        if(busy) { poisoned=true; return false; }
        if(poisoned || retired) return false;
        busy=true; return true;
    }
    struct Call {
        Impl &i; bool entered;
        explicit Call(Impl &value):i(value),entered(i.enter()) {}
        ~Call() { if(entered) i.busy=false; }
    };
    bool source_current(const runtime::LiveSession &s) const {
        if(poisoned || retired || &s!=session || thread!=std::this_thread::get_id() ||
            !database_current || !database_current() || s.state_generation()!=generation ||
            s.read_state_lease()!=state) return false;
        const auto &live=*state;
        const auto &world=live.world;
        if(live.setup.profile!=content::TechnologyProfile::bnw || world.game_state.layout!=bnw ||
            !world.pregame || world.pregame->layout!=bnw || world.map_header.layout!=bnw ||
            !world.map_projection || !world.map_plots || world.map_plots->layout!=bnw) return false;
        const auto active=world.pregame->archive.integers.find("active_player");
        const auto teams=world.pregame->archive.integer_arrays.find("team_types");
        const auto p=live.setup.players.find(player);
        if(player<0 || player>=64 || active==world.pregame->archive.integers.end() || active->second!=player ||
            p==live.setup.players.end() || !p->second.alive || p->second.team<0 || p->second.team>=64 ||
            teams==world.pregame->archive.integer_arrays.end() || static_cast<std::uint64_t>(player)>=teams->second.size() ||
            teams->second[static_cast<std::size_t>(player)]!=p->second.team) return false;
        const auto &header=world.map_header; const auto &map=*world.map_projection;
        if(!map.grid.width || !map.grid.height || header.grid.width!=map.grid.width ||
            header.grid.height!=map.grid.height || header.wrap_x!=map.wrap_x || header.wrap_y!=map.wrap_y ||
            map.cells.size()!=map.grid.size() || world.map_plots->plots.size()!=map.cells.size()) return false;
        return !poisoned && !retired && database_current() && s.state_generation()==generation && s.read_state_lease()==state;
    }
    BnwResourceDebugCacheRead debug_cache_read() const {
        try {
            return {BnwResourceDebugCacheReadStatus::checked_cache,session->bnw_game_debug_mode()};
        } catch(const std::invalid_argument &) {
            // The parent accessor refuses absent/stale reset/Read authority.
            // No false value is inferred for diagnostic imports/dirty owners.
            return {BnwResourceDebugCacheReadStatus::unavailable,{}};
        }
    }
    BnwResourceArtTagReadStamp stamp(std::uint64_t frame) const {
        const auto &live=*state; const auto &map=*live.world.map_projection;
        return {identity,generation,frame,map.grid.width,map.grid.height,player,live.setup.players.at(player).team,
            live.setup.game_turn,map.wrap_x,map.wrap_y,live.world.map_header.guid,debug_cache_read()};
    }
    std::optional<std::int32_t> identity_value(const std::optional<std::int64_t> &number,
        const std::optional<std::uint64_t> &hash,std::int64_t raw,bool raw_hash) const {
        if(number && hash) return {};
        std::optional<std::int64_t> resolved;
        if(raw_hash) {
            if(raw<0 || raw>UINT32_MAX || (hash && *hash!=static_cast<std::uint64_t>(raw))) return {};
            if(raw==0) resolved=-1;
            else if(const auto *value=types.find_hash(static_cast<std::uint64_t>(raw))) resolved=value->id;
            if(!resolved || (number && *number!=*resolved) || (!number && !hash)) return {};
        } else {
            if(hash || !number || *number!=raw) return {};
            resolved=raw;
        }
        // CvPlot's resource/feature getters sign-extend actual native bytes.
        if(*resolved < -1 || *resolved > INT8_MAX) return {};
        return static_cast<std::int32_t>(*resolved);
    }
    BnwResourceArtTagReadFacts read(std::uint64_t index,const runtime::BnwLivePlotVisibility &visibility,
        const BnwResourceDebugCacheRead &debug) const {
        const auto &world=state->world; const auto &map=*world.map_projection;
        require(index<map.cells.size(),"Resource art plot outside current map");
        const auto &cell=map.cells.at(static_cast<std::size_t>(index));
        const auto &raw=world.map_plots->plots.at(static_cast<std::size_t>(index));
        const auto xy=map.grid.coordinates(index);
        require(cell.index==index && cell.x==xy[0] && cell.y==xy[1] && raw.index==index && raw.x==cell.x && raw.y==cell.y &&
            raw.version && *raw.version>=1 && *raw.version<=7,"Resource art current raw/projected plot identity differs");
        const auto team=state->setup.players.at(player).team;
        const auto &words=array(raw,"revealed_words");
        // BNW serializes PlotBoolField::eCount (=4) DWORDs, independently of
        // REALLY_MAX_TEAMS (=80) used by the visibility arrays.
        require(words.values.size()==4 && cell.revealed_teams,"Resource art current reveal owner is unresolved");
        const auto word=words.values.at(static_cast<std::size_t>(team/32));
        require(word>=0 && word<=UINT32_MAX,"Resource art current reveal bits exceed source width");
        BnwResourceArtTagReadFacts result;
        result.plot_index=index; result.x=cell.x; result.y=cell.y;
        if(debug.status==BnwResourceDebugCacheReadStatus::checked_cache && debug.cached_mode)
            result.layout_blockers.erase(result.layout_blockers.begin());
        result.revealed=(static_cast<std::uint32_t>(word)&(std::uint32_t{1}<<(team%32)))!=0;
        require(result.revealed==cell.revealed_teams->contains(team),"Resource art raw/projected reveal differs");
        result.visibility_count=visibility.counts.at(static_cast<std::size_t>(index*visibility.wire_slots+static_cast<std::uint64_t>(team)));
        require(result.visibility_count>=0,"Resource art current visibility count is negative");
        result.wire_resource=cell.resource_type; result.wire_resource_hash=cell.resource_type_hash;
        const auto quantity=field(raw,"resource_count");
        require(quantity>=INT8_MIN && quantity<=INT8_MAX,"Resource art quantity exceeds signed native byte");
        result.quantity=static_cast<std::int32_t>(quantity); result.heavy_threshold=threshold;
        result.quantity_exceeds_heavy_threshold=result.quantity>threshold;
        const auto plot=field(raw,"plot_type");
        require(cell.plot_type && *cell.plot_type==plot && plot>=INT8_MIN && plot<=INT8_MAX,"Resource art current plot_type differs");
        result.plot_type=static_cast<std::int32_t>(plot); result.hills=plot==1; result.water=plot==3;
        const auto city_owner=field(raw,"plot_city_owner"),city_id=field(raw,"plot_city_id");
        require(cell.plot_city && cell.plot_city->owner==city_owner && cell.plot_city->id==city_id,"Resource art current plot-city handle differs");
        result.plot_city=cell.plot_city;
        require(city_owner>=INT32_MIN && city_owner<=INT32_MAX && city_id>=INT32_MIN && city_id<=INT32_MAX,
            "Resource art plot-city handle exceeds native int32");
        // The Impl owns this exact state lease across lookup/copy/requery.
        // Reuse the existing native FLTA/raw-owned city binding, not an
        // independent registrar or the serialized cell.is_city flag.
        if(city_owner>=0 && city_owner<64 && city_id!=-1)
            require(world.all_players && world.all_players->layout==bnw && world.all_players->players.size()==64 &&
                state->cities.profile==content::TechnologyProfile::bnw,"Resource art current city registry profile differs");
        const auto *city=runtime::resolve_bnw_current_city(*state,static_cast<std::int32_t>(city_owner),static_cast<std::int32_t>(city_id));
        result.plot_is_city=city!=nullptr;
        if(city) {
            require(city_scalar(*city,"m_eOwner")==city_owner,"Resource art resolved current city owner differs");
            result.resolved_city_id=city_scalar(*city,"m_iID");
            result.resolved_city_source_core=std::pair{city->core.start_offset,city->core.end_offset};
        }
        const auto feature=identity_value(cell.feature_type,cell.feature_type_hash,
            field(raw,*raw.version>=3 ? "feature_type_hash" : "feature_type"),*raw.version>=3);
        const auto resource=identity_value(cell.resource_type,cell.resource_type_hash,field(raw,"resource_type_hash"),true);
        result.feature=feature; result.logical_resource=resource;
        // ResourceInfo and its four source tags do not depend on whether a
        // separate FeatureType identity can currently be resolved.
        if(!resource) return result;
        result.first_cached_resource_match=first_match(*resource,resource_cache);
        if(result.first_cached_resource_match && (*result.first_cached_resource_match<2 || feature))
            result.cached_resource_predicate=predicate(*result.first_cached_resource_match,
                result.hills,result.water,feature.value_or(-1),marsh);
        if(*resource==-1) { result.info_status=BnwResourceArtInfoStatus::no_resource; return result; }
        if(static_cast<std::uint64_t>(*resource)>=span) return result;
        result.info_slot=static_cast<std::uint64_t>(*resource);
        const auto info=infos.find(*result.info_slot);
        if(info==infos.end()) { result.info_status=BnwResourceArtInfoStatus::null_info_slot; return result; }
        result.authored_id=info->second.authored_id; result.type=info->second.type; result.art_define_tags=info->second.tags;
        result.info_status=result.type ? BnwResourceArtInfoStatus::info : BnwResourceArtInfoStatus::absent_type;
        return result;
    }
    bool current(const runtime::LiveSession &s,const std::shared_ptr<const BnwResourceArtTagReadReceipt> &receipt,std::uint64_t frame) const {
        if(!receipt || latest!=receipt || receipt->owner_.lock().get()!=this || !frame ||
            receipt->stamp().frame_number!=frame || !source_current(s)) return false;
        const auto now=stamp(frame); const auto &old=receipt->stamp();
        if(now.state_generation!=old.state_generation || now.active_player!=old.active_player || now.active_team!=old.active_team ||
            now.turn!=old.turn || now.width!=old.width || now.height!=old.height || now.wrap_x!=old.wrap_x || now.wrap_y!=old.wrap_y ||
            now.map_guid!=old.map_guid || now.debug_cache!=old.debug_cache || now.source.payload_sha256!=old.source.payload_sha256 || now.source.rules_sha256!=old.source.rules_sha256) return false;
        const auto &visibility=runtime::bnw_live_plot_visibility_cache(*state);
        for(const auto &plot:receipt->plots()) if(read(plot.plot_index,visibility,now.debug_cache)!=plot) return false;
        return source_current(s) && debug_cache_read()==old.debug_cache && source_current(s);
    }
};
BnwResourceArtTagReadReceipt::BnwResourceArtTagReadReceipt(BnwResourceArtTagReadStamp stamp,
    std::vector<BnwResourceArtTagReadFacts> facts,std::weak_ptr<const void> owner)
    :stamp_(std::move(stamp)),plots_(std::move(facts)),owner_(std::move(owner)) {}
BnwResourceArtTagReadOwner::BnwResourceArtTagReadOwner(const runtime::LiveSession &session,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const runtime::SessionCheckpointIdentity &identity,std::int64_t player,std::uint64_t maximum) {
    auto current=database.immutable_preview_fingerprint_currentness();
    require(current && current(),"Resource art requires explicit SAME immutable preview Rules scope");
    require(identity.payload_sha256==runtime::identify_bytes(session.source().source.bytes).sha256 &&
        identity.rules_sha256==database.gameplay_fingerprint_sha256(),"Resource art source/rules identity differs");
    require(same_types(types,database.cache_info_types()),"Resource art supplied global type cache differs from SAME Rules scope");
    auto impl=std::make_shared<Impl>(session,types,identity,player,std::move(current));
    const auto collection=database.prefetch_info("Resources",[&](std::uint64_t slot,std::int64_t id,const content::RulesRowView &row) {
        impl->infos.emplace(slot,Impl::Info{id,row.text("Type"),{row.text("ArtDefineTag"),row.text("ArtDefineTagHeavy"),
            row.text("ArtDefineTagAlt"),row.text("ArtDefineTagAltHeavy")}}); return true;
    },{maximum});
    require(collection.all_succeeded(),"Resource art Resources collection read failed"); impl->span=collection.slots;
    constexpr std::array<const char *,9> keys{"RESOURCE_FUR","RESOURCE_OIL","RESOURCE_ALUMINUM","RESOURCE_COAL",
        "RESOURCE_GEMS","RESOURCE_GOLD","RESOURCE_IRON","RESOURCE_MARBLE","RESOURCE_URANIUM"};
    const auto cached=[&](const char *name) {
        const auto *entry=impl->types.find(name);
        // Preserve original low16 MOVW result/sign-extension, including missing -1.
        const auto value=entry ? entry->id : std::int64_t{-1};
        require(value>=INT32_MIN && value<=INT32_MAX,"Resource art type-cache value exceeds native int32");
        const auto bits=static_cast<std::uint16_t>(value);
        return static_cast<std::int16_t>(bits<=INT16_MAX ? static_cast<std::int32_t>(bits) : static_cast<std::int32_t>(bits)-65536);
    };
    for(std::size_t i=0;i<keys.size();++i) impl->resource_cache[i]=cached(keys[i]);
    impl->marsh=cached("FEATURE_MARSH");
    impl->defines=database.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto *heavy=impl->defines.find("HEAVY_RESOURCE_THRESHOLD");
    require(heavy && heavy->lookup.error.empty() && std::holds_alternative<std::int64_t>(heavy->lookup.value),
        "Resource art Heavy source-cache query failed");
    const auto threshold=std::get<std::int64_t>(heavy->lookup.value);
    require(threshold>=INT32_MIN && threshold<=INT32_MAX,"Resource art Heavy threshold exceeds native signed int32");
    impl->threshold=static_cast<std::int32_t>(threshold);
    require(impl->source_current(session),"Resource art selected current session/map/player source differs");
    impl_=std::move(impl);
}
BnwResourceArtTagReadOwner::~BnwResourceArtTagReadOwner() { if(impl_) impl_->retired=true; }
std::shared_ptr<const BnwResourceArtTagReadReceipt> BnwResourceArtTagReadOwner::capture(
    const runtime::LiveSession &session,std::span<const std::uint64_t> indices,std::uint64_t frame) {
    const auto impl=impl_; Impl::Call call(*impl); if(!call.entered) return {};
    impl->latest.reset(); if(!frame || indices.empty() || indices.size()>1024*1024) return {};
    const std::vector<std::uint64_t> requested(indices.begin(),indices.end());
    if(!impl->source_current(session)) return {};
    const auto captured_stamp=impl->stamp(frame);
    const auto &visibility=runtime::bnw_live_plot_visibility_cache(*impl->state);
    std::set<std::uint64_t> unique; std::vector<BnwResourceArtTagReadFacts> facts; facts.reserve(requested.size());
    for(const auto index:requested) { require(unique.insert(index).second,"Resource art duplicate requested plot"); facts.push_back(impl->read(index,visibility,captured_stamp.debug_cache)); }
    auto receipt=std::shared_ptr<const BnwResourceArtTagReadReceipt>(new BnwResourceArtTagReadReceipt(captured_stamp,std::move(facts),impl));
    if(!impl->source_current(session)) return {};
    impl->latest=receipt;
    if(!impl->current(session,receipt,frame)) { impl->latest.reset(); return {}; }
    return receipt;
}
bool BnwResourceArtTagReadOwner::matches(const runtime::LiveSession &session,
    const std::shared_ptr<const BnwResourceArtTagReadReceipt> &borrowed,std::uint64_t frame) const noexcept {
    const auto receipt=borrowed; const auto impl=impl_; Impl::Call call(*impl); if(!call.entered) return false;
    try { return impl->current(session,receipt,frame); } catch(...) { return false; }
}
bool BnwResourceArtTagReadOwner::visit_current_resource_facts(const runtime::LiveSession &session,
    const std::shared_ptr<const BnwResourceArtTagReadReceipt> &borrowed,std::uint64_t frame,
    const std::function<void(std::span<const BnwResourceArtTagReadFacts>)> &visitor) noexcept {
    const auto receipt=borrowed; const auto impl=impl_; Impl::Call call(*impl); if(!call.entered) return false;
    try {
        const auto visitor_pin=visitor;
        if(!visitor_pin || !impl->current(session,receipt,frame)) return false;
        visitor_pin(receipt->plots()); return impl->current(session,receipt,frame);
    } catch(...) { impl->poisoned=true; return false; }
}
} // namespace civ5::ui
