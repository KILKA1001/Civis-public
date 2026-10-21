#include "civ5/runtime/bnw_fresh_pregame_speed.hpp"
#include "civ5/runtime/bnw_fresh_pregame_roster.hpp"
#include "civ5/runtime/bnw_selected_locale_owner.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <thread>
#include <utility>
namespace civ5::runtime {
namespace {
[[noreturn]] void refuse(BnwFreshPregameSpeedError e,const std::string& s) {
    throw BnwFreshPregameSpeedRefusal(e,s);
}
std::int32_t i32(std::int64_t v) {
    if(v<std::numeric_limits<std::int32_t>::min() || v>std::numeric_limits<std::int32_t>::max())
        refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame reached native GetInt overflow");
    return static_cast<std::int32_t>(v);
}
bool same(const content::InfoTypeEntry& a,const content::InfoTypeEntry& b) {
    return a.table==b.table && a.type==b.type && a.id==b.id && a.hash==b.hash;
}
bool ascii_equal(std::string_view a,std::string_view b) {
    const auto lower=[](unsigned char c){
        if(c>127) refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame native _stricmp requires bounded ASCII source");
        return c>='A' && c<='Z'?static_cast<unsigned char>(c+('a'-'A')):c;
    };
    for(char c:a)(void)lower(static_cast<unsigned char>(c));
    for(char c:b)(void)lower(static_cast<unsigned char>(c));
    if(a.size()!=b.size())return false;
    for(std::size_t i=0;i<a.size();++i)if(lower(static_cast<unsigned char>(a[i]))!=lower(static_cast<unsigned char>(b[i])))return false;
    return true;
}
}
struct BnwFreshPregameSpeed::Impl {
    const std::thread::id thread=std::this_thread::get_id();
    const std::filesystem::path selected_config_ini;
    explicit Impl(std::filesystem::path selected):selected_config_ini(std::move(selected)) {}
    std::shared_ptr<const BnwCreationConfig> config;
    std::weak_ptr<BnwFreshPregameSpeed> actual_facade;
    std::shared_ptr<BnwFreshPregameRoster> roster;
    BnwFreshPregameMenuBoundary menu_boundary=BnwFreshPregameMenuBoundary::menu_request_required;
    bool menu_attempted=false,private_game=false,menu_options_attempted=false;

    struct EnvironmentInfo {
        BnwFreshPregameEnvironmentSetterBoundary boundary=
            BnwFreshPregameEnvironmentSetterBoundary::not_called;
        std::uint64_t call_count=0;
        std::string requested_type;
        // Native namespace constructor values; cache ID remains separately -1.
        std::int32_t selected_id;
        bool active=false;
        std::map<std::string,std::int32_t,std::less<>> numbers{{"ID",-1}};
        std::map<std::string,float,std::less<>> floats;
        std::map<std::string,std::string,std::less<>> text{
            {"Type",""},{"Text",""},{"Civilopedia",""},{"Strategy",""},
            {"Help",""},{"DisabledHelp",""},{"DescriptionKey",""},
            {"Description",""},{"LocalizedText",""}};
        std::vector<std::string> order;
        explicit EnvironmentInfo(bool climate):selected_id(-1) {
            if(climate) {
                for(const char* name:{"DesertPercentChange","JungleLatitude","HillRange","MountainPercent"})numbers.emplace(name,0);
                for(const char* name:{"SnowLatitudeChange","TundraLatitudeChange","GrassLatitudeChange",
                    "DesertBottomLatitudeChange","DesertTopLatitudeChange","IceLatitude","RandIceLatitude"})floats.emplace(name,0.0f);
            } else numbers.emplace("SeaLevelChange",0);
        }
        BnwFreshPregameEnvironmentInfoObservation observe()const {
            return {boundary,call_count,requested_type,selected_id,numbers,floats,text,order};
        }
    };
    EnvironmentInfo climate_info{true},sea_level_info{false};
    bool environment_getter_failed=false,world_getter_active=false,world_getter_failed=false;
    void environment_current(const BnwFreshPregameSpeed& actual)const {
        // Strong retained Impl plus original facade AND actual facade slot.
        // A callback cannot substitute the facade allocation and resume stores.
        if(thread!=std::this_thread::get_id())
            refuse(BnwFreshPregameSpeedError::wrong_thread,"Pregame environment belongs to another thread");
        if(actual.impl_.get()!=this)
            refuse(BnwFreshPregameSpeedError::retired,"Pregame environment actual allocation replaced");
        (void)same_facade(actual);
    }
    void cache_environment_row(const BnwFreshPregameSpeed& actual,bool climate,const content::RulesRowView& row) {
        auto& target=climate?climate_info:sea_level_info;
        const auto number=[&](const char* name) {
            environment_current(actual);const auto value=i32(row.integer(name));
            environment_current(actual);target.numbers.at(name)=value;target.order.emplace_back(name);
        };
        const auto text_store=[&](const char* name) {
            environment_current(actual);auto value=row.text(name).value_or("");
            environment_current(actual);target.text.at(name)=std::move(value);target.order.emplace_back(name);
        };
        // CvBaseInfo::CacheResults in exact source order. Its
        // Localization callbacks may fail after earlier stores.
        number("ID");text_store("Type");text_store("Text");text_store("Civilopedia");
        text_store("Strategy");text_store("Help");text_store("DisabledHelp");
        if(!target.text.at("Text").empty()) {
            const auto key=target.text.at("Text");auto value=localized(key);
            environment_current(actual);target.text.at("LocalizedText")=std::move(value);
            target.order.emplace_back("LocalizedText");
        }
        environment_current(actual);const auto description=row.text("Description");
        if(description) {
            target.text.at("DescriptionKey")=*description;target.order.emplace_back("DescriptionKey");
            auto value=localized(*description);environment_current(actual);
            target.text.at("Description")=std::move(value);target.order.emplace_back("Description");
            if(target.text.at("Text").empty()) {
                target.text.at("Text")=*description;target.order.emplace_back("Text <- DescriptionKey");
                target.text.at("LocalizedText")=target.text.at("Description");
                target.order.emplace_back("LocalizedText <- Description");
            }
        }
        if(climate) {
            for(const char* name:{"DesertPercentChange","JungleLatitude","HillRange","MountainPercent"})number(name);
            for(const char* name:{"SnowLatitudeChange","TundraLatitudeChange","GrassLatitudeChange",
                "DesertBottomLatitudeChange","DesertTopLatitudeChange","IceLatitude","RandIceLatitude"}) {
                environment_current(actual);const auto raw=row.real(name);
                if(!std::isfinite(raw) || raw>std::numeric_limits<float>::max() || raw<-std::numeric_limits<float>::max())
                    refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Climate reached GetFloat exceeds finite native float domain");
                const auto value=static_cast<float>(raw);environment_current(actual);
                target.floats.at(name)=value;target.order.emplace_back(name);
            }
        } else number("SeaLevelChange");
        environment_current(actual);
    }


