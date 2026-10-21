#include "civ5/runtime/bnw_research_choice_commit.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <climits>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>

namespace civ5::runtime {
namespace {
void need(bool ok,const char* why) {if(!ok)throw ResearchSelectionUnavailable(why);}
std::string_view name(const char* value) {return value?std::string_view(value):std::string_view{};}
template<class Range> auto& named(Range& range,std::string_view key) {
    auto* result=static_cast<std::remove_reference_t<decltype(*range.begin())>*>(nullptr);
    for(auto& value:range)if(name(value.name)==key) {need(!result,"Duplicate research choice raw field");result=&value;}
    need(result,"Missing research choice raw field");return *result;
}
struct RawQueue {
    saves::NamedSaveInteger* count;
    saves::PlotArrayObservation* queue;
    saves::TeamHashedArray* flags;
};
RawQueue raw_queue(LiveSessionState& state,std::int64_t actor) {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(actor>=0&&actor<64&&state.world.all_players&&
        state.world.all_players->layout==bnw&&state.world.all_players->players.size()==64,
        "Research choice needs actual BNW raw player roster");
    auto& player=state.world.all_players->players.at(static_cast<std::size_t>(actor));
    need(player.index==static_cast<std::uint64_t>(actor)&&player.saved_player_id==actor&&
        player.administration.layout==bnw&&player.knowledge.layout==bnw,
        "Research choice actual raw player identity differs");
    auto& pre=named(player.administration.blocks,"pre_city");
    auto& knowledge=named(player.knowledge.blocks,"player_techs");
    RawQueue result{&named(pre.fields,"m_researchQueue_wire_count"),
        &named(pre.arrays,"m_researchQueue"),&named(knowledge.hashes,"researching_tech")};
    need(result.queue->columns==1&&result.flags->wire_count>=0&&
        static_cast<std::uint64_t>(result.flags->wire_count)==result.flags->rows.size(),
        "Research choice raw queue/flag shape differs");
    const auto size=result.queue->values.size();
    need(size<=static_cast<std::size_t>(INT32_MAX)&&
        ((result.count->value<=0&&size==0)||(result.count->value>0&&
        static_cast<std::uint64_t>(result.count->value)==size)),
        "Research choice raw queue count differs");
    return result;
}
std::set<std::int64_t> checked_flags(const saves::TeamHashedArray& flags,
    const content::TechnologyRules& rules,const content::InfoTypeCatalog& types) {
    std::set<std::int64_t> result,mapped;
    for(const auto& row:flags.rows) {
        if(row.type_hash==0) {need(row.values.empty(),"Research choice malformed empty hash row");continue;}
        const auto* info=types.find_hash(row.type_hash);
        need(info&&info->table=="Technologies"&&info->id>=0&&
            static_cast<std::uint64_t>(info->id)<rules.collection.slots&&
            rules.find(static_cast<std::uint64_t>(info->id))&&mapped.insert(info->id).second&&
            row.values.size()==1&&(row.values[0]==0||row.values[0]==1),
            "Research choice unresolved/duplicate/nonboolean technology hash");
        if(row.values[0])result.insert(info->id);
    }
    return result;
}
bool same_rows(const saves::TeamHashedArray& a,const saves::TeamHashedArray& b) {
    return a.wire_count==b.wire_count&&a.rows.size()==b.rows.size()&&
        std::equal(a.rows.begin(),a.rows.end(),b.rows.begin(),[](const auto& x,const auto& y) {
            return x.type_hash==y.type_hash&&x.values==y.values;
        });
}
}
struct BnwResearchChoiceHooks::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
    std::weak_ptr<const BnwResearchChoiceHooks> self;
    BnwStockLuaAttemptLease attempt;
    const std::uint64_t serial;
    TestAll test_all;
    AcceptCommit accept_commit;
    Dirty dirty;
    bool claimed=false;
    Impl(std::shared_ptr<const ui::SelectedBnwPreviewHost> owner,BnwStockLuaAttemptLease loan,
        TestAll test,AcceptCommit accept,Dirty publish)
        :host(owner),attempt(std::move(loan)),serial(attempt.operation_serial()),
         test_all(std::move(test)),accept_commit(std::move(accept)),dirty(std::move(publish)) {}
};
BnwResearchChoiceHooks::BnwResearchChoiceHooks(std::unique_ptr<Impl> impl):impl_(std::move(impl)) {}
BnwResearchChoiceHooks::~BnwResearchChoiceHooks()=default;
std::shared_ptr<const BnwResearchChoiceHooks> BnwResearchChoiceHooks::issue(
    std::shared_ptr<const ui::SelectedBnwPreviewHost> host,BnwStockLuaAttemptLease attempt,
    TestAll test,AcceptCommit accept,Dirty dirty) {
    need(host&&test&&accept&&dirty,"Research choice genuine SourceOwner hooks incomplete");
    const auto serial=attempt.operation_serial();
    need(attempt.world_lease()==host->session().read_state_lease(),
        "Research choice active operation belongs to another world");
    auto result=std::shared_ptr<BnwResearchChoiceHooks>(new BnwResearchChoiceHooks(
        std::make_unique<Impl>(host,std::move(attempt),std::move(test),std::move(accept),std::move(dirty))));
    result->impl_->self=result;
    result->impl_->attempt.require_operation(serial);
    return result;
}
struct BnwResearchChoiceCommit::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
    std::shared_ptr<const BnwResearchChoiceHooks> hooks;
    std::shared_ptr<LiveSessionState> state;
    std::shared_ptr<const saves::SavedSession> source;
    std::uint64_t generation=0,before_revision=0,after_revision=0;
    std::int64_t actor=-1,active_team=-1,turn=-1;
    SessionCheckpointIdentity identity;
    std::function<bool()> rules_current;
    std::vector<std::int64_t> desired_queue;
    std::set<std::int64_t> desired_flags;
    saves::TeamHashedArray desired_raw_flags;
    BnwResearchChoicePhase phase=BnwResearchChoicePhase::prepared;
    std::size_t completed_dirty=0;
};
BnwResearchChoiceCommit::BnwResearchChoiceCommit(std::unique_ptr<Impl> impl):impl_(std::move(impl)) {}
BnwResearchChoiceCommit::~BnwResearchChoiceCommit()=default;
BnwResearchChoicePhase BnwResearchChoiceCommit::phase()const {
    need(impl_->thread==std::this_thread::get_id(),"Research choice receipt belongs to another thread");return impl_->phase;
}
std::uint64_t BnwResearchChoiceCommit::before_revision()const {(void)phase();return impl_->before_revision;}
std::uint64_t BnwResearchChoiceCommit::after_revision()const {(void)phase();return impl_->after_revision;}
std::int64_t BnwResearchChoiceCommit::player()const {(void)phase();return impl_->actor;}
std::int64_t BnwResearchChoiceCommit::team()const {(void)phase();return impl_->active_team;}
std::size_t BnwResearchChoiceCommit::completed_dirty_calls()const {(void)phase();return impl_->completed_dirty;}
const std::vector<std::int64_t>& BnwResearchChoiceCommit::queue()const {(void)phase();return impl_->desired_queue;}
void BnwResearchChoiceCommit::require_stored_current(const ui::SelectedBnwPreviewHost& selected)const {
    const auto& p=*impl_;
    need(p.thread==std::this_thread::get_id(),"Research choice receipt foreign thread before live borrow");
    const auto host=p.host.lock();
    need(host&&host.get()==&selected,"Research choice actual Host expired/substituted");
    const auto& session=host->session();
    need(p.phase!=BnwResearchChoicePhase::prepared&&session.read_state_lease()==p.state&&
        session.state_generation()==p.generation&&&session.source()==p.source.get()&&
        p.state->ui_source_revision==p.after_revision&&p.state->setup.game_turn==p.turn&&
        host->identity().payload_sha256==p.identity.payload_sha256&&
        host->identity().rules_sha256==p.identity.rules_sha256&&p.rules_current&&p.rules_current(),
        "Research choice stored world/source/rules revision retired");
    const auto raw=raw_queue(*p.state,p.actor);
    need(raw.count->value==static_cast<std::int64_t>(p.desired_queue.size())&&
        raw.queue->values==p.desired_queue&&same_rows(*raw.flags,p.desired_raw_flags)&&
        p.state->player_research.players.at(p.actor).queue==p.desired_queue&&
        p.state->research_turn.players.at(p.actor).researching==p.desired_flags,
        "Research choice raw/owned committed queue differs");
}
std::shared_ptr<const BnwResearchChoiceCommit> BnwResearchChoiceCommit::execute(
    LiveSession& session,std::shared_ptr<const BnwResearchChoiceHooks> supplied,
    const BnwResearchChoiceCommand& command) {
    need(bool(supplied),"Research choice requires genuine same-VM SourceOwner hooks");
    auto& binding=*supplied->impl_;
    need(binding.thread==std::this_thread::get_id(),"Research choice hooks foreign thread before world borrow");
    const auto hooks=binding.self.lock();
    need(hooks&&hooks.get()==supplied.get()&&!hooks.owner_before(supplied)&&!supplied.owner_before(hooks),
        "Research choice forged hook control");
    const auto host=binding.host.lock();
    need(host&&&host->session()==&session&&!binding.claimed,"Research choice SAME Host/once operation unavailable");
    binding.attempt.require_operation(binding.serial);
    need(binding.attempt.world_lease()==session.read_state_lease(),"Research choice actual operation world differs");
    need(session.bnw_game_debug_owner_current(),"Research choice genuine imported Game owner retired");
    auto p=std::make_unique<Impl>();
    p->host=host;p->hooks=hooks;p->state=session.state_;p->source=session.source_;
    p->generation=session.state_generation_;p->before_revision=p->state->ui_source_revision;
    p->actor=command.player;p->turn=p->state->setup.game_turn;p->identity=host->identity();
    p->rules_current=host->database().immutable_preview_fingerprint_currentness();
    auto& state=*p->state;
    need(command.expected_generation==p->generation&&command.expected_ui_source_revision==p->before_revision&&
        command.expected_turn==p->turn&&command.player==host->active_player_id()&&
        command.player==detail::bnw_game_active_player_read(state)&&command.player>=0&&command.player<64&&
        state.setup.players.at(command.player).alive&&
        detail::bnw_player_human_read(state,static_cast<std::int32_t>(command.player),host->database()),
        "Research choice stale/nonactive/nonhuman command");
    need(p->before_revision<std::numeric_limits<std::uint64_t>::max(),"Research choice UI revision exhausted");
    p->after_revision=p->before_revision+1;
    p->active_team=detail::bnw_game_active_team_read(state);
    need(p->active_team==state.setup.players.at(command.player).team&&
        state.setup.profile==content::TechnologyProfile::bnw&&
        state.research_selection.profile==content::TechnologyProfile::bnw&&
        host->technologies().profile==content::TechnologyProfile::bnw&&host->technologies().all_succeeded()&&
        host->defines().profile()==content::GlobalDefineProfile::bnw&&host->types().report().all_succeeded()&&
        command.technology>=0&&host->technologies().find(static_cast<std::uint64_t>(command.technology)),
        "Research choice actual BNW rules/technology unavailable");
    need(!state.game_clock.pending&&!state.player_research_publication&&!state.player_early_publication,
        "Research choice prior native publications unresolved");
    const auto raw=raw_queue(state,command.player);
    const auto old_queue=state.player_research.players.at(command.player).queue;
    const auto old_flags=state.research_turn.players.at(command.player).researching;
    const auto old_raw_flags=*raw.flags;
    const auto old_raw_count=raw.count->value;
    need(raw.queue->values==old_queue&&checked_flags(*raw.flags,host->technologies(),host->types())==old_flags&&
        state.research_turn.players.at(command.player).unmapped_researching==0&&
        std::set<std::int64_t>(old_queue.begin(),old_queue.end())==old_flags&&
        old_queue.size()==old_flags.size()&&!old_flags.contains(command.technology),
        "Research choice raw/owned queue mismatch or already queued technology");
    auto staged=state;
    std::vector<std::int64_t> dirty_teams;
    const auto state_pin=p->state;
    const auto source_pin=p->source;
    const auto generation=p->generation,before_revision=p->before_revision;
    const auto actor_id=p->actor,turn=p->turn;
    const auto rules_current=p->rules_current;
    auto require_precommit=[&] {
        need(session.state_==state_pin&&session.source_==source_pin&&session.state_generation_==generation&&
            session.bnw_game_debug_owner_current()&&state.ui_source_revision==before_revision&&state.setup.game_turn==turn&&
            state.player_research.players.at(actor_id).queue==old_queue&&
            state.research_turn.players.at(actor_id).researching==old_flags&&rules_current&&rules_current(),
            "Research choice source changed during observational Lua callback");
        const auto current_raw=raw_queue(state,actor_id);
        need(current_raw.queue==raw.queue&&current_raw.flags==raw.flags&&current_raw.count==raw.count&&
            current_raw.queue->values==old_queue&&current_raw.count->value==old_raw_count&&
            same_rows(*current_raw.flags,old_raw_flags),"Research choice raw fields changed during Lua callback");
        binding.attempt.require_operation(binding.serial);
    };
    struct Services final:ResearchSelectionServices {
        std::function<std::optional<bool>(std::string_view,std::int64_t,std::int64_t)> test;
        std::vector<std::int64_t>& dirty;
        Services(decltype(test) callback,std::vector<std::int64_t>& queue):test(std::move(callback)),dirty(queue) {}
        std::optional<bool> test_all(std::string_view event,std::int64_t player,std::int64_t tech)override {
            return test(event,player,tech);
        }
        void research_queue_dirty(std::int64_t team)override {dirty.push_back(team);}
    } services([&](std::string_view event,std::int64_t actor,std::int64_t technology) {
        require_precommit();
        need(staged.player_research.players.at(actor).queue==state.player_research.players.at(actor).queue&&
            staged.research_turn.players.at(actor).researching==state.research_turn.players.at(actor).researching,
            "Research choice staged mutation needs genuine VM getter binding");
        need(event=="PlayerCanEverResearch"||event=="PlayerCanResearch","Unexpected research Lua event");
        const auto result=binding.test_all(event=="PlayerCanResearch"?BnwResearchChoiceEvent::player_can_research:
            BnwResearchChoiceEvent::player_can_ever_research,actor,technology);
        require_precommit();return result;
    },dirty_teams);
    ResearchSelectionContext context{staged.research_selection,staged.research_turn,staged.player_research,
        staged.setup,host->technologies(),nullptr,staged.technologies,staged.maintenance.team_eras,
        research_prerequisite_limits(host->defines()),services,p->active_team,staged.science.no_science};
    require_precommit();binding.claimed=true; // Before actual VM callbacks; no uncertain attempt replay.
    need(can_research(context,p->actor,command.technology),"Technology is not currently researchable or actual Lua vetoed");
    need(push_research(context,p->actor,command.technology,command.clear_queue),"Actual research queue push rejected");
    p->desired_queue=staged.player_research.players.at(p->actor).queue;
    p->desired_flags=staged.research_turn.players.at(p->actor).researching;
    need(p->desired_queue.size()<=static_cast<std::size_t>(INT32_MAX)&&
        std::set<std::int64_t>(p->desired_queue.begin(),p->desired_queue.end())==p->desired_flags,
        "Research choice staged queue/flags inconsistent");
    p->desired_raw_flags=*raw.flags;
    std::set<std::int64_t> mapped;
    for(auto& row:p->desired_raw_flags.rows)if(row.type_hash) {
        const auto* type=host->types().find_hash(row.type_hash);
        need(type&&mapped.insert(type->id).second,"Research choice raw flag mapping changed");
        row.values[0]=p->desired_flags.contains(type->id)?1:0;
    }
    need(std::all_of(p->desired_flags.begin(),p->desired_flags.end(),[&](auto id){return mapped.contains(id);}),
        "Selected research technology has no genuine hashed row");
    auto raw_values=p->desired_queue;
    auto raw_rows=p->desired_raw_flags.rows;
    auto owned_queue=p->desired_queue;
    auto owned_flags=p->desired_flags;
    auto& live_owned_queue=state.player_research.players.at(p->actor).queue;
    auto& live_owned_flags=state.research_turn.players.at(p->actor).researching;
    need(dirty_teams.size()<=std::numeric_limits<std::size_t>::max()/3,
        "Research choice dirty count exhausted");
    auto result=std::shared_ptr<BnwResearchChoiceCommit>(new BnwResearchChoiceCommit(std::move(p)));
    auto& receipt=*result->impl_;
    require_precommit();
    need(&state.player_research.players.at(receipt.actor).queue==&live_owned_queue&&
        &state.research_turn.players.at(receipt.actor).researching==&live_owned_flags,
        "Research choice actual owned containers replaced during final bookend");
    // All allocations and callbacks precede this narrow, nonthrowing commit.
    // Neither the map nor semantic cache owners are replaced or invalidated.
    static_assert(noexcept(raw.queue->values.swap(raw_values)));
    static_assert(noexcept(raw.flags->rows.swap(raw_rows)));
    static_assert(noexcept(live_owned_queue.swap(owned_queue)));
    static_assert(noexcept(live_owned_flags.swap(owned_flags)));
    raw.queue->values.swap(raw_values);raw.flags->rows.swap(raw_rows);
    live_owned_queue.swap(owned_queue);live_owned_flags.swap(owned_flags);
    raw.count->value=static_cast<std::int64_t>(receipt.desired_queue.size());
    state.ui_source_revision=receipt.after_revision;receipt.phase=BnwResearchChoicePhase::committed;
    try {
        result->require_stored_current(*host);
        // Private SourceOwner validates this receipt and advances only its
        // active operation_world_revision; mandatory BEFORE new lease checks.
        receipt.phase=BnwResearchChoicePhase::uncertain;
        binding.accept_commit(*result);
        result->require_stored_current(*host);binding.attempt.require_operation(binding.serial);
        receipt.phase=BnwResearchChoicePhase::committed;
        for(const auto team:dirty_teams) {
            need(team==receipt.active_team,"Research choice staged dirty team changed");
            for(const auto kind:{BnwResearchChoiceDirty::research_buttons,BnwResearchChoiceDirty::game_data,
                BnwResearchChoiceDirty::score}) {
                result->require_stored_current(*host);binding.attempt.require_operation(binding.serial);
                receipt.phase=BnwResearchChoicePhase::uncertain; // Before actual external effect; no replay.
                binding.dirty(kind,team);
                result->require_stored_current(*host);binding.attempt.require_operation(binding.serial);
                ++receipt.completed_dirty;receipt.phase=BnwResearchChoicePhase::committed;
            }
        }
        receipt.phase=BnwResearchChoicePhase::complete;
    } catch(...) {receipt.phase=BnwResearchChoicePhase::uncertain;}
    return result;
}
}
