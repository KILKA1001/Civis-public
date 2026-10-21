// Source-derived army fields and virtual operation read chains.
// Regenerate with army_operations_reference.py.
constexpr FieldSpec bnw_army[] = {
    {"version", E::version},
    {"m_iID", E::i32},
    {"m_eOwner", E::i32},
    {"m_iCurrentX", E::i32},
    {"m_iCurrentY", E::i32},
    {"m_iGoalX", E::i32},
    {"m_iGoalY", E::i32},
    {"m_eDomainType", E::i32},
    {"m_iFormationIndex", E::i32},
    {"m_eAIState", E::i32},
    {"m_iOperationID", E::i32},
};
constexpr FieldSpec bnw_slot[] = {
    {"m_iUnitID", E::i32},
    {"m_iEstimatedTurnAtCheckpoint", E::i32},
    {"m_bStartedOnOperation", E::i32},
};
constexpr FieldSpec bnw_CvAIOperation[] = {
    {"version", E::version},
    {"m_iID", E::i32},
    {"m_eOwner", E::i32},
    {"m_eEnemy", E::i32},
    {"m_eCurrentState", E::i32},
    {"m_iDefaultArea", E::i32},
    {"m_bShouldReplaceLossesWithReinforcements", E::boolean},
    {"m_eMoveType", E::i32},
    {"m_iTargetX", E::i32},
    {"m_iTargetY", E::i32},
    {"m_iMusterX", E::i32},
    {"m_iMusterY", E::i32},
    {"m_iStartCityX", E::i32},
    {"m_iStartCityY", E::i32},
    {"m_iLastTurnMoved", E::i32},
    {"m_viArmyIDs", E::stl_i32},
    {"m_viListOfUnitsWeStillNeedToBuild", E::stl_slots},
    {"m_viListOfUnitsCitiesHaveCommittedToBuild", E::stl_slots},
};
constexpr FieldSpec bnw_CvAIOperationBasicCityAttack[] = {
    {"version", E::version},
};
constexpr FieldSpec bnw_CvAIOperationDestroyBarbarianCamp[] = {
    {"version", E::version},
    {"m_bCivilianRescue", E::boolean},
    {"m_iUnitToRescue", E::i32},
};
constexpr FieldSpec bnw_CvAIOperationPillageEnemy[] = {
    {"version", E::version},
};
constexpr FieldSpec bnw_CvAIEscortedOperation[] = {
    {"version", E::version},
    {"m_bEscorted", E::boolean},
    {"m_eCivilianType", E::i32},
    {"m_iTargetArea", E::i32},
};
constexpr FieldSpec bnw_CvAINavalOperation[] = {
    {"version", E::version},
};
constexpr FieldSpec bnw_CvAIOperationNavalBombardment[] = {
    {"version", E::version},
};
constexpr FieldSpec bnw_CvAIOperationNavalSuperiority[] = {
    {"version", E::version},
};
constexpr FieldSpec bnw_CvAIOperationPureNavalCityAttack[] = {
    {"version", E::version},
};
constexpr FieldSpec bnw_CvAIOperationCityCloseDefense[] = {
    {"version", E::version},
};
constexpr FieldSpec bnw_CvAIOperationRapidResponse[] = {
    {"version", E::version},
};
constexpr FieldSpec bnw_CvAINavalEscortedOperation[] = {
    {"version", E::version},
    {"m_eCivilianType", E::i32},
};
constexpr FieldSpec bnw_CvAIOperationNavalAttack[] = {
    {"version", E::version},
};
constexpr FieldSpec bnw_CvAIOperationNukeAttack[] = {
    {"version", E::version},
    {"m_iBestUnitID", E::i32},
};
constexpr LayerSpec bnw_operation_0[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIOperationCityCloseDefense", bnw_CvAIOperationCityCloseDefense},
};
constexpr LayerSpec bnw_operation_1[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIOperationRapidResponse", bnw_CvAIOperationRapidResponse},
};
constexpr LayerSpec bnw_operation_2[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIOperationBasicCityAttack", bnw_CvAIOperationBasicCityAttack},
};
constexpr LayerSpec bnw_operation_3[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIOperationDestroyBarbarianCamp", bnw_CvAIOperationDestroyBarbarianCamp},
};
constexpr LayerSpec bnw_operation_4[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIEscortedOperation", bnw_CvAIEscortedOperation},
};
constexpr LayerSpec bnw_operation_5[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIOperationPillageEnemy", bnw_CvAIOperationPillageEnemy},
};
constexpr LayerSpec bnw_operation_6[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIOperationBasicCityAttack", bnw_CvAIOperationBasicCityAttack},
};
constexpr LayerSpec bnw_operation_7[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIOperationBasicCityAttack", bnw_CvAIOperationBasicCityAttack},
};
constexpr LayerSpec bnw_operation_8[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIEscortedOperation", bnw_CvAIEscortedOperation},
};
constexpr LayerSpec bnw_operation_9[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAINavalOperation", bnw_CvAINavalOperation},
    {"CvAIOperationNavalBombardment", bnw_CvAIOperationNavalBombardment},
};
constexpr LayerSpec bnw_operation_10[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAINavalOperation", bnw_CvAINavalOperation},
    {"CvAIOperationNavalSuperiority", bnw_CvAIOperationNavalSuperiority},
};
constexpr LayerSpec bnw_operation_11[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAINavalEscortedOperation", bnw_CvAINavalEscortedOperation},
};
constexpr LayerSpec bnw_operation_12[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIEscortedOperation", bnw_CvAIEscortedOperation},
};
constexpr LayerSpec bnw_operation_13[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAINavalEscortedOperation", bnw_CvAINavalEscortedOperation},
    {"CvAIOperationNavalAttack", bnw_CvAIOperationNavalAttack},
};
constexpr LayerSpec bnw_operation_14[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAINavalEscortedOperation", bnw_CvAINavalEscortedOperation},
    {"CvAIOperationNavalAttack", bnw_CvAIOperationNavalAttack},
};
constexpr LayerSpec bnw_operation_15[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIOperationBasicCityAttack", bnw_CvAIOperationBasicCityAttack},
};
constexpr LayerSpec bnw_operation_16[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAINavalEscortedOperation", bnw_CvAINavalEscortedOperation},
    {"CvAIOperationNavalAttack", bnw_CvAIOperationNavalAttack},
};
constexpr LayerSpec bnw_operation_17[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIOperationNukeAttack", bnw_CvAIOperationNukeAttack},
};
constexpr LayerSpec bnw_operation_18[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAINavalOperation", bnw_CvAINavalOperation},
    {"CvAIOperationPureNavalCityAttack", bnw_CvAIOperationPureNavalCityAttack},
};
constexpr LayerSpec bnw_operation_19[] = {
    {"CvAIOperation", bnw_CvAIOperation},
    {"CvAIEscortedOperation", bnw_CvAIEscortedOperation},
};
constexpr OperationSpec bnw_operations[] = {
    {0, "AI_OPERATION_CITY_CLOSE_DEFENSE", bnw_operation_0},
    {1, "AI_OPERATION_RAPID_RESPONSE", bnw_operation_1},
    {2, "AI_OPERATION_BASIC_CITY_ATTACK", bnw_operation_2},
    {3, "AI_OPERATION_DESTROY_BARBARIAN_CAMP", bnw_operation_3},
    {4, "AI_OPERATION_FOUND_CITY", bnw_operation_4},
    {5, "AI_OPERATION_PILLAGE_ENEMY", bnw_operation_5},
    {6, "AI_OPERATION_SNEAK_CITY_ATTACK", bnw_operation_6},
    {7, "AI_OPERATION_SMALL_CITY_ATTACK", bnw_operation_7},
    {8, "AI_OPERATION_MERCHANT_DELEGATION", bnw_operation_8},
    {9, "AI_OPERATION_NAVAL_BOMBARDMENT", bnw_operation_9},
    {10, "AI_OPERATION_NAVAL_SUPERIORITY", bnw_operation_10},
    {11, "AI_OPERATION_COLONIZE", bnw_operation_11},
    {12, "AI_OPERATION_QUICK_COLONIZE", bnw_operation_12},
    {13, "AI_OPERATION_NAVAL_ATTACK", bnw_operation_13},
    {14, "AI_OPERATION_NAVAL_SNEAK_ATTACK", bnw_operation_14},
    {15, "AI_OPERATION_CITY_STATE_ATTACK", bnw_operation_15},
    {16, "AI_OPERATION_CITY_STATE_NAVAL_ATTACK", bnw_operation_16},
    {17, "AI_OPERATION_NUKE_ATTACK", bnw_operation_17},
    {18, "AI_OPERATION_PURE_NAVAL_CITY_ATTACK", bnw_operation_18},
    {19, "AI_OPERATION_CONCERT_TOUR", bnw_operation_19},
};
constexpr FieldSpec vp_army[] = {
    {"m_iID", E::i32},
    {"m_eOwner", E::i32},
    {"m_iGoalX", E::i32},
    {"m_iGoalY", E::i32},
    {"m_eType", E::i32},
    {"m_eFormation", E::i32},
    {"m_eAIState", E::i32},
    {"m_iOperationID", E::i32},
};
constexpr FieldSpec vp_slot[] = {
    {"m_iUnitID", E::i32},
    {"m_estTurnsToCheckpoint", E::stl_i32},
    {"m_bIsRequired", E::boolean},
};
constexpr FieldSpec vp_CvAIOperation[] = {
    {"m_iID", E::i32},
    {"m_eType", E::i32},
    {"m_eArmyType", E::i32},
    {"m_eOwner", E::i32},
    {"m_eEnemy", E::i32},
    {"m_eCurrentState", E::i32},
    {"m_eAbortReason", E::i32},
    {"m_iTargetX", E::i32},
    {"m_iTargetY", E::i32},
    {"m_iMusterX", E::i32},
    {"m_iMusterY", E::i32},
    {"m_iTurnStarted", E::i32},
    {"m_iLastTurnMoved", E::i32},
    {"m_viArmyIDs", E::stl_i32},
    {"m_viListOfUnitsWeStillNeedToBuild", E::stl_slots},
    {"m_viListOfUnitsCitiesHaveCommittedToBuild", E::stl_slots},
    {"m_iDistanceMusterToTarget", E::i32},
};
constexpr FieldSpec vp_CvAIOperationCivilian[] = {
    {"m_eCivilianType", E::i32},
};
constexpr LayerSpec vp_operation_0[] = {
    {"CvAIOperation", vp_CvAIOperation},
    {"CvAIOperationCivilian", vp_CvAIOperationCivilian},
};
constexpr LayerSpec vp_operation_1[] = {
    {"CvAIOperation", vp_CvAIOperation},
};
constexpr LayerSpec vp_operation_2[] = {
    {"CvAIOperation", vp_CvAIOperation},
};
constexpr LayerSpec vp_operation_3[] = {
    {"CvAIOperation", vp_CvAIOperation},
};
constexpr LayerSpec vp_operation_4[] = {
    {"CvAIOperation", vp_CvAIOperation},
};
constexpr LayerSpec vp_operation_5[] = {
    {"CvAIOperation", vp_CvAIOperation},
};
constexpr LayerSpec vp_operation_6[] = {
    {"CvAIOperation", vp_CvAIOperation},
};
constexpr LayerSpec vp_operation_7[] = {
    {"CvAIOperation", vp_CvAIOperation},
};
constexpr LayerSpec vp_operation_8[] = {
    {"CvAIOperation", vp_CvAIOperation},
};
constexpr LayerSpec vp_operation_9[] = {
    {"CvAIOperation", vp_CvAIOperation},
};
constexpr LayerSpec vp_operation_10[] = {
    {"CvAIOperation", vp_CvAIOperation},
    {"CvAIOperationCivilian", vp_CvAIOperationCivilian},
};
constexpr LayerSpec vp_operation_11[] = {
    {"CvAIOperation", vp_CvAIOperation},
    {"CvAIOperationCivilian", vp_CvAIOperationCivilian},
};
constexpr LayerSpec vp_operation_12[] = {
    {"CvAIOperation", vp_CvAIOperation},
    {"CvAIOperationCivilian", vp_CvAIOperationCivilian},
};
constexpr OperationSpec vp_operations[] = {
    {0, "AI_OPERATION_FOUND_CITY", vp_operation_0},
    {1, "AI_OPERATION_PILLAGE_ENEMY", vp_operation_1},
    {2, "AI_OPERATION_CITY_ATTACK_LAND", vp_operation_2},
    {3, "AI_OPERATION_CITY_ATTACK_NAVAL", vp_operation_3},
    {4, "AI_OPERATION_CITY_ATTACK_COMBINED", vp_operation_4},
    {5, "AI_OPERATION_CITY_DEFENSE", vp_operation_5},
    {6, "AI_OPERATION_RAPID_RESPONSE", vp_operation_6},
    {7, "AI_OPERATION_NAVAL_SUPERIORITY", vp_operation_7},
    {8, "AI_OPERATION_NUKE_ATTACK", vp_operation_8},
    {9, "AI_OPERATION_CARRIER_GROUP", vp_operation_9},
    {10, "AI_OPERATION_MUSICIAN_CONCERT_TOUR", vp_operation_10},
    {11, "AI_OPERATION_MERCHANT_DELEGATION", vp_operation_11},
    {12, "AI_OPERATION_DIPLOMAT_DELEGATION", vp_operation_12},
};