    BnwFreshPregameEnvironmentInfoObservation environment_info(
        const BnwFreshPregameSpeed& actual,bool climate) {
        environment_current(actual);
        auto& target=climate?climate_info:sea_level_info;
        if(climate_info.active || sea_level_info.active || world_setter_active ||
            environment_getter_failed || world_getter_active || world_getter_failed ||
            climate_info.boundary==BnwFreshPregameEnvironmentSetterBoundary::failed_prefix ||
            sea_level_info.boundary==BnwFreshPregameEnvironmentSetterBoundary::failed_prefix)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"Pregame environment getter cannot borrow active or failed native cache");
        // CvPreGame climateInfo789 / seaLevelInfo2234 first compare the
        // ACTUAL namespace ID against the SAME owned cache's GetID. Equal
        // IDs perform no query, no CacheResult, no Locale and no setter.
        if(target.selected_id!=target.numbers.at("ID")) {
            if(!globals_database || cached_globals_boundary!=BnwFreshPregameCachedGlobalsBoundary::selected_database_subset_stored)
                refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame environment mismatch getter lacks SAME actual selected SQL allocation");
            // Original CvGameDatabase numeric SelectAt export10003550
            // pushes SQL100786c8: select %s from %q where ROWID = %i LIMIT 1.
            // This is numeric ROWID, NEVER substitute authored Info ID.
            target.active=true;
            try {
                bool found=false;const std::string table=climate?"Climates":"SeaLevels";
                const auto selected=target.selected_id;
                const auto report=globals_database->query_rows(
                    {"select * from "+table+" where ROWID = ? LIMIT 1",{static_cast<std::int64_t>(selected)}},
                    [&](const content::RulesRowView& row) {
                        environment_current(actual);found=true;cache_environment_row(actual,climate,row);
                        // The original getter does NOT assign s_climate or
                        // s_seaLevel from a row's authored ID. A mismatch may
                        // legitimately cause another real recache next call.
                        environment_current(actual);
                    },1);
                environment_current(actual);
                if(!report.all_succeeded() || !found)
                    refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame numeric ROWID getter missing actual selected row before Info return");
                target.active=false;
            }catch(...) {target.active=false;environment_getter_failed=true;throw;}
        }
        // Constructor NO_CLIMATE / NO_SEALEVEL matches actual cache -1 and
        // really returns the empty native Info. This is NOT a ready World.
        auto value=target.observe();environment_current(actual);return value;
    }

