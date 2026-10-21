#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/runtime/pregame_roster.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/live_session.hpp"
#include <climits>
#include <utility>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
namespace civ5::runtime {
namespace {
class Query final:public ScriptQuery {
    std::unique_ptr<content::RulesScriptCursor> cursor_;
  public:
    explicit Query(std::unique_ptr<content::RulesScriptCursor> cursor):cursor_(std::move(cursor)){}
    std::optional<ScriptRow> next()override{return cursor_->next();}
    bool step()override{return cursor_->step();}
    ScriptRow row()const override{return cursor_->row();}
    std::pair<bool,ScriptValue> cache_key(bool id)const override{return cursor_->cache_key(id);}
    void reset()override{cursor_->reset();}
    // Native GameInfo ignores individual Bind return values, including a
    // busy statement retained after a caught Lua row-cache error.
    void bind(std::span<const ScriptValue> args)override{cursor_->bind(args,true);}
    void release()override{cursor_->release();}
    std::string_view error()const override{return cursor_->error();}
};
class SavedConnection final:public ScriptSaveDataConnection {
    std::unique_ptr<content::RulesDatabase> database_;
  public:
    explicit SavedConnection(std::unique_ptr<content::RulesDatabase> database):database_(std::move(database)){}
    std::unique_ptr<ScriptQuery> query(std::string_view sql,std::span<const ScriptValue> args)override {
        auto cursor=database_->script_query(sql,args);
        return cursor?std::make_unique<Query>(std::move(cursor)):nullptr;
    }
};
struct UserDataState {
    using Key=std::pair<std::string,std::int64_t>;
    std::filesystem::path root;
    std::map<Key,std::shared_ptr<content::RulesDatabase>> databases;
    std::set<Key> deleted;
    explicit UserDataState(std::filesystem::path source_root){
        if(!source_root.empty()){
            root=std::filesystem::canonical(source_root);
            if(!std::filesystem::is_directory(root))throw ScriptError("Mod user-data source is not a directory");
        }
    }
    static Key key(std::string_view name,std::int64_t version){
        if(name.empty()||name=="."||name==".."||version<0||name.find('\0')!=std::string_view::npos||
           name.find_first_of("\\/:*?\"<>|")!=std::string_view::npos)
            throw ScriptError("Invalid mod user-data name or version");
        return {std::string(name),version};
    }
    std::unique_ptr<ScriptSaveDataConnection> open(std::string_view name,std::int64_t version){
        const auto id=key(name,version);
        auto found=databases.find(id);
        if(found==databases.end()){
            std::shared_ptr<content::RulesDatabase> database;
            if(!root.empty()&&!deleted.contains(id)){
                const auto filename=id.first+"-"+std::to_string(version)+".db";
                const auto candidate=root/std::filesystem::path(std::u8string(filename.begin(),filename.end()));
                if(std::filesystem::exists(candidate)){
                    const auto resolved=std::filesystem::canonical(candidate);
                    if(resolved.parent_path()!=root||!std::filesystem::is_regular_file(resolved))
                        throw ScriptError("Mod user-data source escapes its directory");
                    std::ifstream input(resolved,std::ios::binary);
                    if(!input)throw ScriptError("Cannot read mod user-data source");
                    const std::vector<std::uint8_t> image(std::istreambuf_iterator<char>{input},{});
                    if(!input.eof()&&input.fail())throw ScriptError("Cannot read complete mod user-data source");
                    database=content::RulesDatabase::from_saved_image(image);
                }
            }
            if(!database)database=content::RulesDatabase::new_saved_values();
            found=databases.emplace(id,std::move(database)).first;
        }
        return std::make_unique<SavedConnection>(found->second->reopen_saved());
    }
    void erase(std::string_view name,std::int64_t version){
        const auto id=key(name,version);
        databases.erase(id);
        deleted.insert(id);
    }
};
}
ScriptSaveDataProvider lua_saved_data_services(std::shared_ptr<content::RulesDatabase> database){
    return {[database=std::move(database)]()->std::unique_ptr<ScriptSaveDataConnection>{
        return database?std::make_unique<SavedConnection>(database->reopen_saved()):nullptr;
    }};
}
ScriptUserDataProvider lua_user_data_services(std::filesystem::path source_root){
    auto state=std::make_shared<UserDataState>(std::move(source_root));
    return {
        [state](std::string_view name,std::int64_t version){return state->open(name,version);},
        [state](std::string_view name,std::int64_t version){state->erase(name,version);}
    };
}
namespace {
template<class Players>
ScriptGameplayProvider gameplay_services(const content::RulesDatabase &database,
        Players &players,const std::vector<TeamTechnologies> &teams,const content::InfoTypeCatalog &types,
        std::function<bool(std::int64_t,std::int64_t)> human,const PregameRoster *roster){
    return {
        [&database](std::string_view sql,std::span<const ScriptValue> args)->std::unique_ptr<ScriptQuery>{
            auto cursor=database.script_query(sql,args);
            return cursor?std::make_unique<Query>(std::move(cursor)):nullptr;
        },
        [&players,&teams](bool team_objects){
            std::vector<std::int64_t> ids;
            if(team_objects)for(const auto &team:teams)ids.push_back(team.team());
            else for(const auto &[id,state]:players.players){(void)state;ids.push_back(id);}
            return ids;
        },
        [&teams](std::int64_t team,std::int64_t tech){
            const auto found=std::find_if(teams.begin(),teams.end(),[&](const auto &value){return value.team()==team;});
            if(found==teams.end())throw ScriptError("Lua team no longer exists in shared state");
            return found->has_tech(tech);
        },
        [&players](std::int64_t player){return players.players.at(player).team;},
        [&players](std::int64_t player){return players.players.at(player).alive;},
        roster?std::function<std::int64_t(std::int64_t)>([roster](std::int64_t player){return roster->slots.at(player).leader;}):
            std::function<std::int64_t(std::int64_t)>{},
        [&types]{
            std::vector<std::pair<std::string,std::int64_t>> values;
            for(const auto &[name,entry]:types.entries())values.emplace_back(name,entry.id);
            return values;
        },
        [&players]{return players.game_turn;},
        std::move(human),players.profile==content::TechnologyProfile::vp542,
        [&players]{return players.speed;},
        [&players](std::int64_t player){return players.players.at(player).ever_alive;},
        roster?std::function<bool(std::int64_t)>([roster](std::int64_t player){return roster->slots.at(player).minor;}):
            std::function<bool(std::int64_t)>{},
        roster?std::function<std::int64_t(std::int64_t)>([roster](std::int64_t player){return roster->slots.at(player).minor_type;}):
            std::function<std::int64_t(std::int64_t)>{},
        roster?std::function<std::int64_t(std::int64_t)>([roster](std::int64_t player){return roster->slots.at(player).civilization;}):
            std::function<std::int64_t(std::int64_t)>{},
        {}, // unit provider remains opt-in
        [&database]{return database.script_statement_count();},
        [&database](std::int64_t index){return database.script_statement_sql(index);},
        {},{} // original dedicated Database heap producer remains unissued
    };
}
}
ScriptGameplayProvider lua_gameplay_services(const content::RulesDatabase &database,
        ResearchContext &players,const std::vector<TeamTechnologies> &teams,const content::InfoTypeCatalog &types,
        const content::CustomModOptions *options,const PregameRoster *roster){
    auto human=[&players,&database,options](std::int64_t id,std::int64_t reason){
            const auto &player=players.players.at(id);
            // Inactive open slots are also exposed to Lua. Resolve the lazy
            // source option when first queried, retaining its owner-side cache.
            if(player.slot_status==0&&players.game_type!=2&&!players.pitboss)
                players.pitboss=players.game_option_value(database,"GAMEOPTION_PITBOSS")!=0;
            if(!players.is_human(player))return false;
            if(players.profile==content::TechnologyProfile::vp542){
                // BasicLuaMethod converts an omitted reason to zero (city
                // management), not the C++ default OTHER_ISHUMAN_REASON=21.
                constexpr const char *names[]={"AI_CONTROL_CITY_MANAGEMENT","AI_CONTROL_CITY_PRODUCTION","AI_CONTROL_DIPLOMACY",
                    "AI_CONTROL_ECONOMY","AI_CONTROL_ESPIONAGE","AI_CONTROL_EVENT_CHOICE","AI_CONTROL_FAITH_SPENDING",
                    "AI_CONTROL_GREAT_PERSON_CHOICE","AI_CONTROL_POLICY_CHOICE","AI_CONTROL_RELIGION_CHOICE","AI_CONTROL_TECH_CHOICE",
                    "AI_CONTROL_TOURISM","AI_CONTROL_UNIT_PROMOTIONS","AI_CONTROL_UNITS","AI_CONTROL_WORLD_CONGRESS",
                    nullptr,"HUMAN_USES_AI_HANDICAP",nullptr,"HUMAN_USES_AI_MECHANICS"};
                if(reason>=0&&reason<static_cast<std::int64_t>(std::size(names))&&names[reason]){
                    if(!options||!options->all_succeeded())throw ScriptError("VP IsHuman reason requires resolved CustomMods options");
                    return !options->enabled(names[reason]);
                }
            }
            return true;
        };
    return gameplay_services(database,players,teams,types,std::move(human),roster);
}
ScriptGameplayProvider lua_gameplay_read_services(const content::RulesDatabase& database,
        std::shared_ptr<const LiveSessionState> world,const content::InfoTypeCatalog& types,
        const PregameRoster* roster){
    if(!world||world->setup.profile!=content::TechnologyProfile::bnw)
        throw ScriptError("Stock Lua read services require retained BNW state");
    auto provider=gameplay_services(database,world->setup,world->technologies,types,
        [world,&database](std::int64_t id,std::int64_t){
            if(id<0||id>=64)throw ScriptError("Stock Lua native player index unavailable");
            return detail::bnw_player_human_read(*world,static_cast<std::int32_t>(id),database);
        },roster);
    // Every available closure retains SAME state; no temporary borrowed refs.
    const auto retain=[world](auto callback){
        return [world,callback=std::move(callback)](auto&&... args)->decltype(auto){
            if(!world)throw ScriptError("Stock Lua retained read lease absent");
            return callback(std::forward<decltype(args)>(args)...);
        };
    };
    if(provider.query)provider.query=retain(std::move(provider.query));
    if(provider.db_statement_count)provider.db_statement_count=retain(std::move(provider.db_statement_count));
    if(provider.db_statement_sql)provider.db_statement_sql=retain(std::move(provider.db_statement_sql));
    if(provider.db_memory_usage)provider.db_memory_usage=retain(std::move(provider.db_memory_usage));
    if(provider.db_collect_memory_usage)provider.db_collect_memory_usage=retain(std::move(provider.db_collect_memory_usage));
    if(provider.ids)provider.ids=retain(std::move(provider.ids));
    if(provider.has_tech)provider.has_tech=retain(std::move(provider.has_tech));
    if(provider.player_team)provider.player_team=retain(std::move(provider.player_team));
    if(provider.player_alive)provider.player_alive=retain(std::move(provider.player_alive));
    if(provider.player_leader)provider.player_leader=retain(std::move(provider.player_leader));
    if(provider.info_types)provider.info_types=retain(std::move(provider.info_types));
    if(provider.game_turn)provider.game_turn=retain(std::move(provider.game_turn));
    if(provider.player_human)provider.player_human=retain(std::move(provider.player_human));
    if(provider.game_speed)provider.game_speed=retain(std::move(provider.game_speed));
    if(provider.player_ever_alive)provider.player_ever_alive=retain(std::move(provider.player_ever_alive));
    if(provider.player_minor)provider.player_minor=retain(std::move(provider.player_minor));
    if(provider.player_minor_type)provider.player_minor_type=retain(std::move(provider.player_minor_type));
    if(provider.player_civilization)provider.player_civilization=retain(std::move(provider.player_civilization));
    return provider;
}
} // namespace civ5::runtime
