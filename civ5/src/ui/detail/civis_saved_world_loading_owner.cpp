#include "civis_saved_world_loading_owner.hpp"
#include <atomic>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>
#include <immintrin.h>
#include <numeric>
#include "civis_renderer_loading_return.hpp"
#include "civ5/materials/material_setup.hpp"

namespace civ5::ui::detail {
namespace {void need(bool v,const char* e){if(!v)throw std::runtime_error(e);}}
struct CivisSavedWorldLoadingOwner::State final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false},busy{false};
    std::weak_ptr<const CivisSavedWorldLoadingOwner> self;
    std::shared_ptr<const CivisFrontendPartyWorldSource::LoadingControl> source;
    const saves::SavedSessionSnapshot* snapshot_identity=nullptr; // Compared, never dereferenced.
    std::unique_ptr<const saves::SavedSessionSnapshot> snapshot;
    std::unique_ptr<const saves::SavedMapTerrainSource> terrain_source;
    std::unique_ptr<const saves::SavedMapTeamArtObservation> art;
    std::vector<terrain::TerrainSourceCell> classified;
    std::vector<terrain::TerrainAuxiliaryPlot> auxiliary;
    std::vector<terrain::TerrainRiverPlot> rivers;
    std::uint64_t next_generation=0,active_generation=0;
    const LoadingInputsLoan* active_loan=nullptr;
    void creator_current()const {
        need(creator==std::this_thread::get_id(),"Saved loading inputs foreign creator");
    }
    void live()const {
        creator_current();
        need(alive.load()&&!failed.load()&&!self.expired()&&source&&snapshot&&terrain_source&&art,
             "Saved loading input owner retired/partial/quarantined");
    }
};
struct CivisSavedWorldLoadingOwner::Scope final {
    const std::shared_ptr<State> state;
    std::shared_ptr<const CivisSavedWorldLoadingOwner> pin;
    explicit Scope(const CivisSavedWorldLoadingOwner& owner):state(owner.state_) {
        state->creator_current();pin=state->self.lock();
        need(pin&&pin.get()==&owner&&pin->state_.get()==state.get(),"Saved loading facade/control replaced");
        state->live();
        if(state->busy.exchange(true)){state->failed.store(true);throw std::runtime_error("Saved loading input reentry");}
        try{check();}catch(...){state->failed.store(true);state->busy.store(false);throw;}
    }
    void check()const {
        state->live();
        need(state->busy.load()&&pin&&pin.use_count()>1,
             "Saved loading input lost last outside canonical facade");
    }
    ~Scope() {
        if(state->creator!=std::this_thread::get_id())return;
        if(!pin||pin.use_count()<=1)state->failed.store(true);
        state->busy.store(false); // Before the sole internal facade pin retires.
    }
    Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
};
CivisSavedWorldLoadingOwner::CivisSavedWorldLoadingOwner(std::shared_ptr<State> state):state_(std::move(state)){}
CivisSavedWorldLoadingOwner::~CivisSavedWorldLoadingOwner(){state_->alive.store(false);}

std::shared_ptr<const CivisSavedWorldLoadingOwner> CivisSavedWorldLoadingOwner::capture_owned(
    const CivisFrontendPartyWorldSource& actual) {
    // Parent's private authentic loan owns State and outside-owner accounting;
    // no new Parent facade strong pin or callback tuple is manufactured here.
    auto loan=actual.begin_loading_inputs_owned();
    need(bool(loan),"Saved loading authentic input loan absent");
    loan->require_current();
    auto state=std::make_shared<State>();loan->require_current();
    state->source=loan->loading_control_owned_without_callbacks();
    need(bool(state->source),"Saved loading control not issued by actual loan");
    loan->require_current(); // Actual control allocation must receive fresh Source POST before input reads.
    const auto* snapshot=&loan->snapshot_owned();
    state->snapshot=std::make_unique<const saves::SavedSessionSnapshot>(*snapshot);loan->require_current();
    state->snapshot_identity=snapshot;
    state->terrain_source=std::make_unique<const saves::SavedMapTerrainSource>(loan->terrain_source_owned());loan->require_current();
    state->art=std::make_unique<const saves::SavedMapTeamArtObservation>(loan->team_art_owned());loan->require_current();
    const auto& classified=loan->classified_owned();state->classified.assign(classified.begin(),classified.end());loan->require_current();
    const auto& auxiliary=loan->auxiliary_plots_owned();state->auxiliary.assign(auxiliary.begin(),auxiliary.end());loan->require_current();
    const auto& rivers=loan->river_plots_owned();state->rivers.assign(rivers.begin(),rivers.end());loan->require_current();
    need(&loan->snapshot_owned()==state->snapshot_identity&&
         state->snapshot->map_terrain&&state->terrain_source->cells.size()==state->terrain_source->grid.size()&&
         state->classified.size()==state->terrain_source->grid.size()&&
         state->auxiliary.size()==state->classified.size()&&state->rivers.size()==state->classified.size(),
         "Saved loading capture replaced actual snapshot or input cardinality");
    auto result=std::shared_ptr<const CivisSavedWorldLoadingOwner>(new CivisSavedWorldLoadingOwner(state));
    state->self=result;loan->require_current();
    loan->finish_owned();loan.reset();
    state->source->require_current_owned(); // Fresh source FULL AFTER actual loan retirement/DTOR.
    need(!state->failed.load()&&state->alive.load(),"Saved loading capture retired before publication");
    return result; // Input custody only; no factory/RCP/CRT/prior/order substitution.
}