    void set_environment(const BnwFreshPregameSpeed& actual,bool climate,std::string requested) {
        environment_current(actual);
        auto& target=climate?climate_info:sea_level_info;
        if(menu_boundary!=BnwFreshPregameMenuBoundary::load_pregame_settings_required ||
            !globals_database || cached_globals_boundary!=BnwFreshPregameCachedGlobalsBoundary::selected_database_subset_stored ||
            climate_info.active || sea_level_info.active || world_setter_active || environment_getter_failed || world_getter_active || world_getter_failed ||
            climate_info.boundary==BnwFreshPregameEnvironmentSetterBoundary::failed_prefix ||
            sea_level_info.boundary==BnwFreshPregameEnvironmentSetterBoundary::failed_prefix)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"Environment string setter requires SAME actual menu prefix and no failed/reentrant call");
        if(target.call_count==std::numeric_limits<std::uint64_t>::max())
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Environment string setter operation index overflow");
        target.requested_type=std::move(requested);++target.call_count;
        target.boundary=BnwFreshPregameEnvironmentSetterBoundary::failed_prefix;target.active=true;
        const std::string table=climate?"Climates":"SeaLevels";
        try {
            bool found=false;
            const auto report=globals_database->query_rows(
                {"SELECT * from "+table+" where Type = ? LIMIT 1",{target.requested_type}},
                [&](const content::RulesRowView& row) {
                    environment_current(actual);found=true;
                    cache_environment_row(actual,climate,row);
                    target.selected_id=target.numbers.at("ID");
                    target.order.emplace_back(climate?"s_climate <- cache.GetID":"s_seaLevel <- cache.GetID");
                    environment_current(actual);
                },1);
            environment_current(actual);
            if(!report.all_succeeded() || !found)
                refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Environment string setter actual SQL/Type row unavailable; no guessed cache");
            target.active=false;target.boundary=BnwFreshPregameEnvironmentSetterBoundary::cache_and_selection_stored;
        } catch(...) {
            target.active=false;target.boundary=BnwFreshPregameEnvironmentSetterBoundary::failed_prefix;throw;
        }
    }

    BnwFreshPregameWorldSetterObservation world_setter{BnwFreshPregameWorldSetterBoundary::not_called,0,{}};
    bool world_setter_active=false;
    // Actual namespace PREGAMEVAR(bool,s_gameStarted,false), owned in this
    // unstarted slice; resetGame's reached setter writes the SAME field.
    bool game_started=false;
    std::shared_ptr<const BnwSelectedLocaleOwner> locale;
    const content::RulesDatabase* db=nullptr;
    BnwFreshPregameSpeedBoundary boundary=BnwFreshPregameSpeedBoundary::constructed;
    bool attempted=false;
    std::int32_t speed=-1,timer=-1;
    // Actual namespace static initialization, not resetGame/loadFromIni.
    std::int32_t era=-1,game_type=-1,game_start_type=0;
    // Namespace constructor state, NOT resetGame (which separately writes0).
    std::int32_t game_turn=-1;
    std::string admin_password;
    // SAME selected GameSpeed native cache child; partial stores retained.
    std::int32_t turn_count=0;
    std::optional<std::vector<BnwFreshPregameTurnIncrement>> turn_cache;
    bool turn_cache_attempted=false;
    std::uint32_t map_seed=0,sync_seed=0;
    bool internet_game=false,init_attempted=false,init_complete=false;
    std::vector<bool> era_present;
    // Native FAutoVector(count4) value-initializes all bools. GameOptions
    // default vector/hash map and namespace static enum array start empty/zero.
    std::array<bool,4> multiplayer{};
    std::vector<BnwFreshPregameCustomOption> custom_options;
    std::map<std::uint32_t,std::size_t> option_hash_indices;
    std::array<std::int32_t,22> enum_values{};
    bool quick_combat=false,options_attempted=false,options_complete=false;
    // Private issued source subset, not a full GC/Prefetch startup receipt.
    BnwFreshPregameResetBoundary reset_boundary=BnwFreshPregameResetBoundary::constructor_diagnostic;
    std::int32_t cached_standard_speed=2,cached_standard_era=0;
    std::int32_t cached_standard_handicap=1,cached_multiplayer_handicap=3,cached_ai_handicap=8;
    std::unique_ptr<content::RulesDatabase> globals_database;
    std::optional<content::InfoTypeCatalog> globals_types;
    std::shared_ptr<BnwFreshCivilizationInfoCollection> ordered_info;
    BnwFreshPregameCachedGlobalsBoundary cached_globals_boundary=BnwFreshPregameCachedGlobalsBoundary::constructor_diagnostic;
    BnwFreshPregameCachedGlobalsObservation cached_globals_observe() const {
        return {cached_globals_boundary,cached_standard_handicap,cached_multiplayer_handicap,
            cached_standard_speed,cached_standard_era,cached_ai_handicap};
    }
    void issue_cached_globals_subset() {
        current();
        if(cached_globals_boundary!=BnwFreshPregameCachedGlobalsBoundary::constructor_diagnostic)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"Cached Globals subset cannot replay or recover a failed prefix");
        cached_globals_boundary=BnwFreshPregameCachedGlobalsBoundary::failed_prefix;
        // Activated fresh rules precede the native PostDefines ROWID updates.
        // Keep original immutable source and a private derived SQL allocation.
        globals_database=db->clone_gameplay();current();
        const auto postprocess=globals_database->postprocess_gameplay(content::GameplayPostprocessProfile::bnw,{});
        current();
        // Only the genuine PostDefines substage supplies this scoped cache.
        // Other ResourceUsage/runtime-enum report fields confer no readiness.
        for(const auto& error:postprocess.errors)
            if(error.stage=="prepare_insert" || error.stage=="post_defines")
                refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame native PostDefines source failed");
        for(const auto& row:postprocess.post_defines) {
            if(!row.errors.empty())
                refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame native PostDefines row failed");
            if(row.rowid && (row.name=="STANDARD_HANDICAP" || row.name=="MULTIPLAYER_HANDICAP" ||
                row.name=="STANDARD_GAMESPEED" || row.name=="STANDARD_ERA" || row.name=="AI_HANDICAP"))
                (void)i32(*row.rowid); // closed GetInt32 domain, never claim widened ROWID parity
        }
        if(!postprocess.transaction_released)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame native PostDefines transaction release failed");
        order.emplace_back("PostDefines actual owned SQL clone source");
        globals_types.emplace(globals_database->cache_info_types());current();
        if(!globals_types->report().all_succeeded())
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame native Info Type catalog failed before cacheGlobals");
        order.emplace_back("owned cacheGlobals predecessor Info Type catalog");
        // Relative cacheGlobals source order, before menu or Victory prefetch.
        // This selected DB projection is NOT gDLL.InitGlobals/settings completion.
        const auto cache=[&](const char* name) {
            current();const auto result=globals_database->global_define(name,content::GlobalDefineKind::integer);current();
            if(!result.error.empty())refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame cached Globals selected Define lookup failed");
            return i32(std::get<std::int64_t>(result.value));
        };
        cached_standard_handicap=cache("STANDARD_HANDICAP");order.emplace_back("cacheGlobals STANDARD_HANDICAP");
        cached_multiplayer_handicap=cache("MULTIPLAYER_HANDICAP");order.emplace_back("cacheGlobals MULTIPLAYER_HANDICAP");
        cached_standard_speed=cache("STANDARD_GAMESPEED");order.emplace_back("cacheGlobals STANDARD_GAMESPEED");
        cached_standard_era=cache("STANDARD_ERA");order.emplace_back("cacheGlobals STANDARD_ERA");
        cached_ai_handicap=cache("AI_HANDICAP");order.emplace_back("cacheGlobals AI_HANDICAP");
        current();cached_globals_boundary=BnwFreshPregameCachedGlobalsBoundary::selected_database_subset_stored;
    }
    std::int32_t native_victory_span=0,victory_count=0;
    std::vector<bool> victories;
    std::map<std::string,std::int32_t,std::less<>> reset_integer;
    std::map<std::string,bool,std::less<>> reset_boolean;
    std::map<std::string,float,std::less<>> reset_float;
    std::map<std::string,std::string,std::less<>> reset_string;
    BnwFreshPregameResetObservation reset_observe() const {
        return {reset_boundary,cached_standard_speed,cached_standard_era,victory_count,
            victories,reset_integer,reset_boolean,reset_float,reset_string};
    }
    void issue_reset_inputs() {
        current();
        issue_cached_globals_subset();
        const auto report=db->prefetch_info("Victories",[&](std::uint64_t slot,std::int64_t id,const content::RulesRowView&){
            current();(void)i32(id);
            if(slot>=65536)refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame reset native Victory span exceeds bounded allocation");
            // Native GetNumVictoryInfos consumes vector.size(), including NULL
            // holes/duplicate append slots, never MaxRows(rowid)/authored count.
            return true;
        },content::InfoCollectionBudget{65536});current();
        if(!report.all_succeeded() || report.slots>65536)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame reset selected Victory collection shape unavailable");
        native_victory_span=i32(static_cast<std::int64_t>(report.slots));
        order.emplace_back("reset inputs native Victory collection span");
        reset_boundary=BnwFreshPregameResetBoundary::reset_game_required;
    }
    void reset_victories(const char* prefix) {
        current();victory_count=native_victory_span;order.emplace_back(std::string(prefix)+"s_numVictoryInfos");
        victories.clear();order.emplace_back(std::string(prefix)+"s_victories.clear");
        for(std::int32_t i=0;i<victory_count;++i) {
            victories.push_back(true);order.emplace_back(std::string(prefix)+"s_victories.push_back true["+std::to_string(i)+"]");
        }
    }
    void reset_game() {
        current();
        if(menu_attempted)refuse(BnwFreshPregameSpeedError::wrong_phase,"Cannot bypass actual menu ResetSlots prefix");
        if(reset_boundary!=BnwFreshPregameResetBoundary::reset_game_required || attempted || init_attempted || options_attempted)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"Actual Pregame reset must precede selected settings and cannot replay");
        reset_boundary=BnwFreshPregameResetBoundary::failed_prefix;
        const auto number=[&](const char* name,std::int32_t value){reset_integer[name]=value;order.emplace_back(std::string("resetGame ")+name);};
        const auto flag=[&](const char* name,bool value){reset_boolean[name]=value;
            if(std::string_view(name)=="s_gameStarted")game_started=value;
            order.emplace_back(std::string("resetGame ")+name);};
        const auto text_store=[&](const char* name,const char* value){reset_string[name]=value;order.emplace_back(std::string("resetGame ")+name);};
        const auto floating=[&](const char* name,float value){reset_float[name]=value;order.emplace_back(std::string("resetGame ")+name);};
        game_start_type=0;number("s_gameStartType",0);
        number("s_gameMapType",0);
        flag("s_gameStarted",false);
        text_store("s_gameName","");
        admin_password="";text_store("s_adminPassword","");
        text_store("s_mapScriptName","");
        flag("s_loadWBScenario",false);
        flag("s_mapNoPlayers",false);
        climate_info.selected_id=0;number("s_climate",0);
        sea_level_info.selected_id=1;number("s_seaLevel",1);
        era=cached_standard_era;number("s_era",cached_standard_era);
        speed=cached_standard_speed;number("s_gameSpeed",cached_standard_speed);
        timer=4;number("s_turnTimerType",4);
        number("s_calendar",0);
        reset_victories("resetGame ");
        for(std::size_t i=0;i<multiplayer.size();++i) {multiplayer[i]=false;order.emplace_back("resetGame s_multiplayerOptions.setAt false["+std::to_string(i)+"]");}
        game_turn=0;number("s_gameTurn",0);
        number("s_maxTurns",0);
        number("s_pitBossTurnTime",0);
        number("s_targetScore",0);
        number("s_maxCityElimination",0);
        number("s_numMinorCivs",-1);
        number("s_advancedStartPoints",0);
        sync_seed=0;number("s_syncRandomSeed",0);
        map_seed=0;number("s_mapRandomSeed",0);
        number("s_activePlayer",-1);
        flag("s_quickstart",false);
        flag("s_randomWorldSize",false);
        flag("s_randomMapScript",false);
        flag("s_isEarthMap",false);
        flag("s_autorun",false);
        flag("s_transferredMap",false);
        number("s_autorunTurnLimit",0);
        floating("s_autorunTurnDelay",0.0f);
        number("s_gameUpdateTime",0);
        number("s_bandwidth",2);
        number("s_quickHandicap",-1);
        text_store("s_alias","");
        text_store("s_localPlayerEmailAddress","");
        text_store("s_loadFileName","");
        number("s_loadFileStorage",0);
        floating("s_endTurnTimerLength",0.0f);
        flag("s_privateGame",false);
        internet_game=false;flag("s_isInternetGame",false);
        // ResetMapOptions has only vector.clear, no map generation callback.
        map_options.clear();order.emplace_back("resetGame ResetMapOptions s_MapOptions.clear");
        custom_options.clear();order.emplace_back("resetGame ResetGameOptions s_GameOptions.clear");
        option_hash_indices.clear();order.emplace_back("resetGame ResetGameOptions s_GameOptionsHash.clear");
        // The source rehash loop is empty AFTER clear. Do not invent entries.
        sync_enum_options();
        reset_victories("resetGame ResetGameOptions ");
        current();reset_boundary=BnwFreshPregameResetBoundary::reset_game_settings_assignment_required;
    }
    std::vector<BnwFreshPregameCustomOption> map_options;
    BnwFreshPregameOptionsObservation options_observe() const {
        return {options_attempted,options_complete,multiplayer,custom_options,option_hash_indices,enum_values,quick_combat};
    }
    BnwFreshPregameInitObservation init_observe() const {
        return {init_attempted,init_complete,era,game_type,game_start_type,map_seed,sync_seed,internet_game};
    }
    // CvBaseInfo(-1) then native CvTurnTimerInfo constructor zeros4.
    std::map<std::string,std::int32_t,std::less<>> numbers{
        {"ID",-1},{"BaseTime",0},{"CityResource",0},{"UnitResource",0},{"FirstTurnMultiplier",0}};
    // CvString members default-construct empty, independently of CacheResult.
    std::map<std::string,std::string,std::less<>> text{
        {"Type",""},{"Text",""},{"Civilopedia",""},{"Strategy",""},
        {"Help",""},{"DisabledHelp",""},{"DescriptionKey",""},
        {"Description",""},{"LocalizedText",""}};
    std::vector<std::string> order{"s_gameSpeed <- NO_GAMESPEED","CvTurnTimerInfo constructor","s_turnTimerType <- NO_TURNTIMER"};
    void current() const {
        if(thread!=std::this_thread::get_id())refuse(BnwFreshPregameSpeedError::wrong_thread,"Pregame speed owner belongs to another thread");
        if(!config)refuse(BnwFreshPregameSpeedError::parent_missing,"Pregame speed lost actual Config");
        config->require_current();
        if(&config->game_rules()!=db)refuse(BnwFreshPregameSpeedError::retired,"Pregame speed SAME DB changed");
        if(locale)locale->require_same_config(*config);
    }
    std::shared_ptr<BnwFreshPregameSpeed> same_facade(const BnwFreshPregameSpeed& actual) const {
        // Closed issuer metadata and thread before any Config/DB source borrow.
        if(thread!=std::this_thread::get_id())refuse(BnwFreshPregameSpeedError::wrong_thread,"Pregame roster issuer belongs to another thread");
        auto held=actual_facade.lock();
        if(!held || held.get()!=&actual)
            refuse(BnwFreshPregameSpeedError::retired,"Pregame roster requires SAME original issuer facade");
        current();return held;
    }
    std::string localized(std::string_view key) {
        current();
        if(!locale)locale=BnwSelectedLocaleOwner::from_creation_config(config,selected_config_ini);
        locale->require_same_config(*config);const auto value=locale->localized_text(key);
        locale->require_same_config(*config);current();return value;
    }
    void tuple(const content::InfoTypeCatalog& types,std::int64_t slot,std::int64_t authored,const std::string& type) const {
        if(slot!=authored || slot<0 || slot>std::numeric_limits<std::int32_t>::max())
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame GameSpeed native slot/authored binding unsupported");
        const auto hash=content::saved_info_type_hash(type);const auto* n=types.find(type);const auto* h=types.find_hash(hash);
        if(!n || !h || !same(*n,*h) || n->table!="GameSpeeds" || n->type!=type || n->id!=authored || n->hash!=hash)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame GameSpeed native tuple ambiguous");
    }
    void timer_set(std::int32_t selected) {
        timer=selected;order.emplace_back("s_turnTimerType <- "+std::to_string(selected));
        bool found=false;
        const auto result=db->query_rows({"SELECT * FROM TurnTimers WHERE ID=? LIMIT 1",{static_cast<std::int64_t>(selected)}},
            [&](const content::RulesRowView& row){
                current();found=true;
                numbers["ID"]=i32(row.integer("ID"));order.emplace_back("s_turnTimer.m_iID");
                for(const auto* name:{"Type","Text","Civilopedia","Strategy","Help","DisabledHelp"}) {
                    text[name]=row.text(name).value_or("");order.emplace_back(std::string("s_turnTimer.")+name);
                }
                if(!text["Text"].empty()) {
                    text["LocalizedText"]=localized(text["Text"]);
                    order.emplace_back("s_turnTimer.m_strText <- actual GetLocalizedText(Text)");
                }
                const auto description=row.text("Description");
                if(description) {
                    text["DescriptionKey"]=*description;order.emplace_back("s_turnTimer.DescriptionKey");
                    text["Description"]=localized(*description);
                    order.emplace_back("s_turnTimer.m_strDescription <- actual GetLocalizedText(Description)");
                    if(text["Text"].empty()) {
                        text["Text"]=*description;order.emplace_back("s_turnTimer.m_strTextKey <- Description");
                        text["LocalizedText"]=text["Description"];order.emplace_back("s_turnTimer.m_strText <- Description");
                    }
                }
                for(const auto* name:{"BaseTime","CityResource","UnitResource","FirstTurnMultiplier"}) {
                    // Closed Database GetValue missing-column mutation behavior
                    // is not source-proven. Refuse that reached schema branch.
                    if(!row.has_column(name))
                        refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame reached timer GetValue column unavailable");
                    // GetValue(NULL/noninteger,int&) mutation is closed SDK code.
                    // Read only this reached field's storage type, never future fields.
                    // Exactly one selected-ID row binds it to the outer native row.
                    std::optional<std::string> storage;
                    std::size_t storage_rows=0;
                    current();
                    const auto type_result=db->query_rows({std::string("SELECT typeof(")+name+") AS NativeStorageType FROM TurnTimers WHERE ID=?",
                        {static_cast<std::int64_t>(selected)}},[&](const content::RulesRowView& value){
                            current();++storage_rows;storage=value.text("NativeStorageType");current();
                        },2);
                    current();
                    if(!type_result.all_succeeded() || storage_rows!=1 || !storage || *storage!="integer")
                        refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame reached timer GetValue storage semantics unavailable");
                    numbers[name]=i32(row.integer(name));order.emplace_back(std::string("s_turnTimer.")+name);
                }
                current();
            },1);
        current();
        if(!result.all_succeeded() || !found)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame TurnTimers SelectAt/CacheResult unavailable after timer store");
    }
    void setter(std::int32_t selected) {
        speed=selected;order.emplace_back("s_gameSpeed <- "+std::to_string(selected));
        switch(speed) {
            case 1:timer_set(1);break;
            case 2:timer_set(3);break;
            case 3:timer_set(4);break;
            default:break; // Native Marathon/default preserves actual prior timer.
        }
    }
    void advance() {
        current();
        if(menu_attempted && menu_boundary!=BnwFreshPregameMenuBoundary::constructor_slots_reset_complete)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"Selected settings cannot bypass actual ResetSlots frontier");
        if(reset_boundary!=BnwFreshPregameResetBoundary::constructor_diagnostic &&
           reset_boundary!=BnwFreshPregameResetBoundary::reset_game_settings_assignment_required)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"Selected settings require actual resetGame before assignment");
        if(attempted || boundary!=BnwFreshPregameSpeedBoundary::constructed)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"Pregame selected speed setter already attempted");
        attempted=true;boundary=BnwFreshPregameSpeedBoundary::failed_prefix;
        const auto requested=config->selected_speed_input();current();
        const auto types=db->cache_info_types();current();
        if(!types.report().all_succeeded())refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame actual Info maps unavailable");
        bool matched=false;
        const auto result=db->prefetch_info("GameSpeeds",[&](std::uint64_t slot,std::int64_t authored,const content::RulesRowView& row){
            current();const auto type=row.text("Type");
            if(!type)refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame nonnull GameSpeed lacks Type");
            if(ascii_equal(requested,*type)) {
                if(slot>static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
                    refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame native speed slot exceeds int32");
                tuple(types,static_cast<std::int64_t>(slot),authored,*type);matched=true;
                setter(static_cast<std::int32_t>(slot));
            }
            return true;
        },content::InfoCollectionBudget{65536});
        current();
        // A reached Locale error escapes the visitor, retaining prior stores;
        // no return-false/count-cache-failure or alias continuation is permitted.
        if(!result.all_succeeded() || !matched)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame selected speed scan incomplete/missing");
        boundary=BnwFreshPregameSpeedBoundary::selected_speed_setter_complete;
    }
    void init_inputs(BnwFreshPregameInitRequest request) {
        current();
        if(menu_attempted && menu_boundary!=BnwFreshPregameMenuBoundary::constructor_slots_reset_complete)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"Fresh inputs cannot bypass actual ResetSlots frontier");
        if(init_attempted || boundary!=BnwFreshPregameSpeedBoundary::selected_speed_setter_complete)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"Fresh Init inputs require completed SAME speed setter and once owner");
        init_attempted=true;
        // Native setEra(string): every nonnull matching Info in slot order.
        const auto requested=config->selected_era_input();current();
        const auto types=db->cache_info_types();current();
        if(!types.report().all_succeeded())refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame Era Info maps unavailable");
        bool matched=false;
        const auto report=db->prefetch_info("Eras",[&](std::uint64_t slot,std::int64_t authored,const content::RulesRowView& row){
            current();
            if(slot>=65536)refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame Era native span budget exceeded");
            if(era_present.size()<=slot)era_present.resize(static_cast<std::size_t>(slot)+1,false);
            era_present[static_cast<std::size_t>(slot)]=true;
            const auto type=row.text("Type");
            if(!type)refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame occupied Era lacks Type");
            if(ascii_equal(requested,*type)) {
                const auto hash=content::saved_info_type_hash(*type);
                const auto* n=types.find(*type);const auto* h=types.find_hash(hash);
                if(slot!=static_cast<std::uint64_t>(authored) || !n || !h || !same(*n,*h) ||
                   n->table!="Eras" || n->type!=*type || n->id!=authored || n->hash!=hash)
                    refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame Era native tuple ambiguous");
                era=static_cast<std::int32_t>(slot);matched=true;
                order.emplace_back("s_era <- "+std::to_string(era));
            }
            current();return true;
        },content::InfoCollectionBudget{65536});current();
        if(!report.all_succeeded() || !matched || report.slots>65536)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame selected Era scan incomplete/missing");
        era_present.resize(static_cast<std::size_t>(report.slots),false);
        // Explicit request invokes native setters (loadFromIni clock fallback is
        // NOT executed). Keep its seed setter order: sync, map, then game type.
        if(request.sync_seed>std::numeric_limits<std::uint32_t>::max())
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame sync seed exceeds native unsigned32 input");
        sync_seed=static_cast<std::uint32_t>(request.sync_seed);order.emplace_back("s_syncRandomSeed");
        if(request.map_seed>std::numeric_limits<std::uint32_t>::max())
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Pregame map seed exceeds native unsigned32 input");
        map_seed=static_cast<std::uint32_t>(request.map_seed);order.emplace_back("s_mapRandomSeed");
        const auto type=static_cast<std::int32_t>(request.game_type);
        if(type<0 || type>3)refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Fresh GameTypes input outside native domain");
        // Real setGameType(GameTypes,GAME_NEW); no caller start/loaded receipt.
        game_type=type;order.emplace_back("s_gameType");
        game_start_type=0;order.emplace_back("s_gameStartType <- GAME_NEW");
        if(game_type!=1) {internet_game=false;order.emplace_back("s_isInternetGame <- false");}
        current();init_complete=true;
    }
    void verify_era() {
        current();
        if(!init_complete)refuse(BnwFreshPregameSpeedError::wrong_phase,"Game era verification needs SAME actual inputs");
        const auto present=[&](std::int32_t id){return id>=0 && static_cast<std::size_t>(id)<era_present.size() && era_present[static_cast<std::size_t>(id)];};
        if(!present(era)) {
            // Native starts from actual current era-1, visits descending IDs,
            // NULL skip, first nonnull setEra; no fallback to guessed era0.
            for(std::int32_t id=era-1;id>=0;--id) {
                if(present(id)) {era=id;order.emplace_back("Game verify s_era <- "+std::to_string(id));break;}
            }
        }
        current();order.emplace_back("Game getStartEra/current native EraInfo verification");
    }
    void sync_enum_options(const content::RulesDatabase* selected_option_source=nullptr) {
        // Private selected source only; legacy callers keep their original DB.
        const auto* option_source=selected_option_source?selected_option_source:db;
        // Exact ConvertGameOptionTypeToString enumeration order. The SDK
        // DISABLE_START_BIAS slot16 has no case: NULL -> zero without query.
        static constexpr std::array<const char*,22> names{
            "GAMEOPTION_NO_CITY_RAZING","GAMEOPTION_NO_BARBARIANS","GAMEOPTION_RAGING_BARBARIANS",
            "GAMEOPTION_ALWAYS_WAR","GAMEOPTION_ALWAYS_PEACE","GAMEOPTION_ONE_CITY_CHALLENGE",
            "GAMEOPTION_NO_CHANGING_WAR_PEACE","GAMEOPTION_NEW_RANDOM_SEED","GAMEOPTION_LOCK_MODS",
            "GAMEOPTION_COMPLETE_KILLS","GAMEOPTION_NO_GOODY_HUTS","GAMEOPTION_RANDOM_PERSONALITIES",
            "GAMEOPTION_POLICY_SAVING","GAMEOPTION_PROMOTION_SAVING","GAMEOPTION_END_TURN_TIMER_ENABLED",
            "GAMEOPTION_QUICK_COMBAT",nullptr,"GAMEOPTION_NO_SCIENCE","GAMEOPTION_NO_POLICIES",
            "GAMEOPTION_NO_HAPPINESS","GAMEOPTION_NO_TUTORIAL","GAMEOPTION_NO_RELIGION"};
        for(std::size_t i=0;i<names.size();++i) {
            current();std::int32_t value=0;
            if(names[i]) {
                bool found=false;const std::string_view requested=names[i];
                // GetName(bytes) strlen prefix, first saved/current match.
                for(const auto& option:custom_options) {
                    const auto length=option.name.find('\0');
                    const auto name=std::string_view(option.name).substr(0,length);
                    if(requested.starts_with(name)) {value=option.value;found=true;break;}
                }
                if(!found) {
                    // Native Execute/Step false is ignored by Sync: local value
                    // stays0. LIMIT1 owns actual first row, not Info ID or enum ID.
                    bool selected_row=false;
                    const auto report=option_source->query_rows({"Select \"Default\" from GameOptions where Type = ? LIMIT 1",{std::string(requested)}},
                        [&](const content::RulesRowView& row){current();value=i32(row.integer_at(0));selected_row=true;current();},1);
                    current();
                    if(!report.all_succeeded() && !selected_row)order.emplace_back("GetGameOption native Execute/Step false -> initial0");
                }
            }
            enum_values[i]=value;order.emplace_back("s_EnumBasedGameOptions["+std::to_string(i)+"]");
            if(i==15) {quick_combat=value!=0;order.emplace_back("s_quickCombat <- enum15 nonzero");}
        }
    }
    void set_false_option(const char* literal) {
        current();const std::string requested=literal;
        // Fixed source literal64 bounds, actual string setter: no enum aliases.
        if(requested.empty() || requested.size()>=64)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Native CustomOption source name unsupported");
        for(std::size_t i=0;i<custom_options.size();++i) {
            const auto& old=custom_options[i].name;
            const auto prefix=std::string_view(old).substr(0,old.find('\0'));
            if(std::string_view(requested).starts_with(prefix)) {
                custom_options[i]={requested,0};order.emplace_back("s_GameOptions.setAt "+requested);
                // SDK replacement does NOT rebuild/add a hash entry.
                sync_enum_options();return;
            }
        }
        custom_options.push_back({requested,0});order.emplace_back("s_GameOptions.push_back "+requested);
        // Reuse native raw CRC leaf already used for FString Info hashes;
        // same C-string input, no case folding/final complement or new hash.
        option_hash_indices[content::saved_info_type_hash(requested)]=custom_options.size()-1;
        order.emplace_back("s_GameOptionsHash append "+requested);
        sync_enum_options();
    }
    void clear_sp_options() {
        current();
        if(!init_complete || game_type!=0 || options_attempted)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"SP options require SAME actual single-player inputs and once owner");
        options_attempted=true;
        for(std::size_t i=0;i<multiplayer.size();++i) {
            // Real setMultiplayerOption -> FAutoVector.setAt. Since native
            // constructor bools are false, no changed archive SET command.
            if(multiplayer[i]) {multiplayer[i]=false;order.emplace_back("s_multiplayerOptions changed SET "+std::to_string(i));}
            order.emplace_back("setMultiplayerOption["+std::to_string(i)+"] false");
        }
        set_false_option("GAMEOPTION_SIMULTANEOUS_TURNS");
        set_false_option("GAMEOPTION_DYNAMIC_TURNS");
        set_false_option("GAMEOPTION_PITBOSS");
        current();options_complete=true;
        // Independent hotseat/email native condition is false for actual SP.
        // No unreachable second set of three setter calls is performed.
    }
    const std::vector<BnwFreshPregameTurnIncrement>& turn_increments() {
        current();
        if(!options_complete)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"Turn info requires SAME completed SP options");
        const auto types=db->cache_info_types();current();
        if(!types.report().all_succeeded())
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"GameSpeed getter native maps unavailable");
        std::optional<std::string> selected_type;
        const auto infos=db->prefetch_info("GameSpeeds",[&](std::uint64_t slot,std::int64_t authored,const content::RulesRowView& row){
            current();
            if(slot==static_cast<std::uint64_t>(speed)) {
                const auto type=row.text("Type");
                if(!type)refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Actual selected GameSpeed lacks Type");
                tuple(types,speed,authored,*type);selected_type=*type;
            }
            current();return true;
        },content::InfoCollectionBudget{65536});current();
        if(!infos.all_succeeded() || !selected_type)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Actual selected GameSpeedInfo is NULL");
        // Preserve native fixed256 sprintf query and SQLite row order. Quoted
        // or overlong source types require a separate original SQL-buffer proof.
        if(selected_type->size()>160 || selected_type->find('\'')!=std::string::npos)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"GameSpeed native turn query source unsupported");
        for(const char c:*selected_type)if(static_cast<unsigned char>(c)>127)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"GameSpeed native SQL source must be bounded ASCII");
        if(turn_cache_attempted)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"Turn cache child already attempted; no prefix replay");
        turn_cache_attempted=true;
        bool count_row=false;
        const auto counts=db->query_rows({"select count(*) from GameSpeed_Turns where GameSpeedType = '"+*selected_type+"'",{}},
            [&](const content::RulesRowView& row){current();turn_count=i32(row.integer_at(0));count_row=true;current();},1);
        current();
        if(turn_count<0 || turn_count>65536)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"GameSpeed turn allocation outside native bounded extent");
        // Original constructor count0 survives Execute failure. A successful
        // aggregate must supply its actual row; no guessed nonzero counts.
        if(counts.all_succeeded() && !count_row)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"GameSpeed turn count row unavailable");
        turn_cache.emplace(static_cast<std::size_t>(turn_count));
        auto& values=*turn_cache;
        std::size_t at=0;
        const auto rows=db->query_rows({"select * from GameSpeed_Turns where GameSpeedType = '"+*selected_type+"'",{}},
            [&](const content::RulesRowView& row){
                current();if(at>=values.size())
                    refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Native turn CacheResults exceeds actual allocated count");
                // Both writes are actual CacheResults stores, not future Game
                // reads. Failed select leaves allocated slots UNWRITTEN.
                auto& value=values[at++];value.month_increment=i32(row.integer("MonthIncrement"));
                value.turns=i32(row.integer("TurnsPerIncrement"));current();
            },65537);
        current();
        if(!rows.all_succeeded() && at!=0)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Native turn cache partial Step failure unavailable");
        return values;
    }
    std::int32_t train() const {
        current();
        if(boundary!=BnwFreshPregameSpeedBoundary::selected_speed_setter_complete)
            refuse(BnwFreshPregameSpeedError::wrong_phase,"GameSpeed getter requires completed actual selected setter");
        const auto types=db->cache_info_types();current();
        if(!types.report().all_succeeded())refuse(BnwFreshPregameSpeedError::native_source_unavailable,"GameSpeed getter Info maps unavailable");
        std::optional<std::int32_t> value;
        const auto result=db->prefetch_info("GameSpeeds",[&](std::uint64_t slot,std::int64_t authored,const content::RulesRowView& row){
            current();
            if(slot==static_cast<std::uint64_t>(speed)) {
                const auto type=row.text("Type");
                if(!type)refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Actual GameSpeed Info lacks Type");
                tuple(types,speed,authored,*type);
                // Native CvGameSpeedInfo CacheResults GetInt missing=>0.
                value=i32(row.integer("TrainPercent"));
            }
            return true;
        },content::InfoCollectionBudget{65536});current();
        if(!result.all_succeeded() || !value)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Actual selected GameSpeedInfo is NULL/unavailable");
        return *value;
    }
};
BnwFreshPregameSpeedRefusal::BnwFreshPregameSpeedRefusal(BnwFreshPregameSpeedError e,const std::string& s):std::runtime_error(s),code_(e){}
BnwFreshPregameSpeed::BnwFreshPregameSpeed(std::shared_ptr<Impl> p):impl_(std::move(p)){}
std::shared_ptr<BnwFreshPregameSpeed> BnwFreshPregameSpeed::from_creation_config(std::shared_ptr<const BnwCreationConfig> c,std::filesystem::path selected_config_ini) {
    if(!c)refuse(BnwFreshPregameSpeedError::parent_missing,"Pregame speed needs actual selected Config");
    c->require_current();auto p=std::make_shared<Impl>(std::move(selected_config_ini));p->config=std::move(c);p->db=&p->config->game_rules();p->current();
    auto result=std::shared_ptr<BnwFreshPregameSpeed>(new BnwFreshPregameSpeed(std::move(p)));
    result->impl_->actual_facade=result;
    // Namespace slot/world storage belongs to this SAME actual Pregame realm.
    // Direct friend constructor; no supplied count, boolean or success receipt.
    result->impl_->roster=std::shared_ptr<BnwFreshPregameRoster>(
        new BnwFreshPregameRoster(result->impl_->config,*result));
    result->impl_->current();return result;
}
std::shared_ptr<BnwFreshPregameSpeed> BnwFreshPregameSpeed::from_creation_config_for_reset_setup(
    std::shared_ptr<const BnwCreationConfig> c,std::filesystem::path selected_config_ini) {
    auto result=from_creation_config(std::move(c),std::move(selected_config_ini));
    result->impl_->issue_reset_inputs();return result;
}

