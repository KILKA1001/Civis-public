#include "civ5/ui/selected_city_production_services.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include <iostream>
#include <thread>
#include <type_traits>
using namespace civ5;
namespace {
void need(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
template<class F>void refuses(F&& f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}need(rejected,"Selected production unsupported admission succeeded");}
}
int main(int argc,char** argv){
    try {
        need(argc==4,"Use actual selected.Civ5Save game-root shared-asset-root");
        static_assert(!std::is_default_constructible_v<ui::SelectedCityProductionServices>);
        static_assert(!std::is_default_constructible_v<ui::SelectedCityTrainCreateSource>);
        static_assert(!std::is_copy_constructible_v<ui::SelectedCityTrainCreateSource>);
        static_assert(!std::is_copy_constructible_v<ui::SelectedCityProductionServices>);
        static_assert(!std::is_default_constructible_v<runtime::BnwSelectedProductionActionLoan>);
        static_assert(!std::is_copy_constructible_v<runtime::BnwSelectedProductionActionLoan>);
        static_assert(!std::is_default_constructible_v<ui::SelectedMaintainPreparedAction>);
        static_assert(!std::is_copy_constructible_v<ui::SelectedMaintainPreparedAction>);
        const auto saved=runtime::identify_file(argv[1]);
        auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0,true);
        need(static_cast<bool>(selected.preview),"Actual selected production host import failed");
        const auto catalog=content::discover_packages({std::filesystem::path(argv[2])/"Assets/DLC"});
        need(catalog.diagnostics.empty(),"Actual selected production catalog unavailable");
        auto stock=std::make_shared<runtime::BnwSelectedStockLuaSourceOwner>(argv[2],catalog,selected.profile,selected.preview.host->identity());
        auto preview=std::make_unique<ui::SelectedBnwReadOnlyPreview>(std::move(selected.preview));
        std::shared_ptr<const ui::SelectedBnwPreviewHost> parent;
        runtime::LiveSession* live_session=nullptr;
        preview->with_owned_host_camera_input([&](auto actual,const auto* input){parent=std::move(actual);live_session=&input->session;});
        const auto map=parent->session().state().random.map.state(),game=parent->session().state().random.gameplay.state();
        const auto generation=parent->session().state_generation();
        const auto state=parent->session().read_state_lease();
        // Actual persistent owner, same parent. No new LuaScriptHost, fake load
        // completion, native window or controlled city is supplied by this test.
        const auto progress=stock->start_stock_bootstrap(parent,argv[3]);
        need(!progress.loading_complete&&!progress.gameplay_callbacks_ready,"Partial stock owner falsely completed production hooks");
        auto component=ui::SelectedCityProductionServices::from_selected_preview(*preview,stock);
        for(const auto& [id,city]:state->cities.players.at(parent->active_player_id()).cities){
            if(city.production_orders&&city.production_orders->empty()){
                const auto revision=state->ui_source_revision,occurrence=state->city_dirty_next_occurrence;
                refuses([&]{(void)component->advance_completed_train_component(parent->active_player_id(),id);});
                need(city.production_orders->empty()&&state->ui_source_revision==revision&&state->city_dirty_next_occurrence==occurrence,
                    "Natural empty queue produced TRAIN creation authority or mutated world");
            }
        }
        std::size_t train_native_observations=0;
        for(const auto& [id,city]:state->cities.players.at(parent->active_player_id()).cities){
            if(!city.production_orders||!city.production_orders->empty())continue;
            for(const auto unit:parent->production_rules().units){
                std::optional<ui::SelectedTrainPreflight> q;
                try{q=component->native_train_preflight(parent->active_player_id(),id,unit);}catch(const std::exception&){continue;}
                if(!q||!q->player_native_eligible||!q->city_native_eligible)continue;
                ++train_native_observations;
                const auto& actual=state->world.all_players->players.at(static_cast<std::size_t>(q->owner));
                const saves::PlayerTraitsField* no_train=nullptr;const saves::PlayerCoreField* making=nullptr;
                for(const auto& f:actual.traits.fields)if(f.name&&std::string_view(f.name)=="m_abNoTrain")no_train=&f;
                for(const auto& f:actual.core.fields)if(f.name&&std::string_view(f.name)=="m_paiUnitClassMaking")making=&f;
                need(no_train&&making&&no_train->values.at(static_cast<std::size_t>(q->unit_class))==0&&q->making_before==making->values.at(static_cast<std::size_t>(q->unit_class))&&
                    std::int64_t{q->making_after}==std::int64_t{q->making_before}+1,"Natural Train cache/making differs from independent raw oracle");
            }
        }
        if(!train_native_observations)std::cout<<"UNAVAILABLE natural Train native fixture; no city/tech/trait/order fabricated\n";
        const auto reason=component->unavailable_reason();
        need(reason.find("actual stock Lua epoch unavailable")!=std::string::npos,"Selected production discarded genuine stock frontier");
        const auto services=component->production_services();
        refuses([&]{(void)services.lua_ready();});
        const auto ui_preview=component->production_preview();
        need(ui_preview(*state,0,0,{runtime::BnwProductionKind::construct,0}).status==ui::UiCommandStatus::unavailable,"Unimplemented construct became ready");
        static_assert(!std::is_default_constructible_v<ui::SelectedConstructPreparedAction>);
        static_assert(!std::is_copy_constructible_v<ui::SelectedConstructPreparedAction>);
        std::size_t construct_observations=0;
        const auto actual_owner=parent->active_player_id();
        for(const auto& [id,city]:state->cities.players.at(actual_owner).cities){
            if(!city.production_orders||!city.production_orders->empty())continue;
            for(const auto building:parent->production_rules().buildings){
                std::optional<ui::SelectedConstructPreflight> result;
                try{result=component->native_construct_preflight(actual_owner,id,building);}catch(const std::exception&){continue;}
                if(!result||!result->player_native_eligible||!result->city_native_eligible)continue;
                ++construct_observations;
                // Independent direct raw scalar/cache oracle for the actual
                // naturally admitted class; no fake tech/queue/building grant.
                const auto& raw_player=state->world.all_players->players.at(static_cast<std::size_t>(actual_owner));
                const saves::PlayerCoreField* making=nullptr;
                for(const auto& f:raw_player.core.fields)if(f.name&&std::string_view(f.name)=="m_paiBuildingClassMaking")making=&f;
                need(making&&result->making_before==making->values.at(static_cast<std::size_t>(result->building_class))&&static_cast<std::int64_t>(result->making_after)==result->making_before+std::int64_t{1},
                    "Natural construct making delta differs from actual raw SDK int input");
                const auto original_queue=city.production_orders;
                const auto original_making=making->values;
                const auto original_revision=state->ui_source_revision;
                const auto action=component->execute_construct_command(actual_owner,id,building);
                need(action.disposition==runtime::SelectedProductionDisposition::blocked&&!action.native_committed,
                    "Partial stock owner issued construct without genuine hook/Help/city dirty owners");
                need(city.production_orders==original_queue&&making->values==original_making&&state->ui_source_revision==original_revision,
                    "Blocked construct changed actual queue/making/revision");
            }
        }
        if(!construct_observations)std::cout<<"UNAVAILABLE natural construct fixture: no actual eligible empty-queue ordinary building; no city/tech/cache fabricated\n";
        std::size_t cities=0,positive=0,unsupported=0;
        const auto owner=parent->active_player_id();
        const auto player=state->cities.players.find(owner);
        if(player!=state->cities.players.end())for(const auto& [id,city]:player->second.cities){
            ++cities;(void)city;
            for(const auto process:parent->production_rules().processes){
                std::optional<ui::SelectedMaintainPreflight> result;
                try{result=component->native_maintain_preflight(owner,id,process);}catch(const std::exception&){++unsupported;}
                if(!result)continue;
                if(!result->native_eligible){++unsupported;continue;}
                ++positive;
                // Real issuer only: this selected partial stock bootstrap has
                // no loaded production context/native dirty completion. A
                // missing loan must not write even though native checks pass.
                const auto orders_before=state->cities.players.at(owner).cities.at(id).production_orders;
                const auto revision_before=state->ui_source_revision;
                const auto events_before=state->city_dirty_events;
                const auto actual_action=component->execute_maintain_command(owner,id,process);
                need(actual_action.disposition==runtime::SelectedProductionDisposition::blocked&&!actual_action.native_committed,
                    "Partial actual stock owner issued a production command without its real context/dirty owner");
                need(state->cities.players.at(owner).cities.at(id).production_orders==orders_before&&state->ui_source_revision==revision_before&&state->city_dirty_events==events_before,
                    "Blocked real stock production loan wrote queue/revision/UI events");
                // Independent actual DB/cache oracle; do not accept the
                // component's own process_yields/before as expected operands.
                std::array<std::int64_t,6> expected_yields{};
                const auto report=parent->database().query_rows({"SELECT Yields.ID AS YieldID, Process_ProductionYields.Yield AS Amount FROM Processes INNER JOIN Process_ProductionYields ON Processes.Type=Process_ProductionYields.ProcessType INNER JOIN Yields ON Yields.Type=Process_ProductionYields.YieldType WHERE Processes.ID=?",{process}},[&](const auto& row){const auto y=row.integer("YieldID");need(y>=0&&y<6,"Actual independent yield row outside BNW enum");expected_yields[static_cast<std::size_t>(y)]=row.integer("Amount");});
                need(report.all_succeeded(),"Actual independent process source query failed");
                const auto* raw=runtime::resolve_bnw_current_city(*state,result->owner,result->city);
                need(raw!=nullptr,"Actual independent native city lookup failed");
                const saves::CityCoreField* modifier=nullptr;
                for(const auto& f:raw->core.fields)if(f.name&&std::string_view(f.name)=="m_aiProductionToYieldModifier"){need(!modifier,"Actual independent process cache duplicated");modifier=&f;}
                need(modifier&&modifier->values.size()==6,"Actual independent raw process cache missing");
                for(std::size_t y=0;y<6;++y)
                    need(result->before[y]==modifier->values[y]&&result->process_yields[y]==expected_yields[y]&&static_cast<std::int64_t>(result->after[y])==modifier->values[y]+expected_yields[y],"Actual process preflight differs from independent native source operands");
                need(ui_preview(*state,owner,id,{runtime::BnwProductionKind::maintain,process}).status==ui::UiCommandStatus::unavailable,"Native preflight opened unpublished Lua/UI action");
                auto scratch=*state;
                const auto orders=scratch.cities.players.at(owner).cities.at(id).production_orders;
                const auto dirty=scratch.city_dirty_events;
                std::array<std::int64_t,6> process_before{};
                for(std::size_t y=0;y<6;++y){const auto it=scratch.cities.players.at(owner).cities.at(id).yields.find(static_cast<std::int64_t>(y));process_before[y]=it==scratch.cities.players.at(owner).cities.at(id).yields.end()?0:it->second.process_modifier;}
                refuses([&]{services.apply_owned_effect(scratch,owner,id,runtime::BnwProductionOwnedEffect::start_process,{4,process,-1,false,false,std::nullopt});});
                need(scratch.cities.players.at(owner).cities.at(id).production_orders==orders&&scratch.city_dirty_events==dirty,"Mandatory typed UI refusal changed detached queue/events");
                for(std::size_t y=0;y<6;++y){const auto it=scratch.cities.players.at(owner).cities.at(id).yields.find(static_cast<std::int64_t>(y));need((it==scratch.cities.players.at(owner).cities.at(id).yields.end()?0:it->second.process_modifier)==process_before[y],"Mandatory typed UI refusal changed detached process cache");}
            }
        }
        need(parent->session().state_generation()==generation&&parent->session().read_state_lease().get()==state.get()&&parent->session().state().random.map.state()==map&&parent->session().state().random.gameplay.state()==game,"Production preflight or blocked stock fork changed real world/RNG");
        need(!parent->session().readiness().can_continue(),"Production preflight opened full gameplay continuation");
        std::weak_ptr<const ui::SelectedBnwPreviewHost> weak=parent;
        auto backend=component->make_backend();
        // A copied city DTO/turn is insufficient: a real source-revision frame
        // publication is mandatory even before an actual hook loan is borrowed.
        for(const auto& [id,city]:state->cities.players.at(owner).cities){
            if(!city.production_orders||!city.production_orders->empty())continue;
            const auto frame=ui::UiFrameSnapshot(1,owner,static_cast<std::int32_t>(state->setup.game_turn))
                .with(std::make_shared<const ui::CityScreenFrame>(
                    ui::capture_city_screen_frame(state->cities,state->world,owner,id)));
            if(!parent->production_rules().processes.empty()){
                const auto command=std::get<ui::UiGameplayCommand>(ui::city_choose_production(id,4,*parent->production_rules().processes.begin()));
                const auto refused=backend->availability(command,frame);
                need(refused.status==ui::UiCommandStatus::unavailable&&refused.reason=="Selected production needs the current source-revision client publication",
                    "Copied production city/turn DTO granted an actual client command");
            }
            break;
        }
        preview.reset();component.reset();parent.reset();
        need(!weak.expired(),"Backend callbacks lost actual selected parent lifetime");
        const auto still=weak.lock();need(still!=nullptr,"Actual selected parent was not retained");
        // No controlled city/state: use actual mutable_state to invalidate the
        // real import debug owner, then require refusal before any Lua callback.
        need(live_session!=nullptr,"Genuine mutable input session handoff missing");
        (void)live_session->mutable_state();
        refuses([&]{(void)services.lua_ready();});
        // Foreign calls reject before owner/world/Lua reads, even after local
        // invalidation. Joining makes the exception publication race-free.
        std::string foreign_reason;
        std::thread other([&]{try{(void)services.lua_ready();}catch(const std::exception& e){foreign_reason=e.what();}});other.join();
        need(foreign_reason=="Selected production foreign creator thread","Foreign production check did not run before mutable owner access");
        need(runtime::identify_file(argv[1]).sha256==saved.sha256,"Production fixture changed source save");
        std::cout<<"PASS genuine selected binding/current Lua+UI refusal; actual cities="<<cities<<", native empty-queue maintain preflights="<<positive<<", unsupported="<<unsupported<<"\n";
        if(!cities)std::cout<<"UNAVAILABLE production alpha fixture: selected active player has no actual imported city; requires genuine save with city or actual founding\n";
        if(!positive)std::cout<<"UNAVAILABLE native positive fixture: no naturally eligible empty-queue ordinary process; no fake tech/city/queue used\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