CivisSavedWorldLoadingOwner::LoadingInputsLoan::LoadingInputsLoan(Scope& scope,
    CivisFrontendPartyWorldSource::LoadingInputsLoan& source)
    :scope_(scope),source_(source),generation_([&]{
        scope.check();source.require_current();scope.check();
        need(!scope.state->active_loan&&!scope.state->active_generation&&
             scope.state->next_generation!=std::numeric_limits<std::uint64_t>::max(),
             "Saved loading input occurrence replay/reentry/exhaustion");
        return ++scope.state->next_generation;
    }()) {
    scope_.state->active_generation=generation_;scope_.state->active_loan=this;
}
CivisSavedWorldLoadingOwner::LoadingInputsLoan::~LoadingInputsLoan() {
    if(scope_.state->creator!=std::this_thread::get_id())return;
    if(!complete_||scope_.state->active_loan!=this||scope_.state->active_generation!=generation_)
        scope_.state->failed.store(true);
    if(scope_.state->active_loan==this){scope_.state->active_loan=nullptr;scope_.state->active_generation=0;}
}
void CivisSavedWorldLoadingOwner::LoadingInputsLoan::require_current()const {
    scope_.check();
    need(!complete_&&scope_.state->active_loan==this&&scope_.state->active_generation==generation_,
         "Saved loading input physical address/generation retired");
    source_.require_current();scope_.check();
    need(&source_.snapshot_owned()==scope_.state->snapshot_identity&&
         scope_.state->active_loan==this&&scope_.state->active_generation==generation_,
         "Saved loading source snapshot/loan changed in callbacks");
    scope_.check();
}
void CivisSavedWorldLoadingOwner::LoadingInputsLoan::finish_owned(){require_current();complete_=true;}
const saves::SavedSessionSnapshot& CivisSavedWorldLoadingOwner::LoadingInputsLoan::snapshot_owned()const {
    require_current();return *scope_.state->snapshot;
}
const saves::SavedMapTerrainSource& CivisSavedWorldLoadingOwner::LoadingInputsLoan::terrain_source_owned()const {
    require_current();return *scope_.state->terrain_source;
}
const saves::SavedMapTeamArtObservation& CivisSavedWorldLoadingOwner::LoadingInputsLoan::team_art_owned()const {
    require_current();return *scope_.state->art;
}
std::span<const terrain::TerrainSourceCell> CivisSavedWorldLoadingOwner::LoadingInputsLoan::classified_owned()const {
    require_current();return scope_.state->classified;
}
std::span<const terrain::TerrainAuxiliaryPlot> CivisSavedWorldLoadingOwner::LoadingInputsLoan::auxiliary_plots_owned()const {
    require_current();return scope_.state->auxiliary;
}
std::span<const terrain::TerrainRiverPlot> CivisSavedWorldLoadingOwner::LoadingInputsLoan::river_plots_owned()const {
    require_current();return scope_.state->rivers;
}
void CivisSavedWorldLoadingOwner::require_current()const {
    Scope scope(*this);
    try{scope.state->source->require_current_owned();scope.check();}
    catch(...){scope.state->failed.store(true);throw;}
}
void CivisSavedWorldLoadingOwner::with_loading_inputs_owned(
    const std::function<void(const LoadingInputsLoan&)>& input)const {
    Scope scope(*this);
    try {
        auto source=CivisFrontendPartyWorldSource::begin_loading_inputs_from_control_owned(*scope.state->source);
        scope.check();need(bool(source),"Saved loading current authentic Parent loan absent");
        source->require_current();scope.check();
        {
            LoadingInputsLoan loan(scope,*source);
            {
                const auto consume=input;loan.require_current();
                need(bool(consume),"Saved loading input consumer absent");
                consume(loan);loan.require_current();
            } // Copied target DTOR stays inside actual Child+Parent loan scopes.
            loan.require_current();loan.finish_owned();
        }
        source->finish_owned();source.reset();scope.check();
        scope.state->source->require_current_owned();scope.check(); // Fresh POST after ALL loan retirement.
    }catch(...){scope.state->failed.store(true);throw;}
}
[[noreturn]] void CivisSavedWorldLoadingOwner::advance_world_layout_450430_owned()const {
    // Retaining honest selected inputs does not invent a reached loading call.
    require_current();
    throw std::runtime_error("Mandatory actual 450430 world-layout FactoryDescriptor/FP-RCP constructor return; then 60A7E0 CRT-thread occurrence cursor, retained prior/page/auxiliary-owner and scheduler returns");
}
#include "civis_renderer_loading_return_core.inc"
#include "civis_saved_world_loading_entry_core.inc"
}