void BnwFreshPregameSpeed::advance_ordered_globals_info_prefix_from_game() {
 const auto allocation=impl_;const auto held=allocation->same_facade(*this);
 if(allocation->ordered_info || allocation->menu_attempted || allocation->attempted || allocation->init_attempted || allocation->options_attempted)
  refuse(BnwFreshPregameSpeedError::wrong_phase,"Ordered Globals cache must precede SAME menu/namespace mutations once");
 allocation->issue_cached_globals_subset();allocation->same_facade(*this);
 allocation->ordered_info=BnwFreshCivilizationInfoCollection::from_same_pregame(held,*allocation->globals_database,*allocation->globals_types);
 allocation->same_facade(*this);allocation->ordered_info->advance_ordered_prefix();
 refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Ordered Globals returned without full original queue/tail completion");
}
BnwFreshOrderedInfoObservation BnwFreshPregameSpeed::ordered_info_observation_from_game()const {
 const auto allocation=impl_;const auto held=allocation->same_facade(*this);(void)held;
 if(!allocation->ordered_info)refuse(BnwFreshPregameSpeedError::wrong_phase,"Actual ordered Globals child not issued");
 auto out=allocation->ordered_info->observe();allocation->same_facade(*this);return out;
}
std::string BnwFreshPregameSpeed::localized_for_ordered_info(std::string_view key) {
 const auto allocation=impl_;const auto held=allocation->same_facade(*this);(void)held;
 if(!allocation->ordered_info || !allocation->globals_database || !allocation->globals_types)
  refuse(BnwFreshPregameSpeedError::wrong_phase,"Ordered Info Locale has no SAME issued Globals child");
 auto value=allocation->localized(key);allocation->same_facade(*this);return value;
}

