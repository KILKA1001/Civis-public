#include "civ5/ui/civis_civilopedia_units_query_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <fstream>
#include <limits>
#include <thread>

namespace civ5::ui {
namespace {
constexpr std::string_view units_sql=
    "\t\tSELECT Units.ID, Units.Description, Units.PortraitIndex, Units.IconAtlas \n"
    "\t\tFROM Units INNER JOIN Technologies on PreReqTech = Technologies.Type \n"
    "\t\tWHERE (Units.FaithCost = 0 or RequiresFaithPurchaseEnabled or Units.Cost >= 0) and Units.ShowInPedia = 1 and Technologies.Era = ?;";
constexpr std::string_view buildings_sql=
    "\t\tSELECT Buildings.ID, Buildings.Description, Buildings.PortraitIndex, Buildings.IconAtlas \n"
    "\t\tFROM Buildings \n"
    "\t\tINNER JOIN BuildingClasses ON Buildings.BuildingClass = BuildingClasses.Type \n"
    "\t\tINNER JOIN Technologies ON Buildings.PrereqTech = Technologies.Type \n"
    "\t\tWHERE (FaithCost == 0 or Buildings.Cost >= 0) AND BuildingClasses.MaxGlobalInstances < 0 AND (BuildingClasses.MaxPlayerInstances <> 1 or Buildings.SpecialistCount > 0) AND BuildingClasses.MaxTeamInstances < 0 AND Technologies.Era = ?;";
std::string decode_long_string(std::string_view literal){
    std::string decoded;
    for(std::size_t i=0;i<literal.size();++i){
        const auto c=literal[i];
        if(c=='\r'||c=='\n'){
            if(i+1<literal.size()&&(literal[i+1]=='\r'||literal[i+1]=='\n')&&literal[i+1]!=c)++i;
            decoded+='\n';
        }else decoded+=c;
    }
    if(!decoded.empty()&&decoded.front()=='\n')decoded.erase(0,1);
    return decoded;
}
struct QueryLifetime {
    CivilopediaLayoutOwnerKey identity;
    std::weak_ptr<CivisCivilopediaCallbackOwner> owner;
    std::unique_ptr<content::RulesDatabase> database;
    std::string sql;
    std::optional<std::string> buildings_sql;
    const std::thread::id thread=std::this_thread::get_id();
    bool closed=false;
    void require_thread()const {
        if(std::this_thread::get_id()!=thread)throw runtime::ScriptError("Civis Units query requires its owner thread");
    }
    void require_current()const {
        require_thread();
        if(closed)throw runtime::ScriptError("Civis Units query owner is closed");
        const auto current=owner.lock();
        if(!current||!current->current(identity))throw runtime::ScriptError("Civis Units query context owner expired or became stale");
    }
};
bool hash(std::string_view text){return text.size()==64&&text.find_first_not_of("0123456789abcdefABCDEF")==text.npos;}
std::string pinned(const std::filesystem::path &path,std::string_view sha){
    const auto before=runtime::identify_file(path);
    if(before.bytes>64ULL*1024*1024||before.sha256!=sha)throw runtime::ScriptError("Civis Units query source identity changed");
    std::ifstream file(path,std::ios::binary);
    if(!file)throw runtime::ScriptError("Civis Units query source cannot be read");
    std::string text{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    if(runtime::identify_bytes({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}).sha256!=sha)
        throw runtime::ScriptError("Civis Units query source changed during read");
    return text;
}
}
struct CivisCivilopediaUnitsQuery::Impl {
    std::weak_ptr<QueryLifetime> lifetime;
    std::unique_ptr<content::RulesScriptCursor> cursor;
    std::uint64_t generation=0;
    bool closed=false,failed=false;
    std::shared_ptr<QueryLifetime> require_current()const {
        const auto current=lifetime.lock();
        if(!current)throw runtime::ScriptError("Civis Units query owner expired");
        current->require_current();
        if(closed||failed)throw runtime::ScriptError("Civis Units prepared query is closed or failed");
        return current;
    }
};
struct CivisCivilopediaUnitsQueryOwner::Impl {
    std::shared_ptr<QueryLifetime> lifetime;
    std::vector<std::weak_ptr<CivisCivilopediaUnitsQuery::Impl>> queries;
};
struct CivisCivilopediaUnitsIterator::Impl {
    std::shared_ptr<CivisCivilopediaUnitsQuery::Impl> query;
    std::uint64_t generation=0,rows=0;
    bool eof=false;
};
std::string CivisCivilopediaUnitsQueryOwner::declared_sql_from_source(std::string_view source){
    constexpr std::string_view start="CivilopediaCategory[CategoryUnits].PopulateList = function()";
    constexpr std::string_view finish="CivilopediaCategory[CategoryPromotions].PopulateList = function()";
    const auto begin=source.find(start),end=source.find(finish,begin);
    if(begin==source.npos||end==source.npos||source.find(start,begin+1)!=source.npos)
        throw runtime::ScriptError("Civis Units source block is missing or ambiguous");
    const auto block=source.substr(begin,end-begin);
    constexpr std::string_view prefix="local sql = [[";
    const auto declaration=block.find(prefix),close=block.find("]];",declaration);
    if(declaration==block.npos||close==block.npos||block.find(prefix,declaration+1)!=block.npos||
       block.find("local UnitsByEra = DB.CreateQuery(sql);",close)==block.npos||
       block.find("for unit in UnitsByEra(era.Type) do",close)==block.npos)
        throw runtime::ScriptError("Civis Units prepared query declaration/call changed");
    const auto literal=block.substr(declaration+prefix.size(),close-declaration-prefix.size());
    const auto decoded=decode_long_string(literal);
    if(decoded!=units_sql)throw runtime::ScriptError("Civis Units SQL differs from the sole declared read-only statement");
    return decoded;
}
std::string CivisCivilopediaUnitsQueryOwner::declared_buildings_sql_from_source(std::string_view source){
    constexpr std::string_view start="CivilopediaCategory[CategoryBuildings].PopulateList = function()";
    constexpr std::string_view finish="CivilopediaCategory[CategoryWonders].PopulateList = function()";
    const auto begin=source.find(start),end=source.find(finish,begin);
    if(begin==source.npos||end==source.npos||source.find(start,begin+1)!=source.npos)
        throw runtime::ScriptError("Civis Buildings source block is missing or ambiguous");
    const auto block=source.substr(begin,end-begin);
    constexpr std::string_view factory="local BuildingsByEra = DB.CreateQuery(sql);";
    constexpr std::string_view invoke="for building in BuildingsByEra(era.Type) do";
    const auto call=block.find(factory),iteration=block.find(invoke,call);
    if(call==block.npos||iteration==block.npos||block.find(factory,call+factory.size())!=block.npos||
       block.find(invoke,iteration+invoke.size())!=block.npos||
       block.find("DB.CreateQuery(")!=call+std::string_view("local BuildingsByEra = ").size()||
       block.find("DB.CreateQuery(",call+factory.size())!=block.npos)
        throw runtime::ScriptError("Civis Buildings prepared query declaration/call changed");
    // The later no-tech local sql belongs to DB.Query, not this factory.
    const auto declaration_block=block.substr(0,call);
    constexpr std::string_view prefix="local sql = [[";
    const auto declaration=declaration_block.find(prefix),close=declaration_block.find("]];",declaration);
    if(declaration==block.npos||close==block.npos||declaration_block.find(prefix,declaration+prefix.size())!=block.npos||
       declaration_block.substr(close+3).find_first_not_of(" \t\r\n")!=block.npos)
        throw runtime::ScriptError("Civis Buildings first long-string declaration is missing or ambiguous");
    const auto decoded=decode_long_string(declaration_block.substr(declaration+prefix.size(),close-declaration-prefix.size()));
    if(decoded!=buildings_sql)
        throw runtime::ScriptError("Civis Buildings SQL differs from its exact declared read-only statement");
    return decoded;
}
CivisCivilopediaUnitsQueryOwner::CivisCivilopediaUnitsQueryOwner(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
CivisCivilopediaUnitsQueryOwner::~CivisCivilopediaUnitsQueryOwner()=default;
std::shared_ptr<CivisCivilopediaUnitsQueryOwner> CivisCivilopediaUnitsQueryOwner::open(
    const std::filesystem::path &root,const SelectedBnwCivilopediaLayoutSources &sources,
    const content::RulesDatabase &rules,CivilopediaLayoutOwnerKey identity,
    const std::shared_ptr<CivisCivilopediaCallbackOwner> &owner){
    if(!sources.verified||!sources.control_tree.verified||!owner||!owner->current(identity)||
       identity.frame_number==0||identity.owner_token.empty()||identity.player_id<0||identity.game_turn<0||
       !hash(identity.payload_sha256)||!hash(identity.rules_sha256)||
       sources.civilopedia_sha256!=identity.civilopedia_sha256||sources.xml_sha256!=identity.xml_sha256||
       sources.styles_sha256!=identity.styles_sha256)
        throw runtime::ScriptError("Civis Units query requires matching selected source/context identity");
    auto impl=std::make_unique<Impl>();impl->lifetime=std::make_shared<QueryLifetime>();
    auto &life=*impl->lifetime;life.identity=std::move(identity);life.owner=owner;
    life.sql=declared_sql_from_source(pinned(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua",sources.civilopedia_sha256));
    life.database=rules.clone_gameplay();life.require_current();
    return std::shared_ptr<CivisCivilopediaUnitsQueryOwner>(new CivisCivilopediaUnitsQueryOwner(std::move(impl)));
}
std::shared_ptr<CivisCivilopediaUnitsQueryOwner> CivisCivilopediaUnitsQueryOwner::open_with_buildings(
    const std::filesystem::path &root,const SelectedBnwCivilopediaLayoutSources &sources,
    const content::RulesDatabase &rules,CivilopediaLayoutOwnerKey identity,
    const std::shared_ptr<CivisCivilopediaCallbackOwner> &current_owner){
    auto owner=open(root,sources,rules,std::move(identity),current_owner);
    auto &life=*owner->impl_->lifetime;life.require_current();
    life.buildings_sql=declared_buildings_sql_from_source(pinned(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua",sources.civilopedia_sha256));
    life.require_current();return owner;
}
std::string CivisCivilopediaUnitsQueryOwner::declared_buildings_sql()const {
    impl_->lifetime->require_current();
    if(!impl_->lifetime->buildings_sql)throw runtime::ScriptError("Civis Buildings statement was not requested");
    return *impl_->lifetime->buildings_sql;
}
std::string CivisCivilopediaUnitsQueryOwner::declared_sql()const {impl_->lifetime->require_current();return impl_->lifetime->sql;}
std::shared_ptr<CivisCivilopediaUnitsQuery> CivisCivilopediaUnitsQueryOwner::prepare(std::string_view sql){
    const auto life=impl_->lifetime;life->require_current();
    if(sql!=life->sql&&(!life->buildings_sql||sql!=*life->buildings_sql))
        throw runtime::ScriptError(life->buildings_sql?
            "Civis CreateQuery only allows the exact selected Units and Buildings statements":
            "Civis CreateQuery only allows the exact selected first Units statement");
    auto query=std::make_shared<CivisCivilopediaUnitsQuery::Impl>();query->lifetime=life;
    query->cursor=life->database->script_query(sql,{});
    if(!query->cursor)throw runtime::ScriptError("Civis Units query preparation failed against selected rules");
    std::erase_if(impl_->queries,[](const auto &entry){return entry.expired();});impl_->queries.push_back(query);
    return std::shared_ptr<CivisCivilopediaUnitsQuery>(new CivisCivilopediaUnitsQuery(std::move(query)));
}
void CivisCivilopediaUnitsQueryOwner::close(){
    impl_->lifetime->require_thread();impl_->lifetime->closed=true;
    for(const auto &weak:impl_->queries)if(const auto query=weak.lock()){query->closed=true;query->cursor->release();}
    impl_->lifetime->database.reset();
}
CivisCivilopediaUnitsQuery::CivisCivilopediaUnitsQuery(std::shared_ptr<Impl> impl):impl_(std::move(impl)){}
CivisCivilopediaUnitsQuery::~CivisCivilopediaUnitsQuery()=default;
std::shared_ptr<CivisCivilopediaUnitsIterator> CivisCivilopediaUnitsQuery::iterate(std::span<const content::DatabaseScriptValue> args){
    const auto life=impl_->require_current();
    if(args.size()!=1||!std::holds_alternative<std::string>(args[0])||std::get<std::string>(args[0]).find('\0')!=std::string::npos)
        throw runtime::ScriptError("Civis Units query expects exactly one string era without embedded NUL");
    if(impl_->generation==std::numeric_limits<std::uint64_t>::max())throw runtime::ScriptError("Civis Units iterator generation exhausted");
    auto iterator=std::make_unique<CivisCivilopediaUnitsIterator::Impl>();iterator->query=impl_;
    try {impl_->cursor->reset();impl_->cursor->bind(args);}
    catch(...){impl_->failed=true;impl_->cursor->release();throw;}
    iterator->generation=++impl_->generation;
    return std::shared_ptr<CivisCivilopediaUnitsIterator>(new CivisCivilopediaUnitsIterator(std::move(iterator)));
}
void CivisCivilopediaUnitsQuery::close(){
    const auto life=impl_->lifetime.lock();
    if(!life)throw runtime::ScriptError("Civis Units query owner expired");
    life->require_thread();impl_->closed=true;impl_->cursor->release();
}
CivisCivilopediaUnitsIterator::CivisCivilopediaUnitsIterator(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
CivisCivilopediaUnitsIterator::~CivisCivilopediaUnitsIterator()=default;
std::optional<content::DatabaseScriptRow> CivisCivilopediaUnitsIterator::next(){
    const auto life=impl_->query->require_current();
    if(impl_->generation!=impl_->query->generation)throw runtime::ScriptError("Civis Units iterator belongs to an earlier era invocation");
    if(impl_->eof)return {};
    try {
        auto row=impl_->query->cursor->next();
        if(!impl_->query->cursor->error().empty())throw runtime::ScriptError("Civis Units query step failed: "+std::string(impl_->query->cursor->error()));
        if(!row){impl_->eof=true;return {};}
        if(++impl_->rows>65536)throw runtime::ScriptError("Civis diagnostic Units query exceeds its 65536-row budget");
        return row;
    }catch(...){impl_->query->failed=true;impl_->query->cursor->release();throw;}
}
}
