#include "civ5/runtime/bnw_deficit_scrap_rules.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool valid,const char *message) {
    if (!valid) throw std::runtime_error(message);
}
template<class F> void rejects(F run) {
    bool caught=false;
    try {run();} catch (const std::exception &) {caught=true;}
    check(caught,"Expected BNW scrap rules blocker");
}
SessionCheckpointIdentity identity(const content::RulesDatabase &db,
                                   const saves::LoadedSave &source) {
    return {identify_bytes(source.bytes).sha256,
            identify_bytes(db.gameplay_fingerprint_data()).sha256};
}
} // namespace

int main() {
    try {
        // Values cross-checked against local Expansion2 CIV5Units.xml,
        // CIV5GameSpeeds.xml and GlobalDefines.xml. The selected database is
        // deliberately small so stale catalogs and identity are testable.
        content::RulesDatabase db;
        db.apply_sql("CREATE TABLE Units(ID INTEGER,Type TEXT,Cost INTEGER);"
            "INSERT INTO Units VALUES"
            "(0,'UNIT_WARRIOR',40),(1,'UNIT_UNBUILDABLE',-1);"
            "CREATE TABLE GameSpeeds(ID INTEGER,Type TEXT,TrainPercent INTEGER);"
            "INSERT INTO GameSpeeds VALUES"
            "(0,'GAMESPEED_QUICK',67),(1,'GAMESPEED_STANDARD',100);"
            "CREATE TABLE Defines(Name TEXT,Value INTEGER);"
            "INSERT INTO Defines VALUES"
            "('DISBAND_UNIT_REFUND_PERCENT',25),"
            "('MAX_HIT_POINTS',100),('UNIT_DELETE_DISABLED',0);");
        const auto types=db.cache_info_types();
        const auto defines=db.cache_global_defines(
            content::GlobalDefineProfile::bnw);
        ResearchContext setup;setup.profile=content::TechnologyProfile::bnw;
        setup.speed=1;
        saves::LoadedSave source;source.bytes={1,2,3,4};
        auto stamp=identity(db,source);
        const auto loaded=load_bnw_deficit_scrap_rules(
            db,types,defines,setup,source,stamp);
        check(loaded.production_cost_by_unit_hash.size()==2 &&
              loaded.production_cost_by_unit_hash.at(
                  content::saved_info_type_hash("UNIT_WARRIOR"))==40 &&
              loaded.production_cost_by_unit_hash.at(
                  content::saved_info_type_hash("UNIT_UNBUILDABLE"))==-1 &&
              loaded.refund_percent==25 && loaded.train_percent==100 &&
              loaded.max_hit_points==100 && !loaded.unit_delete_disabled,
              "Selected BNW Units/Defines/GameSpeeds binding is incorrect");
        setup.speed=0;
        check(load_bnw_deficit_scrap_rules(db,types,defines,setup,source,stamp)
                  .train_percent==67,
              "Scrap train percent ignored the owned game speed ID");
        setup.speed=99;
        rejects([&]{(void)load_bnw_deficit_scrap_rules(
            db,types,defines,setup,source,stamp);});
        setup.speed=1;
        auto wrong_source=source;wrong_source.bytes.push_back(5);
        rejects([&]{(void)load_bnw_deficit_scrap_rules(
            db,types,defines,setup,wrong_source,stamp);});
        auto wrong_rules=stamp;wrong_rules.rules_sha256[0]='x';
        rejects([&]{(void)load_bnw_deficit_scrap_rules(
            db,types,defines,setup,source,wrong_rules);});
        auto vp_setup=setup;vp_setup.profile=content::TechnologyProfile::vp542;
        rejects([&]{(void)load_bnw_deficit_scrap_rules(
            db,types,defines,vp_setup,source,stamp);});
        const auto vp_defines=db.cache_global_defines(
            content::GlobalDefineProfile::vp542);
        rejects([&]{(void)load_bnw_deficit_scrap_rules(
            db,types,vp_defines,setup,source,stamp);});

        db.apply_sql("UPDATE GameSpeeds SET Type='GAMESPEED_CHANGED' "
            "WHERE ID=1;");
        stamp=identity(db,source);
        rejects([&]{(void)load_bnw_deficit_scrap_rules(
            db,types,defines,setup,source,stamp);});
        db.apply_sql("UPDATE GameSpeeds SET Type='GAMESPEED_STANDARD' "
            "WHERE ID=1;");
        stamp=identity(db,source);

        db.apply_sql("UPDATE Units SET Type='UNIT_CHANGED' WHERE ID=0;");
        stamp=identity(db,source);
        rejects([&]{(void)load_bnw_deficit_scrap_rules(
            db,types,defines,setup,source,stamp);});
        const auto fresh_types=db.cache_info_types();
        check(load_bnw_deficit_scrap_rules(
            db,fresh_types,defines,setup,source,stamp)
                  .production_cost_by_unit_hash.contains(
                      content::saved_info_type_hash("UNIT_CHANGED")),
              "Fresh BNW catalog failed after a valid Units change");
        db.apply_sql("UPDATE Defines SET Value=50 WHERE "
            "Name='DISBAND_UNIT_REFUND_PERCENT';");
        stamp=identity(db,source);
        rejects([&]{(void)load_bnw_deficit_scrap_rules(
            db,fresh_types,defines,setup,source,stamp);});
        const auto fresh_defines=db.cache_global_defines(
            content::GlobalDefineProfile::bnw);
        check(load_bnw_deficit_scrap_rules(
            db,fresh_types,fresh_defines,setup,source,stamp).refund_percent==50,
              "Fresh BNW define cache did not bind selected refund value");
        db.apply_sql("UPDATE GameSpeeds SET TrainPercent=2147483648 WHERE ID=1;");
        stamp=identity(db,source);
        rejects([&]{(void)load_bnw_deficit_scrap_rules(
            db,fresh_types,fresh_defines,setup,source,stamp);});
        std::cout << "PASS BNW deficit scrap rules binding\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW deficit scrap rules binding: "
                  << error.what() << '\n';
        return 1;
    }
}