void BnwFreshPregameSpeed::advance_reset_game(){impl_->reset_game();}
void BnwFreshPregameSpeed::advance_single_player_menu_prefix() {
    const auto held=impl_->same_facade(*this);
    if(impl_->menu_attempted || impl_->attempted || impl_->init_attempted || impl_->options_attempted ||
       impl_->reset_boundary!=BnwFreshPregameResetBoundary::constructor_diagnostic ||
       impl_->menu_boundary!=BnwFreshPregameMenuBoundary::menu_request_required || !impl_->roster)
        refuse(BnwFreshPregameSpeedError::wrong_phase,"Actual single-player menu prefix requires unmodified constructor realm once");
    impl_->menu_attempted=true;impl_->menu_boundary=BnwFreshPregameMenuBoundary::failed_prefix;
    impl_->issue_cached_globals_subset();
    impl_->private_game=false;impl_->order.emplace_back("SinglePlayer SetPrivateGame false");
    impl_->game_type=0;impl_->order.emplace_back("SinglePlayer SetGameType GAME_SINGLE_PLAYER");
    // The one-argument native setter does not store s_gameStartType.
    impl_->internet_game=false;impl_->order.emplace_back("SinglePlayer SetGameType s_isInternetGame false");
    impl_->current();
    impl_->roster->reset_constructor_slots_from_pregame(*held);
    impl_->current();impl_->menu_boundary=BnwFreshPregameMenuBoundary::reset_game_options_required;
    refuse(BnwFreshPregameSpeedError::native_source_unavailable,
        "SinglePlayer menu requires actual ResetGameOptions then ResetMapOptions and LoadPreGameSettings");
}
void BnwFreshPregameSpeed::advance_single_player_menu_options_prefix() {
    const auto held=impl_->same_facade(*this);(void)held;
    if(impl_->menu_boundary!=BnwFreshPregameMenuBoundary::reset_game_options_required ||
       !impl_->menu_attempted || impl_->menu_options_attempted || !impl_->roster ||
       impl_->cached_globals_boundary!=BnwFreshPregameCachedGlobalsBoundary::selected_database_subset_stored ||
       !impl_->globals_database || !impl_->globals_types)
        refuse(BnwFreshPregameSpeedError::wrong_phase,"Menu options require actual SAME ResetSlots return, once, not a failed127 resume");
    impl_->roster->require_current();
    impl_->menu_options_attempted=true;impl_->menu_boundary=BnwFreshPregameMenuBoundary::failed_prefix;
    // Stock SinglePlayer.lua: ResetGameOptions BEFORE ResetMapOptions.
    impl_->custom_options.clear();impl_->order.emplace_back("SinglePlayer ResetGameOptions s_GameOptions.clear");
    impl_->option_hash_indices.clear();impl_->order.emplace_back("SinglePlayer ResetGameOptions s_GameOptionsHash.clear");
    // Native rehash loop sees an actually empty vector after clear.
    impl_->sync_enum_options(impl_->globals_database.get());
    // Reached actual collection producer at GetNumVictoryInfos, not before Sync.
    const auto report=impl_->globals_database->prefetch_info("Victories",
        [&](std::uint64_t slot,std::int64_t id,const content::RulesRowView&) {
            impl_->current();(void)i32(id);
            if(slot>=65536)refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Menu native Victory span exceeds bounded allocation");
            return true;
        },content::InfoCollectionBudget{65536});impl_->current();
    if(!report.all_succeeded() || report.slots>65536)
        refuse(BnwFreshPregameSpeedError::native_source_unavailable,"Menu actual selected Victory collection unavailable");
    impl_->native_victory_span=i32(static_cast<std::int64_t>(report.slots));
    impl_->reset_victories("SinglePlayer ResetGameOptions ");
    impl_->map_options.clear();impl_->order.emplace_back("SinglePlayer ResetMapOptions s_MapOptions.clear");
    impl_->current();impl_->menu_boundary=BnwFreshPregameMenuBoundary::load_pregame_settings_required;
    refuse(BnwFreshPregameSpeedError::native_source_unavailable,
        "SinglePlayer menu requires actual LoadPreGameSettings after ResetGameOptions and ResetMapOptions");
}


