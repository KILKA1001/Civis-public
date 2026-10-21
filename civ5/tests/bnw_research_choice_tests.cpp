#include "civ5/runtime/bnw_research_choice.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/ui/progression_screens.hpp"
#include "civ5/ui/live_session_frame.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

civ5::runtime::LiveSession make_session() {
    using namespace civ5;
    saves::SavedSession source;
    source.snapshot.game_state.layout=saves::GameStateLayout::expansion2_version1;
    runtime::LiveSessionState state{
        source.snapshot, runtime::ResearchContext{}, {}, runtime::PlayerResearchContext{},
        runtime::CityYieldContext{}, runtime::BnwUnhappinessContext{},
        runtime::MaintenanceContext{}, runtime::TreasuryContext{},
        runtime::ScienceContext{}, runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})}};
    constexpr std::int64_t player=5, team=7;
    state.setup.players[player].team=team;
    state.setup.players[player].alive=true;
    state.player_research.players[player]={};
    state.research_turn.players[player].founded_first_city=true;
    state.research_selection.players[player].civilization="CIV_TEST";
    state.research_selection.disabled_technologies["CIV_TEST"]={};
    state.technologies.emplace_back(content::TechnologyProfile::bnw,team,3);
    state.maintenance.team_eras[team]=0;
    return runtime::LiveSession::from_restored_bnw(std::move(source),std::move(state));
}
} // namespace

