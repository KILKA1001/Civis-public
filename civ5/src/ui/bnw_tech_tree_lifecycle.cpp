#include "civ5/ui/bnw_tech_tree_lifecycle.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <array>
#include <atomic>
#include <limits>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace civ5::ui {
namespace {
std::atomic<std::uint64_t> next_occurrence{1};
std::uint64_t allocate_occurrence() {
    auto value=next_occurrence.load(std::memory_order_relaxed);
    for (;;) {
        if (!value || value==std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("TechTree Civis occurrence space exhausted");
        if (next_occurrence.compare_exchange_weak(value,value+1,std::memory_order_relaxed)) return value;
    }
}
bool equal_event(const ProgressionTechnologyTreeEvent &a,const ProgressionTechnologyTreeEvent &b) {
    return a.player_id==b.player_id && a.turn==b.turn && a.token==b.token;
}
struct StockPin {const char *key,*path,*sha;};
constexpr std::array<StockPin,5> stock{{
    {"Assets/UI/InGame/TopPanel.lua","Assets/DLC/Expansion2/UI/InGame/TopPanel.lua","e1fa9bbeb198fb85fe71b101b6f11ac048721551be5e9de10259bbf20c167aef"},
    {"Assets/UI/InGame/TopPanel.xml","Assets/DLC/Expansion2/UI/InGame/TopPanel.xml","2325da5a03ff39daca1910089891dbd709b916b56ad4470e85c215fd12b065ad"},
    {"Assets/UI/InGame/TechTree/TechTree.lua","Assets/DLC/Expansion2/UI/TechTree/TechTree.lua","2caafe619194a7f599d52d92cc3f4114c3a054ced78b1f8720b651cf78283488"},
    {"Assets/UI/InGame/TechTree/TechTree.xml","Assets/DLC/Expansion2/UI/InGame/TechTree/TechTree.xml","cdbdfd89ca6d77ef850f94b03a167b49abd0952cd7b8944877c4f9a29dbd1e2b"},
    {"Assets/UI/InGame/InGame.xml","Assets/DLC/Expansion2/UI/InGame/InGame.xml","94400b55698da6b4b54aed1f6c6f477c2faa1149edf78f72a4d2430dc640a8f4"}
}};
// Loose stock sources may use the payload root or the selected package root.
// Join the real provenance basis without touching the filesystem or rewriting it.
bool stock_source_path(const assets::AssetCandidate &candidate,const StockPin &pin) {
    const auto &p=candidate.provenance;
    const assets::AssetKey relative(p.source_relative_path);
    const auto root=std::filesystem::u8path(p.source_id);
    if (p.kind!=assets::SourceKind::loose || p.archive_relative_path ||
        !root.is_absolute() || !relative.safe_relative_path()) return false;
    const auto joined=(root/std::filesystem::u8path(p.source_relative_path)).lexically_normal().generic_u8string();
    const std::string physical(joined.begin(),joined.end());
    const auto expected=assets::normalize_asset_name(pin.path);
    const auto original=assets::normalize_asset_name(candidate.original_name);
    return assets::normalize_asset_name(physical).ends_with("/"+expected) &&
        candidate.normalized_name==original &&
        (original==expected || original==relative.normalized_name());
}
std::string identity(const assets::MountedResource &selected,const StockPin &pin) {
    const auto *handle=std::get_if<assets::AssetHandle>(&selected.asset);
    if (!handle) throw std::runtime_error("TechTree selected stock source missing or ambiguous");
    const auto &c=handle->candidate();const auto &p=c.provenance;
    const auto bytes=handle->read_bytes(4ull*1024*1024);
    const auto sha=runtime::identify_bytes(bytes).sha256;
    if (sha!=pin.sha || !stock_source_path(c,pin) || bytes.size()!=c.payload_size) {
        std::ostringstream error;
        error<<"TechTree selected source is outside the inspected stock contract"
             <<": key=["<<pin.key<<"] expected_path=["<<pin.path<<"] expected_sha=["<<pin.sha
             <<"] candidate=["<<c.original_name<<"] source_id=["<<p.source_id
             <<"] source_relative=["<<p.source_relative_path<<"] archive=["<<p.archive_relative_path.value_or("")
             <<"] kind="<<static_cast<int>(p.kind)<<" actual_sha=["<<sha
             <<"] bytes="<<bytes.size()<<" declared_bytes="<<c.payload_size;
        throw std::runtime_error(error.str());
    }
    std::ostringstream out;
    const auto append=[&](std::string_view value){out<<value.size()<<':'<<value;};
    append(handle->requested_key().normalized_name());append(c.original_name);append(c.normalized_name);
    append(p.source_id);append(p.source_relative_path);append(p.archive_relative_path.value_or(""));append(sha);
    out<<':'<<static_cast<int>(p.kind)<<':'<<c.payload_size<<':'<<(selected.mount_id?std::to_string(*selected.mount_id):"default");
    return out.str();
}
std::string identity(const assets::ResourceMountTable &resources,const StockPin &pin) {
    return identity(resources.resolve(assets::AssetKey(std::string(pin.key))),pin);
}
bool same_xml(const content::XmlNode &a,const content::XmlNode &b) {
    if (a.name!=b.name || a.text!=b.text || a.attributes!=b.attributes ||
        a.children.size()!=b.children.size()) return false;
    for (std::size_t i=0;i<a.children.size();++i)
        if (!same_xml(a.children[i],b.children[i])) return false;
    return true;
}
struct FrameEvidence {
    const void *owner=nullptr;
    std::uint64_t owner_id=0,epoch=0,frame=0,generation=0;
    std::int64_t player=-1;
    std::int32_t turn=-1;
    std::optional<ProgressionTechnologyTreeEvent> event;
    // Private base fences: never expose a general sanitizer to callers.
    std::shared_ptr<const UiOwnerFences> base;
    std::shared_ptr<const UiOwnerFences> full;
};
struct ClearBusy {bool &busy;~ClearBusy(){busy=false;}};
}
struct BnwTechTreeLifecycle::Impl {
    const runtime::LiveSession &session;
    const std::int64_t player,team;
    const std::int32_t turn;
    const std::uint64_t generation;
    UiScreenController &screens;
    const assets::ResourceMountTable &resources;
    Observer observer;
    const std::thread::id thread=std::this_thread::get_id();
    const std::uint64_t id=allocate_occurrence();
    std::array<std::string,stock.size()> sources;
    ProgressionScreenSource source;
    std::uint64_t epoch=1;
    std::uint64_t publication=0;
    mutable bool busy=false;
    bool poisoned=false,open=false;
    std::int32_t count=0;
    std::optional<ProgressionTechnologyTreeEvent> event;
    std::optional<std::string> parent;
    std::optional<UiFrameSnapshot> queue_frame;
    std::shared_ptr<const FrameEvidence> latest,completed_evidence;
    const BnwClientFrameOwner *client_owner=nullptr;
    std::shared_ptr<const BnwClientFrame> completed_frame;
    std::vector<BnwTechTreeEffectObservation> effects;