BnwFreshPregameEnvironmentInfoObservation BnwFreshPregameSpeed::selected_climate_info()const {
    const auto allocation=impl_;const auto facade=allocation->same_facade(*this);(void)facade;
    return allocation->environment_info(*this,true);
}
BnwFreshPregameEnvironmentInfoObservation BnwFreshPregameSpeed::selected_sea_level_info()const {
    const auto allocation=impl_;const auto facade=allocation->same_facade(*this);(void)facade;
    return allocation->environment_info(*this,false);
}


BnwFreshPregameRosterObservation BnwFreshPregameSpeed::selected_world_info()const {
    const auto allocation=impl_;const auto facade=allocation->same_facade(*this);(void)facade;
    allocation->environment_current(*this);
    if(!allocation->roster)
        refuse(BnwFreshPregameSpeedError::parent_missing,"WorldInfo getter lost SAME private native cache");
    if(allocation->world_setter_active || allocation->world_getter_active || allocation->world_getter_failed ||
       allocation->environment_getter_failed || allocation->climate_info.active || allocation->sea_level_info.active ||
       allocation->world_setter.boundary==BnwFreshPregameWorldSetterBoundary::failed_prefix ||
       allocation->climate_info.boundary==BnwFreshPregameEnvironmentSetterBoundary::failed_prefix ||
       allocation->sea_level_info.boundary==BnwFreshPregameEnvironmentSetterBoundary::failed_prefix)
        refuse(BnwFreshPregameSpeedError::wrong_phase,"WorldInfo getter cannot borrow active or failed native cache");
    auto value=allocation->roster->observe();allocation->environment_current(*this);
    // CvPreGame::worldInfo3138 compares the real namespace world size with
    // the SAME retained cache ID. An equal pair performs no SQL or Locale.
    if(value.world_size!=value.world_info_id) {
        if(!allocation->globals_database ||
           allocation->cached_globals_boundary!=BnwFreshPregameCachedGlobalsBoundary::selected_database_subset_stored)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"WorldInfo mismatch lacks SAME actual selected SQL allocation");
        allocation->world_getter_active=true;
        try {
            bool found=false;const auto selected=value.world_size;
            // The original numeric SelectAt is ROWID; the numeric SETTER
            // separately uses authored ID. Neither can substitute the other.
            const auto report=allocation->globals_database->query_rows(
                {"select * from Worlds where ROWID = ? LIMIT 1",{static_cast<std::int64_t>(selected)}},
                [&](const content::RulesRowView& row) {
                    allocation->environment_current(*this);found=true;
                    allocation->roster->cache_world_from_selected_row(*this,row);
                    allocation->environment_current(*this);
                    // Getter does NOT publish s_worldSize or call ResetSlots.
                },1);
            allocation->environment_current(*this);
            if(!report.all_succeeded() || !found)
                refuse(BnwFreshPregameSpeedError::native_source_unavailable,"WorldInfo numeric ROWID getter lacks actual selected row");
            value=allocation->roster->observe();allocation->environment_current(*this);
            allocation->world_getter_active=false;
        }catch(...) {allocation->world_getter_active=false;allocation->world_getter_failed=true;throw;}
    }
    allocation->environment_current(*this);return value;
}

