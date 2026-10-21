#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/runtime/bnw_map_occupied_preflight.hpp"
#include "civ5/runtime/bnw_player_pre_turn.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool value,const std::string &why) {
    if(!value) throw std::runtime_error(why);
}
std::int64_t &player_field(saves::PlayerCoreObservation &core,std::string_view name) {
    saves::PlayerCoreField *found=nullptr;
    for(auto &f:core.fields) if(f.name && std::string_view(f.name)==name) {
        require(!found,"Duplicate activation player scalar");found=&f;
    }
    require(found && found->values.size()==1 &&
            (found->encoding==saves::PlayerCoreEncoding::i32 || found->encoding==saves::PlayerCoreEncoding::boolean),
            "Missing activation player scalar: "+std::string(name));
    return found->values[0];
}
std::int64_t &unit_field(saves::UnitCoreObservation &core,std::string_view name) {
    saves::UnitCoreField *found=nullptr;
    for(auto &f:core.fields) if(f.name && std::string_view(f.name)==name) {
        require(!found,"Duplicate activation unit scalar");found=&f;
    }
    require(found && found->values.size()==1 &&
            (found->encoding==saves::UnitCoreEncoding::i32 || found->encoding==saves::UnitCoreEncoding::boolean),
            "Missing activation unit scalar: "+std::string(name));
    return found->values[0];
}
void find_tail(std::span<const saves::UnitTailNode> nodes,std::string_view name,
               const saves::UnitTailNode *&found) {
    for(const auto &node:nodes) {
        if(node.name && std::string_view(node.name)==name) {
            require(!found,"Duplicate activation unit tail scalar");found=&node;
        }
        find_tail(node.children,name,found);
    }
}
std::int64_t tail_scalar(const saves::UnitTailObservation &tail,std::string_view name) {
    const saves::UnitTailNode *found=nullptr;find_tail(tail.blocks,name,found);
    require(found && found->values.size()==1,"Missing activation unit tail scalar: "+std::string(name));
    return found->values[0];
}
std::int64_t maximum_hit_points(const content::RulesDatabase &rules) {
    std::optional<std::int64_t> value;
    const auto query=rules.query_rows({"SELECT Value FROM Defines WHERE Name = 'MAX_HIT_POINTS'",{}},
        [&](const auto &row) {require(!value,"Duplicate MAX_HIT_POINTS");value=row.integer_at(0);},2);
    require(query.all_succeeded() && value && *value>0,"Activation attrition MAX_HIT_POINTS is unavailable");
    return *value;
}
void validate_units(LiveSessionState &state,std::int64_t player) {
    const auto roster=preflight_bnw_map_occupied(state.world);
    require(roster.shape_valid,"Activation unit/map roster mismatch: "+roster.reason);
    auto &units=state.world.all_players->players.at(player).units;
    require(units.layout==bnw && units.header.wire_unit_count>=0 &&
            static_cast<std::uint64_t>(units.header.wire_unit_count)==units.units.size(),
            "Activation unit collection is incomplete");
    for(auto &unit:units.units) {
        require(unit.core.layout==bnw && unit.tail.layout==bnw &&
                unit_field(unit.core,"m_eOwner")==player,
                "Activation unit owner or profile changed");
        const auto id=unit_field(unit.core,"m_iID");
        const auto found=std::find_if(roster.units.begin(),roster.units.end(),[&](const auto &u) {
            return u.handle.owner==player && u.handle.id==id;
        });
        require(found!=roster.units.end() && found->saved_unit_found && found->at_saved_plot,
                "Activation unit is missing from its owned plot");
    }
}
void no_effect_attrition(LiveSessionState &state,std::int64_t player,
                         const content::RulesDatabase &rules) {
    validate_units(state,player);
    const auto max_hit_points=maximum_hit_points(rules);
    for(auto &unit:state.world.all_players->players.at(player).units.units) {
        const auto &tail=unit.tail;
        require((tail_scalar(tail,"m_iEnemyDamageChance")<=0 ||
                 tail_scalar(tail,"m_iEnemyDamage")<=0) &&
                (tail_scalar(tail,"m_iNeutralDamageChance")<=0 ||
                 tail_scalar(tail,"m_iNeutralDamage")<=0),
                "Activation attrition needs territory RNG/damage owner");
        const auto x=unit_field(unit.core,"m_iX"),y=unit_field(unit.core,"m_iY");
        const auto index=state.world.map_header.grid.index(x,y);
        require(state.world.map_projection && index<state.world.map_projection->cells.size(),
                "Activation attrition plot unavailable");
        const auto &raw=state.world.map_plots->plots.at(index);
        const saves::NamedSaveInteger *plot_type=nullptr;
        for(const auto &field:raw.fields) if(field.name && std::string_view(field.name)=="plot_type") {
            require(!plot_type,"Duplicate activation attrition plot type");plot_type=&field;
        }
        require(plot_type && plot_type->value==state.world.map_projection->cells.at(index).plot_type,
                "Activation attrition raw/projected plot type differs");
        // PLOT_MOUNTAIN=0. A mountain requires the actual domain/all-terrain
        // predicate and damage effect; it must never be silently accepted.
        require(state.world.map_projection->cells.at(index).plot_type!=0,
                "Activation mountain attrition needs domain and damage owner");
        const auto damage=unit_field(unit.core,"m_iDamage");
        require(damage>=0 && damage<max_hit_points,
                "Activation attrition needs destroyed-unit notification owner");
        require(tail_scalar(tail,"m_iReligiousStrengthLossRivalTerritory")<=0,
                "Activation religious attrition needs religion/territory/kill owner");
    }
}
bool owned_step(BnwGameActivationStep step) {
    using S=BnwGameActivationStep;
    return step==S::set_turn_active || step==S::publish_end_turn_dirty ||
           step==S::clear_end_turn || step==S::unit_attrition ||
           step==S::active_turn_count_log || step==S::publish_player_turn_start ||
           step==S::all_units_unprocessed;
}
void validate_diplomacy_begin_caller(LiveSessionState &state,std::int64_t player) {
    require(state.world.game_state.layout==bnw&&state.world.all_players&&state.world.all_players->players.size()==64&&player>=0&&player<64,"Activation BeginTurn current game/player registry unknown");
    const saves::NamedSaveFlag *final=nullptr;
    for(const auto &flag:state.world.game_state.flags)if(flag.name&&std::string_view(flag.name)=="final_initialized"){require(!final,"Duplicate activation final initialized flag");final=&flag;}
    require(final&&final->value,"Activation BeginTurn requires actual source final-initialized branch");
    auto &core=state.world.all_players->players.at(player).core;
    require(player_field(core,"m_bAlive")==1&&state.setup.players.at(player).alive&&player_field(core,"m_bTurnActive")==1,"Activation BeginTurn source current alive/active branch differs");
}
void run_owned(BnwGameActivationStep step,BnwGameActivationDraft &draft,
               const BnwGameActivationTicket &ticket,const content::RulesDatabase &rules) {
    using S=BnwGameActivationStep;
    auto &state=draft.state;const auto player=ticket.player;
    auto &core=state.world.all_players->players.at(player).core;
    switch(step) {
    case S::set_turn_active:
        require(player_field(core,"m_bTurnActive")==0,"Activation player is already active");
        player_field(core,"m_bTurnActive")=1;
        break;
    case S::publish_end_turn_dirty:
    case S::publish_player_turn_start:
        draft.owner.pending.push_back({step,player,{}});break;
    case S::clear_end_turn: {
        // The active human branch bypasses remote-player and AI busy guards.
        // Other players require the full busy/received-turn predicates.
        require(state.setup.is_human(state.setup.players.at(player)) &&
                player==ticket.active_player,
                "Activation setEndTurn needs remote/AI busy unit and city owner");
        auto &end=player_field(core,"m_bEndTurn");
        auto &automatic=player_field(core,"m_bAutoMoves");
        require((end==0 || end==1) && (automatic==0 || automatic==1),
                "Activation end-turn or automoves flag is not boolean");
        end=0;
        if(automatic) {
            automatic=0;draft.owner.processed_auto_moves[player]=false;
        }
        break;
    }
    case S::unit_attrition:no_effect_attrition(state,player,rules);break;
    case S::active_player_deferred_fog:
        require(draft.owner.deferred_fog.has_value(),"Activation deferred fog needs a tracked native client owner");
        stage_bnw_deferred_fog_flush(state,*draft.owner.deferred_fog);
        break;
    case S::pathfinder_cache_enter:
    case S::pathfinder_cache_restore: {
        require(draft.owner.pathfinder_cache.has_value(),"Activation retained pathfinder cache controls are missing");
        const auto &binding=draft.owner.pathfinder_cache->identity;
        require(binding.payload_sha256==draft.owner.identity.payload_sha256 &&
                binding.rules_sha256==draft.owner.identity.rules_sha256,
                "Activation pathfinder cache controls belong to another source/rules");
        if(step==S::pathfinder_cache_enter)
            enter_bnw_pathfinder_cache_scope(state,*draft.owner.pathfinder_cache,player);
        else restore_bnw_pathfinder_cache_scope(state,*draft.owner.pathfinder_cache,player);
        break;
    }
    case S::city_connections:
    case S::city_connection_gold: {
        require(draft.owner.city_connections.has_value(),"Activation city connection owner is missing");
        const auto &bound=draft.owner.city_connections->identity;
        require(bound.payload_sha256==draft.owner.identity.payload_sha256 &&
                bound.rules_sha256==draft.owner.identity.rules_sha256,
                "Activation city connection owner belongs to another source/rules");
        const auto result=step==S::city_connections?
            update_bnw_activation_city_connections(state,*draft.owner.city_connections,rules,player):
            update_bnw_activation_city_connection_gold(state,*draft.owner.city_connections,player);
        require(result.outcome!=LiveActionOutcome::rejected,result.reason);break;
    }
    case S::builder_tasking: {
        require(draft.owner.builder_tasking&&draft.owner.city_connections,"Activation Builder requires SAME retained Builder/CityConnections owners");
        const auto query_rules=BnwCityStrategyYieldRules::load(rules);
        const auto applied=update_bnw_activation_builder_tasking(state,*draft.owner.builder_tasking,*draft.owner.city_connections,query_rules,player);
        require(applied.outcome!=LiveActionOutcome::rejected,applied.reason);break;
    }
    case S::diplomacy_requests_begin: {
        require(draft.owner.diplomacy_requests.has_value(),"Activation BeginTurn requires SAME retained diplomacy Read/turn owner");
        validate_diplomacy_begin_caller(state,player);
        const auto applied=execute_bnw_diplomacy_begin_turn_scalar_draft(state,*draft.owner.diplomacy_requests,rules,player);
        require(static_cast<bool>(applied.draft),applied.blocker);
        state=std::move(applied.draft->state);draft.owner.diplomacy_requests=std::move(applied.draft->owner);break;
    }
    case S::active_turn_count_log: {
        state.player_read_tail.active_player_ids.clear();
        for(std::int64_t id=0;id<64;++id) {
            auto &entry=state.world.all_players->players.at(id).core;
            if(player_field(entry,"m_bAlive") && player_field(entry,"m_bTurnActive"))
                state.player_read_tail.active_player_ids.push_back(id);
        }
        require(state.player_read_tail.active_player_ids==std::vector<std::int64_t>{player},
                "Activation sequential active roster is not exactly one player");
        draft.owner.pending.push_back({step,player,
            "changeNumGameTurnActive(1) m_iNumActive=1 : setTurnActive() for player "+std::to_string(player)});
        break;
    }
    case S::all_units_unprocessed:
        validate_units(state,player);
        for(auto &unit:state.world.all_players->players.at(player).units.units) {
            auto &processed=unit_field(unit.core,"m_bAITurnProcessed");
            require(processed==0 || processed==1,"Activation unit processed flag is not boolean");
            processed=0;
        }
        break;
    default:throw std::logic_error("Unexpected native activation step");
    }
}
} // namespace

