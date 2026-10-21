#pragma once
void lua_ui_events_tests(){
    using namespace civ5::runtime;
    LuaScriptHost absent;
    absent.execute("none","assert(Events==nil)","=no-ui-provider");
    rejects([&]{(void)absent.emit_ui_event("UnitSelectionCleared");});

    std::vector<std::pair<std::string,std::vector<ScriptValue>>> commands;
    ScriptUiEventProvider ui;
    ui.notifications={"UnitSelectionCleared","DisplayMovementIndicator"};
    ui.commands={"ClearHexHighlightStyle"};
    ui.command=[&](std::string_view name,std::span<const ScriptValue> values){
        commands.emplace_back(std::string(name),std::vector<ScriptValue>(values.begin(),values.end()));
    };
    LuaScriptHost host({}, {}, {}, {}, {}, {}, {}, ui);
    host.execute("A",R"lua(
        assert(type(Events)=='table' and Events.Unknown==nil)
        marker='A';hits=0
        local event=Events.UnitSelectionCleared
        assert(event.Count()==0)
        local listener=function(id,label)
            assert(marker=='A' and id==5000000001 and label=='a\0b')
            hits=hits+1
            Events.ClearHexHighlightStyle('MovementRangeBorder')
        end
        assert(event.Add(listener)==listener and event.Count()==1)
        assert(LuaEvents.UnitSelectionCleared.Count()==0 and GameEvents.UnitSelectionCleared.Count()==0)
    )lua","=ui-provider");
    host.execute("B",R"lua(
        assert(Events.UnitSelectionCleared.Count()==0 and marker==nil)
        Events.UnitSelectionCleared.Add(function(id,label)
            assert(id==5000000001 and label=='a\0b')
            marker='B'
        end)
    )lua","=ui-consumer");
    const std::array<ScriptValue,2> args{std::int64_t{5000000001LL},std::string("a\0b",3)};
    const auto first=host.emit_ui_event("UnitSelectionCleared",args);
    check(first.listeners==2&&first.errors.empty()&&commands.size()==1&&
              commands[0].first=="ClearHexHighlightStyle"&&commands[0].second.size()==1&&
              commands[0].second[0]==ScriptValue(std::string("MovementRangeBorder")),
          "UI event did not cross contexts and reach the real graphics command handler");
    host.execute("A","assert(hits==1 and marker=='A' and Events.UnitSelectionCleared.Count()==1)","=ui-state-A");
    host.execute("B","assert(marker=='B' and Events.UnitSelectionCleared.Count()==1)","=ui-state-B");
    check(host.emit_ui_event("DisplayMovementIndicator").listeners==0,
          "Unsubscribed UI notification reported a listener");
    rejects([&]{(void)host.emit_ui_event("Undeclared");});
    host.execute("A",R"lua(
        Events.UnitSelectionCleared.RemoveAll()
        Events.UnitSelectionCleared.Add(function() error('UI listener failed') end)
        Events.UnitSelectionCleared.Add(function() hits=hits+1 end)
    )lua","=ui-errors");
    const auto second=host.emit_ui_event("UnitSelectionCleared",args);
    check(second.listeners==3&&second.errors.size()==1&&
              second.errors[0].find("UI listener failed")!=std::string::npos,
          "UI listener error was hidden or prevented later listeners");
    host.execute("A","assert(hits==2)","=ui-after-error");
    host.complete_loading();check(host.loading_complete(),"UI event registration should preserve load completion");

    auto bad_ui=ui;bad_ui.command=[](std::string_view,std::span<const ScriptValue>){
        throw ScriptError("graphics command rejected");
    };
    LuaScriptHost bad({}, {}, {}, {}, {}, {}, {}, bad_ui);
    rejects([&]{bad.execute("bad","Events.ClearHexHighlightStyle('x')","=rejected-ui-command");});
    rejects([&]{bad.complete_loading();});
    LuaScriptHost structured({}, {}, {}, {}, {}, {}, {}, ui);
    rejects([&]{structured.execute("bad","Events.ClearHexHighlightStyle({x=1})","=structured-ui-command");});
    rejects([&]{structured.complete_loading();});
    auto duplicate=ui;duplicate.notifications.push_back("UnitSelectionCleared");
    rejects([&]{LuaScriptHost invalid({}, {}, {}, {}, {}, {}, {}, duplicate);});
    auto overlap=ui;overlap.commands.push_back("UnitSelectionCleared");
    rejects([&]{LuaScriptHost invalid({}, {}, {}, {}, {}, {}, {}, overlap);});
    auto unhandled=ui;unhandled.command={};
    rejects([&]{LuaScriptHost invalid({}, {}, {}, {}, {}, {}, {}, unhandled);});

    // Isolated event bridge regression: exact authored ActionInfoPanel function
    // definitions and Add statements. Dependency fixtures below are deliberately
    // not a selected world/engine publisher or an alpha acceptance witness.
    ScriptUiEventProvider turn_ui;
    turn_ui.notifications={"ActivePlayerTurnEnd","RemotePlayerTurnEnd","EndTurnBlockingChanged"};
    LuaScriptHost turn({}, {}, {}, {}, {}, {}, {}, turn_ui);
    turn.execute("ActionInfoPanel",R"lua(
        waiting,dirty,selected,looked,auto_queries=0,0,0,0,0
        function setEndTurnWaiting() waiting=waiting+1 end
        function OnEndTurnDirty() dirty=dirty+1 end
        local plot={fixture=true}
        local unit={}
        function unit:IsAutomated() return false end
        function unit:IsDelayedDeath() return false end
        function unit:IsReadyToMove() return true end
        function unit:GetPlot() return plot end
        local player={}
        function player:IsTurnActive() return true end
        function player:Units()
            local pending=true
            return function() if pending then pending=false;return unit end end
        end
        Game={GetActivePlayer=function() return 0 end}
        Players={[0]=player}
        OptionsManager={GetAutoUnitCycle=function() auto_queries=auto_queries+1;return true end}
        UI={GetHeadSelectedUnit=function() return nil end,
            LookAt=function(p,mode) assert(p==plot and mode==0);looked=looked+1 end,
            SelectUnit=function(u) assert(u==unit);selected=selected+1 end}
    )lua","=action-info-event-dependency-fixtures");
    turn.execute("ActionInfoPanel",R"lua(
function OnActivePlayerTurnEnd()
	setEndTurnWaiting();
end
Events.ActivePlayerTurnEnd.Add( OnActivePlayerTurnEnd );
function OnRemotePlayerTurnEnd()
	OnEndTurnDirty(); -- The "Waiting For Players" tooltip might need to be updated.
end
Events.RemotePlayerTurnEnd.Add( OnRemotePlayerTurnEnd );
function OnEndTurnBlockingChanged(ePrevEndTurnBlockingType, eNewEndTurnBlockingType)
	
	local pPlayer = Players[Game.GetActivePlayer()];
	if (pPlayer ~= nil) then
		if pPlayer:IsTurnActive() then
		
			-- If they have auto-unit-cycling off, then don't change the selection.
			if (not OptionsManager.GetAutoUnitCycle()) then
				return;
			end
			
			local pSelectedUnit = UI.GetHeadSelectedUnit();
			if (pSelectedUnit ~= nil) then
				if (not pSelectedUnit:IsAutomated() and not pSelectedUnit:IsDelayedDeath() and pSelectedUnit:IsReadyToMove()) then
					return;
				end
			end
			
			-- GetFirstReadyUnit can return a unit that has automation and we don't want to select and center on that, manually look for a unit		
			for pUnit in pPlayer:Units() do
				if (pUnit ~= nil) then
					if (not pUnit:IsAutomated() and not pUnit:IsDelayedDeath() and pUnit:IsReadyToMove()) then
						local pPlot = pUnit:GetPlot();
						UI.LookAt(pPlot, 0);
						UI.SelectUnit(pUnit);
						return;
					end
				end
			end
		end
	end
end
Events.EndTurnBlockingChanged.Add( OnEndTurnBlockingChanged );

        assert(Events.ActivePlayerTurnEnd.Count()==1 and Events.RemotePlayerTurnEnd.Count()==1)
        assert(Events.EndTurnBlockingChanged.Count()==1)
        assert(waiting==0 and dirty==0 and selected==0 and looked==0 and auto_queries==0)
        assert(Events.UnknownTurnEvent==nil)
        assert(GameEvents.ActivePlayerTurnEnd.Count()==0 and LuaEvents.ActivePlayerTurnEnd.Count()==0)
    )lua","@Assets/DLC/Expansion2/UI/InGame/WorldView/ActionInfoPanel.lua:event-regression-exact-slices");
    const auto local_turn=turn.emit_ui_event("ActivePlayerTurnEnd");
    const auto remote_turn=turn.emit_ui_event("RemotePlayerTurnEnd");
    check(local_turn.listeners==1&&local_turn.errors.empty()&&remote_turn.listeners==1&&remote_turn.errors.empty(),
          "Authored ActionInfoPanel turn-end subscriptions were not delivered separately");
    turn.execute("ActionInfoPanel","assert(waiting==1 and dirty==1 and selected==0)","=turn-end-exact-callback-effects");
    turn.execute("PayloadObserver",R"lua(
        blocking_payloads=0
        Events.EndTurnBlockingChanged.Add(function(...)
            assert(select('#',...)==2)
            local previous,new=...
            assert(previous==-1 and new==4)
            blocking_payloads=blocking_payloads+1
        end)
        Events.ActivePlayerTurnEnd.Add(function(...) assert(select('#',...)==0) end)
        Events.RemotePlayerTurnEnd.Add(function(...) assert(select('#',...)==0) end)
    )lua","=turn-event-signature-observer");
    const std::array<ScriptValue,2> blocking_args{std::int64_t{-1},std::int64_t{4}};
    const auto blocking_turn=turn.emit_ui_event("EndTurnBlockingChanged",blocking_args);
    check(blocking_turn.listeners==2&&blocking_turn.errors.empty(),
          "End-turn blocking event lost native previous/new integer operands or authored callback");
    turn.execute("ActionInfoPanel","assert(waiting==1 and dirty==1 and selected==1 and looked==1 and auto_queries==1)","=blocking-exact-callback-effects");
    turn.execute("PayloadObserver","assert(blocking_payloads==1)","=blocking-payload-effects");
    check(turn.emit_ui_event("ActivePlayerTurnEnd").listeners==2&&turn.emit_ui_event("RemotePlayerTurnEnd").listeners==2,
          "Turn-end zero-operand event signature listeners were not delivered");
    turn.execute("ActionInfoPanel",R"lua(
        assert(waiting==2 and dirty==2)
        Events.ActivePlayerTurnEnd.Remove(OnActivePlayerTurnEnd)
        Events.RemotePlayerTurnEnd.Remove(OnRemotePlayerTurnEnd)
        Events.EndTurnBlockingChanged.Remove(OnEndTurnBlockingChanged)
        assert(Events.ActivePlayerTurnEnd.Count()==0 and Events.RemotePlayerTurnEnd.Count()==0)
        assert(Events.EndTurnBlockingChanged.Count()==0)
    )lua","=remove-exact-authored-turn-listeners");
    check(turn.emit_ui_event("ActivePlayerTurnEnd").listeners==1&&turn.emit_ui_event("RemotePlayerTurnEnd").listeners==1&&
          turn.emit_ui_event("EndTurnBlockingChanged",blocking_args).listeners==1,
          "Exact callback Remove damaged another context's subscription list");
    turn.execute("ActionInfoPanel","assert(waiting==2 and dirty==2 and selected==1)","=removed-turn-listeners-inert");
    turn.execute("ActionInfoPanel",R"lua(
        Events.ActivePlayerTurnEnd.Add(function() error('authored event fixture failure') end)
        Events.ActivePlayerTurnEnd.Add(OnActivePlayerTurnEnd)
    )lua","=turn-event-failure-before-real-callback");
    const auto turn_failed=turn.emit_ui_event("ActivePlayerTurnEnd");
    check(turn_failed.listeners==3&&turn_failed.errors.size()==1&&
          turn_failed.errors[0].find("authored event fixture failure")!=std::string::npos,
          "Turn event bridge suppressed a failed listener or aborted later exact callback");
    turn.execute("ActionInfoPanel","assert(waiting==3)","=authored-turn-callback-after-error");
    rejects([&]{(void)turn.emit_ui_event("UndeclaredTurnEvent");});
}