void BnwFreshPregameSpeed::set_selected_climate(std::string selected_type) {
    const auto allocation=impl_;const auto facade=allocation->same_facade(*this);(void)facade;
    allocation->set_environment(*this,true,std::move(selected_type));
}
void BnwFreshPregameSpeed::set_selected_sea_level(std::string selected_type) {
    const auto allocation=impl_;const auto facade=allocation->same_facade(*this);(void)facade;
    allocation->set_environment(*this,false,std::move(selected_type));
}
BnwFreshPregameEnvironmentObservation BnwFreshPregameSpeed::environment_observation()const {
    const auto allocation=impl_;allocation->environment_current(*this);
    const auto value=BnwFreshPregameEnvironmentObservation{allocation->climate_info.observe(),allocation->sea_level_info.observe()};
    allocation->environment_current(*this);return value;
}

void BnwFreshPregameSpeed::set_selected_world_size(std::string selected_type) {
    const auto held=impl_->same_facade(*this);(void)held;
    if(impl_->menu_boundary!=BnwFreshPregameMenuBoundary::load_pregame_settings_required ||
       !impl_->roster || !impl_->globals_database ||
       impl_->cached_globals_boundary!=BnwFreshPregameCachedGlobalsBoundary::selected_database_subset_stored ||
       impl_->world_setter_active || impl_->world_getter_active || impl_->world_getter_failed ||
       impl_->climate_info.active || impl_->sea_level_info.active ||
       impl_->world_setter.boundary==BnwFreshPregameWorldSetterBoundary::failed_prefix)
        refuse(BnwFreshPregameSpeedError::wrong_phase,"World string setter requires actual menu prefix and no failed/reentrant call");
    impl_->roster->require_current();
    if(impl_->game_started)
        refuse(BnwFreshPregameSpeedError::native_source_unavailable,"World setter started-game assertion requires actual live Game/network owner");
    if(impl_->world_setter.call_count==std::numeric_limits<std::uint64_t>::max())
        refuse(BnwFreshPregameSpeedError::native_source_unavailable,"World setter operation index overflow");
    impl_->world_setter.requested_type=std::move(selected_type);
    ++impl_->world_setter.call_count;
    impl_->world_setter.boundary=BnwFreshPregameWorldSetterBoundary::failed_prefix;
    impl_->world_setter_active=true;
    try {
        bool found=false;
        const auto report=impl_->globals_database->query_rows(
            {"SELECT * from Worlds where Type = ? LIMIT 1",{impl_->world_setter.requested_type}},
            [&](const content::RulesRowView& row) {
                impl_->same_facade(*this);found=true;
                impl_->roster->cache_world_from_selected_row(*this,row);
                impl_->roster->publish_world_size_from_cache(*this);
                impl_->world_setter.boundary=BnwFreshPregameWorldSetterBoundary::cache_stored;
                impl_->roster->reset_selected_world_slots_from_pregame(*this);
                impl_->same_facade(*this);
            },1);
        impl_->same_facade(*this);
        if(!report.all_succeeded() || !found)
            refuse(BnwFreshPregameSpeedError::native_source_unavailable,"World string setter actual SQL/Type row unavailable; no guessed world");
        impl_->world_setter_active=false;
        impl_->world_setter.boundary=BnwFreshPregameWorldSetterBoundary::slots_reset_returned;
    } catch(...) {
        impl_->world_setter_active=false;
        impl_->world_setter.boundary=BnwFreshPregameWorldSetterBoundary::failed_prefix;
        throw;
    }
}
std::shared_ptr<const BnwFreshPregameRoster> BnwFreshPregameSpeed::roster() const {
    const auto held=impl_->same_facade(*this);
    if(!impl_->roster)refuse(BnwFreshPregameSpeedError::parent_missing,"Pregame lost actual retained roster");
    impl_->roster->require_current();
    // Retain both allocations: public facade assignment cannot free a pointer
    // already returned in an alias. The old sibling then refuses its stale slot.
    struct RosterLease {std::shared_ptr<BnwFreshPregameSpeed> facade;std::shared_ptr<Impl> allocation;};
    auto lease=std::make_shared<RosterLease>(RosterLease{held,impl_});
    return std::shared_ptr<const BnwFreshPregameRoster>(std::move(lease),impl_->roster.get());
}
std::int32_t BnwFreshPregameSpeed::current_game_type_for_roster() const {
    (void)impl_->same_facade(*this);return impl_->game_type;
}
std::int32_t BnwFreshPregameSpeed::current_cached_standard_handicap_for_roster() const {
    (void)impl_->same_facade(*this);
    if(impl_->cached_globals_boundary!=BnwFreshPregameCachedGlobalsBoundary::selected_database_subset_stored)
        refuse(BnwFreshPregameSpeedError::wrong_phase,"ResetPlayer requires the SAME earlier stored selected Globals subset");
    return impl_->cached_standard_handicap;
}
std::weak_ptr<BnwFreshPregameSpeed> BnwFreshPregameSpeed::current_parent_for_roster() const {
    return impl_->same_facade(*this);
}
std::int32_t BnwFreshPregameSpeed::current_cached_ai_handicap_for_roster() const {
    (void)impl_->same_facade(*this);
    if(impl_->cached_globals_boundary!=BnwFreshPregameCachedGlobalsBoundary::selected_database_subset_stored)
        refuse(BnwFreshPregameSpeedError::wrong_phase,"AI handicap requires SAME reached selected Globals cache");
    return impl_->cached_ai_handicap;
}
std::string BnwFreshPregameSpeed::current_world_localized_for_roster(std::string_view key) const {
    (void)impl_->same_facade(*this);
    if(!impl_->world_setter_active && !impl_->world_getter_active)
        refuse(BnwFreshPregameSpeedError::wrong_phase,"WorldInfo localization requires actual active cache call");
    return impl_->localized(key);
}
std::uint64_t BnwFreshPregameSpeed::current_world_reset_operation_for_roster() const {
    (void)impl_->same_facade(*this);
    if(!impl_->world_setter_active || impl_->world_setter.boundary!=BnwFreshPregameWorldSetterBoundary::cache_stored)
        refuse(BnwFreshPregameSpeedError::wrong_phase,"World ResetSlots requires SAME private completed CacheResult");
    return impl_->world_setter.call_count;
}
void BnwFreshPregameSpeed::require_same_roster(const BnwFreshPregameRoster& sibling) const {
    // No call back into sibling.require_current: this is the actual slot guard.
    if(impl_->thread!=std::this_thread::get_id())refuse(BnwFreshPregameSpeedError::wrong_thread,"Pregame sibling belongs to another thread");
    const auto held=impl_->actual_facade.lock();
    if(!held || held.get()!=this || impl_->roster.get()!=&sibling)
        refuse(BnwFreshPregameSpeedError::retired,"Pregame sibling actual allocation/slot retired");
    impl_->current();
}
void BnwFreshPregameSpeed::advance_selected_speed(){impl_->advance();}
void BnwFreshPregameSpeed::advance_selected_init_inputs(BnwFreshPregameInitRequest r){impl_->init_inputs(r);}
BnwFreshPregameInitObservation BnwFreshPregameSpeed::current_init_inputs() const {
    impl_->current();if(!impl_->init_complete)refuse(BnwFreshPregameSpeedError::wrong_phase,"Game fresh inputs not completed");
    return impl_->init_observe();
}
void BnwFreshPregameSpeed::verify_start_era_from_game(){impl_->verify_era();}
void BnwFreshPregameSpeed::clear_single_player_options_from_game(){impl_->clear_sp_options();}
bool BnwFreshPregameSpeed::current_shuffle_teams_from_game() const {
    impl_->current();if(!impl_->options_complete)refuse(BnwFreshPregameSpeedError::wrong_phase,"ShuffleTeams read needs SAME completed SP options");
    return impl_->multiplayer[2];
}
bool BnwFreshPregameSpeed::current_lock_mods_from_game() const {
    impl_->current();if(!impl_->options_complete)
        refuse(BnwFreshPregameSpeedError::wrong_phase,"LOCK_MODS requires actual completed SP options");
    return impl_->enum_values[8]==1; // Native isOption ==1, not generic nonzero.
}
std::int32_t BnwFreshPregameSpeed::current_game_turn_from_game() const {
    impl_->current();return impl_->game_turn;
}
void BnwFreshPregameSpeed::set_admin_password_from_game(std::string value) {
    impl_->current();if(!impl_->options_complete || impl_->game_type!=0)
        refuse(BnwFreshPregameSpeedError::wrong_phase,"Admin setter requires actual SP Game prerequisite");
    impl_->admin_password=std::move(value);impl_->order.emplace_back("s_adminPassword <- actual native C-string password");
    impl_->current();
}
const std::vector<BnwFreshPregameTurnIncrement>& BnwFreshPregameSpeed::current_turn_increments_from_game() {return impl_->turn_increments();}
void BnwFreshPregameSpeed::require_current() const {impl_->current();}
BnwFreshPregameSpeedObservation BnwFreshPregameSpeed::observe() const {impl_->current();return {impl_->boundary,impl_->speed,impl_->timer,impl_->numbers,impl_->text,impl_->order,impl_->init_observe(),impl_->options_observe(),impl_->game_turn,impl_->admin_password,impl_->turn_count,impl_->turn_cache,impl_->reset_observe(),{impl_->menu_boundary,impl_->private_game,impl_->menu_options_attempted,impl_->map_options},impl_->cached_globals_observe(),impl_->world_setter};}
std::int32_t BnwFreshPregameSpeed::current_train_percent() const {return impl_->train();}
const BnwCreationConfig* BnwFreshPregameSpeed::actual_config() const {impl_->current();return impl_->config.get();}
} // namespace civ5::runtime