    Impl(const runtime::LiveSession &s,std::int64_t p,UiScreenController &c,
         const assets::ResourceMountTable &r,Observer sink)
        :session(s),player(p),team(s.state().setup.players.at(p).team),
         turn(static_cast<std::int32_t>(s.state().setup.game_turn)),generation(s.state_generation()),
         screens(c),resources(r),observer(std::move(sink)) {
        if (!native_current() || !screens.registry().find("TechTree") || screens.visible("TechTree"))
            throw std::invalid_argument("TechTree lifecycle requires a current player and unqueued shared context");
        for (std::size_t i=0;i<stock.size();++i) sources[i]=identity(resources,stock[i]);
        source=read_progression_screen_source(resources,ProgressionScreen::technologies);
        const auto selected_tree=resources.resolve(assets::AssetKey(std::string(stock[3].key)));
        const auto *tree=std::get_if<assets::AssetHandle>(&selected_tree.asset);
        if (!tree || identity(selected_tree,stock[3])!=sources[3] ||
            source.requested_key!=stock[3].key ||
            assets::normalize_asset_name(source.selected_source_path)!=
                assets::normalize_asset_name(tree->candidate().provenance.source_relative_path))
            throw std::runtime_error("TechTree source changed during construction");
        // The shared DTO preserves only the raw relative path and parsed XML.
        // Attest its actual document against the same frozen selected XML.
        const auto tree_bytes=tree->read_bytes(4ull*1024*1024);
        if (tree_bytes.size()!=tree->candidate().payload_size ||
            runtime::identify_bytes(tree_bytes).sha256!=stock[3].sha ||
            !same_xml(source.document,content::read_xml_metadata(
                std::string(tree_bytes.begin(),tree_bytes.end()))) || !sources_current())
            throw std::runtime_error("TechTree source changed during construction");
    }
    bool same_thread() const noexcept {return thread==std::this_thread::get_id();}
    bool native_current() const {
        if (session.state_generation()!=generation) return false;
        const auto &s=session.state();const auto found=s.setup.players.find(player);
        if (found==s.setup.players.end() || !found->second.alive || found->second.team!=team || team<0 ||
            s.setup.game_turn!=turn || !s.world.pregame) return false;
        const auto active=s.world.pregame->archive.integers.find("active_player");
        return active!=s.world.pregame->archive.integers.end() && active->second==player;
    }
    bool sources_current() const {
        for (std::size_t i=0;i<stock.size();++i) if (identity(resources,stock[i])!=sources[i]) return false;
        return native_current(); // Resolve observers may restore/change active player.
    }
    bool shape_current() const {
        if (open) return event && count==1 && screens.visible("TechTree") &&
            screens.focused_screen()==std::optional<std::string>{"TechTree"};
        return !event && count==0 && !screens.visible("TechTree");
    }
    bool valid() const {return !poisoned && native_current() && sources_current() && shape_current();}
    void retire() {
        if (epoch==std::numeric_limits<std::uint64_t>::max()) {poisoned=true;throw std::overflow_error("TechTree epoch exhausted");}
        ++epoch;
    }
    void emit(BnwTechTreeEffect effect,const ProgressionTechnologyTreeEvent &occurrence) {
        effects.push_back({effect,occurrence,count,-1,0,false,false});
        if (observer) observer(effects.back());
        if (!native_current() || !sources_current()) throw std::runtime_error("TechTree owner/source changed during observed effect");
    }
    bool frame_shape(const UiFrameSnapshot &f) const {
        return f.frame()!=0 && f.active_player_id()==player && f.turn()==turn;
    }
    bool evidence_shape(const UiFrameSnapshot &f,const FrameEvidence &e) const {
        if (e.owner!=this || e.owner_id!=id || e.epoch!=epoch || e.generation!=generation || e.frame!=f.frame() ||
            e.player!=player || e.turn!=turn || !frame_shape(f)) return false;
        if (!e.full || f.find<UiOwnerFences>()!=e.full.get()) return false;
        const auto *published=f.find<ProgressionTechnologyTreeEvent>();
        if (bool(e.event)!=bool(event) || bool(published)!=bool(event)) return false;
        return !event || (equal_event(*e.event,*event) && equal_event(*published,*event));
    }
    // Keeps pointer/stamp/sequence and ALL other owner guards. Only this owner's
    // exact private epoch guard is retired by a successful/uncertain admission.
    bool admission_post(const BnwClientFrameOwner &client,const std::shared_ptr<const BnwClientFrame> &cf,
        const LiveUiHost &host,const LiveUiFrame &uf,const FrameEvidence &e) const {
        if (e.owner!=this || e.owner_id!=id || !cf || client.current_frame()!=cf || !native_current() ||
            !e.full || uf.snapshot.find<UiOwnerFences>()!=e.full.get() || cf->ui.find<UiOwnerFences>()!=e.full.get()) return false;
        if (!e.base || !e.base->current()) return false;
        auto sanitized=uf;sanitized.snapshot=uf.snapshot.with(e.base);
        if (!host.is_current_frame(sanitized) || client.current_frame()!=cf) return false;
        // Client publication may carry further fences distinct from host copy.
        const auto *ce=cf->ui.find<FrameEvidence>();
        return ce==&e && ce->base==e.base && native_current() && client.current_frame()==cf;
    }
    bool close_publication_current(const std::shared_ptr<const BnwClientFrame> &cf,const FrameEvidence &e) const {
        if (!client_owner || !cf || client_owner->current_frame()!=cf || e.owner!=this || e.owner_id!=id ||
            e.generation!=generation || !e.base || !e.full || cf->ui.find<FrameEvidence>()!=&e ||
            cf->ui.find<UiOwnerFences>()!=e.full.get() || e.frame!=cf->stamp.identity.frame || !native_current()) return false;
        return e.base->current() && native_current() && client_owner->current_frame()==cf;
    }
};
BnwTechTreeLifecycle::BnwTechTreeLifecycle(const runtime::LiveSession &s,std::int64_t p,
    UiScreenController &screens,const assets::ResourceMountTable &resources,Observer observer)
    :impl_(std::make_shared<Impl>(s,p,screens,resources,std::move(observer))) {}
BnwTechTreeLifecycle::~BnwTechTreeLifecycle()=default;

UiFrameSnapshot BnwTechTreeLifecycle::augment_snapshot(const runtime::LiveSession &s,const UiFrameSnapshot &f) const {
    const auto hold=impl_;auto &i=*hold;
    if (!i.same_thread() || i.busy || &s!=&i.session) throw std::logic_error("TechTree snapshot belongs to another owner/thread");
    i.busy=true;ClearBusy clear{i.busy};
    if (!i.valid() || !i.frame_shape(f) || f.find<FrameEvidence>() || f.find<ProgressionTechnologyTreeEvent>())
        throw std::logic_error("TechTree snapshot admission is stale or already owned");
    auto base=std::make_shared<UiOwnerFences>();if (const auto *original=f.find<UiOwnerFences>()) *base=*original;
    if (!base->current() || !i.native_current()) throw std::logic_error("TechTree base publication is stale");
    if (i.publication==std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("TechTree publication space exhausted");
    ++i.publication;
    auto fences=std::make_shared<UiOwnerFences>(*base);
    fences->checks.push_back([weak=std::weak_ptr<Impl>(impl_),epoch=i.epoch,generation=i.generation,frame=f.frame(),publication=i.publication] {
        const auto owner=weak.lock();if (!owner || !owner->same_thread() || owner->busy || owner->epoch!=epoch || owner->generation!=generation ||
            owner->publication!=publication || !owner->latest || owner->latest->frame!=frame) return false;
        owner->busy=true;ClearBusy clear{owner->busy};return owner->valid() && owner->epoch==epoch && owner->native_current() &&
            owner->publication==publication && owner->latest && owner->latest->frame==frame;
    });
    auto evidence=std::make_shared<const FrameEvidence>(FrameEvidence{&i,i.id,i.epoch,f.frame(),i.generation,i.player,i.turn,i.event,base,fences});
    auto result=f.with(evidence).with(std::shared_ptr<const UiOwnerFences>(fences));
    if (i.event) result=result.with(std::make_shared<const ProgressionTechnologyTreeEvent>(*i.event));
    if (!i.valid() || !base->current() || !i.native_current()) throw std::logic_error("TechTree changed during snapshot publication");
    i.latest=evidence;
    return result;
}
bool BnwTechTreeLifecycle::matches_snapshot(const runtime::LiveSession &s,const UiFrameSnapshot &f) const noexcept {
    try {
        const auto hold=impl_;auto &i=*hold;if (!i.same_thread() || i.busy || &s!=&i.session) return false;
        i.busy=true;ClearBusy clear{i.busy};const auto *e=f.find<FrameEvidence>();
        return e && e==i.latest.get() && i.evidence_shape(f,*e) && e->base && e->base->current() && i.valid() && i.evidence_shape(f,*e);
    } catch (...) {return false;}
}
std::optional<ProgressionTechnologyTreeEvent> BnwTechTreeLifecycle::active_event(const runtime::LiveSession &s) const noexcept {
    try {const auto hold=impl_;auto &i=*hold;if (!i.same_thread() || i.busy || &s!=&i.session) return {};
        i.busy=true;ClearBusy clear{i.busy};return i.valid()?i.event:std::nullopt;
    } catch (...) {return {};}
}
UiCommandResult BnwTechTreeLifecycle::admit_science_action(const runtime::LiveSession &s,
    const BnwClientFrameOwner &client,const std::shared_ptr<const BnwClientFrame> &borrowed_frame,
    const LiveUiHost &host,const LiveUiFrame &borrowed_ui,const TopPanelActionEvent &borrowed_action) {
    const auto hold=impl_;auto &i=*hold;
    if (!i.same_thread() || i.busy || &s!=&i.session) return {UiCommandStatus::invalid,"TechTree admission owner/thread/reentry mismatch"};
    // Copy arguments BEFORE any source/current callbacks. Callers commonly
    // borrow a vector element or replaceable optional/shared_ptr member.
    const auto action=borrowed_action;
    const auto cf=borrowed_frame;
    const auto uf=borrowed_ui;
    // Public current checks include this owner's guard, which must run before
    // the local busy section. Authenticate identities again after callbacks.
    if (!cf || !client.is_current(cf) || !host.is_current_frame(uf))
        return {UiCommandStatus::invalid,"TechTree shared publication is stale"};
    i.busy=true;ClearBusy clear{i.busy};
    const auto *e=uf.snapshot.find<FrameEvidence>();const auto *stamp=uf.snapshot.find<BnwClientFrameStamp>();
    try {
        if (action.action!=TopPanelAction::open_technology_tree || action.route!=TopPanelActionRoute::tech_tree_popup ||
            &host.screens()!=&i.screens || !i.valid() || i.open || !e || e!=cf->ui.find<FrameEvidence>() ||
            !i.evidence_shape(uf.snapshot,*e) || !stamp || stamp->identity!=cf->stamp.identity ||
            stamp->selection!=cf->stamp.selection || stamp->viewport!=cf->stamp.viewport ||
            cf->stamp.identity.player!=i.player || cf->stamp.identity.team!=i.team || cf->stamp.identity.turn!=i.turn ||
            cf->stamp.identity.map_guid!=s.state().world.map_header.guid || e->frame!=cf->stamp.identity.frame || client.current_frame()!=cf)
            return {UiCommandStatus::invalid,"TechTree action/source/frame is not admitted"};
        if (i.client_owner && i.client_owner!=&client) return {UiCommandStatus::invalid,"TechTree client owner changed without rebind"};
        if (const auto focus=i.screens.focused_screen()) {
            const auto *context=i.screens.registry().find(*focus);
            if (!context || context->modal) return {UiCommandStatus::invalid,"TechTree science action has modal focus"};
        }
        if (!i.admission_post(client,cf,host,uf,*e)) return {UiCommandStatus::invalid,"TechTree BEFORE publication changed"};
        const ProgressionTechnologyTreeEvent occurrence{i.player,i.turn,allocate_occurrence()};
        i.effects.reserve(i.effects.size()+7);i.parent=i.screens.focused_screen();i.queue_frame=uf.snapshot;
        i.client_owner=&client;i.completed_frame.reset();i.completed_evidence.reset();
        i.retire(); // One-shot even if a callback has uncertain effects.
        i.open=true;i.event=occurrence;
        try {
            const auto queued=i.screens.dispatch(UiOpenScreen{"TechTree"},uf.snapshot);
            if (queued.status!=UiCommandStatus::navigation || !i.screens.visible("TechTree") ||
                i.screens.focused_screen()!=std::optional<std::string>{"TechTree"}) throw std::runtime_error("TechTree queue effect not observed");
            i.emit(BnwTechTreeEffect::queued,occurrence);
            i.count=1;i.emit(BnwTechTreeEffect::show_semaphore_incremented,occurrence);
            i.emit(BnwTechTreeEffect::popup_shown,occurrence);
            if (!i.valid() || !i.admission_post(client,cf,host,uf,*e) || !i.shape_current() || !i.native_current())
                throw std::runtime_error("TechTree AFTER publication changed");
            return {UiCommandStatus::performed,"Civis local Queue/Show/PopupShown observed; nativeVM=false; RefreshDisplay=false"};
        } catch (...) {i.poisoned=true;return {UiCommandStatus::unavailable,"TechTree occurrence has uncertain local effects; retry prohibited"};}
    } catch (const std::exception &error) {return {UiCommandStatus::invalid,error.what()};}
}
UiCommandResult BnwTechTreeLifecycle::process(const ProgressionTechnologyTreeEvent &requested) {
    const auto hold=impl_;auto &i=*hold;if (!i.same_thread() || i.busy) return {UiCommandStatus::invalid,"TechTree close owner/thread/reentry mismatch"};
    i.busy=true;ClearBusy clear{i.busy};
    try {
        const auto cf=i.client_owner?i.client_owner->current_frame():nullptr;
        const auto evidence=i.latest;
        if (!i.valid() || !i.event || !equal_event(*i.event,requested) || !i.queue_frame || !evidence ||
            !cf || !i.evidence_shape(cf->ui,*evidence) || !i.close_publication_current(cf,*evidence))
            return {UiCommandStatus::invalid,"TechTree close occurrence is stale or foreign"};
        i.effects.reserve(i.effects.size()+4);i.retire();
        try {
            const auto closed=i.screens.dispatch(UiCloseScreen{"TechTree"},*i.queue_frame);
            if (closed.status!=UiCommandStatus::navigation || i.screens.visible("TechTree") || i.screens.focused_screen()!=i.parent)
                throw std::runtime_error("TechTree shared dequeue/parent effect not observed");
            i.emit(BnwTechTreeEffect::dequeued,requested);
            i.count=0;i.emit(BnwTechTreeEffect::hide_semaphore_decremented,requested);
            i.emit(BnwTechTreeEffect::popup_processed,requested);
            i.open=false;i.event.reset();i.queue_frame.reset();
            i.emit(BnwTechTreeEffect::open_flag_cleared,requested);
            if (!i.valid() || !i.close_publication_current(cf,*evidence) || !i.shape_current() ||
                i.screens.focused_screen()!=i.parent || !i.native_current() || i.client_owner->current_frame()!=cf)
                throw std::runtime_error("TechTree changed during close acknowledgement");
            i.completed_frame=cf;i.completed_evidence=evidence;
            return {UiCommandStatus::performed,"Civis local Dequeue/Hide/Processed0 observed; nativeVM=false; RefreshDisplay=false"};
        } catch (...) {i.poisoned=true;return {UiCommandStatus::unavailable,"TechTree close has uncertain local effects; retry prohibited"};}
    } catch (const std::exception &error) {return {UiCommandStatus::invalid,error.what()};}
}
const ProgressionScreenSource &BnwTechTreeLifecycle::source() const {
    if (!impl_->same_thread()) throw std::logic_error("TechTree source accessed from foreign thread");return impl_->source;
}
bool BnwTechTreeLifecycle::local_dequeue_observed(const ProgressionTechnologyTreeEvent &requested) const noexcept {
    try {
        const auto hold=impl_;auto &i=*hold;
        if (!i.same_thread() || i.busy) return false;
        i.busy=true;ClearBusy clear{i.busy};
        if (!i.valid() || i.event || i.count || i.screens.focused_screen()!=i.parent || i.effects.size()<4 ||
            !i.completed_evidence || !i.close_publication_current(i.completed_frame,*i.completed_evidence)) return false;
        constexpr std::array<BnwTechTreeEffect,4> expected{BnwTechTreeEffect::dequeued,BnwTechTreeEffect::hide_semaphore_decremented,
            BnwTechTreeEffect::popup_processed,BnwTechTreeEffect::open_flag_cleared};
        for (std::size_t n=0;n<expected.size();++n) {
            const auto &effect=i.effects[i.effects.size()-4+n];
            if (effect.effect!=expected[n] || !equal_event(effect.occurrence,requested) || effect.native_vm_executed || effect.refresh_display_executed) return false;
        }
        return i.close_publication_current(i.completed_frame,*i.completed_evidence) && i.native_current() &&
            i.shape_current() && i.screens.focused_screen()==i.parent;
    } catch (...) {return false;}
}
std::vector<BnwTechTreeEffectObservation> BnwTechTreeLifecycle::journal() const {
    if (!impl_->same_thread()) throw std::logic_error("TechTree journal accessed from foreign thread");return impl_->effects;
}
std::int32_t BnwTechTreeLifecycle::semaphore() const {
    if (!impl_->same_thread()) throw std::logic_error("TechTree semaphore accessed from foreign thread");return impl_->count;
}
bool BnwTechTreeLifecycle::uncertain() const {
    if (!impl_->same_thread()) throw std::logic_error("TechTree uncertainty accessed from foreign thread");return impl_->poisoned;
}
} // namespace civ5::ui