int main() {
    try {
        using namespace civ5;
        using namespace civ5::runtime;
        content::TechnologyRules technologies;
        technologies.collection.slots=3;
        for (std::int64_t id=0;id<3;++id) {
            auto &tech=technologies.entries[id];
            tech.base.type="TECH_TEST_"+std::to_string(id);
            tech.booleans={{"Disable",false},{"Repeat",false}};
            tech.and_prerequisites.default_value=-1;
            tech.or_prerequisites.default_value=-1;
            tech.and_prerequisites.extent=2;
            tech.or_prerequisites.extent=2;
        }
        technologies.entries[2].and_prerequisites.values={{0,0},{1,1}};
        content::RulesDatabase database;
        database.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES"
            "('NUM_AND_TECH_PREREQS',2),('NUM_OR_TECH_PREREQS',2);");
        const auto defines=database.cache_global_defines(content::GlobalDefineProfile::bnw);
        const BnwResearchChoiceRules rules{technologies,defines,7};
        auto session=make_session();
        const auto initial=session.checkpoint();
        const auto rng=session.state().random.gameplay.state().calls;
        const auto absent=push_bnw_research_choice(session,5,2,false,rules,{});
        check(absent.outcome==LiveActionOutcome::rejected &&
              session.state().player_research.players.at(5).queue.empty(),
              "Missing Lua and dirty services must block without mutation");

        int dirty_calls=0;
        std::vector<std::int64_t> dirty_teams;
        bool ready=false, veto=false, throw_lua=false;
        BnwResearchChoiceServices services;
        services.lua_ready=[&] { return ready; };
        services.lua_test_all_read_only=[&](std::string_view event,
                std::int64_t player, std::int64_t tech, const LiveSessionState &seen)
                -> std::optional<bool> {
            check((event=="PlayerCanEverResearch" || event=="PlayerCanResearch") &&
                      player==5,
                  "Wrong Lua research event");
            check(seen.research_selection.players.at(5).civilization=="CIV_TEST",
                  "Lua evaluator did not see owned selection state");
            if (throw_lua && tech==1) throw ResearchSelectionUnavailable("Lua failed");
            if (veto && tech==1) return false;
            return std::nullopt;
        };
        services.dirty_after_commit=[&](std::int64_t team) {
            ++dirty_calls;
            dirty_teams.push_back(team);
            check(session.state().player_research.players.at(5).queue.size()==3,
                  "Dirty event ran before transaction commit");
        };
        const auto no_host=push_bnw_research_choice(session,5,2,false,rules,services);
        check(no_host.outcome==LiveActionOutcome::rejected && dirty_calls==0,
              "Unavailable Lua host must block before mutation");
        ready=true;veto=true;
        ui::LiveSessionUiSources frame_sources;
        frame_sources.technologies=&technologies;
        frame_sources.database=&database;
        frame_sources.research_choice_rules=&rules;
        frame_sources.research_choice_services=&services;
        std::optional<ui::ProgressionTechPopupEvent> popup_event={
            {5,ui::ProgressionTechPopupMode::research,std::nullopt,11}};
        frame_sources.active_tech_popup=[&](const LiveSession &) {
            return popup_event;
        };
        popup_event->token=0;
        const auto unidentified=ui::capture_live_session_ui_frame(session,0,5,frame_sources);
        const auto *unidentified_popup=unidentified.find<ui::ProgressionTechPopupState>();
        check(unidentified_popup && unidentified_popup->eligible_technologies.empty() &&
              unidentified_popup->unavailable_reason==
                  "BNW popup event has no stable identity",
              "An event without an owner token cannot publish chooser candidates");
        popup_event->token=11;
        const auto observed=ui::capture_live_session_ui_frame(session,1,5,frame_sources);
        const auto *observed_popup=observed.find<ui::ProgressionTechPopupState>();
        check(observed_popup && observed_popup->unavailable_reason.empty() &&
              observed_popup->eligible_technologies==std::vector<std::int64_t>({0}) &&
              dirty_calls==0 &&
              session.state().player_research.players.at(5).queue.empty() &&
              session.state().random.gameplay.state().calls==rng,
              "Real chooser event must read eligible BNW candidates without mutating the session");
        auto popup_services=services;
        popup_services.dirty_after_commit=[](std::int64_t) {};
        int popup_acknowledgements=0;
        bool popup_acknowledgement_fails=false;
        bool popup_acknowledgement_noop=false;
        ui::BnwLiveProgressionBackend popup_backend(session,rules,popup_services,
            [&](const LiveSession &) { return popup_event; },
            [&](const ui::ProgressionTechPopupEvent &processed) {
                check(popup_event && processed.token==popup_event->token,
                      "Popup acknowledgement must name the active event");
                if (popup_acknowledgement_fails)
                    throw std::runtime_error("popup owner did not acknowledge");
                if (popup_acknowledgement_noop) return;
                ++popup_acknowledgements;
                popup_event.reset();
            });
        const auto popup_choice=std::get<ui::UiGameplayCommand>(
            ui::progression_choose_popup_technology(0));
        check(popup_backend.availability(popup_choice,observed).status==
                  ui::UiCommandStatus::ready,
              "Normal popup candidate is ready only with a matching live chooser event");
        popup_event->token=12;
        check(popup_backend.availability(popup_choice,observed).status==
                  ui::UiCommandStatus::unavailable,
              "Identical chooser fields with a different event token reject a stale frame");
        popup_event.reset();
        check(popup_backend.availability(popup_choice,observed).status==
                  ui::UiCommandStatus::unavailable &&
              session.state().player_research.players.at(5).queue.empty(),
              "Closed chooser event must reject an old candidate frame");
        ui::ProgressionFrameState progression;
        progression.player_id=5;
        progression.team_id=7;
        progression.technologies.push_back({2,"TECH_TEST_2","TXT_KEY_TEST",std::nullopt,
                                            0,0,0,0,false,false,0});
        const auto frame=ui::UiFrameSnapshot(1,5,0).with<ui::ProgressionFrameState>(
            std::make_shared<const ui::ProgressionFrameState>(progression));
        const auto command=std::get<ui::UiGameplayCommand>(
            ui::progression_choose_technology(2));
        ui::BnwLiveProgressionBackend ui_backend(session,rules,services);
        check(ui_backend.availability(command,frame).status==ui::UiCommandStatus::ready,
              "Live technology choice must be ready with owned state and Lua services");
        const auto ui_veto=ui_backend.execute(command,frame);
        check(ui_veto.status==ui::UiCommandStatus::unavailable &&
              session.state().player_research.players.at(5).queue.empty(),
              "Lua veto must not report UI success or mutate the live queue");
        const auto rejected=push_bnw_research_choice(session,5,2,false,rules,services);
        check(rejected.outcome==LiveActionOutcome::rejected && dirty_calls==0 &&
              session.state().player_research.players.at(5).queue.empty() &&
              session.state().research_turn.players.at(5).researching.empty() &&
              session.state().random.gameplay.state().calls==rng,
              "Lua veto after first prerequisite must roll back queue, flags and RNG");
        veto=false;throw_lua=true;
        check(ui_backend.execute(command,frame).status==ui::UiCommandStatus::unavailable &&
              session.state().player_research.players.at(5).queue.empty(),
              "Lua exception must remain an unavailable UI action");
        bool threw=false;
        try { (void)push_bnw_research_choice(session,5,2,false,rules,services); }
        catch (const ResearchSelectionUnavailable &) { threw=true; }
        check(threw && dirty_calls==0 &&
              session.state().player_research.players.at(5).queue.empty(),
              "Lua exception must leave the published session untouched");
        throw_lua=false;
        const auto registry=ui::UiContextRegistry::from_documents(
            content::read_xml_metadata(R"(<Context>
                <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
                <LuaContext FileName="Assets/UI/InGame/TechTree/TechTree" ID="TechTree"/>
                <LuaContext FileName="Assets/UI/InGame/TechPopup" ID="TechPopup"/>
            </Context>)"),content::read_xml_metadata("<Context/>"));
        ui::UiScreenController screens(registry);
        check(screens.dispatch(ui::progression_open_command(
                  ui::ProgressionScreen::technologies),frame).status==
                  ui::UiCommandStatus::navigation,
              "Typed technology tree command must open its BNW context");
        check(screens.dispatch(command,frame,&ui_backend).status==ui::UiCommandStatus::performed &&
              session.state().player_research.players.at(5).queue==
                  std::vector<std::int64_t>({0,1,2}) &&
              ui_backend.availability(command,frame).status==ui::UiCommandStatus::unavailable,
              "UI selection commits once and rejects its stale frame");
        session.restore(initial);
        dirty_calls=0;
        dirty_teams.clear();
        popup_event=ui::ProgressionTechPopupEvent{
            5,ui::ProgressionTechPopupMode::research,std::nullopt,11};
        ui::LiveUiHost popup_host(registry,
            ui::make_live_session_frame_provider(session,5,frame_sources),
            &popup_backend);
        auto before_popup=popup_host.begin_frame(1280,960);
        check(popup_host.dispatch(ui::progression_open_command(
                  ui::ProgressionScreen::technology_popup),before_popup).status==
                  ui::UiCommandStatus::navigation,
              "Active chooser event can open TechPopup");
        auto chooser=popup_host.begin_frame(1280,960);
        check(popup_host.dispatch(popup_choice,chooser).status==
                  ui::UiCommandStatus::performed &&
              session.state().player_research.players.at(5).queue==
                  std::vector<std::int64_t>({0}) &&
              popup_acknowledgements==1 && !popup_event &&
              !popup_host.screens().visible("TechPopup"),
              "Confirmed queue choice acknowledges the exact event and closes TechPopup");
        session.restore(initial);
        popup_event=ui::ProgressionTechPopupEvent{
            5,ui::ProgressionTechPopupMode::research,std::nullopt,12};
        popup_acknowledgement_fails=true;
        auto before_failure=popup_host.begin_frame(1280,960);
        check(popup_host.dispatch(ui::progression_open_command(
                  ui::ProgressionScreen::technology_popup),before_failure).status==
                  ui::UiCommandStatus::navigation,
              "Next real chooser event can reopen TechPopup");
        auto uncertain=popup_host.begin_frame(1280,960);
        const auto unacknowledged=popup_host.dispatch(popup_choice,uncertain);
        check(unacknowledged.status==ui::UiCommandStatus::unavailable &&
              unacknowledged.reason.find("queue committed")!=std::string::npos &&
              session.state().player_research.players.at(5).queue==
                  std::vector<std::int64_t>({0}) &&
              popup_host.screens().visible("TechPopup") &&
              popup_acknowledgements==1 &&
              popup_host.dispatch(popup_choice,uncertain).status==
                  ui::UiCommandStatus::unavailable &&
              session.state().player_research.players.at(5).queue==
                  std::vector<std::int64_t>({0}),
              "Failed post-commit acknowledgement remains explicit and cannot replay research");
        session.restore(initial);
        popup_acknowledgement_fails=false;
        popup_acknowledgement_noop=true;
        popup_event=ui::ProgressionTechPopupEvent{
            5,ui::ProgressionTechPopupMode::research,std::nullopt,13};
        const auto unprocessed_frame=ui::capture_live_session_ui_frame(
            session,3,5,frame_sources);
        const auto not_processed=popup_backend.execute(popup_choice,unprocessed_frame);
        check(not_processed.status==ui::UiCommandStatus::unavailable &&
              not_processed.reason.find("not observed")!=std::string::npos &&
              session.state().player_research.players.at(5).queue==
                  std::vector<std::int64_t>({0}) &&
              popup_backend.availability(popup_choice,unprocessed_frame).status==
                  ui::UiCommandStatus::unavailable &&
              popup_acknowledgements==1,
              "Callback return without event removal cannot claim popup completion or replay research");
        session.restore(initial);
        popup_event.reset();
        const auto committed=push_bnw_research_choice(session,5,2,false,rules,services);
        check(committed.outcome==LiveActionOutcome::committed &&
              session.state().player_research.players.at(5).queue==
                  std::vector<std::int64_t>({0,1,2}) &&
              session.state().research_turn.players.at(5).researching==
                  std::set<std::int64_t>({0,1,2}) &&
              dirty_calls==3 && dirty_teams==std::vector<std::int64_t>({7,7,7}) &&
              !session.readiness().can_continue(),
              "Committed choice must publish queue before ordered dirty events");
        session.restore(initial);
        check(session.state().player_research.players.at(5).queue.empty() &&
              session.state().research_selection.players.at(5).civilization=="CIV_TEST",
              "Checkpoint must restore owned research selection with the queue");
        services.dirty_after_commit=[](std::int64_t) {
            throw std::runtime_error("UI closed");
        };
        const auto notification_failure=push_bnw_research_choice(
            session,5,2,false,rules,services);
        check(notification_failure.outcome==LiveActionOutcome::committed &&
              notification_failure.reason.find("dirty notification failed")!=
                  std::string::npos &&
              session.state().player_research.players.at(5).queue.size()==3,
              "Post-commit notification error must not masquerade as rollback");

        auto policy_session=make_session();
        policy_session.mutable_state().setup.players[5].policies_resolved=true;
        ui::ProgressionFrameState policy_state;
        policy_state.player_id=5;
        policy_state.team_id=7;
        policy_state.policy_ownership_resolved=true;
        policy_state.policies.push_back({4,"POLICY_TEST","POLICY_BRANCH_TRADITION",
            "TXT_KEY_POLICY_TEST",std::nullopt,1,1,false});
        policy_state.branches.push_back({2,"POLICY_BRANCH_FREEDOM",
            "TXT_KEY_FREEDOM",std::nullopt,true});
        const auto policy_frame=ui::UiFrameSnapshot(10,5,0).with<ui::ProgressionFrameState>(
            std::make_shared<const ui::ProgressionFrameState>(policy_state));
        ui::BnwLiveProgressionBackend policy_backend(policy_session,rules,{});
        const auto policy_command=std::get<ui::UiGameplayCommand>(
            ui::progression_choose_policy(4));
        const auto policy_gate=policy_backend.availability(policy_command,policy_frame);
        const auto policy_before=policy_session.state().setup.players.at(5).policies;
        const auto policy_rng=policy_session.state().random.gameplay.state().calls;
        check(policy_gate.status==ui::UiCommandStatus::unavailable &&
              policy_gate.reason.find("Selected BNW policy rules are not bound")!=std::string::npos &&
              policy_backend.execute(policy_command,policy_frame).status==
                  ui::UiCommandStatus::unavailable &&
              policy_session.state().setup.players.at(5).policies==policy_before &&
              policy_session.state().random.gameplay.state().calls==policy_rng,
              "Current live policy is explicitly blocked before any incomplete GameCore mutation");
        const auto policy_registry=ui::UiContextRegistry::from_documents(
            content::read_xml_metadata(R"(<Context>
                <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
                <LuaContext FileName="Assets/UI/InGame/Popups/SocialPolicyPopup" ID="SocialPolicyPopup" Hidden="True"/>
            </Context>)"),content::read_xml_metadata("<Context/>"));
        ui::UiScreenController policy_screens(policy_registry);
        check(policy_screens.dispatch(ui::UiOpenScreen{"SocialPolicyPopup"},policy_frame).status==
                  ui::UiCommandStatus::navigation &&
              policy_screens.dispatch(policy_command,policy_frame,&policy_backend).status==
                  ui::UiCommandStatus::unavailable,
              "Typed live policy UI route retains the explicit GameCore blocker");
        check(policy_backend.availability(ui::UiGameplayCommand::make(
                  "TechTree",ui::ProgressionChoosePolicy{4}),policy_frame).status==
                  ui::UiCommandStatus::invalid &&
              policy_backend.availability(std::get<ui::UiGameplayCommand>(
                  ui::progression_choose_policy(99)),policy_frame).status==
                  ui::UiCommandStatus::invalid &&
              policy_backend.availability(policy_command,
                  ui::UiFrameSnapshot(11,5,1).with<ui::ProgressionFrameState>(
                      std::make_shared<const ui::ProgressionFrameState>(policy_state))).status==
                  ui::UiCommandStatus::unavailable,
              "Wrong source, absent policy and stale turn cannot reach policy mutation");
        policy_session.mutable_state().setup.players[5].policies.insert(4);
        check(policy_backend.availability(policy_command,policy_frame).reason==
                  "UI policy ownership is stale",
              "Changed live policy ownership invalidates an old UI frame");
        policy_state.policies.front().adopted=true;
        const auto owned_frame=ui::UiFrameSnapshot(12,5,0).with<ui::ProgressionFrameState>(
            std::make_shared<const ui::ProgressionFrameState>(policy_state));
        check(policy_backend.availability(policy_command,owned_frame).reason==
                  "Policy is already adopted",
              "An already owned policy remains unavailable");
        const auto ideology=std::get<ui::UiGameplayCommand>(
            ui::progression_choose_ideology(2));
        check(policy_backend.availability(ideology,owned_frame).reason.find(
                  "choice event is unavailable")!=std::string::npos,
              "An ideology rules row does not invent a GameCore choice event");
        const auto ideology_event_frame=owned_frame.with<ui::ProgressionIdeologyPopupEvent>(
            std::make_shared<const ui::ProgressionIdeologyPopupEvent>(
                ui::ProgressionIdeologyPopupEvent{5,17}));
        const auto ideology_gate=policy_backend.availability(ideology,ideology_event_frame);
        check(ideology_gate.status==ui::UiCommandStatus::unavailable &&
              ideology_gate.reason.find("branch effects and notifications")!=
                  std::string::npos &&
              policy_backend.execute(ideology,ideology_event_frame).status==
                  ui::UiCommandStatus::unavailable,
              "Ideology choice remains blocked by missing branch side effects");

        auto preflight_session=make_session();
        preflight_session.mutable_state().setup.players[5].policies_resolved=true;
        auto &observed_players=preflight_session.mutable_state().world.all_players.emplace();
        observed_players.layout=saves::GameStateLayout::expansion2_version1;
        auto &observed_player=observed_players.players.emplace_back();
        observed_player.saved_player_id=5;
        const auto add_core=[&](const char *name,std::int64_t value) {
            saves::PlayerCoreField field{};
            field.name=name;
            field.values={value};
            observed_player.core.fields.push_back(std::move(field));
        };
        add_core("m_iJONSCulture",120);
        add_core("m_iCostNextPolicy",100);
        add_core("m_iNumFreePolicies",0);
        add_core("m_iNumFreeTenets",0);
        saves::PlayerStrategyBlock policies_block{};
        policies_block.name="policies";
        saves::TeamHashedArray unlocks{};
        unlocks.name="branch_unlocked";
        unlocks.rows.push_back({content::saved_info_type_hash(
            "POLICY_BRANCH_TRADITION"),{1}});
        unlocks.rows.push_back({content::saved_info_type_hash(
            "POLICY_BRANCH_FREEDOM"),{1}});
        policies_block.hashes.push_back(std::move(unlocks));
        observed_player.strategy.blocks.push_back(std::move(policies_block));
        content::RulesDatabase policy_db;
        policy_db.apply_sql(
            "CREATE TABLE Policies(ID INTEGER PRIMARY KEY,Type TEXT,PolicyBranchType TEXT,Level INTEGER,TechPrereq TEXT);"
            "CREATE TABLE PolicyBranchTypes(ID INTEGER PRIMARY KEY,Type TEXT,PurchaseByLevel INTEGER);"
            "CREATE TABLE Policy_PrereqPolicies(PolicyType TEXT,PrereqPolicy TEXT);"
            "CREATE TABLE Policy_PrereqORPolicies(PolicyType TEXT,PrereqPolicy TEXT);"
            "CREATE TABLE Policy_Disables(PolicyType TEXT,PolicyDisable TEXT);"
            "INSERT INTO Policies VALUES(1,'POLICY_PREREQ','POLICY_BRANCH_TRADITION',0,NULL);"
            "INSERT INTO Policies VALUES(2,'POLICY_ALTERNATE','POLICY_BRANCH_TRADITION',0,NULL);"
            "INSERT INTO Policies VALUES(3,'POLICY_BLOCKER','POLICY_BRANCH_TRADITION',0,NULL);"
            "INSERT INTO Policies VALUES(4,'POLICY_TEST','POLICY_BRANCH_TRADITION',0,NULL);"
            "INSERT INTO PolicyBranchTypes VALUES(0,'POLICY_BRANCH_TRADITION',0);"
            "INSERT INTO PolicyBranchTypes VALUES(2,'POLICY_BRANCH_FREEDOM',1);");
        const auto preview_policy=[&] {
            return ui::preview_bnw_policy_choice(preflight_session,policy_db,
                technologies,defines,5,4);
        };
        const auto known=preview_policy();
        check(known.passes_known_rules && known.culture==120 &&
              known.next_cost==100 && known.policy_type=="POLICY_TEST",
              "Selected BNW policy rule and live payment preview must agree");
        ui::BnwLiveProgressionBackend bound_policy_backend(
            preflight_session,rules,{}, {}, {}, &policy_db);
        const auto effects_gate=bound_policy_backend.availability(
            policy_command,policy_frame);
        check(effects_gate.status==ui::UiCommandStatus::unavailable &&
              effects_gate.reason.find("transactional DoAdoptPolicy")!=std::string::npos &&
              bound_policy_backend.execute(policy_command,policy_frame).status==
                  ui::UiCommandStatus::unavailable &&
              !preflight_session.state().setup.players.at(5).policies.contains(4),
              "Passing known policy rules cannot bypass Lua and transactional effects");
        observed_player.core.fields[0].values[0]=50;
        check(preview_policy().blocker.find("Insufficient culture")!=std::string::npos,
              "Culture below selected next cost must block policy preview");
        observed_player.core.fields[2].values[0]=1;
        check(preview_policy().passes_known_rules,
              "A real free-policy credit can satisfy payment preview");
        observed_player.core.fields[0].values[0]=120;
        observed_player.core.fields[2].values[0]=0;
        observed_player.strategy.blocks[0].hashes[0].rows[0].values[0]=0;
        check(preview_policy().blocker=="Policy branch is locked",
              "Locked live branch must block selected policy");
        observed_player.strategy.blocks[0].hashes[0].rows[0].values[0]=1;
        policy_db.apply_sql(
            "INSERT INTO Policy_PrereqPolicies VALUES('POLICY_TEST','POLICY_PREREQ');");
        check(preview_policy().blocker=="BNW AND policy prerequisite is unmet",
              "Selected AND prerequisite must be owned");
        preflight_session.mutable_state().setup.players[5].policies.insert(1);
        check(preview_policy().passes_known_rules,
              "Owned AND prerequisite permits known-rule preview");
        policy_db.apply_sql(
            "INSERT INTO Policy_PrereqORPolicies VALUES('POLICY_TEST','POLICY_ALTERNATE');");
        check(preview_policy().blocker=="BNW OR policy prerequisite is unmet",
              "Selected OR prerequisite must be owned");
        preflight_session.mutable_state().setup.players[5].policies.insert(2);
        check(preview_policy().passes_known_rules,
              "Owned OR prerequisite permits known-rule preview");
        policy_db.apply_sql(
            "INSERT INTO Policy_Disables VALUES('POLICY_BLOCKER','POLICY_TEST');");
        preflight_session.mutable_state().setup.players[5].policies.insert(3);
        check(preview_policy().blocker=="An adopted policy disables this choice",
              "Selected disabled-by-owned relation must block preview");
        preflight_session.mutable_state().setup.players[5].policies.erase(3);
        policy_db.apply_sql(
            "UPDATE Policies SET TechPrereq='TECH_TEST_1' WHERE ID=4;");
        check(preview_policy().blocker.find("technology prerequisite")!=std::string::npos,
              "Unresearched selected policy technology must block preview");
        preflight_session.mutable_state().technologies[0].set_has_tech(1,true);
        check(preview_policy().passes_known_rules,
              "Live team technology must satisfy selected policy technology gate");
        policy_db.apply_sql(
            "INSERT INTO Policies VALUES(5,'POLICY_TENET_A','POLICY_BRANCH_FREEDOM',1,NULL);"
            "INSERT INTO Policies VALUES(6,'POLICY_TENET_B','POLICY_BRANCH_FREEDOM',1,NULL);"
            "UPDATE Policies SET PolicyBranchType='POLICY_BRANCH_FREEDOM',Level=2 WHERE ID=4;");
        check(preview_policy().blocker=="BNW tenet level requires more lower-level tenets",
              "Level-two tenet needs enough owned lower-level tenets");
        preflight_session.mutable_state().setup.players[5].policies.insert(5);
        preflight_session.mutable_state().setup.players[5].policies.insert(6);
        check(preview_policy().passes_known_rules,
              "Owned lower-level tenets satisfy level gate");
        observed_player.strategy.blocks[0].hashes[0].rows[1].values.clear();
        check(preview_policy().blocker=="Saved BNW branch unlock flag is invalid",
              "Malformed live branch state cannot be treated as unlocked");
        observed_player.strategy.blocks[0].hashes[0].rows[1].values={1};
        saves::TeamHashedArray saved_ownership{};
        saved_ownership.name="has_policy";
        for (const auto &[type, adopted] :
             std::vector<std::pair<const char *,std::int64_t>>{
                 {"POLICY_PREREQ",1},{"POLICY_ALTERNATE",1},
                 {"POLICY_BLOCKER",0},{"POLICY_TEST",0},
                 {"POLICY_TENET_A",1},{"POLICY_TENET_B",1}})
            saved_ownership.rows.push_back({content::saved_info_type_hash(type),{adopted}});
        observed_player.strategy.blocks[0].hashes.push_back(std::move(saved_ownership));
        preflight_session.mutable_state().setup.players[5].policies_resolved=false;
        ui::LiveSessionUiSources policy_sources;
        policy_sources.database=&policy_db;
        policy_sources.technologies=&technologies;
        const auto saved_frame=ui::capture_live_session_ui_frame(
            preflight_session,20,5,policy_sources);
        const auto *saved_policies=ui::progression_for_active_player(saved_frame);
        check(saved_policies && saved_policies->policy_ownership_resolved &&
              saved_policies->policies.size()==6 &&
              saved_policies->policies[0].adopted &&
              !saved_policies->policies[3].adopted &&
              preview_policy().passes_known_rules &&
              bound_policy_backend.availability(policy_command,saved_frame).reason==
                  "Live BNW policy rules or ownership are unresolved",
              "Saved policy flags can inform read-only UI but cannot authorize action");
        observed_player.strategy.blocks[0].hashes[1].rows.pop_back();
        const auto incomplete_frame=ui::capture_live_session_ui_frame(
            preflight_session,21,5,policy_sources);
        const auto *incomplete=ui::progression_for_active_player(incomplete_frame);
        check(incomplete && !incomplete->policy_ownership_resolved &&
              preview_policy().blocker==
                  "Saved BNW policy dimensions differ from selected rules",
              "Missing saved policy hash leaves frame and preflight unresolved");
        std::cout << "PASS BNW research choice\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW research choice: " << error.what() << '\n';
        return 1;
    }
}
