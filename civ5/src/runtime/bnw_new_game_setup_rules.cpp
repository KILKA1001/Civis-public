#include "civ5/runtime/bnw_new_game_setup_rules.hpp"
#include "civ5/content/global_defines.hpp"
#include <limits>
namespace civ5::runtime {
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::int32_t int32(std::int64_t value){
    need(value>=std::numeric_limits<std::int32_t>::min()&&value<=std::numeric_limits<std::int32_t>::max(),
         "BNW setup rules value exceeds original signed GetInt range");return static_cast<std::int32_t>(value);
}
}
BnwNewGameSetupRules::BnwNewGameSetupRules(std::shared_ptr<const BnwCreationConfig> config)
    :config_(std::move(config)),content_(config_->borrow_activated_content_for_game_setup()),database_(&config_->game_rules()){}
std::shared_ptr<const BnwNewGameSetupRules> BnwNewGameSetupRules::from_creation_config(
    std::shared_ptr<const BnwCreationConfig> config){
    need(bool(config),"BNW gameplay setup requires real CreationConfig");config->require_current();
    auto owner=std::shared_ptr<BnwNewGameSetupRules>(new BnwNewGameSetupRules(std::move(config)));
    auto& data=owner->rules_;data.selected=owner->config_->metadata();
    owner->require_current();const auto& db=*owner->database_;
    const auto& speed=data.selected.info[3];const auto& era=data.selected.info[4];
    // Same unordered source SELECT used by CvGameSpeedInfo::CacheResults. The
    // captured row order is the real SQLite iteration order, never sorted.
    std::int32_t expected=-1;
    auto report=db.query_rows({"select count(*) from GameSpeed_Turns where GameSpeedType = ?",{speed.type}},
        [&](const content::RulesRowView& row){owner->require_current();expected=int32(row.integer_at(0));},1);
    need(report.all_succeeded()&&report.rows==1&&expected>=0,"BNW selected GameSpeed turn-count source failed");
    report=db.query_rows({"select * from GameSpeed_Turns where GameSpeedType = ?",{speed.type}},
        [&](const content::RulesRowView& row){owner->require_current();
            need(row.has_column("MonthIncrement")&&row.has_column("TurnsPerIncrement"),"BNW turn increment schema incomplete");
            data.turns.push_back({int32(row.integer("MonthIncrement")),int32(row.integer("TurnsPerIncrement"))});},65536);
    need(report.all_succeeded()&&report.rows==static_cast<std::uint64_t>(expected),"BNW selected GameSpeed turn slots are unwritten/incomplete");
    report=db.query_rows({"SELECT * FROM Eras WHERE ID = ? AND Type = ? LIMIT 1",{static_cast<std::int64_t>(era.id),era.type}},
        [&](const content::RulesRowView& row){owner->require_current();
            need(row.has_column("StartPercent")&&row.has_column("NoReligion"),"BNW selected Era gameplay fields unavailable");
            data.start_percent=int32(row.integer("StartPercent"));data.no_religion=row.boolean("NoReligion");},1);
    need(report.all_succeeded()&&report.rows==1,"BNW selected Era source row missing");
    // Sparse, genuinely prefetched victory slots. These are only the actual
    // gameplay init operands WinsGame/EndScore; no full CvVictoryInfo is claimed.
    const auto victories=db.prefetch_info("Victories",[&](std::uint64_t slot,std::int64_t authored,const content::RulesRowView& row){
        owner->require_current();need(slot<=65535&&authored==static_cast<std::int64_t>(slot)&&
            row.has_column("ID")&&row.integer("ID")==authored&&row.has_column("Type")&&
            row.has_column("WinsGame")&&row.has_column("EndScore"),"BNW victory source slot/schema unsupported");
        const auto type=row.text("Type");need(type&&!type->empty()&&type->find('\0')==std::string::npos,"BNW victory Type source missing");
        if(data.victories.size()<=slot)data.victories.resize(static_cast<std::size_t>(slot)+1);
        data.victories[static_cast<std::size_t>(slot)]=BnwNewGameVictoryInitRule{
            {"Victories",*type,int32(authored),content::saved_info_type_hash(*type)},row.boolean("WinsGame"),row.boolean("EndScore")};
        return true;
    },content::InfoCollectionBudget{65536});
    need(victories.all_succeeded()&&victories.slots<=65536&&data.victories.size()==victories.slots,
         "BNW actual victory initialization operands incomplete");
    const auto start=db.global_define("START_YEAR",content::GlobalDefineKind::integer);
    need(start.found&&start.error.empty()&&std::holds_alternative<std::int64_t>(start.value),"BNW genuine START_YEAR rule missing");
    data.start_year=int32(std::get<std::int64_t>(start.value));owner->require_current();return owner;
}
void BnwNewGameSetupRules::require_current()const{
    need(thread_==std::this_thread::get_id(),"BNW gameplay setup rules used on another thread");
    config_->require_current();const auto actual=config_->borrow_activated_content_for_game_setup();
    need(actual.get()==content_.get()&&!actual.owner_before(content_)&&!content_.owner_before(actual)&&
         actual->rules.get()==database_ && &config_->game_rules()==database_,"BNW gameplay setup lost SAME activated Content/rules/config");
}
void BnwNewGameSetupRules::require_same_config(const BnwCreationConfig& config)const{
    require_current();need(config_.get()==&config,"BNW gameplay setup belongs to another Game Config");
}
BnwNewGameSetupRuleObservation BnwNewGameSetupRules::observe()const{require_current();auto copy=rules_;require_current();return copy;}
}