struct BnwGameActivationPendingAccess {
    static auto make(std::unique_ptr<BnwGameActivationDraft> activation,
        std::unique_ptr<BnwActivationHappinessOwner> happiness,
        const BnwGameActivationTicket &ticket,
        const std::vector<BnwGameActivationStep> &prefix) {
        auto pending=std::unique_ptr<BnwGameActivationHappinessPending>(
            new BnwGameActivationHappinessPending);
        pending->activation_=std::move(activation);
        pending->happiness_=std::move(happiness);
        pending->ticket_=ticket;
        pending->completed_prefix_=prefix;
        return pending;
    }
    static void validate(const BnwGameActivationExecution &result) {
        require(!result.draft&&result.pending_happiness&&result.completed_steps==result.pending_happiness->completed_prefix_&&!result.player_turn_calls&&!result.unit_turn_calls&&result.completed_player_phases.empty(),"Activation requires SAME retained enclosing native prefix; no replay/DoTurn");
    }
    static auto &grand_epoch(BnwGameActivationExecution &result) {
        require(!result.draft&&result.pending_happiness&&result.completed_steps==result.pending_happiness->completed_prefix_&&result.completed_steps.size()==14&&result.completed_steps.back()==BnwGameActivationStep::diplomacy_requests_begin&&result.player_turn_calls==1&&!result.unit_turn_calls&&result.completed_player_phases==std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},"Grand DoGuess requires SAME actual incomplete native DoTurn prefix");
        auto &pending=*result.pending_happiness;const auto &owner=pending.activation_->owner;
        require(pending.diplomacy_begin_complete_&&pending.builder_complete_&&pending.happiness_->native_update_complete()&&owner.diplomacy_requests&&owner.player_do_turn_prefix&&owner.player_do_turn_revolts&&owner.player_do_turn_trade&&owner.player_do_turn_military&&owner.grand_military_average,"Grand DoGuess lost preceding native owners/accepted military average");
        auto &epoch=pending.happiness_->suffix_->continuation.religion.query.epoch;
        require(epoch&&epoch->world&&epoch->host&&epoch->source_world==&pending.happiness_->prefix_->state()&&epoch->getter_world==epoch->world.get()&&epoch->all_selected_contexts_loaded&&epoch->all_listeners_registered&&epoch->all_external_writes_escrowed&&epoch->host->loading_complete(),"Grand DoGuess sole Lua/world epoch binding incomplete");
        validate_diplomacy_begin_caller(*epoch->world,pending.ticket_.player);return epoch;
    }
    static LiveActionResult grand_world_averages(BnwGameActivationExecution &result,const content::RulesDatabase &db) {
        try {
            auto &epoch=grand_epoch(result);auto &pending=*result.pending_happiness;auto &owner=pending.activation_->owner;
            require(!owner.grand_world_averages&&!owner.grand_guess_empty,"Grand world averages already accepted; no native query replay");
            const auto rules=BnwGrandWorldAverageRules::load(db);
            auto journal=stage_bnw_grand_world_average_journal(*epoch->world,*owner.player_do_turn_trade,*owner.player_do_turn_military,*owner.grand_military_average,rules,pending.happiness_->suffix_rules_,db,owner.identity,pending.ticket_.player);
            std::string frontier="Actual source culture/tourism/tech averages retained once by private journal; mandatory DoGuess foreign22loop next before own Grand priorities/RNG/decisions/shadow Diplomacy; NO Grand/phase3/step15/fullturn receipt";
            if(owner.player_do_turn_prefix->misc_buttons_delivery_pending()||!pending.happiness_->ui_consumed())frontier+="; staged MiscButtons/happiness UI delivery mandatory";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            static_assert(std::is_nothrow_move_assignable_v<std::optional<BnwGrandWorldAverageJournal>>);
            static_assert(std::is_nothrow_move_assignable_v<std::string>);
            owner.grand_world_averages=std::move(journal);result.blocker=std::move(frontier);return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_own_culture(BnwGameActivationExecution &result,
        const SavedContentProfile &profile,const content::RulesDatabase &db) {
        try {
            require(!result.draft&&result.pending_happiness&&
                result.completed_steps==result.pending_happiness->completed_prefix_&&
                result.completed_steps.size()==14&&result.player_turn_calls==1&&
                !result.unit_turn_calls&&result.completed_player_phases==
                    std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},
                "Grand own culture requires SAME incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;
            auto &happiness=*pending.happiness_;
            auto &owner=pending.activation_->owner;
            const auto rules=BnwGrandWorldAverageRules::load(db);
            // Allocate the outward result before any non-repeatable callback.
            std::string frontier="Actual own total JONS culture retained on SAME source epoch; mandatory own tourism, foreign63 culture/tourism comparisons and influential22 next before JonRand; NO Grand/phase3/step15/fullturn/publication receipt";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            if(!happiness.total_culture_) {
                auto &epoch=grand_epoch(result);
                require(owner.grand_world_averages&&owner.grand_guess_empty,
                    "Grand own culture requires accepted private averages and source DoGuess");
                const auto player=pending.ticket_.player;
                require(owner.grand_guess_empty->player()==player&&
                    owner.grand_guess_empty->query_revision()==bnw_grand_world_average_inputs_revision(*epoch->world)&&
                    owner.grand_guess_empty->grand_revision()==bnw_grand_native_component_revision(*epoch->world,player)&&
                    owner.player_do_turn_trade->after_world_revision()==bnw_player_do_turn_revolts_revision(*epoch->world,happiness.suffix_rules_,db,owner.identity,player)&&
                    owner.player_do_turn_military->after_world_revision()==bnw_player_military_registry_revision(*epoch->world,player)&&
                    owner.grand_military_average->input_revision()==bnw_grand_military_inputs_revision(*epoch->world),
                    "Grand own culture preceding accepted source inputs stale");
                const auto priority_rules=BnwGrandPriorityRules::load(db);
                auto first=quote_bnw_grand_first_priority(*epoch->world,priority_rules,
                    happiness.suffix_rules_,db,owner.identity,player,&*owner.grand_military_average,&rules);
                require(first.strategy==0&&first.strategy_type=="AIGRANDSTRATEGY_CULTURE"&&
                    first.base_quoted&&first.culture_prefix_quoted&&first.culture_initial.has_value(),
                    "Grand own culture requires actual enabled first Culture/GetBase prefix");
                // GetBase and the era/personality term precede the founder hook.
                // Store them once; Lua may legitimately change their inputs later.
                auto staged=stage_bnw_total_jons_culture(epoch,profile,owner.identity,
                    rules,happiness.suffix_rules_,db,player);
                static_assert(std::is_nothrow_move_assignable_v<std::optional<BnwGrandFirstPriorityQuote>>);
                pending.grand_first_prefix_=std::move(first);
                happiness.total_culture_=std::move(staged);
            }
            auto &total=*happiness.total_culture_;
            require(total.profile().enabled_dlc==profile.enabled_dlc&&
                total.profile().enabled_mods==profile.enabled_mods,
                "Grand own culture selected profile changed while query pending");
            require(total.status()!=BnwTotalJonsCultureStatus::discarded,
                "Grand own culture dispatched failure requires explicit recovery; no callback replay");
            require(total.epoch(),"Grand own culture already completed; no GetBase/query replay");
            if(total.status()==BnwTotalJonsCultureStatus::awaiting_founder) {
                const auto applied=continue_bnw_total_jons_culture(total,rules,happiness.suffix_rules_,db);
                if(!applied.applied) {
                    if(applied.nonrepeatable)happiness.stage_=BnwActivationHappinessStage::failed;
                    result.blocker=applied.blocker;
                    return {LiveActionOutcome::rejected,result.blocker};
                }
            }
            require(total.status()==BnwTotalJonsCultureStatus::complete&&total.quote().total.has_value(),
                "Grand own culture missing actual completed source result");
            auto &slot=happiness.suffix_->continuation.religion.query.epoch;
            require(!slot,"Grand own culture acquired a second enclosing epoch");
            slot=total.take_completed_epoch();
            result.blocker=std::move(frontier);
            return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_culture_priority(BnwGameActivationExecution &result,
        const SavedContentProfile &profile,const content::RulesDatabase &db) {
        try {
            require(!result.draft&&result.pending_happiness&&
                result.completed_steps==result.pending_happiness->completed_prefix_&&
                result.completed_steps.size()==14&&result.player_turn_calls==1&&
                !result.unit_turn_calls&&result.completed_player_phases==
                    std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},
                "Culture priority requires SAME incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;
            auto &happiness=*pending.happiness_;
            const auto rules=BnwGrandWorldAverageRules::load(db);
            std::string frontier="Actual Culture category retained on SAME source epoch; mandatory first JonRand and later Grand slots/decisions/shadow Diplomacy next; NO Grand/phase3/step15/fullturn/publication receipt";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            if(!happiness.grand_culture_priority_) {
                require(happiness.total_culture_&&
                    happiness.total_culture_->profile().enabled_dlc==profile.enabled_dlc&&
                    happiness.total_culture_->profile().enabled_mods==profile.enabled_mods,
                    "Culture priority requires SAME selected own-culture profile");
                auto &epoch=grand_epoch(result);
                happiness.grand_culture_priority_=stage_bnw_grand_culture_priority(
                    pending,epoch,rules,happiness.suffix_rules_,db);
            }
            auto &category=*happiness.grand_culture_priority_;
            require(category.profile().enabled_dlc==profile.enabled_dlc&&
                category.profile().enabled_mods==profile.enabled_mods,
                "Culture priority selected profile changed while pending");
            require(category.status()!=BnwGrandCulturePriorityStatus::discarded,
                "Culture priority dispatched failure requires explicit recovery; no callback replay");
            require(category.epoch(),"Culture priority already completed; no source category replay");
            if(category.status()!=BnwGrandCulturePriorityStatus::complete) {
                const auto advanced=continue_bnw_grand_culture_priority(category,
                    rules,happiness.suffix_rules_,db);
                if(!advanced.applied) {
                    if(advanced.nonrepeatable)happiness.stage_=BnwActivationHappinessStage::failed;
                    result.blocker=advanced.blocker;
                    return {LiveActionOutcome::rejected,result.blocker};
                }
            }
            (void)checked_bnw_grand_culture_priority(category,*category.epoch(),
                rules,happiness.suffix_rules_,db);
            auto &slot=happiness.suffix_->continuation.religion.query.epoch;
            require(!slot,"Culture priority acquired a second enclosing epoch");
            slot=category.take_completed_epoch();
            result.blocker=std::move(frontier);
            return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_first_roll(BnwGameActivationExecution &result,
        const SavedContentProfile &profile,const content::RulesDatabase &db) {
        try {
            require(!result.draft&&result.pending_happiness&&
                result.completed_steps==result.pending_happiness->completed_prefix_&&
                result.completed_steps.size()==14&&result.player_turn_calls==1&&
                !result.unit_turn_calls&&result.completed_player_phases==
                    std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},
                "First Grand roll requires SAME incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;
            auto &happiness=*pending.happiness_;
            require(!happiness.grand_first_roll_,
                "First Grand roll already stored; no RNG or priority replay");
            const auto *category=happiness.grand_culture_priority();
            require(category&&category->profile().enabled_dlc==profile.enabled_dlc&&
                category->profile().enabled_mods==profile.enabled_mods&&profile.can_activate&&
                profile.all_requirements_located,
                "First Grand roll requires SAME completed selected Culture profile");
            const auto rules=BnwGrandWorldAverageRules::load(db);
            // Prepare all fallible outward data before the factory's two stores.
            std::string frontier="First native JonRand/active boost/priorities[0] retained on SAME epoch; mandatory later Grand slots and strategy decision next; NO Grand/phase3/step15/fullturn/publication receipt";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            auto &epoch=grand_epoch(result);
            auto journal=stage_bnw_grand_first_roll(pending,*epoch,
                rules,happiness.suffix_rules_,db);
            static_assert(std::is_nothrow_move_constructible_v<BnwGrandFirstRollJournal>);
            static_assert(std::is_nothrow_move_constructible_v<LiveActionResult>);
            static_assert(std::is_nothrow_move_assignable_v<std::string>);
            // Empty optional + noexcept construction and string move: nothing
            // after the native stores can fail and advertise a false rollback.
            happiness.grand_first_roll_.emplace(std::move(journal));
            result.blocker=std::move(frontier);
            return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_united_nations_category(BnwGameActivationExecution &result,
        const SavedContentProfile &profile,const content::RulesDatabase &db) {
        try {
            require(!result.draft&&result.pending_happiness&&
                result.completed_steps==result.pending_happiness->completed_prefix_&&
                result.completed_steps.size()==14&&result.player_turn_calls==1&&
                !result.unit_turn_calls&&result.completed_player_phases==
                    std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},
                "UN category requires SAME incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;
            auto &happiness=*pending.happiness_;
            require(!happiness.grand_united_nations_category_,
                "UN category already stored; no source getter replay");
            const auto *roll=happiness.grand_first_roll();
            require(roll&&roll->profile().enabled_dlc==profile.enabled_dlc&&
                roll->profile().enabled_mods==profile.enabled_mods&&profile.can_activate&&
                profile.all_requirements_located,
                "UN category requires SAME privately completed first Grand roll");
            const auto rules=BnwGrandWorldAverageRules::load(db);
            std::string frontier="Native slot1 GetBase/UN category locals retained BEFORE second JonRand; mandatory RNG, later Grand slots and decision next; NO Grand/phase3/step15/fullturn/publication receipt";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            const auto &epoch=grand_epoch(result);
            auto journal=stage_bnw_grand_united_nations_category(pending,*epoch,
                rules,happiness.suffix_rules_,db);
            static_assert(std::is_nothrow_move_constructible_v<BnwGrandUnitedNationsPriorityJournal>);
            static_assert(std::is_nothrow_move_constructible_v<LiveActionResult>);
            static_assert(std::is_nothrow_move_assignable_v<std::string>);
            happiness.grand_united_nations_category_.emplace(std::move(journal));
            result.blocker=std::move(frontier);
            return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_second_roll(BnwGameActivationExecution &result,
        const SavedContentProfile &profile,const content::RulesDatabase &db) {
        try {
            require(!result.draft&&result.pending_happiness&&
                result.completed_steps==result.pending_happiness->completed_prefix_&&
                result.completed_steps.size()==14&&result.player_turn_calls==1&&
                !result.unit_turn_calls&&result.completed_player_phases==
                    std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},
                "Second Grand roll requires SAME incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;
            auto &happiness=*pending.happiness_;
            require(!happiness.grand_second_roll_,"Second Grand roll already stored; no replay");
            const auto *category=happiness.grand_united_nations_category();
            require(category&&category->profile().enabled_dlc==profile.enabled_dlc&&
                category->profile().enabled_mods==profile.enabled_mods&&profile.can_activate&&
                profile.all_requirements_located,
                "Second Grand roll requires SAME privately completed UN category");
            const auto rules=BnwGrandWorldAverageRules::load(db);
            std::string frontier="Second native JonRand/active boost/priorities[1] retained on SAME epoch; mandatory Spaceship/Conquest and strategy decision next; NO Grand/phase3/step15/fullturn/publication receipt";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            auto &epoch=grand_epoch(result);
            auto journal=stage_bnw_grand_second_roll(pending,*epoch,rules,happiness.suffix_rules_,db);
            static_assert(std::is_nothrow_move_constructible_v<BnwGrandSecondRollJournal>);
            static_assert(std::is_nothrow_move_constructible_v<LiveActionResult>);
            static_assert(std::is_nothrow_move_assignable_v<std::string>);
            happiness.grand_second_roll_.emplace(std::move(journal));
            result.blocker=std::move(frontier);
            return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_spaceship_category(BnwGameActivationExecution &result,
        const SavedContentProfile &profile,const content::RulesDatabase &db) {
        try {
            require(!result.draft&&result.pending_happiness&&
                result.completed_steps==result.pending_happiness->completed_prefix_&&
                result.completed_steps.size()==14&&result.player_turn_calls==1&&
                !result.unit_turn_calls&&result.completed_player_phases==
                    std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},
                "Spaceship category requires SAME incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;
            auto &happiness=*pending.happiness_;
            require(!happiness.grand_spaceship_category_,"Spaceship category already stored; no replay");
            const auto *roll=happiness.grand_second_roll();
            require(roll&&roll->profile().enabled_dlc==profile.enabled_dlc&&
                roll->profile().enabled_mods==profile.enabled_mods&&profile.can_activate&&
                profile.all_requirements_located,"Spaceship category requires SAME second Grand roll");
            const auto rules=BnwGrandWorldAverageRules::load(db);
            std::string frontier="Native slot2 GetBase/Spaceship category locals retained BEFORE third JonRand; mandatory later Grand effects; NO fullturn receipt";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            const auto &epoch=grand_epoch(result);
            auto journal=stage_bnw_grand_spaceship_category(pending,*epoch,rules,happiness.suffix_rules_,db);
            static_assert(std::is_nothrow_move_constructible_v<BnwGrandSpaceshipPriorityJournal>);
            static_assert(std::is_nothrow_move_constructible_v<LiveActionResult>);
            static_assert(std::is_nothrow_move_assignable_v<std::string>);
            happiness.grand_spaceship_category_.emplace(std::move(journal));
            result.blocker=std::move(frontier); return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_third_roll(BnwGameActivationExecution &result,
        const SavedContentProfile &profile,const content::RulesDatabase &db) {
        try {
            require(!result.draft&&result.pending_happiness&&
                result.completed_steps==result.pending_happiness->completed_prefix_&&
                result.completed_steps.size()==14&&result.player_turn_calls==1&&
                !result.unit_turn_calls&&result.completed_player_phases==
                    std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},
                "Third Grand roll requires SAME incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;
            auto &happiness=*pending.happiness_;
            require(!happiness.grand_third_roll_,"Third Grand roll already stored; no replay");
            const auto *category=happiness.grand_spaceship_category();
            require(category&&category->profile().enabled_dlc==profile.enabled_dlc&&
                category->profile().enabled_mods==profile.enabled_mods&&profile.can_activate&&
                profile.all_requirements_located,"Third Grand roll requires SAME Spaceship category");
            const auto rules=BnwGrandWorldAverageRules::load(db);
            std::string frontier="Third native JonRand/active boost/priorities[2] retained on SAME epoch; mandatory Conquest and strategy decision next; NO fullturn receipt";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            auto &epoch=grand_epoch(result);
            auto journal=stage_bnw_grand_third_roll(pending,*epoch,rules,happiness.suffix_rules_,db);
            static_assert(std::is_nothrow_move_constructible_v<BnwGrandThirdRollJournal>);
            static_assert(std::is_nothrow_move_constructible_v<LiveActionResult>);
            static_assert(std::is_nothrow_move_assignable_v<std::string>);
            happiness.grand_third_roll_.emplace(std::move(journal));
            result.blocker=std::move(frontier); return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_conquest_category(BnwGameActivationExecution &result,
        const SavedContentProfile &profile,const content::RulesDatabase &db) {
        try {
            require(!result.draft&&result.pending_happiness&&
                result.completed_steps==result.pending_happiness->completed_prefix_&&
                result.completed_steps.size()==14&&result.player_turn_calls==1&&
                !result.unit_turn_calls&&result.completed_player_phases==
                    std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},
                "Conquest category requires SAME incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;
            auto &happiness=*pending.happiness_;
            require(!happiness.grand_conquest_category_,"Conquest category already stored; no replay");
            const auto *roll=happiness.grand_third_roll();
            require(roll&&roll->profile().enabled_dlc==profile.enabled_dlc&&
                roll->profile().enabled_mods==profile.enabled_mods&&profile.can_activate&&
                profile.all_requirements_located,"Conquest category requires SAME third Grand roll");
            const auto rules=BnwGrandWorldAverageRules::load(db);
            std::string frontier="Native slot3 GetBase/Conquest and fresh source-gated military average retained BEFORE fourth JonRand; mandatory later Grand effects; NO fullturn receipt";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            const auto &epoch=grand_epoch(result);
            auto journal=stage_bnw_grand_conquest_category(pending,*epoch,rules,happiness.suffix_rules_,db);
            static_assert(std::is_nothrow_move_constructible_v<BnwGrandConquestPriorityJournal>);
            static_assert(std::is_nothrow_move_constructible_v<LiveActionResult>);
            static_assert(std::is_nothrow_move_assignable_v<std::string>);
            happiness.grand_conquest_category_.emplace(std::move(journal));
            result.blocker=std::move(frontier); return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_fourth_roll(BnwGameActivationExecution &result,
        const SavedContentProfile &profile,const content::RulesDatabase &db) {
        try {
            require(!result.draft&&result.pending_happiness&&
                result.completed_steps==result.pending_happiness->completed_prefix_&&
                result.completed_steps.size()==14&&result.player_turn_calls==1&&
                !result.unit_turn_calls&&result.completed_player_phases==
                    std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},
                "Fourth Grand roll requires SAME incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;
            auto &happiness=*pending.happiness_;
            require(!happiness.grand_fourth_roll_,"Fourth Grand roll already stored; no replay");
            const auto *category=happiness.grand_conquest_category();
            require(category&&category->profile().enabled_dlc==profile.enabled_dlc&&
                category->profile().enabled_mods==profile.enabled_mods&&profile.can_activate&&
                profile.all_requirements_located,"Fourth Grand roll requires SAME Conquest category");
            const auto rules=BnwGrandWorldAverageRules::load(db);
            std::string frontier="Fourth native JonRand/active boost/priorities[3] retained on SAME epoch; mandatory priority pressure and strategy decision next; NO fullturn receipt";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            auto &epoch=grand_epoch(result);
            auto journal=stage_bnw_grand_fourth_roll(pending,*epoch,rules,happiness.suffix_rules_,db);
            static_assert(std::is_nothrow_move_constructible_v<BnwGrandFourthRollJournal>);
            static_assert(std::is_nothrow_move_constructible_v<LiveActionResult>);
            static_assert(std::is_nothrow_move_assignable_v<std::string>);
            happiness.grand_fourth_roll_.emplace(std::move(journal));
            result.blocker=std::move(frontier); return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_pressure(BnwGameActivationExecution &result,
        const SavedContentProfile &profile,const content::RulesDatabase &db) {
        try {
            require(!result.draft&&result.pending_happiness&&
                result.completed_steps==result.pending_happiness->completed_prefix_&&
                result.completed_steps.size()==14&&result.player_turn_calls==1&&
                !result.unit_turn_calls&&result.completed_player_phases==
                    std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},
                "Grand pressure requires SAME incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;
            auto &happiness=*pending.happiness_;
            require(!happiness.grand_pressure_,"Grand pressure already stored; no replay");
            const auto *roll=happiness.grand_fourth_roll();
            require(roll&&roll->profile().enabled_dlc==profile.enabled_dlc&&
                roll->profile().enabled_mods==profile.enabled_mods&&profile.can_activate&&
                profile.all_requirements_located,"Grand pressure requires SAME fourth roll");
            const auto rules=BnwGrandWorldAverageRules::load(db);
            std::string frontier="Native Grand pressure/priorities4/strategy age retained on SAME epoch; mandatory adoption, specialization and logging effects next; NO fullturn receipt";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            auto &epoch=grand_epoch(result);
            auto journal=stage_bnw_grand_pressure(pending,*epoch,rules,happiness.suffix_rules_,db);
            static_assert(std::is_nothrow_move_constructible_v<BnwGrandPressureJournal>);
            static_assert(std::is_nothrow_move_constructible_v<LiveActionResult>);
            static_assert(std::is_nothrow_move_assignable_v<std::string>);
            happiness.grand_pressure_.emplace(std::move(journal));
            result.blocker=std::move(frontier); return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_selection(BnwGameActivationExecution &result,
        const SavedContentProfile &profile,const content::RulesDatabase &db,
        std::shared_ptr<const BnwGrandMinimumAgeOwner> minimum_age) {
        try {
            require(!result.draft&&result.pending_happiness&&
                result.completed_steps==result.pending_happiness->completed_prefix_&&
                result.completed_steps.size()==14&&result.player_turn_calls==1&&
                !result.unit_turn_calls&&result.completed_player_phases==
                    std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},
                "Grand selection requires SAME incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;
            auto &happiness=*pending.happiness_;
            require(!happiness.grand_selection_,"Grand selection already stored; no adoption replay");
            const auto *pressure=happiness.grand_pressure();
            require(pressure&&pressure->profile().enabled_dlc==profile.enabled_dlc&&
                pressure->profile().enabled_mods==profile.enabled_mods&&profile.can_activate&&
                profile.all_requirements_located,"Grand selection requires SAME retained pressure");
            const auto rules=BnwGrandWorldAverageRules::load(db);
            // Prepare both possible result/frontier allocations before the
            // source active/age/dirty writes. Retention afterwards cannot fail.
            std::string city_frontier="Native Grand selection/adoption and specialization dirty retained; mandatory LogSpecializationUpdate then LogGrandStrategies; NO Grand/fullturn receipt";
            std::string grand_frontier="Native Grand selection retained; mandatory LogGrandStrategies; NO Grand/fullturn receipt";
            LiveActionResult city_result{LiveActionOutcome::committed,city_frontier};
            LiveActionResult grand_result{LiveActionOutcome::committed,grand_frontier};
            auto &epoch=grand_epoch(result);
            auto journal=stage_bnw_grand_selection(pending,*epoch,rules,
                happiness.suffix_rules_,db,std::move(minimum_age));
            const bool city=journal.progress().frontier==BnwGrandSelectionFrontier::city_specialization_log;
            static_assert(std::is_nothrow_move_constructible_v<BnwGrandSelectionPending>);
            static_assert(std::is_nothrow_move_constructible_v<LiveActionResult>);
            static_assert(std::is_nothrow_move_assignable_v<std::string>);
            happiness.grand_selection_.emplace(std::move(journal));
            result.blocker=city?std::move(city_frontier):std::move(grand_frontier);
            return city?std::move(city_result):std::move(grand_result);
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_guess_empty(BnwGameActivationExecution &result,const content::RulesDatabase &db) {
        try {
            auto &epoch=grand_epoch(result);auto &pending=*result.pending_happiness;auto &owner=pending.activation_->owner;
            require(owner.grand_world_averages.has_value(),"Grand foreign Guess requires SAME privately accepted world averages");
            require(!owner.grand_guess_empty,"Grand empty Guess already accepted; no source loop replay");
            const auto rules=BnwGrandWorldAverageRules::load(db);
            auto journal=stage_bnw_grand_guess_empty(*epoch->world,*owner.player_do_turn_trade,*owner.player_do_turn_military,*owner.grand_military_average,*owner.grand_world_averages,rules,pending.happiness_->suffix_rules_,db,owner.identity,pending.ticket_.player);
            std::string frontier="Actual DoGuess empty eligible foreign22loop retained on SAME accepted averages; native guesses/priorities/active/age and RNG preserved; mandatory own Grand DoTurn FlavorManager/category priorities/founder Lua/JonRand/decisions/city specialization/logging then shadow Diplomacy; NO Grand/phase3/step15/fullturn receipt";
            if(owner.player_do_turn_prefix->misc_buttons_delivery_pending()||!pending.happiness_->ui_consumed())frontier+="; staged MiscButtons/happiness UI delivery mandatory";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            static_assert(std::is_nothrow_move_assignable_v<std::optional<BnwGrandGuessEmptyJournal>>);
            owner.grand_guess_empty=std::move(journal);result.blocker=std::move(frontier);return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult grand_military_average(BnwGameActivationExecution &result,const content::RulesDatabase &rules) {
        try {
            require(!result.draft&&result.pending_happiness&&result.completed_steps==result.pending_happiness->completed_prefix_&&result.completed_steps.size()==14&&result.completed_steps.back()==BnwGameActivationStep::diplomacy_requests_begin&&result.player_turn_calls==1&&!result.unit_turn_calls&&result.completed_player_phases==std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},"Grand military requires SAME actual incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;auto &activation=*pending.activation_;
            require(pending.diplomacy_begin_complete_&&pending.builder_complete_&&pending.happiness_->native_update_complete()&&activation.owner.diplomacy_requests&&activation.owner.player_do_turn_prefix&&activation.owner.player_do_turn_revolts&&activation.owner.player_do_turn_trade&&activation.owner.player_do_turn_military,"Grand military lost preceding actual native owners/journals");
            require(!activation.owner.grand_military_average,"Grand military average already accepted; no query/prefix replay");
            auto &epoch=pending.happiness_->suffix_->continuation.religion.query.epoch;
            require(epoch&&epoch->world&&epoch->host&&epoch->source_world==&pending.happiness_->prefix_->state()&&epoch->getter_world==epoch->world.get()&&epoch->all_selected_contexts_loaded&&epoch->all_listeners_registered&&epoch->all_external_writes_escrowed&&epoch->host->loading_complete(),"Grand military sole Lua/world epoch binding incomplete");
            validate_diplomacy_begin_caller(*epoch->world,pending.ticket_.player);
            auto journal=stage_bnw_grand_military_average(*epoch->world,*activation.owner.player_do_turn_trade,*activation.owner.player_do_turn_military,pending.happiness_->suffix_rules_,rules,activation.owner.identity,pending.ticket_.player);
            std::string frontier="Actual cached world military average retained privately; mandatory GrandStrategy DoGuess culture/tourism then tech averages next, before priorities/RNG/strategy decisions and shadow Diplomacy; NO GrandStrategy/phase3/step15/fullturn receipt";
            if(activation.owner.player_do_turn_prefix->misc_buttons_delivery_pending()||!pending.happiness_->ui_consumed())frontier+="; staged MiscButtons/happiness UI delivery mandatory";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            static_assert(std::is_nothrow_move_assignable_v<std::optional<BnwGrandMilitaryAverageJournal>>);
            activation.owner.grand_military_average=std::move(journal);result.blocker=std::move(frontier);return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult player_military(BnwGameActivationExecution &result,const content::RulesDatabase &rules) {
        try {
            require(!result.draft&&result.pending_happiness&&result.completed_steps==result.pending_happiness->completed_prefix_&&result.completed_steps.size()==14&&result.completed_steps.back()==BnwGameActivationStep::diplomacy_requests_begin&&result.player_turn_calls==1&&!result.unit_turn_calls&&result.completed_player_phases==std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},"Military Reset requires SAME actual incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;auto &activation=*pending.activation_;
            require(pending.diplomacy_begin_complete_&&pending.builder_complete_&&pending.happiness_->native_update_complete()&&activation.owner.diplomacy_requests&&activation.owner.player_do_turn_prefix&&activation.owner.player_do_turn_revolts&&activation.owner.player_do_turn_trade,"Military Reset lost actual preceding native owners/journals");
            require(!activation.owner.player_do_turn_military,"Military Reset already accepted; no counter overwrite replay");
            auto &epoch=pending.happiness_->suffix_->continuation.religion.query.epoch;
            require(epoch&&epoch->world&&epoch->host&&epoch->source_world==&pending.happiness_->prefix_->state()&&epoch->getter_world==epoch->world.get()&&epoch->all_selected_contexts_loaded&&epoch->all_listeners_registered&&epoch->all_external_writes_escrowed&&epoch->host->loading_complete(),"Military Reset sole Lua/world epoch binding incomplete");
            auto trial=*epoch->world;validate_diplomacy_begin_caller(trial,pending.ticket_.player);
            auto journal=stage_bnw_player_do_turn_military(trial,*activation.owner.player_do_turn_trade,pending.happiness_->suffix_rules_,rules,activation.owner.identity,pending.ticket_.player);
            std::string frontier="Actual MilitaryAI::ResetCounters land0/naval0/army-1 retained privately; mandatory GrandStrategyAI::DoTurn249/DoGuess850 world military/culture/tourism/tech averages next, including GetMilitaryMight lazy recompute; NO human shortcut; shadow Diplomacy mandatory after real GrandStrategy";
            if(activation.owner.player_do_turn_prefix->misc_buttons_delivery_pending()||!pending.happiness_->ui_consumed())frontier+="; staged MiscButtons/happiness UI delivery mandatory, no phase3/step15/fullpublication";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            static_assert(std::is_nothrow_move_assignable_v<LiveSessionState>&&std::is_nothrow_move_assignable_v<std::optional<BnwPlayerDoTurnMilitaryJournal>>);
            *epoch->world=std::move(trial);activation.owner.player_do_turn_military=std::move(journal);result.blocker=std::move(frontier);return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult player_trade(BnwGameActivationExecution &result,const content::RulesDatabase &rules) {
        try {
            require(!result.draft&&result.pending_happiness&&result.completed_steps==result.pending_happiness->completed_prefix_&&result.completed_steps.size()==14&&result.completed_steps.back()==BnwGameActivationStep::diplomacy_requests_begin&&result.player_turn_calls==1&&!result.unit_turn_calls&&result.completed_player_phases==std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},"Trade requires SAME actual incomplete native DoTurn prefix");
            auto &pending=*result.pending_happiness;auto &activation=*pending.activation_;
            require(pending.diplomacy_begin_complete_&&pending.builder_complete_&&pending.happiness_->native_update_complete()&&activation.owner.diplomacy_requests&&activation.owner.player_do_turn_prefix&&activation.owner.player_do_turn_revolts,"Trade lost actual preceding native owners/journals");
            require(!activation.owner.player_do_turn_trade,"Trade source journal already accepted; no history clear/aging replay");
            auto &epoch=pending.happiness_->suffix_->continuation.religion.query.epoch;
            require(epoch&&epoch->world&&epoch->host&&epoch->source_world==&pending.happiness_->prefix_->state()&&epoch->getter_world==epoch->world.get()&&epoch->all_selected_contexts_loaded&&epoch->all_listeners_registered&&epoch->all_external_writes_escrowed&&epoch->host->loading_complete(),"Trade sole Lua/world epoch binding incomplete");
            auto trial=*epoch->world;validate_diplomacy_begin_caller(trial,pending.ticket_.player);
            auto journal=stage_bnw_player_do_turn_trade(trial,*activation.owner.player_do_turn_revolts,pending.happiness_->suffix_rules_,rules,activation.owner.identity,pending.ticket_.player);
            std::string frontier="Actual Trade::DoTurn four-stage source journal retained privately; mandatory MilitaryAI::ResetCounters4078 then Strategy/Diplomacy owners next";
            if(activation.owner.player_do_turn_prefix->misc_buttons_delivery_pending()||!pending.happiness_->ui_consumed())frontier+="; staged MiscButtons/happiness UI delivery mandatory, no timers/revolts phase receipt";
            frontier+="; remaining player/unit/cache/UI/full publication incomplete";LiveActionResult committed{LiveActionOutcome::committed,frontier};
            static_assert(std::is_nothrow_move_assignable_v<LiveSessionState>&&std::is_nothrow_move_assignable_v<std::optional<BnwPlayerDoTurnTradeJournal>>);
            *epoch->world=std::move(trial);activation.owner.player_do_turn_trade=std::move(journal);result.blocker=std::move(frontier);return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult player_revolts(BnwGameActivationExecution &result,const content::RulesDatabase &rules) {
        try {
            require(!result.draft&&result.pending_happiness&&result.completed_steps==result.pending_happiness->completed_prefix_&&result.completed_steps.size()==14&&result.completed_steps.back()==BnwGameActivationStep::diplomacy_requests_begin&&result.player_turn_calls==1&&!result.unit_turn_calls&&result.completed_player_phases==std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre},"Player revolt suffix requires SAME actual incomplete native DoTurn prefix; no reset/replay");
            auto &pending=*result.pending_happiness;auto &activation=*pending.activation_;
            require(pending.diplomacy_begin_complete_&&pending.builder_complete_&&pending.happiness_->native_update_complete()&&activation.owner.diplomacy_requests&&activation.owner.player_do_turn_prefix,"Player revolt suffix lost actual preceding native owners");
            require(!activation.owner.player_do_turn_revolts,"Player revolt suffix already accepted; no countdown/RNG/Maya replay");
            auto &epoch=pending.happiness_->suffix_->continuation.religion.query.epoch;
            require(epoch&&epoch->world&&epoch->host&&epoch->source_world==&pending.happiness_->prefix_->state()&&epoch->getter_world==epoch->world.get()&&epoch->all_selected_contexts_loaded&&epoch->all_listeners_registered&&epoch->all_external_writes_escrowed&&epoch->host->loading_complete(),"Player revolt suffix sole Lua/world epoch binding incomplete");
            auto trial=*epoch->world;validate_diplomacy_begin_caller(trial,pending.ticket_.player);
            auto journal=stage_bnw_player_do_turn_revolts(trial,*activation.owner.player_do_turn_prefix,pending.happiness_->suffix_rules_,rules,activation.owner.identity,pending.ticket_.player);
            std::string frontier=journal.trade_required_next()?"Actual Uprisings/CityRevolts/Maya source suffix retained privately; mandatory Trade::DoTurn next for actual alive nonbarbarian major":"Actual Uprisings/CityRevolts/Maya source suffix retained privately; native trade branch excluded, mandatory human doArmySize/post-diplomacy pipeline next";
            if(activation.owner.player_do_turn_prefix->misc_buttons_delivery_pending()||!pending.happiness_->ui_consumed())frontier+="; staged MiscButtons/happiness UI delivery remains mandatory, no timers/revolts phase receipt";
            frontier+="; remaining player/unit/cache/UI and full publication incomplete";LiveActionResult committed{LiveActionOutcome::committed,frontier};
            static_assert(std::is_nothrow_move_assignable_v<LiveSessionState>&&std::is_nothrow_move_assignable_v<std::optional<BnwPlayerDoTurnRevoltsJournal>>);
            *epoch->world=std::move(trial);activation.owner.player_do_turn_revolts=std::move(journal);result.blocker=std::move(frontier);return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult player_prefix(BnwGameActivationExecution &result,const content::RulesDatabase &rules) {
        try {
            validate(result);auto &pending=*result.pending_happiness;auto &activation=*pending.activation_;
            require(pending.diplomacy_begin_complete_&&pending.builder_complete_&&pending.happiness_->native_update_complete()&&pending.completed_prefix_.size()==14&&pending.completed_prefix_.back()==BnwGameActivationStep::diplomacy_requests_begin,"Player DoTurn requires actual retained native activation through BeginTurn");
            require(!activation.owner.player_do_turn_prefix,"Player DoTurn prefix already entered; no timer/cramped replay");
            require(activation.owner.diplomacy_requests.has_value(),"Player DoTurn requires SAME retained diplomacy registry");
            auto &epoch=pending.happiness_->suffix_->continuation.religion.query.epoch;
            require(epoch&&epoch->world&&epoch->host&&epoch->source_world==&pending.happiness_->prefix_->state()&&epoch->getter_world==epoch->world.get()&&epoch->all_selected_contexts_loaded&&epoch->all_listeners_registered&&epoch->all_external_writes_escrowed&&epoch->host->loading_complete(),"Player DoTurn sole Lua/world epoch binding incomplete");
            require(epoch->world->setup.game_turn==pending.ticket_.game_turn,"Player DoTurn retained game-turn ticket changed");
            auto trial=*epoch->world;const auto player=pending.ticket_.player;
            validate_diplomacy_begin_caller(trial,player);
            // Prepare the journal and result before accepting any source store.
            // Stage helper mutates ONLY this trial; late cramped/identity/dirty
            // failure keeps accepted happiness, Builder and BeginTurn untouched.
            std::vector<BnwTurnPhase> phases{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre};
            std::string frontier="Actual human CvPlayer::doTurn prefix through DoUpdateCramped retained privately; mandatory DoUpdateUprisings next, followed by CityRevolts/Maya/trade/diplomacy and remaining player/unit/cache/UI effects";
            auto journal=stage_bnw_player_do_turn_prefix(trial,rules,activation.owner.identity,player,pending.ticket_.active_player);
            if(journal.misc_buttons_delivery_pending())frontier+="; actual MiscButtons dirty receipt pending client delivery (stock fanout not certified)";
            LiveActionResult committed{LiveActionOutcome::committed,frontier};
            static_assert(std::is_nothrow_move_assignable_v<LiveSessionState>&&std::is_nothrow_move_assignable_v<std::optional<BnwPlayerDoTurnPrefixJournal>>&&std::is_nothrow_move_assignable_v<std::vector<BnwTurnPhase>>);
            *epoch->world=std::move(trial);activation.owner.player_do_turn_prefix=std::move(journal);
            result.completed_player_phases=std::move(phases);result.player_turn_calls=1;result.blocker=std::move(frontier);
            return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult diplomacy(BnwGameActivationExecution &result,const content::RulesDatabase &rules) {
        try {
            validate(result);auto &pending=*result.pending_happiness;
            require(!pending.diplomacy_begin_complete_,"Activation diplomacy BeginTurn already completed; no repeat/reset");
            require(pending.builder_complete_&&pending.happiness_->native_update_complete()&&pending.completed_prefix_.size()==13&&pending.completed_prefix_.back()==BnwGameActivationStep::builder_tasking,"Activation BeginTurn requires actual preceding happiness/Builder source receipts");
            auto &epoch=pending.happiness_->suffix_->continuation.religion.query.epoch;
            require(epoch&&epoch->world&&epoch->host&&epoch->source_world==&pending.happiness_->prefix_->state()&&epoch->getter_world==epoch->world.get()&&epoch->all_selected_contexts_loaded&&epoch->all_listeners_registered&&epoch->all_external_writes_escrowed&&epoch->host->loading_complete(),"Activation BeginTurn sole Lua/world epoch binding incomplete");
            auto &activation=*pending.activation_;
            require(activation.owner.diplomacy_requests.has_value(),"Activation BeginTurn requires SAME retained original Read/turn diplomacy owner; no shadow/reset");
            auto trial=*epoch->world;const auto player=pending.ticket_.player;
            validate_diplomacy_begin_caller(trial,player);
            auto receipts=pending.completed_prefix_;receipts.push_back(BnwGameActivationStep::diplomacy_requests_begin);auto returned_receipts=receipts;
            auto applied=execute_bnw_diplomacy_begin_turn_scalar_draft(trial,*activation.owner.diplomacy_requests,rules,player);
            require(static_cast<bool>(applied.draft),applied.blocker);
            static_assert(std::is_nothrow_move_assignable_v<LiveSessionState>&&std::is_nothrow_move_assignable_v<std::optional<BnwDiplomacyTurnOwner>>);
            std::string next_frontier="Native happiness/Builder and actual retained diplomacy BeginTurn complete privately; mandatory player_do_turn, unit DoTurn, cache restore and deferred UI/publication remain";LiveActionResult committed{LiveActionOutcome::committed,next_frontier};
            *epoch->world=std::move(applied.draft->state);activation.owner.diplomacy_requests=std::move(applied.draft->owner);
            pending.completed_prefix_=std::move(receipts);result.completed_steps=std::move(returned_receipts);pending.diplomacy_begin_complete_=true;result.blocker=std::move(next_frontier);return committed;
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult builder(BnwGameActivationExecution &result,const content::RulesDatabase &rules) {
        try {
            validate(result);auto &pending=*result.pending_happiness;
            require(!pending.builder_complete_,"Activation Builder already completed; no repeated Update");
            require(pending.happiness_->native_update_complete(),"Activation Builder requires actual native happiness completion on SAME epoch");
            require(pending.completed_prefix_.size()==12&&pending.completed_prefix_.back()==BnwGameActivationStep::happiness,"Activation Builder source order lacks actual native happiness receipt");
            auto &epoch=pending.happiness_->suffix_->continuation.religion.query.epoch;
            require(epoch&&epoch->world&&epoch->host&&epoch->source_world==&pending.happiness_->prefix_->state()&&epoch->getter_world==epoch->world.get()&&epoch->all_selected_contexts_loaded&&epoch->all_listeners_registered&&epoch->all_external_writes_escrowed&&epoch->host->loading_complete(),"Activation Builder sole Lua/world epoch binding incomplete");
            auto &activation=*pending.activation_;
            require(activation.owner.builder_tasking&&activation.owner.city_connections,"Activation Builder requires SAME retained original-import Builder/CityConnections owners");
            auto trial=*epoch->world;auto staged_owner=*activation.owner.builder_tasking;
            const auto query_rules=BnwCityStrategyYieldRules::load(rules);
            const auto applied=update_bnw_activation_builder_tasking(trial,staged_owner,*activation.owner.city_connections,query_rules,pending.ticket_.player);
            require(applied.outcome!=LiveActionOutcome::rejected,applied.reason);
            // Reserve all journal stores before changing accepted world/owner.
            auto receipts=pending.completed_prefix_;receipts.push_back(BnwGameActivationStep::builder_tasking);auto returned_receipts=receipts;
            static_assert(std::is_nothrow_move_assignable_v<LiveSessionState>&&std::is_nothrow_move_assignable_v<std::optional<BnwActivationBuilderOwner>>);
            std::string next_frontier="Native happiness and actual Builder/city caches complete privately; mandatory diplomacy_requests_begin, player/unit DoTurn, cache restore and deferred UI/publication remain";
            // All allocation/failure points precede the no-throw grouped stores.
            // Keep the allocation to which actual Lua getters are bound.
            *epoch->world=std::move(trial);activation.owner.builder_tasking=std::move(staged_owner);
            pending.completed_prefix_=std::move(receipts);result.completed_steps=std::move(returned_receipts);pending.builder_complete_=true;
            result.blocker=std::move(next_frontier);
            return diplomacy(result,rules);
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
    static LiveActionResult resume(BnwGameActivationExecution &result,
        const SavedContentProfile &profile,BnwGameplayLuaEpochOwner &lua,
        const content::RulesDatabase &rules) {
        try {
            validate(result);auto &pending=*result.pending_happiness;
            require(!pending.happiness_->native_update_complete(),"Activation happiness already dispatched/completed; use remaining native stage without replay");
            require(pending.completed_prefix_.size()==11&&pending.completed_prefix_.back()==BnwGameActivationStep::city_connection_gold,"Activation happiness lacks exact source native prefix");
            // Prepare both journals BEFORE invoking the non-repeatable Lua/native
            // continuation. No allocation can strand accepted happiness at11.
            auto receipts=pending.completed_prefix_;receipts.push_back(BnwGameActivationStep::happiness);auto returned_receipts=receipts;
            const auto applied=continue_bnw_activation_happiness(*pending.happiness_,profile,lua,rules);
            if(applied.outcome==LiveActionOutcome::rejected){result.blocker=pending.happiness_->frontier();return applied;}
            // Boolean contract guard has no allocation on accepted success.
            if(!pending.happiness_->native_update_complete())throw std::runtime_error("Activation happiness missing actual native completion");
            // The actual continuation returns success only with its native stores
            // accepted. Make its receipt recoverable before any frontier copies.
            static_assert(std::is_nothrow_move_assignable_v<std::vector<BnwGameActivationStep>>);
            pending.completed_prefix_=std::move(receipts);result.completed_steps=std::move(returned_receipts);
            return builder(result,rules);
        }catch(const std::exception &e){result.blocker=e.what();return {LiveActionOutcome::rejected,result.blocker};}
    }
};
static BnwGameActivationExecution execute_bnw_game_activation_source_loop(
    const LiveSessionState &source,const BnwGameActivationOwner &owner,
    const content::RulesDatabase &rules,const SessionCheckpointIdentity &identity,
    std::span<const BnwTurnPhase> completed_game_prefix,
    std::span<const BnwTurnHandler> player_handlers,
    std::span<const BnwGameActivationService> services,bool until_happiness) {
    BnwGameActivationExecution result;
    try {
        require(identity.payload_sha256.size()==64 && identity.rules_sha256.size()==64 &&
                owner.identity.payload_sha256==identity.payload_sha256 &&
                owner.identity.rules_sha256==identity.rules_sha256,
                "Activation epoch owner belongs to another source or rules");
        require(identify_bytes(rules.gameplay_fingerprint_data()).sha256==identity.rules_sha256,
                "Activation selected rules fingerprint differs");
        require(source.world.pregame && source.world.all_players &&
                source.world.all_players->players.size()==64,
                "Activation source world is incomplete");
        require(owner.pending.empty(),"Activation previous publication is still pending");
        require(!owner.pathfinder_cache || owner.pathfinder_cache->scopes.empty(),
                "Activation preceding native pathfinder cache scope is still open");
        std::vector<BnwTurnHandler> private_handlers(player_handlers.begin(),player_handlers.end());
        // after_commit is retained below as an outer-transaction obligation.
        for(auto &handler:private_handlers) handler.after_commit={};
        std::vector<BnwGameActivationService> bound(services.begin(),services.end());
        if(until_happiness) for(const auto &service:services) {
            using S=BnwGameActivationStep;
            require(service.step!=S::happiness && service.step!=S::active_player_deferred_fog &&
                    service.step!=S::pathfinder_cache_enter && service.step!=S::pathfinder_cache_restore &&
                    service.step!=S::city_connections && service.step!=S::city_connection_gold && service.step!=S::builder_tasking && service.step!=S::diplomacy_requests_begin,
                    "Native until-happiness operation rejects duplicate/caller happiness or preceding native owner service");
        }
        for(const auto &service:services)
            require(!owned_step(service.step) &&
                    !(owner.deferred_fog && service.step==BnwGameActivationStep::active_player_deferred_fog) &&
                    !(owner.city_connections && (service.step==BnwGameActivationStep::city_connections ||
                       service.step==BnwGameActivationStep::city_connection_gold)) &&
                    !(owner.builder_tasking && service.step==BnwGameActivationStep::builder_tasking) &&
                    !(owner.diplomacy_requests && service.step==BnwGameActivationStep::diplomacy_requests_begin) &&
                    !(owner.pathfinder_cache && (service.step==BnwGameActivationStep::pathfinder_cache_enter ||
                       service.step==BnwGameActivationStep::pathfinder_cache_restore)),
                    "Activation native operation has a second owner");
        const auto active=source.world.pregame->archive.integers.at("active_player");
        std::int64_t first=-1;
        for(const auto &[id,player]:source.setup.players)
            if(player.alive) {first=id;break;}
        for(std::uint32_t id=0;id<=static_cast<std::uint32_t>(BnwGameActivationStep::active_player_turn_start);++id) {
            const auto step=static_cast<BnwGameActivationStep>(id);
            if(step==BnwGameActivationStep::player_do_turn ||
               (first!=active && (step==BnwGameActivationStep::active_player_deferred_fog ||
                step>=BnwGameActivationStep::active_player_unit_cycler))) continue;
            const auto found=std::find_if(bound.begin(),bound.end(),[&](const auto &s){return s.step==step;});
            if(found!=bound.end()) continue;
            // A rejecting binding permits the private prefix to run until its
            // exact unsupported boundary. It never substitutes a successful phase.
            bound.push_back({step,[step](LiveSessionState &,std::int64_t) {
                return LiveActionResult{LiveActionOutcome::rejected,
                    "Missing native activation owner: "+std::string(bnw_game_activation_step_name(step))};
            }});
        }
        const auto checked=preflight_bnw_game_activation(source,rules,
            completed_game_prefix,private_handlers,bound);
        require(checked.structurally_composable,checked.blocker);
        const auto &ticket=checked.ticket;
        const auto epoch=std::pair{ticket.game_turn,ticket.player};
        require(!owner.completed.contains(epoch),"Activation player doTurn already completed for this epoch");
        auto draft=std::make_unique<BnwGameActivationDraft>(BnwGameActivationDraft{source,owner,{}});
        for(const auto step:ticket.activation_order) {
            if(until_happiness&&step==BnwGameActivationStep::happiness) {
                require(draft->owner.city_connections.has_value(),"Native activation happiness requires SAME retained CityConnections owner");
                auto staged=stage_bnw_activation_happiness(draft->state,*draft->owner.city_connections,rules,identity,ticket.player,result.completed_steps);
                require(static_cast<bool>(staged.owner),staged.blocker);
                result.blocker=staged.owner->frontier();
                result.pending_happiness=BnwGameActivationPendingAccess::make(std::move(draft),std::move(staged.owner),ticket,result.completed_steps);
                // No happiness receipt or full draft: actual Lua and the rest
                // of the activation source suffix are still required.
                return result;
            }
            if(owned_step(step) || (draft->owner.deferred_fog &&
               step==BnwGameActivationStep::active_player_deferred_fog) ||
               (draft->owner.city_connections && (step==BnwGameActivationStep::city_connections ||
                step==BnwGameActivationStep::city_connection_gold)) ||
               (draft->owner.builder_tasking && step==BnwGameActivationStep::builder_tasking) ||
               (draft->owner.diplomacy_requests && step==BnwGameActivationStep::diplomacy_requests_begin) ||
               (draft->owner.pathfinder_cache && (step==BnwGameActivationStep::pathfinder_cache_enter ||
                step==BnwGameActivationStep::pathfinder_cache_restore))) run_owned(step,*draft,ticket,rules);
            else if(step==BnwGameActivationStep::player_do_turn) {
                require(++result.player_turn_calls==1,"Activation repeated nested player doTurn");
                for(const auto phase:ticket.player_phase_order) {
                    const auto found=std::find_if(player_handlers.begin(),player_handlers.end(),
                        [&](const auto &h){return h.phase==phase;});
                    const auto applied=found->run(draft->state,ticket.player);
                    require(applied.outcome!=LiveActionOutcome::rejected,
                        "Activation player phase "+std::string(bnw_turn_phase_name(phase))+": "+applied.reason);
                    if(found->after_commit)
                        draft->player_publications.push_back({phase,found->after_commit});
                    result.completed_player_phases.push_back(phase);
                }
            } else {
                if(step==BnwGameActivationStep::player_do_turn_units)
                    require(++result.unit_turn_calls==1,"Activation repeated player doTurnUnits");
                const auto found=std::find_if(bound.begin(),bound.end(),[&](const auto &s){return s.step==step;});
                const auto applied=found->run(draft->state,ticket.player);
                require(applied.outcome!=LiveActionOutcome::rejected,applied.reason);
            }
            result.completed_steps.push_back(step);
        }
        require(result.player_turn_calls==1 && result.unit_turn_calls==1,
                "Activation player/unit turn obligations incomplete");
        require(!draft->owner.pathfinder_cache || draft->owner.pathfinder_cache->scopes.empty(),
                "Activation native pathfinder scope was not restored");
        draft->owner.completed.insert(epoch);
        result.draft=std::move(draft);
    } catch(const std::exception &error) {result.blocker=error.what();}
      catch(...) {result.blocker="Activation callback failed with an unknown exception";}
    return result;
}

BnwGameActivationExecution execute_bnw_game_activation_draft(const LiveSessionState &s,const BnwGameActivationOwner &o,const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::span<const BnwTurnPhase> prefix,std::span<const BnwTurnHandler> handlers,std::span<const BnwGameActivationService> services){return execute_bnw_game_activation_source_loop(s,o,db,id,prefix,handlers,services,false);}
BnwGameActivationExecution execute_bnw_game_activation_until_happiness(const LiveSessionState &s,const BnwGameActivationOwner &o,const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::span<const BnwTurnPhase> prefix,std::span<const BnwTurnHandler> handlers,std::span<const BnwGameActivationService> services){return execute_bnw_game_activation_source_loop(s,o,db,id,prefix,handlers,services,true);}
LiveActionResult continue_bnw_game_activation_happiness(BnwGameActivationExecution &r,const SavedContentProfile &profile,BnwGameplayLuaEpochOwner &lua,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::resume(r,profile,lua,db);}

LiveActionResult continue_bnw_game_activation_builder(BnwGameActivationExecution &r,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::builder(r,db);}

LiveActionResult continue_bnw_game_activation_diplomacy_begin(BnwGameActivationExecution &r,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::diplomacy(r,db);}

LiveActionResult continue_bnw_game_activation_player_prefix(BnwGameActivationExecution &r,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::player_prefix(r,db);}

LiveActionResult continue_bnw_game_activation_grand_world_averages(BnwGameActivationExecution &r,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_world_averages(r,db);}
LiveActionResult continue_bnw_game_activation_grand_guess_empty(BnwGameActivationExecution &r,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_guess_empty(r,db);}
LiveActionResult continue_bnw_game_activation_grand_own_culture(BnwGameActivationExecution &r,const SavedContentProfile &profile,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_own_culture(r,profile,db);}
LiveActionResult continue_bnw_game_activation_grand_culture_priority(BnwGameActivationExecution &r,const SavedContentProfile &profile,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_culture_priority(r,profile,db);}
LiveActionResult continue_bnw_game_activation_grand_first_roll(BnwGameActivationExecution &r,const SavedContentProfile &profile,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_first_roll(r,profile,db);}
LiveActionResult continue_bnw_game_activation_grand_united_nations_category(BnwGameActivationExecution &r,const SavedContentProfile &profile,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_united_nations_category(r,profile,db);}
LiveActionResult continue_bnw_game_activation_grand_second_roll(BnwGameActivationExecution &r,const SavedContentProfile &profile,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_second_roll(r,profile,db);}
LiveActionResult continue_bnw_game_activation_grand_spaceship_category(BnwGameActivationExecution &r,const SavedContentProfile &profile,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_spaceship_category(r,profile,db);}
LiveActionResult continue_bnw_game_activation_grand_third_roll(BnwGameActivationExecution &r,const SavedContentProfile &profile,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_third_roll(r,profile,db);}
LiveActionResult continue_bnw_game_activation_grand_conquest_category(BnwGameActivationExecution &r,const SavedContentProfile &profile,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_conquest_category(r,profile,db);}
LiveActionResult continue_bnw_game_activation_grand_fourth_roll(BnwGameActivationExecution &r,const SavedContentProfile &profile,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_fourth_roll(r,profile,db);}
LiveActionResult continue_bnw_game_activation_grand_pressure(BnwGameActivationExecution &r,const SavedContentProfile &profile,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_pressure(r,profile,db);}
LiveActionResult continue_bnw_game_activation_grand_selection(BnwGameActivationExecution &r,const SavedContentProfile &profile,const content::RulesDatabase &db,std::shared_ptr<const BnwGrandMinimumAgeOwner> minimum_age){return BnwGameActivationPendingAccess::grand_selection(r,profile,db,std::move(minimum_age));}
LiveActionResult continue_bnw_game_activation_grand_military_average(BnwGameActivationExecution &r,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::grand_military_average(r,db);}
LiveActionResult continue_bnw_game_activation_player_military(BnwGameActivationExecution &r,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::player_military(r,db);}
LiveActionResult continue_bnw_game_activation_player_trade(BnwGameActivationExecution &r,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::player_trade(r,db);}
LiveActionResult continue_bnw_game_activation_player_revolts(BnwGameActivationExecution &r,const content::RulesDatabase &db){return BnwGameActivationPendingAccess::player_revolts(r,db);}

std::vector<BnwTurnHandler> make_bnw_game_activation_source_player_handlers(
    const content::RulesDatabase &rules,const SessionCheckpointIdentity &identity,
    std::int64_t active_player) {
    auto early=make_bnw_human_player_early_turn_handlers(rules,identity,active_player);
    std::vector<BnwTurnHandler> result(early.begin(),early.end());
    for(const auto phase:bnw_player_turn_order()) {
        if(phase==BnwTurnPhase::player_achievements ||
           std::find_if(result.begin(),result.end(),[&](const auto &h){return h.phase==phase;})!=result.end()) continue;
        result.push_back({phase,[phase](LiveSessionState &,std::int64_t) {
            return LiveActionResult{LiveActionOutcome::rejected,
                "Source player phase owner unavailable: "+std::string(bnw_turn_phase_name(phase))};
        }});
    }
    return result;
}
} // namespace civ5::runtime
