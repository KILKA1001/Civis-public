#include "civ5/runtime/bnw_fresh_game.hpp"
#include <exception>
#include "civ5/runtime/bnw_selected_user_options_owner.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/runtime/bnw_fresh_city_manager.hpp"
#include "civ5/runtime/bnw_fresh_tactical_map.hpp"
#include "civ5/runtime/bnw_fresh_advisors.hpp"
#include "sqlite3.h"
#include "civ5/runtime/bnw_fresh_pregame_speed.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <set>
#include <thread>
#include <utility>
namespace civ5::runtime {
namespace {
[[noreturn]] void refuse(BnwFreshGameError e,const std::string& text) {
    throw BnwFreshGameRefusal(e,text);
}
// Only the two constructor fields needed for CvRandom.get. This private
// allocation is issued before Game constructor in this isolated fresh realm.
// Full Globals.Init/gDLL.InitGlobals and external SetLogging calls are NOT run.
struct FreshGlobalsRandomLogging {
    const std::thread::id thread=std::this_thread::get_id();
    const std::shared_ptr<const BnwCreationConfig> config;
    bool logging;
    std::int32_t rand_logging;
    explicit FreshGlobalsRandomLogging(std::shared_ptr<const BnwCreationConfig> p)
        :config(std::move(p)),logging(false),rand_logging(0) {config->require_current();}
    void current(const BnwCreationConfig& actual) const {
        if(thread!=std::this_thread::get_id())refuse(BnwFreshGameError::wrong_thread,"Globals logging source belongs to another thread");
        config->require_current();if(config.get()!=&actual)
            refuse(BnwFreshGameError::parent_retired,"Globals logging source has foreign fresh realm");
    }
};
struct Handle {
    HANDLE value=INVALID_HANDLE_VALUE;
    Handle()=default;
    Handle(const Handle&)=delete;
    Handle& operator=(const Handle&)=delete;
    ~Handle() { if(value!=INVALID_HANDLE_VALUE) CloseHandle(value); }
};
// Location is minted by this producer, never accepted from an imported save,
// install path, callback, or caller-provided filename. Hold directory handles
// without FILE_SHARE_DELETE for the whole job; reject junctions/reparse points.
struct LocalSessionCache {
    Handle project,build,root,session;
    std::filesystem::path directory,database;
    static void retain_directory(Handle& h,const std::filesystem::path& path) {
        h.value=CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
        BY_HANDLE_FILE_INFORMATION info{};
        if(h.value==INVALID_HANDLE_VALUE || !GetFileInformationByHandle(h.value,&info) ||
           !(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
           (info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))
            refuse(BnwFreshGameError::cache_path_unavailable,"Fresh Game cache requires retained local non-reparse directories");
    }
    LocalSessionCache() {
        // Root approved local workspace policy, not the CivV installation.
        retain_directory(project,L"C:/CrazyProject");
        const std::filesystem::path build_path=L"C:/CrazyProject/build";
        retain_directory(build,build_path);
        const auto root_path=build_path/L"session-cache";
        if(!CreateDirectoryW(root_path.c_str(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS)
            refuse(BnwFreshGameError::cache_path_unavailable,"Cannot create project session cache root");
        retain_directory(root,root_path);
        GUID id{};
        if(FAILED(CoCreateGuid(&id)))
            refuse(BnwFreshGameError::cache_path_unavailable,"Cannot mint unique cache session UUID");
        std::array<wchar_t,40> text{};
        if(StringFromGUID2(id,text.data(),static_cast<int>(text.size()))==0)
            refuse(BnwFreshGameError::cache_path_unavailable,"Cannot encode cache session UUID");
        directory=root_path/std::wstring(text.data());
        // Collision refuses; NEVER reuse or delete a previous session directory.
        if(!CreateDirectoryW(directory.c_str(),nullptr))
            refuse(BnwFreshGameError::cache_path_unavailable,"Fresh cache session UUID already exists or cannot be created");
        retain_directory(session,directory);
        const auto resolved=std::filesystem::canonical(directory);
        if(resolved.parent_path()!=std::filesystem::canonical(root_path) ||
           resolved.parent_path().parent_path()!=std::filesystem::canonical(build_path))
            refuse(BnwFreshGameError::cache_path_unavailable,"Cache session resolved outside retained project root");
        directory=resolved;database=directory/L"Civ5SavedGameDatabase.db";
    }
    void reset_database() const {
        // This one exact file is minted inside this job's retained UUID leaf.
        if(!DeleteFileW(database.c_str()) && GetLastError()!=ERROR_FILE_NOT_FOUND)
            refuse(BnwFreshGameError::cache_effect_failed,"Cannot delete own saved-game cache database");
        sqlite3* raw=nullptr;
        const auto utf8=database.u8string();
        const int open=sqlite3_open_v2(reinterpret_cast<const char*>(utf8.c_str()),&raw,
            SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE|SQLITE_OPEN_FULLMUTEX,nullptr);
        struct Close { sqlite3* db; ~Close() { if(db) sqlite3_close(db); } } close{raw};
        if(open!=SQLITE_OK || !raw)
            refuse(BnwFreshGameError::cache_effect_failed,"Cannot open own saved-game cache database");
        if(sqlite3_exec(raw,"CREATE TABLE SimpleValues(Name TEXT Primary Key, Value VARIANT)",nullptr,nullptr,nullptr)!=SQLITE_OK)
            refuse(BnwFreshGameError::cache_effect_failed,"Cannot create SimpleValues in own saved-game cache database");
    }
};
struct Timer {
    LARGE_INTEGER start{};
    float elapsed=0.f;
    Timer() { restart(); }
    void restart() {
        if(!QueryPerformanceCounter(&start))
            refuse(BnwFreshGameError::cache_effect_failed,"Native Game timer counter unavailable");
        elapsed=0.f;
    }
    void stop() {
        LARGE_INTEGER now{},freq{};
        if(!QueryPerformanceCounter(&now) || !QueryPerformanceFrequency(&freq) || freq.QuadPart<=0)
            refuse(BnwFreshGameError::cache_effect_failed,"Native Game timer frequency unavailable");
        // FTimer.h defines m_fFreq as QueryPerformanceFrequency reciprocal.
        const auto reciprocal=1.0/static_cast<double>(freq.QuadPart);
        elapsed=static_cast<float>(static_cast<double>(now.QuadPart-start.QuadPart)*reciprocal);
    }
};
struct EmptyVoteList {
    // Exact initial FFreeListArrayBase/TrashArray metadata. Uninit and
    // RemoveAll return without changing these when the array is NULL.
    std::int32_t free_head=-2,free_count=0,last=-1,num_slots=0,current_id=8192;
    struct Node {std::int32_t next_free=-1;std::nullptr_t data=nullptr;};
    std::unique_ptr<std::vector<Node>> nodes;
    void uninit() { if(nodes) {remove_all();nodes.reset();} }
    void init() {
        // Native Init() default8, already a power of two.
        uninit();num_slots=8;last=-1;free_head=-1;free_count=0;current_id=8192;
        nodes=std::make_unique<std::vector<Node>>(8);
    }
    void remove_all() { if(nodes) {last=-1;free_head=-1;free_count=0;for(auto& node:*nodes)node=Node{};} }
};
struct SiteEvaluatorOwned {
    // CvCitySiteEvaluator constructor writes ONLY these two fields;
    // derived CvSiteEvaluatorForSettler constructor is empty.
    std::int32_t expansion=12,growth=13;
    // Native inline arrays exist but constructor does not initialize them.
    std::array<std::optional<std::int32_t>,8> ring;
    std::array<std::optional<std::int32_t>,9> flavor_multiplier;
    std::optional<std::int32_t> naval,brazil,spain,morroco,netherlands,inca;
};
// Native vector element shapes. No element is created by constructor/Init;
// the region/rank algorithms that would fill them remain unimplemented.
struct ContinentOwned {
    std::int32_t fertility=0,next_region_fertility=0,regions_assigned=0,area=-1;
};
struct StartRegionOwned {
    std::array<std::optional<std::int32_t>,4> boundaries;
    std::int32_t fertility=0,area=-1,civs_placed=0;
    bool large_enough_for_major=true;
};
struct PlayerStartRankOwned {std::int32_t player=-1,rank=0;};
struct StartPositionerOwned {
    // SDK constructor does NOT write this pointer or gameplay iterator.
    std::optional<SiteEvaluatorOwned*> evaluator;
    std::vector<ContinentOwned> continents;
    std::vector<StartRegionOwned> regions;
    std::vector<PlayerStartRankOwned> player_order;
    std::optional<std::size_t> player_order_iterator;
    std::int32_t required_separation=0,best_found_on_map=0;
};
// Native fixed-vector size0/capacity: no element constructor is reached.
// This is logical owned empty storage, not original inline-memory ABI parity.
struct GameCultureOwned {
    std::uint32_t great_work_count=0,inline_capacity=22;
    std::optional<bool> reported_influential;
};
struct GameLeaguesOwned {
    std::uint32_t active_count=0,inline_capacity=2;
    std::optional<std::int32_t> ever_founded,diplomatic_victor,generated_id,last_era;
};
struct GameTradeOwned {
    std::uint32_t connection_count=0,inline_capacity=14*12;
    std::optional<std::int32_t> next_id,popup_x,popup_y,popup_type;
    // Native ctor/Reset do NOT initialize m_aaiTechDifference.
    std::array<std::array<std::optional<std::int32_t>,22>,22> tech_difference;
};
struct GameReligionsOwned {
    // BaseVector constructor size0; FStaticVector<CvReligion,16,false>
    // selects inline capacity16 WITHOUT constructing any religion elements.
    // No future element storage/Read/Write operation is implemented here.
    std::uint32_t religion_count=0,inline_capacity=16;
    std::int32_t minimum_faith=0;
};
SimulationRandom native_map_random() {
    SimulationRandom value(RandomAlgorithm::bnw,{});
    value.reset();return value; // Default CvRandom constructor only.
}
struct DealsConstructor {
    std::uint32_t counter=0;
    std::vector<std::int32_t> proposed,current,historical,deals;
    DealsConstructor() { proposed.clear();current.clear();historical.clear(); }
};
} // namespace
struct BnwFreshGame::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    std::shared_ptr<const BnwCreationConfig> config;
    std::weak_ptr<BnwFreshGame> actual_facade;
    bool early_pregame_attempted=false,settings_prefix_attempted=false,ordered_globals_attempted=false;
    std::shared_ptr<const BnwSelectedUserOptionsOwner> pregame_user_options;
    struct CivisSetup {
        const std::shared_ptr<const BnwNewGameSetupRules> rules;
        const BnwNewGameSetupRequest request;
        BnwNewGameSetupObservation state;
        CivisSetup(std::shared_ptr<const BnwNewGameSetupRules> r,BnwNewGameSetupRequest q)
            :rules(std::move(r)),request(std::move(q)){}
    };
    std::unique_ptr<CivisSetup> civis_setup;
    std::unique_ptr<BnwNewGamePlayerSetup> player_setup;
    const content::RulesDatabase* db=nullptr;
    std::unique_ptr<LocalSessionCache> cache;
    BnwFreshGameBoundary boundary=BnwFreshGameBoundary::failed_prefix;
    bool attempted=false,settler_init_attempted=false,start_init_attempted=false,positioner_init_attempted=false,religions_prefix_attempted=false,religions_speed_attempted=false,game_services_attempted=false,tactical_advisors_attempted=false,city_manager_final_attempted=false,init_prefix_attempted=false,sp_options_attempted=false,lock_turns_attempted=false;
    std::uint32_t cache_resets=0;
    std::map<std::string,BnwFreshGameScalar,std::less<>> scalars;
    std::map<std::string,BnwFreshGameArray,std::less<>> arrays;
    std::set<std::string,std::less<>> null_services;
    std::optional<std::vector<std::vector<std::int32_t>>> victory;
    // Fresh private equivalents of process-global supporting storage;
    // constructor NULL allocations, not imported saved arrays.
    std::unique_ptr<std::vector<std::array<std::int32_t,3>>> goody;
    std::unique_ptr<std::vector<std::int16_t>> barbarian_counter,barbarian_spawned;
    std::unique_ptr<BnwFreshCityManager> city_manager;
    std::unique_ptr<FreshGlobalsRandomLogging> random_logging;
    std::int32_t scoped_city_manager_resets=0,scoped_city_manager_shutdowns=0;
    std::vector<std::string> destroyed,great_people,advisor_messages;
    std::vector<std::int32_t> replay,extras,costs;
    EmptyVoteList selections,triggered;
    SimulationRandom map_rng=native_map_random();
    SimulationRandom jon_rng{RandomAlgorithm::bnw,{}};
    DealsConstructor deals;
    Timer end_timer,cur_timer,turn_start_timer;
    std::unique_ptr<SiteEvaluatorOwned> settler,start;
    std::unique_ptr<StartPositionerOwned> positioner;
    std::unique_ptr<GameReligionsOwned> religions;
    std::shared_ptr<BnwFreshPregameSpeed> pregame;
    std::unique_ptr<GameCultureOwned> culture;
    std::unique_ptr<GameLeaguesOwned> leagues;
    std::unique_ptr<GameTradeOwned> trade;
    std::unique_ptr<BnwFreshTacticalMap> tactical;
    std::unique_ptr<BnwFreshAdvisorCounsel> counsel;
    std::unique_ptr<BnwFreshAdvisorRecommender> recommender;
    std::vector<std::string> order;
    void current() const {
        // Stable constructor thread precedes all parent/DB/path/state reads.
        if(thread!=std::this_thread::get_id()) refuse(BnwFreshGameError::wrong_thread,"Fresh Game belongs to another thread");
        if(!config) refuse(BnwFreshGameError::parent_missing,"Fresh Game lost CreationConfig");
        config->require_current();
        if(&config->game_rules()!=db) refuse(BnwFreshGameError::parent_retired,"Fresh Game SAME rules parent changed");
        if(civis_setup)civis_setup->rules->require_same_config(*config);
        if(player_setup){
            const auto actual=actual_facade.lock();
            if(!actual)refuse(BnwFreshGameError::parent_retired,"Player setup lost owning Game facade");
            player_setup->require_same_game(*actual);
        }
        if(pregame && pregame->actual_config()!=config.get())
            refuse(BnwFreshGameError::parent_retired,"Fresh Game lost SAME actual Pregame Config");
    }
    std::shared_ptr<BnwFreshGame> same_facade(const BnwFreshGame& actual)const {
        if(thread!=std::this_thread::get_id())refuse(BnwFreshGameError::wrong_thread,"Owned Pregame setup belongs to another Game thread");
        auto held=actual_facade.lock();
        if(!held || held.get()!=&actual || actual.impl_.get()!=this)
            refuse(BnwFreshGameError::parent_retired,"Owned Pregame setup requires SAME original Game facade/allocation");
        current();return held;
    }
    template<class T> void put(const char* name,T value) { scalars.insert_or_assign(name,BnwFreshGameScalar{std::move(value)});order.emplace_back(name); }
    void null(const char* name) { null_services.emplace(name);order.emplace_back(std::string("NULL ")+name); }
    void ranks() {
        for(const auto* name:{"m_aiEndTurnMessagesReceived","m_aiRankPlayer","m_aiPlayerRank","m_aiPlayerScore",
                             "m_aiRankTeam","m_aiTeamRank","m_aiTeamScore"}) {
            arrays[name].slots.emplace(64);order.emplace_back(std::string("allocate ")+name);
        }
    }
    void uninit(BnwFreshGame& game) {
        goody.reset();order.emplace_back("CvGoodyHuts::Uninit");
        barbarian_counter.reset();barbarian_spawned.reset();order.emplace_back("CvBarbarians::Uninit");
        for(const auto* name:{"m_paiUnitCreatedCount","m_paiUnitClassCreatedCount","m_paiBuildingClassCreatedCount",
            "m_paiProjectCreatedCount","m_paiVoteOutcome","m_aiSecretaryGeneralTimer","m_aiVoteTimer","m_aiDiploVote",
            "m_aiVotesCast","m_aiPreviousVotesCast","m_aiNumVotesForTeam","m_aiTeamCompetitionWinnersScratchPad",
            "m_pabSpecialUnitValid"}) { arrays[name].slots.reset();arrays[name].bool_slots.reset();order.emplace_back(std::string("delete ")+name); }
        victory.reset();order.emplace_back("delete m_ppaaiTeamVictoryRank");
        destroyed.clear();great_people.clear();selections.uninit();triggered.uninit();
        order.emplace_back("empty containers/RNG uninit/replay/extras/costs");
        // CvRandom::uninit has an empty body. No implicit draw/reset here.
        replay.clear();extras.clear();costs.clear();
        for(const auto* name:{"m_pDiploResponseQuery","m_pSettlerSiteEvaluator","m_pStartSiteEvaluator","m_pStartPositioner",
            "m_pGameReligions","m_pGameCulture","m_pGameLeagues","m_pGameTrade","m_pTacticalMap",
            "m_pAdvisorCounsel","m_pAdvisorRecommender"}) null(name);
        positioner.reset();settler.reset();start.reset();religions.reset();culture.reset();leagues.reset();trade.reset();tactical.reset();counsel.reset();recommender.reset();
        put("m_bForceEndingTurn",false);
        put("m_lastTurnAICivsProcessed",std::int32_t{-1});
        put("m_iEndTurnMessagesSent",std::int32_t{0});
        put("m_iElapsedGameTurns",std::int32_t{0});
        put("m_iStartTurn",std::int32_t{0});
        put("m_iWinningTurn",std::int32_t{0});
        put("m_iStartYear",std::int32_t{0});
        put("m_iEstimateEndTurn",std::int32_t{0});
        put("m_iDefaultEstimateEndTurn",std::int32_t{0});
        put("m_iTurnSlice",std::int32_t{0});
        put("m_iCutoffSlice",std::int32_t{0});
        put("m_iNumCities",std::int32_t{0});
        put("m_iTotalPopulation",std::int32_t{0});
        put("m_iNoNukesCount",std::int32_t{0});
        put("m_iNukesExploded",std::int32_t{0});
        put("m_iMaxPopulation",std::int32_t{0});
        put("m_iUnused1",std::int32_t{0});
        put("m_iUnused2",std::int32_t{0});
        put("m_iUnused3",std::int32_t{0});
        put("m_iInitPopulation",std::int32_t{0});
        put("m_iInitLand",std::int32_t{0});
        put("m_iInitTech",std::int32_t{0});
        put("m_iInitWonders",std::int32_t{0});
        put("m_iAIAutoPlay",std::int32_t{0});
        put("m_iTotalReligionTechCost",std::int32_t{0});
        put("m_iCachedWorldReligionTechProgress",std::int32_t{0});
        put("m_iUnitedNationsCountdown",std::int32_t{0});
        put("m_iNumVictoryVotesTallied",std::int32_t{0});
        put("m_iNumVictoryVotesExpected",std::int32_t{0});
        put("m_iVotesNeededForDiploVictory",std::int32_t{0});
        put("m_iMapScoreMod",std::int32_t{0});
        put("m_uiInitialTime",std::uint32_t{0});
        put("m_bScoreDirty",false);
        put("m_bCircumnavigated",false);
        put("m_bDebugMode",false);
        put("m_bDebugModeCache",false);
        put("m_bFOW",true);
        put("m_bFinalInitialized",false);
        put("m_eWaitDiploPlayer",std::int32_t{-1});
        put("m_bPbemTurnSent",false);
        put("m_bHotPbemBetweenTurns",false);
        put("m_bPlayerOptionsSent",false);
        put("m_bNukesValid",false);
        put("m_bEndGameTechResearched",false);
        put("m_bTunerEverConnected",false);
        put("m_bTutorialEverAttacked",false);
        put("m_bStaticTutorialActive",false);
        put("m_bEverRightClickMoved",false);
        put("m_bCombatWarned",false);
        put("m_bArchaeologyTriggered",false);
        put("m_eHandicap",std::int32_t{-1});
        put("m_ePausePlayer",std::int32_t{-1});
        put("m_eAIAutoPlayReturnPlayer",std::int32_t{-1});
        put("m_eBestLandUnit",std::int32_t{-1});
        put("m_eWinner",std::int32_t{-1});
        put("m_eVictory",std::int32_t{-1});
        put("m_eGameState",std::int32_t{0});
        put("m_eBestWondersPlayer",std::int32_t{-1});
        put("m_eBestPoliciesPlayer",std::int32_t{-1});
        put("m_eBestGreatPeoplePlayer",std::int32_t{-1});
        put("m_eReligionTech",std::int32_t{-1});
        put("m_eIndustrialRoute",std::int32_t{-1});
        put("m_strScriptData",std::string{});
        put("m_iEarliestBarbarianReleaseTurn",std::int32_t{0});
        // Native loops interleave four player arrays, then three team arrays.
        for(std::size_t i=0;i<64;++i)
            for(const auto* name:{"m_aiEndTurnMessagesReceived","m_aiRankPlayer","m_aiPlayerRank","m_aiPlayerScore"})
                arrays.at(name).slots->at(i)=0;
        for(std::size_t i=0;i<64;++i)
            for(const auto* name:{"m_aiRankTeam","m_aiTeamRank","m_aiTeamScore"}) arrays.at(name).slots->at(i)=0;
        put("m_iLastMouseoverUnitID",std::int32_t{0});
        game.city_manager_shutdown_from_impl(*this);order.emplace_back("CvCityManager::Shutdown SAME scoped pointer realm");
    }
    void reset_head(std::int32_t handicap,BnwFreshGame& game) {
        current();uninit(game);put("m_fCurrentTurnTimerPauseDelta",0.f);
        order.emplace_back("cache DeleteFile/Open/Create SimpleValues");
        cache->reset_database();++cache_resets;current();
        put("m_eHandicap",handicap);
    }
    void constructor(BnwFreshGame& game) {
        // Default CvRandom() resets once; CvRandom(false) does not.
        order.emplace_back("mapRand constructor reset");
        put("m_mapRand.m_bSynchronous",false);put("m_jonRand.m_bSynchronous",true);
        order.emplace_back("GameDeals constructor counter0/Init clear3");
        put("m_endTurnTimerSemaphore",std::int32_t{0});
        put("m_fCurrentTurnTimerPauseDelta",0.f);put("m_sentAutoMoves",false);
        put("m_bForceEndingTurn",false);null("m_pDiploResponseQuery");put("m_bFOW",true);
        put("m_bArchaeologyTriggered",false);put("m_lastTurnAICivsProcessed",std::int32_t{-1});
        ranks();
        for(const auto* name:{"m_paiUnitCreatedCount","m_paiUnitClassCreatedCount","m_paiBuildingClassCreatedCount",
            "m_paiProjectCreatedCount","m_paiVoteOutcome","m_aiSecretaryGeneralTimer","m_aiVoteTimer","m_aiDiploVote",
            "m_aiVotesCast","m_aiPreviousVotesCast","m_aiNumVotesForTeam","m_aiTeamCompetitionWinnersScratchPad",
            "m_pabSpecialUnitValid"}) {arrays.emplace(name,BnwFreshGameArray{});order.emplace_back(std::string("NULL ")+name);}
        null("m_ppaaiTeamVictoryRank");
        for(const auto* name:{"m_pSettlerSiteEvaluator","m_pStartSiteEvaluator","m_pStartPositioner","m_pGameReligions",
            "m_pGameCulture","m_pGameLeagues","m_pGameTrade","m_pTacticalMap","m_pAdvisorCounsel","m_pAdvisorRecommender"}) null(name);
        end_timer.restart();end_timer.stop();order.emplace_back("endTurnTimer Start/Stop");
        reset_head(-1,game);
        selections.remove_all();triggered.remove_all();order.emplace_back("vote containers RemoveAll");
        map_rng.reset();jon_rng.reset();order.emplace_back("mapRand reset/jonRand reset");
        put("m_iNumSessions",std::int32_t{1});advisor_messages.clear();
        game.city_manager_reset_from_impl(*this);order.emplace_back("CvCityManager::Reset SAME scoped pointer realm");
        current();boundary=BnwFreshGameBoundary::constructor_reset_complete;
    }
    std::vector<bool> info(std::string_view table) {
        current();std::vector<bool> present;
        const auto r=db->prefetch_info(std::string(table),[&](std::uint64_t slot,std::int64_t authored,const content::RulesRowView& row) {
            current();
            if(slot>=65536 || authored<0 || authored>std::numeric_limits<std::int32_t>::max() ||
               !row.has_column("ID") || row.integer("ID")!=authored)
                refuse(BnwFreshGameError::native_rules_unavailable,"Game native collection ID/span unavailable");
            present.resize(static_cast<std::size_t>(slot)+1,false);present[static_cast<std::size_t>(slot)]=true;
            return true; // Native Prefetch keeps nonnull entry even when CacheResults returns false.
        },content::InfoCollectionBudget{65536});
        current();
        if(!r.all_succeeded() || r.slots>65536 || r.slots!=present.size())
            refuse(BnwFreshGameError::native_rules_unavailable,"Game native collection unavailable");
        return present;
    }
    void allocation(const char* name,std::size_t count,std::int32_t value,const std::vector<bool>* present=nullptr) {
        auto& out=arrays[name].slots;out.emplace(count);order.emplace_back(std::string("allocate/write ")+name);
        for(std::size_t i=0;i<count;++i) if(!present || present->at(i)) out->at(i)=value;
    }
    std::int32_t awards() {
        current();std::int64_t n=0;
        const auto r=db->query_rows({"SELECT Value from Defines where Name = ? LIMIT 1",{"NUM_VICTORY_POINT_AWARDS"}},
            [&](const content::RulesRowView& row) {current();n=row.integer_at(0);},1);
        current();
        if(!r.all_succeeded() || n<0 || n>65536)
            refuse(BnwFreshGameError::native_rules_unavailable,"Game victory award cached Define unavailable");
        // GC constructor5 is overwritten by CacheGlobals.getDefineINT: missing=>0.
        return static_cast<std::int32_t>(n);
    }
    std::int32_t site_define(const char* name) {
        current();const auto result=db->global_define(name,content::GlobalDefineKind::integer);current();
        const auto value=std::get<std::int64_t>(result.value);
        if(!result.error.empty() || value<std::numeric_limits<std::int32_t>::min() ||
           value>std::numeric_limits<std::int32_t>::max())
            refuse(BnwFreshGameError::native_rules_unavailable,"SiteEvaluator cached native Define unavailable");
        // CacheGlobals overwrites constructor constants; absent row => GetInt0.
        return static_cast<std::int32_t>(value);
    }
    void site_init(SiteEvaluatorOwned& site,const std::string& label) {
        const auto site_put=[&](const char* field,std::int32_t value) {
            put((label+"."+field).c_str(),value);
        };
        const auto ring_store=[&](std::size_t slot,std::int32_t value) {
            site.ring.at(slot)=value;
            order.emplace_back(label+".m_iRingModifier["+std::to_string(slot)+"]");
        };
        ring_store(0,1);
        ring_store(1,site_define("CITY_RING_1_MULTIPLIER"));
        ring_store(2,site_define("CITY_RING_2_MULTIPLIER"));
        ring_store(3,site_define("CITY_RING_3_MULTIPLIER"));
        ring_store(4,site_define("CITY_RING_4_MULTIPLIER"));
        ring_store(5,site_define("CITY_RING_5_MULTIPLIER"));
        ring_store(6,0);ring_store(7,0);
        // Reconstruct the existing native global name map once at its FIRST
        // reached getter. Do not use Prefetch slots, flavor count or hash map.
        current();const auto types=db->cache_info_types();current();
        if(!types.report().all_succeeded())
            refuse(BnwFreshGameError::native_rules_unavailable,"SiteEvaluator native Info name map unavailable");
        const auto native_type=[&](const char* name) {
            current();const auto* entry=types.find(name);
            const auto id=entry?entry->id:std::int64_t{-1};
            if(id<std::numeric_limits<std::int32_t>::min() || id>std::numeric_limits<std::int32_t>::max())
                refuse(BnwFreshGameError::native_rules_unavailable,"SiteEvaluator native Info ID exceeds int32");
            return static_cast<std::int32_t>(id);
        };
        site.growth=native_type("FLAVOR_GROWTH");site_put("m_iGrowthIndex",site.growth);
        site.expansion=native_type("FLAVOR_EXPANSION");site_put("m_iExpansionIndex",site.expansion);
        site.naval=native_type("FLAVOR_NAVAL");site_put("m_iNavalIndex",*site.naval);
        site.brazil=1000;site_put("m_iBrazilMultiplier",*site.brazil);
        site.spain=55000;site_put("m_iSpainMultiplier",*site.spain);
        site.morroco=1000;site_put("m_iMorrocoMultiplier",*site.morroco);
        site.netherlands=2000;site_put("m_iNetherlandsMultiplier",*site.netherlands);
        site.inca=500;site_put("m_iIncaMultiplier",*site.inca);
    }
    void settler_init() {
        current();
        if(boundary!=BnwFreshGameBoundary::settler_site_init_required || !settler)
            refuse(BnwFreshGameError::wrong_phase,"SiteEvaluator Init requires actual nonconstructor reset prefix");
        if(settler_init_attempted)
            refuse(BnwFreshGameError::already_attempted,"SiteEvaluator Init already attempted");
        settler_init_attempted=true;boundary=BnwFreshGameBoundary::failed_prefix;
        order.emplace_back("CvCitySiteEvaluator::Init");
        site_init(*settler,"Settler");
        current();boundary=BnwFreshGameBoundary::start_site_construction_required;
        // Next native callee is new CvSiteEvaluatorForStart followed by Init.
        // No Start allocation, StartPositioner, Game service, map or RNG tail.
    }
    void start_init() {
        current();
        if(boundary!=BnwFreshGameBoundary::start_site_construction_required || !settler || start)
            refuse(BnwFreshGameError::wrong_phase,"Start Init requires actual completed Settler prefix");
        if(start_init_attempted)
            refuse(BnwFreshGameError::already_attempted,"Start Init already attempted");
        start_init_attempted=true;boundary=BnwFreshGameBoundary::failed_prefix;
        // Base constructor12/13, then empty native derived constructor.
        start=std::make_unique<SiteEvaluatorOwned>();null_services.erase("m_pStartSiteEvaluator");
        put("Start.m_iExpansionIndex",start->expansion);put("Start.m_iGrowthIndex",start->growth);
        order.emplace_back("CvSiteEvaluatorForStart constructor");
        order.emplace_back("CvCitySiteEvaluator::Init");site_init(*start,"Start");
        current();boundary=BnwFreshGameBoundary::start_positioner_construction_required;
        // Native Start does NOT override Init. ComputeFlavorMultipliers(NULL)
        // belongs to the next StartPositioner Init and remains unexecuted.
    }
    void positioner_init() {
        current();
        if(boundary!=BnwFreshGameBoundary::start_positioner_construction_required || !start || positioner)
            refuse(BnwFreshGameError::wrong_phase,"StartPositioner requires actual completed Start evaluator");
        if(positioner_init_attempted)
            refuse(BnwFreshGameError::already_attempted,"StartPositioner Init already attempted");
        positioner_init_attempted=true;boundary=BnwFreshGameBoundary::failed_prefix;
        positioner=std::make_unique<StartPositionerOwned>();null_services.erase("m_pStartPositioner");
        order.emplace_back("CvStartPositioner vector member construction empty3");
        put("StartPositioner.m_iRequiredSeparation",positioner->required_separation);
        put("StartPositioner.m_iBestFoundValueOnMap",positioner->best_found_on_map);
        order.emplace_back("CvStartPositioner constructor");
        // Source Init's pointer is the actual Start allocation created above;
        // it is not supplied by a caller or inferred from a progress boolean.
        positioner->evaluator=start.get();
        order.emplace_back("StartPositioner.m_pSiteEvaluator <- SAME owned Start");
        auto* evaluator=*positioner->evaluator;
        if(evaluator!=start.get())
            refuse(BnwFreshGameError::poisoned,"StartPositioner lost actual evaluator identity");
        order.emplace_back("CvSiteEvaluatorForStart::ComputeFlavorMultipliers(NULL)");
        for(std::size_t i=0;i<evaluator->flavor_multiplier.size();++i) {
            evaluator->flavor_multiplier[i]=1;
            order.emplace_back("Start.m_iFlavorMultiplier["+std::to_string(i)+"]");
        }
        current();boundary=BnwFreshGameBoundary::game_deals_init_required;
        // Next native caller is m_kGameDeals.Init (clear3), then new GameReligions.
        // This method executes no region/rank/fertility/map/player/RNG/Lua work.
    }
    void deals_religions_prefix() {
        current();
        if(boundary!=BnwFreshGameBoundary::game_deals_init_required || !positioner || religions)
            refuse(BnwFreshGameError::wrong_phase,"GameReligions prefix requires actual completed StartPositioner");
        if(religions_prefix_attempted)
            refuse(BnwFreshGameError::already_attempted,"GameReligions prefix already attempted");
        religions_prefix_attempted=true;boundary=BnwFreshGameBoundary::failed_prefix;
        // CvGameDeals::Init does NOT reset m_uiDealCounter or other vectors.
        deals.proposed.clear();order.emplace_back("GameDeals.m_ProposedDeals.clear");
        deals.current.clear();order.emplace_back("GameDeals.m_CurrentDeals.clear");
        deals.historical.clear();order.emplace_back("GameDeals.m_HistoricalDeals.clear");
        religions=std::make_unique<GameReligionsOwned>();null_services.erase("m_pGameReligions");
        order.emplace_back("GameReligions.m_CurrentReligions construct size0 capacity16");
        put("GameReligions.m_iMinimumFaithForNextPantheon.constructor",religions->minimum_faith);
        // First actual Init assignment, before GameSpeed/TrainPercent read.
        // CacheGlobals overwrites the constructor Define with GetInt: absent0.
        religions->minimum_faith=site_define("RELIGION_MIN_FAITH_FIRST_PANTHEON");
        put("GameReligions.m_iMinimumFaithForNextPantheon.InitDefine",religions->minimum_faith);
        current();boundary=BnwFreshGameBoundary::game_religions_speed_source_required;
        order.emplace_back("required: actual CvPreGame::gameSpeed for GameReligions multiply/divide");
        // CreationConfig describes selected setter calls but never executes
        // them. No metadata ID substitution, guessed TrainPercent, culture
        // allocation or post-reset RNG tail may cross this owner frontier.
    }
    void trade_reset(const char* phase) {
        trade->connection_count=0;order.emplace_back(std::string(phase)+".m_aTradeConnections.clear");
        trade->next_id=0;put((std::string(phase)+".m_iNextID").c_str(),*trade->next_id);
        trade->popup_x=0;put((std::string(phase)+".popup.iPlotX").c_str(),*trade->popup_x);
        trade->popup_y=0;put((std::string(phase)+".popup.iPlotY").c_str(),*trade->popup_y);
        trade->popup_type=0;put((std::string(phase)+".popup.type").c_str(),*trade->popup_type);
    }
    void game_services_init() {
        current();
        if(boundary!=BnwFreshGameBoundary::game_culture_construction_required || !religions || (!pregame && !civis_setup) || culture || leagues || trade)
            refuse(BnwFreshGameError::wrong_phase,"Game services require SAME completed religion Init");
        if(game_services_attempted)refuse(BnwFreshGameError::already_attempted,"Game services already attempted");
        game_services_attempted=true;boundary=BnwFreshGameBoundary::failed_prefix;
        culture=std::make_unique<GameCultureOwned>();null_services.erase("m_pGameCulture");
        culture->great_work_count=0;order.emplace_back("GameCulture.m_CurrentGreatWorks.clear");
        culture->reported_influential=false;put("GameCulture.m_bReportedSomeoneInfluential",*culture->reported_influential);
        current();
        leagues=std::make_unique<GameLeaguesOwned>();null_services.erase("m_pGameLeagues");
        leagues->active_count=0;order.emplace_back("GameLeagues.m_vActiveLeagues.clear");
        leagues->ever_founded=0;put("GameLeagues.m_iNumLeaguesEverFounded",*leagues->ever_founded);
        leagues->diplomatic_victor=-1;put("GameLeagues.m_eDiplomaticVictor",*leagues->diplomatic_victor);
        leagues->generated_id=0;put("GameLeagues.m_iGeneratedIDCount",*leagues->generated_id);
        leagues->last_era=-1;put("GameLeagues.m_eLastEraTrigger",*leagues->last_era);
        current();
        // Actual shipped CvGameLeagues::Init has an empty body. This call is
        // executed here; it grants no DoTurn/founding/world initialization.
        order.emplace_back("GameLeagues.Init empty native body");
        current();
        trade=std::make_unique<GameTradeOwned>();null_services.erase("m_pGameTrade");
        trade_reset("GameTrade.constructor.Reset");current();
        trade_reset("GameTrade.Init.Reset");current();
        boundary=BnwFreshGameBoundary::tactical_map_construction_required;
        // Actual next caller new CvTacticalAnalysisMap, before Advisor objects
        // and reset tail. No tactical/map/world/cache/RNG tail completion.
    }
    void religions_speed_init(std::filesystem::path selected_config_ini) {
        current();
        if(boundary!=BnwFreshGameBoundary::game_religions_speed_source_required || !religions || pregame)
            refuse(BnwFreshGameError::wrong_phase,"Religion numeric suffix requires actual first Init store");
        if(religions_speed_attempted)refuse(BnwFreshGameError::already_attempted,"Religion numeric suffix already attempted");
        religions_speed_attempted=true;boundary=BnwFreshGameBoundary::failed_prefix;
        // New owned Pregame slice executes real setters; never pass metadata
        // IDs or public observation values to the native GameSpeed getter.
        pregame=BnwFreshPregameSpeed::from_creation_config(config,std::move(selected_config_ini));
        pregame->advance_selected_speed();current();
        const auto train=pregame->current_train_percent();current();
        const auto product=static_cast<std::int64_t>(religions->minimum_faith)*train;
        if(product<std::numeric_limits<std::int32_t>::min() || product>std::numeric_limits<std::int32_t>::max())
            refuse(BnwFreshGameError::native_rules_unavailable,"GameReligions native int32 multiply overflow");
        religions->minimum_faith=static_cast<std::int32_t>(product);
        put("GameReligions.m_iMinimumFaithForNextPantheon.multiply",religions->minimum_faith);
        religions->minimum_faith/=100;
        put("GameReligions.m_iMinimumFaithForNextPantheon.divide100",religions->minimum_faith);
        current();boundary=BnwFreshGameBoundary::game_culture_construction_required;
        // Mandatory next caller is actual CvGameCulture constructor.
    }
    void nonconstructor(BnwFreshGame& game) {
        current();reset_head(config->metadata().info[5].id,game);
        auto slots=info("Units");allocation("m_paiUnitCreatedCount",slots.size(),0,&slots);
        slots=info("UnitClasses");allocation("m_paiUnitClassCreatedCount",slots.size(),0,&slots);
        slots=info("BuildingClasses");allocation("m_paiBuildingClassCreatedCount",slots.size(),0,&slots);
        slots=info("Projects");allocation("m_paiProjectCreatedCount",slots.size(),0);
        slots=info("Votes");allocation("m_paiVoteOutcome",slots.size(),-1);
        slots=info("VoteSources");allocation("m_aiDiploVote",slots.size(),0);
        allocation("m_aiVotesCast",63,-1);allocation("m_aiPreviousVotesCast",63,-1);
        allocation("m_aiNumVotesForTeam",63,0);allocation("m_aiTeamCompetitionWinnersScratchPad",63,0);
        slots=info("SpecialUnits");arrays["m_pabSpecialUnitValid"].bool_slots.emplace(slots.size(),false);
        order.emplace_back("allocate/write m_pabSpecialUnitValid false");
        slots=info("VoteSources");
        arrays["m_aiSecretaryGeneralTimer"].slots.emplace(slots.size());
        arrays["m_aiVoteTimer"].slots.emplace(slots.size());
        for(std::size_t i=0;i<slots.size();++i) {
            arrays.at("m_aiSecretaryGeneralTimer").slots->at(i)=0;
            arrays.at("m_aiVoteTimer").slots->at(i)=0;
        }
        order.emplace_back("SecretaryGeneralTimer/VoteTimer allocate/interleaved0");
        slots=info("Victories");victory.emplace(slots.size());null_services.erase("m_ppaaiTeamVictoryRank");
        order.emplace_back("allocate victory rank rows");
        // Getter is reached once PER row. Empty Victories does not read Define.
        std::size_t total_awards=0;
        for(auto& row:*victory) {
            const auto width=awards();
            if(static_cast<std::size_t>(width)>1048576-total_awards)
                refuse(BnwFreshGameError::native_rules_unavailable,"Game victory matrix allocation exceeds host budget");
            row.assign(static_cast<std::size_t>(width),-1);total_awards+=static_cast<std::size_t>(width);
        }
        settler=std::make_unique<SiteEvaluatorOwned>();null_services.erase("m_pSettlerSiteEvaluator");
        put("Settler.m_iExpansionIndex",settler->expansion);put("Settler.m_iGrowthIndex",settler->growth);
        order.emplace_back("CvSiteEvaluatorForSettler constructor");
        current();boundary=BnwFreshGameBoundary::settler_site_init_required;
        // Mandatory CvCitySiteEvaluator::Init is NOT executed. Its ring/flavor
        // getters, all later services and nonconstructor reset tail are unreached.
    }
};
BnwFreshGameRefusal::BnwFreshGameRefusal(BnwFreshGameError c,const std::string& text):std::runtime_error(text),code_(c) {}
BnwFreshGame::BnwFreshGame(std::shared_ptr<Impl> p):impl_(std::move(p)) {}
std::shared_ptr<BnwFreshGame> BnwFreshGame::from_creation_config(std::shared_ptr<const BnwCreationConfig> c) {
    if(!c) refuse(BnwFreshGameError::parent_missing,"Fresh Game requires SAME CreationConfig");
    c->require_current(); // Guard before any source, filesystem, clock or cache effect.
    auto p=std::make_shared<Impl>();p->config=std::move(c);p->db=&p->config->game_rules();
    auto game=std::shared_ptr<BnwFreshGame>(new BnwFreshGame(p));
    p->actual_facade=game;p->current();
    // Actual private Game friend issues one isolated realm BEFORE constructor.
    // Fresh owned cities will later bind this realm; imported caches never do.
    p->random_logging=std::make_unique<FreshGlobalsRandomLogging>(p->config);
    p->city_manager.reset(new BnwFreshCityManager(p->config));
    p->order.emplace_back("Game scoped CityManager pointer realm allocated");
    p->cache=std::make_unique<LocalSessionCache>();p->constructor(*game);
    return game;
}

void BnwFreshGame::initialize_civis_single_player_setup(BnwNewGameSetupRequest request) {
    const auto p=impl_;const auto held=p->same_facade(*this);(void)held;
    if(p->civis_setup || p->early_pregame_attempted || p->ordered_globals_attempted || p->attempted ||
       p->boundary!=BnwFreshGameBoundary::constructor_reset_complete)
        refuse(BnwFreshGameError::wrong_phase,"Civis setup requires the SAME unused fresh Game constructor return");
    auto rules=BnwNewGameSetupRules::from_creation_config(p->config);
    rules->require_same_config(*p->config);p->same_facade(*this);
    if(request.max_turns<0 || request.map_source.relative_path.empty() ||
       request.map_source.relative_path.find('\0')!=std::string::npos)
        refuse(BnwFreshGameError::native_rules_unavailable,"Civis setup requires explicit nonnegative turns and an exact map source");
    for(const auto& [id,value]:request.map_custom_options)
        if(id<1 || value<1)refuse(BnwFreshGameError::native_rules_unavailable,"BNW map custom choices must retain actual positive script OptionID/value");
    // Read the activated sparse catalog, not a caller's observation or an
    // initialized Globals grant. Only required gameplay init operands are read.
    std::map<std::string,bool,std::less<>> options;
    const auto option_rows=p->db->prefetch_info("GameOptions",[&](std::uint64_t slot,std::int64_t id,const content::RulesRowView& row){
        p->current();if(slot>65535 || id!=static_cast<std::int64_t>(slot) || !row.has_column("ID") || row.integer("ID")!=id ||
            !row.has_column("Type") || !row.has_column("Default"))
            refuse(BnwFreshGameError::native_rules_unavailable,"BNW GameOptions source schema/slot unavailable");
        const auto type=row.text("Type");if(!type || type->empty() || type->find('\0')!=std::string::npos ||
            !options.emplace(*type,row.boolean("Default")).second)
            refuse(BnwFreshGameError::native_rules_unavailable,"BNW GameOptions source Type duplicate/unavailable");
        return true;
    },content::InfoCollectionBudget{65536});
    if(!option_rows.all_succeeded())refuse(BnwFreshGameError::native_rules_unavailable,"BNW GameOptions source failed");
    for(const auto* name:{"GAMEOPTION_SIMULTANEOUS_TURNS","GAMEOPTION_DYNAMIC_TURNS","GAMEOPTION_PITBOSS", "GAMEOPTION_LOCK_MODS","GAMEOPTION_QUICK_COMBAT"})
        if(!options.contains(name))refuse(BnwFreshGameError::native_rules_unavailable,"BNW mandatory initialized option source missing");
    for(const auto& [type,value]:request.game_options){
        const auto found=options.find(type);if(found==options.end())refuse(BnwFreshGameError::native_rules_unavailable,"Unknown selected BNW game option");
        found->second=value;
    }
    if(rules->rules_.no_religion && !options.contains("GAMEOPTION_NO_RELIGION"))
        refuse(BnwFreshGameError::native_rules_unavailable,"Selected NoReligion era lacks actual BNW option source");
    std::vector<bool> victories(rules->rules_.victories.size(),true);
    for(const auto& type:request.disabled_victories){
        bool found=false;for(std::size_t slot=0;slot<rules->rules_.victories.size();++slot){
            const auto& rule=rules->rules_.victories[slot];if(rule && rule->identity.type==type){victories[slot]=false;found=true;}
        }
        if(!found)refuse(BnwFreshGameError::native_rules_unavailable,"Unknown selected BNW victory");
    }
    // Retain genuine sources before the first reset effect. Any partial effect
    // stays owned and observable; re-entry cannot replay RNG or reset stores.
    p->civis_setup=std::make_unique<Impl::CivisSetup>(std::move(rules),std::move(request));
    auto& setup=*p->civis_setup;auto& state=setup.state;
    state.selected=setup.rules->rules_.selected;state.map_source=setup.request.map_source;
    state.map_custom_options=setup.request.map_custom_options;state.game_options=std::move(options);
    state.victory_valid=std::move(victories);state.max_turns=setup.request.max_turns;
    p->boundary=BnwFreshGameBoundary::failed_prefix;
    try {
        // Civis stores the chosen IDs on the actual Game, then reuses its
        // existing complete Reset pieces in original order. This does not
        // manufacture an original host Pregame/settings/Globals occurrence.
        for(const auto& info:state.selected.info)p->put(("Civis.Selected."+info.table).c_str(),info.id);
        p->attempted=true;p->nonconstructor(*this);p->settler_init();p->start_init();p->positioner_init();p->deals_religions_prefix();
        const auto& speed=state.selected.info[3];std::int64_t train=0;
        const auto speed_row=p->db->query_rows({"SELECT TrainPercent FROM GameSpeeds WHERE ID = ? AND Type = ? LIMIT 1",{static_cast<std::int64_t>(speed.id),speed.type}},
            [&](const content::RulesRowView& row){p->current();if(!row.has_column("TrainPercent")){
                refuse(BnwFreshGameError::native_rules_unavailable,"Selected actual GameSpeed TrainPercent missing");}
                train=row.integer("TrainPercent");},1);
        if(!speed_row.all_succeeded() || speed_row.rows!=1 || train<std::numeric_limits<std::int32_t>::min() || train>std::numeric_limits<std::int32_t>::max())
            refuse(BnwFreshGameError::native_rules_unavailable,"Selected actual GameSpeed religion source failed");
        const auto product=static_cast<std::int64_t>(p->religions->minimum_faith)*train;
        if(product<std::numeric_limits<std::int32_t>::min() || product>std::numeric_limits<std::int32_t>::max())
            refuse(BnwFreshGameError::native_rules_unavailable,"BNW religion reset arithmetic overflow");
        p->religions->minimum_faith=static_cast<std::int32_t>(product);
        p->put("GameReligions.m_iMinimumFaithForNextPantheon.multiply",p->religions->minimum_faith);
        p->religions->minimum_faith/=100;p->put("GameReligions.m_iMinimumFaithForNextPantheon.divide100",p->religions->minimum_faith);
        p->boundary=BnwFreshGameBoundary::game_culture_construction_required;p->game_services_init();
        advance_tactical_advisors_reset_tail();advance_city_manager_reset();
        p->selections.init();p->triggered.init();p->order.emplace_back("Civis init vote selections/triggered Init8");
        p->put("m_jonRand.callStackDebuggingEnabled",false);
        p->map_rng.reset(setup.request.map_seed%73637381u);p->jon_rng.reset(setup.request.sync_seed%52319761u);
        p->order.emplace_back("Civis init actual map/jon RNG SDK seed residues");
        // Genuine selected Era source was verified by Rules30; the invalid-era
        // fallback is unreachable for this accepted configuration.
        for(const auto* name:{"GAMEOPTION_SIMULTANEOUS_TURNS","GAMEOPTION_DYNAMIC_TURNS","GAMEOPTION_PITBOSS"})state.game_options.at(name)=false;
        for(std::size_t index=0;index<4;++index)p->put(("Civis.MPOption."+std::to_string(index)).c_str(),false);
        p->order.emplace_back("Civis SINGLE_PLAYER clear MP4 and turn options; shuffle=false");
        if(state.game_options.at("GAMEOPTION_LOCK_MODS")){
            std::array<char,8> password{};
            for(std::size_t i=0;i<7;++i)password[i]=static_cast<char>(p->jon_rng.draw(128));
            state.admin_password=std::string(password.data());p->order.emplace_back("Civis LOCK_MODS actual seven Random Keyword draws");
        }
        const auto signed32=[](std::int64_t value){
            if(value<std::numeric_limits<std::int32_t>::min() || value>std::numeric_limits<std::int32_t>::max()){
                refuse(BnwFreshGameError::native_rules_unavailable,"BNW setup signed clock arithmetic overflow");}
            return static_cast<std::int32_t>(value);};
        std::int32_t total=0;for(const auto& increment:setup.rules->rules_.turns)total=signed32(static_cast<std::int64_t>(total)+increment.turns);
        const auto start=signed32(static_cast<std::int64_t>(total)*setup.rules->rules_.start_percent)/100;
        // CvGame::setGameTurn only runs when old!=new. Preserve the original
        // first mandatory publication BEFORE storing a nonzero new turn.
        if(state.game_turn!=start){
            state.pending_game_turn=start;state.boundary=BnwNewGameSetupBoundary::turn_publication_required;
            state.mandatory={"Actual initial setGameTurn debug publication, dirty bits, GameplayTurnChanged and endTurnTimerReset before advancing init"};
            p->boundary=BnwFreshGameBoundary::civis_turn_publication_required;
            refuse(BnwFreshGameError::native_rules_unavailable,state.mandatory.front());
        }
        if(setup.rules->rules_.no_religion)state.game_options.at("GAMEOPTION_NO_RELIGION")=true;
        state.start_turn=state.game_turn;p->put("m_iStartTurn",*state.start_turn);
        state.default_end_turn=total;p->put("m_iDefaultEstimateEndTurn",total);
        if(state.max_turns==0){
            state.estimated_end_turn=total;
            if(total>state.game_turn){for(std::size_t i=0;i<setup.rules->rules_.victories.size();++i){
                const auto& victory=setup.rules->rules_.victories[i];
                if(victory && state.victory_valid[i] && victory->end_score){state.max_turns=signed32(static_cast<std::int64_t>(total)-state.game_turn);break;}
            }}
        }else state.estimated_end_turn=signed32(static_cast<std::int64_t>(state.game_turn)+state.max_turns);
        p->put("m_iEstimateEndTurn",*state.estimated_end_turn);
        state.start_year=setup.rules->rules_.start_year;p->put("m_iStartYear",*state.start_year);
        const auto specials=p->db->prefetch_info("SpecialUnits",[&](std::uint64_t slot,std::int64_t authored,const content::RulesRowView& row){
            p->current();const auto& allocated=p->arrays.at("m_pabSpecialUnitValid").bool_slots;
            if(!allocated || slot>=allocated->size() || authored!=static_cast<std::int64_t>(slot) || !row.has_column("Valid"))
                refuse(BnwFreshGameError::native_rules_unavailable,"BNW SpecialUnit validity source unavailable");
            if(row.boolean("Valid"))p->arrays.at("m_pabSpecialUnitValid").bool_slots->at(static_cast<std::size_t>(slot))=true;
            return true;
        },content::InfoCollectionBudget{65536});
        if(!specials.all_succeeded() || specials.slots!=p->arrays.at("m_pabSpecialUnitValid").bool_slots->size())
            refuse(BnwFreshGameError::native_rules_unavailable,"BNW SpecialUnit initialization span changed");
        state.quick_combat=state.game_options.at("GAMEOPTION_QUICK_COMBAT");
        p->put("m_bArchaeologyTriggered",false);
        p->goody=std::make_unique<std::vector<std::array<std::int32_t,3>>>(22,std::array<std::int32_t,3>{-1,-1,-1});
        p->order.emplace_back("Civis CvGoodyHuts.Reset actual22x3 all -1");
        // The qualified original SDK doUpdateCacheOnTurn body is EMPTY.
        p->order.emplace_back("Civis doUpdateCacheOnTurn original empty body");
        p->same_facade(*this);state.boundary=BnwNewGameSetupBoundary::players_init_required;
        state.mandatory={"Actual same-game CvGame::InitPlayers team/player/AI initialization", "Selected original-order InitMap and initial items/diplomacy/archaeology/score", "Live session publication after final initialization"};
        p->boundary=BnwFreshGameBoundary::civis_player_init_required;
    }catch(...){
        if(state.boundary!=BnwNewGameSetupBoundary::turn_publication_required){state.boundary=BnwNewGameSetupBoundary::failed_prefix;p->boundary=BnwFreshGameBoundary::failed_prefix;}
        throw;
    }
}
BnwNewGameSetupObservation BnwFreshGame::civis_setup_observation()const {
    const auto p=impl_;const auto held=p->same_facade(*this);(void)held;
    if(!p->civis_setup)refuse(BnwFreshGameError::wrong_phase,"Civis setup has not been issued on SAME Game");
    auto result=p->civis_setup->state;result.map_random=p->map_rng.state();result.gameplay_random=p->jon_rng.state();
    if(p->goody){result.goody_memory=*p->goody;}
    p->same_facade(*this);return result;
}

void BnwFreshGame::initialize_civis_team_roster_prefix(BnwNewGamePlayerSetupRequest request) {
    const auto p=impl_;const auto held=p->same_facade(*this);(void)held;
    if(!p->civis_setup || p->player_setup || p->boundary!=BnwFreshGameBoundary::civis_player_init_required ||
       p->civis_setup->state.boundary!=BnwNewGameSetupBoundary::players_init_required)
        refuse(BnwFreshGameError::wrong_phase,"InitPlayers requires SAME completed actual Civis Game init prefix");
    p->civis_setup->rules->require_same_config(*p->config);
    p->player_setup.reset(new BnwNewGamePlayerSetup(*this,p->civis_setup->rules,*p->db,std::move(request)));
    p->boundary=BnwFreshGameBoundary::failed_prefix;
    p->player_setup->advance_team_roster_prefix(*this);
    p->same_facade(*this);p->boundary=BnwFreshGameBoundary::civis_player_reset_required;
    p->civis_setup->state.mandatory={"Actual remaining same-game CvPlayer::init Reset/containers/setAlive/traits/AI for every native player", "Selected original-order map/initial items and final initialized live session"};
}
void BnwFreshGame::advance_civis_first_player_reset_prefix() {
    const auto p=impl_;const auto held=p->same_facade(*this);(void)held;
    if(!p->civis_setup || !p->player_setup || p->boundary!=BnwFreshGameBoundary::civis_player_reset_required)
        refuse(BnwFreshGameError::wrong_phase,"Actual first player Reset requires SAME owned team/roster normal return");
    p->civis_setup->rules->require_same_config(*p->config);
    p->boundary=BnwFreshGameBoundary::failed_prefix;
    if(!p->tactical)refuse(BnwFreshGameError::native_rules_unavailable,"Actual owning Game Tactical pointer source unavailable");
    p->tactical->require_current();const auto handicap=std::get<std::int32_t>(p->scalars.at("m_eHandicap"));
    p->player_setup->advance_first_player_reset_prefix(*this,p->civis_setup->rules->content_,*p->tactical,handicap);
    p->same_facade(*this);p->boundary=BnwFreshGameBoundary::civis_first_player_homeland_required;
    p->civis_setup->state.mandatory={"Actual next HomelandAI Init and remaining first Player Reset", "Remaining player Init/setAlive/traits/personality/AI and native64 roster before map publication"};
}
BnwNewGamePlayerSetupObservation BnwFreshGame::civis_player_setup_observation()const {
    const auto p=impl_;const auto held=p->same_facade(*this);(void)held;
    if(!p->player_setup)refuse(BnwFreshGameError::wrong_phase,"SAME Game team/player prefix has not been issued");
    auto result=p->player_setup->observe_from_game(*this);p->same_facade(*this);return result;
}

void BnwFreshGame::advance_single_player_pregame_menu_prefix(std::filesystem::path selected_config_ini) {
    impl_->current();if(impl_->civis_setup)refuse(BnwFreshGameError::wrong_phase,"Civis setup cannot enter the separate legacy initialization route");
    const auto allocation=impl_;const auto facade=allocation->same_facade(*this);(void)facade;
    if(allocation->boundary!=BnwFreshGameBoundary::constructor_reset_complete || allocation->attempted ||
       allocation->pregame || allocation->early_pregame_attempted)
        refuse(BnwFreshGameError::wrong_phase,"Owned Pregame menu setup must precede SAME Game nonconstructor Reset once");
    const auto genuine_input=allocation->config->borrow_selected_language_input();allocation->same_facade(*this);
    if(genuine_input)
        refuse(BnwFreshGameError::native_rules_unavailable,
            "Actual Boot-affiliated Game menu requires genuine complete ordered Globals initialization; use ordered source first");
    // The older constructor-only Config route remains a diagnostic component;
    // it cannot issue Boot settings or an initialized Globals/new-game receipt.
    allocation->early_pregame_attempted=true;allocation->boundary=BnwFreshGameBoundary::failed_prefix;
    // This is the ONE actual Pregame realm retained by Game. No supplied
    // independently constructed Pregame/observation/ready flag is accepted.
    allocation->pregame=BnwFreshPregameSpeed::from_creation_config(allocation->config,std::move(selected_config_ini));
    allocation->same_facade(*this);
    allocation->order.emplace_back("Game owned actual Pregame namespace allocated before menu setup");
    try {allocation->pregame->advance_single_player_menu_prefix();}
    catch(const BnwFreshPregameSpeedRefusal& e) {
        allocation->same_facade(*this);const auto actual=allocation->pregame->observe();allocation->same_facade(*this);
        if(e.code()==BnwFreshPregameSpeedError::native_source_unavailable &&
           actual.menu.boundary==BnwFreshPregameMenuBoundary::reset_game_options_required) {
            allocation->order.emplace_back("Game SAME Pregame menu ResetSlots returned; options required");
            allocation->boundary=BnwFreshGameBoundary::pregame_menu_options_required;
            // The mandatory frontier is reported to the caller, never a
            // successful continuation or a completed new-game setup receipt.
            refuse(BnwFreshGameError::native_rules_unavailable,e.what());
        }
        throw;
    }
    refuse(BnwFreshGameError::native_rules_unavailable,"Owned Pregame source returned without its mandatory options frontier");
}
void BnwFreshGame::advance_single_player_pregame_menu_options_prefix() {
    const auto allocation=impl_;const auto facade=allocation->same_facade(*this);(void)facade;
    if(!allocation->early_pregame_attempted || !allocation->pregame ||
       allocation->boundary!=BnwFreshGameBoundary::pregame_menu_options_required)
        refuse(BnwFreshGameError::wrong_phase,"Owned Pregame options require SAME actual menu ResetSlots return");
    allocation->boundary=BnwFreshGameBoundary::failed_prefix;
    try {allocation->pregame->advance_single_player_menu_options_prefix();}
    catch(const BnwFreshPregameSpeedRefusal& e) {
        allocation->same_facade(*this);const auto actual=allocation->pregame->observe();allocation->same_facade(*this);
        if(e.code()==BnwFreshPregameSpeedError::native_source_unavailable &&
           actual.menu.boundary==BnwFreshPregameMenuBoundary::load_pregame_settings_required) {
            allocation->order.emplace_back("Game SAME Pregame options/map options returned; LoadPreGameSettings required");
            allocation->boundary=BnwFreshGameBoundary::pregame_settings_source_required;
            refuse(BnwFreshGameError::native_rules_unavailable,e.what());
        }
        throw;
    }
    refuse(BnwFreshGameError::native_rules_unavailable,"Owned Pregame source returned without its mandatory settings frontier");
}
BnwFreshPregameSpeedObservation BnwFreshGame::pregame_setup_observation()const {
    const auto allocation=impl_;const auto facade=allocation->same_facade(*this);(void)facade;
    if(!allocation->early_pregame_attempted || !allocation->pregame)
        refuse(BnwFreshGameError::wrong_phase,"No early SAME Game-owned Pregame setup was issued");
    auto value=allocation->pregame->observe();allocation->same_facade(*this);return value;
}


void BnwFreshGame::advance_pregame_settings_last_civilization_prefix(
    std::shared_ptr<const BnwSelectedUserOptionsOwner> selected_options) {
    const auto allocation=impl_;const auto facade=allocation->same_facade(*this);(void)facade;
    if(!allocation->early_pregame_attempted || !allocation->pregame ||
       allocation->boundary!=BnwFreshGameBoundary::pregame_settings_source_required || allocation->settings_prefix_attempted)
        refuse(BnwFreshGameError::wrong_phase,"Actual LoadPreGameSettings requires SAME owned menu options return once");
    if(!selected_options)refuse(BnwFreshGameError::parent_missing,"Actual LoadPreGameSettings lacks Boot-issued user registry");
    const auto input=allocation->config->borrow_selected_language_input();allocation->same_facade(*this);
    if(!input)refuse(BnwFreshGameError::native_rules_unavailable,"Constructor metadata Config cannot issue actual Boot settings input");
    // Exact factory Boot affiliation, not path equality or copied receipts.
    selected_options->require_same_game_input(*input);allocation->same_facade(*this);
    allocation->pregame_user_options=std::move(selected_options);
    allocation->settings_prefix_attempted=true;allocation->boundary=BnwFreshGameBoundary::failed_prefix;
    const auto last=allocation->pregame_user_options->last_civilization();allocation->same_facade(*this);
    allocation->order.emplace_back("LoadPreGameSettings native UserSettings.LastCiv read");
    allocation->put("LoadPreGameSettings.LastCiv",last);
    if(last>=0) {
        // Host6AF9A9 native short circuit next reaches true GameContext's
        // GetNumCivilizationInfos, then GetCivilizationInfo/IsPlayable before
        // setCivilization. No SQL row count or incomplete cache impersonates it.
        allocation->boundary=BnwFreshGameBoundary::pregame_civilization_info_required;
        refuse(BnwFreshGameError::native_rules_unavailable,"LoadPreGameSettings reached mandatory actual CivilizationInfo collection/IsPlayable/setCivilization source");
    }
    // Genuine negative saved/default value takes original JLT branch: no
    // CivilizationInfo access and no player setter is reached in this branch.
    allocation->order.emplace_back("LoadPreGameSettings actual negative LastCiv branch; no civilization setter reached");
    allocation->boundary=BnwFreshGameBoundary::pregame_last_speed_required;
    refuse(BnwFreshGameError::native_rules_unavailable,"LoadPreGameSettings requires next actual UserSettings.LastSpeed read and native GameSpeed setter");
}


void BnwFreshGame::advance_ordered_globals_info_prefix(std::filesystem::path selected_config_ini) {
    impl_->current();if(impl_->civis_setup)refuse(BnwFreshGameError::wrong_phase,"Civis setup cannot enter the separate legacy initialization route");
 const auto allocation=impl_;const auto facade=allocation->same_facade(*this);(void)facade;
 if(allocation->boundary!=BnwFreshGameBoundary::constructor_reset_complete || allocation->attempted ||
    allocation->pregame || allocation->early_pregame_attempted || allocation->ordered_globals_attempted)
  refuse(BnwFreshGameError::wrong_phase,"Ordered Globals initialization requires pristine SAME Game once before menu");
 const auto input=allocation->config->borrow_selected_language_input();allocation->same_facade(*this);
 if(!input)refuse(BnwFreshGameError::native_rules_unavailable,"Ordered native Globals route requires genuine retained Boot input");
 allocation->ordered_globals_attempted=true;allocation->early_pregame_attempted=true;allocation->boundary=BnwFreshGameBoundary::failed_prefix;
 allocation->pregame=BnwFreshPregameSpeed::from_creation_config(allocation->config,std::move(selected_config_ini));allocation->same_facade(*this);
 allocation->order.emplace_back("SAME Game-owned actual Pregame ordered Globals before menu");
 try{allocation->pregame->advance_ordered_globals_info_prefix_from_game();}
 catch(...){
  const auto original_failure=std::current_exception();
  allocation->same_facade(*this);
  // Child observation is a diagnostic only. A failed constructor/source does
  // not manufacture a successful predecessor or ready continuation.
  try{const auto observed=allocation->pregame->ordered_info_observation_from_game();
   allocation->same_facade(*this);
   if(observed.completed_tables==std::vector<std::string>{"Colors"}&&observed.mandatory_next=="PlayerColors")
    allocation->boundary=BnwFreshGameBoundary::ordered_globals_player_colors_required;
  }catch(...){std::rethrow_exception(original_failure);}
  std::rethrow_exception(original_failure);
 }
 refuse(BnwFreshGameError::native_rules_unavailable,"Ordered Globals source returned without mandatory full-cache completion");
}
BnwFreshOrderedInfoObservation BnwFreshGame::ordered_globals_info_observation()const {
 const auto allocation=impl_;const auto facade=allocation->same_facade(*this);(void)facade;
 if(!allocation->ordered_globals_attempted || !allocation->pregame)refuse(BnwFreshGameError::wrong_phase,"Actual ordered Globals route not issued");
 auto out=allocation->pregame->ordered_info_observation_from_game();allocation->same_facade(*this);return out;
}

void BnwFreshGame::require_current() const {impl_->current();}
std::vector<std::string> BnwFreshGame::pregame_order() const {
    impl_->current();if(!impl_->pregame)refuse(BnwFreshGameError::wrong_phase,"Actual Pregame not yet issued");
    return impl_->pregame->observe().order;
}
void BnwFreshGame::city_manager_reset_from_impl(const Impl& owner) {
    impl_->current();
    if(&owner!=impl_.get() || !impl_->city_manager)
        refuse(BnwFreshGameError::parent_missing,"CityManager reset requires exact owning Game Impl");
    impl_->city_manager->reset();++impl_->scoped_city_manager_resets;
}
void BnwFreshGame::city_manager_shutdown_from_impl(const Impl& owner) {
    impl_->current();
    if(&owner!=impl_.get() || !impl_->city_manager)
        refuse(BnwFreshGameError::parent_missing,"CityManager shutdown requires exact owning Game Impl");
    impl_->city_manager->shutdown();++impl_->scoped_city_manager_shutdowns;
}
std::shared_ptr<const BnwFreshCityManager> BnwFreshGame::city_manager() const {
    impl_->current();if(!impl_->city_manager)refuse(BnwFreshGameError::parent_missing,"Game lost its scoped CityManager");
    impl_->city_manager->require_current();
    return std::shared_ptr<const BnwFreshCityManager>(impl_,impl_->city_manager.get());
}
void BnwFreshGame::advance_init_prefix(BnwFreshPregameInitRequest request) {
    impl_->current();
    if(impl_->boundary!=BnwFreshGameBoundary::scoped_reset_complete_owned_cities_required || !impl_->pregame)
        refuse(BnwFreshGameError::wrong_phase,"Game Init requires SAME actual scoped reset return and Pregame");
    if(impl_->init_prefix_attempted)refuse(BnwFreshGameError::already_attempted,"Game Init prefix already attempted");
    impl_->init_prefix_attempted=true;impl_->boundary=BnwFreshGameBoundary::failed_prefix;
    // Setter request is input preparation, not part of init's completed reset.
    // No caller seed/type observation is accepted as an executed owner receipt.
    impl_->pregame->advance_selected_init_inputs(request);impl_->current();
    impl_->order.emplace_back("Init prerequisite SAME actual fresh Pregame setters complete");
    impl_->selections.init();impl_->order.emplace_back("init m_voteSelections.Init8");
    impl_->triggered.init();impl_->order.emplace_back("init m_votesTriggered.Init8");
    const auto actual=impl_->pregame->current_init_inputs();impl_->current();
    // isNetworkMultiPlayer || isPbem || isHotSeat in native short-circuit order.
    const bool multiplayer=actual.game_type==1 || actual.game_type==3 || actual.game_type==2;
    if(!multiplayer) {
        // CvRandom::setCallStackDebuggingEnabled body is _DEBUG-only.
        // Bounded Release/UBSan source profile: genuine native call, empty body;
        // m_bSynchronous remains the constructor's true value, not overwritten.
#ifdef _DEBUG
        refuse(BnwFreshGameError::native_rules_unavailable,"Debug call-stack instrumentation owner is unavailable");
#else
        impl_->order.emplace_back("init jonRand.setCallStackDebuggingEnabled(false) Release empty body");
#endif
    }
    impl_->map_rng.reset(static_cast<std::uint64_t>(actual.map_seed%73637381u));
    impl_->order.emplace_back("init m_mapRand.init(seed % 73637381)");
    impl_->jon_rng.reset(static_cast<std::uint64_t>(actual.sync_seed%52319761u));
    impl_->order.emplace_back("init m_jonRand.init(seed % 52319761)");
    impl_->pregame->verify_start_era_from_game();impl_->current();
    impl_->order.emplace_back("init actual Pregame era verification complete");
    impl_->boundary=BnwFreshGameBoundary::init_mp_options_roster_required;
    // Native next branches require true MP/options stores then player roster.
    // No completed options/ShuffleTeams/eraPercent/map/full-Init grant.
}
void BnwFreshGame::advance_single_player_options() {
    impl_->current();
    if(impl_->boundary!=BnwFreshGameBoundary::init_mp_options_roster_required || !impl_->pregame)
        refuse(BnwFreshGameError::wrong_phase,"SP options require SAME actual Game Init prefix");
    const auto actual=impl_->pregame->current_init_inputs();impl_->current();
    if(actual.game_type!=0)
        refuse(BnwFreshGameError::native_rules_unavailable,"Only genuine single-player options branch is implemented");
    if(impl_->sp_options_attempted)refuse(BnwFreshGameError::already_attempted,"SP options already attempted");
    impl_->sp_options_attempted=true;impl_->boundary=BnwFreshGameBoundary::failed_prefix;
    impl_->pregame->clear_single_player_options_from_game();impl_->current();
    impl_->order.emplace_back("init actual SP MP/options setters complete");
    if(impl_->pregame->current_shuffle_teams_from_game())
        refuse(BnwFreshGameError::native_rules_unavailable,"Actual shuffle-teams requires genuine roster/RNG owner");
    impl_->order.emplace_back("init actual isMPOption SHUFFLE_TEAMS false; native roster branch not reached");
    impl_->current();impl_->boundary=BnwFreshGameBoundary::init_lock_mods_speed_required;
    // Stop BEFORE actual isOption LOCK_MODS/password and GameSpeed turn sums.
    // No admin password getter/setter, options-for-MP, roster or ready grant.
}
void BnwFreshGame::advance_constructor_scoped_lock_mods_turn_prefix() {
    impl_->current();
    if(impl_->boundary!=BnwFreshGameBoundary::init_lock_mods_speed_required || !impl_->pregame || !impl_->random_logging)
        refuse(BnwFreshGameError::wrong_phase,"LOCK/turns requires SAME completed actual SP options");
    const auto inputs=impl_->pregame->current_init_inputs();impl_->current();
    if(inputs.game_type!=0)
        refuse(BnwFreshGameError::native_rules_unavailable,"Non-SP LOCK branch has not been issued");
    impl_->random_logging->current(*impl_->config);
    if(impl_->lock_turns_attempted)refuse(BnwFreshGameError::already_attempted,"LOCK/turns already attempted");
    impl_->lock_turns_attempted=true;impl_->boundary=BnwFreshGameBoundary::failed_prefix;
    const bool locked=impl_->pregame->current_lock_mods_from_game();impl_->current();
    impl_->order.emplace_back("init isOption LOCK_MODS ==1");
    if(locked) {
        std::array<char,8> password{};
        for(std::size_t i=0;i<7;++i) {
#ifdef _DEBUG
            refuse(BnwFreshGameError::native_rules_unavailable,"RNG Debug recordCallStack producer unavailable");
#else
            // Same source arithmetic leaf, but preserve native observable order:
            // recordCallStack Release empty -> counter -> logging -> seed store.
            const auto old=impl_->jon_rng.state();
            if(old.calls>=std::numeric_limits<std::uint32_t>::max())
                refuse(BnwFreshGameError::native_rules_unavailable,"Native RNG wrapped call counter outside bounded fresh prefix");
            auto next=impl_->jon_rng;const auto value=next.draw(128);
            auto reached=old;reached.calls=next.state().calls;
            impl_->jon_rng=SimulationRandom(RandomAlgorithm::bnw,reached);
            impl_->order.emplace_back("init password RNG call counter["+std::to_string(i)+"]");
            impl_->random_logging->current(*impl_->config);
            // False is an actual constructor-issued field. No caller logging
            // boolean or selected INI shortcut. External GC setters are absent.
            if(impl_->random_logging->logging)
                refuse(BnwFreshGameError::native_rules_unavailable,"Actual enabled random logging effect owner unavailable");
            impl_->order.emplace_back("init password GC.getLogging actualfalse["+std::to_string(i)+"]");
            impl_->jon_rng=std::move(next);password[i]=static_cast<char>(value);
            impl_->order.emplace_back("init password RNG seed then char["+std::to_string(i)+"]");
#endif
        }
        password[7]=0;
        // CvString = char* stops at the FIRST embedded NUL, while all7 RNG
        // calls already occurred. Never reject control bytes or hash password.
        impl_->pregame->set_admin_password_from_game(std::string(password.data()));impl_->current();
        impl_->order.emplace_back("init CvPreGame.setAdminPassword actual C-string");
    }
    // Materialize SAME immutable native GameSpeedInfo cache before Game reads.
    const auto& increments=impl_->pregame->current_turn_increments_from_game();impl_->current();
    impl_->order.emplace_back("init actual GameSpeedInfo turn cache acquired");
    const auto turn=impl_->pregame->current_game_turn_from_game();impl_->current();
    if(turn==0) {
        // This owner has ONLY namespace construction (-1), not resetGame0.
        // A future genuine setup producer must join here before start percent,
        // setGameTurn logging/UI/GameplayTurnChanged or NoReligion effects.
        impl_->boundary=BnwFreshGameBoundary::init_turn_changed_hooks_required;
        return;
    }
    impl_->put("m_iStartTurn",turn);
    std::int32_t estimate=0;
    for(std::size_t i=0;i<increments.size();++i) {
        if(!increments[i].turns)
            refuse(BnwFreshGameError::native_rules_unavailable,"Reached native turn increment remains UNWRITTEN");
        const auto next=static_cast<std::int64_t>(estimate)+*increments[i].turns;
        if(next<std::numeric_limits<std::int32_t>::min() || next>std::numeric_limits<std::int32_t>::max())
            refuse(BnwFreshGameError::native_rules_unavailable,"Reached native estimate addition overflow");
        estimate=static_cast<std::int32_t>(next);
        impl_->order.emplace_back("init estimate += turn increment["+std::to_string(i)+"]");
    }
    impl_->put("m_iDefaultEstimateEndTurn",estimate);
    impl_->current();impl_->boundary=BnwFreshGameBoundary::constructor_scoped_init_victory_setup_required;
    // Required next producer: true Pregame setup/maxTurns/victories state.
    // No namespace defaults are a completed party/roster/newGame receipt.
}
void BnwFreshGame::advance_city_manager_reset() {
    impl_->current();
    if(impl_->boundary!=BnwFreshGameBoundary::city_manager_reset_required || !impl_->tactical || !impl_->counsel || !impl_->recommender)
        refuse(BnwFreshGameError::wrong_phase,"Final CityManager reset requires SAME native allocation/reset tail");
    if(impl_->city_manager_final_attempted)refuse(BnwFreshGameError::already_attempted,"Final CityManager reset already attempted");
    impl_->city_manager_final_attempted=true;impl_->boundary=BnwFreshGameBoundary::failed_prefix;
    city_manager_reset_from_impl(*impl_);
    impl_->order.emplace_back("reset-tail CvCityManager::Reset SAME scoped pointer realm");
    impl_->current();impl_->boundary=BnwFreshGameBoundary::scoped_reset_complete_owned_cities_required;
    // Reset body has returned in this owned isolated realm. A new game's
    // genuine owned-city allocator/cache routing and Init are still required.
}

void BnwFreshGame::advance_reset_prefix() {
    impl_->current();if(impl_->civis_setup)refuse(BnwFreshGameError::wrong_phase,"Civis setup cannot enter the separate legacy initialization route");
    impl_->current();
    if(impl_->early_pregame_attempted) {
        const auto allocation=impl_;const auto facade=allocation->same_facade(*this);(void)facade;
        // Actual LoadPreGameSettings producer is still mandatory. No final
        // settings boundary exists yet, so the early route cannot enter Reset.
        refuse(BnwFreshGameError::native_rules_unavailable,"SAME Game-owned Pregame setup requires actual LoadPreGameSettings before Game Reset/Init");
    }
    if(impl_->attempted) refuse(BnwFreshGameError::already_attempted,"Game nonconstructor reset already attempted");
    impl_->attempted=true;impl_->boundary=BnwFreshGameBoundary::failed_prefix;
    // Reached native/cache stores survive later failure; no replay or rollback
    // masquerading as original Game reset behavior.
    impl_->nonconstructor(*this);
}
void BnwFreshGame::advance_settler_site_init() {impl_->settler_init();}
void BnwFreshGame::advance_start_site_init() {impl_->start_init();}
void BnwFreshGame::advance_start_positioner_init() {impl_->positioner_init();}
void BnwFreshGame::advance_deals_game_religions_prefix() {impl_->deals_religions_prefix();}
void BnwFreshGame::advance_game_religions_speed_init(std::filesystem::path selected_config_ini) {impl_->religions_speed_init(std::move(selected_config_ini));}
void BnwFreshGame::advance_culture_leagues_trade_init() {impl_->game_services_init();}
void BnwFreshGame::advance_tactical_advisors_reset_tail() {
    impl_->current();
    if(impl_->boundary!=BnwFreshGameBoundary::tactical_map_construction_required || !impl_->trade || (!impl_->pregame && !impl_->civis_setup) ||
       impl_->tactical || impl_->counsel || impl_->recommender)
        refuse(BnwFreshGameError::wrong_phase,"Game allocation tail requires SAME completed Trade.Init");
    if(impl_->tactical_advisors_attempted)refuse(BnwFreshGameError::already_attempted,"Game allocation tail already attempted");
    impl_->tactical_advisors_attempted=true;impl_->boundary=BnwFreshGameBoundary::failed_prefix;
    // Private constructors are invoked by the actual friend facade member,
    // never through make_unique/private access in a detached helper.
    impl_->tactical.reset(new BnwFreshTacticalMap(impl_->config,impl_->config->game_rules()));
    impl_->null_services.erase("m_pTacticalMap");impl_->order.emplace_back("Game.new CvTacticalAnalysisMap constructor ONLY");
    impl_->current();impl_->tactical->require_current();
    impl_->counsel.reset(new BnwFreshAdvisorCounsel(impl_->config));
    impl_->null_services.erase("m_pAdvisorCounsel");impl_->order.emplace_back("Game.new CvAdvisorCounsel");
    impl_->current();impl_->counsel->require_current();
    impl_->recommender.reset(new BnwFreshAdvisorRecommender(impl_->config));
    impl_->null_services.erase("m_pAdvisorRecommender");impl_->order.emplace_back("Game.new CvAdvisorRecommender");
    impl_->current();impl_->recommender->require_current();
    impl_->selections.remove_all();impl_->order.emplace_back("reset-tail m_voteSelections.RemoveAll");
    impl_->triggered.remove_all();impl_->order.emplace_back("reset-tail m_votesTriggered.RemoveAll");
    impl_->map_rng.reset();impl_->order.emplace_back("reset-tail m_mapRand.reset");
    impl_->jon_rng.reset();impl_->order.emplace_back("reset-tail m_jonRand.reset");
    impl_->put("m_iNumSessions",std::int32_t{1});
    impl_->advisor_messages.clear();impl_->order.emplace_back("reset-tail m_AdvisorMessagesViewed.clear");
    impl_->current();impl_->boundary=BnwFreshGameBoundary::city_manager_reset_required;
    // Final Reset remains a separate once operation on SAME constructor-issued realm.
}
std::shared_ptr<const BnwFreshTacticalMap> BnwFreshGame::tactical_analysis_map() const {
    impl_->current();if(!impl_->tactical)refuse(BnwFreshGameError::wrong_phase,"Game TacticalMap has not been allocated");
    impl_->tactical->require_current();
    return std::shared_ptr<const BnwFreshTacticalMap>(impl_,impl_->tactical.get());
}
std::shared_ptr<const BnwFreshAdvisorCounsel> BnwFreshGame::advisor_counsel() const {
    impl_->current();if(!impl_->counsel)refuse(BnwFreshGameError::wrong_phase,"Game AdvisorCounsel has not been allocated");
    impl_->counsel->require_current();return std::shared_ptr<const BnwFreshAdvisorCounsel>(impl_,impl_->counsel.get());
}
std::shared_ptr<const BnwFreshAdvisorRecommender> BnwFreshGame::advisor_recommender() const {
    impl_->current();if(!impl_->recommender)refuse(BnwFreshGameError::wrong_phase,"Game AdvisorRecommender has not been allocated");
    impl_->recommender->require_current();return std::shared_ptr<const BnwFreshAdvisorRecommender>(impl_,impl_->recommender.get());
}

BnwFreshGameObservation BnwFreshGame::observe() const {
    impl_->current();
    auto arrays=impl_->arrays;
    if(impl_->settler) {
        // Detached diagnostics of actual owned INLINE native fields. These
        // arrays cannot issue a Game/Init owner or fill unwritten slots.
        arrays["Settler.m_iRingModifier"].slots.emplace(impl_->settler->ring.begin(),impl_->settler->ring.end());
        arrays["Settler.m_iFlavorMultiplier"].slots.emplace(impl_->settler->flavor_multiplier.begin(),impl_->settler->flavor_multiplier.end());
    }
    if(impl_->start) {
        arrays["Start.m_iRingModifier"].slots.emplace(impl_->start->ring.begin(),impl_->start->ring.end());
        arrays["Start.m_iFlavorMultiplier"].slots.emplace(impl_->start->flavor_multiplier.begin(),impl_->start->flavor_multiplier.end());
    }
    auto scalars=impl_->scalars;
    if(impl_->random_logging) {
        impl_->random_logging->current(*impl_->config);
        scalars["Globals.constructor.m_bLogging"]=impl_->random_logging->logging;
        scalars["Globals.constructor.m_iRandLogging"]=impl_->random_logging->rand_logging;
    }
    scalars["CityManager.scopedResetCalls"]=impl_->scoped_city_manager_resets;
    scalars["CityManager.scopedShutdownCalls"]=impl_->scoped_city_manager_shutdowns;
    if(impl_->positioner) {
        // Detached diagnostic sizes from ACTUAL default-constructed vectors.
        scalars["StartPositioner.m_ContinentVector.size"]=static_cast<std::int32_t>(impl_->positioner->continents.size());
        scalars["StartPositioner.m_StartRegionVector.size"]=static_cast<std::int32_t>(impl_->positioner->regions.size());
        scalars["StartPositioner.m_PlayerOrder.size"]=static_cast<std::int32_t>(impl_->positioner->player_order.size());
    }
    if(impl_->religions) {
        scalars["GameReligions.m_iMinimumFaithForNextPantheon.current"]=impl_->religions->minimum_faith;
        scalars["GameReligions.m_CurrentReligions.size"]=static_cast<std::int32_t>(impl_->religions->religion_count);
        scalars["GameReligions.m_CurrentReligions.capacity"]=static_cast<std::int32_t>(impl_->religions->inline_capacity);
        scalars["GameDeals.m_ProposedDeals.size"]=static_cast<std::int32_t>(impl_->deals.proposed.size());
        scalars["GameDeals.m_CurrentDeals.size"]=static_cast<std::int32_t>(impl_->deals.current.size());
        scalars["GameDeals.m_HistoricalDeals.size"]=static_cast<std::int32_t>(impl_->deals.historical.size());
        scalars["GameDeals.m_uiDealCounter.current"]=impl_->deals.counter;
    }
    scalars["reset-tail.advisorMessages.size"]=static_cast<std::int32_t>(impl_->advisor_messages.size());
    const auto vote=[&](const char* name,const EmptyVoteList& list) {
        const std::string prefix=name;
        scalars[prefix+".slots"]=list.num_slots;scalars[prefix+".last"]=list.last;
        scalars[prefix+".freeHead"]=list.free_head;scalars[prefix+".freeCount"]=list.free_count;
        scalars[prefix+".currentID"]=list.current_id;
        if(list.nodes) {
            auto& slots=arrays[prefix+".nextFree"].slots;slots.emplace();
            auto& nulls=arrays[prefix+".nullData"].bool_slots;nulls.emplace();
            for(const auto& node:*list.nodes) {slots->emplace_back(node.next_free);nulls->push_back(node.data==nullptr);}
        }
    };
    vote("voteSelections",impl_->selections);vote("votesTriggered",impl_->triggered);
    scalars["reset-tail.voteSelections.freeHead"]=impl_->selections.free_head;
    scalars["reset-tail.votesTriggered.freeHead"]=impl_->triggered.free_head;
    if(impl_->culture) {
        scalars["GameCulture.m_CurrentGreatWorks.size"]=static_cast<std::int32_t>(impl_->culture->great_work_count);
        scalars["GameCulture.m_CurrentGreatWorks.capacity"]=static_cast<std::int32_t>(impl_->culture->inline_capacity);
    }
    if(impl_->leagues) {
        scalars["GameLeagues.m_vActiveLeagues.size"]=static_cast<std::int32_t>(impl_->leagues->active_count);
        scalars["GameLeagues.m_vActiveLeagues.capacity"]=static_cast<std::int32_t>(impl_->leagues->inline_capacity);
    }
    if(impl_->trade) {
        scalars["GameTrade.m_aTradeConnections.size"]=static_cast<std::int32_t>(impl_->trade->connection_count);
        scalars["GameTrade.m_aTradeConnections.capacity"]=static_cast<std::int32_t>(impl_->trade->inline_capacity);
        auto& flat=arrays["GameTrade.m_aaiTechDifference"].slots;flat.emplace();
        for(const auto& row:impl_->trade->tech_difference)flat->insert(flat->end(),row.begin(),row.end());
    }
    if(impl_->pregame) {
        const auto actual=impl_->pregame->observe();
        scalars["GameSpeedInfo.m_iNumTurnIncrements"]=actual.turn_count;
        if(actual.turn_cache) {
            auto& months=arrays["GameSpeedInfo.turn.MonthIncrement"].slots;months.emplace();
            auto& turns=arrays["GameSpeedInfo.turn.TurnsPerIncrement"].slots;turns.emplace();
            for(const auto& value:*actual.turn_cache) {months->push_back(value.month_increment);turns->push_back(value.turns);}
        }
        scalars["Pregame.s_gameTurn"]=actual.game_turn;
        scalars["Pregame.s_adminPassword"]=actual.admin_password;
        scalars["Pregame.s_gameSpeed"]=actual.speed;scalars["Pregame.s_turnTimerType"]=actual.timer;
        scalars["Pregame.s_era"]=actual.init_inputs.era;scalars["Pregame.s_gameType"]=actual.init_inputs.game_type;
        scalars["Pregame.s_gameStartType"]=actual.init_inputs.game_start_type;
        scalars["Pregame.s_mapRandomSeed"]=actual.init_inputs.map_seed;scalars["Pregame.s_syncRandomSeed"]=actual.init_inputs.sync_seed;
        scalars["Pregame.s_isInternetGame"]=actual.init_inputs.internet_game;
        arrays["Pregame.s_multiplayerOptions"].bool_slots.emplace(actual.options.multiplayer.begin(),actual.options.multiplayer.end());
        arrays["Pregame.s_EnumBasedGameOptions"].slots.emplace(actual.options.enum_values.begin(),actual.options.enum_values.end());
        scalars["Pregame.s_quickCombat"]=actual.options.quick_combat;
        scalars["Pregame.options.complete"]=actual.options.complete;
        for(std::size_t i=0;i<actual.options.custom.size();++i) {
            const auto name="Pregame.option["+std::to_string(i)+"]";
            scalars[name+".name"]=actual.options.custom[i].name;scalars[name+".value"]=actual.options.custom[i].value;
        }
        scalars["Pregame.options.count"]=static_cast<std::uint32_t>(actual.options.custom.size());
        scalars["Pregame.options.hashCount"]=static_cast<std::uint32_t>(actual.options.hash_indices.size());
        for(const auto& [hash,index]:actual.options.hash_indices)
            scalars["Pregame.optionHash["+std::to_string(hash)+"].index"]=static_cast<std::int32_t>(index);
    }
    return {impl_->boundary,std::move(scalars),std::move(arrays),impl_->victory,
        impl_->map_rng.state(),impl_->jon_rng.state(),impl_->cache->database,impl_->cache_resets,impl_->order};
}
} // namespace civ5::runtime
