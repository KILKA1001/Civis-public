#include "civ5/runtime/bnw_deficit_rules.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool ok,const char *message) {
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    bool caught=false;
    try {action();} catch (const std::exception &) {caught=true;}
    check(caught,"Expected BNW deficit rules blocker");
}
} // namespace

int main() {
    try {
        content::RulesDatabase db;
        db.apply_sql("CREATE TABLE Domains(ID INTEGER,Type TEXT);"
            "INSERT INTO Domains VALUES(0,'DOMAIN_LAND'),(1,'DOMAIN_SEA'),"
            "(2,'DOMAIN_AIR');"
            "CREATE TABLE Units(ID INTEGER,Type TEXT,Domain TEXT,Combat INTEGER,"
            "RangedCombat INTEGER,Moves INTEGER,Suicide INTEGER,NukeDamageLevel INTEGER);"
            "INSERT INTO Units VALUES"
            "(0,'UNIT_LAND','DOMAIN_LAND',16,0,2,0,-1),"
            "(1,'UNIT_SEA','DOMAIN_SEA',0,16,4,0,-1),"
            "(2,'UNIT_SUICIDE','DOMAIN_AIR',16,0,1,1,1),"
            "(3,'UNIT_STILL','DOMAIN_LAND',16,0,0,0,-1);"
            "CREATE TABLE Terrains(ID INTEGER,Type TEXT);"
            "INSERT INTO Terrains VALUES(0,'TERRAIN_TEST');"
            "CREATE TABLE Features(ID INTEGER,Type TEXT);"
            "INSERT INTO Features VALUES(0,'FEATURE_TEST');"
            "CREATE TABLE UnitCombatInfos(ID INTEGER,Type TEXT);"
            "INSERT INTO UnitCombatInfos VALUES(0,'UNITCOMBAT_TEST');"
            "CREATE TABLE UnitClasses(ID INTEGER,Type TEXT);"
            "INSERT INTO UnitClasses VALUES(0,'UNITCLASS_TEST');"
            "CREATE TABLE UnitPromotions(ID INTEGER,Type TEXT,CityAttack INTEGER,"
            "AttackMod INTEGER,DefenseMod INTEGER,DropRange INTEGER,Blitz INTEGER,"
            "MustSetUpToRangedAttack INTEGER,OnlyDefensive INTEGER);"
            "INSERT INTO UnitPromotions VALUES"
            "(0,'PROMOTION_TEST',20,10,10,1,1,1,1),"
            "(1,'PROMOTION_SECOND',0,20,0,0,0,0,0),"
            "(2,'PROMOTION_DEFENSIVE',0,-20,0,0,0,0,1);"
            "CREATE TABLE UnitPromotions_Terrains(PromotionType TEXT,TerrainType TEXT,"
            "Attack INTEGER,Defense INTEGER);"
            "INSERT INTO UnitPromotions_Terrains VALUES"
            "('PROMOTION_TEST','TERRAIN_TEST',40,20);"
            "CREATE TABLE UnitPromotions_Features(PromotionType TEXT,FeatureType TEXT,"
            "Attack INTEGER,Defense INTEGER);"
            "INSERT INTO UnitPromotions_Features VALUES"
            "('PROMOTION_TEST','FEATURE_TEST',40,20);"
            "CREATE TABLE UnitPromotions_UnitCombatMods(PromotionType TEXT,"
            "UnitCombatType TEXT,Modifier INTEGER);"
            "INSERT INTO UnitPromotions_UnitCombatMods VALUES"
            "('PROMOTION_TEST','UNITCOMBAT_TEST',40);"
            "CREATE TABLE UnitPromotions_UnitClasses(PromotionType TEXT,"
            "UnitClassType TEXT,Modifier INTEGER,Attack INTEGER,Defense INTEGER);"
            "INSERT INTO UnitPromotions_UnitClasses VALUES"
            "('PROMOTION_TEST','UNITCLASS_TEST',40,20,20);"
            "CREATE TABLE UnitPromotions_Domains(PromotionType TEXT,DomainType TEXT,"
            "Modifier INTEGER);"
            "INSERT INTO UnitPromotions_Domains VALUES"
            "('PROMOTION_TEST','DOMAIN_LAND',40);"
            "CREATE TABLE Unit_FreePromotions(UnitType TEXT,PromotionType TEXT);"
            "CREATE TABLE Defines(Name TEXT,Value INTEGER);"
            "INSERT INTO Defines VALUES('MAX_HIT_POINTS',100),"
            "('UNIT_DELETE_DISABLED',0);");
        const auto types=db.cache_info_types();
        const auto defines=db.cache_global_defines(
            content::GlobalDefineProfile::bnw);
        const auto loaded=load_bnw_deficit_rules(db,types,defines);
        const auto hash=[](std::string_view type) {
            return content::saved_info_type_hash(type);
        };
        check(loaded.rules.max_hit_points==100 &&
              !loaded.rules.unit_delete_disabled &&
              loaded.rules.units.size()==4 &&
              loaded.rules.units.at(hash("UNIT_LAND")).domain==
                  BnwDeficitDomain::land &&
              loaded.rules.units.at(hash("UNIT_LAND")).intrinsic_power==64 &&
              loaded.rules.units.at(hash("UNIT_SEA")).domain==
                  BnwDeficitDomain::sea &&
              loaded.rules.units.at(hash("UNIT_SEA")).intrinsic_power==41 &&
              loaded.rules.units.at(hash("UNIT_SUICIDE")).intrinsic_power==4032 &&
              loaded.rules.units.at(hash("UNIT_STILL")).intrinsic_power==0,
              "BNW active SQL did not produce validated hash/domain/base power");
        saves::SavedSessionSnapshot world;
        world.game_state.layout=saves::GameStateLayout::expansion2_version1;
        auto &all=world.all_players.emplace();
        all.layout=saves::GameStateLayout::expansion2_version1;
        auto &owner=all.players.emplace_back();owner.saved_player_id=5;
        owner.units.layout=owner.cities.layout=
            saves::GameStateLayout::expansion2_version1;
        auto &unit=owner.units.units.emplace_back();
        unit.core.layout=saves::GameStateLayout::expansion2_version1;
        const auto field=[&](const char *name,std::int64_t value) {
            saves::UnitCoreField f{};f.name=name;f.values={value};
            unit.core.fields.push_back(std::move(f));
        };
        field("m_iID",1);field("m_eOwner",5);field("m_iX",0);
        field("m_iY",0);field("m_iBaseCombat",16);field("m_iMoves",60);
        field("m_iLevel",0);field("m_iDamage",0);
        field("idUnitType",hash("UNIT_LAND"));
        BnwDeficitMilitaryContext ai;
        ai.land_defense=ai.naval_defense=1;
        ai.plot_unit_fighting[{0,0}]=false;
        SimulationRandom rng(RandomAlgorithm::bnw,{0,0,0});
        const auto selected=choose_bnw_deficit_candidate(
            world,5,loaded.rules,ai,rng);
        check(selected.selected && selected.selected->unit_id==1 &&
              selected.selected->power==64 && rng.state().calls==1,
              "Loaded Unit Info did not bind saved hash to deficit candidate");

        db.apply_sql("INSERT INTO Unit_FreePromotions VALUES"
            "('UNIT_LAND','PROMOTION_SECOND'),"
            "('UNIT_LAND','PROMOTION_TEST'),"
            "('UNIT_SEA','PROMOTION_DEFENSIVE');");
        const auto promoted=load_bnw_deficit_rules(db,types,defines);
        check(promoted.rules.units.size()==4 &&
              promoted.rules.units.at(hash("UNIT_LAND")).intrinsic_power==126 &&
              promoted.rules.units.at(hash("UNIT_SEA")).intrinsic_power==41,
              "Free-promotion power omitted SDK modifier branches, ID order or ranged gate");
        rng=SimulationRandom(RandomAlgorithm::bnw,{0,0,0});
        const auto promoted_candidate=choose_bnw_deficit_candidate(
            world,5,promoted.rules,ai,rng);
        check(promoted_candidate.selected &&
              promoted_candidate.selected->power==126 && rng.state().calls==1,
              "Promoted SQL power did not reach saved-unit deficit selection");
        // Independent source example: 16^1.5=64; class40 is one eighth,
        // then /100, hence +3 (not the terrain/combat quarter-weight +6).
        auto class_only=db.clone_gameplay();
        class_only->apply_sql("DELETE FROM Unit_FreePromotions;"
            "INSERT INTO Unit_FreePromotions VALUES('UNIT_LAND','PROMOTION_SECOND');"
            "UPDATE UnitPromotions SET CityAttack=0,AttackMod=0,DefenseMod=0,"
            "DropRange=0,Blitz=0,MustSetUpToRangedAttack=0,OnlyDefensive=0;"
            "DELETE FROM UnitPromotions_Terrains;DELETE FROM UnitPromotions_Features;"
            "DELETE FROM UnitPromotions_UnitCombatMods;DELETE FROM UnitPromotions_Domains;"
            "DELETE FROM UnitPromotions_UnitClasses;"
            "INSERT INTO UnitPromotions_UnitClasses VALUES"
            "('PROMOTION_SECOND','UNITCLASS_TEST',40,0,0);");
        const auto class_power=load_bnw_deficit_rules(*class_only,types,defines);
        check(class_power.rules.units.at(hash("UNIT_LAND")).intrinsic_power==67,
              "SDK UnitClass general bonus must divide by eight before /100");
        db.apply_sql("INSERT INTO UnitPromotions_Terrains VALUES"
            "('PROMOTION_TEST','TERRAIN_TEST',40,20);");
        rejects([&]{(void)load_bnw_deficit_rules(db,types,defines);});
        db.apply_sql("DELETE FROM UnitPromotions_Terrains WHERE rowid IN "
            "(SELECT max(rowid) FROM UnitPromotions_Terrains);");
        db.apply_sql("UPDATE UnitPromotions SET AttackMod=2000000000 "
            "WHERE Type='PROMOTION_TEST';");
        rejects([&]{(void)load_bnw_deficit_rules(db,types,defines);});
        db.apply_sql("UPDATE UnitPromotions SET AttackMod=10 "
            "WHERE Type='PROMOTION_TEST';");
        db.apply_sql("UPDATE UnitPromotions_Domains SET DomainType="
            "'DOMAIN_UNKNOWN' WHERE PromotionType='PROMOTION_TEST';");
        rejects([&]{(void)load_bnw_deficit_rules(db,types,defines);});
        db.apply_sql("UPDATE UnitPromotions_Domains SET DomainType="
            "'DOMAIN_LAND' WHERE PromotionType='PROMOTION_TEST';");
        db.apply_sql("INSERT INTO Unit_FreePromotions VALUES"
            "('UNIT_SEA','UNKNOWN_PROMOTION');");
        rejects([&]{(void)load_bnw_deficit_rules(db,types,defines);});
        db.apply_sql("DELETE FROM Unit_FreePromotions WHERE"
            " PromotionType='UNKNOWN_PROMOTION';");
        db.apply_sql("UPDATE Units SET Domain='DOMAIN_UNKNOWN' WHERE ID=1;");
        rejects([&]{(void)load_bnw_deficit_rules(db,types,defines);});
        db.apply_sql("UPDATE Units SET Domain='DOMAIN_SEA' WHERE ID=1;");
        db.apply_sql("UPDATE Units SET Combat=2000000000 WHERE ID=0;");
        rejects([&]{(void)load_bnw_deficit_rules(db,types,defines);});
        db.apply_sql("UPDATE Units SET Combat=16 WHERE ID=0;");
        db.apply_sql("INSERT INTO Units VALUES"
            "(4,'UNIT_NEW','DOMAIN_LAND',1,0,1,0,-1);");
        rejects([&]{(void)load_bnw_deficit_rules(db,types,defines);});
        const auto fresh_types=db.cache_info_types();
        const auto fresh=load_bnw_deficit_rules(db,fresh_types,defines);
        check(fresh.rules.units.contains(hash("UNIT_NEW")),
              "Fresh BNW Info catalog could not load added unit");
        const auto vp_defines=db.cache_global_defines(
            content::GlobalDefineProfile::vp542);
        rejects([&]{(void)load_bnw_deficit_rules(db,fresh_types,vp_defines);});
        db.apply_sql("UPDATE Defines SET Value=0 WHERE Name='MAX_HIT_POINTS';");
        const auto zero_hit_points=db.cache_global_defines(
            content::GlobalDefineProfile::bnw);
        rejects([&]{(void)load_bnw_deficit_rules(
            db,fresh_types,zero_hit_points);});
        std::cout << "PASS BNW deficit rules loader\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW deficit rules loader: " << error.what() << '\n';
        return 1;
    }
}
