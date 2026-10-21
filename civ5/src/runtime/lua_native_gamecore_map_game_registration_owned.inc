// Actual Expansion2 SDK literal method order; new x64 executable dispatch.
struct NativeGameCoreMethodRecord { const char*name; lua_CFunction callback; };
template<unsigned Kind,std::size_t Ordinal>static int native_gamecore_method_endpoint_owned(lua_State*L){return native_gamecore_map_game_invoke_owned(L,Kind,Ordinal);}
static constexpr NativeGameCoreMethodRecord native_gamecore_map_methods[]={
    {"Areas",&Impl::native_gamecore_method_endpoint_owned<0,0>},
    {"FindBiggestArea",&Impl::native_gamecore_method_endpoint_owned<0,1>},
    {"FindWater",&Impl::native_gamecore_method_endpoint_owned<0,2>},
    {"GetClimate",&Impl::native_gamecore_method_endpoint_owned<0,3>},
    {"GetFractalFlags",&Impl::native_gamecore_method_endpoint_owned<0,4>},
    {"GetGridSize",&Impl::native_gamecore_method_endpoint_owned<0,5>},
    {"GetNumPlots",&Impl::native_gamecore_method_endpoint_owned<0,6>},
    {"GetNumResources",&Impl::native_gamecore_method_endpoint_owned<0,7>},
    {"GetNumResourcesOnLand",&Impl::native_gamecore_method_endpoint_owned<0,8>},
    {"GetPlot",&Impl::native_gamecore_method_endpoint_owned<0,9>},
    {"GetPlotByIndex",&Impl::native_gamecore_method_endpoint_owned<0,10>},
    {"GetPlotXY",&Impl::native_gamecore_method_endpoint_owned<0,11>},
    {"GetArea",&Impl::native_gamecore_method_endpoint_owned<0,12>},
    {"GetIndexAfterLastArea",&Impl::native_gamecore_method_endpoint_owned<0,13>},
    {"GetLandPlots",&Impl::native_gamecore_method_endpoint_owned<0,14>},
    {"GetNumAreas",&Impl::native_gamecore_method_endpoint_owned<0,15>},
    {"GetNumLandAreas",&Impl::native_gamecore_method_endpoint_owned<0,16>},
    {"GetRandomResourceQuantity",&Impl::native_gamecore_method_endpoint_owned<0,17>},
    {"GetSeaLevel",&Impl::native_gamecore_method_endpoint_owned<0,18>},
    {"GetWorldSize",&Impl::native_gamecore_method_endpoint_owned<0,19>},
    {"IsPlot",&Impl::native_gamecore_method_endpoint_owned<0,20>},
    {"IsWrapX",&Impl::native_gamecore_method_endpoint_owned<0,21>},
    {"IsWrapY",&Impl::native_gamecore_method_endpoint_owned<0,22>},
    {"MaxPlotDistance",&Impl::native_gamecore_method_endpoint_owned<0,23>},
    {"Rand",&Impl::native_gamecore_method_endpoint_owned<0,24>},
    {"CalculateAreas",&Impl::native_gamecore_method_endpoint_owned<0,25>},
    {"RecalculateAreas",&Impl::native_gamecore_method_endpoint_owned<0,26>},
    {"PlotDistance",&Impl::native_gamecore_method_endpoint_owned<0,27>},
    {"PlotXYWithRangeCheck",&Impl::native_gamecore_method_endpoint_owned<0,28>},
    {"PlotDirection",&Impl::native_gamecore_method_endpoint_owned<0,29>},
    {"DefaultContinentStamper",&Impl::native_gamecore_method_endpoint_owned<0,30>},
    {"DoPlaceNaturalWonders",&Impl::native_gamecore_method_endpoint_owned<0,31>},
    {"GetCustomOption",&Impl::native_gamecore_method_endpoint_owned<0,32>},
    {"UpdateDeferredFog",&Impl::native_gamecore_method_endpoint_owned<0,33>},
    {"ChangeAIMapHint",&Impl::native_gamecore_method_endpoint_owned<0,34>},
    {"GetAIMapHint",&Impl::native_gamecore_method_endpoint_owned<0,35>},
};
static constexpr NativeGameCoreMethodRecord native_gamecore_game_methods[]={
    {"CanHandleAction",&Impl::native_gamecore_method_endpoint_owned<1,0>},
    {"HandleAction",&Impl::native_gamecore_method_endpoint_owned<1,1>},
    {"UpdateScore",&Impl::native_gamecore_method_endpoint_owned<1,2>},
    {"CycleCities",&Impl::native_gamecore_method_endpoint_owned<1,3>},
    {"CycleUnits",&Impl::native_gamecore_method_endpoint_owned<1,4>},
    {"CyclePlotUnits",&Impl::native_gamecore_method_endpoint_owned<1,5>},
    {"SelectionListMove",&Impl::native_gamecore_method_endpoint_owned<1,6>},
    {"SelectionListGameNetMessage",&Impl::native_gamecore_method_endpoint_owned<1,7>},
    {"SelectedCitiesGameNetMessage",&Impl::native_gamecore_method_endpoint_owned<1,8>},
    {"CityPushOrder",&Impl::native_gamecore_method_endpoint_owned<1,9>},
    {"CityPurchaseUnit",&Impl::native_gamecore_method_endpoint_owned<1,10>},
    {"CityPurchaseBuilding",&Impl::native_gamecore_method_endpoint_owned<1,11>},
    {"CityPurchaseProject",&Impl::native_gamecore_method_endpoint_owned<1,12>},
    {"GetProductionPerPopulation",&Impl::native_gamecore_method_endpoint_owned<1,13>},
    {"GetAdjustedPopulationPercent",&Impl::native_gamecore_method_endpoint_owned<1,14>},
    {"GetAdjustedLandPercent",&Impl::native_gamecore_method_endpoint_owned<1,15>},
    {"GetUnitedNationsCountdown",&Impl::native_gamecore_method_endpoint_owned<1,16>},
    {"SetUnitedNationsCountdown",&Impl::native_gamecore_method_endpoint_owned<1,17>},
    {"CountCivPlayersAlive",&Impl::native_gamecore_method_endpoint_owned<1,18>},
    {"CountCivPlayersEverAlive",&Impl::native_gamecore_method_endpoint_owned<1,19>},
    {"CountCivTeamsAlive",&Impl::native_gamecore_method_endpoint_owned<1,20>},
    {"CountCivTeamsEverAlive",&Impl::native_gamecore_method_endpoint_owned<1,21>},
    {"CountHumanPlayersAlive",&Impl::native_gamecore_method_endpoint_owned<1,22>},
    {"CountHumanPlayersEverAlive",&Impl::native_gamecore_method_endpoint_owned<1,23>},
    {"CountTotalCivPower",&Impl::native_gamecore_method_endpoint_owned<1,24>},
    {"CountTotalNukeUnits",&Impl::native_gamecore_method_endpoint_owned<1,25>},
    {"CountKnownTechNumTeams",&Impl::native_gamecore_method_endpoint_owned<1,26>},
    {"GoldenAgeLength",&Impl::native_gamecore_method_endpoint_owned<1,27>},
    {"VictoryDelay",&Impl::native_gamecore_method_endpoint_owned<1,28>},
    {"GetImprovementUpgradeTime",&Impl::native_gamecore_method_endpoint_owned<1,29>},
    {"CanTrainNukes",&Impl::native_gamecore_method_endpoint_owned<1,30>},
    {"GetCurrentEra",&Impl::native_gamecore_method_endpoint_owned<1,31>},
    {"GetDiploResponse",&Impl::native_gamecore_method_endpoint_owned<1,32>},
    {"GetActiveTeam",&Impl::native_gamecore_method_endpoint_owned<1,33>},
    {"GetActiveCivilizationType",&Impl::native_gamecore_method_endpoint_owned<1,34>},
    {"IsNetworkMultiPlayer",&Impl::native_gamecore_method_endpoint_owned<1,35>},
    {"IsGameMultiPlayer",&Impl::native_gamecore_method_endpoint_owned<1,36>},
    {"IsTeamGame",&Impl::native_gamecore_method_endpoint_owned<1,37>},
    {"ReviveActivePlayer",&Impl::native_gamecore_method_endpoint_owned<1,38>},
    {"GetNumHumanPlayers",&Impl::native_gamecore_method_endpoint_owned<1,39>},
    {"GetNumSequentialHumans",&Impl::native_gamecore_method_endpoint_owned<1,40>},
    {"GetGameTurn",&Impl::native_gamecore_method_endpoint_owned<1,41>},
    {"SetGameTurn",&Impl::native_gamecore_method_endpoint_owned<1,42>},
    {"GetTurnYear",&Impl::native_gamecore_method_endpoint_owned<1,43>},
    {"GetGameTurnYear",&Impl::native_gamecore_method_endpoint_owned<1,44>},
    {"GetTurnString",&Impl::native_gamecore_method_endpoint_owned<1,45>},
    {"GetElapsedGameTurns",&Impl::native_gamecore_method_endpoint_owned<1,46>},
    {"GetMaxTurns",&Impl::native_gamecore_method_endpoint_owned<1,47>},
    {"SetMaxTurns",&Impl::native_gamecore_method_endpoint_owned<1,48>},
    {"ChangeMaxTurns",&Impl::native_gamecore_method_endpoint_owned<1,49>},
    {"GetMaxCityElimination",&Impl::native_gamecore_method_endpoint_owned<1,50>},
    {"SetMaxCityElimination",&Impl::native_gamecore_method_endpoint_owned<1,51>},
    {"GetNumAdvancedStartPoints",&Impl::native_gamecore_method_endpoint_owned<1,52>},
    {"SetNumAdvancedStartPoints",&Impl::native_gamecore_method_endpoint_owned<1,53>},
    {"GetStartTurn",&Impl::native_gamecore_method_endpoint_owned<1,54>},
    {"GetWinningTurn",&Impl::native_gamecore_method_endpoint_owned<1,55>},
    {"GetStartYear",&Impl::native_gamecore_method_endpoint_owned<1,56>},
    {"SetStartYear",&Impl::native_gamecore_method_endpoint_owned<1,57>},
    {"GetEstimateEndTurn",&Impl::native_gamecore_method_endpoint_owned<1,58>},
    {"SetEstimateEndTurn",&Impl::native_gamecore_method_endpoint_owned<1,59>},
    {"GetTurnSlice",&Impl::native_gamecore_method_endpoint_owned<1,60>},
    {"GetMinutesPlayed",&Impl::native_gamecore_method_endpoint_owned<1,61>},
    {"GetTargetScore",&Impl::native_gamecore_method_endpoint_owned<1,62>},
    {"SetTargetScore",&Impl::native_gamecore_method_endpoint_owned<1,63>},
    {"IsStaticTutorialActive",&Impl::native_gamecore_method_endpoint_owned<1,64>},
    {"SetStaticTutorialActive",&Impl::native_gamecore_method_endpoint_owned<1,65>},
    {"IsEverRightClickMoved",&Impl::native_gamecore_method_endpoint_owned<1,66>},
    {"SetEverRightClickMoved",&Impl::native_gamecore_method_endpoint_owned<1,67>},
    {"IsTutorialLogging",&Impl::native_gamecore_method_endpoint_owned<1,68>},
    {"IsTutorialDebugging",&Impl::native_gamecore_method_endpoint_owned<1,69>},
    {"GetTutorialLevel",&Impl::native_gamecore_method_endpoint_owned<1,70>},
    {"HasAdvisorMessageBeenSeen",&Impl::native_gamecore_method_endpoint_owned<1,71>},
    {"SetAdvisorMessageHasBeenSeen",&Impl::native_gamecore_method_endpoint_owned<1,72>},
    {"SetAdvisorBadAttackInterrupt",&Impl::native_gamecore_method_endpoint_owned<1,73>},
    {"SetAdvisorCityAttackInterrupt",&Impl::native_gamecore_method_endpoint_owned<1,74>},
    {"GetAllowRClickMovementWhileScrolling",&Impl::native_gamecore_method_endpoint_owned<1,75>},
    {"GetNumGameTurnActive",&Impl::native_gamecore_method_endpoint_owned<1,76>},
    {"CountNumHumanGameTurnActive",&Impl::native_gamecore_method_endpoint_owned<1,77>},
    {"GetNumCities",&Impl::native_gamecore_method_endpoint_owned<1,78>},
    {"GetNumCivCities",&Impl::native_gamecore_method_endpoint_owned<1,79>},
    {"GetTotalPopulation",&Impl::native_gamecore_method_endpoint_owned<1,80>},
    {"GetNoNukesCount",&Impl::native_gamecore_method_endpoint_owned<1,81>},
    {"IsNoNukes",&Impl::native_gamecore_method_endpoint_owned<1,82>},
    {"ChangeNoNukesCount",&Impl::native_gamecore_method_endpoint_owned<1,83>},
    {"GetNukesExploded",&Impl::native_gamecore_method_endpoint_owned<1,84>},
    {"ChangeNukesExploded",&Impl::native_gamecore_method_endpoint_owned<1,85>},
    {"GetMaxPopulation",&Impl::native_gamecore_method_endpoint_owned<1,86>},
    {"GetInitPopulation",&Impl::native_gamecore_method_endpoint_owned<1,87>},
    {"GetInitLand",&Impl::native_gamecore_method_endpoint_owned<1,88>},
    {"GetInitTech",&Impl::native_gamecore_method_endpoint_owned<1,89>},
    {"GetInitWonders",&Impl::native_gamecore_method_endpoint_owned<1,90>},
    {"GetNumWorldWonders",&Impl::native_gamecore_method_endpoint_owned<1,91>},
    {"GetAIAutoPlay",&Impl::native_gamecore_method_endpoint_owned<1,92>},
    {"SetAIAutoPlay",&Impl::native_gamecore_method_endpoint_owned<1,93>},
    {"IsScoreDirty",&Impl::native_gamecore_method_endpoint_owned<1,94>},
    {"SetScoreDirty",&Impl::native_gamecore_method_endpoint_owned<1,95>},
    {"IsCircumnavigated",&Impl::native_gamecore_method_endpoint_owned<1,96>},
    {"MakeCircumnavigated",&Impl::native_gamecore_method_endpoint_owned<1,97>},
    {"DoFromUIDiploEvent",&Impl::native_gamecore_method_endpoint_owned<1,98>},
    {"IsDebugMode",&Impl::native_gamecore_method_endpoint_owned<1,99>},
    {"SetDebugMode",&Impl::native_gamecore_method_endpoint_owned<1,100>},
    {"ToggleDebugMode",&Impl::native_gamecore_method_endpoint_owned<1,101>},
    {"UpdateFOW",&Impl::native_gamecore_method_endpoint_owned<1,102>},
    {"GetPitbossTurnTime",&Impl::native_gamecore_method_endpoint_owned<1,103>},
    {"SetPitbossTurnTime",&Impl::native_gamecore_method_endpoint_owned<1,104>},
    {"IsHotSeat",&Impl::native_gamecore_method_endpoint_owned<1,105>},
    {"IsPbem",&Impl::native_gamecore_method_endpoint_owned<1,106>},
    {"IsPitboss",&Impl::native_gamecore_method_endpoint_owned<1,107>},
    {"IsSimultaneousTeamTurns",&Impl::native_gamecore_method_endpoint_owned<1,108>},
    {"IsFinalInitialized",&Impl::native_gamecore_method_endpoint_owned<1,109>},
    {"GetActivePlayer",&Impl::native_gamecore_method_endpoint_owned<1,110>},
    {"SetActivePlayer",&Impl::native_gamecore_method_endpoint_owned<1,111>},
    {"GetPausePlayer",&Impl::native_gamecore_method_endpoint_owned<1,112>},
    {"SetPausePlayer",&Impl::native_gamecore_method_endpoint_owned<1,113>},
    {"IsPaused",&Impl::native_gamecore_method_endpoint_owned<1,114>},
    {"GetBestLandUnit",&Impl::native_gamecore_method_endpoint_owned<1,115>},
    {"GetBestLandUnitCombat",&Impl::native_gamecore_method_endpoint_owned<1,116>},
    {"GetFaithCost",&Impl::native_gamecore_method_endpoint_owned<1,117>},
    {"GetWinner",&Impl::native_gamecore_method_endpoint_owned<1,118>},
    {"GetVictory",&Impl::native_gamecore_method_endpoint_owned<1,119>},
    {"SetWinner",&Impl::native_gamecore_method_endpoint_owned<1,120>},
    {"GetGameState",&Impl::native_gamecore_method_endpoint_owned<1,121>},
    {"SetGameState",&Impl::native_gamecore_method_endpoint_owned<1,122>},
    {"GetVotesNeededForDiploVictory",&Impl::native_gamecore_method_endpoint_owned<1,123>},
    {"IsUnitedNationsActive",&Impl::native_gamecore_method_endpoint_owned<1,124>},
    {"GetNumVictoryVotesTallied",&Impl::native_gamecore_method_endpoint_owned<1,125>},
    {"GetVoteCast",&Impl::native_gamecore_method_endpoint_owned<1,126>},
    {"GetPreviousVoteCast",&Impl::native_gamecore_method_endpoint_owned<1,127>},
    {"GetNumVotesForTeam",&Impl::native_gamecore_method_endpoint_owned<1,128>},
    {"SetNumVotesForTeam",&Impl::native_gamecore_method_endpoint_owned<1,129>},
    {"ChangeNumVotesForTeam",&Impl::native_gamecore_method_endpoint_owned<1,130>},
    {"GetHandicapType",&Impl::native_gamecore_method_endpoint_owned<1,131>},
    {"GetCalendar",&Impl::native_gamecore_method_endpoint_owned<1,132>},
    {"GetStartEra",&Impl::native_gamecore_method_endpoint_owned<1,133>},
    {"GetGameSpeedType",&Impl::native_gamecore_method_endpoint_owned<1,134>},
    {"GetRankPlayer",&Impl::native_gamecore_method_endpoint_owned<1,135>},
    {"GetPlayerRank",&Impl::native_gamecore_method_endpoint_owned<1,136>},
    {"GetPlayerScore",&Impl::native_gamecore_method_endpoint_owned<1,137>},
    {"GetRankTeam",&Impl::native_gamecore_method_endpoint_owned<1,138>},
    {"GetTeamRank",&Impl::native_gamecore_method_endpoint_owned<1,139>},
    {"GetTeamScore",&Impl::native_gamecore_method_endpoint_owned<1,140>},
    {"IsOption",&Impl::native_gamecore_method_endpoint_owned<1,141>},
    {"SetOption",&Impl::native_gamecore_method_endpoint_owned<1,142>},
    {"IsMPOption",&Impl::native_gamecore_method_endpoint_owned<1,143>},
    {"GetUnitCreatedCount",&Impl::native_gamecore_method_endpoint_owned<1,144>},
    {"GetUnitClassCreatedCount",&Impl::native_gamecore_method_endpoint_owned<1,145>},
    {"IsUnitClassMaxedOut",&Impl::native_gamecore_method_endpoint_owned<1,146>},
    {"GetBuildingClassCreatedCount",&Impl::native_gamecore_method_endpoint_owned<1,147>},
    {"IsBuildingClassMaxedOut",&Impl::native_gamecore_method_endpoint_owned<1,148>},
    {"GetProjectCreatedCount",&Impl::native_gamecore_method_endpoint_owned<1,149>},
    {"IsProjectMaxedOut",&Impl::native_gamecore_method_endpoint_owned<1,150>},
    {"IsVictoryValid",&Impl::native_gamecore_method_endpoint_owned<1,151>},
    {"SetVictoryValid",&Impl::native_gamecore_method_endpoint_owned<1,152>},
    {"IsSpecialUnitValid",&Impl::native_gamecore_method_endpoint_owned<1,153>},
    {"MakeSpecialUnitValid",&Impl::native_gamecore_method_endpoint_owned<1,154>},
    {"IsNukesValid",&Impl::native_gamecore_method_endpoint_owned<1,155>},
    {"MakeNukesValid",&Impl::native_gamecore_method_endpoint_owned<1,156>},
    {"IsInAdvancedStart",&Impl::native_gamecore_method_endpoint_owned<1,157>},
    {"SetName",&Impl::native_gamecore_method_endpoint_owned<1,158>},
    {"GetName",&Impl::native_gamecore_method_endpoint_owned<1,159>},
    {"Rand",&Impl::native_gamecore_method_endpoint_owned<1,160>},
    {"CalculateSyncChecksum",&Impl::native_gamecore_method_endpoint_owned<1,161>},
    {"CalculateOptionsChecksum",&Impl::native_gamecore_method_endpoint_owned<1,162>},
    {"GetReplayMessage",&Impl::native_gamecore_method_endpoint_owned<1,163>},
    {"GetReplayMessages",&Impl::native_gamecore_method_endpoint_owned<1,164>},
    {"GetNumReplayMessages",&Impl::native_gamecore_method_endpoint_owned<1,165>},
    {"GetReplayInfo",&Impl::native_gamecore_method_endpoint_owned<1,166>},
    {"SaveReplay",&Impl::native_gamecore_method_endpoint_owned<1,167>},
    {"AddPlayer",&Impl::native_gamecore_method_endpoint_owned<1,168>},
    {"SetPlotExtraYield",&Impl::native_gamecore_method_endpoint_owned<1,169>},
    {"ChangePlotExtraCost",&Impl::native_gamecore_method_endpoint_owned<1,170>},
    {"IsCivEverActive",&Impl::native_gamecore_method_endpoint_owned<1,171>},
    {"IsLeaderEverActive",&Impl::native_gamecore_method_endpoint_owned<1,172>},
    {"IsUnitEverActive",&Impl::native_gamecore_method_endpoint_owned<1,173>},
    {"IsBuildingEverActive",&Impl::native_gamecore_method_endpoint_owned<1,174>},
    {"CanDoControl",&Impl::native_gamecore_method_endpoint_owned<1,175>},
    {"DoControl",&Impl::native_gamecore_method_endpoint_owned<1,176>},
    {"DoMinorPledgeProtection",&Impl::native_gamecore_method_endpoint_owned<1,177>},
    {"DoMinorGoldGift",&Impl::native_gamecore_method_endpoint_owned<1,178>},
    {"DoMinorGiftGold",&Impl::native_gamecore_method_endpoint_owned<1,179>},
    {"DoMinorGiftTileImprovement",&Impl::native_gamecore_method_endpoint_owned<1,180>},
    {"DoMinorBullyGold",&Impl::native_gamecore_method_endpoint_owned<1,181>},
    {"DoMinorBullyUnit",&Impl::native_gamecore_method_endpoint_owned<1,182>},
    {"DoMinorBuyout",&Impl::native_gamecore_method_endpoint_owned<1,183>},
    {"GetBestWondersPlayer",&Impl::native_gamecore_method_endpoint_owned<1,184>},
    {"GetBestPoliciesPlayer",&Impl::native_gamecore_method_endpoint_owned<1,185>},
    {"GetBestGreatPeoplePlayer",&Impl::native_gamecore_method_endpoint_owned<1,186>},
    {"IsEverAttackedTutorial",&Impl::native_gamecore_method_endpoint_owned<1,187>},
    {"GameplayDiplomacyAILeaderMessage",&Impl::native_gamecore_method_endpoint_owned<1,188>},
    {"GetResourceUsageType",&Impl::native_gamecore_method_endpoint_owned<1,189>},
    {"GetNumResourceRequiredForUnit",&Impl::native_gamecore_method_endpoint_owned<1,190>},
    {"GetNumResourceRequiredForBuilding",&Impl::native_gamecore_method_endpoint_owned<1,191>},
    {"IsCombatWarned",&Impl::native_gamecore_method_endpoint_owned<1,192>},
    {"SetCombatWarned",&Impl::native_gamecore_method_endpoint_owned<1,193>},
    {"GetAdvisorCounsel",&Impl::native_gamecore_method_endpoint_owned<1,194>},
    {"SetAdvisorRecommenderCity",&Impl::native_gamecore_method_endpoint_owned<1,195>},
    {"IsUnitRecommended",&Impl::native_gamecore_method_endpoint_owned<1,196>},
    {"IsBuildingRecommended",&Impl::native_gamecore_method_endpoint_owned<1,197>},
    {"IsProjectRecommended",&Impl::native_gamecore_method_endpoint_owned<1,198>},
    {"SetAdvisorRecommenderTech",&Impl::native_gamecore_method_endpoint_owned<1,199>},
    {"IsTechRecommended",&Impl::native_gamecore_method_endpoint_owned<1,200>},
    {"GetVariableCitySizeFromPopulation",&Impl::native_gamecore_method_endpoint_owned<1,201>},
    {"GetResearchAgreementCost",&Impl::native_gamecore_method_endpoint_owned<1,202>},
    {"GetCustomOption",&Impl::native_gamecore_method_endpoint_owned<1,203>},
    {"GetNumCitiesPolicyCostMod",&Impl::native_gamecore_method_endpoint_owned<1,204>},
    {"GetNumCitiesTechCostMod",&Impl::native_gamecore_method_endpoint_owned<1,205>},
    {"GetBuildingYieldChange",&Impl::native_gamecore_method_endpoint_owned<1,206>},
    {"GetBuildingYieldModifier",&Impl::native_gamecore_method_endpoint_owned<1,207>},
    {"GetWorldNumCitiesUnhappinessPercent",&Impl::native_gamecore_method_endpoint_owned<1,208>},
    {"GetDealDuration",&Impl::native_gamecore_method_endpoint_owned<1,209>},
    {"GetPeaceDuration",&Impl::native_gamecore_method_endpoint_owned<1,210>},
    {"GetUnitUpgradesTo",&Impl::native_gamecore_method_endpoint_owned<1,211>},
    {"GetCombatPrediction",&Impl::native_gamecore_method_endpoint_owned<1,212>},
    {"GetTimeString",&Impl::native_gamecore_method_endpoint_owned<1,213>},
    {"GetMinimumFaithNextPantheon",&Impl::native_gamecore_method_endpoint_owned<1,214>},
    {"SetMinimumFaithNextPantheon",&Impl::native_gamecore_method_endpoint_owned<1,215>},
    {"GetAvailablePantheonBeliefs",&Impl::native_gamecore_method_endpoint_owned<1,216>},
    {"GetAvailableFounderBeliefs",&Impl::native_gamecore_method_endpoint_owned<1,217>},
    {"GetAvailableFollowerBeliefs",&Impl::native_gamecore_method_endpoint_owned<1,218>},
    {"GetAvailableEnhancerBeliefs",&Impl::native_gamecore_method_endpoint_owned<1,219>},
    {"GetAvailableBonusBeliefs",&Impl::native_gamecore_method_endpoint_owned<1,220>},
    {"GetAvailableReformationBeliefs",&Impl::native_gamecore_method_endpoint_owned<1,221>},
    {"GetNumFollowers",&Impl::native_gamecore_method_endpoint_owned<1,222>},
    {"GetNumCitiesFollowing",&Impl::native_gamecore_method_endpoint_owned<1,223>},
    {"GetBeliefsInReligion",&Impl::native_gamecore_method_endpoint_owned<1,224>},
    {"GetNumReligionsStillToFound",&Impl::native_gamecore_method_endpoint_owned<1,225>},
    {"GetNumReligionsFounded",&Impl::native_gamecore_method_endpoint_owned<1,226>},
    {"GetHolyCityForReligion",&Impl::native_gamecore_method_endpoint_owned<1,227>},
    {"GetReligionName",&Impl::native_gamecore_method_endpoint_owned<1,228>},
    {"GetFounderBenefitsReligion",&Impl::native_gamecore_method_endpoint_owned<1,229>},
    {"FoundPantheon",&Impl::native_gamecore_method_endpoint_owned<1,230>},
    {"FoundReligion",&Impl::native_gamecore_method_endpoint_owned<1,231>},
    {"EnhanceReligion",&Impl::native_gamecore_method_endpoint_owned<1,232>},
    {"SetHolyCity",&Impl::native_gamecore_method_endpoint_owned<1,233>},
    {"GetFounder",&Impl::native_gamecore_method_endpoint_owned<1,234>},
    {"SetFounder",&Impl::native_gamecore_method_endpoint_owned<1,235>},
    {"GetTurnsBetweenMinorCivElections",&Impl::native_gamecore_method_endpoint_owned<1,236>},
    {"GetTurnsUntilMinorCivElection",&Impl::native_gamecore_method_endpoint_owned<1,237>},
    {"GetNumActiveLeagues",&Impl::native_gamecore_method_endpoint_owned<1,238>},
    {"GetNumLeaguesEverFounded",&Impl::native_gamecore_method_endpoint_owned<1,239>},
    {"GetLeague",&Impl::native_gamecore_method_endpoint_owned<1,240>},
    {"GetActiveLeague",&Impl::native_gamecore_method_endpoint_owned<1,241>},
    {"IsProcessingMessages",&Impl::native_gamecore_method_endpoint_owned<1,242>},
    {"GetGreatWorkTooltip",&Impl::native_gamecore_method_endpoint_owned<1,243>},
    {"GetGreatWorkName",&Impl::native_gamecore_method_endpoint_owned<1,244>},
    {"GetGreatWorkType",&Impl::native_gamecore_method_endpoint_owned<1,245>},
    {"GetGreatWorkClass",&Impl::native_gamecore_method_endpoint_owned<1,246>},
    {"GetGreatWorkArtist",&Impl::native_gamecore_method_endpoint_owned<1,247>},
    {"GetGreatWorkEra",&Impl::native_gamecore_method_endpoint_owned<1,248>},
    {"GetGreatWorkEraAbbreviation",&Impl::native_gamecore_method_endpoint_owned<1,249>},
    {"GetGreatWorkEraShort",&Impl::native_gamecore_method_endpoint_owned<1,250>},
    {"GetGreatWorkCreator",&Impl::native_gamecore_method_endpoint_owned<1,251>},
    {"GetGreatWorkController",&Impl::native_gamecore_method_endpoint_owned<1,252>},
    {"GetGreatWorkCurrentThemingBonus",&Impl::native_gamecore_method_endpoint_owned<1,253>},
    {"GetArtifactName",&Impl::native_gamecore_method_endpoint_owned<1,254>},
    {"GetNumFreePolicies",&Impl::native_gamecore_method_endpoint_owned<1,255>},
    {"GetLongestCityConnectionPlots",&Impl::native_gamecore_method_endpoint_owned<1,256>},
    {"SelectedUnit_SpeculativePopupTradeRoute_Display",&Impl::native_gamecore_method_endpoint_owned<1,257>},
    {"SelectedUnit_SpeculativePopupTradeRoute_Hide",&Impl::native_gamecore_method_endpoint_owned<1,258>},
    {"MouseoverUnit",&Impl::native_gamecore_method_endpoint_owned<1,259>},
    {"GetNumArchaeologySites",&Impl::native_gamecore_method_endpoint_owned<1,260>},
    {"GetNumHiddenArchaeologySites",&Impl::native_gamecore_method_endpoint_owned<1,261>},
};
static constexpr NativeGameCoreMethodRecord native_gamecore_plot_methods[]={
    {"CanHaveFeature",&Impl::native_gamecore_method_endpoint_owned<2,0>},
    {"GetFeatureType",&Impl::native_gamecore_method_endpoint_owned<2,1>},
    {"GetTerrainType",&Impl::native_gamecore_method_endpoint_owned<2,2>},
    {"IsRiver",&Impl::native_gamecore_method_endpoint_owned<2,3>},
    {"IsWater",&Impl::native_gamecore_method_endpoint_owned<2,4>},
    {"SetFeatureType",&Impl::native_gamecore_method_endpoint_owned<2,5>},
    {"SetTerrainType",&Impl::native_gamecore_method_endpoint_owned<2,6>},
    {"IsNone",&Impl::native_gamecore_method_endpoint_owned<2,7>},
    {"Erase",&Impl::native_gamecore_method_endpoint_owned<2,8>},
    {"GetTeam",&Impl::native_gamecore_method_endpoint_owned<2,9>},
    {"NukeExplosion",&Impl::native_gamecore_method_endpoint_owned<2,10>},
    {"GetInlandCorner",&Impl::native_gamecore_method_endpoint_owned<2,11>},
    {"UpdateFog",&Impl::native_gamecore_method_endpoint_owned<2,12>},
    {"UpdateVisibility",&Impl::native_gamecore_method_endpoint_owned<2,13>},
    {"IsAdjacentToArea",&Impl::native_gamecore_method_endpoint_owned<2,14>},
    {"ShareAdjacentArea",&Impl::native_gamecore_method_endpoint_owned<2,15>},
    {"IsAdjacentToLand",&Impl::native_gamecore_method_endpoint_owned<2,16>},
    {"IsAdjacentToShallowWater",&Impl::native_gamecore_method_endpoint_owned<2,17>},
    {"IsCoastalLand",&Impl::native_gamecore_method_endpoint_owned<2,18>},
    {"IsWithinTeamCityRadius",&Impl::native_gamecore_method_endpoint_owned<2,19>},
    {"IsLake",&Impl::native_gamecore_method_endpoint_owned<2,20>},
    {"IsFreshWater",&Impl::native_gamecore_method_endpoint_owned<2,21>},
    {"IsRiverSide",&Impl::native_gamecore_method_endpoint_owned<2,22>},
    {"IsRiverConnection",&Impl::native_gamecore_method_endpoint_owned<2,23>},
    {"IsRiverCrossingFlowClockwise",&Impl::native_gamecore_method_endpoint_owned<2,24>},
    {"GetNearestLandArea",&Impl::native_gamecore_method_endpoint_owned<2,25>},
    {"SeeFromLevel",&Impl::native_gamecore_method_endpoint_owned<2,26>},
    {"GetNearestLandPlot",&Impl::native_gamecore_method_endpoint_owned<2,27>},
    {"SeeThroughLevel",&Impl::native_gamecore_method_endpoint_owned<2,28>},
    {"CanHaveResource",&Impl::native_gamecore_method_endpoint_owned<2,29>},
    {"CanHaveImprovement",&Impl::native_gamecore_method_endpoint_owned<2,30>},
    {"CanBuild",&Impl::native_gamecore_method_endpoint_owned<2,31>},
    {"GetBuildTime",&Impl::native_gamecore_method_endpoint_owned<2,32>},
    {"GetBuildTurnsLeft",&Impl::native_gamecore_method_endpoint_owned<2,33>},
    {"GetBuildTurnsTotal",&Impl::native_gamecore_method_endpoint_owned<2,34>},
    {"GetFeatureProduction",&Impl::native_gamecore_method_endpoint_owned<2,35>},
    {"GetBestDefender",&Impl::native_gamecore_method_endpoint_owned<2,36>},
    {"GetSelectedUnit",&Impl::native_gamecore_method_endpoint_owned<2,37>},
    {"GetUnitPower",&Impl::native_gamecore_method_endpoint_owned<2,38>},
    {"DefenseModifier",&Impl::native_gamecore_method_endpoint_owned<2,39>},
    {"MovementCost",&Impl::native_gamecore_method_endpoint_owned<2,40>},
    {"GetExtraMovePathCost",&Impl::native_gamecore_method_endpoint_owned<2,41>},
    {"ChangeExtraMovePathCost",&Impl::native_gamecore_method_endpoint_owned<2,42>},
    {"IsAdjacentOwned",&Impl::native_gamecore_method_endpoint_owned<2,43>},
    {"IsAdjacentPlayer",&Impl::native_gamecore_method_endpoint_owned<2,44>},
    {"IsAdjacentTeam",&Impl::native_gamecore_method_endpoint_owned<2,45>},
    {"IsOwned",&Impl::native_gamecore_method_endpoint_owned<2,46>},
    {"IsBarbarian",&Impl::native_gamecore_method_endpoint_owned<2,47>},
    {"IsRevealedBarbarian",&Impl::native_gamecore_method_endpoint_owned<2,48>},
    {"HasBarbarianCamp",&Impl::native_gamecore_method_endpoint_owned<2,49>},
    {"IsVisible",&Impl::native_gamecore_method_endpoint_owned<2,50>},
    {"IsActiveVisible",&Impl::native_gamecore_method_endpoint_owned<2,51>},
    {"IsVisibleToWatchingHuman",&Impl::native_gamecore_method_endpoint_owned<2,52>},
    {"IsAdjacentVisible",&Impl::native_gamecore_method_endpoint_owned<2,53>},
    {"IsAdjacentNonvisible",&Impl::native_gamecore_method_endpoint_owned<2,54>},
    {"IsAdjacentRevealed",&Impl::native_gamecore_method_endpoint_owned<2,55>},
    {"IsAdjacentNonrevealed",&Impl::native_gamecore_method_endpoint_owned<2,56>},
    {"RemoveGoody",&Impl::native_gamecore_method_endpoint_owned<2,57>},
    {"IsGoody",&Impl::native_gamecore_method_endpoint_owned<2,58>},
    {"IsRevealedGoody",&Impl::native_gamecore_method_endpoint_owned<2,59>},
    {"IsFriendlyTerritory",&Impl::native_gamecore_method_endpoint_owned<2,60>},
    {"IsCity",&Impl::native_gamecore_method_endpoint_owned<2,61>},
    {"IsFriendlyCity",&Impl::native_gamecore_method_endpoint_owned<2,62>},
    {"IsEnemyCity",&Impl::native_gamecore_method_endpoint_owned<2,63>},
    {"IsBeingWorked",&Impl::native_gamecore_method_endpoint_owned<2,64>},
    {"IsUnit",&Impl::native_gamecore_method_endpoint_owned<2,65>},
    {"IsVisibleEnemyDefender",&Impl::native_gamecore_method_endpoint_owned<2,66>},
    {"GetNumDefenders",&Impl::native_gamecore_method_endpoint_owned<2,67>},
    {"GetNumVisibleEnemyDefenders",&Impl::native_gamecore_method_endpoint_owned<2,68>},
    {"GetNumVisiblePotentialEnemyDefenders",&Impl::native_gamecore_method_endpoint_owned<2,69>},
    {"IsVisibleEnemyUnit",&Impl::native_gamecore_method_endpoint_owned<2,70>},
    {"IsVisibleOtherUnit",&Impl::native_gamecore_method_endpoint_owned<2,71>},
    {"GetNumFriendlyUnitsOfType",&Impl::native_gamecore_method_endpoint_owned<2,72>},
    {"IsFighting",&Impl::native_gamecore_method_endpoint_owned<2,73>},
    {"IsRoute",&Impl::native_gamecore_method_endpoint_owned<2,74>},
    {"IsTradeRoute",&Impl::native_gamecore_method_endpoint_owned<2,75>},
    {"IsValidDomainForLocation",&Impl::native_gamecore_method_endpoint_owned<2,76>},
    {"IsValidDomainForAction",&Impl::native_gamecore_method_endpoint_owned<2,77>},
    {"IsImpassable",&Impl::native_gamecore_method_endpoint_owned<2,78>},
    {"GetX",&Impl::native_gamecore_method_endpoint_owned<2,79>},
    {"GetY",&Impl::native_gamecore_method_endpoint_owned<2,80>},
    {"At",&Impl::native_gamecore_method_endpoint_owned<2,81>},
    {"GetPlotIndex",&Impl::native_gamecore_method_endpoint_owned<2,82>},
    {"GetLatitude",&Impl::native_gamecore_method_endpoint_owned<2,83>},
    {"Area",&Impl::native_gamecore_method_endpoint_owned<2,84>},
    {"WaterArea",&Impl::native_gamecore_method_endpoint_owned<2,85>},
    {"GetArea",&Impl::native_gamecore_method_endpoint_owned<2,86>},
    {"SetArea",&Impl::native_gamecore_method_endpoint_owned<2,87>},
    {"GetFeatureVariety",&Impl::native_gamecore_method_endpoint_owned<2,88>},
    {"GetOwnershipDuration",&Impl::native_gamecore_method_endpoint_owned<2,89>},
    {"IsOwnershipScore",&Impl::native_gamecore_method_endpoint_owned<2,90>},
    {"SetOwnershipDuration",&Impl::native_gamecore_method_endpoint_owned<2,91>},
    {"ChangeOwnershipDuration",&Impl::native_gamecore_method_endpoint_owned<2,92>},
    {"GetImprovementDuration",&Impl::native_gamecore_method_endpoint_owned<2,93>},
    {"SetImprovementDuration",&Impl::native_gamecore_method_endpoint_owned<2,94>},
    {"ChangeImprovementDuration",&Impl::native_gamecore_method_endpoint_owned<2,95>},
    {"GetUpgradeProgress",&Impl::native_gamecore_method_endpoint_owned<2,96>},
    {"GetUpgradeTimeLeft",&Impl::native_gamecore_method_endpoint_owned<2,97>},
    {"SetUpgradeProgress",&Impl::native_gamecore_method_endpoint_owned<2,98>},
    {"ChangeUpgradeProgress",&Impl::native_gamecore_method_endpoint_owned<2,99>},
    {"GetCityRadiusCount",&Impl::native_gamecore_method_endpoint_owned<2,100>},
    {"IsCityRadius",&Impl::native_gamecore_method_endpoint_owned<2,101>},
    {"IsStartingPlot",&Impl::native_gamecore_method_endpoint_owned<2,102>},
    {"SetStartingPlot",&Impl::native_gamecore_method_endpoint_owned<2,103>},
    {"IsNEOfRiver",&Impl::native_gamecore_method_endpoint_owned<2,104>},
    {"SetNEOfRiver",&Impl::native_gamecore_method_endpoint_owned<2,105>},
    {"IsWOfRiver",&Impl::native_gamecore_method_endpoint_owned<2,106>},
    {"SetWOfRiver",&Impl::native_gamecore_method_endpoint_owned<2,107>},
    {"IsNWOfRiver",&Impl::native_gamecore_method_endpoint_owned<2,108>},
    {"SetNWOfRiver",&Impl::native_gamecore_method_endpoint_owned<2,109>},
    {"GetRiverEFlowDirection",&Impl::native_gamecore_method_endpoint_owned<2,110>},
    {"GetRiverSEFlowDirection",&Impl::native_gamecore_method_endpoint_owned<2,111>},
    {"GetRiverSWFlowDirection",&Impl::native_gamecore_method_endpoint_owned<2,112>},
    {"IsPotentialCityWork",&Impl::native_gamecore_method_endpoint_owned<2,113>},
    {"IsPotentialCityWorkForArea",&Impl::native_gamecore_method_endpoint_owned<2,114>},
    {"GetOwner",&Impl::native_gamecore_method_endpoint_owned<2,115>},
    {"SetOwner",&Impl::native_gamecore_method_endpoint_owned<2,116>},
    {"GetPlotType",&Impl::native_gamecore_method_endpoint_owned<2,117>},
    {"IsFlatlands",&Impl::native_gamecore_method_endpoint_owned<2,118>},
    {"IsHills",&Impl::native_gamecore_method_endpoint_owned<2,119>},
    {"IsOpenGround",&Impl::native_gamecore_method_endpoint_owned<2,120>},
    {"IsRoughGround",&Impl::native_gamecore_method_endpoint_owned<2,121>},
    {"IsMountain",&Impl::native_gamecore_method_endpoint_owned<2,122>},
    {"SetPlotType",&Impl::native_gamecore_method_endpoint_owned<2,123>},
    {"GetResourceType",&Impl::native_gamecore_method_endpoint_owned<2,124>},
    {"GetNonObsoleteResourceType",&Impl::native_gamecore_method_endpoint_owned<2,125>},
    {"SetResourceType",&Impl::native_gamecore_method_endpoint_owned<2,126>},
    {"GetNumResource",&Impl::native_gamecore_method_endpoint_owned<2,127>},
    {"SetNumResource",&Impl::native_gamecore_method_endpoint_owned<2,128>},
    {"ChangeNumResource",&Impl::native_gamecore_method_endpoint_owned<2,129>},
    {"GetImprovementType",&Impl::native_gamecore_method_endpoint_owned<2,130>},
    {"SetImprovementType",&Impl::native_gamecore_method_endpoint_owned<2,131>},
    {"SetImprovementPillaged",&Impl::native_gamecore_method_endpoint_owned<2,132>},
    {"GetRouteType",&Impl::native_gamecore_method_endpoint_owned<2,133>},
    {"SetRouteType",&Impl::native_gamecore_method_endpoint_owned<2,134>},
    {"IsRoutePillaged",&Impl::native_gamecore_method_endpoint_owned<2,135>},
    {"GetPlotCity",&Impl::native_gamecore_method_endpoint_owned<2,136>},
    {"GetWorkingCity",&Impl::native_gamecore_method_endpoint_owned<2,137>},
    {"GetWorkingCityOverride",&Impl::native_gamecore_method_endpoint_owned<2,138>},
    {"GetReconCount",&Impl::native_gamecore_method_endpoint_owned<2,139>},
    {"GetRiverCrossingCount",&Impl::native_gamecore_method_endpoint_owned<2,140>},
    {"GetYield",&Impl::native_gamecore_method_endpoint_owned<2,141>},
    {"CalculateNatureYield",&Impl::native_gamecore_method_endpoint_owned<2,142>},
    {"CalculateBestNatureYield",&Impl::native_gamecore_method_endpoint_owned<2,143>},
    {"CalculateTotalBestNatureYield",&Impl::native_gamecore_method_endpoint_owned<2,144>},
    {"CalculateImprovementYieldChange",&Impl::native_gamecore_method_endpoint_owned<2,145>},
    {"CalculateYield",&Impl::native_gamecore_method_endpoint_owned<2,146>},
    {"HasYield",&Impl::native_gamecore_method_endpoint_owned<2,147>},
    {"GetYieldWithBuild",&Impl::native_gamecore_method_endpoint_owned<2,148>},
    {"CountNumAirUnits",&Impl::native_gamecore_method_endpoint_owned<2,149>},
    {"GetFoundValue",&Impl::native_gamecore_method_endpoint_owned<2,150>},
    {"IsBestAdjacentFound",&Impl::native_gamecore_method_endpoint_owned<2,151>},
    {"GetPlayerCityRadiusCount",&Impl::native_gamecore_method_endpoint_owned<2,152>},
    {"IsPlayerCityRadius",&Impl::native_gamecore_method_endpoint_owned<2,153>},
    {"GetVisibilityCount",&Impl::native_gamecore_method_endpoint_owned<2,154>},
    {"ChangeVisibilityCount",&Impl::native_gamecore_method_endpoint_owned<2,155>},
    {"GetRevealedOwner",&Impl::native_gamecore_method_endpoint_owned<2,156>},
    {"GetRevealedTeam",&Impl::native_gamecore_method_endpoint_owned<2,157>},
    {"IsRiverCrossing",&Impl::native_gamecore_method_endpoint_owned<2,158>},
    {"IsRiverCrossingToPlot",&Impl::native_gamecore_method_endpoint_owned<2,159>},
    {"IsRevealed",&Impl::native_gamecore_method_endpoint_owned<2,160>},
    {"SetRevealed",&Impl::native_gamecore_method_endpoint_owned<2,161>},
    {"GetRevealedImprovementType",&Impl::native_gamecore_method_endpoint_owned<2,162>},
    {"GetRevealedRouteType",&Impl::native_gamecore_method_endpoint_owned<2,163>},
    {"GetBuildProgress",&Impl::native_gamecore_method_endpoint_owned<2,164>},
    {"ChangeBuildProgress",&Impl::native_gamecore_method_endpoint_owned<2,165>},
    {"GetInvisibleVisibilityCount",&Impl::native_gamecore_method_endpoint_owned<2,166>},
    {"IsInvisibleVisible",&Impl::native_gamecore_method_endpoint_owned<2,167>},
    {"ChangeInvisibleVisibilityCount",&Impl::native_gamecore_method_endpoint_owned<2,168>},
    {"GetNumUnits",&Impl::native_gamecore_method_endpoint_owned<2,169>},
    {"GetUnit",&Impl::native_gamecore_method_endpoint_owned<2,170>},
    {"GetNumLayerUnits",&Impl::native_gamecore_method_endpoint_owned<2,171>},
    {"GetLayerUnit",&Impl::native_gamecore_method_endpoint_owned<2,172>},
    {"GetScriptData",&Impl::native_gamecore_method_endpoint_owned<2,173>},
    {"SetScriptData",&Impl::native_gamecore_method_endpoint_owned<2,174>},
    {"GetActiveFogOfWarMode",&Impl::native_gamecore_method_endpoint_owned<2,175>},
    {"IsImprovementPillaged",&Impl::native_gamecore_method_endpoint_owned<2,176>},
    {"CanSeePlot",&Impl::native_gamecore_method_endpoint_owned<2,177>},
    {"GetContinentArtType",&Impl::native_gamecore_method_endpoint_owned<2,178>},
    {"SetContinentArtType",&Impl::native_gamecore_method_endpoint_owned<2,179>},
    {"IsResourceConnectedByImprovement",&Impl::native_gamecore_method_endpoint_owned<2,180>},
    {"IsBuildRemovesFeature",&Impl::native_gamecore_method_endpoint_owned<2,181>},
    {"GetArchaeologyArtifactType",&Impl::native_gamecore_method_endpoint_owned<2,182>},
    {"GetArchaeologyArtifactEra",&Impl::native_gamecore_method_endpoint_owned<2,183>},
    {"GetArchaeologyArtifactPlayer1",&Impl::native_gamecore_method_endpoint_owned<2,184>},
    {"GetArchaeologyArtifactPlayer2",&Impl::native_gamecore_method_endpoint_owned<2,185>},
    {"GetArchaeologyArtifactWork",&Impl::native_gamecore_method_endpoint_owned<2,186>},
    {"HasWrittenArtifact",&Impl::native_gamecore_method_endpoint_owned<2,187>},
    {"GetCityPurchaseID",&Impl::native_gamecore_method_endpoint_owned<2,188>},
    {"SetCityPurchaseID",&Impl::native_gamecore_method_endpoint_owned<2,189>},
};
// SDK CvLuaStaticInstance: named GLOBAL table, MISS only when nil,
// RegisterMembers emits zero-upvalue closures. Invocation and registration
// are separate: no provider is read/called when these closures are created.
static void native_gamecore_map_game_invocation_current_owned(lua_State*L){
    native_gamecore_invocation_current_owned(L);
    native_db_query_current(L);auto&p=owner(L);
    if(p.map.calculated_yield){
        try{p.map.calculated_yield->require_same_world(p.native_gamecore_bootstrap->world);}
        catch(...){p.source_exception=std::current_exception();return (void)luaL_error(L,"GameCore actual calculated plot/Game owner changed");}
    }
    native_db_query_current(L);
}
static int native_gamecore_map_game_core_push_owned(lua_State*L){
    native_gamecore_invocation_current_owned(L);
    native_db_query_current(L);auto&p=owner(L);auto*q=p.native_gamecore_bootstrap.get();
    if(!q||q->retired||q->phase!=NativeGameCoreBootstrapOwner::Phase::returned||!q->world||q->epoch!=p.failure_epoch||q->writes!=p.failed.writes_||q->uncertainty!=p.execution_uncertainty||q->global!=G(p.state)||q->master!=p.state||q->table_ref<0||!q->table||q->host!=&p||q->creator!=p.ui_thread)
        return luaL_error(L,"GameCore namespace has no genuine persistent returned issuer");
    lua_rawgeti(L,LUA_REGISTRYINDEX,q->table_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=q->table)return luaL_error(L,"GameCore canonical table root changed");
    return lua_gettop(L);
}
int native_gamecore_plot_type_ref=LUA_NOREF;const void*native_gamecore_plot_type=nullptr;
const void*native_gamecore_plot_slots_data=nullptr;std::size_t native_gamecore_plot_slots_count=0;
static const void*native_gamecore_plot_pointer_owned(lua_State*L,std::int64_t id){
    native_gamecore_map_game_invocation_current_owned(L);auto&p=owner(L);auto*q=p.native_gamecore_bootstrap.get();
    if(!q||!q->world||!q->world->world.map_plots)return (luaL_error(L,"GameCore actual retained plot storage absent"),nullptr);
    const auto&slots=q->world->world.map_plots->plots;
    if(id<0||static_cast<std::uint64_t>(id)>=slots.size()||slots[static_cast<std::size_t>(id)].index!=static_cast<std::uint64_t>(id))
        return (luaL_error(L,"GameCore actual plot slot identity differs"),nullptr);
    if(!p.native_gamecore_plot_slots_data){p.native_gamecore_plot_slots_data=slots.data();p.native_gamecore_plot_slots_count=slots.size();}
    if(p.native_gamecore_plot_slots_data!=slots.data()||p.native_gamecore_plot_slots_count!=slots.size())
        return (luaL_error(L,"GameCore actual plot slot buffer was replaced"),nullptr);
    return &slots[static_cast<std::size_t>(id)];
}
static void native_gamecore_plot_type_push_owned(lua_State*L){
    native_db_query_current(L);auto&p=owner(L);const int core=native_gamecore_map_game_core_push_owned(L);
    lua_getfield(L,core,"Plot");
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_newtable(L);
        lua_newtable(L);lua_newtable(L);lua_pushliteral(L,"v");lua_setfield(L,-2,"__mode");lua_setmetatable(L,-2);lua_setfield(L,-2,"__instances");
        for(std::size_t i=0;i<std::size(native_gamecore_plot_methods);++i){
            lua_pushcfunction(L,native_gamecore_plot_methods[i].callback);lua_setfield(L,-2,native_gamecore_plot_methods[i].name);
        }
        lua_pushvalue(L,-1);lua_setfield(L,core,"Plot");
        p.native_gamecore_plot_type=lua_topointer(L,-1);lua_pushvalue(L,-1);p.native_gamecore_plot_type_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        native_gamecore_map_game_invocation_current_owned(L);
    }else{
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_gamecore_plot_type_ref);
        if(p.native_gamecore_plot_type_ref<0||lua_topointer(L,-1)!=p.native_gamecore_plot_type||!lua_rawequal(L,-1,-2))
            return (void)luaL_error(L,"GameCore Plot type is foreign to its actual producer");
        lua_pop(L,1);
    }
    lua_remove(L,core);
}
static void native_gamecore_plot_push_owned(lua_State*L,std::int64_t id){
    native_gamecore_map_game_invocation_current_owned(L);
    const void*const pointer=native_gamecore_plot_pointer_owned(L,id);
    native_gamecore_plot_type_push_owned(L);const int type=lua_gettop(L);
    lua_getfield(L,type,"__instances");const int cache=lua_gettop(L);
    if(!lua_istable(L,cache))return (void)luaL_error(L,"GameCore actual Plot weak cache changed");
    lua_pushlightuserdata(L,const_cast<void*>(pointer));lua_rawget(L,cache);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_newtable(L);
        // SDK-visible lightuserdata/pointer-key weak cache, backed by the
        // SAME retained ported world slot. No original32-bit pointer cast.
        lua_pushlightuserdata(L,const_cast<void*>(pointer));lua_setfield(L,-2,"__instance");
        lua_newtable(L);lua_pushvalue(L,type);lua_setfield(L,-2,"__index");lua_setmetatable(L,-2);
        lua_pushlightuserdata(L,const_cast<void*>(pointer));lua_pushvalue(L,-2);lua_rawset(L,cache);
    }
    lua_remove(L,cache);lua_remove(L,type);native_gamecore_map_game_invocation_current_owned(L);
}
static std::int64_t native_gamecore_plot_receiver_owned(lua_State*L){
    native_gamecore_map_game_invocation_current_owned(L);luaL_checktype(L,1,LUA_TTABLE);
    lua_getfield(L,1,"__instance");
    if(lua_type(L,-1)!=LUA_TLIGHTUSERDATA)return (luaL_error(L,"GameCore Plot requires its SDK lightuserdata instance"),0);
    const void*const pointer=lua_touserdata(L,-1);lua_pop(L,1);native_gamecore_map_game_invocation_current_owned(L);auto&p=owner(L);auto*q=p.native_gamecore_bootstrap.get();
    if(!q||!q->world||!q->world->world.map_plots)return (luaL_error(L,"GameCore actual plot storage absent"),0);
    const auto&slots=q->world->world.map_plots->plots;std::int64_t id=-1;
    // Equality only: do not subtract/order/dereference an unauthenticated
    // caller pointer. The real retained slot issues the provider ID.
    for(std::size_t i=0;i<slots.size();++i)if(pointer==&slots[i]){id=static_cast<std::int64_t>(i);break;}
    if(id<0||native_gamecore_plot_pointer_owned(L,id)!=pointer)return (luaL_error(L,"GameCore Plot pointer is not its real world slot"),0);
    // SDK GetInstance accepts a valid pointer in any table, including a
    // __index forwarding table; cache identity is not an extra receiver rule.
    return id;
}
static std::int32_t native_gamecore_original_integer_owned(lua_State*L,int index,bool checked=false){
    if(checked&&!lua_isnumber(L,index))luaL_typerror(L,index,"number");
    const double rounded=std::nearbyint(lua_tonumber(L,index));
    // Original Win32 lua_tointeger FISTP DWORD uses the current FP rounding
    // mode and emits the integer-indefinite bit pattern on masked invalid.
    if(!std::isfinite(rounded)||rounded<INT32_MIN||rounded>INT32_MAX)return INT32_MIN;
    return static_cast<std::int32_t>(rounded);
}
// Exact original SDK Game public endpoint, not a value/name match.
// The persistent producer authenticates the already-copied selected namespace;
// this lookup cannot publish a table or accept a foreign replacement.
static bool native_gamecore_game_processing_endpoint_current_owned(lua_State*L,int environment){
    native_gamecore_invocation_current_owned(L);
    if(environment<0&&environment>LUA_REGISTRYINDEX)environment=lua_gettop(L)+environment+1;
    if(!native_gamecore_preserve_namespace_owned(L,environment,"Game"))return false;
    const int top=lua_gettop(L);
    lua_pushliteral(L,"Game");lua_rawget(L,environment);
    lua_pushliteral(L,"IsProcessingMessages");lua_rawget(L,-2);
    bool same=lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==&Impl::native_gamecore_method_endpoint_owned<1,242>;
    if(same){if(lua_getupvalue(L,-1,1)){lua_pop(L,1);same=false;}}
    lua_settop(L,top);native_gamecore_invocation_current_owned(L);return same;
}
static int native_gamecore_map_game_invoke_owned(lua_State*L,unsigned kind,std::size_t ordinal){
    native_gamecore_map_game_invocation_current_owned(L);auto&p=owner(L);
    const auto*records=kind==0?native_gamecore_map_methods:kind==1?native_gamecore_game_methods:native_gamecore_plot_methods;
    const auto count=kind==0?std::size(native_gamecore_map_methods):kind==1?std::size(native_gamecore_game_methods):std::size(native_gamecore_plot_methods);
    if(ordinal>=count)return luaL_error(L,"GameCore method ordinal changed");
    const char*name=records[ordinal].name;
    if(kind==1&&ordinal==242)return worldview_sp_game_processing_messages_call_owned(L);
    std::int64_t first=0,second=0;bool boolean=false;bool is_bool=false;int results=1;std::optional<std::int64_t> plot;
    p.source_exception={};p.source_error=nullptr;
    // Lua argument errors happen before C++ provider calls, never across a
    // local owning loan. Providers perform fresh current_map_borrow/current
    // checks inside the actual existing bnw_lua_calculated_plot_services.
    const std::int64_t a=native_gamecore_original_integer_owned(L,1),b=native_gamecore_original_integer_owned(L,2),c=native_gamecore_original_integer_owned(L,3),d=native_gamecore_original_integer_owned(L,4);
    // Keep the checked methods' real SDK coercion/argument errors.
    const bool rand_method=kind==0&&!std::strcmp(name,"Rand"),option_method=kind==0&&!std::strcmp(name,"GetCustomOption");
    const auto random_max=rand_method?native_gamecore_original_integer_owned(L,1,true):0;
    const char*label=rand_method?luaL_checkstring(L,2):option_method?luaL_checkstring(L,1):nullptr;
    // SDK lIsOption selects lua_isnumber (numeric strings included),
    // else lua_tostring (NULL allowed). The existing a uses Win32 rounding.
    const bool game_option_method=kind==1&&ordinal==141;
    const bool game_option_numeric=game_option_method&&lua_isnumber(L,1);
    const char*game_option_name=game_option_method&&!game_option_numeric?lua_tostring(L,1):nullptr;
    const std::int64_t receiver=kind==2?native_gamecore_plot_receiver_owned(L):0;
    try{
        if(kind==0){
            if(!std::strcmp(name,"GetGridSize")||!std::strcmp(name,"GetNumPlots")){
                if(!p.map.grid_size)throw ScriptError("actual Map grid provider absent");
                const auto pair=p.map.grid_size();first=pair.first;second=pair.second;
                if(!std::strcmp(name,"GetGridSize"))results=2;else first=wide::multiply_divide(first,second,1);
            }else if(!std::strcmp(name,"IsWrapX")||!std::strcmp(name,"IsWrapY")){
                if(!p.map.wraps)throw ScriptError("actual Map wrap provider absent");
                const auto pair=p.map.wraps();boolean=!std::strcmp(name,"IsWrapX")?pair.first:pair.second;is_bool=true;
            }else if(!std::strcmp(name,"GetFractalFlags")){
                if(!p.map.wraps)throw ScriptError("actual Map wrap provider absent");
                const auto pair=p.map.wraps();boolean=pair.first;second=pair.second;results=-3;
            }else if(!std::strcmp(name,"GetPlot")){
                if(!p.map.plot)throw ScriptError("actual Map plot provider absent");plot=p.map.plot(a,b);results=-1;
            }else if(!std::strcmp(name,"GetPlotByIndex")){
                if(!p.map.plot_by_index)throw ScriptError("actual Map indexed plot provider absent");plot=p.map.plot_by_index(a);results=-1;
            }else if(!std::strcmp(name,"PlotDirection")){
                if(!p.map.direction)throw ScriptError("actual Map direction provider absent");plot=p.map.direction(a,b,c);results=-2;
            }else if(!std::strcmp(name,"PlotDistance")){
                if(!p.map.distance)throw ScriptError("actual Map distance provider absent");first=p.map.distance(a,b,c,d);
            }else if(!std::strcmp(name,"GetWorldSize")){
                if(!p.map.setup_world_size)throw ScriptError("actual Map world-size provider absent");first=p.map.setup_world_size();
            }else if(!std::strcmp(name,"Rand")){
                if(!p.map.random)throw ScriptError("actual Map random provider absent");first=p.map.random(random_max,std::string_view(label));
            }else if(!std::strcmp(name,"GetCustomOption")){
                if(!p.map.setup_custom_option)throw ScriptError("actual Map custom option provider absent");
                plot=p.map.setup_custom_option(std::string_view(label));if(plot)first=*plot;else results=0;
            }else if(!std::strcmp(name,"RecalculateAreas")){
                if(!p.map.setup_recalculate_areas)throw ScriptError("actual Map RecalculateAreas provider absent");p.map.setup_recalculate_areas();results=0;
            }else throw ScriptError(std::string("actual Map backend unavailable: ")+name);
        }else if(kind==1){
            if(!std::strcmp(name,"GetGameTurn")){if(!p.gameplay.game_turn)throw ScriptError("actual Game turn provider absent");first=p.gameplay.game_turn();}
            else if(!std::strcmp(name,"GetGameSpeedType")){if(!p.gameplay.game_speed)throw ScriptError("actual Game speed provider absent");first=p.gameplay.game_speed();}
            else if(ordinal==35){ // SDK CvGame::isNetworkMultiPlayer -> CvPreGame gameType==NETWORK.
                if(!p.map.calculated_yield)throw ScriptError("actual Game.IsNetworkMultiPlayer imported owner absent");
                boolean=p.map.calculated_yield->pregame_is_multiplayer_game();is_bool=true;
            }
            else if(game_option_method){
                if(!p.map.calculated_yield)throw ScriptError("actual Game.IsOption imported owner absent");
                boolean=game_option_numeric?p.map.calculated_yield->game_is_option(static_cast<std::int32_t>(a)):
                    p.map.calculated_yield->game_is_option(game_option_name);is_bool=true;
            }
            else if(!std::strcmp(name,"GetActivePlayer")||!std::strcmp(name,"GetActiveTeam")){
                if(!p.map.calculated_yield)throw ScriptError("actual Game active owner absent");
                first=!std::strcmp(name,"GetActivePlayer")?p.map.calculated_yield->active_player():p.map.calculated_yield->active_team();
            }else throw ScriptError(std::string("actual Game backend unavailable: ")+name);
        }else{
            if(!std::strcmp(name,"GetX")||!std::strcmp(name,"GetY")||!std::strcmp(name,"GetPlotIndex")||!std::strcmp(name,"IsWater")||!std::strcmp(name,"IsMountain")||!std::strcmp(name,"IsImpassable")){
                if(!p.map.plot_value)throw ScriptError("actual Plot value provider absent");
                const auto op=!std::strcmp(name,"GetX")?ScriptPlotQuery::x:!std::strcmp(name,"GetY")?ScriptPlotQuery::y:!std::strcmp(name,"GetPlotIndex")?ScriptPlotQuery::index:!std::strcmp(name,"IsWater")?ScriptPlotQuery::water:!std::strcmp(name,"IsMountain")?ScriptPlotQuery::mountain:ScriptPlotQuery::impassable;
                p.plot_result=p.map.plot_value(receiver,op,-1);
                if(const auto*n=std::get_if<std::int64_t>(&p.plot_result))first=*n;
                else if(const auto*n=std::get_if<bool>(&p.plot_result)){boolean=*n;is_bool=true;}
                else throw ScriptError("actual Plot scalar result type changed");
            }else if(!std::strcmp(name,"CalculateYield")){
                if(!p.map.calculated_yield)throw ScriptError("actual Plot calculated yield provider absent");first=p.map.calculated_yield->value(receiver,b,lua_toboolean(L,3)!=0);
            }else if(!std::strcmp(name,"IsRevealed")){
                if(!p.map.calculated_yield)throw ScriptError("actual Plot reveal provider absent");boolean=p.map.calculated_yield->is_revealed(receiver,b,lua_toboolean(L,3)!=0);is_bool=true;
            }else if(!std::strcmp(name,"GetResourceType")){
                if(!p.map.resource_type)throw ScriptError("actual Plot resource provider absent");first=p.map.resource_type(receiver,lua_gettop(L)>=2?b:-1);
            }else throw ScriptError(std::string("actual Plot backend unavailable: ")+name);
        }
    }catch(...){p.source_exception=std::current_exception();p.source_error="GameCore actual method provider unavailable or failed";}
    if(p.source_error)return luaL_error(L,"%s.%s: %s",kind==0?"Map":kind==1?"Game":"Plot",name,p.source_error);
    native_gamecore_map_game_invocation_current_owned(L);
    if(results==-3){lua_newtable(L);if(boolean){lua_pushboolean(L,true);lua_setfield(L,-2,"FRAC_WRAP_X");}if(second){lua_pushboolean(L,true);lua_setfield(L,-2,"FRAC_WRAP_Y");}native_gamecore_map_game_invocation_current_owned(L);return 1;}
    if(results==-1||results==-2){if(!plot){if(results==-2){lua_pushnil(L);return 1;}return 0;}if(*plot<0||*plot>INT32_MAX)return luaL_error(L,"Map actual plot index outside native range");native_gamecore_plot_push_owned(L,*plot);return 1;}
    if(!results)return 0;
    if(is_bool)lua_pushboolean(L,boolean);else lua_pushinteger(L,first);
    if(results==2)lua_pushinteger(L,second);native_gamecore_map_game_invocation_current_owned(L);return results;
}
static std::pair<std::size_t,std::size_t> native_gamecore_register_map_game_kind_owned(lua_State*L,unsigned kind){
    native_gamecore_export_current_owned(L);const char*name=kind==0?"Map":"Game";
    const auto*rows=kind==0?native_gamecore_map_methods:native_gamecore_game_methods;
    const auto count=kind==0?std::size(native_gamecore_map_methods):std::size(native_gamecore_game_methods);
    lua_getfield(L,LUA_GLOBALSINDEX,name);
    if(lua_isnil(L,-1)){lua_pop(L,1);lua_newtable(L);lua_pushvalue(L,-1);lua_setfield(L,LUA_GLOBALSINDEX,name);}
    for(std::size_t i=0;i<count;++i){lua_pushcfunction(L,rows[i].callback);lua_setfield(L,-2,rows[i].name);native_gamecore_export_current_owned(L);}
    return {1,count}; // own protected SDK pRegister call discards this table
}
static std::pair<std::size_t,std::size_t> native_gamecore_register_map_owned(lua_State*L){return native_gamecore_register_map_game_kind_owned(L,0);}
static std::pair<std::size_t,std::size_t> native_gamecore_register_game_owned(lua_State*L){return native_gamecore_register_map_game_kind_owned(L,1);}
// This helper is called only by the actual prepared-environment configurer.
// The Logic producer authenticates its issued cachedGameCore namespace.
static int native_gamecore_bootstrap_preserve_callback_owned(lua_State*L){
    luaL_checktype(L,1,LUA_TTABLE);const char*name=luaL_checkstring(L,2);auto&p=owner(L);
    if(!p.worldview_native_host||!p.native_gamecore_bootstrap){lua_pushboolean(L,false);return 1;}
    if(!std::strcmp(name,"Map")||!std::strcmp(name,"Game")||!std::strcmp(name,"DirectionTypes")||!std::strcmp(name,"PlotTypes")){
        const bool own=native_gamecore_preserve_namespace_owned(L,1,name);lua_pushboolean(L,own);return 1;
    }
    if(!std::strcmp(name,"Plot")){
        if(!native_gamecore_preserve_namespace_owned(L,1,"Map")){lua_pushboolean(L,false);return 1;}
        native_gamecore_plot_type_push_owned(L);lua_getfield(L,1,"Plot");
        if(!lua_isnil(L,-1)&&!lua_rawequal(L,-1,-2))return luaL_error(L,"Plot bootstrap cannot overwrite a foreign namespace");lua_pop(L,1);
        lua_setfield(L,1,"Plot");native_gamecore_map_game_invocation_current_owned(L);lua_pushboolean(L,true);return 1;
    }
    return luaL_error(L,"GameCore bootstrap preservation key changed");
}

static int native_gamecore_gameplay_preserve_callback_owned(lua_State*L){
    luaL_checktype(L,1,LUA_TTABLE);const char*name=luaL_checkstring(L,2);
    if(!std::strcmp(name,"Game"))return native_gamecore_bootstrap_preserve_callback_owned(L);
    if(!std::strcmp(name,"Player")||!std::strcmp(name,"Team")||!std::strcmp(name,"Players")||!std::strcmp(name,"Teams")){
        auto&p=owner(L);if(!p.worldview_native_host||!p.native_gamecore_bootstrap){lua_pushboolean(L,false);return 1;}
        const bool own=native_gamecore_preserve_namespace_owned(L,1,name);lua_pushboolean(L,own);return 1;
    }
    return worldview_gameplay_namespace_preserve_callback_owned(L);
}
